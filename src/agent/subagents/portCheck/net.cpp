/* $Id: net.cpp,v 1.3 2005-01-29 21:24:03 victor Exp $ */

#include <nms_common.h>
#include <nms_agent.h>

#include "net.h"

int NetConnectTCP(char *szHost, DWORD dwAddr, unsigned short nPort)
{
	int nSocket;

	nSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (nSocket > 0)
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
			close(nSocket);
			nSocket = -1;
		}
	}

	return nSocket;
}

int NetRead(int nSocket, char *pBuff, int nSize)
{
	return recv(nSocket, pBuff, nSize, 0);
}

int NetWrite(int nSocket, char *pBuff, int nSize)
{
	return send(nSocket, pBuff, nSize, 0);
}

void NetClose(int nSocket)
{
   shutdown(nSocket, SHUT_RDWR);
	closesocket(nSocket);
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
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
