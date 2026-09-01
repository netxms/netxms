/* 
** NetXMS - Network Management System
** Generic driver for Cisco devices
** Copyright (C) 2003-2024 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: cisco.cpp
**
**/

#include "cisco.h"
#include <netxms-version.h>

/**
 * Get driver version
 */
const TCHAR *CiscoDeviceDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
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
bool CiscoDeviceDriver::getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   // Cisco devices often leave entPhysicalMfgName empty on chassis and stack entities,
   // so vendor name cannot be obtained from ENTITY-MIB. Other fields are left empty
   // intentionally to be filled by server from ENTITY-MIB.
   wcscpy(hwInfo->vendor, L"Cisco Systems Inc.");
   return true;
}

/**
 * Determine maximum speed based on interface name
 */
static uint64_t MaxSpeedFromInterfaceName(const TCHAR *ifName)
{
   StringBuffer name(ifName);
   name.toUppercase();

   // Ethernet interfaces
   if (name.startsWith(_T("FA")))
      return _ULL(100000000); // 100 Mbps

   if (name.startsWith(_T("GI")))
      return _ULL(1000000000); // 1 Gbps

   if (name.startsWith(_T("TE")))
      return _ULL(10000000000); // 10 Gbps

   if (name.startsWith(_T("TWE")))
      return _ULL(25000000000); // 25 Gbps

   if (name.startsWith(_T("FO")))
      return _ULL(40000000000); // 40 Gbps

   if (name.startsWith(_T("HU")))
      return _ULL(100000000000); // 100 Gbps

   // Serial interfaces (various speeds)
   if (name.startsWith(_T("SE")))
      return _ULL(2048000); // Default T1 speed

   // ATM interfaces
   if (name.startsWith(_T("ATM")))
      return _ULL(155000000); // Default OC-3 speed

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
InterfaceList *CiscoDeviceDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
   // Get interface list from standard MIB
   InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, useIfXTable);
   if (ifList == nullptr)
      return nullptr;

   // Set maximum speed based on interface name
   for(int i = 0; i < ifList->size(); i++)
      ifList->get(i)->maxSpeed = MaxSpeedFromInterfaceName(ifList->get(i)->name);

   return ifList;
}

/**
 * Parse VLAN membership bit map
 *
 * @param vlanList VLAN list
 * @param ifIndex interface index for current interface
 * @param map VLAN membership map
 * @param offset VLAN ID offset from 0
 */
static void ParseVlanMap(VlanList *vlanList, UINT32 ifIndex, BYTE *map, int offset)
{
	// VLAN map description from Cisco MIB:
	// ======================================
	// A string of octets containing one bit per VLAN in the
	// management domain on this trunk port.  The first octet
	// corresponds to VLANs with VlanIndex values of 0 through 7;
	// the second octet to VLANs 8 through 15; etc.  The most
	// significant bit of each octet corresponds to the lowest
	// value VlanIndex in that octet.  If the bit corresponding to
	// a VLAN is set to '1', then the local system is enabled for
	// sending and receiving frames on that VLAN; if the bit is set
	// to '0', then the system is disabled from sending and
	// receiving frames on that VLAN.

	int vlanId = offset;
	for(int i = 0; i < 128; i++)
	{
		BYTE mask = 0x80;
		while(mask > 0)
		{
			if (map[i] & mask)
			{
				vlanList->addMemberPort(vlanId, ifIndex);
			}
			mask >>= 1;
			vlanId++;
		}
	}
}

/**
 * Handler for trunk port enumeration on Cisco device
 */
