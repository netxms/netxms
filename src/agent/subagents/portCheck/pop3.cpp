/* $Id$ */

#include <nms_common.h>
#include <nms_agent.h>

#include "main.h"
#include "net.h"

LONG H_CheckPOP3(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
{
	LONG nRet = SYSINFO_RC_SUCCESS;
	char szHost[256];
	char szUser[256];
	char szPassword[256];
	TCHAR szTimeout[64];
	bool bIsOk = false;

	AgentGetParameterArgA(pszParam, 1, szHost, sizeof(szHost));
	AgentGetParameterArgA(pszParam, 2, szUser, sizeof(szUser));
	AgentGetParameterArgA(pszParam, 3, szPassword, sizeof(szPassword));
   AgentGetParameterArg(pszParam, 4, szTimeout, sizeof(szTimeout));

	if (szHost[0] == 0 || szUser[0] == 0 || szPassword[0] == 0)
	{
		return SYSINFO_RC_ERROR;
	}

	DWORD dwTimeout = _tcstoul(szTimeout, NULL, 0);
	ret_int(pValue, CheckPOP3(szHost, 0, 110, szUser, szPassword, dwTimeout));
	return nRet;
}

int CheckPOP3(char *szAddr, DWORD dwAddr, short nPort, char *szUser, char *szPass, DWORD dwTimeout)
{
	int nRet = 0;
	SOCKET nSd;

	nSd = NetConnectTCP(szAddr, dwAddr, nPort, dwTimeout);
	if (nSd != INVALID_SOCKET)
	{
		char szBuff[512];
		char szTmp[128];

		nRet = PC_ERR_HANDSHAKE;

#define CHECK_OK (NetCanRead(nSd, 1000) && (NetRead(nSd, szBuff, sizeof(szBuff)) > 3) \
				&& (strncmp(szBuff, "+OK", 3) == 0))

		if (CHECK_OK)
		{
			snprintf(szTmp, sizeof(szTmp), "USER %s\r\n", szUser);
			if (NetWrite(nSd, szTmp, (int)strlen(szTmp)) > 0)
			{
				if (CHECK_OK)
				{
					snprintf(szTmp, sizeof(szTmp), "PASS %s\r\n", szPass);
					if (NetWrite(nSd, szTmp, (int)strlen(szTmp)) > 0)
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
