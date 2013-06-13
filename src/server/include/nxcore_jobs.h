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

/**
 * Job status
 */
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

/**
 * Job class
 */
class ServerJobQueue;
class NetObj;

class NXCORE_EXPORTABLE ServerJob
{
private:
	UINT32 m_id;
	UINT32 m_userId;
	TCHAR *m_type;
	UINT32 m_remoteNode;
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
	bool m_blockNextJobsOnFailure;

	static THREAD_RESULT THREAD_CALL WorkerThreadStarter(void *);
	static void sendNotification(ClientSession *session, void *arg);

	void createHistoryRecord();
	void updateHistoryRecord(bool onStart);

protected:
	virtual bool run();
	virtual bool onCancel();
	virtual const TCHAR *getAdditionalInfo();

	void notifyClients(bool isStatusChange);
	void changeStatus(ServerJobStatus newStatus);
	void markProgress(int pctCompleted);
	void setFailureMessage(const TCHAR *msg);

	void setDescription(const TCHAR *description);

public:
	ServerJob(const TCHAR *type, const TCHAR *description, UINT32 node, UINT32 userId, bool createOnHold);
	virtual ~ServerJob();

	void start();
	bool cancel();
	bool hold();
	bool unhold();

	void setAutoCancelDelay(int delay) { m_autoCancelDelay = delay; }
	int getAutoCancelDelay() { return m_autoCancelDelay; }

	void setBlockNextJobsOnFailure(bool flag) { m_blockNextJobsOnFailure = flag; }
	bool isBlockNextJobsOnFailure() { return m_blockNextJobsOnFailure; }

	UINT32 getId() { return m_id; }
	UINT32 getUserId() { return m_userId; }
	const TCHAR *getType() { return m_type; }
	const TCHAR *getDescription() { return m_description; }
	ServerJobStatus getStatus() { return m_status; }
	int getProgress() { return m_progress; }
	UINT32 getRemoteNode() { return m_remoteNode; }
	const TCHAR *getFailureMessage() { return CHECK_NULL_EX(m_failureMessage); }
	time_t getLastStatusChange() { return m_lastStatusChange; }

	void setOwningQueue(ServerJobQueue *queue);

	void fillMessage(CSCPMessage *msg);
};

/**
 * Job queue class
 */
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
	bool cancel(UINT32 jobId);
	bool hold(UINT32 jobId);
	bool unhold(UINT32 jobId);
	void runNext();
	void cleanup();

	ServerJob *findJob(UINT32 jobId);
	int getJobCount(const TCHAR *type = NULL);

	void jobCompleted(ServerJob *job);

	UINT32 fillMessage(CSCPMessage *msg, UINT32 *varIdBase);
};

/**
 * Job manager API
 */
bool NXCORE_EXPORTABLE AddJob(ServerJob *job);
void GetJobList(CSCPMessage *msg);
UINT32 NXCORE_EXPORTABLE CancelJob(UINT32 userId, CSCPMessage *msg);
UINT32 NXCORE_EXPORTABLE HoldJob(UINT32 userId, CSCPMessage *msg);
UINT32 NXCORE_EXPORTABLE UnholdJob(UINT32 userId, CSCPMessage *msg);

/**
 * File upload job
 */
class FileUploadJob : public ServerJob
{
protected:
	static int m_activeJobs;
	static int m_maxActiveJobs;
	static MUTEX m_sharedDataMutex;

	Node *m_node;
	TCHAR *m_localFile;
	TCHAR *m_remoteFile;
	TCHAR *m_info;
	INT64 m_fileSize;

	virtual bool run();
	virtual const TCHAR *getAdditionalInfo();
	static void uploadCallback(INT64 size, void *arg);

public:
	static void init();

	FileUploadJob(Node *node, const TCHAR *localFile, const TCHAR *remoteFile, UINT32 userId, bool createOnHold);
	virtual ~FileUploadJob();
};

/**
 * File download job
 */
class FileDownloadJob : public ServerJob
{
private:
	Node *m_node;
	ClientSession *m_session;
	UINT32 m_requestId;
	TCHAR *m_localFile;
	TCHAR *m_remoteFile;
	TCHAR *m_info;
	INT64 m_fileSize;
	SOCKET m_socket;

protected:
	virtual bool run();
	virtual bool onCancel();
	virtual const TCHAR *getAdditionalInfo();

	static void progressCallback(size_t size, void *arg);

public:
	FileDownloadJob(Node *node, const TCHAR *remoteName, ClientSession *session, UINT32 requestId);
	virtual ~FileDownloadJob();

	static TCHAR *buildServerFileName(UINT32 nodeId, const TCHAR *remoteFile, TCHAR *buffer, size_t bufferSize);
};

/**
 * Agent policy deployment job
 */
class AgentPolicy;

class PolicyDeploymentJob : public ServerJob
{
protected:
	Node *m_node;
	AgentPolicy *m_policy;

	virtual bool run();

public:
	PolicyDeploymentJob(Node *node, AgentPolicy *policy, UINT32 userId);
	virtual ~PolicyDeploymentJob();
};


//
// Agent policy uninstall job
//

class PolicyUninstallJob : public ServerJob
{
protected:
	Node *m_node;
	AgentPolicy *m_policy;

	virtual bool run();

public:
	PolicyUninstallJob(Node *node, AgentPolicy *policy, UINT32 userId);
	virtual ~PolicyUninstallJob();
};


#endif   /* _nxcore_jobs_h_ */
