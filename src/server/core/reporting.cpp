/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
#include <nms_users.h>

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
      session->incRefCount();
   }
   ~FileRequest()
   {
      session->decRefCount();
   }
};

/**
 * Outstanding file requests
 */
static ObjectArray<FileRequest> s_fileRequests(16, 16, Ownership::True);
static Mutex s_fileRequestLock;

/**
 * Remove all pending file transfer requests for session
 */
void RemovePendingFileTransferRequests(ClientSession *session)
{
   s_fileRequestLock.lock();
   for(int i = 0; i < s_fileRequests.size(); i++)
   {
      FileRequest *f = s_fileRequests.get(i);
      if (f->session == session)
      {
         s_fileRequests.remove(i);
         i--;
      }
   }
   s_fileRequestLock.unlock();
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

   void sendConfiguration();
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
         NotifyClientSessions(*msg);
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
   s_fileRequestLock.lock();
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
   s_fileRequestLock.unlock();
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
 * Send configuration to reporting server
 */
void RSConnector::sendConfiguration()
{
   NXCPMessage msg(CMD_CONFIGURE_REPORTING_SERVER, generateMessageId());
   msg.setField(VID_DB_DRIVER, g_szDbDriver);
   msg.setField(VID_DB_SERVER, g_szDbServer);
   msg.setField(VID_DB_NAME, g_szDbName);
   msg.setField(VID_DB_LOGIN, g_szDbLogin);
   msg.setField(VID_DB_PASSWORD, g_szDbPassword);

   TCHAR jdbcOptions[MAX_DB_STRING];
   ConfigReadStr(L"ReportingServer.JDBC.Properties", jdbcOptions, MAX_DB_STRING, L"");
   msg.setField(VID_JDBC_OPTIONS, jdbcOptions);

   msg.setField(VID_RETENTION_TIME, ConfigReadULong(L"ReportingServer.ResultsRetentionTime", 90));

   TCHAR channelName[MAX_OBJECT_NAME];
   if (ConfigReadStr(L"DefaultNotificationChannel.SMTP.Text", channelName, MAX_OBJECT_NAME, L"SMTP-Text"))
   {
      char *configuration = GetNotificationChannelConfiguration(channelName);
      if (configuration != nullptr)
      {
         Config parsedConfiguration;
         if (parsedConfiguration.loadConfigFromMemory(configuration, strlen(configuration), L"SMTP", nullptr, true, false))
         {
            const wchar_t *tlsMode = parsedConfiguration.getValue(L"/SMTP/TLSMode", L"NONE");
            msg.setField(VID_SMTP_SERVER, parsedConfiguration.getValue(L"/SMTP/Server"));
            msg.setField(VID_SMTP_PORT, parsedConfiguration.getValueAsUInt(L"/SMTP/Port", !wcsicmp(tlsMode, L"TLS") ? 465 : 25));
            msg.setField(VID_SMTP_LOCAL_HOSTNAME, parsedConfiguration.getValue(L"/SMTP/LocalHostName"));
            msg.setField(VID_SMTP_FROM_NAME, parsedConfiguration.getValue(L"/SMTP/FromName"));
            msg.setField(VID_SMTP_FROM_ADDRESS, parsedConfiguration.getValue(L"/SMTP/FromAddr"));
            msg.setField(VID_SMTP_LOGIN, parsedConfiguration.getValue(L"/SMTP/Login"));
            msg.setField(VID_SMTP_PASSWORD, parsedConfiguration.getValue(L"/SMTP/Password"));
            msg.setField(VID_SMTP_TLS_MODE, tlsMode);
         }
         else
         {
            printMessage(_T("Cannot parse configuration of notification channel \"%s\""), channelName);
         }
         MemFree(configuration);
      }
      else
      {
         printMessage(_T("Cannot get configuration of notification channel \"%s\""), channelName);
      }
   }
   else
   {
      printMessage(_T("Server configuration variable DefaultNotificationChannel.SMTP.Text is not set"));
   }

   sendMessage(&msg);
}

/**
 * Reporting server connector instance
 */
static RSConnector *s_connector = nullptr;

/**
 * Reporting server connection manager
 */
void ReportingServerConnector()
{
   TCHAR hostname[256];
   ConfigReadStr(_T("ReportingServer.Hostname"), hostname, 256, _T("127.0.0.1"));
   uint16_t port = static_cast<uint16_t>(ConfigReadInt(_T("ReportingServer.Port"), 4710));

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Reporting Server connector started (%s:%d)"), hostname, port);

   // Keep connection open
   s_connector = new RSConnector(InetAddress::resolveHostName(hostname), port);
   while(!SleepAndCheckForShutdown(15))
   {
      if (s_connector->nop() != ISC_ERR_SUCCESS)
      {
         if (s_connector->connect(0) == ISC_ERR_SUCCESS)
         {
            nxlog_debug_tag(DEBUG_TAG, 2, _T("Connection to Reporting Server restored"));
            s_connector->sendConfiguration();
         }
      }
	}
   s_connector->disconnect();
	delete_and_null(s_connector);

	nxlog_debug_tag(DEBUG_TAG, 1, _T("Reporting Server connector stopped"));
}

/**
 * Context for data view builder callback
 */
struct ViewBuilderContext
{
   StringBuffer *query;
   uint32_t userId;
   uint32_t tableCount;
};

/**
 * Data view builder callback
 */
static EnumerationCallbackResult ViewBuilderCallback(NetObj *object, ViewBuilderContext *context)
{
   if (object->isDataCollectionTarget() && object->checkAccessRights(context->userId, OBJECT_ACCESS_READ) && (static_cast<DataCollectionTarget*>(object)->getItemCount() > 0))
   {
      context->query->append(_T("SELECT * FROM idata_")).append(object->getId()).append(_T(" UNION ALL "));
      context->tableCount++;
   }
   return _CONTINUE;
}

/**
 * Prepare data view for reporting
 */
static bool PrepareReportingDataView(uint32_t userId, TCHAR *viewName)
{
   _sntprintf(viewName, MAX_OBJECT_NAME, _T("idata_view_") INT64_FMT, GetCurrentTimeMs());
   StringBuffer query(_T("CREATE VIEW "));
   query.append(viewName).append(_T(" AS "));

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
 * Check if data view is required for this report
 */
static bool IsDataViewRequired(const uuid& reportId)
{
   NXCPMessage request(CMD_RS_GET_REPORT_DEFINITION, s_connector->generateMessageId());
   request.setField(VID_REPORT_DEFINITION, reportId);
   if (!s_connector->sendMessage(&request))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("IsDataViewRequired(%s): cannot send request to reporting server"), reportId.toString().cstr());
      return false;
   }

   NXCPMessage *response = s_connector->waitForMessage(CMD_REQUEST_COMPLETED, request.getId(), 10000);
   if (response == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("IsDataViewRequired(%s): request timeout"), reportId.toString().cstr());
      return false;
   }

   bool required = response->getFieldAsBoolean(VID_REQUIRES_DATA_VIEW);
   delete response;

   nxlog_debug_tag(DEBUG_TAG, 4, _T("IsDataViewRequired(%s): data view is%s required"), reportId.toString().cstr(), required ? _T("") : _T(" not"));
   return required;
}

