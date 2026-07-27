/* 
** NetXMS - Network Management System
** Driver for Juniper Networks switches
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
** File: juniper.cpp
**/

#include "juniper.h"
#include <netxms-version.h>
#include <nxnetconf.h>

/**
 * Get driver name
 */
const TCHAR *JuniperDriver::getName()
{
   return _T("JUNIPER");
}

/**
 * Get driver version
 */
const TCHAR *JuniperDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int JuniperDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return oid.startsWith({ 1, 3, 6, 1, 4, 1, 2636, 1 }) ? 255 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param context device context
 * @param oid Device OID
 */
bool JuniperDriver::isDeviceSupported(DeviceContext *context, const SNMP_ObjectId& oid)
{
	return true;
}

/**
 * Get hardware information from device.
 *
 * @param context device context
 * @param node Node
 * @param driverData driver data
 * @param hwInfo pointer to hardware information structure to fill
 * @return true if hardware information is available
 */
bool JuniperDriver::getHardwareInformation(DeviceContext *context, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   SNMP_Transport *snmp = context->getSNMPTransport();
   _tcscpy(hwInfo->vendor, _T("Juniper Networks"));

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 2636, 3, 1, 2, 0 }));  // Product name
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 2636, 3, 1, 3, 0 }));  // Serial number
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 2636, 3, 1, 4, 0 }));  // Revision

   // Return success only if at least product name is available.
   // Driver should return false if product name cannot be retrieved to allow server to try ENTITY MIB.
   bool success = false;
   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      TCHAR buffer[256];

      const SNMP_Variable *v = response->getVariable(0);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->productName, v->getValueAsString(buffer, 256), 128);
         success = true;
      }

      v = response->getVariable(1);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         wcslcpy(hwInfo->serialNumber, v->getValueAsString(buffer, 256), sizeof(hwInfo->serialNumber) / sizeof(wchar_t));
      }

      v = response->getVariable(2);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->productVersion, v->getValueAsString(buffer, 256), 16);
      }

      delete response;
   }

   return success;
}

/**
 * Get list of interfaces for given node
 *
 * @param context device context
 * @param node Node
 */
InterfaceList *JuniperDriver::getInterfaces(DeviceContext *context, NObject *node, DriverData *driverData, bool useIfXTable)
{
   SNMP_Transport *snmp = context->getSNMPTransport();
	// Get interface list from standard MIB
	InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(context, node, driverData, useIfXTable);
	if (ifList == nullptr)
	{
	   nxlog_debug_tag(JUNIPER_DEBUG_TAG, 5, _T("getInterfaces: call to NetworkDeviceDriver::getInterfaces failed"));
		return nullptr;
	}

	// Mark internal service interfaces as excluded from topology
	for(int i = 0; i < ifList->size(); i++)
	{
	   InterfaceInfo *iface = ifList->get(i);
	   if (!_tcsnicmp(iface->name, _T("bme"), 3) || !_tcsnicmp(iface->name, _T("jsrv"), 4) || !_tcsnicmp(iface->name, _T("esi"), 3))
	   {
	      iface->excludeFromTopology = true;
	      nxlog_debug_tag(JUNIPER_DEBUG_TAG, 5, _T("getInterfaces: marking internal service interface \"%s\" as excluded from topology"), iface->name);
	   }
	}

	// Update physical port locations
	SNMP_Snapshot *chassisTable = SNMP_Snapshot::create(snmp, _T(".1.3.6.1.4.1.2636.3.3.2.1"));
	if (chassisTable != nullptr)
	{
      nxlog_debug_tag(JUNIPER_DEBUG_TAG, 5, _T("getInterfaces: cannot create snapshot of chassis table"));

	   // Update slot/port for physical interfaces
      for(int i = 0; i < ifList->size(); i++)
      {
         InterfaceInfo *iface = ifList->get(i);
         if (iface->type != IFTYPE_ETHERNET_CSMACD)
            continue;

         SNMP_ObjectId oid { 1, 3, 6, 1, 4, 1, 2636, 3, 3, 2, 1, 1, 0 };
         oid.changeElement(oid.length() - 1, iface->index);
         int slot = chassisTable->getAsInt32(oid);

         oid.changeElement(oid.length() - 2, 2);
         int pic = chassisTable->getAsInt32(oid);

         oid.changeElement(oid.length() - 2, 3);
         int port = chassisTable->getAsInt32(oid);

         if ((slot == 0) || (pic == 0) || (port == 0))
            continue;

         iface->isPhysicalPort = true;
         iface->location.module = slot - 1;  // Juniper numbers slots from 0 but reports in SNMP as n + 1
         iface->location.pic = pic - 1;  // Juniper numbers PICs from 0 but reports in SNMP as n + 1
         iface->location.port = port - 1;  // Juniper numbers ports from 0 but reports in SNMP as n + 1
      }

      // Attach logical interfaces to physical
      for(int i = 0; i < ifList->size(); i++)
      {
         InterfaceInfo *iface = ifList->get(i);
         if (iface->type != IFTYPE_PROP_VIRTUAL)
            continue;

         SNMP_ObjectId oid { 1, 3, 6, 1, 4, 1, 2636, 3, 3, 2, 1, 1, 0 };
         oid.changeElement(oid.length() - 1, iface->index);
         int slot = chassisTable->getAsInt32(oid);

         oid.changeElement(oid.length() - 2, 2);
         int pic = chassisTable->getAsInt32(oid);

         oid.changeElement(oid.length() - 2, 3);
         int port = chassisTable->getAsInt32(oid);

         if ((slot == 0) || (pic == 0) || (port == 0))
            continue;

         InterfaceInfo *parent = ifList->findByPhysicalLocation(0, slot - 1, pic - 1, port - 1);
         if (parent != nullptr)
            iface->parentIndex = parent->index;
      }

      delete chassisTable;
	}
	return ifList;
}

