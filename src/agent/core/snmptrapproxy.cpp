/*
** NetXMS multiplatform core agent
** Copyright (C) 2014-2020 Raden Solutions
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
** File: snmptrapproxy.cpp
**
**/

#include "nxagentd.h"

/**
 * Counter for received SNMP traps
 */
uint64_t g_snmpTraps = 0;

/**
 * Raw SNMP packet received from network
 */
struct SNMPPacket
{
   InetAddress addr;  // Source address
   UINT16 port;       // Receiver port
   BYTE *data;        // Raw packet data
   size_t lenght;     // Data length

   SNMPPacket(const InetAddress& _addr, UINT16 _port, BYTE *_data, size_t _length)
   {
      addr = _addr;
      port = _port;
      data = _data;
      lenght = _length;
   }

   ~SNMPPacket()
   {
      MemFree(data);
   }
};

/**
 * SNMP trap read thread
 */
void SNMPTrapReceiver()
{
   if (g_dwFlags & AF_DISABLE_IPV4)
   {
      nxlog_debug(1, _T("SNMPTrapReceiver: IPv4 disabled, exiting"));
      return;
   }

   SOCKET hSocket = CreateSocket(AF_INET, SOCK_DGRAM, 0);
   if (hSocket == INVALID_SOCKET)
   {
      TCHAR buffer[1024];
      nxlog_debug(1, _T("SNMPTrapReceiver: cannot create socket (%s)"), GetLastSocketErrorText(buffer, 1024));
      return;
   }

   SetSocketExclusiveAddrUse(hSocket);
   SetSocketReuseFlag(hSocket);

   // Fill in local address structure
   struct sockaddr_in sa;
   memset(&sa, 0, sizeof(sa));
   sa.sin_family = AF_INET;
   if (!_tcscmp(g_szSNMPTrapListenAddress, _T("*")))
   {
      sa.sin_addr.s_addr = htonl(INADDR_ANY);
   }
   else
	{
      InetAddress bindAddress = InetAddress::resolveHostName(g_szSNMPTrapListenAddress, AF_INET);
      if (bindAddress.isValid() && (bindAddress.getFamily() == AF_INET))
      {
		   sa.sin_addr.s_addr = htonl(bindAddress.getAddressV4());
      }
      else
      {
   		sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      }
	}
   sa.sin_port = htons(g_snmpTrapPort);

   // Bind socket
   if (bind(hSocket, (struct sockaddr *)&sa, sizeof(struct sockaddr_in)) != 0)
   {
      TCHAR buffer[1024];
      nxlog_debug(1, _T("SNMPTrapReceiver: cannot bind socket (%s)"), GetLastSocketErrorText(buffer, 1024));
      closesocket(hSocket);
      return;
   }

   TCHAR ipAddrStr[64];
   DebugPrintf(3, _T("SNMPTrapReceiver: listening on %s:%d"),
      IpToStr(ntohl(sa.sin_addr.s_addr), ipAddrStr), (int)ntohs(sa.sin_port));

   SNMP_TrapProxyTransport *pTransport = new SNMP_TrapProxyTransport(hSocket);
   pTransport->enableEngineIdAutoupdate(true);
   pTransport->setPeerUpdatedOnRecv(true);

   // Wait for packets
   while(!(g_dwFlags & AF_SHUTDOWN))
   {
      BYTE *packet = NULL;
      socklen_t nAddrLen = sizeof(struct sockaddr_in);
      int bytes = pTransport->readRawMessage(&packet, 2000, (struct sockaddr *)&sa, &nAddrLen);
      if ((bytes > 0) && (packet != NULL))
      {
         SNMPPacket *message = new SNMPPacket(InetAddress::createFromSockaddr(reinterpret_cast<struct sockaddr*>(&sa)), g_snmpTrapPort, packet, bytes);
         nxlog_debug(6, _T("SNMPTrapReceiver: packet received from %s"), message->addr.toString(ipAddrStr));
         g_snmpTraps++;

         NXCPMessage *msg = new NXCPMessage(CMD_SNMP_TRAP, GenerateMessageId(), 4); // Use version 4
         msg->setField(VID_IP_ADDRESS, message->addr);
         msg->setField(VID_PORT, message->port);
         msg->setField(VID_PDU_SIZE, static_cast<UINT32>(message->lenght));
         msg->setField(VID_PDU, message->data, message->lenght);
         msg->setField(VID_ZONE_UIN, g_zoneUIN);
         g_notificationProcessorQueue.put(msg);
      }
      else
      {
         // Sleep on error
         ThreadSleepMs(100);
      }
   }

   delete pTransport;
   DebugPrintf(1, _T("SNMP Trap Receiver terminated"));
}

/** Implementation of class SNMP_TrapProxyTransport **/

/**
 * Constructor
 */
SNMP_TrapProxyTransport::SNMP_TrapProxyTransport(SOCKET hSocket) : SNMP_UDPTransport(hSocket)
{
}

/**
 * Read PDU from socket but do not decode and parse it
 */
int SNMP_TrapProxyTransport::readRawMessage(BYTE **rawData, UINT32 dwTimeout, struct sockaddr *pSender, socklen_t *piAddrSize)
{
   int bytes;
   size_t pduLength;

   if (m_dwBytesInBuffer < 2)
   {
      bytes = recvData(dwTimeout, pSender, piAddrSize);
      if (bytes <= 0)
      {
         clearBuffer();
         return bytes;
      }
      m_dwBytesInBuffer += bytes;
   }

   pduLength = preParsePDU();
   if (pduLength == 0)
   {
      // Clear buffer
      clearBuffer();
      return 0;
   }

   // Move existing data to the beginning of buffer if there are not enough space at the end
   if (pduLength > m_dwBufferSize - m_dwBufferPos)
   {
      memmove(m_pBuffer, &m_pBuffer[m_dwBufferPos], m_dwBytesInBuffer);
      m_dwBufferPos = 0;
   }

   // Read entire PDU into buffer
   while(m_dwBytesInBuffer < pduLength)
   {
      bytes = recvData(dwTimeout, pSender, piAddrSize);
      if (bytes <= 0)
      {
         clearBuffer();
         return bytes;
      }
      m_dwBytesInBuffer += bytes;
   }

   *rawData = (BYTE*)MemAlloc(pduLength);
   memcpy(*rawData, &m_pBuffer[m_dwBufferPos], pduLength);

   m_dwBytesInBuffer -= pduLength;
   if (m_dwBytesInBuffer == 0)
      m_dwBufferPos = 0;

   return (int)pduLength;
}
