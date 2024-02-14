/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
TcpProxy::TcpProxy(CommSession *session, uint32_t channelId, SOCKET s)
{
   // Starting with version 4.5.3 server provides own channel ID
   m_channelId = (channelId != 0) ? channelId : InterlockedIncrement(&m_nextId);
   m_session = session;
   m_socket = s;
   m_readError = false;
}

/**
 * Destructor
 */
TcpProxy::~TcpProxy()
{
   shutdown(m_socket, SHUT_RDWR);
   closesocket(m_socket);

   NXCPMessage msg(CMD_CLOSE_TCP_PROXY, 0, m_session->getProtocolVersion());
   msg.setField(VID_CHANNEL_ID, m_channelId);
   msg.setField(VID_ERROR_INDICATOR, m_readError);
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
   {
      m_readError = (bytes < 0);
      return false;
   }

   NXCP_MESSAGE *header = reinterpret_cast<NXCP_MESSAGE*>(buffer);
   header->code = htons(CMD_TCP_PROXY_DATA);
   header->id = htonl(m_channelId);
   header->flags = htons(MF_BINARY);
   header->numFields = htonl(static_cast<UINT32>(bytes));

   uint32_t size = NXCP_HEADER_SIZE + bytes;
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
   SendEx(m_socket, data, size, 0, nullptr);
}

/**
 * Callback for address range scan
 */
static void RangeScanCallback(const InetAddress& addr, uint32_t rtt, void *context)
{
   TCHAR buffer[64];
   static_cast<StringList*>(context)->add(addr.toString(buffer));
}

/**
 * Handler for list TCP.ScanAddressRange
 */
LONG H_TCPAddressRangeScan(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   if (!session->isMasterServer() || !(g_dwFlags & AF_ENABLE_TCP_PROXY))
   {
      session->debugPrintf(5, _T("Request for address range scan via TCP rejected"));
      return SYSINFO_RC_UNSUPPORTED;
   }

   char startAddr[128], endAddr[128];
   TCHAR portText[64];
   if (!AgentGetParameterArgA(cmd, 1, startAddr, 128) ||
       !AgentGetParameterArgA(cmd, 2, endAddr, 128) ||
       !AgentGetParameterArg(cmd, 3, portText, 64))
   {
      return SYSINFO_RC_UNSUPPORTED;
   }

   InetAddress start = InetAddress::parse(startAddr);
   InetAddress end = InetAddress::parse(endAddr);
   uint16_t port = (portText[0] != 0) ? static_cast<uint16_t>(_tcstoul(portText, nullptr, 0)) : 4700;
   if (!start.isValid() || !end.isValid() || (port == 0))
   {
      return SYSINFO_RC_UNSUPPORTED;
   }

   TCPScanAddressRange(start, end, port, RangeScanCallback, value);
   session->debugPrintf(5, _T("Address range %s - %s scan via TCP completed"), start.toString().cstr(), end.toString().cstr());
   return SYSINFO_RC_SUCCESS;
}
