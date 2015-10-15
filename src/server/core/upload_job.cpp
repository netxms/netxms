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

void ScheduledUploadFile(const ScheduleParameters *params)
{
   //get parameters - node id or name, server file name, agent file name
   TCHAR serverFile[MAX_PATH];
   TCHAR agentFile[MAX_PATH];
   AgentGetParameterArg(params->m_params, 1, serverFile, MAX_PATH, false);
   AgentGetParameterArg(params->m_params, 2, agentFile, MAX_PATH, false);

   if(params->m_objectId == 0 || serverFile == NULL || agentFile == NULL)
   {
      DbgPrintf(4, _T("UploadFile: One of parameters was nodeId=\'%d\', serverFile=\'%s\', agentFile=\'%s\'"),
            params->m_userId, serverFile, agentFile);
      return;
   }

   Node *object = (Node *)FindObjectById(params->m_objectId, OBJECT_NODE);
   if (object != NULL)
   {
      if (object->checkAccessRights(params->m_userId, OBJECT_ACCESS_CONTROL))
      {
         TCHAR fullPath[MAX_PATH];

         // Create full path to the file store
         _tcscpy(fullPath, g_netxmsdDataDir);
         _tcscat(fullPath, DDIR_FILES);
         _tcscat(fullPath, FS_PATH_SEPARATOR);
         int len = (int)_tcslen(fullPath);
         nx_strncpy(&fullPath[len], GetCleanFileName(serverFile), MAX_PATH - len);

         ServerJob *job = new FileUploadJob((Node *)object, fullPath, agentFile, params->m_userId, false);
         if (AddJob(job))
         {
            DbgPrintf(4, _T("ScheduledUploadFile: File(%s) uploaded to %s node, to %s "), serverFile, object->getName(), agentFile);
            //auditlog?
         }
         else
         {
            delete job;
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
	RegisterSchedulerTaskHandler(_T("Upload.File"), ScheduledUploadFile, SYSTEM_ACCESS_SCHEDULE_FILE_UPLOAD);
}

/**
 * Constructor
 */
FileUploadJob::FileUploadJob(Node *node, const TCHAR *localFile, const TCHAR *remoteFile, UINT32 userId, bool createOnHold)
              : ServerJob(_T("UPLOAD_FILE"), _T("Upload file to managed node"), node->getId(), userId, createOnHold)
{
	m_node = node;
	node->incRefCount();

	TCHAR buffer[1024];
	_sntprintf(buffer, 1024, _T("Upload file %s"), GetCleanFileName(localFile));
	setDescription(buffer);

	m_localFile = _tcsdup(localFile);
	m_remoteFile = (remoteFile != NULL) ? _tcsdup(remoteFile) : NULL;

	_sntprintf(buffer, 1024, _T("Local file: %s; Remote file: %s"), m_localFile, CHECK_NULL(m_remoteFile));
	m_info = _tcsdup(buffer);
}

/**
 *  Destructor
 */
FileUploadJob::~FileUploadJob()
{
	m_node->decRefCount();
	safe_free(m_localFile);
	safe_free(m_remoteFile);
	safe_free(m_info);
}

/**
 * Run job
 */
bool FileUploadJob::run()
{
	bool success = false;

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

	AgentConnectionEx *conn = m_node->createAgentConnection();
	if (conn != NULL)
	{
		m_fileSize = (INT64)FileSize(m_localFile);
		UINT32 rcc = conn->uploadFile(m_localFile, m_remoteFile, uploadCallback, this);
		if (rcc == ERR_SUCCESS)
		{
			success = true;
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

	MutexLock(m_sharedDataMutex);
	m_activeJobs--;
	MutexUnlock(m_sharedDataMutex);

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
