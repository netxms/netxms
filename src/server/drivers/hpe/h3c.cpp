/* 
** NetXMS - Network Management System
** Driver for H3C (now HPE A-series) switches
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
** File: h3c.cpp
**/

#include "hpe.h"
#include <netxms-regex.h>

#define DEBUG_TAG_H3C   _T("ndd.h3c")

/**
 * Get driver name
 */
const TCHAR *H3CDriver::getName()
{
	return _T("H3C");
}

/**
 * Get driver version
 */
const TCHAR *H3CDriver::getVersion()
{
	return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int H3CDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
	return oid.startsWith({ 1, 3, 6, 1, 4, 1, 43, 1, 16, 4, 3 }) ? 255 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool H3CDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
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
void H3CDriver::analyzeDevice(SNMP_Transport *snmp, const SNMP_ObjectId& oid, NObject *node, DriverData **driverData)
{
   TCHAR sysDescr[256];
   if (SnmpGetEx(snmp, { 1, 3, 6, 1, 2, 1, 1, 1, 0 }, sysDescr, sizeof(sysDescr), SG_STRING_RESULT, nullptr) != SNMP_ERR_SUCCESS)
      return;

   const char *eptr;
   int eoffset;
   PCRE *re = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(_T("Software Version ([0-9]+)\\.([0-9]+) Release")), PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, nullptr);
   if (re == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_H3C, 5, _T("H3CDriver::analyzeDevice(%s): cannot compile software version regexp: %hs at offset %d"), node->getName(), eptr, eoffset);
      return;
   }

   int pmatch[30];
   if (_pcre_exec_t(re, nullptr, reinterpret_cast<PCRE_TCHAR*>(sysDescr), static_cast<int>(_tcslen(sysDescr)), 0, 0, pmatch, 30) == 3)
   {
      uint32_t major = IntegerFromCGroup(sysDescr, pmatch, 1);
      nxlog_debug_tag(DEBUG_TAG_H3C, 5, _T("H3CDriver::analyzeDevice(%s): detected software version %u.%02u"), node->getName(), major, IntegerFromCGroup(sysDescr, pmatch, 2));
      if (major <= 3)
      {
         node->setCustomAttribute(_T(".ndd.h3c.vlanFix"), _T("true"), StateChange::IGNORE);
      }
   }

   _pcre_free_t(re);
}

/**
 * Handler for port walk
 */
static uint32_t PortWalkHandler(SNMP_Variable *var, SNMP_Transport *snmp, InterfaceList *ifList)
{
   InterfaceInfo *iface = ifList->findByIfIndex(var->getValueAsUInt());
   if (iface != nullptr)
   {
      iface->isPhysicalPort = true;
      iface->location.chassis = var->getName().getElement(17);
      iface->location.module = var->getName().getElement(18);
      iface->location.pic = var->getName().getElement(19);
      iface->location.port = var->getName().getElement(20);
   }
   return SNMP_ERR_SUCCESS;
}

/**
 * Handler for IPv6 address walk
 */
static uint32_t IPv6WalkHandler(SNMP_Variable *var, SNMP_Transport *snmp, InterfaceList *ifList)
{
   // Address type should be IPv6 and address length 16 bytes
   if ((var->getName().getElement(18) == 2) &&
       (var->getName().length() == 36) &&
       (var->getName().getElement(19) == 16))
   {
      InterfaceInfo *iface = ifList->findByIfIndex(var->getName().getElement(17));
      if (iface != nullptr)
      {
         BYTE addrBytes[16];
         for(int i = 20; i < 36; i++)
            addrBytes[i - 20] = (BYTE)var->getName().getElement(i);
         InetAddress addr(addrBytes, var->getValueAsInt());
         iface->ipAddrList.add(addr);
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
InterfaceList *H3CDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
	// Get interface list from standard MIB
	InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, useIfXTable);
	if (ifList == nullptr)
		return nullptr;

	// Find physical ports
   SnmpWalk(snmp, _T(".1.3.6.1.4.1.43.45.1.2.23.1.18.4.5.1.3"), PortWalkHandler, ifList);

   // Read IPv6 addresses
   SnmpWalk(snmp, _T(".1.3.6.1.4.1.43.45.1.10.2.71.1.1.2.1.4"), IPv6WalkHandler, ifList);

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

   // However, H3C switches with older software (probably 3.x) return
   // port bits in reversed order within each byte
   uint32_t port = offset;
   BYTE mask = 0x01;
   while(mask != 0)
   {
      if (map & mask)
      {
         vlan->add(port);
      }
      mask <<= 1;
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
   return SNMP_ERR_SUCCESS;
}

/**
 * Get list of VLANs on given node
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return VLAN list or NULL
 */
VlanList *H3CDriver::getVlans(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   // Use default implementation if fix is not needed
   if (!node->getCustomAttributeAsBoolean(_T(".ndd.h3c.vlanFix"), false))
      return NetworkDeviceDriver::getVlans(snmp, node, driverData);

   VlanList *list = new VlanList();

   // dot1qVlanStaticName
   if (SnmpWalk(snmp, _T(".1.3.6.1.2.1.17.7.1.4.3.1.1"), HandlerVlanList, list) != SNMP_ERR_SUCCESS)
      goto failure;

   // Use dot1qVlanStaticEgressPorts because dot1qVlanCurrentEgressPorts usually empty
   if (SnmpWalk(snmp, _T(".1.3.6.1.2.1.17.7.1.4.3.1.2"), HandlerVlanEgressPorts, list) != SNMP_ERR_SUCCESS)
      goto failure;

   return list;

failure:
   delete list;
   return nullptr;
}

/**
 * Get orientation of the modules in the device
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return module orientation
 */
int H3CDriver::getModulesOrientation(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   return NDD_ORIENTATION_HORIZONTAL;
}

/**
 * Get port layout of given module
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @param module Module number (starting from 1)
 * @param layout Layout structure to fill
 */
void H3CDriver::getModuleLayout(SNMP_Transport *snmp, NObject *node, DriverData *driverData, int module, NDD_MODULE_LAYOUT *layout)
{
   layout->numberingScheme = NDD_PN_DU_LR;
   layout->rows = 2;
}
