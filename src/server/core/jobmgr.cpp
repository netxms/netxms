/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 Victor Kirhenshtein
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
// Static data
//

static ObjectIndex s_jobNodes;


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
		s_jobNodes.put(job->getId(), object);
		success = true;
	}
	return success;
}


//
// Unregister job from job manager
//

void UnregisterJob(UINT32 jobId)
{
	s_jobNodes.remove(jobId);
}


//
// Get job list
//

struct __job_callback_data
{
	CSCPMessage *msg;
	UINT32 jobCount;
	UINT32 baseId;
};

static void JobListCallback(NetObj *object, void *data)
{
	struct __job_callback_data *jcb = (struct __job_callback_data *)data;
	ServerJobQueue *queue = ((Node *)object)->getJobQueue();
	jcb->jobCount += queue->fillMessage(jcb->msg, &jcb->baseId);
}

void GetJobList(CSCPMessage *msg)
{
	struct __job_callback_data jcb;

	jcb.msg = msg;
	jcb.jobCount = 0;
	jcb.baseId = VID_JOB_LIST_BASE;
	g_idxNodeById.forEach(JobListCallback, &jcb);
	msg->SetVariable(VID_JOB_COUNT, jcb.jobCount);
}


//
// Implementatoin for job status changing operations: cancel, hold, unhold
//

static UINT32 ChangeJobStatus(UINT32 userId, CSCPMessage *msg, int operation)
{
	UINT32 rcc = RCC_INVALID_JOB_ID;
	UINT32 jobId = msg->GetVariableLong(VID_JOB_ID);
	Node *node = (Node *)s_jobNodes.get(jobId);
	if (node != NULL)
	{
		ServerJobQueue *queue = node->getJobQueue();
		if (queue->findJob(jobId) != NULL)
		{
			if (node->checkAccessRights(userId, OBJECT_ACCESS_CONTROL))
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
		else
		{
			// Remove stalled record in job_id -> node mapping
			s_jobNodes.remove(jobId);
		}
	}
	return rcc;
}


//
// Cancel job
//

UINT32 NXCORE_EXPORTABLE CancelJob(UINT32 userId, CSCPMessage *msg)
{
	return ChangeJobStatus(userId, msg, CANCEL_JOB);
}


//
// Hold job
//

UINT32 NXCORE_EXPORTABLE HoldJob(UINT32 userId, CSCPMessage *msg)
{
	return ChangeJobStatus(userId, msg, HOLD_JOB);
}


//
// Unhold job
//

UINT32 NXCORE_EXPORTABLE UnholdJob(UINT32 userId, CSCPMessage *msg)
{
	return ChangeJobStatus(userId, msg, UNHOLD_JOB);
}


//
// Job manager worker thread
//

static void CleanupJobQueue(NetObj *object, void *data)
{
	ServerJobQueue *queue = ((Node *)object)->getJobQueue();
	queue->cleanup();
}

THREAD_RESULT THREAD_CALL JobManagerThread(void *arg)
{
	DbgPrintf(2, _T("Job Manager worker thread started"));

	while(!SleepAndCheckForShutdown(10))
	{
		DbgPrintf(7, _T("Job Manager: checking queues"));
		g_idxNodeById.forEach(CleanupJobQueue, NULL);
	}

	DbgPrintf(2, _T("Job Manager worker thread stopped"));
	return THREAD_OK;
}
