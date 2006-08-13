/* $Id: net.cpp,v 1.8 2006-08-13 22:59:00 victor Exp $ */

#include <nms_common.h>
#include <nms_agent.h>

#include "net.h"

SOCKET NetConnectTCP(char *szHost, DWORD dwAddr, unsigned short nPort)
{
	SOCKET nSocket;

	nSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (nSocket != INVALID_SOCKET)
	{
		struct sockaddr_in sa;

		sa.sin_family = AF_INET;
		sa.sin_port = htons(nPort);
		if (szHost != NULL)
		{
			sa.sin_addr.s_addr = inet_addr(szHost);
		}
		else
		{
			sa.sin_addr.s_addr = htonl(dwAddr);
		}
		
		if (connect(nSocket, (struct sockaddr*)&sa, sizeof(sa)) < 0)
		{
			closesocket(nSocket);
			nSocket = INVALID_SOCKET;
		}
	}

	return nSocket;
}

bool NetCanRead(SOCKET nSocket, int nTimeout /* ms */)
{
	bool ret = false;
	struct timeval timeout;
	fd_set rdfs;

	FD_ZERO(&rdfs);
	FD_SET(nSocket, &rdfs);
	timeout.tv_sec = nTimeout / 1000;
	timeout.tv_usec = (nTimeout % 1000) * 1000;

	if (select(SELECT_NFDS(nSocket + 1), &rdfs, NULL, NULL, &timeout) > 0)
	{
		ret = true;
	}

	return ret;
}

int NetRead(SOCKET nSocket, char *pBuff, int nSize)
{
	return recv(nSocket, pBuff, nSize, 0);
}

int NetWrite(SOCKET nSocket, char *pBuff, int nSize)
{
	return send(nSocket, pBuff, nSize, 0);
}

void NetClose(SOCKET nSocket)
{
   shutdown(nSocket, SHUT_RDWR);
	closesocket(nSocket);
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.7  2006/08/06 10:32:02  victor
- Both 32 and 6 bit installers works correctly
- All subagents ported to 64bit
- Agent now reports platform windows-x64 instead of windows-amd64

Revision 1.6  2006/03/15 13:28:18  victor
- int changed to SOCKET
- Telnet checker added to VC++ project

Revision 1.5  2006/03/15 12:00:10  alk
simple telnet service checker added: it connects, response WON'T/DON'T to
all offers and disconnects (this prevents from "peer died" in logs)

Revision 1.4  2005/11/16 22:40:18  victor
close() replaced with closesocket()

Revision 1.3  2005/01/29 21:24:03  victor
Fixed some Windows compatibility issues

Revision 1.2  2005/01/28 02:50:32  alk
added support for CMD_CHECK_NETWORK_SERVICE
suported:
	ssh: host/port req.
	pop3: host/port/request string req. request string format: "login:password"

Revision 1.1.1.1  2005/01/18 18:38:54  alk
Initial import

implemented:
	ServiceCheck.POP3(host, user, password) - connect to host:110 and try to login


*/
