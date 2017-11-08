/*
** NetXMS - Network Management System
** Copyright (C) 2003-2016 Victor Kirhenshtein
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
** File: download_job.cpp
**
**/

#include "nxcore.h"

/**
 * Constructor for download job
 */
FileDownloadJob::FileDownloadJob(Node *node, const TCHAR *remoteFile, UINT32 maxFileSize, bool follow, ClientSession *session, UINT32 requestId)
                : ServerJob(_T("DOWNLOAD_FILE"), _T("Download file"), node->getId(), session->getUserId(), false)
{
	m_session = session;
	session->incRefCount();

	m_agentConnection = NULL;

	m_node = node;
	node->incRefCount();

	m_requestId = requestId;

	m_remoteFile = _tcsdup(remoteFile);

	TCHAR buffer[1024];
	buildServerFileName(node->getId(), m_remoteFile, buffer, 1024);
	m_localFile = _tcsdup(buffer);

	_sntprintf(buffer, 1024, _T("Download file %s@%s"), m_remoteFile, node->getName());
	setDescription(buffer);

	_sntprintf(buffer, 1024, _T("Local file: %s; Remote file: %s"), m_localFile, m_remoteFile);
	m_info = _tcsdup(buffer);

	setAutoCancelDelay(60);

	m_maxFileSize = maxFileSize;
	m_follow = follow;
	m_currentSize = 0;

   DbgPrintf(5, _T("FileDownloadJob: job created for file %s at node %s, follow = %s"), m_remoteFile, m_node->getName(), m_follow ? _T("true") : _T("false"));
}

/**
 * Destructor for download job
 */
FileDownloadJob::~FileDownloadJob()
{
	m_node->decRefCount();
	m_session->decRefCount();
	if (m_agentConnection != NULL)
	   m_agentConnection->decRefCount();
	free(m_localFile);
	free(m_remoteFile);
	free(m_info);
}

/**
 * Returns localFileName
 */
const TCHAR *FileDownloadJob::getLocalFileName()
{
   return m_localFile;
}

/**
 * Send message callback
 */
void FileDownloadJob::fileResendCallback(NXCP_MESSAGE *msg, void *arg)
{
   msg->id = htonl(((FileDownloadJob *)arg)->m_requestId);
	((FileDownloadJob *)arg)->m_session->sendRawMessage(msg);
}

/**
 * Progress callback
 */
void FileDownloadJob::progressCallback(size_t size, void *arg)
{
   ((FileDownloadJob *)arg)->m_currentSize += (INT64)size;
   if (((FileDownloadJob *)arg)->m_fileSize > 0)
      ((FileDownloadJob *)arg)->markProgress((int)(((FileDownloadJob *)arg)->m_currentSize * _LL(90) / ((FileDownloadJob *)arg)->m_fileSize));
   else
      ((FileDownloadJob *)arg)->markProgress(90);
}

/**
 * Job implementation
 */
