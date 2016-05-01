/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2016 Victor Kirhenshtein
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
 * VRRP info class constructor
 */
VrrpInfo::VrrpInfo(int version)
{
	m_version = version;
	m_routers = new ObjectArray<VrrpRouter>(16, 16, true);
}

/**
 * VRRP info class destructor
 */
VrrpInfo::~VrrpInfo()
{
	delete m_routers;
}

/**
 * Router constructor
 */
VrrpRouter::VrrpRouter(UINT32 id, UINT32 ifIndex, int state, BYTE *macAddr)
{
	m_id = id;
	m_ifIndex = ifIndex;
	m_state = state;
	memcpy(m_virtualMacAddr, macAddr, MAC_ADDR_LENGTH);
	m_ipAddrCount = 0;
	m_ipAddrList = NULL;
}

/**
 * Router destructor
 */
VrrpRouter::~VrrpRouter()
{
	safe_free(m_ipAddrList);
}

/**
 * Add virtual IP
 */
void VrrpRouter::addVirtualIP(SNMP_Variable *var)
{
	if (var->getValueAsInt() != VRRP_VIP_ACTIVE)
		return;	// Ignore non-active VIPs

	// IP is encoded in last 4 elements of the OID
	const UINT32 *oid = var->getName().value();
	UINT32 vip = (oid[13] << 24) | (oid[14] << 16) | (oid[15] << 8) | oid[16];

	if (m_ipAddrCount % 16 == 0)
		m_ipAddrList = (UINT32 *)realloc(m_ipAddrList, (m_ipAddrCount + 16) * sizeof(UINT32));
	m_ipAddrList[m_ipAddrCount++] = vip; 
}

/**
 * VRRP walker callback
 */
UINT32 VrrpRouter::walkerCallback(SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
	((VrrpRouter *)arg)->addVirtualIP(var);
	return SNMP_ERR_SUCCESS;
}

/**
 * Read VRs virtual IPs
 */
bool VrrpRouter::readVirtualIP(SNMP_Transport *transport)
{
	TCHAR oid[256];
	_sntprintf(oid, 256, _T(".1.3.6.1.2.1.68.1.4.1.2.%u.%u"), m_ifIndex, m_id);
	return SnmpWalk(transport, oid, VrrpRouter::walkerCallback, this) == SNMP_ERR_SUCCESS;
}

/**
 * VRRP virtual router table walker's callback
 */
UINT32 VRRPHandler(SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
	const SNMP_ObjectId& oid = var->getName();

	// Entries indexed by ifIndex and VRID
	UINT32 ifIndex = oid.value()[11];
	UINT32 vrid = oid.value()[12];
	int state = var->getValueAsInt();

	UINT32 oidMac[64];
	memcpy(oidMac, oid.value(), oid.length() * sizeof(UINT32));
	oidMac[10] = 2;	// .1.3.6.1.2.1.68.1.3.1.2.ifIndex.vrid = virtual MAC
	BYTE macAddr[MAC_ADDR_LENGTH];
	if (SnmpGetEx(transport, NULL, oidMac, 13, &macAddr, MAC_ADDR_LENGTH, SG_RAW_RESULT, NULL) == SNMP_ERR_SUCCESS)
	{
		VrrpRouter *router = new VrrpRouter(vrid, ifIndex, state, macAddr);
		if (router->readVirtualIP(transport))
			((VrrpInfo *)arg)->addRouter(router);
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
		return NULL;

	SNMP_Transport *transport = node->createSnmpTransport();
	if (transport == NULL)
		return NULL;

	LONG version;
	if (SnmpGetEx(transport, _T(".1.3.6.1.2.1.68.1.1.0"), NULL, 0, &version, sizeof(LONG), 0, NULL) != SNMP_ERR_SUCCESS)
	{
		delete transport;
		return NULL;
	}

	VrrpInfo *info = new VrrpInfo(version);
	if (SnmpWalk(transport, _T(".1.3.6.1.2.1.68.1.3.1.3"), VRRPHandler, info) != SNMP_ERR_SUCCESS)
	{
		delete info;
		info = NULL;
	}

	delete transport;
	return info;
}
