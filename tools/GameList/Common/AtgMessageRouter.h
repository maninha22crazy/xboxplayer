//--------------------------------------------------------------------------------------
// AtgMessageRouter.h
//
// Message router that routes messages to message-type specific consumers
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once
#include "AtgMessages.h"
#include "AtgTasks.h"
#include <vector>
#include <map>

namespace ATG
{

using namespace std;

typedef VOID ( *PFNMESSAGECONSUMERCALLBACK ) ( DWORD dwMessageId, VOID* pMsg, VOID* pConsumer );

typedef struct _MessageConsumerCallbackData
{
    PFNMESSAGECONSUMERCALLBACK pfnCallback;
    VOID* pConsumer;   
} MessageConsumerCallbackData;

typedef vector< MessageConsumerCallbackData* > MessageConsumersVector;
typedef MessageConsumersVector::const_iterator MessageConsumersVectorCIter;
typedef MessageConsumersVector::iterator MessageConsumersVectorIter;

typedef map< DWORD/*MSG_AREA*/, MessageConsumersVector* > MessageConsumersMap;
typedef MessageConsumersMap::iterator MessageConsumersMapIter;
typedef MessageConsumersMap::const_iterator MessageConsumersMapCIter;
typedef pair< DWORD, MessageConsumersVector* > MSG_CONSUMER_PAIR;

template <class T>
class MessageRouter
{
public:
    MessageRouter();
    ~MessageRouter();
    BOOL Initialize();
    VOID Dispatch();
    VOID Send( const TaskData& taskData );
    VOID SendImmediate( const TaskData& taskData );
    VOID RegisterCallback( DWORD msgArea, 
                           PFNMESSAGECONSUMERCALLBACK pfnCallback, 
                           VOID* pConsumer,
                           BOOL fCanOnlyRegisterOnce = false );
    VOID UnregisterCallback( VOID* pConsumer );

private:
    MessageConsumersVector* GetMessageConsumersByArea( DWORD msgArea );
    BOOL IsRegistered( VOID* pConsumer, MessageConsumersVector* pvConsumers );

private:
    MessageConsumersMap     m_mapMsgDestinations;
    T                       m_taskQueue;                // Taskqueue for current session
};

}