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
** File: arp.cpp
**
**/

#include "libnxsrv.h"

/**
 * Constructor
 */
ArpCache::ArpCache() : m_entries(64, 64, Ownership::True), m_ipIndex(Ownership::False)
{
   m_timestamp = time(nullptr);
}

/**
 * Add entry
 */
void ArpCache::addEntry(ArpEntry *entry)
{
   entry->ipAddr.setMaskBits(0); // always use 0 bits to avoid hash map issues on lookup
   m_entries.add(entry);
   m_ipIndex.set(entry->ipAddr, entry);
}

/**
 * Find ARP entry by IP address
 */
const ArpEntry *ArpCache::findByIP(const InetAddress& addr)
{
   InetAddress key = addr;
   key.setMaskBits(0);
   return m_ipIndex.get(key);
}

/**
 * Dump to log
 */
void ArpCache::dumpToLog() const
{
   if (nxlog_get_debug_level_tag(DEBUG_TAG_TOPO_ARP) < 7)
      return;

   TCHAR buffer1[64], buffer2[64];
   for(int i = 0; i < m_entries.size(); i++)
   {
      const ArpEntry *e = m_entries.get(i);
      nxlog_debug_tag(DEBUG_TAG_TOPO_ARP, 7, _T("   %-15s = %s"), e->ipAddr.toString(buffer1), e->macAddr.toString(buffer2));
   }
}
