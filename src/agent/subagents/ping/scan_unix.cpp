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
 * Add responding address to result set
 */
static void AddResult(StructArray<InetAddress> *results, const InetAddress& addr)
{
   for(int i = 0; i < results->size(); i++)
      if (results->get(i)->equals(addr))
         return;  // already added
   results->add(&addr);

   TCHAR text[64];
   nxlog_debug_tag(DEBUG_TAG, 7, _T("ScanAddressRange: got response from %s"), addr.toString(text));
}

/**
 * Check for responses
 */
static void CheckForResponses(const InetAddress& start, const InetAddress& end, StructArray<InetAddress> *results, SOCKET s, uint32_t timeout)
{
   SocketPoller sp;
   for(UINT32 timeLeft = timeout; timeLeft > 0;)
   {
      sp.reset();
      sp.add(s);

      UINT64 startTime = GetCurrentTimeMs();
      if (sp.poll(timeLeft) > 0)
      {
         UINT32 elapsedTime = (UINT32)(GetCurrentTimeMs() - startTime);
         timeLeft -= std::min(elapsedTime, timeLeft);

         // Receive reply
         socklen_t addrLen = sizeof(struct sockaddr_in);
         struct sockaddr_in saSrc;
         ECHOREPLY reply;
         if (recvfrom(s, (char *)&reply, sizeof(ECHOREPLY), 0, (struct sockaddr *)&saSrc, &addrLen) > 0)
         {
            // Check response
            if (reply.m_icmpHdr.m_cType == 0)
            {
               InetAddress addr = InetAddress::createFromSockaddr((struct sockaddr *)&saSrc);
               if (addr.inRange(start, end))
               {
                  AddResult(results, addr);
               }
            }
         }
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
StructArray<InetAddress> *ScanAddressRange(const InetAddress& start, const InetAddress& end, uint32_t timeout)
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

   StructArray<InetAddress> *results = new StructArray<InetAddress>();

   // Fill in request structure
   ECHOREQUEST request;
   request.m_icmpHdr.m_cType = 8;   // ICMP ECHO REQUEST
   request.m_icmpHdr.m_cCode = 0;
   request.m_icmpHdr.m_wId = static_cast<uint16_t>(GetCurrentThreadId());
   request.m_icmpHdr.m_wSeq = 0;
   memcpy(request.m_data, "NetXMS Scan Ping", 16);

   struct sockaddr_in saDest;
   memset(&saDest, 0, sizeof(sockaddr_in));
   saDest.sin_family = AF_INET;
   saDest.sin_port = 0;

   for(uint32_t a = start.getAddressV4(); a <= end.getAddressV4(); a++)
   {
      saDest.sin_addr.s_addr = htonl(a);

      request.m_icmpHdr.m_wSeq++;
      request.m_icmpHdr.m_wChecksum = 0;
      request.m_icmpHdr.m_wChecksum = CalculateIPChecksum(&request, sizeof(ECHOREQUEST));

      sendto(s, (char *)&request, sizeof(ECHOREQUEST), 0, (struct sockaddr *)&saDest, sizeof(struct sockaddr_in));

      CheckForResponses(start, end, results, s, 20);
   }

   CheckForResponses(start, end, results, s, timeout);
   closesocket(s);
   return results;
}
