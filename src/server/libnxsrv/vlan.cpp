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
VlanList::VlanList(int initialAlloc)
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
 * Fill NXCP  message with vlans data
 */
void VlanList::fillMessage(CSCPMessage *msg)
{
	msg->SetVariable(VID_NUM_VLANS, (DWORD)m_size);
	DWORD varId = VID_VLAN_LIST_BASE;
	for(int i = 0; i < m_size; i++)
	{
		msg->SetVariable(varId++, (WORD)m_vlans[i]->getVlanId());
		msg->SetVariable(varId++, m_vlans[i]->getName());
		msg->SetVariable(varId++, (DWORD)m_vlans[i]->getNumPorts());
		msg->SetVariableToInt32Array(varId++, (DWORD)m_vlans[i]->getNumPorts(), m_vlans[i]->getPorts());
		varId += 6;
	}
}

/**
 * VlanInfo constructor
 */
VlanInfo::VlanInfo(int vlanId)
{
	m_vlanId = vlanId;
	m_name = NULL;
	m_allocated = 64;
	m_numPorts = 0;
	m_ports = (DWORD *)malloc(m_allocated * sizeof(DWORD));
}

/**
 * VlanInfo destructor
 */
VlanInfo::~VlanInfo()
{
	safe_free(m_ports);
	safe_free(m_name);
}

/**
 * Add port info
 */
void VlanInfo::add(DWORD slot, DWORD port)
{
	if (m_numPorts == m_allocated)
	{
		m_allocated += 64;
		m_ports = (DWORD *)realloc(m_ports, sizeof(DWORD) * m_allocated);
	}
	m_ports[m_numPorts++] = (slot << 16) | port;
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
