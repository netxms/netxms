/*
** NetXMS - Network Management System
** Copyright (C) 2023-2025 Raden Solutions
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
** File: main.cpp
**
**/

#include "webapi.h"
#include <netxms-version.h>

/**
 * Module metadata
 */
DEFINE_MODULE_METADATA("WEBAPI", "Raden Solutions", NETXMS_VERSION_STRING_A, NETXMS_BUILD_TAG_A)

void CheckPendingLoginRequests(const shared_ptr<ScheduledTaskParameters>& parameters);

/**
 * Handlers
 */
int H_AlarmAcknowledge(Context *context);
int H_AlarmDetails(Context *context);
int H_AlarmResolve(Context *context);
int H_AlarmTerminate(Context *context);
int H_Alarms(Context *context);
int H_DataCollectionHistory(Context *context);
int H_FindMacAddress(Context *context);
int H_GrafanaGetAlarms(Context *context);
int H_GrafanaGetSummaryTable(Context *context);
int H_GrafanaGetObjectQuery(Context *context);
int H_Login(Context *context);
int H_Logout(Context *context);
int H_ObjectDetails(Context *context);
int H_ObjectExecuteAgentCommand(Context *context);
int H_ObjectExecuteScript(Context *context);
int H_ObjectQuery(Context *context);
int H_ObjectSetMaintenance(Context *context);
int H_ObjectSetManaged(Context *context);
int H_Objects(Context *context);
int H_ObjectSearch(Context *context);
int H_ObjectTools(Context *context);
int H_QueryAdHocSummaryTable(Context *context);
int H_QuerySummaryTable(Context *context);
int H_Root(Context *context);
int H_ServerInfo(Context *context);
int H_Status(Context *context);
int H_SummaryTables(Context *context);
int H_TakeScreenshot(Context *context);

/**
 * TCP port
 */
static uint16_t s_listenerPort = 8000;

/**
 * HTTPD instance
 */
static MHD_Daemon *s_daemon = nullptr;

/**
 * Send response to the client
 */
static inline MHD_Result SendResponse(MHD_Connection *connection, int responseCode, const StringMap *headers = nullptr, void *data = nullptr, size_t size  = 0)
{
   nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Response code %d to web API call"), responseCode);
   MHD_Response *response = MHD_create_response_from_buffer(size, data, MHD_RESPMEM_PERSISTENT);
   if (headers != nullptr)
   {
      headers->forEach(
         [response] (const TCHAR *key, const void *value) -> EnumerationCallbackResult
         {
            char header[128], content[1024];
            tchar_to_utf8(key, -1, header, sizeof(header));
            tchar_to_utf8(static_cast<const TCHAR*>(value), -1, content, sizeof(content));
            MHD_add_response_header(response, header, content);
            return _CONTINUE;
         });
   }
   MHD_Result rc = MHD_queue_response(connection, responseCode, response);
   MHD_destroy_response(response);
   return rc;
}

/**
 * Connection handler
 */
static MHD_Result ConnectionHandler(void *serverContext, MHD_Connection *connection, const char *url, const char *method, const char *version, const char *uploadData, size_t *uploadDataSize, void **connectionContext)
{
   Context *context = static_cast<Context*>(*connectionContext);
   if (context == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Web API request: %hs %hs"), method, url);
      if (nxlog_get_debug_level_tag(DEBUG_TAG_WEBAPI) >= 7)
         MHD_get_connection_values(connection, MHD_HEADER_KIND,
            [] (void *context, MHD_ValueKind kind, const char *key, const char *value)
            {
               nxlog_debug_tag(DEBUG_TAG_WEBAPI, 7, _T("   %hs: %hs"), key, value);
               return MHD_YES;
            }, nullptr);

      int responseCode;
      context = RouteRequest(connection, url, method, &responseCode);
      if (context == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Cannot route request for %hs \"%hs\" (response code %d)"), method, url, responseCode);
         return SendResponse(connection, responseCode);
      }

      *connectionContext = context;
      if (context->isDataRequired())
         return MHD_YES;
   }

   nxlog_debug_tag(DEBUG_TAG_WEBAPI, 7, _T("Request data size: %d"), static_cast<int>(*uploadDataSize));
   if (*uploadDataSize != 0)
   {
      context->onUploadData(uploadData, *uploadDataSize);
      *uploadDataSize = 0;
      return MHD_YES;
   }

   context->onUploadComplete();
   int responseCode = context->invokeHandler();
   return SendResponse(connection, responseCode, context->getResponseHeaders(), context->getResponseData(), context->getResponseDataSize());
}

/**
 * Request completion handler
 */
static void RequestCompleted(void *context, MHD_Connection *connection, void **connectionContext, MHD_RequestTerminationCode tcode)
{
   nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Web API request completed (completion code %d)"), tcode);
   delete static_cast<Context*>(*connectionContext);
   *connectionContext = nullptr;
}

/**
 * Custom logger
 */
static void Logger(void *context, const char *format, va_list args)
{
   char buffer[8192];
   vsnprintf(buffer, 8192, format, args);
   nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("MicroHTTPD: %hs"), buffer);
}

/**
 * Initialize module
 */
