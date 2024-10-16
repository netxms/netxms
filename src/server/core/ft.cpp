/*
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
** File: ft.cpp
**/

#include "nxcore.h"
#include <nxtask.h>

#define DEBUG_TAG _T("file.upload")

/**
 * Thread pool for file upload
 */
ThreadPool *g_fileTransferThreadPool = nullptr;

/**
 * File upload job
 */
class FileUploadJob
{
private:
   shared_ptr<Node> m_node;
   uint32_t m_userId;
   TCHAR *m_localFile;
   TCHAR *m_localFileFullPath;
   TCHAR *m_remoteFile;
   TCHAR *m_info;
   int64_t m_fileSize;
   int m_retryCount;
   bool m_valid;

   void setLocalFileFullPath();

protected:
   String serializeParameters()
   {
      return StringBuffer(m_localFile)
         .append(_T(','))
         .append(CHECK_NULL_EX(m_remoteFile))
         .append(_T(','))
         .append(m_retryCount);
   }

public:
   FileUploadJob(const shared_ptr<Node>& node, const TCHAR *localFile, const TCHAR *remoteFile, uint32_t userId);
   FileUploadJob(const TCHAR *params, const shared_ptr<Node>& node, uint32_t userId);
   ~FileUploadJob()
   {
      MemFree(m_localFile);
      MemFree(m_localFileFullPath);
      MemFree(m_remoteFile);
      MemFree(m_info);
   }

   bool run(BackgroundTask *task);

   const bool isValid() const
   {
      return m_valid;
   }

   String createDescription() const
   {
      if (!m_valid)
         return String(_T("Invalid job"));
      StringBuffer sb(_T("Uploading file "));
      sb.append(GetCleanFileName(m_localFile));
      sb.append(_T(" to "));
      sb.append(m_node->getName());
      return sb;
   }
};

/**
 * Scheduled file upload
 */
void ScheduledFileUpload(const shared_ptr<ScheduledTaskParameters>& parameters)
{
   shared_ptr<NetObj> object = FindObjectById(parameters->m_objectId, OBJECT_NODE);
   if (object != nullptr)
   {
      if (object->checkAccessRights(parameters->m_userId, OBJECT_ACCESS_CONTROL))
      {
         auto job = new FileUploadJob(parameters->m_persistentData, static_pointer_cast<Node>(object), parameters->m_userId);
         CreateBackgroundTask(g_fileTransferThreadPool,
            [job] (BackgroundTask *task) -> bool
            {
               bool success = job->run(task);
               delete job;
               return success;
            }, job->createDescription());
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
 * Constructor
 */
FileUploadJob::FileUploadJob(const shared_ptr<Node>& node, const TCHAR *localFile, const TCHAR *remoteFile, uint32_t userId) : m_node(node)
{
   m_userId = userId;

   m_remoteFile = MemCopyString(remoteFile);
	m_localFile = MemCopyString(localFile);
	setLocalFileFullPath();

	TCHAR buffer[1024];
	_sntprintf(buffer, 1024, _T("Local file: %s; Remote file: %s"), m_localFile, CHECK_NULL(m_remoteFile));
	m_info = MemCopyString(buffer);

	m_fileSize = 0;
	m_retryCount = 5;
	m_valid = true;
}

/**
 * Create file upload job from scheduled task
 */
FileUploadJob::FileUploadJob(const TCHAR *params, const shared_ptr<Node>& node, uint32_t userId) : m_node(node)
{
   m_userId = userId;

   StringList fileList(params, _T(","));
   if (fileList.size() < 2)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("FileUploadJob: invalid job parameters \"%s\" (nodeId=%d, userId=%d)"), params, node->getId(), userId);
      m_remoteFile = nullptr;
      m_localFile = nullptr;
      m_localFileFullPath = nullptr;
      m_info = nullptr;
      m_valid = false;
      return;
   }

   if (fileList.size() == 3)
      m_retryCount = _tcstol(fileList.get(2), nullptr, 0);

	m_localFile = MemCopyString(fileList.get(0));
	setLocalFileFullPath();
	m_remoteFile = fileList.get(1)[0] != 0 ? MemCopyString(fileList.get(1)) : nullptr;

	TCHAR buffer[1024];
	_sntprintf(buffer, 1024, _T("Local file: %s; Remote file: %s"), m_localFile, CHECK_NULL(fileList.get(1)));
	m_info = MemCopyString(buffer);

	m_fileSize = 0;
   m_valid = true;
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
bool FileUploadJob::run(BackgroundTask *task)
{
   if (!m_valid)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("File upload to %s [%u] failed (invalid job configuration)"), m_node->getName(), m_node->getId());
      return false;
   }

	bool success = false;

	shared_ptr<AgentConnectionEx> conn = m_node->createAgentConnection();
	if (conn != nullptr)
	{
		m_fileSize = FileSize(m_localFileFullPath);
		uint32_t rcc = conn->uploadFile(m_localFileFullPath, m_remoteFile, false,
		   [this, task] (size_t size)
		   {
		      task->markProgress((m_fileSize > 0) ? static_cast<int>((size * _LL(100) / m_fileSize)) : 100);
		   }, NXCP_STREAM_COMPRESSION_DEFLATE);
		if (rcc == ERR_SUCCESS)
		{
         nxlog_debug_tag(DEBUG_TAG, 5, _T("File upload to %s [%u] completed (%s)"), m_node->getName(), m_node->getId(), m_info);
			success = true;
		}
		else
		{
		   nxlog_debug_tag(DEBUG_TAG, 5, _T("File upload to %s [%u] failed (%s) (%s)"), m_node->getName(), m_node->getId(), AgentErrorCodeToText(rcc), m_info);
		}
	}
	else
	{
      nxlog_debug_tag(DEBUG_TAG, 5, _T("File upload to %s [%u] failed (agent connection unavailable) (%s)"), m_node->getName(), m_node->getId(), m_info);
	}

   if (!success && (m_retryCount-- > 0))
   {
      AddOneTimeScheduledTask(_T("Upload.File"), time(nullptr) + 600, serializeParameters(), nullptr, m_userId, m_node->getId(), SYSTEM_ACCESS_FULL, _T(""), nullptr, true);
   }
	return success;
}

/**
 * Start file upload to agent
 */
uint64_t StartFileUploadToAgent(const shared_ptr<Node>& node, const TCHAR *localFile, const TCHAR *remoteFile, uint32_t userId)
{
   auto job = new FileUploadJob(node, localFile, remoteFile, userId);
   return CreateBackgroundTask(g_fileTransferThreadPool,
      [job] (BackgroundTask *task) -> bool
      {
         bool success = job->run(task);
         delete job;
         return success;
      }, job->createDescription())->getId();
}
