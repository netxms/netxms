/* $Id: net.cpp,v 1.1.1.1 2005-01-18 18:38:54 alk Exp $ */

#include <nms_common.h>
#include <nms_agent.h>

#include "net.h"

int NetConnectTCP(char *szHost, unsigned short nPort)
{
	int nSocket;

	nSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (nSocket > 0)
	{
		struct sockaddr_in sa;

		sa.sin_family = AF_INET;
		sa.sin_port = htons(nPort);
		sa.sin_addr.s_addr = inet_addr(szHost);
		
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
	close(nSocket);
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $

*/
