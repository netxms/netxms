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

#ifdef WITH_IPV6

#define MAX_PACKET_SIZE   8192

#if HAVE_POLL_H
#include <poll.h>
#endif

#ifdef __HP_aCC
#pragma pack 1
#else
#pragma pack(1)
#endif

/**
 * Combined IPv6 + ICMPv6 header for checksum calculation
 */
struct PACKET_HEADER
{
   // IPv6 header
   BYTE srcAddr[16];
   BYTE destAddr[16];
   UINT32 length;
   BYTE padding[3];
   BYTE nextHeader;
   
   // ICMPv6 header
   BYTE type;
   BYTE code;
   UINT16 checksum;
   
   // Custom fields
   UINT32 id;
   UINT32 sequence;
   BYTE data[8];      // actual length may differ
};

/**
 * ICMP reply header
 */
struct ICMP6_REPLY
{
   // ICMPv6 header
   BYTE type;
   BYTE code;
   UINT16 checksum;
   
   // Custom fields
   UINT32 id;
   UINT32 sequence;
};

/**
 * ICMP error report structure
 */
struct ICMP6_ERROR_REPORT
{
   // ICMPv6 header
   BYTE type;
   BYTE code;
   UINT16 checksum;
   
   // Custom fields
   UINT32 unused;
   BYTE ipv6hdr[8];
   BYTE srcAddr[16];
   BYTE destAddr[16];
};

#ifdef __HP_aCC
#pragma pack
#else
#pragma pack()
#endif

/**
 * Find source address for given destination
 */
static bool FindSourceAddress(struct sockaddr_in6 *dest, struct sockaddr_in6 *src)
{
   int sd = socket(AF_INET6, SOCK_DGRAM, 0);
   if (sd < 0)
      return false;

   bool success = false;
   dest->sin6_port = htons(1025);
   if (connect(sd, (struct sockaddr *)dest, sizeof(struct sockaddr_in6)) != -1)
   {
      socklen_t len = sizeof(struct sockaddr_in6);
      if (getsockname(sd, (struct sockaddr *)src, &len) != -1)
      {
         src->sin6_port = 0;
         success = true;
      }
   }
   dest->sin6_port = 0;
   close(sd);
   return success;
}

/**
 * ICMPv6 checksum calculation
 */
static UINT16 CalculateChecksum(UINT16 *addr, int len)
{  
   int count = len;
   UINT32 sum = 0;

   // Sum up 2-byte values until none or only one byte left
   while(count > 1)
   {
      sum += *(addr++);
      count -= 2;
   }

   // Add left-over byte, if any
   if (count > 0)
   {
      sum += *(BYTE *)addr;
   }

   // Fold 32-bit sum into 16 bits; we lose information by doing this,
   // increasing the chances of a collision.
   // sum = (lower 16 bits) + (upper 16 bits shifted right 16 bits)
   while(sum >> 16)
   {
      sum = (sum & 0xffff) + (sum >> 16);
   }

   // Checksum is one's compliment of sum.
   return (UINT16)(~sum);
}

/**
 * Wait for reply from given address
 */
static UINT32 WaitForReply(int sock, struct sockaddr_in6 *addr, UINT32 id, UINT32 sequence, UINT32 dwTimeout, UINT32 *prtt)
{
   UINT32 rtt = 0;
   UINT32 result = ICMP_TIMEOUT;
   UINT32 dwTimeLeft, dwElapsedTime;
#if HAVE_ALLOCA
   char *buffer = (char *)alloca(MAX_PACKET_SIZE);
#else
   char *buffer = (char *)malloc(MAX_PACKET_SIZE);
#endif

   SocketPoller sp;
   socklen_t iAddrLen;
   struct sockaddr_in6 saSrc;

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
         iAddrLen = sizeof(struct sockaddr_in6);
         if (recvfrom(sock, buffer, MAX_PACKET_SIZE, 0, (struct sockaddr *)&saSrc, &iAddrLen) > 0)
         {
            // Check response
            ICMP6_REPLY *reply = (ICMP6_REPLY *)buffer;
            if (!memcmp(saSrc.sin6_addr.s6_addr, addr->sin6_addr.s6_addr, 16) &&
                (reply->type == 129) &&  // ICMPv6 Echo Reply
                (reply->id == id) &&
                (reply->sequence == sequence))
            {
               result = ICMP_SUCCESS;   // We succeed
               if (prtt != NULL)
                  *prtt = rtt;
               break;
            }

            // Check for "destination unreacheable" error
            if (((reply->type == 1) || (reply->type == 3)) &&  // 1 = Destination Unreachable, 3 = Time Exceeded
                !memcmp(((ICMP6_ERROR_REPORT *)reply)->destAddr, addr->sin6_addr.s6_addr, 16))
            {
               result = ICMP_UNREACHEABLE;
               break;
            }
         }
      }
      else     // select() or poll() ended on timeout
      {
         dwTimeLeft = 0;
      }
   }
#if !HAVE_ALLOCA
   free(buffer);
#endif
   return result;
}

/**
 * Ping IPv6 address
 */
UINT32 IcmpPing6(const InetAddress &addr, int retries, UINT32 timeout, UINT32 *rtt, UINT32 packetSize)
{
   struct sockaddr_in6 src, dest;
   addr.fillSockAddr((SockAddrBuffer *)&dest);
   if (!FindSourceAddress(&dest, &src))
      return ICMP_UNREACHEABLE;  // no route to host

   int sd = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
   if (sd < 0)
      return ICMP_RAW_SOCK_FAILED;

   // Prepare packet and calculate checksum
   static char payload[64] = "NetXMS ICMPv6 probe [01234567890]";
   size_t size = MAX(sizeof(PACKET_HEADER), MIN(packetSize, MAX_PACKET_SIZE));
#if HAVE_ALLOCA
   PACKET_HEADER *p = (PACKET_HEADER *)alloca(size);
#else
   PACKET_HEADER *p = (PACKET_HEADER *)malloc(size);
#endif
   memset(p, 0, size);
   memcpy(p->srcAddr, src.sin6_addr.s6_addr, 16);
   memcpy(p->destAddr, dest.sin6_addr.s6_addr, 16);
   p->nextHeader = 58;
   p->type = 128;  // ICMPv6 Echo Request
   p->id = getpid();
   memcpy(p->data, payload, MIN(33, size - sizeof(PACKET_HEADER) + 8));

   // Send packets
   int bytes = size - 40;  // excluding IPv6 header
   UINT32 result = ICMP_UNREACHEABLE;
#if HAVE_RAND_R
   unsigned int seed = (unsigned int)(time(NULL) * *((UINT32 *)&addr.getAddressV6()[12]));
#endif
   for(int i = 0; i < retries; i++)
   {
      p->sequence++;
      p->checksum = CalculateChecksum((UINT16 *)p, size);
      if (sendto(sd, (char *)p + 40, bytes, 0, (struct sockaddr *)&dest, sizeof(struct sockaddr_in6)) == bytes)
      {
          result = WaitForReply(sd, &dest, p->id, p->sequence, timeout, rtt);
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

   close(sd);
#if !HAVE_ALLOCA
   free(p);
#endif
   return result;
}

#else

/**
 * Ping IPv6 address (stub for non IPv6 platforms)
 */
UINT32 IcmpPing6(const InetAddress &addr, int iNumRetries, UINT32 dwTimeout, UINT32 *pdwRTT, UINT32 dwPacketSize)
{
   return ICMP_API_ERROR;
}

#endif
