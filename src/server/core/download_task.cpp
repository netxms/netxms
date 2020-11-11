/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
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
** File: download_task.cpp
**
**/

#include "nxcore.h"

#define DEBUG_TAG _T("file.download")

/**
 * Constructor for file download task
 */
FileDownloadTask::FileDownloadTask(const shared_ptr<Node>& node, ClientSession *session, uint32_t requestId, const TCHAR *remoteFile,
         bool allowExpansion, uint64_t maxFileSize, bool monitor) : m_node(node)
{
	m_session = session;
	session->incRefCount();

	m_requestId = requestId;
	m_remoteFile = MemCopyString(remoteFile);

	TCHAR buffer[MAX_PATH];
	buildServerFileName(node->getId(), m_remoteFile, buffer, MAX_PATH);
	m_localFile = MemCopyString(buffer);

	m_maxFileSize = maxFileSize;
	m_monitor = monitor;
	m_currentSize = 0;
	m_allowExpansion = allowExpansion;

   nxlog_debug_tag(DEBUG_TAG, 5, _T("Download task created for file %s at node %s, monitor = %s"), remoteFile, node->getName(), monitor ? _T("true") : _T("false"));
}

/**
 * Destructor for file download task
 */
FileDownloadTask::~FileDownloadTask()
{
	m_session->decRefCount();
	MemFree(m_localFile);
	MemFree(m_remoteFile);
}

/**
 * Do file download
 */
void FileDownloadTask::run()
{
   MONITORED_FILE *fileDescriptor = new MONITORED_FILE();
   _tcscpy(fileDescriptor->fileName, m_localFile);
   fileDescriptor->nodeID = m_node->getId();
   fileDescriptor->session = m_session;

   if (m_monitor && g_monitoringList.isDuplicate(fileDescriptor))
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Monitor flag removed by checkDuplicate()"));
      m_monitor = false;
   }

   uint32_t rcc = ERR_CONNECT_FAILED;
   bool success = false;
   m_agentConnection = m_node->createAgentConnection();
	if (m_agentConnection != nullptr)
	{
		NXCPMessage msg(m_agentConnection->getProtocolVersion()), *response;

		m_agentConnection->setDeleteFileOnDownloadFailure(false);

		nxlog_debug_tag(DEBUG_TAG, 5, _T("Sending file stat request for file %s@%s"), m_remoteFile, m_node->getName());
		msg.setCode(CMD_GET_FILE_DETAILS);
		msg.setId(m_agentConnection->generateRequestId());
		msg.setField(VID_FILE_NAME, m_remoteFile);
		response = m_agentConnection->customRequest(&msg);
		if (response != nullptr)
		{
			NXCPMessage notify;
			m_fileSize = (INT64)response->getFieldAsUInt64(VID_FILE_SIZE);
			notify.setCode(CMD_REQUEST_COMPLETED);
			notify.setId(m_requestId);
			notify.setField(VID_FILE_SIZE, m_fileSize);
			notify.setField(VID_NAME, m_localFile);
			notify.setField(VID_FILE_NAME, m_remoteFile);
			m_session->sendMessage(&notify);

			rcc = response->getFieldAsUInt32(VID_RCC);
			nxlog_debug_tag(DEBUG_TAG, 5, _T("Stat request for file %s@%s RCC=%u (%s)"), m_remoteFile, m_node->getName(), rcc, AgentErrorCodeToText(rcc));
			if (rcc == ERR_SUCCESS)
			{
				nxlog_debug_tag(DEBUG_TAG, 5, _T("Sending download request for file %s@%s"), m_remoteFile, m_node->getName());
				msg.setCode(CMD_GET_AGENT_FILE);
				msg.setId(m_agentConnection->generateRequestId());
				msg.setField(VID_FILE_NAME, m_remoteFile);
				msg.setField(VID_ALLOW_PATH_EXPANSION, m_allowExpansion);

            // default - get parameters
            if (m_maxFileSize > 0)
            {
               msg.setField(VID_FILE_OFFSET, (UINT32)(-((int)m_maxFileSize)));
            }
            else
            {
               msg.setField(VID_FILE_OFFSET, 0);
            }
            msg.setField(VID_FILE_FOLLOW, m_monitor);
            msg.setField(VID_NAME, m_localFile);
            msg.setField(VID_ENABLE_COMPRESSION, (m_session == nullptr) || m_session->isCompressionEnabled());

            delete response;
				response = m_agentConnection->customRequest(&msg, m_localFile, false, nullptr, fileResendCallback, this);
				if (response != nullptr)
				{
					rcc = response->getFieldAsUInt32(VID_RCC);
					nxlog_debug_tag(DEBUG_TAG, 5, _T("Download request for file %s@%s RCC=%u (%s)"), m_remoteFile, m_node->getName(), rcc, AgentErrorCodeToText(rcc));
					if (rcc == ERR_SUCCESS)
					{
						success = true;
					}
				}
				else
				{
               nxlog_debug_tag(DEBUG_TAG, 5, _T("Download request for file %s@%s timeout"), m_remoteFile, m_node->getName());
				}
			}
         delete response;
		}
		else
		{
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Stat request for file %s@%s timeout"), m_remoteFile, m_node->getName());
		}
	}
	else
	{
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Agent connection not available for file %s@%s"), m_remoteFile, m_node->getName());
	}

	NXCPMessage response;
	response.setCode(CMD_REQUEST_COMPLETED);
	response.setId(m_requestId);
	if (success)
	{
	   response.setField(VID_RCC, RCC_SUCCESS);
		if (m_monitor)
		{
         g_monitoringList.addFile(fileDescriptor, m_node.get(), m_agentConnection);
		}
		else
		{
         delete fileDescriptor;
		}
	}
	else
	{
	   // Send "abort file transfer" command to client
	   NXCPMessage abortCmd;
	   abortCmd.setCode(CMD_ABORT_FILE_TRANSFER);
	   abortCmd.setId(m_requestId);
	   abortCmd.setField(VID_JOB_CANCELLED, false);
	   m_session->sendMessage(&abortCmd);

      response.setField(VID_RCC, AgentErrorToRCC(rcc));
      delete fileDescriptor;
	}
   m_session->sendMessage(&response);
}

/**
 * Build file ID
 */
TCHAR *FileDownloadTask::buildServerFileName(uint32_t nodeId, const TCHAR *remoteFile, TCHAR *buffer, size_t bufferSize)
{
	BYTE hash[MD5_DIGEST_SIZE];
   CalculateMD5Hash((BYTE *)remoteFile, _tcslen(remoteFile) * sizeof(TCHAR), hash);
	TCHAR hashStr[128];
	_sntprintf(buffer, bufferSize, _T("agent_file_%u_%s"), nodeId, BinToStr(hash, MD5_DIGEST_SIZE, hashStr));
	return buffer;
}

/**
 * Send message callback
 */
void FileDownloadTask::fileResendCallback(NXCPMessage *msg, void *arg)
{
   msg->setId(static_cast<FileDownloadTask*>(arg)->m_requestId);
   static_cast<FileDownloadTask*>(arg)->m_session->sendMessage(msg);
}
