/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2016 Victor Kirhenshtein
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
	m_data = NULL;
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
void VlanList::addMemberPort(int vlanId, uint32_t portId)
{
	VlanInfo *vlan = findById(vlanId);
	if (vlan != nullptr)
	{
		vlan->add(portId);
	}
}

/**
 * Add member port to VLAN
 *
 * @param vlanId VLAN ID
 * @param location port's location
 */
void VlanList::addMemberPort(int vlanId, const InterfacePhysicalLocation& location)
{
   VlanInfo *vlan = findById(vlanId);
   if (vlan != nullptr)
   {
      vlan->add(location);
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
	UINT32 fieldId = VID_VLAN_LIST_BASE;
	for(int i = 0; i < m_size; i++)
	{
		msg->setField(fieldId++, (UINT16)m_vlans[i]->getVlanId());
		msg->setField(fieldId++, m_vlans[i]->getName());
		msg->setField(fieldId++, (UINT32)m_vlans[i]->getNumPorts());
		for(int j = 0; j < m_vlans[i]->getNumPorts(); j++)
		{
		   VlanPortInfo *p = &(m_vlans[i]->getPorts()[j]);
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
 * VlanInfo constructor
 */
VlanInfo::VlanInfo(int vlanId, int prm)
{
	m_vlanId = vlanId;
	m_portRefMode = prm;
	m_name = NULL;
	m_allocated = 64;
	m_numPorts = 0;
	m_ports = MemAllocArray<VlanPortInfo>(m_allocated);
	m_nodeId = 0;
}

/**
 * VlanInfo constructor for creating independent object
 */
VlanInfo::VlanInfo(const VlanInfo *src, uint32_t nodeId)
{
   m_vlanId = src->m_vlanId;
   m_portRefMode = src->m_portRefMode;
   m_name = MemCopyString(src->m_name);
   m_allocated = src->m_numPorts;
   m_numPorts = src->m_numPorts;
   m_ports = MemCopyArray(src->m_ports, m_numPorts);
   m_nodeId = nodeId;
}

/**
 * VlanInfo destructor
 */
VlanInfo::~VlanInfo()
{
	MemFree(m_ports);
	MemFree(m_name);
}

/**
 * Add port identified by single 32bit ID (usually ifIndex)
 */
void VlanInfo::add(uint32_t portId)
{
   for(int i = 0; i < m_numPorts; i++)
      if (m_ports[i].portId == portId)
         return;  // Already added
	if (m_numPorts == m_allocated)
	{
		m_allocated += 64;
		m_ports = MemReallocArray(m_ports, m_allocated);
	}
	memset(&m_ports[m_numPorts], 0, sizeof(VlanPortInfo));
	m_ports[m_numPorts].portId = portId;
	m_numPorts++;
}

/**
 * Add port identified by location
 */
void VlanInfo::add(const InterfacePhysicalLocation& location)
{
   for(int i = 0; i < m_numPorts; i++)
      if (m_ports[i].location.equals(location))
         return;  // Already added
   if (m_numPorts == m_allocated)
   {
      m_allocated += 64;
      m_ports = MemReallocArray(m_ports, m_allocated);
   }
   memset(&m_ports[m_numPorts], 0, sizeof(VlanPortInfo));
   m_ports[m_numPorts].location = location;
   m_numPorts++;
}

/**
 * Set VLAN name
 *
 * @param name new name
 */
void VlanInfo::setName(const TCHAR *name)
{
	MemFree(m_name);
	m_name = MemCopyString(name);
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
	if ((index >= 0) && (index < m_numPorts))
	{
		m_ports[index].location = location;
		m_ports[index].ifIndex = ifIndex;
		m_ports[index].objectId = id;
	}
}
