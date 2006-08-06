/* $Id: http.cpp,v 1.8 2006-08-06 10:32:02 victor Exp $ */

#include <nms_common.h>
#include <nms_agent.h>
#ifdef _WIN32
#include <netxms-regex.h>
#else
#include <regex.h>
#endif

#include "main.h"
#include "net.h"

LONG H_CheckHTTP(char *pszParam, char *pArg, char *pValue)
{
	LONG nRet = SYSINFO_RC_SUCCESS;

	char szHost[1024];
	char szPort[1024];
	char szURI[1024];
	char szHeader[1024];
	char szMatch[1024];
	unsigned short nPort;

   NxGetParameterArg(pszParam, 1, szHost, sizeof(szHost));
   NxGetParameterArg(pszParam, 2, szPort, sizeof(szPort));
   NxGetParameterArg(pszParam, 3, szURI, sizeof(szURI));
   NxGetParameterArg(pszParam, 4, szHeader, sizeof(szHeader));
   NxGetParameterArg(pszParam, 5, szMatch, sizeof(szMatch));

	if (szHost[0] == 0 || szPort[0] == 0 || szURI[0] == 0 || szHeader[0] == 0
			|| szMatch[0] == 0)
	{
		return SYSINFO_RC_ERROR;
	}

	nPort = (unsigned short)atoi(szPort);
	if (nPort == 0)
	{
		nPort = 80;
	}

	ret_int(pValue, CheckHTTP(szHost, 0, nPort, szURI, szHeader, szMatch));
	return nRet;
}

int CheckHTTP(char *szAddr, DWORD dwAddr, short nPort, char *szURI,
		char *szHost, char *szMatch)
{
	int nBytes, nRet = 0;
	SOCKET nSd;
	regex_t preg;

	if (regcomp(&preg, szMatch, REG_EXTENDED | REG_ICASE | REG_NOSUB) != 0)
	{
		return PC_ERR_BAD_PARAMS;
	}

	nSd = NetConnectTCP(szAddr, dwAddr, nPort);
	if (nSd > 0)
	{
		char szTmp[4096];

		nRet = PC_ERR_HANDSHAKE;

		snprintf(szTmp, sizeof(szTmp),
				"GET %s HTTP/1.1\r\nConnection: close\r\nHost: %s:%d\r\n\r\n",
				szURI, szHost, nPort);

		if (NetWrite(nSd, szTmp, (int)strlen(szTmp)) > 0)
		{
			if ((nBytes = NetRead(nSd, szTmp, sizeof(szTmp))) >= 8)
			{
            szTmp[nBytes] = 0;
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

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.7  2006/07/30 08:22:13  victor
- Added checking for CDP and Nortel topology autodiscovery support
- Other minor changes

Revision 1.6  2005/10/18 21:33:26  victor
- Default port for ServiceCheck.HTTP(*) changed from 22 to 80 :)
- All ServiceCheck.XXX parameters now returns actual failure code, not just 0 or 1

Revision 1.5  2005/10/18 09:01:16  alk
Added commands (ServiceCheck.*) for
	http
	smtp
	custom

Revision 1.4  2005/08/17 12:09:23  victor
responce changed to response (issue #37)

Revision 1.3  2005/01/31 15:29:31  victor
Regular expressions implemented under Windows

Revision 1.2  2005/01/29 21:24:03  victor
Fixed some Windows compatibility issues

Revision 1.1  2005/01/29 00:21:29  alk
+ http checker

request string: "HOST:URI"
response string: posix regex, e.g. '^HTTP/1.[01] 200 .*'

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
