/*
** NetXMS - Network Management System
** Copyright (C) 2003-2022 Raden Solutions
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
**/

#include "netutil.h"

uint32_t s_defaultTimeout = 3000;

/**
 * Connect to given host
 */
SOCKET NetConnectTCP(const char* hostname, const InetAddress& addr, uint16_t port, uint32_t dwTimeout)
{
   InetAddress hostAddr = (hostname != nullptr) ? InetAddress::resolveHostName(hostname) : addr;
   if (!hostAddr.isValidUnicast() && !hostAddr.isLoopback())
      return INVALID_SOCKET;

   return ConnectToHost(hostAddr, port, (dwTimeout != 0) ? dwTimeout : s_defaultTimeout);
}

/**
 * Write to the socket
 */
bool NetWrite(SOCKET hSocket, const void* data, size_t size)
{
   return SendEx(hSocket, data, size, 0, nullptr) == static_cast<ssize_t>(size);
}

/**
 * Read from socket
 */
ssize_t NetRead(SOCKET hSocket, void* buffer, size_t size)
{
#ifdef _WIN32
   return recv(hSocket, static_cast<char*>(buffer), static_cast<int>(size), 0);
#else
   ssize_t rc;
   do
   {
      rc = recv(hSocket, static_cast<char*>(buffer), size, 0);
   }
   while ((rc == -1) && (errno == EINTR));
   return rc;
#endif
}

/**
 * Close socket gracefully
 */
void NetClose(SOCKET nSocket)
{
   shutdown(nSocket, SHUT_RDWR);
   closesocket(nSocket);
}
