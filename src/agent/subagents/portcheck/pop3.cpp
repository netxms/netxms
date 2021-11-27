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

/**
 * Check POP3 service - parameter handler
 */
LONG H_CheckPOP3(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
	LONG nRet = SYSINFO_RC_SUCCESS;
	char szHost[256];
	char szUser[256];
	char szPassword[256];
	TCHAR szTimeout[64];

	AgentGetParameterArgA(param, 1, szHost, sizeof(szHost));
	AgentGetParameterArgA(param, 2, szUser, sizeof(szUser));
	AgentGetParameterArgA(param, 3, szPassword, sizeof(szPassword));
   AgentGetParameterArg(param, 4, szTimeout, sizeof(szTimeout));

	if (szHost[0] == 0 || szUser[0] == 0 || szPassword[0] == 0)
	{
		return SYSINFO_RC_ERROR;
	}

	UINT32 dwTimeout = _tcstoul(szTimeout, NULL, 0);
   INT64 start = GetCurrentTimeMs();
	int result = CheckPOP3(szHost, InetAddress::INVALID, 110, szUser, szPassword, dwTimeout);
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
 * Check POP3 service
 */
int CheckPOP3(char *szAddr, const InetAddress& addr, short nPort, char *szUser, char *szPass, UINT32 dwTimeout)
{
	int nRet = 0;
	SOCKET nSd;

	nSd = NetConnectTCP(szAddr, addr, nPort, dwTimeout);
	if (nSd != INVALID_SOCKET)
	{
		char szBuff[512];
		char szTmp[128];

		nRet = PC_ERR_HANDSHAKE;

#define CHECK_OK (SocketCanRead(nSd, 1000) && (NetRead(nSd, szBuff, sizeof(szBuff)) > 3) \
				&& (strncmp(szBuff, "+OK", 3) == 0))

		if (CHECK_OK)
		{
			snprintf(szTmp, sizeof(szTmp), "USER %s\r\n", szUser);
			if (NetWrite(nSd, szTmp, strlen(szTmp)))
			{
				if (CHECK_OK)
				{
					snprintf(szTmp, sizeof(szTmp), "PASS %s\r\n", szPass);
					if (NetWrite(nSd, szTmp, strlen(szTmp)))
					{
						if (CHECK_OK)
						{
							nRet = PC_ERR_NONE;
						}
					}
				}
			}
		}

		NetClose(nSd);
	}
	else
	{
		nRet = PC_ERR_CONNECT;
	}

	return nRet;
}
