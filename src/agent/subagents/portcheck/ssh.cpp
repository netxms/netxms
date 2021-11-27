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
** File: ssh.cpp
**
**/

#include "portcheck.h"

/**
 * Check SSH service - parameter handler
 */
LONG H_CheckSSH(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
	LONG nRet = SYSINFO_RC_SUCCESS;

	char szHost[256];
	TCHAR szPort[256];
	TCHAR szTimeout[64];
	unsigned short nPort;

   AgentGetParameterArgA(param, 1, szHost, sizeof(szHost));
   AgentGetParameterArg(param, 2, szPort, sizeof(szPort));
   AgentGetParameterArg(param, 3, szTimeout, sizeof(szTimeout));

	if (szHost[0] == 0)
	{
		return SYSINFO_RC_ERROR;
	}

	nPort = (unsigned short)_tcstoul(szPort, NULL, 10);
	if (nPort == 0)
	{
		nPort = 22;
	}

	UINT32 dwTimeout = _tcstoul(szTimeout, NULL, 0);
   INT64 start = GetCurrentTimeMs();
	int result = CheckSSH(szHost, InetAddress::INVALID, nPort, NULL, NULL, dwTimeout);
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
 * Check SSH service
 */
int CheckSSH(char *szAddr, const InetAddress& addr, short nPort, char *szUser, char *szPass, UINT32 dwTimeout)
{
	int nRet = 0;
	SOCKET nSd;

	nSd = NetConnectTCP(szAddr, addr, nPort, dwTimeout);
	if (nSd != INVALID_SOCKET)
	{
		char szBuff[512];
		char szTmp[128];

		nRet = PC_ERR_HANDSHAKE;

		if (SocketCanRead(nSd, 1000))
		{
			if (NetRead(nSd, szBuff, sizeof(szBuff)) >= 8)
			{
				int nMajor, nMinor;

				if (sscanf(szBuff, "SSH-%d.%d-", &nMajor, &nMinor) == 2)
				{
					snprintf(szTmp, sizeof(szTmp), "SSH-%d.%d-NetXMS\n",
							nMajor, nMinor);
					if (NetWrite(nSd, szTmp, (int)strlen(szTmp)) > 0)
					{
						nRet = PC_ERR_NONE;
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
