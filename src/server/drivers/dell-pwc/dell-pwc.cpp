/* 
** NetXMS - Network Management System
** Driver for Dell PowerConnect switches
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
** File: dell-pwc.cpp
**/

#include "dell-pwc.h"
#include <netxms-version.h>

/**
 * Get driver name
 */
const TCHAR *PowerConnectDriver::getName()
{
   return _T("DELL-PWC");
}

/**
 * Get driver version
 */
const TCHAR *PowerConnectDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int PowerConnectDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return oid.startsWith({ 1, 3, 6, 1, 4, 1, 674, 10895 }) ? 127 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool PowerConnectDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
   return true;
}

/**
 * Do additional checks on the device required by driver.
 * Driver can set device's custom attributes from within
 * this function.
 *
 * @param snmp SNMP transport
 * @param node Node
 */
void PowerConnectDriver::analyzeDevice(SNMP_Transport *snmp, const SNMP_ObjectId& oid, NObject *node, DriverData **driverData)
{
   node->setCustomAttribute(_T(".powerConnect.slotSize"), _T("52"), StateChange::IGNORE);
}

/**
 * Get hardware information from device.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param hwInfo pointer to hardware information structure to fill
 * @return true if hardware information is available
 */
bool PowerConnectDriver::getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   // Only set vendor here, rest will be retrieved from ENTITY MIB
   _tcscpy(hwInfo->vendor, _T("Dell"));
   return true;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 */
InterfaceList *PowerConnectDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
	// Get interface list from standard MIB
	InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, useIfXTable);
	if (ifList == nullptr)
		return nullptr;

	uint32_t slotSize = node->getCustomAttributeAsUInt32(_T(".powerConnect.slotSize"), 52);

	// Find physical ports
	for(int i = 0; i < ifList->size(); i++)
	{
		InterfaceInfo *iface = ifList->get(i);
		if (iface->type == IFTYPE_ETHERNET_CSMACD)
		{
			iface->isPhysicalPort = true;
			iface->location.module = (iface->index / slotSize) + 1;
			iface->location.port = iface->index % slotSize;
		}
	}

	return ifList;
}

/**
 * Driver entry point
 */
DECLARE_NDD_ENTRY_POINT(PowerConnectDriver);

/**
 * DLL entry point
 */
#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