static uint32_t HandlerTrunkPorts(SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
   VlanList *vlanList = (VlanList *)arg;
   size_t nameLen = var->getName().length();
   uint32_t ifIndex = var->getName().getElement(nameLen - 1);

   // Check if port is acting as trunk
   uint32_t oidName[256], value;
   memcpy(oidName, var->getName().value(), nameLen * sizeof(UINT32));
   oidName[nameLen - 2] = 14;	// .1.3.6.1.4.1.9.9.46.1.6.1.1.14
	if (SnmpGetEx(transport, NULL, oidName, nameLen, &value, sizeof(UINT32), 0, NULL) != SNMP_ERR_SUCCESS)
	   return SNMP_ERR_SUCCESS;	// Cannot get trunk state, ignore port
	if (value != 1)
	   return SNMP_ERR_SUCCESS;	// Not a trunk port, ignore

	// Native VLAN
	int vlanId = var->getValueAsInt();
	if (vlanId != 0)
		vlanList->addMemberPort(vlanId, ifIndex, true);

	// VLAN map for VLAN IDs 0..1023
   oidName[nameLen - 2] = 4;	// .1.3.6.1.4.1.9.9.46.1.6.1.1.4
	BYTE map[128];
	memset(map, 0, 128);
	if (SnmpGetEx(transport, NULL, oidName, nameLen, map, 128, SG_RAW_RESULT, NULL) == SNMP_ERR_SUCCESS)
		ParseVlanMap(vlanList, ifIndex, map, 0);

	// VLAN map for VLAN IDs 1024..2047
   oidName[nameLen - 2] = 17;	// .1.3.6.1.4.1.9.9.46.1.6.1.1.17
	memset(map, 0, 128);
	if (SnmpGetEx(transport, NULL, oidName, nameLen, map, 128, SG_RAW_RESULT, NULL) == SNMP_ERR_SUCCESS)
		ParseVlanMap(vlanList, ifIndex, map, 1024);

	// VLAN map for VLAN IDs 2048..3071
   oidName[nameLen - 2] = 18;	// .1.3.6.1.4.1.9.9.46.1.6.1.1.18
	memset(map, 0, 128);
	if (SnmpGetEx(transport, NULL, oidName, nameLen, map, 128, SG_RAW_RESULT, NULL) == SNMP_ERR_SUCCESS)
		ParseVlanMap(vlanList, ifIndex, map, 2048);

	// VLAN map for VLAN IDs 3072..4095
   oidName[nameLen - 2] = 19;	// .1.3.6.1.4.1.9.9.46.1.6.1.1.19
	memset(map, 0, 128);
	if (SnmpGetEx(transport, NULL, oidName, nameLen, map, 128, SG_RAW_RESULT, NULL) == SNMP_ERR_SUCCESS)
		ParseVlanMap(vlanList, ifIndex, map, 3072);

   return SNMP_ERR_SUCCESS;
}

/**
 * Handler for access port enumeration on Cisco device
 */
static uint32_t HandlerAccessPorts(SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
   VlanList *vlanList = (VlanList *)arg;
   size_t nameLen = var->getName().length();
   uint32_t ifIndex = var->getName().getElement(nameLen - 1);

   uint32_t oidName[256];
   memcpy(oidName, var->getName().value(), nameLen * sizeof(UINT32));

	// Entry type: 3=multi-vlan
	if (var->getValueAsInt() == 3)
	{
		BYTE map[128];

		oidName[nameLen - 2] = 4;	// .1.3.6.1.4.1.9.9.68.1.2.2.1.4
		memset(map, 0, 128);
		if (SnmpGetEx(transport, nullptr, oidName, nameLen, map, 128, SG_RAW_RESULT, nullptr) == SNMP_ERR_SUCCESS)
			ParseVlanMap(vlanList, ifIndex, map, 0);

		// VLAN map for VLAN IDs 1024..2047
		oidName[nameLen - 2] = 5;	// .1.3.6.1.4.1.9.9.68.1.2.2.1.5
		memset(map, 0, 128);
		if (SnmpGetEx(transport, NULL, oidName, nameLen, map, 128, SG_RAW_RESULT, NULL) == SNMP_ERR_SUCCESS)
			ParseVlanMap(vlanList, ifIndex, map, 1024);

		// VLAN map for VLAN IDs 2048..3071
		oidName[nameLen - 2] = 6;	// .1.3.6.1.4.1.9.9.68.1.2.2.1.6
		memset(map, 0, 128);
		if (SnmpGetEx(transport, NULL, oidName, nameLen, map, 128, SG_RAW_RESULT, NULL) == SNMP_ERR_SUCCESS)
			ParseVlanMap(vlanList, ifIndex, map, 2048);

		// VLAN map for VLAN IDs 3072..4095
		oidName[nameLen - 2] = 7;	// .1.3.6.1.4.1.9.9.68.1.2.2.1.7
		memset(map, 0, 128);
		if (SnmpGetEx(transport, NULL, oidName, nameLen, map, 128, SG_RAW_RESULT, NULL) == SNMP_ERR_SUCCESS)
			ParseVlanMap(vlanList, ifIndex, map, 3072);
	}
	else
	{
		// Port is in just one VLAN, it's ID must be in vmVlan
	   oidName[nameLen - 2] = 2;	// .1.3.6.1.4.1.9.9.68.1.2.2.1.2
		uint32_t vlanId = 0;
		if (SnmpGetEx(transport, nullptr, oidName, nameLen, &vlanId, sizeof(uint32_t), 0, nullptr) == SNMP_ERR_SUCCESS)
		{
			if (vlanId != 0)
				vlanList->addMemberPort((int)vlanId, ifIndex, false);
		}
	}

	return SNMP_ERR_SUCCESS;
}

