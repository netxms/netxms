#include <nms_common.h>
#include <nms_agent.h>

#include "main.h"
#include "net.h"

/**
 * Check SMTP service - parameter handler
 */
LONG H_CheckSMTP(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
	LONG nRet = SYSINFO_RC_SUCCESS;
	char szHost[256];
	char szTo[256];
	TCHAR szTimeout[64];

	AgentGetParameterArgA(param, 1, szHost, sizeof(szHost));
	AgentGetParameterArgA(param, 2, szTo, sizeof(szTo));
   AgentGetParameterArg(param, 3, szTimeout, sizeof(szTimeout));

	if (szHost[0] == 0 || szTo[0] == 0)
	{
		return SYSINFO_RC_ERROR;
	}

	UINT32 dwTimeout = _tcstoul(szTimeout, NULL, 0);
   INT64 start = GetCurrentTimeMs();
	int result = CheckSMTP(szHost, InetAddress::INVALID, 25, szTo, dwTimeout);
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
 * Check SMTP service
 */
int CheckSMTP(char *szAddr, const InetAddress& addr, short nPort, char *szTo, UINT32 dwTimeout)
{
	int nRet = 0;
	SOCKET nSd;
	int nErr = 0; 

	nSd = NetConnectTCP(szAddr, addr, nPort, dwTimeout);
	if (nSd != INVALID_SOCKET)
	{
		char szBuff[2048];
		char szTmp[128];
		char szHostname[128];

		nRet = PC_ERR_HANDSHAKE;

#define CHECK_OK(x) nErr = 1; while(1) { \
	if (NetCanRead(nSd, (dwTimeout != 0) ? dwTimeout : 1000)) { \
		if (NetRead(nSd, szBuff, sizeof(szBuff)) > 3) { \
			if (szBuff[3] == '-') { continue; } \
			if (strncmp(szBuff, x" ", 4) == 0) { nErr = 0; } break; \
		} else { break; } \
	} else { break; } } \
	if (nErr == 0)


		CHECK_OK("220")
		{
			if (gethostname(szHostname, sizeof(szHostname)) == -1)
			{
				strcpy(szHostname, "netxms-portcheck");
			}
			
			snprintf(szTmp, sizeof(szTmp), "HELO %s\r\n", szHostname);
			if (NetWrite(nSd, szTmp, (int)strlen(szTmp)) > 0)
			{
				CHECK_OK("250")
				{
					snprintf(szTmp, sizeof(szTmp), "MAIL FROM: noreply@%s\r\n", g_szDomainName);
					if (NetWrite(nSd, szTmp, (int)strlen(szTmp)) > 0)
					{
						CHECK_OK("250")
						{
							snprintf(szTmp, sizeof(szTmp), "RCPT TO: %s\r\n", szTo);
							if (NetWrite(nSd, szTmp, (int)strlen(szTmp)) > 0)
							{
								CHECK_OK("250")
								{
									if (NetWrite(nSd, "DATA\r\n", 6) > 0)
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
											
											if (NetWrite(nSd, szBuff, (int)strlen(szBuff)) > 0)
											{
												CHECK_OK("250")
												{
													if (NetWrite(nSd, "QUIT\r\n", 6) > 0)
													{
														CHECK_OK("221")
														{
															nRet = PC_ERR_NONE;
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
		nRet = PC_ERR_CONNECT;
	}

	return nRet;
}
