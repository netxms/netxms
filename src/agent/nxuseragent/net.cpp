/*
** NetXMS user agent
** Copyright (C) 2009-2024 Raden Solutions
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
** File: net.cpp
**/

#include "nxuseragent.h"
#include <iphlpapi.h>

static StructArray<InetAddress> s_addressList;
static Mutex s_addressListLock;

/**
 * Update address list
 */
void UpdateAddressList()
{
   const ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
   ULONG size = 0;
   if (GetAdaptersAddresses(AF_UNSPEC, flags, NULL, NULL, &size) != ERROR_BUFFER_OVERFLOW)
   {
      TCHAR buffer[1024];
      nxlog_debug(3, _T("GetAdaptersAddresses failed (%s)"), GetSystemErrorText(GetLastError(), buffer, 1024));
      return;
   }

   s_addressListLock.lock();
   s_addressList.clear();
   IP_ADAPTER_ADDRESSES *buffer = (IP_ADAPTER_ADDRESSES *)MemAlloc(size);
   if (GetAdaptersAddresses(AF_UNSPEC, flags, NULL, buffer, &size) == ERROR_SUCCESS)
   {
      for (IP_ADAPTER_ADDRESSES *iface = buffer; iface != NULL; iface = iface->Next)
      {
         for (IP_ADAPTER_UNICAST_ADDRESS *pAddr = iface->FirstUnicastAddress; pAddr != NULL; pAddr = pAddr->Next)
         {
            InetAddress addr = InetAddress::createFromSockaddr(pAddr->Address.lpSockaddr);
            if (addr.isValidUnicast())
            {
               addr.setMaskBits(pAddr->OnLinkPrefixLength);
               s_addressList.add(&addr);
            }
         }
      }
   }
   s_addressListLock.unlock();
}

/**
 * Get address list size
 */
int GetAddressListSize()
{
   s_addressListLock.lock();
   int size = s_addressList.size();
   s_addressListLock.unlock();
   return size;
}

/**
 * Get address list
 */
unique_ptr<StructArray<InetAddress>> GetAddressList()
{
   s_addressListLock.lock();
   auto list = make_unique<StructArray<InetAddress>>(s_addressList);
   s_addressListLock.unlock();
   return list;
}
