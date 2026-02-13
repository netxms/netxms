/*
** NetXMS - Network Management System
** Copyright (C) 2023-2026 Raden Solutions
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

#define WS_GUID         "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WS_GUID_LEN     36
#define WS_KEY_LEN      24
#define WS_KEY_GUID_LEN ((WS_KEY_LEN) + (WS_GUID_LEN))

Context *RouteRequest(MHD_Connection *connection, const char *path, const char *method, int *responseCode);
void CheckPendingLoginRequests(const shared_ptr<ScheduledTaskParameters>& parameters);

int H_Login(Context *context);
int H_Logout(Context *context);

bool ReadTLSConfiguration();
void StartReproxy(uint16_t webApiPort, const InetAddress& webApiAddress);
void StopReproxy();

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
static inline MHD_Result SendResponse(MHD_Connection *connection, int responseCode, const char *contentType, const StringMap *headers = nullptr, void *data = nullptr, size_t size  = 0)
{
   nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Response code %d to web API call"), responseCode);
   MHD_Response *response = MHD_create_response_from_buffer(size, data, MHD_RESPMEM_PERSISTENT);

   // Add security headers
   MHD_add_response_header(response, "X-Content-Type-Options", "nosniff");
   MHD_add_response_header(response, "X-Frame-Options", "DENY");
   MHD_add_response_header(response, "Cache-Control", "no-store");
   MHD_add_response_header(response, "Content-Type", (contentType != nullptr) && (contentType[0] != 0) ? contentType : "application/json; charset=utf-8");

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
 * Calculate value for MHD_HTTP_HEADER_SEC_WEBSOCKET_ACCEPT
 */
static bool WSCalculateAcceptValue(MHD_Connection *connection, MHD_Response *response)
{
   const char *key = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_SEC_WEBSOCKET_KEY);
   if ((key == nullptr) || (strlen(key) != WS_KEY_LEN))
      return false;

   BYTE keyResponse[WS_KEY_GUID_LEN];
   memcpy(keyResponse, key, WS_KEY_LEN);
   memcpy(&keyResponse[WS_KEY_LEN], WS_GUID, WS_GUID_LEN);

   BYTE hash[SHA1_DIGEST_SIZE];
   CalculateSHA1Hash(keyResponse, WS_KEY_GUID_LEN, hash);

   char encodedHash[SHA1_DIGEST_SIZE * 2];
   base64_encode(reinterpret_cast<char*>(hash), SHA1_DIGEST_SIZE, encodedHash, sizeof(encodedHash));
   MHD_add_response_header(response, MHD_HTTP_HEADER_SEC_WEBSOCKET_ACCEPT, encodedHash);
   return true;
}

/**
 * Read websocket frame
 */
bool NXCORE_EXPORTABLE ReadWebsocketFrame(SOCKET socketHandle, ByteStream *out, BYTE *frameType)
{
   BYTE buffer[16];

   // Read the first 2 bytes to get the frame header
   if (!RecvAll(socketHandle, buffer, 2, INFINITE))
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 7, L"Error reading websocket frame header");
      return false;
   }

   // Parse header
   *frameType = buffer[0] & 0x0F;
   bool masked = ((buffer[1] & 0x80) != 0);
   size_t payloadLength = buffer[1] & 0x7F;

   // Check masking
   if (!masked && ((*frameType == 1) || (*frameType == 2)))
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 7, L"Received unmasked websocket data frame from client");
      return false;
   }

   // Read extended payload length if needed
   if (payloadLength == 126)
   {
      if (!RecvAll(socketHandle, buffer, 2, 1000))
      {
         nxlog_debug_tag(DEBUG_TAG_WEBAPI, 7, L"Error reading websocket frame payload length");
         return false;
      }
      payloadLength = (static_cast<size_t>(buffer[0]) << 8) | static_cast<size_t>(buffer[1]);
   }
   else if (payloadLength == 127)
   {
      if (!RecvAll(socketHandle, buffer, 8, 1000))
      {
         nxlog_debug_tag(DEBUG_TAG_WEBAPI, 7, L"Error reading websocket frame payload length");
         return false;
      }
      payloadLength = ntohq(*reinterpret_cast<uint64_t*>(buffer));
   }

   // Limit maximum frame size to 4MB to prevent memory exhaustion
   if (payloadLength > 4 * 1024 * 1024)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 7, L"WebSocket frame too large (%zu bytes)", payloadLength);
      return false;
   }

   // Read the masking key if the frame is masked
   BYTE mask[4];
   if (masked && !RecvAll(socketHandle, mask, 4, 1000))
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 7, L"Error reading websocket frame payload mask");
      return false;
   }

   // Read the payload data
   Buffer<BYTE, 4096> payload_data(payloadLength);
   if (!RecvAll(socketHandle, payload_data, payloadLength, 1000))
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 7, L"Error reading websocket frame payload");
      return false;
   }

   // Unmask the payload data
   for (size_t i = 0; i < payloadLength; i++)
      payload_data[i] ^= mask[i % 4];

   out->write(payload_data, payloadLength);
   return true;
}

/**
 * Send websocket frame
 */
void NXCORE_EXPORTABLE SendWebsocketFrame(SOCKET socketHandle, const void *data, size_t dataLen)
{
   size_t headerLen;
   BYTE header[16];
   header[0] = 0x81; // FIN + text frame
   if (dataLen <= 125)
   {
      header[1] = static_cast<BYTE>(dataLen);
      headerLen = 2;
   }
   else if (dataLen <= 65535)
   {
      header[1] = 126; // 2 bytes payload length
      header[2] = (dataLen >> 8) & 0xFF;
      header[3] = dataLen & 0xFF;
      headerLen = 4;
   }
   else
   {
      header[1] = 127; // 8 bytes payload length
      uint64_t v = htonq(dataLen);
      memcpy(&header[2], &v, 8);
      headerLen = 10;
   }

   Buffer<BYTE, 4096> frame(dataLen + headerLen);
   memcpy(frame, header, headerLen);
   memcpy(frame + headerLen, data, dataLen);
   SendEx(socketHandle, frame, dataLen + headerLen, 0, nullptr);
}

