/* $Id$ */

#include <nms_common.h>
#include <nms_agent.h>

#include "main.h"
#include "net.h"

LONG H_CheckCustom(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
{
	LONG nRet = SYSINFO_RC_SUCCESS;

	char szHost[1024];
	TCHAR szPort[1024];
	TCHAR szTimeout[64];
	unsigned short nPort;

   AgentGetParameterArgA(pszParam, 1, szHost, sizeof(szHost));
   AgentGetParameterArg(pszParam, 2, szPort, sizeof(szPort));
   AgentGetParameterArg(pszParam, 3, szTimeout, sizeof(szTimeout));

	if (szHost[0] == 0 || szPort[0] == 0)
	{
		return SYSINFO_RC_ERROR;
	}

	nPort = (unsigned short)_tcstol(szPort, NULL, 10);
	if (nPort == 0)
	{
		return SYSINFO_RC_ERROR;
	}

	DWORD dwTimeout = _tcstoul(szTimeout, NULL, 0);
	ret_int(pValue, CheckCustom(szHost, 0, nPort, dwTimeout));
	return nRet;
}


int CheckCustom(char *szAddr, DWORD dwAddr, short nPort, DWORD dwTimeout)
{
	int nRet;
	SOCKET nSd;

	nSd = NetConnectTCP(szAddr, dwAddr, nPort, dwTimeout);
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
