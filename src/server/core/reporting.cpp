/* 
** NetXMS - Network Management System
** Copyright (C) 2013 Alex Kirhenshtein
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
** File: reporting.cpp
**
**/

#include "nxcore.h"

/**
 * File request information
 */
struct FileRequest
{
   UINT32 originalRequestId;
   UINT32 serverRequestId;
   ClientSession *session;

   FileRequest(UINT32 originalRequestId, UINT32 serverRequestId, ClientSession *session)
   {
      this->originalRequestId = originalRequestId;
      this->serverRequestId = serverRequestId;
      this->session = session;
   }
};

/**
 * Outstanding file requests
 */
static ObjectArray<FileRequest> s_fileRequests(16, 16, true);
static MUTEX s_fileRequestLock = MutexCreate();

/**
 * Remove all pending file transfer requests for session
 */
void RemovePendingFileTransferRequests(ClientSession *session)
{
   MutexLock(s_fileRequestLock);
   for(int i = 0; i < s_fileRequests.size(); i++)
   {
      FileRequest *f = s_fileRequests.get(i);
      if (f->session == session)
      {
         s_fileRequests.remove(i);
         i--;
      }
   }
   MutexUnlock(s_fileRequestLock);
}

/**
 * Reporting server connector
 */
class RSConnector : public ISC
{
protected:
   virtual void onBinaryMessage(CSCP_MESSAGE *rawMsg);

public:
   RSConnector(UINT32 addr, WORD port) : ISC(addr, port)
   {
   }

   virtual void PrintMsg(const TCHAR *format, ...)
   {
      va_list args;
      va_start(args, format);
      DbgPrintf2(7, format, args);
      va_end(args);
   }
};

/**
 * Custom handler for binary messages
 */
void RSConnector::onBinaryMessage(CSCP_MESSAGE *rawMsg)
{
   if ((ntohs(rawMsg->wCode) != CMD_FILE_DATA) && (ntohs(rawMsg->wCode) != CMD_ABORT_FILE_TRANSFER))
      return;

   MutexLock(s_fileRequestLock);
   for(int i = 0; i < s_fileRequests.size(); i++)
   {
      FileRequest *f = s_fileRequests.get(i);
      if (f->serverRequestId == ntohl(rawMsg->dwId))
      {
         rawMsg->dwId = htonl(f->originalRequestId);
         f->session->sendRawMessage(rawMsg);
         if ((ntohs(rawMsg->wFlags) & MF_END_OF_FILE) || (ntohs(rawMsg->wCode) == CMD_ABORT_FILE_TRANSFER))
         {
            s_fileRequests.remove(i);
         }
         break;
      }
   }
   MutexUnlock(s_fileRequestLock);
}

/**
 * Reporting server connector instance
 */
static RSConnector *m_connector = NULL;

/**
 * Reporting server connection manager
 */
THREAD_RESULT THREAD_CALL ReportingServerConnector(void *arg)
{
	TCHAR hostname[256];
	ConfigReadStr(_T("ReportingServerHostname"), hostname, 256, _T("localhost"));
   WORD port = (WORD)ConfigReadInt(_T("ReportingServerPort"), 4710);

	DbgPrintf(1, _T("Reporting Server connector started (%s:%d)"), hostname, port);

   // Keep connection open
   m_connector = new RSConnector(ResolveHostName(hostname), port);
   while(!SleepAndCheckForShutdown(15))
   {
      if (m_connector->nop() != ISC_ERR_SUCCESS)
      {
         if (m_connector->connect(0) == ISC_ERR_SUCCESS)
         {
            DbgPrintf(6, _T("Connection to Reporting Server restored"));
         }
      }
	}
   m_connector->disconnect();
	delete m_connector;
   m_connector = NULL;

	DbgPrintf(1, _T("Reporting Server connector stopped"));
   return THREAD_OK;
}

/**
 * Forward client message to reporting server
 */
CSCPMessage *ForwardMessageToReportingServer(CSCPMessage *request, ClientSession *session)
{
   if (m_connector == NULL || !m_connector->connected())
   {
      return NULL;
   }

   UINT32 originalId = request->GetId();
   UINT32 rqId = m_connector->generateMessageId();
   request->SetId(rqId);

   // File transfer requests
   if (request->GetCode() == CMD_RS_RENDER_RESULT)
   {
      MutexLock(s_fileRequestLock);
      s_fileRequests.add(new FileRequest(originalId, rqId, session));
      MutexUnlock(s_fileRequestLock);
   }

   CSCPMessage *reply = NULL;
   if (m_connector->sendMessage(request))
   {
      reply = m_connector->waitForMessage(CMD_REQUEST_COMPLETED, request->GetId(), 10000);
   }

   if (reply != NULL)
   {
      reply->SetId(originalId);
   }

   return reply;
}
