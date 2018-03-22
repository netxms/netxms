/*
** NetXMS - Network Management System
** Copyright (C) 2003-2018 Victor Kirhenshtein
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
ArpCache::ArpCache()
{
   m_entries = new ObjectArray<ArpEntry>(64, 64, true);
   m_ipIndex = new HashMap<InetAddress, ArpEntry>(false);
   m_timestamp = time(NULL);
}

/**
 * Destructor
 */
ArpCache::~ArpCache()
{
   delete m_entries;
   delete m_ipIndex;
}

/**
 * Add entry
 */
void ArpCache::addEntry(ArpEntry *entry)
{
   m_entries->add(entry);
   m_ipIndex->set(entry->ipAddr, entry);
}
