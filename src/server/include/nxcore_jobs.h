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

#include "nxcore_schedule.h"

#define JOB_RESCHEDULE_OFFSET 600

#define MAX_JOB_NAME_LEN      128

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
 * Job status
 */
enum ServerJobResult
{
	JOB_RESULT_SUCCESS = 0,
	JOB_RESULT_FAILED,
	JOB_RESULT_RESCHEDULE
};

class ServerJobQueue;
class NetObj;

/**
 * Job class
 */
class NXCORE_EXPORTABLE ServerJob
{
private:
	UINT32 m_id;
	UINT32 m_userId;
	TCHAR m_type[MAX_JOB_NAME_LEN];
	UINT32 m_nodeId;
   Node *m_node;
	TCHAR m_description[MAX_DB_STRING];
	ServerJobStatus m_status;
	int m_progress;
	TCHAR *m_failureMessage;
	THREAD m_workerThread;
	ServerJobQueue *m_owningQueue;
	time_t m_lastStatusChange;
	int m_autoCancelDelay;	// Interval in seconds to cancel failed job automatically (0 = disabled)
	time_t m_lastNotification;
	MUTEX m_notificationLock;
	NXCPMessage m_notificationMessage;
	bool m_blockNextJobsOnFailure;
	bool m_valid;

	static THREAD_RESULT THREAD_CALL WorkerThreadStarter(void *);
	static void sendNotification(ClientSession *session, void *arg);

	void createHistoryRecord();
	void updateHistoryRecord(bool onStart);

protected:
	int m_retryCount;

	virtual ServerJobResult run();
	virtual bool onCancel();
	virtual const TCHAR *getAdditionalInfo();
	virtual const String serializeParameters();

	void notifyClients(bool isStatusChange);
	void changeStatus(ServerJobStatus newStatus);
	void markProgress(int pctCompleted);
	void setFailureMessage(const TCHAR *msg);
	void setDescription(const TCHAR *description);
   void invalidate() { m_valid = false; }

	int getRetryDelay();

public:
	ServerJob(const TCHAR *type, const TCHAR *description, UINT32 nodeId, UINT32 userId, bool createOnHold, int retryCount = -1);
	ServerJob(const TCHAR *params, UINT32 nodeId, UINT32 userId);
	virtual ~ServerJob();

	void start();
	bool cancel();
	bool hold();
	bool unhold();

	void setAutoCancelDelay(int delay) { m_autoCancelDelay = delay; }
	int getAutoCancelDelay() const { return m_autoCancelDelay; }

	void setBlockNextJobsOnFailure(bool flag) { m_blockNextJobsOnFailure = flag; }
	bool isBlockNextJobsOnFailure() const { return m_blockNextJobsOnFailure; }
	bool isValid() const { return m_valid; }

	UINT32 getId() const { return m_id; }
	UINT32 getUserId() const { return m_userId; }
	const TCHAR *getType() const { return m_type; }
	const TCHAR *getDescription() const { return m_description; }
	ServerJobStatus getStatus() const { return m_status; }
	int getProgress() const { return m_progress; }
	UINT32 getNodeId() const { return m_nodeId; }
   Node *getNode() const { return m_node; }
	const TCHAR *getFailureMessage() const { return CHECK_NULL_EX(m_failureMessage); }
	time_t getLastStatusChange() const { return m_lastStatusChange; }

	void setOwningQueue(ServerJobQueue *queue);

	void fillMessage(NXCPMessage *msg);

	virtual void rescheduleExecution();
};

/**
 * Job queue class
 */
class NXCORE_EXPORTABLE ServerJobQueue
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

	UINT32 fillMessage(NXCPMessage *msg, UINT32 *varIdBase);
};

/**
 * Job manager API
 */
