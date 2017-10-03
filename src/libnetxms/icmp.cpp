/* 
** libnetxms - Common NetXMS utility library
** Copyright (C) 2003-2015 Victor Kirhenshtein
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


//
// Constants
//

#define MAX_PING_SIZE      8192
#define ICMP_REQUEST_ID    0x5050


#ifdef _WIN32

#include <iphlpapi.h>
#include <icmpapi.h>

/**
 * Do an ICMP ping to specific address
 * Return value: TRUE if host is alive and FALSE otherwise
 * Parameters: addr - IP address
 *             iNumRetries - number of retries
 *             dwTimeout - Timeout waiting for response in milliseconds
 */
UINT32 LIBNETXMS_EXPORTABLE IcmpPing(const InetAddress &addr, int iNumRetries, UINT32 dwTimeout, UINT32 *pdwRTT, UINT32 dwPacketSize)
{
   static char payload[MAX_PING_SIZE] = "NetXMS ICMP probe [01234567890]";

   HANDLE hIcmpFile = (addr.getFamily() == AF_INET) ? IcmpCreateFile() : Icmp6CreateFile();
	if (hIcmpFile == INVALID_HANDLE_VALUE)
		return ICMP_API_ERROR;

   DWORD replySize = dwPacketSize + 16 + ((addr.getFamily() == AF_INET) ? sizeof(ICMP_ECHO_REPLY) : sizeof(ICMPV6_ECHO_REPLY));
	char *reply = (char *)alloca(replySize);
	int retries = iNumRetries;
	UINT32 rc = ICMP_API_ERROR;
	do
	{
		if (addr.getFamily() == AF_INET)
      {
         rc = IcmpSendEcho(hIcmpFile, htonl(addr.getAddressV4()), payload, (WORD)dwPacketSize, NULL, reply, replySize, dwTimeout);
      }
      else
      {
         sockaddr_in6 sa, da;

         memset(&sa, 0, sizeof(sa));
         sa.sin6_addr = in6addr_any;
         sa.sin6_family = AF_INET6;

         memset(&da, 0, sizeof(da));
         da.sin6_family = AF_INET6;
         memcpy(da.sin6_addr.s6_addr, addr.getAddressV6(), 16);

         IP_OPTION_INFORMATION opt;
         memset(&opt, 0, sizeof(opt));
         opt.Ttl = 127;
         rc = Icmp6SendEcho2(hIcmpFile, NULL, NULL, NULL, &sa, &da, payload, (WORD)dwPacketSize, &opt, reply, replySize, dwTimeout);
      }
		if (rc != 0)
		{
         ULONG status;
         ULONG rtt;
         if (addr.getFamily() == AF_INET)
         {
#if defined(_WIN64)
			   ICMP_ECHO_REPLY32 *er = (ICMP_ECHO_REPLY32 *)reply;
#else
			   ICMP_ECHO_REPLY *er = (ICMP_ECHO_REPLY *)reply;
#endif
            status = er->Status;
            rtt = er->RoundTripTime;
         }
         else
         {
			   ICMPV6_ECHO_REPLY *er = (ICMPV6_ECHO_REPLY *)reply;
            status = er->Status;
            rtt = er->RoundTripTime;
         }

			switch(status)
			{
				case IP_SUCCESS:
					rc = ICMP_SUCCESS;
					if (pdwRTT != NULL)
						*pdwRTT = rtt;
					break;
				case IP_REQ_TIMED_OUT:
					rc = ICMP_TIMEOUT;
					break;
				case IP_BUF_TOO_SMALL:
				case IP_NO_RESOURCES:
				case IP_PACKET_TOO_BIG:
				case IP_GENERAL_FAILURE:
					rc = ICMP_API_ERROR;
					break;
				default:
					rc = ICMP_UNREACHEABLE;
					break;
			}
		}
		else
		{
			rc = (GetLastError() == IP_REQ_TIMED_OUT) ? ICMP_TIMEOUT : ICMP_API_ERROR;
		}
		retries--;
	}
	while((rc != ICMP_SUCCESS) && (retries > 0));

	IcmpCloseHandle(hIcmpFile);
	return rc;
}

#else	/* not _WIN32 */

/**
 * ICMP echo request structure
 */
struct ECHOREQUEST
{
   ICMPHDR m_icmpHdr;
   BYTE m_cData[MAX_PING_SIZE - sizeof(ICMPHDR) - sizeof(IPHDR)];
};

/**
 * ICMP echo reply structure
 */
struct ECHOREPLY
{
   IPHDR m_ipHdr;
   ICMPHDR m_icmpHdr;
   BYTE m_cData[MAX_PING_SIZE - sizeof(ICMPHDR) - sizeof(IPHDR)];
};

/**
 * Checksum routine for Internet Protocol family headers (C Version)
 *
 * Author -
 *	Mike Muuss
 *	U. S. Army Ballistic Research Laboratory
 *	December, 1983
 */
