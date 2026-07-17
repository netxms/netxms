/*
** NetXMS - Network Management System
** Driver for Zyxel ES (Enterprise Switch) product line
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
** File: es.cpp
**/

#include "zyxel.h"
#include <netxms-version.h>

/**
 * Get driver name
 */
const TCHAR *ZyxelESSwitchDriver::getName()
{
   return _T("ZYXEL-ES");
}

/**
 * Get driver version
 */
const TCHAR *ZyxelESSwitchDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int ZyxelESSwitchDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return oid.startsWith({ 1, 3, 6, 1, 4, 1, 890, 1, 15 }) ? 127 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param context device context
 * @param oid Device OID
 */
bool ZyxelESSwitchDriver::isDeviceSupported(DeviceContext *context, const SNMP_ObjectId& oid)
{
   return true;
}

/**
 * Get port layout of given module. Default implementation always set NDD_PN_UNKNOWN as layout.
 *
 * @param context device context
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @param module Module number (starting from 1)
 * @param layout Layout structure to fill
 */
void ZyxelESSwitchDriver::getModuleLayout(DeviceContext *context, NObject *node, DriverData *driverData, int module, NDD_MODULE_LAYOUT *layout)
{
   layout->numberingScheme = NDD_PN_DU_LR;
   layout->rows = 2;
}

/**
 * Get list of interfaces for given node
 *
 * @param context device context
 * @param node Node
 * @param driverData driver data
 * @param useIfXTable if true, usage of ifXTable is allowed
 */
InterfaceList *ZyxelESSwitchDriver::getInterfaces(DeviceContext *context, NObject *node, DriverData *driverData, bool useIfXTable)
{
   InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(context, node, driverData, useIfXTable);
   if (ifList == nullptr)
      return nullptr;

   for (int i = 0; i < ifList->size(); i++)
   {
      InterfaceInfo *iface = ifList->get(i);
      if (iface->type == IFTYPE_ETHERNET_CSMACD)
      {
         iface->isPhysicalPort = true;
         iface->location.module = 1;
         iface->location.port = iface->index;
      }
   }

   return ifList;
}
