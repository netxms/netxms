/* $Id: pop3.cpp,v 1.1.1.1 2005-01-18 18:38:54 alk Exp $ */

#include <nms_common.h>
#include <nms_agent.h>

#include "net.h"

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

	nSd = NetConnectTCP(szHost, 110);
	if (nSd > 0)
	{
		char szBuff[512];
		char szTmp[128];

#define CHECK_OK ((NetRead(nSd, szBuff, sizeof(szBuff)) > 3) \
				&& (strncmp(szBuff, "+OK", 3) == 0))

		if (CHECK_OK)
		{
			snprintf(szTmp, sizeof(szTmp), "USER %s\r\n", szUser);
			if (NetWrite(nSd, szTmp, strlen(szTmp)) > 0)
			{
				if (CHECK_OK)
				{
					snprintf(szTmp, sizeof(szTmp), "PASS %s\r\n", szPassword);
					if (NetWrite(nSd, szTmp, strlen(szTmp)) > 0)
					{
						if (CHECK_OK)
						{
							bIsOk = true;
						}
					}
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