static WORD IPChecksum(BYTE *addr, int len)
{
	int nleft = len;
	UINT32 sum = 0;
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

/**
 * Wait for reply from given address
 */
static UINT32 WaitForReply(int sock, UINT32 addr, UINT16 sequence, UINT32 dwTimeout, UINT32 *prtt)
{
   UINT32 rtt = 0;
   UINT32 result = ICMP_TIMEOUT;
   UINT32 dwTimeLeft, dwElapsedTime;
   ECHOREPLY reply;

   SocketPoller sp;
   socklen_t iAddrLen;
   struct sockaddr_in saSrc;

   // Wait for response
   for(dwTimeLeft = dwTimeout; dwTimeLeft > 0;)
   {
      sp.reset();
      sp.add(sock);

      UINT64 qwStartTime = GetCurrentTimeMs();
      if (sp.poll(dwTimeLeft) > 0)
		{
			dwElapsedTime = (UINT32)(GetCurrentTimeMs() - qwStartTime);
			dwTimeLeft -= std::min(dwElapsedTime, dwTimeLeft);
			rtt += dwElapsedTime;

			// Receive reply
			iAddrLen = sizeof(struct sockaddr_in);
			if (recvfrom(sock, (char *)&reply, sizeof(ECHOREPLY), 0, (struct sockaddr *)&saSrc, &iAddrLen) > 0)
			{
				// Check response
				if ((reply.m_ipHdr.m_iaSrc.s_addr == addr) && 
					 (reply.m_icmpHdr.m_cType == 0) &&
					 (reply.m_icmpHdr.m_wId == ICMP_REQUEST_ID) &&
					 (reply.m_icmpHdr.m_wSeq == sequence))
				{
					result = ICMP_SUCCESS;   // We succeed
					if (prtt != NULL)
						*prtt = rtt;
               break;
				}

				// Check for "destination unreachable" error
				if ((reply.m_icmpHdr.m_cType == 3) &&
					 (reply.m_icmpHdr.m_cCode == 1))    // code 1 is "host unreachable"
				{
					if (((IPHDR *)reply.m_cData)->m_iaDst.s_addr == addr)
					{
						result = ICMP_UNREACHEABLE;
						break;
					}
				}
			}
		}
		else     // select() or poll() ended on timeout
		{
			dwTimeLeft = 0;
		}
	}
   return result;
}

/**
 * Do an ICMP ping to specific IPv4 address
 * Return value: TRUE if host is alive and FALSE otherwise
 * Parameters: addr - IP address in network byte order
 *             iNumRetries - number of retries
 *             dwTimeout - Timeout waiting for response in milliseconds
 */
static UINT32 LIBNETXMS_EXPORTABLE IcmpPing4(UINT32 addr, int retries, UINT32 timeout, UINT32 *rtt, UINT32 packetSize)
{
   static char szPayload[64] = "NetXMS ICMP probe [01234567890]";

   // Check packet size
   if (packetSize < sizeof(ICMPHDR) + sizeof(IPHDR))
      packetSize = sizeof(ICMPHDR) + sizeof(IPHDR);
   else if (packetSize > MAX_PING_SIZE)
      packetSize = MAX_PING_SIZE;

   // Create raw socket
   SOCKET sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
   if (sock == INVALID_SOCKET)
   {
      return ICMP_RAW_SOCK_FAILED;
   }

   // Setup destination address structure
   struct sockaddr_in saDest;
   memset(&saDest, 0, sizeof(sockaddr_in));
   saDest.sin_addr.s_addr = addr;
   saDest.sin_family = AF_INET;
   saDest.sin_port = 0;

   // Fill in request structure
   ECHOREQUEST request;
   request.m_icmpHdr.m_cType = 8;   // ICMP ECHO REQUEST
   request.m_icmpHdr.m_cCode = 0;
   request.m_icmpHdr.m_wId = ICMP_REQUEST_ID;
   request.m_icmpHdr.m_wSeq = 0;
   memcpy(request.m_cData, szPayload, MIN(packetSize - sizeof(ICMPHDR) - sizeof(IPHDR), 64));

   UINT32 result = ICMP_TIMEOUT;

   // Do ping
#if HAVE_RAND_R
   unsigned int seed = (unsigned int)(time(NULL) * addr);
#endif
   int bytes = packetSize - sizeof(IPHDR);
   for(int i = 0; i < retries; i++)
   {
      request.m_icmpHdr.m_wId = ICMP_REQUEST_ID;
      request.m_icmpHdr.m_wSeq++;
      request.m_icmpHdr.m_wChecksum = 0;
      request.m_icmpHdr.m_wChecksum = IPChecksum((BYTE *)&request, bytes);
      if (sendto(sock, (char *)&request, bytes, 0, (struct sockaddr *)&saDest, sizeof(struct sockaddr_in)) == bytes)
      {
          result = WaitForReply(sock, addr, request.m_icmpHdr.m_wSeq, timeout, rtt);
          if (result != ICMP_TIMEOUT)
             break;  // success or fatal error
      }

      UINT32 minDelay = 500 * i; // min = 0 in first run, then wait longer and longer
      UINT32 maxDelay = 200 + minDelay * 2;  // increased random window between retries
#if HAVE_RAND_R
      UINT32 delay = minDelay + (rand_r(&seed) % maxDelay);
#else
      UINT32 delay = minDelay + (UINT32)(GetCurrentTimeMs() % maxDelay);
#endif
      ThreadSleepMs(delay);
   }

   closesocket(sock);
   return result;
}

/**
 * Ping IPv6 address
 */
UINT32 IcmpPing6(const InetAddress &addr, int retries, UINT32 timeout, UINT32 *rtt, UINT32 packetSize);

/**
 * Do an ICMP ping to specific IP address
 * Return value: TRUE if host is alive and FALSE otherwise
 * Parameters: addr - IP address
 *             iNumRetries - number of retries
 *             dwTimeout - Timeout waiting for response in milliseconds
 */
UINT32 LIBNETXMS_EXPORTABLE IcmpPing(const InetAddress &addr, int iNumRetries, UINT32 dwTimeout, UINT32 *pdwRTT, UINT32 dwPacketSize)
{
   if (addr.getFamily() == AF_INET)
      return IcmpPing4(htonl(addr.getAddressV4()), iNumRetries, dwTimeout, pdwRTT, dwPacketSize);
#ifdef WITH_IPV6
   if (addr.getFamily() == AF_INET6)
      return IcmpPing6(addr, iNumRetries, dwTimeout, pdwRTT, dwPacketSize);
#endif
   return ICMP_API_ERROR;
}

#endif
