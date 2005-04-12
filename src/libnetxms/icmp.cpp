/* 
** libnetxms - Common NetXMS utility library
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: icmp.cpp
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

#define PING_SIZE    62


//
// ICMP echo request structure
//

struct ECHOREQUEST
{
   ICMPHDR m_icmpHdr;
   BYTE m_cData[PING_SIZE - 1];
};


//
// ICMP echo reply structure
//

struct ECHOREPLY
{
   IPHDR m_ipHdr;
   ICMPHDR m_icmpHdr;
   BYTE m_cData[256];
};


//
// * Checksum routine for Internet Protocol family headers (C Version)
// *
// * Author -
// *	Mike Muuss
// *	U. S. Army Ballistic Research Laboratory
// *	December, 1983
//

static WORD IPChecksum(WORD *addr, int len)
{
	int nleft = len, sum = 0;
	WORD *w = addr;
	WORD answer;

	/*
	 *  Our algorithm is simple, using a 32 bit accumulator (sum),
	 *  we add sequential 16 bit words to it, and at the end, fold
	 *  back all the carry bits from the top 16 bits into the lower
	 *  16 bits.
	 */
	while(nleft > 1)
   {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if (nleft == 1) 
   {
		WORD u = 0;

		*(BYTE *)(&u) = *(BYTE *)w ;
		sum += u;
	}

	/*
	 * add back carry outs from top 16 bits to low 16 bits
	 */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return answer;
}


//
// Do an ICMP ping to specific address
// Return value: TRUE if host is alive and FALSE otherwise
// Parameters: dwAddr - IP address with network byte order
//             iNumRetries - number of retries
//             dwTimeout - Timeout waiting for responce in milliseconds
//

DWORD LIBNETXMS_EXPORTABLE IcmpPing(DWORD dwAddr, int iNumRetries, DWORD dwTimeout, DWORD *pdwRTT)
{
   SOCKET sock;
   struct sockaddr_in saDest;
   DWORD dwResult = ICMP_TIMEOUT;
   ECHOREQUEST request;
   ECHOREPLY reply;
   DWORD dwTimeLeft, dwElapsedTime;
   INT64 qwStartTime;

   // Create raw socket
   sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
   if (sock == -1)
   {
      return ICMP_RAW_SOCK_FAILED;
   }

   // Setup destination address structure
   saDest.sin_addr.s_addr = dwAddr;
   saDest.sin_family = AF_INET;
   saDest.sin_port = 0;

   // Fill in request structure
   request.m_icmpHdr.m_cType = 8;   // ICMP ECHO REQUEST
   request.m_icmpHdr.m_cCode = 0;
   request.m_icmpHdr.m_wId = 0x1020;
   request.m_icmpHdr.m_wSeq = 0;

   // Do ping
   while(iNumRetries--)
   {
      request.m_icmpHdr.m_wSeq++;
      request.m_icmpHdr.m_wChecksum = 0;
      request.m_icmpHdr.m_wChecksum = IPChecksum((WORD *)&request, sizeof(ECHOREQUEST));
      if (sendto(sock, (char *)&request, sizeof(ECHOREQUEST), 0, (struct sockaddr *)&saDest, sizeof(struct sockaddr_in)) == sizeof(ECHOREQUEST))
      {
#ifdef __FreeBSD__ // use kquque
			int kq;
			struct kevent ke;
			struct timespec ts;
         socklen_t iAddrLen;
         struct sockaddr_in saSrc;

			kq = kqueue();
			EV_SET(&ke, sock, EVFILT_READ, EV_ADD, 0, 5, NULL);
			kevent(kq, &ke, 1, NULL, 0, NULL);

         // Wait for responce
         for(dwTimeLeft = dwTimeout; dwTimeLeft > 0;)
         {
            qwStartTime = GetCurrentTimeMs();

				ts.tv_sec = dwTimeLeft / 1000;
				ts.tv_nsec = (dwTimeLeft % 1000) * 1000 * 1000;

				memset(&ke, 0, sizeof(ke));
				if (kevent(kq, NULL, 0, &ke, 1, &ts) > 0)
#else

#if HAVE_POLL
         struct pollfd fds;
#else
         struct timeval timeout;
         fd_set rdfs;
#endif
         socklen_t iAddrLen;
         struct sockaddr_in saSrc;

         // Wait for responce
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
            qwStartTime = GetCurrentTimeMs();
#if HAVE_POLL
	         if (poll(&fds, 1, dwTimeLeft) > 0)
#else
	         if (select(sock + 1, &rdfs, NULL, NULL, &timeout) > 0)
#endif

#endif // __FreeBSD__
            {
               dwElapsedTime = (DWORD)(GetCurrentTimeMs() - qwStartTime);
               dwTimeLeft -= min(dwElapsedTime, dwTimeLeft);

               // Receive reply
               iAddrLen = sizeof(struct sockaddr_in);
               if (recvfrom(sock, (char *)&reply, sizeof(ECHOREPLY), 0, (struct sockaddr *)&saSrc, &iAddrLen) > 0)
               {
                  // Check responce
                  if ((reply.m_ipHdr.m_iaSrc.s_addr == dwAddr) && 
                      (reply.m_icmpHdr.m_cType == 0) &&
                      (reply.m_icmpHdr.m_wId == request.m_icmpHdr.m_wId) &&
                      (reply.m_icmpHdr.m_wSeq == request.m_icmpHdr.m_wSeq))
                  {
                     dwResult = ICMP_SUCCESS;   // We succeed
                     if (pdwRTT != NULL)
                        *pdwRTT = dwElapsedTime;
                     goto stop_ping;            // Stop sending packets
                  }

                  // Check for "destination unreacheable" error
                  if ((reply.m_icmpHdr.m_cType == 3) &&
                      (reply.m_icmpHdr.m_cCode == 1))    // code 1 is "host unreacheable"
                  {
                     if (((IPHDR *)reply.m_icmpHdr.m_cData)->m_iaDst.s_addr == dwAddr)
                     {
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
      }

      ThreadSleepMs(500);     // Wait half a second before sending next packet
   }

stop_ping:
   closesocket(sock);
   return dwResult;
}
