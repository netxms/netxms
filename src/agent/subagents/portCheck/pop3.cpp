/* $Id: pop3.cpp,v 1.2 2005-01-28 02:50:32 alk Exp $ */

#include <nms_common.h>
#include <nms_agent.h>

#include "main.h"
#include "net.h"
#include "pop3.h"

LONG H_CheckPOP3(char *pszParam, char *pArg, char *pValue)
{
	LONG nRet = SYSINFO_RC_SUCCESS;
	char szHost[256];
	char szUser[256];
	char szPassword[256];
	int nSd;
	bool bIsOk = false;

   NxGetParameterArg(pszParam, 1, szHost, sizeof(szHost));
   NxGetParameterArg(pszParam, 2, szUser, sizeof(szUser));
   NxGetParameterArg(pszParam, 3, szPassword, sizeof(szPassword));

	if (szHost[0] == 0 || szUser[0] == 0 || szPassword[0] == 0)
	{
		return SYSINFO_RC_ERROR;
	}

	if (CheckPOP3(szHost, 0, 110, szUser, szPassword) == 0)
	{
		ret_int(pValue, 1);
	}
	else
	{
		ret_int(pValue, 0);
	}

	return nRet;
}

int CheckPOP3(char *szAddr, DWORD dwAddr, short nPort, char *szUser, char *szPass)
{
	int nRet = 0;
	int nSd;

	nSd = NetConnectTCP(szAddr, dwAddr, nPort);
	if (nSd > 0)
	{
		char szBuff[512];
		char szTmp[128];

		nRet = PC_ERR_HANDSHAKE;

#define CHECK_OK ((NetRead(nSd, szBuff, sizeof(szBuff)) > 3) \
				&& (strncmp(szBuff, "+OK", 3) == 0))

		if (CHECK_OK)
		{
			snprintf(szTmp, sizeof(szTmp), "USER %s\r\n", szUser);
			if (NetWrite(nSd, szTmp, strlen(szTmp)) > 0)
			{
				if (CHECK_OK)
				{
					snprintf(szTmp, sizeof(szTmp), "PASS %s\r\n", szPass);
					if (NetWrite(nSd, szTmp, strlen(szTmp)) > 0)
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

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.1.1.1  2005/01/18 18:38:54  alk
Initial import

implemented:
	ServiceCheck.POP3(host, user, password) - connect to host:110 and try to login


*/
