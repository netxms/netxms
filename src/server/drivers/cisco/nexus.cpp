/*
** NetXMS - Network Management System
** Driver for Cisco Nexus switches
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
** File: nexus.cpp
**/

#include "cisco.h"
#include <entity_mib.h>
#include <netxms-regex.h>

/**
 * Get driver name
 */
const TCHAR *CiscoNexusDriver::getName()
{
   return _T("CISCO-NEXUS");
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int CiscoNexusDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   static uint32_t supportedDevices[] =
   {
      612, 719, 720, 777, 798, 820, 840, 903, 907, 913, 914, 930, 932, 1003, 1004, 1005, 1006, 1008, 1038, 1046,
      1058, 1059, 1060, 1061, 1084, 1105, 1106, 1107, 1147, 1163, 1175, 1190, 1205, 1210, 1211, 1237, 1238, 1239,
      1288, 1289, 1290, 1291, 1292, 1294, 1308, 1339, 1340, 1352, 1353, 1354, 1388, 1389, 1390, 1391, 1409, 1410,
      1417, 1467, 1489, 1506, 1507, 1508, 1509, 1510, 1539, 1540, 1541, 1570, 1571, 1609, 1619, 1620, 1621, 1625,
      1626, 1627, 1648, 1652, 1666, 1675, 1684, 1690, 1691, 1692, 1693, 1694, 1695, 1696, 1697, 1706, 1712, 1713,
      1726, 1727, 1728, 1738, 1743, 1744, 1745, 1746, 1758, 1759, 1777, 1778, 1779, 1780, 1781, 1782, 1783, 1784,
      1812, 1824, 1827, 1839, 1843, 1850, 1893, 1912, 1921, 1951, 1954, 1955, 2010, 2039, 2076, 2077, 2115, 2117,
      2183, 2191, 2207, 2227, 2228, 2236, 2237, 2239, 2272, 0
   };

   if (!oid.startsWith({ 1, 3, 6, 1, 4, 1, 9, 12, 3, 1, 3 }))
      return 0;

   uint32_t id = oid.getElement(11);
   for(int i = 0; supportedDevices[i] != 0; i++)
      if (supportedDevices[i] == id)
         return 200;
   return 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool CiscoNexusDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
   return true;
}

/**
 * Handler for IP address enumeration
 */
static UINT32 HandlerIPAddressList(SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
   //.1.3.6.1.4.1.9.9.309.1.1.3.1.1.436731904.1.4.10.5.130.2
   const UINT32 *oid = var->getName().value();

   InetAddress addr;
   if (((oid[15] == 1) && (oid[16] == 4)) || ((oid[15] == 3) && (oid[16] == 8))) // ipv4 and ipv4z
   {
      addr = InetAddress((uint32_t)((oid[17] << 24) | (oid[18] << 16) | (oid[19] << 8) | oid[20]));
   }
   else if (((oid[15] == 2) && (oid[16] == 16)) || ((oid[15] == 4) && (oid[16] == 20))) // ipv6 and ipv6z
   {
      BYTE bytes[16];
      for(int i = 17, j = 0; j < 16; i++, j++)
         bytes[j] = (BYTE)oid[i];
      addr = InetAddress(bytes);
   }
   else
   {
      return SNMP_ERR_SUCCESS;   // Unknown or unsupported address format
   }

   auto ifList = static_cast<InterfaceList*>(arg);
   InterfaceInfo *iface = ifList->findByIfIndex(oid[14]);
   if (iface != nullptr)
      iface->ipAddrList.add(addr);

   return SNMP_ERR_SUCCESS;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param useAliases policy for interface alias usage
 * @param useIfXTable if true, usage of ifXTable is allowed
 */
InterfaceList *CiscoNexusDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
   // Get interface list from standard MIB
   InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, useIfXTable);
   if (ifList == nullptr)
      return nullptr;

   // Read IP addresses
   SnmpWalk(snmp, _T(".1.3.6.1.4.1.9.9.309.1.1.3.1.1"), HandlerIPAddressList, ifList); // ciiIPIfAddressPrefixLength

   const char *eptr;
   int eoffset;
   PCRE *reBase = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(_T("^(Ethernet|fc)([0-9]+)/([0-9]+)$")), PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, nullptr);
   if (reBase == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_CISCO, 5, _T("CiscoNexusDriver::getInterfaces: cannot compile base regexp: %hs at offset %d"), eptr, eoffset);
      return ifList;
   }
   PCRE *reFex = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(_T("^(Ethernet|fc)([0-9]+)/([0-9]+)/([0-9]+)$")), PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, nullptr);
   if (reFex == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_CISCO, 5, _T("CiscoNexusDriver::getInterfaces: cannot compile FEX regexp: %hs at offset %d"), eptr, eoffset);
      _pcre_free_t(reBase);
      return ifList;
   }

   int pmatch[30];
   for(int i = 0; i < ifList->size(); i++)
   {
      InterfaceInfo *iface = ifList->get(i);
      if (_pcre_exec_t(reBase, nullptr, reinterpret_cast<PCRE_TCHAR*>(iface->name), static_cast<int>(_tcslen(iface->name)), 0, 0, pmatch, 30) == 4)
      {
         iface->isPhysicalPort = true;
         iface->location.chassis = 1;
         iface->location.module = IntegerFromCGroup(iface->name, pmatch, 2);
         iface->location.port = IntegerFromCGroup(iface->name, pmatch, 3);
      }
      else if (_pcre_exec_t(reFex, nullptr, reinterpret_cast<PCRE_TCHAR*>(iface->name), static_cast<int>(_tcslen(iface->name)), 0, 0, pmatch, 30) == 5)
      {
         iface->isPhysicalPort = true;
         iface->location.chassis = IntegerFromCGroup(iface->name, pmatch, 2);
         iface->location.module = IntegerFromCGroup(iface->name, pmatch, 3);
         iface->location.port = IntegerFromCGroup(iface->name, pmatch, 4);
      }
   }

   _pcre_free_t(reBase);
   _pcre_free_t(reFex);
   return ifList;
}

