/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Raden Solutions
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

#define DEBUG_TAG _T("reporting")

/**
 * File request information
 */
struct FileRequest
{
   uint32_t originalRequestId;
   uint32_t serverRequestId;
   ClientSession *session;

   FileRequest(uint32_t _originalRequestId, uint32_t _serverRequestId, ClientSession *_session)
   {
      originalRequestId = _originalRequestId;
      serverRequestId = _serverRequestId;
      session = _session;
   }
};

/**
 * Outstanding file requests
 */
static ObjectArray<FileRequest> s_fileRequests(16, 16, Ownership::True);
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
private:
   void createObjectAccessSnapshot(NXCPMessage *request);

protected:
   virtual bool onMessage(NXCPMessage *msg) override;

   void processFileTransfer(NXCPMessage *msg);

public:
   RSConnector(const InetAddress& addr, uint16_t port) : ISC(addr, port)
   {
   }

   virtual void printMessage(const TCHAR *format, ...) override
   {
      va_list args;
      va_start(args, format);
      nxlog_debug_tag2(DEBUG_TAG, 6, format, args);
      va_end(args);
   }
};

/**
 * Custom handler for reporting server messages
 */
bool RSConnector::onMessage(NXCPMessage *msg)
{
   if (msg->getCode() == CMD_REQUEST_COMPLETED)
      return false;

   TCHAR buffer[128];
   printMessage(_T("Processing message %s"), NXCPMessageCodeName(msg->getCode(), buffer));

   switch(msg->getCode())
   {
      case CMD_RS_NOTIFY:
         NotifyClientSessions(*msg, nullptr);
         return true;
      case CMD_CREATE_OBJECT_ACCESS_SNAPSHOT:
         createObjectAccessSnapshot(msg);
         return true;
      case CMD_FILE_DATA:
      case CMD_ABORT_FILE_TRANSFER:
         processFileTransfer(msg);
         return true;
   }
   return false;
}

/**
 * Process file transfer messages
 */
void RSConnector::processFileTransfer(NXCPMessage *msg)
{
   MutexLock(s_fileRequestLock);
   for(int i = 0; i < s_fileRequests.size(); i++)
   {
      FileRequest *f = s_fileRequests.get(i);
      if (f->serverRequestId == msg->getId())
      {
         msg->setId(f->originalRequestId);
         f->session->sendMessage(msg);
         if (msg->isEndOfFile() || (msg->getCode() == CMD_ABORT_FILE_TRANSFER))
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("File request with ID %d removed from list"), f->originalRequestId);
            s_fileRequests.remove(i);
         }
         break;
      }
   }
   MutexUnlock(s_fileRequestLock);
}

/**
 * Process request for object access snapshot creation
 */
void RSConnector::createObjectAccessSnapshot(NXCPMessage *request)
{
   bool success = CreateObjectAccessSnapshot(request->getFieldAsUInt32(VID_USER_ID), request->getFieldAsInt32(VID_OBJECT_CLASS));
   NXCPMessage response(CMD_REQUEST_COMPLETED, request->getId());
   response.setField(VID_RCC, success ? RCC_SUCCESS : RCC_DB_FAILURE);
   sendMessage(&response);
}

/**
 * Reporting server connector instance
 */
static RSConnector *m_connector = nullptr;

/**
 * Reporting server connection manager
 */
void ReportingServerConnector()
{
	TCHAR hostname[256];
	ConfigReadStr(_T("ReportingServerHostname"), hostname, 256, _T("localhost"));
   uint16_t port = static_cast<uint16_t>(ConfigReadInt(_T("ReportingServerPort"), 4710));

	nxlog_debug_tag(DEBUG_TAG, 1, _T("Reporting Server connector started (%s:%d)"), hostname, port);

   // Keep connection open
   m_connector = new RSConnector(InetAddress::resolveHostName(hostname), port);
   while(!SleepAndCheckForShutdown(15))
   {
      if (m_connector->nop() != ISC_ERR_SUCCESS)
      {
         if (m_connector->connect(0) == ISC_ERR_SUCCESS)
         {
            nxlog_debug_tag(DEBUG_TAG, 2, _T("Connection to Reporting Server restored"));
         }
      }
	}
   m_connector->disconnect();
	delete m_connector;
   m_connector = NULL;

	nxlog_debug_tag(DEBUG_TAG, 1, _T("Reporting Server connector stopped"));
}

struct ViewBuilderContext
{
   StringBuffer *query;
   uint32_t userId;
   uint32_t tableCount;
};

