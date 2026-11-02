/*
** NetXMS multiplatform core agent
** Copyright (C) 2014-2026 Raden Solutions
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
#include <socket_listener.h>

/**
 * Counter for received SNMP traps
 */
uint64_t g_snmpTraps = 0;

/**
 * SNMP trap receiver
 */
class SNMPTrapListener : public DatagramSocketListener
{
private:
   BYTE *m_buffer;

protected:
   virtual bool isStopConditionReached() override
   {
      return (g_dwFlags & AF_SHUTDOWN) != 0;
   }

   virtual ConnectionProcessingResult processDatagram(SOCKET s) override;

public:
   SNMPTrapListener() : DatagramSocketListener(g_snmpTrapPort, (g_dwFlags & AF_DISABLE_IPV4) == 0, (g_dwFlags & AF_DISABLE_IPV6) == 0)
   {
      m_buffer = MemAllocArrayNoInit<BYTE>(SNMP_DEFAULT_MSG_MAX_SIZE);
      setName(_T("SNMPTrap"));
   }

   virtual ~SNMPTrapListener()
   {
      MemFree(m_buffer);
   }
};

/**
 * Forward received SNMP packet to server without decoding
 */
ConnectionProcessingResult SNMPTrapListener::processDatagram(SOCKET s)
{
   SockAddrBuffer addr;
   socklen_t addrLen = sizeof(SockAddrBuffer);
   int bytes = recvfrom(s, reinterpret_cast<char*>(m_buffer), SNMP_DEFAULT_MSG_MAX_SIZE, 0, reinterpret_cast<struct sockaddr*>(&addr), &addrLen);
   if (bytes <= 0)
   {
      // Sleep on error
      ThreadSleepMs(100);
      return CPR_COMPLETED;
   }

   InetAddress sender = InetAddress::createFromSockaddr(reinterpret_cast<struct sockaddr*>(&addr));
   TCHAR ipAddrStr[64];

   // Forward only datagrams that start with complete BER sequence (same check as SNMP_UDPTransport::readMessage)
   size_t pduLength = SnmpGetMessageLength(m_buffer, bytes);
   if ((pduLength == 0) || (pduLength > static_cast<size_t>(bytes)))
   {
      nxlog_debug(6, _T("SNMPTrapReceiver: malformed packet from %s dropped (%d bytes)"), sender.toString(ipAddrStr), bytes);
      return CPR_COMPLETED;
   }

   nxlog_debug(6, _T("SNMPTrapReceiver: packet received from %s"), sender.toString(ipAddrStr));
   g_snmpTraps++;

   NXCPMessage *msg = new NXCPMessage(CMD_SNMP_TRAP, GenerateMessageId(), 4); // Use version 4
   msg->setField(VID_IP_ADDRESS, sender);
   msg->setField(VID_PORT, g_snmpTrapPort);
   msg->setField(VID_PDU_SIZE, static_cast<uint32_t>(pduLength));
   msg->setField(VID_PDU, m_buffer, pduLength);
   msg->setField(VID_ZONE_UIN, g_zoneUIN);
   g_notificationProcessorQueue.put(msg);
   return CPR_COMPLETED;
}

/**
 * SNMP trap read thread
 */
void SNMPTrapReceiver()
{
   SNMPTrapListener listener;
   listener.setListenAddress(g_snmpTrapListenAddress);
   if (!listener.initialize())
   {
      nxlog_debug(1, _T("SNMPTrapReceiver: cannot bind at least one socket, exiting"));
      return;
   }

   listener.mainLoop();
   nxlog_debug(1, _T("SNMP Trap Receiver terminated"));
}
