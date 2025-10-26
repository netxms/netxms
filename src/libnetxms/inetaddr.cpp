/*
** NetXMS - Network Management System
** Utility Library
** Copyright (C) 2003-2024 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the**
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
#include <nxcpapi.h>

/**
 * Invalid address
 */
const InetAddress __EXPORT InetAddress::INVALID;

/**
 * Loopback address (IPv4)
 */
const InetAddress __EXPORT InetAddress::LOOPBACK(0x7F000001);

/**
 * No address indicator (IPv4 0.0.0.0)
 */
const InetAddress __EXPORT InetAddress::NONE((uint32_t)0);

/**
 * IPv4 link local subnet
 */
const InetAddress __EXPORT InetAddress::IPV4_LINK_LOCAL(0xA9FE0000, 0xFFFF0000);

/**
 * IPv6 link local subnet
 */
const InetAddress __EXPORT InetAddress::IPV6_LINK_LOCAL((const BYTE *)"\xfe\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 10);

/**
 * Convert to string
 */
String InetAddress::toString() const
{
   TCHAR buffer[64];
   return String((m_family == AF_UNSPEC) ? _T("UNSPEC") : ((m_family == AF_INET) ? IpToStr(m_addr.v4, buffer) : Ip6ToStr(m_addr.v6, buffer)));
}

/**
 * Convert to string
 */
TCHAR *InetAddress::toString(TCHAR *buffer) const
{
   if (m_family == AF_UNSPEC)
   {
      _tcscpy(buffer, _T("UNSPEC"));
      return buffer;
   }
   return (m_family == AF_INET) ? IpToStr(m_addr.v4, buffer) : Ip6ToStr(m_addr.v6, buffer);
}

#ifdef UNICODE

/**
 * Convert to string (single byte version)
 */
char *InetAddress::toStringA(char *buffer) const
{
   if (m_family == AF_UNSPEC)
   {
      strcpy(buffer, "UNSPEC");
      return buffer;
   }
   return (m_family == AF_INET) ? IpToStrA(m_addr.v4, buffer) : Ip6ToStrA(m_addr.v6, buffer);
}

#endif

/**
 * Convert IP address to sequence of SNMP OID elements
 */
void InetAddress::toOID(uint32_t *oid) const
{
   if (m_family == AF_INET)
   {
      *oid++ = m_addr.v4 >> 24;
      *oid++ = (m_addr.v4 >> 16) & 0xFF;
      *oid++ = (m_addr.v4 >> 8) & 0xFF;
      *oid = m_addr.v4 & 0xFF;
   }
   else if (m_family == AF_INET6)
   {
      for(int i = 0; i < 16; i++)
         oid[i] = m_addr.v6[i];
   }
}

/**
 * Convert to JSON object
 */
json_t *InetAddress::toJson() const
{
   json_t *root = json_object();
   json_object_set_new(root, "family", json_integer(m_family));

   char buffer[64];
   if (m_family == AF_INET)
      json_object_set_new(root, "address", json_string(IpToStrA(m_addr.v4, buffer)));
   else if (m_family == AF_INET6)
      json_object_set_new(root, "address", json_string(Ip6ToStrA(m_addr.v6, buffer)));

   json_object_set_new(root, "prefixLength", json_integer(m_maskBits));
   return root;
}

/**
 * Build hash key. Supplied array must be at least 18 bytes long.
 */
BYTE *InetAddress::buildHashKey(BYTE *key) const
{
   if (m_family == AF_INET)
   {
      key[0] = 6;
      key[1] = AF_INET;
      memcpy(&key[2], &m_addr.v4, 4);
      memset(&key[6], 0, 12);
   }
   else
   {
      key[0] = 18;
      key[1] = AF_INET6;
      memcpy(&key[2], &m_addr.v6, 16);
   }
   return key;
}

/**
 * Get corresponding subnet address for this InetAddress
 */
InetAddress InetAddress::getSubnetAddress() const
{
   InetAddress addr(*this);
   if ((m_family == AF_INET) && (m_maskBits < 32))
   {
      addr.m_addr.v4 = (m_maskBits == 0) ? 0 : (m_addr.v4 & (0xFFFFFFFF << (32 - m_maskBits)));
   }
   else if ((m_family == AF_INET6) && (m_maskBits < 128))
   {
      int b = m_maskBits / 8;
      int shift = m_maskBits % 8;
      BYTE mask = (shift > 0) ? (BYTE)(0xFF << (8 - shift)) : 0;
      addr.m_addr.v6[b] &= mask;
      for(int i = b + 1; i < 16; i++)
         addr.m_addr.v6[i] = 0;
   }
   return addr;
}

/**
 * Get corresponding subnet broadcast address for this InetAddress
 */
InetAddress InetAddress::getSubnetBroadcast() const
{
   InetAddress addr(*this);
   if ((m_family == AF_INET) && (m_maskBits < 32))
   {
      addr.m_addr.v4 = m_addr.v4 | (0xFFFFFFFF >> m_maskBits);
   }
   return addr;
}

/**
 * Check if this address is a subnet broadcast for given subnet mask length
 */
bool InetAddress::isSubnetBroadcast(int maskBits) const
{
   if (m_family == AF_INET)
   {
      uint32_t mask = 0xFFFFFFFF << (32 - maskBits);
      return (m_addr.v4 & (~mask)) == (~mask);
   }
   return false;
}

/**
 * Check if this InetAddress contains given InetAddress using current network mask
 */
bool InetAddress::contains(const InetAddress &a) const
{
   if (a.m_family != m_family)
      return false;

   if (m_family == AF_INET)
   {
      uint32_t mask = (m_maskBits > 0) ? (0xFFFFFFFF << (32 - m_maskBits)) : 0;
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
         BYTE mask = (shift > 0) ? (BYTE)(0xFF << (8 - shift)) : 0;
         addr[b] &= mask;
         for(int i = b + 1; i < 16; i++)
            addr[i] = 0;
      }
      return !memcmp(addr, m_addr.v6, 16);
   }
}

