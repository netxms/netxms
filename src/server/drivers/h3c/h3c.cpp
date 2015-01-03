/* 
** NetXMS - Network Management System
** Driver for H3C (now HP A-series) switches
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
** File: h3c.cpp
**/

#include "h3c.h"

/**
 * Driver name
 */
static TCHAR s_driverName[] = _T("H3C");

/**
 * Driver version
 */
static TCHAR s_driverVersion[] = NETXMS_VERSION_STRING;

/**
 * Get driver name
 */
const TCHAR *H3CDriver::getName()
{
	return s_driverName;
}

/**
 * Get driver version
 */
const TCHAR *H3CDriver::getVersion()
{
	return s_driverVersion;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int H3CDriver::isPotentialDevice(const TCHAR *oid)
{
	return (_tcsncmp(oid, _T(".1.3.6.1.4.1.43.1.16.4.3."), 25) == 0) ? 255 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool H3CDriver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
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
void H3CDriver::analyzeDevice(SNMP_Transport *snmp, const TCHAR *oid, StringMap *attributes, DriverData **driverData)
{
}

/**
 * Handler for port walk
 */
static UINT32 PortWalkHandler(UINT32 snmpVersion, SNMP_Variable *var, SNMP_Transport *snmp, void *arg)
{
   InterfaceList *ifList = (InterfaceList *)arg;
   UINT32 ifIndex = var->getValueAsUInt();
   for(int i = 0; i < ifList->size(); i++)
   {
      NX_INTERFACE_INFO *iface = ifList->get(i);
      if (iface->index == ifIndex)
      {
         iface->isPhysicalPort = true;
         iface->slot = var->getName()->getValue()[18];
         iface->port = var->getName()->getValue()[20];
         break;
      }
   }
   return SNMP_ERR_SUCCESS;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 */
InterfaceList *H3CDriver::getInterfaces(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, int useAliases, bool useIfXTable)
{
	// Get interface list from standard MIB
	InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, attributes, driverData, useAliases, useIfXTable);
	if (ifList == NULL)
		return NULL;

	// Find physical ports
   SnmpWalk(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.43.45.1.2.23.1.18.4.5.1.3"), PortWalkHandler, ifList, FALSE);
	return ifList;
}

/**
 * Driver entry point
 */
DECLARE_NDD_ENTRY_POINT(s_driverName, H3CDriver);

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
