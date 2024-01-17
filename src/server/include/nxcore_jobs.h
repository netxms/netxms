/*
** NetXMS - Network Management System
** Copyright (C) 2003-2022 Victor Kirhenshtein
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

#ifdef _WIN32
template class NXCORE_TEMPLATE_EXPORTABLE shared_ptr<NetObj>;
#endif

/**
 * Job class
 */
class NXCORE_EXPORTABLE ServerJob
{
private:
   uint32_t m_id;
   uint32_t m_userId;
	TCHAR m_type[MAX_JOB_NAME_LEN];
	TCHAR m_description[MAX_DB_STRING];
	ServerJobStatus m_status;
	int m_progress;
	TCHAR *m_failureMessage;
	THREAD m_workerThread;
	ServerJobQueue *m_owningQueue;
	time_t m_lastStatusChange;
	int m_autoCancelDelay;	// Interval in seconds to cancel failed job automatically (0 = disabled)
	time_t m_lastNotification;
	Mutex m_notificationLock;
	NXCPMessage m_notificationMessage;
	bool m_blockNextJobsOnFailure;
	bool m_valid;

	static void workerThread(ServerJob *job);
	static void sendNotification(ClientSession *session, ServerJob *job);

protected:
   uint32_t m_objectId;
   shared_ptr<NetObj> m_object;
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
	ServerJob(const TCHAR *type, const TCHAR *description, const shared_ptr<NetObj>& object, UINT32 userId, bool createOnHold, int retryCount = -1);
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

	uint32_t getId() const { return m_id; }
	uint32_t getUserId() const { return m_userId; }
	const TCHAR *getType() const { return m_type; }
	const TCHAR *getDescription() const { return m_description; }
	ServerJobStatus getStatus() const { return m_status; }
	int getProgress() const { return m_progress; }
	uint32_t getObjectId() const { return m_objectId; }
   const shared_ptr<NetObj>& getObject() const { return m_object; }
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
	Mutex m_accessMutex;
	uint32_t m_objectId;

public:
	ServerJobQueue(uint32_t objectId);
	~ServerJobQueue();

	void add(ServerJob *job);
	bool cancel(UINT32 jobId);
	bool hold(UINT32 jobId);
	bool unhold(UINT32 jobId);
	void runNext();
	void cleanup();

	ServerJob *findJob(UINT32 jobId);
	int getJobCount(const TCHAR *type = nullptr) const;
	uint32_t getObjectId() const { return m_objectId; }

	void jobCompleted(ServerJob *job);

	UINT32 fillMessage(NXCPMessage *msg, UINT32 *varIdBase) const;
};

/**
 * Job manager API
 */
bool NXCORE_EXPORTABLE AddJob(ServerJob *job);
void GetJobList(NXCPMessage *msg);
int NXCORE_EXPORTABLE GetJobCount(uint32_t objectId = 0, const TCHAR *type = nullptr);
uint32_t NXCORE_EXPORTABLE CancelJob(uint32_t userId, const NXCPMessage& request);
uint32_t NXCORE_EXPORTABLE HoldJob(uint32_t userId, const NXCPMessage& request);
uint32_t NXCORE_EXPORTABLE UnholdJob(uint32_t userId, const NXCPMessage& request);

/**
 * File upload job
 */
class FileUploadJob : public ServerJob
{
private:
	static int m_activeJobs;
	static int m_maxActiveJobs;
	static Mutex m_sharedDataMutex;

	TCHAR *m_localFile;
	TCHAR *m_localFileFullPath;
	TCHAR *m_remoteFile;
	TCHAR *m_info;
	int64_t m_fileSize;

   void setLocalFileFullPath();

protected:
	virtual ServerJobResult run() override;
	virtual const TCHAR *getAdditionalInfo() override;
	virtual const String serializeParameters() override;

public:
	static void init();

	FileUploadJob(const shared_ptr<Node>& node, const TCHAR *localFile, const TCHAR *remoteFile, uint32_t userId, bool createOnHold);
	FileUploadJob(const TCHAR *params, const shared_ptr<Node>& node, uint32_t userId);
	virtual ~FileUploadJob();

	virtual void rescheduleExecution() override;
};

/**
 * DCI recalculation job
 */
class DCIRecalculationJob : public ServerJob
{
private:
   DCItem *m_dci;
   bool m_cancelled;

protected:
   virtual ServerJobResult run() override;
   virtual bool onCancel() override;

public:
   DCIRecalculationJob(const shared_ptr<DataCollectionTarget>& object, const DCItem *dci, uint32_t userId);
   virtual ~DCIRecalculationJob();
};

#endif   /* _nxcore_jobs_h_ */
