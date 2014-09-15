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
Inet4Address::Inet4Address(UINT32 addr)
{
   m_addr = addr;
   m_maskBits = 32;
}

/**
 * Clone IPv4 address object
 */
InetAddress *Inet4Address::clone()
{
   Inet4Address *a = new Inet4Address(m_addr);
   a->m_maskBits = m_maskBits;
   return a;
}

/**
 * Returns true if address is a wildcard address
 */
bool Inet4Address::isAnyLocal()
{
   return m_addr == 0;
}

/**
 * Returns true if address is a loopback address
 */
bool Inet4Address::isLoopback()
{
   return (m_addr & 0xFF000000) == 0x7F000000;
}

/**
 * Returns true if address is a multicast address
 */
bool Inet4Address::isMulticast()
{
   return (m_addr >= 0xE0000000) && (m_addr != 0xFFFFFFFF);
}

/**
 * Returns true if address is a broadcast address
 */
bool Inet4Address::isBroadcast()
{
   return m_addr == 0xFFFFFFFF;
}

/**
 * Get address family
 */
int Inet4Address::getFamily()
{
   return AF_INET;
}

/**
 * Convert to string
 */
String Inet4Address::toString()
{
   TCHAR buffer[32];
   String s = IpToStr(m_addr, buffer);
   return s;
}

/**
 * Convert to string
 */
TCHAR *Inet4Address::toString(TCHAR *buffer)
{
   return IpToStr(m_addr, buffer);
}

/**
 * Check if this InetAddress contain given InetAddress using current network mask
 */
bool Inet4Address::contain(InetAddress *a)
{
   if (a->getFamily() != getFamily())
      return false;

   UINT32 mask = (m_maskBits > 0) ? (0xFFFFFFFF << (32 - m_maskBits)) : 0;
   return (((Inet4Address *)a)->m_addr & mask) == m_addr;
}

/**
 * Check if two inet addresses are equals
 */
bool Inet4Address::equals(InetAddress *a)
{
   if (a->getFamily() != getFamily())
      return false;
   return (((Inet4Address *)a)->m_addr == m_addr) && (a->getMaskBits() == m_maskBits);
}

/**
 * Compare two inet addresses
 */
int Inet4Address::compareTo(InetAddress *a)
{
   int r = a->getFamily() - getFamily();
   if (r != 0)
      return r;

   return (m_addr == ((Inet4Address *)a)->m_addr) ? (m_maskBits - a->getMaskBits()) : ((m_addr < ((Inet4Address *)a)->m_addr) ? -1 : 1);
}

/**
 * Create IPv6 address object
 */
Inet6Address::Inet6Address(BYTE *addr)
{
   memcpy(m_addr, addr, 16);
   m_maskBits = 128;
}

/**
 * Clone IPv6 address object
 */
InetAddress *Inet6Address::clone()
{
   Inet6Address *a = new Inet6Address(m_addr);
   a->m_maskBits = m_maskBits;
   return a;
}

/**
 * Returns true if address is a wildcard address
 */
bool Inet6Address::isAnyLocal()
{
   return !memcmp(m_addr, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16);
}

/**
 * Returns true if address is a loopback address
 */
bool Inet6Address::isLoopback()
{
   return !memcmp(m_addr, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01", 16);
}

/**
 * Returns true if address is a multicast address
 */
bool Inet6Address::isMulticast()
{
   return m_addr[0] == 0xFF;
}

/**
 * Returns true if address is a broadcast address
 */
bool Inet6Address::isBroadcast()
{
   return false;
}

/**
 * Get address family
 */
int Inet6Address::getFamily()
{
   return AF_INET6;
}

/**
 * Convert to string
 */
String Inet6Address::toString()
{
   TCHAR buffer[32];
   String s = Ip6ToStr(m_addr, buffer);
   return s;
}

/**
 * Convert to string
 */
TCHAR *Inet6Address::toString(TCHAR *buffer)
{
   return Ip6ToStr(m_addr, buffer);
}

/**
 * Check if this InetAddress contain given InetAddress using current network mask
 */
bool Inet6Address::contain(InetAddress *a)
{
   if (a->getFamily() != getFamily())
      return false;

   BYTE addr[16];
   memcpy(addr, ((Inet6Address *)a)->m_addr, 16);
   if (m_maskBits < 128)
   {
      int b = m_maskBits / 8;
      int shift = m_maskBits % 8;
      BYTE mask = (shift > 0) ? (BYTE)((1 << (8 - shift)) & 0xFF) : 0;
      addr[b] &= mask;
      for(int i = b + 1; i < 16; i++)
         addr[i] = 0;
   }
   return !memcmp(addr, m_addr, 16);
}

/**
 * Check if two inet addresses are equals
 */
bool Inet6Address::equals(InetAddress *a)
{
   if (a->getFamily() != getFamily())
      return false;
   return !memcmp(((Inet6Address *)a)->m_addr, m_addr, 16) && (a->getMaskBits() == m_maskBits);
}

/**
 * Compare two inet addresses
 */
int Inet6Address::compareTo(InetAddress *a)
{
   int r = a->getFamily() - getFamily();
   if (r != 0)
      return r;

   r = memcmp(((Inet6Address *)a)->m_addr, m_addr, 16);
   return (r == 0) ? (m_maskBits - a->getMaskBits()) : r;
}

/**
 * Resolve hostname
 */
InetAddress *InetAddress::resolveHostName(const WCHAR *hostname)
{
   char mbName[256];
   WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, hostname, -1, mbName, 256, NULL, NULL);
   return resolveHostName(mbName);
}

/**
 * Resolve hostname
 */
InetAddress *InetAddress::resolveHostName(const char *hostname)
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
      return new Inet4Address(ntohl(addr4.sin_addr.s_addr));
   }
#else
   struct in_addr addr4;
   if (inet_aton(hostname, &addr4))
   {
      return new Inet4Address(ntohl(addr4.s_addr));
   }
#endif

   // Check for IPv6 address
#ifdef _WIN32
   struct sockaddr_in6 addr6;
   addr6.sin6_family = AF_INET6;
   addrLen = sizeof(addr6);
   if (WSAStringToAddressA(hostnameCopy, AF_INET6, NULL, (struct sockaddr *)&addr6, &addrLen) == 0)
   {
      return new Inet6Address(addr6.sin6_addr.u.Byte);
   }
#else
   struct in6_addr addr6;
   if (inet_pton(AF_INET6, hostname, &addr6))
   {
      return new Inet6Address(addr6.s6_addr);
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
         return new Inet4Address(ntohl(*((UINT32 *)hs->h_addr)));
      }
      else if (hs->h_addrtype == AF_INET6)
      {
         return new Inet6Address((BYTE *)hs->h_addr);
      }
   }
   return NULL;
}

/**
 * Create IndetAddress from struct sockaddr
 */
InetAddress *InetAddress::createFromSockaddr(struct sockaddr *s)
{
   if (s->sa_family == AF_INET)
      return new Inet4Address(ntohl(((struct sockaddr_in *)s)->sin_addr.s_addr));
   if (s->sa_family == AF_INET6)
      return new Inet6Address(((struct sockaddr_in6 *)s)->sin6_addr.u.Byte);
   return NULL;
}
