/* 
** NetXMS - Network Management System
** Driver for Dell PowerConnect switches
** Copyright (C) 2003-2012 Victor Kirhenshtein
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


//
// Static data
//

static TCHAR s_driverName[] = _T("DELL-PWC");
static TCHAR s_driverVersion[] = NETXMS_VERSION_STRING;


/**
 * Get driver name
 */
const TCHAR *PowerConnectDriver::getName()
{
	return s_driverName;
}

/**
 * Get driver version
 */
const TCHAR *PowerConnectDriver::getVersion()
{
	return s_driverVersion;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int PowerConnectDriver::isPotentialDevice(const TCHAR *oid)
{
	return (_tcsncmp(oid, _T(".1.3.6.1.4.1.674.10895"), 22) == 0) ? 127 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool PowerConnectDriver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
{
	return true;
}

/**
 * Do additional checks on the device required by driver.
 * Driver can set device's custom attributes from within
 * this function.
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 */
void PowerConnectDriver::analyzeDevice(SNMP_Transport *snmp, const TCHAR *oid, StringMap *attributes, void **driverData)
{
	attributes->set(_T(".powerConnect.slotSize"), _T("52"));
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 */
InterfaceList *PowerConnectDriver::getInterfaces(SNMP_Transport *snmp, StringMap *attributes, void *driverData, int useAliases, bool useIfXTable)
{
	// Get interface list from standard MIB
	InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, attributes, driverData, useAliases, useIfXTable);
	if (ifList == NULL)
		return NULL;

	UINT32 slotSize = attributes->getULong(_T(".powerConnect.slotSize"), 52);

	// Find physical ports
	for(int i = 0; i < ifList->getSize(); i++)
	{
		NX_INTERFACE_INFO *iface = ifList->get(i);
		if (iface->dwType == IFTYPE_ETHERNET_CSMACD)
		{
			iface->isPhysicalPort = true;
			iface->dwSlotNumber = (iface->dwIndex / slotSize) + 1;
			iface->dwPortNumber = iface->dwIndex % slotSize;
		}
	}

	return ifList;
}

/**
 * Driver entry point
 */
DECLARE_NDD_ENTRY_POINT(s_driverName, PowerConnectDriver);

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