/**
 * Send websocket binary frame
 */
void NXCORE_EXPORTABLE SendWebsocketBinaryFrame(SOCKET socketHandle, const void *data, size_t dataLen)
{
   size_t headerLen;
   BYTE header[16];
   header[0] = 0x82; // FIN + binary frame
   if (dataLen <= 125)
   {
      header[1] = static_cast<BYTE>(dataLen);
      headerLen = 2;
   }
   else if (dataLen <= 65535)
   {
      header[1] = 126; // 2 bytes payload length
      header[2] = (dataLen >> 8) & 0xFF;
      header[3] = dataLen & 0xFF;
      headerLen = 4;
   }
   else
   {
      header[1] = 127; // 8 bytes payload length
      uint64_t v = htonq(dataLen);
      memcpy(&header[2], &v, 8);
      headerLen = 10;
   }

   Buffer<BYTE, 4096> frame(dataLen + headerLen);
   memcpy(frame, header, headerLen);
   memcpy(frame + headerLen, data, dataLen);
   SendEx(socketHandle, frame, dataLen + headerLen, 0, nullptr);
}

/**
 * Send websocket "close" frame with status code (RFC 6455 Section 5.5.1)
 */
void NXCORE_EXPORTABLE SendWebsocketCloseFrame(SOCKET socketHandle, uint16_t statusCode)
{
   BYTE frame[4];
   frame[0] = 0x88; // FIN + close frame
   frame[1] = 2;    // Payload length (2 bytes for status code)
   frame[2] = static_cast<BYTE>((statusCode >> 8) & 0xFF);  // Status code high byte (network byte order)
   frame[3] = static_cast<BYTE>(statusCode & 0xFF);         // Status code low byte
   SendEx(socketHandle, frame, 4, 0, nullptr);
}

/**
 * Upgrade connection to websocket
 */
static MHD_Result UpgradeToWebsocket(Context *context, MHD_Connection *connection, const char *url)
{
   if (!context->isProtocolUpgradeAllowed())
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 5, L"Upgrade to websocket is not allowed for endpoint %hs", url);
      context->setErrorResponse("Upgrade to websocket is not allowed for this endpoint");
      return SendResponse(connection, 400, context->getContentType(), context->getResponseHeaders(), context->getResponseData(), context->getResponseDataSize());
   }

   MHD_Response *response = MHD_create_response_for_upgrade(context->getProtocolUpgradeHandler(), context);
   if (!WSCalculateAcceptValue(connection, response))
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 5, L"Upgrade to websocket failed for endpoint %hs (invalid websocket key format)", url);
      context->setErrorResponse("Invalid websocket key format");
      return SendResponse(connection, 400, context->getContentType(), context->getResponseHeaders(), context->getResponseData(), context->getResponseDataSize());
   }

   nxlog_debug_tag(DEBUG_TAG_WEBAPI, 5, L"Upgrade to websocket for endpoint %hs approved", url);
   MHD_add_response_header(response, MHD_HTTP_HEADER_UPGRADE, "websocket");
   MHD_Result rc = MHD_queue_response(connection, MHD_HTTP_SWITCHING_PROTOCOLS, response);
   MHD_destroy_response(response);
   return rc;
}

/**
 * Check if current request is for upgrading to websocket
 */
static inline bool IsWebsocketUpgradeRequest(MHD_Connection *connection)
{
   const char *ch = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_CONNECTION);
   if ((ch == nullptr) || (stristr(ch, "Upgrade") == nullptr))
      return false;

   const char *upgrade = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_UPGRADE);
   return (upgrade != nullptr) && !stricmp(upgrade, "websocket");
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
         return SendResponse(connection, responseCode, nullptr);
      }

      *connectionContext = context;
      if (context->isDataRequired())
         return MHD_YES;
   }

   nxlog_debug_tag(DEBUG_TAG_WEBAPI, 7, _T("Request data size: %d"), static_cast<int>(*uploadDataSize));
   if (*uploadDataSize != 0)
   {
      if (!context->onUploadData(uploadData, *uploadDataSize))
      {
         return SendResponse(connection, 413, context->getContentType());  // Payload Too Large
      }
      *uploadDataSize = 0;
      return MHD_YES;
   }

   context->onUploadComplete();

   if (IsWebsocketUpgradeRequest(connection))
      return UpgradeToWebsocket(context, connection, url);

   int responseCode = context->invokeHandler();
   return SendResponse(connection, responseCode, context->getContentType(), context->getResponseHeaders(), context->getResponseData(), context->getResponseDataSize());
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

   if (!ReadTLSConfiguration())
      return false;

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

   unsigned int flags = MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_POLL | MHD_USE_ERROR_LOG | MHD_ALLOW_UPGRADE;
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

   StartReproxy(s_listenerPort, s_listenerAddr);
}

/**
 * Shutdown web API
 */
void ShutdownWebAPI()
{
   StopReproxy();

   if (!s_webApiEnabled || (s_daemon == nullptr))
      return;

   nxlog_debug_tag(DEBUG_TAG_WEBAPI, 2, L"Waiting for web API server to stop");
   MHD_stop_daemon(s_daemon);
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_WEBAPI, L"Web API server stopped");
}
