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

void UnregisterJob(UINT32 jobId);


//
// Constructor
//

ServerJob::ServerJob(const TCHAR *type, const TCHAR *description, UINT32 node, UINT32 userId, bool createOnHold)
{
	m_id = CreateUniqueId(IDG_JOB);
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
	createHistoryRecord();
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
	safe_free(m_failureMessage);
	MutexDestroy(m_notificationLock);
}


//
// Send notification to clients
//

void ServerJob::sendNotification(ClientSession *session, void *arg)
{
	ServerJob *job = (ServerJob *)arg;
	if (job->m_resolvedObject->checkAccessRights(session->getUserId(), OBJECT_ACCESS_READ))
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

	MutexLock(m_notificationLock);
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
	job->updateHistoryRecord(true);

	if (job->run())
	{
		job->changeStatus(JOB_COMPLETED);
	}
	else
	{
		if (job->m_status == JOB_CANCEL_PENDING)
			job->changeStatus(JOB_CANCELLED);
		else
			job->changeStatus(JOB_FAILED);
	}
	job->m_workerThread = INVALID_THREAD_HANDLE;

	DbgPrintf(4, _T("Job %d finished, status=%s"), job->m_id, (job->m_status == JOB_COMPLETED) ? _T("COMPLETED") : ((job->m_status == JOB_CANCELLED) ? _T("CANCELLED") : _T("FAILED")));
	job->updateHistoryRecord(false);

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

/**
 * Create record in job history table
 */
void ServerJob::createHistoryRecord()
{
	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	
	DB_STATEMENT hStmt = DBPrepare(hdb, 
		_T("INSERT INTO job_history (id,time_created,time_started,time_finished,job_type,")
		_T("description,node_id,user_id,status) VALUES (?,?,0,0,?,?,?,?,?)"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
		DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (UINT32)time(NULL));
		DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_type, DB_BIND_STATIC);
		DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, CHECK_NULL_EX(m_description), DB_BIND_STATIC);
		DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_remoteNode);
		DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_userId);
		DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, (LONG)m_status);
		DBExecute(hStmt);
		DBFreeStatement(hStmt);
	}
	DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Update job history record
 */
void ServerJob::updateHistoryRecord(bool onStart)
{
	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	
	DB_STATEMENT hStmt = DBPrepare(hdb, 
		onStart ? 
			_T("UPDATE job_history SET time_started=?,status=?,description=?,additional_info=? WHERE id=?") : 
			_T("UPDATE job_history SET time_finished=?,status=?,description=?,additional_info=?,failure_message=? WHERE id=?"));
	
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (UINT32)time(NULL));
		DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (LONG)m_status);
		DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, CHECK_NULL_EX(m_description), DB_BIND_STATIC);
		DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, getAdditionalInfo(), DB_BIND_TRANSIENT);
		if (onStart)
		{
			DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_id);
		}
		else
		{
			DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, CHECK_NULL_EX(m_failureMessage), DB_BIND_STATIC);
			DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_id);
		}
		DBExecute(hStmt);
		DBFreeStatement(hStmt);
	}
	DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Get additional info for logging
 */
const TCHAR *ServerJob::getAdditionalInfo()
{
	return _T("");
}
