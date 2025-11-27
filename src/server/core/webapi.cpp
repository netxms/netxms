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
** File: webapi.cpp
**
**/

#include "nxcore.h"
#include <netxms-webapi.h>
#include <netxms-version.h>

Context *RouteRequest(MHD_Connection *connection, const char *path, const char *method, int *responseCode);
void CheckPendingLoginRequests(const shared_ptr<ScheduledTaskParameters>& parameters);

int H_Login(Context *context);
int H_Logout(Context *context);

/**
 * Loaded server config
 */
extern Config g_serverConfig;

/**
 * TCP address and port
 */
static InetAddress s_listenerAddr = InetAddress::LOOPBACK;
static uint16_t s_listenerPort = 8000;
static bool s_webApiEnabled = true;

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
   size_t last = strlen(buffer) - 1;
   if (buffer[last] == '\n')
      buffer[last] = 0;
   nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("MicroHTTPD: %hs"), buffer);
}

/**
 * Handler for /
 */
static int H_Root(Context *context)
{
   json_t *response = json_object();
   json_object_set_new(response, "description", json_string("NetXMS web service API"));
   json_object_set_new(response, "version", json_string(NETXMS_VERSION_STRING_A));
   json_object_set_new(response, "build", json_string(NETXMS_BUILD_TAG_A));
   json_object_set_new(response, "apiVersion", json_integer(1));
   context->setResponseData(response);
   json_decref(response);
   return 200;
}

/**
 * Initialize web API
 */
bool InitWebAPI()
{
   s_webApiEnabled = g_serverConfig.getValueAsBoolean(_T("/WEBAPI/Enable"), true);
   if (!s_webApiEnabled)
   {
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_WEBAPI, _T("Web API is disabled by configuration"));
      return true;
   }

   const wchar_t *addr = g_serverConfig.getValue(L"/WEBAPI/Address", L"");
   if (!wcsicmp(addr, L"loopback") || !wcsicmp(addr, L"localhost"))
   {
      s_listenerAddr = InetAddress::LOOPBACK;
   }
   else if (!wcsicmp(addr, L"any") || !wcscmp(addr, L"*"))
   {
      s_listenerAddr = InetAddress::NONE;  // Bind to all interfaces
   }
   else if (addr[0] != 0)
   {
      InetAddress a = InetAddress::parse(addr);
      if (a.isValid())
      {
         s_listenerAddr = a;
      }
      else
      {
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_WEBAPI, _T("Invalid web API listener address %s"), addr);
         return false;
      }
   }
   s_listenerPort = static_cast<uint16_t>(g_serverConfig.getValueAsInt(_T("/WEBAPI/Port"), 8000));

   RouteBuilder("")
      .GET(H_Root)
      .noauth()
      .build();
   RouteBuilder("v1/login")
      .POST(H_Login)
      .noauth()
      .build();
   RouteBuilder("v1/logout")
      .POST(H_Logout)
      .build();

   return true;
}

/**
 * Start web API
 */
void StartWebAPI()
{
   if (!s_webApiEnabled)
      return;

   nxlog_debug_tag(DEBUG_TAG_WEBAPI, 2, _T("Starting web API server"));

   if (s_listenerAddr.isLoopback())
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 2, _T("Web API will listen on loopback address only"));
   else if (s_listenerAddr.isAnyLocal())
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 2, _T("Web API will listen on all available interfaces"));
   else
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 2, _T("Web API will listen on address %s"), s_listenerAddr.toString().cstr());

   unsigned int flags = MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_POLL | MHD_USE_ERROR_LOG;
#if MHD_VERSION < 0x00097706
   if (s_listenerAddr.getFamily() == AF_INET6)
      flags |= MHD_USE_IPv6;
#endif

   SockAddrBuffer sa;
   s_listenerAddr.fillSockAddr(&sa, s_listenerPort);
   s_daemon = MHD_start_daemon(
         flags, s_listenerPort, nullptr, nullptr,
         ConnectionHandler, nullptr,
         MHD_OPTION_EXTERNAL_LOGGER, Logger, nullptr,
         MHD_OPTION_NOTIFY_COMPLETED, RequestCompleted, nullptr,
#if MHD_VERSION >= 0x00097706
         MHD_OPTION_SOCK_ADDR_LEN, SA_LEN(reinterpret_cast<struct sockaddr*>(&sa)), &sa,
#else
         MHD_OPTION_SOCK_ADDR, reinterpret_cast<struct sockaddr*>(&sa),
#endif
         MHD_OPTION_END);
   if (s_daemon != nullptr)
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_WEBAPI, _T("Web API initialized on %s:%u"), s_listenerAddr.toString().cstr(), s_listenerPort);
   else
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_WEBAPI, _T("Web API initialization failed (MicroHTTPD initialization error)"));

   RegisterSchedulerTaskHandler(_T("WebAPI.CheckPendingLoginRequests"), CheckPendingLoginRequests, 0); //No access right because it will be used only by server
   AddUniqueRecurrentScheduledTask(_T("WebAPI.CheckPendingLoginRequests"), _T("*/2 * * * *"), _T(""), nullptr, 0, 0, SYSTEM_ACCESS_FULL, _T("Check for expired login requests"), nullptr, true);
}

/**
 * Shutdown web API
 */
void ShutdownWebAPI()
{
   if (!s_webApiEnabled || (s_daemon == nullptr))
      return;

   nxlog_debug_tag(DEBUG_TAG_WEBAPI, 2, L"Waiting for web API server to stop");
   MHD_stop_daemon(s_daemon);
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_WEBAPI, L"Web API server stopped");
}
