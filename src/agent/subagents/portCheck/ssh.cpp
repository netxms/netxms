/* $Id: ssh.cpp,v 1.1 2005-01-19 13:42:47 alk Exp $ */

#include <nms_common.h>
#include <nms_agent.h>

#include "net.h"

LONG H_CheckSSH(char *pszParam, char *pArg, char *pValue)
{
	LONG nRet = SYSINFO_RC_SUCCESS;
	char szHost[256];
	char szPort[256];
	unsigned short nPort;
	int nSd;
	bool bIsOk = false;

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

	nSd = NetConnectTCP(szHost, nPort);
	if (nSd > 0)
	{
		char szBuff[512];
		char szTmp[128];

		if (NetRead(nSd, szBuff, sizeof(szBuff)) >= 8)
		{
			int nMajor, nMinor;

			if (sscanf(szBuff, "SSH-%d.%d-", &nMajor, &nMinor) == 2)
			{
				snprintf(szTmp, sizeof(szTmp), "SSH-%d.%d-NetXMS\n",
						nMajor, nMinor);
				if (NetWrite(nSd, szTmp, strlen(szTmp)) > 0)
				{
					bIsOk = true;
				}
			}
		}

		NetClose(nSd);
	}
	ret_int(pValue, bIsOk ? 1 : 0);

	return nRet;
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $

*/
