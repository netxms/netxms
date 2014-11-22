/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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
** File: jobqueue.cpp
**
**/

#include "nxcore.h"

/**
 * Constructor
 */
ServerJobQueue::ServerJobQueue()
{
	m_jobCount = 0;
	m_jobList = NULL;
	m_accessMutex = MutexCreate();
}

/**
 * Destructor
 */
ServerJobQueue::~ServerJobQueue()
{
	int i;

	for(i = 0; i < m_jobCount; i++)
	{
		m_jobList[i]->cancel();
		delete m_jobList[i];
	}
	safe_free(m_jobList);

	MutexDestroy(m_accessMutex);
}

/**
 * Add job
 */
void ServerJobQueue::add(ServerJob *job)
{
	MutexLock(m_accessMutex);
	m_jobList = (ServerJob **)realloc(m_jobList, sizeof(ServerJob *) * (m_jobCount + 1));
	m_jobList[m_jobCount] = job;
	m_jobCount++;
	job->setOwningQueue(this);
	MutexUnlock(m_accessMutex);

	DbgPrintf(4, _T("Job %d added to queue (node=%d, type=%s, description=\"%s\")"),
	          job->getId(), job->getRemoteNode(), job->getType(), job->getDescription());

	runNext();
}

/**
 * Run next job if possible
 */
void ServerJobQueue::runNext()
{
	int i;

	MutexLock(m_accessMutex);
	for(i = 0; i < m_jobCount; i++)
		if ((m_jobList[i]->getStatus() != JOB_ON_HOLD) &&
			 ((m_jobList[i]->getStatus() != JOB_FAILED) || m_jobList[i]->isBlockNextJobsOnFailure()))
			break;
	if ((i < m_jobCount) && (m_jobList[i]->getStatus() == JOB_PENDING))
		m_jobList[i]->start();
	MutexUnlock(m_accessMutex);
}

/**
 * Handler for job completion
 */
void ServerJobQueue::jobCompleted(ServerJob *job)
{
	int i;

	MutexLock(m_accessMutex);
	for(i = 0; i < m_jobCount; i++)
		if (m_jobList[i] == job)
		{
			if ((job->getStatus() == JOB_COMPLETED) ||
				 (job->getStatus() == JOB_CANCELLED))
			{
				// Delete and remove from list
				delete job;
				m_jobCount--;
				memmove(&m_jobList[i], &m_jobList[i + 1], sizeof(ServerJob *) * (m_jobCount - i));
			}
			break;
		}
	MutexUnlock(m_accessMutex);

	runNext();
}

/**
 * Cancel job
 */
bool ServerJobQueue::cancel(UINT32 jobId)
{
	int i;
	bool success = false;

	MutexLock(m_accessMutex);
	for(i = 0; i < m_jobCount; i++)
		if (m_jobList[i]->getId()  == jobId)
		{
			if (m_jobList[i]->cancel())
			{
				DbgPrintf(4, _T("Job %d cancelled (node=%d, type=%s, description=\"%s\")"),
							 m_jobList[i]->getId(), m_jobList[i]->getRemoteNode(), m_jobList[i]->getType(), m_jobList[i]->getDescription());

				if (m_jobList[i]->getStatus() != JOB_CANCEL_PENDING)
				{
					// Delete and remove from list
					delete m_jobList[i];
					m_jobCount--;
					memmove(&m_jobList[i], &m_jobList[i + 1], sizeof(ServerJob *) * (m_jobCount - i));
				}
				success = true;
			}
			break;
		}
	MutexUnlock(m_accessMutex);

	runNext();
	return success;
}

/**
 * Hold job
 */
bool ServerJobQueue::hold(UINT32 jobId)
{
	int i;
	bool success = false;

	MutexLock(m_accessMutex);
	for(i = 0; i < m_jobCount; i++)
		if (m_jobList[i]->getId()  == jobId)
		{
			if (m_jobList[i]->hold())
			{
				DbgPrintf(4, _T("Job %d put on hold (node=%d, type=%s, description=\"%s\")"),
							 m_jobList[i]->getId(), m_jobList[i]->getRemoteNode(), m_jobList[i]->getType(), m_jobList[i]->getDescription());

				success = true;
			}
			break;
		}
	MutexUnlock(m_accessMutex);

	runNext();
	return success;
}

