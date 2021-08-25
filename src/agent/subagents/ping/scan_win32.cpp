/*
** NetXMS PING subagent
** Copyright (C) 2004-2019 Victor Kirhenshtein
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
** File: scan_win32.cpp
**
**/

#include "ping.h"
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
   StructArray<InetAddress> *results;
   volatile int *pendingRequests;

   EchoRequest(const InetAddress& a, StructArray<InetAddress> *r, volatile int *prq)
   {
      addr = a;
      replyBufferSize = 80 + sizeof(ICMP_ECHO_REPLY);
      replyBuffer = MemAlloc(replyBufferSize);
      memset(replyBuffer, 0, replyBufferSize);
      results = r;
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
         TCHAR text[64];
         nxlog_debug_tag(DEBUG_TAG, 7, _T("ScanAddressRange: got response from %s"), request->addr.toString(text));
         request->results->add(&request->addr);
      }
   }
   (*request->pendingRequests)--;
   delete request;
   return 0;
}

/**
  * Scan IP address range and return list of responding addresses
  */
StructArray<InetAddress> *ScanAddressRange(const InetAddress& start, const InetAddress& end, uint32_t timeout)
{
   static char payload[64] = "NetXMS Scan Ping";

   if ((start.getFamily() != AF_INET) || (end.getFamily() != AF_INET) ||
      (start.getAddressV4() > end.getAddressV4()))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("ScanAddressRange: invalid arguments"), _tcserror(errno));
      return NULL;   // invalid arguments
   }

   HANDLE hIcmpFile = IcmpCreateFile();
   if (hIcmpFile == INVALID_HANDLE_VALUE)
   {
      TCHAR buffer[1024];
      nxlog_debug_tag(DEBUG_TAG, 5, _T("ScanAddressRange: cannot create ICMP handle (%s)"), GetSystemErrorText(GetLastError(), buffer, 1024));
      return NULL;
   }

   TCHAR text1[64], text2[64];
   nxlog_debug_tag(DEBUG_TAG, 5, _T("ScanAddressRange: scanning %s - %s"), start.toString(text1), end.toString(text2));

   StructArray<InetAddress> *results = new StructArray<InetAddress>();

   volatile int pendingRequests = 0;
   for(UINT32 a = start.getAddressV4(); a <= end.getAddressV4(); a++)
   {
      EchoRequest *rq = new EchoRequest(a, results, &pendingRequests);
      DWORD rc = IcmpSendEcho2(hIcmpFile, NULL, (FARPROC)EchoCallback, rq, htonl(a), payload, 64, NULL, rq->replyBuffer, rq->replyBufferSize, timeout);
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
   return results;
}
