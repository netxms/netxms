/*
** NetXMS - Network Management System
** Copyright (C) 2003-2011 Victor Kirhenshtein
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
 * Constructor
 */
InterfaceList::InterfaceList(int initialAlloc)
{
	m_interfaces = new ObjectArray<InterfaceInfo>(initialAlloc, 32, true);
	m_data = NULL;
   m_needPrefixWalk = false;
}

/**
 * Destructor
 */
InterfaceList::~InterfaceList()
{
	delete m_interfaces;
}

/**
 * Find interface entry by ifIndex
 */
InterfaceInfo *InterfaceList::findByIfIndex(UINT32 ifIndex)
{
   // Delete loopback interface(s) from list
   for(int i = 0; i < m_interfaces->size(); i++)
      if (m_interfaces->get(i)->index == ifIndex)
			return m_interfaces->get(i);
	return NULL;
}
