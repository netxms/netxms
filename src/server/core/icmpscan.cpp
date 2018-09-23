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
** File: icmpscan.cpp
**
**/

#include "nxcore.h"

#ifdef _WIN32

#include <iphlpapi.h>
#include <icmpapi.h>

/**
 * Information about queued echo request
 */
struct EchoRequest
{
   InetAddress addr;
   void *replyBuffer;
   DWORD replyBufferSize;
   void(*callback)(const InetAddress&, UINT32, void *);
   void *context;
   volatile int *pendingRequests;

   EchoRequest(const InetAddress& a, void(*cb)(const InetAddress&, UINT32, void *), void *ctx, volatile int *prq)
   {
      addr = a;
      replyBufferSize = 80 + sizeof(ICMP_ECHO_REPLY);
      replyBuffer = MemAlloc(replyBufferSize);
      memset(replyBuffer, 0, replyBufferSize);
      callback = cb;
      context = ctx;
      pendingRequests = prq;
   }
};

/**
 * Echo callback
 */
static int WINAPI EchoCallback(void *context)
{
   EchoRequest *request = static_cast<EchoRequest*>(context);
   if (IcmpParseReplies(request->replyBuffer, request->replyBufferSize) > 0)
   {
#if defined(_WIN64)
      ICMP_ECHO_REPLY32 *er = static_cast<ICMP_ECHO_REPLY32*>(request->replyBuffer);
#else
      ICMP_ECHO_REPLY *er = static_cast<ICMP_ECHO_REPLY*>(request->replyBuffer);
#endif
      if (er->Status == IP_SUCCESS)
      {
         request->callback(request->addr, er->RoundTripTime, request->context);
      }
   }
   (*request->pendingRequests)--;
   delete request;
   return 0;
}

/**
 * Scan range of IPv4 addresses
 */
void ScanAddressRange(const InetAddress& from, const InetAddress& to, void(*callback)(const InetAddress&, UINT32, void *), void *context)
{
   static char payload[64] = "NetXMS ICMP probe [range scan]";

   HANDLE hIcmpFile = IcmpCreateFile();
   if (hIcmpFile == INVALID_HANDLE_VALUE)
      return;

   volatile int pendingRequests = 0;
   for(UINT32 a = from.getAddressV4(); a <= to.getAddressV4(); a++)
   {
      EchoRequest *rq = new EchoRequest(a, callback, context, &pendingRequests);
      DWORD rc = IcmpSendEcho2(hIcmpFile, NULL, (FARPROC)EchoCallback, rq, htonl(a), payload, 64, NULL, rq->replyBuffer, rq->replyBufferSize, g_icmpPingTimeout);
      if ((rc == 0) && (GetLastError() == ERROR_IO_PENDING))
      {
         pendingRequests++;
      }
      else
      {
         delete rq;
      }
   }

   while(pendingRequests > 0)
      SleepEx(1000, TRUE);

   IcmpCloseHandle(hIcmpFile);
}

#else /* _WIN32 */

/**
 * ICMP echo request structure
 */
struct ECHOREQUEST
{
   ICMPHDR m_icmpHdr;
   BYTE m_cData[64];
};

/**
 * ICMP echo reply structure
 */
struct ECHOREPLY
{
   IPHDR m_ipHdr;
   ICMPHDR m_icmpHdr;
   BYTE m_cData[64];
};

/**
 * Scan status for each address
 */
struct ScanStatus
{
   INT64 startTime;
   bool success;
   UINT32 rtt;
};

/**
 * Process ICMP response
 */
static void ProcessResponse(SOCKET sock, UINT32 baseAddr, UINT32 lastAddr, ScanStatus *status)
{
   ECHOREPLY reply;
   struct sockaddr_in saSrc;
   socklen_t addrLen = sizeof(struct sockaddr_in);
   if (recvfrom(sock, (char *)&reply, sizeof(ECHOREPLY), 0, (struct sockaddr *)&saSrc, &addrLen) > 0)
   {
      UINT32 addr = ntohl(reply.m_ipHdr.m_iaSrc.s_addr);
      if ((addr >= baseAddr) && (addr <= lastAddr) &&
          (reply.m_icmpHdr.m_cType == 0) &&
          !status[addr - baseAddr].success)
      {
         status[addr - baseAddr].success = true;
         status[addr - baseAddr].rtt = static_cast<UINT32>(GetCurrentTimeMs() - status[addr - baseAddr].startTime);
      }
   }
}

/**
* Scan range of IPv4 addresses
*/
void ScanAddressRange(const InetAddress& from, const InetAddress& to, void(*callback)(const InetAddress&, UINT32, void *), void *context)
{
   SOCKET sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
   if (sock == INVALID_SOCKET)
      return;

   ECHOREQUEST request;
   memset(&request, 0, sizeof(ECHOREQUEST));
   request.m_icmpHdr.m_cType = 8;   // ICMP ECHO REQUEST
   request.m_icmpHdr.m_cCode = 0;
   request.m_icmpHdr.m_wId = (WORD)GetCurrentThreadId();
   request.m_icmpHdr.m_wSeq = 0;

   struct sockaddr_in saDest;
   memset(&saDest, 0, sizeof(sockaddr_in));
   saDest.sin_family = AF_INET;
   saDest.sin_port = 0;

   SocketPoller sp;
   ScanStatus *status = MemAllocArray<ScanStatus>(to.getAddressV4() - from.getAddressV4() + 1);
   UINT32 baseAddr = from.getAddressV4();
   for(UINT32 a = baseAddr, i = 0; a <= to.getAddressV4(); a++, i++)
   {
      request.m_icmpHdr.m_wSeq++;
      request.m_icmpHdr.m_wChecksum = 0;
      request.m_icmpHdr.m_wChecksum = CalculateIPChecksum(&request, sizeof(ECHOREQUEST));
      saDest.sin_addr.s_addr = htonl(a);
      status[i].startTime = GetCurrentTimeMs();
      status[i].success = false;
      sendto(sock, (char *)&request, sizeof(ECHOREQUEST), 0, (struct sockaddr *)&saDest, sizeof(struct sockaddr_in));

      sp.reset();
      sp.add(sock);
      if (sp.poll(10) > 0)
      {
         ProcessResponse(sock, baseAddr, to.getAddressV4(), status);
      }
   }

   UINT32 elapsedTime = 0;
   while(elapsedTime < g_icmpPingTimeout)
   {
      sp.reset();
      sp.add(sock);
      INT64 startTime = GetCurrentTimeMs();
      if (sp.poll(g_icmpPingTimeout - elapsedTime) <= 0)
         break;

      ProcessResponse(sock, baseAddr, to.getAddressV4(), status);
      elapsedTime += static_cast<UINT32>(GetCurrentTimeMs() - startTime);
   }

   closesocket(sock);

   for(UINT32 a = baseAddr, i = 0; a <= to.getAddressV4(); a++, i++)
   {
      if (status[i].success)
         callback(a, status[i].rtt, context);
   }
   MemFree(status);
}

#endif   /* _WIN32 */
