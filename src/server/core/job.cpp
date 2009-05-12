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
** File: job.cpp
**
**/

#include "nxcore.h"


//
// Static members
//

DWORD ServerJob::s_freeId = 1;


//
// Constructor
//

ServerJob::ServerJob(const TCHAR *type, const TCHAR *description, DWORD node)
{
	m_id = s_freeId++;
	m_type = _tcsdup(CHECK_NULL(type));
	m_description = _tcsdup(CHECK_NULL(description));
	m_status = JOB_PENDING;
	m_remoteNode = node;
	m_progress = 0;
	m_failureMessage = NULL;
	m_owningQueue = NULL;
	m_workerThread = INVALID_THREAD_HANDLE;
}


//
// Destructor
//

ServerJob::~ServerJob()
{
	ThreadJoin(m_workerThread);

	safe_free(m_type);
	safe_free(m_description);
}


//
// Set owning queue
//

void ServerJob::setOwningQueue(ServerJobQueue *queue)
{
	m_owningQueue = queue;
}


//
// Update progress
//

void ServerJob::markProgress(int pctCompleted)
{
	if ((pctCompleted > m_progress) && (pctCompleted <= 100))
		m_progress = pctCompleted;
}


//
// Worker thread starter
//

THREAD_RESULT THREAD_CALL ServerJob::WorkerThreadStarter(void *arg)
{
	ServerJob *job = (ServerJob *)arg;
	DbgPrintf(4, _T("Job %d started"), job->m_id);

	if (job->run())
	{
		job->m_status = JOB_COMPLETED;
	}
	else
	{
		job->m_status = JOB_FAILED;
	}
	job->m_workerThread = INVALID_THREAD_HANDLE;

	DbgPrintf(4, _T("Job %d finished, status=%s"), job->m_id, (job->m_status == JOB_COMPLETED) ? _T("COMPLETED") : _T("FAILED"));

	if (job->m_owningQueue != NULL)
		job->m_owningQueue->jobCompleted(job);
	return THREAD_OK;
}


//
// Start job
//

void ServerJob::start()
{
	m_status = JOB_ACTIVE;
	m_workerThread = ThreadCreateEx(WorkerThreadStarter, 0, this);
}


//
// Cancel job
//

bool ServerJob::cancel()
{
	switch(m_status)
	{
		case JOB_COMPLETED:
			return false;
		case JOB_ACTIVE:
			return onCancel();
		default:
			m_status = JOB_CANCELLED;
			return true;
	}
}


//
// Default run (empty)
//

bool ServerJob::run()
{
	return true;
}


//
// Default cancel handler
//

bool ServerJob::onCancel()
{
	return false;
}


//
// Set failure message
//

void ServerJob::setFailureMessage(const TCHAR *msg)
{
	safe_free(m_failureMessage);
	m_failureMessage = (msg != NULL) ? _tcsdup(msg) : NULL;
}
