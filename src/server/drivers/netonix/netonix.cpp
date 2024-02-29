/* 
** NetXMS - Network Management System
** Driver for Netonix switches
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
** File: netonix.cpp
**/

#include "netonix.h"
#include <netxms-version.h>

/**
 * Get driver name
 */
const TCHAR *NetonixDriver::getName()
{
	return _T("NETONIX");
}

/**
 * Get driver version
 */
const TCHAR *NetonixDriver::getVersion()
{
	return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int NetonixDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return oid.equals({ 1, 3, 6, 1, 4, 1, 46242 }) ? 127 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool NetonixDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
   return true;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 */
InterfaceList *NetonixDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
   // Get interface list from standard MIB
   InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, useIfXTable);
   if (ifList == nullptr)
      return nullptr;

	// Mark all ports as physical
   // Device do not return logical ports
	for(int i = 0; i < ifList->size(); i++)
	{
		InterfaceInfo *iface = ifList->get(i);
      iface->isPhysicalPort = true;
      iface->location.module = 1;
      iface->location.port = iface->index;
	}

   return ifList;
}

/**
 * Driver entry point
 */
DECLARE_NDD_ENTRY_POINT(NetonixDriver);

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