/**
 * Check if this InetAddress are in same subnet with given InetAddress
 * (using mask bits from this InetAddress)
 */
bool InetAddress::sameSubnet(const InetAddress &a) const
{
   if (a.m_family != m_family)
      return false;

   if (m_family == AF_INET)
   {
      uint32_t mask = (m_maskBits > 0) ? (0xFFFFFFFF << (32 - m_maskBits)) : 0;
      return (a.m_addr.v4 & mask) == (m_addr.v4 & mask);
   }
   else
   {
      BYTE addr1[16], addr2[16];
      memcpy(addr1, a.m_addr.v6, 16);
      memcpy(addr2, m_addr.v6, 16);
      if (m_maskBits < 128)
      {
         int b = m_maskBits / 8;
         int shift = m_maskBits % 8;
         BYTE mask = (shift > 0) ? (BYTE)(0xFF << (8 - shift)) : 0;
         addr1[b] &= mask;
         addr2[b] &= mask;
         for(int i = b + 1; i < 16; i++)
         {
            addr1[i] = 0;
            addr2[i] = 0;
         }
      }
      return !memcmp(addr1, addr2, 16);
   }
}

/**
 * Check if two inet addresses are equal
 * This method ignores mask bits and only compares addresses.
 * To compare two address/mask pairs use InetAddress::compareTo.
 */
bool InetAddress::equals(const InetAddress &a) const
{
   if (a.m_family != m_family)
      return false;
   return ((m_family == AF_INET) ? (a.m_addr.v4 == m_addr.v4) : !memcmp(a.m_addr.v6, m_addr.v6, 16));
}

