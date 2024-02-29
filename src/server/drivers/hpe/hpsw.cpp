/* 
** NetXMS - Network Management System
** Driver for HP switches with HH3C MIB support
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
** File: hpsw.cpp
**/

#include "hpe.h"

/**
 * Get driver name
 */
const TCHAR *HPSwitchDriver::getName()
{
	return _T("HPSW");
}

/**
 * Get driver version
 */
const TCHAR *HPSwitchDriver::getVersion()
{
	return NETXMS_BUILD_TAG;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int HPSwitchDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
	return oid.startsWith({ 1, 3, 6, 1, 4, 1, 25506, 11, 1 }) ? 255 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool HPSwitchDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
	return true;
}

/**
 * Handler for port walk
 */
static uint32_t PortWalkHandler(SNMP_Variable *var, SNMP_Transport *snmp, InterfaceList *ifList)
{
   uint32_t ifIndex = var->getValueAsUInt();
   for(int i = 0; i < ifList->size(); i++)
   {
      InterfaceInfo *iface = ifList->get(i);
      if (iface->index == ifIndex)
      {
         iface->isPhysicalPort = true;
         iface->location.chassis = var->getName().getElement(14);
         iface->location.module = var->getName().getElement(15);
         iface->location.pic = var->getName().getElement(16);
         iface->location.port = var->getName().getElement(17);
         break;
      }
   }
   return SNMP_ERR_SUCCESS;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 */
InterfaceList *HPSwitchDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
	// Get interface list from standard MIB
	InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, useIfXTable);
	if (ifList == nullptr)
		return nullptr;

	// Find physical ports (walk hh3cLswPortIfindex)
   SnmpWalk(snmp, _T(".1.3.6.1.4.1.25506.8.35.18.4.5.1.3"), PortWalkHandler, ifList);
	return ifList;
}