static bool InitModule(Config *config)
{
   s_listenerPort = static_cast<uint16_t>(config->getValueAsInt(_T("/WEBAPI/ListenerPort"), 8000));

   RouteBuilder("")
      .GET(H_Root)
      .noauth()
      .build();
   RouteBuilder("v1/alarms")
      .GET(H_Alarms)
      .build();
   RouteBuilder("v1/alarms/:alarm-id")
      .GET(H_AlarmDetails)
      .build();
   RouteBuilder("v1/alarms/:alarm-id/acknowledge")
      .POST(H_AlarmAcknowledge)
      .build();
   RouteBuilder("v1/alarms/:alarm-id/resolve")
      .POST(H_AlarmResolve)
      .build();
   RouteBuilder("v1/alarms/:alarm-id/terminate")
      .POST(H_AlarmTerminate)
      .build();
   RouteBuilder("v1/dci-summary-tables")
      .GET(H_SummaryTables)
      .build();
   RouteBuilder("v1/dci-summary-tables/:table-id/query")
      .POST(H_QuerySummaryTable)
      .build();
   RouteBuilder("v1/dci-summary-tables/adhoc-query")
      .POST(H_QueryAdHocSummaryTable)
      .build();
   RouteBuilder("v1/find/mac-address")
      .GET(H_FindMacAddress)
      .build();
   RouteBuilder("v1/grafana/infinity/alarms")
      .POST(H_GrafanaGetAlarms)
      .build();
   RouteBuilder("v1/grafana/infinity/object-query")
      .POST(H_GrafanaGetObjectQuery)
      .build();
   RouteBuilder("v1/grafana/infinity/summary-table")
      .POST(H_GrafanaGetSummaryTable)
      .build();
   RouteBuilder("v1/login")
      .POST(H_Login)
      .noauth()
      .build();
   RouteBuilder("v1/logout")
      .POST(H_Logout)
      .build();
   RouteBuilder("v1/objects")
      .GET(H_Objects)
      .build();
   RouteBuilder("v1/objects/:object-id")
      .GET(H_ObjectDetails)
      .build();
   RouteBuilder("v1/objects/:object-id/data-collection/:dci-id/history")
      .GET(H_DataCollectionHistory)
      .build();
   RouteBuilder("v1/objects/:object-id/execute-agent-command")
      .POST(H_ObjectExecuteAgentCommand)
      .build();
   RouteBuilder("v1/objects/:object-id/execute-script")
      .POST(H_ObjectExecuteScript)
      .build();
   RouteBuilder("v1/objects/:object-id/set-maintenance")
      .POST(H_ObjectSetMaintenance)
      .build();
   RouteBuilder("v1/objects/:object-id/set-managed")
      .POST(H_ObjectSetManaged)
      .build();
   RouteBuilder("v1/objects/:object-id/take-screenshot")
      .GET(H_TakeScreenshot)
      .build();
   RouteBuilder("v1/objects/query")
      .POST(H_ObjectQuery)
      .build();
   RouteBuilder("v1/objects/search")
      .POST(H_ObjectSearch)
      .build();
   RouteBuilder("v1/object-tools")
      .GET(H_ObjectTools)
      .build();
   RouteBuilder("v1/server-info")
      .GET(H_ServerInfo)
      .build();
   RouteBuilder("v1/status")
      .GET(H_Status)
      .build();

   return true;
}

/**
 * Server start handler
 */
static void OnServerStart()
{
   nxlog_debug_tag(DEBUG_TAG_WEBAPI, 2, _T("Starting web API server"));
   s_daemon = MHD_start_daemon(
         MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_POLL | MHD_USE_ERROR_LOG,
         s_listenerPort, nullptr, nullptr,
         ConnectionHandler, nullptr,
         MHD_OPTION_EXTERNAL_LOGGER, Logger, nullptr,
         MHD_OPTION_NOTIFY_COMPLETED, RequestCompleted, nullptr,
         MHD_OPTION_END);
   if (s_daemon != nullptr)
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_WEBAPI, _T("Web API initialized on port %u"), s_listenerPort);
   else
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_WEBAPI, _T("Web API initialization failed (MicroHTTPD initialization error)"));

   RegisterSchedulerTaskHandler(_T("WebAPI.CheckPendingLoginRequests"), CheckPendingLoginRequests, 0); //No access right because it will be used only by server
   AddUniqueRecurrentScheduledTask(_T("WebAPI.CheckPendingLoginRequests"), _T("*/2 * * * *"), _T(""), nullptr, 0, 0, SYSTEM_ACCESS_FULL, _T("Check for expired login requests"), nullptr, true);
}

/**
 * Shutdown module
 */
static void ShutdownModule()
{
   nxlog_debug_tag(DEBUG_TAG_WEBAPI, 2, _T("Waiting for web API server to stop"));
   MHD_stop_daemon(s_daemon);
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_WEBAPI, _T("Web API server stopped"));
}

/**
 * Module entry point
 */
extern "C" bool __EXPORT NXM_Register(NXMODULE *module, Config *config)
{
   module->dwSize = sizeof(NXMODULE);
   _tcscpy(module->szName, _T("WEBAPI"));
   module->pfInitialize = InitModule;
   module->pfServerStarted = OnServerStart;
   module->pfShutdown = ShutdownModule;
   return true;
}

#ifdef _WIN32

/**
 * DLL Entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