/**
 * Compare two inet addresses. Returns -1 if this address is lesser then given, and 1 if greater.
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
      r = memcmp(m_addr.v6, a.m_addr.v6, 16);
      return (r == 0) ? (m_maskBits - a.m_maskBits) : r;
   }
}

/**
 * Check if address is within given range
 */
bool InetAddress::inRange(const InetAddress& start, const InetAddress& end) const
{
   if ((start.m_family != end.m_family) || (start.m_family != m_family))
      return false;

   if (m_family == AF_INET)
      return (m_addr.v4 >= start.m_addr.v4) && (m_addr.v4 <= end.m_addr.v4);

   if (m_family == AF_INET6)
      return (memcmp(m_addr.v6, start.m_addr.v6, 16) >= 0) && (memcmp(m_addr.v6, end.m_addr.v6, 16) <= 0);

   return false;
}

/**
 * Fill sockaddr structure
 */
struct sockaddr *InetAddress::fillSockAddr(SockAddrBuffer *buffer, UINT16 port) const
{
   if (!isValid())
      return nullptr;

   memset(buffer, 0, sizeof(SockAddrBuffer));
   ((struct sockaddr *)buffer)->sa_family = m_family;
   if (m_family == AF_INET)
   {
      buffer->sa4.sin_addr.s_addr = htonl(m_addr.v4);
      buffer->sa4.sin_port = htons(port);
   }
   else
   {
#ifdef WITH_IPV6
      memcpy(buffer->sa6.sin6_addr.s6_addr, m_addr.v6, 16);
      buffer->sa6.sin6_port = htons(port);
#else
      return nullptr;
#endif
   }
   return (struct sockaddr *)buffer;
}

/**
 * Get host name by IP address
 *
 * @param buffer buffer to place host name to
 * @param buflen buffer length in characters
 * @return buffer on success, NULL on failure
 */
TCHAR *InetAddress::getHostByAddr(TCHAR *buffer, size_t buflen) const
{
   if (!isValid())
      return nullptr;

#ifdef _WIN32

   SOCKADDR_INET sa;
   memset(&sa, 0, sizeof(sa));
   if (m_family == AF_INET)
   {
      sa.Ipv4.sin_family = AF_INET;
      sa.Ipv4.sin_addr.s_addr = htonl(m_addr.v4);
   }
   else
   {
      sa.Ipv6.sin6_family = AF_INET6;
      memcpy(sa.Ipv6.sin6_addr.s6_addr, m_addr.v6, 16);
   }
   if (GetNameInfo(reinterpret_cast<SOCKADDR*>(&sa), SA_LEN(reinterpret_cast<SOCKADDR*>(&sa)),
         buffer, static_cast<DWORD>(buflen), nullptr, 0, NI_NAMEREQD) != 0)
      return nullptr;

#elif HAVE_GETNAMEINFO

   // Use getnameinfo - thread-safe and modern approach
   struct sockaddr_storage sa;
   socklen_t saLen;
   memset(&sa, 0, sizeof(sa));

   if (m_family == AF_INET)
   {
      struct sockaddr_in *sa4 = reinterpret_cast<struct sockaddr_in*>(&sa);
      sa4->sin_family = AF_INET;
      sa4->sin_addr.s_addr = htonl(m_addr.v4);
      saLen = sizeof(struct sockaddr_in);
   }
   else if (m_family == AF_INET6)
   {
#ifdef WITH_IPV6
      struct sockaddr_in6 *sa6 = reinterpret_cast<struct sockaddr_in6*>(&sa);
      sa6->sin6_family = AF_INET6;
      memcpy(sa6->sin6_addr.s6_addr, m_addr.v6, 16);
      saLen = sizeof(struct sockaddr_in6);
#else
      return nullptr;
#endif
   }
   else
   {
      return nullptr;
   }

   char hostBuffer[NI_MAXHOST];
   if (getnameinfo(reinterpret_cast<struct sockaddr*>(&sa), saLen, 
                   hostBuffer, sizeof(hostBuffer), nullptr, 0, NI_NAMEREQD) != 0)
      return nullptr;

#ifdef UNICODE
   mb_to_wchar(hostBuffer, -1, buffer, buflen);
   buffer[buflen - 1] = 0;
#else
   strlcpy(buffer, hostBuffer, buflen);
#endif

#else /* not _WIN32 && not HAVE_GETNAMEINFO */

   struct hostent *hs = nullptr;
#if HAVE_GETHOSTBYADDR_R
   // Use gethostbyaddr_r - thread-safe reentrant version
   struct hostent hostbuf;
   char tempBuffer[1024];
   int h_errnop;
#endif

   if (m_family == AF_INET)
   {
      uint32_t addr = htonl(m_addr.v4);
#if HAVE_GETHOSTBYADDR_R
      if (gethostbyaddr_r(reinterpret_cast<const char*>(&addr), 4, AF_INET, 
                         &hostbuf, tempBuffer, sizeof(tempBuffer), &hs, &h_errnop) != 0)
         hs = nullptr;
#else
      hs = gethostbyaddr((const char *)&addr, 4, AF_INET);
#endif
   }
   else if (m_family == AF_INET6)
   {
#if HAVE_GETHOSTBYADDR_R
      if (gethostbyaddr_r(reinterpret_cast<const char*>(m_addr.v6), 16, AF_INET6, 
                         &hostbuf, tempBuffer, sizeof(tempBuffer), &hs, &h_errnop) != 0)
         hs = nullptr;
#else
      hs = gethostbyaddr((const char *)m_addr.v6, 16, AF_INET6);
#endif
   }

   if (hs == nullptr)
      return nullptr;

   // Some versions of libc may return IP address instead of NULL if it cannot be resolved
   if (equals(InetAddress::parse(hs->h_name)))
      return nullptr;

#ifdef UNICODE
   mb_to_wchar(hs->h_name, -1, buffer, buflen);
   buffer[buflen - 1] = 0;
#else
   strlcpy(buffer, hs->h_name, buflen);
#endif

#endif   /* _WIN32 */

   return buffer;
}

