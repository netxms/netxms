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
            ForwardingDatabaseEntry *entry = fdb->addPlaceholder();
            memset(entry, 0, sizeof(ForwardingDatabaseEntry));
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

         char context[128];
         if (snmp->getSnmpVersion() < SNMP_VERSION_3)
         {
            sprintf(context, "%s@%u", savedSecurityContext->getCommunity(), vlanId);
            snmp->setSecurityContext(new SNMP_SecurityContext(context));
         }
         else
         {
            sprintf(context, "vlan-%u", vlanId);
            SNMP_SecurityContext *securityContext = new SNMP_SecurityContext(savedSecurityContext);
            securityContext->setContextName(context);
            snmp->setSecurityContext(securityContext);
         }

         if (SnmpWalk(snmp, { 1, 3, 6, 1, 2, 1, 17, 4, 3, 1, 1 },
            [snmp, vlanId, fdb] (SNMP_Variable *var) -> uint32_t
            {
               return FDBHandler(var, snmp, vlanId, fdb);
            }) == SNMP_ERR_SUCCESS)
         {
            nxlog_debug_tag(DEBUG_TAG_TOPO_FDB, 5, _T("CiscoDeviceDriver::getForwardingDatabase(%s [%u]): %d entries read from dot1dTpFdbTable in context %hs"), node->getName(), node->getId(), fdb->size() - size, context);
         }
         else
         {
            // Some Cisco switches may not return data for certain system VLANs
            nxlog_debug_tag(DEBUG_TAG_TOPO_FDB, 5, _T("CiscoDeviceDriver::getForwardingDatabase(%s [%u]): cannot read FDB in context %hs"), node->getName(), node->getId(), context);
         }

         size = fdb->size();
      }
      delete vlans;
      snmp->setSecurityContext(savedSecurityContext);
   }

   return fdb;
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

   // Error response detection (used by configuration restore)
   hints->errorPattern = "^%\\s*(Invalid input|Incomplete command|Ambiguous command|Unrecognized command|Error|Access denied|Bad )";

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
 * Check if config restore is supported
 */
bool CiscoDeviceDriver::isConfigRestoreSupported()
{
   return true;
}

/**
 * Check if given configuration line opens a multi-line banner block. On match extracts banner
 * delimiter (single character, or two-character "^X" representation of a control character)
 * into provided buffer (at least 3 characters long).
 */
static bool IsBannerBlockStart(const wchar_t *line, wchar_t *delimiter)
{
   if (wcsncmp(line, L"banner ", 7) != 0)
      return false;

   // Skip banner type (motd, login, exec, etc.) and following spaces
   const wchar_t *p = line + 7;
   while(*p == L' ')
      p++;
   while((*p != 0) && (*p != L' '))
      p++;
   while(*p == L' ')
      p++;
   if (*p == 0)
      return false;

   size_t delimLen = ((*p == L'^') && (*(p + 1) != 0) && (*(p + 1) != L' ')) ? 2 : 1;
   memcpy(delimiter, p, delimLen * sizeof(wchar_t));
   delimiter[delimLen] = 0;

   // If the delimiter occurs again on the same line, the banner is single-line
   return wcsstr(p + delimLen, delimiter) == nullptr;
}

/**
 * Convert configuration text into list of command units for line-by-line replay. Drops empty
 * lines, comment lines, and "show running-config" preamble remnants. Multi-line banner
 * definitions are grouped into single units (device does not print prompts for banner content).
 */
static void PrepareCiscoConfigCommands(const ByteStream& config, StringList *commands)
{
   StringBuffer text(reinterpret_cast<const char*>(config.buffer()), static_cast<ssize_t>(config.size()), "UTF-8");
   text.replace(L"\r", L"");
   StringList lines = text.split(L"\n", false);
   for(int i = 0; i < lines.size(); i++)
   {
      StringBuffer line(lines.get(i));
      line.trim();

      if (line.isEmpty() || (line.charAt(0) == L'!'))
         continue;

      if (!wcsncmp(line.cstr(), L"Building configuration", 22) || !wcsncmp(line.cstr(), L"Current configuration", 21))
         continue;

      wchar_t delimiter[3];
      if (IsBannerBlockStart(line.cstr(), delimiter))
      {
         // Accumulate banner content verbatim until closing delimiter
         StringBuffer unit(line);
         while(++i < lines.size())
         {
            const wchar_t *contentLine = lines.get(i);
            unit.append(L'\n');
            unit.append(contentLine);
            if (wcsstr(contentLine, delimiter) != nullptr)
               break;
         }
         commands->add(unit.cstr());
         continue;
      }

      commands->add(line.cstr());
   }
}

/**
 * Restore configuration via interactive SSH (merge semantics)
 */
bool CiscoDeviceDriver::restoreConfig(DeviceBackupContext *ctx, const ByteStream& config, StringBuffer *errorLog,
      const std::function<void (int, int)>& progressCallback)
{
   SSHInteractiveChannel *ssh = ctx->getInteractiveSSH();
   if (ssh == nullptr)
   {
      errorLog->append(L"Cannot open interactive SSH channel");
      return false;
   }
   if (!ssh->isPrivileged())
   {
      errorLog->append(L"Privileged mode is required for configuration restore");
      return false;
   }

   StringList commands;
   PrepareCiscoConfigCommands(config, &commands);
   if (commands.isEmpty())
   {
      errorLog->append(L"Configuration is empty");
      return false;
   }

   StringList *output = ssh->executeCommand("configure terminal");
   if (output == nullptr)
   {
      errorLog->appendFormattedString(L"Cannot enter configuration mode: %s", ssh->getLastErrorMessage());
      return false;
   }
   delete output;

   bool success = ssh->feedConfigurationLines(commands, errorLog, progressCallback);

   // Always leave configuration mode, even after failure
   delete ssh->executeCommand("end");

   if (!success)
      return false;   // never save partially applied configuration

   output = ssh->executeCommand("write memory", 60000);
   if (output == nullptr)
   {
      errorLog->appendFormattedString(L"Failed to save configuration: %s", ssh->getLastErrorMessage());
      return false;
   }
   delete output;
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