/**
 * Forward client message to reporting server
 */
NXCPMessage *ForwardMessageToReportingServer(NXCPMessage *request, ClientSession *session)
{
   if ((s_connector == nullptr) || !s_connector->connected())
      return nullptr;

   uint32_t originalId = request->getId();
   uint32_t rqId = s_connector->generateMessageId();
   request->setId(rqId);
   request->setField(VID_USER_ID, session->getUserId());

   TCHAR viewName[MAX_OBJECT_NAME];
   switch(request->getCode())
   {
      case CMD_RS_RENDER_RESULT: // File transfer requests
         s_fileRequestLock.lock();
         s_fileRequests.add(new FileRequest(originalId, rqId, session));
         s_fileRequestLock.unlock();
         break;
      case CMD_RS_EXECUTE_REPORT:
         if (IsDataViewRequired(request->getFieldAsGUID(VID_REPORT_DEFINITION)) && PrepareReportingDataView(session->getUserId(), viewName))
         {
            request->setField(VID_VIEW_NAME, viewName);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("Data view is not required or cannot be prepared for report %s"), request->getFieldAsGUID(VID_REPORT_DEFINITION).toString().cstr());
         }

         // Issue authentication token for reporting server (valid for 5 minutes)
         request->setField(VID_AUTH_TOKEN, IssueAuthenticationToken(session->getUserId(), 300, AuthenticationTokenType::SERVICE)->token.toString());
         break;
   }

   NXCPMessage *reply = nullptr;
   if (s_connector->sendMessage(request))
   {
      reply = s_connector->waitForMessage(CMD_REQUEST_COMPLETED, request->getId(), 600000);
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
   if ((s_connector == nullptr) || !s_connector->connected())
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("Cannot execute scheduled report \"%s\" (no connection with reporting server)"), parameters->m_comments);
      return;
   }

   NXCPMessage request(CMD_RS_EXECUTE_REPORT, s_connector->generateMessageId());
   request.setField(VID_USER_ID, parameters->m_userId);
   request.setField(VID_EXECUTION_PARAMETERS, parameters->m_persistentData);

   // Extract report ID from job configuration
   Config jobConfig;
   char *xml = UTF8StringFromWideString(parameters->m_persistentData);
   jobConfig.loadXmlConfigFromMemory(xml, strlen(xml), nullptr, "job", false);
   MemFree(xml);
   uuid reportId = jobConfig.getValueAsUUID(L"/reportId");
   nxlog_debug_tag(DEBUG_TAG, 3, L"Preparing execution of report %s (%s)", reportId.toString().cstr(), parameters->m_comments);

   // Prepare data view if needed
   wchar_t viewName[MAX_OBJECT_NAME] = L"";
   if (IsDataViewRequired(reportId) && PrepareReportingDataView(parameters->m_userId, viewName))
      request.setField(VID_VIEW_NAME, viewName);
   else
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Data view is not required or cannot be prepared for report %s"), reportId.toString().cstr());

   // Issue authentication token for reporting server (valid for 5 minutes)
   request.setField(VID_AUTH_TOKEN, IssueAuthenticationToken(parameters->m_userId, 300, AuthenticationTokenType::SERVICE)->token.toString());

   if (s_connector->sendMessage(&request))
   {
      NXCPMessage *response = s_connector->waitForMessage(CMD_REQUEST_COMPLETED, request.getId(), 5000);
      if (response != nullptr)
      {
         uint32_t rcc = response->getFieldAsUInt32(VID_RCC);
         if (rcc == RCC_SUCCESS)
         {
            nxlog_debug_tag(DEBUG_TAG, 3, L"Scheduled report \"%s\" execution started (job ID %s)",
                     parameters->m_comments, response->getFieldAsGUID(VID_TASK_ID).toString().cstr());
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 3, L"Cannot execute report \"%s\" (reporting server error %u)", parameters->m_comments, rcc);
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
