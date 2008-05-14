/* $Id$ */

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

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.5  2006/08/06 10:32:02  victor
- Both 32 and 6 bit installers works correctly
- All subagents ported to 64bit
- Agent now reports platform windows-x64 instead of windows-amd64

Revision 1.4  2005/10/18 21:33:26  victor
- Default port for ServiceCheck.HTTP(*) changed from 22 to 80 :)
- All ServiceCheck.XXX parameters now returns actual failure code, not just 0 or 1

Revision 1.3  2005/10/18 09:01:16  alk
Added commands (ServiceCheck.*) for
	http
	smtp
	custom

Revision 1.2  2005/08/17 12:09:23  victor
responce changed to response (issue #37)

Revision 1.1  2005/02/08 18:32:56  alk
+ simple "custom" checker added


*/
