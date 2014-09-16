/* 
** NetXMS - Network Management System
** Utility Library
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
** File: inetaddr.cpp
**
**/

#include "libnetxms.h"

/**
 * Create IPv4 address object
 */
InetAddress::InetAddress(UINT32 addr)
{
   m_family = AF_INET;
   m_addr.v4 = addr;
   m_maskBits = 32;
}

/**
 * Create IPv6 address object
 */
InetAddress::InetAddress(BYTE *addr)
{
   m_family = AF_INET6;
   memcpy(m_addr.v6, addr, 16);
   m_maskBits = 128;
}

/**
 * Create invalid address object
 */
InetAddress::InetAddress()
{
   m_family = AF_UNSPEC;
   m_maskBits = 0;
}

/**
 * Returns true if address is a wildcard address
 */
bool InetAddress::isAnyLocal() const
{
   return (m_family == AF_INET) ? (m_addr.v4 == 0) : !memcmp(m_addr.v6, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16);
}

/**
 * Returns true if address is a loopback address
 */
bool InetAddress::isLoopback() const
{
   return (m_family == AF_INET) ? ((m_addr.v4 & 0xFF000000) == 0x7F000000) : !memcmp(m_addr.v6, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01", 16);
}

/**
 * Returns true if address is a multicast address
 */
bool InetAddress::isMulticast() const
{
   return (m_family == AF_INET) ? ((m_addr.v4 >= 0xE0000000) && (m_addr.v4 != 0xFFFFFFFF)) : (m_addr.v6[0] == 0xFF);
}

/**
 * Returns true if address is a broadcast address
 */
bool InetAddress::isBroadcast() const
{
   return (m_family == AF_INET) ? (m_addr.v4 == 0xFFFFFFFF) : false;
}

/**
 * Convert to string
 */
String InetAddress::toString() const
{
   TCHAR buffer[64];
   String s = (m_family == AF_INET) ? IpToStr(m_addr.v4, buffer) : Ip6ToStr(m_addr.v6, buffer);
   return s;
}

/**
 * Convert to string
 */
TCHAR *InetAddress::toString(TCHAR *buffer) const
{
   return (m_family == AF_INET) ? IpToStr(m_addr.v4, buffer) : Ip6ToStr(m_addr.v6, buffer);
}

/**
 * Check if this InetAddress contain given InetAddress using current network mask
 */
bool InetAddress::contain(const InetAddress &a) const
{
   if (a.m_family != m_family)
      return false;

   if (m_family == AF_INET)
   {
      UINT32 mask = (m_maskBits > 0) ? (0xFFFFFFFF << (32 - m_maskBits)) : 0;
      return (a.m_addr.v4 & mask) == m_addr.v4;
   }
   else
   {
      BYTE addr[16];
      memcpy(addr, a.m_addr.v6, 16);
      if (m_maskBits < 128)
      {
         int b = m_maskBits / 8;
         int shift = m_maskBits % 8;
         BYTE mask = (shift > 0) ? (BYTE)((1 << (8 - shift)) & 0xFF) : 0;
         addr[b] &= mask;
         for(int i = b + 1; i < 16; i++)
            addr[i] = 0;
      }
      return !memcmp(addr, m_addr.v6, 16);
   }
}

/**
 * Check if two inet addresses are equals
 */
bool InetAddress::equals(const InetAddress &a) const
{
   if (a.m_family != m_family)
      return false;
   return ((m_family == AF_INET) ? (a.m_addr.v4 == m_addr.v4) : !memcmp(a.m_addr.v6, m_addr.v6, 16)) && (a.m_maskBits == m_maskBits);
}

/**
 * Compare two inet addresses
 */
int InetAddress::compareTo(const InetAddress &a) const
{
   int r = a.m_family - m_family;
   if (r != 0)
      return r;

   if (m_family == AF_INET)
   {
      return (m_addr.v4 == a.m_addr.v4) ? (m_maskBits - a.m_maskBits) : ((m_addr.v4 < a.m_addr.v4) ? -1 : 1);
   }
   else
   {
      r = memcmp(a.m_addr.v6, m_addr.v6, 16);
      return (r == 0) ? (m_maskBits - a.m_maskBits) : r;
   }
}

/**
 * Resolve hostname
 */
InetAddress InetAddress::resolveHostName(const WCHAR *hostname)
{
   char mbName[256];
   WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, hostname, -1, mbName, 256, NULL, NULL);
   return resolveHostName(mbName);
}

/**
 * Resolve hostname
 */
InetAddress InetAddress::resolveHostName(const char *hostname)
{
   // Check for IPv4 address
#ifdef _WIN32
   char hostnameCopy[256];
   strncpy(hostnameCopy, hostname, 255);

   struct sockaddr_in addr4;
   addr4.sin_family = AF_INET;
   INT addrLen = sizeof(addr4);
   if (WSAStringToAddressA(hostnameCopy, AF_INET, NULL, (struct sockaddr *)&addr4, &addrLen) == 0)
   {
      return InetAddress(ntohl(addr4.sin_addr.s_addr));
   }
#else
   struct in_addr addr4;
   if (inet_aton(hostname, &addr4))
   {
      return InetAddress(ntohl(addr4.s_addr));
   }
#endif

   // Check for IPv6 address
#ifdef _WIN32
   struct sockaddr_in6 addr6;
   addr6.sin6_family = AF_INET6;
   addrLen = sizeof(addr6);
   if (WSAStringToAddressA(hostnameCopy, AF_INET6, NULL, (struct sockaddr *)&addr6, &addrLen) == 0)
   {
      return InetAddress(addr6.sin6_addr.u.Byte);
   }
#else
   struct in6_addr addr6;
   if (inet_pton(AF_INET6, hostname, &addr6))
   {
      return InetAddress(addr6.s6_addr);
   }
#endif

   // Not a valid IP address, resolve hostname
#if HAVE_GETHOSTBYNAME2_R
   struct hostent h, *hs = NULL;
   char buffer[1024];
   int err;
   gethostbyname2_r(hostname, AF_INET, &h, buffer, 1024, &hs, &err);
#else
   struct hostent *hs = gethostbyname(hostname);
#endif
   if (hs != NULL)
   {
      if (hs->h_addrtype == AF_INET)
      {
         return InetAddress(ntohl(*((UINT32 *)hs->h_addr)));
      }
      else if (hs->h_addrtype == AF_INET6)
      {
         return InetAddress((BYTE *)hs->h_addr);
      }
   }
   return InetAddress();
}

/**
 * Create IndetAddress from struct sockaddr
 */
InetAddress InetAddress::createFromSockaddr(struct sockaddr *s)
{
   if (s->sa_family == AF_INET)
      return InetAddress(ntohl(((struct sockaddr_in *)s)->sin_addr.s_addr));
   if (s->sa_family == AF_INET6)
      return InetAddress(((struct sockaddr_in6 *)s)->sin6_addr.s6_addr);
   return InetAddress();
}
