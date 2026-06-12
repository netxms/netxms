/*
** NetXMS PING subagent
** Copyright (C) 2004-2021 Victor Kirhenshtein
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
** File: scan_unix.cpp
**
**/

#include "ping.h"
#include <nxnet.h>

/**
 * ICMP echo request structure
 */
struct ECHOREQUEST
{
   ICMPHDR m_icmpHdr;
   BYTE m_data[128];
};

/**
 * ICMP echo reply structure
 */
struct ECHOREPLY
{
   IPHDR m_ipHdr;
   ICMPHDR m_icmpHdr;
   BYTE m_data[128];
};

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
 * Process ICMP response
 */
static void ProcessResponse(SOCKET s, uint32_t baseAddr, uint32_t lastAddr, ScanStatus *status, uint16_t requestId)
{
   socklen_t addrLen = sizeof(struct sockaddr_in);
   struct sockaddr_in saSrc;
   ECHOREPLY reply;
   if (recvfrom(s, (char *)&reply, sizeof(ECHOREPLY), 0, (struct sockaddr *)&saSrc, &addrLen) > 0)
   {
      // Raw ICMP sockets receive a copy of every incoming echo reply on the host, so we must
      // accept only replies to our own requests (matched by echo ID) to avoid cross-talk with
      // other concurrent scans or pollers.
      if ((reply.m_icmpHdr.m_cType == 0) && (reply.m_icmpHdr.m_wId == requestId))
      {
         uint32_t addr = ntohl(saSrc.sin_addr.s_addr);
         // startTime == 0 means we have not sent a request to this address yet in this scan
         if ((addr >= baseAddr) && (addr <= lastAddr) && !status[addr - baseAddr].success && (status[addr - baseAddr].startTime != 0))
         {
            status[addr - baseAddr].success = true;
            // Clamp to 1 so that sub-millisecond responses are distinguishable from "no RTT data"
            status[addr - baseAddr].rtt = std::max(static_cast<uint32_t>(GetCurrentTimeMs() - status[addr - baseAddr].startTime), static_cast<uint32_t>(1));

            TCHAR text[64];
            nxlog_debug_tag(DEBUG_TAG, 7, _T("ScanAddressRange: got response from %s (rtt=%u)"), InetAddress(addr).toString(text), status[addr - baseAddr].rtt);
         }
      }
   }
}

/**
 * Check for responses
 */
static void CheckForResponses(uint32_t baseAddr, uint32_t lastAddr, ScanStatus *status, SOCKET s, uint32_t timeout, uint16_t requestId)
{
   SocketPoller sp;
   for(uint32_t timeLeft = timeout; timeLeft > 0;)
   {
      sp.reset();
      sp.add(s);

      uint64_t startTime = GetCurrentTimeMs();
      if (sp.poll(timeLeft) > 0)
      {
         uint32_t elapsedTime = (uint32_t)(GetCurrentTimeMs() - startTime);
         timeLeft -= std::min(elapsedTime, timeLeft);
         ProcessResponse(s, baseAddr, lastAddr, status, requestId);
      }
      else     // select() or poll() ended on timeout
      {
         timeLeft = 0;
      }
   }
}

/**
 * Scan IP address range and return list of responding addresses
 */
StructArray<ScanResult> *ScanAddressRange(const InetAddress& start, const InetAddress& end, uint32_t timeout)
{
   if ((start.getFamily() != AF_INET) || (end.getFamily() != AF_INET) ||
       (start.getAddressV4() > end.getAddressV4()))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("ScanAddressRange: invalid arguments"), _tcserror(errno));
      return NULL;   // invalid arguments
   }

   SOCKET s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
   if (s == INVALID_SOCKET)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("ScanAddressRange: cannot open raw socket (%s)"), _tcserror(errno));
      return NULL;
   }

   TCHAR text1[64], text2[64];
   nxlog_debug_tag(DEBUG_TAG, 5, _T("ScanAddressRange: scanning %s - %s"), start.toString(text1), end.toString(text2));

   // Fill in request structure
   ECHOREQUEST request;
   request.m_icmpHdr.m_cType = 8;   // ICMP ECHO REQUEST
   request.m_icmpHdr.m_cCode = 0;
   request.m_icmpHdr.m_wId = static_cast<uint16_t>(GetCurrentThreadId());
   request.m_icmpHdr.m_wSeq = 0;
   memcpy(request.m_data, "NetXMS Scan Ping", 16);

   uint16_t requestId = request.m_icmpHdr.m_wId;

   struct sockaddr_in saDest;
   memset(&saDest, 0, sizeof(sockaddr_in));
   saDest.sin_family = AF_INET;
   saDest.sin_port = 0;

   uint32_t baseAddr = start.getAddressV4();
   uint32_t lastAddr = end.getAddressV4();
   ScanStatus *status = MemAllocArray<ScanStatus>(lastAddr - baseAddr + 1);

   for(uint32_t a = baseAddr; a <= lastAddr; a++)
   {
      saDest.sin_addr.s_addr = htonl(a);

      request.m_icmpHdr.m_wSeq++;
      request.m_icmpHdr.m_wChecksum = 0;
      request.m_icmpHdr.m_wChecksum = CalculateIPChecksum(&request, sizeof(ECHOREQUEST));

      status[a - baseAddr].startTime = GetCurrentTimeMs();
      sendto(s, (char *)&request, sizeof(ECHOREQUEST), 0, (struct sockaddr *)&saDest, sizeof(struct sockaddr_in));

      CheckForResponses(baseAddr, lastAddr, status, s, 20, requestId);
   }

   CheckForResponses(baseAddr, lastAddr, status, s, timeout, requestId);
   closesocket(s);

   StructArray<ScanResult> *results = new StructArray<ScanResult>();
   for(uint32_t a = baseAddr; a <= lastAddr; a++)
   {
      if (status[a - baseAddr].success)
      {
         ScanResult r;
         r.address = InetAddress(a);
         r.rtt = status[a - baseAddr].rtt;
         results->add(&r);
      }
   }
   MemFree(status);
   return results;
}
