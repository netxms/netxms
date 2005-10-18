/* $Id: custom.cpp,v 1.3 2005-10-18 09:01:16 alk Exp $ */

#include <nms_common.h>
#include <nms_agent.h>

#include "main.h"
#include "net.h"

LONG H_CheckCustom(char *pszParam, char *pArg, char *pValue)
{
	LONG nRet = SYSINFO_RC_SUCCESS;

	char szHost[1024];
	char szPort[1024];
	unsigned short nPort;

   NxGetParameterArg(pszParam, 1, szHost, sizeof(szHost));
   NxGetParameterArg(pszParam, 2, szPort, sizeof(szPort));

	if (szHost[0] == 0 || szPort[0] == 0)
	{
		return SYSINFO_RC_ERROR;
	}

	nPort = (unsigned short)atoi(szPort);
	if (nPort == 0)
	{
		return SYSINFO_RC_ERROR;
	}

	if (CheckCustom(szHost, 0, nPort, NULL, NULL) == 0)
	{
		ret_int(pValue, 0);
	}
	else
	{
		ret_int(pValue, 1);
	}

	return nRet;
}


int CheckCustom(char *szAddr, DWORD dwAddr, short nPort, char *szRequest,
		char *szResponse)
{
	int nRet = 0;
	int nSd;

	nSd = NetConnectTCP(szAddr, dwAddr, nPort);
	if (nSd > 0)
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

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.2  2005/08/17 12:09:23  victor
responce changed to response (issue #37)

Revision 1.1  2005/02/08 18:32:56  alk
+ simple "custom" checker added


*/