/**
 * Handler for VLAN enumeration
 */
static uint32_t HandlerVlanList(SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
   VlanList *vlanList = static_cast<VlanList*>(arg);

   VlanInfo *vlan = new VlanInfo(var->getName().getLastElement(), VLAN_PRM_IFINDEX);

   TCHAR buffer[256];
   vlan->setName(var->getValueAsString(buffer, 256));

   vlanList->add(vlan);
   return SNMP_ERR_SUCCESS;
}

/**
 * Add trunk port to VLANs
 */
static void AddPortToVLANs(SNMP_Variable *v, int baseId, VlanList *vlanList)
{
   // Each octet within the value of this object specifies a
   // set of eight VLANs, with the first octet specifying
   // VLAN id 1 through 8, the second octet specifying VLAN
   // ids 9 through 16, etc.   Within each octet, the most
   // significant bit represents the lowest numbered
   // VLAN id, and the least significant bit represents the
   // highest numbered VLAN id.  Thus, each VLAN of the
   // port is represented by a single bit within the
   // value of this object.  If that bit has a value of
   // '1' then that VLAN is included in the set of
   // VLANs; the VLAN is not included if its bit has a
   // value of '0'.

   BYTE vlanMask[128];
   memset(vlanMask, 0, 128);
   v->getRawValue(vlanMask, 128);
   for(int i = 0; i < 128; i++)
   {
      BYTE mask = 0x80;
      for(int b = 0; b < 8; b++, mask >>= 1)
      {
         if (vlanMask[i] & mask)
         {
            vlanList->addMemberPort(baseId + i * 8 + b, v->getName().getLastElement());
         }
      }
   }
}

/**
 * Handler for VLAN ports enumeration
 */
static uint32_t HandlerVlanPorts(SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
   VlanList *vlanList = static_cast<VlanList*>(arg);

   SNMP_ObjectId oid = var->getName();
   uint32_t ifIndex = oid.getLastElement();

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), transport->getSnmpVersion());
   if (var->getValueAsInt() == 3)
   {
      for(int i = 4; i <= 7; i++)
      {
         oid.changeElement(oid.length() - 2, i);
         request.bindVariable(new SNMP_Variable(oid));
      }
   }
   else
   {
      oid.changeElement(oid.length() - 2, 2);
      request.bindVariable(new SNMP_Variable(oid));
   }

   SNMP_PDU *response = nullptr;
   if (transport->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      if (var->getValueAsInt() == 3)
      {
         for(int i = 0; i < 4; i++)
         {
            SNMP_Variable *v = response->getVariable(i);
            if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
               AddPortToVLANs(v, i * 1024, vlanList);
         }
      }
      else
      {
         SNMP_Variable *v = response->getVariable(0);
         if ((v != nullptr) && (v->getType() == ASN_INTEGER))
         {
            vlanList->addMemberPort(v->getValueAsInt(), ifIndex);
         }
      }
      delete response;
   }

   return SNMP_ERR_SUCCESS;
}

/**
 * Handler for VLAN ports enumeration
 */
static uint32_t HandlerVlanTrunkPorts(SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
   auto context = static_cast<std::pair<VlanList*, int>*>(arg);
   AddPortToVLANs(var, context->second, context->first);
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
VlanList *CiscoNexusDriver::getVlans(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   VlanList *list = new VlanList();
   std::pair<VlanList*, int> context(list, 0);  // Context for vlanTrunkPortVlansEnabled*

   // vtpVlanName
   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.9.9.46.1.3.1.1.4"), HandlerVlanList, list) != SNMP_ERR_SUCCESS)
      goto failure;

   // vmVlanType
   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.9.9.68.1.2.2.1.1"), HandlerVlanPorts, list) != SNMP_ERR_SUCCESS)
      goto failure;

   // vlanTrunkPortVlansEnabled
   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.9.9.46.1.6.1.1.4"), HandlerVlanTrunkPorts, &context) != SNMP_ERR_SUCCESS)
      goto failure;

   // vlanTrunkPortVlansEnabled2k
   context.second = 1024;
   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.9.9.46.1.6.1.1.17"), HandlerVlanTrunkPorts, &context) != SNMP_ERR_SUCCESS)
      goto failure;

   // vlanTrunkPortVlansEnabled3k
   context.second = 2048;
   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.9.9.46.1.6.1.1.18"), HandlerVlanTrunkPorts, &context) != SNMP_ERR_SUCCESS)
      goto failure;

   // vlanTrunkPortVlansEnabled4k
   context.second = 3072;
   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.9.9.46.1.6.1.1.19"), HandlerVlanTrunkPorts, &context) != SNMP_ERR_SUCCESS)
      goto failure;

   return list;

failure:
   delete list;
   return nullptr;
}
