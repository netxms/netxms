/* $Id: http.cpp,v 1.2 2005-01-29 21:24:03 victor Exp $ */

#include <nms_common.h>
#include <nms_agent.h>
/* TODO: WINDOWS COMPATIBILITY */
#ifndef _WIN32
#include <regex.h>

#include "main.h"
#include "net.h"

LONG H_CheckHTTP(char *pszParam, char *pArg, char *pValue)
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

	if (CheckSSH(szHost, 0, nPort, NULL, NULL) == 0)
	{
		ret_int(pValue, 0);
	}
	else
	{
		ret_int(pValue, 1);
	}

	return nRet;
}

int CheckHTTP(char *szAddr, DWORD dwAddr, short nPort, char *szURI,
		char *szHost, char *szMatch)
{
	int nRet = 0;
	int nSd;
	regex_t preg;

	if (regcomp(&preg, szMatch, REG_EXTENDED | REG_ICASE | REG_NOSUB) != 0)
	{
		return PC_ERR_BAD_PARAMS;
	}

	nSd = NetConnectTCP(szAddr, dwAddr, nPort);
	if (nSd > 0)
	{
		char szTmp[2048];

		nRet = PC_ERR_HANDSHAKE;

		snprintf(szTmp, sizeof(szTmp),
				"GET %s HTTP/1.1\r\nConnection: close\r\nHost: %s:%d\r\n\r\n",
				szURI, szHost, nPort);

		printf("GET:\n|%s|\n", szTmp);

		if (NetWrite(nSd, szTmp, strlen(szTmp)) > 0)
		{
			if (NetRead(nSd, szTmp, sizeof(szTmp)) >= 8)
			{
				if (regexec(&preg, szTmp, 0, NULL, 0) == 0)
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

	regfree(&preg);

	return nRet;
}

#else

int CheckHTTP(char *szAddr, DWORD dwAddr, short nPort, char *szURI,
		char *szHost, char *szMatch)
{
   return 1;
}

#endif

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.1  2005/01/29 00:21:29  alk
+ http checker

request string: "HOST:URI"
responce string: posix regex, e.g. '^HTTP/1.[01] 200 .*'

requst sent to server:
---
GET URI HTTP/1.1\r\n
Connection: close\r\n
Host: HOST\r\n\r\n
---

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
