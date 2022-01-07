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
** File: pop3.cpp
**
**/

#include "portcheck.h"
#include <tls_conn.h>

/**
 * Check POP3/POP3S service - parameter handler
 */
LONG H_CheckPOP3(const TCHAR* param, const TCHAR* arg, TCHAR* value, AbstractCommSession* session)
{
   LONG ret = SYSINFO_RC_SUCCESS;
   char host[256];
   char user[256];
   char pass[256];
   char portAsChar[256];
   uint16_t port;

   AgentGetParameterArgA(param, 1, host, sizeof(host));
   AgentGetParameterArgA(param, 2, user, sizeof(user));
   AgentGetParameterArgA(param, 3, pass, sizeof(pass));
   uint32_t timeout = GetTimeoutFromArgs(param, 4);
   AgentGetParameterArgA(param, 5, portAsChar, sizeof(portAsChar));

   if (host[0] == 0 || user[0] == 0 || pass[0] == 0)
      return SYSINFO_RC_UNSUPPORTED;

   if (portAsChar[0] == 0)
   {
      port = (arg[1] == 'S') ? 995 : 110;
   }
   else
   {
      port = ParsePort(portAsChar);
      if (port == 0)
         return SYSINFO_RC_UNSUPPORTED;
   }

   int64_t start = GetCurrentTimeMs();

   int result = CheckPOP3(arg[1] == 'S', InetAddress::resolveHostName(host), port, user, pass, timeout);
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
 * Check POP3/POP3S service
 */
int CheckPOP3(bool enableTLS, const InetAddress& addr, uint16_t port, const char* user, const char* pass, uint32_t timeout)
{
   int status = 0;
   TLSConnection tc(SUBAGENT_DEBUG_TAG, false, timeout);
   if (tc.connect(addr, port, enableTLS, timeout))
   {
      char buff[512];
      char tmp[128];

      status = PC_ERR_HANDSHAKE;

#define CHECK_OK (tc.recv(buff, sizeof(buff)) > 3) && (strncmp(buff, "+OK", 3) == 0)

      if (CHECK_OK)
      {
         snprintf(tmp, sizeof(tmp), "USER %s\r\n", user);
         if (tc.send(tmp, strlen(tmp)))
         {
            if (CHECK_OK)
            {
               snprintf(tmp, sizeof(tmp), "PASS %s\r\n", pass);
               if (tc.send(tmp, strlen(tmp)))
               {
                  if (CHECK_OK)
                  {
                     status = PC_ERR_NONE;
                  }
               }
            }
         }
      }
   }
   else
   {
      status = PC_ERR_CONNECT;
   }

   return status;
}
