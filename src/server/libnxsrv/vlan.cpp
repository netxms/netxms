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
** File: vlan.cpp
**/

#include "libnxsrv.h"


/**
 * VlanList constructor
 */
VlanList::VlanList(int initialAlloc) : RefCountObject()
{
	m_allocated = initialAlloc;
	m_size = 0;
	m_data = NULL;
	m_vlans = (VlanInfo **)malloc(sizeof(VlanInfo *) * m_allocated);
}

/**
 * Destructor
 */
VlanList::~VlanList()
{
	for(int i = 0; i < m_size; i++)
		delete m_vlans[i];
	safe_free(m_vlans);
}

/**
 * Add new VLAN (list becomes owner of VLAN object)
 */
void VlanList::add(VlanInfo *vlan)
{
	if (m_size == m_allocated)
	{
		m_allocated += 32;
		m_vlans = (VlanInfo **)realloc(m_vlans, sizeof(VlanInfo *) * m_allocated);
	}
	m_vlans[m_size++] = vlan;
}

/**
 * Add member port to VLAN
 *
 * @param vlanId VLAN ID
 * @param portId port's 32bit identifier (usually ifIndex or slot/port pair)
 */
void VlanList::addMemberPort(int vlanId, UINT32 portId)
{
	VlanInfo *vlan = findById(vlanId);
	if (vlan != NULL)
	{
		vlan->add(portId);
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
	return NULL;
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
	return NULL;
}

/**
 * Fill NXCP  message with vlans data
 */
void VlanList::fillMessage(CSCPMessage *msg)
{
	msg->SetVariable(VID_NUM_VLANS, (UINT32)m_size);
	UINT32 varId = VID_VLAN_LIST_BASE;
	for(int i = 0; i < m_size; i++)
	{
		msg->SetVariable(varId++, (UINT16)m_vlans[i]->getVlanId());
		msg->SetVariable(varId++, m_vlans[i]->getName());
		msg->SetVariable(varId++, (UINT32)m_vlans[i]->getNumPorts());
		msg->SetVariableToInt32Array(varId++, (UINT32)m_vlans[i]->getNumPorts(), m_vlans[i]->getPorts());
		msg->SetVariableToInt32Array(varId++, (UINT32)m_vlans[i]->getNumPorts(), m_vlans[i]->getIfIndexes());
		msg->SetVariableToInt32Array(varId++, (UINT32)m_vlans[i]->getNumPorts(), m_vlans[i]->getIfIds());
		varId += 4;
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
	m_ports = (UINT32 *)malloc(m_allocated * sizeof(UINT32));
	m_indexes = NULL;
	m_ids = NULL;
}

/**
 * VlanInfo destructor
 */
VlanInfo::~VlanInfo()
{
	safe_free(m_ports);
	safe_free(m_name);
	safe_free(m_indexes);
	safe_free(m_ids);
}

/**
 * Add port identified by single 32bit ID (usually ifIndex)
 */
void VlanInfo::add(UINT32 ifIndex)
{
	if (m_numPorts == m_allocated)
	{
		m_allocated += 64;
		m_ports = (UINT32 *)realloc(m_ports, sizeof(UINT32) * m_allocated);
	}
	m_ports[m_numPorts++] = ifIndex;
}

/**
 * Add port identified by slot/port pair
 */
void VlanInfo::add(UINT32 slot, UINT32 port)
{
	add((slot << 16) | port);
}

/**
 * Set VLAN name
 *
 * @param name new name
 */
void VlanInfo::setName(const TCHAR *name)
{
	safe_free(m_name);
	m_name = _tcsdup(name);
}

/**
 * Prepare for port resolving. Add operation is not permitted after this call.
 */
void VlanInfo::prepareForResolve()
{
	if (m_indexes == NULL)
	{
		m_indexes = (UINT32 *)malloc(sizeof(UINT32) * m_numPorts);
		memset(m_indexes, 0, sizeof(UINT32) * m_numPorts);
	}
	if (m_ids == NULL)
	{
		m_ids = (UINT32 *)malloc(sizeof(UINT32) * m_numPorts);
		memset(m_ids, 0, sizeof(UINT32) * m_numPorts);
	}
}

/**
 * Set full port's information: slot/port, ifIndex, object ID.
 * Must be called after call to prepareForResolve.
 *
 * @param index port's index
 * @param sp slot/port pair
 * @param ifIndex interface index
 * @param id interface object ID
 */
void VlanInfo::resolvePort(int index, UINT32 sp, UINT32 ifIndex, UINT32 id)
{
	if ((index >= 0) && (index < m_numPorts))
	{
		m_ports[index] = sp;
		m_indexes[index] = ifIndex;
		m_ids[index] = id;
	}
}
