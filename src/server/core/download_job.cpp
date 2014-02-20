/*
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
                : ServerJob(_T("DOWNLOAD_FILE"), _T("Download file"), node->Id(), session->getUserId(), false)
{
	m_session = session;
	session->incRefCount();

	m_node = node;
	node->incRefCount();

	m_requestId = requestId;

	m_remoteFile = Event::expandText(NULL, node->Id(), remoteFile, NULL);

	TCHAR buffer[1024];
	buildServerFileName(node->Id(), m_remoteFile, buffer, 1024);
	m_localFile = _tcsdup(buffer);

	_sntprintf(buffer, 1024, _T("Download file %s@%s"), m_remoteFile, node->Name());
	setDescription(buffer);

	_sntprintf(buffer, 1024, _T("Local file: %s; Remote file: %s"), m_localFile, m_remoteFile);
	m_info = _tcsdup(buffer);

	setAutoCancelDelay(60);

	m_maxFileSize = maxFileSize;
	m_follow = follow;

   DbgPrintf(5, _T("FileDownloadJob: job created for file %s at node %s, follow = %s"), m_remoteFile, m_node->Name(), m_follow ? _T("true") : _T("false"));
}

/**
 * Destructor for download job
 */
FileDownloadJob::~FileDownloadJob()
{
	m_node->decRefCount();
	m_session->decRefCount();
	safe_free(m_localFile);
	safe_free(m_remoteFile);
	safe_free(m_info);
}

/**
 * Progress callback
 */
void FileDownloadJob::progressCallback(size_t size, void *arg)
{
	if (((FileDownloadJob *)arg)->m_fileSize > 0)
		((FileDownloadJob *)arg)->markProgress((int)((INT64)size * _LL(90) / ((FileDownloadJob *)arg)->m_fileSize));
	else
		((FileDownloadJob *)arg)->markProgress(90);
}

/**
 * Job implementation
 */
bool FileDownloadJob::run()
{
	AgentConnection *conn;
	UINT32 rcc = 0xFFFFFFFF;
	bool success = false;

   MONITORED_FILE * newFile = new MONITORED_FILE();
   _tcscpy(newFile->fileName, m_localFile);
   newFile->nodeID = m_node->Id();
   newFile->session = m_session;

   if (g_monitoringList.checkDuplicate(newFile))
   {
      DbgPrintf(6, _T("FileDownloadJob: follow flag cancelled by checkDuplicate()"));
      m_follow = false;
   }

	conn = m_node->createAgentConnection();
	if (conn != NULL)
	{
		CSCPMessage msg, *response;

		m_socket = conn->getSocket();
		conn->setDeleteFileOnDownloadFailure(false);

		DbgPrintf(5, _T("FileDownloadJob: Sending file stat request for file %s@%s"), m_remoteFile, m_node->Name());
		msg.SetCode(CMD_GET_FILE_DETAILS);
		msg.SetId(conn->generateRequestId());
		msg.SetVariable(VID_FILE_NAME, m_remoteFile);
		response = conn->customRequest(&msg);
		if (response != NULL)
		{
			rcc = response->GetVariableLong(VID_RCC);
			DbgPrintf(5, _T("FileDownloadJob: Stat request for file %s@%s RCC=%d"), m_remoteFile, m_node->Name(), rcc);
			if (rcc == ERR_SUCCESS)
			{
				m_fileSize = (INT64)response->GetVariableInt64(VID_FILE_SIZE);
				time_t modTime = (time_t)response->GetVariableInt64(VID_MODIFY_TIME);
				delete response;

				DbgPrintf(5, _T("FileDownloadJob: Sending download request for file %s@%s"), m_remoteFile, m_node->Name());
				msg.SetCode(CMD_GET_AGENT_FILE);
				msg.SetId(conn->generateRequestId());
				msg.SetVariable(VID_FILE_NAME, m_remoteFile);

				// If file exists, request partial download
	#ifdef _WIN32
				struct _stat fs;
	#else
				struct stat fs;
	#endif
				bool appendFile = false;
				if (_tstat(m_localFile, &fs) == 0)
				{
					if ((m_fileSize > (INT64)fs.st_size) && (modTime <= fs.st_mtime))
					{
						msg.SetVariable(VID_FILE_OFFSET, (UINT32)fs.st_size);
						appendFile = true;
						DbgPrintf(5, _T("FileDownloadJob: File %s already exist, requesting download from offset %u"), m_localFile, (UINT32)fs.st_size);
					}
				}

            //default - get parameters

            msg.SetVariable(VID_FILE_SIZE_LIMIT, m_maxFileSize);
            msg.SetVariable(VID_FILE_FOLLOW, (INT16)(m_follow ? 1 : 0));
            msg.SetVariable(VID_NAME, m_localFile);

				response = conn->customRequest(&msg, m_localFile, appendFile, progressCallback, this);
				if (response != NULL)
				{
					rcc = response->GetVariableLong(VID_RCC);
					DbgPrintf(5, _T("FileDownloadJob: Download request for file %s@%s RCC=%d"), m_remoteFile, m_node->Name(), rcc);
					if (rcc == ERR_SUCCESS)
					{
						success = true;
					}
					else
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

		delete conn;
	}
	else
	{
		setFailureMessage(_T("Agent connection not available"));
	}

	CSCPMessage response;
	response.SetCode(CMD_REQUEST_COMPLETED);
	response.SetId(m_requestId);
	if (success)
	{
	   response.SetVariable(VID_RCC, RCC_SUCCESS);
		m_session->sendMessage(&response);
		m_session->sendFile(m_localFile, m_requestId, m_maxFileSize);
		if(m_follow)
		{
         g_monitoringList.addMonitoringFile(newFile);
		}
	}
	else
	{
      switch(rcc)
      {
         case ERR_ACCESS_DENIED:
            response.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
            break;
         case ERR_IO_FAILURE:
            response.SetVariable(VID_RCC, RCC_IO_ERROR);
            break;
			case ERR_FILE_OPEN_ERROR:
			case ERR_FILE_STAT_FAILED:
            response.SetVariable(VID_RCC, RCC_FILE_IO_ERROR);
            break;
         default:
            response.SetVariable(VID_RCC, RCC_COMM_FAILURE);
            break;
      }
		m_session->sendMessage(&response);
	}
	return success;
}


//
// Job cancellation handler
//

bool FileDownloadJob::onCancel()
{
	shutdown(m_socket, SHUT_RDWR);
	return true;
}


//
// Get additional info for logging
//

const TCHAR *FileDownloadJob::getAdditionalInfo()
{
	return m_info;
}


//
// Build name of staging file on server
//

TCHAR *FileDownloadJob::buildServerFileName(UINT32 nodeId, const TCHAR *remoteFile, TCHAR *buffer, size_t bufferSize)
{
	BYTE hash[MD5_DIGEST_SIZE];
	TCHAR hashStr[128];

	CalculateMD5Hash((BYTE *)remoteFile, _tcslen(remoteFile) * sizeof(TCHAR), hash);
	_sntprintf(buffer, bufferSize, _T("%s") DDIR_FILES FS_PATH_SEPARATOR _T("agent_file_%u_%s"),
	           g_szDataDir, nodeId, BinToStr(hash, MD5_DIGEST_SIZE, hashStr));
	return buffer;
}