/**
 * Unhold job
 */
bool ServerJobQueue::unhold(UINT32 jobId)
{
	int i;
	bool success = false;

	MutexLock(m_accessMutex);
	for(i = 0; i < m_jobCount; i++)
		if (m_jobList[i]->getId()  == jobId)
		{
			if (m_jobList[i]->unhold())
			{
				DbgPrintf(4, _T("Job %d unhold (node=%d, type=%s, description=\"%s\")"),
							 m_jobList[i]->getId(), m_jobList[i]->getRemoteNode(), m_jobList[i]->getType(), m_jobList[i]->getDescription());

				success = true;
			}
			break;
		}
	MutexUnlock(m_accessMutex);

	runNext();
	return success;
}

/**
 * Clean up queue
 */
void ServerJobQueue::cleanup()
{
	int i;
	time_t now;

	MutexLock(m_accessMutex);
	now = time(NULL);
	for(i = 0; i < m_jobCount; i++)
	{
		if (m_jobList[i]->getStatus() == JOB_FAILED)
		{
			int delay = m_jobList[i]->getAutoCancelDelay();
			if ((delay > 0) && (now - m_jobList[i]->getLastStatusChange() >= delay))
			{
				DbgPrintf(4, _T("Failed job %d deleted from queue (node=%d, type=%s, description=\"%s\")"),
							 m_jobList[i]->getId(), m_jobList[i]->getRemoteNode(), m_jobList[i]->getType(), m_jobList[i]->getDescription());

				// Delete and remove from list
				m_jobList[i]->cancel();
				delete m_jobList[i];
				m_jobCount--;
				memmove(&m_jobList[i], &m_jobList[i + 1], sizeof(ServerJob *) * (m_jobCount - i));
			}
		}
	}
	MutexUnlock(m_accessMutex);

	runNext();
}

/**
 * Find job by ID
 */
ServerJob *ServerJobQueue::findJob(UINT32 jobId)
{
	ServerJob *job = NULL;

	MutexLock(m_accessMutex);
	for(int i = 0; i < m_jobCount; i++)
		if (m_jobList[i]->getId()  == jobId)
		{
			job = m_jobList[i];
			break;
		}
	MutexUnlock(m_accessMutex);

	return job;
}

/**
 * Fill NXCP message with jobs' information
 * Increments base variable id; returns number of jobs added to message
 */
UINT32 ServerJobQueue::fillMessage(NXCPMessage *msg, UINT32 *varIdBase)
{
	UINT32 id = *varIdBase;
	int i;

	MutexLock(m_accessMutex);
	for(i = 0; i < m_jobCount; i++, id += 2)
	{
		msg->setField(id++, m_jobList[i]->getId());
		msg->setField(id++, m_jobList[i]->getType());
		msg->setField(id++, m_jobList[i]->getDescription());
		msg->setField(id++, m_jobList[i]->getRemoteNode());
		msg->setField(id++, (WORD)m_jobList[i]->getStatus());
		msg->setField(id++, (WORD)m_jobList[i]->getProgress());
		msg->setField(id++, m_jobList[i]->getFailureMessage());
		msg->setField(id++, m_jobList[i]->getUserId());
	}
	MutexUnlock(m_accessMutex);
	*varIdBase = id;
	return i;
}

/**
 * Get number of jobs in job queue
 */
int ServerJobQueue::getJobCount(const TCHAR *type)
{
	int count;
	MutexLock(m_accessMutex);
	if (type == NULL)
	{
		count = m_jobCount;
	}
	else
	{
		count = 0;
		for(int i = 0; i < m_jobCount; i++)
			if (!_tcscmp(m_jobList[i]->getType(), type))
			{
				count++;
			}
	}
	MutexUnlock(m_accessMutex);
	return count;
}
