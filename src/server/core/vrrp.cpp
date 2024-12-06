/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
** File: vrrp.cpp
**
**/

#include "nxcore.h"

/**
 * Router constructor
 */
VrrpRouter::VrrpRouter(uint32_t id, uint32_t ifIndex, int state, const BYTE *macAddr)
{
	m_id = id;
	m_ifIndex = ifIndex;
	m_state = state;
	memcpy(m_virtualMacAddr, macAddr, MAC_ADDR_LENGTH);
	m_ipAddrCount = 0;
	m_ipAddrList = nullptr;
}

/**
 * Router destructor
 */
VrrpRouter::~VrrpRouter()
{
	MemFree(m_ipAddrList);
}

/**
 * Add virtual IP
 */
void VrrpRouter::addVirtualIP(SNMP_Variable *var)
{
	if (var->getValueAsInt() != VRRP_VIP_ACTIVE)
		return;	// Ignore non-active VIPs

	// IP is encoded in last 4 elements of the OID
	const uint32_t *oid = var->getName().value();
	uint32_t vip = (oid[13] << 24) | (oid[14] << 16) | (oid[15] << 8) | oid[16];

	if (m_ipAddrCount % 16 == 0)
		m_ipAddrList = MemRealloc(m_ipAddrList, m_ipAddrCount + 16);
	m_ipAddrList[m_ipAddrCount++] = vip; 
}

/**
 * Read VRs virtual IPs
 */
bool VrrpRouter::readVirtualIP(SNMP_Transport *transport)
{
	uint32_t oid[] = { 1, 3, 6, 1, 2, 1, 68, 1, 4, 1, 2, m_ifIndex, m_id };
	return SnmpWalk(transport, oid, sizeof(oid) / sizeof(uint32_t),
	   [this] (SNMP_Variable *var) -> uint32_t
	   {
	      this->addVirtualIP(var);
	      return SNMP_ERR_SUCCESS;
	   }
	) == SNMP_ERR_SUCCESS;
}

/**
 * VRRP virtual router table walker's callback
 */
uint32_t VRRPHandler(SNMP_Variable *var, SNMP_Transport *transport, VrrpInfo *info)
{
	const SNMP_ObjectId& oid = var->getName();

	// Entries indexed by ifIndex and VRID
	uint32_t ifIndex = oid.value()[11];
	uint32_t vrid = oid.value()[12];
	int state = var->getValueAsInt();

	uint32_t oidMac[64];
	memcpy(oidMac, oid.value(), oid.length() * sizeof(UINT32));
	oidMac[10] = 2;	// .1.3.6.1.2.1.68.1.3.1.2.ifIndex.vrid = virtual MAC
	BYTE macAddr[MAC_ADDR_LENGTH];
	if (SnmpGetEx(transport, nullptr, oidMac, 13, &macAddr, MAC_ADDR_LENGTH, SG_RAW_RESULT, nullptr) == SNMP_ERR_SUCCESS)
	{
		VrrpRouter *router = new VrrpRouter(vrid, ifIndex, state, macAddr);
		if (router->readVirtualIP(transport))
		   info->addRouter(router);
		else
			delete router;
	}

	return SNMP_ERR_SUCCESS;
}

/**
 * Get list of VRRP virtual routers
 */
VrrpInfo *GetVRRPInfo(Node *node)
{
	if (!node->isSNMPSupported())
		return nullptr;

	SNMP_Transport *transport = node->createSnmpTransport();
	if (transport == nullptr)
		return nullptr;

	int32_t version;
	if (SnmpGetEx(transport, { 1, 3, 6, 1, 2, 1, 68, 1, 1, 0 }, &version, sizeof(int32_t), 0, nullptr) != SNMP_ERR_SUCCESS)
	{
		delete transport;
		return nullptr;
	}

	VrrpInfo *info = new VrrpInfo(version);
	if (SnmpWalk(transport, { 1, 3, 6, 1, 2, 1, 68, 1, 3, 1, 3 }, VRRPHandler, info) != SNMP_ERR_SUCCESS)
	{
		delete info;
		info = nullptr;
	}

	delete transport;
	return info;
}