/**
 * Get VLANs 
 */
VlanList *CiscoDeviceDriver::getVlans(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
	auto vlanList = new VlanList();

	// Vlan list
	if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.9.9.46.1.3.1.1.4"),
	      [vlanList] (SNMP_Variable *var) -> uint32_t
	      {
	         TCHAR buffer[256];
	         VlanInfo *vlan = new VlanInfo(var->getName().getLastElement(), VLAN_PRM_IFINDEX, var->getValueAsString(buffer, 256));
	         vlanList->add(vlan);
	         return SNMP_ERR_SUCCESS;
	      }
	   ) != SNMP_ERR_SUCCESS)
		goto failure;

	// Trunk ports
	if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.9.9.46.1.6.1.1.5"), HandlerTrunkPorts, vlanList) != SNMP_ERR_SUCCESS)
		goto failure;

	// Access ports
	if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.9.9.68.1.2.2.1.1"), HandlerAccessPorts, vlanList) != SNMP_ERR_SUCCESS)
		goto failure;

	return vlanList;

failure:
	delete vlanList;
	return nullptr;
}

/**
 * FDB walker's callback
 */
static uint32_t FDBHandler(SNMP_Variable *var, SNMP_Transport *snmp, uint16_t vlanId, StructArray<ForwardingDatabaseEntry> *fdb)
{
   SNMP_ObjectId oid(var->getName());

   // Get port number and status
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());

   oid.changeElement(10, 2);  // 1.3.6.1.2.1.17.4.3.1.2 - port number
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(10, 3);  // 1.3.6.1.2.1.17.4.3.1.3 - status
   request.bindVariable(new SNMP_Variable(oid));

   SNMP_PDU *response;
   uint32_t rcc = snmp->doRequest(&request, &response);
   if (rcc == SNMP_ERR_SUCCESS)
   {
      SNMP_Variable *varPort = response->getVariable(0);
      SNMP_Variable *varStatus = response->getVariable(1);
      if (varPort != nullptr && varStatus != nullptr)
      {
         uint32_t port = varPort->getValueAsUInt();
         int status = varStatus->getValueAsInt();
         if ((port > 0) && ((status == 3) || (status == 5) || (status == 6)))  // status: 3 == learned, 5 == static, 6 == secure (possibly H3C specific)
         {
            ForwardingDatabaseEntry *entry = new(fdb->addPlaceholder()) ForwardingDatabaseEntry();
            entry->bridgePort = port;
            entry->macAddr = var->getValueAsMACAddr();
            entry->vlanId = vlanId;
            entry->type = static_cast<uint16_t>(status);
         }
      }
      delete response;
   }

   return rcc;
}

/**
 * Set per-VLAN SNMP security context (community@vlan for SNMP v1/v2c, context name "vlan-N" for SNMP v3)
 */
