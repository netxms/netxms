/* 
** NetXMS - Network Management System
** Driver for D-Link switches
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
** File: dlink.cpp
**/

#include "dlink.h"
#include <netxms-version.h>

#define DEBUG_TAG _T("ndd.dlink")

/**
 * Get driver name
 */
const TCHAR *DLinkDriver::getName()
{
	return _T("DLINK");
}

/**
 * Get driver version
 */
const TCHAR *DLinkDriver::getVersion()
{
	return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int DLinkDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
	return oid.startsWith({ 1, 3, 6, 1, 4, 1, 171, 10 }) ? 127 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool DLinkDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
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
void DLinkDriver::analyzeDevice(SNMP_Transport *snmp, const SNMP_ObjectId& oid, NObject *node, DriverData **driverData)
{
	node->setCustomAttribute(_T(".dlink.slotSize"), 48);

	// Check if device returns incorrect values for ifHighSpeed
	bool ifHighSpeedInBps = false;
	SnmpWalk(snmp, _T(".1.3.6.1.2.1.31.1.1.1.15"), [snmp, node, &ifHighSpeedInBps](SNMP_Variable *v) {
	   uint32_t ifHighSpeed = v->getValueAsUInt();
	   if (ifHighSpeed != 0)
      {
	      uint32_t ifIndex = v->getName().getLastElement();

	      TCHAR oid[256];
	      _sntprintf(oid, 256, _T(".1.3.6.1.2.1.2.2.1.5.%u"), ifIndex);

	      uint32_t ifSpeed;
	      if (SnmpGetEx(snmp, oid, nullptr, 0, &ifSpeed, sizeof(uint32_t), 0) == SNMP_ERR_SUCCESS)
         {
	         if (ifSpeed == ifHighSpeed)
            {
	            ifHighSpeedInBps = true;
	            nxlog_debug_tag(DEBUG_TAG, 5, _T("DLinkDriver::analyzeDevice(%s): ifSpeed == ifHighSpeed for interface %u, assuming that ifHighSpeed reports values in bps"), node->getName(), ifIndex);
	            return SNMP_ERR_ABORTED;
            }
         }
      }
	   return SNMP_ERR_SUCCESS;
	});
   node->setCustomAttribute(_T(".dlink.ifHighSpeedInBps"), ifHighSpeedInBps ? _T("true") : _T("false"), StateChange::CLEAR);
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param useIfXTable if true, usage of ifXTable is allowed
 */
InterfaceList *DLinkDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
	// Get interface list from standard MIB
	InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, useIfXTable);
	if (ifList == nullptr)
		return nullptr;

	uint32_t slotSize = node->getCustomAttributeAsUInt32(_T(".dlink.slotSize"), 48);

	// Find physical ports
	for(int i = 0; i < ifList->size(); i++)
	{
		InterfaceInfo *iface = ifList->get(i);
		if (iface->index < 1024)
		{
			iface->isPhysicalPort = true;
			iface->location.module = (iface->index / slotSize) + 1;
			iface->location.port = iface->index % slotSize;
		}
	}

	if (node->getCustomAttributeAsBoolean(_T(".dlink.ifHighSpeedInBps"), false))
	{
	   uint32_t ifHighSpeed;
      uint32_t oid[] = { 1, 3, 6, 1, 2, 1, 31, 1, 1, 1, 15, 0 };
	   for(int i = 0; i < ifList->size(); i++)
	   {
	      InterfaceInfo *iface = ifList->get(i);
	      if (iface->speed != 0)
	      {
	         oid[11] = iface->index;
	         if (SnmpGetEx(snmp, nullptr, oid, 12, &ifHighSpeed, sizeof(uint32_t), 0) == SNMP_ERR_SUCCESS)
	         {
	            iface->speed = ifHighSpeed;
	         }
	      }
	   }
	}

	return ifList;
}

/**
 * Handler for VLAN enumeration
 */
static uint32_t HandlerVlanList(SNMP_Variable *var, SNMP_Transport *transport, VlanList *vlanList)
{
	VlanInfo *vlan = new VlanInfo(var->getName().getLastElement(), VLAN_PRM_BPORT);

	TCHAR buffer[256];
	vlan->setName(var->getValueAsString(buffer, 256));

	vlanList->add(vlan);
   return SNMP_ERR_SUCCESS;
}

/**
 * Parse VLAN membership bit map
 *
 * @param vlanList VLAN list
 * @param ifIndex interface index for current interface
 * @param map VLAN membership map
 * @param offset VLAN ID offset from 0
 */
static void ParseVlanPorts(VlanList *vlanList, VlanInfo *vlan, BYTE map, int offset)
{
	// VLAN egress port map description from Q-BRIDGE-MIB:
	// ===================================================
	// Each octet within this value specifies a set of eight
	// ports, with the first octet specifying ports 1 through
	// 8, the second octet specifying ports 9 through 16, etc.
	// Within each octet, the most significant bit represents
	// the lowest numbered port, and the least significant bit
	// represents the highest numbered port.  Thus, each port
	// of the bridge is represented by a single bit within the
	// value of this object.  If that bit has a value of '1'
	// then that port is included in the set of ports; the port
	// is not included if its bit has a value of '0'.

	uint32_t port = offset;
	BYTE mask = 0x80;
	while(mask > 0)
	{
		if (map & mask)
		{
			vlan->add(port);
		}
		mask >>= 1;
		port++;
	}
}

