/* 
** NetXMS - Network Management System
** SMS driver for Portech MV-37x VoIP GSM gateways
** Copyright (C) 2011 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: net.cpp
**
**/

#include "portech.h"


SOCKET NetConnectTCP(const TCHAR *hostName, unsigned short nPort, DWORD dwTimeout)
{
	SOCKET nSocket;

	nSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (nSocket != INVALID_SOCKET)
	{
		struct sockaddr_in sa;
		sa.sin_family = AF_INET;
		sa.sin_port = htons(nPort);
		sa.sin_addr.s_addr = ResolveHostName(hostName);

		if (ConnectEx(nSocket, (struct sockaddr*)&sa, sizeof(sa), (dwTimeout != 0) ? dwTimeout : 30000) < 0)
		{
			closesocket(nSocket);
			nSocket = INVALID_SOCKET;
		}
	}

	return nSocket;
}

static bool NetCanRead(SOCKET nSocket, int nTimeout /* ms */)
{
	bool ret = false;
	struct timeval timeout;
	fd_set readFdSet;

	FD_ZERO(&readFdSet);
	FD_SET(nSocket, &readFdSet);
	timeout.tv_sec = nTimeout / 1000;
	timeout.tv_usec = (nTimeout % 1000) * 1000;

	if (select(SELECT_NFDS(nSocket + 1), &readFdSet, NULL, NULL, &timeout) > 0)
	{
		ret = true;
	}

	return ret;
}

static int NetRead(SOCKET nSocket, char *pBuff, int nSize)
{
	return RecvEx(nSocket, pBuff, nSize, 0, INFINITE);
}

int NetWrite(SOCKET nSocket, const char *pBuff, int nSize)
{
	return SendEx(nSocket, pBuff, nSize, 0, NULL);
}

bool NetWriteLine(SOCKET nSocket, const char *line)
{
	if (NetWrite(nSocket, line, (int)strlen(line)) <= 0)
		return false;
	return NetWrite(nSocket, "\r\n", 2) > 0;
}

void NetClose(SOCKET nSocket)
{
	shutdown(nSocket, SHUT_RDWR);
	closesocket(nSocket);
}

bool NetWaitForText(SOCKET nSocket, const char *text, int timeout)
{
	static char buffer[256] = "";
	static int pos = 0;
	
	int textLen = (int)strlen(text);
	int bufLen = (int)strlen(buffer);

	char *p = strstr(buffer, text);
	if (p != NULL)
	{
		int index = (int)(p - buffer);
		pos = bufLen - (index + textLen);
		memmove(buffer, &buffer[bufLen - pos], pos);
		return true;
	}

	pos = min(bufLen, textLen - 1);
	memmove(buffer, &buffer[bufLen - pos], pos);

	while(1)
	{
		if (!NetCanRead(nSocket, timeout))
			return false;

		int size = NetRead(nSocket, &buffer[pos], 255 - pos);
		buffer[size + pos] = 0;
		bufLen = (int)strlen(buffer);

		p = strstr(buffer, text);
		if (p != NULL)
		{
			int index = (int)(p - buffer);
			pos = bufLen - (index + textLen);
			memmove(buffer, &buffer[bufLen - pos], pos);
			return true;
		}

		pos = min(bufLen, textLen - 1);
		memmove(buffer, &buffer[bufLen - pos], pos);
	}
}
