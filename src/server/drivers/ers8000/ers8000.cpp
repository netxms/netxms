/* 
** NetXMS - Network Management System
** Driver for Avaya ERS 8xxx switches (former Nortel/Bay Networks Passport)
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
** File: ers8000.cpp
**/

#include "ers8000.h"


//
// Static data
//

static TCHAR s_driverName[] = _T("ERS8000");
static TCHAR s_driverVersion[] = NETXMS_VERSION_STRING;


/**
 * Get driver name
 */
const TCHAR *PassportDriver::getName()
{
	return s_driverName;
}

/**
 * Get driver version
 */
const TCHAR *PassportDriver::getVersion()
{
	return s_driverVersion;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int PassportDriver::isPotentialDevice(const TCHAR *oid)
{
	return (_tcsncmp(oid, _T(".1.3.6.1.4.1.2272"), 17) == 0) ? 127 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool PassportDriver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
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
void PassportDriver::analyzeDevice(SNMP_Transport *snmp, const TCHAR *oid, StringMap *attributes, void **driverData)
{
	int model = _tcstol(&oid[18], NULL, 10);
	if ((model == 43) || (model == 44) || (model == 45))	// Passport 1600 series
	{
		attributes->set(_T(".rapidCity.is1600"), _T("true"));
		attributes->set(_T(".rapidCity.maxSlot"), _T("1"));
	}
	else if ((model == 31) || (model == 33) || (model == 48) || (model == 50))
	{
		attributes->set(_T(".rapidCity.maxSlot"), _T("6"));
	}
	else if (model == 34)
	{
		attributes->set(_T(".rapidCity.maxSlot"), _T("3"));
	}
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 */
InterfaceList *PassportDriver::getInterfaces(SNMP_Transport *snmp, StringMap *attributes, void *driverData, int useAliases, bool useIfXTable)
{
	// Get interface list from standard MIB
	InterfaceList *ifList = AvayaERSDriver::getInterfaces(snmp, attributes, driverData, useAliases, useIfXTable);
	if (ifList == NULL)
		return NULL;

	DWORD maxSlot = attributes->getULong(_T(".rapidCity.maxSlot"), 10);
	bool is1600Series = attributes->getBoolean(_T(".rapidCity.is1600"), false);
	
	// Calculate slot/port pair from ifIndex
	for(int i = 0; i < ifList->getSize(); i++)
	{
		DWORD slot = ifList->get(i)->dwIndex / 64;

		// Some 1600 may report physical ports with indexes started at 1, not 64
		if ((slot == 0) && is1600Series)
		{
			ifList->get(i)->dwSlotNumber = slot;
			ifList->get(i)->dwPortNumber = ifList->get(i)->dwIndex;
			ifList->get(i)->isPhysicalPort = true;
		}
		else if ((slot > 0) && (slot <= maxSlot))
		{
			ifList->get(i)->dwSlotNumber = slot;
			ifList->get(i)->dwPortNumber = ifList->get(i)->dwIndex % 64 + 1;
			ifList->get(i)->isPhysicalPort = true;
		}
	}

	getVlanInterfaces(snmp, ifList);

	return ifList;
}

/**
 * Driver entry point
 */
DECLARE_NDD_ENTRY_POINT(s_driverName, PassportDriver);

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
