/* 
** NetXMS - Network Management System
** Driver for Cisco Small Business switches
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
** File: cisco-sb.cpp
**/

#include "cisco-sb.h"

/**
 * Driver name
 */
static TCHAR s_driverName[] = _T("CISCO-SB");

/**
 * Get driver name
 */
const TCHAR *CiscoSbDriver::getName()
{
	return s_driverName;
}

/**
 * Get driver version
 */
const TCHAR *CiscoSbDriver::getVersion()
{
	return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int CiscoSbDriver::isPotentialDevice(const TCHAR *oid)
{
	if (_tcsncmp(oid, _T(".1.3.6.1.4.1.9.6.1."), 19) != 0)
		return 0;

	if (!_tcsncmp(&oid[19], _T("80."), 3) || !_tcsncmp(&oid[19], _T("81."), 3) ||
       !_tcsncmp(&oid[19], _T("82."), 3) || !_tcsncmp(&oid[19], _T("83."), 3) ||
       !_tcsncmp(&oid[19], _T("85."), 3))
		return 252;
	return 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool CiscoSbDriver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
{
	return true;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 */
InterfaceList *CiscoSbDriver::getInterfaces(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, int useAliases, bool useIfXTable)
{
	// Get interface list from standard MIB
	InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, attributes, driverData, useAliases, useIfXTable);
	if (ifList == NULL)
		return NULL;

   // Check if there are indexes below 49
   bool highBase = true;
	for(int i = 0; i < ifList->getSize(); i++)
   {
      if (ifList->get(i)->dwIndex < 49)
      {
         highBase = false;
         break;
      }
   }

	// Find physical ports
	for(int i = 0; i < ifList->getSize(); i++)
	{
		NX_INTERFACE_INFO *iface = ifList->get(i);
		if (iface->dwIndex < 1000)
		{
			iface->isPhysicalPort = true;
			iface->dwSlotNumber = (iface->dwIndex - 1) / 110 + 1;
			iface->dwPortNumber = (iface->dwIndex - 1) % 110 + 1;
         if (highBase)
            iface->dwPortNumber -= 48;
		}
	}

	return ifList;
}

/**
 * Driver entry point
 */
DECLARE_NDD_ENTRY_POINT(s_driverName, CiscoSbDriver);

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
