/*
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
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
** File: iflist.cpp
**
**/

#include "libnxsrv.h"

/**
 * Find interface entry by ifIndex
 */
InterfaceInfo *InterfaceList::findByIfIndex(uint32_t ifIndex) const
{
   for(int i = 0; i < m_interfaces.size(); i++)
   {
      InterfaceInfo *iface = m_interfaces.get(i);
      if (iface->index == ifIndex)
			return iface;
   }
	return nullptr;
}

/**
 * Find interface entry by physical position
 */
InterfaceInfo *InterfaceList::findByPhysicalLocation(const InterfacePhysicalLocation& location) const
{
   for(int i = 0; i < m_interfaces.size(); i++)
   {
      InterfaceInfo *iface = m_interfaces.get(i);
      if (iface->isPhysicalPort && iface->location.equals(location))
         return iface;
   }
   return nullptr;
}

/**
 * Find interface by IP address
 */
InterfaceInfo *InterfaceList::findByIpAddress(const InetAddress& addr) const
{
   for(int i = 0; i < m_interfaces.size(); i++)
   {
      InterfaceInfo *iface = m_interfaces.get(i);
      if (iface->ipAddrList.hasAddress(addr))
         return iface;
   }
   return nullptr;
}