/**
 * Get VLANs
 */
VlanList *JuniperDriver::getVlans(DeviceContext *context, NObject *node, DriverData *driverData)
{
   SNMP_Transport *snmp = context->getSNMPTransport();
   BYTE buffer[256];
   if (SnmpGetEx(snmp, { 1, 3, 6, 1, 4, 1, 2636, 3, 48, 1, 3, 1, 1, 2 }, buffer, 256, SG_GET_NEXT_REQUEST | SG_RAW_RESULT, nullptr) == SNMP_ERR_SUCCESS)
   {
      nxlog_debug_tag(JUNIPER_DEBUG_TAG, 5, _T("getVlans: device supports jnxL2aldVlanTable"));
      return getVlansDot1q(context, node, driverData);
   }

   SNMP_Snapshot *vlanTable = SNMP_Snapshot::create(snmp, { 1, 3, 6, 1, 4, 1, 2636, 3, 40, 1, 5, 1, 5, 1 });
   if (vlanTable == nullptr)
   {
      nxlog_debug_tag(JUNIPER_DEBUG_TAG, 5, _T("getVlans: cannot create snapshot of VLAN table, fallback to NetworkDeviceDriver::getVlans"));
      return NetworkDeviceDriver::getVlans(context, node, driverData);
   }

   SNMP_Snapshot *portTable = SNMP_Snapshot::create(snmp, { 1, 3, 6, 1, 4, 1, 2636, 3, 40, 1, 5, 1, 7, 1 });
   if (portTable == nullptr)
   {
      delete vlanTable;
      nxlog_debug_tag(JUNIPER_DEBUG_TAG, 5, _T("getVlans: cannot create snapshot of port table, fallback to NetworkDeviceDriver::getVlans"));
      return NetworkDeviceDriver::getVlans(context, node, driverData);
   }

   VlanList *vlans = new VlanList();
   SNMP_ObjectId oid { 1, 3, 6, 1, 4, 1, 2636, 3, 40, 1, 5, 1, 5, 1, 5 };  // jnxExVlanTag
   const SNMP_Variable *v;
   while((v = vlanTable->getNext(oid)) != nullptr)
   {
      VlanInfo *vlan = new VlanInfo(v->getValueAsInt(), VLAN_PRM_BPORT);
      vlans->add(vlan);

      oid = v->getName();
      oid.changeElement(oid.length() - 2, 2);   // jnxExVlanName
      const SNMP_Variable *name = vlanTable->get(oid);
      if (name != nullptr)
      {
         TCHAR buffer[256];
         vlan->setName(name->getValueAsString(buffer, 256));
      }

      // VLAN ports
      oid.changeElement(oid.length() - 2, 4);   // jnxExVlanPortGroupInstance
      uint32_t vlanIndex = vlanTable->getAsUInt32(oid);

      SNMP_ObjectId baseOid { 1, 3, 6, 1, 4, 1, 2636, 3, 40, 1, 5, 1, 7, 1, 5, 0 };
      baseOid.changeElement(baseOid.length() - 1, vlanIndex);
      const SNMP_Variable *p = nullptr;
      while((p = portTable->getNext((p != nullptr) ? p->getName() : baseOid)) != nullptr)
      {
         if (p->getName().compare(baseOid) != OID_LONGER)
            break;
         vlan->add(p->getName().getElement(p->getName().length() - 1));
      }

      oid = v->getName();
   }

   delete vlanTable;
   delete portTable;
   return vlans;
}

