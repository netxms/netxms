/*
** NetXMS - Network Management System
** Copyright (C) 2003-2022 Raden Solutions
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
** File: tcp.cpp
**
**/

#include "netsvc.h"

/**
 * Check custom TCP service (make sure TCP port accepts connection)
 */
int CheckTCP(const char *hostname, const InetAddress& addr, uint16_t port, uint32_t timeout)
{
   int status;
   SOCKET hSocket = NetConnectTCP(hostname, addr, port, timeout);
   if (hSocket != INVALID_SOCKET)
   {
      status = PC_ERR_NONE;
      NetClose(hSocket);
   }
   else
   {
      status = PC_ERR_CONNECT;
   }
   return status;
}

/**
 * Check custom TCP service - metric sub-handler
 */
LONG NetworkServiceStatus_TCP(const char *host, const char *port, const OptionList& options, int *result)
{
   if (host[0] == 0)
      return SYSINFO_RC_UNSUPPORTED;

   uint16_t nPort = static_cast<uint16_t>(strtoul(port, nullptr, 10));
   if (nPort == 0)
      nPort = 22;

   *result = CheckTCP(host, InetAddress::INVALID, nPort, options.getAsUInt32(_T("timeout"), g_netsvcTimeout));
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for legacy metric
 */
LONG H_CheckTCP(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char host[1024];
   TCHAR portText[32];
   if (!AgentGetParameterArgA(param, 1, host, sizeof(host)) ||
       !AgentGetParameterArg(param, 2, portText, sizeof(portText) / sizeof(TCHAR)))
      return SYSINFO_RC_UNSUPPORTED;

   if (host[0] == 0 || portText[0] == 0)
      return SYSINFO_RC_UNSUPPORTED;

   uint16_t port = static_cast<uint16_t>(_tcstol(portText, nullptr, 10));
   if (port == 0)
      return SYSINFO_RC_UNSUPPORTED;

   uint32_t timeout = GetTimeoutFromArgs(param, 3);

   LONG rc = SYSINFO_RC_SUCCESS;
   int64_t start = GetCurrentTimeMs();
   int result = CheckTCP(host, InetAddress::INVALID, port, timeout);
   if (*arg == 'R')
   {
      if (result == PC_ERR_NONE)
         ret_int64(value, GetCurrentTimeMs() - start);
      else if (g_netsvcFlags & NETSVC_AF_NEGATIVE_TIME_ON_ERROR)
         ret_int64(value, -(GetCurrentTimeMs() - start));
      else
         rc = SYSINFO_RC_ERROR;
   }
   else
   {
      ret_int(value, result);
   }
   return rc;
}