static void SetVlanSecurityContext(SNMP_Transport *snmp, const SNMP_SecurityContext *baseSecurityContext, uint16_t vlanId)
{
   if (snmp->getSnmpVersion() < SNMP_VERSION_3)
   {
      char community[256];
      snprintf(community, sizeof(community), "%s@%u", baseSecurityContext->getCommunity(), vlanId);
      snmp->setSecurityContext(new SNMP_SecurityContext(community));
   }
   else
   {
      char contextName[32];
      snprintf(contextName, sizeof(contextName), "vlan-%u", vlanId);
      SNMP_SecurityContext *securityContext = new SNMP_SecurityContext(baseSecurityContext);
      securityContext->setContextName(contextName);
      snmp->setSecurityContext(securityContext);
   }
}

/**
 * Get switch forwarding database.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return switch forwarding database or NULL on failure
 */
StructArray<ForwardingDatabaseEntry> *CiscoDeviceDriver::getForwardingDatabase(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   StructArray<ForwardingDatabaseEntry> *fdb = NetworkDeviceDriver::getForwardingDatabase(snmp, node, driverData);
   if (fdb == nullptr)
      return nullptr;

   int size = fdb->size();
   VlanList *vlans = getVlans(snmp, node, driverData);
   if (vlans != nullptr)
   {
      SNMP_SecurityContext *savedSecurityContext = new SNMP_SecurityContext(snmp->getSecurityContext());
      for(int i = 0; i < vlans->size(); i++)
      {
         uint16_t vlanId = vlans->get(i)->getVlanId();
         SetVlanSecurityContext(snmp, savedSecurityContext, vlanId);

         if (SnmpWalk(snmp, { 1, 3, 6, 1, 2, 1, 17, 4, 3, 1, 1 },
            [snmp, vlanId, fdb] (SNMP_Variable *var) -> uint32_t
            {
               return FDBHandler(var, snmp, vlanId, fdb);
            }) == SNMP_ERR_SUCCESS)
         {
            nxlog_debug_tag(DEBUG_TAG_TOPO_FDB, 5, _T("CiscoDeviceDriver::getForwardingDatabase(%s [%u]): %d entries read from dot1dTpFdbTable in VLAN %u"), node->getName(), node->getId(), fdb->size() - size, vlanId);
         }
         else
         {
            // Some Cisco switches may not return data for certain system VLANs
            nxlog_debug_tag(DEBUG_TAG_TOPO_FDB, 5, _T("CiscoDeviceDriver::getForwardingDatabase(%s [%u]): cannot read FDB in VLAN %u"), node->getName(), node->getId(), vlanId);
         }

         // Resolve bridge port numbers to interface indexes while still in this VLAN's SNMP context,
         // where they are unambiguous - bridge port numbers are only guaranteed meaningful within one
         // VLAN's bridge. Entries with interface index already set will not be re-resolved by server
         // core from VLAN-agnostic bridge port mapping. Check the whole array, not only entries just
         // added, because entries for this VLAN may already be present from dot1qTpFdbTable walk done
         // by base class implementation (and duplicates are later removed keeping first occurrence).
         bool resolutionNeeded = false;
         for(int j = 0; j < fdb->size(); j++)
         {
            ForwardingDatabaseEntry *e = fdb->get(j);
            if ((e->vlanId == vlanId) && (e->ifIndex == 0) && (e->bridgePort != 0))
            {
               resolutionNeeded = true;
               break;
            }
         }
         if (resolutionNeeded)
         {
            StructArray<BridgePort> vlanBridgePorts(0, 64);
            if (SnmpWalk(snmp, { 1, 3, 6, 1, 2, 1, 17, 1, 4, 1, 2 },
               [&vlanBridgePorts] (SNMP_Variable *var) -> uint32_t
               {
                  BridgePort *p = vlanBridgePorts.addPlaceholder();
                  p->portNumber = var->getName().getElement(11);
                  p->ifIndex = var->getValueAsUInt();
                  return SNMP_ERR_SUCCESS;
               }) == SNMP_ERR_SUCCESS)
            {
               int resolved = 0, candidates = 0;
               for(int j = 0; j < fdb->size(); j++)
               {
                  ForwardingDatabaseEntry *e = fdb->get(j);
                  if ((e->vlanId != vlanId) || (e->ifIndex != 0))
                     continue;
                  candidates++;
                  for(int k = 0; k < vlanBridgePorts.size(); k++)
                  {
                     BridgePort *p = vlanBridgePorts.get(k);
                     if (p->portNumber == e->bridgePort)
                     {
                        e->ifIndex = p->ifIndex;
                        resolved++;
                        break;
                     }
                  }
               }
               nxlog_debug_tag(DEBUG_TAG_TOPO_FDB, 5, _T("CiscoDeviceDriver::getForwardingDatabase(%s [%u]): %d of %d entries resolved to interface index in VLAN %u"),
                  node->getName(), node->getId(), resolved, candidates, vlanId);
            }
            else
            {
               // Leave interface indexes at 0 and let server core attempt its own resolution from bridge port mapping
               nxlog_debug_tag(DEBUG_TAG_TOPO_FDB, 5, _T("CiscoDeviceDriver::getForwardingDatabase(%s [%u]): cannot read dot1dBasePortTable in VLAN %u, leaving interface indexes unresolved"),
                  node->getName(), node->getId(), vlanId);
            }
         }

         size = fdb->size();
      }
      delete vlans;
      snmp->setSecurityContext(savedSecurityContext);
   }

   return fdb;
}

