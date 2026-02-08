/*
** NetXMS Prometheus remote write receiver subagent
** Copyright (C) 2025 Raden Solutions
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
**/

#include "prometheus.h"
#include <microhttpd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static struct MHD_Daemon *s_daemon = nullptr;
static char s_endpoint[256];

/**
 * Connection data structure
 */
struct ConnectionData
{
   char *buffer;
   size_t size;
   size_t allocated;
};

/**
 * HTTP connection handler
 */
static MHD_Result HandleConnection(void *cls, struct MHD_Connection *connection,
                                   const char *url, const char *method,
                                   const char *version, const char *uploadData,
                                   size_t *uploadDataSize, void **conCls)
{
   if (*conCls == nullptr)
   {
      ConnectionData *connData = MemAllocStruct<ConnectionData>();
      connData->buffer = nullptr;
      connData->size = 0;
      connData->allocated = 0;
      *conCls = connData;
      return MHD_YES;
   }

   ConnectionData *connData = static_cast<ConnectionData*>(*conCls);

   if (strcmp(method, "POST") != 0)
   {
      const char *response = "Method not allowed";
      struct MHD_Response *mhdResponse = MHD_create_response_from_buffer(
         strlen(response), const_cast<char*>(response), MHD_RESPMEM_PERSISTENT);
      MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_METHOD_NOT_ALLOWED, mhdResponse);
      MHD_destroy_response(mhdResponse);
      MemFree(connData->buffer);
      MemFree(connData);
      *conCls = nullptr;
      return ret;
   }

   if (strcmp(url, s_endpoint) != 0)
   {
      const char *response = "Not found";
      struct MHD_Response *mhdResponse = MHD_create_response_from_buffer(
         strlen(response), const_cast<char*>(response), MHD_RESPMEM_PERSISTENT);
      MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, mhdResponse);
      MHD_destroy_response(mhdResponse);
      MemFree(connData->buffer);
      MemFree(connData);
      *conCls = nullptr;
      return ret;
   }

   if (*uploadDataSize != 0)
   {
      size_t newSize = connData->size + *uploadDataSize;
      if (newSize > connData->allocated)
      {
         size_t newAllocated = std::max(newSize, connData->allocated * 2);
         connData->buffer = static_cast<char*>(MemRealloc(connData->buffer, newAllocated));
         connData->allocated = newAllocated;
      }
      memcpy(connData->buffer + connData->size, uploadData, *uploadDataSize);
      connData->size = newSize;
      *uploadDataSize = 0;
      return MHD_YES;
   }

   const char *response;
   int statusCode;

   if (connData->size == 0)
   {
      response = "No data received";
      statusCode = MHD_HTTP_BAD_REQUEST;
   }
   else
   {
      if (HandleWriteRequest(connData->buffer, connData->size))
      {
         response = "OK";
         statusCode = MHD_HTTP_OK;
      }
      else
      {
         response = "Failed to process request";
         statusCode = MHD_HTTP_BAD_REQUEST;
      }
   }

   struct MHD_Response *mhdResponse = MHD_create_response_from_buffer(
      strlen(response), const_cast<char*>(response), MHD_RESPMEM_PERSISTENT);
   MHD_Result ret = MHD_queue_response(connection, statusCode, mhdResponse);
   MHD_destroy_response(mhdResponse);

   MemFree(connData->buffer);
   MemFree(connData);
   *conCls = nullptr;

   return ret;
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
   nxlog_debug_tag(DEBUG_TAG, 6, _T("MicroHTTPD: %hs"), buffer);
}

/**
 * Start HTTP receiver
 */
bool StartReceiver(const TCHAR *listenAddress, uint16_t port, const TCHAR *endpoint)
{
   tchar_to_utf8(endpoint, -1, s_endpoint, 256);

   InetAddress addr = InetAddress::parse(listenAddress);
   if (!addr.isValid())
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Invalid listen address: %s"), listenAddress);
      return false;
   }

   unsigned int flags = MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_POLL | MHD_USE_ERROR_LOG;
#if MHD_VERSION < 0x00097706
   if (addr.getFamily() == AF_INET6)
      flags |= MHD_USE_IPv6;
#endif

   SockAddrBuffer sa;
   addr.fillSockAddr(&sa, port);
   s_daemon = MHD_start_daemon(
         flags, port, nullptr, nullptr,
         &HandleConnection, nullptr,
         MHD_OPTION_EXTERNAL_LOGGER, Logger, nullptr,
#if MHD_VERSION >= 0x00097706
         MHD_OPTION_SOCK_ADDR_LEN, SA_LEN(reinterpret_cast<struct sockaddr*>(&sa)), &sa,
#else
         MHD_OPTION_SOCK_ADDR, reinterpret_cast<struct sockaddr*>(&sa),
#endif
         MHD_OPTION_END);
   if (s_daemon == nullptr)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Failed to start HTTP server on %s:%u"), listenAddress, port);
      return false;
   }

   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Listening on http://%s:%u%s"), listenAddress, port, endpoint);
   return true;
}

/**
 * Stop HTTP receiver
 */
void StopReceiver()
{
   if (s_daemon != nullptr)
   {
      MHD_stop_daemon(s_daemon);
      s_daemon = nullptr;
      nxlog_debug_tag(DEBUG_TAG, 1, _T("HTTP server stopped"));
   }
}
