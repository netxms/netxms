/*
** NetXMS - Network Management System
** SNMP support library
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: snmpscan.cpp
**
**/

#include "libnxsnmp.h"

/**
 * Scan status for each address
 */
struct ScanStatus
{
   int64_t startTime;
   bool success;
   uint32_t rtt;
};

/**
 * Process SNMP response
 */
static void ProcessResponse(SOCKET sock, uint32_t baseAddr, uint32_t lastAddr, ScanStatus *status)
{
   char reply[8192];
   struct sockaddr_in saSrc;
   socklen_t addrLen = sizeof(struct sockaddr_in);
   if (recvfrom(sock, reply, sizeof(reply), 0, reinterpret_cast<struct sockaddr*>(&saSrc), &addrLen) > 0)
   {
      uint32_t addr = ntohl(saSrc.sin_addr.s_addr);
      if ((addr >= baseAddr) && (addr <= lastAddr) && !status[addr - baseAddr].success)
      {
         status[addr - baseAddr].success = true;
         status[addr - baseAddr].rtt = static_cast<uint32_t>(GetCurrentTimeMs() - status[addr - baseAddr].startTime);
      }
   }
}

/**
 * Scan range of IPv4 addresses using SNMP requests
 */
uint32_t LIBNXSNMP_EXPORTABLE SnmpScanAddressRange(const InetAddress& from, const InetAddress& to, uint16_t port, SNMP_Version snmpVersion, const char *community, void (*callback)(const InetAddress&, uint32_t, void*), void *context)
{
   SOCKET sock = CreateSocket(AF_INET, SOCK_DGRAM, 0);
   if (sock == INVALID_SOCKET)
      return SNMP_ERR_SOCKET;

   struct sockaddr_in localAddr;
   memset(&localAddr, 0, sizeof(localAddr));
   localAddr.sin_family = AF_INET;
   localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
   if (bind(sock, (struct sockaddr *)&localAddr, sizeof(localAddr)) != 0)
   {
      closesocket(sock);
      return SNMP_ERR_SOCKET;
   }
   SetSocketNonBlocking(sock);

   SNMP_SecurityContext securityContext;
   SNMP_PDU request(SNMP_GET_REQUEST, 1, snmpVersion);
   if (snmpVersion == SNMP_VERSION_3)
   {
      // Use engine ID discovery as request
      request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 6, 3, 10, 2, 1, 1, 0 }));
   }
   else
   {
      securityContext.setCommunity(community);
      request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 2, 1, 1, 1, 0 }));
   }
   SNMP_PDUBuffer pdu;
   size_t size = request.encode(&pdu, &securityContext);

   struct sockaddr_in saDest;
   memset(&saDest, 0, sizeof(sockaddr_in));
   saDest.sin_family = AF_INET;
   saDest.sin_port = htons(port);

   SocketPoller sp;
   ScanStatus *status = MemAllocArray<ScanStatus>(to.getAddressV4() - from.getAddressV4() + 1);
   uint32_t baseAddr = from.getAddressV4();
   for(uint32_t a = baseAddr, i = 0; a <= to.getAddressV4(); a++, i++)
   {
      saDest.sin_addr.s_addr = htonl(a);
      status[i].startTime = GetCurrentTimeMs();
      status[i].success = false;
      sendto(sock, (char *)pdu.buffer(), static_cast<int>(size), 0, (struct sockaddr *)&saDest, sizeof(struct sockaddr_in));

      sp.reset();
      sp.add(sock);
      if (sp.poll(10) > 0)
      {
         ProcessResponse(sock, baseAddr, to.getAddressV4(), status);
      }
   }

   uint32_t elapsedTime = 0;
   uint32_t timeout = SnmpGetDefaultTimeout();
   while(elapsedTime < timeout)
   {
      sp.reset();
      sp.add(sock);
      int64_t startTime = GetCurrentTimeMs();
      if (sp.poll(timeout - elapsedTime) <= 0)
         break;

      ProcessResponse(sock, baseAddr, to.getAddressV4(), status);
      elapsedTime += static_cast<uint32_t>(GetCurrentTimeMs() - startTime);
   }

   closesocket(sock);

   for(uint32_t a = baseAddr, i = 0; a <= to.getAddressV4(); a++, i++)
   {
      if (status[i].success)
         callback(a, status[i].rtt, context);
   }
   MemFree(status);

   return SNMP_ERR_SUCCESS;
}
