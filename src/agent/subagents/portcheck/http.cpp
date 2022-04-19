/*
** NetXMS - Network Management System
** Copyright (C) 2003-2021 Raden Solutions
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
** File: http.cpp
**
**/

#include "portcheck.h"
#include <netxms-regex.h>
#include <tls_conn.h>

/**
 * Save HTTP(s) responce to file for later investigation
 * (Should be enabled by setting "FailedDirectory" in config
 */
static void SaveResponse(const InetAddress& ip, const char *buffer, const char *hostname = nullptr)
{
   if (g_szFailedDir[0] == 0)
      return;

   char fileName[MAX_PATH], ipAddrText[64];
   snprintf(fileName, MAX_PATH, "%s%s%s-%u", g_szFailedDir, FS_PATH_SEPARATOR_A, (hostname != nullptr) ? hostname : ip.toStringA(ipAddrText), (uint32_t)time(nullptr));
   FILE* f = fopen(fileName, "wb");
   if (f != nullptr)
   {
      fwrite(buffer, strlen(buffer), 1, f);
      fclose(f);
   }
}

/**
 * Check HTTP/HTTPS service - parameter handler
 */
LONG H_CheckHTTP(const TCHAR* param, const TCHAR* arg, TCHAR* value, AbstractCommSession* session)
{
   char hostname[1024], portText[32], uri[1024], header[256], match[1024];
   AgentGetParameterArgA(param, 1, hostname, sizeof(hostname));
   AgentGetParameterArgA(param, 2, portText, sizeof(portText));
   AgentGetParameterArgA(param, 3, uri, sizeof(uri));
   AgentGetParameterArgA(param, 4, header, sizeof(header));
   AgentGetParameterArgA(param, 5, match, sizeof(match));

   if (hostname[0] == 0 || uri[0] == 0)
      return SYSINFO_RC_ERROR;

   uint16_t port = 0;
   if (portText[0] == 0)
   {
      port = (arg[1] == 'S') ? 443 : 80;
   }
   else
   {
      port = ParsePort(portText);
      if (port == 0)
         return SYSINFO_RC_UNSUPPORTED;
   }

   uint32_t timeout = GetTimeoutFromArgs(param, 6);
   int64_t start = GetCurrentTimeMs();

   LONG ret = SYSINFO_RC_SUCCESS;
   int result = CheckHTTP(hostname, InetAddress::resolveHostName(hostname), port, arg[1] == 'S', uri, header, match, timeout);
   if (*arg == 'R')
   {
      if (result == PC_ERR_NONE)
         ret_int64(value, GetCurrentTimeMs() - start);
      else if (g_serviceCheckFlags & SCF_NEGATIVE_TIME_ON_ERROR)
         ret_int(value, -result);
      else
         ret = SYSINFO_RC_ERROR;
   }
   else
   {
      ret_int(value, result);
   }
   return ret;
}

/**
 * Check HTTP/HTTPS service
 */
int CheckHTTP(const char *hostname, const InetAddress& addr, uint16_t port, bool useTLS, const char* uri, const char* header, const char* match, uint32_t timeout)
{
   int ret = 0;
   TLSConnection tc(SUBAGENT_DEBUG_TAG, false, timeout);

   if (match[0] == 0)
      match = "^HTTP/(1\\.[01]|2) 200 .*";

   const char* errptr;
   int erroffset;
   pcre* preg = pcre_compile(match, PCRE_COMMON_FLAGS_A | PCRE_CASELESS, &errptr, &erroffset, nullptr);
   if (preg == nullptr)
   {
      return PC_ERR_BAD_PARAMS;
   }

   if (tc.connect(addr, port, useTLS, timeout, hostname))
   {
      ret = PC_ERR_HANDSHAKE;

      char request[4096], hostHeader[1024], ipAddrText[64];
      snprintf(hostHeader, sizeof(hostHeader), "Host: %s:%u\r\n", (header[0] != 0) ? header : addr.toStringA(ipAddrText), port);
      snprintf(request, sizeof(request), "GET %s HTTP/1.1\r\nConnection: close\r\nAccept: */*\r\n%s\r\n", uri, hostHeader);
      if (tc.send(request, strlen(request)))
      {
#define CHUNK_SIZE 10240
         char* buff = (char*)MemAlloc(CHUNK_SIZE);
         ssize_t offset = 0;
         ssize_t buffSize = CHUNK_SIZE;

         while (tc.poll(5000))
         {
            ssize_t bytes = tc.recv(buff + offset, buffSize - offset);
            if (bytes > 0)
            {
               offset += bytes;
               if (buffSize - offset < (CHUNK_SIZE / 2))
               {
                  char* tmp = (char*)realloc(buff, buffSize + CHUNK_SIZE);
                  if (tmp != NULL)
                  {
                     buffSize += CHUNK_SIZE;
                     buff = tmp;
                  }
                  else
                  {
                     MemFreeAndNull(buff);
                     break;
                  }
               }
            }
            else
            {
               break;
            }
         }

         if (buff != NULL && offset > 0)
         {
            buff[offset] = 0;

            int ovector[30];
            if (pcre_exec(preg, nullptr, buff, static_cast<int>(strlen(buff)), 0, 0, ovector, 30) >= 0)
            {
               ret = PC_ERR_NONE;
            }
            else
            {
               SaveResponse(addr, buff, hostname);
            }
         }
         MemFree(buff);
      }
   }
   else
   {
      ret = PC_ERR_CONNECT;
   }

   pcre_free(preg);
   return ret;
}
