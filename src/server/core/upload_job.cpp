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
** File: upload_job.cpp
**
**/

#include "nxcore.h"
#include <nxcore_jobs.h>

#define DEBUG_TAG _T("file.upload")

/**
 * Static members
 */
int FileUploadJob::m_activeJobs = 0;
int FileUploadJob::m_maxActiveJobs = 10;
Mutex FileUploadJob::m_sharedDataMutex(MutexType::FAST);

/**
 * Scheduled file upload
 */
static void ScheduledFileUpload(const shared_ptr<ScheduledTaskParameters>& parameters)
{
   shared_ptr<NetObj> object = FindObjectById(parameters->m_objectId, OBJECT_NODE);
   if (object != nullptr)
   {
      if (object->checkAccessRights(parameters->m_userId, OBJECT_ACCESS_CONTROL))
      {
         ServerJob *job = new FileUploadJob(parameters->m_persistentData, static_pointer_cast<Node>(object), parameters->m_userId);
         if (!AddJob(job))
         {
            delete job;
            nxlog_debug_tag(DEBUG_TAG, 4, _T("ScheduledUploadFile: Failed to add job (incorrect parameters or no such object)."));
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("ScheduledUploadFile: Access to node %s denied"), object->getName());
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("ScheduledUploadFile: Node with id=\'%d\' not found"), parameters->m_userId);
   }
}

/**
 * Static initializer
 */
void FileUploadJob::init()
{
	m_maxActiveJobs = ConfigReadInt(_T("MaxActiveUploadJobs"), 10);
	RegisterSchedulerTaskHandler(_T("Upload.File"), ScheduledFileUpload, SYSTEM_ACCESS_SCHEDULE_FILE_UPLOAD);
}

/**
 * Constructor
 */
FileUploadJob::FileUploadJob(const shared_ptr<Node>& node, const TCHAR *localFile, const TCHAR *remoteFile, uint32_t userId, bool createOnHold)
              : ServerJob(_T("UPLOAD_FILE"), _T("Upload file to managed node"), node, userId, createOnHold)
{
	TCHAR buffer[1024];
	_sntprintf(buffer, 1024, _T("Upload file %s"), GetCleanFileName(localFile));
	setDescription(buffer);

	m_localFile = MemCopyString(localFile);
	setLocalFileFullPath();
	m_remoteFile = MemCopyString(remoteFile);

	_sntprintf(buffer, 1024, _T("Local file: %s; Remote file: %s"), m_localFile, CHECK_NULL(m_remoteFile));
	m_info = MemCopyString(buffer);

	m_fileSize = 0;
}

/**
 * Create file upload job from scheduled task
 */
FileUploadJob::FileUploadJob(const TCHAR *params, const shared_ptr<Node>& node, uint32_t userId)
              : ServerJob(_T("UPLOAD_FILE"), _T("Upload file to managed node"), node, userId, false)
{
   m_localFile = nullptr;
   m_localFileFullPath = nullptr;
   m_remoteFile = nullptr;
   m_info = nullptr;

   StringList fileList(params, _T(","));
   if (fileList.size() < 2)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("FileUploadJob: invalid job parameters \"%s\" (nodeId=%d, userId=%d)"), params, node->getId(), userId);
      invalidate();
      return;
   }

   if (fileList.size() == 3)
      m_retryCount = _tcstol(fileList.get(2), nullptr, 0);

	TCHAR buffer[1024];
	_sntprintf(buffer, 1024, _T("Upload file %s"), GetCleanFileName(fileList.get(0)));
	setDescription(buffer);

	m_localFile = MemCopyString(fileList.get(0));
	setLocalFileFullPath();
	m_remoteFile = fileList.get(1)[0] != 0 ? MemCopyString(fileList.get(1)) : nullptr;

	_sntprintf(buffer, 1024, _T("Local file: %s; Remote file: %s"), m_localFile, CHECK_NULL(fileList.get(1)));
	m_info = MemCopyString(buffer);

	m_fileSize = 0;
}

/**
 *  Destructor
 */
FileUploadJob::~FileUploadJob()
{
	MemFree(m_localFile);
	MemFree(m_localFileFullPath);
	MemFree(m_remoteFile);
	MemFree(m_info);
}

/**
 * Set full path for local file
 */
void FileUploadJob::setLocalFileFullPath()
{
   TCHAR fullPath[MAX_PATH];
   _tcscpy(fullPath, g_netxmsdDataDir);
   _tcscat(fullPath, DDIR_FILES);
   _tcscat(fullPath, FS_PATH_SEPARATOR);
   size_t len = _tcslen(fullPath);
   _tcslcpy(&fullPath[len], GetCleanFileName(m_localFile), MAX_PATH - len);
   m_localFileFullPath = MemCopyString(fullPath);
}

/**
 * Run job
 */
ServerJobResult FileUploadJob::run()
{
	ServerJobResult result = JOB_RESULT_FAILED;

	while(true)
	{
		m_sharedDataMutex.lock();
		if (m_activeJobs < m_maxActiveJobs)
		{
			m_activeJobs++;
			m_sharedDataMutex.unlock();
			break;
		}
		m_sharedDataMutex.unlock();
		ThreadSleep(5);
	}

	shared_ptr<AgentConnectionEx> conn = static_cast<Node&>(*m_object).createAgentConnection();
	if (conn != nullptr)
	{
		m_fileSize = FileSize(m_localFileFullPath);
		uint32_t rcc = conn->uploadFile(m_localFileFullPath, m_remoteFile, false,
		   [this] (size_t size)
		   {
		      markProgress((m_fileSize > 0) ? static_cast<int>((size * _LL(100) / m_fileSize)) : 100);
		   }, NXCP_STREAM_COMPRESSION_DEFLATE);
		if (rcc == ERR_SUCCESS)
		{
			result = JOB_RESULT_SUCCESS;
		}
		else
		{
			setFailureMessage(AgentErrorCodeToText(rcc));
		}
	}
	else
	{
		setFailureMessage(_T("Agent connection not available"));
	}

	m_sharedDataMutex.lock();
	m_activeJobs--;
	m_sharedDataMutex.unlock();

   if ((result == JOB_RESULT_FAILED) && (m_retryCount-- > 0))
   {
      TCHAR description[256];
      _sntprintf(description, 256, _T("File upload failed. Waiting %d minutes to restart job."), getRetryDelay() / 60);
      setDescription(description);
      result = JOB_RESULT_RESCHEDULE;
   }

	return result;
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
   return StringBuffer(m_localFile)
      .append(_T(','))
      .append(CHECK_NULL_EX(m_remoteFile))
      .append(_T(','))
      .append(m_retryCount);
}

/**
 * Schedule another attempt for job execution
 */
void FileUploadJob::rescheduleExecution()
{
   AddOneTimeScheduledTask(_T("Upload.File"), time(nullptr) + getRetryDelay(), serializeParameters(), nullptr,
            getUserId(), m_objectId, SYSTEM_ACCESS_FULL, _T(""), nullptr, true);
}
