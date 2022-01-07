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
static void SaveResponse(const InetAddress& ip, char* buffer, char* hostname = nullptr)
{
   if (g_szFailedDir[0] == 0)
      return;

   time_t now = time(nullptr);
   char fileName[2048];
   char tmp[64];
   snprintf(fileName, 2048, "%s%s%s-%d",
            g_szFailedDir, FS_PATH_SEPARATOR_A,
            hostname != nullptr ? hostname : ip.toStringA(tmp),
            (int)now);
   FILE* f = fopen(fileName, "wb");
   if (f != NULL)
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
   LONG ret = SYSINFO_RC_SUCCESS;

   char host[1024];
   char portAsChar[1024];
   char uri[1024];
   char header[1024];
   char match[1024];
   uint16_t port = 0;

   AgentGetParameterArgA(param, 1, host, sizeof(host));
   AgentGetParameterArgA(param, 2, portAsChar, sizeof(portAsChar) / sizeof(TCHAR));
   AgentGetParameterArgA(param, 3, uri, sizeof(uri));
   AgentGetParameterArgA(param, 4, header, sizeof(header));
   AgentGetParameterArgA(param, 5, match, sizeof(match));

   if (host[0] == 0 || uri[0] == 0)
      return SYSINFO_RC_ERROR;

   if (portAsChar[0] == 0)
   {
      port = (arg[1] == 'S') ? 443 : 80;
   }
   else
   {
      port = ParsePort(portAsChar);
      if (port == 0)
         return SYSINFO_RC_UNSUPPORTED;
   }

   uint32_t timeout = GetTimeoutFromArgs(param, 6);
   int64_t start = GetCurrentTimeMs();

   int result = CheckHTTP(arg[1] == 'S', InetAddress::resolveHostName(host), port, uri, header, match, timeout, host);
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
int CheckHTTP(bool tls, const InetAddress& addr, short port, char* uri, char* header, char* match, uint32_t timeout, char* hostname)
{
   int ret = 0;
   TLSConnection tc(SUBAGENT_DEBUG_TAG, false, timeout);

   if (match[0] == 0)
   {
      strcpy(match, "^HTTP/(1\\.[01]|2) 200 .*");
   }

   const char* errptr;
   int erroffset;
   pcre* preg = pcre_compile(match, PCRE_COMMON_FLAGS_A | PCRE_CASELESS, &errptr, &erroffset, NULL);
   if (preg == NULL)
   {
      return PC_ERR_BAD_PARAMS;
   }

   if (tc.connect(addr, port, tls, timeout))
   {
      char tmp[4096];
      char hostHeader[4096];

      ret = PC_ERR_HANDSHAKE;

      char addrAsChar[1024];

      snprintf(hostHeader, sizeof(hostHeader), "Host: %s:%u\r\n", header[0] != 0 ? header : addr.toStringA(addrAsChar), port);

      snprintf(tmp, sizeof(tmp),
               "GET %s HTTP/1.1\r\nConnection: close\r\nAccept: */*\r\n%s\r\n",
               uri, hostHeader);

      if (tc.send(tmp, strlen(tmp)))
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
