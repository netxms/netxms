/* 
** NetXMS - Network Management System
** Driver for HP ProCurve switches
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
** File: procurve.cpp
**/

#include "procurve.h"


//
// Static data
//

static TCHAR s_driverName[] = _T("PROCURVE");
static TCHAR s_driverVersion[] = NETXMS_VERSION_STRING;


/**
 * Get driver name
 */
const TCHAR *ProCurveDriver::getName()
{
	return s_driverName;
}

/**
 * Get driver version
 */
const TCHAR *ProCurveDriver::getVersion()
{
	return s_driverVersion;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int ProCurveDriver::isPotentialDevice(const TCHAR *oid)
{
	return (_tcsncmp(oid, _T(".1.3.6.1.4.1.11.2.3.7.11"), 24) == 0) ? 127 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool ProCurveDriver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
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
void ProCurveDriver::analyzeDevice(SNMP_Transport *snmp, const TCHAR *oid, StringMap *attributes, DriverData **driverData)
{
	int model = _tcstol(&oid[25], NULL, 10);
	
	// modular switches
	if ((model == 7) || (model == 9) || (model == 13) || (model == 14) || (model == 23) || (model == 27))
	{
		attributes->set(_T(".procurve.isModular"), _T("1"));
		attributes->set(_T(".procurve.slotSize"), _T("24"));
	}
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 */
InterfaceList *ProCurveDriver::getInterfaces(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, int useAliases, bool useIfXTable)
{
	// Get interface list from standard MIB
	InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, attributes, driverData, useAliases, useIfXTable);
	if (ifList == NULL)
		return NULL;

	bool isModular = attributes->getBoolean(_T(".procurve.isModular"), false);
	UINT32 slotSize = attributes->getULong(_T(".procurve.slotSize"), 24);

	// Find physical ports
	for(int i = 0; i < ifList->size(); i++)
	{
		NX_INTERFACE_INFO *iface = ifList->get(i);
		if (iface->dwType == IFTYPE_ETHERNET_CSMACD)
		{
			iface->isPhysicalPort = true;
			if (isModular)
			{
				iface->dwSlotNumber = (iface->dwIndex / slotSize) + 1;
				iface->dwPortNumber = iface->dwIndex % slotSize;
			}
			else
			{
				iface->dwSlotNumber = 1;
				iface->dwPortNumber = iface->dwIndex;
			}
		}
	}

	return ifList;
}

/**
 * Driver entry point
 */
DECLARE_NDD_ENTRY_POINT(s_driverName, ProCurveDriver);

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