bool NXCORE_EXPORTABLE AddJob(ServerJob *job);
void GetJobList(NXCPMessage *msg);
UINT32 NXCORE_EXPORTABLE CancelJob(UINT32 userId, NXCPMessage *msg);
UINT32 NXCORE_EXPORTABLE HoldJob(UINT32 userId, NXCPMessage *msg);
UINT32 NXCORE_EXPORTABLE UnholdJob(UINT32 userId, NXCPMessage *msg);

/**
 * File upload job
 */
class FileUploadJob : public ServerJob
{
private:
	static int m_activeJobs;
	static int m_maxActiveJobs;
	static MUTEX m_sharedDataMutex;

	TCHAR *m_localFile;
	TCHAR *m_localFileFullPath;
	TCHAR *m_remoteFile;
	TCHAR *m_info;
	INT64 m_fileSize;

   void setLocalFileFullPath();

   static void uploadCallback(INT64 size, void *arg);

protected:
	virtual ServerJobResult run();
	virtual const TCHAR *getAdditionalInfo();
	virtual const String serializeParameters();

public:
	static void init();

	FileUploadJob(Node *node, const TCHAR *localFile, const TCHAR *remoteFile, UINT32 userId, bool createOnHold);
	FileUploadJob(const TCHAR *params, UINT32 nodeId, UINT32 userId);
	virtual ~FileUploadJob();

	virtual void rescheduleExecution();
};

/**
 * File download job
 */
class FileDownloadJob : public ServerJob
{
private:
	Node *m_node;
	ClientSession *m_session;
	AgentConnection *m_agentConnection;
	UINT32 m_requestId;
	TCHAR *m_localFile;
	TCHAR *m_remoteFile;
	TCHAR *m_info;
	INT64 m_fileSize;
	INT64 m_currentSize;
	UINT32 m_maxFileSize;
	bool m_follow;

protected:
	virtual ServerJobResult run();
	virtual bool onCancel();
	virtual const TCHAR *getAdditionalInfo();

	static void progressCallback(size_t size, void *arg);
	static void fileResendCallback(NXCP_MESSAGE *msg, void *arg);
	static TCHAR *buildServerFileName(UINT32 nodeId, const TCHAR *remoteFile, TCHAR *buffer, size_t bufferSize);

public:
	FileDownloadJob(Node *node, const TCHAR *remoteName, UINT32 maxFileSize, bool follow, ClientSession *session, UINT32 requestId);
	virtual ~FileDownloadJob();

	const TCHAR *getLocalFileName();
};

/**
 * DCI recalculation job
 */
class DCIRecalculationJob : public ServerJob
{
private:
   DataCollectionTarget *m_object;
   DCItem *m_dci;
   bool m_cancelled;

protected:
   virtual ServerJobResult run();
   virtual bool onCancel();

public:
   DCIRecalculationJob(DataCollectionTarget *object, DCItem *dci, UINT32 userId);
   virtual ~DCIRecalculationJob();
};

class AgentPolicy;

/**
 * Agent policy install job
 */
class PolicyInstallJob : public ServerJob
{
private:
   UINT32 m_templateId;
   uuid m_policyGuid;

protected:
	virtual ServerJobResult run();
	virtual const String serializeParameters();

public:
	PolicyInstallJob(DataCollectionTarget *node, UINT32 templateId, uuid policyGuid, const TCHAR *policyName, UINT32 userId);
	PolicyInstallJob(const TCHAR *params, UINT32 nodeId, UINT32 userId);
	virtual ~PolicyInstallJob();
};

/**
 * Agent policy uninstall job
 */
class PolicyUninstallJob : public ServerJob
{
private:
   TCHAR m_policyType[32];
   uuid m_policyGuid;

protected:
	virtual ServerJobResult run();
	virtual const String serializeParameters();

public:
	PolicyUninstallJob(DataCollectionTarget *node, const TCHAR *m_policyType, uuid policyGuid, UINT32 userId);
   PolicyUninstallJob(const TCHAR *params, UINT32 nodeId, UINT32 userId);
	virtual ~PolicyUninstallJob();
};


#endif   /* _nxcore_jobs_h_ */
