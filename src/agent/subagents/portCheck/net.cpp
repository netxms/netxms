/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2018 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: net.cpp
**
**/

#include <nms_common.h>
#include <nms_agent.h>

#include "net.h"

SOCKET NetConnectTCP(const char *hostname, const InetAddress& addr, unsigned short nPort, UINT32 dwTimeout)
{
   InetAddress hostAddr = (hostname != NULL) ? InetAddress::resolveHostName(hostname) : addr;
   if (!hostAddr.isValidUnicast() && !hostAddr.isLoopback())
      return INVALID_SOCKET;

	return ConnectToHost(hostAddr, nPort, (dwTimeout != 0) ? dwTimeout : m_dwDefaultTimeout);
}

bool NetCanRead(SOCKET nSocket, int nTimeout /* ms */)
{
   SocketPoller sp;
   sp.add(nSocket);
   return sp.poll(nTimeout) > 0;
}

bool NetCanWrite(SOCKET nSocket, int nTimeout /* ms */)
{
   SocketPoller sp(true);
   sp.add(nSocket);
   return sp.poll(nTimeout) > 0;
}

ssize_t NetRead(SOCKET nSocket, char *pBuff, size_t nSize)
{
	return RecvEx(nSocket, pBuff, nSize, 0, INFINITE);
}

ssize_t NetWrite(SOCKET nSocket, const char *pBuff, size_t nSize)
{
	return SendEx(nSocket, pBuff, nSize, 0, NULL);
}

void NetClose(SOCKET nSocket)
{
	shutdown(nSocket, SHUT_RDWR);
	closesocket(nSocket);
}
