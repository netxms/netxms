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
	m_allocated = initialAlloc;
	m_size = 0;
	m_data = NULL;
	m_interfaces = (NX_INTERFACE_INFO *)malloc(sizeof(NX_INTERFACE_INFO) * m_allocated);
   m_needPrefixWalk = false;
}

/**
 * Destructor
 */
InterfaceList::~InterfaceList()
{
	safe_free(m_interfaces);
}

/**
 * Add new interface
 */
void InterfaceList::add(NX_INTERFACE_INFO *iface)
{
	if (m_size == m_allocated)
	{
		m_allocated += 32;
		m_interfaces = (NX_INTERFACE_INFO *)realloc(m_interfaces, sizeof(NX_INTERFACE_INFO) * m_allocated);
	}
	memcpy(&m_interfaces[m_size++], iface, sizeof(NX_INTERFACE_INFO));
}

/**
 * Find interface entry by ifIndex
 */
NX_INTERFACE_INFO *InterfaceList::findByIfIndex(UINT32 ifIndex)
{
   // Delete loopback interface(s) from list
   for(int i = 0; i < m_size; i++)
		if (m_interfaces[i].index == ifIndex)
			return &m_interfaces[i];
	return NULL;
}

/**
 * Remove entry
 */
void InterfaceList::remove(int index)
{
	if ((index < 0) || (index >= m_size))
		return;

	m_size--;
	memmove(&m_interfaces[index], &m_interfaces[index + 1], sizeof(NX_INTERFACE_INFO) * (m_size - index));
}
