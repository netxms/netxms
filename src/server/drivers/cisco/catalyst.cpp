/* 
** NetXMS - Network Management System
** Driver for Cisco Catalyst switches
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
** File: catalyst.cpp
**/

#include "cisco.h"

/**
 * Get driver name
 */
const TCHAR *CatalystDriver::getName()
{
   return _T("CATALYST-GENERIC");
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int CatalystDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return oid.startsWith({ 1, 3, 6, 1, 4, 1, 9, 1 }) ? 250 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool CatalystDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
   uint32_t value = 0;
   if (SnmpGetEx(snmp, _T(".1.3.6.1.4.1.9.5.1.2.14.0"), nullptr, 0, &value, sizeof(uint32_t), 0, nullptr) == SNMP_ERR_SUCCESS)
      return true;

   SNMP_PDU request(SNMP_GET_NEXT_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   SNMP_ObjectId rootOid({ 1, 3, 6, 1, 4, 1, 9, 5, 1, 4, 1, 1, 11 });
   request.bindVariable(new SNMP_Variable(rootOid));
   SNMP_PDU *response = nullptr;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      bool success = false;
      if (response->getNumVariables() > 0)
      {
         SNMP_Variable *var = response->getVariable(0);
         success = (var->getType() != ASN_NO_SUCH_OBJECT) &&
                   (var->getType() != ASN_NO_SUCH_INSTANCE) &&
                   (var->getType() != ASN_END_OF_MIBVIEW) &&
                   (var->getName().compare(rootOid) == OID_LONGER);
      }
      delete response;
      return success;
   }

   return false;
}

/**
 * Handler for switch port enumeration
 */
static uint32_t HandlerPortList(SNMP_Variable *var, SNMP_Transport *transport, InterfaceList *ifList)
{
   InterfaceInfo *iface = ifList->findByIfIndex(var->getValueAsUInt());
   if (iface != nullptr)
   {
		size_t nameLen = var->getName().length();
		
		uint32_t moduleIndex = var->getName().getElement(nameLen - 2);
		uint32_t oid[] = { 1, 3, 6, 1, 4, 1, 9, 5, 1, 3, 1, 1, 25, 0 };
		oid[13] = moduleIndex;
		uint32_t slot;
		if (SnmpGetEx(transport, nullptr, oid, 14, &slot, sizeof(uint32_t), 0, nullptr) != SNMP_ERR_SUCCESS)
			slot = moduleIndex;	// Assume slot # equal to module index if it cannot be read

		iface->location.module = slot;
		iface->location.port = var->getName().getElement(nameLen - 1);
		iface->isPhysicalPort = true;
	}
	return SNMP_ERR_SUCCESS;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param useIfXTable if true, usage of ifXTable is allowed
 */
InterfaceList *CatalystDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
   // Get interface list from standard MIB
   InterfaceList *ifList = CiscoDeviceDriver::getInterfaces(snmp, node, driverData, useIfXTable);
   if (ifList == nullptr)
      return nullptr;

   // Set slot and port number for physical interfaces
   SnmpWalk(snmp, _T(".1.3.6.1.4.1.9.5.1.4.1.1.11"), HandlerPortList, ifList);

   return ifList;
}
