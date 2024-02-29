/* 
** NetXMS - Network Management System
** Driver for HP ProCurve switches
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
** File: procurve.cpp
**/

#include "hpe.h"

/**
 * Get driver name
 */
const TCHAR *ProCurveDriver::getName()
{
	return _T("PROCURVE");
}

/**
 * Get driver version
 */
const TCHAR *ProCurveDriver::getVersion()
{
	return NETXMS_BUILD_TAG;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int ProCurveDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
	return oid.startsWith({ 1, 3, 6, 1, 4, 1, 11, 2, 3, 7, 11 }) ? 127 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool ProCurveDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
	return true;
}

/**
 * Do additional checks on the device required by driver.
 * Driver can set device's custom attributes from within
 * this function.
 *
 * @param snmp SNMP transport
 * @param node Node
 */
void ProCurveDriver::analyzeDevice(SNMP_Transport *snmp, const SNMP_ObjectId& oid, NObject *node, DriverData **driverData)
{
	uint32_t model = oid.getElement(11);

	// modular switches
	if ((model == 7) || (model == 9) || (model == 13) || (model == 14) || (model == 23) || 
	    (model == 27) || (model == 46) || (model == 47) || (model == 50) || (model == 51))
	{
		node->setCustomAttribute(_T(".procurve.isModular"), _T("1"), StateChange::IGNORE);
		node->setCustomAttribute(_T(".procurve.slotSize"), _T("24"), StateChange::IGNORE);
	}
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 */
InterfaceList *ProCurveDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
	// Get interface list from standard MIB
	InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, useIfXTable);
	if (ifList == nullptr)
		return nullptr;

	bool isModular = node->getCustomAttributeAsBoolean(_T(".procurve.isModular"), false);
	uint32_t slotSize = node->getCustomAttributeAsUInt32(_T(".procurve.slotSize"), 24);

	// Find physical ports
	for(int i = 0; i < ifList->size(); i++)
	{
		InterfaceInfo *iface = ifList->get(i);
		if (iface->type == IFTYPE_ETHERNET_CSMACD)
		{
			iface->isPhysicalPort = true;
			if (isModular)
			{
			   int p = iface->index % slotSize;
			   if (p != 0)
			   {
               iface->location.module = (iface->index / slotSize) + 1;
               iface->location.port = p;
			   }
			   else
			   {
			      // Last port in slot
               iface->location.module = iface->index / slotSize;
               iface->location.port = slotSize;
			   }
			}
			else
			{
				iface->location.module = 1;
				iface->location.port = iface->index;
			}
		}
	}

	return ifList;
}