/**
 * Resolve hostname
 */
InetAddress InetAddress::resolveHostName(const WCHAR *hostname, int af)
{
   char mbName[256];
   WideCharToMultiByteSysLocale(hostname, mbName, 256);
   return resolveHostName(mbName, af);
}

/**
 * Resolve hostname. Address family could be set to AF_UNSPEC to allow any
 * valid address to be returned, or to AF_INET or AF_INET6 so limit possible
 * protocols.
 */
InetAddress InetAddress::resolveHostName(const char *hostname, int af)
{
   InetAddress addr = parse(hostname);
   if (addr.isValid())
      return addr;

   // Not a valid IP address, resolve hostname
#if HAVE_GETADDRINFO
   struct addrinfo *ai, hints;
   memset(&hints, 0, sizeof(hints));
   hints.ai_family = af;
   if (getaddrinfo(hostname, nullptr, &hints, &ai) == 0)
   {
      addr = InetAddress::createFromSockaddr(ai->ai_addr);
      freeaddrinfo(ai);
      return addr;
   }
#else
#if HAVE_GETHOSTBYNAME2_R
   struct hostent h, *hs = nullptr;
   char buffer[1024];
   int err;
   gethostbyname2_r(hostname, (af != AF_UNSPEC) ? af : AF_INET, &h, buffer, 1024, &hs, &err);
#else
   struct hostent *hs = gethostbyname(hostname);
#endif
   if (hs != nullptr)
   {
      if ((hs->h_addrtype == AF_INET) && ((af == AF_UNSPEC) || (af == AF_INET)))
      {
         return InetAddress(ntohl(*((uint32_t*)hs->h_addr)));
      }
      else if ((hs->h_addrtype == AF_INET6) && ((af == AF_UNSPEC) || (af == AF_INET6)))
      {
         return InetAddress((BYTE *)hs->h_addr);
      }
   }
#endif /* HAVE_GETADDRINFO */
   return InetAddress();
}