/**
 * Handler for VLAN egress port enumeration
 */
static uint32_t HandlerVlanEgressPorts(SNMP_Variable *var, SNMP_Transport *transport, VlanList *vlanList)
{
   uint32_t vlanId = var->getName().getLastElement();
	VlanInfo *vlan = vlanList->findById(vlanId);
	if (vlan != nullptr)
	{
		BYTE buffer[4096];
		size_t size = var->getRawValue(buffer, 4096);
		for(int i = 0; i < (int)size; i++)
		{
			ParseVlanPorts(vlanList, vlan, buffer[i], i * 8 + 1);
		}
	}
   vlanList->setData(vlanList);  // to indicate that callback was called
	return SNMP_ERR_SUCCESS;
}

/**
 * Get VLANs 
 */
VlanList *DLinkDriver::getVlans(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   // Most DLINK devices stores VLAN information in common place dot1qBridge SNMP SubTree .iso.org.dod.internet.mgmt.mib-2.dot1dBridge.qBridgeMIB.qBridgeMIBObjects.dot1qVlan
   // Standard function can get this information
   nxlog_debug_tag(DEBUG_TAG, 6, _T("DLinkDriver::getVlans(%s [%u]): Processing VLANs"), node->getName(), node->getId());

   VlanList *list = NetworkDeviceDriver::getVlans(snmp, node, driverData);
   if ((list != nullptr) && (list->size() > 0))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("DLinkDriver::getVlans(%s [%u]): standard VLAN reading method successful (%d entries)"), node->getName(), node->getId(), list->size());
      return list;   // retrieved from standard MIBs
   }
   delete list;   // In case it is empty

   TCHAR systemOid[128];
   if (SnmpGet(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.2.1.1.2.0"), nullptr, 0, systemOid, sizeof(systemOid), SG_STRING_RESULT) != SNMP_ERR_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("DLinkDriver::getVlans(%s [%u]): cannot read system OID from device"), node->getName(), node->getId());
      return nullptr;
   }

   nxlog_debug_tag(DEBUG_TAG, 6, _T("DLinkDriver::getVlans(%s [%u]): trying custom methods (device OID = %s)"), node->getName(), node->getId(), systemOid);
   list = new VlanList();
   if (_tcsncmp(systemOid, _T("1.3.6.1.4.1.171.10.153."), 23) == 0)
   {
      // .1.3.6.1.4.1.171.10.153 DGS-1210 Series some models returns VLAN information from COMMON location
      // DGS-1210 Projects stores also at DGS120
      // VLAN NAMES:        .1.3.6.1.4.1.171.11.153.1000.7.6.1.1
      // VLAN EGRESS PORTS: .1.3.6.1.4.1.171.11.153.1000.7.6.1.2
      nxlog_debug_tag(DEBUG_TAG, 5, _T("DLinkDriver::getVlans: got device with ID: %u  system OID [%s] it's a DGS-1210 variant"), node->getId(), systemOid);
      if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.171.11.153.1000.7.6.1.1"), HandlerVlanList, list) != SNMP_ERR_SUCCESS)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("DLinkDriver::getVlans(%s [%u]): cannot get VLAN names for DGS-1210 series switch"), node->getName(), node->getId());
         delete list;
         return nullptr;
      }

      if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.171.11.153.1000.7.6.1.2"), HandlerVlanEgressPorts, list) != SNMP_ERR_SUCCESS)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("DLinkDriver::getVlans(%s [%u]): cannot get VLAN ports for DGS-1210 series switch"), node->getName(), node->getId());
         delete_and_null(list);
      }
   }
   else
   {
      // Some DLINK models stores VLAN information in a DEVICE SNMP subtree
      TCHAR oid[128];
      _sntprintf(oid, 128, _T("%s.1.7.6.1.1"), systemOid);
      nxlog_debug_tag(DEBUG_TAG, 6, _T("DLinkDriver::getVlans(%s [%u]): reading VLAN names from %s"), node->getName(), node->getId(), oid);
      if (SnmpWalk(snmp, oid, HandlerVlanList, list) != SNMP_ERR_SUCCESS)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("DLinkDriver::getVlans(%s [%u]): cannot get VLAN names from %s"), node->getName(), node->getId(), oid);
         delete list;
         return nullptr;
      }

      _sntprintf(oid, 128, _T("%s.1.7.6.1.2"), systemOid);
      nxlog_debug_tag(DEBUG_TAG, 6, _T("DLinkDriver::getVlans(%s [%u]): reading VLAN egress ports from %s"), node->getName(), node->getId(), oid);
      if (SnmpWalk(snmp, oid, HandlerVlanEgressPorts, list) != SNMP_ERR_SUCCESS)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("DLinkDriver::getVlans(%s [%u]): cannot get VLAN egress ports from %s"), node->getName(), node->getId(), oid);
         delete_and_null(list);
      }
   }
   return list;
}

/**
 * Driver entry point
 */
DECLARE_NDD_ENTRY_POINT(DLinkDriver);

/**
 * DLL entry point
 */
#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