/**
 * Handler for VLAN egress port enumeration
 */
static void ProcessVlanPortRecord(SNMP_Variable *var, VlanList *vlanList, bool tagged)
{
   uint32_t vlanId = var->getName().getLastElement();
   VlanInfo *vlan = vlanList->findById(vlanId);
   if (vlan != nullptr)
   {
      TCHAR buffer[4096];
      var->getValueAsString(buffer, 4096);
      String::split(buffer, _T(","), true,
         [vlan, tagged] (const String& e)
         {
            uint32_t p = _tcstoul(e, nullptr, 10);
            vlan->add(p, tagged);
         });
   }
}

/**
 * Get VLANs from dot1qVlanStaticTable. Devices may report ports not in standard way (as bit mask), but as string with comma-separated numbers.
 * Example:
 *    1.3.6.1.2.1.17.7.1.4.3.1.2.104 [STRING] = 490,506,516,526
 */
VlanList *JuniperDriver::getVlansDot1q(DeviceContext *context, NObject *node, DriverData *driverData)
{
   SNMP_Transport *snmp = context->getSNMPTransport();
   // Check if non-standard format of VLAN membership entries is used
   bool standardFormat = false;
   SnmpWalk(snmp, { 1, 3, 6, 1, 2, 1, 17, 7, 1, 4, 3, 1, 2 },
      [&standardFormat] (SNMP_Variable *var) -> uint32_t
      {
         BYTE buffer[1024];
         size_t size = var->getRawValue(buffer, sizeof(buffer));
         for(size_t i = 0; i < size; i++)
         {
            BYTE b = buffer[i];
            if (!((b >= '0' && b <='9') || b == ','))
            {
               // Something except digit or comma
               standardFormat = true;
               return SNMP_ERR_ABORTED;
            }
         }
         return SNMP_ERR_SUCCESS;
      });
   if (standardFormat)
   {
      nxlog_debug_tag(JUNIPER_DEBUG_TAG, 5, _T("getVlansDot1q: dot1qVlanStaticTable seems to contain binary data, fallback to NetworkDeviceDriver::getVlans"));
      return NetworkDeviceDriver::getVlans(context, node, driverData);
   }

   VlanList *vlanList = new VlanList();

   // dot1qVlanStaticName
   if (SnmpWalk(snmp, { 1, 3, 6, 1, 2, 1, 17, 7, 1, 4, 3, 1, 1 },
         [vlanList] (SNMP_Variable *var) -> uint32_t
         {
            uint32_t tag = var->getName().getLastElement();

            TCHAR name[256];
            var->getValueAsString(name, 256);
            // VLAN name may be suffixed by +nnn, where nnn is VLAN tag
            TCHAR *p = _tcsrchr(name, '+');
            if (p != nullptr)
            {
               TCHAR *eptr;
               uint32_t n = _tcstoul(p + 1, &eptr, 10);
               if ((*eptr == 0) && (n == tag))
                  *p = 0;
            }

            VlanInfo *vlan = new VlanInfo(tag, VLAN_PRM_BPORT, name);
            vlanList->add(vlan);
            return SNMP_ERR_SUCCESS;
         }) != SNMP_ERR_SUCCESS)
      goto failure;

   // dot1qVlanStaticName
   if (SnmpWalk(snmp, { 1, 3, 6, 1, 2, 1, 17, 7, 1, 4, 3, 1, 2 },
         [vlanList] (SNMP_Variable *var) -> uint32_t
         {
            ProcessVlanPortRecord(var, vlanList, true);
            return SNMP_ERR_SUCCESS;
         }) != SNMP_ERR_SUCCESS)
      goto failure;

   // dot1qVlanStaticUntaggedPorts
   if (SnmpWalk(snmp, { 1, 3, 6, 1, 2, 1, 17, 7, 1, 4, 3, 1, 4 },
         [vlanList] (SNMP_Variable *var) -> uint32_t
         {
            ProcessVlanPortRecord(var, vlanList, false);
            return SNMP_ERR_SUCCESS;
         }) != SNMP_ERR_SUCCESS)
      goto failure;

   return vlanList;

failure:
   delete vlanList;
   return nullptr;
}

