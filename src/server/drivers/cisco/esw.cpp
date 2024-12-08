/* 
** NetXMS - Network Management System
** Driver for Cisco ESW switches
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
** File: esw.cpp
**/

#include "cisco.h"

/**
 * Get driver name
 */
const TCHAR *CiscoEswDriver::getName()
{
	return _T("CISCO-ESW");
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int CiscoEswDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
	if (!oid.startsWith({ 1, 3, 6, 1, 4, 1, 9, 1 }))
		return 0;

	uint32_t model = oid.getElement(8);
	if (((model >= 1058) && (model <= 1064)) || (model == 1176) || (model == 1177))
		return 251;
	return 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool CiscoEswDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
	return true;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param useIfXTable if true, usage of ifXTable is allowed
 */
InterfaceList *CiscoEswDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
	// Get interface list from standard MIB
	InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, useIfXTable);
	if (ifList == nullptr)
		return nullptr;

	// Find physical ports
	for(int i = 0; i < ifList->size(); i++)
	{
		InterfaceInfo *iface = ifList->get(i);
		if ((iface->type == IFTYPE_ETHERNET_CSMACD) && (iface->index <= 48))
		{
			iface->isPhysicalPort = true;
			iface->location.module = 1;
			iface->location.port = iface->index;
		}
	}

	return ifList;
}
