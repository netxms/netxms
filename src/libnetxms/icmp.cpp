/* 
** libnetxms - Common NetXMS utility library
** Copyright (C) 2003-2011 Victor Kirhenshtein
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
** File: icmp.cpp
**
**/

#include "libnetxms.h"
#include <nms_threads.h>

#if HAVE_POLL_H
#include <poll.h>
#endif

//
// Constants
//

#define MAX_PING_SIZE      8192
#define ICMP_REQUEST_ID    0x5050


//
// ICMP echo request structure
//

struct ECHOREQUEST
{
   ICMPHDR m_icmpHdr;
   BYTE m_cData[MAX_PING_SIZE - sizeof(ICMPHDR) - sizeof(IPHDR)];
};


//
// ICMP echo reply structure
//

struct ECHOREPLY
{
   IPHDR m_ipHdr;
   ICMPHDR m_icmpHdr;
   BYTE m_cData[MAX_PING_SIZE - sizeof(ICMPHDR) - sizeof(IPHDR)];
};


//
// * Checksum routine for Internet Protocol family headers (C Version)
// *
// * Author -
// *	Mike Muuss
// *	U. S. Army Ballistic Research Laboratory
// *	December, 1983
//

static WORD IPChecksum(BYTE *addr, int len)
{
	int nleft = len;
	DWORD sum = 0;
	BYTE *curr = addr;

	/*
	 *  Our algorithm is simple, using a 32 bit accumulator (sum),
	 *  we add sequential 16 bit words to it, and at the end, fold
	 *  back all the carry bits from the top 16 bits into the lower
	 *  16 bits.
	 */
	while(nleft > 1)
   {
		sum += ((WORD)(*curr << 8) | (WORD)(*(curr + 1)));
		curr += 2;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if (nleft == 1) 
		sum += (WORD)(*curr);

	/*
	 * add back carry outs from top 16 bits to low 16 bits
	 */
	while(sum >> 16)
		sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	return htons((WORD)(~sum));
}


//
// Do an ICMP ping to specific address
// Return value: TRUE if host is alive and FALSE otherwise
// Parameters: dwAddr - IP address with network byte order
//             iNumRetries - number of retries
//             dwTimeout - Timeout waiting for response in milliseconds
//

DWORD LIBNETXMS_EXPORTABLE IcmpPing(DWORD dwAddr, int iNumRetries,
                                    DWORD dwTimeout, DWORD *pdwRTT,
                                    DWORD dwPacketSize)
{
   SOCKET sock;
   struct sockaddr_in saDest;
   DWORD dwResult = ICMP_TIMEOUT;
   ECHOREQUEST request;
   ECHOREPLY reply;
   DWORD dwTimeLeft, dwElapsedTime, dwRTT;
   int nBytes;
   INT64 qwStartTime;
   static char szPayload[64] = "NetXMS ICMP probe [01234567890]";

#ifdef _WIN32
	LARGE_INTEGER pc, pcFreq;
	BOOL pcSupported = QueryPerformanceFrequency(&pcFreq);
	QWORD pcTicksPerMs = pcFreq.QuadPart / 1000;	// Ticks per millisecond
#endif

   // Check packet size
   if (dwPacketSize < sizeof(ICMPHDR) + sizeof(IPHDR))
      dwPacketSize = sizeof(ICMPHDR) + sizeof(IPHDR);
   else if (dwPacketSize > MAX_PING_SIZE)
      dwPacketSize = MAX_PING_SIZE;

   // Create raw socket
   sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
   if (sock == -1)
   {
      return ICMP_RAW_SOCK_FAILED;
   }

   // Setup destination address structure
   memset(&saDest, 0, sizeof(sockaddr_in));
   saDest.sin_addr.s_addr = dwAddr;
   saDest.sin_family = AF_INET;
   saDest.sin_port = 0;

   // Fill in request structure
   request.m_icmpHdr.m_cType = 8;   // ICMP ECHO REQUEST
   request.m_icmpHdr.m_cCode = 0;
   request.m_icmpHdr.m_wId = ICMP_REQUEST_ID;
   request.m_icmpHdr.m_wSeq = 0;
   memcpy(request.m_cData, szPayload, min(dwPacketSize - sizeof(ICMPHDR) - sizeof(IPHDR), 64));

   // Do ping
   nBytes = dwPacketSize - sizeof(IPHDR);
   while(iNumRetries--)
   {
      dwRTT = 0;  // Round-trip time for current request
      request.m_icmpHdr.m_wId = ICMP_REQUEST_ID;
      request.m_icmpHdr.m_wSeq++;
      request.m_icmpHdr.m_wChecksum = 0;
      request.m_icmpHdr.m_wChecksum = IPChecksum((BYTE *)&request, nBytes);
      if (sendto(sock, (char *)&request, nBytes, 0, (struct sockaddr *)&saDest, sizeof(struct sockaddr_in)) == nBytes)
      {
#ifdef USE_KQUEUE
			int kq;
			struct kevent ke;
			struct timespec ts;
         socklen_t iAddrLen;
         struct sockaddr_in saSrc;

			kq = kqueue();
			EV_SET(&ke, sock, EVFILT_READ, EV_ADD, 0, 5, NULL);
			kevent(kq, &ke, 1, NULL, 0, NULL);

         // Wait for response
         for(dwTimeLeft = dwTimeout; dwTimeLeft > 0;)
         {
            qwStartTime = GetCurrentTimeMs();

				ts.tv_sec = dwTimeLeft / 1000;
				ts.tv_nsec = (dwTimeLeft % 1000) * 1000 * 1000;

				memset(&ke, 0, sizeof(ke));
				if (kevent(kq, NULL, 0, &ke, 1, &ts) > 0)
#else    /* not USE_KQUEUE */

#if HAVE_POLL
         struct pollfd fds;
#else
         struct timeval timeout;
         fd_set rdfs;
#endif
         socklen_t iAddrLen;
         struct sockaddr_in saSrc;

         // Wait for response
         for(dwTimeLeft = dwTimeout; dwTimeLeft > 0;)
         {
#if HAVE_POLL
            fds.fd = sock;
            fds.events = POLLIN;
            fds.revents = POLLIN;
#else
	         FD_ZERO(&rdfs);
	         FD_SET(sock, &rdfs);
	         timeout.tv_sec = dwTimeLeft / 1000;
	         timeout.tv_usec = (dwTimeLeft % 1000) * 1000;
#endif

#ifdef _WIN32
				if (pcSupported)
				{
					QueryPerformanceCounter(&pc);
				}
				else
				{
	            qwStartTime = GetCurrentTimeMs();
				}
#else		/* _WIN32 */
            qwStartTime = GetCurrentTimeMs();
#endif	/* _WIN32 else */

#if HAVE_POLL
	         if (poll(&fds, 1, dwTimeLeft) > 0)
#else
	         if (select(SELECT_NFDS(sock + 1), &rdfs, NULL, NULL, &timeout) > 0)
#endif

#endif   /* USE_KQUEUE else */
            {
#ifdef _WIN32
					if (pcSupported)
					{
						LARGE_INTEGER pcCurr;
						QueryPerformanceCounter(&pcCurr);
						dwElapsedTime = (DWORD)((pcCurr.QuadPart - pc.QuadPart) / pcTicksPerMs);
					}
					else
					{
	               dwElapsedTime = (DWORD)(GetCurrentTimeMs() - qwStartTime);
					}
#else
               dwElapsedTime = (DWORD)(GetCurrentTimeMs() - qwStartTime);
#endif
               dwTimeLeft -= min(dwElapsedTime, dwTimeLeft);
               dwRTT += dwElapsedTime;

               // Receive reply
               iAddrLen = sizeof(struct sockaddr_in);
               if (recvfrom(sock, (char *)&reply, sizeof(ECHOREPLY), 0, (struct sockaddr *)&saSrc, &iAddrLen) > 0)
               {
                  // Check response
                  if ((reply.m_ipHdr.m_iaSrc.s_addr == dwAddr) && 
                      (reply.m_icmpHdr.m_cType == 0) &&
                      (reply.m_icmpHdr.m_wId == ICMP_REQUEST_ID) &&
                      (reply.m_icmpHdr.m_wSeq == request.m_icmpHdr.m_wSeq))
                  {
#ifdef USE_KQUEUE
			            close(kq);
#endif
                     dwResult = ICMP_SUCCESS;   // We succeed
                     if (pdwRTT != NULL)
                        *pdwRTT = dwRTT;
                     goto stop_ping;            // Stop sending packets
                  }

                  // Check for "destination unreacheable" error
                  if ((reply.m_icmpHdr.m_cType == 3) &&
                      (reply.m_icmpHdr.m_cCode == 1))    // code 1 is "host unreacheable"
                  {
                     if (((IPHDR *)reply.m_cData)->m_iaDst.s_addr == dwAddr)
                     {
#ifdef USE_KQUEUE
			               close(kq);
#endif
                        dwResult = ICMP_UNREACHEABLE;
                        goto stop_ping;      // Host unreacheable, stop trying
                     }
                  }
               }
            }
            else     // select() or poll() ended on timeout
            {
               dwTimeLeft = 0;
            }
         }
#ifdef USE_KQUEUE
			close(kq);
#endif
      }

      ThreadSleepMs(500);     // Wait half a second before sending next packet
   }

stop_ping:
   closesocket(sock);
   return dwResult;
}
