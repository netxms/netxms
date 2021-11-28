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
** File: custom.cpp
**
**/

#include "portcheck.h"

/**
 * Check custom service - parameter handler
 */
LONG H_CheckCustom(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char host[1024];
   TCHAR portText[32], timeoutText[64];
   AgentGetParameterArgA(param, 1, host, sizeof(host));
   AgentGetParameterArg(param, 2, portText, sizeof(portText) / sizeof(TCHAR));
   AgentGetParameterArg(param, 3, timeoutText, sizeof(timeoutText) / sizeof(TCHAR));

	if (host[0] == 0 || portText[0] == 0)
		return SYSINFO_RC_ERROR;

	uint16_t port = static_cast<uint16_t>(_tcstol(portText, nullptr, 10));
	if (port == 0)
		return SYSINFO_RC_ERROR;

   LONG rc = SYSINFO_RC_SUCCESS;

	uint32_t timeout = _tcstoul(timeoutText, nullptr, 0);
   int64_t start = GetCurrentTimeMs();
   int result = CheckCustom(host, InetAddress::INVALID, port, timeout);
   if (*arg == 'R')
   {
      if (result == PC_ERR_NONE)
         ret_int64(value, GetCurrentTimeMs() - start);
      else if (g_serviceCheckFlags & SCF_NEGATIVE_TIME_ON_ERROR)
         ret_int(value, -result);
      else
         rc = SYSINFO_RC_ERROR;
   }
   else
   {
	   ret_int(value, result);
   }
   return rc;
}

/**
 * Check custom service
 */
int CheckCustom(char *hostname, const InetAddress& addr, uint16_t port, uint32_t timeout)
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

   char buffer[64];
   nxlog_debug_tag(SUBAGENT_DEBUG_TAG, 7, _T("CheckCustom(%hs, %d): result=%d"), (hostname != nullptr) ? hostname : addr.toStringA(buffer), (int)port, status);

	return status;
}
