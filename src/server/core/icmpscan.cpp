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
* Scan range of IPv4 addresses
*/
void ScanAddressRange(const InetAddress& from, const InetAddress& to, void(*callback)(const InetAddress&, UINT32, void *), void *context)
{
   // FIXME: UNIX implementation
}

#endif   /* _WIN32 */
