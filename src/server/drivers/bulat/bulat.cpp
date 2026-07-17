/*
** NetXMS - Network Management System
** Driver for Bulat switches
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: bulat.cpp
**
**/

#include "bulat.h"

/**
 * Get driver name
 */
const TCHAR *BulatDriver::getName()
{
   return _T("BULAT");
}

/**
 * Get driver version
 */
const TCHAR *BulatDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver.
 *
 * Known sysObjectIDs:
 *   BK-D437   1.3.6.1.4.1.47191.1.1.1.281
 *   BK-A837   1.3.6.1.4.1.47191.7.37
 *   BS-4520   1.3.6.1.4.1.47191.1.1.1.640
 *
 * @param oid Device OID
 */
int BulatDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return (oid.startsWith({ 1, 3, 6, 1, 4, 1, 47191, 1, 1, 1 }) ||
           oid.startsWith({ 1, 3, 6, 1, 4, 1, 47191, 7 })) ? 127 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param context device context
 * @param oid Device OID
 */
bool BulatDriver::isDeviceSupported(DeviceContext *context, const SNMP_ObjectId& oid)
{
   return true;
}

/**
 * Get list of interfaces for given node. Bulat devices number physical Ethernet
 * ports directly via ifIndex, so we map each Ethernet (ifType=6) interface to
 * a single chassis/module.
 *
 * @param context device context
 * @param node Node
 * @param driverData driver-specific data
 * @param useIfXTable if true, usage of ifXTable is allowed
 */
InterfaceList *BulatDriver::getInterfaces(DeviceContext *context, NObject *node, DriverData *driverData, bool useIfXTable)
{
   InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(context, node, driverData, true);
   if (ifList == nullptr)
      return nullptr;

   for(int i = 0; i < ifList->size(); i++)
   {
      InterfaceInfo *iface = ifList->get(i);
      if (iface->type == IFTYPE_ETHERNET_CSMACD)
      {
         iface->isPhysicalPort = true;
         iface->location.chassis = 1;
         iface->location.module = 0;
         iface->location.port = iface->index;
      }
   }

   return ifList;
}

/**
 * Driver entry point
 */
DECLARE_NDD_ENTRY_POINT(BulatDriver);

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