/**
 * Get orientation of the modules in the device
 *
 * @param context device context
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return module orientation
 */
int JuniperDriver::getModulesOrientation(DeviceContext *context, NObject *node, DriverData *driverData)
{
   return NDD_ORIENTATION_HORIZONTAL;
}

/**
 * Get port layout of given module
 * @param context device context
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @param module Module number (starting from 1)
 * @param layout Layout structure to fill
 */
void JuniperDriver::getModuleLayout(DeviceContext *context, NObject *node, DriverData *driverData, int module, NDD_MODULE_LAYOUT *layout)
{
   layout->numberingScheme = NDD_PN_UD_LR;
   layout->rows = 2;
}

/**
 * Returns true if lldpRemTable uses ifIndex instead of bridge port number for referencing interfaces.
 * Default implementation always return false;
 *
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return true if lldpRemTable uses ifIndex instead of bridge port number
 */
bool JuniperDriver::isLldpRemTableUsingIfIndex(const NObject *node, DriverData *driverData)
{
   return true;
}

/**
 * Get SSH driver hints for Juniper JunOS devices
 */
void JuniperDriver::getSSHDriverHints(SSHDriverHints *hints) const
{
   // Some Juniper devices (or firmware versions) may drop into Unix shell
   // instead of JunOS CLI after SSH login. Shell prompt examples:
   //   root@:RE:0%    (no hostname, dual-RE)
   //   root@switch:RE:0%
   //   root@switch%
   hints->promptPattern = "^[\\w.-]*@[\\w.-]*(:[\\w]+:\\d+)?%\\s*$";

   // JunOS CLI prompt patterns (operational and configuration mode):
   //   user@hostname>           (operational mode)
   //   user@hostname#           (configuration mode)
   //   user@hostname:instance>  (with routing instance)
   hints->enabledPromptPattern = "^[\\w.-]+@[\\w.-]+(:[\\w.-]+)?[>#]\\s*$";

   // Enter JunOS CLI from Unix shell; no-op if already in CLI
   // (escalatePrivilege detects m_privileged=true and skips)
   hints->enableCommand = "cli";
   hints->enablePromptPattern = nullptr;

   // Pagination control - use "set cli screen-length 0" in operational mode
   hints->paginationDisableCmd = "set cli screen-length 0";
   hints->paginationPrompt = "---\\([Mm]ore\\s*\\d*%?\\)---|---\\(more\\)---";
   hints->paginationContinue = " ";

   // Exit command
   hints->exitCommand = "exit";

   // Test command for verifying command mode support
   hints->testCommand = "show version | match JUNOS";
   hints->testCommandPattern = "JUNOS";

   // Timeouts (JunOS devices may be slower due to XML parsing)
   hints->commandTimeout = 60000;
   hints->connectTimeout = 20000;
}

/**
 * Check if configuration backup is supported
 */
bool JuniperDriver::isConfigBackupSupported()
{
   return true;
}

/**
 * XML writer for serializing configuration data into byte stream
 */
class XmlByteStreamWriter : public pugi::xml_writer
{
private:
   ByteStream& m_out;

public:
   XmlByteStreamWriter(ByteStream& out) : m_out(out) { }

