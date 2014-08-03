/* 
** NetXMS - Network Management System
** Driver for Cisco ESW switches
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
** File: cisco-esw.cpp
**/

#include "cisco-esw.h"

/**
 * Driver name
 */
static TCHAR s_driverName[] = _T("CISCO-ESW");

/**
 * Get driver name
 */
const TCHAR *CiscoEswDriver::getName()
{
	return s_driverName;
}

/**
 * Get driver version
 */
const TCHAR *CiscoEswDriver::getVersion()
{
	return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int CiscoEswDriver::isPotentialDevice(const TCHAR *oid)
{
	if (_tcsncmp(oid, _T(".1.3.6.1.4.1.9.1."), 17) != 0)
		return 0;

	int model = _tcstol(&oid[17], NULL, 10);
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
bool CiscoEswDriver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
{
	return true;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 */
InterfaceList *CiscoEswDriver::getInterfaces(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, int useAliases, bool useIfXTable)
{
	// Get interface list from standard MIB
	InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, attributes, driverData, useAliases, useIfXTable);
	if (ifList == NULL)
		return NULL;

	// Find physical ports
	for(int i = 0; i < ifList->size(); i++)
	{
		NX_INTERFACE_INFO *iface = ifList->get(i);
		if ((iface->dwType == IFTYPE_ETHERNET_CSMACD) && (iface->dwIndex <= 48))
		{
			iface->isPhysicalPort = true;
			iface->dwSlotNumber = 1;
			iface->dwPortNumber = iface->dwIndex;
		}
	}

	return ifList;
}

/**
 * Driver entry point
 */
DECLARE_NDD_ENTRY_POINT(s_driverName, CiscoEswDriver);

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