/**
 * Parse string as IP address
 */
InetAddress InetAddress::parse(const WCHAR *str)
{
   if ((str == nullptr) || (*str == 0))
      return InetAddress();

#ifdef _WIN32
   WCHAR strCopy[256];
   wcslcpy(strCopy, str, 256);

   struct sockaddr_in addr4;
   addr4.sin_family = AF_INET;
   INT addrLen = sizeof(addr4);
   if (WSAStringToAddressW(strCopy, AF_INET, nullptr, reinterpret_cast<SOCKADDR*>(&addr4), &addrLen) == 0)
   {
      return InetAddress(ntohl(addr4.sin_addr.s_addr));
   }

   // Check for IPv6 address
   struct sockaddr_in6 addr6;
   addr6.sin6_family = AF_INET6;
   addrLen = sizeof(addr6);
   if (WSAStringToAddressW(strCopy, AF_INET6, nullptr, reinterpret_cast<SOCKADDR*>(&addr6), &addrLen) == 0)
   {
      return InetAddress(addr6.sin6_addr.u.Byte);
   }

   return InetAddress();
#else
   char mb[256];
   wchar_to_mb(str, -1, mb, 256);
   return parse(mb);
#endif
}

/**
 * Parse string as IP address
 */
InetAddress InetAddress::parse(const char *str)
{
   if ((str == nullptr) || (*str == 0))
      return InetAddress();

#ifdef _WIN32
   WCHAR wc[256];
   mb_to_wchar(str, -1, wc, 256);
   return parse(wc);
#else
   // Check for IPv4 address
   struct in_addr addr4;
   if (inet_pton(AF_INET, str, &addr4))
   {
      return InetAddress(ntohl(addr4.s_addr));
   }

   // Check for IPv6 address
   struct in6_addr addr6;
   if (inet_pton(AF_INET6, str, &addr6))
   {
      return InetAddress(addr6.s6_addr);
   }

   return InetAddress();
#endif
}

/**
 * Parse IPv4 address with mask in dotted notation
 */
InetAddress InetAddress::parse(const WCHAR *addrStr, const WCHAR *maskStr)
{
   if ((addrStr == nullptr) || (*addrStr == 0) || (maskStr == nullptr) || (*maskStr == 0))
      return InetAddress();

#ifdef _WIN32
   WCHAR addrCopy[256], maskCopy[256];
   wcslcpy(addrCopy, addrStr, 256);
   wcslcpy(maskCopy, maskStr, 256);

   struct sockaddr_in addr, mask;
   addr.sin_family = AF_INET;
   mask.sin_family = AF_INET;
   INT addrLen = sizeof(addr), maskLen = sizeof(mask);
   if ((WSAStringToAddressW(addrCopy, AF_INET, nullptr, (struct sockaddr *)&addr, &addrLen) == 0) &&
       (WSAStringToAddressW(maskCopy, AF_INET, nullptr, (struct sockaddr *)&mask, &maskLen) == 0))
   {
      return InetAddress(ntohl(addr.sin_addr.s_addr), BitsInMask(ntohl(mask.sin_addr.s_addr)));
   }
   return InetAddress();
#else
   char mbAddr[256], mbMask[256];
   wchar_to_mb(addrStr, -1, mbAddr, 256);
   wchar_to_mb(maskStr, -1, mbMask, 256);
   return parse(mbAddr, mbMask);
#endif
}

/**
 * Parse IPv4 address with mask in dotted notation
 */
