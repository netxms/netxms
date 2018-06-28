/*
** NetXMS multiplatform core agent
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
** File: tcpproxy.cpp
**
**/

#include "nxagentd.h"

/**
 * Next free proxy ID
 */
static VolatileCounter m_nextId = 0;

/**
 * Constructor
 */
TcpProxy::TcpProxy(CommSession *session, SOCKET s)
{
   m_id = InterlockedIncrement(&m_nextId);
   m_session = session;
   m_socket = s;
}

/**
 * Destructor
 */
TcpProxy::~TcpProxy()
{
   shutdown(m_socket, SHUT_RDWR);
   closesocket(m_socket);

   NXCPMessage msg;
   msg.setCode(CMD_CLOSE_TCP_PROXY);
   msg.setField(VID_CHANNEL_ID, m_id);
   m_session->postMessage(&msg);
}

/**
 * Read data from socket
 */
bool TcpProxy::readSocket()
{
   BYTE buffer[65536];
   int bytes = recv(m_socket, (char *)buffer + NXCP_HEADER_SIZE, 65536 - NXCP_HEADER_SIZE, 0);
   if (bytes <= 0)
      return false;

   NXCP_MESSAGE *header = reinterpret_cast<NXCP_MESSAGE*>(buffer);
   header->code = htons(CMD_TCP_PROXY_DATA);
   header->id = htonl(m_id);
   header->flags = htons(MF_BINARY);
   header->numFields = htonl(static_cast<UINT32>(bytes));

   UINT32 size = NXCP_HEADER_SIZE + bytes;
   if ((size % 8) != 0)
      size += 8 - size % 8;
   header->size = htonl(size);

   m_session->postRawMessage(header);
   return true;
}

/**
 * Write data to socket
 */
void TcpProxy::writeSocket(const BYTE *data, size_t size)
{
   // FIXME: change to thread pool?
   SendEx(m_socket, data, size, 0, NULL);
}
