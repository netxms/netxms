/* $Id$ */

#include <nms_common.h>
#include <nms_agent.h>
#ifdef _WIN32
#include <netxms-regex.h>
#else
#include <regex.h>
#endif

#include "main.h"
#include "net.h"

LONG H_CheckHTTP(const char *pszParam, const char *pArg, char *pValue)
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

	if (szHost[0] == 0 || szPort[0] == 0 || szURI[0] == 0)
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

	if (szMatch[0] == 0)
	{
		strcpy(szMatch, "^HTTP/1.[01] 200 .*");
	}
	
	nSd = NetConnectTCP(szAddr, dwAddr, nPort);
	if (nSd != INVALID_SOCKET)
	{
		char szTmp[4096];
		char szHostHeader[4096];

		nRet = PC_ERR_HANDSHAKE;
		
		szHostHeader[0] = 0;
		if (szHost[0] != 0)
		{
			snprintf(szHostHeader, sizeof(szHostHeader),
				"Host: %s:%d\r\n",
				szHost, nPort);
		}

		snprintf(szTmp, sizeof(szTmp),
				"GET %s HTTP/1.1\r\nConnection: close\r\n%s\r\n",
				szURI, szHostHeader);

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
Revision 1.10  2006/09/06 09:06:53  alk
Check.HTTP, 4th and 5th params are optional now
if "match" ($5) does not exist, default "^HTTP/1.[01] 200 .*" is used
if "host-header" ($4) does not exist, "Host: ..." will be ommited

Revision 1.9  2006/08/13 22:58:59  victor
- Default session timeout changed to 120 seconds on non-Windows systems
- Changed socket() error checking - on Windows, SOCKET is an unsigned integer, so conditions like (sock < 0) will not become true event if socket() fails - fixed

Revision 1.8  2006/08/06 10:32:02  victor
- Both 32 and 6 bit installers works correctly
- All subagents ported to 64bit
- Agent now reports platform windows-x64 instead of windows-amd64

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
