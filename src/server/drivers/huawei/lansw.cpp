/* 
** NetXMS - Network Management System
** Driver for Huawei switch devices
** Copyright (C) 2003-2024 Raden Solutions
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
** File: lansw.cpp
**/

#include "huawei.h"

#define DEBUG_TAG _T("ndd.huawei.lansw")

/**
 * Get driver name
 */
const TCHAR *HuaweiSWDriver::getName()
{
   return _T("HUAWEI-SW");
}

/**
 * Get driver version
 */
const TCHAR *HuaweiSWDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int HuaweiSWDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
	return oid.startsWith({ 1, 3, 6, 1, 4, 1, 2011, 2, 23 }) ? 255 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool HuaweiSWDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
	return true;
}

/**
 * Get hardware information from device.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param hwInfo pointer to hardware information structure to fill
 * @return true if hardware information is available
 */
bool HuaweiSWDriver::getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   // Only set vendor here, rest will be retrieved from ENTITY MIB
   _tcscpy(hwInfo->vendor, _T("Huawei"));
   return true;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param useIfXTable if true, usage of ifXTable is allowed
 */
InterfaceList *HuaweiSWDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
   InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, true);
   if (ifList == nullptr)
      return nullptr;
   
   // Physical interfaces can be mixed with virtual ones, so we just count ports, assuming single chassis and consequential port numbering.
   uint32_t port = 1;
   for (int i = 0; i < ifList->size(); i++)
   {
      InterfaceInfo *iface = ifList->get(i);
      if (iface->type == 6)
      {
         iface->location.chassis = 1;
         iface->isPhysicalPort = true;
         iface->location.module = 0;
         iface->location.port = port++;
      }
   }

   return ifList;
}
