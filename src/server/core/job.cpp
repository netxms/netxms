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
** File: job.cpp
**
**/

#include "nxcore.h"


//
// Externals
//

void UnregisterJob(DWORD jobId);


//
// Static members
//

DWORD ServerJob::s_freeId = 1;


//
// Constructor
//

ServerJob::ServerJob(const TCHAR *type, const TCHAR *description, DWORD node, DWORD userId, bool createOnHold)
{
	m_id = s_freeId++;
	m_userId = userId;
	m_type = _tcsdup(CHECK_NULL(type));
	m_description = _tcsdup(CHECK_NULL(description));
	m_status = createOnHold ? JOB_ON_HOLD : JOB_PENDING;
	m_lastStatusChange = time(NULL);
	m_autoCancelDelay = 0;
	m_remoteNode = node;
	m_resolvedObject = FindObjectById(m_remoteNode);
	m_progress = 0;
	m_failureMessage = NULL;
	m_owningQueue = NULL;
	m_workerThread = INVALID_THREAD_HANDLE;
	m_lastNotification = 0;
	m_notificationLock = MutexCreate();
	m_blockNextJobsOnFailure = false;
}


//
// Destructor
//

ServerJob::~ServerJob()
{
	UnregisterJob(m_id);

	ThreadJoin(m_workerThread);

	safe_free(m_type);
	safe_free(m_description);
	MutexDestroy(m_notificationLock);
}


//
// Send notification to clients
//

void ServerJob::sendNotification(ClientSession *session, void *arg)
{
	ServerJob *job = (ServerJob *)arg;
	if (job->m_resolvedObject->CheckAccessRights(session->getUserId(), OBJECT_ACCESS_READ))
		session->postMessage(&job->m_notificationMessage);
}


//
// Notify clients
//

void ServerJob::notifyClients(bool isStatusChange)
{
	if (m_resolvedObject == NULL)
		return;

	time_t t = time(NULL);
	if (!isStatusChange && (t - m_lastNotification < 3))
		return;	// Don't send progress notifications often then every 3 seconds
	m_lastNotification = t;

	MutexLock(m_notificationLock, INFINITE);
	m_notificationMessage.SetCode(CMD_JOB_CHANGE_NOTIFICATION);
	fillMessage(&m_notificationMessage);
	EnumerateClientSessions(ServerJob::sendNotification, this);
	MutexUnlock(m_notificationLock);
}


//
// Change status
//

void ServerJob::changeStatus(ServerJobStatus newStatus)
{
	m_status = newStatus;
	m_lastStatusChange = time(NULL);
	notifyClients(true);
}


//
// Set owning queue
//

void ServerJob::setOwningQueue(ServerJobQueue *queue)
{
	m_owningQueue = queue;
	notifyClients(true);
}


//
// Update progress
//

void ServerJob::markProgress(int pctCompleted)
{
	if ((pctCompleted > m_progress) && (pctCompleted <= 100))
	{
		m_progress = pctCompleted;
		notifyClients(false);
	}
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
		job->changeStatus(JOB_COMPLETED);
	}
	else
	{
		if (job->m_status != JOB_CANCEL_PENDING)
			job->changeStatus(JOB_FAILED);
	}
	job->m_workerThread = INVALID_THREAD_HANDLE;

	DbgPrintf(4, _T("Job %d finished, status=%s"), job->m_id, (job->m_status == JOB_COMPLETED) ? _T("COMPLETED") : ((job->m_status == JOB_CANCEL_PENDING) ? _T("CANCELLED") : _T("FAILED")));

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
		case JOB_CANCEL_PENDING:
			return false;
		case JOB_ACTIVE:
			if (!onCancel())
				return false;
			changeStatus(JOB_CANCEL_PENDING);
			return true;
		default:
			changeStatus(JOB_CANCELLED);
			return true;
	}
}


//
// Hold job
//

bool ServerJob::hold()
{
	if (m_status == JOB_PENDING)
	{
		changeStatus(JOB_ON_HOLD);
		return true;
	}
	return false;
}


//
// Unhold job
//

bool ServerJob::unhold()
{
	if (m_status == JOB_ON_HOLD)
	{
		changeStatus(JOB_PENDING);
		return true;
	}
	return false;
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


//
// Set description
//

void ServerJob::setDescription(const TCHAR *description)
{ 
	safe_free(m_description);
	m_description = _tcsdup(description); 
}


//
// Fill NXCP message with job's data
//

void ServerJob::fillMessage(CSCPMessage *msg)
{
	msg->SetVariable(VID_JOB_ID, m_id);
	msg->SetVariable(VID_USER_ID, m_userId);
	msg->SetVariable(VID_JOB_TYPE, m_type);
	msg->SetVariable(VID_OBJECT_ID, m_remoteNode);
	msg->SetVariable(VID_DESCRIPTION, CHECK_NULL_EX(m_description));
	msg->SetVariable(VID_JOB_STATUS, (WORD)m_status);
	msg->SetVariable(VID_JOB_PROGRESS, (WORD)m_progress);
	if (m_status == JOB_FAILED)
		msg->SetVariable(VID_FAILURE_MESSAGE, (m_failureMessage != NULL) ? m_failureMessage : _T("Internal error"));
	else
		msg->SetVariable(VID_FAILURE_MESSAGE, CHECK_NULL_EX(m_failureMessage));
}
