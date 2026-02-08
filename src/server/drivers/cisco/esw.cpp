/* 
** NetXMS - Network Management System
** Driver for Cisco ESW switches
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: esw.cpp
**/

#include "cisco.h"

/**
 * Get driver name
 */
const TCHAR *CiscoEswDriver::getName()
{
	return _T("CISCO-ESW");
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int CiscoEswDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
	if (!oid.startsWith({ 1, 3, 6, 1, 4, 1, 9, 1 }))
		return 0;

	uint32_t model = oid.getElement(8);
	if (((model >= 1058) && (model <= 1064)) || (model == 1176) || (model == 1177))
		return 251;
	return 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool CiscoEswDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
	return true;
}

/**
 * Determine maximum speed from ESW interface characteristics
 */
static uint64_t MaxSpeedFromInterfaceInfo(const InterfaceInfo *iface, uint32_t model)
{
   // Check interface name first for standard Cisco naming
   StringBuffer name(iface->name);
   name.toUppercase();

   // Standard Cisco interface naming
   if (name.startsWith(_T("FASTETHERNET")) || name.startsWith(_T("FA")))
      return _ULL(100000000); // 100 Mbps

   if (name.startsWith(_T("GIGABITETHERNET")) || name.startsWith(_T("GI")))
      return _ULL(1000000000); // 1 Gbps

   if (name.startsWith(_T("TENGIGABITETHERNET")) || name.startsWith(_T("TE")))
      return _ULL(10000000000); // 10 Gbps

   // ESW-specific speed detection based on interface index and device model
   if ((iface->type == IFTYPE_ETHERNET_CSMACD) && (iface->index <= 48))
   {
      // Physical ports - determine speed based on device model and port range
      switch (model)
      {
         case 1058: // ESW-520 series
         case 1059:
         case 1060:
            // 8x 10/100 + 2x Gigabit uplinks
            return (iface->index <= 8) ? _ULL(100000000) : _ULL(1000000000);

         case 1061: // ESW-540 series
         case 1062:
         case 1063:
         case 1064:
            // 24x 10/100 + 4x Gigabit uplinks
            return (iface->index <= 24) ? _ULL(100000000) : _ULL(1000000000);

         case 1176: // ESW-500 series
         case 1177:
            // 8x 10/100/1000
            return _ULL(1000000000);

         default:
            // Default ESW behavior - assume 100M for lower ports, 1G for higher ports
            return (iface->index <= 24) ? _ULL(100000000) : _ULL(1000000000);
      }
   }

   return 0; // Unknown interface type
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param useIfXTable if true, usage of ifXTable is allowed
 */
InterfaceList *CiscoEswDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
	// Get interface list from standard MIB
	InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, useIfXTable);
	if (ifList == nullptr)
		return nullptr;

	// Get device model for speed detection
	SNMP_ObjectId deviceOid;
	if (SnmpGetEx(snmp, _T(".1.3.6.1.2.1.1.2.0"), nullptr, 0, &deviceOid, sizeof(SNMP_ObjectId), SG_OBJECT_ID_RESULT) != SNMP_ERR_SUCCESS)
	{
		nxlog_debug_tag(DEBUG_TAG_CISCO, 4, _T("CiscoEswDriver::getInterfaces: cannot get device OID"));
	}
   uint32_t model = (deviceOid.length() >= 9) ? deviceOid.getElement(8) : 0;

	// Process interfaces
	for(int i = 0; i < ifList->size(); i++)
	{
		InterfaceInfo *iface = ifList->get(i);
		
		// Mark physical ports
		if ((iface->type == IFTYPE_ETHERNET_CSMACD) && (iface->index <= 48))
		{
			iface->isPhysicalPort = true;
			iface->location.module = 1;
			iface->location.port = iface->index;
		}

		// Set maximum speed based on ESW-specific logic
		iface->maxSpeed = MaxSpeedFromInterfaceInfo(iface, model);
	}

	return ifList;
}
