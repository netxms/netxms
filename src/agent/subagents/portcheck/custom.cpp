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
	char szHost[1024];
	TCHAR szPort[1024];
	TCHAR szTimeout[64];

   AgentGetParameterArgA(param, 1, szHost, sizeof(szHost));
   AgentGetParameterArg(param, 2, szPort, sizeof(szPort));
   AgentGetParameterArg(param, 3, szTimeout, sizeof(szTimeout));

	if (szHost[0] == 0 || szPort[0] == 0)
		return SYSINFO_RC_ERROR;

	uint16_t nPort = static_cast<uint16_t>(_tcstol(szPort, nullptr, 10));
	if (nPort == 0)
		return SYSINFO_RC_ERROR;

   LONG nRet = SYSINFO_RC_SUCCESS;

	uint32_t dwTimeout = _tcstoul(szTimeout, nullptr, 0);
   int64_t start = GetCurrentTimeMs();
   int result = CheckCustom(szHost, InetAddress::INVALID, nPort, dwTimeout);
   if (*arg == 'R')
   {
      if (result == PC_ERR_NONE)
         ret_int64(value, GetCurrentTimeMs() - start);
      else if (g_serviceCheckFlags & SCF_NEGATIVE_TIME_ON_ERROR)
         ret_int(value, -result);
      else
         nRet = SYSINFO_RC_ERROR;
   }
   else
   {
	   ret_int(value, result);
   }
   return nRet;
}

/**
 * Check custom service
 */
int CheckCustom(char *hostname, const InetAddress& addr, short nPort, UINT32 dwTimeout)
{
	int nRet;
	SOCKET nSd = NetConnectTCP(hostname, addr, nPort, dwTimeout);
	if (nSd != INVALID_SOCKET)
	{
		nRet = PC_ERR_NONE;
		NetClose(nSd);
	}
	else
	{
		nRet = PC_ERR_CONNECT;
	}

   char buffer[64];
   nxlog_debug_tag(SUBAGENT_DEBUG_TAG, 7, _T("CheckCustom(%hs, %d): result=%d"), (hostname != NULL) ? hostname : addr.toStringA(buffer), (int)nPort, nRet);

	return nRet;
}
