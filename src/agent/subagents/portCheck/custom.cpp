/* $Id$ */

#include <nms_common.h>
#include <nms_agent.h>

#include "main.h"
#include "net.h"

LONG H_CheckCustom(const char *pszParam, const char *pArg, char *pValue)
{
	LONG nRet = SYSINFO_RC_SUCCESS;

	char szHost[1024];
	char szPort[1024];
	unsigned short nPort;

   AgentGetParameterArg(pszParam, 1, szHost, sizeof(szHost));
   AgentGetParameterArg(pszParam, 2, szPort, sizeof(szPort));

	if (szHost[0] == 0 || szPort[0] == 0)
	{
		return SYSINFO_RC_ERROR;
	}

	nPort = (unsigned short)atoi(szPort);
	if (nPort == 0)
	{
		return SYSINFO_RC_ERROR;
	}

	ret_int(pValue, CheckCustom(szHost, 0, nPort, NULL, NULL));
	return nRet;
}


int CheckCustom(char *szAddr, DWORD dwAddr, short nPort, char *szRequest,
		char *szResponse)
{
	int nRet;
	SOCKET nSd;

	nSd = NetConnectTCP(szAddr, dwAddr, nPort);
	if (nSd != INVALID_SOCKET)
	{
		nRet = PC_ERR_NONE;

		NetClose(nSd);
	}
	else
	{
		nRet = PC_ERR_CONNECT;
	}

	return nRet;
}
