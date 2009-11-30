//--------------------------------------------------------------------------------------
// AtgSyncTaskQueue.h
//
// Synchronous task-queue implementation
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once
#include <algorithm>
#include "AtgTasks.h"
#include <queue>
#include <vector>

namespace ATG
{

//------------------------------------------------------------------------
// Name: class TaskQueue
// Desc: Single threaded task queue class that uses a AtgPriorityQueue
//       for its queue
//------------------------------------------------------------------------
class TaskQueue
{
public:
    TaskQueue() 
	{
	}

	~TaskQueue()
	{
	}

    //------------------------------------------------------------------------
    // Name: Push()
    // Desc: Pushes a task to the queue
    //------------------------------------------------------------------------
    void Push(const TaskData& t)
    {
        // Add T to queue;
        ATG::DebugSpew( "TaskQueue: Pushing 0x%08x...\n", t.dwMessage );
        m_queue.push(t);
    }

    //------------------------------------------------------------------------
    // Name: Push()
    // Desc: Pops a task from the queue
    //------------------------------------------------------------------------
    bool Pop(TaskData* t_out)
    {
        bool fRet = false;

        if( m_queue.size() )
        {
            // Get front of queue            
            const TaskData& t = m_queue.front();

            // Pop off front of queue
            m_queue.pop();

            // Copy data to in/out param
            *t_out = t;

            ATG::DebugSpew( "TaskQueue: Popping 0x%08x...\n", t.dwMessage );
            
            fRet = true;
        }

        return fRet;
    }

    //------------------------------------------------------------------------
    // Name: IsEmpty()
    // Desc: Returns true if queue is empty; otherwise false
    //------------------------------------------------------------------------
    bool IsEmpty()
    {
        bool fEmpty = true;

        if( m_queue.size() )
        {
            fEmpty = false;
        }

        return fEmpty;
    }

    //------------------------------------------------------------------------
    // Name: Initialize()
    // Desc: Public task queue initialization function
    //------------------------------------------------------------------------
    DWORD Initialize()
    {
    	return ERROR_SUCCESS;
    }

private:
    std::queue<TaskData> m_queue;
};

}