ServerJobResult FileDownloadJob::run()
{
	UINT32 rcc = 0xFFFFFFFF;
	ServerJobResult success = JOB_RESULT_FAILED;

   MONITORED_FILE * newFile = new MONITORED_FILE();
   _tcscpy(newFile->fileName, m_localFile);
   newFile->nodeID = m_node->getId();
   newFile->session = m_session;

   if (g_monitoringList.checkDuplicate(newFile))
   {
      DbgPrintf(6, _T("FileDownloadJob: follow flag cancelled by checkDuplicate()"));
      m_follow = false;
   }

   m_agentConnection = m_node->createAgentConnection();
	if (m_agentConnection != NULL)
	{
		NXCPMessage msg(m_agentConnection->getProtocolVersion()), *response;

		m_agentConnection->setDeleteFileOnDownloadFailure(false);

		DbgPrintf(5, _T("FileDownloadJob: Sending file stat request for file %s@%s"), m_remoteFile, m_node->getName());
		msg.setCode(CMD_GET_FILE_DETAILS);
		msg.setId(m_agentConnection->generateRequestId());
		msg.setField(VID_FILE_NAME, m_remoteFile);
		response = m_agentConnection->customRequest(&msg);
		if (response != NULL)
		{
			NXCPMessage notify;
			m_fileSize = (INT64)response->getFieldAsUInt64(VID_FILE_SIZE);
			notify.setCode(CMD_REQUEST_COMPLETED);
			notify.setId(m_requestId);
			notify.setField(VID_FILE_SIZE, m_fileSize);
			m_session->sendMessage(&notify);

			rcc = response->getFieldAsUInt32(VID_RCC);
			DbgPrintf(5, _T("FileDownloadJob: Stat request for file %s@%s RCC=%d"), m_remoteFile, m_node->getName(), rcc);
			if (rcc == ERR_SUCCESS)
			{
				delete response;

				DbgPrintf(5, _T("FileDownloadJob: Sending download request for file %s@%s"), m_remoteFile, m_node->getName());
				msg.setCode(CMD_GET_AGENT_FILE);
				msg.setId(m_agentConnection->generateRequestId());
				msg.setField(VID_FILE_NAME, m_remoteFile);

            // default - get parameters
            if (m_maxFileSize > 0)
            {
               msg.setField(VID_FILE_OFFSET, (UINT32)(-((int)m_maxFileSize)));
            }
            else
            {
               msg.setField(VID_FILE_OFFSET, 0);
            }
            msg.setField(VID_FILE_FOLLOW, m_follow);
            msg.setField(VID_NAME, m_localFile);
            msg.setField(VID_ENABLE_COMPRESSION, (m_session == NULL) || m_session->isCompressionEnabled());

				response = m_agentConnection->customRequest(&msg, m_localFile, false, progressCallback, fileResendCallback, this);
				if (response != NULL)
				{
					rcc = response->getFieldAsUInt32(VID_RCC);
					DbgPrintf(5, _T("FileDownloadJob: Download request for file %s@%s RCC=%d"), m_remoteFile, m_node->getName(), rcc);
					if (rcc == ERR_SUCCESS)
					{
						success = JOB_RESULT_SUCCESS;
					}
					else if(getStatus() != JOB_CANCELLED && getStatus() != JOB_CANCEL_PENDING)
					{
						TCHAR buffer[1024];

						_sntprintf(buffer, 1024, _T("Error %d: %s"), rcc, AgentErrorCodeToText((int)rcc));
						setFailureMessage(buffer);
					}
					delete response;
				}
				else
				{
					setFailureMessage(_T("Request timed out"));
				}
			}
			else
			{
				TCHAR buffer[1024];

				_sntprintf(buffer, 1024, _T("Error %d: %s"), rcc, AgentErrorCodeToText((int)rcc));
				setFailureMessage(buffer);

				delete response;
			}
		}
		else
		{
			setFailureMessage(_T("Request timed out"));
		}
	}
	else
	{
		setFailureMessage(_T("Agent connection not available"));
	}

	NXCPMessage response;
	response.setCode(CMD_REQUEST_COMPLETED);
	response.setId(m_requestId);
	if (success == JOB_RESULT_SUCCESS)
	{
	   response.setField(VID_RCC, RCC_SUCCESS);
		m_session->sendMessage(&response);
		if (m_follow)
		{
         g_monitoringList.addMonitoringFile(newFile, m_node, m_agentConnection);
		}
		else
		{
         delete newFile;
		}
	}
	else
	{
	   // Send "abort file transfer" command to client
	   NXCPMessage abortCmd;
	   abortCmd.setCode(CMD_ABORT_FILE_TRANSFER);
	   abortCmd.setId(m_requestId);
	   abortCmd.setField(VID_JOB_CANCELED, getStatus() != JOB_CANCELLED || getStatus() != JOB_CANCEL_PENDING);
	   m_session->sendMessage(&abortCmd);

	   if(getStatus() != JOB_CANCELLED || getStatus() != JOB_CANCEL_PENDING)
	   {
	      response.setField(VID_RCC, RCC_SUCCESS);
	   }
	   else
	   {
         switch(rcc)
         {
            case ERR_ACCESS_DENIED:
               response.setField(VID_RCC, RCC_ACCESS_DENIED);
               break;
            case ERR_IO_FAILURE:
               response.setField(VID_RCC, RCC_IO_ERROR);
               break;
            case ERR_FILE_OPEN_ERROR:
            case ERR_FILE_STAT_FAILED:
               response.setField(VID_RCC, RCC_FILE_IO_ERROR);
               break;
            default:
               response.setField(VID_RCC, RCC_COMM_FAILURE);
               break;
         }
	   }
		m_session->sendMessage(&response);
	}

	if (m_agentConnection != NULL)
	{
	   m_agentConnection->decRefCount();
	   m_agentConnection = NULL;
	}
	return success;
}

/**
 * Job cancellation handler
 */
bool FileDownloadJob::onCancel()
{
   m_agentConnection->cancelFileDownload();
	return true;
}

/**
 * Get additional info for logging
 */
const TCHAR *FileDownloadJob::getAdditionalInfo()
{
	return m_info;
}

/**
 * Build file ID
 */
TCHAR *FileDownloadJob::buildServerFileName(UINT32 nodeId, const TCHAR *remoteFile, TCHAR *buffer, size_t bufferSize)
{
	BYTE hash[MD5_DIGEST_SIZE];
	TCHAR hashStr[128];

	CalculateMD5Hash((BYTE *)remoteFile, _tcslen(remoteFile) * sizeof(TCHAR), hash);
	_sntprintf(buffer, bufferSize, _T("agent_file_%u_%s"), nodeId, BinToStr(hash, MD5_DIGEST_SIZE, hashStr));
	return buffer;
}
