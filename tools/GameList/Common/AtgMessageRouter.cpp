//--------------------------------------------------------------------------------------
// AtgMessageRouter.cpp
//
// Message router that routes messages to message-type specific consumers
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "stdafx.h"
#include "AtgMessageRouter.h"

namespace ATG
{

template <class T>
MessageRouter<T>::MessageRouter()
{
    m_mapMsgDestinations = MessageConsumersMap();
}

template <class T>
MessageRouter<T>::~MessageRouter()
{    
    MessageConsumersMapCIter citer;
    MessageConsumersVector* pvConsumers = NULL;
    
    for ( citer = m_mapMsgDestinations.begin(); citer != m_mapMsgDestinations.end(); citer++ )
    {
        pvConsumers = citer->second;
        delete pvConsumers;
    }    
}

template <class T>
BOOL MessageRouter<T>::Initialize()
{
    return TRUE;
}

template <class T>
VOID MessageRouter<T>::Send( const TaskData& taskData )
{
    m_taskQueue.Push( taskData );    
}

template <class T>
VOID MessageRouter<T>::SendImmediate( const TaskData& taskData )
{
    MessageConsumerCallbackData* pdata = NULL;
    MessageConsumersVectorCIter citer;

    //
    // Send message to area-specific consumers
    //
    MessageConsumersVector* pv = GetMessageConsumersByArea( MSG_AREA( taskData.dwMessage ) ); 
    if( !pv )
    {
        return;
    }
 
    // Note: We have to use index-based referencing
    // of our vector elements here instead of
    // iterator semantics because the callback
    // can result in elements of pv being removed
    // in the for loop!
    for ( size_t i=0; i < pv->size(); i++ )
    {
        pdata = pv->at(i);
        VOID* pConsumer = pdata->pConsumer;
        
        // If pMsg->pMsgDestination == pConsumer, 
        // this is a point-to-point message, and we
        // don't enumerate further
        if( taskData.pMsgDestination )
        {
            if( taskData.pMsgDestination == pConsumer )
            {
                pdata->pfnCallback( taskData.dwMessage, taskData.pMessage, pConsumer );
                return;
            }
            else
            {
                continue;
            }        
        }
        else
        {
            pdata->pfnCallback( taskData.dwMessage, taskData.pMessage, pConsumer );
        }
    }
}

template <class T>
VOID MessageRouter<T>::Dispatch()
{
    TaskData td;

    if( !m_taskQueue.Pop( &td ) )
    {
        return;
    }
    
    //
    // Send message to area-specific consumers
    //
    MessageConsumersVector* pv = GetMessageConsumersByArea( MSG_AREA( td.dwMessage ) ); 
    if( !pv )
    {
        return;
    }
 
    MessageConsumerCallbackData* pdata = NULL;
    MessageConsumersVectorCIter citer;
    for ( citer = pv->begin(); citer != pv->end(); citer++ )
    {
        pdata = (*citer);
        VOID* pConsumer = pdata->pConsumer;
        
        // If pMsg->pMsgDestination == pConsumer, 
        // this is a point-to-point message, and we
        // don't enumerate further
        if( td.pMsgDestination )
        {
            if( td.pMsgDestination == pConsumer )
            {
                pdata->pfnCallback( td.dwMessage, td.pMessage, pConsumer );
                return;
            }
            else
            {
                continue;
            }        
        }
        else
        {
            pdata->pfnCallback( td.dwMessage, td.pMessage, pConsumer );
        }
    }            
}

template <class T>
MessageConsumersVector* MessageRouter<T>::GetMessageConsumersByArea( DWORD msgArea )
{
    MessageConsumersMapCIter citer;

    citer = m_mapMsgDestinations.find( msgArea );
    if( citer == m_mapMsgDestinations.end() )
    {
        return NULL;
    }
    
    return citer->second;
}

template <class T>
VOID MessageRouter<T>::RegisterCallback( DWORD msgArea, 
                                             PFNMESSAGECONSUMERCALLBACK pfnCallback,
                                             VOID* pConsumer,
                                             BOOL fCanOnlyRegisterOnce )
{
    MessageConsumersMapCIter citer;
    MessageConsumersVector* pvConsumers = NULL;
    
    citer = m_mapMsgDestinations.find( msgArea );
    if( citer != m_mapMsgDestinations.end() )
    {
        pvConsumers = citer->second;

        // Check if multiple registrations are allowed for this consumer
        // for this msgArea
        if( fCanOnlyRegisterOnce && IsRegistered( pConsumer, pvConsumers ) )
        {
            return;
        }
    }
    else
    {
        pvConsumers = new MessageConsumersVector();
        m_mapMsgDestinations.insert( MSG_CONSUMER_PAIR( msgArea, pvConsumers ) );        
    }
    
    MessageConsumerCallbackData* pdata = new MessageConsumerCallbackData();
    pdata->pfnCallback = pfnCallback;
    pdata->pConsumer = pConsumer; 
    pvConsumers->push_back( pdata );
}

template <class T>
BOOL MessageRouter<T>::IsRegistered( VOID* pConsumer, MessageConsumersVector* pvConsumers )
{
    MessageConsumersVectorCIter cIter;
    MessageConsumerCallbackData* pdata = NULL;
    
    for ( cIter = pvConsumers->begin(); cIter != pvConsumers->end(); cIter++ )
    {
        pdata = (*cIter);
        if( pConsumer == pdata->pConsumer )
        {
            return true;            
        }
    }
    return false;
}


template <class T>
VOID MessageRouter<T>::UnregisterCallback( VOID* pConsumer )
{
    MessageConsumersMapCIter citer;
    MessageConsumersVector* pvConsumers = NULL;
    
    MessageConsumersVectorIter viter;
    MessageConsumerCallbackData* pdata = NULL;
    BOOL fFound = false;

    //
    // Remove pConsumer from all message areas it was registered for
    //    
    for ( citer = m_mapMsgDestinations.begin(); citer != m_mapMsgDestinations.end(); citer++ )
    {
        pvConsumers = citer->second;
        
        for ( viter = pvConsumers->begin(); viter != pvConsumers->end(); viter++ )
        {
            pdata = (*viter);
            if( pConsumer == pdata->pConsumer )
            {
                fFound = true;
                break;            
            }            
        }
        
        if( fFound )
        {
            pvConsumers->erase( viter );
            fFound = false;
        }        
    }    
}

}