/*
** NetXMS - Network Management System
** Copyright (C) 2003-2022 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
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
** File: tcpscan.cpp
**
**/

#include "libnxagent.h"

#define BLOCK_SIZE   32

/**
 * Scan data for each address
 */
struct ScanData
{
   SOCKET handle;
   int64_t startTime;
   bool completed;
   bool success;
   uint32_t rtt;
};

/**
 * Scan block of addresses
 */
static void ScanBlock(uint32_t startAddr, uint32_t endAddr, uint16_t port, void (*callback)(const InetAddress&, uint32_t, void*), void *context)
{
   struct sockaddr_in remoteAddr;
   memset(&remoteAddr, 0, sizeof(remoteAddr));
   remoteAddr.sin_family = AF_INET;
   remoteAddr.sin_port = htons(port);

   ScanData targets[BLOCK_SIZE];
   memset(targets, 0, sizeof(targets));

   int64_t pollStartTime = GetCurrentTimeMs();
   int targetCount = endAddr - startAddr + 1;
   int waitCount = 0;
   for(int i = 0; i < targetCount; i++)
   {
      targets[i].handle = CreateSocket(AF_INET, SOCK_STREAM, 0);
      SetSocketNonBlocking(targets[i].handle);
      remoteAddr.sin_addr.s_addr = htonl(startAddr + i);
      if (connect(targets[i].handle, reinterpret_cast<sockaddr*>(&remoteAddr), sizeof(remoteAddr)) == 0)
      {
         // connected immediately
         targets[i].success = true;
         targets[i].completed = true;
      }
      else if ((WSAGetLastError() != WSAEWOULDBLOCK) && (WSAGetLastError() != WSAEINPROGRESS))
      {
         // failed immediately
         targets[i].completed = true;
      }
      else
      {
         targets[i].startTime = pollStartTime;
         waitCount++;
      }
   }

   while((waitCount > 0) && (GetCurrentTimeMs() - pollStartTime < 2000))
   {
      SocketPoller sp(true);
      for(int i = 0; i < targetCount; i++)
         if (!targets[i].completed)
            sp.add(targets[i].handle);
      int rc = sp.poll(std::max(2000 - static_cast<int>(GetCurrentTimeMs() - pollStartTime), 0));
      if (rc <= 0)
         break;

      for(int i = 0; i < targetCount; i++)
         if (!targets[i].completed && sp.isSet(targets[i].handle))
         {
            targets[i].completed = true;
            targets[i].success = sp.isReady(targets[i].handle);
            targets[i].rtt = static_cast<uint32_t>(GetCurrentTimeMs() - targets[i].startTime);
            waitCount--;
         }
   }

   for(int i = 0; i < targetCount; i++)
   {
      if (targets[i].success)
         callback(startAddr + i, targets[i].rtt, context);
      closesocket(targets[i].handle);
   }
}

/**
 * Scan range of IPv4 addresses using SNMP requests
 */
void LIBNXAGENT_EXPORTABLE TCPScanAddressRange(const InetAddress& from, const InetAddress& to, uint16_t port, void (*callback)(const InetAddress&, uint32_t, void*), void *context)
{
   for(uint32_t a = from.getAddressV4(); a <= to.getAddressV4(); a += BLOCK_SIZE)
      ScanBlock(a, std::min(a + BLOCK_SIZE - 1, to.getAddressV4()), port, callback, context);
}
