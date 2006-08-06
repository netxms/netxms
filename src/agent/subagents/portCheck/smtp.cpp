#include <nms_common.h>
#include <nms_agent.h>

#include "main.h"
#include "net.h"

LONG H_CheckSMTP(char *pszParam, char *pArg, char *pValue)
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
	if (nSd > 0)
	{
		char szBuff[512];
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
											if (NetWrite(nSd, "NetXMS test mail\r\n.\r\n",
														21) > 0)
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
