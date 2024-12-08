/* 
** NetXMS - Network Management System
** Driver for Cisco catalyst 2900XL, 2950, and 3500XL switches
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
** File: cat2900xl.cpp
**/

#include "cisco.h"

/**
 * Get driver name
 */
const TCHAR *Cat2900Driver::getName()
{
	return _T("CATALYST-2900XL");
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int Cat2900Driver::isPotentialDevice(const SNMP_ObjectId& oid)
{
	static uint32_t supportedDevices[] =
	   {
         170, 171, 183, 184, 217, 218, 219, 220,
         221, 246, 247, 248, 323, 324, 325, 359,
         369, 370, 427, 428, 429, 430, 472, 480,
         482, 483, 484, 488, 489, 508, 551, 559,
         560, 0
	   };

	if (!oid.startsWith({ 1, 3, 6, 1, 4, 1, 9, 1 }))
		return 0;	// Not a Cisco device

	uint32_t id = oid.getElement(8);
	for(int i = 0; supportedDevices[i] != 0; i++)
		if (supportedDevices[i] == id)
			return 200;
	return 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool Cat2900Driver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
	return true;
}

/**
 * Handler for switch port enumeration
 */
static uint32_t HandlerPortList(SNMP_Variable *var, SNMP_Transport *transport, InterfaceList *ifList)
{
	InterfaceInfo *iface = ifList->findByIfIndex(var->getValueAsUInt());
	if (iface != nullptr)
	{
		size_t nameLen = var->getName().length();
		iface->location.module = var->getName().getElement(nameLen - 2);
		iface->location.port = var->getName().getElement(nameLen - 1);
		iface->isPhysicalPort = true;
	}
	return SNMP_ERR_SUCCESS;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param useIfXTable if true, usage of ifXTable is allowed
 */
InterfaceList *Cat2900Driver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
	// Get interface list from standard MIB
	InterfaceList *ifList = CiscoDeviceDriver::getInterfaces(snmp, node, driverData, useIfXTable);
	if (ifList == NULL)
		return NULL;
	
	// Set slot and port number for physical interfaces
	SnmpWalk(snmp, _T(".1.3.6.1.4.1.9.9.87.1.4.1.1.25"), HandlerPortList, ifList);

	return ifList;
}