InetAddress InetAddress::parse(const char *addrStr, const char *maskStr)
{
   if ((addrStr == nullptr) || (*addrStr == 0) || (maskStr == nullptr) || (*maskStr == 0))
      return InetAddress();

#ifdef _WIN32
   WCHAR wcAddr[256], wcMask[256];
   mb_to_wchar(addrStr, -1, wcAddr, 256);
   mb_to_wchar(maskStr, -1, wcMask, 256);
   return parse(wcAddr, wcMask);
#else
   struct in_addr addr, mask;
   if (inet_aton(addrStr, &addr) && inet_aton(maskStr, &mask))
   {
      return InetAddress(ntohl(addr.s_addr), BitsInMask(ntohl(mask.s_addr)));
   }
   return InetAddress();
#endif
}

/**
 * Create IndetAddress from struct sockaddr
 */
InetAddress InetAddress::createFromSockaddr(const struct sockaddr *s)
{
   if (s->sa_family == AF_INET)
      return InetAddress(ntohl(reinterpret_cast<const struct sockaddr_in*>(s)->sin_addr.s_addr));
#ifdef WITH_IPV6
   if (s->sa_family == AF_INET6)
      return InetAddress(reinterpret_cast<const struct sockaddr_in6*>(s)->sin6_addr.s6_addr);
#endif
   return InetAddress();
}

/**
 * Add address to list
 */
void InetAddressList::add(const InetAddress &addr)
{
   if (!hasAddress(addr))
      m_list.add(new InetAddress(addr));
}

/**
 * Add multiple addresses to list
 */
void InetAddressList::add(const InetAddressList &addrList)
{
   for(int i = 0; i < addrList.m_list.size(); i++)
      add(*(addrList.m_list.get(i)));
}

/**
 * Remove address from list
 */
void InetAddressList::remove(const InetAddress &addr)
{
   int index = indexOf(addr);
   if (index != -1)
      m_list.remove(index);
}

/**
 * Replace IP address (update it's properties - currently only network mask)
 */
void InetAddressList::replace(const InetAddress& addr)
{
   int index = indexOf(addr);
   if (index != -1)
   {
      m_list.get(index)->setMaskBits(addr.getMaskBits());
   }
}

/**
 * Get index in list of given address
 */
int InetAddressList::indexOf(const InetAddress &addr) const
{
   for(int i = 0; i < m_list.size(); i++)
      if (m_list.get(i)->equals(addr))
         return i;
   return -1;
}

/**
 * Get first valid unicast address from the list
 */
const InetAddress& InetAddressList::getFirstUnicastAddress() const
{
   for(int i = 0; i < m_list.size(); i++)
   {
      InetAddress *a = m_list.get(i);
      if (a->isValidUnicast())
         return *a;
   }
   return InetAddress::INVALID;
}

/**
 * Get first valid IPv4 unicast address from the list
 */
const InetAddress& InetAddressList::getFirstUnicastAddressV4() const
{
   for(int i = 0; i < m_list.size(); i++)
   {
      InetAddress *a = m_list.get(i);
      if ((a->getFamily() == AF_INET) && a->isValidUnicast())
         return *a;
   }
   return InetAddress::INVALID;
}

/**
 * Check if given address is within same subnet as one of addresses on this list
 */
const InetAddress& InetAddressList::findSameSubnetAddress(const InetAddress& addr) const
{
   for(int i = 0; i < m_list.size(); i++)
   {
      InetAddress *a = m_list.get(i);
      if (!a->isAnyLocal() && !a->isBroadcast() && !a->isMulticast() && a->sameSubnet(addr))
         return *a;
   }
   return InetAddress::INVALID;
}

/**
 * Check if all addresses in list are loopback
 */
bool InetAddressList::isLoopbackOnly() const
{
   if (m_list.size() == 0)
      return false;
   for(int i = 0; i < m_list.size(); i++)
   {
      if (!m_list.get(i)->isLoopback())
         return false;
   }
   return true;
}

/**
 * Fill NXCP message
 */