static void ViewBuilderCallback(NetObj *object, ViewBuilderContext *context)
{
   if (object->isDataCollectionTarget() && object->checkAccessRights(context->userId, OBJECT_ACCESS_READ) && (static_cast<DataCollectionTarget*>(object)->getItemCount() > 0))
   {
      context->query->append(_T("SELECT * FROM idata_"));
      context->query->append(object->getId());
      context->query->append(_T(" UNION ALL "));
      context->tableCount++;
   }
}

/**
 * Prepare data view for reporting
 */
static bool PrepareReportingDataView(uint32_t userId, TCHAR *viewName)
{
   _sntprintf(viewName, MAX_OBJECT_NAME, _T("idata_view_") INT64_FMT, GetCurrentTimeMs());
   StringBuffer query(_T("CREATE VIEW "));
   query.append(viewName);
   query.append(_T(" AS "));

   if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
   {
      query.append(_T("SELECT * FROM idata"));
   }
   else
   {
      ViewBuilderContext context;
      context.query = &query;
      context.userId = userId;
      context.tableCount = 0;
      g_idxObjectById.forEach(ViewBuilderCallback, &context);
      if (context.tableCount == 0)
         return false;
      query.shrink(11); // Remove trailing "UNION ALL"
   }

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   bool success = DBQuery(hdb, query);
   DBConnectionPoolReleaseConnection(hdb);
   return success;
}

/**
 * Forward client message to reporting server
 */
NXCPMessage *ForwardMessageToReportingServer(NXCPMessage *request, ClientSession *session)
{
   if ((m_connector == nullptr) || !m_connector->connected())
      return nullptr;

   uint32_t originalId = request->getId();
   uint32_t rqId = m_connector->generateMessageId();
   request->setId(rqId);
   request->setField(VID_USER_ID, session->getUserId());

   TCHAR viewName[MAX_OBJECT_NAME];
   switch(request->getCode())
   {
      case CMD_RS_RENDER_RESULT: // File transfer requests
         MutexLock(s_fileRequestLock);
         s_fileRequests.add(new FileRequest(originalId, rqId, session));
         MutexUnlock(s_fileRequestLock);
         break;
      case CMD_RS_EXECUTE_REPORT:
         if (PrepareReportingDataView(session->getUserId(), viewName))
         {
            request->setField(VID_VIEW_NAME, viewName);
         }
         break;
   }

   NXCPMessage *reply = nullptr;
   if (m_connector->sendMessage(request))
   {
      reply = m_connector->waitForMessage(CMD_REQUEST_COMPLETED, request->getId(), 600000);
   }

   if (reply != nullptr)
   {
      reply->setId(originalId);
   }

   return reply;
}

/**
 * Scheduled task for report execution
 */
void ExecuteReport(const shared_ptr<ScheduledTaskParameters>& parameters)
{
   if ((m_connector == nullptr) || !m_connector->connected())
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("Cannot execute scheduled report \"%s\" (no connection with reporting server)"), parameters->m_comments);
      return;
   }

   NXCPMessage request(CMD_RS_EXECUTE_REPORT, m_connector->generateMessageId());
   request.setField(VID_USER_ID, parameters->m_userId);
   request.setField(VID_EXECUTION_PARAMETERS, parameters->m_persistentData);

   TCHAR viewName[MAX_OBJECT_NAME] = _T("");
   if (PrepareReportingDataView(parameters->m_userId, viewName))
      request.setField(VID_VIEW_NAME, viewName);

   if (m_connector->sendMessage(&request))
   {
      NXCPMessage *response = m_connector->waitForMessage(CMD_REQUEST_COMPLETED, request.getId(), 5000);
      if (response != nullptr)
      {
         uint32_t rcc = response->getFieldAsUInt32(VID_RCC);
         if (rcc == RCC_SUCCESS)
         {
            nxlog_debug_tag(DEBUG_TAG, 3, _T("Scheduled report \"%s\" execution started (job ID %s)"),
                     parameters->m_comments, response->getFieldAsGUID(VID_JOB_ID).toString().cstr());
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 3, _T("Cannot execute report \"%s\" (reporting server error %u)"), parameters->m_comments, rcc);
         }
         delete response;
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 3, _T("Cannot execute report \"%s\" (request timeout)"), parameters->m_comments);
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("Cannot execute report \"%s\" (communication failure)"), parameters->m_comments);
      if (viewName[0] != 0)
      {
         TCHAR query[256];
         _sntprintf(query, 256, _T("DROP VIEW %s"), viewName);
         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
         DBQuery(hdb, query);
         DBConnectionPoolReleaseConnection(hdb);
      }
   }
}
