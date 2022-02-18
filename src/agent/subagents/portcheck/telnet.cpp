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
** File: telnet.cpp
**
**/

#include "portcheck.h"

/**
 * Check telnet service - parameter handler
 */
LONG H_CheckTelnet(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
	LONG nRet = SYSINFO_RC_SUCCESS;

	char szHost[256];
	TCHAR szPort[256];
	AgentGetParameterArgA(param, 1, szHost, sizeof(szHost));
	AgentGetParameterArg(param, 2, szPort, sizeof(szPort));

	if (szHost[0] == 0)
		return SYSINFO_RC_ERROR;

	uint16_t nPort = static_cast<uint16_t>(_tcstoul(szPort, nullptr, 10));
	if (nPort == 0)
	{
		nPort = 23;
	}

   uint32_t timeout = GetTimeoutFromArgs(param, 3);
   int64_t start = GetCurrentTimeMs();
	int result = CheckTelnet(szHost, InetAddress::INVALID, nPort, nullptr, nullptr, timeout);
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
 * Check telnet service
 */
int CheckTelnet(const char *hostname, const InetAddress& addr, uint16_t port, char *username, char *password, uint32_t timeout)
{
	int status = 0;
	SOCKET nSd = NetConnectTCP(hostname, addr, port, timeout);
	if (nSd != INVALID_SOCKET)
	{
		unsigned char szBuff[512];

		status = PC_ERR_HANDSHAKE;
		while(SocketCanRead(nSd, 1000) && status == PC_ERR_HANDSHAKE) // 1sec
		{
			ssize_t size = NetRead(nSd, (char *)szBuff, sizeof(szBuff));
			unsigned char out[4];

			memset(out, 0, sizeof(out));
			for (ssize_t i = 0; i < size; i++)
			{
				if (szBuff[i] == 0xFF) // IAC
				{
					out[0] = 0xFF;
					continue;
				}
				if (out[0] == 0xFF && (szBuff[i] == 251 || szBuff[i] == 252))
				{
					out[1] = 254;
					continue;
				}
				if (out[0] == 0xFF && (szBuff[i] == 253 || szBuff[i] == 254))
				{
					out[1] = 252;
					continue;
				}

				if (out[0] == 0xFF && out[1] != 0)
				{
					out[2] = szBuff[i];

					// send reject
					NetWrite(nSd, out, 3);

					memset(out, 0, sizeof(out));
					continue;
				}

				// end of handshake, get out from here
				status = PC_ERR_NONE;
				break;
			}
		}

		NetClose(nSd);
	}
	else
	{
		status = PC_ERR_CONNECT;
	}

	return status;
}