   virtual void write(const void *data, size_t size) override
   {
      m_out.write(data, size);
   }
};

/**
 * Recursively remove junos: prefixed attributes from configuration tree. They carry
 * commit metadata (junos:commit-seconds, junos:commit-user, etc.) that changes with
 * every commit, and reference a namespace declared on the rpc-reply element which is
 * not part of the serialized subtree.
 */
static void RemoveJunosMetadataAttributes(pugi::xml_node node)
{
   pugi::xml_attribute attr = node.first_attribute();
   while (attr)
   {
      pugi::xml_attribute next = attr.next_attribute();
      if (!strncmp(attr.name(), "junos:", 6) || !strcmp(attr.name(), "xmlns:junos"))
         node.remove_attribute(attr);
      attr = next;
   }
   for(pugi::xml_node child = node.first_child(); child; child = child.next_sibling())
      RemoveJunosMetadataAttributes(child);
}

/**
 * Get running configuration via NETCONF in native XML format
 */
static bool GetRunningConfigViaNETCONF(DeviceContext *ctx, ByteStream *output)
{
   char *reply = ctx->executeNETCONFRequest("<get-config><source><running/></source></get-config>");
   if (reply == nullptr)
   {
      nxlog_debug_tag(JUNIPER_DEBUG_TAG, 5, _T("GetRunningConfigViaNETCONF: RPC execution failed"));
      return false;
   }

   NETCONF_Response response;
   bool success = response.parse(reply, strlen(reply)) && response.isSuccess();
   MemFree(reply);
   if (!success)
   {
      nxlog_debug_tag(JUNIPER_DEBUG_TAG, 5, _T("GetRunningConfigViaNETCONF: device returned error (%s)"), response.getErrorText().cstr());
      return false;
   }

   pugi::xml_node configuration = NETCONF_FindChildByLocalName(response.getData(), "configuration");
   if (!configuration)
   {
      nxlog_debug_tag(JUNIPER_DEBUG_TAG, 5, _T("GetRunningConfigViaNETCONF: no configuration element in device reply"));
      return false;
   }

   RemoveJunosMetadataAttributes(configuration);

   XmlByteStreamWriter writer(*output);
   configuration.print(writer, "    ", pugi::format_default);
   return output->size() > 0;
}

/**
 * Get running configuration from device
 */
bool JuniperDriver::getRunningConfig(DeviceContext *ctx, ByteStream *output)
{
   if (ctx->isNETCONFAvailable())
   {
      if (GetRunningConfigViaNETCONF(ctx, output))
         return true;
      nxlog_debug_tag(JUNIPER_DEBUG_TAG, 5, _T("getRunningConfig: NETCONF configuration read failed, falling back to CLI"));
   }

   SSHInteractiveChannel *ssh = ctx->getInteractiveSSH();
   if (ssh == nullptr)
      return false;
   return ssh->executeCommand("show configuration", output);
}

/**
 * Check if configuration restore is supported
 */
bool JuniperDriver::isConfigRestoreSupported()
{
   return true;
}

/**
 * Build load-configuration RPC content from saved configuration. Accepts both native
 * XML format (configuration element produced by NETCONF backup) and curly-brace text
 * format (produced by CLI backup). Resulting request is written as NUL-terminated
 * UTF-8 string.
 */
static bool BuildLoadConfigurationRequest(const ByteStream& config, ByteStream *request)
{
   const char *data = reinterpret_cast<const char*>(config.buffer());
   size_t size = config.size();
   size_t i = 0;
   while((i < size) && ((data[i] == ' ') || (data[i] == '\t') || (data[i] == '\r') || (data[i] == '\n')))
      i++;
   if (i >= size)
      return false;

   pugi::xml_document document;
   pugi::xml_node load = document.append_child("load-configuration");
   load.append_attribute("action") = "override";
   if (data[i] == '<')
   {
      load.append_attribute("format") = "xml";
      if (!load.append_buffer(data + i, size - i) || !NETCONF_FindChildByLocalName(load, "configuration"))
         return false;
   }
   else
   {
      load.append_attribute("format") = "text";
      char *text = MemAllocArrayNoInit<char>(size - i + 1);
      memcpy(text, data + i, size - i);
      text[size - i] = 0;
      load.append_child("configuration-text").text().set(text);
      MemFree(text);
   }

   XmlByteStreamWriter writer(*request);
   document.print(writer, "", pugi::format_raw | pugi::format_no_declaration);
   request->write('\0');
   return true;
}

