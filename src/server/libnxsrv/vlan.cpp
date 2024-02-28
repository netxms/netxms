/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
** File: vlan.cpp
**/

#include "libnxsrv.h"

/**
 * VlanList constructor
 */
VlanList::VlanList(int initialAlloc)
{
	m_allocated = initialAlloc;
	m_size = 0;
	m_data = nullptr;
	m_vlans = MemAllocArray<VlanInfo*>(m_allocated);
}

/**
 * Destructor
 */
VlanList::~VlanList()
{
	for(int i = 0; i < m_size; i++)
		delete m_vlans[i];
	MemFree(m_vlans);
}

/**
 * Add new VLAN (list becomes owner of VLAN object)
 */
void VlanList::add(VlanInfo *vlan)
{
	if (m_size == m_allocated)
	{
		m_allocated += 32;
		m_vlans = MemReallocArray(m_vlans, m_allocated);
	}
	m_vlans[m_size++] = vlan;
}

/**
 * Add member port to VLAN
 *
 * @param vlanId VLAN ID
 * @param portId port's 32bit identifier (usually ifIndex or slot/port pair)
 */
void VlanList::addMemberPort(int vlanId, uint32_t portId, bool tagged)
{
	VlanInfo *vlan = findById(vlanId);
	if (vlan != nullptr)
	{
		vlan->add(portId, tagged);
	}
}

/**
 * Add member port to VLAN
 *
 * @param vlanId VLAN ID
 * @param location port's location
 */
void VlanList::addMemberPort(int vlanId, const InterfacePhysicalLocation& location, bool tagged)
{
   VlanInfo *vlan = findById(vlanId);
   if (vlan != nullptr)
   {
      vlan->add(location, tagged);
   }
}

/**
 * Find VLAN by ID
 *
 * @param id VLAN ID
 */
VlanInfo *VlanList::findById(int id)
{
	for(int i = 0; i < m_size; i++)
		if (m_vlans[i]->getVlanId() == id)
			return m_vlans[i];
	return nullptr;
}

/**
 * Find VLAN by name
 *
 * @param name VLAN name
 */
VlanInfo *VlanList::findByName(const TCHAR *name)
{
	for(int i = 0; i < m_size; i++)
		if (!_tcsicmp(m_vlans[i]->getName(), name))
			return m_vlans[i];
	return nullptr;
}

/**
 * Fill NXCP  message with VLANs data
 */
void VlanList::fillMessage(NXCPMessage *msg)
{
	msg->setField(VID_NUM_VLANS, (UINT32)m_size);
	uint32_t fieldId = VID_VLAN_LIST_BASE;
	for(int i = 0; i < m_size; i++)
	{
		msg->setField(fieldId++, static_cast<uint16_t>(m_vlans[i]->getVlanId()));
		msg->setField(fieldId++, m_vlans[i]->getName());
		msg->setField(fieldId++, static_cast<uint32_t>(m_vlans[i]->getNumPorts()));
		for(int j = 0; j < m_vlans[i]->getNumPorts(); j++)
		{
		   const VlanPortInfo *p = m_vlans[i]->getPort(j);
	      msg->setField(fieldId++, p->ifIndex);
         msg->setField(fieldId++, p->objectId);
         msg->setField(fieldId++, p->location.chassis);
         msg->setField(fieldId++, p->location.module);
         msg->setField(fieldId++, p->location.pic);
         msg->setField(fieldId++, p->location.port);
		}
	}
}

/**
 * Add port identified by single 32bit ID (usually ifIndex)
 */
void VlanInfo::add(uint32_t portId, bool tagged)
{
   for(int i = 0; i < m_ports.size(); i++)
      if (m_ports.get(i)->portId == portId)
      {
         m_ports.get(i)->tagged = tagged;
         return;  // Already added
      }
   VlanPortInfo *port = m_ports.addPlaceholder();
	memset(port, 0, sizeof(VlanPortInfo));
	port->portId = portId;
	port->tagged = tagged;
}

/**
 * Add port identified by location
 */
void VlanInfo::add(const InterfacePhysicalLocation& location, bool tagged)
{
   for(int i = 0; i < m_ports.size(); i++)
      if (m_ports.get(i)->location.equals(location))
      {
         m_ports.get(i)->tagged = tagged;
         return;  // Already added
      }
   VlanPortInfo *port = m_ports.addPlaceholder();
   memset(port, 0, sizeof(VlanPortInfo));
   port->location = location;
   port->tagged = tagged;
}

/**
 * Set full port's information: location, ifIndex, object ID.
 * Must be called after call to prepareForResolve.
 *
 * @param index port's index
 * @param loctaion port location
 * @param ifIndex interface index
 * @param id interface object ID
 */
void VlanInfo::resolvePort(int index, const InterfacePhysicalLocation& location, uint32_t ifIndex, uint32_t id)
{
   VlanPortInfo *port = m_ports.get(index);
	if (port != nullptr)
	{
		port->location = location;
		port->ifIndex = ifIndex;
		port->objectId = id;
	}
}
