/*
** NetXMS - Network Management System
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
** File: upload_job.cpp
**
**/

#include "nxcore.h"

/**
 * Static members
 */
int FileUploadJob::m_activeJobs = 0;
int FileUploadJob::m_maxActiveJobs = 10;
MUTEX FileUploadJob::m_sharedDataMutex = INVALID_MUTEX_HANDLE;

/**
 * Scheduled file upload
 */
void ScheduledFileUpload(const ScheduledTaskParameters *params)
{
   Node *object = (Node *)FindObjectById(params->m_objectId, OBJECT_NODE);
   if (object != NULL)
   {
      if (object->checkAccessRights(params->m_userId, OBJECT_ACCESS_CONTROL))
      {
         ServerJob *job = new FileUploadJob(params->m_params, params->m_objectId, params->m_userId);
         if (!AddJob(job))
         {
            delete job;
            DbgPrintf(4, _T("ScheduledUploadFile: Failed to add job(incorrect parameters or no such object)."));
         }
      }
      else
         DbgPrintf(4, _T("ScheduledUploadFile: Access to node %s denied"), object->getName());
   }
   else
      DbgPrintf(4, _T("ScheduledUploadFile: Node with id=\'%d\' not found"), params->m_userId);
}

/**
 * Static initializer
 */
void FileUploadJob::init()
{
	m_sharedDataMutex = MutexCreate();
	m_maxActiveJobs = ConfigReadInt(_T("MaxActiveUploadJobs"), 10);
	RegisterSchedulerTaskHandler(_T("Upload.File"), ScheduledFileUpload, SYSTEM_ACCESS_SCHEDULE_FILE_UPLOAD);
}

/**
 * Constructor
 */
FileUploadJob::FileUploadJob(Node *node, const TCHAR *localFile, const TCHAR *remoteFile, UINT32 userId, bool createOnHold)
              : ServerJob(_T("UPLOAD_FILE"), _T("Upload file to managed node"), node->getId(), userId, createOnHold)
{
	TCHAR buffer[1024];
	_sntprintf(buffer, 1024, _T("Upload file %s"), GetCleanFileName(localFile));
	setDescription(buffer);

	m_localFile = _tcsdup(localFile);
	setLocalFileFullPath();
	m_remoteFile = (remoteFile != NULL) ? _tcsdup(remoteFile) : NULL;

	_sntprintf(buffer, 1024, _T("Local file: %s; Remote file: %s"), m_localFile, CHECK_NULL(m_remoteFile));
	m_info = _tcsdup(buffer);

	m_fileSize = 0;
}

/**
 * Create file upload job from scheduled task
 */
FileUploadJob::FileUploadJob(const TCHAR *params, UINT32 nodeId, UINT32 userId)
              : ServerJob(_T("UPLOAD_FILE"), _T("Upload file to managed node"), nodeId, userId, false)
{
   m_localFile = NULL;
   m_localFileFullPath = NULL;
   m_remoteFile = NULL;
   m_info = NULL;

   if (!isValid())
   {
      nxlog_debug(4, _T("FileUploadJob: base job object is invalid for (\"%s\", nodeId=%d, userId=%d)"), params, nodeId, userId);
      return;
   }

   StringList fileList(params, _T(","));
   if (fileList.size() < 2)
   {
      nxlog_debug(4, _T("FileUploadJob: invalid job parameters \"%s\" (nodeId=%d, userId=%d)"), params, nodeId, userId);
      invalidate();
      return;
   }

   if (fileList.size() == 3)
      m_retryCount = _tcstol(fileList.get(2), NULL, 0);

	TCHAR buffer[1024];
	_sntprintf(buffer, 1024, _T("Upload file %s"), GetCleanFileName(fileList.get(0)));
	setDescription(buffer);

	m_localFile = _tcsdup(fileList.get(0));
	setLocalFileFullPath();
	m_remoteFile = fileList.get(1)[0] != 0 ? _tcsdup(fileList.get(1)) : NULL;

	_sntprintf(buffer, 1024, _T("Local file: %s; Remote file: %s"), m_localFile, CHECK_NULL(fileList.get(1)));
	m_info = _tcsdup(buffer);

	m_fileSize = 0;
}