/**
 * Get bridge port to interface mapping. Cisco switches expose BRIDGE-MIB per VLAN, using community
 * string indexing for SNMP v1/v2c and context name for SNMP v3, so bridge port table has to be read
 * in the same per-VLAN contexts as forwarding database.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return bridge port to interface mapping or NULL on failure
 */
StructArray<BridgePort> *CiscoDeviceDriver::getBridgePorts(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   StructArray<BridgePort> *bridgePorts = NetworkDeviceDriver::getBridgePorts(snmp, node, driverData);
   if (bridgePorts == nullptr)
      bridgePorts = new StructArray<BridgePort>(0, 64);

   VlanList *vlans = getVlans(snmp, node, driverData);
   if (vlans != nullptr)
   {
      int size = bridgePorts->size();
      SNMP_SecurityContext *savedSecurityContext = new SNMP_SecurityContext(snmp->getSecurityContext());
      for(int i = 0; i < vlans->size(); i++)
      {
         uint16_t vlanId = vlans->get(i)->getVlanId();
         SetVlanSecurityContext(snmp, savedSecurityContext, vlanId);

         if (SnmpWalk(snmp, { 1, 3, 6, 1, 2, 1, 17, 1, 4, 1, 2 },
            [bridgePorts, vlanId, node] (SNMP_Variable *var) -> uint32_t
            {
               uint32_t portNumber = var->getName().getElement(11);
               uint32_t ifIndex = var->getValueAsUInt();
               for(int j = 0; j < bridgePorts->size(); j++)
               {
                  BridgePort *p = bridgePorts->get(j);
                  if (p->ifIndex == ifIndex)
                  {
                     // Keep mapping already collected (mappings read from default context are collected
                     // first and should not be replaced by per-VLAN ones). Mapping has to stay injective
                     // in both directions: Node::getInterfaceList() applies these by interface index and
                     // takes the last match.
                     if (p->portNumber != portNumber)
                     {
                        nxlog_debug_tag(DEBUG_TAG_TOPO_FDB, 4, _T("CiscoDeviceDriver::getBridgePorts(%s [%u]): bridge port %u in VLAN %u maps to ifIndex %u which is already mapped from bridge port %u"),
                           node->getName(), node->getId(), portNumber, vlanId, ifIndex, p->portNumber);
                     }
                     return SNMP_ERR_SUCCESS;
                  }
                  if (p->portNumber == portNumber)
                  {
                     // A conflict means bridge port numbering on this device is VLAN-scoped rather than
                     // global, and cannot be fully represented by VLAN-agnostic mapping returned from
                     // this method. FDB resolution is still correct in that case because interface
                     // indexes are resolved within each VLAN context in getForwardingDatabase().
                     if (p->ifIndex != ifIndex)
                     {
                        nxlog_debug_tag(DEBUG_TAG_TOPO_FDB, 4, _T("CiscoDeviceDriver::getBridgePorts(%s [%u]): conflicting mapping for bridge port %u in VLAN %u (ifIndex %u, already mapped to ifIndex %u)"),
                           node->getName(), node->getId(), portNumber, vlanId, ifIndex, p->ifIndex);
                     }
                     return SNMP_ERR_SUCCESS;
                  }
               }
               BridgePort *p = bridgePorts->addPlaceholder();
               p->portNumber = portNumber;
               p->ifIndex = ifIndex;
               return SNMP_ERR_SUCCESS;
            }) == SNMP_ERR_SUCCESS)
         {
            nxlog_debug_tag(DEBUG_TAG_TOPO_FDB, 5, _T("CiscoDeviceDriver::getBridgePorts(%s [%u]): %d mappings read from dot1dBasePortTable in VLAN %u"), node->getName(), node->getId(), bridgePorts->size() - size, vlanId);
         }
         else
         {
            // Some Cisco switches may not return data for certain system VLANs
            nxlog_debug_tag(DEBUG_TAG_TOPO_FDB, 5, _T("CiscoDeviceDriver::getBridgePorts(%s [%u]): cannot read dot1dBasePortTable in VLAN %u"), node->getName(), node->getId(), vlanId);
         }

         size = bridgePorts->size();
      }
      delete vlans;
      snmp->setSecurityContext(savedSecurityContext);
   }

   return bridgePorts;
}

