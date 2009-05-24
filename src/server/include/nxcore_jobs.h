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
** File: nxcore_jobs.h
**
**/

#ifndef _nxcore_jobs_h_
#define _nxcore_jobs_h_


//
// Job status
//

enum ServerJobStatus
{
	JOB_PENDING = 0,
	JOB_ACTIVE,
	JOB_ON_HOLD,
	JOB_COMPLETED,
	JOB_FAILED,
	JOB_CANCELLED,
	JOB_CANCEL_PENDING
};


//
// Job class
//

class ServerJobQueue;
class NetObj;

class NXCORE_EXPORTABLE ServerJob
{
private:
	DWORD m_id;
	TCHAR *m_type;
	DWORD m_remoteNode;
	TCHAR *m_description;
	ServerJobStatus m_status;
	int m_progress;
	TCHAR *m_failureMessage;
	THREAD m_workerThread;
	ServerJobQueue *m_owningQueue;
	time_t m_lastStatusChange;
	int m_autoCancelDelay;	// Interval in seconds to cancel failed job automatically (0 = disabled)
	time_t m_lastNotification;
	NetObj *m_resolvedObject;
	MUTEX m_notificationLock;
	CSCPMessage m_notificationMessage;

	static DWORD s_freeId;
	static THREAD_RESULT THREAD_CALL WorkerThreadStarter(void *);
	static void sendNotification(ClientSession *session, void *arg);

protected:
	virtual bool run();
	virtual bool onCancel();

	void notifyClients(bool isStatusChange);
	void changeStatus(ServerJobStatus newStatus);
	void markProgress(int pctCompleted);
	void setFailureMessage(const TCHAR *msg);

	void setDescription(const TCHAR *description);

public:
	ServerJob(const TCHAR *type, const TCHAR *description, DWORD node);
	virtual ~ServerJob();

	void start();
	bool cancel();

	void setAutoCancelDelay(int delay) { m_autoCancelDelay = delay; }
	int getAutoCancelDelay() { return m_autoCancelDelay; }

	DWORD getId() { return m_id; }
	const TCHAR *getType() { return m_type; }
	const TCHAR *getDescription() { return m_description; }
	ServerJobStatus getStatus() { return m_status; }
	int getProgress() { return m_progress; }
	DWORD getRemoteNode() { return m_remoteNode; }
	const TCHAR *getFailureMessage() { return CHECK_NULL(m_failureMessage); }
	time_t getLastStatusChange() { return m_lastStatusChange; }

	void setOwningQueue(ServerJobQueue *queue);

	void fillMessage(CSCPMessage *msg);
};


//
// Job queue class
//

class ServerJobQueue
{
private:
	int m_jobCount;
	ServerJob **m_jobList;
	MUTEX m_accessMutex;

public:
	ServerJobQueue();
	~ServerJobQueue();

	void add(ServerJob *job);
	bool cancel(DWORD jobId);
	void runNext();

	ServerJob *findJob(DWORD jobId);

	void jobCompleted(ServerJob *job);

	DWORD fillMessage(CSCPMessage *msg, DWORD *varIdBase);
};


//
// Job manager API
//

bool NXCORE_EXPORTABLE AddJob(ServerJob *job);
void GetJobList(CSCPMessage *msg);
DWORD CancelJob(DWORD userId, CSCPMessage *msg);

#endif   /* _nxcore_jobs_h_ */