void InetAddressList::fillMessage(NXCPMessage *msg, uint32_t baseFieldId, uint32_t sizeFieldId) const
{
   msg->setField(sizeFieldId, m_list.size());
   uint32_t fieldId = baseFieldId;
   for(int i = 0; i < m_list.size(); i++)
   {
      msg->setField(fieldId++, *m_list.get(i));
   }
}

/**
 * Convert address list to string representation
 */
String InetAddressList::toString(const TCHAR *separator) const
{
   if (m_list.isEmpty())
      return String();

   StringBuffer sb;
   TCHAR text[64];
   sb.append(m_list.get(0)->toString(text));
   sb.append(_T('/'));
   sb.append(m_list.get(0)->getMaskBits());

   for(int i = 1; i < m_list.size(); i++)
   {
      sb.append(separator);
      sb.append(m_list.get(i)->toString(text));
      sb.append(_T('/'));
      sb.append(m_list.get(i)->getMaskBits());
   }

   return sb;
}

/**
 * Resolve host name to address list
 */
InetAddressList *InetAddressList::resolveHostName(const WCHAR *hostname)
{
   char mbName[256];
   wchar_to_mb(hostname, -1, mbName, 256);
   return resolveHostName(mbName);
}

/**
 * Resolve host name to address list
 */
InetAddressList *InetAddressList::resolveHostName(const char *hostname)
{
   InetAddressList *result = new InetAddressList();

   InetAddress addr = InetAddress::parse(hostname);
   if (addr.isValid())
   {
      result->add(addr);
      return result;
   }

   // Not a valid IP address, resolve hostname
#if HAVE_GETADDRINFO
   struct addrinfo *ai;
   if (getaddrinfo(hostname, nullptr, nullptr, &ai) == 0)
   {
      for(struct addrinfo *p = ai; p->ai_next != nullptr; p = p->ai_next)
         result->add(InetAddress::createFromSockaddr(p->ai_addr));
      freeaddrinfo(ai);
   }
#elif HAVE_GETHOSTBYNAME2_R
   struct hostent h, *hs = nullptr;
   char buffer[4096];
   int err;
   gethostbyname2_r(hostname, AF_INET, &h, buffer, 4096, &hs, &err);
   if (hs != NULL)
   {
      if (hs->h_addrtype == AF_INET)
      {
         for(int i = 0; hs->h_addr_list[i] != nullptr; i++)
            result->add(InetAddress(ntohl(*((UINT32 *)hs->h_addr_list[i])));
      }
      else if (hs->h_addrtype == AF_INET6)
      {
         for(int i = 0; hs->h_addr_list[i] != nullptr; i++)
            result->add(InetAddress((BYTE *)hs->h_addr_list[i]));
      }
   }

   hs = NULL;
   gethostbyname2_r(hostname, AF_INET6, &h, buffer, 4096, &hs, &err);
   if (hs != nullptr)
   {
      if (hs->h_addrtype == AF_INET)
      {
         for(int i = 0; hs->h_addr_list[i] != nullptr; i++)
            result->add(InetAddress(ntohl(*((UINT32 *)hs->h_addr_list[i])));
      }
      else if (hs->h_addrtype == AF_INET6)
      {
         for(int i = 0; hs->h_addr_list[i] != nullptr; i++)
            result->add(InetAddress((BYTE *)hs->h_addr_list[i]));
      }
   }
#else
   struct hostent *hs = gethostbyname(hostname);
   if (hs != nullptr)
   {
      if (hs->h_addrtype == AF_INET)
      {
         for(int i = 0; hs->h_addr_list[i] != nullptr; i++)
            result->add(InetAddress(ntohl(*((UINT32 *)hs->h_addr_list[i])));
      }
      else if (hs->h_addrtype == AF_INET6)
      {
         for(int i = 0; hs->h_addr_list[i] != nullptr; i++)
            result->add(InetAddress((BYTE *)hs->h_addr_list[i]));
      }
   }
#endif /* HAVE_GETADDRINFO */
   return result;
}
