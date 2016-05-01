/* 
** NetXMS - Network Management System
** Driver for Cisco catalyst 2900XL, 2950, and 3500XL switches
** Copyright (C) 2003-2016 Victor Kirhenshtein
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

#include "cat2900xl.h"


//
// Static data
//

static TCHAR s_driverName[] = _T("CATALYST-2900XL");
static TCHAR s_driverVersion[] = NETXMS_VERSION_STRING;


/**
 * Get driver name
 */
const TCHAR *Cat2900Driver::getName()
{
	return s_driverName;
}

/**
 * Get driver version
 */
const TCHAR *Cat2900Driver::getVersion()
{
	return s_driverVersion;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int Cat2900Driver::isPotentialDevice(const TCHAR *oid)
{
	static int supportedDevices[] = { 170, 171, 183, 184, 217, 218, 219, 220, 
	                                  221, 246, 247, 248, 323, 324, 325, 359,
												 369, 370, 427, 428, 429, 430, 472, 480, 
												 482, 483, 484, 488, 489, 508, 551, 559, 
												 560, -1 };

	if (_tcsncmp(oid, _T(".1.3.6.1.4.1.9.1."), 17))
		return 0;	// Not a Cisco device

	int id = _tcstol(&oid[17], NULL, 10);
	for(int i = 0; supportedDevices[i] != -1; i++)
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
bool Cat2900Driver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
{
	return true;
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
		iface->slot = var->getName().getElement(nameLen - 2);
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
InterfaceList *Cat2900Driver::getInterfaces(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, int useAliases, bool useIfXTable)
{
	// Get interface list from standard MIB
	InterfaceList *ifList = CiscoDeviceDriver::getInterfaces(snmp, attributes, driverData, useAliases, useIfXTable);
	if (ifList == NULL)
		return NULL;
	
	// Set slot and port number for physical interfaces
	SnmpWalk(snmp, _T(".1.3.6.1.4.1.9.9.87.1.4.1.1.25"), HandlerPortList, ifList);

	return ifList;
}

/**
 * Driver entry point
 */
DECLARE_NDD_ENTRY_POINT(s_driverName, Cat2900Driver);


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
