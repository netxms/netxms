/* $Id: ssh.cpp,v 1.7 2006-08-13 22:59:00 victor Exp $ */

#include <nms_common.h>
#include <nms_agent.h>

#include "main.h"
#include "net.h"

LONG H_CheckSSH(char *pszParam, char *pArg, char *pValue)
{
	LONG nRet = SYSINFO_RC_SUCCESS;

	char szHost[256];
	char szPort[256];
	unsigned short nPort;

   NxGetParameterArg(pszParam, 1, szHost, sizeof(szHost));
   NxGetParameterArg(pszParam, 2, szPort, sizeof(szPort));

	if (szHost[0] == 0)
	{
		return SYSINFO_RC_ERROR;
	}

	nPort = (unsigned short)atoi(szPort);
	if (nPort == 0)
	{
		nPort = 22;
	}

	ret_int(pValue, CheckSSH(szHost, 0, nPort, NULL, NULL));
	return nRet;
}

int CheckSSH(char *szAddr, DWORD dwAddr, short nPort, char *szUser, char *szPass)
{
	int nRet = 0;
	SOCKET nSd;

	nSd = NetConnectTCP(szAddr, dwAddr, nPort);
	if (nSd != INVALID_SOCKET)
	{
		char szBuff[512];
		char szTmp[128];

		nRet = PC_ERR_HANDSHAKE;

		if (NetRead(nSd, szBuff, sizeof(szBuff)) >= 8)
		{
			int nMajor, nMinor;

			if (sscanf(szBuff, "SSH-%d.%d-", &nMajor, &nMinor) == 2)
			{
				snprintf(szTmp, sizeof(szTmp), "SSH-%d.%d-NetXMS\n",
						nMajor, nMinor);
				if (NetWrite(nSd, szTmp, (int)strlen(szTmp)) > 0)
				{
					nRet = PC_ERR_NONE;
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

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.6  2006/08/06 10:32:03  victor
- Both 32 and 6 bit installers works correctly
- All subagents ported to 64bit
- Agent now reports platform windows-x64 instead of windows-amd64

Revision 1.5  2005/10/18 21:33:26  victor
- Default port for ServiceCheck.HTTP(*) changed from 22 to 80 :)
- All ServiceCheck.XXX parameters now returns actual failure code, not just 0 or 1

Revision 1.4  2005/01/28 23:45:01  alk
SMTP check added, requst string == rcpt to

Revision 1.3  2005/01/28 23:19:36  alk
VID_SERVICE_STATUS set

Revision 1.2  2005/01/28 02:50:32  alk
added support for CMD_CHECK_NETWORK_SERVICE
suported:
	ssh: host/port req.
	pop3: host/port/request string req. request string format: "login:password"

Revision 1.1  2005/01/19 13:42:47  alk
+ ServiceCheck.SSH(host[, port]) Added


*/
