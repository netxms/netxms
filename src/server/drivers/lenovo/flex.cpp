/*
** NetXMS - Network Management System
** Driver for Lenovo Flex System Fabric switches
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
** File: flex.cpp
**
**/

#include "lenovo.h"

#define DEBUG_TAG_LENOVO_FLEX _T("ndd.lenovo.flex")

/**
 * Flex Switch port layout:
 *   ifIndex   1..170 - 14 internal ports (INT1-INT14) plus reserved range
 *   ifIndex 171..180 - 10 external ports (EXT1-EXT10)
 *   ifIndex    > 180 - management ports (MGT1, EXTM)
 */
#define FLEX_INTERNAL_PORT_LIMIT  170
#define FLEX_EXTERNAL_PORT_LIMIT  180

/**
 * Get driver name
 */
const TCHAR *LenovoFlexDriver::getName()
{
   return _T("LENOVO-FLEX");
}

/**
 * Get driver version
 */
const TCHAR *LenovoFlexDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int LenovoFlexDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return oid.startsWith({ 1, 3, 6, 1, 4, 1, 20301, 1, 18 }) ? 127 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool LenovoFlexDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
   return true;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data
 * @param useIfXTable if true, usage of ifXTable is allowed
 */
InterfaceList *LenovoFlexDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
   InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, true);
   if (ifList == nullptr)
      return nullptr;

   uint32_t internalPort = 1;
   uint32_t externalPort = 1;
   uint32_t managementPort = 1;
   for (int i = 0; i < ifList->size(); i++)
   {
      InterfaceInfo *iface = ifList->get(i);
      if (iface->type != IFTYPE_ETHERNET_CSMACD)
         continue;

      iface->isPhysicalPort = true;
      iface->location.chassis = 1;
      if (iface->index > FLEX_EXTERNAL_PORT_LIMIT)
      {
         iface->location.module = 2;
         iface->location.port = managementPort++;
      }
      else if (iface->index > FLEX_INTERNAL_PORT_LIMIT)
      {
         iface->location.module = 1;
         iface->location.port = externalPort++;
      }
      else
      {
         iface->location.module = 0;
         iface->location.port = internalPort++;
      }
      nxlog_debug_tag(DEBUG_TAG_LENOVO_FLEX, 6, _T("LenovoFlexDriver::getInterfaces(%s [%u]): ifIndex=%u module=%d port=%d"),
            node->getName(), node->getId(), iface->index, iface->location.module, iface->location.port);
   }

   return ifList;
}

/**
 * Get orientation of the modules in the device
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return module orientation
 */
int LenovoFlexDriver::getModulesOrientation(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   return NDD_ORIENTATION_HORIZONTAL;
}

/**
 * Get port layout of given module
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @param module Module number (starting from 1)
 * @param layout Layout structure to fill
 */
void LenovoFlexDriver::getModuleLayout(SNMP_Transport *snmp, NObject *node, DriverData *driverData, int module, NDD_MODULE_LAYOUT *layout)
{
   layout->numberingScheme = NDD_PN_LR_UD;
   layout->rows = 1;
}
