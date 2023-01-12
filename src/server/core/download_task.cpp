/*
** NetXMS - Network Management System
** Copyright (C) 2003-2021 Victor Kirhenshtein
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

   BYTE hash[MD5_DIGEST_SIZE];
   CalculateMD5Hash(reinterpret_cast<const BYTE*>(remoteFile), _tcslen(remoteFile) * sizeof(TCHAR), hash);
   _tcscpy(m_localFile, _T("agent-file-"));
   BinToStr(hash, MD5_DIGEST_SIZE, &m_localFile[11]);
   m_localFile[43] = _T('-');
   IntegerToString(node->getId(), &m_localFile[44]);

	m_maxFileSize = maxFileSize;
	m_monitor = monitor;
	m_currentSize = 0;
	m_fileSize = 0;
	m_allowExpansion = allowExpansion;

   nxlog_debug_tag(DEBUG_TAG, 5, _T("Download task created for file %s at node %s, monitor = %s"), remoteFile, node->getName(), monitor ? _T("true") : _T("false"));
}

/**
 * Destructor for file download task
 */
FileDownloadTask::~FileDownloadTask()
{
	m_session->decRefCount();
	MemFree(m_remoteFile);
}

/**
 * Do file download
 */
void FileDownloadTask::run()
{
   uint32_t rcc = ERR_CONNECT_FAILED;
   bool success = false;
   uuid monitorId;

   shared_ptr<AgentConnectionEx> agentConnection = m_node->createAgentConnection();
	if (agentConnection != nullptr)
	{
		NXCPMessage msg(agentConnection->getProtocolVersion());

		agentConnection->setDeleteFileOnDownloadFailure(false);

		nxlog_debug_tag(DEBUG_TAG, 5, _T("Sending file stat request for file %s@%s"), m_remoteFile, m_node->getName());
		msg.setCode(CMD_GET_FILE_DETAILS);
		msg.setId(agentConnection->generateRequestId());
		msg.setField(VID_FILE_NAME, m_remoteFile);
		NXCPMessage *response = agentConnection->customRequest(&msg);
		if (response != nullptr)
		{
         m_fileSize = response->getFieldAsUInt64(VID_FILE_SIZE);

         // Send first confirmation to the client
			NXCPMessage notify(CMD_REQUEST_COMPLETED, m_requestId);
			notify.setField(VID_FILE_SIZE, m_fileSize);
			notify.setField(VID_NAME, m_localFile);
			notify.setField(VID_FILE_NAME, m_remoteFile);
         if (m_monitor)
         {
            monitorId = uuid::generate();
            notify.setField(VID_MONITOR_ID, monitorId);
         }
			m_session->sendMessage(notify);

			rcc = response->getFieldAsUInt32(VID_RCC);
			nxlog_debug_tag(DEBUG_TAG, 5, _T("Stat request for file %s@%s RCC=%u (%s)"), m_remoteFile, m_node->getName(), rcc, AgentErrorCodeToText(rcc));
			if (rcc == ERR_SUCCESS)
			{
				nxlog_debug_tag(DEBUG_TAG, 5, _T("Sending download request for file %s@%s"), m_remoteFile, m_node->getName());
				msg.setCode(CMD_GET_AGENT_FILE);
				msg.setId(agentConnection->generateRequestId());
				msg.setField(VID_FILE_NAME, m_remoteFile);
				msg.setField(VID_ALLOW_PATH_EXPANSION, m_allowExpansion);

            // default - get parameters
            if (m_maxFileSize < m_fileSize)
            {
               msg.setField(VID_FILE_OFFSET, -static_cast<int32_t>(m_maxFileSize));
            }
            else
            {
               msg.setField(VID_FILE_OFFSET, 0);
            }
            msg.setField(VID_FILE_FOLLOW, m_monitor && !IsFileMonitorActive(m_node->getId(), m_localFile));
            msg.setField(VID_NAME, m_localFile);
            msg.setField(VID_ENABLE_COMPRESSION, (m_session == nullptr) || m_session->isCompressionEnabled());

            delete response;
				response = agentConnection->customRequest(&msg, m_localFile, false, nullptr,
				   [this] (NXCPMessage *agentMsg)
				   {
				      agentMsg->setId(m_requestId);
				      m_session->sendMessage(agentMsg);
				   });
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

	NXCPMessage response(CMD_REQUEST_COMPLETED, m_requestId);
	if (success)
	{
	   response.setField(VID_RCC, RCC_SUCCESS);
		if (m_monitor)
		{
		   AddFileMonitor(m_node.get(), agentConnection, m_session, m_localFile, monitorId);
		}
	}
	else
	{
	   // Send "abort file transfer" command to client
	   NXCPMessage abortCmd(CMD_ABORT_FILE_TRANSFER, m_requestId);
	   abortCmd.setField(VID_JOB_CANCELLED, false);
	   m_session->sendMessage(abortCmd);

      response.setField(VID_RCC, AgentErrorToRCC(rcc));
	}
   m_session->sendMessage(response);
}
