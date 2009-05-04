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
	JOB_FAILED
};


//
// Job class
//

class ServerJobQueue;

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

	static DWORD s_freeId;
	static THREAD_RESULT THREAD_CALL WorkerThreadStarter(void *);

protected:
	virtual bool run();
	virtual bool onCancel();

	void markProgress(int pctCompleted);
	void setFailureMessage(const TCHAR *msg);

public:
	ServerJob(const TCHAR *type, const TCHAR *description, DWORD node);
	virtual ~ServerJob();

	void start();
	bool cancel();

	DWORD getId() { return m_id; }
	const TCHAR *getType() { return m_type; }
	const TCHAR *getDescription() { return m_description; }
	ServerJobStatus getStatus() { return m_status; }
	int getProgress() { return m_progress; }
	DWORD getRemoteNode() { return m_remoteNode; }
	const TCHAR *getFailureMessage() { return CHECK_NULL(m_failureMessage); }

	void setOwningQueue(ServerJobQueue *queue);
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
	void runNext();

	void jobCompleted(ServerJob *job);

	DWORD fillMessage(CSCPMessage *msg, DWORD *varIdBase);
};


//
// Job manager API
//

bool NXCORE_EXPORTABLE AddJob(ServerJob *job);
void GetJobList(CSCPMessage *msg);


#endif   /* _nxcore_jobs_h_ */
