#include <nms_common.h>
#include <nms_agent.h>

#include "main.h"
#include "net.h"

LONG H_CheckSMTP(const char *pszParam, const char *pArg, char *pValue)
{
	LONG nRet = SYSINFO_RC_SUCCESS;
	char szHost[256];
	char szTo[256];
	bool bIsOk = false;

	NxGetParameterArg(pszParam, 1, szHost, sizeof(szHost));
	NxGetParameterArg(pszParam, 2, szTo, sizeof(szTo));

	if (szHost[0] == 0 || szTo[0] == 0)
	{
		return SYSINFO_RC_ERROR;
	}

	ret_int(pValue, CheckSMTP(szHost, 0, 25, szTo));
	return nRet;
}

int CheckSMTP(char *szAddr, DWORD dwAddr, short nPort, char *szTo)
{
	int nRet = 0;
	SOCKET nSd;
	int nErr = 0; 

	nSd = NetConnectTCP(szAddr, dwAddr, nPort);
	if (nSd != INVALID_SOCKET)
	{
		char szBuff[2048];
		char szTmp[128];
		char szHostname[128];

		nRet = PC_ERR_HANDSHAKE;

#define CHECK_OK(x) nErr = 1; while(1) { \
	if (NetRead(nSd, szBuff, sizeof(szBuff)) > 3) { \
			if (szBuff[3] == '-') { continue; } \
			if (strncmp(szBuff, x" ", 4) == 0) { nErr = 0; } break; \
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
					snprintf(szTmp, sizeof(szTmp), "MAIL FROM: noreply@%s\r\n",
						szHostname);
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
#ifdef HAVE_GMTIME_R
											struct tm currentTM;
											gmtime_r(&currentTime, &currentTM);
											pCurrentTM = &currentTM;
#else
											pCurrentTM = gmtime(&currentTime);
#endif
											strftime(szTime, sizeof(szTime), "%a, %d %b %Y %H:%M:%S %z\r\n", pCurrentTM);

											snprintf(szBuff, sizeof(szBuff), "From: <noreply@%s>\r\nTo: <%s>\r\nSubject: NetXMS test mail\r\nDate: %s\r\n\r\nNetXMS test mail\r\n.\r\n",
											         szHostname, szTo, szTime);
											
											if (NetWrite(nSd, szBuff, strlen(szBuff)) > 0)
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
