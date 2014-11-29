/* $Id$ */

#include <nms_common.h>
#include <nms_agent.h>

#include "main.h"
#include "net.h"

/**
 * Check custom service - parameter handler
 */
LONG H_CheckCustom(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
	LONG nRet = SYSINFO_RC_SUCCESS;

	char szHost[1024];
	TCHAR szPort[1024];
	TCHAR szTimeout[64];
	unsigned short nPort;

   AgentGetParameterArgA(param, 1, szHost, sizeof(szHost));
   AgentGetParameterArg(param, 2, szPort, sizeof(szPort));
   AgentGetParameterArg(param, 3, szTimeout, sizeof(szTimeout));

	if (szHost[0] == 0 || szPort[0] == 0)
	{
		return SYSINFO_RC_ERROR;
	}

	nPort = (unsigned short)_tcstol(szPort, NULL, 10);
	if (nPort == 0)
	{
		return SYSINFO_RC_ERROR;
	}

	UINT32 dwTimeout = _tcstoul(szTimeout, NULL, 0);
   INT64 start = GetCurrentTimeMs();
   int result = CheckCustom(szHost, 0, nPort, dwTimeout);
   if (*arg == 'R')
   {
	   ret_int64(value, GetCurrentTimeMs() - start);
   }
   else
   {
	   ret_int(value, result);
   }
   return nRet;
}

/**
 * Check custom service
 */
int CheckCustom(char *szAddr, UINT32 dwAddr, short nPort, UINT32 dwTimeout)
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
