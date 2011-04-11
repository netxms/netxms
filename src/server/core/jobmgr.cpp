/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2009 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: jobmgr.cpp
**
**/

#include "nxcore.h"


//
// Job operation codes
//

#define CANCEL_JOB		0
#define HOLD_JOB			1
#define UNHOLD_JOB		2


//
// Add job
//

bool NXCORE_EXPORTABLE AddJob(ServerJob *job)
{
	bool success = false;

	NetObj *object = FindObjectById(job->getRemoteNode());
	if ((object != NULL) && (object->Type() == OBJECT_NODE))
	{
		ServerJobQueue *queue = ((Node *)object)->getJobQueue();
		queue->add(job);
		success = true;
	}
	return success;
}


//
// Get job list
//

void GetJobList(CSCPMessage *msg)
{
	DWORD i, id, count;

   RWLockReadLock(g_rwlockIdIndex, INFINITE);
   for(i = 0, id = VID_JOB_LIST_BASE, count = 0; i < g_dwIdIndexSize; i++)
   {
      if (((NetObj *)g_pIndexById[i].pObject)->Type() == OBJECT_NODE)
      {
			ServerJobQueue *queue = ((Node *)g_pIndexById[i].pObject)->getJobQueue();
			count += queue->fillMessage(msg, &id);
      }
   }
   RWLockUnlock(g_rwlockIdIndex);
	msg->SetVariable(VID_JOB_COUNT, count);
}


//
// Implementatoin for job status changing operations: cancel, hold, unhold
//

static DWORD ChangeJobStatus(DWORD userId, CSCPMessage *msg, int operation)
{
	DWORD i, jobId, rcc = RCC_INVALID_JOB_ID;

	jobId = msg->GetVariableLong(VID_JOB_ID);

   RWLockReadLock(g_rwlockIdIndex, INFINITE);
   for(i = 0; i < g_dwIdIndexSize; i++)
   {
      if (((NetObj *)g_pIndexById[i].pObject)->Type() == OBJECT_NODE)
      {
			ServerJobQueue *queue = ((Node *)g_pIndexById[i].pObject)->getJobQueue();
			if (queue->findJob(jobId) != NULL)
			{
				if (((NetObj *)g_pIndexById[i].pObject)->CheckAccessRights(userId, OBJECT_ACCESS_CONTROL))
				{
					switch(operation)
					{
						case CANCEL_JOB:
							rcc = queue->cancel(jobId) ? RCC_SUCCESS : RCC_JOB_CANCEL_FAILED;
							break;
						case HOLD_JOB:
							rcc = queue->hold(jobId) ? RCC_SUCCESS : RCC_JOB_HOLD_FAILED;
							break;
						case UNHOLD_JOB:
							rcc = queue->unhold(jobId) ? RCC_SUCCESS : RCC_JOB_UNHOLD_FAILED;
							break;
						default:
							rcc = RCC_INTERNAL_ERROR;
							break;
					}
				}
				else
				{
					rcc = RCC_ACCESS_DENIED;
				}
			}
      }
   }
   RWLockUnlock(g_rwlockIdIndex);
	return rcc;
}


//
// Cancel job
//

DWORD NXCORE_EXPORTABLE CancelJob(DWORD userId, CSCPMessage *msg)
{
	return ChangeJobStatus(userId, msg, CANCEL_JOB);
}


//
// Hold job
//

DWORD NXCORE_EXPORTABLE HoldJob(DWORD userId, CSCPMessage *msg)
{
	return ChangeJobStatus(userId, msg, HOLD_JOB);
}


//
// Unhold job
//

DWORD NXCORE_EXPORTABLE UnholdJob(DWORD userId, CSCPMessage *msg)
{
	return ChangeJobStatus(userId, msg, UNHOLD_JOB);
}


//
// Job manager worker thread
//

THREAD_RESULT THREAD_CALL JobManagerThread(void *arg)
{
	DbgPrintf(2, _T("Job Manager worker thread started"));

	while(!SleepAndCheckForShutdown(10))
	{
		DbgPrintf(7, _T("Job Manager: checking queues"));

		RWLockReadLock(g_rwlockIdIndex, INFINITE);
		for(DWORD i = 0; i < g_dwIdIndexSize; i++)
		{
			if (((NetObj *)g_pIndexById[i].pObject)->Type() == OBJECT_NODE)
			{
				ServerJobQueue *queue = ((Node *)g_pIndexById[i].pObject)->getJobQueue();
				queue->cleanup();
			}
		}
		RWLockUnlock(g_rwlockIdIndex);
	}

	DbgPrintf(2, _T("Job Manager worker thread stopped"));
	return THREAD_OK;
}