/**
 *  Destructor
 */
FileUploadJob::~FileUploadJob()
{
	free(m_localFile);
	free(m_localFileFullPath);
	free(m_remoteFile);
	free(m_info);
}

/**
 * Set full path for local file
 */
void FileUploadJob::setLocalFileFullPath()
{
   int nLen;
   TCHAR fullPath[MAX_PATH];

   // Create full path to the file store
   _tcscpy(fullPath, g_netxmsdDataDir);
   _tcscat(fullPath, DDIR_FILES);
   _tcscat(fullPath, FS_PATH_SEPARATOR);
   nLen = (int)_tcslen(fullPath);
   nx_strncpy(&fullPath[nLen], GetCleanFileName(m_localFile), MAX_PATH - nLen);
   m_localFileFullPath = _tcsdup(fullPath);
}

/**
 * Run job
 */
ServerJobResult FileUploadJob::run()
{
	ServerJobResult success = JOB_RESULT_FAILED;

	while(true)
	{
		MutexLock(m_sharedDataMutex);
		if (m_activeJobs < m_maxActiveJobs)
		{
			m_activeJobs++;
			MutexUnlock(m_sharedDataMutex);
			break;
		}
		MutexUnlock(m_sharedDataMutex);
		ThreadSleep(5);
	}

	AgentConnectionEx *conn = getNode()->createAgentConnection();
	if (conn != NULL)
	{
		m_fileSize = (INT64)FileSize(m_localFileFullPath);
		UINT32 rcc = conn->uploadFile(m_localFileFullPath, m_remoteFile, uploadCallback, this, NXCP_STREAM_COMPRESSION_DEFLATE);
		if (rcc == ERR_SUCCESS)
		{
			success = JOB_RESULT_SUCCESS;
		}
		else
		{
			setFailureMessage(AgentErrorCodeToText(rcc));
		}
		conn->decRefCount();
	}
	else
	{
		setFailureMessage(_T("Agent connection not available"));
	}

	MutexLock(m_sharedDataMutex);
	m_activeJobs--;
	MutexUnlock(m_sharedDataMutex);

   if(success == JOB_RESULT_FAILED && m_retryCount-- > 0)
   {
      TCHAR description[256];
      _sntprintf(description, 256, _T("File upload failed. Wainting %d minutes to restart job."), getRetryDelay() / 60);
      setDescription(description);
      success = JOB_RESULT_RESCHEDULE;
   }

	return success;
}

/**
 * Upload progress callback
 */
void FileUploadJob::uploadCallback(INT64 size, void *arg)
{
	if (((FileUploadJob *)arg)->m_fileSize > 0)
		((FileUploadJob *)arg)->markProgress((int)(size * _LL(100) / ((FileUploadJob *)arg)->m_fileSize));
	else
		((FileUploadJob *)arg)->markProgress(100);
}

/**
 *  Get additional info for logging
 */
const TCHAR *FileUploadJob::getAdditionalInfo()
{
	return m_info;
}

/**
 * Serializes job parameters into TCHAR line separated by ';'
 */
const String FileUploadJob::serializeParameters()
{
   String params;
   params.append(m_localFile);
   params.append(_T(','));
   params.append(CHECK_NULL_EX(m_remoteFile));
   params.append(_T(','));
   params.append(m_retryCount);
   return params;
}

/**
 * Schedules repeated execution
 */
void FileUploadJob::rescheduleExecution()
{
   AddOneTimeScheduledTask(_T("Policy.Uninstall"), time(NULL) + getRetryDelay(), serializeParameters(), 0, getNodeId(), SYSTEM_ACCESS_FULL, _T(""), SCHEDULED_TASK_SYSTEM);//TODO: change to correct user
}
