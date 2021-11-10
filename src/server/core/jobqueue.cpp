/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2021 Raden Solutions
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
#include <nxcore_jobs.h>

/**
 * Constructor
 */
ServerJobQueue::ServerJobQueue(uint32_t objectId) : m_accessMutex(MutexType::FAST)
{
	m_jobCount = 0;
	m_jobList = nullptr;
	m_objectId = objectId;
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
	MemFree(m_jobList);
}

/**
 * Add job
 */
void ServerJobQueue::add(ServerJob *job)
{
	m_accessMutex.lock();
	m_jobList = MemReallocArray(m_jobList, m_jobCount + 1);
	m_jobList[m_jobCount++] = job;
	job->setOwningQueue(this);
	m_accessMutex.unlock();

	DbgPrintf(4, _T("Job %d added to queue (node=%d, type=%s, description=\"%s\")"),
	          job->getId(), job->getObjectId(), job->getType(), job->getDescription());

	runNext();
}

/**
 * Run next job if possible
 */
void ServerJobQueue::runNext()
{
	int i;

	m_accessMutex.lock();
	for(i = 0; i < m_jobCount; i++)
		if ((m_jobList[i]->getStatus() != JOB_ON_HOLD) &&
			 ((m_jobList[i]->getStatus() != JOB_FAILED) || m_jobList[i]->isBlockNextJobsOnFailure()))
			break;
	if ((i < m_jobCount) && (m_jobList[i]->getStatus() == JOB_PENDING))
		m_jobList[i]->start();
	m_accessMutex.unlock();
}

/**
 * Handler for job completion
 */
void ServerJobQueue::jobCompleted(ServerJob *job)
{
	int i;

	m_accessMutex.lock();
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
	m_accessMutex.unlock();

	runNext();
}

/**
 * Cancel job
 */
bool ServerJobQueue::cancel(UINT32 jobId)
{
	int i;
	bool success = false;

	m_accessMutex.lock();
	for(i = 0; i < m_jobCount; i++)
		if (m_jobList[i]->getId()  == jobId)
		{
			if (m_jobList[i]->cancel())
			{
				nxlog_debug(4, _T("Job %d cancelled (node=%d, type=%s, description=\"%s\")"),
							   m_jobList[i]->getId(), m_jobList[i]->getObjectId(), m_jobList[i]->getType(), m_jobList[i]->getDescription());

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
	m_accessMutex.unlock();

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

	m_accessMutex.lock();
	for(i = 0; i < m_jobCount; i++)
		if (m_jobList[i]->getId()  == jobId)
		{
			if (m_jobList[i]->hold())
			{
				nxlog_debug(4, _T("Job %d put on hold (node=%d, type=%s, description=\"%s\")"),
							   m_jobList[i]->getId(), m_jobList[i]->getObjectId(), m_jobList[i]->getType(), m_jobList[i]->getDescription());

				success = true;
			}
			break;
		}
	m_accessMutex.unlock();

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

	m_accessMutex.lock();
	for(i = 0; i < m_jobCount; i++)
		if (m_jobList[i]->getId()  == jobId)
		{
			if (m_jobList[i]->unhold())
			{
				nxlog_debug(4, _T("Job %d unhold (node=%d, type=%s, description=\"%s\")"),
							   m_jobList[i]->getId(), m_jobList[i]->getObjectId(), m_jobList[i]->getType(), m_jobList[i]->getDescription());

				success = true;
			}
			break;
		}
	m_accessMutex.unlock();

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

	m_accessMutex.lock();
	now = time(NULL);
	for(i = 0; i < m_jobCount; i++)
	{
		if (m_jobList[i]->getStatus() == JOB_FAILED)
		{
			int delay = m_jobList[i]->getAutoCancelDelay();
			if ((delay > 0) && (now - m_jobList[i]->getLastStatusChange() >= delay))
			{
				nxlog_debug(4, _T("Failed job %d deleted from queue (node=%d, type=%s, description=\"%s\")"),
							   m_jobList[i]->getId(), m_jobList[i]->getObjectId(), m_jobList[i]->getType(), m_jobList[i]->getDescription());

				// Delete and remove from list
				m_jobList[i]->cancel();
				delete m_jobList[i];
				m_jobCount--;
				memmove(&m_jobList[i], &m_jobList[i + 1], sizeof(ServerJob *) * (m_jobCount - i));
			}
		}
	}
	m_accessMutex.unlock();

	runNext();
}

/**
 * Find job by ID
 */
ServerJob *ServerJobQueue::findJob(UINT32 jobId)
{
	ServerJob *job = NULL;

	m_accessMutex.lock();
	for(int i = 0; i < m_jobCount; i++)
		if (m_jobList[i]->getId()  == jobId)
		{
			job = m_jobList[i];
			break;
		}
	m_accessMutex.unlock();

	return job;
}

/**
 * Fill NXCP message with jobs' information
 * Increments base variable id; returns number of jobs added to message
 */
UINT32 ServerJobQueue::fillMessage(NXCPMessage *msg, UINT32 *varIdBase) const
{
	UINT32 id = *varIdBase;
	int i;

	m_accessMutex.lock();
	for(i = 0; i < m_jobCount; i++, id += 2)
	{
		msg->setField(id++, m_jobList[i]->getId());
		msg->setField(id++, m_jobList[i]->getType());
		msg->setField(id++, m_jobList[i]->getDescription());
		msg->setField(id++, m_jobList[i]->getObjectId());
		msg->setField(id++, (WORD)m_jobList[i]->getStatus());
		msg->setField(id++, (WORD)m_jobList[i]->getProgress());
		msg->setField(id++, m_jobList[i]->getFailureMessage());
		msg->setField(id++, m_jobList[i]->getUserId());
	}
	m_accessMutex.unlock();
	*varIdBase = id;
	return i;
}

/**
 * Get number of jobs in job queue
 */
int ServerJobQueue::getJobCount(const TCHAR *type) const
{
	int count;
	m_accessMutex.lock();
	if (type == nullptr)
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
	m_accessMutex.unlock();
	return count;
}
