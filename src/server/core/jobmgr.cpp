/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Raden Solutions
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

#define DEBUG_TAG _T("job.manager")

/**
 * Job operation codes
 */
#define CANCEL_JOB		0
#define HOLD_JOB			1
#define UNHOLD_JOB		2

/**
 * Static data
 */
static AbstractIndexWithDestructor<ServerJobQueue> s_jobQueues(Ownership::True);
static AbstractIndex<ServerJobQueue> s_jobQueueIndex(Ownership::False);
static Mutex s_indexLock;

/**
 * Add job
 */
bool NXCORE_EXPORTABLE AddJob(ServerJob *job)
{
	bool success = false;
	if (job->isValid())
	{
	   s_indexLock.lock();
		ServerJobQueue *queue = s_jobQueues.get(job->getObjectId());
		if (queue == nullptr)
		{
		   queue = new ServerJobQueue(job->getObjectId());
		   s_jobQueues.put(job->getObjectId(), queue);
		}
		s_indexLock.unlock();
		queue->add(job);
		s_jobQueueIndex.put(job->getId(), queue);
		success = true;
	}
	return success;
}

/**
 * Unregister job from job manager
 */
void UnregisterJob(UINT32 jobId)
{
   s_jobQueueIndex.remove(jobId);
}

/**
 * Data for job enumeration callback
 */
struct __job_callback_data
{
	NXCPMessage *msg;
	uint32_t jobCount;
	uint32_t baseId;
};

/**
 * Callback for job enumeration
 */
static void JobListCallback(ServerJobQueue *queue, void *data)
{
	struct __job_callback_data *jcb = (struct __job_callback_data *)data;
	jcb->jobCount += queue->fillMessage(jcb->msg, &jcb->baseId);
}

/**
 * Get list of all jobs
 */
void GetJobList(NXCPMessage *msg)
{
	struct __job_callback_data jcb;

	jcb.msg = msg;
	jcb.jobCount = 0;
	jcb.baseId = VID_JOB_LIST_BASE;
	s_jobQueues.forEach(JobListCallback, &jcb);
	msg->setField(VID_JOB_COUNT, jcb.jobCount);
}

/**
 * Implementation for job status changing operations: cancel, hold, unhold
 */
static UINT32 ChangeJobStatus(UINT32 userId, NXCPMessage *msg, int operation)
{
	UINT32 rcc = RCC_INVALID_JOB_ID;
	UINT32 jobId = msg->getFieldAsUInt32(VID_JOB_ID);
   ServerJobQueue *queue = s_jobQueueIndex.get(jobId);
   if (queue->findJob(jobId) != nullptr)
   {
      shared_ptr<NetObj> object = FindObjectById(queue->getObjectId());
      if ((object != nullptr) && object->checkAccessRights(userId, OBJECT_ACCESS_CONTROL))
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
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Processed job status change request (userId=%u, operation=%d, jobId=%u, rcc=%u)"),
                  userId, operation, jobId, rcc);
      }
      else
      {
         rcc = RCC_ACCESS_DENIED;
      }
   }
   else
   {
      // Remove stalled record in job_id -> node mapping
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Removing stalled job to queue mapping for job ID %u"), jobId);
      s_jobQueueIndex.remove(jobId);
   }
	return rcc;
}

/**
 * Cancel job
 */
UINT32 NXCORE_EXPORTABLE CancelJob(UINT32 userId, NXCPMessage *msg)
{
	return ChangeJobStatus(userId, msg, CANCEL_JOB);
}

/**
 * Hold job
 */
UINT32 NXCORE_EXPORTABLE HoldJob(UINT32 userId, NXCPMessage *msg)
{
	return ChangeJobStatus(userId, msg, HOLD_JOB);
}

/**
 * Unhold job
 */
UINT32 NXCORE_EXPORTABLE UnholdJob(UINT32 userId, NXCPMessage *msg)
{
	return ChangeJobStatus(userId, msg, UNHOLD_JOB);
}

/**
 * Cleanup job queue
 */
static void CleanupJobQueue(ServerJobQueue *queue, void *data)
{
	queue->cleanup();
}

/**
 * Job manager worker thread
 */
THREAD_RESULT THREAD_CALL JobManagerThread(void *arg)
{
   ThreadSetName("JobManager");
	nxlog_debug_tag(DEBUG_TAG, 2, _T("Worker thread started"));

	while(!SleepAndCheckForShutdown(10))
	{
	   nxlog_debug_tag(DEBUG_TAG, 7, _T("Checking queues"));
	   s_jobQueues.forEach(CleanupJobQueue, NULL);
	}

	nxlog_debug_tag(DEBUG_TAG, 2, _T("Worker thread stopped"));
	return THREAD_OK;
}
