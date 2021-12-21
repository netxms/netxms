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
** File: smtp.cpp
**
**/

#include "portcheck.h"

/**
 * Check SMTP service - parameter handler
 */
LONG H_CheckSMTP(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
	char host[256], recipient[256];
	AgentGetParameterArgA(param, 1, host, sizeof(host));
	AgentGetParameterArgA(param, 2, recipient, sizeof(recipient));

	if ((host[0] == 0) || (recipient[0] == 0))
		return SYSINFO_RC_UNSUPPORTED;

   LONG rc = SYSINFO_RC_SUCCESS;
   uint32_t timeout = GetTimeoutFromArgs(param, 3);
   int64_t start = GetCurrentTimeMs();
	int result = CheckSMTP(host, InetAddress::INVALID, 25, recipient, timeout);
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
 * Check SMTP service
 */
int CheckSMTP(char *szAddr, const InetAddress& addr, short nPort, char *szTo, UINT32 dwTimeout)
{
	int status = PC_ERR_NONE;
	int nErr = 0;

	SOCKET nSd = NetConnectTCP(szAddr, addr, nPort, dwTimeout);
	if (nSd != INVALID_SOCKET)
	{
		char szBuff[2048];
		char szTmp[128];
		char szHostname[128];

		status = PC_ERR_HANDSHAKE;

#define CHECK_OK(x) nErr = 1; while(1) { \
	if (SocketCanRead(nSd, (dwTimeout != 0) ? dwTimeout : 1000)) { \
		if (NetRead(nSd, szBuff, sizeof(szBuff)) > 3) { \
			if (szBuff[3] == '-') { continue; } \
			if (strncmp(szBuff, x" ", 4) == 0) { nErr = 0; } break; \
		} else { break; } \
	} else { break; } } \
	if (nErr == 0)

		CHECK_OK("220")
		{
         strlcpy(szHostname, g_hostName, sizeof(szHostname));
		   if (szHostname[0] == 0)
		   {
#ifdef UNICODE
		      WCHAR wname[128] = L"";
            GetLocalHostName(wname, 128, true);
            wchar_to_utf8(wname, -1, szHostname, sizeof(szHostname));
#else
		      GetLocalHostName(szHostname, sizeof(szHostname), true);
#endif
	         if (szHostname[0] == 0)
            {
               strcpy(szHostname, "netxms-portcheck");
            }
		   }
			
			snprintf(szTmp, sizeof(szTmp), "HELO %s\r\n", szHostname);
			if (NetWrite(nSd, szTmp, strlen(szTmp)))
			{
				CHECK_OK("250")
				{
					snprintf(szTmp, sizeof(szTmp), "MAIL FROM: noreply@%s\r\n", g_szDomainName);
					if (NetWrite(nSd, szTmp, strlen(szTmp)))
					{
						CHECK_OK("250")
						{
							snprintf(szTmp, sizeof(szTmp), "RCPT TO: %s\r\n", szTo);
							if (NetWrite(nSd, szTmp, strlen(szTmp)))
							{
								CHECK_OK("250")
								{
									if (NetWrite(nSd, "DATA\r\n", 6))
									{
										CHECK_OK("354")
										{
											// date
											time_t currentTime;
											struct tm *pCurrentTM;
											char szTime[64];

											time(&currentTime);
#ifdef HAVE_LOCALTIME_R
											struct tm currentTM;
											localtime_r(&currentTime, &currentTM);
											pCurrentTM = &currentTM;
#else
											pCurrentTM = localtime(&currentTime);
#endif
											strftime(szTime, sizeof(szTime), "%a, %d %b %Y %H:%M:%S %z\r\n", pCurrentTM);

											snprintf(szBuff, sizeof(szBuff), "From: <noreply@%s>\r\nTo: <%s>\r\nSubject: NetXMS test mail\r\nDate: %s\r\n\r\nNetXMS test mail\r\n.\r\n",
											         szHostname, szTo, szTime);
											
											if (NetWrite(nSd, szBuff, strlen(szBuff)))
											{
												CHECK_OK("250")
												{
													if (NetWrite(nSd, "QUIT\r\n", 6))
													{
														CHECK_OK("221")
														{
															status = PC_ERR_NONE;
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
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
