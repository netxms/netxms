/* 
** NetXMS - Network Management System
** Driver for Avaya ERS switches (except ERS 8xxx) (former Nortel/Bay Networks BayStack)
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
** File: baystack.cpp
**/

#include "avaya.h"

/**
 * Get driver name
 */
const TCHAR *BayStackDriver::getName()
{
	return _T("BAYSTACK");
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int BayStackDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
	return oid.startsWith({ 1, 3, 6, 1, 4, 1, 45, 3 }) ? 127 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool BayStackDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
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
void BayStackDriver::analyzeDevice(SNMP_Transport *snmp, const SNMP_ObjectId& oid, NObject *node, DriverData **driverData)
{
   uint32_t slotSize;
	
	if (oid.startsWith({ 1, 3, 6, 1, 4, 1, 45, 3, 74 }) ||  // 56xx
	    oid.startsWith({ 1, 3, 6, 1, 4, 1, 45, 3, 65 }))    // 5530-24TFD
	{
		slotSize = 128;
	}
	/* TODO: should OPtera Metro 1200 be there? */
	else if (oid.startsWith({ 1, 3, 6, 1, 4, 1, 45, 3, 35 }) || // BayStack 450
	         oid.startsWith({ 1, 3, 6, 1, 4, 1, 45, 3, 40 }) || // BPS2000
	         oid.startsWith({ 1, 3, 6, 1, 4, 1, 45, 3, 43 }))   // BayStack 420
	{
		slotSize = 32;
	}
	else
	{
		slotSize = 64;
	}

	node->setCustomAttribute(_T(".baystack.slotSize"), slotSize);

	uint32_t numVlans;
	if (SnmpGet(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.2272.1.3.1.0"), nullptr, 0, &numVlans, sizeof(uint32_t), 0) == SNMP_ERR_SUCCESS)
		node->setCustomAttribute(_T(".baystack.rapidCity.vlan"), numVlans);
	else
		node->setCustomAttribute(_T(".baystack.rapidCity.vlan"), (uint32_t)0);
}

/**
 * Get slot size from object's custom attributes. Default implementation always return constant value 64.
 *
 * @param node object to get custom attribute
 * @return slot size
 */
uint32_t BayStackDriver::getSlotSize(NObject *node)
{
	return node->getCustomAttributeAsUInt32(_T(".baystack.slotSize"), 64);
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param useIfXTable if true, usage of ifXTable is allowed
 */
InterfaceList *BayStackDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
	// Get interface list from standard MIB
	InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, useIfXTable);
	if (ifList == nullptr)
		return nullptr;

   // Translate interface names 
	// TODO: does it really needed?
   for(int i = 0; i < ifList->size(); i++)
   {
		InterfaceInfo *iface = ifList->get(i);

		const TCHAR *ptr;
      if ((ptr = _tcsstr(iface->name, _T("- Port"))) != nullptr)
		{
			ptr += 2;
         memmove(iface->name, ptr, _tcslen(ptr) + 1);
		}
      else if ((ptr = _tcsstr(iface->name, _T("- Unit"))) != nullptr)
		{
			ptr += 2;
         memmove(iface->name, ptr, _tcslen(ptr) + 1);
		}
      else if ((_tcsstr(iface->name, _T("BayStack")) != nullptr) ||
               (_tcsstr(iface->name, _T("Nortel Ethernet Switch")) != nullptr))
      {
         ptr = _tcsrchr(iface->name, _T('-'));
         if (ptr != nullptr)
         {
            ptr++;
            while(*ptr == _T(' '))
               ptr++;
            memmove(iface->name, ptr, _tcslen(ptr) + 1);
         }
      }
		Trim(iface->name);
   }
	
	// Calculate slot/port pair from ifIndex
	uint32_t slotSize = node->getCustomAttributeAsUInt32(_T(".baystack.slotSize"), 64);
	for(int i = 0; i < ifList->size(); i++)
	{
	   uint32_t slot = ifList->get(i)->index / slotSize + 1;
		if ((slot > 0) && (slot <= 8))
		{
			ifList->get(i)->location.module = slot;
			ifList->get(i)->location.port = ifList->get(i)->index % slotSize;
			ifList->get(i)->isPhysicalPort = true;
		}
	}

	if (node->getCustomAttributeAsUInt32(_T(".baystack.rapidCity.vlan"), 0) > 0)
		getVlanInterfaces(snmp, ifList);

	uint32_t mgmtIpAddr, mgmtNetMask;
	if ((SnmpGet(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.45.1.6.4.2.2.1.2.1"), nullptr, 0, &mgmtIpAddr, sizeof(uint32_t), 0) == SNMP_ERR_SUCCESS) &&
	    (SnmpGet(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.45.1.6.4.2.2.1.3.1"), nullptr, 0, &mgmtNetMask, sizeof(uint32_t), 0) == SNMP_ERR_SUCCESS))
	{
		// Get switch base MAC address
		BYTE baseMacAddr[MAC_ADDR_LENGTH];
		SnmpGet(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.45.1.6.4.2.2.1.10.1"), nullptr, 0, baseMacAddr, MAC_ADDR_LENGTH, SG_RAW_RESULT);

		if (mgmtIpAddr != 0)
		{
			int i;

			// Add management virtual interface if management IP is missing in interface list
			for(i = 0; i < ifList->size(); i++)
         {
            if (ifList->get(i)->hasAddress(mgmtIpAddr))
					break;
         }
			if (i == ifList->size())
			{
            InterfaceInfo *iface = new InterfaceInfo(0);
            iface->ipAddrList.add(InetAddress(mgmtIpAddr, mgmtNetMask));
				_tcscpy(iface->name, _T("MGMT"));
				memcpy(iface->macAddr, baseMacAddr, MAC_ADDR_LENGTH);
				ifList->add(iface);
			}
		}

		// Update wrongly reported MAC addresses
		for(int i = 0; i < ifList->size(); i++)
		{
			InterfaceInfo *curr = ifList->get(i);
			if ((curr->location.module != 0) &&
				 (!memcmp(curr->macAddr, "\x00\x00\x00\x00\x00\x00", MAC_ADDR_LENGTH) ||
			     !memcmp(curr->macAddr, baseMacAddr, MAC_ADDR_LENGTH)))
			{
				memcpy(curr->macAddr, baseMacAddr, MAC_ADDR_LENGTH);
				curr->macAddr[5] += (BYTE)curr->location.port;
			}
		}
	}

	return ifList;
}