/**
 * Get SSH driver hints for Cisco IOS/IOS-XE devices
 */
void CiscoDeviceDriver::getSSHDriverHints(SSHDriverHints *hints) const
{
   // Cisco IOS prompt patterns:
   // - User mode: hostname>
   // - Privileged mode: hostname#
   // - Config mode: hostname(config)#, hostname(config-if)#, etc.
   // Prompt may include domain name: hostname.domain.com>
   hints->promptPattern = "^[\\w.-]+(\\([\\w-]+\\))?[>#]\\s*$";
   hints->enabledPromptPattern = "^[\\w.-]+(\\([\\w-]+\\))?#\\s*$";

   // Enable command and password prompt
   hints->enableCommand = "enable";
   hints->enablePromptPattern = "[Pp]assword:\\s*$";

   // Pagination control
   hints->paginationDisableCmd = "terminal length 0";
   hints->paginationPrompt = " --[Mm]ore-- |<--- More --->|--More--";
   hints->paginationContinue = " ";

   // Exit command
   hints->exitCommand = "exit";

   // Test command for verifying command mode support
   hints->testCommand = "show version | include Cisco";
   hints->testCommandPattern = "Cisco";

   // Timeouts (Cisco devices are generally responsive)
   hints->commandTimeout = 30000;
   hints->connectTimeout = 15000;
}

/**
 * Check if config backup is supported
 */
bool CiscoDeviceDriver::isConfigBackupSupported()
{
   return true;
}

/**
 * Get running configuration via interactive SSH
 */
bool CiscoDeviceDriver::getRunningConfig(DeviceBackupContext *ctx, ByteStream *output)
{
   SSHInteractiveChannel *ssh = ctx->getInteractiveSSH();
   if (ssh == nullptr)
      return false;
   if (!ssh->executeCommand("show running-config", output))
      return false;
   StripCiscoConfigPreamble(output);
   return true;
}

/**
 * Get startup configuration via interactive SSH
 */
bool CiscoDeviceDriver::getStartupConfig(DeviceBackupContext *ctx, ByteStream *output)
{
   SSHInteractiveChannel *ssh = ctx->getInteractiveSSH();
   if (ssh == nullptr)
      return false;
   if (!ssh->executeCommand("show startup-config", output))
      return false;
   StripCiscoConfigPreamble(output);
   return true;
}

/**
 * Driver module entry point
 */
NDD_BEGIN_DRIVER_LIST
NDD_DRIVER(CatalystDriver)
NDD_DRIVER(Cat2900Driver)
NDD_DRIVER(CiscoEswDriver)
NDD_DRIVER(CiscoFirepowerDriver)
NDD_DRIVER(CiscoNexusDriver)
NDD_DRIVER(CiscoSbDriver)
NDD_DRIVER(CiscoWirelessControllerDriver)
NDD_DRIVER(CiscoC9800Driver)
NDD_DRIVER(GenericCiscoDriver)
NDD_END_DRIVER_LIST
DECLARE_NDD_MODULE_ENTRY_POINT

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
