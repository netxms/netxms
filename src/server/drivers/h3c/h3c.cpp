/* 
** NetXMS - Network Management System
** Driver for H3C (now HP A-series) switches
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
static UINT32 PortWalkHandler(SNMP_Variable *var, SNMP_Transport *snmp, void *arg)
{
   InterfaceList *ifList = (InterfaceList *)arg;
   InterfaceInfo *iface = ifList->findByIfIndex(var->getValueAsUInt());
   if (iface != NULL)
   {
      iface->isPhysicalPort = true;
      iface->slot = var->getName().getElement(18);
      iface->port = var->getName().getElement(20);
   }
   return SNMP_ERR_SUCCESS;
}

/**
 * Handler for IPv6 address walk
 */
static UINT32 IPv6WalkHandler(SNMP_Variable *var, SNMP_Transport *snmp, void *arg)
{
   InterfaceList *ifList = (InterfaceList *)arg;
   // Address type should be IPv6 and address length 16 bytes
   if ((var->getName().getElement(18) == 2) &&
       (var->getName().length() == 36) &&
       (var->getName().getElement(19) == 16))
   {
      InterfaceInfo *iface = ifList->findByIfIndex(var->getName().getElement(17));
      if (iface != NULL)
      {
         BYTE addrBytes[16];
         for(int i = 20; i < 36; i++)
            addrBytes[i - 20] = (BYTE)var->getName().getElement(i);
         InetAddress addr(addrBytes, var->getValueAsInt());
         iface->ipAddrList.add(addr);
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
   SnmpWalk(snmp, _T(".1.3.6.1.4.1.43.45.1.2.23.1.18.4.5.1.3"), PortWalkHandler, ifList);

   // Read IPv6 addresses
   SnmpWalk(snmp, _T(".1.3.6.1.4.1.43.45.1.10.2.71.1.1.2.1.4"), IPv6WalkHandler, ifList);

	return ifList;
}

/**
 * Get orientation of the modules in the device
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return module orientation
 */
int H3CDriver::getModulesOrientation(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData)
{
   return NDD_ORIENTATION_HORIZONTAL;
}

/**
 * Get port layout of given module
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 * @param driverData driver-specific data previously created in analyzeDevice
 * @param module Module number (starting from 1)
 * @param layout Layout structure to fill
 */
void H3CDriver::getModuleLayout(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, int module, NDD_MODULE_LAYOUT *layout)
{
   layout->numberingScheme = NDD_PN_DU_LR;
   layout->rows = 2;
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
