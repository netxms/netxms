/* 
** NetXMS - Network Management System
** Driver for Cisco Catalyst switches
** Copyright (C) 2003-2018 Victor Kirhenshtein
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
** File: catalyst.cpp
**/

#include "cisco.h"

/**
 * Get driver name
 */
const TCHAR *CatalystDriver::getName()
{
	return _T("CATALYST-GENERIC");
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int CatalystDriver::isPotentialDevice(const TCHAR *oid)
{
	return (_tcsncmp(oid, _T(".1.3.6.1.4.1.9.1."), 17) == 0) ? 250 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool CatalystDriver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
{
	UINT32 value = 0;
	return SnmpGetEx(snmp, _T(".1.3.6.1.4.1.9.5.1.2.14.0"), NULL, 0, &value, sizeof(UINT32), 0, NULL) == SNMP_ERR_SUCCESS;
}

/**
 * Handler for switch port enumeration
 */
static UINT32 HandlerPortList(SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
	InterfaceList *ifList = (InterfaceList *)arg;

	InterfaceInfo *iface = ifList->findByIfIndex(var->getValueAsUInt());
	if (iface != NULL)
	{
		size_t nameLen = var->getName().length();
		
		UINT32 moduleIndex = var->getName().getElement(nameLen - 2);
		UINT32 oid[] = { 1, 3, 6, 1, 4, 1, 9, 5, 1, 3, 1, 1, 25, 0 };
		oid[13] = moduleIndex;
		UINT32 slot;
		if (SnmpGetEx(transport, NULL, oid, 14, &slot, sizeof(UINT32), 0, NULL) != SNMP_ERR_SUCCESS)
			slot = moduleIndex;	// Assume slot # equal to module index if it cannot be read

		iface->slot = slot;
		iface->port = var->getName().getElement(nameLen - 1);
		iface->isPhysicalPort = true;
	}
	return SNMP_ERR_SUCCESS;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 */
InterfaceList *CatalystDriver::getInterfaces(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, int useAliases, bool useIfXTable)
{
	// Get interface list from standard MIB
	InterfaceList *ifList = CiscoDeviceDriver::getInterfaces(snmp, attributes, driverData, useAliases, useIfXTable);
	if (ifList == NULL)
		return NULL;
	
	// Set slot and port number for physical interfaces
	SnmpWalk(snmp, _T(".1.3.6.1.4.1.9.5.1.4.1.1.11"), HandlerPortList, ifList);

	return ifList;
}