/**
 * Restore configuration via NETCONF using candidate datastore with full override
 * semantics. Confirmed commit is used so that device rolls back automatically if the
 * confirming commit cannot be delivered (e.g. restored configuration cuts management
 * connectivity).
 */
bool JuniperDriver::restoreConfig(DeviceContext *ctx, const ByteStream& config, StringBuffer *errorLog,
      const std::function<void (int, int)>& progressCallback)
{
   if (!ctx->isNETCONFAvailable())
   {
      errorLog->append(L"NETCONF access is required for configuration restore");
      return false;
   }

   ByteStream loadRequest;
   if (!BuildLoadConfigurationRequest(config, &loadRequest))
   {
      errorLog->append(L"Cannot parse saved configuration");
      return false;
   }

   static const char *unlockRequest = "<unlock><target><candidate/></target></unlock>";
   static const wchar_t *stepNames[] = { L"lock candidate datastore", L"load configuration", L"commit configuration", L"confirm commit", L"unlock candidate datastore" };

   const char *requests[5];
   requests[0] = "<lock><target><candidate/></target></lock>";
   requests[1] = reinterpret_cast<const char*>(loadRequest.buffer());
   requests[2] = "<commit><confirmed/><confirm-timeout>300</confirm-timeout></commit>";
   requests[3] = "<commit/>";
   requests[4] = unlockRequest;

   char *replies[5];
   int received = ctx->executeNETCONFRequests(5, requests, replies, 120000);
   if (received < 0)
   {
      errorLog->append(L"Cannot execute NETCONF request (device or proxy connection failure)");
      return false;
   }

   // Proxy agent stops the sequence at first failed RPC, so only the last received
   // reply can indicate failure; a missing reply means device connection was lost
   int failedStep = -1;
   for(int i = 0; (i < received) && (i < 4); i++)
   {
      NETCONF_Response response;
      if (response.parse(replies[i], strlen(replies[i])) && response.isSuccess())
      {
         progressCallback(i + 1, 4);
         continue;
      }
      String errorText = response.getErrorText();
      errorLog->appendFormattedString(L"Cannot %s: %s", stepNames[i], errorText.isEmpty() ? L"device returned error" : errorText.cstr());
      failedStep = i;
      break;
   }
   if ((failedStep == -1) && (received < 4))
   {
      errorLog->appendFormattedString(L"Device connection lost during \"%s\" operation", stepNames[received]);
      failedStep = received;
   }

   bool success = (failedStep == -1);
   if (success && (received == 5))
   {
      NETCONF_Response response;
      if (!response.parse(replies[4], strlen(replies[4])) || !response.isSuccess())
         nxlog_debug_tag(JUNIPER_DEBUG_TAG, 4, _T("restoreConfig: configuration committed but candidate datastore unlock failed (%s)"), response.getErrorText().cstr());
   }

   for(int i = 0; i < 5; i++)
      MemFree(replies[i]);

   if (success)
      return true;

   if ((failedStep == 1) || (failedStep == 2))
   {
      // Candidate datastore may contain partially loaded configuration - discard changes
      // and release the lock (best effort; session termination releases the lock anyway)
      const char *cleanupRequests[2] = { "<discard-changes/>", unlockRequest };
      char *cleanupReplies[2];
      ctx->executeNETCONFRequests(2, cleanupRequests, cleanupReplies, 60000);
      MemFree(cleanupReplies[0]);
      MemFree(cleanupReplies[1]);
   }
   else if (failedStep == 3)
   {
      errorLog->append(L"; device will roll back to previous configuration automatically (confirmed commit timeout 300 seconds)");
   }
   return false;
}

/**
 * Driver module entry point
 */
NDD_BEGIN_DRIVER_LIST
NDD_DRIVER(JuniperDriver)
NDD_DRIVER(NetscreenDriver)
NDD_END_DRIVER_LIST
DECLARE_NDD_MODULE_ENTRY_POINT

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
