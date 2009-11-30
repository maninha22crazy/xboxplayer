//--------------------------------------------------------------------------------------
// AtgTasks.h
//
// Common definitions used by TaskQueue class
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once
#include "AtgUtil.h"

namespace ATG
{

//-----------------------------------------------------------------------------
// Name: struct TaskData
// Desc: Structure for holding task data, plus some comparison operators
//-----------------------------------------------------------------------------
typedef struct _TaskData
{
    DWORD dwMessage;
    VOID* pMessage;
    VOID* pMsgDestination;

    // Assignment operator
    _TaskData& operator = ( const _TaskData& ref )
    {
        dwMessage = ref.dwMessage; 
        pMessage = ref.pMessage;
        pMsgDestination = ref.pMsgDestination;

        return  *this;
    }

    // Use dwMessage for comparison operand
    bool operator < ( _TaskData& ref )
    {
        return dwMessage < ref.dwMessage;
    }

    // Use dwMessage for comparison operand
    bool operator > ( _TaskData& ref )
    {
        return dwMessage > ref.dwMessage;
    }

    #ifdef _DEBUG
    void Dump() const
    {
        ATG::DebugSpew( "dwMessage: %d; pMessage: %p; pMsgDestination: %p\n", 
                        dwMessage, pMessage, pMsgDestination);       
    }
    #endif

} TaskData;

#define MAX_SEM_COUNT 65535

//-----------------------------------------------------------------------------
// Name: class CritSec
// Desc: Bare-bones critical section class
//-----------------------------------------------------------------------------
class CritSec
{	
public:
	CritSec() { InitializeCriticalSection( &m_cs );	}
	~CritSec() { DeleteCriticalSection( &m_cs ); }
	inline void Lock() { EnterCriticalSection( &m_cs ); }
	inline void Unlock() { LeaveCriticalSection( &m_cs ); }				
private:
	CRITICAL_SECTION m_cs;
};

}