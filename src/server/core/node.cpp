/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
** File: node.cpp
**
**/

#include "nxcore.h"
#include <agent_tunnel.h>
#include <entity_mib.h>
#include <ethernet_ip.h>
#include <netxms_maps.h>
#include <asset_management.h>
#include <ncdrv.h>
#include <device-backup.h>
#include <netxms_maps.h>

#ifndef _WIN32
#include <sys/resource.h>
#endif

#define ROUTING_TABLE_CACHE_TIMEOUT  120  // Routing table cache timeout in seconds

#define SMCLP_ALLOWED_SYMBOLS L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789/-_"

#define DEBUG_TAG_DC_AGENT_CACHE    _T("dc.agent.cache")
#define DEBUG_TAG_DC_SNMP           _T("dc.snmp")
#define DEBUG_TAG_ICMP_POLL         _T("poll.icmp")
#define DEBUG_TAG_NODE_INTERFACES   _T("node.iface")
#define DEBUG_TAG_ROUTES_POLL       _T("poll.routes")
#define DEBUG_TAG_SNMP_TRAP_FLOOD   _T("snmp.trap.flood")

/**
 * Performance counters
 */
extern VolatileCounter64 g_snmpTrapsReceived;
extern VolatileCounter64 g_syslogMessagesReceived;
extern VolatileCounter64 g_windowsEventsReceived;
extern uint32_t g_averageDCIQueuingTime;

/**
 * Poller thread pool
 */
extern ThreadPool *g_pollerThreadPool;

/**
 * Unbind agent tunnel from node
 */
uint32_t UnbindAgentTunnel(uint32_t nodeId, uint32_t userId);

/**
 * Software package management functions
 */
int PackageNameVersionComparator(const SoftwarePackage **p1, const SoftwarePackage **p2);
ObjectArray<SoftwarePackage> *CalculatePackageChanges(ObjectArray<SoftwarePackage> *oldSet, ObjectArray<SoftwarePackage> *newSet);

/**
 * Hardware inventory management functions
 */
int HardwareComponentComparator(const HardwareComponent **c1, const HardwareComponent **c2);
ObjectArray<HardwareComponent> *CalculateHardwareChanges(ObjectArray<HardwareComponent> *oldSet, ObjectArray<HardwareComponent> *newSet);

/**
 * Get syncer run time statistic
 */
int64_t GetSyncerRunTime(StatisticType statType);

/**
 * Get internal metric from performance data storage driver
 */
DataCollectionError GetPerfDataStorageDriverMetric(const wchar_t *driver, const wchar_t *metric, wchar_t *value);

/**
 * Get attribute via EtherNet/IP
 */
DataCollectionError GetEtherNetIPAttribute(const InetAddress& addr, uint16_t port, const TCHAR *symbolicPath, uint32_t timeout, TCHAR *buffer, size_t size);

/**
 * Get status of notification channel
 */
bool GetNotificationChannelStatus(const TCHAR *name, NotificationChannelStatus *status);

/**
 * Get active discovery state
 */
bool IsActiveDiscoveryRunning();
String GetCurrentActiveDiscoveryRange();

/**
 * Poll cancellation checkpoint
 */
#define POLL_CANCELLATION_CHECKPOINT() \
         do { if (g_flags & AF_SHUTDOWN) { pollerUnlock(); return; } } while(0)

/**
 * Poll cancellation checkpoint with additional hook
 */
#define POLL_CANCELLATION_CHECKPOINT_EX(hook) \
         do { if (g_flags & AF_SHUTDOWN) { hook; pollerUnlock(); return; } } while(0)

/**
 * Convert hook script load error to text
 */
static const TCHAR *HookScriptLoadErrorToText(ScriptVMFailureReason reason)
{
   switch(reason)
   {
      case ScriptVMFailureReason::SCRIPT_IS_EMPTY:
         return _T("is empty");
      case ScriptVMFailureReason::SCRIPT_NOT_FOUND:
         return _T("not found");
      default:
         return _T("cannot be loaded");
   }
}

/**
 * Node class default constructor
 */
Node::Node() : super(Pollable::STATUS | Pollable::CONFIGURATION | Pollable::DISCOVERY | Pollable::TOPOLOGY | Pollable::ROUTING_TABLE | Pollable::ICMP),
         m_routingTableMutex(MutexType::FAST), m_topologyMutex(MutexType::FAST)
{
   m_status = STATUS_UNKNOWN;
   m_type = NODE_TYPE_UNKNOWN;
   m_subType[0] = 0;
   m_hypervisorType[0] = 0;
   m_capabilities = 0;
   m_expectedCapabilities = 0;
   m_zoneUIN = 0;
   m_agentPort = AGENT_LISTEN_PORT;
   m_agentCacheMode = AGENT_CACHE_DEFAULT;
   m_agentSecret[0] = 0;
   m_snmpVersion = SNMP_VERSION_2C;
   m_snmpPort = SNMP_DEFAULT_PORT;
   m_snmpSecurity = new SNMP_SecurityContext("public");
   m_snmpCodepage[0] = 0;
   m_ospfRouterId = 0;
   m_downSince = 0;
   m_savedDownSince = 0;
   m_bootTime = 0;
   m_agentUpTime = 0;
   m_proxyConnections = new ProxyAgentConnection[MAX_PROXY_TYPE];
   m_pendingDataConfigurationSync = 0;
   m_lastAgentTrapId = 0;
   m_lastSNMPTrapId = 0;
   m_lastSyslogMessageId = 0;
   m_lastWindowsEventId = 0;
   m_lastAgentPushRequestId = 0;
   m_agentCertSubject = nullptr;
   m_agentCertMappingMethod = MAP_CERTIFICATE_BY_CN;
   m_agentCertMappingData = nullptr;
   m_agentVersion[0] = 0;
   m_platformName[0] = 0;
   m_sysDescription = nullptr;
   m_sysName = nullptr;
   m_sysContact = nullptr;
   m_sysLocation = nullptr;
   m_lldpNodeId = nullptr;
   m_lldpLocalPortInfo = nullptr;
   m_agentParameters = nullptr;
   m_agentTables = nullptr;
   m_driverParameters = nullptr;
   m_smclpMetrics = nullptr;
   m_pollerNode = 0;
   m_agentProxy = 0;
   m_snmpProxy = 0;
   m_eipProxy = 0;
   m_mqttProxy = 0;
   m_modbusProxy = 0;
   m_icmpProxy = 0;
   memset(m_lastEvents, 0, sizeof(m_lastEvents));
   m_routingLoopEvents = new ObjectArray<RoutingLoopEvent>(0, 16, Ownership::True);
   m_failTimeAgent = TIMESTAMP_NEVER;
   m_failTimeSNMP = TIMESTAMP_NEVER;
   m_failTimeSSH = TIMESTAMP_NEVER;
   m_failTimeEtherNetIP = TIMESTAMP_NEVER;
   m_failTimeModbus = TIMESTAMP_NEVER;
   m_recoveryTime = TIMESTAMP_NEVER;
   m_lastAgentCommTime = TIMESTAMP_NEVER;
   m_lastAgentConnectAttempt = TIMESTAMP_NEVER;
   m_agentRestartTime = TIMESTAMP_NEVER;
   m_vrrpInfo = nullptr;
   m_topologyRebuildTimestamp = TIMESTAMP_NEVER;
   m_l1TopologyUsed = false;
   m_topologyDepth = -1;
   m_pendingState = -1;
   m_pollCountAgent = 0;
   m_pollCountSNMP = 0;
   m_pollCountSSH = 0;
   m_pollCountEtherNetIP = 0;
   m_pollCountModbus = 0;
   m_pollCountICMP = 0;
   m_requiredPollCount = 0; // Use system default
   m_pollsAfterIpUpdate = 0;
   m_nUseIfXTable = IFXTABLE_DEFAULT;  // Use system default
   m_radioInterfaces = nullptr;
   m_wirelessStations = nullptr;
   m_driver = FindDriverByName(nullptr);
   m_driverData = nullptr;
   m_softwarePackages = nullptr;
   m_hardwareComponents = nullptr;
   m_winPerfObjects = nullptr;
   m_winPerfObjectsTimestamp = 0;
   memset(m_baseBridgeAddress, 0, MAC_ADDR_LENGTH);
   m_physicalContainer = 0;
   m_rackPosition = 0;
   m_rackHeight = 1;
   m_syslogCodepage[0] = 0;
   m_syslogMessageCount = 0;
   m_snmpTrapCount = 0;
   m_snmpTrapLastTotal = 0;
   m_snmpTrapStormLastCheckTime = 0;
   m_snmpTrapStormActualDuration = 0;
   m_sshKeyId = 0;
   m_sshPort = SSH_PORT;
   m_sshProxy = 0;
   m_vncPort = 5900;
   m_vncProxy = 0;
   m_portNumberingScheme = NDD_PN_UNKNOWN;
   m_portRowCount = 0;
   m_agentCompressionMode = NODE_AGENT_COMPRESSION_DEFAULT;
   m_rackOrientation = FILL;
   m_icmpStatCollectionMode = IcmpStatCollectionMode::DEFAULT;
   m_icmpStatCollectors = nullptr;
   m_chassisPlacementConf = nullptr;
   m_eipPort = ETHERNET_IP_DEFAULT_PORT;
   m_cipDeviceType = 0;
   m_cipState = 0;
   m_cipStatus = 0;
   m_cipVendorCode = 0;
   m_modbusTcpPort = MODBUS_TCP_DEFAULT_PORT;
   m_modbusUnitId = 255;
   m_lastConfigBackupJobStatus = DeviceBackupJobStatus::UNKNOWN;
}

/**
 * Create new node from new node data
 */
Node::Node(const NewNodeData *newNodeData, uint32_t flags) : super(Pollable::STATUS | Pollable::CONFIGURATION | Pollable::DISCOVERY | Pollable::TOPOLOGY | Pollable::ROUTING_TABLE | Pollable::ICMP),
         m_ipAddress(newNodeData->ipAddr), m_primaryHostName(newNodeData->ipAddr.toString()), m_routingTableMutex(MutexType::FAST), m_topologyMutex(MutexType::FAST),
         m_sshLogin(newNodeData->sshLogin), m_sshPassword(newNodeData->sshPassword), m_vncPassword(newNodeData->vncPassword)
{
   m_runtimeFlags |= ODF_CONFIGURATION_POLL_PENDING;
   m_status = STATUS_UNKNOWN;
   m_type = NODE_TYPE_UNKNOWN;
   m_subType[0] = 0;
   m_hypervisorType[0] = 0;
   m_capabilities = 0;
   m_expectedCapabilities = 0;
   m_flags = flags;
   m_zoneUIN = newNodeData->zoneUIN;
   m_agentPort = newNodeData->agentPort;
   m_agentCacheMode = AGENT_CACHE_DEFAULT;
   m_agentSecret[0] = 0;
   m_snmpVersion = SNMP_VERSION_2C;
   m_snmpPort = newNodeData->snmpPort;
   if (newNodeData->snmpSecurity != nullptr)
      m_snmpSecurity = new SNMP_SecurityContext(newNodeData->snmpSecurity);
   else
      m_snmpSecurity = new SNMP_SecurityContext("public");
   if (newNodeData->name[0] != 0)
      _tcslcpy(m_name, newNodeData->name, MAX_OBJECT_NAME);
   else
      newNodeData->ipAddr.toString(m_name);    // Make default name from IP address
   m_snmpCodepage[0] = 0;
   m_ospfRouterId = 0;
   m_downSince = 0;
   m_savedDownSince = 0;
   m_bootTime = 0;
   m_agentUpTime = 0;
   m_proxyConnections = new ProxyAgentConnection[MAX_PROXY_TYPE];
   m_pendingDataConfigurationSync = 0;
   m_lastAgentTrapId = 0;
   m_lastSNMPTrapId = 0;
   m_lastSyslogMessageId = 0;
   m_lastWindowsEventId = 0;
   m_lastAgentPushRequestId = 0;
   m_agentCertSubject = nullptr;
   m_agentCertMappingMethod = MAP_CERTIFICATE_BY_CN;
   m_agentCertMappingData = nullptr;
   m_agentVersion[0] = 0;
   m_platformName[0] = 0;
   m_sysDescription = nullptr;
   m_sysName = nullptr;
   m_sysContact = nullptr;
   m_sysLocation = nullptr;
   m_lldpNodeId = nullptr;
   m_lldpLocalPortInfo = nullptr;
   m_agentParameters = nullptr;
   m_agentTables = nullptr;
   m_driverParameters = nullptr;
   m_smclpMetrics = nullptr;
   m_pollerNode = 0;
   m_agentProxy = newNodeData->agentProxyId;
   m_snmpProxy = newNodeData->snmpProxyId;
   m_eipProxy = newNodeData->eipProxyId;
   m_mqttProxy = newNodeData->mqttProxyId;
   m_modbusProxy = newNodeData->modbusProxyId;
   m_icmpProxy = newNodeData->icmpProxyId;
   memset(m_lastEvents, 0, sizeof(m_lastEvents));
   m_routingLoopEvents = new ObjectArray<RoutingLoopEvent>(0, 16, Ownership::True);
   m_isHidden = true;
   m_failTimeAgent = TIMESTAMP_NEVER;
   m_failTimeSNMP = TIMESTAMP_NEVER;
   m_failTimeSSH = TIMESTAMP_NEVER;
   m_failTimeEtherNetIP = TIMESTAMP_NEVER;
   m_failTimeModbus = TIMESTAMP_NEVER;
   m_recoveryTime = TIMESTAMP_NEVER;
   m_lastAgentCommTime = TIMESTAMP_NEVER;
   m_lastAgentConnectAttempt = TIMESTAMP_NEVER;
   m_agentRestartTime = TIMESTAMP_NEVER;
   m_vrrpInfo = nullptr;
   m_topologyRebuildTimestamp = TIMESTAMP_NEVER;
   m_l1TopologyUsed = false;
   m_topologyDepth = -1;
   m_pendingState = -1;
   m_pollCountAgent = 0;
   m_pollCountSNMP = 0;
   m_pollCountSSH = 0;
   m_pollCountEtherNetIP = 0;
   m_pollCountModbus = 0;
   m_pollCountICMP = 0;
   m_requiredPollCount = 0; // Use system default
   m_pollsAfterIpUpdate = 0;
   m_nUseIfXTable = IFXTABLE_DEFAULT;  // Use system default
   m_radioInterfaces = nullptr;
   m_wirelessStations = nullptr;
   m_driver = FindDriverByName(nullptr);
   m_driverData = nullptr;
   m_softwarePackages = nullptr;
   m_hardwareComponents = nullptr;
   m_winPerfObjects = nullptr;
   m_winPerfObjectsTimestamp = 0;
   memset(m_baseBridgeAddress, 0, MAC_ADDR_LENGTH);
   m_physicalContainer = 0;
   m_rackPosition = 0;
   m_rackHeight = 1;
   m_syslogCodepage[0] = 0;
   m_syslogMessageCount = 0;
   m_snmpTrapCount = 0;
   m_snmpTrapLastTotal = 0;
   m_snmpTrapStormLastCheckTime = 0;
   m_snmpTrapStormActualDuration = 0;
   m_sshKeyId = 0;
   m_sshPort = newNodeData->sshPort;
   m_sshProxy = newNodeData->sshProxyId;
   m_vncPort = newNodeData->vncPort;
   m_vncProxy = newNodeData->vncProxyId;
   m_portNumberingScheme = NDD_PN_UNKNOWN;
   m_portRowCount = 0;
   m_agentCompressionMode = NODE_AGENT_COMPRESSION_DEFAULT;
   m_rackOrientation = FILL;
   m_agentId = newNodeData->agentId;
   m_icmpStatCollectionMode = IcmpStatCollectionMode::DEFAULT;
   m_icmpStatCollectors = nullptr;
   setCreationTime();
   m_chassisPlacementConf = nullptr;
   m_eipPort = newNodeData->eipPort;
   m_cipDeviceType = 0;
   m_cipState = 0;
   m_cipStatus = 0;
   m_cipVendorCode = 0;
   m_webServiceProxy = newNodeData->webServiceProxyId;
   m_modbusTcpPort = newNodeData->modbusTcpPort;
   m_modbusUnitId = newNodeData->modbusUnitId;
   m_l1TopologyUsed = false;
   m_topologyDepth = -1;
   m_lastConfigBackupJobStatus = DeviceBackupJobStatus::UNKNOWN;
}

/**
 * Node destructor
 */
Node::~Node()
{
   delete m_driverData;
   delete[] m_proxyConnections;
   delete m_agentParameters;
   delete m_agentTables;
   delete m_driverParameters;
   delete m_smclpMetrics;
   MemFree(m_sysDescription);
   delete m_vrrpInfo;
   delete m_snmpSecurity;
   delete m_radioInterfaces;
   delete m_wirelessStations;
   MemFree(m_lldpNodeId);
   delete m_lldpLocalPortInfo;
   delete m_softwarePackages;
   delete m_hardwareComponents;
   delete m_winPerfObjects;
   MemFree(m_sysName);
   MemFree(m_sysContact);
   MemFree(m_sysLocation);
   delete m_routingLoopEvents;
   MemFree(m_agentCertSubject);
   MemFree(m_agentCertMappingData);
   delete m_icmpStatCollectors;
   MemFree(m_chassisPlacementConf);
}

/**
 * Create object from database data
 */
bool Node::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
{
   int i, iNumRows;
   bool bResult = false;

   m_id = id;

   if (!loadCommonProperties(hdb, preparedStatements) || !super::loadFromDatabase(hdb, id, preparedStatements))
      return false;

   if (Pollable::loadFromDatabase(hdb, m_id))
   {
      if (static_cast<uint32_t>(time(nullptr) - m_configurationPollState.getLastCompleted()) < g_configurationPollingInterval)
         m_runtimeFlags |= ODF_CONFIGURATION_POLL_PASSED;
   }

   DB_STATEMENT hStmt = PrepareObjectLoadStatement(hdb, preparedStatements, LSI_NODE,
      _T("SELECT primary_name,primary_ip,snmp_version,secret,agent_port,snmp_oid,agent_version,")
      _T("platform_name,poller_node_id,zone_guid,proxy_node,snmp_proxy,required_polls,uname,use_ifxtable,")
      _T("snmp_port,community,usm_auth_password,usm_priv_password,usm_methods,snmp_sys_name,bridge_base_addr,")
      _T("down_since,boot_time,driver_name,icmp_proxy,agent_cache_mode,snmp_sys_contact,snmp_sys_location,")
      _T("physical_container_id,rack_image_front,rack_position,rack_height,last_agent_comm_time,syslog_msg_count,")
      _T("snmp_trap_count,node_type,node_subtype,ssh_login,ssh_password,ssh_proxy,port_rows,port_numbering_scheme,")
      _T("agent_comp_mode,tunnel_id,lldp_id,capabilities,fail_time_snmp,fail_time_agent,fail_time_ssh,rack_orientation,")
      _T("rack_image_rear,agent_id,agent_cert_subject,hypervisor_type,hypervisor_info,icmp_poll_mode,")
      _T("chassis_placement_config,vendor,product_code,product_name,product_version,serial_number,cip_device_type,")
      _T("cip_status,cip_state,eip_proxy,eip_port,hardware_id,cip_vendor_code,agent_cert_mapping_method,")
      _T("agent_cert_mapping_data,snmp_engine_id,ssh_port,ssh_key_id,syslog_codepage,snmp_codepage,ospf_router_id,")
      _T("mqtt_proxy,modbus_proxy,modbus_tcp_port,modbus_unit_id,snmp_context_engine_id,vnc_password,vnc_port,")
      _T("vnc_proxy,path_check_reason,path_check_node_id,path_check_iface_id,expected_capabilities,last_events FROM nodes WHERE id=?"));
   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult == nullptr)
      return false;     // Query failed

   if (DBGetNumRows(hResult) == 0)
   {
      DBFreeResult(hResult);
      nxlog_debug_tag(DEBUG_TAG_OBJECT_INIT, 2, _T("Missing record in \"nodes\" table for node object %d"), id);
      return false;
   }

   TCHAR buffer[256];

   m_primaryHostName = DBGetFieldAsSharedString(hResult, 0, 0);
   m_ipAddress = DBGetFieldInetAddr(hResult, 0, 1);
   m_snmpVersion = static_cast<SNMP_Version>(DBGetFieldLong(hResult, 0, 2));
   DBGetField(hResult, 0, 3, m_agentSecret, MAX_SECRET_LENGTH);
   m_agentPort = static_cast<uint16_t>(DBGetFieldLong(hResult, 0, 4));
   m_snmpObjectId = SNMP_ObjectId::parse(DBGetField(hResult, 0, 5, buffer, 256));
   DBGetField(hResult, 0, 6, m_agentVersion, MAX_AGENT_VERSION_LEN);
   DBGetField(hResult, 0, 7, m_platformName, MAX_PLATFORM_NAME_LEN);
   m_pollerNode = DBGetFieldUInt32(hResult, 0, 8);
   m_zoneUIN = DBGetFieldUInt32(hResult, 0, 9);
   m_agentProxy = DBGetFieldUInt32(hResult, 0, 10);
   m_snmpProxy = DBGetFieldUInt32(hResult, 0, 11);
   m_requiredPollCount = DBGetFieldLong(hResult, 0, 12);
   m_sysDescription = DBGetField(hResult, 0, 13, nullptr, 0);
   m_nUseIfXTable = (BYTE)DBGetFieldLong(hResult, 0, 14);
   m_snmpPort = static_cast<uint16_t>(DBGetFieldLong(hResult, 0, 15));

   // SNMP authentication parameters
   char snmpAuthObject[256], snmpAuthPassword[256], snmpPrivPassword[256], snmpEngineId[256];
   DBGetFieldA(hResult, 0, 16, snmpAuthObject, 256);
   DBGetFieldA(hResult, 0, 17, snmpAuthPassword, 256);
   DBGetFieldA(hResult, 0, 18, snmpPrivPassword, 256);
   int snmpMethods = DBGetFieldLong(hResult, 0, 19);
   DBGetFieldA(hResult, 0, 72, snmpEngineId, 256);
   delete m_snmpSecurity;
   if (m_snmpVersion == SNMP_VERSION_3)
   {
      m_snmpSecurity = new SNMP_SecurityContext(snmpAuthObject, snmpAuthPassword, snmpPrivPassword,
               static_cast<SNMP_AuthMethod>(snmpMethods & 0xFF), static_cast<SNMP_EncryptionMethod>(snmpMethods >> 8));
      if (snmpEngineId[0] != 0)
      {
         BYTE engineId[128];
         size_t engineIdLen = StrToBinA(snmpEngineId, engineId, 128);
         if (engineIdLen > 0)
            m_snmpSecurity->setAuthoritativeEngine(SNMP_Engine(engineId, engineIdLen, 0, 0));
      }
      m_snmpSecurity->recalculateKeys();

      DBGetFieldA(hResult, 0, 82, snmpEngineId, 256);
      if (snmpEngineId[0] != 0)
      {
         BYTE engineId[128];
         size_t engineIdLen = StrToBinA(snmpEngineId, engineId, 128);
         if (engineIdLen > 0)
            m_snmpSecurity->setContextEngine(SNMP_Engine(engineId, engineIdLen));
      }
   }
   else
   {
      // This will create security context with V2C security model
      // USM fields will be loaded but keys will not be calculated
      m_snmpSecurity = new SNMP_SecurityContext(snmpAuthObject);
      m_snmpSecurity->setAuthMethod(static_cast<SNMP_AuthMethod>(snmpMethods & 0xFF));
      m_snmpSecurity->setAuthPassword(snmpAuthPassword);
      m_snmpSecurity->setPrivMethod(static_cast<SNMP_EncryptionMethod>(snmpMethods >> 8));
      m_snmpSecurity->setPrivPassword(snmpPrivPassword);
   }

   m_sysName = DBGetField(hResult, 0, 20, nullptr, 0);

   TCHAR baseAddr[16];
   TCHAR *value = DBGetField(hResult, 0, 21, baseAddr, 16);
   if (value != nullptr)
      StrToBin(value, m_baseBridgeAddress, MAC_ADDR_LENGTH);

   m_savedDownSince = m_downSince = DBGetFieldLong(hResult, 0, 22);
   m_bootTime = DBGetFieldLong(hResult, 0, 23);

   // Setup driver
   TCHAR driverName[34];
   DBGetField(hResult, 0, 24, driverName, 34);
   Trim(driverName);
   if (driverName[0] != 0)
      m_driver = FindDriverByName(driverName);

   m_icmpProxy = DBGetFieldUInt32(hResult, 0, 25);
   m_agentCacheMode = DBGetFieldInt16(hResult, 0, 26);
   if ((m_agentCacheMode != AGENT_CACHE_ON) && (m_agentCacheMode != AGENT_CACHE_OFF))
      m_agentCacheMode = AGENT_CACHE_DEFAULT;

   m_sysContact = DBGetField(hResult, 0, 27, nullptr, 0);
   m_sysLocation = DBGetField(hResult, 0, 28, nullptr, 0);

   m_physicalContainer = DBGetFieldUInt32(hResult, 0, 29);
   m_rackImageFront = DBGetFieldGUID(hResult, 0, 30);
   m_rackPosition = DBGetFieldInt16(hResult, 0, 31);
   m_rackHeight = DBGetFieldInt16(hResult, 0, 32);
   m_lastAgentCommTime = DBGetFieldLong(hResult, 0, 33);
   m_syslogMessageCount = DBGetFieldInt64(hResult, 0, 34);
   m_snmpTrapCount = DBGetFieldInt64(hResult, 0, 35);
   m_snmpTrapLastTotal = m_snmpTrapCount;
   m_type = (NodeType)DBGetFieldInt32(hResult, 0, 36);
   DBGetField(hResult, 0, 37, m_subType, MAX_NODE_SUBTYPE_LENGTH);
   m_sshLogin = DBGetFieldAsSharedString(hResult, 0, 38);
   m_sshPassword = DBGetFieldAsSharedString(hResult, 0, 39);
   m_sshProxy = DBGetFieldUInt32(hResult, 0, 40);
   m_portRowCount = DBGetFieldUInt32(hResult, 0, 41);
   m_portNumberingScheme = DBGetFieldUInt32(hResult, 0, 42);
   m_agentCompressionMode = DBGetFieldInt16(hResult, 0, 43);
   m_tunnelId = DBGetFieldGUID(hResult, 0, 44);
   m_lldpNodeId = DBGetField(hResult, 0, 45, nullptr, 0);
   if ((m_lldpNodeId != nullptr) && (*m_lldpNodeId == 0))
      MemFreeAndNull(m_lldpNodeId);
   m_capabilities = DBGetFieldUInt64(hResult, 0, 46);
   m_failTimeSNMP = DBGetFieldLong(hResult, 0, 47);
   m_failTimeAgent = DBGetFieldLong(hResult, 0, 48);
   m_failTimeSSH = DBGetFieldLong(hResult, 0, 49);
   m_rackOrientation = static_cast<RackOrientation>(DBGetFieldLong(hResult, 0, 50));
   m_rackImageRear = DBGetFieldGUID(hResult, 0, 51);
   m_agentId = DBGetFieldGUID(hResult, 0, 52);
   m_agentCertSubject = DBGetField(hResult, 0, 53, nullptr, 0);
   if ((m_agentCertSubject != nullptr) && (m_agentCertSubject[0] == 0))
      MemFreeAndNull(m_agentCertSubject);
   DBGetField(hResult, 0, 54, m_hypervisorType, MAX_HYPERVISOR_TYPE_LENGTH);
   m_hypervisorInfo = DBGetFieldAsSharedString(hResult, 0, 55);

   switch(DBGetFieldLong(hResult, 0, 56))
   {
      case 1:
         m_icmpStatCollectionMode = IcmpStatCollectionMode::ON;
         break;
      case 2:
         m_icmpStatCollectionMode = IcmpStatCollectionMode::OFF;
         break;
      default:
         m_icmpStatCollectionMode = IcmpStatCollectionMode::DEFAULT;
         break;
   }
   m_chassisPlacementConf = DBGetField(hResult, 0, 57, nullptr, 0);

   m_vendor = DBGetFieldAsSharedString(hResult, 0, 58);
   m_productCode = DBGetFieldAsSharedString(hResult, 0, 59);
   m_productName = DBGetFieldAsSharedString(hResult, 0, 60);
   m_productVersion = DBGetFieldAsSharedString(hResult, 0, 61);
   m_serialNumber = DBGetFieldAsSharedString(hResult, 0, 62);
   m_cipDeviceType = DBGetFieldUInt16(hResult, 0, 63);
   m_cipStatus = DBGetFieldUInt16(hResult, 0, 64);
   m_cipState = static_cast<uint8_t>(DBGetFieldUInt16(hResult, 0, 65));
   m_eipProxy = DBGetFieldUInt32(hResult, 0, 66);
   m_eipPort = DBGetFieldUInt16(hResult, 0, 67);
   BYTE hardwareId[HARDWARE_ID_LENGTH];
   DBGetFieldByteArray2(hResult, 0, 68, hardwareId, HARDWARE_ID_LENGTH, 0);
   m_hardwareId = NodeHardwareId(hardwareId);
   m_cipVendorCode = DBGetFieldUInt16(hResult, 0, 69);
   m_agentCertMappingMethod = static_cast<CertificateMappingMethod>(DBGetFieldLong(hResult, 0, 70));

   m_agentCertMappingData = DBGetField(hResult, 0, 71, nullptr, 0);
   if ((m_agentCertMappingData != nullptr) && (m_agentCertMappingData[0] == 0))
      MemFreeAndNull(m_agentCertMappingData);

   m_sshPort = DBGetFieldUInt16(hResult, 0, 73);
   m_sshKeyId = DBGetFieldLong(hResult, 0, 74);
   DBGetFieldUTF8(hResult, 0, 75, m_syslogCodepage, 16);
   DBGetFieldUTF8(hResult, 0, 76, m_snmpCodepage, 16);
   m_ospfRouterId = DBGetFieldIPAddr(hResult, 0, 77);
   m_mqttProxy = DBGetFieldUInt32(hResult, 0, 78);
   m_modbusProxy = DBGetFieldUInt32(hResult, 0, 79);
   m_modbusTcpPort = DBGetFieldUInt16(hResult, 0, 80);
   m_modbusUnitId = DBGetFieldUInt16(hResult, 0, 81);
   m_vncPassword = DBGetFieldAsSharedString(hResult, 0, 83);
   m_vncPort = DBGetFieldUInt16(hResult, 0, 84);
   m_vncProxy = DBGetFieldUInt32(hResult, 0, 85);
   m_pathCheckResult.reason = static_cast<NetworkPathFailureReason>(DBGetFieldInt32(hResult, 0, 86));
   if (m_pathCheckResult.reason != NetworkPathFailureReason::NONE)
   {
      m_pathCheckResult.rootCauseFound = true;
      m_pathCheckResult.rootCauseNodeId = DBGetFieldUInt32(hResult, 0, 87);
      m_pathCheckResult.rootCauseInterfaceId = DBGetFieldUInt32(hResult, 0, 88);
   }
   m_expectedCapabilities = DBGetFieldUInt64(hResult, 0, 89);
   StringList elements = DBGetFieldAsString(hResult, 0, 90).split(L";");
   for (i = 0; i < MIN(elements.size(), MAX_LAST_EVENTS); i++)
      m_lastEvents[i] = _tcstoul(elements.get(i), nullptr, 10);

   DBFreeResult(hResult);

   if (m_isDeleted)
   {
      return true;
   }

   // Link node to subnets
   hStmt = PrepareObjectLoadStatement(hdb, preparedStatements, LSI_SUBNET_MAPPING, _T("SELECT subnet_id FROM nsmap WHERE node_id=?"));
   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   hResult = DBSelectPrepared(hStmt);
   if (hResult == nullptr)
      return false;     // Query failed

   iNumRows = DBGetNumRows(hResult);
   for(i = 0; i < iNumRows; i++)
   {
      uint32_t subnetId = DBGetFieldUInt32(hResult, i, 0);
      shared_ptr<NetObj> subnet = FindObjectById(subnetId, OBJECT_SUBNET);
      if (subnet != nullptr)
      {
         linkObjects(subnet, self());
      }
      else
      {
         nxlog_write(NXLOG_ERROR, _T("Inconsistent database: node %s [%u] linked to non-existing subnet [%u]"), m_name, m_id, subnetId);
      }
   }

   DBFreeResult(hResult);

   loadItemsFromDB(hdb, preparedStatements);
   loadACLFromDB(hdb, preparedStatements);

   // Walk through all items in the node and load appropriate thresholds
   bResult = true;
   for(i = 0; i < m_dcObjects.size(); i++)
   {
      if (!m_dcObjects.get(i)->loadThresholdsFromDB(hdb, preparedStatements))
      {
         nxlog_debug_tag(DEBUG_TAG_OBJECT_INIT, 3, _T("Cannot load thresholds for DCI %u of node %s [%u]"), m_dcObjects.get(i)->getId(), m_name, id);
         bResult = false;
      }
   }
   loadDCIListForCleanup(hdb);

   updatePhysicalContainerBinding(m_physicalContainer);

   if (bResult)
   {
      // Load components
      hStmt = PrepareObjectLoadStatement(hdb, preparedStatements, LSI_NODE_COMPONENTS,
         _T("SELECT component_index,parent_index,position,component_class,if_index,name,description,model,serial_number,vendor,firmware FROM node_components WHERE node_id=?"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         hResult = DBSelectPrepared(hStmt);
         if (hResult != nullptr)
         {
            int count = DBGetNumRows(hResult);
            if (count > 0)
            {
               ObjectArray<Component> elements(count);
               for(int i = 0; i < count; i++)
               {
                  TCHAR name[256], description[256], model[256], serial[64], vendor[64], firmware[128];
                  elements.add(new Component(
                           DBGetFieldUInt32(hResult, i, 0), // index
                           DBGetFieldUInt32(hResult, i, 3), // class
                           DBGetFieldUInt32(hResult, i, 1), // parent index
                           DBGetFieldUInt32(hResult, i, 2), // position
                           DBGetFieldUInt32(hResult, i, 4), // ifIndex
                           DBGetField(hResult, i, 5, name, 256),
                           DBGetField(hResult, i, 6, description, 256),
                           DBGetField(hResult, i, 7, model, 256),
                           DBGetField(hResult, i, 8, serial, 64),
                           DBGetField(hResult, i, 9, vendor, 64),
                           DBGetField(hResult, i, 10, firmware, 128)
                           ));
               }

               Component *root = nullptr;
               for(int i = 0; i < elements.size(); i++)
                  if (elements.get(i)->getParentIndex() == 0)
                  {
                     root = elements.get(i);
                     break;
                  }

               if (root != nullptr)
               {
                  root->buildTree(&elements);
                  m_components = make_shared<ComponentTree>(root);
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG_OBJECT_INIT, 6, _T("Node::loadFromDatabase(%s [%u]): root element for component tree not found"), m_name, m_id);
                  elements.setOwner(Ownership::True);   // cause element destruction on exit
               }
            }
            DBFreeResult(hResult);
         }
         else
         {
            bResult = false;
         }
      }
      else
      {
         bResult = false;
      }

      if (!bResult)
         nxlog_debug_tag(DEBUG_TAG_OBJECT_INIT, 3, _T("Cannot load components for node %s [%u]"), m_name, m_id);
   }

   if (bResult)
   {
      // Load software packages
      hStmt = PrepareObjectLoadStatement(hdb, preparedStatements, LSI_SOFTWARE_INVENTORY,
         _T("SELECT name,version,vendor,install_date,url,description,uninstall_key FROM software_inventory WHERE node_id=?"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         hResult = DBSelectPrepared(hStmt);
         if (hResult != nullptr)
         {
            int count = DBGetNumRows(hResult);
            if (count > 0)
            {
               m_softwarePackages = new ObjectArray<SoftwarePackage>(count, 64, Ownership::True);
               for(int i = 0; i < count; i++)
                  m_softwarePackages->add(new SoftwarePackage(hResult, i));
               m_softwarePackages->sort(PackageNameVersionComparator);
            }
            DBFreeResult(hResult);
         }
         else
         {
            bResult = false;
         }
      }
      else
      {
         bResult = false;
      }

      if (!bResult)
         nxlog_debug_tag(DEBUG_TAG_OBJECT_INIT, 3, _T("Cannot load software package list for node %s [%u]"), m_name, m_id);
   }

   if (bResult)
   {
      // Load hardware components
      hStmt = PrepareObjectLoadStatement(hdb, preparedStatements, LSI_HARDWARE_INVENTORY,
         _T("SELECT category,component_index,hw_type,vendor,model,location,capacity,part_number,serial_number,description FROM hardware_inventory WHERE node_id=?"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         hResult = DBSelectPrepared(hStmt);
         if (hResult != nullptr)
         {
            int count = DBGetNumRows(hResult);
            if (count > 0)
            {
               m_hardwareComponents = new ObjectArray<HardwareComponent>(count, 16, Ownership::True);
               for(int i = 0; i < count; i++)
                  m_hardwareComponents->add(new HardwareComponent(hResult, i));
               m_hardwareComponents->sort(HardwareComponentComparator);
            }
            DBFreeResult(hResult);
         }
         else
         {
            bResult = false;
         }
      }
      else
      {
         bResult = false;
      }

      if (!bResult)
         nxlog_debug_tag(DEBUG_TAG_OBJECT_INIT, 3, _T("Cannot load hardware information for node %s [%u]"), m_name, m_id);
   }

   if (bResult && isOSPFSupported())
   {
      // Load OSPF areas
      hStmt = DBPrepare(hdb, _T("SELECT area_id FROM ospf_areas WHERE node_id=?"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         hResult = DBSelectPrepared(hStmt);
         if (hResult != nullptr)
         {
            int count = DBGetNumRows(hResult);
            if (count > 0)
            {
               for(int i = 0; i < count; i++)
               {
                  OSPFArea *a = m_ospfAreas.addPlaceholder();
                  memset(a, 0, sizeof(OSPFArea));
                  a->id = DBGetFieldIPAddr(hResult, i, 0);
               }
            }
            DBFreeResult(hResult);
         }
         else
         {
            bResult = false;
         }
         DBFreeStatement(hStmt);
      }
      else
      {
         bResult = false;
      }

      if (!bResult)
         nxlog_debug_tag(DEBUG_TAG_OBJECT_INIT, 3, _T("Cannot load OSPF area information for node %s [%u]"), m_name, m_id);
   }

   if (bResult && isOSPFSupported())
   {
      // Load OSPF neighbors
      hStmt = DBPrepare(hdb, _T("SELECT router_id,area_id,ip_address,remote_node_id,if_index,is_virtual,neighbor_state FROM ospf_neighbors WHERE node_id=?"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         hResult = DBSelectPrepared(hStmt);
         if (hResult != nullptr)
         {
            int count = DBGetNumRows(hResult);
            if (count > 0)
            {
               for(int i = 0; i < count; i++)
               {
                  OSPFNeighbor *n = m_ospfNeighbors.addPlaceholder();
                  memset(n, 0, sizeof(OSPFNeighbor));
                  n->routerId = DBGetFieldIPAddr(hResult, i, 0);
                  n->areaId = DBGetFieldIPAddr(hResult, i, 1);
                  n->ipAddress = DBGetFieldInetAddr(hResult, i, 2);
                  n->nodeId = DBGetFieldUInt32(hResult, i, 3);
                  n->ifIndex = DBGetFieldUInt32(hResult, i, 4);
                  n->isVirtual = DBGetFieldLong(hResult, i, 5) ? true : false;
                  n->state = static_cast<OSPFNeighborState>(DBGetFieldLong(hResult, i, 6));
               }
            }
            DBFreeResult(hResult);
         }
         else
         {
            bResult = false;
         }
         DBFreeStatement(hStmt);
      }
      else
      {
         bResult = false;
      }

      if (!bResult)
         nxlog_debug_tag(DEBUG_TAG_OBJECT_INIT, 3, _T("Cannot load OSPF area information for node %s [%u]"), m_name, m_id);
   }

   if (bResult && isIcmpStatCollectionEnabled())
   {
      m_icmpStatCollectors = new StringObjectMap<IcmpStatCollector>(Ownership::True);
      hStmt = DBPrepare(hdb, _T("SELECT poll_target FROM icmp_statistics WHERE object_id=?"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         hResult = DBSelectPrepared(hStmt);
         if (hResult != nullptr)
         {
            int period = ConfigReadInt(_T("ICMP.StatisticPeriod"), 60);
            int count = DBGetNumRows(hResult);
            for(int i = 0; i < count; i++)
            {
               TCHAR target[128];
               DBGetField(hResult, i, 0, target, 128);
               IcmpStatCollector *c = IcmpStatCollector::loadFromDatabase(hdb, m_id, target, period);
               if (c != nullptr)
                  m_icmpStatCollectors->set(target, c);
               else
                  nxlog_debug_tag(DEBUG_TAG_OBJECT_INIT, 3, _T("Cannot load ICMP statistic collector %s for node %s [%u]"), target, m_name, m_id);
            }
            DBFreeResult(hResult);

            // Check that primary target exist
            if (!m_icmpStatCollectors->contains(_T("PRI")))
               m_icmpStatCollectors->set(_T("PRI"), new IcmpStatCollector(period));
         }
         else
         {
            bResult = false;
         }
         DBFreeStatement(hStmt);
      }
      else
      {
         bResult = false;
      }
   }

   if (bResult)
   {
      hStmt = DBPrepare(hdb, _T("SELECT ip_addr FROM icmp_target_address_list WHERE node_id=?"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         hResult = DBSelectPrepared(hStmt);
         if (hResult != nullptr)
         {
            int count = DBGetNumRows(hResult);
            for(int i = 0; i < count; i++)
            {
               InetAddress addr = DBGetFieldInetAddr(hResult, i, 0);
               if (addr.isValidUnicast() && !m_icmpTargets.hasAddress(addr))
                  m_icmpTargets.add(addr);
            }
            DBFreeResult(hResult);
         }
         else
         {
            bResult = false;
         }
         DBFreeStatement(hStmt);
      }
      else
      {
         bResult = false;
      }
   }

   // Load radio interfaces
   if (bResult)
   {
      DB_RESULT hResult = executeSelectOnObject(hdb, _T("SELECT radio_index,if_index,name,bssid,ssid,frequency,band,channel,power_dbm,power_mw FROM radios WHERE owner_id={id}"));
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         if (count > 0)
         {
            m_radioInterfaces = new StructArray<RadioInterfaceInfo>(count);
            for(int i = 0; i < count; i++)
            {
               RadioInterfaceInfo *rif = m_radioInterfaces->addPlaceholder();
               rif->index = DBGetFieldULong(hResult, i, 0);
               rif->ifIndex = DBGetFieldULong(hResult, i, 1);
               DBGetField(hResult, i, 2, rif->name, MAX_OBJECT_NAME);

               TCHAR bssid[16];
               DBGetField(hResult, i, 3, bssid, 16);
               StrToBin(bssid, rif->bssid, MAC_ADDR_LENGTH);

               DBGetField(hResult, i, 4, rif->ssid, MAX_SSID_LENGTH);
               rif->frequency = static_cast<uint16_t>(DBGetFieldULong(hResult, i, 5));
               rif->band = static_cast<RadioBand>(DBGetFieldLong(hResult, i, 6));
               rif->channel = static_cast<uint16_t>(DBGetFieldULong(hResult, i, 7));
               rif->powerDBm = DBGetFieldLong(hResult, i, 8);
               rif->powerMW = DBGetFieldLong(hResult, i, 9);
            }
         }
         DBFreeResult(hResult);
      }
      else
      {
         bResult = false;
      }
   }

   return bResult;
}

/**
 * Post-load hook
 */
void Node::postLoad()
{
   super::postLoad();

   for(int i = 0; i < m_ospfNeighbors.size(); i++)
   {
      OSPFNeighbor *n = m_ospfNeighbors.get(i);
      if (!n->isVirtual)
      {
         shared_ptr<Interface> iface = findInterfaceByIndex(n->ifIndex);
         if (iface != nullptr)
            n->ifObject = iface->getId();
      }
   }
}

/**
 * Save component
 */
static bool SaveComponent(DB_STATEMENT hStmt, const Component *component)
{
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, component->getIndex());
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, component->getParentIndex());
   DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, component->getPosition());
   DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, component->getClass());
   DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, component->getIfIndex());
   DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, component->getName(), DB_BIND_STATIC, 255);
   DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, component->getDescription(), DB_BIND_STATIC, 255);
   DBBind(hStmt, 9, DB_SQLTYPE_VARCHAR, component->getModel(), DB_BIND_STATIC, 255);
   DBBind(hStmt, 10, DB_SQLTYPE_VARCHAR, component->getSerial(), DB_BIND_STATIC, 63);
   DBBind(hStmt, 11, DB_SQLTYPE_VARCHAR, component->getVendor(), DB_BIND_STATIC, 63);
   DBBind(hStmt, 12, DB_SQLTYPE_VARCHAR, component->getFirmware(), DB_BIND_STATIC, 127);
   if (!DBExecute(hStmt))
      return false;

   const ObjectArray<Component>& children = component->getChildren();
   for(int i = 0; i < children.size(); i++)
      if (!SaveComponent(hStmt, children.get(i)))
         return false;
   return true;
}

/**
 * Save object to database
 */
bool Node::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);

   if (success && (m_modified & MODIFY_NODE_PROPERTIES))
   {
      static const wchar_t *columns[] = {
         _T("primary_ip"), _T("primary_name"), _T("snmp_port"), _T("capabilities"), _T("snmp_version"), _T("community"),
         _T("agent_port"), _T("secret"), _T("snmp_oid"), _T("uname"), _T("agent_version"), _T("platform_name"),
         _T("poller_node_id"), _T("zone_guid"), _T("proxy_node"), _T("snmp_proxy"),
         _T("icmp_proxy"), _T("required_polls"), _T("use_ifxtable"), _T("usm_auth_password"), _T("usm_priv_password"),
         _T("usm_methods"), _T("snmp_sys_name"), _T("bridge_base_addr"), _T("down_since"), _T("driver_name"),
         _T("rack_image_front"), _T("rack_position"), _T("rack_height"), _T("physical_container_id"), _T("boot_time"), _T("agent_cache_mode"),
         _T("snmp_sys_contact"), _T("snmp_sys_location"), _T("last_agent_comm_time"), _T("syslog_msg_count"),
         _T("snmp_trap_count"), _T("node_type"), _T("node_subtype"), _T("ssh_login"), _T("ssh_password"), _T("ssh_key_id"), _T("ssh_port"),
         _T("ssh_proxy"), _T("port_rows"), _T("port_numbering_scheme"), _T("agent_comp_mode"), _T("tunnel_id"), _T("lldp_id"),
         _T("fail_time_snmp"), _T("fail_time_agent"), _T("fail_time_ssh"), _T("rack_orientation"), _T("rack_image_rear"), _T("agent_id"),
         _T("agent_cert_subject"), _T("hypervisor_type"), _T("hypervisor_info"), _T("icmp_poll_mode"), _T("chassis_placement_config"),
         _T("vendor"), _T("product_code"), _T("product_name"), _T("product_version"), _T("serial_number"), _T("cip_device_type"),
         _T("cip_status"), _T("cip_state"), _T("eip_proxy"), _T("eip_port"), _T("hardware_id"), _T("cip_vendor_code"),
         _T("agent_cert_mapping_method"), _T("agent_cert_mapping_data"), _T("snmp_engine_id"), _T("snmp_context_engine_id"),
         _T("syslog_codepage"), _T("snmp_codepage"), _T("ospf_router_id"), _T("mqtt_proxy"), _T("modbus_proxy"), _T("modbus_tcp_port"),
         _T("modbus_unit_id"), _T("vnc_password"), _T("vnc_port"), _T("vnc_proxy"), _T("path_check_reason"), _T("path_check_node_id"),
         _T("path_check_iface_id"), L"expected_capabilities", L"last_events", nullptr
      };

      DB_STATEMENT hStmt = DBPrepareMerge(hdb, L"nodes", L"id", m_id, columns);
      if (hStmt != nullptr)
      {
         lockProperties();

         int32_t snmpMethods = m_snmpSecurity->getAuthMethod() | (m_snmpSecurity->getPrivMethod() << 8);
         TCHAR ipAddr[64], baseAddress[16], cacheMode[16], compressionMode[16], hardwareId[HARDWARE_ID_LENGTH * 2 + 1], routerId[16];

         const TCHAR *icmpPollMode;
         switch(m_icmpStatCollectionMode)
         {
            case IcmpStatCollectionMode::ON:
               icmpPollMode = _T("1");
               break;
            case IcmpStatCollectionMode::OFF:
               icmpPollMode = _T("2");
               break;
            default:
               icmpPollMode = _T("0");
               break;
         }

         const TCHAR *agentCertMappingMethod;
         switch(m_agentCertMappingMethod)
         {
            case MAP_CERTIFICATE_BY_CN:
               agentCertMappingMethod = _T("2");
               break;
            case MAP_CERTIFICATE_BY_PUBKEY:
               agentCertMappingMethod = _T("1");
               break;
            default:
               agentCertMappingMethod = _T("0");
               break;
         }

         TCHAR oidText[256];

         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_ipAddress.toString(ipAddr), DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_primaryHostName, DB_BIND_TRANSIENT);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_snmpPort);
         DBBind(hStmt, 4, DB_SQLTYPE_BIGINT, m_capabilities);
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_snmpVersion));
         DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, WideStringFromMBString(m_snmpSecurity->getAuthName()), DB_BIND_DYNAMIC);
         DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_agentPort);
         DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, m_agentSecret, DB_BIND_STATIC);
         DBBind(hStmt, 9, DB_SQLTYPE_VARCHAR, m_snmpObjectId.toString(oidText, 256), DB_BIND_STATIC);
         DBBind(hStmt, 10, DB_SQLTYPE_VARCHAR, m_sysDescription, DB_BIND_STATIC);
         DBBind(hStmt, 11, DB_SQLTYPE_VARCHAR, m_agentVersion, DB_BIND_STATIC);
         DBBind(hStmt, 12, DB_SQLTYPE_VARCHAR, m_platformName, DB_BIND_STATIC);
         DBBind(hStmt, 13, DB_SQLTYPE_INTEGER, m_pollerNode);
         DBBind(hStmt, 14, DB_SQLTYPE_INTEGER, m_zoneUIN);
         DBBind(hStmt, 15, DB_SQLTYPE_INTEGER, m_agentProxy);
         DBBind(hStmt, 16, DB_SQLTYPE_INTEGER, m_snmpProxy);
         DBBind(hStmt, 17, DB_SQLTYPE_INTEGER, m_icmpProxy);
         DBBind(hStmt, 18, DB_SQLTYPE_INTEGER, m_requiredPollCount);
         DBBind(hStmt, 19, DB_SQLTYPE_INTEGER, m_nUseIfXTable);
         DBBind(hStmt, 20, DB_SQLTYPE_VARCHAR, WideStringFromMBString(m_snmpSecurity->getAuthPassword()), DB_BIND_DYNAMIC);
         DBBind(hStmt, 21, DB_SQLTYPE_VARCHAR, WideStringFromMBString(m_snmpSecurity->getPrivPassword()), DB_BIND_DYNAMIC);
         DBBind(hStmt, 22, DB_SQLTYPE_INTEGER, snmpMethods);
         DBBind(hStmt, 23, DB_SQLTYPE_VARCHAR, m_sysName, DB_BIND_STATIC, 127);
         DBBind(hStmt, 24, DB_SQLTYPE_VARCHAR, BinToStr(m_baseBridgeAddress, MAC_ADDR_LENGTH, baseAddress), DB_BIND_STATIC);
         DBBind(hStmt, 25, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_downSince));
         DBBind(hStmt, 26, DB_SQLTYPE_VARCHAR, m_driver->getName(), DB_BIND_STATIC);
         DBBind(hStmt, 27, DB_SQLTYPE_VARCHAR, m_rackImageFront);   // rack image front
         DBBind(hStmt, 28, DB_SQLTYPE_INTEGER, m_rackPosition); // rack position
         DBBind(hStmt, 29, DB_SQLTYPE_INTEGER, m_rackHeight);   // device height in rack units
         DBBind(hStmt, 30, DB_SQLTYPE_INTEGER, m_physicalContainer);   // rack ID
         DBBind(hStmt, 31, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_bootTime));
         DBBind(hStmt, 32, DB_SQLTYPE_VARCHAR, IntegerToString(m_agentCacheMode, cacheMode), DB_BIND_STATIC, 1);
         DBBind(hStmt, 33, DB_SQLTYPE_VARCHAR, m_sysContact, DB_BIND_STATIC, 127);
         DBBind(hStmt, 34, DB_SQLTYPE_VARCHAR, m_sysLocation, DB_BIND_STATIC, 255);
         DBBind(hStmt, 35, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_lastAgentCommTime));
         DBBind(hStmt, 36, DB_SQLTYPE_BIGINT, m_syslogMessageCount);
         DBBind(hStmt, 37, DB_SQLTYPE_BIGINT, m_snmpTrapCount);
         DBBind(hStmt, 38, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_type));
         DBBind(hStmt, 39, DB_SQLTYPE_VARCHAR, m_subType, DB_BIND_STATIC);
         DBBind(hStmt, 40, DB_SQLTYPE_VARCHAR, m_sshLogin, DB_BIND_STATIC, 63);
         DBBind(hStmt, 41, DB_SQLTYPE_VARCHAR, m_sshPassword, DB_BIND_STATIC, 63);
         DBBind(hStmt, 42, DB_SQLTYPE_INTEGER, m_sshKeyId);
         DBBind(hStmt, 43, DB_SQLTYPE_INTEGER, m_sshPort);
         DBBind(hStmt, 44, DB_SQLTYPE_INTEGER, m_sshProxy);
         DBBind(hStmt, 45, DB_SQLTYPE_INTEGER, m_portRowCount);
         DBBind(hStmt, 46, DB_SQLTYPE_INTEGER, m_portNumberingScheme);
         DBBind(hStmt, 47, DB_SQLTYPE_VARCHAR, IntegerToString(m_agentCompressionMode, compressionMode), DB_BIND_STATIC, 1);
         DBBind(hStmt, 48, DB_SQLTYPE_VARCHAR, m_tunnelId);
         DBBind(hStmt, 49, DB_SQLTYPE_VARCHAR, m_lldpNodeId, DB_BIND_STATIC);
         DBBind(hStmt, 50, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_failTimeSNMP));
         DBBind(hStmt, 51, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_failTimeAgent));
         DBBind(hStmt, 52, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_failTimeSSH));
         DBBind(hStmt, 53, DB_SQLTYPE_INTEGER, m_rackOrientation);
         DBBind(hStmt, 54, DB_SQLTYPE_VARCHAR, m_rackImageRear);
         DBBind(hStmt, 55, DB_SQLTYPE_VARCHAR, m_agentId);
         DBBind(hStmt, 56, DB_SQLTYPE_VARCHAR, m_agentCertSubject, DB_BIND_STATIC);
         DBBind(hStmt, 57, DB_SQLTYPE_VARCHAR, m_hypervisorType, DB_BIND_STATIC);
         DBBind(hStmt, 58, DB_SQLTYPE_VARCHAR, m_hypervisorInfo, DB_BIND_STATIC);
         DBBind(hStmt, 59, DB_SQLTYPE_VARCHAR, icmpPollMode, DB_BIND_STATIC);
         DBBind(hStmt, 60, DB_SQLTYPE_VARCHAR, m_chassisPlacementConf, DB_BIND_STATIC);
         DBBind(hStmt, 61, DB_SQLTYPE_VARCHAR, m_vendor, DB_BIND_STATIC, 127);
         DBBind(hStmt, 62, DB_SQLTYPE_VARCHAR, m_productCode, DB_BIND_STATIC, 31);
         DBBind(hStmt, 63, DB_SQLTYPE_VARCHAR, m_productName, DB_BIND_STATIC, 127);
         DBBind(hStmt, 64, DB_SQLTYPE_VARCHAR, m_productVersion, DB_BIND_STATIC, 15);
         DBBind(hStmt, 65, DB_SQLTYPE_VARCHAR, m_serialNumber, DB_BIND_STATIC, 31);
         DBBind(hStmt, 66, DB_SQLTYPE_INTEGER, m_cipDeviceType);
         DBBind(hStmt, 67, DB_SQLTYPE_INTEGER, m_cipStatus);
         DBBind(hStmt, 68, DB_SQLTYPE_INTEGER, m_cipState);
         DBBind(hStmt, 69, DB_SQLTYPE_INTEGER, m_eipProxy);
         DBBind(hStmt, 70, DB_SQLTYPE_INTEGER, m_eipPort);
         DBBind(hStmt, 71, DB_SQLTYPE_VARCHAR, BinToStr(m_hardwareId.value(), HARDWARE_ID_LENGTH, hardwareId), DB_BIND_STATIC);
         DBBind(hStmt, 72, DB_SQLTYPE_INTEGER, m_cipVendorCode);
         DBBind(hStmt, 73, DB_SQLTYPE_VARCHAR, agentCertMappingMethod, DB_BIND_STATIC);
         DBBind(hStmt, 74, DB_SQLTYPE_VARCHAR, m_agentCertMappingData, DB_BIND_STATIC);
         if (m_snmpSecurity != nullptr)
         {
            DBBind(hStmt, 75, DB_SQLTYPE_VARCHAR, m_snmpSecurity->getAuthoritativeEngine().toString(), DB_BIND_TRANSIENT);
            DBBind(hStmt, 76, DB_SQLTYPE_VARCHAR, m_snmpSecurity->getContextEngine().toString(), DB_BIND_TRANSIENT);
         }
         else
         {
            DBBind(hStmt, 75, DB_SQLTYPE_VARCHAR, _T(""), DB_BIND_STATIC);
            DBBind(hStmt, 76, DB_SQLTYPE_VARCHAR, _T(""), DB_BIND_STATIC);
         }
         DBBind(hStmt, 77, DB_SQLTYPE_VARCHAR, DB_CTYPE_UTF8_STRING, m_syslogCodepage, DB_BIND_STATIC);
         DBBind(hStmt, 78, DB_SQLTYPE_VARCHAR, DB_CTYPE_UTF8_STRING, m_snmpCodepage, DB_BIND_STATIC);
         if (m_ospfRouterId != 0)
            DBBind(hStmt, 79, DB_SQLTYPE_VARCHAR, IpToStr(m_ospfRouterId, routerId), DB_BIND_STATIC);
         else
            DBBind(hStmt, 79, DB_SQLTYPE_VARCHAR, _T(""), DB_BIND_STATIC);
         DBBind(hStmt, 80, DB_SQLTYPE_INTEGER, m_mqttProxy);
         DBBind(hStmt, 81, DB_SQLTYPE_INTEGER, m_modbusProxy);
         DBBind(hStmt, 82, DB_SQLTYPE_INTEGER, m_modbusTcpPort);
         DBBind(hStmt, 83, DB_SQLTYPE_INTEGER, m_modbusUnitId);
         DBBind(hStmt, 84, DB_SQLTYPE_VARCHAR, m_vncPassword, DB_BIND_STATIC, 63);
         DBBind(hStmt, 85, DB_SQLTYPE_INTEGER, m_vncPort);
         DBBind(hStmt, 86, DB_SQLTYPE_INTEGER, m_vncProxy);
         DBBind(hStmt, 87, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_pathCheckResult.reason));
         DBBind(hStmt, 88, DB_SQLTYPE_INTEGER, m_pathCheckResult.rootCauseNodeId);
         DBBind(hStmt, 89, DB_SQLTYPE_INTEGER, m_pathCheckResult.rootCauseInterfaceId);
         DBBind(hStmt, 90, DB_SQLTYPE_BIGINT, m_expectedCapabilities);
         StringList lastEvents;
         for (int i = 0; i < MAX_LAST_EVENTS; i++)
         {
            lastEvents.add(m_lastEvents[i]);
         }
         DBBind(hStmt, 91, DB_SQLTYPE_VARCHAR, lastEvents.join(L";"), DB_BIND_DYNAMIC);
         DBBind(hStmt, 92, DB_SQLTYPE_INTEGER, m_id);

         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);

         if (success)
         {
            m_savedDownSince = m_downSince;
         }

         unlockProperties();
      }
      else
      {
         success = false;
      }
   }

   if (success && (m_modified & MODIFY_COMPONENTS))
   {
      success = executeQueryOnObject(hdb, _T("DELETE FROM node_components WHERE node_id=?"));
      lockProperties();
      if (success && (m_components != nullptr))
      {
         const Component *root = m_components->getRoot();
         if (root != nullptr)
         {
            DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO node_components (node_id,component_index,parent_index,position,component_class,if_index,name,description,model,serial_number,vendor,firmware) VALUES (?,?,?,?,?,?,?,?,?,?,?,?)"));
            if (hStmt != nullptr)
            {
               DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
               success = SaveComponent(hStmt, root);
               DBFreeStatement(hStmt);
            }
            else
            {
               success = false;
            }
         }
      }
      unlockProperties();
   }

   if (success && (m_modified & MODIFY_SOFTWARE_INVENTORY))
   {
      success = executeQueryOnObject(hdb, _T("DELETE FROM software_inventory WHERE node_id=?"));
      lockProperties();
      if ((m_softwarePackages != nullptr) && !m_softwarePackages->isEmpty())
      {
         DB_STATEMENT hStmt = DBPrepare(hdb,
                  _T("INSERT INTO software_inventory (node_id,name,version,vendor,install_date,url,description,uninstall_key) VALUES (?,?,?,?,?,?,?,?)"),
                  m_softwarePackages->size() > 1);
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            for(int i = 0; success && (i < m_softwarePackages->size()); i++)
               success = m_softwarePackages->get(i)->saveToDatabase(hStmt);
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }
      unlockProperties();
   }

   if (success && (m_modified & MODIFY_HARDWARE_INVENTORY))
   {
      success = executeQueryOnObject(hdb, _T("DELETE FROM hardware_inventory WHERE node_id=?"));
      lockProperties();
      if (success && (m_hardwareComponents != nullptr) && !m_hardwareComponents->isEmpty())
      {
         DB_STATEMENT hStmt = DBPrepare(hdb,
                  _T("INSERT INTO hardware_inventory (node_id,category,component_index,hw_type,vendor,model,location,capacity,part_number,serial_number,description) VALUES (?,?,?,?,?,?,?,?,?,?,?)"),
                  m_hardwareComponents->size() > 1);
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            for(int i = 0; success && i < m_hardwareComponents->size(); i++)
               success = m_hardwareComponents->get(i)->saveToDatabase(hStmt);
            DBFreeStatement(hStmt);
         }
      }
      unlockProperties();
   }

   if (success && (m_modified & MODIFY_OSPF_AREAS))
   {
      success = executeQueryOnObject(hdb, _T("DELETE FROM ospf_areas WHERE node_id=?"));
      lockProperties();
      if (success && !m_ospfAreas.isEmpty())
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO ospf_areas (node_id,area_id) VALUES (?,?)"));
         if (hStmt != nullptr)
         {
            TCHAR areaId[16];
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            for(int i = 0; (i < m_ospfAreas.size()) && success; i++)
            {
               DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, IpToStr(m_ospfAreas.get(i)->id, areaId), DB_BIND_STATIC);
               success = DBExecute(hStmt);
            }
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }
      unlockProperties();
   }

   if (success && (m_modified & MODIFY_OSPF_NEIGHBORS))
   {
      success = executeQueryOnObject(hdb, _T("DELETE FROM ospf_neighbors WHERE node_id=?"));
      lockProperties();
      if (success && !m_ospfNeighbors.isEmpty())
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO ospf_neighbors (node_id,router_id,area_id,ip_address,remote_node_id,if_index,is_virtual,neighbor_state) VALUES (?,?,?,?,?,?,?,?)"));
         if (hStmt != nullptr)
         {
            TCHAR routerId[16], areaId[16];
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            for(int i = 0; (i < m_ospfNeighbors.size()) && success; i++)
            {
               OSPFNeighbor *n = m_ospfNeighbors.get(i);
               DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, IpToStr(n->routerId, routerId), DB_BIND_STATIC);
               DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, n->isVirtual ? IpToStr(n->areaId, areaId) : nullptr, DB_BIND_STATIC);
               DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, n->ipAddress);
               DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, n->nodeId);
               DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, n->ifIndex);
               DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, n->isVirtual ? _T("1") : _T("0"), DB_BIND_STATIC);
               DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, static_cast<int32_t>(n->state));
               success = DBExecute(hStmt);
            }
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }
      unlockProperties();
   }

   // Save ICMP pollers
   if (success && (m_modified & MODIFY_ICMP_POLL_SETTINGS))
   {
      lockProperties();

      success = executeQueryOnObject(hdb, L"DELETE FROM icmp_statistics WHERE object_id=?");
      if (success && isIcmpStatCollectionEnabled() && (m_icmpStatCollectors != nullptr) && !m_icmpStatCollectors->isEmpty())
      {
         success = (m_icmpStatCollectors->forEach(
            [hdb, this] (const TCHAR *target, IcmpStatCollector *collector) -> EnumerationCallbackResult
            {
               return collector->saveToDatabase(hdb, m_id, target) ? _CONTINUE : _STOP;
            }) == _CONTINUE);
      }

      if (success)
         success = executeQueryOnObject(hdb, L"DELETE FROM icmp_target_address_list WHERE node_id=?");

      if (success && !m_icmpTargets.isEmpty())
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, L"INSERT INTO icmp_target_address_list (node_id,ip_addr) VALUES (?,?)", m_icmpTargets.size() > 1);
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            for(int i = 0; success && i < m_icmpTargets.size(); i++)
            {
               DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_icmpTargets.get(i));
               success = DBExecute(hStmt);
            }
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }

      unlockProperties();
   }

   if (success && (m_modified & MODIFY_RADIO_INTERFACES))
   {
      success = executeQueryOnObject(hdb, _T("DELETE FROM radios WHERE owner_id=?"));
      lockProperties();
      if (success && (m_radioInterfaces != nullptr) && !m_radioInterfaces->isEmpty())
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO radios (owner_id,radio_index,if_index,name,bssid,ssid,frequency,band,channel,power_dbm,power_mw) VALUES (?,?,?,?,?,?,?,?,?,?,?)"));
         if (hStmt != nullptr)
         {
            TCHAR bssid[16];
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            for(int i = 0; (i < m_radioInterfaces->size()) && success; i++)
            {
               RadioInterfaceInfo *rif = m_radioInterfaces->get(i);
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, rif->index);
               DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, rif->ifIndex);
               DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, rif->name, DB_BIND_STATIC);
               DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, BinToStr(rif->bssid, MAC_ADDR_LENGTH, bssid), DB_BIND_STATIC);
               DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, rif->ssid, DB_BIND_STATIC);
               DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, rif->frequency);
               DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, rif->band);
               DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, rif->channel);
               DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, rif->powerDBm);
               DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, rif->powerMW);
               success = DBExecute(hStmt);
            }
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }
      unlockProperties();
   }

   return success;
}

/**
 * Save runtime data to database. Called only on server shutdown to save
 * less important but frequently changing runtime data when it is not feasible
 * to mark object as modified on each change of such data.
 */
bool Node::saveRuntimeData(DB_HANDLE hdb)
{
   if (!super::saveRuntimeData(hdb))
      return false;

   lockProperties();
   if (isIcmpStatCollectionEnabled() && (m_icmpStatCollectors != nullptr) && !m_icmpStatCollectors->isEmpty())
   {
      if (m_icmpStatCollectors->forEach(
         [hdb, this] (const TCHAR *target, IcmpStatCollector *collector) -> EnumerationCallbackResult
         {
            return collector->saveToDatabase(hdb, m_id, target) ? _CONTINUE : _STOP;
         }) == _STOP)
      {
         unlockProperties();
         return false;
      }
   }

   if ((m_lastAgentCommTime == TIMESTAMP_NEVER) && (m_syslogMessageCount == 0) && (m_snmpTrapCount == 0) && (m_snmpSecurity == nullptr) && (m_downSince == m_savedDownSince))
   {
      unlockProperties();
      return true;
   }

   unlockProperties();

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("UPDATE nodes SET last_agent_comm_time=?,syslog_msg_count=?,snmp_trap_count=?,snmp_engine_id=?,snmp_context_engine_id=?,down_since=? WHERE id=?"));
   if (hStmt == nullptr)
      return false;

   lockProperties();
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_lastAgentCommTime));
   DBBind(hStmt, 2, DB_SQLTYPE_BIGINT, m_syslogMessageCount);
   DBBind(hStmt, 3, DB_SQLTYPE_BIGINT, m_snmpTrapCount);
   if (m_snmpSecurity != nullptr)
   {
      DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_snmpSecurity->getAuthoritativeEngine().toString(), DB_BIND_TRANSIENT);
      DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, m_snmpSecurity->getContextEngine().toString(), DB_BIND_TRANSIENT);
   }
   else
   {
      DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, _T(""), DB_BIND_STATIC);
      DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, _T(""), DB_BIND_STATIC);
   }
   DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_downSince));
   DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_id);
   m_savedDownSince = m_downSince;
   unlockProperties();

   bool success = DBExecute(hStmt);
   DBFreeStatement(hStmt);

   return success;
}

/**
 * Delete object from database
 */
bool Node::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM nodes WHERE id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM nsmap WHERE node_id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM icmp_statistics WHERE object_id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM icmp_target_address_list WHERE node_id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM software_inventory WHERE node_id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM hardware_inventory WHERE node_id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM node_components WHERE node_id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM ospf_areas WHERE node_id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM ospf_neighbors WHERE node_id=?"));
   return success;
}

/**
 * Find MAC address in node's ARP cache. If there is no valid cached version of ARP cache,
 * attempt to read single ARP record instead of retrieving full ARP cache.
 */
MacAddress Node::findMacAddressInArpCache(const InetAddress& ipAddr)
{
   if (ipAddr.getFamily() != AF_INET)
      return MacAddress::NONE;

   TCHAR ipAddrText[64];
   ipAddr.toString(ipAddrText);
   nxlog_debug_tag(DEBUG_TAG_TOPO_ARP, 6, _T("Node::findMacAddressInArpCache(%s [%u]): Search MAC address for IP address %s"), m_name, m_id, ipAddrText);

   shared_ptr<ArpCache> arpCache;
   lockProperties();
   if ((m_arpCache != nullptr) && (m_arpCache->timestamp() > time(nullptr) - 3600))
   {
      arpCache = m_arpCache;
   }
   unlockProperties();
   if (arpCache != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_TOPO_ARP, 6, _T("Node::findMacAddressInArpCache(%s [%u]): using cached ARP table"), m_name, m_id);
      const ArpEntry *e = arpCache->findByIP(ipAddr);
      if (e != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_TOPO_ARP, 6, _T("Node::findMacAddressInArpCache(%s [%u]): MAC address %s for IP address %s found in cached ARP table"),
               m_name, m_id, e->macAddr.toString().cstr(), ipAddrText);
         return e->macAddr;
      }
      return MacAddress::NONE;
   }

   MacAddress macAddr;
   if (isSNMPSupported())
   {
      // Find matching interface
      shared_ptr<Interface> iface = findInterfaceInSameSubnet(ipAddr);
      if (iface != nullptr)
      {
         SNMP_Transport *snmp = createSnmpTransport();
         if (snmp != nullptr)
         {
            uint32_t oid[] = { 1, 3, 6, 1, 2, 1, 4, 22, 1, 2, 0, 0, 0, 0, 0 };
            oid[10] = iface->getIfIndex();
            ipAddr.toOID(&oid[11]);

            SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
            request.bindVariable(new SNMP_Variable(oid, sizeof(oid) / sizeof(uint32_t)));

            SNMP_PDU *response;
            if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
            {
               SNMP_Variable *v = response->getVariable(0);
               if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
               {
                  macAddr = v->getValueAsMACAddr();
                  nxlog_debug_tag(DEBUG_TAG_TOPO_ARP, 6, _T("Node::findMacAddressInArpCache(%s [%u]): found MAC address %s  for IP address %s using direct SNMP request"),
                        m_name, m_id, macAddr.toString().cstr(), ipAddrText);
               }
               delete response;
            }
            delete snmp;
         }
      }
   }
   else if (isNativeAgent())
   {
      arpCache = getArpCache();
      if (arpCache != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_TOPO_ARP, 6, _T("Node::findMacAddressInArpCache(%s [%u]): ARP cache retrieved from node"), m_name, m_id);
         const ArpEntry *e = arpCache->findByIP(ipAddr);
         if (e != nullptr)
         {
            nxlog_debug_tag(DEBUG_TAG_TOPO_ARP, 6, _T("Node::findMacAddressInArpCache(%s [%u]): MAC address %s for IP address %s found in retrieved ARP table"),
                  m_name, m_id, e->macAddr.toString().cstr(), ipAddrText);
            return e->macAddr;
         }
         return MacAddress::NONE;
      }
   }

   return macAddr;
}

/**
 * Get ARP cache from node
 */
shared_ptr<ArpCache> Node::getArpCache(bool forceRead)
{
   nxlog_debug_tag(DEBUG_TAG_TOPO_ARP, 6, _T("Requested ARP cache for node %s [%u] (forceRead=%s)"), m_name, m_id, BooleanToString(forceRead));

   shared_ptr<ArpCache> arpCache;
   if (!forceRead)
   {
      lockProperties();
      if ((m_arpCache != nullptr) && (m_arpCache->timestamp() > time(nullptr) - 3600))
      {
         arpCache = m_arpCache;
      }
      unlockProperties();
      if (arpCache != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_TOPO_ARP, 6, _T("Valid ARP cache found for node %s [%u] (%d entries)"), m_name, m_id, arpCache->size());
         return arpCache;
      }
   }

   if (m_capabilities & NC_IS_LOCAL_MGMT)
   {
      nxlog_debug_tag(DEBUG_TAG_TOPO_ARP, 6, _T("Reading local ARP cache for node %s [%u]"), m_name, m_id);
      arpCache = GetLocalArpCache();
   }
   else if (m_capabilities & NC_IS_NATIVE_AGENT)
   {
      shared_ptr<AgentConnectionEx> conn = getAgentConnection();
      if (conn != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_TOPO_ARP, 6, _T("Reading ARP cache for node %s [%u] via agent"), m_name, m_id);
         arpCache = conn->getArpCache();
      }
   }
   else if (m_capabilities & NC_IS_SNMP)
   {
      SNMP_Transport *transport = createSnmpTransport();
      if (transport != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_TOPO_ARP, 6, _T("Reading ARP cache for node %s [%u] via SNMP"), m_name, m_id);
         arpCache = m_driver->getArpCache(transport, m_driverData);
         delete transport;
      }
   }

   if (arpCache != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_TOPO_ARP, 6, _T("Read ARP cache from node %s [%u] (%d entries)"), m_name, m_id, arpCache->size());
      arpCache->dumpToLog();

      lockProperties();
      m_arpCache = arpCache;
      unlockProperties();
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_TOPO_ARP, 6, _T("Cannot read ARP cache from node %s [%u]"), m_name, m_id);
   }
   return arpCache;
}

/**
 * Get list of interfaces from node
 * @return Dynamically allocated list of interfaces. Must be freed by the caller.
 */
InterfaceList *Node::getInterfaceList()
{
   InterfaceList *ifList = nullptr;

   if ((m_capabilities & NC_IS_NATIVE_AGENT) && (!(m_flags & NF_DISABLE_NXCP)))
   {
      shared_ptr<AgentConnectionEx> conn = getAgentConnection();
      if (conn != nullptr)
      {
         ifList = conn->getInterfaceList();
      }
   }
   if ((ifList == nullptr) && (m_capabilities & NC_IS_LOCAL_MGMT))
   {
      ifList = GetLocalInterfaceList();
   }
   if ((ifList == nullptr) && (m_capabilities & NC_IS_SNMP) && !(m_flags & NF_DISABLE_SNMP))
   {
      SNMP_Transport *snmpTransport = createSnmpTransport();
      if (snmpTransport != nullptr)
      {
         bool useIfXTable;
         if (m_nUseIfXTable == IFXTABLE_DEFAULT)
         {
            useIfXTable = ConfigReadBoolean(_T("Objects.Interfaces.UseIfXTable"), true);
         }
         else
         {
            useIfXTable = (m_nUseIfXTable == IFXTABLE_ENABLED);
         }

         nxlog_debug_tag(DEBUG_TAG_NODE_INTERFACES, 6, L"Node::getInterfaceList(node=%s [%u]): calling driver (useIfXTable=%s)",
                  m_name, m_id, BooleanToString(useIfXTable));
         ifList = m_driver->getInterfaces(snmpTransport, this, m_driverData, useIfXTable);
         if (ifList != nullptr)
         {
            if (ConfigReadBoolean(L"Objects.Interfaces.IgnoreIfNotPresent", false))
            {
               uint32_t oid[11] = { 1, 3, 6, 1, 2, 1, 2, 2, 1, 8, 0 };
               for(int i = 0; i < ifList->size(); i++)
               {
                  InterfaceInfo *ifInfo = ifList->get(i);
                  oid[10] = ifInfo->index;
                  uint32_t state = 0;
                  if (SnmpGetEx(snmpTransport, nullptr, oid, 11, &state, sizeof(uint32_t), 0) == SNMP_ERR_SUCCESS)
                  {
                     if (state == 6) // not present
                     {
                        sendPollerMsg(_T("   Interface \"%s\" (ifIndex = %u) ignored because it is in NOT PRESENT state\r\n"), ifInfo->name, ifInfo->index);
                        nxlog_debug_tag(DEBUG_TAG_NODE_INTERFACES, 6, _T("Node::getInterfaceList(node=%s [%u]): interface \"%s\" ifIndex=%u ignored because it is in NOT PRESENT state"),
                           m_name, m_id, ifInfo->name, ifInfo->index);
                        ifList->remove(i);
                        i--;
                     }
                  }
               }
            }

            if (m_capabilities & NC_IS_BRIDGE)
            {
               // Map port numbers from dot1dBasePortTable to interface indexes
               StructArray<BridgePort> *bridgePorts = m_driver->getBridgePorts(snmpTransport, this, m_driverData);
               if (bridgePorts != nullptr)
               {
                  for(int i = 0; i < bridgePorts->size(); i++)
                  {
                     BridgePort *p = bridgePorts->get(i);
                     InterfaceInfo *ifInfo = ifList->findByIfIndex(p->ifIndex);
                     if (ifInfo != nullptr)
                        ifInfo->bridgePort = p->portNumber;
                  }
                  delete bridgePorts;
               }
            }
         }

         delete snmpTransport;
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_NODE_INTERFACES, 6, _T("Node::getInterfaceList(node=%s [%u]): cannot create SNMP transport"), m_name, m_id);
      }
   }

   if (ifList != nullptr)
   {
      checkInterfaceNames(ifList);
      addVrrpInterfaces(ifList);
   }

   return ifList;
}

/**
 * Add VRRP interfaces to interface list
 */
void Node::addVrrpInterfaces(InterfaceList *ifList)
{
   int i, j, k;
   TCHAR buffer[32];

   lockProperties();
   if (m_vrrpInfo != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_NODE_INTERFACES, 6, _T("Node::addVrrpInterfaces(node=%s [%d]): m_vrrpInfo->size()=%d"), m_name, (int)m_id, m_vrrpInfo->size());

      for(i = 0; i < m_vrrpInfo->size(); i++)
      {
         VrrpRouter *router = m_vrrpInfo->getRouter(i);
         nxlog_debug_tag(DEBUG_TAG_NODE_INTERFACES, 6, _T("Node::addVrrpInterfaces(node=%s [%u]): vrouter %d state=%d"), m_name, m_id, i, router->getState());
         if (router->getState() != VRRP_STATE_MASTER)
            continue;   // Do not add interfaces if router is not in master state

         // Get netmask for this VR
         int maskBits = 0;
         for(j = 0; j < ifList->size(); j++)
         {
            InterfaceInfo *iface = ifList->get(j);
            if (iface->index == router->getIfIndex())
            {
               for(int k = 0; k < iface->ipAddrList.size(); k++)
               {
                  const InetAddress& addr = iface->ipAddrList.get(k);
                  if (addr.getSubnetAddress().contains(router->getVip(0)))
                  {
                     maskBits = addr.getMaskBits();
                  }
               }
               break;
            }
         }

         // Walk through all VR virtual IPs
         for(j = 0; j < router->getVipCount(); j++)
         {
            uint32_t vip = router->getVip(j);
            nxlog_debug_tag(DEBUG_TAG_NODE_INTERFACES, 6, _T("Node::addVrrpInterfaces(node=%s [%u]): checking VIP %s@%d"), m_name, m_id, IpToStr(vip, buffer), i);
            if (vip != 0)
            {
               for(k = 0; k < ifList->size(); k++)
                  if (ifList->get(k)->hasAddress(vip))
                     break;
               if (k == ifList->size())
               {
                  InterfaceInfo *iface = new InterfaceInfo(0);
                  _sntprintf(iface->name, MAX_DB_STRING, _T("vrrp.%u.%u.%d"), router->getId(), router->getIfIndex(), j);
                  memcpy(iface->macAddr, router->getVirtualMacAddr(), MAC_ADDR_LENGTH);
                  InetAddress addr(vip);
                  addr.setMaskBits(maskBits);
                  iface->ipAddrList.add(addr);
                  ifList->add(iface);
                  nxlog_debug_tag(DEBUG_TAG_NODE_INTERFACES, 6, _T("Node::addVrrpInterfaces(node=%s [%u]): added interface %s"), m_name, m_id, iface->name);
               }
            }
         }
      }
   }
   unlockProperties();
}

/**
 * Find interface by index.
 *
 * @param ifIndex interface index to match
 * @return pointer to interface object or nullptr if appropriate interface couldn't be found
 */
shared_ptr<Interface> Node::findInterfaceByIndex(uint32_t ifIndex) const
{
   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
      if (getChildList().get(i)->getObjectClass() == OBJECT_INTERFACE)
      {
         auto iface = static_pointer_cast<Interface>(getChildList().getShared(i));
         if (iface->getIfIndex() == ifIndex)
         {
            unlockChildList();
            return iface;
         }
      }
   unlockChildList();
   return shared_ptr<Interface>();
}

/**
 * Find interface by name or description
 * Returns pointer to interface object or nullptr if appropriate interface couldn't be found
 */
shared_ptr<Interface> Node::findInterfaceByName(const TCHAR *name) const
{
   if ((name == nullptr) || (name[0] == 0))
      return shared_ptr<Interface>();

   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
      if (getChildList().get(i)->getObjectClass() == OBJECT_INTERFACE)
      {
         auto iface = static_pointer_cast<Interface>(getChildList().getShared(i));
         if (!_tcsicmp(iface->getIfName(), name) || !_tcsicmp(iface->getDescription(), name) || !_tcsicmp(iface->getName(), name))
         {
            unlockChildList();
            return iface;
         }
      }
   unlockChildList();
   return shared_ptr<Interface>();
}

/**
 * Find interface by alias
 * Returns pointer to interface object or nullptr if appropriate interface couldn't be found
 */
shared_ptr<Interface> Node::findInterfaceByAlias(const TCHAR *alias) const
{
   if ((alias == nullptr) || (alias[0] == 0))
      return shared_ptr<Interface>();

   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
      if (getChildList().get(i)->getObjectClass() == OBJECT_INTERFACE)
      {
         auto iface = static_pointer_cast<Interface>(getChildList().getShared(i));
         if (!_tcsicmp(iface->getAlias(), alias))
         {
            unlockChildList();
            return iface;
         }
      }
   unlockChildList();
   return shared_ptr<Interface>();
}

/**
 * Find interface by physical location
 * Returns pointer to interface object or nullptr if appropriate interface couldn't be found
 */
shared_ptr<Interface> Node::findInterfaceByLocation(const InterfacePhysicalLocation& location) const
{
   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
      if (getChildList().get(i)->getObjectClass() == OBJECT_INTERFACE)
      {
         auto iface = static_pointer_cast<Interface>(getChildList().getShared(i));
         if (iface->isPhysicalPort() && iface->getPhysicalLocation().equals(location))
         {
            unlockChildList();
            return iface;
         }
      }
   unlockChildList();
   return shared_ptr<Interface>();
}

/**
 * Find interface by MAC address
 * Returns pointer to interface object or nullptr if appropriate interface couldn't be found
 */
shared_ptr<Interface> Node::findInterfaceByMAC(const MacAddress& macAddr) const
{
   shared_ptr<Interface> iface;
   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *curr = getChildList().get(i);
      if ((curr->getObjectClass() == OBJECT_INTERFACE) &&
          static_cast<Interface*>(curr)->getMacAddress().equals(macAddr))
      {
         iface = static_pointer_cast<Interface>(getChildList().getShared(i));
         break;
      }
   }
   unlockChildList();
   return iface;
}

/**
 * Find interface by IP address
 * Returns pointer to interface object or nullptr if appropriate interface couldn't be found
 */
shared_ptr<Interface> Node::findInterfaceByIP(const InetAddress& addr) const
{
   shared_ptr<Interface> iface;

   if (!addr.isValid())
      return iface;

   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *curr = getChildList().get(i);
      if ((curr->getObjectClass() == OBJECT_INTERFACE) &&
          static_cast<Interface*>(curr)->getIpAddressList()->hasAddress(addr))
      {
         iface = static_pointer_cast<Interface>(getChildList().getShared(i));
         break;
      }
   }
   unlockChildList();
   return iface;
}

/**
 * Find interface by IP subnet
 * Returns pointer to interface object or nullptr if appropriate interface couldn't be found
 */
shared_ptr<Interface> Node::findInterfaceBySubnet(const InetAddress& subnet) const
{
   shared_ptr<Interface> iface;
   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *curr = getChildList().get(i);
      if (curr->getObjectClass() != OBJECT_INTERFACE)
         continue;

      const InetAddressList *addrList = static_cast<Interface*>(curr)->getIpAddressList();
      for(int j = 0; j < addrList->size(); j++)
      {
         if (subnet.contains(addrList->get(j)))
         {
            iface = static_pointer_cast<Interface>(getChildList().getShared(i));
            goto stop_search;
         }
      }
   }
stop_search:
   unlockChildList();
   return iface;
}

/**
 * Find interface on this node that is in same subnet as given interface (using mask bits from interface object on node).
 * Returns pointer to interface object or nullptr if appropriate interface couldn't be found
 */
shared_ptr<Interface> Node::findInterfaceInSameSubnet(const InetAddress& addr) const
{
   shared_ptr<Interface> iface;
   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *curr = getChildList().get(i);
      if (curr->getObjectClass() != OBJECT_INTERFACE)
         continue;

      const InetAddressList *addrList = static_cast<Interface*>(curr)->getIpAddressList();
      for(int j = 0; j < addrList->size(); j++)
      {
         if (addrList->get(j).sameSubnet(addr))
         {
            iface = static_pointer_cast<Interface>(getChildList().getShared(i));
            goto stop_search;
         }
      }
   }
stop_search:
   unlockChildList();
   return iface;
}

/**
 * Find interface by bridge port number
 */
shared_ptr<Interface> Node::findBridgePort(uint32_t bridgePortNumber) const
{
   shared_ptr<Interface> iface;
   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *curr = getChildList().get(i);
      if ((curr->getObjectClass() == OBJECT_INTERFACE) && (static_cast<Interface*>(curr)->getBridgePortNumber() == bridgePortNumber))
      {
         iface = static_pointer_cast<Interface>(getChildList().getShared(i));
         break;
      }
   }
   unlockChildList();
   return iface;
}

/**
 * Find connection point for node
 */
shared_ptr<NetObj> Node::findConnectionPoint(UINT32 *localIfId, BYTE *localMacAddr, int *type)
{
   shared_ptr<NetObj> cp;
   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      if (getChildList().get(i)->getObjectClass() == OBJECT_INTERFACE)
      {
         auto iface = static_cast<Interface*>(getChildList().get(i));
         MacAddress macAddress = iface->getMacAddress();
         cp = FindInterfaceConnectionPoint(macAddress, type);
         if (cp != nullptr)
         {
            *localIfId = iface->getId();
            memcpy(localMacAddr, macAddress.value(), MAC_ADDR_LENGTH);
            break;
         }
      }
   }
   unlockChildList();
   return cp;
}

/**
 * Check if given IP address is one of node's interfaces
 */
bool Node::isMyIP(const InetAddress& addr) const
{
   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *curr = getChildList().get(i);
      if ((curr->getObjectClass() == OBJECT_INTERFACE) &&
          static_cast<Interface*>(curr)->getIpAddressList()->hasAddress(addr))
      {
         unlockChildList();
         return true;
      }
   }
   unlockChildList();
   return false;
}

/**
 * Build interface object name based on alias usage settings and actual interface information
 */
static void BuildInterfaceObjectName(TCHAR *ifObjectName, const InterfaceInfo& ifInfo, int useAliases)
{
   // Use interface description if name is not set
   const TCHAR *ifName = (ifInfo.name[0] != 0) ? ifInfo.name : ifInfo.description;

   switch(useAliases)
   {
      case 1:  // Use only alias if available, otherwise name
         _tcslcpy(ifObjectName, (ifInfo.alias[0] != 0) ? ifInfo.alias : ifName, MAX_OBJECT_NAME);
         break;
      case 2:  // Concatenate alias with name
         if (ifInfo.alias[0] != 0)
         {
            _sntprintf(ifObjectName, MAX_OBJECT_NAME, _T("%s (%s)"), ifInfo.alias, ifName);
            ifObjectName[MAX_OBJECT_NAME - 1] = 0;
         }
         else
         {
            _tcslcpy(ifObjectName, ifName, MAX_OBJECT_NAME);
         }
         break;
      case 3:  // Concatenate name with alias
         if (ifInfo.alias[0] != 0)
         {
            _sntprintf(ifObjectName, MAX_OBJECT_NAME, _T("%s (%s)"), ifName, ifInfo.alias);
            ifObjectName[MAX_OBJECT_NAME - 1] = 0;
         }
         else
         {
            _tcslcpy(ifObjectName, ifName, MAX_OBJECT_NAME);
         }
         break;
      default: // Do not use alias
         _tcslcpy(ifObjectName, ifName, MAX_OBJECT_NAME);
         break;
   }
}

/**
 * Create interface object. Can return nullptr if interface creation hook
 * blocks interface creation.
 */
shared_ptr<Interface> Node::createInterfaceObject(const InterfaceInfo& info, bool manuallyCreated, bool fakeInterface, bool syntheticMask)
{
   shared_ptr<Interface> iface;
   if (info.name[0] != 0)
   {
      TCHAR ifObjectName[MAX_OBJECT_NAME];
      BuildInterfaceObjectName(ifObjectName, info, ConfigReadInt(_T("Objects.Interfaces.UseAliases"), 0));
      iface = make_shared<Interface>(ifObjectName, info.name, (info.description[0] != 0) ? info.description : info.name,
               info.index, info.ipAddrList, info.type, m_zoneUIN);
   }
   else
   {
      iface = make_shared<Interface>(info.ipAddrList, m_zoneUIN, syntheticMask);
   }
   iface->setAlias(info.alias);
   iface->setIfAlias(info.alias);
   iface->setMacAddress(MacAddress(info.macAddr, MAC_ADDR_LENGTH), false);
   iface->setBridgePortNumber(info.bridgePort);
   iface->setPhysicalLocation(info.location);
   iface->setPhysicalPortFlag(info.isPhysicalPort);
   iface->setManualCreationFlag(manuallyCreated);
   iface->setMTU(info.mtu);
   iface->setSpeed(info.speed);
   iface->setIfTableSuffix(info.ifTableSuffixLength, info.ifTableSuffix);

   // Expand interface name
   TCHAR expandedName[MAX_OBJECT_NAME];
   iface->expandName(iface->getName(), expandedName);
   iface->setName(expandedName);

   int defaultExpectedState = ConfigReadInt(_T("Objects.Interfaces.DefaultExpectedState"), IF_DEFAULT_EXPECTED_STATE_UP);
   switch(defaultExpectedState)
   {
      case IF_DEFAULT_EXPECTED_STATE_AUTO:
         iface->setExpectedState(fakeInterface ? IF_EXPECTED_STATE_UP : IF_EXPECTED_STATE_AUTO);
         break;
      case IF_DEFAULT_EXPECTED_STATE_IGNORE:
         iface->setExpectedState(IF_EXPECTED_STATE_IGNORE);
         break;
      default:
         iface->setExpectedState(IF_EXPECTED_STATE_UP);
         break;
   }

   // Call hook script if interface is automatically created
   if (!manuallyCreated)
   {
      ScriptVMHandle vm = CreateServerScriptVM(_T("Hook::CreateInterface"), self());
      if (!vm.isValid())
      {
         nxlog_debug_tag(DEBUG_TAG_NODE_INTERFACES, 7, _T("Node::createInterfaceObject(%s [%u]): hook script \"Hook::CreateInterface\" %s"), m_name, m_id, HookScriptLoadErrorToText(vm.failureReason()));
         return iface;
      }

      bool pass;
      vm->setUserData(this);
      NXSL_Value *argv = iface->createNXSLObject(vm);
      if (vm->run(1, &argv))
      {
         pass = vm->getResult()->getValueAsBoolean();
      }
      else
      {
         pass = false;  // Consider hook runtime error as blocking
         ReportScriptError(SCRIPT_CONTEXT_OBJECT, this, 0, vm->getErrorText(), _T("Hook::CreateInterface"));
         nxlog_debug_tag(DEBUG_TAG_NODE_INTERFACES, 4, _T("Node::createInterfaceObject(%s [%u]): hook script execution error: %s"), m_name, m_id, vm->getErrorText());
         sendPollerMsg(POLLER_ERROR _T("   Runtime error in interface creation hook script for interface \"%s\" (%s)\r\n"), iface->getName(), vm->getErrorText());
      }
      vm.destroy();
      nxlog_debug_tag(DEBUG_TAG_NODE_INTERFACES, 6, _T("Node::createInterfaceObject(%s [%u]): interface \"%s\" (ifIndex=%d) %s by filter"),
                m_name, m_id, iface->getName(), info.index, pass ? _T("accepted") : _T("rejected"));
      if (!pass)
      {
         sendPollerMsg(POLLER_WARNING _T("   Creation of interface object \"%s\" blocked by filter\r\n"), iface->getName());
         iface.reset();
      }
   }
   return iface;
}

/**
 * Create new interface - convenience wrapper
 */
shared_ptr<Interface> Node::createNewInterface(const InetAddress& ipAddr, const MacAddress& macAddr, bool fakeInterface)
{
   InterfaceInfo info(1);
   info.ipAddrList.add(ipAddr);
   if (macAddr.isValid() && (macAddr.length() == MAC_ADDR_LENGTH))
      memcpy(info.macAddr, macAddr.value(), MAC_ADDR_LENGTH);
   return createNewInterface(&info, false, fakeInterface);
}

/**
 * Create new interface
 */
shared_ptr<Interface> Node::createNewInterface(InterfaceInfo *info, bool manuallyCreated, bool fakeInterface)
{
   bool bSyntheticMask = false;
   TCHAR buffer[64];

   nxlog_debug_tag(DEBUG_TAG_NODE_INTERFACES, 5, _T("Node::createNewInterface(\"%s\", ifIndex=%u, ifType=%u, bp=%d, chassis=%u, module=%u, pic=%u, port=%u) called for node %s [%u]"),
         info->name, info->index, info->type, info->bridgePort, info->location.chassis, info->location.module,
         info->location.pic, info->location.port, m_name, m_id);
   for(int i = 0; i < info->ipAddrList.size(); i++)
   {
      const InetAddress& addr = info->ipAddrList.get(i);
      nxlog_debug_tag(DEBUG_TAG_NODE_INTERFACES, 5, _T("Node::createNewInterface(%s): IP address %s/%d"), info->name, addr.toString(buffer), addr.getMaskBits());
   }

   SharedObjectArray<Subnet> bindList;
   InetAddressList createList;

   // Find subnet(s) to place this node to
   if (info->type != IFTYPE_SOFTWARE_LOOPBACK)
   {
      shared_ptr<Cluster> cluster = getCluster();
      for(int i = 0; i < info->ipAddrList.size(); i++)
      {
         InetAddress addr = info->ipAddrList.get(i);
         bool addToSubnet = addr.isValidUnicast() && ((cluster == nullptr) || !cluster->isSyncAddr(addr));
         nxlog_debug_tag(DEBUG_TAG_NODE_INTERFACES, 5, _T("Node::createNewInterface: node=%s [%d] ip=%s/%d cluster=%s [%d] add=%s"),
                   m_name, m_id, addr.toString(buffer), addr.getMaskBits(),
                   (cluster != nullptr) ? cluster->getName() : _T("(null)"),
                   (cluster != nullptr) ? cluster->getId() : 0, BooleanToString(addToSubnet));
         if (addToSubnet)
         {
            shared_ptr<Subnet> pSubnet = FindSubnetForNode(m_zoneUIN, addr);
            if (pSubnet == nullptr)
            {
               // Check if netmask is 0 (detect), and if yes, create
               // new subnet with default mask
               if (addr.getMaskBits() == 0)
               {
                  bSyntheticMask = true;
                  addr.setMaskBits((addr.getFamily() == AF_INET) ? ConfigReadInt(_T("Objects.Subnets.DefaultSubnetMaskIPv4"), 24) : ConfigReadInt(_T("Objects.Subnets.DefaultSubnetMaskIPv6"), 64));
                  info->ipAddrList.replace(addr);
               }

               // Create new subnet object
               if (addr.getHostBits() > 1)
               {
                  if (AdjustSubnetBaseAddress(addr, m_zoneUIN))
                  {
                     info->ipAddrList.replace(addr);  // mask could be adjusted
                     createList.add(addr);
                  }
               }
            }
            else
            {
               // Set correct netmask if we were asked for it
               if (addr.getMaskBits() == 0)
               {
                  bSyntheticMask = pSubnet->isSyntheticMask();
                  addr.setMaskBits(pSubnet->getIpAddress().getMaskBits());
                  info->ipAddrList.replace(addr);
               }
            }
            if (pSubnet != nullptr)
            {
               bindList.add(pSubnet);
            }
         }  // addToSubnet
      } // loop by address list
   }

   // Insert to objects' list and generate event
   shared_ptr<Interface>  iface = createInterfaceObject(*info, manuallyCreated, fakeInterface, bSyntheticMask);
   if (iface == nullptr)
      return iface;

   NetObjInsert(iface, true, false);
   linkObjects(self(), iface);
   if (!m_isHidden)
      iface->unhide();

   const InetAddress& addr = iface->getFirstIpAddress();
   EventBuilder(EVENT_INTERFACE_ADDED, m_id)
      .param(_T("interfaceObjectId"), iface->getId())
      .param(_T("interfaceName"), iface->getName())
      .param(_T("interfaceIpAddress"), addr)
      .param(_T("interfaceNetmask"), addr.getMaskBits())
      .param(_T("interfaceIndex"), iface->getIfIndex())
      .post();

   if (!iface->isExcludedFromTopology())
   {
      for(int i = 0; i < bindList.size(); i++)
         bindList.get(i)->addNode(self());

      for(int i = 0; i < createList.size(); i++)
      {
         InetAddress addr = InetAddress(createList.get(i));
         createSubnet(addr, bSyntheticMask);
      }
   }
   return iface;
}

/**
 * Delete interface from node
 */
void Node::deleteInterface(Interface *iface)
{
   nxlog_debug_tag(DEBUG_TAG_NODE_INTERFACES, 5, _T("Node::deleteInterface(node=%s [%u], interface=%s [%u])"), m_name, m_id, iface->getName(), iface->getId());

   // Check if we should unlink node from interface's subnet
   if (!iface->isExcludedFromTopology())
   {
      const ObjectArray<InetAddress>& list = iface->getIpAddressList()->getList();
      for(int i = 0; i < list.size(); i++)
      {
         bool doUnlink = true;
         const InetAddress *addr = list.get(i);

         readLockChildList();
         for(int j = 0; j < getChildList().size(); j++)
         {
            NetObj *curr = getChildList().get(j);
            if ((curr->getObjectClass() == OBJECT_INTERFACE) && (curr != iface) &&
                static_cast<Interface*>(curr)->getIpAddressList()->findSameSubnetAddress(*addr).isValid())
            {
               doUnlink = false;
               break;
            }
         }
         unlockChildList();

         if (doUnlink)
         {
            // Last interface in subnet, should unlink node
            shared_ptr<Subnet> subnet = FindSubnetByIP(m_zoneUIN, addr->getSubnetAddress());
            if (subnet != nullptr)
            {
               unlinkObjects(subnet.get(), this);
            }
            nxlog_debug_tag(DEBUG_TAG_NODE_INTERFACES, 5, _T("Node::deleteInterface(node=%s [%u], interface=%s [%u]): unlinked from subnet %s [%u]"),
                      m_name, m_id, iface->getName(), iface->getId(),
                      (subnet != nullptr) ? subnet->getName() : _T("(null)"),
                      (subnet != nullptr) ? subnet->getId() : 0);
         }
      }
   }
   iface->deleteObject();
}

/**
 * Set object's management status
 */
void Node::onMgmtStatusChange(bool isManaged, int oldStatus)
{
   // Generate event if current object is a node
   EventBuilder(isManaged ? EVENT_NODE_UNKNOWN : EVENT_NODE_UNMANAGED, m_id)
      .param(_T("previousNodeStatus"), oldStatus)
      .post();

   if (IsZoningEnabled())
   {
      shared_ptr<Zone> zone = FindZoneByProxyId(m_id);
      if (zone != nullptr)
      {
         zone->updateProxyStatus(self(), true);
      }
   }

   // call to onDataCollectionChange will force data collection
   // configuration upload to this node or relevant proxy nodes
   onDataCollectionChange();

   CALL_ALL_MODULES(pfOnNodeMgmtStatusChange, (self(), isManaged));
}

/**
 * Calculate node status based on child objects status
 */
void Node::calculateCompoundStatus(bool forcedRecalc)
{
   static uint32_t eventCodes[] = { EVENT_NODE_NORMAL, EVENT_NODE_WARNING,
      EVENT_NODE_MINOR, EVENT_NODE_MAJOR, EVENT_NODE_CRITICAL,
      EVENT_NODE_UNKNOWN, EVENT_NODE_UNMANAGED };

   int oldStatus = m_status;
   super::calculateCompoundStatus(forcedRecalc);
   if (m_status != oldStatus)
      EventBuilder(eventCodes[m_status], m_id)
         .param(_T("previousNodeStatus"), oldStatus)
         .post();
}

/**
 * Delete node in background
 */
static void BackgroundDeleteNode(const shared_ptr<Node>& node)
{
   node->deleteObject();
}

/**
 * Lock node for status poll
 */
bool Node::lockForStatusPoll()
{
   bool success = false;

   lockProperties();
   if (!m_isDeleted && !m_isDeleteInitiated)
   {
      if (m_runtimeFlags & ODF_FORCE_STATUS_POLL)
      {
         success = m_statusPollState.schedule();
         if (success)
            m_runtimeFlags &= ~ODF_FORCE_STATUS_POLL;
      }
      else if ((m_status != STATUS_UNMANAGED) &&
               !(m_flags & DCF_DISABLE_STATUS_POLL) &&
               (getCluster() == nullptr) &&
               !(m_runtimeFlags & ODF_CONFIGURATION_POLL_PENDING) &&
               (static_cast<uint32_t>(time(nullptr) - m_statusPollState.getLastCompleted()) > getCustomAttributeAsUInt32(_T("SysConfig:Objects.StatusPollingInterval"), g_statusPollingInterval)) &&
               !isAgentRestarting() && !isProxyAgentRestarting())
      {
         success = m_statusPollState.schedule();
      }
   }
   unlockProperties();
   return success;
}

/**
 * Lock object for configuration poll
 */
bool Node::lockForConfigurationPoll()
{
   bool success = false;

   lockProperties();
   if (!m_isDeleted && !m_isDeleteInitiated)
   {
      if (m_runtimeFlags & ODF_FORCE_CONFIGURATION_POLL)
      {
         success = m_configurationPollState.schedule();
         if (success)
            m_runtimeFlags &= ~ODF_FORCE_CONFIGURATION_POLL;
      }
      else if ((m_status != STATUS_UNMANAGED) &&
               !(m_flags & DCF_DISABLE_CONF_POLL) &&
               (static_cast<uint32_t>(time(nullptr) - m_configurationPollState.getLastCompleted()) > getCustomAttributeAsUInt32(_T("SysConfig:Objects.ConfigurationPollingInterval"), g_configurationPollingInterval)) &&
               !isAgentRestarting() && !isProxyAgentRestarting())
      {
         success = m_configurationPollState.schedule();
      }
   }
   unlockProperties();
   return success;
}

/**
 * Lock node for discovery poll
 */
bool Node::lockForDiscoveryPoll()
{
   bool success = false;
   lockProperties();
   if (!m_isDeleted && !m_isDeleteInitiated &&
       (g_flags & AF_PASSIVE_NETWORK_DISCOVERY) &&
       (m_status != STATUS_UNMANAGED) &&
       !(m_flags & NF_DISABLE_DISCOVERY_POLL) &&
       (m_runtimeFlags & ODF_CONFIGURATION_POLL_PASSED) &&
       (static_cast<uint32_t>(time(nullptr) - m_discoveryPollState.getLastCompleted()) > getCustomAttributeAsUInt32(_T("SysConfig:NetworkDiscovery.PassiveDiscovery.Interval"), g_discoveryPollingInterval)) &&
       !isAgentRestarting() && !isProxyAgentRestarting())
   {
      success = m_discoveryPollState.schedule();
   }
   unlockProperties();
   return success;
}

/**
 * Lock node for routing table poll
 */
bool Node::lockForRoutingTablePoll()
{
   bool success = false;
   lockProperties();
   if (!m_isDeleted && !m_isDeleteInitiated &&
       (m_status != STATUS_UNMANAGED) &&
       !(m_flags & NF_DISABLE_ROUTE_POLL) &&
       (m_runtimeFlags & ODF_CONFIGURATION_POLL_PASSED) &&
       (static_cast<uint32_t>(time(nullptr) - m_routingPollState.getLastCompleted()) > getCustomAttributeAsUInt32(_T("SysConfig:Topology.RoutingTable.UpdateInterval"), g_routingTableUpdateInterval)) &&
       !isAgentRestarting() && !isProxyAgentRestarting())
   {
      success = m_routingPollState.schedule();
   }
   unlockProperties();
   return success;
}

/**
 * Lock node for topology poll
 */
bool Node::lockForTopologyPoll()
{
   bool success = false;
   lockProperties();
   if (!m_isDeleted && !m_isDeleteInitiated &&
       (m_status != STATUS_UNMANAGED) &&
       !(m_flags & NF_DISABLE_TOPOLOGY_POLL) &&
       (m_runtimeFlags & ODF_CONFIGURATION_POLL_PASSED) &&
       (static_cast<uint32_t>(time(nullptr) - m_topologyPollState.getLastCompleted()) > getCustomAttributeAsUInt32(_T("SysConfig:Topology.PollingInterval"), g_topologyPollingInterval)) &&
       !isAgentRestarting() && !isProxyAgentRestarting())
   {
      success = m_topologyPollState.schedule();
   }
   unlockProperties();
   return success;
}

/*
 * Lock node for ICMP poll
 */
bool Node::lockForIcmpPoll()
{
   bool success = false;

   lockProperties();
   if (!m_isDeleted && !m_isDeleteInitiated &&
       (m_status != STATUS_UNMANAGED) &&
       isIcmpStatCollectionEnabled() &&
       !(m_runtimeFlags & ODF_CONFIGURATION_POLL_PENDING) &&
       (static_cast<uint32_t>(time(nullptr) - m_icmpPollState.getLastCompleted()) > getCustomAttributeAsUInt32(_T("SysConfig:ICMP.PollingInterval"), g_icmpPollingInterval)) &&
       !isProxyAgentRestarting())
   {
      success = m_icmpPollState.schedule();
   }
   unlockProperties();
   return success;
}

/**
 * Perform status poll on node
 */
void Node::statusPoll(PollerInfo *poller, ClientSession *pSession, uint32_t rqId)
{
   lockProperties();
   if (m_isDeleteInitiated || IsShutdownInProgress())
   {
      m_statusPollState.complete(0);
      unlockProperties();
      return;
   }
   // Poller can be called directly - in that case poll flag will not be set
   unlockProperties();

   checkTrapShouldBeProcessed();

   ObjectQueue<Event> *eventQueue = new ObjectQueue<Event>(64, Ownership::True);     // Delayed event queue
   poller->setStatus(_T("wait for lock"));
   pollerLock(status);

   POLL_CANCELLATION_CHECKPOINT_EX(delete eventQueue);

   uint64_t oldCapabilities = m_capabilities;
   uint32_t oldState = m_state;
   NetworkPathCheckResult oldPathCheckResult = m_pathCheckResult;

   poller->setStatus(_T("preparing"));
   m_pollRequestor = pSession;
   m_pollRequestId = rqId;

   if (m_status == STATUS_UNMANAGED)
   {
      sendPollerMsg(POLLER_WARNING _T("Node %s is unmanaged, status poll aborted\r\n"), m_name);
      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Node %s [%u] is unmanaged, status poll aborted"), m_name, m_id);
      delete eventQueue;
      pollerUnlock();
      return;
   }

   sendPollerMsg(_T("Starting status poll of node %s\r\n"), m_name);
   nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Starting status poll of node %s (ID: %d)"), m_name, m_id);

   // Read capability expiration time and current time
   time_t capabilityExpirationTime = static_cast<time_t>(ConfigReadULong(_T("Objects.Nodes.CapabilityExpirationTime"), 604800));
   time_t capabilityExpirationGracePeriod = static_cast<time_t>(ConfigReadULong(_T("Objects.Nodes.CapabilityExpirationGracePeriod"), 3600));
   time_t now = time(nullptr);

   bool agentConnected = false;
   bool resyncDataCollectionConfiguration = false;
   bool forceResolveHostName = false;

   int retryCount = 5;

restart_status_poll:
   if (g_primaryIpUpdateMode == PrimaryIPUpdateMode::ALWAYS)
   {
      if (m_pollsAfterIpUpdate >= g_pollsBetweenPrimaryIpUpdate)
      {
         poller->setStatus(_T("updating primary IP"));
         updatePrimaryIpAddr();
         m_pollsAfterIpUpdate = 0;
      }
      else
      {
         m_pollsAfterIpUpdate++;
      }
   }
   else if (forceResolveHostName)
   {
      poller->setStatus(_T("updating primary IP"));
      updatePrimaryIpAddr();
      forceResolveHostName = false;
   }

   uint32_t requiredPolls = (m_requiredPollCount > 0) ? m_requiredPollCount : g_requiredPolls;

   // Check SNMP agent connectivity
   if ((m_capabilities & NC_IS_SNMP) && (!(m_flags & NF_DISABLE_SNMP)) && m_ipAddress.isValidUnicast())
   {
      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): check SNMP"), m_name);
      SNMP_Transport *pTransport = createSnmpTransport();
      if (pTransport != nullptr)
      {
         poller->setStatus(_T("check SNMP"));
         sendPollerMsg(_T("Checking SNMP agent connectivity\r\n"));
         SharedString testOid = getCustomAttribute(_T("snmp.testoid"));
         if (testOid.isEmpty())
         {
            testOid = _T(".1.3.6.1.2.1.1.2.0");
         }

         TCHAR buffer[256];
         uint32_t snmpErr = SnmpGet(m_snmpVersion, pTransport, testOid, nullptr, 0, buffer, sizeof(buffer), 0);
         if ((snmpErr == SNMP_ERR_SUCCESS) || (snmpErr == SNMP_ERR_NO_OBJECT))
         {
            if (m_state & NSF_SNMP_UNREACHABLE)
            {
               m_pollCountSNMP++;
               if (m_pollCountSNMP >= requiredPolls)
               {
                  m_state &= ~NSF_SNMP_UNREACHABLE;
                  PostSystemEventEx(eventQueue, EVENT_SNMP_OK, m_id);
                  sendPollerMsg(POLLER_INFO _T("Connectivity with SNMP agent restored\r\n"));
                  nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): Connectivity with SNMP agent restored"), m_name);
                  m_pollCountSNMP = 0;
               }
            }
            else
            {
               m_pollCountSNMP = 0;
            }

            // Update authoritative engine data for SNMPv3
            if ((pTransport->getSnmpVersion() == SNMP_VERSION_3) && (pTransport->getAuthoritativeEngine() != nullptr))
            {
               lockProperties();
               m_snmpSecurity->setAuthoritativeEngine(*pTransport->getAuthoritativeEngine());
               m_snmpSecurity->recalculateKeys();
               if (pTransport->getContextEngine() != nullptr)
               {
                  m_snmpSecurity->setContextEngine(*pTransport->getContextEngine());
               }
               unlockProperties();
            }
         }
         else if ((snmpErr == SNMP_ERR_ENGINE_ID) && (m_snmpVersion == SNMP_VERSION_3) && (retryCount > 0))
         {
            // Reset authoritative engine data
            lockProperties();
            m_snmpSecurity->setAuthoritativeEngine(SNMP_Engine());
            m_snmpSecurity->setContextEngine(SNMP_Engine());
            unlockProperties();
            delete pTransport;
            retryCount--;
            goto restart_status_poll;
         }
         else
         {
            if ((snmpErr == SNMP_ERR_COMM) && pTransport->isProxyTransport() && (retryCount > 0))
            {
               nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): got communication error on proxy transport, checking connection to proxy"), m_name);
               uint32_t proxyNodeId = getEffectiveSnmpProxy();
               shared_ptr<NetObj> proxyNode = FindObjectById(proxyNodeId, OBJECT_NODE);
               if (proxyNode != nullptr)
               {
                  shared_ptr<AgentConnectionEx> pconn = static_cast<Node*>(proxyNode.get())->acquireProxyConnection(SNMP_PROXY, true);
                  if (pconn != nullptr)
                  {
                     nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): proxy connection for SNMP is valid"), m_name);
                     retryCount--;
                     delete pTransport;
                     goto restart_status_poll;
                  }
                  else
                  {
                     nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): cannot acquire proxy connection for SNMP"), m_name);
                  }
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): cannot find SNMP proxy node object %u"), m_name, proxyNodeId);
               }
            }

            sendPollerMsg(POLLER_ERROR _T("SNMP agent unreachable\r\n"));
            nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): SNMP agent unreachable (poll count %d of %d)"), m_name, m_pollCountSNMP, requiredPolls);
            if (m_state & NSF_SNMP_UNREACHABLE)
            {
               if (!(m_expectedCapabilities & NC_IS_SNMP) && (now > m_failTimeSNMP + capabilityExpirationTime) &&
                   !(m_state & DCSF_UNREACHABLE) && (now > m_recoveryTime + capabilityExpirationGracePeriod))
               {
                  m_capabilities &= ~NC_IS_SNMP;
                  m_state &= ~NSF_SNMP_UNREACHABLE;
                  m_snmpObjectId = SNMP_ObjectId();
                  sendPollerMsg(POLLER_WARNING _T("Attribute isSNMP set to FALSE\r\n"));
                  nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 4, _T("StatusPoll(%s): Attribute isSNMP set to FALSE"), m_name);
               }
            }
            else
            {
               m_pollCountSNMP++;
               if (m_pollCountSNMP >= requiredPolls)
               {
                  if (g_primaryIpUpdateMode == PrimaryIPUpdateMode::ON_FAILURE)
                  {
                     InetAddress addr = ResolveHostName(m_zoneUIN, m_primaryHostName);
                     if (addr.isValidUnicast() && !m_ipAddress.equals(addr))
                     {
                        nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): primary IP address changed, restarting poll"), m_name);
                        forceResolveHostName = true;
                        goto restart_status_poll;
                     }
                  }

                  m_state |= NSF_SNMP_UNREACHABLE;
                  PostSystemEventEx(eventQueue, EVENT_SNMP_FAIL, m_id);
                  m_failTimeSNMP = now;
                  m_pollCountSNMP = 0;
               }
            }
         }
         delete pTransport;
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): cannot create SNMP transport"), m_name);
      }
      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): SNMP check finished"), m_name);
   }

   POLL_CANCELLATION_CHECKPOINT_EX(delete eventQueue);

   // Check SSH connectivity
   if ((m_capabilities & NC_IS_SSH) && (!(m_flags & NF_DISABLE_SSH)) && m_ipAddress.isValidUnicast())
   {
      sendPollerMsg(_T("Checking SSH connectivity...\r\n"));
      if (checkSshConnection())
      {
         sendPollerMsg(_T("SSH connection avaliable\r\n"));
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("StatusPoll(%s): SSH connected"), m_name);
         if (m_state & NSF_SSH_UNREACHABLE)
         {
            m_pollCountSSH++;
            if (m_pollCountSSH >= requiredPolls)
            {
               m_state &= ~NSF_SSH_UNREACHABLE;
               PostSystemEventEx(eventQueue, EVENT_SSH_OK, m_id);
               m_pollCountSSH = 0;
            }
         }
      }
      else
      {
         sendPollerMsg(_T("Cannot connect to SSH\r\n"));
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): SSH unreachable"), m_name);
         if (m_state & NSF_SSH_UNREACHABLE)
         {
            if (!(m_expectedCapabilities & NC_IS_SSH) && (now > m_failTimeSSH + capabilityExpirationTime) &&
                !(m_state & DCSF_UNREACHABLE) && (now > m_recoveryTime + capabilityExpirationGracePeriod))
            {
               m_capabilities &= ~NC_IS_SSH;
               m_state &= ~NSF_SSH_UNREACHABLE;
               sendPollerMsg(POLLER_WARNING _T("Attribute isSSH set to FALSE\r\n"));
               if (m_capabilities & NC_IS_SMCLP)
               {
                  m_capabilities &= ~NC_IS_SMCLP;
                  sendPollerMsg(POLLER_WARNING _T("Attribute isSM-CLP set to FALSE\r\n"));
               }
            }
         }
         else
         {
            m_pollCountSSH++;
            if (m_pollCountSSH >= requiredPolls)
            {
               m_state |= NSF_SSH_UNREACHABLE;
               PostSystemEventEx(eventQueue, EVENT_SSH_UNREACHABLE, m_id);
               m_failTimeSSH = now;
               m_pollCountSSH = 0;
            }
         }
      }
   }

   POLL_CANCELLATION_CHECKPOINT_EX(delete eventQueue);

   // Check native agent connectivity
   if ((m_capabilities & NC_IS_NATIVE_AGENT) && (!(m_flags & NF_DISABLE_NXCP)))
   {
      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): checking agent"), m_name);
      poller->setStatus(_T("check agent"));
      sendPollerMsg(_T("Checking NetXMS agent connectivity\r\n"));

      uint32_t error, socketError;
      bool newConnection;
      agentLock();
      if (connectToAgent(&error, &socketError, &newConnection, true))
      {
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("StatusPoll(%s): connected to agent"), m_name);
         if (m_state & NSF_AGENT_UNREACHABLE)
         {
            m_pollCountAgent++;
            if (m_pollCountAgent >= requiredPolls)
            {
               m_state &= ~NSF_AGENT_UNREACHABLE;
               PostSystemEventEx(eventQueue, EVENT_AGENT_OK, m_id);
               sendPollerMsg(POLLER_INFO _T("Connectivity with NetXMS agent restored\r\n"));
               m_pollCountAgent = 0;

               // Reset connection time of all proxy connections so they can be re-established immediately
               for(int i = 0; i < MAX_PROXY_TYPE; i++)
               {
                  m_proxyConnections[i].lock();
                  m_proxyConnections[i].setLastConnectTime(0);
                  m_proxyConnections[i].unlock();
               }
            }

            // Reset connection time of all proxy connections so they can be re-established immediately
            for(int i = 0; i < MAX_PROXY_TYPE; i++)
            {
               m_proxyConnections[i].lock();
               m_proxyConnections[i].setLastConnectTime(0);
               m_proxyConnections[i].unlock();
            }
         }
         else
         {
            m_pollCountAgent = 0;
         }
         agentConnected = true;
      }
      else
      {
         wchar_t errorText[256];
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): agent unreachable, error=\"%u %s\", socketError=\"%s\". Poll count %d of %d"),
                  m_name, error, AgentErrorCodeToText(error), GetSocketErrorText(socketError, errorText, 256), m_pollCountAgent, requiredPolls);
         sendPollerMsg(POLLER_ERROR _T("Cannot connect to NetXMS agent (%s)\r\n"), AgentErrorCodeToText(error));
         if (m_state & NSF_AGENT_UNREACHABLE)
         {
            if (!(m_expectedCapabilities & NC_IS_NATIVE_AGENT) && (now > m_failTimeAgent + capabilityExpirationTime) &&
                !(m_state & DCSF_UNREACHABLE) && (now > m_recoveryTime + capabilityExpirationGracePeriod))
            {
               m_capabilities &= ~NC_IS_NATIVE_AGENT;
               m_state &= ~NSF_AGENT_UNREACHABLE;
               m_platformName[0] = 0;
               m_agentVersion[0] = 0;
               sendPollerMsg(POLLER_WARNING _T("Attribute isNetXMSAgent set to FALSE\r\n"));
            }
         }
         else
         {
            m_pollCountAgent++;
            if (m_pollCountAgent >= requiredPolls)
            {
               if (g_primaryIpUpdateMode == PrimaryIPUpdateMode::ON_FAILURE)
               {
                  InetAddress addr = ResolveHostName(m_zoneUIN, m_primaryHostName);
                  if (addr.isValidUnicast() && !m_ipAddress.equals(addr))
                  {
                     nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): primary IP address changed, restarting poll"), m_name);
                     forceResolveHostName = true;
                     agentUnlock();
                     goto restart_status_poll;
                  }
               }

               m_state |= NSF_AGENT_UNREACHABLE;
               PostSystemEventEx(eventQueue, EVENT_AGENT_FAIL, m_id);
               m_failTimeAgent = now;
               m_pollCountAgent = 0;
            }
         }
      }
      agentUnlock();
      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("StatusPoll(%s): agent check finished"), m_name);

      // If file update connection is active, send NOP command to prevent disconnection by idle timeout
      lockProperties();
      shared_ptr<AgentConnection> fileUpdateConnection = m_fileUpdateConnection;
      unlockProperties();
      if (fileUpdateConnection != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): sending keepalive command on file monitoring connection"), m_name);
         fileUpdateConnection->nop();
      }
   }

   POLL_CANCELLATION_CHECKPOINT_EX(delete eventQueue);

   // Check EtherNet/IP connectivity
   if ((m_capabilities & NC_IS_ETHERNET_IP) && (!(m_flags & NF_DISABLE_ETHERNET_IP)))
   {
      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): checking EtherNet/IP"), m_name);
      poller->setStatus(_T("check EtherNet/IP"));
      sendPollerMsg(_T("Checking EtherNet/IP connectivity\r\n"));

      CIP_Identity *identity = nullptr;
      EIP_Status status;

      uint32_t eipProxy = getEffectiveEtherNetIPProxy();
      if (eipProxy != 0)
      {
         // TODO: implement proxy request
         status = EIP_Status::callFailure(EIP_CALL_COMM_ERROR);
      }
      else
      {
         identity = EIP_ListIdentity(m_ipAddress, m_eipPort, 5000, &status);
      }

      if (identity != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("StatusPoll(%s): connected to device via EtherNet/IP"), m_name);
         if (m_state & NSF_ETHERNET_IP_UNREACHABLE)
         {
            m_pollCountEtherNetIP++;
            if (m_pollCountEtherNetIP >= requiredPolls)
            {
               m_state &= ~NSF_ETHERNET_IP_UNREACHABLE;
               PostSystemEventEx(eventQueue, EVENT_ETHERNET_IP_OK, m_id);
               sendPollerMsg(POLLER_INFO _T("EtherNet/IP connectivity restored\r\n"));
               m_pollCountEtherNetIP = 0;
            }
         }
         else
         {
            m_pollCountEtherNetIP = 0;
         }

         m_cipState = identity->state;
         m_cipStatus = identity->status;

         MemFree(identity);
      }
      else
      {
         String reason = status.failureReason();
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): EtherNet/IP unreachable (%s), poll count %d of %d"),
                  m_name, reason.cstr(), m_pollCountEtherNetIP, requiredPolls);
         sendPollerMsg(POLLER_ERROR _T("Cannot connect to device via EtherNet/IP (%s)\r\n"), reason.cstr());
         if (m_state & NSF_ETHERNET_IP_UNREACHABLE)
         {
            if (!(m_expectedCapabilities & NC_IS_ETHERNET_IP) && (now > m_failTimeEtherNetIP + capabilityExpirationTime) &&
                !(m_state & DCSF_UNREACHABLE) && (now > m_recoveryTime + capabilityExpirationGracePeriod))
            {
               m_capabilities &= ~NC_IS_ETHERNET_IP;
               m_state &= ~NSF_ETHERNET_IP_UNREACHABLE;
               sendPollerMsg(POLLER_WARNING _T("Attribute isEtherNetIP set to FALSE\r\n"));
            }
         }
         else
         {
            m_pollCountEtherNetIP++;
            if (m_pollCountEtherNetIP >= requiredPolls)
            {
               if (g_primaryIpUpdateMode == PrimaryIPUpdateMode::ON_FAILURE)
               {
                  InetAddress addr = ResolveHostName(m_zoneUIN, m_primaryHostName);
                  if (addr.isValidUnicast() && !m_ipAddress.equals(addr))
                  {
                     nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): primary IP address changed, restarting poll"), m_name);
                     forceResolveHostName = true;
                     goto restart_status_poll;
                  }
               }

               m_state |= NSF_ETHERNET_IP_UNREACHABLE;
               PostSystemEventEx(eventQueue, EVENT_ETHERNET_IP_UNREACHABLE, m_id);
               m_failTimeEtherNetIP = now;
               m_pollCountEtherNetIP = 0;
            }
         }
      }

      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("StatusPoll(%s): EtherNet/IP check finished"), m_name);
   }

   POLL_CANCELLATION_CHECKPOINT_EX(delete eventQueue);

   // Check Modbus TCP connectivity
   if ((m_capabilities & NC_IS_MODBUS_TCP) && (!(m_flags & NF_DISABLE_MODBUS_TCP)))
   {
      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): checking Modbus TCP"), m_name);
      poller->setStatus(_T("check Modbus TCP"));
      sendPollerMsg(_T("Checking Modbus TCP connectivity\r\n"));

      ModbusOperationStatus status = MODBUS_STATUS_COMM_FAILURE;
      ModbusTransport *transport = createModbusTransport();
      if ((transport != nullptr) && ((status = transport->checkConnection()) == MODBUS_STATUS_SUCCESS))
      {
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("StatusPoll(%s): connected to device via Modbus TCP"), m_name);
         if (m_state & NSF_MODBUS_UNREACHABLE)
         {
            m_pollCountModbus++;
            if (m_pollCountModbus >= requiredPolls)
            {
               m_state &= ~NSF_MODBUS_UNREACHABLE;
               PostSystemEventEx(eventQueue, EVENT_MODBUS_OK, m_id);
               sendPollerMsg(POLLER_INFO _T("Modbus TCP connectivity restored\r\n"));
               m_pollCountModbus = 0;
            }
         }
         else
         {
            m_pollCountModbus = 0;
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): Modbus TCP is unreachable (%s), poll count %d of %d"),
                  m_name, GetModbusStatusText(status), m_pollCountModbus, requiredPolls);
         sendPollerMsg(POLLER_ERROR _T("Cannot connect to device via Modbus TCP (%s)\r\n"), GetModbusStatusText(status));
         if (m_state & NSF_MODBUS_UNREACHABLE)
         {
            if (!(m_expectedCapabilities & NC_IS_MODBUS_TCP) && (now > m_failTimeModbus + capabilityExpirationTime) &&
                !(m_state & DCSF_UNREACHABLE) && (now > m_recoveryTime + capabilityExpirationGracePeriod))
            {
               m_capabilities &= ~NC_IS_MODBUS_TCP;
               m_state &= ~NSF_MODBUS_UNREACHABLE;
               sendPollerMsg(POLLER_WARNING _T("Attribute isModbusTCP set to FALSE\r\n"));
            }
         }
         else
         {
            m_pollCountModbus++;
            if (m_pollCountModbus >= requiredPolls)
            {
               if (g_primaryIpUpdateMode == PrimaryIPUpdateMode::ON_FAILURE)
               {
                  InetAddress addr = ResolveHostName(m_zoneUIN, m_primaryHostName);
                  if (addr.isValidUnicast() && !m_ipAddress.equals(addr))
                  {
                     nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): primary IP address changed, restarting poll"), m_name);
                     forceResolveHostName = true;
                     goto restart_status_poll;
                  }
               }

               m_state |= NSF_MODBUS_UNREACHABLE;
               PostSystemEventEx(eventQueue, EVENT_MODBUS_UNREACHABLE, m_id);
               m_failTimeModbus = now;
               m_pollCountModbus = 0;
            }
         }
      }
      delete transport;

      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("StatusPoll(%s): Modbus TCP check finished"), m_name);
   }

   poller->setStatus(_T("prepare polling list"));

   POLL_CANCELLATION_CHECKPOINT_EX(delete eventQueue);

   // Find service poller node object
   shared_ptr<Node> pollerNode;
   lockProperties();
   if (m_pollerNode != 0)
   {
      uint32_t id = m_pollerNode;
      unlockProperties();
      pollerNode = static_pointer_cast<Node>(FindObjectById(id, OBJECT_NODE));
   }
   else
   {
      unlockProperties();
   }

   // If nothing found, use management server
   if (pollerNode == nullptr)
   {
      pollerNode = static_pointer_cast<Node>(FindObjectById(g_dwMgmtNode, OBJECT_NODE));
   }

   // Create polling list
   SharedObjectArray<NetObj> pollList(32, 32);
   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      shared_ptr<NetObj> curr = getChildList().getShared(i);
      if (curr->getStatus() != STATUS_UNMANAGED)
         pollList.add(curr);
   }
   unlockChildList();

   // Poll interfaces and services
   poller->setStatus(_T("child poll"));
   nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("StatusPoll(%s): starting child object poll"), m_name);
   shared_ptr<Cluster> cluster = getCluster();
   SNMP_Transport *snmp = createSnmpTransport();
   for(int i = 0; i < pollList.size(); i++)
   {
      NetObj *curr = pollList.get(i);
      switch(curr->getObjectClass())
      {
         case OBJECT_INTERFACE:
            nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("StatusPoll(%s): polling interface %d [%s]"), m_name, curr->getId(), curr->getName());
            static_cast<Interface*>(curr)->statusPoll(pSession, rqId, eventQueue, cluster.get(), snmp, m_icmpProxy);
            break;
         case OBJECT_NETWORKSERVICE:
            nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("StatusPoll(%s): polling network service %d [%s]"), m_name, curr->getId(), curr->getName());
            static_cast<NetworkService*>(curr)->statusPoll(pSession, rqId, pollerNode, eventQueue);
            break;
         default:
            nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("StatusPoll(%s): skipping object %d [%s] class %d"), m_name, curr->getId(), curr->getName(), curr->getObjectClass());
            break;
      }

      POLL_CANCELLATION_CHECKPOINT_EX({ delete eventQueue; delete snmp; });
   }
   delete snmp;
   nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("StatusPoll(%s): finished child object poll"), m_name);

   // Check if entire node is down
   // This check is disabled for nodes without IP address
   // The only exception is node without valid address connected via agent tunnel
   if (m_ipAddress.isValidUnicast() || agentConnected)
   {
      bool allDown = true;
      readLockChildList();
      for(int i = 0; i < getChildList().size(); i++)
      {
         NetObj *curr = getChildList().get(i);
         if ((curr->getObjectClass() == OBJECT_INTERFACE) &&
             (static_cast<Interface*>(curr)->getAdminState() != IF_ADMIN_STATE_DOWN) &&
             (static_cast<Interface*>(curr)->getConfirmedOperState() == IF_OPER_STATE_UP) &&
             (curr->getStatus() != STATUS_UNMANAGED))
         {
            allDown = false;
            break;
         }
      }
      unlockChildList();
      if (allDown && (m_capabilities & NC_IS_NATIVE_AGENT) && !(m_flags & NF_DISABLE_NXCP))
      {
         if (m_state & NSF_AGENT_UNREACHABLE)
         {
            // Use TCP ping to check if node actually unreachable if possible
            if (m_ipAddress.isValidUnicast() && (getEffectiveAgentProxy() == 0) && !(m_flags & NF_AGENT_OVER_TUNNEL_ONLY))
            {
               nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): using TCP ping on primary IP address"), m_name);
               sendPollerMsg(_T("Checking primary IP address with TCP ping on agent's port\r\n"));
               TcpPingResult r = TcpPing(m_ipAddress, m_agentPort, 1000);
               if ((r == TCP_PING_SUCCESS) || (r == TCP_PING_REJECT))
               {
                  nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): agent is unreachable but IP address is likely reachable (TCP ping returns %d)"), m_name, r);
                  sendPollerMsg(POLLER_INFO _T("   Primary IP address is responding to TCP ping on port %d\r\n"), m_agentPort);
                  allDown = false;
               }
               else
               {
                  sendPollerMsg(POLLER_ERROR _T("   Primary IP address is not responding to TCP ping on port %d\r\n"), m_agentPort);
               }
            }
         }
         else
         {
            allDown = false;
         }
      }
      if (allDown && (m_capabilities & NC_IS_SNMP) &&
          !(m_flags & NF_DISABLE_SNMP) && !(m_state & NSF_SNMP_UNREACHABLE))
      {
         allDown = false;
      }
      if (allDown && (m_capabilities & NC_IS_SSH) &&
          !(m_flags & NF_DISABLE_SSH) && !(m_state & NSF_SSH_UNREACHABLE))
      {
         allDown = false;
      }
      if (allDown && (m_capabilities & NC_IS_ETHERNET_IP) && !(m_flags & NF_DISABLE_ETHERNET_IP))
      {
         if (m_state & NSF_ETHERNET_IP_UNREACHABLE)
         {
            // Use TCP ping to check if node actually unreachable if possible
            if (m_ipAddress.isValidUnicast() && (getEffectiveEtherNetIPProxy() == 0))
            {
               nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): using TCP ping on primary IP address"), m_name);
               sendPollerMsg(_T("Checking primary IP address with TCP ping on EtherNet/IP port\r\n"));
               TcpPingResult r = TcpPing(m_ipAddress, m_eipPort, 1000);
               if ((r == TCP_PING_SUCCESS) || (r == TCP_PING_REJECT))
               {
                  nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): EtherNet/IP is unreachable but IP address is likely reachable (TCP ping returns %d)"), m_name, r);
                  sendPollerMsg(POLLER_INFO _T("   Primary IP address is responding to TCP ping on port %d\r\n"), m_eipPort);
                  allDown = false;
               }
               else
               {
                  sendPollerMsg(POLLER_ERROR _T("   Primary IP address is not responding to TCP ping on port %d\r\n"), m_eipPort);
               }
            }
         }
         else
         {
            allDown = false;
         }
      }
      if (allDown && (m_capabilities & NC_IS_MODBUS_TCP) && !(m_flags & NF_DISABLE_MODBUS_TCP))
      {
         if (m_state & NSF_MODBUS_UNREACHABLE)
         {
            // Use TCP ping to check if node actually unreachable if possible
            if (m_ipAddress.isValidUnicast() && (getEffectiveModbusProxy() == 0))
            {
               nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): using TCP ping on primary IP address"), m_name);
               sendPollerMsg(_T("Checking primary IP address with TCP ping on Modbus TCP port\r\n"));
               TcpPingResult r = TcpPing(m_ipAddress, m_modbusTcpPort, 1000);
               if ((r == TCP_PING_SUCCESS) || (r == TCP_PING_REJECT))
               {
                  nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): Modbus TCP is unreachable but IP address is likely reachable (TCP ping returns %d)"), m_name, r);
                  sendPollerMsg(POLLER_INFO _T("   Primary IP address is responding to TCP ping on port %d\r\n"), m_modbusTcpPort);
                  allDown = false;
               }
               else
               {
                  sendPollerMsg(POLLER_ERROR _T("   Primary IP address is not responding to TCP ping on port %d\r\n"), m_modbusTcpPort);
               }
            }
         }
         else
         {
            allDown = false;
         }
      }
      if (allDown && (m_flags & NF_PING_PRIMARY_IP) && m_ipAddress.isValidUnicast())
      {
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): using ICMP ping on primary IP address"), m_name);
         sendPollerMsg(_T("Checking primary IP address with ICMP ping\r\n"));
         if (IcmpPing(m_ipAddress, 3, g_icmpPingTimeout, nullptr, g_icmpPingSize, false) == ICMP_SUCCESS)
         {
            nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): primary IP address responds to ICMP ping, considering node as reachable"), m_name);
            sendPollerMsg(POLLER_INFO _T("   Primary IP address is responding to ICMP ping\r\n"));
            allDown = false;
         }
         else
         {
            sendPollerMsg(POLLER_ERROR _T("   Primary IP address is not responding to ICMP ping\r\n"));
         }
      }

      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): allDown=%s, statFlags=0x%08X"), m_name, BooleanToString(allDown), m_state);
      if (allDown)
      {
         if (!(m_state & DCSF_UNREACHABLE))
         {
            m_state |= DCSF_UNREACHABLE;
            m_downSince = time(nullptr);
            poller->setStatus(_T("check network path"));
            NetworkPathCheckResult patchCheckResult = checkNetworkPath(rqId);
            nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): network path check result: (rootCauseFound=%s, reason=%d, node=%u, iface=%u)"),
                     m_name, BooleanToString(patchCheckResult.rootCauseFound), static_cast<int>(patchCheckResult.reason),
                     patchCheckResult.rootCauseNodeId, patchCheckResult.rootCauseInterfaceId);
            if (patchCheckResult.rootCauseFound)
            {
               m_state |= DCSF_NETWORK_PATH_PROBLEM;

               // Set interfaces and network services to UNKNOWN state
               readLockChildList();
               for(int i = 0; i < getChildList().size(); i++)
               {
                  NetObj *curr = getChildList().get(i);
                  if ((curr->getObjectClass() == OBJECT_INTERFACE) || (curr->getObjectClass() == OBJECT_NETWORKSERVICE))
                  {
                     curr->resetStatus();
                  }
               }
               unlockChildList();

               // Clear delayed event queue
               delete_and_null(eventQueue);

               lockProperties();
               m_pathCheckResult = patchCheckResult;
               unlockProperties();

               static const TCHAR *reasonNames[] = {
                        _T("None"), _T("Router down"), _T("Switch down"), _T("Wireless AP down"),
                        _T("Proxy node down"), _T("Proxy agent unreachable"), _T("VPN tunnel down"),
                        _T("Routing loop"), _T("Interface disabled")
               };
               String description = patchCheckResult.buildDescription();
               EventBuilder(EVENT_NODE_UNREACHABLE, m_id)
                  .param(_T("reasonCode"), static_cast<int32_t>(patchCheckResult.reason))
                  .param(_T("reason"), reasonNames[static_cast<int32_t>(patchCheckResult.reason)])
                  .param(_T("rootCauseNodeId"), patchCheckResult.rootCauseNodeId, EventBuilder::OBJECT_ID_FORMAT)
                  .param(_T("rootCauseNodeName"), GetObjectName(patchCheckResult.rootCauseNodeId, _T("")))
                  .param(_T("rootCauseInterfaceId"), patchCheckResult.rootCauseInterfaceId, EventBuilder::OBJECT_ID_FORMAT)
                  .param(_T("rootCauseInterfaceName"), GetObjectName(patchCheckResult.rootCauseInterfaceId, _T("")))
                  .param(_T("description"), description)
                  .post();

               sendPollerMsg(POLLER_WARNING _T("Detected network path problem (%s)\r\n"), description.cstr());
            }
            else if ((m_flags & (NF_DISABLE_NXCP | NF_DISABLE_SNMP | NF_DISABLE_ICMP | NF_DISABLE_ETHERNET_IP)) == (NF_DISABLE_NXCP | NF_DISABLE_SNMP | NF_DISABLE_ICMP | NF_DISABLE_ETHERNET_IP))
            {
               sendPollerMsg(POLLER_WARNING _T("All poller methods (NetXMS agent, SNMP, ICMP and EtherNet/IP) are currently disabled\r\n"));
            }
            else
            {
               PostSystemEvent(EVENT_NODE_DOWN, m_id);
            }

            RemoveFileMonitorsByNodeId(m_id);
            sendPollerMsg(POLLER_ERROR _T("Node is unreachable\r\n"));
            resyncDataCollectionConfiguration = true; // Will cause removal of all remotely collected DCIs from proxy
         }
         else
         {
            if ((m_state & DCSF_NETWORK_PATH_PROBLEM) && !checkNetworkPath(rqId).rootCauseFound)
            {
               if ((m_flags & (NF_DISABLE_NXCP | NF_DISABLE_SNMP | NF_DISABLE_ICMP | NF_DISABLE_ETHERNET_IP)) == (NF_DISABLE_NXCP | NF_DISABLE_SNMP | NF_DISABLE_ICMP | NF_DISABLE_ETHERNET_IP))
               {
                  sendPollerMsg(POLLER_WARNING _T("All poller methods (NetXMS agent, SNMP, ICMP and EtherNet/IP) are currently disabled\r\n"));
               }
               else
               {
                  PostSystemEvent(EVENT_NODE_DOWN, m_id);
               }

               m_state &= ~DCSF_NETWORK_PATH_PROBLEM;
            }
            sendPollerMsg(POLLER_WARNING _T("Node is still unreachable\r\n"));
         }
      }
      else
      {
         m_downSince = 0;
         if (m_state & DCSF_UNREACHABLE)
         {
            int reason = (m_state & DCSF_NETWORK_PATH_PROBLEM) ? 1 : 0;
            m_state &= ~(DCSF_UNREACHABLE | DCSF_NETWORK_PATH_PROBLEM);
            EventBuilder(EVENT_NODE_UP, m_id)
               .param(_T("reason"), reason)
               .post();
            sendPollerMsg(POLLER_INFO L"Node recovered from unreachable state\r\n");
            resyncDataCollectionConfiguration = true; // Will cause addition of all remotely collected DCIs on proxy
            // Set recovery time to provide grace period for capability expiration
            m_recoveryTime = now;
            goto restart_status_poll;
         }
         else
         {
            sendPollerMsg(POLLER_INFO L"Node is connected\r\n");
         }
      }
   }

   POLL_CANCELLATION_CHECKPOINT_EX(delete eventQueue);

   // Get uptime and update boot time
   if (!(m_state & DCSF_UNREACHABLE))
   {
      time_t prevBootTime = m_bootTime;
      wchar_t buffer[MAX_RESULT_LENGTH];
      if (getMetricFromAgent(L"System.Uptime", buffer, MAX_RESULT_LENGTH) == DCE_SUCCESS)
      {
         m_bootTime = time(nullptr) - wcstol(buffer, nullptr, 0);
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("StatusPoll(%s [%d]): boot time set to %u from agent"), m_name, m_id, (UINT32)m_bootTime);
      }
      else if (getMetricFromSNMP(m_snmpPort, SNMP_VERSION_DEFAULT, _T(".1.3.6.1.2.1.1.3.0"), buffer, MAX_RESULT_LENGTH, SNMP_RAWTYPE_NONE) == DCE_SUCCESS)
      {
         m_bootTime = time(nullptr) - wcstol(buffer, nullptr, 0) / 100;   // sysUpTime is in hundredths of a second
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("StatusPoll(%s [%d]): boot time set to %u from SNMP"), m_name, m_id, (UINT32)m_bootTime);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("StatusPoll(%s [%d]): unable to get system uptime"), m_name, m_id);
      }
      if (m_bootTime > prevBootTime)
      {
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, L"StatusPoll(%s [%d]): system restart detected (previous boot time = " INT64_FMT ", new boot time = " INT64_FMT ")",
            m_name, m_id, static_cast<int64_t>(prevBootTime), static_cast<int64_t>(m_bootTime));
         sendPollerMsg(L"System was restarted since last status poll\r\n");
         updateInterfaceConfiguration(rqId);
      }
   }
   else
   {
      m_bootTime = 0;
   }

   // Get agent uptime to check if it was restarted
   if (!(m_state & DCSF_UNREACHABLE) && isNativeAgent())
   {
      wchar_t buffer[MAX_RESULT_LENGTH];
      if (getMetricFromAgent(L"Agent.Uptime", buffer, MAX_RESULT_LENGTH) == DCE_SUCCESS)
      {
         time_t prevAgentUptime = m_agentUpTime;
         m_agentUpTime = wcstol(buffer, nullptr, 0);
         if (prevAgentUptime > m_agentUpTime)
         {
            nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, L"StatusPoll(%s [%d]): agent restart detected", m_name, m_id);
            sendPollerMsg(L"Agent was restarted since last status poll\r\n");
            // agent was restarted, cancel existing file monitors (on agent side they should have been cancelled by restart)
            RemoveFileMonitorsByNodeId(m_id);
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, L"StatusPoll(%s [%d]): unable to get agent uptime", m_name, m_id);
         RemoveFileMonitorsByNodeId(m_id);
         m_agentUpTime = 0;
      }
   }
   else
   {
      m_agentUpTime = 0;
   }

   // Get geolocation
   if (!(m_state & DCSF_UNREACHABLE))
   {
      bool geoLocationRetrieved = false;
      if (isNativeAgent())
      {
         TCHAR buffer[MAX_RESULT_LENGTH];
         if (getMetricFromAgent(_T("GPS.LocationData"), buffer, MAX_RESULT_LENGTH) == DCE_SUCCESS)
         {
            GeoLocation location = GeoLocation::parseAgentData(buffer);
            if (location.getType() != GL_UNSET)
            {
               nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("StatusPoll(%s [%d]): location set to %s, %s from agent"), m_name, m_id, location.getLatitudeAsString(), location.getLongitudeAsString());
               setGeoLocation(location);
               geoLocationRetrieved = true;
            }
         }
      }
      if (!geoLocationRetrieved && isSNMPSupported())
      {
         SNMP_Transport *transport = createSnmpTransport();
         if (transport != nullptr)
         {
            GeoLocation location = m_driver->getGeoLocation(transport, this, m_driverData);
            if (location.getType() != GL_UNSET)
            {
               nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("StatusPoll(%s [%d]): location set to %s, %s from SNMP device driver"), m_name, m_id, location.getLatitudeAsString(), location.getLongitudeAsString());
               setGeoLocation(location);
               geoLocationRetrieved = true;
            }
            delete transport;
         }
      }
      if (!geoLocationRetrieved)
      {
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("StatusPoll(%s [%d]): unable to get system location"), m_name, m_id);
      }
   }

   // Get agent log and agent local database status
   if (!(m_state & DCSF_UNREACHABLE) && isNativeAgent())
   {
      TCHAR buffer[MAX_RESULT_LENGTH];
      if (getMetricFromAgent(_T("Agent.LogFile.Status"), buffer, MAX_RESULT_LENGTH) == DCE_SUCCESS)
      {
         uint32_t status = _tcstol(buffer, nullptr, 0);
         if (status != 0)
            EventBuilder(EVENT_AGENT_LOG_PROBLEM, m_id)
               .param(_T("status"), status)
               .param(_T("description"), _T("could not open"))
               .post();
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("StatusPoll(%s [%d]): unable to get agent log status"), m_name, m_id);
      }

      if (getMetricFromAgent(_T("Agent.LocalDatabase.Status"), buffer, MAX_RESULT_LENGTH) == DCE_SUCCESS)
      {
         uint32_t status = _tcstol(buffer, nullptr, 0);
         const TCHAR *statusDescription[3]= {
                                       _T("normal"),
                                       _T("could not open database"),
                                       _T("could not update database"),
                                       };
         if (status != 0)
            EventBuilder(EVENT_AGENT_LOCAL_DATABASE_PROBLEM, m_id)
               .param(_T("statusCode"), status)
               .param(_T("description"), statusDescription[status])
               .post();
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("StatusPoll(%s [%u]): unable to get agent local database status"), m_name, m_id);
      }
   }

   // Get user agent capability
   if (!(m_state & DCSF_UNREACHABLE) && isNativeAgent())
   {
      TCHAR buffer[MAX_RESULT_LENGTH];
      if (getMetricFromAgent(_T("Agent.IsUserAgentInstalled"), buffer, MAX_RESULT_LENGTH) == DCE_SUCCESS)
      {
         uint32_t status = _tcstol(buffer, nullptr, 0);
         if (status != 0)
            m_capabilities |= NC_HAS_USER_AGENT;
         else
            m_capabilities &= ~NC_HAS_USER_AGENT;
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("StatusPoll(%s [%u]): cannot get user agent information"), m_name, m_id);
      }
   }

   // Check time on remote system
   if (!(m_state & DCSF_UNREACHABLE) && isNativeAgent())
   {
      shared_ptr<AgentConnectionEx> conn = getAgentConnection();
      if (conn != nullptr)
      {
         int64_t remoteTime;
         int32_t offset;
         bool allowSync;
         uint32_t rcc = conn->getRemoteSystemTime(&remoteTime, &offset, nullptr, &allowSync);
         if (rcc == ERR_SUCCESS)
         {
            TCHAR timeText[64];
            FormatTimestamp(static_cast<time_t>(remoteTime / 1000), timeText);
            nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("StatusPoll(%s [%u]): remote system time is %s, offset %d ms"), m_name, m_id, timeText, offset);
            sendPollerMsg(_T("Remote system time is %s, offset %d ms\r\n"), timeText, offset);
            if (abs(offset) > 5000)
            {
               sendPollerMsg(POLLER_WARNING _T("   Time difference exceeds 5 seconds\r\n"));
               if (allowSync)
               {
                  sendPollerMsg(_T("   Performing coarse time synchronization\r\n"));
                  uint32_t rcc = conn->synchronizeTime();
                  if (rcc == ERR_SUCCESS)
                     sendPollerMsg(POLLER_INFO _T("   Time is synchronized\r\n"));
                  else
                     sendPollerMsg(POLLER_ERROR _T("   Cannot synchronize time (%s)\r\n"), AgentErrorCodeToText(rcc));
               }
            }
         }
      }
   }

   // Send delayed events and destroy delayed event queue
   if (eventQueue != nullptr)
   {
      ResendEvents(eventQueue);
      delete eventQueue;
   }

   POLL_CANCELLATION_CHECKPOINT();

   // Call hooks in loaded modules
   ENUMERATE_MODULES(pfStatusPollHook)
   {
      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("StatusPoll(%s [%d]): calling hook in module %s"), m_name, m_id, CURRENT_MODULE.name);
      CURRENT_MODULE.pfStatusPollHook(this, pSession, rqId, poller);
   }

   // Execute hook script
   poller->setStatus(_T("hook"));
   executeHookScript(_T("StatusPoll"));

   if (resyncDataCollectionConfiguration)
   {
      // call to onDataCollectionChange will force data collection
      // configuration upload to this node or relevant proxy nodes
      onDataCollectionChange();
   }

   poller->setStatus(_T("cleanup"));

   if (oldCapabilities != m_capabilities)
      EventBuilder(EVENT_NODE_CAPABILITIES_CHANGED, m_id)
         .param(_T("oldCapabilities"), oldCapabilities, EventBuilder::HEX_64BIT_FORMAT)
         .param(_T("newCapabilities"), m_capabilities, EventBuilder::HEX_64BIT_FORMAT)
         .post();

   if (oldState != m_state)
   {
      markAsModified(MODIFY_RUNTIME); // only notify clients
   }

   if ((oldCapabilities != m_capabilities) || !oldPathCheckResult.equals(m_pathCheckResult))
   {
      markAsModified(MODIFY_NODE_PROPERTIES);
   }

   calculateCompoundStatus();

   if (IsZoningEnabled())
   {
      shared_ptr<Zone> zone = FindZoneByProxyId(m_id);
      if (zone != nullptr)
      {
         zone->updateProxyStatus(self(), true);
      }
   }

   sendPollerMsg(_T("Finished status poll of node %s\r\n"), m_name);
   sendPollerMsg(_T("Node status after poll is %s\r\n"), GetStatusAsText(m_status, true));

   lockProperties();
   m_pollRequestor = nullptr;
   unlockProperties();

   pollerUnlock();
   nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Finished status poll of node %s (ID: %d)"), m_name, m_id);

   // Check if the node has to be deleted due to long downtime
   if (rqId == 0)
   {
      time_t unreachableDeleteDays = (time_t)ConfigReadInt(_T("Objects.DeleteUnreachableNodesPeriod"), 0);
      if ((unreachableDeleteDays > 0) && (m_downSince > 0) &&
          (time(nullptr) - m_downSince > unreachableDeleteDays * 24 * 3600))
      {
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 2, _T("Delete node %s [%u] because it is unreachable for more than %d days"),
                  m_name, m_id, static_cast<int>(unreachableDeleteDays));
         ThreadPoolExecute(g_mainThreadPool, BackgroundDeleteNode, self());
      }
   }
}

/**
 * Check single element of network path
 */
NetworkPathCheckResult Node::checkNetworkPathElement(uint32_t nodeId, const TCHAR *nodeType, bool isProxy, bool isSwitch, uint32_t requestId, bool secondPass)
{
   shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectById(nodeId, OBJECT_NODE));
   if (node == nullptr)
      return NetworkPathCheckResult();

   nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("Node::checkNetworkPathElement(%s [%d]): found %s: %s [%d]"), m_name, m_id, nodeType, node->getName(), node->getId());

   if (secondPass && (node->m_statusPollState.getLastCompleted() < time(nullptr) - 1))
   {
      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("Node::checkNetworkPathElement(%s [%d]): forced status poll on node %s [%d]"),
                m_name, m_id, node->getName(), node->getId());
      node->startForcedStatusPoll();
      PollerInfo *poller = RegisterPoller(PollerType::STATUS, node);
      poller->startExecution();
      node->statusPoll(poller, nullptr, 0);
      delete poller;
   }

   if (node->isDown())
   {
      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Node::checkNetworkPathElement(%s [%d]): %s %s [%d] is down"),
                m_name, m_id, nodeType, node->getName(), node->getId());
      sendPollerMsg(POLLER_WARNING _T("   %s %s is down\r\n"), nodeType, node->getName());
      return NetworkPathCheckResult(isProxy ? NetworkPathFailureReason::PROXY_NODE_DOWN : (isSwitch ? NetworkPathFailureReason::SWITCH_DOWN : NetworkPathFailureReason::ROUTER_DOWN), node->getId());
   }
   if (isProxy && node->isNativeAgent() && (node->getState() & NSF_AGENT_UNREACHABLE))
   {
      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Node::checkNetworkPathElement(%s [%d]): agent on %s %s [%d] is down"),
                m_name, m_id, nodeType, node->getName(), node->getId());
      sendPollerMsg(POLLER_WARNING _T("   Agent on %s %s is down\r\n"), nodeType, node->getName());
      return NetworkPathCheckResult(NetworkPathFailureReason::PROXY_AGENT_UNREACHABLE, node->getId());
   }
   return NetworkPathCheckResult();
}

/**
 * Check network path between node and management server to detect possible intermediate node failure - layer 2
 *
 * @return true if network path problems found
 */
NetworkPathCheckResult Node::checkNetworkPathLayer2(uint32_t requestId, bool secondPass)
{
   if (IsShutdownInProgress())
      return NetworkPathCheckResult();

   time_t now = time(nullptr);

   // Check proxy node(s)
   if (IsZoningEnabled() && (m_zoneUIN != 0))
   {
      shared_ptr<Zone> zone = FindZoneByUIN(m_zoneUIN);
      if ((zone != nullptr) && !zone->isProxyNode(m_id))
      {
         NetworkPathCheckResult result = checkNetworkPathElement(zone->getAvailableProxyNodeId(this), _T("zone proxy"), true, false, requestId, secondPass);
         if (result.rootCauseFound)
            return result;
      }
   }

   // Check directly connected switch
   sendPollerMsg(_T("Checking Ethernet connectivity...\r\n"));
   shared_ptr<Interface> iface = findInterfaceByIP(m_ipAddress);
   if (iface != nullptr)
   {
      if  (iface->getPeerNodeId() != 0)
      {
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("Node::checkNetworkPath(%s [%d]): found interface object for primary IP: %s [%d]"), m_name, m_id, iface->getName(), iface->getId());
         NetworkPathCheckResult result = checkNetworkPathElement(iface->getPeerNodeId(), _T("upstream switch"), false, true, requestId, secondPass);
         if (result.rootCauseFound)
            return result;

         shared_ptr<NetObj> switchNode = FindObjectById(iface->getPeerNodeId(), OBJECT_NODE);
         shared_ptr<Interface> switchIface = static_pointer_cast<Interface>(FindObjectById(iface->getPeerInterfaceId(), OBJECT_INTERFACE));
         if ((switchNode != nullptr) && (switchIface != nullptr) &&
             ((switchIface->getAdminState() == IF_ADMIN_STATE_DOWN) || (switchIface->getAdminState() == IF_ADMIN_STATE_TESTING)))
         {
            nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Node::checkNetworkPath(%s [%d]): upstream interface %s [%d] on switch %s [%d] is administratively down"),
                        m_name, m_id, switchIface->getName(), switchIface->getId(), switchNode->getName(), switchNode->getId());
            sendPollerMsg(POLLER_WARNING _T("   Upstream interface %s on node %s is administratively down\r\n"), switchIface->getName(), switchNode->getName());
            result.rootCauseFound = true;
            result.reason = NetworkPathFailureReason::INTERFACE_DISABLED;
            result.rootCauseNodeId = switchNode->getId();
            result.rootCauseInterfaceId = switchIface->getId();
            return result;
         }
      }
      else
      {
         int type = 0;
         shared_ptr<NetObj> cp = FindInterfaceConnectionPoint(iface->getMacAddress(), &type);
         if (cp != nullptr)
         {
            nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("Node::checkNetworkPath(%s [%d]): found connection point: %s [%u]"), m_name, m_id, cp->getName(), cp->getId());
            if (secondPass)
            {
               shared_ptr<Node> node = (cp->getObjectClass() == OBJECT_INTERFACE) ? static_cast<Interface*>(cp.get())->getParentNode() : static_cast<AccessPoint*>(cp.get())->getController();
               if ((node != nullptr) && !node->isDown() && (node->m_statusPollState.getLastCompleted() < now - 1))
               {
                  nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("Node::checkNetworkPath(%s [%d]): forced status poll on node %s [%d]"),
                              m_name, m_id, node->getName(), node->getId());
                  static_cast<Pollable&>(*node).doForcedStatusPoll(RegisterPoller(PollerType::STATUS, node));
               }
            }

            if (cp->getObjectClass() == OBJECT_INTERFACE)
            {
               Interface *iface = static_cast<Interface*>(cp.get());
               if ((iface->getAdminState() == IF_ADMIN_STATE_DOWN) || (iface->getAdminState() == IF_ADMIN_STATE_TESTING))
               {
                  String parentNodeName = iface->getParentNodeName();
                  nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Node::checkNetworkPath(%s [%u]): upstream interface %s [%u] on switch %s [%u] is administratively down"),
                              m_name, m_id, iface->getName(), iface->getId(), parentNodeName.cstr(), iface->getParentNodeId());
                  sendPollerMsg(POLLER_WARNING _T("   Upstream interface %s on node %s is administratively down\r\n"),
                                iface->getName(), parentNodeName.cstr());
                  return NetworkPathCheckResult(NetworkPathFailureReason::INTERFACE_DISABLED, iface->getParentNodeId(), iface->getId());
               }
            }
            else if (cp->getObjectClass() == OBJECT_ACCESSPOINT)
            {
               AccessPoint *ap = static_cast<AccessPoint*>(cp.get());
               if ((ap->getState() == AP_DOWN) || (ap->getState() == AP_UNPROVISIONED))
               {
                  nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Node::checkNetworkPath(%s [%d]): wireless access point %s [%u] is down"),
                              m_name, m_id, ap->getName(), ap->getId());
                  sendPollerMsg(POLLER_WARNING _T("   Wireless access point %s is down\r\n"), ap->getName());
                  return NetworkPathCheckResult(NetworkPathFailureReason::WIRELESS_AP_DOWN, ap->getId());
               }
            }
         }
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Node::checkNetworkPath(%s [%d]): cannot find interface object for primary IP"), m_name, m_id);
   }
   return NetworkPathCheckResult();
}

/**
 * Check network path between node and management server to detect possible intermediate node failure - layer 3
 *
 * @return true if network path problems found
 */
NetworkPathCheckResult Node::checkNetworkPathLayer3(uint32_t requestId, bool secondPass)
{
   if (IsShutdownInProgress())
      return NetworkPathCheckResult();

   shared_ptr<Node> mgmtNode = static_pointer_cast<Node>(FindObjectById(g_dwMgmtNode, OBJECT_NODE));
   if (mgmtNode == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Node::checkNetworkPath(%s [%d]): cannot find management node"), m_name, m_id);
      return NetworkPathCheckResult();
   }

   shared_ptr<NetworkPath> trace = TraceRoute(mgmtNode, self());
   if ((trace == nullptr) && (m_lastKnownNetworkPath == nullptr))
   {
      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Node::checkNetworkPath(%s [%d]): trace not available"), m_name, m_id);
      return NetworkPathCheckResult();
   }
   if ((trace == nullptr) || !trace->isComplete())
   {
      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Node::checkNetworkPath(%s [%d]): current trace is %s, using cached trace"),
               m_name, m_id, (trace == nullptr) ? _T("not available") : _T("incomplete"));
      lockProperties();
      if (m_lastKnownNetworkPath != nullptr)
         trace = m_lastKnownNetworkPath;
      unlockProperties();
   }
   nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Node::checkNetworkPath(%s [%d]): trace available, %d hops, %s"),
             m_name, m_id, trace->getHopCount(), trace->isComplete() ? _T("complete") : _T("incomplete"));

   // We will do path check in two passes
   // If unreachable intermediate node will be found on first pass,
   // then method will just return true. Otherwise, we will do
   // second pass, this time forcing status poll on each node in the path.
   sendPollerMsg(_T("Checking network path (%s pass)...\r\n"), secondPass ? _T("second") : _T("first"));
   NetworkPathCheckResult result;
   for(int i = 0; i < trace->getHopCount(); i++)
   {
      NetworkPathElement *hop = trace->getHopInfo(i);
      if ((hop->object == nullptr) || (hop->object->getId() == m_id) || (hop->object->getObjectClass() != OBJECT_NODE))
         continue;

      // Check for loops
      if (i > 0)
      {
         for(int j = i - 1; j >= 0; j--)
         {
            NetworkPathElement *prevHop = trace->getHopInfo(j);
            if (prevHop->object == hop->object)
            {
               prevHop = trace->getHopInfo(i - 1);
               nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Node::checkNetworkPath(%s [%d]): routing loop detected on upstream node %s [%d]"),
                           m_name, m_id, prevHop->object->getName(), prevHop->object->getId());
               sendPollerMsg(POLLER_WARNING _T("   Routing loop detected on upstream node %s\r\n"), prevHop->object->getName());

               EventBuilder(EVENT_ROUTING_LOOP_DETECTED, prevHop->object->getId())
                  .param(_T("protocol"), (trace->getSourceAddress().getFamily() == AF_INET6) ? _T("IPv6") : _T("IPv4"))
                  .param(_T("destNodeId"), m_id, EventBuilder::OBJECT_ID_FORMAT)
                  .param(_T("destAddress"), m_ipAddress)
                  .param(_T("sourceNodeId"), g_dwMgmtNode, EventBuilder::OBJECT_ID_FORMAT)
                  .param(_T("sourceAddress"), (trace->getSourceAddress()))
                  .param(_T("prefix"), prevHop->route)
                  .param(_T("prefixLength"), prevHop->route.getMaskBits())
                  .param(_T("nextHopNodeId"), hop->object->getId(), EventBuilder::OBJECT_ID_FORMAT)
                  .param(_T("nextHopAddress"), prevHop->nextHop)
                  .post();

               result.rootCauseFound = true;
               result.reason = NetworkPathFailureReason::ROUTING_LOOP;
               result.rootCauseNodeId = hop->object->getId();
               break;
            }
         }
         if (result.rootCauseFound)
            break;
      }

      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("Node::checkNetworkPath(%s [%d]): checking upstream router %s [%d]"),
                  m_name, m_id, hop->object->getName(), hop->object->getId());
      result = checkNetworkPathElement(hop->object->getId(), _T("upstream router"), false, false, requestId, secondPass);
      if (result.rootCauseFound)
         break;

      if (hop->type == NetworkPathElementType::ROUTE)
      {
         shared_ptr<Interface> iface = static_cast<Node&>(*hop->object).findInterfaceByIndex(hop->ifIndex);
         if ((iface != nullptr) && ((iface->getAdminState() == IF_ADMIN_STATE_DOWN) || (iface->getAdminState() == IF_ADMIN_STATE_TESTING)))
         {
            nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Node::checkNetworkPath(%s [%d]): upstream interface %s [%d] on node %s [%d] is administratively down"),
                        m_name, m_id, iface->getName(), iface->getId(), hop->object->getName(), hop->object->getId());
            sendPollerMsg(POLLER_WARNING _T("   Upstream interface %s on node %s is administratively down\r\n"), iface->getName(), hop->object->getName());
            result.rootCauseFound = true;
            result.reason = NetworkPathFailureReason::INTERFACE_DISABLED;
            result.rootCauseNodeId = hop->object->getId();
            result.rootCauseInterfaceId = iface->getId();
            break;
         }
      }
      else if (hop->type == NetworkPathElementType::VPN)
      {
         // Next hop is behind VPN tunnel
         shared_ptr<VPNConnector> vpnConn = static_pointer_cast<VPNConnector>(FindObjectById(hop->ifIndex, OBJECT_VPNCONNECTOR));
         if ((vpnConn != nullptr) && (vpnConn->getStatus() == STATUS_CRITICAL))
         {
            result.rootCauseFound = true;
            result.reason = NetworkPathFailureReason::VPN_TUNNEL_DOWN;
            result.rootCauseNodeId = hop->object->getId();
            result.rootCauseInterfaceId = vpnConn->getId();
            break;
         }
      }
   }

   lockProperties();
   if ((trace != m_lastKnownNetworkPath) && trace->isComplete())
   {
      m_lastKnownNetworkPath = trace;
   }
   unlockProperties();
   return result;
}

/**
 * Check network path between node and management server to detect possible intermediate node failure
 *
 * @return true if network path problems found
 */
NetworkPathCheckResult Node::checkNetworkPath(uint32_t requestId)
{
   NetworkPathCheckResult result = checkNetworkPathLayer2(requestId, false);
   if (result.rootCauseFound)
      return result;

   result = checkNetworkPathLayer3(requestId, false);
   if (result.rootCauseFound)
      return result;

   nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Node::checkNetworkPath(%s [%u]): will do second pass"), m_name, m_id);

   result = checkNetworkPathLayer2(requestId, true);
   if (result.rootCauseFound)
      return result;

   return checkNetworkPathLayer3(requestId, true);
}

/**
 * Check agent policy binding
 * Intended to be called only from configuration poller
 */
void Node::checkAgentPolicyBinding(const shared_ptr<AgentConnectionEx>& conn)
{
   sendPollerMsg(L"   Checking agent policy deployment\r\n");

   AgentPolicyInfo *ap;
   uint32_t rcc = conn->getPolicyInventory(&ap);
   if (rcc == ERR_SUCCESS)
   {
      // Check for unbound but installed policies
      for(int i = 0; i < ap->size(); i++)
      {
         uuid guid = ap->getGuid(i);
         bool found = false;
         readLockParentList();
         for(int i = 0; i < getParentList().size(); i++)
         {
            if (getParentList().get(i)->getObjectClass() == OBJECT_TEMPLATE)
            {
                if (static_cast<Template*>(getParentList().get(i))->hasPolicy(guid))
                {
                   found = true;
                   break;
                }
            }
         }

         if (!found)
         {
            sendPollerMsg(_T("      Removing policy %s from agent\r\n"), guid.toString().cstr());
            auto data = make_shared<AgentPolicyRemovalData>(self(), guid, ap->getType(i), isNewPolicyTypeFormatSupported());
            nx_swprintf(data->debugId, 256, L"%s [%u] from %s/%s", getName(), getId(), L"unknown", guid.toString().cstr());

            wchar_t key[64];
            nx_swprintf(key, 64, L"AgentPolicyDeployment_%u", m_id);
            ThreadPoolExecuteSerialized(g_pollerThreadPool, key, RemoveAgentPolicy, data);
         }

         unlockParentList();
      }

      // Check for bound but not installed policies and schedule it's installation again
      readLockParentList();
      for(int i = 0; i < getParentList().size(); i++)
      {
         if (getParentList().get(i)->getObjectClass() == OBJECT_TEMPLATE)
         {
            static_cast<Template*>(getParentList().get(i))->checkPolicyDeployment(self(), ap);
         }
      }
      unlockParentList();

      m_capabilities |= ap->isNewTypeFormat() ? NC_IS_NEW_POLICY_TYPES : 0;
      delete ap;
   }
   else
   {
      sendPollerMsg(POLLER_WARNING _T("      Cannot get policy inventory from agent (%s)\r\n"), AgentErrorCodeToText(rcc));
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): AgentConnection::getPolicyInventory() failed: rcc=%d"), m_name, rcc);
   }
}

/**
 * Update primary IP address from primary name
 */
void Node::updatePrimaryIpAddr()
{
   if (m_primaryHostName.isNull() || m_primaryHostName.isEmpty() || IsShutdownInProgress())
      return;

   InetAddress ipAddr = ResolveHostName(m_zoneUIN, m_primaryHostName);
   if (!ipAddr.equals(getIpAddress()) && (ipAddr.isValidUnicast() || !_tcscmp(m_primaryHostName, _T("0.0.0.0")) || ((m_capabilities & NC_IS_LOCAL_MGMT) && ipAddr.isLoopback())))
   {
      TCHAR buffer1[64], buffer2[64];
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 4, _T("IP address for node %s [%u] changed from %s to %s"), m_name, m_id, m_ipAddress.toString(buffer1), ipAddr.toString(buffer2));
      EventBuilder(EVENT_IP_ADDRESS_CHANGED, m_id)
         .param(_T("newIpAddress"), ipAddr)
         .param(_T("oldIpAddress"), m_ipAddress)
         .post();

      lockProperties();
      setPrimaryIPAddress(ipAddr);
      unlockProperties();

      agentLock();
      deleteAgentConnection();
      agentUnlock();
   }
}

/**
 * Read baseboard information from agent
 */
static int ReadBaseboardInformation(Node *node, ObjectArray<HardwareComponent> *components)
{
   TCHAR buffer[256 * 4];
   memset(buffer, 0, sizeof(buffer));

   static const TCHAR *metrics[4] =
   {
      _T("Hardware.Baseboard.Manufacturer"),
      _T("Hardware.Baseboard.Product"),
      _T("Hardware.Baseboard.SerialNumber"),
      _T("Hardware.Baseboard.Type")
   };

   int readCount = 0;
   for(int i = 0; i < 4; i++)
   {
      if (node->getMetricFromAgent(metrics[i], &buffer[i * 256], 256) == ERR_SUCCESS)
         readCount++;
   }

   if (readCount > 0)
   {
      components->add(new HardwareComponent(HWC_BASEBOARD, 0, &buffer[256 * 3], buffer, &buffer[256], nullptr, &buffer[512]));
   }
   return readCount;
}

/**
 * Read hardware component information using given table
 */
static int ReadHardwareInformation(Node *node, ObjectArray<HardwareComponent> *components, HardwareComponentCategory category, const TCHAR *tableName)
{
   shared_ptr<Table> table;
   if (node->getTableFromAgent(tableName, &table) != DCE_SUCCESS)
      return 0;

   bool resequence = false;
   int base = components->size();
   for(int i = 0; i < table->getNumRows(); i++)
   {
      HardwareComponent *c = new HardwareComponent(category, *table, i);
      if (!resequence)
      {
         // Check for duplicate indexes - older agent versions that use BIOS handles may return duplicate handles
         for(int j = base; j < components->size(); j++)
         {
            if (components->get(j)->getIndex() == c->getIndex())
            {
               resequence = true;
               break;
            }
         }
      }
      components->add(c);
   }

   if (resequence)
   {
      for(int i = base, index = 0; i < components->size(); i++, index++)
         components->get(i)->setIndex(index);
   }

   return 1;
}

/**
 * Update hardware property
 */
bool Node::updateSystemHardwareProperty(SharedString &property, const TCHAR *value, const TCHAR *displayName, uint32_t requestId)
{
   bool modified = false;
   if (_tcscmp(property, value))
   {
      if (*value != 0)
         sendPollerMsg(_T("   System hardware %s set to %s\r\n"), displayName, value);
      property = value;
      modified = true;
   }
   return modified;
}

/**
 * Update general system hardware information
 */
bool Node::updateSystemHardwareInformation(PollerInfo *poller, uint32_t requestId)
{
   // Skip for EtherNet/IP devices because hardware information is updated
   // from identity message and if SNMP is present on device it may override
   // it with incorrect values
   if (m_capabilities & NC_IS_ETHERNET_IP)
   {
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): skipping Node::updateSystemHardwareInformation for EtherNet/IP device"), m_name);
      return false;
   }

   poller->setStatus(_T("hardware check"));
   sendPollerMsg(_T("Updating general system hardware information\r\n"));

   DeviceHardwareInfo hwInfo;
   memset(&hwInfo, 0, sizeof(hwInfo));

   bool success = false;
   if (m_capabilities & NC_IS_NATIVE_AGENT)
   {
      if (getMetricFromAgent(_T("Hardware.System.Manufacturer"), hwInfo.vendor, sizeof(hwInfo.vendor) / sizeof(TCHAR)) == DCE_SUCCESS)
         success = true;
      if (getMetricFromAgent(_T("Hardware.System.Product"), hwInfo.productName, sizeof(hwInfo.productName) / sizeof(TCHAR)) == DCE_SUCCESS)
         success = true;
      if (getMetricFromAgent(_T("Hardware.System.ProductCode"), hwInfo.productCode, sizeof(hwInfo.productCode) / sizeof(TCHAR)) == DCE_SUCCESS)
         success = true;
      if (getMetricFromAgent(_T("Hardware.System.Version"), hwInfo.productVersion, sizeof(hwInfo.productVersion) / sizeof(TCHAR)) == DCE_SUCCESS)
         success = true;
      if (getMetricFromAgent(_T("Hardware.System.SerialNumber"), hwInfo.serialNumber, sizeof(hwInfo.serialNumber) / sizeof(TCHAR)) == DCE_SUCCESS)
         success = true;
   }

   if (!success && (m_capabilities & NC_IS_SNMP))
   {
      SNMP_Transport *snmp = createSnmpTransport();
      if (snmp != nullptr)
      {
         success = m_driver->getHardwareInformation(snmp, this, m_driverData, &hwInfo);
         delete snmp;
      }
      if ((!success || (hwInfo.vendor[0] == 0) || (hwInfo.productName[0] == 0) || (hwInfo.productVersion[0] == 0) || (hwInfo.serialNumber[0] == 0)) && (m_capabilities & NC_HAS_ENTITY_MIB))
      {
         // Try to get hardware information from ENTITY MIB
         lockProperties();
         if (m_components != nullptr)
         {
            const Component *root = m_components->getRoot();
            if (root != nullptr)
            {
               // Device could be reported as but consist from single chassis only
               if ((root->getClass() == COMPONENT_CLASS_STACK) && (root->getChildren().size() == 1) && (root->getChildren().get(0)->getClass() == COMPONENT_CLASS_CHASSIS))
               {
                  root = root->getChildren().get(0);
               }
               if ((root->getClass() == COMPONENT_CLASS_CHASSIS) || (root->getClass() == COMPONENT_CLASS_STACK))
               {
                  if (hwInfo.vendor[0] == 0)
                     _tcslcpy(hwInfo.vendor, root->getVendor(), 128);
                  if (hwInfo.productName[0] == 0)
                     _tcslcpy(hwInfo.productName, (*root->getModel() != 0) ? root->getModel() : root->getDescription(), 128);
                  if (hwInfo.productVersion[0] == 0)
                     _tcslcpy(hwInfo.productVersion, root->getFirmware(), 16);
                  if (hwInfo.serialNumber[0] == 0)
                     _tcslcpy(hwInfo.serialNumber, root->getSerial(), 32);
               }
               success = (hwInfo.vendor[0] != 0) || (hwInfo.productName[0] != 0) || (hwInfo.productVersion[0] != 0) || (hwInfo.serialNumber[0] != 0);
            }
         }
         unlockProperties();
      }
   }

   bool modified = false;
   if (success)
   {
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): system hardware information: vendor=%s product=%s pcode=%s version=%s serial=%s"),
               m_name, hwInfo.vendor, hwInfo.productName, hwInfo.productCode, hwInfo.productVersion, hwInfo.serialNumber);

      lockProperties();
      if (updateSystemHardwareProperty(m_vendor, hwInfo.vendor, _T("vendor"), requestId))
         modified = true;
      if (updateSystemHardwareProperty(m_productName, hwInfo.productName, _T("product name"), requestId))
         modified = true;
      if (updateSystemHardwareProperty(m_productCode, hwInfo.productCode, _T("product code"), requestId))
         modified = true;
      if (updateSystemHardwareProperty(m_productVersion, hwInfo.productVersion, _T("product version"), requestId))
         modified = true;
      if (updateSystemHardwareProperty(m_serialNumber, hwInfo.serialNumber, _T("serial number"), requestId))
         modified = true;
      unlockProperties();

      if (modified)
         markAsModified(MODIFY_NODE_PROPERTIES);
   }

   return modified;
}

/**
 * Update list of hardware components for node
 */
bool Node::updateHardwareComponents(PollerInfo *poller, uint32_t requestId)
{
   if (!(m_capabilities & NC_IS_NATIVE_AGENT))
      return false;

   poller->setStatus(_T("hardware check"));
   sendPollerMsg(_T("Reading list of installed hardware components\r\n"));

   ObjectArray<HardwareComponent> *components = new ObjectArray<HardwareComponent>(16, 16, Ownership::True);
   int readCount = ReadBaseboardInformation(this, components);
   readCount += ReadHardwareInformation(this, components, HWC_PROCESSOR, _T("Hardware.Processors"));
   readCount += ReadHardwareInformation(this, components, HWC_MEMORY, _T("Hardware.MemoryDevices"));
   readCount += ReadHardwareInformation(this, components, HWC_STORAGE, _T("Hardware.StorageDevices"));
   readCount += ReadHardwareInformation(this, components, HWC_BATTERY, _T("Hardware.Batteries"));
   readCount += ReadHardwareInformation(this, components, HWC_NETWORK_ADAPTER, _T("Hardware.NetworkAdapters"));

   if (readCount == 0)
   {
      sendPollerMsg(POLLER_WARNING _T("Cannot read hardware component information\r\n"));
      delete components;
      return false;
   }
   sendPollerMsg(POLLER_INFO _T("Received information on %d hardware components\r\n"), components->size());

   static const TCHAR *categoryNames[] = { _T("Other"), _T("Baseboard"), _T("Processor"), _T("Memory device"), _T("Storage device"), _T("Battery"), _T("Network adapter") };

   lockProperties();
   if (m_hardwareComponents != nullptr)
   {
      ObjectArray<HardwareComponent> *changes = CalculateHardwareChanges(m_hardwareComponents, components);
      for(int i = 0; i < changes->size(); i++)
      {
         HardwareComponent *c = changes->get(i);
         switch(c->getChangeCode())
         {
            case CHANGE_NONE:
            case CHANGE_UPDATED: // should not be set for hardware component
               break;
            case CHANGE_ADDED:
               nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): new hardware component \"%s %s (%s)\" serial number %s added"),
                        m_name, categoryNames[c->getCategory()], c->getModel(), c->getType(), c->getSerialNumber());
               sendPollerMsg(_T("   %s %s (%s) added, serial number %s\r\n"),
                        categoryNames[c->getCategory()], c->getModel(), c->getType(), c->getSerialNumber());
               EventBuilder(EVENT_HARDWARE_COMPONENT_ADDED, m_id)
                  .param(_T("category"), categoryNames[c->getCategory()])
                  .param(_T("type"), c->getType())
                  .param(_T("vendor"), c->getVendor())
                  .param(_T("model"), c->getModel())
                  .param(_T("location"), c->getLocation())
                  .param(_T("partNumber"), c->getPartNumber())
                  .param(_T("serialNumber"), c->getSerialNumber())
                  .param(_T("capacity"), c->getCapacity())
                  .param(_T("description"), c->getDescription())
                  .post();
               break;
            case CHANGE_REMOVED:
               nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): hardware component \"%s %s (%s)\" serial number %s removed"),
                        m_name, categoryNames[c->getCategory()], c->getModel(), c->getType(), c->getSerialNumber());
               sendPollerMsg(_T("   %s %s (%s) removed, serial number %s\r\n"),
                        categoryNames[c->getCategory()], c->getModel(), c->getType(), c->getSerialNumber());
               EventBuilder(EVENT_HARDWARE_COMPONENT_REMOVED, m_id)
                  .param(_T("category"), categoryNames[c->getCategory()])
                  .param(_T("type"), c->getType())
                  .param(_T("vendor"), c->getVendor())
                  .param(_T("model"), c->getModel())
                  .param(_T("location"), c->getLocation())
                  .param(_T("partNumber"), c->getPartNumber())
                  .param(_T("serialnumber"), c->getSerialNumber())
                  .param(_T("capacity"), c->getCapacity())
                  .param(_T("description"), c->getDescription())
                  .post();
               break;
         }
      }
      delete changes;
      delete m_hardwareComponents;
      setModified(MODIFY_HARDWARE_INVENTORY);
   }
   else if (components != nullptr)
   {
      setModified(MODIFY_HARDWARE_INVENTORY);
   }

   m_hardwareComponents = components;
   unlockProperties();
   return true;
}

/**
 * Update list of software packages for node
 */
bool Node::updateSoftwarePackages(PollerInfo *poller, uint32_t requestId)
{
   if (!(m_capabilities & NC_IS_NATIVE_AGENT))
      return false;

   poller->setStatus(_T("software check"));
   sendPollerMsg(_T("Reading list of installed software packages\r\n"));

   shared_ptr<Table> table;
   if (getTableFromAgent(_T("System.InstalledProducts"), &table) != DCE_SUCCESS)
   {
      sendPollerMsg(POLLER_WARNING _T("Unable to get information about installed software packages\r\n"));
      return false;
   }

   ObjectArray<SoftwarePackage> *packages = new ObjectArray<SoftwarePackage>(table->getNumRows(), 16, Ownership::True);
   for(int i = 0; i < table->getNumRows(); i++)
   {
      SoftwarePackage *pkg = SoftwarePackage::createFromTableRow(*table, i);
      if (pkg != nullptr)
         packages->add(pkg);
   }
   packages->sort(PackageNameVersionComparator);
   for(int i = 0; i < packages->size(); i++)
   {
      SoftwarePackage *curr = packages->get(i);
      for(int j = i + 1; j < packages->size(); j++)
      {
         SoftwarePackage *next = packages->get(j);
         if (_tcscmp(curr->getName(), next->getName()))
            break;
         if (!_tcscmp(curr->getVersion(), next->getVersion()))
         {
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_CONF_POLL, _T("Inconsistent software package data received from node %s [%u]: duplicate entry for package %s version %s"),
                     m_name, m_id, curr->getName(), curr->getVersion());
            packages->remove(j);
            j--;
         }
      }
   }
   sendPollerMsg(POLLER_INFO _T("Got information about %d installed software packages\r\n"), packages->size());

   lockProperties();
   if (m_softwarePackages != nullptr)
   {
      ObjectArray<SoftwarePackage> *changes = CalculatePackageChanges(m_softwarePackages, packages);
      for(int i = 0; i < changes->size(); i++)
      {
         SoftwarePackage *p = changes->get(i);
         switch(p->getChangeCode())
         {
            case CHANGE_NONE:
               break;
            case CHANGE_ADDED:
               nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): new package %s version %s"), m_name, p->getName(), p->getVersion());
               sendPollerMsg(_T("   New package %s version %s\r\n"), p->getName(), p->getVersion());
               EventBuilder(EVENT_PACKAGE_INSTALLED, m_id)
                  .param(_T("name"), p->getName())
                  .param(_T("version"), p->getVersion())
                  .post();

               break;
            case CHANGE_REMOVED:
               nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): package %s version %s removed"), m_name, p->getName(), p->getVersion());
               sendPollerMsg(_T("   Package %s version %s removed\r\n"), p->getName(), p->getVersion());
               EventBuilder(EVENT_PACKAGE_REMOVED, m_id)
                  .param(_T("name"), p->getName())
                  .param(_T("version"), p->getVersion())
                  .post();
               break;
            case CHANGE_UPDATED:
               SoftwarePackage *prev = changes->get(++i);   // next entry contains previous package version
               nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): package %s updated (%s -> %s)"), m_name, p->getName(), prev->getVersion(), p->getVersion());
               sendPollerMsg(_T("   Package %s updated (%s -> %s)\r\n"), p->getName(), prev->getVersion(), p->getVersion());
               EventBuilder(EVENT_PACKAGE_UPDATED, m_id)
                  .param(_T("name"), p->getName())
                  .param(_T("version"), p->getVersion())
                  .param(_T("previousVersion"), prev->getVersion())
                  .post();
               break;
         }
      }
      delete changes;
      delete m_softwarePackages;
      setModified(MODIFY_SOFTWARE_INVENTORY);
   }
   else if (packages != nullptr)
   {
      setModified(MODIFY_SOFTWARE_INVENTORY);
   }

   m_softwarePackages = packages;
   unlockProperties();
   return true;
}

/**
 * Data for DeleteDuplicateNode
 */
struct DeleteDuplicateNodeData
{
   shared_ptr<Node> originalNode;
   shared_ptr<Node> duplicateNode;
   TCHAR *reason;

   DeleteDuplicateNodeData(shared_ptr<Node> original, shared_ptr<Node> duplicate, const TCHAR *_reason) : originalNode(original), duplicateNode(duplicate)
   {
      reason = MemCopyString(_reason);
   }

   ~DeleteDuplicateNodeData()
   {
      MemFree(reason);
   }
};

/**
 * Background worker for duplicate node delete
 */
static void DeleteDuplicateNode(DeleteDuplicateNodeData *data)
{
   EventBuilder(EVENT_DUPLICATE_NODE_DELETED, g_dwMgmtNode)
      .param(_T("originalNodeObjectId"), data->originalNode->getId())
      .param(_T("originalNodeName"), data->originalNode->getName())
      .param(_T("originalNodePrimaryHostName"), data->originalNode->getPrimaryHostName().cstr())
      .param(_T("duplicateNodeObjectId"), data->duplicateNode->getId())
      .param(_T("duplicateNodeName"), data->duplicateNode->getName())
      .param(_T("duplicateNodePrimaryHostName"), data->duplicateNode->getName())
      .param(_T("reason"), data->reason)
      .post();

   data->duplicateNode->deleteObject();
   // Calling updateObjectIndexes will update all indexes that could be broken
   // by deleting duplicate IP address entries
   data->originalNode->updateObjectIndexes();
   delete data;
}

/**
 * Exit from "unreachable" state if full configuration poll was successful on any communication method (agent, SNMP, etc.)
 */
#define ExitFromUnreachableState() do { \
   if ((m_runtimeFlags & NDF_RECHECK_CAPABILITIES) && (m_state & DCSF_UNREACHABLE)) \
   { \
      int reason = (m_state & DCSF_NETWORK_PATH_PROBLEM) ? 1 : 0; \
      m_state &= ~(DCSF_UNREACHABLE | DCSF_NETWORK_PATH_PROBLEM); \
      EventBuilder(EVENT_NODE_UP, m_id) \
         .param(_T("reason"), reason) \
         .post(); \
      sendPollerMsg(POLLER_INFO _T("Node recovered from unreachable state\r\n")); \
      m_recoveryTime = time(nullptr); \
   } \
} while(0)

/**
 * Perform configuration poll on node
 */
void Node::configurationPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
{
   lockProperties();
   if (m_isDeleteInitiated || IsShutdownInProgress())
   {
      m_configurationPollState.complete(0);
      unlockProperties();
      return;
   }
   unlockProperties();

   poller->setStatus(_T("wait for lock"));
   pollerLock(configuration);

   POLL_CANCELLATION_CHECKPOINT();

   m_pollRequestor = session;
   m_pollRequestId = rqId;

   if (m_status == STATUS_UNMANAGED)
   {
      sendPollerMsg(POLLER_WARNING _T("Node %s is unmanaged, configuration poll aborted\r\n"), m_name);
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("Node %s [%u] is unmanaged, configuration poll aborted"), m_name, m_id);
      pollerUnlock();
      return;
   }

   uint64_t oldCapabilities = m_capabilities;
   uint32_t modified = 0;

   sendPollerMsg(_T("Starting configuration of for node %s\r\n"), m_name);
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 4, _T("Starting configuration poll of node %s (ID: %d)"), m_name, m_id);

   // Check for forced capabilities recheck
   if (m_runtimeFlags & NDF_RECHECK_CAPABILITIES)
   {
      sendPollerMsg(POLLER_WARNING _T("Capability reset\r\n"));
      lockProperties();
      m_capabilities &= (NC_IS_LOCAL_MGMT | m_expectedCapabilities); // reset all except "local management" flag and expected
      m_runtimeFlags &= ~ODF_CONFIGURATION_POLL_PASSED;
      m_snmpObjectId = SNMP_ObjectId();
      m_platformName[0] = 0;
      m_agentVersion[0] = 0;
      MemFreeAndNull(m_sysDescription);
      MemFreeAndNull(m_sysName);
      MemFreeAndNull(m_sysContact);
      MemFreeAndNull(m_sysLocation);
      MemFreeAndNull(m_lldpNodeId);
      m_hypervisorType[0] = 0;
      m_hypervisorInfo = nullptr;
      m_vendor = nullptr;
      m_productCode = nullptr;
      m_productName = nullptr;
      m_productVersion = nullptr;
      m_serialNumber = nullptr;
      unlockProperties();
   }

   updatePrimaryIpAddr();

   poller->setStatus(_T("capability check"));
   sendPollerMsg(_T("Checking node's capabilities...\r\n"));

   if (confPollAgent())
      modified |= MODIFY_NODE_PROPERTIES;

   if ((oldCapabilities & NC_IS_NATIVE_AGENT) && !(m_capabilities & NC_IS_NATIVE_AGENT))
      m_lastAgentCommTime = TIMESTAMP_NEVER;

   POLL_CANCELLATION_CHECKPOINT();

   // Setup permanent connection to agent if not present (needed for proper configuration re-sync)
   if (m_capabilities & NC_IS_NATIVE_AGENT)
   {
      uint32_t error, socketError;
      agentLock();
      bool connected = connectToAgent(&error, &socketError, nullptr, true);
      agentUnlock();

      if (connected)
      {
         // Check if agent was marked as unreachable before full poll
         if ((m_state & NSF_AGENT_UNREACHABLE) && (m_runtimeFlags & NDF_RECHECK_CAPABILITIES))
         {
            m_state &= ~NSF_AGENT_UNREACHABLE;
            PostSystemEvent(EVENT_AGENT_OK, m_id);
            sendPollerMsg(POLLER_INFO _T("   Connectivity with NetXMS agent restored\r\n"));
            m_pollCountAgent = 0;

            // Reset connection time of all proxy connections so they can be re-established immediately
            for(int i = 0; i < MAX_PROXY_TYPE; i++)
            {
               m_proxyConnections[i].lock();
               m_proxyConnections[i].setLastConnectTime(0);
               m_proxyConnections[i].unlock();
            }
         }

         ExitFromUnreachableState();
      }
      else
      {
         wchar_t errorText[256];
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("ConfPoll(%s): agent unreachable, error=\"%u %s\", socketError=\"%s\""),
                  m_name, error, AgentErrorCodeToText(error), GetSocketErrorText(socketError, errorText, 256));
      }
   }

   POLL_CANCELLATION_CHECKPOINT();

   if (confPollSnmp())
      modified |= MODIFY_NODE_PROPERTIES;

   // Check if SNMP was marked as unreachable before full poll
   if ((m_capabilities & NC_IS_SNMP) && !(m_expectedCapabilities & NC_IS_SNMP) && (m_state & NSF_SNMP_UNREACHABLE) && (m_runtimeFlags & NDF_RECHECK_CAPABILITIES))
   {
      m_state &= ~NSF_SNMP_UNREACHABLE;
      PostSystemEvent(EVENT_SNMP_OK, m_id);
      sendPollerMsg(POLLER_INFO _T("   Connectivity with SNMP agent restored\r\n"));
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("ConfPoll(%s): Connectivity with SNMP agent restored"), m_name);
      m_pollCountSNMP = 0;
   }

   POLL_CANCELLATION_CHECKPOINT();

   if (confPollSsh())
      modified |= MODIFY_NODE_PROPERTIES;

   // Check if SSH was marked as unreachable before full poll
   if ((m_capabilities & NC_IS_SSH) && !(m_expectedCapabilities & NC_IS_SSH) && (m_state & NSF_SSH_UNREACHABLE) && (m_runtimeFlags & NDF_RECHECK_CAPABILITIES))
   {
      m_state &= ~NSF_SSH_UNREACHABLE;
      PostSystemEvent(EVENT_SSH_OK, m_id);
      sendPollerMsg(POLLER_INFO _T("   SSH connectivity restored\r\n"));
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("ConfPoll(%s): SSH connectivity restored"), m_name);
      m_pollCountSSH = 0;
   }

   POLL_CANCELLATION_CHECKPOINT();

   if (confPollVnc())
      modified |= MODIFY_NODE_PROPERTIES;

   POLL_CANCELLATION_CHECKPOINT();

   if (confPollEthernetIP())
      modified |= MODIFY_NODE_PROPERTIES;

   // Check if Ethernet/IP was marked as unreachable before full poll
   if ((m_capabilities & NC_IS_ETHERNET_IP) && !(m_expectedCapabilities & NC_IS_ETHERNET_IP) && (m_state & NSF_ETHERNET_IP_UNREACHABLE) && (m_runtimeFlags & NDF_RECHECK_CAPABILITIES))
   {
      m_state &= ~NSF_ETHERNET_IP_UNREACHABLE;
      PostSystemEvent(EVENT_ETHERNET_IP_OK, m_id);
      sendPollerMsg(POLLER_INFO _T("   EtherNet/IP connectivity restored\r\n"));
      m_pollCountEtherNetIP = 0;
   }

   POLL_CANCELLATION_CHECKPOINT();

   if (confPollModbus())
      modified |= MODIFY_NODE_PROPERTIES;

   // Check if MODBUS was marked as unreachable before full poll
   if ((m_capabilities & NC_IS_MODBUS_TCP) && !(m_expectedCapabilities & NC_IS_MODBUS_TCP) && (m_state & NSF_MODBUS_UNREACHABLE) && (m_runtimeFlags & NDF_RECHECK_CAPABILITIES))
   {
      m_state &= ~NSF_MODBUS_UNREACHABLE;
      PostSystemEvent(EVENT_MODBUS_OK, m_id);
      sendPollerMsg(POLLER_INFO _T("   Modbus connectivity restored\r\n"));
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("ConfPoll(%s): Modbus connectivity restored"), m_name);
      m_pollCountModbus = 0;
   }

   POLL_CANCELLATION_CHECKPOINT();

   // Generate event if node flags has been changed
   if (oldCapabilities != m_capabilities)
   {
      EventBuilder(EVENT_NODE_CAPABILITIES_CHANGED, m_id)
         .param(_T("oldCapabilities"), oldCapabilities, EventBuilder::HEX_64BIT_FORMAT)
         .param(_T("newCapabilities"), m_capabilities, EventBuilder::HEX_64BIT_FORMAT)
         .post();
      modified |= MODIFY_NODE_PROPERTIES;
   }

   // Update system description
   wchar_t buffer[MAX_RESULT_LENGTH] = { 0 };
   if ((m_capabilities & NC_IS_NATIVE_AGENT) && !(m_flags & NF_DISABLE_NXCP))
   {
      getMetricFromAgent(L"System.Uname", buffer, MAX_RESULT_LENGTH);
   }
   else if ((m_capabilities & NC_IS_SNMP) && !(m_flags & NF_DISABLE_SNMP))
   {
      getMetricFromSNMP(m_snmpPort, SNMP_VERSION_DEFAULT, L"1.3.6.1.2.1.1.1.0", buffer, MAX_RESULT_LENGTH, SNMP_RAWTYPE_NONE);
   }

   if (buffer[0] != _T('\0'))
   {
      TranslateStr(buffer, _T("\r\n"), _T(" "));
      TranslateStr(buffer, _T("\n"), _T(" "));
      TranslateStr(buffer, _T("\r"), _T(" "));

      lockProperties();

      if ((m_sysDescription == nullptr) || _tcscmp(m_sysDescription, buffer))
      {
         MemFree(m_sysDescription);
         m_sysDescription = MemCopyString(buffer);
         modified |= MODIFY_NODE_PROPERTIES;
         sendPollerMsg(_T("   System description changed to %s\r\n"), m_sysDescription);
      }

      unlockProperties();
   }

   // Retrieve interface list
   poller->setStatus(_T("interface check"));
   sendPollerMsg(_T("Capability check finished\r\n"));

   if (updateInterfaceConfiguration(rqId))
      modified |= MODIFY_NODE_PROPERTIES;

   POLL_CANCELLATION_CHECKPOINT();

   if (g_flags & AF_MERGE_DUPLICATE_NODES)
   {
      shared_ptr<Node> duplicateNode;
      TCHAR reason[1024];
      DuplicateCheckResult dcr = checkForDuplicates(&duplicateNode, reason, 1024);
      if (dcr == REMOVE_THIS)
      {
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 3, _T("Removing node %s [%u] as duplicate"), m_name, m_id);

         poller->setStatus(_T("cleanup"));
         lockProperties();
         m_runtimeFlags &= ~(ODF_CONFIGURATION_POLL_PENDING | NDF_RECHECK_CAPABILITIES);
         unlockProperties();
         pollerUnlock();

         duplicateNode->reconcileWithDuplicateNode(this);
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 4, _T("Aborted configuration poll of node %s (ID: %d)"), m_name, m_id);
         ThreadPoolExecute(g_pollerThreadPool, DeleteDuplicateNode, new DeleteDuplicateNodeData(duplicateNode, self(), reason));
         return;
      }
      else if (dcr == REMOVE_OTHER)
      {
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 3, _T("Removing node %s [%u] as duplicate"), duplicateNode->getName(), duplicateNode->getId());
         reconcileWithDuplicateNode(duplicateNode.get());
         ThreadPoolExecute(g_pollerThreadPool, DeleteDuplicateNode, new DeleteDuplicateNodeData(self(), duplicateNode, reason));
      }
   }

   // Check node name
   sendPollerMsg(_T("Checking node name\r\n"));
   if (g_flags & AF_RESOLVE_NODE_NAMES)
   {
      InetAddress addr = InetAddress::parse(m_name);
      if (addr.isValidUnicast())
      {
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("ConfPoll(%s [%u]): node name is an IP address and need to be resolved"), m_name, m_id);
         sendPollerMsg(_T("Node name is an IP address and need to be resolved\r\n"));
         poller->setStatus(_T("resolving name"));
         const TCHAR *facility;
         if (resolveName(false, &facility))
         {
            sendPollerMsg(POLLER_INFO _T("Node name resolved to %s using %s\r\n"), m_name, facility);
            nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 4, _T("ConfPoll(%s [%u]): node name resolved using %s"), m_name, m_id, facility);
            modified |= MODIFY_COMMON_PROPERTIES;
         }
         else
         {
            sendPollerMsg(POLLER_WARNING _T("Node name cannot be resolved\r\n"));
            nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 4, _T("ConfPoll(%s [%u]): node name cannot be resolved"), m_name, m_id);
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("ConfPoll(%s [%u]): node name cannot be interpreted as valid IP address"), m_name, m_id);
         sendPollerMsg(_T("Node name cannot be interpreted as valid IP address, no need to resolve to host name\r\n"));
      }
   }
   else if (g_flags & AF_SYNC_NODE_NAMES_WITH_DNS)
   {
      sendPollerMsg(_T("Synchronizing node name with DNS\r\n"));
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("ConfPoll(%s [%u]): synchronizing node name with DNS"), m_name, m_id);
      poller->setStatus(_T("resolving name"));
      const TCHAR *facility;
      if (resolveName(true, &facility))
      {
         sendPollerMsg(POLLER_INFO _T("Node name resolved to %s\r\n"), m_name);
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 4, _T("ConfPoll(%s [%u]): node name resolved"), m_name, m_id);
         modified |= MODIFY_COMMON_PROPERTIES;
      }
   }
   else
   {
      sendPollerMsg(_T("Node name is OK\r\n"));
   }

   POLL_CANCELLATION_CHECKPOINT();

   updateSystemHardwareInformation(poller, rqId);
   updateHardwareComponents(poller, rqId);
   updateSoftwarePackages(poller, rqId);

   POLL_CANCELLATION_CHECKPOINT();

   // Update node type
   TCHAR hypervisorType[MAX_HYPERVISOR_TYPE_LENGTH], hypervisorInfo[MAX_HYPERVISOR_INFO_LENGTH];
   NodeType type = detectNodeType(hypervisorType, hypervisorInfo);
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): detected node type: %d (%s)"), m_name, type, typeName(type));
   if ((type == NODE_TYPE_VIRTUAL) || (type == NODE_TYPE_CONTAINER))
   {
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): hypervisor: %s (%s)"), m_name, hypervisorType, hypervisorInfo);
   }
   lockProperties();
   if ((type != NODE_TYPE_UNKNOWN) &&
            ((type != m_type) || _tcscmp(hypervisorType, m_hypervisorType) || _tcscmp(hypervisorInfo, CHECK_NULL_EX(m_hypervisorInfo))))
   {
      m_type = type;
      modified |= MODIFY_NODE_PROPERTIES;
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): node type set to %d (%s)"), m_name, type, typeName(type));
      sendPollerMsg(_T("   Node type changed to %s\r\n"), typeName(type));

      if (*hypervisorType != 0)
         sendPollerMsg(_T("   Hypervisor type set to %s\r\n"), hypervisorType);
      _tcslcpy(m_hypervisorType, hypervisorType, MAX_HYPERVISOR_TYPE_LENGTH);

      if (*hypervisorInfo != 0)
         sendPollerMsg(_T("   Hypervisor information set to %s\r\n"), hypervisorInfo);
      m_hypervisorInfo = hypervisorInfo;
   }
   unlockProperties();

   POLL_CANCELLATION_CHECKPOINT();

   // Network device configuration backup integration
   if (g_flags & AF_DEVICE_BACKUP_API_ENABLED)
   {
      poller->setStatus(_T("backup"));
      if (DevBackupIsDeviceRegistered(*this))
      {
         DeviceBackupApiStatus status = DevBackupValidateDeviceRegistration(this);
         if (status == DeviceBackupApiStatus::SUCCESS)
         {
            std::pair<DeviceBackupApiStatus, DeviceBackupJobStatus> jobStatus = DevBackupGetLastJobStatus(*this);
            m_lastConfigBackupJobStatus = (jobStatus.first == DeviceBackupApiStatus::SUCCESS) ? jobStatus.second : DeviceBackupJobStatus::UNKNOWN;
         }
         else
         {
            sendPollerMsg(POLLER_ERROR _T("Cannot validate registration for device configuration backup (%s)\r\n"), GetDeviceBackupApiErrorMessage(status));
            nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfigurationPoll(%s [%u]): registration for device configuration backup failed (%s)"), m_name, m_id, GetDeviceBackupApiErrorMessage(status));
            if (status == DeviceBackupApiStatus::DEVICE_NOT_REGISTERED)
               m_capabilities &= ~NC_REGISTERED_FOR_BACKUP;
         }
      }
      else
      {
         bool pass;
         ScriptVMHandle vm = CreateServerScriptVM(_T("Hook::RegisterForConfigurationBackup"), self());
         if (vm.isValid())
         {
            vm->setUserData(this);
            if (vm->run())
            {
               pass = vm->getResult()->getValueAsBoolean();
            }
            else
            {
               pass = false;  // Consider hook runtime error as blocking
               ReportScriptError(SCRIPT_CONTEXT_OBJECT, this, 0, vm->getErrorText(), _T("Hook::RegisterForConfigurationBackup"));
               nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfigurationPoll(%s [%u]): hook script Hook::RegisterForConfigurationBackup execution error: %s"), m_name, m_id, vm->getErrorText());
               sendPollerMsg(POLLER_ERROR _T("Runtime error in configuration backup registration hook script (%s)\r\n"), vm->getErrorText());
            }
            vm.destroy();
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 7, _T("ConfigurationPoll(%s [%u]): hook script \"Hook::RegisterForConfigurationBackup\" %s"), m_name, m_id, HookScriptLoadErrorToText(vm.failureReason()));
            pass = (vm.failureReason() == ScriptVMFailureReason::SCRIPT_IS_EMPTY) || (vm.failureReason() == ScriptVMFailureReason::SCRIPT_NOT_FOUND);
         }

         if (pass)
         {
            DeviceBackupApiStatus status = DevBackupRegisterDevice(this);
            if (status == DeviceBackupApiStatus::SUCCESS)
            {
               m_capabilities |= NC_REGISTERED_FOR_BACKUP;
               sendPollerMsg(POLLER_INFO _T("Successfully registered for device configuration backup\r\n"));
            }
            else
            {
               m_capabilities &= ~NC_REGISTERED_FOR_BACKUP;
               sendPollerMsg(_T("Registration for device configuration backup failed (%s)\r\n"), GetDeviceBackupApiErrorMessage(status));
               nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfigurationPoll(%s [%u]): registration for device configuration backup failed (%s)"), m_name, m_id, GetDeviceBackupApiErrorMessage(status));
            }
         }
         else
         {
            m_capabilities &= ~NC_REGISTERED_FOR_BACKUP;
            sendPollerMsg(_T("Registration for device configuration backup blocked by hook script\r\n"));
            nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfigurationPoll(%s [%u]): registration for device configuration backup blocked by hook script"), m_name, m_id);
         }
      }
   }

   // Call hooks in loaded modules
   ENUMERATE_MODULES(pfConfPollHook)
   {
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfigurationPoll(%s [%d]): calling hook in module %s"), m_name, m_id, CURRENT_MODULE.name);
      if (CURRENT_MODULE.pfConfPollHook(this, session, rqId, poller))
         modified |= MODIFY_ALL;   // FIXME: change module call to get exact modifications
   }

   POLL_CANCELLATION_CHECKPOINT();

   // Execute hook script
   poller->setStatus(_T("hook"));
   executeHookScript(_T("ConfigurationPoll"));

   POLL_CANCELLATION_CHECKPOINT();

   poller->setStatus(_T("autobind"));
   applyTemplates();
   updateContainerMembership();
   updateClusterMembership();

   sendPollerMsg(_T("Finished configuration poll of node %s\r\n"), m_name);
   sendPollerMsg(_T("Node configuration was%schanged after poll\r\n"), (modified != 0) ? _T(" ") : _T(" not "));

   m_runtimeFlags &= ~ODF_CONFIGURATION_POLL_PENDING;
   m_runtimeFlags |= ODF_CONFIGURATION_POLL_PASSED;

   UpdateAssetLinkage(this);
   autoFillAssetProperties();

   // Finish configuration poll
   poller->setStatus(_T("cleanup"));
   lockProperties();
   m_runtimeFlags &= ~NDF_RECHECK_CAPABILITIES;
   m_pollRequestor = nullptr;
   unlockProperties();
   pollerUnlock();
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 4, _T("Finished configuration poll of node %s (ID: %d)"), m_name, m_id);

   if (modified != 0)
   {
      markAsModified(modified);
   }
}

/**
 * Check if agent on given node is restarting
 */
static inline bool IsProxyAgentRestarting(uint32_t proxyId, time_t now)
{
   if (proxyId == 0)
      return false;
   shared_ptr<NetObj> proxyNode = FindObjectById(proxyId, OBJECT_NODE);
   return (proxyNode != nullptr) && (static_cast<Node&>(*proxyNode).getAgentRestartTime() + g_agentRestartWaitTime >= now);
}

/**
 * Check if any of the agent's proxies can still be in process of restarting
 */
bool Node::isProxyAgentRestarting()
{
   time_t now = time(nullptr);
   return IsProxyAgentRestarting(getEffectiveAgentProxy(), now) ||
          IsProxyAgentRestarting(getEffectiveSnmpProxy(), now) ||
          IsProxyAgentRestarting(getEffectiveEtherNetIPProxy(), now) ||
          IsProxyAgentRestarting(getEffectiveIcmpProxy(), now);
}

/**
 * Filter node index by zone UIN
 */
static bool FilterByZone(NetObj *object, void *zoneUIN)
{
   return static_cast<Node*>(object)->getZoneUIN() == CAST_FROM_POINTER(zoneUIN, int32_t);
}

/**
 * Check for duplicate nodes
 */
DuplicateCheckResult Node::checkForDuplicates(shared_ptr<Node> *duplicate, TCHAR *reason, size_t size)
{
   DuplicateCheckResult result = NO_DUPLICATES;
   unique_ptr<SharedObjectArray<NetObj>> nodes = g_idxNodeById.getObjects(FilterByZone, CAST_TO_POINTER(m_zoneUIN, void*));
   int i;
   for(i = 0; i < nodes->size(); i++)
   {
      Node *node = static_cast<Node*>(nodes->get(i));
      if (node->m_id == m_id)
         continue;

      if (isDuplicateOf(node, reason, 1024))
      {
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 3, _T("Node %s [%u] is a duplicate of node %s [%u]"),
                  m_name, m_id, node->getName(), node->getId());

         if ((node->m_status == STATUS_UNMANAGED) || (node->m_state & DCSF_UNREACHABLE))
         {
            result = REMOVE_OTHER;
         }
         else if (node->isNativeAgent() && !isNativeAgent())
         {
            result = REMOVE_THIS;
         }
         else if (!node->isNativeAgent() && isNativeAgent())
         {
            result = REMOVE_OTHER;
         }
         else if (node->isSNMPSupported() && !isSNMPSupported())
         {
            result = REMOVE_THIS;
         }
         else if (!node->isSNMPSupported() && isSNMPSupported())
         {
            result = REMOVE_OTHER;
         }
         else if (node->m_id > m_id)
         {
            result = REMOVE_OTHER;
         }
         else
         {
            result = REMOVE_THIS;
         }
         *duplicate = node->self();
         break;
      }
   }
   return result;
}

/**
 * Check if this node is a duplicate of given node
 */
bool Node::isDuplicateOf(Node *node, TCHAR *reason, size_t size)
{
   // Check if primary IP is on one of other node's interfaces
   if (!(m_flags & NF_EXTERNAL_GATEWAY) && m_ipAddress.isValidUnicast() &&
       !(node->getFlags() &  NF_EXTERNAL_GATEWAY) && node->getIpAddress().isValidUnicast())
   {
      shared_ptr<Interface> iface = node->findInterfaceByIP(m_ipAddress);
      if (iface != nullptr)
      {
         _sntprintf(reason, size, _T("Primary IP address %s of node %s [%u] found on interface %s of node %s [%u]"),
                  (const TCHAR *)m_ipAddress.toString(), m_name, m_id, iface->getName(), node->getName(), node->getId());
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 3, _T("%s"), reason);
         return true;
      }
   }

   // Check for exact match of interface list
   //TODO

   return false;
}

/**
 * Reconcile with duplicate node.
 *
 * @param node Pointer to duplicate node about to be deleted.
 */
void Node::reconcileWithDuplicateNode(Node *node)
{
   // Copy all non-template DCIs
   node->readLockDciAccess();
   writeLockDciAccess();

   for(int i = 0; i < node->m_dcObjects.size(); i++)
   {
      DCObject *dci = node->m_dcObjects.get(i);
      if (dci->getTemplateId() != 0)
         continue;

      // Check if this node already have same DCI
      bool found = false;
      for(int j = 0; j < m_dcObjects.size(); j++)
      {
         DCObject *curr = m_dcObjects.get(j);
         if ((curr->getDataSource() == dci->getDataSource()) &&
             (curr->getSourceNode() == dci->getSourceNode()) &&
             !_tcsicmp(curr->getName(), dci->getName()))
         {
            found = true;
            break;
         }
      }

      if (!found)
      {
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("Creating copy of DCI \"%s\" [%u] from node %s [%u] on node %s [%u]"),
                  dci->getName().cstr(), dci->getId(), node->m_name, node->m_id, m_name, m_id);

         DCObject *dciCopy = dci->clone();
         dciCopy->changeBinding(CreateUniqueId(IDG_ITEM), self(), false);
         addDCObject(dciCopy, true);
      }
   }

   unlockDciAccess();
   node->unlockDciAccess();

   // Apply all manual templates from duplicate node
   node->readLockParentList();
   for(int i = 0; i < node->getParentList().size(); i++)
   {
      NetObj *object = node->getParentList().get(i);
      if (object->getObjectClass() != OBJECT_TEMPLATE)
         continue;

      if (static_cast<Template*>(object)->isAutoBindEnabled())
         continue;

      if (!object->isDirectChild(m_id))
      {
         g_templateUpdateQueue.put(new TemplateUpdateTask(static_pointer_cast<DataCollectionOwner>(object->self()), m_id, APPLY_TEMPLATE, false));
      }
   }
   node->unlockParentList();
}

/**
 * Detect node type
 */
NodeType Node::detectNodeType(TCHAR *hypervisorType, TCHAR *hypervisorInfo)
{
   NodeType type = NODE_TYPE_UNKNOWN;
   *hypervisorType = 0;
   *hypervisorInfo = 0;

   if (m_capabilities & NC_IS_SNMP)
   {
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("Node::detectNodeType(%s [%d]): SNMP node, driver name is %s"), m_name, m_id, m_driver->getName());

      bool vtypeReportedByDevice = false;
      SNMP_Transport *snmp = createSnmpTransport();
      if (snmp != nullptr)
      {
         VirtualizationType vt;
         vtypeReportedByDevice = m_driver->getVirtualizationType(snmp, this, m_driverData, &vt);
         delete snmp;
         if (vtypeReportedByDevice)
         {
            if (vt != VTYPE_NONE)
               type = (vt == VTYPE_FULL) ? NODE_TYPE_VIRTUAL : NODE_TYPE_CONTAINER;
            else
              type = NODE_TYPE_PHYSICAL;
         }
      }

      if (!vtypeReportedByDevice)
      {
         // Assume physical device if it supports SNMP and driver is not "GENERIC" nor "NET-SNMP"
         if (_tcscmp(m_driver->getName(), _T("GENERIC")) && _tcscmp(m_driver->getName(), _T("NET-SNMP")))
         {
            type = NODE_TYPE_PHYSICAL;
         }
         else
         {
            if (m_capabilities & NC_IS_PRINTER)
            {
               // Assume that printers are physical devices
               type = NODE_TYPE_PHYSICAL;
            }
         }
      }
   }
   if (m_capabilities & NC_IS_NATIVE_AGENT)
   {
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("Node::detectNodeType(%s [%d]): NetXMS agent node"), m_name, m_id);

      shared_ptr<AgentConnectionEx> conn = getAgentConnection();
      if (conn != nullptr)
      {
         TCHAR buffer[MAX_RESULT_LENGTH];
         if (conn->getParameter(_T("System.IsVirtual"), buffer, MAX_RESULT_LENGTH) == ERR_SUCCESS)
         {
            VirtualizationType vt = static_cast<VirtualizationType>(_tcstol(buffer, nullptr, 10));
            if (vt != VTYPE_NONE)
            {
               type = (vt == VTYPE_FULL) ? NODE_TYPE_VIRTUAL : NODE_TYPE_CONTAINER;
               if (conn->getParameter(_T("Hypervisor.Type"), buffer, MAX_RESULT_LENGTH) == ERR_SUCCESS)
               {
                  _tcslcpy(hypervisorType, buffer, MAX_HYPERVISOR_TYPE_LENGTH);
                  if (conn->getParameter(_T("Hypervisor.Version"), buffer, MAX_RESULT_LENGTH) == ERR_SUCCESS)
                  {
                     _tcslcpy(hypervisorInfo, buffer, MAX_HYPERVISOR_INFO_LENGTH);
                  }
               }
            }
            else
            {
               type = NODE_TYPE_PHYSICAL;
            }
         }
      }
   }
   return type;
}

/**
 * Configuration poll: check for NetXMS agent
 */
bool Node::confPollAgent()
{
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): checking for NetXMS agent Flags={%08X} StateFlags={%08X} RuntimeFlags={%08X}"), m_name, m_flags, m_state, m_runtimeFlags);
   if (((m_capabilities & NC_IS_NATIVE_AGENT) && (m_state & NSF_AGENT_UNREACHABLE)) || (m_flags & NF_DISABLE_NXCP))
   {
      sendPollerMsg(_T("   NetXMS agent polling is %s\r\n"), (m_flags & NF_DISABLE_NXCP) ? _T("disabled") : _T("not possible"));
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): agent polling is %s"), m_name, (m_flags & NF_DISABLE_NXCP) ? _T("disabled") : _T("not possible"));
      return false;
   }
   bool hasChanges = false;
   sendPollerMsg(_T("   Checking NetXMS agent...\r\n"));
   shared_ptr<AgentConnectionEx> pAgentConn;
   shared_ptr<AgentTunnel> tunnel = GetTunnelForNode(m_id);
   if (tunnel != nullptr)
   {
      pAgentConn = make_shared<AgentConnectionEx>(m_id, tunnel, m_agentSecret, isAgentCompressionAllowed());
   }
   else
   {
      if (m_flags & NF_AGENT_OVER_TUNNEL_ONLY)
      {
         sendPollerMsg(POLLER_ERROR _T("   Direct agent connection is disabled and there are no active tunnels\r\n"));
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): direct agent connection is disabled and there are no active tunnels"), m_name);
         return false;
      }
      InetAddress addr = m_ipAddress;
      if ((m_capabilities & NC_IS_LOCAL_MGMT) && (g_mgmtAgentAddress[0] != 0))
      {
         // If node is local management node and external agent address is set, use it instead of node's primary IP address
         addr = InetAddress::resolveHostName(g_mgmtAgentAddress);
         if (addr.isValid())
         {
            nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s [%u]): using external agent address %s"), m_name, m_id, g_mgmtAgentAddress);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s [%u]): cannot resolve external agent address %s"), m_name, m_id, g_mgmtAgentAddress);
         }
      }
      if (!addr.isValidUnicast() && !((m_capabilities & NC_IS_LOCAL_MGMT) && addr.isLoopback()))
      {
         sendPollerMsg(POLLER_ERROR _T("   Node primary IP is invalid and there are no active tunnels\r\n"));
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): node primary IP is invalid and there are no active tunnels"), m_name);
         return false;
      }
      pAgentConn = make_shared<AgentConnectionEx>(m_id, addr, m_agentPort, m_agentSecret, isAgentCompressionAllowed());
      setAgentProxy(pAgentConn.get());
   }
   pAgentConn->setCommandTimeout(g_agentCommandTimeout);
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): checking for NetXMS agent - connecting"), m_name);

   // Try to connect to agent
   uint32_t rcc;
   if (!pAgentConn->connect(g_serverKey, &rcc))
   {
      //if given port is wrong, try to check all port list
      if (rcc == ERR_CONNECT_FAILED)
      {
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPollAgent(%s): NetXMS agent not connected on port %i"), m_name, m_agentPort);
         IntegerArray<uint16_t> knownPorts = GetWellKnownPorts(_T("agent"), m_zoneUIN);
         for (int i = 0; i < knownPorts.size(); i++)
         {
            uint16_t port = knownPorts.get(i);
            nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("ConfPollAgent(%s): re-checking connection on port %d"), m_name, port);
            pAgentConn->setPort(port);
            bool portSuccess = pAgentConn->connect(g_serverKey, &rcc);
            if (portSuccess || (rcc == ERR_AUTH_REQUIRED) || (rcc == ERR_AUTH_FAILED))
            {
               m_agentPort = port;
               hasChanges = true;
               sendPollerMsg(_T("   NetXMS agent detected on port %d\r\n"), port);
               nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("ConfPollAgent(%s): NetXMS agent connected on port %d"), m_name, m_agentPort);
               break;
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("ConfPollAgent(%s): NetXMS agent couldn't connect on port %d"), m_name, port);
            }
         }
      }

      // If there are authentication problem, try default shared secret
      if ((rcc == ERR_AUTH_REQUIRED) || (rcc == ERR_AUTH_FAILED))
      {
         StringList secrets;
         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT secret FROM shared_secrets WHERE zone=? OR zone=-1 ORDER BY zone DESC, id ASC"));
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_zoneUIN);

            DB_RESULT hResult = DBSelectPrepared(hStmt);
            if (hResult != NULL)
            {
               int count = DBGetNumRows(hResult);
               wchar_t secret[128];
               for(int i = 0; i < count; i++)
                  secrets.add(DBGetField(hResult, i, 0, secret, 128));
               DBFreeResult(hResult);
            }
            DBFreeStatement(hStmt);
         }
         DBConnectionPoolReleaseConnection(hdb);

         for(int i = 0; (i < secrets.size()) && !IsShutdownInProgress(); i++)
         {
            pAgentConn->setSharedSecret(secrets.get(i));
            if (pAgentConn->connect(g_serverKey, &rcc))
            {
               _tcslcpy(m_agentSecret, secrets.get(i), MAX_SECRET_LENGTH);
               nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): checking for NetXMS agent - shared secret changed to system default"), m_name);
               break;
            }

            if (((rcc != ERR_AUTH_REQUIRED) || (rcc != ERR_AUTH_FAILED)))
            {
               break;
            }
         }
      }
   }

   if (rcc == ERR_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): checking for NetXMS agent - connected"), m_name);
      lockProperties();
      m_capabilities |= NC_IS_NATIVE_AGENT;
      if (m_state & NSF_AGENT_UNREACHABLE)
      {
         m_state &= ~NSF_AGENT_UNREACHABLE;
         PostSystemEvent(EVENT_AGENT_OK, m_id);
         sendPollerMsg(POLLER_INFO _T("   Connectivity with NetXMS agent restored\r\n"));
      }
      else
      {
         sendPollerMsg(POLLER_INFO _T("   NetXMS agent is active\r\n"));
      }
      unlockProperties();

      TCHAR buffer[MAX_RESULT_LENGTH];
      if (pAgentConn->getParameter(_T("Agent.Version"), buffer, MAX_AGENT_VERSION_LEN) == ERR_SUCCESS)
      {
         lockProperties();
         if (_tcscmp(m_agentVersion, buffer))
         {
            _tcscpy(m_agentVersion, buffer);
            hasChanges = true;
            sendPollerMsg(_T("   NetXMS agent version changed to %s\r\n"), m_agentVersion);
         }
         unlockProperties();
      }

      if (pAgentConn->getParameter(_T("Agent.ID"), buffer, MAX_RESULT_LENGTH) == ERR_SUCCESS)
      {
         uuid agentId = uuid::parse(buffer);
         lockProperties();
         if (!m_agentId.equals(agentId))
         {
            EventBuilder(EVENT_AGENT_ID_CHANGED, m_id)
               .param(_T("oldAgentId"), m_agentId)
               .param(_T("newAgentId"), agentId)
               .post();

            m_agentId = agentId;
            hasChanges = true;
            sendPollerMsg(_T("   NetXMS agent ID changed to %s\r\n"), m_agentId.toString(buffer));
         }
         unlockProperties();
      }

      if (pAgentConn->getParameter(_T("System.PlatformName"), buffer, MAX_PLATFORM_NAME_LEN) == ERR_SUCCESS)
      {
         lockProperties();
         if (_tcscmp(m_platformName, buffer))
         {
            _tcscpy(m_platformName, buffer);
            hasChanges = true;
            sendPollerMsg(_T("   Platform name changed to %s\r\n"), m_platformName);
         }
         unlockProperties();
      }

      if (pAgentConn->getParameter(_T("System.HardwareId"), buffer, MAX_RESULT_LENGTH) == ERR_SUCCESS)
      {
         BYTE hardwareId[HARDWARE_ID_LENGTH];
         StrToBin(buffer, hardwareId, sizeof(hardwareId));
         lockProperties();
         if (!m_hardwareId.equals(hardwareId))
         {
            m_hardwareId = NodeHardwareId(hardwareId);
            hasChanges = true;
            sendPollerMsg(_T("   Hardware ID changed to %s\r\n"), buffer);
         }
         unlockProperties();
      }

      // Check IP forwarding status
      if (pAgentConn->getParameter(_T("Net.IP.Forwarding"), buffer, 16) == ERR_SUCCESS)
      {
         if (_tcstoul(buffer, nullptr, 10) != 0)
            m_capabilities |= NC_IS_ROUTER;
         else
            m_capabilities &= ~NC_IS_ROUTER;
      }

      // Check for 64 bit counter support.
      // if Net.Interface.64BitCounters not supported by agent then use
      // only presence of 64 bit parameters as indicator
      bool netIf64bitCounters = true;
      if (pAgentConn->getParameter(_T("Net.Interface.64BitCounters"), buffer, MAX_DB_STRING) == ERR_SUCCESS)
      {
         netIf64bitCounters = _tcstol(buffer, nullptr, 10) ? true : false;
      }

      ObjectArray<AgentParameterDefinition> *plist;
      ObjectArray<AgentTableDefinition> *tlist;
      uint32_t rcc = pAgentConn->getSupportedParameters(&plist, &tlist);
      if (rcc == ERR_SUCCESS)
      {
         lockProperties();
         delete m_agentParameters;
         delete m_agentTables;
         m_agentParameters = plist;
         m_agentTables = tlist;

         // Check for 64-bit interface counters
         m_capabilities &= ~NC_HAS_AGENT_IFXCOUNTERS;
         if (netIf64bitCounters)
         {
            for(int i = 0; i < plist->size(); i++)
            {
               if (!_tcsicmp(plist->get(i)->getName(), _T("Net.Interface.BytesIn64(*)")))
               {
                  m_capabilities |= NC_HAS_AGENT_IFXCOUNTERS;
                  break;
               }
            }
         }

         unlockProperties();
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): AgentConnection::getSupportedParameters() failed: rcc=%u"), m_name, rcc);
      }

      // Check for service manager support
      StringList *services;
      rcc = pAgentConn->getList(L"System.Services", &services);
      if (rcc == ERR_SUCCESS)
      {
         delete services;

         lockProperties();
         if (!(m_capabilities & NC_HAS_SERVICE_MANAGER))
         {
            m_capabilities |= NC_HAS_SERVICE_MANAGER;
            hasChanges = true;
         }
         unlockProperties();

         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): service manager interface is supported by agent"), m_name);
      }
      else if (rcc == ERR_UNKNOWN_METRIC)
      {
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): service manager interface is not supported by agent"), m_name);

         lockProperties();
         if (m_capabilities & NC_HAS_SERVICE_MANAGER)
         {
            m_capabilities &= ~NC_HAS_SERVICE_MANAGER;
            hasChanges = true;
         }
         unlockProperties();
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): AgentConnection::getList(\"System.Services\") failed: rcc=%u"), m_name, rcc);
      }

      // Get supported Windows Performance Counters
      if (!_tcsncmp(m_platformName, _T("windows-"), 8) && (!(m_flags & NF_DISABLE_PERF_COUNT)) &&
          !ConfigReadBoolean(_T("Objects.Nodes.ReadWinPerfCountersOnDemand"), true))
      {
         sendPollerMsg(_T("   Reading list of available Windows Performance Counters...\r\n"));
         ObjectArray<WinPerfObject> *perfObjects = WinPerfObject::getWinPerfObjectsFromNode(this, pAgentConn.get());
         lockProperties();
         delete m_winPerfObjects;
         m_winPerfObjects = perfObjects;
         if (m_winPerfObjects != nullptr)
         {
            sendPollerMsg(POLLER_INFO _T("   %d counters read\r\n"), m_winPerfObjects->size());
            if (!(m_capabilities & NC_HAS_WINPDH))
            {
               m_capabilities |= NC_HAS_WINPDH;
               hasChanges = true;
            }
         }
         else
         {
            sendPollerMsg(POLLER_ERROR _T("   Unable to get Windows Performance Counters list\r\n"));
            if (m_capabilities & NC_HAS_WINPDH)
            {
               m_capabilities &= ~NC_HAS_WINPDH;
               hasChanges = true;
            }
         }
         m_winPerfObjectsTimestamp = time(nullptr);
         unlockProperties();
      }

      // Check if file manager is supported
      bool fileManagerPresent = false;
      NXCPMessage request(CMD_GET_FOLDER_CONTENT, pAgentConn->generateRequestId(), pAgentConn->getProtocolVersion());
      request.setField(VID_ROOT, true);
      request.setField(VID_FILE_NAME, _T("/"));
      NXCPMessage *response = pAgentConn->customRequest(&request);
      if (response != nullptr)
      {
         if (response->getFieldAsUInt32(VID_RCC) == ERR_SUCCESS)
            fileManagerPresent = true;
         delete response;
      }
      lockProperties();
      if (fileManagerPresent)
      {
         m_capabilities |= NC_HAS_FILE_MANAGER;
         sendPollerMsg(_T("   File manager is available\r\n"));
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): file manager is available"), m_name);
      }
      else
      {
         m_capabilities &= ~NC_HAS_FILE_MANAGER;
         sendPollerMsg(_T("   File manager is not available\r\n"));
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): file manager is not available"), m_name);
      }
      unlockProperties();

      // Server ID should be set on this connection before updating configuration elements on agent side
      if (pAgentConn->setServerId(g_serverId) == ERR_SUCCESS)
      {
         checkAgentPolicyBinding(pAgentConn);

         // Update user support application notification list
         if (m_capabilities & NC_HAS_USER_AGENT)
         {
            NXCPMessage msg(pAgentConn->getProtocolVersion());
            msg.setCode(CMD_UPDATE_UA_NOTIFICATIONS);
            FillUserAgentNotificationsAll(&msg, this);
            NXCPMessage *response = pAgentConn->customRequest(&msg);
            if (response != nullptr)
            {
               rcc = response->getFieldAsUInt32(VID_RCC);
               delete response;
            }
            else
            {
               rcc = ERR_REQUEST_TIMEOUT;
            }
            if (rcc == ERR_SUCCESS)
               nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): user support application notifications synchronized"), m_name);
            else
               nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): cannot synchronize user support application notifications (%s)"), m_name, AgentErrorCodeToText(rcc));
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): cannot set server ID (%s)"), m_name, AgentErrorCodeToText(rcc));
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): checking for NetXMS agent - failed to connect (%s)"), m_name, AgentErrorCodeToText(rcc));
      sendPollerMsg(_T("   Cannot connect to NetXMS agent (%s)\r\n"), AgentErrorCodeToText(rcc));
   }
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): checking for NetXMS agent - finished"), m_name);
   return hasChanges;
}

/**
 * Configuration poll: check for Ethernet/IP
 */
bool Node::confPollEthernetIP()
{
   if (((m_capabilities & NC_IS_ETHERNET_IP) && (m_state & NSF_ETHERNET_IP_UNREACHABLE)) ||
       !m_ipAddress.isValidUnicast() || (m_flags & NF_DISABLE_ETHERNET_IP))
   {
      sendPollerMsg(_T("   EtherNet/IP polling is %s\r\n"), (m_flags & NF_DISABLE_ETHERNET_IP) ? _T("disabled") : _T("not possible"));
      return false;
   }

   bool hasChanges = false;

   sendPollerMsg(_T("   Checking EtherNet/IP...\r\n"));
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): checking EtherNet/IP"), m_name);

   CIP_Identity *identity = nullptr;
   EIP_Status status;

   uint32_t eipProxy = getEffectiveEtherNetIPProxy();
   if (eipProxy != 0)
   {
      // TODO: implement proxy request
      status = EIP_Status::callFailure(EIP_CALL_COMM_ERROR);
   }
   else
   {
      identity = EIP_ListIdentity(m_ipAddress, m_eipPort, 5000, &status);
   }

   if (identity != nullptr)
   {
      const TCHAR *vendorName = CIP_VendorNameFromCode(identity->vendor);

      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): EtherNet/IP device - %s by %s"), m_name,
               CIP_DeviceTypeNameFromCode(identity->deviceType), vendorName);
      sendPollerMsg(_T("   EtherNet/IP connection established\r\n"));
      sendPollerMsg(_T("      Device type: %s\r\n"), CIP_DeviceTypeNameFromCode(identity->deviceType));
      sendPollerMsg(_T("      Vendor: %s\r\n"), vendorName);
      sendPollerMsg(_T("      Product: %s\r\n"), identity->productName);

      lockProperties();
      m_capabilities |= NC_IS_ETHERNET_IP;
      if (m_state & NSF_ETHERNET_IP_UNREACHABLE)
      {
         m_state &= ~NSF_ETHERNET_IP_UNREACHABLE;
         PostSystemEvent(EVENT_ETHERNET_IP_OK, m_id);
         sendPollerMsg(POLLER_INFO _T("   EtherNet/IP connectivity restored\r\n"));
      }

      if (m_cipVendorCode != identity->vendor)
      {
         m_cipVendorCode = identity->vendor;
         hasChanges = true;
      }

      if (_tcscmp(m_vendor, vendorName))
      {
         m_vendor = vendorName;
         hasChanges = true;
      }

      if (_tcscmp(m_productName, identity->productName))
      {
         m_productName = identity->productName;
         hasChanges = true;
      }

      TCHAR buffer[64];
      _sntprintf(buffer, 64, _T("%u.%03u"), identity->productRevisionMajor, identity->productRevisionMinor);
      if (_tcscmp(m_productVersion, buffer))
      {
         m_productVersion = buffer;
         hasChanges = true;
      }

      _sntprintf(buffer, 64, _T("%04X"), identity->productCode);
      if (_tcscmp(m_productCode, buffer))
      {
         m_productCode = buffer;
         hasChanges = true;
      }

      _sntprintf(buffer, 64, _T("%08X"), identity->serialNumber);
      if (_tcscmp(m_serialNumber, buffer))
      {
         m_serialNumber = buffer;
         hasChanges = true;
      }

      if (m_cipDeviceType != identity->deviceType)
      {
         m_cipDeviceType = identity->deviceType;
         hasChanges = true;
      }

      if (m_cipStatus != identity->status)
      {
         m_cipStatus = identity->status;
         hasChanges = true;
      }

      if (m_cipState != identity->state)
      {
         m_cipState = identity->state;
         hasChanges = true;
      }

      unlockProperties();
      MemFree(identity);

      ExitFromUnreachableState();
   }
   else
   {
      String reason = status.failureReason();
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): checking for EtherNet/IP - failed to get device identity (%s)"), m_name, reason.cstr());
      sendPollerMsg(_T("   Cannot establish EtherNet/IP connection or get device identity (%s)\r\n"), reason.cstr());
   }

   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): check for EtherNet/IP completed"), m_name);
   return hasChanges;
}

/**
 * Configuration poll: check for MODBUS
 */
bool Node::confPollModbus()
{
   if (((m_capabilities & NC_IS_MODBUS_TCP) && (m_state & NSF_MODBUS_UNREACHABLE)) ||
       !m_ipAddress.isValidUnicast() || (m_flags & NF_DISABLE_MODBUS_TCP))
   {
      sendPollerMsg(_T("   Modbus TCP polling is %s\r\n"), (m_flags & NF_DISABLE_MODBUS_TCP) ? _T("disabled") : _T("not possible"));
      return false;
   }

   bool hasChanges = false;

   sendPollerMsg(_T("   Checking Modbus TCP...\r\n"));
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): checking Modbus TCP"), m_name);

   ModbusTransport *transport = createModbusTransport();
   if (transport != nullptr)
   {
      ModbusOperationStatus status = transport->checkConnection();
      if (status == MODBUS_STATUS_SUCCESS)
      {
         sendPollerMsg(_T("   Device is Modbus capable\r\n"));

         lockProperties();
         m_capabilities |= NC_IS_MODBUS_TCP;
         if (m_state & NSF_MODBUS_UNREACHABLE)
         {
            m_state &= ~NSF_MODBUS_UNREACHABLE;
            PostSystemEvent(EVENT_MODBUS_OK, m_id);
            sendPollerMsg(POLLER_INFO _T("   Modbus connectivity restored\r\n"));
         }
         unlockProperties();

         ExitFromUnreachableState();

         ModbusDeviceIdentification deviceIdentification;
         status = transport->readDeviceIdentification(&deviceIdentification);
         if (status == MODBUS_STATUS_SUCCESS)
         {
            nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): Modbus device identification successfully completed (vendor=\"%s\", product-code=\"%s\", product-name=\"%s\""),
               m_name, deviceIdentification.vendorName, deviceIdentification.productCode, deviceIdentification.productName);
            sendPollerMsg(_T("   Modbus device identification successfully completed\r\n"));
            sendPollerMsg(_T("      Vendor: %s\r\n"), deviceIdentification.vendorName);
            sendPollerMsg(_T("      Product code: %s\r\n"), deviceIdentification.productCode);
            sendPollerMsg(_T("      Product name: %s\r\n"), deviceIdentification.productName);
            sendPollerMsg(_T("      Revision: %s\r\n"), deviceIdentification.revision);

            lockProperties();

            if (_tcscmp(m_vendor, deviceIdentification.vendorName))
            {
               m_vendor = deviceIdentification.vendorName;
               hasChanges = true;
            }

            if (_tcscmp(m_productName, deviceIdentification.productName))
            {
               m_productName = deviceIdentification.productName;
               hasChanges = true;
            }

            if (_tcscmp(m_productCode, deviceIdentification.productCode))
            {
               m_productCode = deviceIdentification.productCode;
               hasChanges = true;
            }

            if (_tcscmp(m_productVersion, deviceIdentification.revision))
            {
               m_productVersion = deviceIdentification.revision;
               hasChanges = true;
            }

            unlockProperties();
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): cannot get device identification data via Modbus (%s)"), m_name, GetModbusStatusText(status));
            sendPollerMsg(_T("   Cannot get device identification data via Modbus (%s)\r\n"), GetModbusStatusText(status));
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): device does not respond to Modbus TCP request (%s)"), m_name, GetModbusStatusText(status));
         sendPollerMsg(_T("   Device does not respond to Modbus TCP request (%s)\r\n"), GetModbusStatusText(status));
      }
      delete transport;
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): cannot create transport for Modbus TCP"), m_name);
      sendPollerMsg(_T("   Modbus TCP connection is not possible\r\n"));
   }

   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): check for Modbus TCP completed"), m_name);
   return hasChanges;
}

/**
 * SNMP walker callback which sets indicator to true after first varbind and aborts walk
 */
static uint32_t IndicatorSnmpWalkerCallback(SNMP_Variable *var, SNMP_Transport *transport, bool *flag)
{
   *flag = true;
   return SNMP_ERR_COMM;
}

/**
 * Configuration poll: check for SNMP
 */
bool Node::confPollSnmp()
{
   if (((m_capabilities & NC_IS_SNMP) && (m_state & NSF_SNMP_UNREACHABLE) && !ConfigReadBoolean(_T("Objects.Nodes.ConfigurationPoll.AlwaysCheckSNMP"), true)) ||
       !m_ipAddress.isValidUnicast() || (m_flags & NF_DISABLE_SNMP))
   {
      sendPollerMsg(_T("   SNMP polling is %s\r\n"), (m_flags & NF_DISABLE_SNMP) ? _T("disabled") : _T("not possible"));
      return false;
   }

   bool hasChanges = false;

   sendPollerMsg(_T("   Checking SNMP...\r\n"));
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): calling SnmpCheckCommSettings()"), m_name);
   StringList oids;
   SharedString customOid = getCustomAttribute(_T("snmp.testoid"));
   if (!customOid.isEmpty())
      oids.add(customOid);
   oids.add(_T(".1.3.6.1.2.1.1.2.0"));
   oids.add(_T(".1.3.6.1.2.1.1.1.0"));
   AddDriverSpecificOids(&oids);

   // Check current SNMP settings first
   SNMP_Transport *pTransport = createSnmpTransport();
   if ((pTransport != nullptr) && !SnmpTestRequest(pTransport, oids, ConfigReadBoolean(_T("SNMP.Discovery.SeparateProbeRequests"), false)))
   {
      delete_and_null(pTransport);
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): node is not responding to SNMP using current settings"), m_name);
      if (m_flags & NF_SNMP_SETTINGS_LOCKED)
      {
         sendPollerMsg(_T("   Node is not responding to SNMP using current settings and SNMP settings are locked\r\n"));
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): SNMP settings are locked, skip SnmpCheckCommSettings()"), m_name);
         return false;
      }
   }
   if (pTransport == nullptr)
      pTransport = SnmpCheckCommSettings(getEffectiveSnmpProxy(), (getEffectiveSnmpProxy() == m_id) ? InetAddress::LOOPBACK : m_ipAddress,
               &m_snmpVersion, m_snmpPort, m_snmpSecurity, oids, m_zoneUIN, false);
   if (pTransport == nullptr)
   {
      sendPollerMsg(_T("   No response from SNMP agent\r\n"));
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): unable to create SNMP transport"), m_name);
      return false;
   }

   lockProperties();
   m_snmpPort = pTransport->getPort();
   delete m_snmpSecurity;
   m_snmpSecurity = new SNMP_SecurityContext(pTransport->getSecurityContext());
   if (m_snmpVersion == SNMP_VERSION_3)
   {
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): SNMPv3 authoritative engine ID: %s"), m_name, m_snmpSecurity->getAuthoritativeEngine().toString().cstr());
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): SNMPv3 context engine ID: %s"), m_name, m_snmpSecurity->getContextEngine().toString().cstr());
   }
   m_capabilities |= NC_IS_SNMP;
   if (m_state & NSF_SNMP_UNREACHABLE)
   {
      m_state &= ~NSF_SNMP_UNREACHABLE;
      PostSystemEvent(EVENT_SNMP_OK, m_id);
      sendPollerMsg(POLLER_INFO _T("   Connectivity with SNMP agent restored\r\n"));
   }
   unlockProperties();
   sendPollerMsg(_T("   SNMP agent is active (version %s)\r\n"),
            (m_snmpVersion == SNMP_VERSION_3) ? _T("3") : ((m_snmpVersion == SNMP_VERSION_2C) ? _T("2c") : _T("1")));
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): SNMP agent detected (version %s)"), m_name,
            (m_snmpVersion == SNMP_VERSION_3) ? _T("3") : ((m_snmpVersion == SNMP_VERSION_2C) ? _T("2c") : _T("1")));

   ExitFromUnreachableState();

   SNMP_ObjectId snmpObjectId({ 0, 0 }); // Set snmp object ID to 0.0 if it cannot be read
   SnmpGet(m_snmpVersion, pTransport, { 1, 3, 6, 1, 2, 1, 1, 2, 0 }, &snmpObjectId, 0, SG_OBJECT_ID_RESULT);
   lockProperties();
   if (!m_snmpObjectId.equals(snmpObjectId))
   {
      m_snmpObjectId = snmpObjectId;
      hasChanges = true;
   }
   unlockProperties();

   // Select device driver
   NetworkDeviceDriver *driver = FindDriverForNode(this, pTransport);
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): selected device driver %s"), m_name, driver->getName());
   lockProperties();
   if (driver != m_driver)
   {
      m_driver = driver;
      sendPollerMsg(_T("   New network device driver selected: %s\r\n"), m_driver->getName());
   }
   else
   {
      sendPollerMsg(_T("   Use previously selected network device driver %s\r\n"), m_driver->getName());
   }
   unlockProperties();

   // Allow driver to gather additional info
   m_driver->analyzeDevice(pTransport, m_snmpObjectId, this, &m_driverData);
   if (m_driverData != nullptr)
   {
      m_driverData->attachToNode(m_id, m_guid, m_name);
   }

   NDD_MODULE_LAYOUT layout;
   m_driver->getModuleLayout(pTransport, this, m_driverData, 1, &layout); // TODO module set to 1
   if (layout.numberingScheme == NDD_PN_UNKNOWN)
   {
      // Try to find port numbering information in database
      LookupDevicePortLayout(m_snmpObjectId, &layout);
   }
   m_portRowCount = layout.rows;
   m_portNumberingScheme = layout.numberingScheme;

   if (m_driver->hasMetrics())
   {
      ObjectArray<AgentParameterDefinition> *metrics = m_driver->getAvailableMetrics(pTransport, this, m_driverData);
      if (metrics != nullptr)
      {
         sendPollerMsg(_T("   %d metrics provided by network device driver\r\n"), metrics->size());
         lockProperties();
         delete m_driverParameters;
         m_driverParameters = metrics;
         unlockProperties();
      }
   }

   sendPollerMsg(_T("   Collecting system information from SNMP agent...\r\n"));

   // Get sysName, sysContact, sysLocation
   if (querySnmpSysProperty(pTransport, _T(".1.3.6.1.2.1.1.5.0"), _T("name"), &m_sysName))
      hasChanges = true;
   if (querySnmpSysProperty(pTransport, _T(".1.3.6.1.2.1.1.4.0"), _T("contact"), &m_sysContact))
      hasChanges = true;
   if (querySnmpSysProperty(pTransport, _T(".1.3.6.1.2.1.1.6.0"), _T("location"), &m_sysLocation))
      hasChanges = true;

   // Check IP forwarding
   if (CheckSNMPIntegerValue(pTransport, { 1, 3, 6, 1, 2, 1, 4, 1, 0 }, 1))
   {
      lockProperties();
      m_capabilities |= NC_IS_ROUTER;
      unlockProperties();
   }
   else
   {
      lockProperties();
      m_capabilities &= ~NC_IS_ROUTER;
      unlockProperties();
   }

   checkIfXTable(pTransport);
   checkBridgeMib(pTransport);

   // Check for ENTITY-MIB support
   // Some Cisco devices do not support entLastChangeTime but do support necessary tables
   // Such devices can be checked with GET NEXT on entPhysicalClass
   BYTE buffer[256];
   if ((SnmpGet(m_snmpVersion, pTransport, { 1, 3, 6, 1, 2, 1, 47, 1, 4, 1, 0 }, buffer, sizeof(buffer), SG_RAW_RESULT) == SNMP_ERR_SUCCESS) ||
       (SnmpGet(m_snmpVersion, pTransport, { 1, 3, 6, 1, 2, 1, 47, 1, 1, 1, 1, 5 }, buffer, sizeof(buffer), SG_GET_NEXT_REQUEST | SG_RAW_RESULT) == SNMP_ERR_SUCCESS))
   {
      lockProperties();
      m_capabilities |= NC_HAS_ENTITY_MIB;
      m_capabilities &= ~NC_EMULATED_ENTITY_MIB;
      unlockProperties();
   }
   else
   {
      bool emulationSupported = m_driver->isEntityMibEmulationSupported(pTransport, this, m_driverData);
      lockProperties();
      m_capabilities &= ~NC_HAS_ENTITY_MIB;
      if (emulationSupported)
         m_capabilities |= NC_EMULATED_ENTITY_MIB;
      else
         m_capabilities &= ~NC_EMULATED_ENTITY_MIB;
      unlockProperties();
   }

   shared_ptr<ComponentTree> components;
   if (m_capabilities & NC_HAS_ENTITY_MIB)
   {
      TCHAR debugInfo[256];
      _sntprintf(debugInfo, 256, _T("%s [%u]"), m_name, m_id);
      components = BuildComponentTree(pTransport, debugInfo);
   }
   else if (m_capabilities & NC_EMULATED_ENTITY_MIB)
   {
      components = m_driver->buildComponentTree(pTransport, this, m_driverData);
   }
   shared_ptr<DeviceView> deviceView = m_driver->buildDeviceView(pTransport, this, m_driverData, (components != nullptr) ? components->getRoot() : nullptr);

   lockProperties();

   if (m_components != nullptr)
   {
      if (components == nullptr)
      {
         time_t expirationTime = static_cast<time_t>(ConfigReadULong(_T("Objects.Nodes.CapabilityExpirationTime"), 604800));
         if (m_components->getTimestamp() + expirationTime < time(nullptr))
         {
            m_components = components;
            setModified(MODIFY_COMPONENTS, false);
         }
      }
      else if (!components->equals(m_components.get()))
      {
         m_components = components;
         setModified(MODIFY_COMPONENTS, false);
      }
   }
   else if (components != nullptr)
   {
      m_components = components;
      setModified(MODIFY_COMPONENTS, false);
   }

   if (m_deviceView != nullptr)
   {
      if (deviceView == nullptr)
      {
         time_t expirationTime = static_cast<time_t>(ConfigReadULong(_T("Objects.Nodes.CapabilityExpirationTime"), 604800));
         if (m_deviceView->getTimestamp() + expirationTime < time(nullptr))
         {
            m_deviceView = deviceView;
            m_capabilities &= ~NC_DEVICE_VIEW;
         }
      }
      else
      {
         m_deviceView = deviceView;
      }
   }
   else if (deviceView != nullptr)
   {
      m_deviceView = deviceView;
      m_capabilities |= NC_DEVICE_VIEW;
   }

   unlockProperties();

   // Check for printer MIB support
   bool present = false;
   SnmpWalk(pTransport, _T(".1.3.6.1.2.1.43"), IndicatorSnmpWalkerCallback, &present);
   if (present)
   {
      lockProperties();
      m_capabilities |= NC_IS_PRINTER;
      unlockProperties();
   }
   else
   {
      lockProperties();
      m_capabilities &= ~NC_IS_PRINTER;
      unlockProperties();
   }

   // Check for CDP (Cisco Discovery Protocol) support
   if (CheckSNMPIntegerValue(pTransport, _T(".1.3.6.1.4.1.9.9.23.1.3.1.0"), 1))
   {
      lockProperties();
      m_capabilities |= NC_IS_CDP;
      unlockProperties();
   }
   else
   {
      lockProperties();
      m_capabilities &= ~NC_IS_CDP;
      unlockProperties();
   }

   // Check for NDP (Nortel Discovery Protocol) support
   if (CheckSNMPIntegerValue(pTransport, _T(".1.3.6.1.4.1.45.1.6.13.1.2.0"), 1))
   {
      lockProperties();
      m_capabilities |= NC_IS_NDP;
      unlockProperties();
   }
   else
   {
      lockProperties();
      m_capabilities &= ~NC_IS_NDP;
      unlockProperties();
   }

   // Check for LLDP (Link Layer Discovery Protocol) support
   // MikroTik devices are known to not return lldpLocChassisId and lldpLocChassisIdSubtype so we
   // use lldpLocSysCapEnabled as fallback check
   bool lldpMIB = (SnmpGet(m_snmpVersion, pTransport, { 1, 0, 8802, 1, 1, 2, 1, 3, 2, 0 }, buffer, sizeof(buffer), 0) == SNMP_ERR_SUCCESS) ||
                  (SnmpGet(m_snmpVersion, pTransport, { 1, 0, 8802, 1, 1, 2, 1, 3, 6, 0 }, buffer, sizeof(buffer), 0) == SNMP_ERR_SUCCESS);
   bool lldpV2MIB = (SnmpGet(m_snmpVersion, pTransport, { 1, 3, 111, 2, 802, 1, 1, 13, 1, 3, 2, 0 }, buffer, sizeof(buffer), 0) == SNMP_ERR_SUCCESS);
   if (lldpMIB || lldpV2MIB)
   {
      lockProperties();
      m_capabilities |= NC_IS_LLDP;
      if (lldpV2MIB)
         m_capabilities |= NC_LLDP_V2_MIB;
      else
         m_capabilities &= ~NC_LLDP_V2_MIB;
      unlockProperties();

      uint32_t type;
      BYTE data[120];
      uint32_t dataLen;
      if ((SnmpGetEx(pTransport, LLDP_OID(_T("1.3.1.0"), lldpV2MIB), nullptr, 0, &type, sizeof(uint32_t), 0, nullptr) == SNMP_ERR_SUCCESS) &&
          (SnmpGetEx(pTransport, LLDP_OID(_T("1.3.2.0"), lldpV2MIB), nullptr, 0, data, 120, SG_RAW_RESULT, &dataLen) == SNMP_ERR_SUCCESS))
      {
         String lldpId = BuildLldpId(type, data, dataLen);
         lockProperties();
         if ((m_lldpNodeId == nullptr) || _tcscmp(m_lldpNodeId, lldpId))
         {
            MemFree(m_lldpNodeId);
            m_lldpNodeId = MemCopyString(lldpId);
            hasChanges = true;
            sendPollerMsg(_T("   LLDP node ID changed to %s\r\n"), m_lldpNodeId);
         }
         unlockProperties();
      }

      ObjectArray<LLDP_LOCAL_PORT_INFO> *lldpPorts = GetLLDPLocalPortInfo(*this, pTransport);
      lockProperties();
      delete m_lldpLocalPortInfo;
      m_lldpLocalPortInfo = lldpPorts;
      unlockProperties();
   }
   else
   {
      lockProperties();
      m_capabilities &= ~(NC_IS_LLDP | NC_LLDP_V2_MIB);
      unlockProperties();
   }

   // Check for 802.1x support
   if (CheckSNMPIntegerValue(pTransport, { 1, 0, 8802, 1, 1, 1, 1, 1, 1, 0 }, 1))
   {
      lockProperties();
      m_capabilities |= NC_IS_8021X;
      unlockProperties();
   }
   else
   {
      lockProperties();
      m_capabilities &= ~NC_IS_8021X;
      unlockProperties();
   }

   checkOSPFSupport(pTransport);

   // Get VRRP information
   VrrpInfo *vrrpInfo = GetVRRPInfo(this);
   if (vrrpInfo != nullptr)
   {
      lockProperties();
      m_capabilities |= NC_IS_VRRP;
      delete m_vrrpInfo;
      m_vrrpInfo = vrrpInfo;
      unlockProperties();
   }
   else
   {
      lockProperties();
      m_capabilities &= ~NC_IS_VRRP;
      unlockProperties();
   }

   // Get wireless controller data
   if (m_driver->isWirelessController(pTransport, this, m_driverData))
   {
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): node is a wireless controller"), m_name);
      sendPollerMsg(_T("   This device is a wireless controller\r\n"));
      lockProperties();
      m_capabilities |= NC_IS_WIFI_CONTROLLER;
      unlockProperties();
   }
   else
   {
      lockProperties();
      m_capabilities &= ~NC_IS_WIFI_CONTROLLER;
      unlockProperties();
   }

   // Get wireless access point data
   if (m_driver->isWirelessAccessPoint(pTransport, this, m_driverData))
   {
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): node is a wireless access point"), m_name);
      sendPollerMsg(_T("   This device is a wireless access point\r\n"));
      sendPollerMsg(_T("   Reading radio interface information\r\n"));
      lockProperties();
      m_capabilities |= NC_IS_WIFI_AP;
      unlockProperties();

      StructArray<RadioInterfaceInfo> *radioInterfaces = m_driver->getRadioInterfaces(pTransport, this, m_driverData);
      if (radioInterfaces != nullptr)
      {
         sendPollerMsg(POLLER_INFO _T("   %d radio interfaces found\r\n"), radioInterfaces->size());
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): got information about %d radio interfaces"), m_name, radioInterfaces->size());
         lockProperties();
         if (!CompareRadioInterfaceLists(radioInterfaces, m_radioInterfaces))
         {
            delete m_radioInterfaces;
            m_radioInterfaces = radioInterfaces;
            setModified(MODIFY_RADIO_INTERFACES, false);
         }
         else
         {
            delete radioInterfaces;
         }
         unlockProperties();
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): failed to read radio interface information"), m_name);
         sendPollerMsg(POLLER_ERROR _T("   Failed to read radio interface information\r\n"));
      }
   }
   else
   {
      lockProperties();
      m_capabilities &= ~NC_IS_WIFI_AP;
      unlockProperties();
   }

   delete pTransport;
   return hasChanges;
}

/**
 * Check if this node is reachable via SSH
 */
bool Node::checkSshConnection()
{
   SharedString sshLogin = getSshLogin(); // Use getter instead of direct access because we need proper lock
   if (sshLogin.isNull() || sshLogin.isEmpty())
      return false;
   return SSHCheckConnection(getEffectiveSshProxy(), m_ipAddress, m_sshPort, sshLogin, getSshPassword(), m_sshKeyId);
}

/**
 * Configuration poll: check for SSH
 */
bool Node::confPollSsh()
{
   if ((m_flags & NF_DISABLE_SSH) || !m_ipAddress.isValidUnicast())
      return false;

   sendPollerMsg(_T("   Checking SSH connectivity...\r\n"));

   bool modified = false;
   bool success = checkSshConnection();
   if (!success)
   {
      SSHCredentials credentials;
      uint16_t port;
      if (SSHCheckCommSettings(getEffectiveSshProxy(), m_ipAddress, m_zoneUIN, &credentials, &port))
      {
         lockProperties();
         m_sshPort = port;
         m_sshLogin = credentials.login;
         m_sshPassword = credentials.password;
         m_sshKeyId = credentials.keyId;
         unlockProperties();
         modified = true;
      }
   }

   // Process result
   if (success)
   {
      sendPollerMsg(POLLER_INFO _T("   SSH connection is available\r\n"));
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 7, _T("ConfPoll(%s): SSH connected"), m_name);
      lockProperties();
      if (!(m_capabilities & NC_IS_SSH))
      {
         m_capabilities |= NC_IS_SSH;
         modified = true;
      }
      unlockProperties();

      ExitFromUnreachableState();

      if (confPollSmclp())
         modified = true;
   }
   else
   {
      sendPollerMsg(_T("   Cannot connect to SSH\r\n"));
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("ConfPoll(%s): SSH unreachable"), m_name);
   }

   return modified;
}

/**
 * SM-CLP configuraiton poll
 */
bool Node::confPollSmclp()
{
   bool modified = false;

   StringBuffer output;
   bool success = getDataFromSmclp(L"version", &output);
   if (success)
   {
      success = output.contains(L"status=0");
   }

   if(success)
   {
      sendPollerMsg(POLLER_INFO _T("   SM-CLP protocol is supported\r\n"));
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 7, _T("ConfPoll(%s): SM-CLP protocol is supported"), m_name);
      if (!(m_capabilities & NC_IS_SMCLP))
      {
         m_capabilities |= NC_IS_SMCLP;
         modified = true;
      }
   }
   else
   {
      sendPollerMsg(_T("   SM-CLP protocol is not supported\r\n"));
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("ConfPoll(%s): SM-CLP protocol is not supported"), m_name);
   }

   if (success && !(m_flags & NF_DISABLE_SMCLP_PROPERTIES))
   {
      sendPollerMsg(_T("   Reading list of available SM-CLP targets and properties...\r\n"));
      StringList *metrics = getAvailableMetricFromSmclp();
      lockProperties();
      delete m_smclpMetrics;
      m_smclpMetrics = metrics;
      if (m_smclpMetrics != nullptr)
      {
         sendPollerMsg(POLLER_INFO _T("   %d SM-CLP metrics read\r\n"), m_smclpMetrics->size());
      }
      else
      {
         sendPollerMsg(POLLER_ERROR _T("   Unable to get SM-CLP metric list\r\n"));
      }
      unlockProperties();
   }

   return modified;
}

/**
 * Configuration poll: check for VNC
 */
bool Node::confPollVnc()
{
   if (m_flags & NF_DISABLE_VNC)
      return false;

   bool modified = false;

   // Check if VNC is reachable from proxy
   if (m_ipAddress.isValidUnicast())
   {
      sendPollerMsg(_T("   Checking direct VNC connectivity...\r\n"));

      bool success = VNCCheckConnection(getEffectiveVncProxy(), m_ipAddress, m_vncPort);
      if (!success)
      {
         uint16_t port;
         if (VNCCheckCommSettings(getEffectiveVncProxy(), m_ipAddress, m_zoneUIN, &port))
         {
            lockProperties();
            m_vncPort = port;
            unlockProperties();
            modified = true;
         }
      }

      if (success)
      {
         sendPollerMsg(POLLER_INFO _T("   Direct VNC connection is available\r\n"));
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 7, _T("ConfPoll(%s): VNC connected (direct connection)"), m_name);
         lockProperties();
         if (!(m_capabilities & NC_IS_VNC))
         {
            m_capabilities |= NC_IS_VNC;
            modified = true;
         }
         unlockProperties();

         ExitFromUnreachableState();
      }
      else
      {
         sendPollerMsg(_T("   Cannot connect to VNC\r\n"));
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("ConfPoll(%s): VNC unreachable directly"), m_name);
      }
   }

   // Check if VNC is reachable from local agent
   if (isNativeAgent())
   {
      sendPollerMsg(_T("   Checking VNC connectivity via local agent...\r\n"));

      bool success = VNCCheckConnection(this, InetAddress::LOOPBACK, m_vncPort);
      if (!success)
      {
         uint16_t port;
         if (VNCCheckCommSettings(m_id, InetAddress::LOOPBACK, m_zoneUIN, &port))
         {
            lockProperties();
            m_vncPort = port;
            unlockProperties();
            modified = true;
         }
      }

      if (success)
      {
         sendPollerMsg(POLLER_INFO _T("   VNC connection via local agent is available\r\n"));
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 7, _T("ConfPoll(%s): VNC connected (via local agent)"), m_name);
         lockProperties();
         if (!(m_capabilities & NC_IS_LOCAL_VNC))
         {
            m_capabilities |= NC_IS_LOCAL_VNC;
            modified = true;
         }
         unlockProperties();

         ExitFromUnreachableState();
      }
      else
      {
         sendPollerMsg(_T("   Cannot connect to VNC\r\n"));
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("ConfPoll(%s): VNC unreachable via local agent"), m_name);
      }
   }

   return modified;
}

/**
 * Query SNMP sys property (sysName, sysLocation, etc.)
 */
bool Node::querySnmpSysProperty(SNMP_Transport *snmp, const TCHAR *oid, const TCHAR *propName, TCHAR **value)
{
   TCHAR buffer[256];
   bool hasChanges = false;

   if (SnmpGet(m_snmpVersion, snmp, oid, nullptr, 0, buffer, sizeof(buffer), SG_STRING_RESULT) == SNMP_ERR_SUCCESS)
   {
      lockProperties();
      if ((*value == nullptr) || _tcscmp(*value, buffer))
      {
         MemFree(*value);
         *value = _tcsdup(buffer);
         hasChanges = true;
         sendPollerMsg(_T("   System %s changed to %s\r\n"), propName, *value);
      }
      unlockProperties();
   }
   return hasChanges;
}

/**
 * Configuration poll: check for BRIDGE MIB
 */
void Node::checkBridgeMib(SNMP_Transport *snmp)
{
   // Older Cisco Nexus software (7.x probably) does not return base bridge address but still
   // support some parts of bridge MIB. We do mark such devices as isBridge to allow more correct
   // L2 topology maps and device visualization.
   BYTE baseBridgeAddress[64];
   memset(baseBridgeAddress, 0, sizeof(baseBridgeAddress));
   uint32_t portCount = 0;
   if ((SnmpGet(m_snmpVersion, snmp, { 1, 3, 6, 1, 2, 1, 17, 1, 1, 0 }, baseBridgeAddress, sizeof(baseBridgeAddress), SG_RAW_RESULT) == SNMP_ERR_SUCCESS) ||
       (SnmpGet(m_snmpVersion, snmp, { 1, 3, 6, 1, 2, 1, 17, 1, 2, 0 }, &portCount, sizeof(uint32_t), 0) == SNMP_ERR_SUCCESS))
   {
      lockProperties();
      m_capabilities |= NC_IS_BRIDGE;
      memcpy(m_baseBridgeAddress, baseBridgeAddress, 6);
      unlockProperties();

      // Check for Spanning Tree (IEEE 802.1d) MIB support
      uint32_t stpVersion;
      if (SnmpGet(m_snmpVersion, snmp, { 1, 3, 6, 1, 2, 1, 17, 2, 1, 0 }, &stpVersion, sizeof(uint32_t), 0) == SNMP_ERR_SUCCESS)
      {
         // Cisco Nexus devices may return 1 (unknown) instead of 3 (ieee8021d)
         lockProperties();
         if ((stpVersion == 3) || (stpVersion == 1))
            m_capabilities |= NC_IS_STP;
         else
            m_capabilities &= ~NC_IS_STP;
         unlockProperties();
      }
      else
      {
         lockProperties();
         m_capabilities &= ~NC_IS_STP;
         unlockProperties();
      }
   }
   else
   {
      lockProperties();
      m_capabilities &= ~(NC_IS_BRIDGE | NC_IS_STP);
      unlockProperties();
   }
}

/**
 * Configuration poll: check for ifXTable
 */
void Node::checkIfXTable(SNMP_Transport *snmp)
{
   bool present = false;
   SnmpWalk(snmp, { 1, 3, 6, 1, 2, 1, 31, 1, 1, 1, 1 }, IndicatorSnmpWalkerCallback, &present);
   if (present)
   {
      lockProperties();
      m_capabilities |= NC_HAS_IFXTABLE;
      unlockProperties();
   }
   else
   {
      lockProperties();
      m_capabilities &= ~NC_HAS_IFXTABLE;
      unlockProperties();
   }
}

/**
 * Execute interface update hook
 */
void Node::executeInterfaceUpdateHook(Interface *iface)
{
   NXSL_VM *vm = CreateServerScriptVM(_T("Hook::UpdateInterface"), self());
   if (vm == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 7, _T("Node::executeInterfaceUpdateHook(%s [%u]): hook script \"Hook::UpdateInterface\" not found"), m_name, m_id);
      return;
   }

   vm->setUserData(this);
   vm->setGlobalVariable("$interface", iface->createNXSLObject(vm));
   NXSL_Value *argv = iface->createNXSLObject(vm);
   if (!vm->run(1, &argv))
   {
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 4, _T("Node::executeInterfaceUpdateHook(%s [%u]): hook script execution error: %s"), m_name, m_id, vm->getErrorText());
      ReportScriptError(SCRIPT_CONTEXT_OBJECT, this, 0, vm->getErrorText(), _T("Hook::UpdateInterface"));
   }
   delete vm;
}

/**
 * Delete duplicate interfaces
 * (find and delete multiple interfaces with same ifIndex value created by version prior to 2.0-M3)
 */
bool Node::deleteDuplicateInterfaces(uint32_t requestId)
{
   ObjectArray<Interface> deleteList;

   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *curr = getChildList().get(i);
      if ((curr->getObjectClass() != OBJECT_INTERFACE) || static_cast<Interface*>(curr)->isManuallyCreated())
         continue;

      Interface *iface = static_cast<Interface*>(curr);
      for(int j = i + 1; j < getChildList().size(); j++)
      {
         NetObj *next = getChildList().get(j);
         if ((next->getObjectClass() != OBJECT_INTERFACE) ||
             static_cast<Interface*>(next)->isManuallyCreated() ||
             deleteList.contains(static_cast<Interface*>(next)))
            continue;

         if (iface->getIfIndex() == static_cast<Interface*>(next)->getIfIndex())
         {
            deleteList.add(static_cast<Interface*>(next));
            nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("Node::deleteDuplicateInterfaces(%s [%d]): found duplicate interface %s [%d], original %s [%d], ifIndex=%d"),
               m_name, m_id, next->getName(), next->getId(), iface->getName(), iface->getId(), iface->getIfIndex());
         }
      }
   }
   unlockChildList();

   for(int i = 0; i < deleteList.size(); i++)
   {
      Interface *iface = deleteList.get(i);
      sendPollerMsg(POLLER_WARNING L"   Duplicate interface \"%s\" deleted\r\n", iface->getName());
      deleteInterface(iface);
   }

   return deleteList.size() > 0;
}

/**
 * Update interface configuration
 */
bool Node::updateInterfaceConfiguration(uint32_t requestId)
{
   sendPollerMsg(L"Checking interface configuration...\r\n");

   bool hasChanges = deleteDuplicateInterfaces(requestId);

   InterfaceList *ifList = getInterfaceList();
   if ((ifList != nullptr) && (ifList->size() > 0))
   {
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("Node::updateInterfaceConfiguration(%s [%u]): got %d interfaces"), m_name, m_id, ifList->size());

      // Find non-existing interfaces
      readLockChildList();
      ObjectArray<Interface> deleteList(getChildList().size());
      for(int i = 0; i < getChildList().size(); i++)
      {
         if (getChildList().get(i)->getObjectClass() == OBJECT_INTERFACE)
         {
            auto iface = static_cast<Interface*>(getChildList().get(i));
            if (iface->isFake())
            {
               // always delete fake interfaces if we got actual interface list
               deleteList.add(iface);
               continue;
            }

            if (!iface->isManuallyCreated())
            {
               int j;
               for(j = 0; j < ifList->size(); j++)
               {
                  if (ifList->get(j)->index == iface->getIfIndex())
                     break;
               }

               if (j == ifList->size())
               {
                  // No such interface in current configuration, add it to delete list
                  deleteList.add(iface);
               }
            }
         }
      }
      unlockChildList();

      // Delete non-existent interfaces
      if (deleteList.size() > 0)
      {
         for(int j = 0; j < deleteList.size(); j++)
         {
            Interface *iface = deleteList.get(j);
            sendPollerMsg(POLLER_WARNING _T("   Interface \"%s\" no longer exists\r\n"), iface->getName());
            const InetAddress& addr = iface->getFirstIpAddress();
            EventBuilder(EVENT_INTERFACE_DELETED, m_id)
               .param(_T("interfaceIndex"), iface->getIfIndex())
               .param(_T("interfaceName"), iface->getName())
               .param(_T("interfaceIpAddress"), addr)
               .param(_T("interfaceMask"), addr.getMaskBits())
               .post();
               
            deleteInterface(iface);
         }
         hasChanges = true;
      }

      int useAliases = ConfigReadInt(_T("Objects.Interfaces.UseAliases"), 0);

      // Add new interfaces and check configuration of existing
      for(int j = 0; j < ifList->size(); j++)
      {
         InterfaceInfo *ifInfo = ifList->get(j);
         shared_ptr<Interface> pInterface;
         bool isNewInterface = true;
         bool interfaceUpdated = false;

         TCHAR ifObjectName[MAX_OBJECT_NAME];
         BuildInterfaceObjectName(ifObjectName, *ifInfo, useAliases);

         readLockChildList();
         for(int i = 0; i < getChildList().size(); i++)
         {
            if (getChildList().get(i)->getObjectClass() == OBJECT_INTERFACE)
            {
               pInterface = static_pointer_cast<Interface>(getChildList().getShared(i));
               if (ifInfo->index == pInterface->getIfIndex())
               {
                  // Existing interface, check configuration
                  if (memcmp(ifInfo->macAddr, "\x00\x00\x00\x00\x00\x00", MAC_ADDR_LENGTH) &&
                      !pInterface->getMacAddress().equals(ifInfo->macAddr, MAC_ADDR_LENGTH))
                  {
                     wchar_t macAddrTextOld[64], macAddrTextNew[64];
                     pInterface->getMacAddress().toString(macAddrTextOld);
                     MACToStr(ifInfo->macAddr, macAddrTextNew);
                     EventBuilder(EVENT_MAC_ADDR_CHANGED, m_id)
                        .param(L"interfaceObjectId", pInterface->getId(), EventBuilder::OBJECT_ID_FORMAT)
                        .param(L"interfaceIndex", pInterface->getIfIndex())
                        .param(L"interfaceName", pInterface->getName())
                        .param(L"oldMacAddress", macAddrTextOld)
                        .param(L"newMacAddress", macAddrTextNew)
                        .post();

                     pInterface->setMacAddress(MacAddress(ifInfo->macAddr, MAC_ADDR_LENGTH), true);
                     interfaceUpdated = true;
                  }
                  wchar_t expandedName[MAX_OBJECT_NAME];
                  pInterface->expandName(ifObjectName, expandedName);
                  if (_tcscmp(expandedName, pInterface->getName()))
                  {
                     pInterface->setName(expandedName);
                     interfaceUpdated = true;
                  }
                  if (_tcscmp(ifInfo->name, pInterface->getIfName()))
                  {
                     pInterface->setIfName(ifInfo->name);
                     interfaceUpdated = true;
                  }
                  if (_tcscmp(ifInfo->description, pInterface->getDescription()))
                  {
                     pInterface->setDescription(ifInfo->description);
                     interfaceUpdated = true;
                  }
                  if (_tcscmp(ifInfo->alias, pInterface->getIfAlias()))
                  {
                     if (pInterface->getAlias().isEmpty() || !_tcscmp(pInterface->getIfAlias(), pInterface->getAlias()))
                        pInterface->setAlias(ifInfo->alias);
                     pInterface->setIfAlias(ifInfo->alias);
                     interfaceUpdated = true;
                  }
                  if (ifInfo->bridgePort != pInterface->getBridgePortNumber())
                  {
                     pInterface->setBridgePortNumber(ifInfo->bridgePort);
                     interfaceUpdated = true;
                  }
                  if (!ifInfo->location.equals(pInterface->getPhysicalLocation()))
                  {
                     pInterface->setPhysicalLocation(ifInfo->location);
                     interfaceUpdated = true;
                  }
                  if (ifInfo->isPhysicalPort != pInterface->isPhysicalPort())
                  {
                     pInterface->setPhysicalPortFlag(ifInfo->isPhysicalPort);
                     interfaceUpdated = true;
                  }
                  if (ifInfo->mtu != pInterface->getMTU())
                  {
                     pInterface->setMTU(ifInfo->mtu);
                     interfaceUpdated = true;
                  }
                  if (ifInfo->type != pInterface->getIfType())
                  {
                     pInterface->setIfType(ifInfo->type);
                     interfaceUpdated = true;
                  }
                  if ((ifInfo->ifTableSuffixLength != pInterface->getIfTableSuffixLen()) ||
                      memcmp(ifInfo->ifTableSuffix, pInterface->getIfTableSuffix(),
                         std::min(ifInfo->ifTableSuffixLength, pInterface->getIfTableSuffixLen())))
                  {
                     pInterface->setIfTableSuffix(ifInfo->ifTableSuffixLength, ifInfo->ifTableSuffix);
                     interfaceUpdated = true;
                  }
                  if (ifInfo->maxSpeed != pInterface->getMaxSpeed())
                  {
                     pInterface->setMaxSpeed(ifInfo->maxSpeed);
                     if ((ifInfo->maxSpeed > 0) && (pInterface->getSpeed() < ifInfo->maxSpeed))
                     {
                        pInterface->setSpeed(0); // reset current speed to force check against max speed at next status poll
                     }
                     interfaceUpdated = true;
                  }

                  // Check for deleted IPs and changed masks
                  const InetAddressList *ifList = pInterface->getIpAddressList();
                  for(int n = 0; n < ifList->size(); n++)
                  {
                     const InetAddress& ifAddr = ifList->get(n);
                     InetAddress addr = ifInfo->ipAddrList.findAddress(ifAddr);
                     if (addr.isValid())
                     {
                        if (addr.getMaskBits() == 0)
                        {
                           shared_ptr<Subnet> subnet = FindSubnetForNode(m_zoneUIN, addr);
                           if (subnet != nullptr)
                           {
                              TCHAR buffer[64];
                              nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("Node::getInterfaceList(node=%s [%u]): missing subnet mask for interface %s address %s set from subnet %s"),
                                 m_name, m_id, pInterface->getName(), addr.toString(buffer), subnet->getName());
                              addr.setMaskBits(subnet->getIpAddress().getMaskBits());
                           }
                        }

                        if (addr.getMaskBits() != ifAddr.getMaskBits())
                        {
                           EventBuilder(EVENT_IF_MASK_CHANGED, m_id)
                              .param(_T("interfaceObjectId"), pInterface->getId())
                              .param(_T("interfaceName"), pInterface->getName())
                              .param(_T("interfaceIpAddress"), addr)
                              .param(_T("interfaceNetmask"), addr.getMaskBits())
                              .param(_T("interfaceIndex"), pInterface->getIfIndex())
                              .param(_T("interfaceOldMask"), ifAddr.getMaskBits())
                              .post();

                           pInterface->setNetMask(addr);
                           sendPollerMsg(POLLER_INFO _T("   IP network mask changed to /%d on interface \"%s\" address %s\r\n"),
                              addr.getMaskBits(), pInterface->getName(), (const TCHAR *)ifAddr.toString());
                           interfaceUpdated = true;
                        }
                     }
                     else
                     {
                        sendPollerMsg(POLLER_WARNING _T("   IP address %s removed from interface \"%s\"\r\n"),
                           (const TCHAR *)ifAddr.toString(), pInterface->getName());

                        EventBuilder(EVENT_IF_IPADDR_DELETED, m_id)
                           .param(_T("interfaceObjectId"), pInterface->getId())
                           .param(_T("interfaceName"), pInterface->getName())
                           .param(_T("ipAddress"), ifAddr)
                           .param(_T("networkMask"), ifAddr.getMaskBits())
                           .param(_T("interfaceIndex"), pInterface->getIfIndex())
                           .post();
                        pInterface->deleteIpAddress(ifAddr);
                        interfaceUpdated = true;
                     }
                  }

                  // Check for added IPs
                  for(int m = 0; m < ifInfo->ipAddrList.size(); m++)
                  {
                     InetAddress addr = ifInfo->ipAddrList.get(m);
                     if (!ifList->hasAddress(addr))
                     {
                        if (addr.getMaskBits() == 0)
                        {
                           shared_ptr<Subnet> subnet = FindSubnetForNode(m_zoneUIN, addr);
                           if (subnet != nullptr)
                           {
                              TCHAR buffer[64];
                              nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("Node::getInterfaceList(node=%s [%u]): missing subnet mask for interface %s address %s set from subnet %s"),
                                 m_name, m_id, pInterface->getName(), addr.toString(buffer), subnet->getName());
                              addr.setMaskBits(subnet->getIpAddress().getMaskBits());
                           }
                        }

                        pInterface->addIpAddress(addr);
                        EventBuilder(EVENT_IF_IPADDR_ADDED, m_id)
                           .param(_T("interfaceObjectId"), pInterface->getId())
                           .param(_T("interfaceName"), pInterface->getName())
                           .param(_T("ipAddress"), addr)
                           .param(_T("networkMask"), addr.getMaskBits())
                           .param(_T("interfaceIndex"), pInterface->getIfIndex())
                           .post();

                        sendPollerMsg(POLLER_INFO _T("   IP address %s added to interface \"%s\"\r\n"),
                           (const TCHAR *)addr.toString(), pInterface->getName());
                        interfaceUpdated = true;
                     }
                  }

                  isNewInterface = false;
                  break;
               }
            }
         }
         unlockChildList();

         if (isNewInterface)
         {
            // New interface
            sendPollerMsg(POLLER_INFO _T("   Found new interface \"%s\"\r\n"), ifInfo->name);
            if (createNewInterface(ifInfo, false, false) != nullptr)
            {
               hasChanges = true;
            }
         }
         else if (interfaceUpdated)
         {
            executeInterfaceUpdateHook(pInterface.get());
         }
      }

      // Set parent interfaces
      for(int j = 0; j < ifList->size(); j++)
      {
         InterfaceInfo *ifInfo = ifList->get(j);
         if (ifInfo->parentIndex != 0)
         {
            shared_ptr<Interface> parent = findInterfaceByIndex(ifInfo->parentIndex);
            if (parent != nullptr)
            {
               shared_ptr<Interface> iface = findInterfaceByIndex(ifInfo->index);
               if (iface != nullptr)
               {
                  iface->setParentInterface(parent->getId());
                  nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("Node::updateInterfaceConfiguration(%s [%u]): set sub-interface: %s [%d] -> %s [%d]"),
                           m_name, m_id, parent->getName(), parent->getId(), iface->getName(), iface->getId());
               }
            }
         }
         else
         {
            shared_ptr<Interface> iface = findInterfaceByIndex(ifInfo->index);
            if ((iface != nullptr) && (iface->getParentInterfaceId() != 0))
            {
               iface->setParentInterface(0);
               nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("Node::updateInterfaceConfiguration(%s [%u]): removed reference to parent interface from %s [%d]"),
                        m_name, m_id, iface->getName(), iface->getId());
            }
         }
      }
   }
   else if (!(m_flags & NF_EXTERNAL_GATEWAY))    /* pIfList == nullptr or empty */
   {
      sendPollerMsg(POLLER_ERROR _T("Unable to get interface list from node\r\n"));
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("Node::updateInterfaceConfiguration(%s [%u]): Unable to get interface list from node"), m_name, m_id);

      // Delete all existing interfaces in case of forced capability recheck
      if (m_runtimeFlags & NDF_RECHECK_CAPABILITIES)
      {
         SharedObjectArray<Interface> deleteList;
         readLockChildList();
         for(int i = 0; i < getChildList().size(); i++)
         {
            NetObj *curr = getChildList().get(i);
            if ((curr->getObjectClass() == OBJECT_INTERFACE) && !static_cast<Interface*>(curr)->isManuallyCreated())
               deleteList.add(static_pointer_cast<Interface>(getChildList().getShared(i)));
         }
         unlockChildList();
         for(int j = 0; j < deleteList.size(); j++)
         {
            auto iface = deleteList.get(j);
            sendPollerMsg(POLLER_WARNING _T("   Interface \"%s\" no longer exists\r\n"), iface->getName());
            const InetAddress& addr = iface->getIpAddressList()->getFirstUnicastAddress();
            EventBuilder(EVENT_INTERFACE_DELETED, m_id)
               .param(_T("interfaceIndex"), iface->getIfIndex())
               .param(_T("interfaceName"), iface->getName())
               .param(_T("interfaceIpAddress"), addr)
               .param(_T("interfaceMask"), addr.getMaskBits())
               .post();
            deleteInterface(iface);
         }
      }

      // Check if we have pseudo-interface object
      MacAddress macAddr(MacAddress::ZERO);
      Interface *iface;
      uint32_t ifaceCount = getInterfaceCount(&iface);
      if ((ifList != nullptr) && ((ifaceCount > 1) || ((ifaceCount == 1) && !iface->isFake())))
      {
         // Node has interfaces from previous polls but do not report them anymore
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("Node::updateInterfaceConfiguration(%s [%u]): clearing interface list"), m_name, m_id);
         unique_ptr<SharedObjectArray<NetObj>> ifaces = getChildren(OBJECT_INTERFACE);
         for(int i = 0; i < ifaces->size(); i++)
            deleteInterface(static_cast<Interface*>(ifaces->get(i)));
         ifaceCount = 0;
         iface = nullptr;
      }
      if (ifaceCount == 1)
      {
         if (iface->isFake())
         {
            // Check if primary IP is different from interface's IP
            if (!iface->getIpAddressList()->hasAddress(m_ipAddress))
            {
               deleteInterface(iface);
               if (m_ipAddress.isValidUnicast())
               {
                  shared_ptr<Subnet> pSubnet = FindSubnetForNode(m_zoneUIN, m_ipAddress);
                  if (pSubnet != nullptr)
                     macAddr = pSubnet->findMacAddress(m_ipAddress);
                  TCHAR szMac[32];
                  macAddr.toString(szMac);
                  nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("Node::updateInterfaceConfiguration(%s [%u]): got MAC for unknown interface: %s"), m_name, m_id, szMac);
                  InetAddress ifaceAddr = m_ipAddress;
                  ifaceAddr.setMaskBits(0);  // autodetect
                  createNewInterface(ifaceAddr, macAddr, true);
               }
            }
            else
            {
               // check MAC address
               shared_ptr<Subnet> pSubnet = FindSubnetForNode(m_zoneUIN, m_ipAddress);
               if (pSubnet != nullptr)
               {
                  nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("Node::updateInterfaceConfiguration(%s [%u]): found subnet %s [%u]"),
                            m_name, m_id, pSubnet->getName(), pSubnet->getId());
                  macAddr = pSubnet->findMacAddress(m_ipAddress);
               }
               if (macAddr.isValid() && !macAddr.equals(iface->getMacAddress()))
               {
                  TCHAR oldMAC[32], newMAC[32];
                  iface->getMacAddress().toString(oldMAC);
                  macAddr.toString(newMAC);
                  nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("Node::updateInterfaceConfiguration(%s [%u]): MAC change for unknown interface: %s to %s"), m_name, m_id, oldMAC, newMAC);
                  EventBuilder(EVENT_MAC_ADDR_CHANGED, m_id)
                        .param(_T("interfaceObjectId"), iface->getId(), EventBuilder::OBJECT_ID_FORMAT)
                        .param(_T("interfaceIndex"), iface->getIfIndex())
                        .param(_T("interfaceObjectId"), iface->getName())
                        .param(_T("oldMacAddress"), oldMAC)
                        .param(_T("newMacAddress"), newMAC)
                        .post();
                  iface->setMacAddress(macAddr, true);
               }
            }
         }
      }
      else if (ifaceCount == 0)
      {
         // No interfaces at all, create pseudo-interface
         if (m_ipAddress.isValidUnicast())
         {
            shared_ptr<Subnet> pSubnet = FindSubnetForNode(m_zoneUIN, m_ipAddress);
            if (pSubnet != nullptr)
            {
               nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("Node::updateInterfaceConfiguration(%s [%u]): found subnet %s [%u]"),
                         m_name, m_id, pSubnet->getName(), pSubnet->getId());
               macAddr = pSubnet->findMacAddress(m_ipAddress);
            }
            TCHAR szMac[32];
            macAddr.toString(szMac);
            nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("Node::updateInterfaceConfiguration(%s [%u]): got MAC for unknown interface: %s"), m_name, m_id, szMac);
            InetAddress ifaceAddr = m_ipAddress;
            ifaceAddr.setMaskBits(0);  // autodetect
            createNewInterface(ifaceAddr, macAddr, true);
         }
      }
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("Node::updateInterfaceConfiguration(%s [%u]): pIfList == nullptr, dwCount = %u"), m_name, m_id, ifaceCount);
   }

   delete ifList;
   checkSubnetBinding();

   sendPollerMsg(_T("Interface configuration check finished\r\n"));
   return hasChanges;
}

/**
 * Get instances for instance discovery DCO
 */
StringMap *Node::getInstanceList(DCObject *dco)
{
   shared_ptr<Node> node;
   if (dco->getSourceNode() != 0)
   {
      node = static_pointer_cast<Node>(FindObjectById(dco->getSourceNode(), OBJECT_NODE));
      if (node == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_INSTANCE_POLL, 6, _T("Node::getInstanceList(%s [%u]): source node [%u] not found"), dco->getName().cstr(), dco->getId(), dco->getSourceNode());
         return nullptr;
      }
      if (!node->isTrustedObject(m_id))
      {
         nxlog_debug_tag(DEBUG_TAG_INSTANCE_POLL, 6, _T("Node::getInstanceList(%s [%u]): this node (%s [%u]) is not trusted by source node %s [%d]"),
                  dco->getName().cstr(), dco->getId(), m_name, m_id, node->getName(), node->getId());
         return nullptr;
      }
   }
   else
   {
      node = self();
   }

   StringList *instances = nullptr;
   StringMap *instanceMap = nullptr;
   shared_ptr<Table> instanceTable;
   wchar_t tableName[MAX_DB_STRING], nameColumn[MAX_DB_STRING];
   switch(dco->getInstanceDiscoveryMethod())
   {
      case IDM_AGENT_LIST:
         node->getListFromAgent(dco->getInstanceDiscoveryData(), &instances);
         break;
      case IDM_AGENT_TABLE:
      case IDM_INTERNAL_TABLE:
         parseInstanceDiscoveryTableName(dco->getInstanceDiscoveryData(), tableName, nameColumn);
         if (dco->getInstanceDiscoveryMethod() == IDM_AGENT_TABLE)
            node->getTableFromAgent(tableName, &instanceTable);
         else
            node->getInternalTable(tableName, &instanceTable);
         if (instanceTable != nullptr)
         {
            instanceMap = new StringMap();
            wchar_t buffer[1024];
            int nameColumnIndex = (nameColumn[0] != 0) ? instanceTable->getColumnIndex(nameColumn) : -1;
            for(int i = 0; i < instanceTable->getNumRows(); i++)
            {
               instanceTable->buildInstanceString(i, buffer, 1024);
               if (nameColumnIndex != -1)
               {
                  const wchar_t *name = instanceTable->getAsString(i, nameColumnIndex, buffer);
                  if (name != nullptr)
                     instanceMap->set(buffer, name);
                  else
                     instanceMap->set(buffer, buffer);
               }
               else
               {
                  instanceMap->set(buffer, buffer);
               }
            }
         }
         break;
      case IDM_SCRIPT:
         node->getStringMapFromScript(dco->getInstanceDiscoveryData(), &instanceMap, this);
         break;
      case IDM_SNMP_WALK_VALUES:
         node->getListFromSNMP(dco->getSnmpPort(), dco->getSnmpVersion(), dco->getInstanceDiscoveryData(), &instances);
         break;
      case IDM_SNMP_WALK_OIDS:
         node->getOIDSuffixListFromSNMP(dco->getSnmpPort(), dco->getSnmpVersion(), dco->getInstanceDiscoveryData(), &instanceMap);
         break;
      case IDM_WEB_SERVICE:
         node->getListFromWebService(dco->getInstanceDiscoveryData(), &instances);
         break;
      case IDM_WINPERF:
         wchar_t query[256];
         _sntprintf(query, 256, L"PDH.ObjectInstances(\"%s\")", EscapeStringForAgent(dco->getInstanceDiscoveryData()).cstr());
         node->getListFromAgent(query, &instances);
         break;
      case IDM_SMCLP_TARGETS:
         node->getTargetListFromSmclp(dco->getInstanceDiscoveryData(), &instances);
         break;
      case IDM_SMCLP_PROPERTIES:
         node->getPropertyListFromSmclp(dco->getInstanceDiscoveryData(), &instances);
         break;
      default:
         instances = nullptr;
         break;
   }
   if ((instances == nullptr) && (instanceMap == nullptr))
      return nullptr;

   if (instanceMap == nullptr)
   {
      instanceMap = new StringMap;
      for(int i = 0; i < instances->size(); i++)
         instanceMap->set(instances->get(i), instances->get(i));
   }
   delete instances;
   return instanceMap;
}

/**
 * Connect to native agent. Assumes that access to agent connection is already locked.
 */
bool Node::connectToAgent(uint32_t *error, uint32_t *socketError, bool *newConnection, bool forceConnect)
{
   if ((g_flags & AF_SHUTDOWN) || m_isDeleteInitiated)
      return false;

   if (!forceConnect && (m_agentConnection == nullptr) && (time(nullptr) - m_lastAgentConnectAttempt < 30))
   {
      nxlog_debug_tag(DEBUG_TAG_AGENT, 7, _T("Node::connectToAgent(%s [%u]): agent is unreachable, will not retry connection"), m_name, m_id);
      if (error != nullptr)
         *error = ERR_CONNECT_FAILED;
      if (socketError != nullptr)
         *socketError = 0;
      return false;
   }

   // Check if tunnel is available
   shared_ptr<AgentTunnel> tunnel = GetTunnelForNode(m_id);
   if ((tunnel == nullptr) && ((!m_ipAddress.isValidUnicast() && !((m_capabilities & NC_IS_LOCAL_MGMT) && m_ipAddress.isLoopback())) || (m_flags & NF_AGENT_OVER_TUNNEL_ONLY)))
   {
      nxlog_debug_tag(DEBUG_TAG_AGENT, 7, _T("Node::connectToAgent(%s [%u]): %s and there are no active tunnels"), m_name, m_id,
               (m_flags & NF_AGENT_OVER_TUNNEL_ONLY) ? _T("direct agent connections are disabled") : _T("node primary IP is invalid"));
      return false;
   }

   // Create new agent connection object if needed
   if (m_agentConnection == nullptr)
   {
      if (tunnel != nullptr)
      {
         m_agentConnection = make_shared<AgentConnectionEx>(m_id, tunnel, m_agentSecret, isAgentCompressionAllowed());
      }
      else
      {
         InetAddress addr = m_ipAddress;
         if ((m_capabilities & NC_IS_LOCAL_MGMT) && (g_mgmtAgentAddress[0] != 0))
         {
            // If node is local management node and external agent address is set, use it instead of node's primary IP address
            addr = InetAddress::resolveHostName(g_mgmtAgentAddress);
            if (addr.isValid())
            {
               nxlog_debug_tag(DEBUG_TAG_AGENT, 7, _T("Node::connectToAgent(%s [%u]): using external agent address %s"), m_name, m_id, g_mgmtAgentAddress);
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG_AGENT, 7, _T("Node::connectToAgent(%s [%u]): cannot resolve external agent address %s"), m_name, m_id, g_mgmtAgentAddress);
               return false;
            }
         }
         m_agentConnection = make_shared<AgentConnectionEx>(m_id, addr, m_agentPort, m_agentSecret, isAgentCompressionAllowed());
      }
      nxlog_debug_tag(DEBUG_TAG_AGENT, 7, _T("Node::connectToAgent(%s [%d]): new agent connection created"), m_name, m_id);
   }
   else
   {
      // Check if we already connected
      if (m_agentConnection->nop() == ERR_SUCCESS)
      {
         nxlog_debug_tag(DEBUG_TAG_AGENT, 7, _T("Node::connectToAgent(%s [%u]): already connected"), m_name, m_id);
         if (newConnection != nullptr)
            *newConnection = false;
         setLastAgentCommTime();
         return true;
      }

      // Close current connection or clean up after broken connection
      m_agentConnection->disconnect();
      m_agentConnection->setTunnel(tunnel);
      nxlog_debug_tag(DEBUG_TAG_AGENT, 7, _T("Node::connectToAgent(%s [%u]): existing connection reset"), m_name, m_id);
   }
   if (newConnection != nullptr)
      *newConnection = true;
   m_agentConnection->setPort(m_agentPort);
   m_agentConnection->setSharedSecret(m_agentSecret);
   if (tunnel == nullptr)
      setAgentProxy(m_agentConnection.get());
   m_agentConnection->setCommandTimeout(g_agentCommandTimeout);
   nxlog_debug_tag(DEBUG_TAG_AGENT, 7, _T("Node::connectToAgent(%s [%u]): calling connect on port %d"), m_name, m_id, (int)m_agentPort);
   bool success = m_agentConnection->connect(g_serverKey, error, socketError, g_serverId);
   if (success)
   {
      uint32_t rcc = m_agentConnection->setServerId(g_serverId);
      if (rcc == ERR_SUCCESS)
      {
         syncDataCollectionWithAgent(m_agentConnection.get());
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_AGENT, 5, _T("Node::connectToAgent(%s [%u]): cannot set server ID on agent (%s)"), m_name, m_id, AgentErrorCodeToText(rcc));
         if (rcc == ERR_UNKNOWN_COMMAND)
         {
            m_state |= NSF_CACHE_MODE_NOT_SUPPORTED;
         }
      }
      m_agentConnection->enableTraps();
      clearFileUpdateConnection();
      setLastAgentCommTime();
      CALL_ALL_MODULES(pfOnConnectToAgent, (self(), m_agentConnection));
   }
   else
   {
      deleteAgentConnection();
      m_lastAgentConnectAttempt = time(nullptr);
   }
   return success;
}

/**
 * Convert SNMP error code to DC collection error code
 */
inline DataCollectionError DCErrorFromSNMPError(UINT32 snmpError)
{
   switch(snmpError)
   {
      case SNMP_ERR_SUCCESS:
         return DCE_SUCCESS;
      case SNMP_ERR_NO_OBJECT:
      case SNMP_ERR_BAD_OID:
      case SNMP_ERR_PARAM:
         return DCE_NOT_SUPPORTED;
      case SNMP_ERR_AGENT:
         return DCE_COLLECTION_ERROR;
      default:
         return DCE_COMM_ERROR;
   }
}

/**
 * Get DCI value via SNMP. Buffer size should be at least 64 characters.
 */
DataCollectionError Node::getMetricFromSNMP(uint16_t port, SNMP_Version version, const TCHAR *name, TCHAR *buffer, size_t size, int interpretRawValue)
{
   if ((((m_state & NSF_SNMP_UNREACHABLE) || !(m_capabilities & NC_IS_SNMP)) && (port == 0)) ||
       (m_state & DCSF_UNREACHABLE) ||
       (m_flags & NF_DISABLE_SNMP))
   {
      nxlog_debug_tag(DEBUG_TAG_DC_SNMP _T(".error"), 7, _T("Node(%s)->getMetricFromSNMP(%s): SNMP unreachable or disabled (state=0x%08x flags=0x%08x is_snmp=%s)"),
         m_name, name, m_state, m_flags, BooleanToString((m_capabilities & NC_IS_SNMP) != 0));
      return DCErrorFromSNMPError(SNMP_ERR_COMM);
   }

   if (size < 64)
   {
      nxlog_debug_tag(DEBUG_TAG_DC_SNMP _T(".error"), 7, _T("Node(%s)->getMetricFromSNMP(%s): result buffer is too small"), m_name, name);
      return DCE_COLLECTION_ERROR;
   }

   uint32_t snmpResult;
   SNMP_Transport *snmp = createSnmpTransport(port, version);
   if (snmp != nullptr)
   {
      if (interpretRawValue == SNMP_RAWTYPE_NONE)
      {
         snmpResult = SnmpGetEx(snmp, name, nullptr, 0, buffer, size * sizeof(TCHAR), SG_PSTRING_RESULT, nullptr);
      }
      else
      {
         BYTE rawValue[1024];
         memset(rawValue, 0, 1024);
         uint32_t length;
         snmpResult = SnmpGetEx(snmp, name, nullptr, 0, rawValue, 1024, SG_RAW_RESULT, &length);
         if (snmpResult == SNMP_ERR_SUCCESS)
         {
            switch(interpretRawValue)
            {
               case SNMP_RAWTYPE_INT32:
                  IntegerToString(static_cast<int32_t>(ntohl(*reinterpret_cast<uint32_t*>(rawValue))), buffer);
                  break;
               case SNMP_RAWTYPE_UINT32:
                  IntegerToString(static_cast<uint32_t>(ntohl(*reinterpret_cast<uint32_t*>(rawValue))), buffer);
                  break;
               case SNMP_RAWTYPE_INT64:
                  IntegerToString(static_cast<int64_t>(ntohq(*reinterpret_cast<uint64_t*>(rawValue))), buffer);
                  break;
               case SNMP_RAWTYPE_UINT64:
                  IntegerToString(ntohq(*reinterpret_cast<uint64_t*>(rawValue)), buffer);
                  break;
               case SNMP_RAWTYPE_DOUBLE:
                  _sntprintf(buffer, size, _T("%f"), ntohd(*reinterpret_cast<double*>(rawValue)));
                  break;
               case SNMP_RAWTYPE_IP_ADDR:
                  if (length == 4)
                     IpToStr(ntohl(*reinterpret_cast<uint32_t*>(rawValue)), buffer);
                  else
                     buffer[0] = 0;
                  break;
               case SNMP_RAWTYPE_IP6_ADDR:
                  if (length == 16)
                     Ip6ToStr(rawValue, buffer);
                  else
                     buffer[0] = 0;
                  break;
               case SNMP_RAWTYPE_MAC_ADDR:
                  if ((length == 6) || (length == 8))
                     BinToStrEx(rawValue, length, buffer, _T(':'), 0);
                  else
                     buffer[0] = 0;
                  break;
               default:
                  buffer[0] = 0;
                  break;
            }
         }
      }
      delete snmp;

      if (snmpResult == SNMP_ERR_SUCCESS)
      {
         nxlog_debug_tag(DEBUG_TAG_DC_SNMP, 7, _T("Node(%s)->getMetricFromSNMP(%s): success"), m_name, name);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_DC_SNMP _T(".error"), 7, _T("Node(%s)->getMetricFromSNMP(%s): SNMP error %u (%s)"),
            m_name, name, snmpResult, SnmpGetErrorText(snmpResult));
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_DC_SNMP _T(".error"), 7, _T("Node(%s)->getMetricFromSNMP(%s): cannot create SNMP transport"), m_name, name);
      snmpResult = SNMP_ERR_COMM;
   }
   return DCErrorFromSNMPError(snmpResult);
}

/**
 * Read one row for SNMP table
 */
static uint32_t ReadSNMPTableRow(SNMP_Transport *snmp, const SNMP_ObjectId *rowOid, size_t baseOidLen,
         uint32_t index, const ObjectArray<DCTableColumn> &columns, Table *table)
{
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   for(int i = 0; i < columns.size(); i++)
   {
      const DCTableColumn *c = columns.get(i);
      if (c->getSnmpOid().isValid())
      {
         uint32_t oid[MAX_OID_LEN];
         size_t oidLen = c->getSnmpOid().length();
         memcpy(oid, c->getSnmpOid().value(), oidLen * sizeof(uint32_t));
         if (rowOid != nullptr)
         {
            size_t suffixLen = rowOid->length() - baseOidLen;
            memcpy(&oid[oidLen], rowOid->value() + baseOidLen, suffixLen * sizeof(uint32_t));
            oidLen += suffixLen;
         }
         else
         {
            oid[oidLen++] = index;
         }
         request.bindVariable(new SNMP_Variable(oid, oidLen));
      }
   }

   SNMP_PDU *response;
   uint32_t rc = snmp->doRequest(&request, &response);
   if (rc == SNMP_ERR_SUCCESS)
   {
      if (((int)response->getNumVariables() >= columns.size()) &&
          (response->getErrorCode() == SNMP_PDU_ERR_SUCCESS))
      {
         table->addRow();
         for(int i = 0; i < response->getNumVariables(); i++)
         {
            SNMP_Variable *v = response->getVariable(i);
            if ((v != nullptr) && (v->getType() != ASN_NO_SUCH_OBJECT) && (v->getType() != ASN_NO_SUCH_INSTANCE))
            {
               const DCTableColumn *c = columns.get(i);
               if ((c != nullptr) && c->isConvertSnmpStringToHex())
               {
                  size_t size = v->getValueLength();
                  TCHAR *buffer = MemAllocString(size * 2 + 1);
                  BinToStr(v->getValue(), size, buffer);
                  table->setPreallocated(i, buffer);
               }
               else
               {
                  bool convert = false;
                  TCHAR buffer[1024];
                  table->set(i, v->getValueAsPrintableString(buffer, 1024, &convert));
               }
            }
         }
      }
      delete response;
   }
   return rc;
}

/**
 * Get table from SNMP
 */
DataCollectionError Node::getTableFromSNMP(uint16_t port, SNMP_Version version, const TCHAR *oid, const ObjectArray<DCTableColumn> &columns, shared_ptr<Table> *table)
{
   SNMP_Transport *snmp = createSnmpTransport(port, version);
   if (snmp == nullptr)
      return DCE_COMM_ERROR;

   ObjectArray<SNMP_ObjectId> oidList(64, 64, Ownership::True);
   uint32_t rc = SnmpWalk(snmp, oid,
      [&oidList] (SNMP_Variable *varbind) -> uint32_t
      {
         oidList.add(new SNMP_ObjectId(varbind->getName()));
         return SNMP_ERR_SUCCESS;
      });
   if (rc == SNMP_ERR_SUCCESS)
   {
      *table = make_shared<Table>();
      for(int i = 0; i < columns.size(); i++)
      {
         const DCTableColumn *c = columns.get(i);
         if (c->getSnmpOid().isValid())
            (*table)->addColumn(c->getName(), c->getDataType(), c->getDisplayName(), c->isInstanceColumn());
      }

      size_t baseOidLen = SnmpGetOIDLength(oid);
      for(int i = 0; i < oidList.size(); i++)
      {
         rc = ReadSNMPTableRow(snmp, oidList.get(i), baseOidLen, 0, columns, table->get());
         if (rc != SNMP_ERR_SUCCESS)
         {
            table->reset();
            break;
         }
      }
   }
   delete snmp;
   return DCErrorFromSNMPError(rc);
}

/**
 * Callback for SnmpWalk in Node::getListFromSNMP
 */
static uint32_t SNMPGetListCallback(SNMP_Variable *varbind, SNMP_Transport *snmp, StringList *values)
{
   bool convert = false;
   TCHAR buffer[256];
   values->add(varbind->getValueAsPrintableString(buffer, 256, &convert));
   return SNMP_ERR_SUCCESS;
}

/**
 * Get list of values from SNMP
 */
DataCollectionError Node::getListFromSNMP(uint16_t port, SNMP_Version version, const TCHAR *oid, StringList **list)
{
   *list = nullptr;
   SNMP_Transport *snmp = createSnmpTransport(port, version);
   if (snmp == nullptr)
      return DCE_COMM_ERROR;

   *list = new StringList;
   uint32_t rc = SnmpWalk(snmp, oid, SNMPGetListCallback, *list);
   delete snmp;
   if (rc != SNMP_ERR_SUCCESS)
   {
      delete *list;
      *list = nullptr;
   }
   return DCErrorFromSNMPError(rc);
}

/**
 * Get list of OID suffixes from SNMP
 */
DataCollectionError Node::getOIDSuffixListFromSNMP(uint16_t port, SNMP_Version version, const TCHAR *oid, StringMap **values)
{
   *values = nullptr;
   SNMP_Transport *snmp = createSnmpTransport(port, version);
   if (snmp == nullptr)
      return DCE_COMM_ERROR;

   uint32_t oidBin[256];
   size_t oidLen = SnmpParseOID(oid, oidBin, 256);
   if (oidLen == 0)
   {
      delete snmp;
      return DCE_NOT_SUPPORTED;
   }

   auto oidSuffixes = new StringMap();
   uint32_t rc = SnmpWalk(snmp, oid,
      [oidLen, oidSuffixes] (SNMP_Variable *varbind) -> uint32_t
      {
         const SNMP_ObjectId& oid = varbind->getName();
         if (oid.length() <= oidLen)
            return SNMP_ERR_SUCCESS;
         TCHAR buffer[256];
         SnmpConvertOIDToText(oid.length() - oidLen, &(oid.value()[oidLen]), buffer, 256);

         const TCHAR *key = (buffer[0] == _T('.')) ? &buffer[1] : buffer;

         TCHAR value[256] = _T("");
         bool convert = false;
         varbind->getValueAsPrintableString(value, 256, &convert);
         oidSuffixes->set(key, (value[0] != 0) ? value : key);
         return SNMP_ERR_SUCCESS;
      });
   delete snmp;
   if (rc == SNMP_ERR_SUCCESS)
   {
      *values = oidSuffixes;
   }
   else
   {
      delete oidSuffixes;
   }
   return DCErrorFromSNMPError(rc);
}

/**
 * Get item's value via native agent
 */
DataCollectionError Node::getMetricFromAgent(const TCHAR *name, TCHAR *buffer, size_t size)
{
   if ((m_state & NSF_AGENT_UNREACHABLE) ||
       (m_state & DCSF_UNREACHABLE) ||
       (m_flags & NF_DISABLE_NXCP) ||
       !(m_capabilities & NC_IS_NATIVE_AGENT))
      return DCE_COMM_ERROR;

   uint32_t agentError = ERR_NOT_CONNECTED;
   DataCollectionError rc = DCE_COMM_ERROR;
   int retry = 3;

   shared_ptr<AgentConnectionEx> conn = getAgentConnection();
   if (conn == nullptr)
      goto end_loop;

   // Get parameter from agent
   while(retry-- > 0)
   {
      agentError = conn->getParameter(name, buffer, size);
      switch(agentError)
      {
         case ERR_SUCCESS:
            rc = DCE_SUCCESS;
            setLastAgentCommTime();
            goto end_loop;
         case ERR_UNKNOWN_METRIC:
         case ERR_UNSUPPORTED_METRIC:
            rc = DCE_NOT_SUPPORTED;
            setLastAgentCommTime();
            goto end_loop;
         case ERR_NO_SUCH_INSTANCE:
            rc = DCE_NO_SUCH_INSTANCE;
            setLastAgentCommTime();
            goto end_loop;
         case ERR_NOT_CONNECTED:
         case ERR_CONNECTION_BROKEN:
         case ERR_REQUEST_TIMEOUT:
            conn = getAgentConnection();
            if (conn == nullptr)
               goto end_loop;
            break;
         case ERR_INTERNAL_ERROR:
            rc = DCE_COLLECTION_ERROR;
            setLastAgentCommTime();
            goto end_loop;
      }
   }

end_loop:
   nxlog_debug_tag(DEBUG_TAG_DC_AGENT, 7, _T("Node(%s)->getMetricFromAgent(%s): agentError=%u rc=%d"), m_name, name, agentError, rc);
   return rc;
}

/**
 * Helper function to get metric from agent as double
 */
double Node::getMetricFromAgentAsDouble(const TCHAR *name, double defaultValue)
{
   TCHAR buffer[MAX_RESULT_LENGTH];
   if (getMetricFromAgent(name, buffer, MAX_RESULT_LENGTH) != DCE_SUCCESS)
      return defaultValue;

   TCHAR *eptr;
   double v = _tcstod(buffer, &eptr);
   return (*eptr == 0) ? v : defaultValue;
}

/**
 * Helper function to get metric from agent as INT32
 */
int32_t Node::getMetricFromAgentAsInt32(const TCHAR *name, int32_t defaultValue)
{
   TCHAR buffer[MAX_RESULT_LENGTH];
   if (getMetricFromAgent(name, buffer, MAX_RESULT_LENGTH) != DCE_SUCCESS)
      return defaultValue;

   TCHAR *eptr;
   int32_t v = _tcstol(buffer, &eptr, 0);
   return (*eptr == 0) ? v : defaultValue;
}

/**
 * Helper function to get metric from agent as UINT32
 */
uint32_t Node::getMetricFromAgentAsUInt32(const TCHAR *name, uint32_t defaultValue)
{
   TCHAR buffer[MAX_RESULT_LENGTH];
   if (getMetricFromAgent(name, buffer, MAX_RESULT_LENGTH) != DCE_SUCCESS)
      return defaultValue;

   TCHAR *eptr;
   uint32_t v = _tcstoul(buffer, &eptr, 0);
   return (*eptr == 0) ? v : defaultValue;
}

/**
 * Helper function to get metric from agent as INT64
 */
int64_t Node::getMetricFromAgentAsInt64(const TCHAR *name, int64_t defaultValue)
{
   TCHAR buffer[MAX_RESULT_LENGTH];
   if (getMetricFromAgent(name, buffer, MAX_RESULT_LENGTH) != DCE_SUCCESS)
      return defaultValue;

   TCHAR *eptr;
   int64_t v = _tcstoll(buffer, &eptr, 0);
   return (*eptr == 0) ? v : defaultValue;
}

/**
 * Helper function to get metric from agent as UINT64
 */
uint64_t Node::getMetricFromAgentAsUInt64(const TCHAR *name, uint64_t defaultValue)
{
   TCHAR buffer[MAX_RESULT_LENGTH];
   if (getMetricFromAgent(name, buffer, MAX_RESULT_LENGTH) != DCE_SUCCESS)
      return defaultValue;

   TCHAR *eptr;
   uint64_t v = _tcstoull(buffer, &eptr, 0);
   return (*eptr == 0) ? v : defaultValue;
}

/**
 * Get table from agent
 */
DataCollectionError Node::getTableFromAgent(const TCHAR *name, shared_ptr<Table> *table)
{
   uint32_t error = ERR_NOT_CONNECTED;
   DataCollectionError result = DCE_COMM_ERROR;
   uint32_t retries = 3;

   if ((m_state & NSF_AGENT_UNREACHABLE) ||
       (m_state & DCSF_UNREACHABLE) ||
       (m_flags & NF_DISABLE_NXCP) ||
       !(m_capabilities & NC_IS_NATIVE_AGENT))
      return DCE_COMM_ERROR;

   shared_ptr<AgentConnectionEx> conn = getAgentConnection();
   if (conn == nullptr)
      goto end_loop;

   // Get parameter from agent
   while(retries-- > 0)
   {
      Table *tableObject;
      error = conn->getTable(name, &tableObject);
      switch(error)
      {
         case ERR_SUCCESS:
            *table = shared_ptr<Table>(tableObject);
            result = DCE_SUCCESS;
            setLastAgentCommTime();
            goto end_loop;
         case ERR_UNKNOWN_METRIC:
         case ERR_UNSUPPORTED_METRIC:
            result = DCE_NOT_SUPPORTED;
            setLastAgentCommTime();
            goto end_loop;
         case ERR_NO_SUCH_INSTANCE:
            result = DCE_NO_SUCH_INSTANCE;
            setLastAgentCommTime();
            goto end_loop;
         case ERR_NOT_CONNECTED:
         case ERR_CONNECTION_BROKEN:
         case ERR_REQUEST_TIMEOUT:
            conn = getAgentConnection();
            if (conn == nullptr)
               goto end_loop;
            break;
         case ERR_INTERNAL_ERROR:
            result = DCE_COLLECTION_ERROR;
            setLastAgentCommTime();
            goto end_loop;
      }
   }

end_loop:
   nxlog_debug_tag(DEBUG_TAG_DC_AGENT, 7, _T("Node(%s)->getTableFromAgent(%s): error=%u result=%d"), m_name, name, error, result);
   return result;
}

/**
 * Get list from agent
 */
DataCollectionError Node::getListFromAgent(const TCHAR *name, StringList **list)
{
   uint32_t agentError = ERR_NOT_CONNECTED;
   DataCollectionError rc = DCE_COMM_ERROR;
   int retryCount = 3;

   *list = nullptr;

   if ((m_state & NSF_AGENT_UNREACHABLE) ||
       (m_state & DCSF_UNREACHABLE) ||
       (m_flags & NF_DISABLE_NXCP) ||
       !(m_capabilities & NC_IS_NATIVE_AGENT))
      return DCE_COMM_ERROR;

   shared_ptr<AgentConnectionEx> conn = getAgentConnection();
   if (conn == nullptr)
      goto end_loop;

   // Get parameter from agent
   while(retryCount-- > 0)
   {
      agentError = conn->getList(name, list);
      switch(agentError)
      {
         case ERR_SUCCESS:
            rc = DCE_SUCCESS;
            setLastAgentCommTime();
            goto end_loop;
         case ERR_UNKNOWN_METRIC:
         case ERR_UNSUPPORTED_METRIC:
            rc = DCE_NOT_SUPPORTED;
            setLastAgentCommTime();
            goto end_loop;
         case ERR_NO_SUCH_INSTANCE:
            rc = DCE_NO_SUCH_INSTANCE;
            setLastAgentCommTime();
            goto end_loop;
         case ERR_NOT_CONNECTED:
         case ERR_CONNECTION_BROKEN:
         case ERR_REQUEST_TIMEOUT:
            conn = getAgentConnection();
            if (conn == nullptr)
               goto end_loop;
            break;
         case ERR_INTERNAL_ERROR:
            rc = DCE_COLLECTION_ERROR;
            setLastAgentCommTime();
            goto end_loop;
      }
   }

end_loop:
   nxlog_debug_tag(DEBUG_TAG_DC_AGENT, 7, _T("Node(%s)->getListFromAgent(%s): agentError=%u rc=%d"), m_name, name, agentError, rc);
   return rc;
}

/**
 * Get data form SM-CLP (internal method)
 */
bool Node::getDataFromSmclp(const wchar_t *command, StringBuffer *output)
{
   uint32_t proxyId = getEffectiveSshProxy();
   shared_ptr<Node> proxy = static_pointer_cast<Node>(FindObjectById(proxyId, OBJECT_NODE));
   uint32_t rcc = ERR_NOT_CONNECTED;
   if (proxy != nullptr)
   {
      shared_ptr<AgentConnectionEx> conn = proxy->acquireProxyConnection(ProxyType::SMCLP_PROXY, false);
      if (conn != nullptr)
      {
         StringList arguments;

         lockProperties();
         wchar_t ipAddr[64];
         arguments.add(m_ipAddress.toString(ipAddr));
         arguments.add(m_sshPort);
         arguments.add(m_sshLogin);
         arguments.add(m_sshPassword);
         arguments.add(command);
         arguments.add(m_sshKeyId);
         unlockProperties();

         rcc = conn->executeCommand(_T("SSH.Command"), arguments, true,
            [] (ActionCallbackEvent event, const TCHAR *text, void *context) -> void
            {
               if (event == ACE_DATA)
                  static_cast<StringBuffer*>(context)->append(text);
            }, output);
      }
   }
   return rcc == ERR_SUCCESS;
}

/**
 * Get item's value via SM-CLP protocol
 */
DataCollectionError Node::getMetricFromSmclp(const wchar_t *metric, wchar_t *buffer, size_t size)
{
   DataCollectionError result = DCE_COLLECTION_ERROR;

   if (!(m_capabilities & NC_IS_SMCLP))
      return DCE_NOT_SUPPORTED;

   if ((m_state & DCSF_UNREACHABLE) || (m_state & NSF_SSH_UNREACHABLE))
      return DCE_COMM_ERROR;

   if (wcsspn(metric, SMCLP_ALLOWED_SYMBOLS) != wcslen(metric))
      return DCE_NOT_SUPPORTED;

   wchar_t path[MAX_PARAM_NAME];
   wcslcpy(path, metric, MAX_PARAM_NAME);
   wchar_t *property = wcsrchr(path, L'/');
   if (property == nullptr)
      return DCE_NOT_SUPPORTED;

   *property = L' ';
   property++;

   StringBuffer output;
   StringBuffer command(L"show ");
   command.append(path);
   if (getDataFromSmclp(command, &output))
   {
      wcscat(property, L"=");

      ssize_t propertyIndex = output.find(property);
      if (propertyIndex == -1)
         return DCE_COLLECTION_ERROR;

      ssize_t valueSart = propertyIndex + wcslen(property);
      ssize_t valueEnd = output.find(L"\n", valueSart);
      if (propertyIndex == -1)
         return DCE_COLLECTION_ERROR;

      wcslcpy(buffer, output + valueSart, MIN(size, valueEnd - valueSart));
      result = DCE_SUCCESS;
   }

   nxlog_debug_tag(_T("dc.smclp"), 7, _T("Node(%s)->getMetricFromSmclp(%s): result=%d"), m_name, metric, result);
   return result;
}

/**
 * Get list of targets for provided path
 */
DataCollectionError Node::getTargetListFromSmclp(const wchar_t *target, StringList **list)
{
   *list = nullptr;
   DataCollectionError result = DCE_COLLECTION_ERROR;

   if (!(m_capabilities & NC_IS_SMCLP))
      return DCE_NOT_SUPPORTED;

   if ((m_state & DCSF_UNREACHABLE) || (m_state & NSF_SSH_UNREACHABLE))
      return DCE_COMM_ERROR;

   if (wcsspn(target, SMCLP_ALLOWED_SYMBOLS) != wcslen(target))
      return DCE_NOT_SUPPORTED;

   StringBuffer output;

   StringBuffer command(L"show ");
   command.append(target);
   if (getDataFromSmclp(command, &output))
   {
      if (!output.contains(L"Properties"))
         return DCE_COLLECTION_ERROR;

      StringList lines = output.split(L"\n", true);
      *list = new StringList();
      int state = 0;
      for (int i = 0; i < lines.size(); i++)
      {
         String line = lines.get(i);
         if (state == 1)
         {
            if (line.contains(L"Properties"))
               break;

            (*list)->add(line);
         }
         if (line.contains(L"Targets"))
         {
            state = 1;
         }
      }
      result = DCE_SUCCESS;
   }

   nxlog_debug_tag(_T("dc.smclp"), 7, _T("Node(%s)->getTargetListFromSmclp(%s): result=%d"), m_name, target, result);
   return result;
}

/**
 * Get list of properties for provided path
 */
DataCollectionError Node::getPropertyListFromSmclp(const wchar_t *target, StringList **list)
{
   *list = nullptr;
   DataCollectionError result = DCE_COLLECTION_ERROR;

   if (!(m_capabilities & NC_IS_SMCLP))
      return DCE_NOT_SUPPORTED;

   if ((m_state & DCSF_UNREACHABLE) || (m_state & NSF_SSH_UNREACHABLE))
      return DCE_COMM_ERROR;

   if (wcsspn(target, SMCLP_ALLOWED_SYMBOLS) != wcslen(target))
      return DCE_NOT_SUPPORTED;

   StringBuffer output;

   StringBuffer command(L"show ");
   command.append(target);
   if (getDataFromSmclp(command, &output))
   {
      if (!output.contains(L"Verbs"))
         return DCE_COLLECTION_ERROR;

      StringList lines = output.split(L"\n", true);
      *list = new StringList();
      int state = 0;
      for (int i = 0; i < lines.size(); i++)
      {
         String line = lines.get(i);
         if (state == 1)
         {
            if (line.contains(L"Verbs"))
               break;

            (*list)->add(line.substring(0, line.find(L"=")));
         }
         if (line.contains(L"Properties"))
         {
            state = 1;
         }
      }
      result = DCE_SUCCESS;
   }

   nxlog_debug_tag(_T("dc.smclp"), 7, _T("Node(%s)->getPropertyListFromSmclp(%s): result=%d"), m_name, target, result);
   return result;
}

/**
 * Update list of possible parameters via SM-CLP protocol
 * Object should be locked while call
 */
StringList *Node::getAvailableMetricFromSmclp()
{
   if ((m_state & DCSF_UNREACHABLE) || (m_state & NSF_SSH_UNREACHABLE) || !(m_capabilities & NC_IS_SMCLP))
      return nullptr;

   StringBuffer output;
   StringList *metrics = nullptr;
   if (getDataFromSmclp(L"show -a", &output))
   {
      metrics = new StringList();
      StringList currentElements;
      int state = 0;
      StringBuffer root;

      StringList list = output.split(L"\n", true);
      for (int i = 0; i < list.size(); i++)
      {
         String string = list.get(i);
         switch (state)
         {
            case 0: //previous entry point finished
               if (string.startsWith(L"/"))
               {
                  root = string;
                  state = 1;
               }
               break;
            case 1: //search for target
               if (string.equals(L"Properties"))
               {
                  state = 2;
               }
               break;
            case 2: //read all targets will it's not
               if (string.equals(L"Verbs"))
               {
                  state = 3;
               }
               else
               {
                  StringBuffer sb(root);
                  sb.append(L"/");
                  sb.append(string.substring(0, string.find(L"=")));
                  currentElements.add(sb);
               }
               break;
            case 3: // check if target is dynamic
               if (!string.contains(L"delete")) // ignore dynamic target properties
               {
                  metrics->addAll(currentElements);
               }
               currentElements.clear();
               state = 0;
               break;
         }
      }
   }

   nxlog_debug_tag(_T("dc.smclp"), 7, _T("Node::getAvailableMetricFromSMCLP(%s)"), m_name);
   return metrics;
}

/**
 * Get metric value from network device driver
 */
DataCollectionError Node::getMetricFromDeviceDriver(const TCHAR *metric, TCHAR *buffer, size_t size)
{
   lockProperties();
   NetworkDeviceDriver *driver = m_driver;
   unlockProperties();

   if ((driver == nullptr) || !driver->hasMetrics())
      return DCE_NOT_SUPPORTED;

   SNMP_Transport *transport = createSnmpTransport();
   if (transport == nullptr)
      return DCE_COMM_ERROR;
   DataCollectionError rc = driver->getMetric(transport, this, m_driverData, metric, buffer, size);
   delete transport;
   return rc;
}

/**
 * Get metric value via Modbus protocol
 */
DataCollectionError Node::getMetricFromModbus(const TCHAR *metric, TCHAR *buffer, size_t size, uint16_t defaultUnitId)
{
   if (!(m_capabilities & NC_IS_MODBUS_TCP))
      return DCE_NOT_SUPPORTED;

   uint16_t unitId = (defaultUnitId != 0xFFFF) ? defaultUnitId : m_modbusUnitId;
   const TCHAR *source;
   const TCHAR *conversion;
   int address = 0;
   if (!ParseModbusMetric(metric, &unitId, &source, &address, &conversion))
   {
      nxlog_debug_tag(DEBUG_TAG_DC_MODBUS, 7, _T("Node(%s)->getMetricFromModbus(%s): cannot parse metric"), m_name, metric);
      return DCE_NOT_SUPPORTED;
   }

   ModbusTransport *transport = createModbusTransport();
   if (transport == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_DC_MODBUS, 7, _T("Node(%s)->getMetricFromModbus(%s): cannot create Modbus transport"), m_name, metric);
      return DCE_COMM_ERROR;
   }

   ModbusOperationStatus status;
   if (!_tcsnicmp(source, _T("hold:"), 5))
   {
      status = transport->readHoldingRegisters(address, conversion, buffer);
   }
   else if (!_tcsnicmp(source, _T("input:"), 6))
   {
      status = transport->readInputRegisters(address, conversion, buffer);
   }
   else if (!_tcsnicmp(source, _T("coil:"), 5))
   {
      status = transport->readCoil(address, buffer);
   }
   else if (!_tcsnicmp(source, _T("discrete:"), 9))
   {
      status = transport->readDiscreteInput(address, buffer);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_DC_MODBUS, 7, _T("Node(%s)->getMetricFromModbus(%s): invalid source"), m_name, metric);
      status = MODBUS_STATUS_PROTOCOL_ERROR;
   }
   delete transport;

   DataCollectionError result = (status == MODBUS_STATUS_SUCCESS) ? DCE_SUCCESS :
            ((status == MODBUS_STATUS_PROTOCOL_ERROR) ? DCE_NOT_SUPPORTED : DCE_COMM_ERROR);
   nxlog_debug_tag(DEBUG_TAG_DC_MODBUS, 7, _T("Node(%s)->getMetricFromModbus(%s): modbusStatus=%d result=%d"), m_name, metric, status, result);
   return result;
}

/**
 * Get metric value via EtherNet/IP protocol
 */
DataCollectionError Node::getMetricFromEtherNetIP(const TCHAR *metric, TCHAR *buffer, size_t size)
{
   if (!(m_capabilities & NC_IS_ETHERNET_IP))
      return DCE_NOT_SUPPORTED;

   DataCollectionError result = GetEtherNetIPAttribute(m_ipAddress, m_eipPort, metric, 5000, buffer, size);
   nxlog_debug_tag(DEBUG_TAG_DC_MODBUS, 7, _T("Node(%s)->getMetricFromEtherNetIP(%s): result=%d"), m_name, metric, result);
   return result;
}

/**
 * Get value for server's internal table parameter
 */
DataCollectionError Node::getInternalTable(const TCHAR *name, shared_ptr<Table> *result)
{
   DataCollectionError rc = super::getInternalTable(name, result);
   if (rc != DCE_NOT_SUPPORTED)
      return rc;
   rc = DCE_SUCCESS;

   if (!_tcsicmp(name, _T("Hardware.Components")))
   {
      lockProperties();
      if (m_hardwareComponents != nullptr)
      {
         auto table = make_shared<Table>();
         table->addColumn(_T("CATEGORY"), DCI_DT_STRING, _T("Category"), true);
         table->addColumn(_T("INDEX"), DCI_DT_UINT, _T("Index"), true);
         table->addColumn(_T("TYPE"), DCI_DT_STRING, _T("Type"));
         table->addColumn(_T("VENDOR"), DCI_DT_STRING, _T("Vendor"));
         table->addColumn(_T("MODEL"), DCI_DT_STRING, _T("Model"));
         table->addColumn(_T("CAPACITY"), DCI_DT_UINT64, _T("Capacity"));
         table->addColumn(_T("PART_NUMBER"), DCI_DT_STRING, _T("Part number"));
         table->addColumn(_T("SERIAL_NUMBER"), DCI_DT_STRING, _T("Serial number"));
         table->addColumn(_T("LOCATION"), DCI_DT_STRING, _T("Location"));
         table->addColumn(_T("DESCRIPTION"), DCI_DT_STRING, _T("Description"));

         int hcSize = m_hardwareComponents->size();
         for (int i = 0; i < hcSize; i++)
         {
            HardwareComponent *hc = m_hardwareComponents->get(i);
            table->addRow();
            table->set(0, hc->getCategoryName());
            table->set(1, hc->getIndex());
            table->set(2, hc->getType());
            table->set(3, hc->getVendor());
            table->set(4, hc->getModel());
            table->set(5, hc->getCapacity());
            table->set(6, hc->getPartNumber());
            table->set(7, hc->getSerialNumber());
            table->set(8, hc->getLocation());
            table->set(9, hc->getDescription());
         }
         *result = table;
      }
      else
      {
         rc = DCE_COLLECTION_ERROR;
      }
      unlockProperties();
   }
   else if (!_tcsicmp(name, _T("Network.Interfaces")))
   {
      auto table = make_shared<Table>();
      table->addColumn(_T("ID"), DCI_DT_UINT, _T("ID"), true);
      table->addColumn(_T("INDEX"), DCI_DT_UINT, _T("Index"));
      table->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"));
      table->addColumn(_T("DESCRIPTION"), DCI_DT_STRING, _T("Description"));
      table->addColumn(_T("ALIAS"), DCI_DT_STRING, _T("Alias"));
      table->addColumn(_T("TYPE"), DCI_DT_UINT, _T("Type"));
      table->addColumn(_T("SPEED"), DCI_DT_UINT64, _T("Speed"));
      table->addColumn(_T("MTU"), DCI_DT_UINT, _T("MTU"));
      table->addColumn(_T("MAC_ADDRESS"), DCI_DT_STRING, _T("MAC address"));
      table->addColumn(_T("IP_ADDRESSES"), DCI_DT_STRING, _T("IP addresses"));
      table->addColumn(_T("ADMIN_STATE"), DCI_DT_UINT, _T("Administrative state"));
      table->addColumn(_T("OPER_STATE"), DCI_DT_UINT, _T("Operational state"));
      table->addColumn(_T("UTILIZATION_IN"), DCI_DT_FLOAT, _T("Inbound utilization"));
      table->addColumn(_T("UTILIZATION_OUT"), DCI_DT_FLOAT, _T("Outbound utilization"));
      table->addColumn(_T("BPORT"), DCI_DT_UINT, _T("Bridge port"));
      table->addColumn(_T("LOCATION"), DCI_DT_STRING, _T("Location"));
      table->addColumn(_T("PEER_IFACE_ID"), DCI_DT_UINT, _T("Peer interface ID"));
      table->addColumn(_T("PEER_NODE_ID"), DCI_DT_UINT, _T("Peer node ID"));
      table->addColumn(_T("PEER_PROTO"), DCI_DT_UINT, _T("Peer discovery protocol"));

      TCHAR buffer[256];
      readLockChildList();
      for (int i = 0; i < getChildList().size(); i++)
      {
         const NetObj *object = getChildList().get(i);
         if (object->getObjectClass() != OBJECT_INTERFACE)
            continue;

         const Interface *iface = static_cast<const Interface*>(object);
         table->addRow();
         table->set(0, iface->getId());
         table->set(1, iface->getIfIndex());
         table->set(2, iface->getName());
         table->set(3, iface->getDescription());
         table->set(4, iface->getIfAlias());
         table->set(5, iface->getIfType());
         table->set(6, iface->getSpeed());
         table->set(7, iface->getMTU());
         table->set(8, iface->getMacAddress().toString(buffer));
         table->set(9, iface->getIpAddressList()->toString());
         table->set(10, iface->getAdminState());
         table->set(11, iface->getOperState());
         if (iface->getInboundUtilization() >= 0)
            table->set(12, static_cast<float>(iface->getInboundUtilization()) / 10.0);
         if (iface->getOutboundUtilization() >= 0)
            table->set(13, static_cast<float>(iface->getOutboundUtilization()) / 10.0);
         table->set(14, iface->getBridgePortNumber());
         table->set(15, iface->getPhysicalLocation().toString(buffer, 256));
         table->set(16, iface->getPeerInterfaceId());
         table->set(17, iface->getPeerNodeId());
         table->set(18, iface->getPeerDiscoveryProtocol());
      }
      unlockChildList();
      *result = table;
   }
   else if (!_tcsicmp(name, _T("Topology.OSPF.Areas")))
   {
      if (isOSPFSupported())
      {
         auto table = make_shared<Table>();
         table->addColumn(_T("ID"), DCI_DT_STRING, _T("ID"), true);
         table->addColumn(_T("LSA"), DCI_DT_UINT, _T("LSA count"));
         table->addColumn(_T("ABR"), DCI_DT_UINT, _T("Area border router count"));
         table->addColumn(_T("ASBR"), DCI_DT_UINT, _T("AS border router count"));

         TCHAR areaId[16];
         lockProperties();
         for(int i = 0; i < m_ospfAreas.size(); i++)
         {
            OSPFArea *a = m_ospfAreas.get(i);
            table->addRow();
            table->set(0, IpToStr(a->id, areaId));
            table->set(1, a->lsaCount);
            table->set(2, a->areaBorderRouterCount);
            table->set(3, a->asBorderRouterCount);
         }
         unlockProperties();
         *result = table;
      }
      else
      {
         rc = DCE_NOT_SUPPORTED;
      }
   }
   else if (!_tcsicmp(name, _T("Topology.OSPF.Neighbors")))
   {
      if (isOSPFSupported())
      {
         auto table = make_shared<Table>();
         table->addColumn(_T("ROUTER_ID"), DCI_DT_STRING, _T("Router ID"), true);
         table->addColumn(_T("IP_ADDRESS"), DCI_DT_STRING, _T("IP address"), true);
         table->addColumn(_T("NODE_ID"), DCI_DT_UINT, _T("Node ID"));
         table->addColumn(_T("NODE_NAME"), DCI_DT_STRING, _T("Node name"));
         table->addColumn(_T("IF_INDEX"), DCI_DT_UINT, _T("Interface index"));
         table->addColumn(_T("IF_NAME"), DCI_DT_STRING, _T("Interface name"));
         table->addColumn(_T("AREA_ID"), DCI_DT_STRING, _T("Area ID"));
         table->addColumn(_T("IS_VIRTUAL"), DCI_DT_INT, _T("Is virtual"));
         table->addColumn(_T("STATE"), DCI_DT_STRING, _T("State"));

         TCHAR id[16];
         lockProperties();
         for(int i = 0; i < m_ospfNeighbors.size(); i++)
         {
            OSPFNeighbor *n = m_ospfNeighbors.get(i);
            table->addRow();
            table->set(0, IpToStr(n->routerId, id));
            table->set(1, n->ipAddress.toString());
            table->set(2, n->nodeId);
            table->set(3, GetObjectName(n->nodeId, _T("")));
            table->set(4, n->ifIndex);
            table->set(5, GetObjectName(n->ifObject, _T("")));
            table->set(6, n->isVirtual ? IpToStr(n->areaId, id) : _T(""));
            table->set(7, static_cast<int32_t>(n->isVirtual ? 1 : 0));
            table->set(8, OSPFNeighborStateToText(n->state));
         }
         unlockProperties();
         *result = table;
      }
      else
      {
         rc = DCE_NOT_SUPPORTED;
      }
   }
   else if (!_tcsicmp(name, _T("Topology.RoutingTable")))
   {
      shared_ptr<RoutingTable> rt = getCachedRoutingTable();
      if (rt != nullptr)
      {
         auto table = make_shared<Table>();
         table->addColumn(_T("ID"), DCI_DT_INT, _T("ID"), true);
         table->addColumn(_T("DEST_ADDR"), DCI_DT_UINT, _T("Destination address"));
         table->addColumn(_T("DEST_MASK"), DCI_DT_UINT, _T("Destination mask"));
         table->addColumn(_T("NEXT_HOP"), DCI_DT_UINT, _T("Next hop"));
         table->addColumn(_T("IF_INDEX"), DCI_DT_UINT, _T("Interface index"));
         table->addColumn(_T("IF_NAME"), DCI_DT_STRING, _T("Interface name"));
         table->addColumn(_T("ROUTE_TYPE"), DCI_DT_UINT, _T("Route type"));
         table->addColumn(_T("METRIC"), DCI_DT_UINT, _T("Metric"));
         table->addColumn(_T("PROTOCOL"), DCI_DT_STRING, _T("Protocol"));

         for(int i = 0; i < rt->size(); i++)
         {
            auto *r = rt->get(i);
            table->addRow();
            table->set(0, i + 1);
            table->set(1, r->destination.toString());
            table->set(2, r->destination.getMaskBits());
            table->set(3, r->nextHop.toString());
            table->set(4, r->ifIndex);
            auto iface = findInterfaceByIndex(r->ifIndex);
            table->set(5, (iface != nullptr) ? iface->getName() : _T(""));
            table->set(6, r->routeType);
            table->set(7, r->metric);
            table->set(8, r->protocol);
         }
         *result = table;
      }
      else
      {
         rc = isBridge() ? DCE_COLLECTION_ERROR : DCE_NOT_SUPPORTED;
      }
   }
   else if (!_tcsicmp(name, _T("Topology.SwitchForwardingDatabase")))
   {
      shared_ptr<ForwardingDatabase> fdb = getSwitchForwardingDatabase();
      if (fdb != nullptr)
      {
         *result = fdb->getAsTable();
      }
      else
      {
         rc = isBridge() ? DCE_COLLECTION_ERROR : DCE_NOT_SUPPORTED;
      }
   }
   else if (!_tcsicmp(name, _T("Topology.WirelessStations")))
   {
      *result = wsListAsTable();
      if (*result == nullptr)
      {
         rc = isWirelessController() ? DCE_COLLECTION_ERROR : DCE_NOT_SUPPORTED;
      }
   }
   else if (m_capabilities & NC_IS_LOCAL_MGMT)
   {
      if (!_tcsicmp(name, _T("Server.EventProcessors")))
      {
         auto table = make_shared<Table>();
         table->addColumn(_T("ID"), DCI_DT_INT, _T("ID"), true);
         table->addColumn(_T("BINDINGS"), DCI_DT_UINT, _T("Bindings"));
         table->addColumn(_T("QUEUE_SIZE"), DCI_DT_UINT, _T("Queue Size"));
         table->addColumn(_T("AVG_WAIT_TIME"), DCI_DT_UINT, _T("Avg. Wait Time"));
         table->addColumn(_T("MAX_WAIT_TIME"), DCI_DT_UINT, _T("Max Wait Time"));
         table->addColumn(_T("PROCESSED_EVENTS"), DCI_DT_COUNTER64, _T("Processed Events"));

         StructArray<EventProcessingThreadStats> *stats = GetEventProcessingThreadStats();
         for(int i = 0; i < stats->size(); i++)
         {
            EventProcessingThreadStats *s = stats->get(i);
            table->addRow();
            table->set(0, i + 1);
            table->set(1, s->bindings);
            table->set(2, s->queueSize);
            table->set(3, s->averageWaitTime);
            table->set(4, s->maxWaitTime);
            table->set(5, s->processedEvents);
         }
         delete stats;

         *result = table;
      }
      else if (!_tcsicmp(name, _T("Server.NotificationChannels")))
      {
         auto table = make_shared<Table>();
         GetNotificationChannels(table.get());
         *result = table;
      }
      else if (!_tcsicmp(name, _T("Server.Queues")))
      {
         auto table = make_shared<Table>();
         GetAllQueueStatistics(table.get());
         *result = table;
      }
      else
      {
         rc = DCE_NOT_SUPPORTED;
      }
   }
   else
   {
      rc = DCE_NOT_SUPPORTED;
   }

   return rc;
}

/**
 * Get statistic for specific event processor
 */
static DataCollectionError GetEventProcessorStatistic(const TCHAR *param, int type, TCHAR *value)
{
   TCHAR pidText[64];
   if (!AgentGetParameterArg(param, 1, pidText, 64))
      return DCE_NOT_SUPPORTED;
   int pid = _tcstol(pidText, nullptr, 0);
   if (pid < 1)
      return DCE_NOT_SUPPORTED;

   StructArray<EventProcessingThreadStats> *stats = GetEventProcessingThreadStats();
   if (pid > stats->size())
   {
      delete stats;
      return DCE_NOT_SUPPORTED;
   }

   auto s = stats->get(pid - 1);
   switch(type)
   {
      case 'B':
         ret_uint(value, s->bindings);
         break;
      case 'P':
         ret_uint64(value, s->processedEvents);
         break;
      case 'Q':
         ret_uint(value, s->queueSize);
         break;
      case 'W':
         ret_uint(value, s->averageWaitTime);
         break;
   }

   delete stats;
   return DCE_SUCCESS;
}

/**
 * Get notification channel status and execute provided callback
 */
static DataCollectionError GetNotificationChannelStatus(const TCHAR *metric, std::function<void (const NotificationChannelStatus&)> callback)
{
   wchar_t channel[64];
   AgentGetParameterArg(metric, 1, channel, 64);

   NotificationChannelStatus status;
   if (!GetNotificationChannelStatus(channel, &status))
      return DCE_NO_SUCH_INSTANCE;

   callback(status);
   return DCE_SUCCESS;
}

/**
 * Get value for server's internal parameter
 */
DataCollectionError Node::getInternalMetric(const TCHAR *name, TCHAR *buffer, size_t size)
{
   DataCollectionError rc = super::getInternalMetric(name, buffer, size);
   if (rc != DCE_NOT_SUPPORTED)
      return rc;
   rc = DCE_SUCCESS;

   if (!_tcsicmp(name, _T("AgentStatus")))
   {
      if (m_capabilities & NC_IS_NATIVE_AGENT)
      {
         buffer[0] = (m_state & NSF_AGENT_UNREACHABLE) ? _T('1') : _T('0');
         buffer[1] = 0;
      }
      else
      {
         _tcscpy(buffer, _T("-1"));
      }
   }
   else if (!_tcsicmp(_T("ICMP.Jitter"), name))
   {
      rc = getIcmpStatistic(nullptr, IcmpStatFunction::JITTER, buffer);
   }
   else if (MatchString(_T("ICMP.Jitter(*)"), name, false))
   {
      rc = getIcmpStatistic(name, IcmpStatFunction::JITTER, buffer);
   }
   else if (!_tcsicmp(_T("ICMP.PacketLoss"), name))
   {
      rc = getIcmpStatistic(nullptr, IcmpStatFunction::LOSS, buffer);
   }
   else if (MatchString(_T("ICMP.PacketLoss(*)"), name, false))
   {
      rc = getIcmpStatistic(name, IcmpStatFunction::LOSS, buffer);
   }
   else if (!_tcsicmp(_T("ICMP.ResponseTime.Average"), name))
   {
      rc = getIcmpStatistic(nullptr, IcmpStatFunction::AVERAGE, buffer);
   }
   else if (MatchString(_T("ICMP.ResponseTime.Average(*)"), name, false))
   {
      rc = getIcmpStatistic(name, IcmpStatFunction::AVERAGE, buffer);
   }
   else if (!_tcsicmp(_T("ICMP.ResponseTime.Last"), name))
   {
      rc = getIcmpStatistic(nullptr, IcmpStatFunction::LAST, buffer);
   }
   else if (MatchString(_T("ICMP.ResponseTime.Last(*)"), name, false))
   {
      rc = getIcmpStatistic(name, IcmpStatFunction::LAST, buffer);
   }
   else if (!_tcsicmp(_T("ICMP.ResponseTime.Max"), name))
   {
      rc = getIcmpStatistic(nullptr, IcmpStatFunction::MAX, buffer);
   }
   else if (MatchString(_T("ICMP.ResponseTime.Max(*)"), name, false))
   {
      rc = getIcmpStatistic(name, IcmpStatFunction::MAX, buffer);
   }
   else if (!_tcsicmp(_T("ICMP.ResponseTime.Min"), name))
   {
      rc = getIcmpStatistic(nullptr, IcmpStatFunction::MIN, buffer);
   }
   else if (MatchString(_T("ICMP.ResponseTime.Min(*)"), name, false))
   {
      rc = getIcmpStatistic(name, IcmpStatFunction::MIN, buffer);
   }
   else if (MatchString(_T("Net.IP.NextHop(*)"), name, false))
   {
      if ((m_capabilities & NC_IS_NATIVE_AGENT) || (m_capabilities & NC_IS_SNMP))
      {
         TCHAR arg[256] = _T("");
         AgentGetParameterArg(name, 1, arg, 256);
         InetAddress destAddr = InetAddress::parse(arg);
         if (destAddr.isValidUnicast())
         {
            bool isVpn;
            uint32_t ifIndex;
            InetAddress nextHop;
            InetAddress route;
            TCHAR name[MAX_OBJECT_NAME];
            if (getNextHop(m_ipAddress, destAddr, &nextHop, &route, &ifIndex, &isVpn, name))
            {
               nextHop.toString(buffer);
            }
            else
            {
               _tcscpy(buffer, _T("UNREACHABLE"));
            }
         }
         else
         {
            rc = DCE_NOT_SUPPORTED;
         }
      }
      else
      {
         rc = DCE_NOT_SUPPORTED;
      }
   }
   else if (MatchString(_T("NetSvc.ResponseTime(*)"), name, false))
   {
      shared_ptr<NetObj> object = objectFromParameter(name);
      if ((object != nullptr) && (object->getObjectClass() == OBJECT_NETWORKSERVICE))
      {
         IntegerToString(static_cast<NetworkService*>(object.get())->getResponseTime(), buffer);
      }
      else
      {
         rc = DCE_NOT_SUPPORTED;
      }
   }
   else if (!_tcsicmp(name, _T("ReceivedSNMPTraps")))
   {
      lockProperties();
      IntegerToString(m_snmpTrapCount, buffer);
      unlockProperties();
   }
   else if (!_tcsicmp(name, _T("ReceivedSyslogMessages")))
   {
      lockProperties();
      IntegerToString(m_syslogMessageCount, buffer);
      unlockProperties();
   }
   else if (!_tcsicmp(name, _T("ZoneProxy.Assignments")))
   {
      shared_ptr<Zone> zone = FindZoneByProxyId(m_id);
      if (zone != nullptr)
      {
         IntegerToString(zone->getProxyNodeAssignments(m_id), buffer);
      }
      else
      {
         rc = DCE_NOT_SUPPORTED;
      }
   }
   else if (!_tcsicmp(name, _T("ZoneProxy.State")))
   {
      shared_ptr<Zone> zone = FindZoneByProxyId(m_id);
      if (zone != nullptr)
      {
         buffer[0] = zone->isProxyNodeAvailable(m_id) ? _T('1') : _T('0');
         buffer[1] = 0;
      }
      else
      {
         rc = DCE_NOT_SUPPORTED;
      }
   }
   else if (!_tcsicmp(name, _T("ZoneProxy.ZoneUIN")))
   {
      shared_ptr<Zone> zone = FindZoneByProxyId(m_id);
      if (zone != nullptr)
      {
         IntegerToString(zone->getUIN(), buffer);
      }
      else
      {
         rc = DCE_NOT_SUPPORTED;
      }
   }
   else if (!_tcsicmp(name, _T("WirelessController.APCount.Down")))
   {
      if (m_capabilities & NC_IS_WIFI_CONTROLLER)
      {
         IntegerToString(getAccessPointCount(AP_DOWN), buffer);
      }
      else
      {
         rc = DCE_NOT_SUPPORTED;
      }
   }
   else if (!_tcsicmp(name, _T("WirelessController.APCount.Operational")))
   {
      if (m_capabilities & NC_IS_WIFI_CONTROLLER)
      {
         IntegerToString(getAccessPointCount(AP_UP), buffer);
      }
      else
      {
         rc = DCE_NOT_SUPPORTED;
      }
   }
   else if (!_tcsicmp(name, _T("WirelessController.APCount.Total")))
   {
      if (m_capabilities & NC_IS_WIFI_CONTROLLER)
      {
         IntegerToString(getAccessPointCount(), buffer);
      }
      else
      {
         rc = DCE_NOT_SUPPORTED;
      }
   }
   else if (!_tcsicmp(name, _T("WirelessController.APCount.Unprovisioned")))
   {
      if (m_capabilities & NC_IS_WIFI_CONTROLLER)
      {
         IntegerToString(getAccessPointCount(AP_UNPROVISIONED), buffer);
      }
      else
      {
         rc = DCE_NOT_SUPPORTED;
      }
   }
   else if (m_capabilities & NC_IS_LOCAL_MGMT)
   {
      if (!_tcsicmp(name, _T("Server.ActiveAlarms")))
      {
         ret_uint(buffer, GetAlarmCount());
      }
      else if (!_tcsicmp(name, _T("Server.ActiveNetworkDiscovery.CurrentRange")))
      {
         ret_string(buffer, GetCurrentActiveDiscoveryRange());
      }
      else if (!_tcsicmp(name, _T("Server.ActiveNetworkDiscovery.IsRunning")))
      {
         ret_boolean(buffer, IsActiveDiscoveryRunning());
      }
      else if (!_tcsicmp(name, _T("Server.AgentTunnels.Bound.AgentProxy")))
      {
         ret_int(buffer, GetTunnelCount(TunnelCapabilityFilter::AGENT_PROXY, true));
      }
      else if (!_tcsicmp(name, _T("Server.AgentTunnels.Bound.SnmpProxy")))
      {
         ret_int(buffer, GetTunnelCount(TunnelCapabilityFilter::SNMP_PROXY, true));
      }
      else if (!_tcsicmp(name, _T("Server.AgentTunnels.Bound.SnmpTrapProxy")))
      {
         ret_int(buffer, GetTunnelCount(TunnelCapabilityFilter::SNMP_TRAP_PROXY, true));
      }
      else if (!_tcsicmp(name, _T("Server.AgentTunnels.Bound.SyslogProxy")))
      {
         ret_int(buffer, GetTunnelCount(TunnelCapabilityFilter::SYSLOG_PROXY, true));
      }
      else if (!_tcsicmp(name, _T("Server.AgentTunnels.Bound.Total")))
      {
         ret_int(buffer, GetTunnelCount(TunnelCapabilityFilter::ANY, true));
      }
      else if (!_tcsicmp(name, _T("Server.AgentTunnels.Bound.UserAgent")))
      {
         ret_int(buffer, GetTunnelCount(TunnelCapabilityFilter::USER_AGENT, true));
      }
      else if (!_tcsicmp(name, _T("Server.AgentTunnels.Unbound.Total")))
      {
         ret_int(buffer, GetTunnelCount(TunnelCapabilityFilter::ANY, false));
      }
      else if (!_tcsicmp(name, _T("Server.AverageDCIQueuingTime")))
      {
         _sntprintf(buffer, size, _T("%u"), g_averageDCIQueuingTime);
      }
      else if (!_tcsicmp(name, _T("Server.Certificate.ExpirationDate")))
      {
         ret_string(buffer, GetServerCertificateExpirationDate());
      }
      else if (!_tcsicmp(name, _T("Server.Certificate.ExpirationTime")))
      {
         ret_int64(buffer, GetServerCertificateExpirationTime());
      }
      else if (!_tcsicmp(name, _T("Server.Certificate.ExpiresIn")))
      {
         ret_int(buffer, GetServerCertificateDaysUntilExpiration());
      }
      else if (!_tcsicmp(name, _T("Server.ClientSessions.Authenticated")))
      {
         _sntprintf(buffer, size, _T("%d"), GetSessionCount(true, false, -1, nullptr));
      }
      else if (MatchString(_T("Server.ClientSessions.Authenticated(*)"), name, false))
      {
         wchar_t loginName[256];
         AgentGetParameterArg(name, 1, loginName, 256);
         IntegerToString(GetSessionCount(true, false, -1, loginName), buffer);
      }
      else if (!_tcsicmp(name, _T("Server.ClientSessions.Desktop")))
      {
         IntegerToString(GetSessionCount(true, true, CLIENT_TYPE_DESKTOP, nullptr), buffer);
      }
      else if (MatchString(_T("Server.ClientSessions.Desktop(*)"), name, false))
      {
         wchar_t loginName[256];
         AgentGetParameterArg(name, 1, loginName, 256);
         IntegerToString(GetSessionCount(true, false, CLIENT_TYPE_DESKTOP, loginName), buffer);
      }
      else if (!_tcsicmp(name, _T("Server.ClientSessions.Mobile")))
      {
         IntegerToString(GetSessionCount(true, true, CLIENT_TYPE_MOBILE, nullptr), buffer);
      }
      else if (MatchString(_T("Server.ClientSessions.Mobile(*)"), name, false))
      {
         wchar_t loginName[256];
         AgentGetParameterArg(name, 1, loginName, 256);
         IntegerToString(GetSessionCount(true, false, CLIENT_TYPE_MOBILE, loginName), buffer);
      }
      else if (!_tcsicmp(name, _T("Server.ClientSessions.Total")))
      {
         IntegerToString(GetSessionCount(true, true, -1, nullptr), buffer);
      }
      else if (!_tcsicmp(name, _T("Server.ClientSessions.Web")))
      {
         IntegerToString(GetSessionCount(true, true, CLIENT_TYPE_WEB, nullptr), buffer);
      }
      else if (MatchString(_T("Server.ClientSessions.Web(*)"), name, false))
      {
         TCHAR loginName[256];
         AgentGetParameterArg(name, 1, loginName, 256);
         IntegerToString(GetSessionCount(true, false, CLIENT_TYPE_WEB, loginName), buffer);
      }
      else if (!_tcsicmp(name, _T("Server.DataCollectionItems")))
      {
         int dciCount = 0;
         g_idxObjectById.forEach([&dciCount](NetObj *object) -> EnumerationCallbackResult
            {
               if (object->isDataCollectionTarget())
                  dciCount += static_cast<DataCollectionTarget*>(object)->getItemCount();
               return _CONTINUE;
            });
         ret_int(buffer, dciCount);
      }
      else if (!_tcsicmp(name, _T("Server.DB.Queries.Failed")))
      {
         LIBNXDB_PERF_COUNTERS counters;
         DBGetPerfCounters(&counters);
         IntegerToString(counters.failedQueries, buffer);
      }
      else if (!_tcsicmp(name, _T("Server.DB.Queries.LongRunning")))
      {
         LIBNXDB_PERF_COUNTERS counters;
         DBGetPerfCounters(&counters);
         IntegerToString(counters.longRunningQueries, buffer);
      }
      else if (!_tcsicmp(name, _T("Server.DB.Queries.NonSelect")))
      {
         LIBNXDB_PERF_COUNTERS counters;
         DBGetPerfCounters(&counters);
         IntegerToString(counters.nonSelectQueries, buffer);
      }
      else if (!_tcsicmp(name, _T("Server.DB.Queries.Select")))
      {
         LIBNXDB_PERF_COUNTERS counters;
         DBGetPerfCounters(&counters);
         IntegerToString(counters.selectQueries, buffer);
      }
      else if (!_tcsicmp(name, _T("Server.DB.Queries.Total")))
      {
         LIBNXDB_PERF_COUNTERS counters;
         DBGetPerfCounters(&counters);
         IntegerToString(counters.totalQueries, buffer);
      }
      else if (!_tcsicmp(name, _T("Server.DBWriter.Requests.IData")))
      {
         IntegerToString(g_idataWriteRequests, buffer);
      }
      else if (!_tcsicmp(name, _T("Server.DBWriter.Requests.Other")))
      {
         IntegerToString(g_otherWriteRequests, buffer);
      }
      else if (!_tcsicmp(name, _T("Server.DBWriter.Requests.RawData")))
      {
         IntegerToString(g_rawDataWriteRequests, buffer);
      }
      else if (MatchString(_T("Server.EventProcessor.AverageWaitTime(*)"), name, false))
      {
         rc = GetEventProcessorStatistic(name, 'W', buffer);
      }
      else if (MatchString(_T("Server.EventProcessor.Bindings(*)"), name, false))
      {
         rc = GetEventProcessorStatistic(name, 'B', buffer);
      }
      else if (MatchString(_T("Server.EventProcessor.ProcessedEvents(*)"), name, false))
      {
         rc = GetEventProcessorStatistic(name, 'P', buffer);
      }
      else if (MatchString(_T("Server.EventProcessor.QueueSize(*)"), name, false))
      {
         rc = GetEventProcessorStatistic(name, 'Q', buffer);
      }
      else if (!_tcsicmp(name, _T("Server.FileHandleLimit")))
      {
#ifdef _WIN32
         rc = DCE_NOT_SUPPORTED;
#else
         struct rlimit rl;
         if (getrlimit(RLIMIT_NOFILE, &rl) == 0)
            IntegerToString(static_cast<uint64_t>(rl.rlim_cur), buffer);
         else
            rc = DCE_COLLECTION_ERROR;
#endif
      }
      else if (!_tcsicmp(name, _T("Server.Heap.Active")))
      {
         int64_t bytes = GetActiveHeapMemory();
         if (bytes != -1)
            IntegerToString(bytes, buffer);
         else
            rc = DCE_NOT_SUPPORTED;
      }
      else if (!_tcsicmp(name, _T("Server.Heap.Allocated")))
      {
         int64_t bytes = GetAllocatedHeapMemory();
         if (bytes != -1)
            IntegerToString(bytes, buffer);
         else
            rc = DCE_NOT_SUPPORTED;
      }
      else if (!_tcsicmp(name, _T("Server.Heap.Mapped")))
      {
         int64_t bytes = GetMappedHeapMemory();
         if (bytes != -1)
            IntegerToString(bytes, buffer);
         else
            rc = DCE_NOT_SUPPORTED;
      }
      else if (!_tcsicmp(name, _T("Server.MemoryUsage.Alarms")))
      {
         ret_uint64(buffer, GetAlarmMemoryUsage());
      }
      else if (!_tcsicmp(name, _T("Server.MemoryUsage.DataCollectionCache")))
      {
         ret_uint64(buffer, GetDCICacheMemoryUsage());
      }
      else if (!_tcsicmp(name, _T("Server.MemoryUsage.RawDataWriter")))
      {
         ret_uint64(buffer, GetRawDataWriterMemoryUsage());
      }
      else if (MatchString(_T("Server.NotificationChannel.HealthCheckStatus(*)"), name, false))
      {
         rc = GetNotificationChannelStatus(name, [buffer](const NotificationChannelStatus& status) { ret_boolean(buffer, status.healthCheckStatus); });
      }
      else if (MatchString(_T("Server.NotificationChannel.LastMessageTimestamp(*)"), name, false))
      {
         rc = GetNotificationChannelStatus(name, [buffer](const NotificationChannelStatus& status) { ret_uint64(buffer, status.lastMessageTime); });
      }
      else if (MatchString(_T("Server.NotificationChannel.MessageCount(*)"), name, false))
      {
         rc = GetNotificationChannelStatus(name, [buffer](const NotificationChannelStatus& status) { ret_uint(buffer, status.messageCount); });
      }
      else if (MatchString(_T("Server.NotificationChannel.QueueSize(*)"), name, false))
      {
         rc = GetNotificationChannelStatus(name, [buffer](const NotificationChannelStatus& status) { ret_uint(buffer, status.queueSize); });
      }
      else if (MatchString(_T("Server.NotificationChannel.SendFailureCount(*)"), name, false))
      {
         rc = GetNotificationChannelStatus(name, [buffer](const NotificationChannelStatus& status) { ret_uint(buffer, status.failedSendCount); });
      }
      else if (MatchString(_T("Server.NotificationChannel.SendStatus(*)"), name, false))
      {
         rc = GetNotificationChannelStatus(name, [buffer](const NotificationChannelStatus& status) { ret_boolean(buffer, status.sendStatus); });
      }
      else if (!_tcsicmp(name, _T("Server.ObjectCount.AccessPoints")))
      {
         ret_uint(buffer, static_cast<uint32_t>(g_idxAccessPointById.size()));
      }
      else if (!_tcsicmp(name, _T("Server.ObjectCount.Clusters")))
      {
         ret_uint(buffer, static_cast<uint32_t>(g_idxClusterById.size()));
      }
      else if (!_tcsicmp(name, _T("Server.ObjectCount.Interfaces")))
      {
         uint32_t count = 0;
         g_idxObjectById.forEach(
            [&count] (NetObj *object) -> EnumerationCallbackResult
            {
               if (object->getObjectClass() == OBJECT_INTERFACE)
                  count++;
               return _CONTINUE;
            });
         ret_uint(buffer, count);
      }
      else if (!_tcsicmp(name, _T("Server.ObjectCount.Nodes")))
      {
         ret_uint(buffer, static_cast<uint32_t>(g_idxNodeById.size()));
      }
      else if (!_tcsicmp(name, _T("Server.ObjectCount.Sensors")))
      {
         ret_uint(buffer, static_cast<uint32_t>(g_idxSensorById.size()));
      }
      else if (!_tcsicmp(name, _T("Server.ObjectCount.Total")))
      {
         ret_uint(buffer, static_cast<uint32_t>(g_idxObjectById.size()));
      }
      else if (MatchString(_T("Server.PDS.DriverStat(*)"), name, false))
      {
         wchar_t driver[64], metric[64];
         AgentGetParameterArg(name, 1, driver, 64);
         AgentGetParameterArg(name, 2, metric, 64);
         rc = GetPerfDataStorageDriverMetric(driver, metric, buffer);
      }
      else if (!_tcsicmp(name, _T("Server.Pollers.Autobind")))
      {
         ret_int(buffer, GetPollerCount(PollerType::AUTOBIND));
      }
      else if (!_tcsicmp(name, _T("Server.Pollers.Configuration")))
      {
         ret_int(buffer, GetPollerCount(PollerType::CONFIGURATION));
      }
      else if (!_tcsicmp(name, _T("Server.Pollers.Discovery")))
      {
         ret_int(buffer, GetPollerCount(PollerType::DISCOVERY));
      }
      else if (!_tcsicmp(name, _T("Server.Pollers.ICMP")))
      {
         ret_int(buffer, GetPollerCount(PollerType::ICMP));
      }
      else if (!_tcsicmp(name, _T("Server.Pollers.InstanceDiscovery")))
      {
         ret_int(buffer, GetPollerCount(PollerType::INSTANCE_DISCOVERY));
      }
      else if (!_tcsicmp(name, _T("Server.Pollers.MapUpdate")))
      {
         ret_int(buffer, GetPollerCount(PollerType::MAP_UPDATE));
      }
      else if (!_tcsicmp(name, _T("Server.Pollers.RoutingTable")))
      {
         ret_int(buffer, GetPollerCount(PollerType::ROUTING_TABLE));
      }
      else if (!_tcsicmp(name, _T("Server.Pollers.Status")))
      {
         ret_int(buffer, GetPollerCount(PollerType::STATUS));
      }
      else if (!_tcsicmp(name, _T("Server.Pollers.Topology")))
      {
         ret_int(buffer, GetPollerCount(PollerType::TOPOLOGY));
      }
      else if (!_tcsicmp(name, _T("Server.Pollers.Total")))
      {
         ret_int(buffer, GetTotalPollerCount());
      }
      else if (MatchString(_T("Server.QueueSize.Average(*)"), name, false))
      {
         rc = GetQueueStatistics(name, StatisticType::AVERAGE, buffer);
      }
      else if (MatchString(_T("Server.QueueSize.Current(*)"), name, false))
      {
         rc = GetQueueStatistics(name, StatisticType::CURRENT, buffer);
      }
      else if (MatchString(_T("Server.QueueSize.Max(*)"), name, false))
      {
         rc = GetQueueStatistics(name, StatisticType::MAX, buffer);
      }
      else if (MatchString(_T("Server.QueueSize.Min(*)"), name, false))
      {
         rc = GetQueueStatistics(name, StatisticType::MIN, buffer);
      }
      else if (!_tcsicmp(name, _T("Server.ReceivedSNMPTraps")))
      {
         ret_uint64(buffer, g_snmpTrapsReceived);
      }
      else if (!_tcsicmp(name, _T("Server.ReceivedSyslogMessages")))
      {
         ret_uint64(buffer, g_syslogMessagesReceived);
      }
      else if (!_tcsicmp(name, _T("Server.ReceivedWindowsEvents")))
      {
         ret_uint64(buffer, g_windowsEventsReceived);
      }
      else if (!_tcsicmp(_T("Server.SyncerRunTime.Average"), name))
      {
         ret_int64(buffer, GetSyncerRunTime(StatisticType::AVERAGE));
      }
      else if (!_tcsicmp(_T("Server.SyncerRunTime.Last"), name))
      {
         ret_int64(buffer, GetSyncerRunTime(StatisticType::CURRENT));
      }
      else if (!_tcsicmp(_T("Server.SyncerRunTime.Max"), name))
      {
         ret_int64(buffer, GetSyncerRunTime(StatisticType::MAX));
      }
      else if (!_tcsicmp(_T("Server.SyncerRunTime.Min"), name))
      {
         ret_int64(buffer, GetSyncerRunTime(StatisticType::MIN));
      }
      else if (MatchString(_T("Server.ThreadPool.ActiveRequests(*)"), name, false))
      {
         rc = GetThreadPoolStat(THREAD_POOL_ACTIVE_REQUESTS, name, buffer);
      }
      else if (MatchString(_T("Server.ThreadPool.AverageWaitTime(*)"), name, false))
      {
         rc = GetThreadPoolStat(THREAD_POOL_AVERAGE_WAIT_TIME, name, buffer);
      }
      else if (MatchString(_T("Server.ThreadPool.CurrSize(*)"), name, false))
      {
         rc = GetThreadPoolStat(THREAD_POOL_CURR_SIZE, name, buffer);
      }
      else if (MatchString(_T("Server.ThreadPool.Load(*)"), name, false))
      {
         rc = GetThreadPoolStat(THREAD_POOL_LOAD, name, buffer);
      }
      else if (MatchString(_T("Server.ThreadPool.LoadAverage(*)"), name, false))
      {
         rc = GetThreadPoolStat(THREAD_POOL_LOADAVG_1, name, buffer);
      }
      else if (MatchString(_T("Server.ThreadPool.LoadAverage5(*)"), name, false))
      {
         rc = GetThreadPoolStat(THREAD_POOL_LOADAVG_5, name, buffer);
      }
      else if (MatchString(_T("Server.ThreadPool.LoadAverage15(*)"), name, false))
      {
         rc = GetThreadPoolStat(THREAD_POOL_LOADAVG_15, name, buffer);
      }
      else if (MatchString(_T("Server.ThreadPool.MaxSize(*)"), name, false))
      {
         rc = GetThreadPoolStat(THREAD_POOL_MAX_SIZE, name, buffer);
      }
      else if (MatchString(_T("Server.ThreadPool.MinSize(*)"), name, false))
      {
         rc = GetThreadPoolStat(THREAD_POOL_MIN_SIZE, name, buffer);
      }
      else if (MatchString(_T("Server.ThreadPool.ScheduledRequests(*)"), name, false))
      {
         rc = GetThreadPoolStat(THREAD_POOL_SCHEDULED_REQUESTS, name, buffer);
      }
      else if (MatchString(_T("Server.ThreadPool.Usage(*)"), name, false))
      {
         rc = GetThreadPoolStat(THREAD_POOL_USAGE, name, buffer);
      }
      else if (!_tcsicmp(name, _T("Server.TotalEventsProcessed")))
      {
         ret_uint64(buffer, g_totalEventsProcessed);
      }
      else if (!_tcsicmp(name, _T("Server.Uptime")))
      {
         ret_int64(buffer, static_cast<int64_t>(time(nullptr) - g_serverStartTime));
      }
      else
      {
         rc = DCE_NOT_SUPPORTED;
      }
   }
   else
   {
      rc = DCE_NOT_SUPPORTED;
   }

   return rc;
}

/**
 * Get metric value for client
 */
uint32_t Node::getMetricForClient(int origin, uint32_t userId, const TCHAR *name, TCHAR *buffer, size_t size)
{
   DataCollectionError rc = DCE_ACCESS_DENIED;
   switch(origin)
   {
      case DS_NATIVE_AGENT:
         if (checkAccessRights(userId, OBJECT_ACCESS_READ_AGENT))
            rc = getMetricFromAgent(name, buffer, size);
         break;
      case DS_SNMP_AGENT:
         if (checkAccessRights(userId, OBJECT_ACCESS_READ_SNMP))
            rc = getMetricFromSNMP(0, SNMP_VERSION_DEFAULT, name, buffer, size, SNMP_RAWTYPE_NONE);
         break;
      case DS_DEVICE_DRIVER:
         if (checkAccessRights(userId, OBJECT_ACCESS_READ_SNMP))
            rc = getMetricFromDeviceDriver(name, buffer, size);
         break;
      case DS_SMCLP:
         if (checkAccessRights(userId, OBJECT_ACCESS_READ_AGENT))
            rc = getMetricFromSmclp(name, buffer, size);
         break;
      default:
         return super::getMetricForClient(origin, userId, name, buffer, size);
   }
   return RCCFromDCIError(rc);
}

/**
 * Get table for client
 */
uint32_t Node::getTableForClient(int origin, uint32_t userId, const TCHAR *name, shared_ptr<Table> *table)
{
   DataCollectionError rc = DCE_ACCESS_DENIED;
   switch(origin)
   {
      case DS_NATIVE_AGENT:
         // Allow access to agent table Agent.SessionAgents if user has access rights for taking screenshots
         // Data from this table required by client to determine correct UI session name
         if (checkAccessRights(userId, OBJECT_ACCESS_READ_AGENT) ||
             (!_tcsicmp(name, _T("Agent.SessionAgents")) && checkAccessRights(userId, OBJECT_ACCESS_SCREENSHOT)))
            rc = getTableFromAgent(name, table);
         break;
      default:
         return super::getTableForClient(origin, userId, name, table);
   }
   return RCCFromDCIError(rc);
}

/**
 * Create NXCP message with essential object's data
 */
void Node::fillMessageLockedEssential(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageLockedEssential(msg, userId);

   msg->setField(VID_STATE_FLAGS, m_state);
   msg->setField(VID_CAPABILITIES, m_capabilities);
   msg->setField(VID_EXPECTED_CAPABILITIES, m_expectedCapabilities);
   msg->setField(VID_IP_ADDRESS, m_ipAddress);
   msg->setField(VID_PRODUCT_NAME, m_productName);
   msg->setField(VID_VENDOR, m_vendor);
   if (m_capabilities & NC_IS_ETHERNET_IP)
   {
      msg->setField(VID_CIP_DEVICE_TYPE_NAME, CIP_DeviceTypeNameFromCode(m_cipDeviceType));
   }

   if (m_radioInterfaces != nullptr)
   {
      msg->setField(VID_RADIO_COUNT, static_cast<uint16_t>(m_radioInterfaces->size()));
      uint32_t fieldId = VID_RADIO_LIST_BASE;
      for(int i = 0; i < m_radioInterfaces->size(); i++)
      {
         RadioInterfaceInfo *rif = m_radioInterfaces->get(i);
         msg->setField(fieldId++, rif->index);
         msg->setField(fieldId++, rif->name);
         msg->setField(fieldId++, rif->bssid, MAC_ADDR_LENGTH);
         msg->setField(fieldId++, rif->frequency);
         msg->setField(fieldId++, static_cast<int16_t>(rif->band));
         msg->setField(fieldId++, rif->channel);
         msg->setField(fieldId++, rif->powerDBm);
         msg->setField(fieldId++, rif->powerMW);
         msg->setField(fieldId++, rif->ssid);
         fieldId++;
      }
   }
   else
   {
      msg->setField(VID_RADIO_COUNT, static_cast<uint16_t>(0));
   }
}

/**
 * Create NXCP message with object's data
 */
void Node::fillMessageLocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageLocked(msg, userId);
   msg->setField(VID_PRIMARY_NAME, m_primaryHostName);
   msg->setField(VID_NODE_TYPE, (INT16)m_type);
   msg->setField(VID_NODE_SUBTYPE, m_subType);
   msg->setField(VID_HYPERVISOR_TYPE, m_hypervisorType);
   msg->setField(VID_HYPERVISOR_INFO, m_hypervisorInfo);
   msg->setField(VID_PRODUCT_CODE, m_productCode);
   msg->setField(VID_PRODUCT_VERSION, m_productVersion);
   msg->setField(VID_SERIAL_NUMBER, m_serialNumber);
   msg->setField(VID_AGENT_PORT, m_agentPort);
   msg->setField(VID_AGENT_CACHE_MODE, m_agentCacheMode);
   msg->setField(VID_SHARED_SECRET, m_agentSecret);
   msg->setFieldFromMBString(VID_SNMP_AUTH_OBJECT, m_snmpSecurity->getAuthName());
   msg->setFieldFromMBString(VID_SNMP_AUTH_PASSWORD, m_snmpSecurity->getAuthPassword());
   msg->setFieldFromMBString(VID_SNMP_PRIV_PASSWORD, m_snmpSecurity->getPrivPassword());
   msg->setField(VID_SNMP_USM_METHODS, (WORD)((WORD)m_snmpSecurity->getAuthMethod() | ((WORD)m_snmpSecurity->getPrivMethod() << 8)));
   msg->setField(VID_SNMP_OID, m_snmpObjectId.toString());  // FIXME: send in binary form
   msg->setField(VID_SNMP_PORT, m_snmpPort);
   msg->setField(VID_SNMP_VERSION, (WORD)m_snmpVersion);
   msg->setField(VID_AGENT_VERSION, m_agentVersion);
   msg->setField(VID_AGENT_ID, m_agentId);
   msg->setField(VID_HARDWARE_ID, m_hardwareId.value(), HARDWARE_ID_LENGTH);
   msg->setField(VID_PLATFORM_NAME, m_platformName);
   msg->setField(VID_POLLER_NODE_ID, m_pollerNode);
   msg->setField(VID_ZONE_UIN, m_zoneUIN);
   msg->setField(VID_AGENT_PROXY, m_agentProxy);
   msg->setField(VID_SNMP_PROXY, m_snmpProxy);
   msg->setField(VID_MQTT_PROXY, m_mqttProxy);
   msg->setField(VID_MODBUS_PROXY, m_modbusProxy);
   msg->setField(VID_ETHERNET_IP_PROXY, m_eipProxy);
   msg->setField(VID_ICMP_PROXY, m_icmpProxy);
   msg->setField(VID_REQUIRED_POLLS, (WORD)m_requiredPollCount);
   msg->setField(VID_SYS_NAME, CHECK_NULL_EX(m_sysName));
   msg->setField(VID_SYS_DESCRIPTION, CHECK_NULL_EX(m_sysDescription));
   msg->setField(VID_SYS_CONTACT, CHECK_NULL_EX(m_sysContact));
   msg->setField(VID_SYS_LOCATION, CHECK_NULL_EX(m_sysLocation));
   msg->setFieldFromTime(VID_BOOT_TIME, m_bootTime);
   msg->setFieldFromTime(VID_AGENT_COMM_TIME, m_lastAgentCommTime);
   msg->setField(VID_BRIDGE_BASE_ADDRESS, m_baseBridgeAddress, 6);
   if (m_lldpNodeId != nullptr)
      msg->setField(VID_LLDP_NODE_ID, m_lldpNodeId);
   msg->setField(VID_USE_IFXTABLE, (WORD)m_nUseIfXTable);
   if (m_vrrpInfo != nullptr)
   {
      msg->setField(VID_VRRP_VERSION, (WORD)m_vrrpInfo->getVersion());
      msg->setField(VID_VRRP_VR_COUNT, (WORD)m_vrrpInfo->size());
   }
   msg->setField(VID_DRIVER_NAME, m_driver->getName());
   msg->setField(VID_DRIVER_VERSION, m_driver->getVersion());
   msg->setField(VID_PHYSICAL_CONTAINER_ID, m_physicalContainer);
   msg->setField(VID_RACK_IMAGE_FRONT, m_rackImageFront);
   msg->setField(VID_RACK_IMAGE_REAR, m_rackImageRear);
   msg->setField(VID_RACK_POSITION, m_rackPosition);
   msg->setField(VID_RACK_HEIGHT, m_rackHeight);
   msg->setField(VID_SSH_PROXY, m_sshProxy);
   msg->setField(VID_SSH_LOGIN, m_sshLogin);
   msg->setField(VID_SSH_KEY_ID, m_sshKeyId);
   msg->setField(VID_SSH_PASSWORD, m_sshPassword);
   msg->setField(VID_SSH_PORT, m_sshPort);
   msg->setField(VID_VNC_PROXY, m_vncProxy);
   msg->setField(VID_VNC_PASSWORD, m_vncPassword);
   msg->setField(VID_VNC_PORT, m_vncPort);
   msg->setField(VID_PORT_ROW_COUNT, m_portRowCount);
   msg->setField(VID_PORT_NUMBERING_SCHEME, m_portNumberingScheme);
   msg->setField(VID_AGENT_COMPRESSION_MODE, m_agentCompressionMode);
   msg->setField(VID_RACK_ORIENTATION, static_cast<INT16>(m_rackOrientation));
   msg->setField(VID_ICMP_COLLECTION_MODE, (INT16)m_icmpStatCollectionMode);
   msg->setField(VID_CHASSIS_PLACEMENT, m_chassisPlacementConf);
   if (isIcmpStatCollectionEnabled() && (m_icmpStatCollectors != nullptr))
   {
      IcmpStatCollector *collector = m_icmpStatCollectors->get(_T("PRI"));
      if (collector != nullptr)
      {
         msg->setField(VID_ICMP_LAST_RESPONSE_TIME, collector->last());
         msg->setField(VID_ICMP_MIN_RESPONSE_TIME, collector->min());
         msg->setField(VID_ICMP_MAX_RESPONSE_TIME, collector->max());
         msg->setField(VID_ICMP_AVG_RESPONSE_TIME, collector->average());
         msg->setField(VID_ICMP_PACKET_LOSS, collector->packetLoss());
      }
      msg->setField(VID_HAS_ICMP_DATA, true);
   }
   else
   {
      msg->setField(VID_HAS_ICMP_DATA, false);
   }

   msg->setField(VID_ICMP_TARGET_COUNT, m_icmpTargets.size());
   uint32_t fieldId = VID_ICMP_TARGET_LIST_BASE;
   for(int i = 0; i < m_icmpTargets.size(); i++)
      msg->setField(fieldId++, m_icmpTargets.get(i));

   msg->setField(VID_ETHERNET_IP_PORT, m_eipPort);
   if (m_capabilities & NC_IS_ETHERNET_IP)
   {
      msg->setField(VID_CIP_DEVICE_TYPE, m_cipDeviceType);
      msg->setField(VID_CIP_STATUS, m_cipStatus);
      msg->setField(VID_CIP_STATUS_TEXT, CIP_DecodeDeviceStatus(m_cipStatus));
      msg->setField(VID_CIP_EXT_STATUS_TEXT, CIP_DecodeExtendedDeviceStatus(m_cipStatus));
      msg->setField(VID_CIP_STATE, static_cast<uint16_t>(m_cipState));
      msg->setField(VID_CIP_STATE_TEXT, CIP_DeviceStateTextFromCode(m_cipState));
      msg->setField(VID_CIP_VENDOR_CODE, m_cipVendorCode);
   }

   msg->setField(VID_MODBUS_TCP_PORT, m_modbusTcpPort);
   msg->setField(VID_MODBUS_UNIT_ID, m_modbusUnitId);

   msg->setField(VID_LAST_BACKUP_JOB_STATUS, static_cast<int16_t>(m_lastConfigBackupJobStatus));

   msg->setField(VID_CERT_MAPPING_METHOD, static_cast<int16_t>(m_agentCertMappingMethod));
   msg->setField(VID_CERT_MAPPING_DATA, m_agentCertMappingData);
   msg->setField(VID_AGENT_CERT_SUBJECT, m_agentCertSubject);
   msg->setFieldFromUtf8String(VID_SYSLOG_CODEPAGE, m_syslogCodepage);
   msg->setFieldFromUtf8String(VID_SNMP_CODEPAGE, m_snmpCodepage);
   msg->setField(VID_OSPF_ROUTER_ID, InetAddress(m_ospfRouterId));

   msg->setField(VID_PATH_CHECK_REASON, static_cast<int16_t>(m_pathCheckResult.reason));
   msg->setField(VID_PATH_CHECK_NODE_ID, m_pathCheckResult.rootCauseNodeId);
   msg->setField(VID_PATH_CHECK_INTERFACE_ID, m_pathCheckResult.rootCauseInterfaceId);
}

/**
 * Fill NXCP message with object's data - property lock is not held
 */
void Node::fillMessageUnlocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageUnlocked(msg, userId);
   msg->setField(VID_MAC_ADDR, getPrimaryMacAddress());

   int networkServiceCount = 0;
   int vpnCount = 0;
   readLockChildList();
   for (int i = 0; i < getChildList().size(); i++)
   {
      NetObj *iface = getChildList().get(i);
      if (iface->getObjectClass() == OBJECT_NETWORKSERVICE)
         networkServiceCount++;
      if (iface->getObjectClass() == OBJECT_VPNCONNECTOR)
         vpnCount++;
   }
   unlockChildList();
   msg->setField(VID_NETWORK_SERVICE_COUNT, networkServiceCount);
   msg->setField(VID_VPN_CONNECTOR_COUNT, vpnCount);
}

/**
 * Update flags
 */
void Node::updateFlags(uint32_t flags, uint32_t mask)
{
   bool wasRemoteAgent = ((m_flags & NF_EXTERNAL_GATEWAY) != 0);
   super::updateFlags(flags, mask);

   if (wasRemoteAgent && !(m_flags & NF_EXTERNAL_GATEWAY) && m_ipAddress.isValidUnicast())
   {
      if (IsZoningEnabled())
      {
         shared_ptr<Zone> zone = FindZoneByUIN(m_zoneUIN);
         if (zone != nullptr)
         {
            zone->addToIndex(m_ipAddress, self());
         }
         else
         {
            nxlog_write(NXLOG_WARNING, _T("Cannot find zone object with UIN %u for node object %s [%u]"), m_zoneUIN, m_name, m_id);
         }
      }
      else
      {
         g_idxNodeByAddr.put(m_ipAddress, self());
      }
   }
   else if (!wasRemoteAgent && (m_flags & NF_EXTERNAL_GATEWAY) && m_ipAddress.isValidUnicast())
   {
      if (IsZoningEnabled())
      {
         shared_ptr<Zone> zone = FindZoneByUIN(m_zoneUIN);
         if (zone != nullptr)
         {
            zone->removeFromNodeIndex(m_ipAddress);
         }
         else
         {
            nxlog_write(NXLOG_WARNING, _T("Cannot find zone object with UIN %u for node object %s [%u]"), m_zoneUIN, m_name, m_id);
         }
      }
      else
      {
         g_idxNodeByAddr.remove(m_ipAddress);
      }
   }
}

/**
 * Modify object from NXCP message
 */
uint32_t Node::modifyFromMessageInternal(const NXCPMessage& msg, ClientSession *session)
{
   // Change primary IP address
   if (msg.isFieldExist(VID_IP_ADDRESS))
   {
      InetAddress ipAddr = msg.getFieldAsInetAddress(VID_IP_ADDRESS);
      if (!(m_flags & NF_EXTERNAL_GATEWAY))
      {
         // Check if received IP address is one of node's interface addresses
         unlockProperties(); //removing possible deadlock
         readLockChildList();
         int i, count = getChildList().size();
         for (i = 0; i < count; i++)
         {
            NetObj *curr = getChildList().get(i);
            if ((curr->getObjectClass() == OBJECT_INTERFACE) &&
                static_cast<Interface*>(curr)->getIpAddressList()->hasAddress(ipAddr))
               break;
         }
         unlockChildList();
         lockProperties();
         if (i == count)
         {
            return RCC_INVALID_IP_ADDR;
         }

         // Check that there is no node with same IP as we try to change
         if ((FindNodeByIP(m_zoneUIN, ipAddr) != nullptr) || (FindSubnetByIP(m_zoneUIN, ipAddr) != nullptr))
         {
            return RCC_IP_ADDRESS_CONFLICT;
         }
      }
      setPrimaryIPAddress(ipAddr); //properties locking mutex already locked in NetObj::modifyFromMessage

      // Update primary name if it is not set with the same message
      if (!msg.isFieldExist(VID_PRIMARY_NAME))
      {
         m_primaryHostName = m_ipAddress.toString();
      }

      agentLock();
      deleteAgentConnection();
      agentUnlock();
   }

   // Change primary host name
   if (msg.isFieldExist(VID_PRIMARY_NAME))
   {
      TCHAR primaryName[MAX_DNS_NAME];
      msg.getFieldAsString(VID_PRIMARY_NAME, primaryName, MAX_DNS_NAME);

      if (_tcscmp(m_primaryHostName, primaryName))
      {
         InetAddress ipAddr = ResolveHostName(m_zoneUIN, primaryName);
         if (ipAddr.isValid() && !(m_flags & NF_EXTERNAL_GATEWAY))
         {
            // Check if received IP address is one of node's interface addresses
            readLockChildList();
            int i, count = getChildList().size();
            for(i = 0; i < count; i++)
            {
               NetObj *curr = getChildList().get(i);
               if ((curr->getObjectClass() == OBJECT_INTERFACE) &&
                   static_cast<Interface*>(curr)->getIpAddressList()->hasAddress(ipAddr))
                  break;
            }
            unlockChildList();
            if (i == count)
            {
               // Check that there is no node with same IP as we try to change
               if ((FindNodeByIP(m_zoneUIN, ipAddr) != nullptr) || (FindSubnetByIP(m_zoneUIN, ipAddr) != nullptr))
               {
                  return RCC_IP_ADDRESS_CONFLICT;
               }
            }
         }

         m_primaryHostName = primaryName;
         m_runtimeFlags |= ODF_FORCE_CONFIGURATION_POLL | NDF_RECHECK_CAPABILITIES;
      }
   }

   // Poller node ID
   if (msg.isFieldExist(VID_POLLER_NODE_ID))
   {
      uint32_t nodeId = msg.getFieldAsUInt32(VID_POLLER_NODE_ID);
      if (nodeId != 0)
      {
         shared_ptr<NetObj> pObject = FindObjectById(nodeId);

         // Check if received id is a valid node id
         if (pObject == nullptr)
         {
            return RCC_INVALID_OBJECT_ID;
         }
         if (pObject->getObjectClass() != OBJECT_NODE)
         {
            return RCC_INVALID_OBJECT_ID;
         }
      }
      m_pollerNode = nodeId;
   }

   // Change expected capabilities
   if (msg.isFieldExist(VID_EXPECTED_CAPABILITIES))
      m_expectedCapabilities = msg.getFieldAsUInt64(VID_EXPECTED_CAPABILITIES);

   // Change listen port of native agent
   if (msg.isFieldExist(VID_AGENT_PORT))
      m_agentPort = msg.getFieldAsUInt16(VID_AGENT_PORT);

   // Change cache mode for agent DCI
   if (msg.isFieldExist(VID_AGENT_CACHE_MODE))
      m_agentCacheMode = msg.getFieldAsInt16(VID_AGENT_CACHE_MODE);

   // Change shared secret of native agent
   if (msg.isFieldExist(VID_SHARED_SECRET))
      msg.getFieldAsString(VID_SHARED_SECRET, m_agentSecret, MAX_SECRET_LENGTH);

   // Change SNMP protocol version
   if (msg.isFieldExist(VID_SNMP_VERSION))
   {
      m_snmpVersion = static_cast<SNMP_Version>(msg.getFieldAsUInt16(VID_SNMP_VERSION));
      m_snmpSecurity->setSecurityModel((m_snmpVersion == SNMP_VERSION_3) ? SNMP_SECURITY_MODEL_USM : SNMP_SECURITY_MODEL_V2C);
   }

   // Change SNMP port
   if (msg.isFieldExist(VID_SNMP_PORT))
      m_snmpPort = msg.getFieldAsUInt16(VID_SNMP_PORT);

   // Change SNMP authentication data
   if (msg.isFieldExist(VID_SNMP_AUTH_OBJECT))
   {
      char mbBuffer[256];

      msg.getFieldAsMBString(VID_SNMP_AUTH_OBJECT, mbBuffer, 256);
      if (m_snmpSecurity->getSecurityModel() == SNMP_SECURITY_MODEL_USM)
         m_snmpSecurity->setUserName(mbBuffer);
      else
         m_snmpSecurity->setCommunity(mbBuffer);

      msg.getFieldAsMBString(VID_SNMP_AUTH_PASSWORD, mbBuffer, 256);
      m_snmpSecurity->setAuthPassword(mbBuffer);

      msg.getFieldAsMBString(VID_SNMP_PRIV_PASSWORD, mbBuffer, 256);
      m_snmpSecurity->setPrivPassword(mbBuffer);

      uint16_t methods = msg.getFieldAsUInt16(VID_SNMP_USM_METHODS);
      m_snmpSecurity->setAuthMethod(static_cast<SNMP_AuthMethod>(methods & 0xFF));
      m_snmpSecurity->setPrivMethod(static_cast<SNMP_EncryptionMethod>(methods >> 8));

      if (m_snmpVersion == SNMP_VERSION_3)
         m_snmpSecurity->recalculateKeys();
   }

   // Change EtherNet/IP port
   if (msg.isFieldExist(VID_ETHERNET_IP_PORT))
      m_eipPort = msg.getFieldAsUInt16(VID_ETHERNET_IP_PORT);

   // Change proxy node
   if (msg.isFieldExist(VID_AGENT_PROXY))
      m_agentProxy = msg.getFieldAsUInt32(VID_AGENT_PROXY);

   // Change SNMP proxy node
   if (msg.isFieldExist(VID_SNMP_PROXY))
   {
      uint32_t oldProxy = m_snmpProxy;
      m_snmpProxy = msg.getFieldAsUInt32(VID_SNMP_PROXY);
      if (oldProxy != m_snmpProxy)
      {
         ThreadPoolExecute(g_mainThreadPool, this, &Node::onSnmpProxyChange, oldProxy);
      }
   }

   // Change ICMP proxy node
   if (msg.isFieldExist(VID_ICMP_PROXY))
      m_icmpProxy = msg.getFieldAsUInt32(VID_ICMP_PROXY);

   // Change EtherNet/IP proxy node
   if (msg.isFieldExist(VID_ETHERNET_IP_PROXY))
      m_eipProxy = msg.getFieldAsUInt32(VID_ETHERNET_IP_PROXY);

   // Change MQTT proxy node
   if (msg.isFieldExist(VID_MQTT_PROXY))
      m_mqttProxy = msg.getFieldAsUInt32(VID_MQTT_PROXY);

   // Change MODBUS proxy node
   if (msg.isFieldExist(VID_MODBUS_PROXY))
      m_modbusProxy = msg.getFieldAsUInt32(VID_MODBUS_PROXY);

   // Change MODBUS-TCP port
   if (msg.isFieldExist(VID_MODBUS_TCP_PORT))
      m_modbusTcpPort = msg.getFieldAsUInt16(VID_MODBUS_TCP_PORT);

   // Change MODBUS unit ID
   if (msg.isFieldExist(VID_MODBUS_UNIT_ID))
      m_modbusUnitId = msg.getFieldAsUInt16(VID_MODBUS_UNIT_ID);

   // Number of required polls
   if (msg.isFieldExist(VID_REQUIRED_POLLS))
      m_requiredPollCount = (int)msg.getFieldAsUInt16(VID_REQUIRED_POLLS);

   // Enable/disable usage of ifXTable
   if (msg.isFieldExist(VID_USE_IFXTABLE))
      m_nUseIfXTable = (BYTE)msg.getFieldAsUInt16(VID_USE_IFXTABLE);

   // Physical container settings
   if (msg.isFieldExist(VID_PHYSICAL_CONTAINER_ID))
   {
      m_physicalContainer = msg.getFieldAsUInt32(VID_PHYSICAL_CONTAINER_ID);
      ThreadPoolExecute(g_mainThreadPool, this, &Node::updatePhysicalContainerBinding, m_physicalContainer);
   }
   if (msg.isFieldExist(VID_RACK_IMAGE_FRONT))
      m_rackImageFront = msg.getFieldAsGUID(VID_RACK_IMAGE_FRONT);
   if (msg.isFieldExist(VID_RACK_IMAGE_REAR))
      m_rackImageRear = msg.getFieldAsGUID(VID_RACK_IMAGE_REAR);
   if (msg.isFieldExist(VID_RACK_POSITION))
      m_rackPosition = msg.getFieldAsInt16(VID_RACK_POSITION);
   if (msg.isFieldExist(VID_RACK_HEIGHT))
      m_rackHeight = msg.getFieldAsInt16(VID_RACK_HEIGHT);
   if (msg.isFieldExist(VID_CHASSIS_PLACEMENT))
      msg.getFieldAsString(VID_CHASSIS_PLACEMENT, &m_chassisPlacementConf);

   if (msg.isFieldExist(VID_SSH_PROXY))
      m_sshProxy = msg.getFieldAsUInt32(VID_SSH_PROXY);

   if (msg.isFieldExist(VID_SSH_LOGIN))
      m_sshLogin = msg.getFieldAsSharedString(VID_SSH_LOGIN);

   if (msg.isFieldExist(VID_SSH_PASSWORD))
      m_sshPassword = msg.getFieldAsSharedString(VID_SSH_PASSWORD);

   if (msg.isFieldExist(VID_SSH_KEY_ID))
      m_sshKeyId = msg.getFieldAsUInt32(VID_SSH_KEY_ID);

   if (msg.isFieldExist(VID_SSH_PORT))
      m_sshPort = msg.getFieldAsUInt16(VID_SSH_PORT);

   if (msg.isFieldExist(VID_VNC_PROXY))
      m_vncProxy = msg.getFieldAsUInt32(VID_VNC_PROXY);

   if (msg.isFieldExist(VID_VNC_PASSWORD))
      m_vncPassword = msg.getFieldAsSharedString(VID_VNC_PASSWORD);

   if (msg.isFieldExist(VID_VNC_PORT))
      m_vncPort = msg.getFieldAsUInt16(VID_VNC_PORT);

   if (msg.isFieldExist(VID_AGENT_COMPRESSION_MODE))
      m_agentCompressionMode = msg.getFieldAsInt16(VID_AGENT_COMPRESSION_MODE);

   if (msg.isFieldExist(VID_RACK_ORIENTATION))
      m_rackOrientation = static_cast<RackOrientation>(msg.getFieldAsUInt16(VID_RACK_ORIENTATION));

   if (msg.isFieldExist(VID_ICMP_COLLECTION_MODE))
   {
      switch(msg.getFieldAsInt16(VID_ICMP_COLLECTION_MODE))
      {
         case 1:
            m_icmpStatCollectionMode = IcmpStatCollectionMode::ON;
            break;
         case 2:
            m_icmpStatCollectionMode = IcmpStatCollectionMode::OFF;
            break;
         default:
            m_icmpStatCollectionMode = IcmpStatCollectionMode::DEFAULT;
            break;
      }
      if (isIcmpStatCollectionEnabled())
      {
         if (m_icmpStatCollectors == nullptr)
            m_icmpStatCollectors = new StringObjectMap<IcmpStatCollector>(Ownership::True);
         if (!m_icmpStatCollectors->contains(_T("PRI")))
            m_icmpStatCollectors->set(_T("PRI"), new IcmpStatCollector(ConfigReadInt(_T("ICMP.StatisticPeriod"), 60)));
      }
      else
      {
         delete_and_null(m_icmpStatCollectors);
      }
   }

   if (msg.isFieldExist(VID_ICMP_TARGET_COUNT))
   {
      m_icmpTargets.clear();
      int count = msg.getFieldAsInt32(VID_ICMP_TARGET_COUNT);
      uint32_t fieldId = VID_ICMP_TARGET_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         InetAddress addr = msg.getFieldAsInetAddress(fieldId++);
         if (addr.isValidUnicast() && !m_icmpTargets.hasAddress(addr))
            m_icmpTargets.add(addr);
      }
   }

   if (msg.isFieldExist(VID_CERT_MAPPING_METHOD))
   {
      m_agentCertMappingMethod = static_cast<CertificateMappingMethod>(msg.getFieldAsInt16(VID_CERT_MAPPING_METHOD));
      TCHAR *oldMappingData = m_agentCertMappingData;
      m_agentCertMappingData = msg.getFieldAsString(VID_CERT_MAPPING_DATA);
      if (m_agentCertMappingData != nullptr)
      {
         Trim(m_agentCertMappingData);
         if (*m_agentCertMappingData == 0)
            MemFreeAndNull(m_agentCertMappingData);
      }
      UpdateAgentCertificateMappingIndex(self(), oldMappingData, m_agentCertMappingData);
      MemFree(oldMappingData);
   }

   if (msg.isFieldExist(VID_SYSLOG_CODEPAGE))
   {
      msg.getFieldAsUtf8String(VID_SYSLOG_CODEPAGE, m_syslogCodepage, 16);
   }

   if (msg.isFieldExist(VID_SNMP_CODEPAGE))
   {
      msg.getFieldAsUtf8String(VID_SNMP_CODEPAGE, m_snmpCodepage, 16);
   }

   return super::modifyFromMessageInternal(msg, session);
}

/**
 * Thread pool callback executed when SNMP proxy changes
 */
void Node::onSnmpProxyChange(uint32_t oldProxy)
{
   // resync data collection configuration with new proxy
   shared_ptr<NetObj> node = FindObjectById(m_snmpProxy, OBJECT_NODE);
   if (node != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_DC_SNMP, 4, _T("Node::onSnmpProxyChange(%s [%d]): data collection sync needed for %s [%u]"), m_name, m_id, node->getName(), node->getId());
      static_cast<Node*>(node.get())->scheduleDataCollectionSyncWithAgent();
   }

   // resync data collection configuration with old proxy
   node = FindObjectById(oldProxy, OBJECT_NODE);
   if (node != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_DC_SNMP, 4, _T("Node::onSnmpProxyChange(%s [%d]): data collection sync needed for %s [%u]"), m_name, m_id, node->getName(), node->getId());
      static_cast<Node*>(node.get())->scheduleDataCollectionSyncWithAgent();
   }
}

/**
 * Wakeup node using magic packet
 */
uint32_t Node::wakeUp()
{
   uint32_t rcc = RCC_NO_WOL_INTERFACES;

   readLockChildList();

   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *object = getChildList().get(i);
      if ((object->getObjectClass() == OBJECT_INTERFACE) &&
          (object->getStatus() != STATUS_UNMANAGED) &&
          static_cast<Interface*>(object)->getIpAddressList()->getFirstUnicastAddressV4().isValid())
      {
         rcc = static_cast<Interface*>(object)->wakeUp();
         if (rcc == RCC_SUCCESS)
            break;
      }
   }

   // If no interface found try to find interface in unmanaged state
   if (rcc != RCC_SUCCESS)
   {
      for(int i = 0; i < getChildList().size(); i++)
      {
         NetObj *object = getChildList().get(i);
         if ((object->getObjectClass() == OBJECT_INTERFACE) &&
             (object->getStatus() == STATUS_UNMANAGED) &&
             static_cast<Interface*>(object)->getIpAddressList()->getFirstUnicastAddressV4().isValid())
         {
            rcc = static_cast<Interface*>(object)->wakeUp();
            if (rcc == RCC_SUCCESS)
               break;
         }
      }
   }

   unlockChildList();
   return rcc;
}

/**
 * Get status of interface with given index from native agent
 */
void Node::getInterfaceStateFromAgent(uint32_t index, InterfaceAdminState *adminState, InterfaceOperState *operState, uint64_t *speed)
{
   TCHAR metric[128], buffer[32];

   // Get administrative status
   _sntprintf(metric, 128, _T("Net.Interface.AdminStatus(%u)"), index);
   if (getMetricFromAgent(metric, buffer, 32) == DCE_SUCCESS)
   {
      *adminState = static_cast<InterfaceAdminState>(_tcstol(buffer, nullptr, 10));

      switch(*adminState)
      {
         case IF_ADMIN_STATE_TESTING:
            *operState = IF_OPER_STATE_UNKNOWN;
            break;
         case IF_ADMIN_STATE_DOWN:
         case 0:     // Agents before 0.2.1 may return 0 instead of 2
            *operState = IF_OPER_STATE_DOWN;
            break;
         case IF_ADMIN_STATE_UP:     // Interface administratively up, check link state
            _sntprintf(metric, 128, _T("Net.Interface.Link(%u)"), index);
            if (getMetricFromAgent(metric, buffer, 32) == DCE_SUCCESS)
            {
               uint32_t linkState = _tcstoul(buffer, nullptr, 10);
               if (linkState)
               {
                  *operState = IF_OPER_STATE_UP;
                  _sntprintf(metric, 128, _T("Net.Interface.Speed(%u)"), index);
                  if (getMetricFromAgent(metric, buffer, 32) == DCE_SUCCESS)
                  {
                     *speed = _tcstoull(buffer, nullptr, 10);
                  }
               }
               else
               {
                  *operState = IF_OPER_STATE_DOWN;
               }
            }
            else
            {
               *operState = IF_OPER_STATE_UNKNOWN;
            }
            break;
         default:
            *adminState = IF_ADMIN_STATE_UNKNOWN;
            *operState = IF_OPER_STATE_UNKNOWN;
            break;
      }
   }
   else
   {
      *adminState = IF_ADMIN_STATE_UNKNOWN;
      *operState = IF_OPER_STATE_UNKNOWN;
   }
}

/**
 * Add SM-CLP metrics supported by this node to provided set
 */
void Node::getSmclpMetrics(StringSet *metrics) const
{
   lockProperties();
   if (m_smclpMetrics != nullptr)
   {
      metrics->addAll(m_smclpMetrics);
      nxlog_debug_tag(_T("dc.smclp"), 6, _T("Node::getSmclpProperties(%s): added %d metrics"), m_name, m_smclpMetrics->size());
   }
   unlockProperties();
}

/**
 * Put list of supported parameters into NXCP message
 */
void Node::writeParamListToMessage(NXCPMessage *pMsg, int origin, WORD flags)
{
   lockProperties();

   ObjectArray<AgentParameterDefinition> *parameters = ((origin == DS_NATIVE_AGENT) ? m_agentParameters : ((origin == DS_DEVICE_DRIVER) ? m_driverParameters : nullptr));
   if ((flags & 0x01) && (parameters != nullptr))
   {
      pMsg->setField(VID_NUM_PARAMETERS, (UINT32)parameters->size());

      int i;
      UINT32 dwId;
      for(i = 0, dwId = VID_PARAM_LIST_BASE; i < parameters->size(); i++)
      {
         dwId += parameters->get(i)->fillMessage(pMsg, dwId);
      }
      nxlog_debug_tag(DEBUG_TAG_DC_AGENT, 6, _T("Node[%s]::writeParamListToMessage(): sending %d parameters (origin=%d)"), m_name, parameters->size(), origin);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_DC_AGENT, 6, _T("Node[%s]::writeParamListToMessage(): parameter list is missing (origin=%d)"), m_name, origin);
      pMsg->setField(VID_NUM_PARAMETERS, (UINT32)0);
   }

   ObjectArray<AgentTableDefinition> *tables = ((origin == DS_NATIVE_AGENT) ? m_agentTables : nullptr);
   if ((flags & 0x02) && (tables != nullptr))
   {
      pMsg->setField(VID_NUM_TABLES, (UINT32)tables->size());

      int i;
      UINT32 dwId;
      for(i = 0, dwId = VID_TABLE_LIST_BASE; i < tables->size(); i++)
      {
         dwId += tables->get(i)->fillMessage(pMsg, dwId);
      }
      nxlog_debug_tag(DEBUG_TAG_DC_AGENT, 6, _T("Node[%s]::writeParamListToMessage(): sending %d tables (origin=%d)"), m_name, tables->size(), origin);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_DC_AGENT, 6, _T("Node[%s]::writeParamListToMessage(): table list is missing (origin=%d)"), m_name, origin);
      pMsg->setField(VID_NUM_TABLES, (UINT32)0);
   }

   unlockProperties();
}

/**
 * Put list of supported Windows performance counters into NXCP message
 */
void Node::writeWinPerfObjectsToMessage(NXCPMessage *msg)
{
   bool onDemand = ConfigReadBoolean(_T("Objects.Nodes.ReadWinPerfCountersOnDemand"), true);
   time_t now = time(nullptr);

   lockProperties();

   if (onDemand && ((m_winPerfObjects == nullptr) || (m_winPerfObjectsTimestamp < now - 14400)))
   {
      unlockProperties();

      ObjectArray<WinPerfObject> *perfObjects = nullptr;

      shared_ptr<AgentConnection> conn = getAgentConnection();
      if (conn != nullptr)
      {
         perfObjects = WinPerfObject::getWinPerfObjectsFromNode(this, conn.get());
      }

      lockProperties();
      if (perfObjects != nullptr)
      {
         delete m_winPerfObjects;
         m_winPerfObjects = perfObjects;
      }
      m_winPerfObjectsTimestamp = now;
   }

   if (m_winPerfObjects != nullptr)
   {
      msg->setField(VID_NUM_OBJECTS, static_cast<uint32_t>(m_winPerfObjects->size()));

      uint32_t id = VID_PARAM_LIST_BASE;
      for(int i = 0; i < m_winPerfObjects->size(); i++)
      {
         WinPerfObject *o = m_winPerfObjects->get(i);
         id = o->fillMessage(msg, id);
      }
      nxlog_debug_tag(DEBUG_TAG_DC_AGENT, 6, _T("Node[%s]::writeWinPerfObjectsToMessage(): sending %d objects"), m_name, m_winPerfObjects->size());
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_DC_AGENT, 6, _T("Node[%s]::writeWinPerfObjectsToMessage(): m_winPerfObjects == nullptr"), m_name);
      msg->setField(VID_NUM_OBJECTS, static_cast<uint32_t>(0));
   }

   unlockProperties();
}

/**
 * Open list of supported parameters for reading
 */
ObjectArray<AgentParameterDefinition> *Node::openParamList(int origin)
{
   lockProperties();
   return (origin == DS_NATIVE_AGENT) ? m_agentParameters : ((origin == DS_DEVICE_DRIVER) ? m_driverParameters : nullptr);
}

/**
 * Open list of supported tables for reading
 */
ObjectArray<AgentTableDefinition> *Node::openTableList()
{
   lockProperties();
   return m_agentTables;
}

/**
 * Check status of network service
 */
UINT32 Node::checkNetworkService(UINT32 *pdwStatus, const InetAddress& ipAddr, int iServiceType,
                                WORD wPort, WORD wProto, TCHAR *pszRequest,
                                TCHAR *pszResponse, UINT32 *responseTime)
{
   UINT32 dwError = ERR_NOT_CONNECTED;
   *responseTime = 0;

   if ((m_capabilities & NC_IS_NATIVE_AGENT) &&
       (!(m_state & NSF_AGENT_UNREACHABLE)) &&
       (!(m_state & DCSF_UNREACHABLE)))
   {
      shared_ptr<AgentConnectionEx> conn = createAgentConnection();
      if (conn != nullptr)
      {
         dwError = conn->checkNetworkService(pdwStatus, ipAddr, iServiceType, wPort, wProto, pszRequest, pszResponse, responseTime);
      }
   }
   return dwError;
}

/**
 * Handler for object deletion
 */
void Node::onObjectDelete(const NetObj& object)
{
   lockProperties();
   uint32_t objectId = object.getId();
   if (objectId == m_pollerNode)
   {
      // If deleted object is our poller node, change it to default
      m_pollerNode = 0;
      setModified(MODIFY_NODE_PROPERTIES);
      nxlog_debug_tag(DEBUG_TAG_OBJECT_RELATIONS, 3, _T("Node::onObjectDelete(%s [%u]): poller node %s [%u] deleted"), m_name, m_id, object.getName(), objectId);
   }

   // If deleted object is our proxy, change proxy to default
   if (objectId == m_snmpProxy)
   {
      uint32_t oldProxy = m_snmpProxy;
      m_snmpProxy = 0;
      setModified(MODIFY_NODE_PROPERTIES);
      nxlog_debug_tag(DEBUG_TAG_OBJECT_RELATIONS, 3, _T("Node::onObjectDelete(%s [%u]): SNMP proxy node %s [%u] deleted"), m_name, m_id, object.getName(), objectId);
      ThreadPoolExecute(g_mainThreadPool, this, &Node::onSnmpProxyChange, oldProxy);
   }
   if (objectId == m_agentProxy)
   {
      m_agentProxy = 0;
      setModified(MODIFY_NODE_PROPERTIES);
      nxlog_debug_tag(DEBUG_TAG_OBJECT_RELATIONS, 3, _T("Node::onObjectDelete(%s [%u]): agent proxy node %s [%u] deleted"), m_name, m_id, object.getName(), objectId);
   }
   if (objectId == m_icmpProxy)
   {
      m_icmpProxy = 0;
      setModified(MODIFY_NODE_PROPERTIES);
      nxlog_debug_tag(DEBUG_TAG_OBJECT_RELATIONS, 3, _T("Node::onObjectDelete(%s [%u]): ICMP proxy node %s [%u] deleted"), m_name, m_id, object.getName(), objectId);
   }
   if (objectId == m_eipProxy)
   {
      m_eipProxy = 0;
      setModified(MODIFY_NODE_PROPERTIES);
      nxlog_debug_tag(DEBUG_TAG_OBJECT_RELATIONS, 3, _T("Node::onObjectDelete(%s [%u]): EtherNet/IP proxy node %s [%u] deleted"), m_name, m_id, object.getName(), objectId);
   }
   if (objectId == m_mqttProxy)
   {
      m_mqttProxy = 0;
      setModified(MODIFY_NODE_PROPERTIES);
      nxlog_debug_tag(DEBUG_TAG_OBJECT_RELATIONS, 3, _T("Node::onObjectDelete(%s [%u]): MQTT proxy node %s [%u] deleted"), m_name, m_id, object.getName(), objectId);
   }
   if (objectId == m_modbusProxy)
   {
      m_modbusProxy = 0;
      setModified(MODIFY_NODE_PROPERTIES);
      nxlog_debug_tag(DEBUG_TAG_OBJECT_RELATIONS, 3, _T("Node::onObjectDelete(%s [%u]): MODBUS proxy node %s [%u] deleted"), m_name, m_id, object.getName(), objectId);
   }
   if (objectId == m_sshProxy)
   {
      m_sshProxy = 0;
      setModified(MODIFY_NODE_PROPERTIES);
      nxlog_debug_tag(DEBUG_TAG_OBJECT_RELATIONS, 3, _T("Node::onObjectDelete(%s [%u]): SSH proxy node %s [%u] deleted"), m_name, m_id, object.getName(), objectId);
   }

   // If deleted object is our physical container
   if (objectId == m_physicalContainer)
   {
      m_physicalContainer = 0;
      updatePhysicalContainerBinding(0);
      setModified(MODIFY_NODE_PROPERTIES);
      nxlog_debug_tag(DEBUG_TAG_OBJECT_RELATIONS, 3, _T("Node::onObjectDelete(%s [%u]): physical container %s [%u] deleted"), m_name, m_id, object.getName(), objectId);
   }
   unlockProperties();

   super::onObjectDelete(object);
}

/**
 * Check node for OSPF support
 */
void Node::checkOSPFSupport(SNMP_Transport *snmp)
{
   int32_t adminStatus;
   if (SnmpGet(m_snmpVersion, snmp, { 1, 3, 6, 1, 2, 1, 14, 1, 2, 0 }, &adminStatus, sizeof(int32_t), 0) == SNMP_ERR_SUCCESS)
   {
      lockProperties();
      if (adminStatus)
      {
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("Node::checkOSPFSupport(%s [%u]): OSPF is enabled"), m_name, m_id);
         m_capabilities |= NC_IS_OSPF;
      }
      else
      {
         m_capabilities &= ~NC_IS_OSPF;
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("Node::checkOSPFSupport(%s [%u]): OSPF is disabled"), m_name, m_id);
      }
      unlockProperties();
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("Node::checkOSPFSupport(%s [%u]): OSPF MIB not supported"), m_name, m_id);
   }

   if (m_capabilities & NC_IS_OSPF)
   {
      uint32_t routerId = 0;
      if (SnmpGet(m_snmpVersion, snmp, { 1, 3, 6, 1, 2, 1, 14, 1, 1, 0 }, &routerId, sizeof(int32_t), 0) == SNMP_ERR_SUCCESS)
      {
         lockProperties();
         if (m_ospfRouterId != routerId)
         {
            TCHAR buffer[16];
            nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("Node::checkOSPFSupport(%s [%u]): OSPF router ID changed to %s"), m_name, m_id, IpToStr(routerId, buffer));
            m_ospfRouterId = routerId;
            setModified(MODIFY_NODE_PROPERTIES);
         }
         unlockProperties();
      }
   }
}

/**
 * Write node's OSPF data to NXCP message
 */
void Node::writeOSPFDataToMessage(NXCPMessage *msg)
{
   lockProperties();

   msg->setField(VID_OSPF_ROUTER_ID, m_ospfRouterId);

   uint32_t fieldId = VID_OSPF_AREA_LIST_BASE;
   for(OSPFArea *a : m_ospfAreas)
   {
      msg->setField(fieldId++, a->id);
      msg->setField(fieldId++, a->lsaCount);
      msg->setField(fieldId++, a->areaBorderRouterCount);
      msg->setField(fieldId++, a->asBorderRouterCount);
      fieldId += 6;
   }
   msg->setField(VID_OSPF_AREA_COUNT, m_ospfAreas.size());

   fieldId = VID_OSPF_NEIGHBOR_LIST_BASE;
   for(OSPFNeighbor *n : m_ospfNeighbors)
   {
      msg->setField(fieldId++, n->routerId);
      msg->setField(fieldId++, n->nodeId);
      msg->setField(fieldId++, n->ipAddress);
      msg->setField(fieldId++, n->ifIndex);
      msg->setField(fieldId++, n->ifObject);
      msg->setField(fieldId++, n->areaId);
      msg->setField(fieldId++, n->isVirtual);
      msg->setField(fieldId++, static_cast<uint16_t>(n->state));
      fieldId += 2;
   }
   msg->setField(VID_OSPF_NEIGHBOR_COUNT, m_ospfNeighbors.size());

   unlockProperties();
}

/**
 * Create ready to use agent connection
 */
shared_ptr<AgentConnectionEx> Node::createAgentConnection(bool sendServerId)
{
   if (!(m_capabilities & NC_IS_NATIVE_AGENT) ||
       (m_flags & NF_DISABLE_NXCP) ||
       ((m_state & NSF_AGENT_UNREACHABLE) && (m_pollCountAgent == 0)) ||
       (m_status == STATUS_UNMANAGED) ||
       m_isDeleteInitiated)
   {
      nxlog_debug_tag(DEBUG_TAG_AGENT, 6, _T("Node::createAgentConnection(%s [%u]): connection to agent is not possible (status=%d flags=0x%08x)"), m_name, m_id, m_status, m_flags);
      return shared_ptr<AgentConnectionEx>();
   }

   shared_ptr<AgentConnectionEx> conn;
   shared_ptr<AgentTunnel> tunnel = GetTunnelForNode(m_id);
   if (tunnel != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_AGENT, 6, _T("Node::createAgentConnection(%s [%u]): using agent tunnel"), m_name, m_id);
      conn = make_shared<AgentConnectionEx>(m_id, tunnel, m_agentSecret, isAgentCompressionAllowed());
   }
   else
   {
      InetAddress addr = m_ipAddress;
      if ((m_capabilities & NC_IS_LOCAL_MGMT) && (g_mgmtAgentAddress[0] != 0))
      {
         // If node is local management node and external agent address is set, use it instead of node's primary IP address
         addr = InetAddress::resolveHostName(g_mgmtAgentAddress);
         if (addr.isValid())
         {
            nxlog_debug_tag(DEBUG_TAG_AGENT, 7, _T("Node::createAgentConnection(%s [%u]): using external agent address %s"), m_name, m_id, g_mgmtAgentAddress);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG_AGENT, 7, _T("Node::createAgentConnection(%s [%u]): cannot resolve external agent address %s"), m_name, m_id, g_mgmtAgentAddress);
         }
      }
      if ((!addr.isValidUnicast() && !((m_capabilities & NC_IS_LOCAL_MGMT) && addr.isLoopback())) || (m_flags & NF_AGENT_OVER_TUNNEL_ONLY))
      {
         nxlog_debug_tag(DEBUG_TAG_AGENT, 7, _T("Node::createAgentConnection(%s [%u]): %s and there are no active tunnels"), m_name, m_id,
                  (m_flags & NF_AGENT_OVER_TUNNEL_ONLY) ? _T("direct agent connections are disabled") : _T("node primary IP is invalid"));
         return shared_ptr<AgentConnectionEx>();
      }
      conn = make_shared<AgentConnectionEx>(m_id, addr, m_agentPort, m_agentSecret, isAgentCompressionAllowed());
      if (!setAgentProxy(conn.get()))
      {
         return shared_ptr<AgentConnectionEx>();
      }
   }
   conn->setCommandTimeout(g_agentCommandTimeout);
   uint32_t errorCode, socketErrorCode;
   if (!conn->connect(g_serverKey, &errorCode, &socketErrorCode, sendServerId ? g_serverId : 0))
   {
      wchar_t errorText[256];
      nxlog_debug_tag(DEBUG_TAG_AGENT, 6, _T("Node::createAgentConnection(%s [%u]): connect failed, error = \"%d %s\", socket error = \"%s\""),
         m_name, m_id, errorCode, AgentErrorCodeToText(errorCode), GetSocketErrorText(socketErrorCode, errorText, 256));
      conn.reset();
   }
   else
   {
      setLastAgentCommTime();
   }
   nxlog_debug_tag(DEBUG_TAG_AGENT, 6, _T("Node::createAgentConnection(%s [%u]): conn=%p"), m_name, m_id, conn.get());
   return conn;
}

/**
 * Get agent connection. It may or may not be primary connection.
 * File transfers and other long running oprations should be avoided.
 */
shared_ptr<AgentConnectionEx> Node::getAgentConnection(bool forcePrimary)
{
   if (m_status == STATUS_UNMANAGED)
      return shared_ptr<AgentConnectionEx>();

   shared_ptr<AgentConnectionEx> conn;

   bool success;
   int retryCount = 5;
   while(--retryCount >= 0)
   {
      success = m_agentMutex.tryLock();
      if (success)
      {
         if (connectToAgent())
         {
            conn = m_agentConnection;
         }
         m_agentMutex.unlock();
         break;
      }
      ThreadSleepMs(50);
   }

   if (!success && !forcePrimary)
   {
      // was unable to obtain lock on primary connection
      nxlog_debug_tag(DEBUG_TAG_AGENT, 6, _T("Node::getAgentConnection(%s [%d]): cannot obtain lock on primary connection"), m_name, (int)m_id);
      conn = createAgentConnection(false);
   }

   return conn;
}

/**
 * Acquire SNMP proxy agent connection
 */
shared_ptr<AgentConnectionEx> Node::acquireProxyConnection(ProxyType type, bool validate)
{
   if (!m_proxyConnections[type].timedLock(4000))
   {
      nxlog_debug_tag(DEBUG_TAG_AGENT, 5, _T("Node::acquireProxyConnection(%s [%u] type=%d): cannot acquire lock on proxy connection"), m_name, m_id, (int)type);
      return shared_ptr<AgentConnectionEx>();
   }

   shared_ptr<AgentConnectionEx> conn = m_proxyConnections[type].get();
   if ((conn != nullptr) && !conn->isConnected())
   {
      conn.reset();
      m_proxyConnections[type].set(shared_ptr<AgentConnectionEx>());
      nxlog_debug_tag(DEBUG_TAG_AGENT, 4, _T("Node::acquireProxyConnection(%s [%u] type=%d): existing agent connection dropped"), m_name, m_id, (int)type);
   }

   if ((conn != nullptr) && validate)
   {
      uint32_t rcc = conn->nop();
      if (rcc == ERR_SUCCESS)
      {
         nxlog_debug_tag(DEBUG_TAG_AGENT, 4, _T("Node::acquireProxyConnection(%s [%u] type=%d): existing agent connection passed validation"), m_name, m_id, (int)type);
      }
      else
      {
         conn.reset();
         m_proxyConnections[type].set(shared_ptr<AgentConnectionEx>());
         nxlog_debug_tag(DEBUG_TAG_AGENT, 4, _T("Node::acquireProxyConnection(%s [%u] type=%d): existing agent connection failed validation (RCC=%u) and dropped"),
                  m_name, m_id, (int)type, rcc);
      }
   }

   if ((conn == nullptr) && (time(nullptr) - m_proxyConnections[type].getLastConnectTime() > 60))
   {
      conn = createAgentConnection();
      if (conn != nullptr)
      {
         if (conn->isMasterServer())
         {
            m_proxyConnections[type].set(conn);
            nxlog_debug_tag(DEBUG_TAG_AGENT, 4, _T("Node::acquireProxyConnection(%s [%u] type=%d): new agent connection created"), m_name, m_id, (int)type);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG_AGENT, 5, _T("Node::acquireProxyConnection(%s [%u] type=%d): server does not have master access to agent"), m_name, m_id, (int)type);
            conn.reset();
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_AGENT, 5, _T("Node::acquireProxyConnection(%s [%u] type=%d): cannot create new agent connection"), m_name, m_id, (int)type);
      }
      m_proxyConnections[type].setLastConnectTime(time(nullptr));
   }

   m_proxyConnections[type].unlock();
   return conn;
}

/**
 * Set node's primary IP address.
 * Assumed that all necessary locks already in place
 */
void Node::setPrimaryIPAddress(const InetAddress& addr)
{
   if (addr.equals(m_ipAddress))
      return;

   InetAddress oldIpAddr = m_ipAddress;
   m_ipAddress = addr;
   if (!(m_flags & NF_EXTERNAL_GATEWAY))
   {
      UpdateNodeIndex(oldIpAddr, addr, self());
      if (!(m_capabilities & (NC_IS_NATIVE_AGENT | NC_IS_SNMP | NC_IS_ETHERNET_IP)))
      {
         unlockProperties(); // to avoid deadlock
         readLockChildList();
         for (int i = 0; i < getChildList().size(); i++)
         {
            NetObj *iface = getChildList().get(i);
            if (iface->getObjectClass() == OBJECT_INTERFACE && static_cast<Interface*>(iface)->isFake())
            {
               static_cast<Interface*>(iface)->setIpAddress(m_ipAddress);
            }
         }
         unlockChildList();
         lockProperties();
      }
   }

   setModified(MODIFY_NODE_PROPERTIES);
}

/**
 * Change node's IP address.
 *
 * @param ipAddr new IP address
 */
void Node::changeIPAddress(const InetAddress& ipAddr)
{
   _pollerLock();

   lockProperties();

   // check if primary name is an IP address
   if (InetAddress::resolveHostName(m_primaryHostName).equals(m_ipAddress))
   {
      TCHAR ipAddrText[64];
      m_ipAddress.toString(ipAddrText);
      if (!_tcscmp(ipAddrText, m_primaryHostName))
         m_primaryHostName = ipAddr.toString();

      setPrimaryIPAddress(ipAddr);
      m_runtimeFlags |= ODF_FORCE_CONFIGURATION_POLL | NDF_RECHECK_CAPABILITIES;

      // Change status of node and all it's children to UNKNOWN
      m_status = STATUS_UNKNOWN;
      readLockChildList();
      for(int i = 0; i < getChildList().size(); i++)
      {
         NetObj *object = getChildList().get(i);
         object->resetStatus();
         if (object->getObjectClass() == OBJECT_INTERFACE)
         {
            if (static_cast<Interface*>(object)->isFake())
            {
               static_cast<Interface*>(object)->setIpAddress(ipAddr);
            }
         }
      }
      unlockChildList();

      setModified(MODIFY_NODE_PROPERTIES);
   }
   unlockProperties();

   agentLock();
   deleteAgentConnection();
   agentUnlock();

   _pollerUnlock();
}

/**
 * Change node's zone
 */
void Node::changeZone(UINT32 newZoneUIN)
{
   int i;

   _pollerLock();

   // Unregister from old zone
   shared_ptr<Zone> zone = FindZoneByUIN(m_zoneUIN);
   if (zone != nullptr)
      zone->removeFromIndex(*this);

   lockProperties();
   m_zoneUIN = newZoneUIN;
   m_runtimeFlags |= ODF_FORCE_CONFIGURATION_POLL | NDF_RECHECK_CAPABILITIES;
   unlockProperties();

   // Remove from subnets
   readLockParentList();
   NetObj **subnets = MemAllocArray<NetObj*>(getParentList().size());
   int count = 0;
   for(i = 0; i < getParentList().size(); i++)
   {
      NetObj *curr = getParentList().get(i);
      if (curr->getObjectClass() == OBJECT_SUBNET)
         subnets[count++] = curr;
   }
   unlockParentList();

   for(i = 0; i < count; i++)
   {
      unlinkObjects(subnets[i], this);
   }
   MemFree(subnets);

   // Register in new zone
   zone = FindZoneByUIN(newZoneUIN);
   if (zone != nullptr)
      zone->addToIndex(self());

   // Change zone UIN on interfaces
   readLockChildList();
   for(i = 0; i < getChildList().size(); i++)
   {
      NetObj *curr = getChildList().get(i);
      if (curr->getObjectClass() == OBJECT_INTERFACE)
         static_cast<Interface*>(curr)->updateZoneUIN();
   }
   unlockChildList();

   lockProperties();
   setModified(MODIFY_RELATIONS | MODIFY_NODE_PROPERTIES);
   unlockProperties();

   agentLock();
   deleteAgentConnection();
   agentUnlock();

   _pollerUnlock();
}

/**
 * Set connection for file update messages
 */
bool Node::setFileUpdateConnection(const shared_ptr<AgentConnection>& connection)
{
   bool changed = false;
   lockProperties();
   if (m_fileUpdateConnection == nullptr)
   {
      m_fileUpdateConnection = connection;
      changed = true;
   }
   unlockProperties();

   if (changed)
   {
      connection->enableFileUpdates();
      nxlog_debug_tag(_T("file.monitor"), 6, _T("Set file monitor connection for node %s [%u]"), m_name, m_id);
   }
   return changed;
}

/**
 * Clear connection for file update messages
 */
void Node::clearFileUpdateConnection()
{
   lockProperties();
   m_fileUpdateConnection.reset();
   unlockProperties();
   nxlog_debug_tag(_T("file.monitor"), 6, _T("Cleared file monitor connection for node %s [%d]"), m_name, m_id);
}

/**
 * Get number of interface objects and pointer to the last one
 */
uint32_t Node::getInterfaceCount(Interface **lastInterface)
{
   readLockChildList();
   uint32_t count = 0;
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *curr = getChildList().get(i);
      if (curr->getObjectClass() == OBJECT_INTERFACE)
      {
         count++;
         *lastInterface = static_cast<Interface*>(curr);
      }
   }
   unlockChildList();
   return count;
}

/**
 * Read routing table from node
 */
shared_ptr<RoutingTable> Node::readRoutingTable()
{
   shared_ptr<RoutingTable> routingTable;

   if ((m_capabilities & NC_IS_NATIVE_AGENT) && (!(m_flags & NF_DISABLE_NXCP)))
   {
      shared_ptr<AgentConnectionEx> conn = getAgentConnection();
      if (conn != nullptr)
      {
         size_t limit = getCustomAttributeAsInt32(L"SysConfig:Topology.RoutingTable.MaxSize" , ConfigReadInt(L"Topology.RoutingTable.MaxSize", 4000));
         routingTable = conn->getRoutingTable(limit);
      }
   }
   if ((routingTable == nullptr) && (m_capabilities & NC_IS_SNMP) && (!(m_flags & NF_DISABLE_SNMP)))
   {
      SNMP_Transport *snmp = createSnmpTransport();
      if (snmp != nullptr)
      {
         routingTable = SnmpGetRoutingTable(snmp, *this);
         delete snmp;
      }
   }

   if (routingTable != nullptr)
      routingTable->sort();

   return routingTable;
}

/**
 * Get cached routing table if available, read from node otherwise and updated cached version.
 */
shared_ptr<RoutingTable> Node::getRoutingTable()
{
   routingTableLock();
   auto routingTable = m_routingTable;
   routingTableUnlock();

   if ((routingTable == nullptr) || (routingTable->timestamp() < time(nullptr) - ROUTING_TABLE_CACHE_TIMEOUT))
   {
      routingTable = readRoutingTable();
      if (routingTable != nullptr)
      {
         routingTableLock();
         m_routingTable = routingTable;
         routingTableUnlock();
      }
   }

   return routingTable;
}

/**
 * Get outward interface for routing to given destination address
 */
bool Node::getOutwardInterface(const InetAddress& destAddr, InetAddress *srcAddr, uint32_t *srcIfIndex)
{
   bool found = false;
   routingTableLock();
   if (m_routingTable != nullptr)
   {
      const ROUTE *route = SelectBestRoute(*m_routingTable, destAddr);
      if (route != nullptr)
      {
         *srcIfIndex = route->ifIndex;
         shared_ptr<Interface> iface = findInterfaceByIndex(route->ifIndex);
         if (iface != nullptr)
         {
            *srcAddr = iface->getIpAddressList()->getFirstUnicastAddressV4();
         }
         else
         {
            *srcAddr = m_ipAddress;  // use primary IP if outward interface does not have IP address or cannot be found
         }
         found = true;
      }
   }
   else
   {
      nxlog_debug_tag(_T("topology.ip"), 6, _T("Node::getOutwardInterface(%s [%u]): no routing table"), m_name, m_id);
   }
   routingTableUnlock();
   return found;
}

/**
 * Get next hop for given destination address
 */
bool Node::getNextHop(const InetAddress& srcAddr, const InetAddress& destAddr, InetAddress *nextHop, InetAddress *route, uint32_t *ifIndex, bool *isVpn, TCHAR *name)
{
   bool nextHopFound = false;
   *name = 0;

   // Check directly connected networks and VPN connectors
   bool nonFunctionalInterfaceFound = false;
   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *object = getChildList().get(i);
      if (object->getObjectClass() == OBJECT_VPNCONNECTOR)
      {
         if (static_cast<VPNConnector*>(object)->isRemoteAddr(destAddr) && static_cast<VPNConnector*>(object)->isLocalAddr(srcAddr))
         {
            *nextHop = static_cast<VPNConnector*>(object)->getPeerGatewayAddr();
            *route = InetAddress::INVALID;
            *ifIndex = object->getId();
            *isVpn = true;
            _tcslcpy(name, object->getName(), MAX_OBJECT_NAME);
            nextHopFound = true;
            break;
         }
      }
      else if ((object->getObjectClass() == OBJECT_INTERFACE) &&
               static_cast<Interface*>(object)->getIpAddressList()->findSameSubnetAddress(destAddr).isValid())
      {
         *nextHop = destAddr;
         *route = InetAddress::INVALID;
         *ifIndex = static_cast<Interface*>(object)->getIfIndex();
         *isVpn = false;
         _tcslcpy(name, object->getName(), MAX_OBJECT_NAME);
         if ((static_cast<Interface*>(object)->getAdminState() == IF_ADMIN_STATE_UP) && (static_cast<Interface*>(object)->getOperState() == IF_OPER_STATE_UP))
         {
            // found operational interface
            nextHopFound = true;
            break;
         }
         // non-operational interface found, continue search
         // but will use this interface if other suitable interfaces will not be found
         nonFunctionalInterfaceFound = true;
      }
   }
   unlockChildList();

   // Check routing table
   // If directly connected subnet found, only check host routes
   nextHopFound = nextHopFound || nonFunctionalInterfaceFound;
   routingTableLock();
   if (m_routingTable != nullptr)
   {
      const ROUTE *bestRoute = SelectBestRoute(*m_routingTable, destAddr);
      if ((bestRoute != nullptr) && (!nextHopFound || (bestRoute->destination.getHostBits() == 0)))
      {
         shared_ptr<Interface> iface = findInterfaceByIndex(bestRoute->ifIndex);
         if (bestRoute->nextHop.isAnyLocal() && (iface != nullptr) &&
             (iface->getIpAddressList()->getFirstUnicastAddressV4().getHostBits() == 0))
         {
            // On Linux XEN VMs can be pointed by individual host routes to virtual interfaces
            // where each vif has netmask 255.255.255.255 and next hop in routing table set to 0.0.0.0
            *nextHop = destAddr;
         }
         else
         {
            *nextHop = bestRoute->nextHop;
         }
         *route = bestRoute->destination;
         *ifIndex = bestRoute->ifIndex;
         *isVpn = false;
         if (iface != nullptr)
         {
            _tcslcpy(name, iface->getName(), MAX_OBJECT_NAME);
         }
         else
         {
            _sntprintf(name, MAX_OBJECT_NAME, _T("%d"), bestRoute->ifIndex);
         }
         nextHopFound = true;
      }
   }
   else
   {
      nxlog_debug_tag(_T("topology.ip"), 6, _T("Node::getNextHop(%s [%u]): no routing table"), m_name, m_id);
   }
   routingTableUnlock();

   return nextHopFound;
}

/**
 * Routing table poll
 */
void Node::routingTablePoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
{
   lockProperties();
   if (m_isDeleteInitiated || IsShutdownInProgress())
   {
      m_routingPollState.complete(0);
      unlockProperties();
      return;
   }
   unlockProperties();

   pollerLock(routing);
   getRoutingTable();   // will update m_routingTable if needed
   pollerUnlock();
}

/**
 * Call SNMP Enumerate with node's SNMP parameters
 */
uint32_t Node::callSnmpEnumerate(const TCHAR *rootOid, uint32_t (*handler)(SNMP_Variable *, SNMP_Transport *, void *), void *callerData, const char *context, bool failOnShutdown)
{
   if (!(m_capabilities & NC_IS_SNMP) || (m_state & NSF_SNMP_UNREACHABLE) || (m_state & DCSF_UNREACHABLE))
      return SNMP_ERR_COMM;

   uint32_t rc;
   SNMP_Transport *pTransport = createSnmpTransport(0, SNMP_VERSION_DEFAULT, context);
   if (pTransport != nullptr)
   {
      rc = SnmpWalk(pTransport, rootOid, handler, callerData, false, failOnShutdown);
      delete pTransport;
   }
   else
   {
      rc = SNMP_ERR_COMM;
   }
   return rc;
}

/**
 * Set proxy information for agent's connection
 */
bool Node::setAgentProxy(AgentConnectionEx *conn)
{
   uint32_t proxyNode = m_agentProxy;

   if (IsZoningEnabled() && (proxyNode == 0) && (m_zoneUIN != 0))
   {
      shared_ptr<Zone> zone = FindZoneByUIN(m_zoneUIN);
      if ((zone != nullptr) && !zone->isProxyNode(m_id))
      {
         proxyNode = zone->getAvailableProxyNodeId(this);
      }
   }

   if (proxyNode == 0)
      return true;

   shared_ptr<Node> node = static_pointer_cast<Node>(g_idxNodeById.get(proxyNode));
   if (node == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_AGENT, 4, _T("Node::setAgentProxy(%s [%d]): cannot find proxy node [%d]"), m_name, m_id, proxyNode);
      return false;
   }

   shared_ptr<AgentTunnel> tunnel = GetTunnelForNode(proxyNode);
   if (tunnel != nullptr)
   {
      conn->setProxy(tunnel, node->m_agentSecret);
   }
   else
   {
      conn->setProxy(node->m_ipAddress, node->m_agentPort, node->m_agentSecret);
   }
   return true;
}

/**
 * Prepare node object for deletion
 */
void Node::prepareForDeletion()
{
   // Wait for all pending polls
   nxlog_debug(4, _T("Node::prepareForDeletion(%s [%u]): waiting for outstanding polls to finish"), m_name, m_id);
   while (m_statusPollState.isPending() || m_configurationPollState.isPending() ||
          m_discoveryPollState.isPending() || m_routingPollState.isPending() ||
          m_topologyPollState.isPending() || m_instancePollState.isPending() ||
          m_icmpPollState.isPending())
   {
      ThreadSleepMs(100);
   }
   nxlog_debug_tag(DEBUG_TAG_OBJECT_LIFECYCLE, 4, _T("Node::PrepareForDeletion(%s [%u]): no outstanding polls left"), m_name, m_id);

   UnbindAgentTunnel(m_id, 0);

   // Clear possible references to self and other nodes
   m_lastKnownNetworkPath.reset();

   // Remove interfaces from all circuits
   unique_ptr<SharedObjectArray<NetObj>> interfaces = getChildren(OBJECT_INTERFACE);
   for(int i = 0; i < interfaces->size(); i++)
   {
      NetObj *iface = interfaces->get(i);
      if (iface->getParentCount() > 1)
      {
         unique_ptr<SharedObjectArray<NetObj>> circuits = iface->getParents(OBJECT_CIRCUIT);
         for(int j = 0; j < circuits->size(); j++)
         {
            NetObj *circuit = circuits->get(j);
            NetObj::unlinkObjects(circuit, iface);
            circuit->calculateCompoundStatus();
            nxlog_debug_tag(DEBUG_TAG_OBJECT_LIFECYCLE, 4, _T("Node::prepareForDeletion(%s [%u]): interface \"%s\" removed from circuit \"%s\" [%u]"),
               m_name, m_id, iface->getName(), circuit->getName(), circuit->getId());
         }
      }
   }

   super::prepareForDeletion();
}

/**
 * Cleanup object before server shutdown
 */
void Node::cleanup()
{
   // Clear possible references to self and other nodes
   m_lastKnownNetworkPath.reset();

   super::cleanup();
}

/**
 * Check and update if needed interface names
 */
void Node::checkInterfaceNames(InterfaceList *pIfList)
{
   // Cut interface names to MAX_OBJECT_NAME and check for unnamed interfaces
   for(int i = 0; i < pIfList->size(); i++)
   {
      pIfList->get(i)->name[MAX_OBJECT_NAME - 1] = 0;
      if (pIfList->get(i)->name[0] == 0)
         _sntprintf(pIfList->get(i)->name, MAX_OBJECT_NAME, _T("%u"), pIfList->get(i)->index);
   }
}

/**
 * Get cluster object this node belongs to, if any
 */
shared_ptr<Cluster> Node::getCluster() const
{
   shared_ptr<Cluster> cluster;
   readLockParentList();
   for(int i = 0; i < getParentList().size(); i++)
      if (getParentList().get(i)->getObjectClass() == OBJECT_CLUSTER)
      {
         cluster = static_pointer_cast<Cluster>(getParentList().getShared(i));
         break;
      }
   unlockParentList();
   return cluster;
}

/**
 * Get wireless domain object this node belongs to, if any
 */
shared_ptr<WirelessDomain> Node::getWirelessDomain() const
{
   shared_ptr<WirelessDomain> wd;
   readLockParentList();
   for(int i = 0; i < getParentList().size(); i++)
      if (getParentList().get(i)->getObjectClass() == OBJECT_WIRELESSDOMAIN)
      {
         wd = static_pointer_cast<WirelessDomain>(getParentList().getShared(i));
         break;
      }
   unlockParentList();
   return wd;
}

/**
 * Generic function for calculating effective proxy node ID
 */
static uint32_t GetEffectiveProtocolProxy(Node *node, uint32_t configuredProxy, uint32_t zoneUIN, ProxySelection proxySelection = ProxySelection::AVAILABLE, uint32_t defaultProxy = 0)
{
   uint32_t effectiveProxy = (proxySelection == ProxySelection::BACKUP) ? 0 : configuredProxy;
   if (IsZoningEnabled() && (effectiveProxy == 0) && (zoneUIN != 0))
   {
      // Use zone default proxy if set
      shared_ptr<Zone> zone = FindZoneByUIN(zoneUIN);
      if (zone != nullptr)
      {
         effectiveProxy = zone->isProxyNode(node->getId()) ? node->getId() :
            ((proxySelection == ProxySelection::AVAILABLE) ? zone->getAvailableProxyNodeId(node) :
                zone->getProxyNodeId(node, proxySelection == ProxySelection::BACKUP));
      }
   }
   return (effectiveProxy != 0) ? effectiveProxy : defaultProxy;
}

/**
 * Get effective SNMP proxy for this node
 */
uint32_t Node::getEffectiveSnmpProxy(ProxySelection proxySelection)
{
   return GetEffectiveProtocolProxy(this, m_snmpProxy, m_zoneUIN, proxySelection);
}

/**
 * Get effective EtherNet/IP proxy for this node
 */
uint32_t Node::getEffectiveEtherNetIPProxy(ProxySelection proxySelection)
{
   return GetEffectiveProtocolProxy(this, m_eipProxy, m_zoneUIN, proxySelection);
}

/**
 * Get effective MQTT proxy for this node
 */
uint32_t Node::getEffectiveMqttProxy()
{
   return GetEffectiveProtocolProxy(this, m_mqttProxy, m_zoneUIN, ProxySelection::AVAILABLE, g_dwMgmtNode);
}

/**
 * Get effective MODBUS proxy for this node
 */
uint32_t Node::getEffectiveModbusProxy(ProxySelection proxySelection)
{
   return GetEffectiveProtocolProxy(this, m_modbusProxy, m_zoneUIN, proxySelection);
}

/**
 * Get effective SSH proxy for this node
 */
uint32_t Node::getEffectiveSshProxy()
{
   return GetEffectiveProtocolProxy(this, m_sshProxy, m_zoneUIN, ProxySelection::AVAILABLE, g_dwMgmtNode);
}

/**
 * Get effective VNC proxy for this node
 */
uint32_t Node::getEffectiveVncProxy()
{
   return GetEffectiveProtocolProxy(this, m_vncProxy, m_zoneUIN, ProxySelection::AVAILABLE, g_dwMgmtNode);
}

/**
 * Get effective TCP proxy for this node
 */
uint32_t Node::getEffectiveTcpProxy()
{
   return GetEffectiveProtocolProxy(this, 0, m_zoneUIN, ProxySelection::AVAILABLE, g_dwMgmtNode);
}

/**
 * Get effective ICMP proxy for this node
 */
uint32_t Node::getEffectiveIcmpProxy()
{
   return GetEffectiveProtocolProxy(this, m_icmpProxy, m_zoneUIN);
}

/**
 * Get effective Agent proxy for this node
 */
uint32_t Node::getEffectiveAgentProxy()
{
   uint32_t agentProxy = m_agentProxy;
   if (IsZoningEnabled() && (agentProxy == 0) && (m_zoneUIN != 0))
   {
      // Use zone default proxy if set
      shared_ptr<Zone> zone = FindZoneByUIN(m_zoneUIN);
      if ((zone != nullptr) && !zone->isProxyNode(m_id))
      {
         agentProxy = zone->getAvailableProxyNodeId(this);
      }
   }
   return agentProxy;
}

/**
 * Create SNMP transport
 */
SNMP_Transport *Node::createSnmpTransport(uint16_t port, SNMP_Version version, const char *context, const char *community)
{
   if (community != nullptr)
   {
      // Check community from list
      bool valid = false;
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT count(*) FROM snmp_communities WHERE (zone=? OR zone=-1) AND community=?"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_zoneUIN);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, DB_CTYPE_UTF8_STRING, community, DB_BIND_STATIC);
         DB_RESULT hResult = DBSelectPrepared(hStmt);
         if (hResult != nullptr)
         {
            valid = (DBGetFieldLong(hResult, 0, 0) > 0);
            DBFreeResult(hResult);
         }
         DBFreeStatement(hStmt);
      }
      DBConnectionPoolReleaseConnection(hdb);

      if (!valid)
         return nullptr;
   }

   if ((m_flags & NF_DISABLE_SNMP) || (m_status == STATUS_UNMANAGED) || (g_flags & AF_SHUTDOWN) || m_isDeleteInitiated)
      return nullptr;

   SNMP_Transport *transport = nullptr;
   uint32_t snmpProxy = getEffectiveSnmpProxy();
   if (snmpProxy == 0)
   {
      transport = new SNMP_UDPTransport();
      static_cast<SNMP_UDPTransport*>(transport)->createUDPTransport(m_ipAddress, (port != 0) ? port : m_snmpPort);
   }
   else
   {
      shared_ptr<Node> proxyNode = (snmpProxy == m_id) ? self() : static_pointer_cast<Node>(g_idxNodeById.get(snmpProxy));
      if (proxyNode != nullptr)
      {
         shared_ptr<AgentConnectionEx> conn = proxyNode->acquireProxyConnection(SNMP_PROXY);
         if (conn != nullptr)
         {
            // Use loopback address if node is SNMP proxy for itself
            transport = new SNMP_ProxyTransport(conn, (snmpProxy == m_id) ? InetAddress::LOOPBACK : m_ipAddress, (port != 0) ? port : m_snmpPort);
         }
      }
   }

   // Set security
   if (transport != nullptr)
   {
      lockProperties();
      SNMP_Version effectiveVersion = (version != SNMP_VERSION_DEFAULT) ? version : m_snmpVersion;
      transport->setSnmpVersion(effectiveVersion);
      if (m_snmpCodepage[0] != 0)
      {
         transport->setCodepage(m_snmpCodepage);
      }
      else if (g_snmpCodepage[0] != 0)
      {
         transport->setCodepage(g_snmpCodepage);
      }

      if (context == nullptr)
      {
         if (community == nullptr)
            transport->setSecurityContext(new SNMP_SecurityContext(m_snmpSecurity));
         else
            transport->setSecurityContext(new SNMP_SecurityContext(community));
      }
      else
      {
         if (effectiveVersion < SNMP_VERSION_3)
         {
            char fullCommunity[128];
            if (community == nullptr)
               snprintf(fullCommunity, 128, "%s@%s", m_snmpSecurity->getCommunity(), context);
            else
               snprintf(fullCommunity, 128, "%s@%s", community, context);
            transport->setSecurityContext(new SNMP_SecurityContext(fullCommunity));
         }
         else
         {
            SNMP_SecurityContext *securityContext = new SNMP_SecurityContext(m_snmpSecurity);
            securityContext->setContextName(context);
            transport->setSecurityContext(securityContext);
         }
      }
      unlockProperties();
   }
   return transport;
}

/**
 * Get SNMP security context
 * ATTENTION: This method returns new copy of security context
 * which must be destroyed by the caller
 */
SNMP_SecurityContext *Node::getSnmpSecurityContext() const
{
   lockProperties();
   SNMP_SecurityContext *ctx = new SNMP_SecurityContext(m_snmpSecurity);
   unlockProperties();
   return ctx;
}

/**
 * Resolve node's name
 */
bool Node::resolveName(bool useOnlyDNS, const TCHAR* *const facility)
{
   bool resolved = false;
   bool truncated = false;

   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 4, _T("Resolving name for node %s [%u]"), m_name, m_id);

   TCHAR name[MAX_OBJECT_NAME];
   if (isMyIP(m_ipAddress))
   {
      if (m_zoneUIN != 0)
      {
         shared_ptr<Zone> zone = FindZoneByUIN(m_zoneUIN);
         shared_ptr<AgentConnectionEx> conn = (zone != nullptr) ? zone->acquireConnectionToProxy() : shared_ptr<AgentConnectionEx>();
         if (conn != nullptr)
         {
            resolved = (conn->getHostByAddr(m_ipAddress, name, MAX_OBJECT_NAME) != nullptr);
            *facility = _T("proxy agent");
         }
      }
      else
      {
         resolved = (m_ipAddress.getHostByAddr(name, MAX_OBJECT_NAME) != nullptr);
         *facility = _T("system resolver");
      }

      // Check for systemd synthetic record
      if (resolved && (name[0] == '_'))
      {
         TCHAR buffer[64];
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("IP address %s on node %s [%u] was resolved to \"%s\" which looks like systemd synthetic record and therefore ignored"), m_ipAddress.toString(buffer), m_name, m_id, name);
         resolved = false;
      }
   }
   else
   {
      TCHAR buffer[64];
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("Primary IP address %s of node %s [%u] cannot be found on any of node's interfaces and therefore ignored"), m_ipAddress.toString(buffer), m_name, m_id);
   }

   // Use primary IP if resolved successfully
   if (resolved)
   {
      _tcslcpy(m_name, name, MAX_OBJECT_NAME);
      if (!(g_flags & AF_USE_FQDN_FOR_NODE_NAMES))
      {
         TCHAR *pPoint = _tcschr(m_name, _T('.'));
         if (pPoint != nullptr)
         {
            *pPoint = _T('\0');
            truncated = true;
         }
      }
   }
   else
   {
      // Try to resolve each interface's IP address
      readLockChildList();
      for(int i = 0; i < getChildList().size(); i++)
      {
         NetObj *curr = getChildList().get(i);
         if ((curr->getObjectClass() == OBJECT_INTERFACE) && !static_cast<Interface*>(curr)->isLoopback())
         {
            const InetAddressList *list = static_cast<Interface*>(curr)->getIpAddressList();
            for(int n = 0; n < list->size(); n++)
            {
               const InetAddress& a = list->get(i);
               if (a.isValidUnicast() && (a.getHostByAddr(name, MAX_OBJECT_NAME) != nullptr))
               {
                  _tcslcpy(m_name, name, MAX_OBJECT_NAME);
                  resolved = true;
                  *facility = _T("system resolver");
                  break;
               }
            }
         }
      }
      unlockChildList();

      // Try to get hostname from agent if address resolution fails
      if (!(resolved || useOnlyDNS))
      {
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 4, _T("Resolving name for node %s [%u] via agent"), m_name, m_id);
         TCHAR buffer[256];
         if (getMetricFromAgent(_T("System.Hostname"), buffer, 256) == DCE_SUCCESS)
         {
            Trim(buffer);
            if (buffer[0] != 0)
            {
               _tcslcpy(m_name, buffer, MAX_OBJECT_NAME);
               resolved = true;
               *facility = _T("agent");
            }
         }
      }

      // Try to get hostname from SNMP if other methods fails
      if (!(resolved || useOnlyDNS))
      {
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 4, _T("Resolving name for node %s [%u] via SNMP"), m_name, m_id);
         TCHAR buffer[MAX_RESULT_LENGTH];
         if (getMetricFromSNMP(0, SNMP_VERSION_DEFAULT, _T(".1.3.6.1.2.1.1.5.0"), buffer, MAX_RESULT_LENGTH, SNMP_RAWTYPE_NONE) == DCE_SUCCESS)
         {
            Trim(buffer);
            if (buffer[0] != 0)
            {
               _tcslcpy(m_name, buffer, MAX_OBJECT_NAME);
               resolved = true;
               *facility = _T("SNMP");
            }
         }
      }
   }

   if (resolved)
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 4, _T("Name for node [%u] was resolved to %s%s using %s"), m_id, m_name, truncated ? _T(" (truncated to host part)") : _T(""), *facility);
   else
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 4, _T("Name for node [%u] was not resolved"), m_id);
   return resolved;
}

/**
 * Get current layer 2 topology cache or rebuild it
 * Will return nullptr if there are no topology information
 */
shared_ptr<NetworkMapObjectList> Node::getAndUpdateL2Topology(uint32_t *status, int radius, bool useL1Topology)
{
   shared_ptr<NetworkMapObjectList> result;
   time_t expTime = ConfigReadULong(_T("Topology.AdHocRequest.ExpirationTime"), 900);
   int nDepth = (radius <= 0) ? ConfigReadInt(_T("Topology.DefaultDiscoveryRadius"), 5) : radius;

   m_topologyMutex.lock();
   if ((m_topology != nullptr) && (m_topologyRebuildTimestamp + expTime >= time(nullptr)) && (m_l1TopologyUsed == useL1Topology) && (m_topologyDepth == nDepth))
   {
      result = m_topology;
   }
   m_topologyMutex.unlock();

   if (result == nullptr)
   {
      m_topologyMutex.lock();
      if (m_linkLayerNeighbors != nullptr || !isSNMPSupported() ||
               ((m_capabilities & (NC_IS_CDP | NC_IS_LLDP | NC_IS_NDP | NC_IS_BRIDGE)) == 0)) // Fail only if topology information is not available when it should be
      {
         m_topologyMutex.unlock();

         result = make_shared<NetworkMapObjectList>();
         BuildL2Topology(*result, this, nullptr, nDepth, true, useL1Topology);

         m_topologyMutex.lock();
         m_topology = result;
         m_l1TopologyUsed = useL1Topology;
         m_topologyDepth = nDepth;
         m_topologyRebuildTimestamp = time(nullptr);
      }
      else
      {
         m_topology.reset();
         *status = RCC_NO_L2_TOPOLOGY_SUPPORT;
      }
      m_topologyMutex.unlock();
   }
   return result;
}

/**
 * Rebuild layer 2 topology and return it
 */
shared_ptr<NetworkMapObjectList> Node::buildL2Topology(int radius, bool includeEndNodes, bool useL1Topology, NetworkMap *filterProvider)
{
   m_topologyMutex.lock();
   if ((m_linkLayerNeighbors == nullptr) && isSNMPSupported() &&
            ((m_capabilities & (NC_IS_CDP | NC_IS_LLDP | NC_IS_NDP | NC_IS_BRIDGE)) != 0)) // Fail only if topology information is not available when it should be
   {
      m_topologyMutex.unlock();
      return shared_ptr<NetworkMapObjectList>();

   }
   m_topologyMutex.unlock();

   shared_ptr<NetworkMapObjectList> result = make_shared<NetworkMapObjectList>();
   int nDepth = (radius <= 0) ? ConfigReadInt(_T("Topology.DefaultDiscoveryRadius"), 5) : radius;
   BuildL2Topology(*result, this, filterProvider, nDepth, includeEndNodes, useL1Topology);
   return result;
}

/**
 * Get L2 connections for given second node ID
 * Will return empty list if no connections found
 */
void Node::getL2Connections(uint32_t secondNodeId, ObjectArray<ObjLink> *links)
{
   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *object = getChildList().get(i);
      if (object->getObjectClass() == OBJECT_INTERFACE)
      {
         Interface *iface = static_cast<Interface*>(object);
         if (iface->getPeerNodeId() == secondNodeId)
         {
            ObjLink *link = new ObjLink();
            link->object1 = m_id;
            link->iface1 = iface->getId();
            link->object2 = secondNodeId;
            link->iface2 = iface->getPeerInterfaceId();
            link->type = LINK_TYPE_NORMAL;
            links->add(link);
         }
      }
   }
   unlockChildList();
}

/**
 * Topology poll
 */
void Node::topologyPoll(PollerInfo *poller, ClientSession *pSession, uint32_t rqId)
{
   lockProperties();
   if (m_isDeleteInitiated || IsShutdownInProgress())
   {
      m_topologyPollState.complete(0);
      unlockProperties();
      return;
   }
   unlockProperties();

   poller->setStatus(_T("wait for lock"));
   pollerLock(topology);

   POLL_CANCELLATION_CHECKPOINT();

   m_pollRequestor = pSession;
   m_pollRequestId = rqId;

   if (m_status == STATUS_UNMANAGED)
   {
      sendPollerMsg(POLLER_WARNING _T("Node %s is unmanaged, topology poll aborted\r\n"), m_name);
      nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 5, _T("Node %s [%u] is unmanaged, topology poll aborted"), m_name, m_id);
      pollerUnlock();
      return;
   }

   sendPollerMsg(_T("Starting topology poll of node %s\r\n"), m_name);
   nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 4, _T("Started topology poll of node %s [%d]"), m_name, m_id);

   if (isSNMPSupported())
   {
      poller->setStatus(_T("reading VLANs"));
      SNMP_Transport *snmp = createSnmpTransport();
      if (snmp != nullptr)
      {
         VlanList *vlanList = m_driver->getVlans(snmp, this, m_driverData);
         delete snmp;

         m_topologyMutex.lock();
         if (vlanList != nullptr)
         {
            resolveVlanPorts(vlanList);
            sendPollerMsg(POLLER_INFO _T("VLAN list successfully retrieved from node\r\n"));
            nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 4, _T("VLAN list retrieved from node %s [%d]"), m_name, m_id);
            m_vlans = shared_ptr<VlanList>(vlanList);
         }
         else
         {
            sendPollerMsg(POLLER_WARNING _T("Cannot get VLAN list\r\n"));
            nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 4, _T("Cannot retrieve VLAN list from node %s [%d]"), m_name, m_id);
            m_vlans.reset();
         }
         m_topologyMutex.unlock();

         lockProperties();
         uint64_t oldCaps = m_capabilities;
         if (vlanList != nullptr)
            m_capabilities |= NC_HAS_VLANS;
         else
            m_capabilities &= ~NC_HAS_VLANS;
         if (oldCaps != m_capabilities)
            setModified(MODIFY_NODE_PROPERTIES);
         unlockProperties();
      }
   }

   POLL_CANCELLATION_CHECKPOINT();

   if (isBridge())
   {
      poller->setStatus(_T("reading FDB"));

      shared_ptr<ForwardingDatabase> fdb;

      SNMP_Transport *snmp = createSnmpTransport();
      if (snmp != nullptr)
      {
         StructArray<ForwardingDatabaseEntry> *fdbEntries = m_driver->getForwardingDatabase(snmp, this, m_driverData);
         if (fdbEntries != nullptr)
         {
            StructArray<BridgePort> *bridgePorts = nullptr;
            if (!m_driver->isFdbUsingIfIndex(this, m_driverData))
               bridgePorts = m_driver->getBridgePorts(snmp, this, m_driverData);
            fdb = make_shared<ForwardingDatabase>(m_id, *fdbEntries, bridgePorts);
            delete bridgePorts;
            delete fdbEntries;
         }

         m_topologyMutex.lock();
         m_fdb = fdb;
         m_topologyMutex.unlock();

         if (fdb != nullptr)
         {
            nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 4, _T("Switch forwarding database retrieved for node %s [%u]"), m_name, m_id);
            sendPollerMsg(POLLER_INFO _T("Switch forwarding database retrieved\r\n"));
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 4, _T("Failed to get switch forwarding database from node %s [%u]"), m_name, m_id);
            sendPollerMsg(POLLER_WARNING _T("Failed to get switch forwarding database\r\n"));
         }

         delete snmp;
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 5, _T("Node %s [%u] is not a bridge - skipping FDB retrieval"), m_name, m_id);
   }

   POLL_CANCELLATION_CHECKPOINT();

   poller->setStatus(_T("building neighbor list"));
   shared_ptr<LinkLayerNeighbors> nbs = BuildLinkLayerNeighborList(this);
   if (nbs != nullptr)
   {
      sendPollerMsg(POLLER_INFO _T("Link layer topology retrieved (%d connections found)\r\n"), nbs->size());
      nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 4, _T("Link layer topology retrieved for node %s [%d] (%d connections found)"), m_name, (int)m_id, nbs->size());

      m_topologyMutex.lock();
      m_linkLayerNeighbors = nbs;
      m_topologyMutex.unlock();

      // Walk through interfaces and update peers
      sendPollerMsg(_T("Updating peer information on interfaces\r\n"));
      for(int i = 0; i < nbs->size(); i++)
      {
         LL_NEIGHBOR_INFO *ni = nbs->getConnection(i);
         if (ni->isCached)
            continue;   // ignore cached information

         shared_ptr<NetObj> object = FindObjectById(ni->objectId);
         if (object == nullptr)
            continue;

         if (object->getObjectClass() == OBJECT_NODE)
         {
            nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 5, _T("Node::topologyPoll(%s [%u]): found peer node %s [%u], localIfIndex=%u remoteIfIndex=%u"),
                      m_name, m_id, object->getName(), object->getId(), ni->ifLocal, ni->ifRemote);
            shared_ptr<Interface> ifLocal = findInterfaceByIndex(ni->ifLocal);
            shared_ptr<Interface> ifRemote = static_cast<Node*>(object.get())->findInterfaceByIndex(ni->ifRemote);
            nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 5, _T("Node::topologyPoll(%s [%u]): localIfObject=%s remoteIfObject=%s"), m_name, m_id,
                      (ifLocal != nullptr) ? ifLocal->getName() : _T("(null)"),
                      (ifRemote != nullptr) ? ifRemote->getName() : _T("(null)"));
            if ((ifLocal != nullptr) && (ifRemote != nullptr))
            {
               // Update old peers for local and remote interfaces, if any
               if ((ifRemote->getPeerInterfaceId() != 0) && (ifRemote->getPeerInterfaceId() != ifLocal->getId()))
               {
                  ClearPeer(ifRemote->getPeerInterfaceId());
               }
               if ((ifLocal->getPeerInterfaceId() != 0) && (ifLocal->getPeerInterfaceId() != ifRemote->getId()))
               {
                  ClearPeer(ifLocal->getPeerInterfaceId());
               }

               ifLocal->setPeer(static_cast<Node*>(object.get()), ifRemote.get(), ni->protocol, false);
               ifRemote->setPeer(this, ifLocal.get(), ni->protocol, true);
               sendPollerMsg(_T("   Local interface %s linked to remote interface %s:%s\r\n"),
                             ifLocal->getName(), object->getName(), ifRemote->getName());
               nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 5, _T("Local interface %s:%s linked to remote interface %s:%s"),
                         m_name, ifLocal->getName(), object->getName(), ifRemote->getName());
            }
         }
         else if (object->getObjectClass() == OBJECT_ACCESSPOINT)
         {
            nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 5, _T("Node::topologyPoll(%s [%u]): found peer access point %s [%u], localIfIndex=%u"),
                      m_name, m_id, object->getName(), object->getId(), ni->ifLocal);
            shared_ptr<Interface> ifLocal = findInterfaceByIndex(ni->ifLocal);
            nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 5, _T("Node::topologyPoll(%s [%u]): localIfObject=%s"), m_name, m_id,
                      (ifLocal != nullptr) ? ifLocal->getName() : _T("(null)"));
            if (ifLocal != nullptr)
            {
               // Update old peers for local interface, if any
               if ((static_cast<AccessPoint&>(*object).getPeerInterfaceId() != 0) && (static_cast<AccessPoint&>(*object).getPeerInterfaceId() != ifLocal->getId()))
               {
                  ClearPeer(static_cast<AccessPoint&>(*object).getPeerInterfaceId());
               }
               if ((ifLocal->getPeerInterfaceId() != 0) && (ifLocal->getPeerInterfaceId() != object->getId()))
               {
                  ClearPeer(ifLocal->getPeerInterfaceId());
               }

               ifLocal->setPeer(static_cast<AccessPoint*>(object.get()), ni->protocol);
               static_cast<AccessPoint&>(*object).setPeer(this, ifLocal.get(), ni->protocol);
               sendPollerMsg(_T("   Local interface %s linked to access point %s\r\n"), ifLocal->getName(), object->getName());
               nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 5, _T("Local interface %s:%s linked to access point %s"), m_name, ifLocal->getName(), object->getName());
            }
         }
      }

      uint32_t peerRetentionTime = ConfigReadULong(_T("Objects.Interfaces.PeerRetentionTime"), 30) * 86400; // Default 30 days

      readLockChildList();
      for(int i = 0; i < getChildList().size(); i++)
      {
         if (getChildList().get(i)->getObjectClass() != OBJECT_INTERFACE)
            continue;

         Interface *iface = static_cast<Interface*>(getChildList().get(i));

         // Clear self-linked interfaces caused by bug in previous release
         if ((iface->getPeerNodeId() == m_id) && (iface->getPeerInterfaceId() == iface->getId()))
         {
            iface->clearPeer();
            nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 6, _T("Node::topologyPoll(%s [%u]): Self-linked interface %s [%u] fixed"), m_name, m_id, iface->getName(), iface->getId());
         }
         // Remove outdated peer information
         else if (iface->getPeerNodeId() != 0)
         {
            shared_ptr<NetObj> peerObject = FindObjectById(iface->getPeerNodeId());
            if ((peerObject == nullptr) || ((peerObject->getObjectClass() != OBJECT_NODE) && (peerObject->getObjectClass() != OBJECT_ACCESSPOINT)))
            {
               sendPollerMsg(_T("   Removed outdated peer information from interface %s\r\n"), iface->getName());
               nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 6, _T("Node::topologyPoll(%s [%u]): peer node set on interface \"%s\" but node or access point object does not exist"), m_name, m_id, iface->getName());
               iface->clearPeer();
               continue;
            }

            // Remove peer information if more than one MAC address detected on port
            // Do not change information when interface is down
            if ((iface->getConfirmedOperState() == IF_OPER_STATE_UP) && (iface->getPeerDiscoveryProtocol() == LL_PROTO_FDB) && nbs->isMultipointInterface(iface->getIfIndex()))
            {
               ClearPeer(iface->getPeerInterfaceId());
               iface->clearPeer();
               nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 6, _T("Node::topologyPoll(%s [%u]): Removed outdated peer information from interface %s [%u] (multiple MAC addresses detected)"), m_name, m_id, iface->getName(), iface->getId());
               continue;
            }

            // Remove expired peer information
            if (iface->clearExpiredPeerData(peerRetentionTime))
            {
               sendPollerMsg(_T("   Removed outdated peer information from interface %s\r\n"), iface->getName());
               nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 6, _T("Node::topologyPoll(%s [%u]): removed outdated peer information from interface \"%s\" [%u] (no confirmation for more than %d days)"),
                  m_name, m_id, iface->getName(), iface->getId(), peerRetentionTime / 86400);
            }
         }
      }
      unlockChildList();

      sendPollerMsg(_T("Link layer topology processed\r\n"));
      nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 4, _T("Link layer topology processed for node %s [%d]"), m_name, m_id);
   }
   else
   {
      sendPollerMsg(POLLER_ERROR _T("Cannot get link layer topology\r\n"));
   }

   POLL_CANCELLATION_CHECKPOINT();

   // Read list of associated wireless stations
   if (m_capabilities & (NC_IS_WIFI_CONTROLLER | NC_IS_WIFI_AP))
   {
      poller->setStatus(_T("reading wireless stations"));
      SNMP_Transport *snmp = createSnmpTransport();
      if (snmp != nullptr)
      {
         ObjectArray<WirelessStationInfo> *stations = m_driver->getWirelessStations(snmp, this, m_driverData);
         delete snmp;
         if (stations != nullptr)
         {
            sendPollerMsg(_T("   %d wireless stations found\r\n"), stations->size());
            nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 6, _T("%d wireless stations found on controller node %s [%u]"), stations->size(), m_name, m_id);

            for(int i = 0; i < stations->size(); i++)
            {
               WirelessStationInfo *ws = stations->get(i);

               if (m_capabilities & NC_IS_WIFI_CONTROLLER)
               {
                  shared_ptr<AccessPoint> ap;
                  shared_ptr<WirelessDomain> wirelessDomain = getWirelessDomain();
                  if (wirelessDomain != nullptr)
                  {
                     switch(ws->apMatchPolicy)
                     {
                        case AP_MATCH_BY_BSSID:
                           ap = wirelessDomain->findAccessPointByBSSID(ws->bssid);
                           break;
                        case AP_MATCH_BY_RFINDEX:
                           ap = wirelessDomain->findAccessPointByRadioId(ws->rfIndex);
                           break;
                        case AP_MATCH_BY_SERIAL:
                           ap = wirelessDomain->findAccessPointBySerial(ws->apSerial);
                           break;
                     }
                  }
                  if (ap != nullptr)
                  {
                     ws->apObjectId = ap->getId();
                     ap->getRadioName(ws->rfIndex, ws->rfName, MAX_OBJECT_NAME);
                  }
                  else
                  {
                     ws->apObjectId = 0;
                     ws->rfName[0] = 0;
                  }
               }
               else
               {
                  ws->apObjectId = m_id;
                  getRadioName(ws->rfIndex, ws->rfName, MAX_OBJECT_NAME);
               }

               shared_ptr<Node> node = FindNodeByMAC(ws->macAddr);
               ws->nodeId = (node != nullptr) ? node->getId() : 0;
               if ((node != nullptr) && (!ws->ipAddr.isValid()))
               {
                  shared_ptr<Interface> iface = node->findInterfaceByMAC(MacAddress(ws->macAddr, MAC_ADDR_LENGTH));
                  if ((iface != nullptr) && iface->getIpAddressList()->getFirstUnicastAddressV4().isValid())
                     ws->ipAddr = iface->getIpAddressList()->getFirstUnicastAddressV4();
                  else
                     ws->ipAddr = node->getIpAddress();
               }
            }
         }

         lockProperties();
         delete m_wirelessStations;
         m_wirelessStations = stations;
         unlockProperties();
      }
   }

   if (m_capabilities & NC_IS_OSPF)
   {
      StructArray<OSPFArea> areas;
      StructArray<OSPFInterface> interfaces;
      StructArray<OSPFNeighbor> neighbors;
      bool success = CollectOSPFInformation(this, &areas, &interfaces, &neighbors);

      if (success)
      {
         neighbors.sort(
            []  (const void *a1, const void *a2) -> int
            {
               uint32_t id1 = static_cast<const OSPFNeighbor*>(a1)->routerId;
               uint32_t id2 = static_cast<const OSPFNeighbor*>(a2)->routerId;
               if (id1 < id2)
                  return -1;
               if (id1 > id2)
                  return -1;

               uint32_t ifIndex1 = static_cast<const OSPFNeighbor*>(a1)->ifIndex;
               uint32_t ifIndex2 = static_cast<const OSPFNeighbor*>(a2)->ifIndex;
               return (ifIndex1 < ifIndex2) ? -1 : ((ifIndex1 > ifIndex2) ? 1 : 0);
            });

         areas.sort(
            [] (const void *a1, const void *a2) -> int
            {
               uint32_t id1 = static_cast<const OSPFArea*>(a1)->id;
               uint32_t id2 = static_cast<const OSPFArea*>(a2)->id;
               return (id1 < id2) ? -1 : ((id1 > id2) ? 1 : 0);
            });

         lockProperties();

         bool changed = (m_ospfAreas.size() != areas.size());
         if (!changed)
         {
            for(int i = 0; (i < areas.size()) && !changed; i++)
            {
               if (areas.get(i)->id != m_ospfAreas.get(i)->id)
               {
                  changed = true;
                  break;
               }
            }
         }

         m_ospfAreas.clear();
         m_ospfAreas.addAll(areas);
         if (changed)
         {
            nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 6, _T("OSPF area configuration changed for node %s [%u]"), m_name, m_id);
            setModified(MODIFY_OSPF_AREAS);
         }

         changed = (m_ospfNeighbors.size() != neighbors.size());
         if (!changed)
         {
            for(int i = 0; (i < neighbors.size()) && !changed; i++)
            {
               OSPFNeighbor *n1 = m_ospfNeighbors.get(i);
               OSPFNeighbor *n2 = neighbors.get(i);
               if ((n1->routerId != n2->routerId) || (n1->ifIndex != n2->ifIndex) || (n1->nodeId != n2->nodeId) || (n1->areaId != n2->areaId) || (n1->state != n2->state) || !n1->ipAddress.equals(n2->ipAddress))
               {
                  changed = true;
                  break;
               }
            }
         }

         m_ospfNeighbors.clear();
         m_ospfNeighbors.addAll(neighbors);
         if (changed)
         {
            nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 6, _T("OSPF neighbor configuration changed for node %s [%u]"), m_name, m_id);
            setModified(MODIFY_OSPF_NEIGHBORS);
         }

         unlockProperties();
      }
      else
      {
         lockProperties();
         m_ospfAreas.clear();
         m_ospfNeighbors.clear();
         unlockProperties();
      }

      // Update OSPF information on interfaces
      readLockChildList();
      for(int i = 0; i < getChildList().size(); i++)
      {
         NetObj *curr = getChildList().get(i);
         if (curr->getObjectClass() != OBJECT_INTERFACE)
            continue;

         Interface *iface = static_cast<Interface*>(curr);
         bool found = false;
         for(int j = 0; j < interfaces.size(); j++)
         {
            OSPFInterface *ospfInterface = interfaces.get(j);
            if (iface->getIfIndex() == ospfInterface->ifIndex)
            {
               iface->setOSPFInformation(*ospfInterface);
               found = true;
               break;
            }
         }
         if (!found)
         {
            iface->clearOSPFInformation();
         }
      }
      unlockChildList();
   }

   if (m_ipAddress.isValidUnicast())
   {
      POLL_CANCELLATION_CHECKPOINT();

      shared_ptr<NetObj> mgmtNode = FindObjectById(g_dwMgmtNode, OBJECT_NODE);
      if (mgmtNode != nullptr)
      {
         shared_ptr<NetworkPath> trace = TraceRoute(static_pointer_cast<Node>(mgmtNode), self());
         if ((trace != nullptr) && trace->isComplete())
         {
            lockProperties();
            m_lastKnownNetworkPath = trace;
            unlockProperties();
            nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 6, _T("TopologyPoll(%s [%d]): network path updated"), m_name, m_id);
         }
      }
   }

   POLL_CANCELLATION_CHECKPOINT();

   // Call hooks in loaded modules
   poller->setStatus(_T("calling modules"));
   ENUMERATE_MODULES(pfTopologyPollHook)
   {
      nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 5, _T("TopologyPoll(%s [%d]): calling hook in module %s"), m_name, m_id, CURRENT_MODULE.name);
      CURRENT_MODULE.pfTopologyPollHook(this, pSession, rqId, poller);
   }

   POLL_CANCELLATION_CHECKPOINT();

   // Execute hook script
   poller->setStatus(_T("hook"));
   executeHookScript(_T("TopologyPoll"));

   sendPollerMsg(_T("Finished topology poll of node %s\r\n"), m_name);

   pollerUnlock();

   nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 4, _T("Finished topology poll of node %s [%d]"), m_name, m_id);
}

/**
 * Update host connections using forwarding database information
 */
void Node::addHostConnections(LinkLayerNeighbors *nbs)
{
   shared_ptr<ForwardingDatabase> fdb = getSwitchForwardingDatabase();
   if (fdb == nullptr)
      return;

   nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 5, _T("Node::addHostConnections(%s [%u]): FDB retrieved"), m_name, m_id);

   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      if (getChildList().get(i)->getObjectClass() != OBJECT_INTERFACE)
         continue;

      Interface *ifLocal = static_cast<Interface*>(getChildList().get(i));
      MacAddress macAddr;
      if (fdb->isSingleMacOnPort(ifLocal->getIfIndex(), &macAddr))
      {
         TCHAR macAddrText[64];
         nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 6, _T("Node::addHostConnections(%s [%u]): found single MAC %s on interface %s"),
            m_name, m_id, macAddr.toString(macAddrText), ifLocal->getName());
         shared_ptr<Interface> ifRemote = FindInterfaceByMAC(macAddr);
         if (ifRemote != nullptr)
         {
            nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 6, _T("Node::addHostConnections(%s [%u]): found remote interface %s [%u]"),
               m_name, m_id, ifRemote->getName(), ifRemote->getId());
            shared_ptr<Node> peerNode = ifRemote->getParentNode();
            if (peerNode != nullptr)
            {
               if (peerNode->getId() != m_id)
               {
                  LL_NEIGHBOR_INFO info;
                  info.ifLocal = ifLocal->getIfIndex();
                  info.ifRemote = ifRemote->getIfIndex();
                  info.objectId = peerNode->getId();
                  info.isPtToPt = true;
                  info.protocol = LL_PROTO_FDB;
                  info.isCached = false;
                  nbs->addConnection(info);
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 6, _T("Node::addHostConnections(%s [%u]): found own MAC address %s"), m_name, m_id, macAddrText);
               }
            }
         }
      }
      else
      {
         nbs->markMultipointInterface(ifLocal->getIfIndex());
      }
   }
   unlockChildList();
}

/**
 * Add existing connections to link layer neighbors table
 */
void Node::addExistingConnections(LinkLayerNeighbors *nbs)
{
   readLockChildList();
   for(int i = 0; i < (int)getChildList().size(); i++)
   {
      if (getChildList().get(i)->getObjectClass() != OBJECT_INTERFACE)
         continue;

      Interface *ifLocal = (Interface *)getChildList().get(i);
      if ((ifLocal->getPeerNodeId() != 0) && (ifLocal->getPeerInterfaceId() != 0))
      {
         shared_ptr<Interface> ifRemote = static_pointer_cast<Interface>(FindObjectById(ifLocal->getPeerInterfaceId(), OBJECT_INTERFACE));
         if (ifRemote != nullptr)
         {
            LL_NEIGHBOR_INFO info;
            info.ifLocal = ifLocal->getIfIndex();
            info.ifRemote = ifRemote->getIfIndex();
            info.objectId = ifLocal->getPeerNodeId();
            info.isPtToPt = true;
            info.protocol = ifLocal->getPeerDiscoveryProtocol();
            info.isCached = true;
            nbs->addConnection(info);
         }
      }
   }
   unlockChildList();
}

/**
 * Resolve port indexes in VLAN list
 */
void Node::resolveVlanPorts(VlanList *vlanList)
{
   HashMap<uint32_t, IntegerArray<uint32_t>> vlansByIface(Ownership::False);
   for(int i = 0; i < vlanList->size(); i++)
   {
      VlanInfo *vlan = vlanList->get(i);
      for(int j = 0; j < vlan->getNumPorts(); j++)
      {
         const VlanPortInfo *port = vlan->getPort(j);
         shared_ptr<Interface> iface;
         switch(vlan->getPortReferenceMode())
         {
            case VLAN_PRM_IFINDEX:  // interface index
               iface = findInterfaceByIndex(port->portId);
               break;
            case VLAN_PRM_BPORT:    // bridge port number
               iface = findBridgePort(port->portId);
               break;
            case VLAN_PRM_PHYLOC:   // physical location
               iface = findInterfaceByLocation(port->location);
               break;
         }
         if (iface != nullptr)
         {
            vlan->resolvePort(j, iface->getPhysicalLocation(), iface->getIfIndex(), iface->getId());
            IntegerArray<uint32_t> *vlans = vlansByIface.get(iface->getId());
            if (vlans == nullptr)
            {
               vlans = new IntegerArray<uint32_t>(16, 16);
               vlansByIface.set(iface->getId(), vlans);
            }
            vlans->add(vlan->getVlanId());
         }
      }
   }

   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *o = getChildList().get(i);
      if (o->getObjectClass() == OBJECT_INTERFACE)
      {
         IntegerArray<uint32_t> *vlans = vlansByIface.get(o->getId());
         if (vlans != nullptr)
            vlans->sortAscending();
         static_cast<Interface*>(o)->updateVlans(vlans);
      }
   }
   unlockChildList();
}

/**
 * Subnet creation lock
 */
static Mutex s_subnetCreationMutex;

/**
 * Create new subnet and binds to this node
 */
shared_ptr<Subnet> Node::createSubnet(InetAddress& baseAddr, bool syntheticMask)
{
   InetAddress addr = baseAddr.getSubnetAddress();
   if ((addr.getFamily() == AF_INET) && ((addr.getAddressV4() & 0xFF000000) == 0))
      return shared_ptr<Subnet>();  // Do not create subnet from 0.0.0.0/8

   if (syntheticMask)
   {
      if (!AdjustSubnetBaseAddress(addr, m_zoneUIN))
         return shared_ptr<Subnet>();
   }

   shared_ptr<Subnet> subnet = make_shared<Subnet>(addr, m_zoneUIN, syntheticMask);

   ScriptVMHandle vm = CreateServerScriptVM(_T("Hook::CreateSubnet"), self());
   if (vm.isValid())
   {
      bool pass;
      vm->setUserData(this);
      NXSL_Value *argv = subnet->createNXSLObject(vm);
      if (vm->run(1, &argv))
      {
         pass = vm->getResult()->getValueAsBoolean();
      }
      else
      {
         pass = false;  // Consider script runtime error as blocking
         nxlog_debug(4, _T("Node::createSubnet(%s [%u]): hook script execution error: %s"), m_name, m_id, vm->getErrorText());
         ReportScriptError(SCRIPT_CONTEXT_OBJECT, this, 0, vm->getErrorText(), _T("Hook::CreateSubnet"));
      }
      vm.destroy();
      nxlog_debug(6, _T("Node::createSubnet(%s [%u]): subnet \"%s\" %s by filter"),
                m_name, m_id, subnet->getName(), pass ? _T("accepted") : _T("rejected"));
      if (!pass)
      {
         subnet.reset();
      }
   }
   else
   {
      nxlog_debug(7, _T("Node::createSubnet(%s [%u]): hook script \"Hook::CreateSubnet\" %s"), m_name, m_id, HookScriptLoadErrorToText(vm.failureReason()));
   }

   if (subnet != nullptr)
   {
      // Insert new subnet atomically - otherwise two polls running in parallel could create same subnet twice
      s_subnetCreationMutex.lock();
      shared_ptr<Subnet> s = FindSubnetByIP(m_zoneUIN, subnet->getIpAddress());
      if (s == nullptr)
      {
         NetObjInsert(subnet, true, false);
         if (g_flags & AF_ENABLE_ZONING)
         {
            shared_ptr<Zone> zone = FindZoneByUIN(m_zoneUIN);
            if (zone != nullptr)
            {
               linkObjects(zone, subnet);
            }
            else
            {
               nxlog_debug(1, _T("Node::createSubnet(): Inconsistent configuration - zone %d does not exist"), (int)m_zoneUIN);
            }
         }
         else
         {
            linkObjects(g_entireNetwork, subnet);
         }
         nxlog_debug(4, _T("Node::createSubnet(): Created new subnet %s [%u] for node %s [%u]"),
                  subnet->getName(), subnet->getId(), m_name, m_id);
      }
      else
      {
         // This subnet already exist
         subnet = s;
      }
      s_subnetCreationMutex.unlock();
      subnet->addNode(self());
   }
   return subnet;
}

/**
 * Check subnet bindings
 */
void Node::checkSubnetBinding()
{
   shared_ptr<Cluster> cluster = getCluster();

   // Build consolidated IP address list
   InetAddressList addrList;
   readLockChildList();
   for(int n = 0; n < getChildList().size(); n++)
   {
      NetObj *curr = getChildList().get(n);
      if (curr->getObjectClass() != OBJECT_INTERFACE)
         continue;

      Interface *iface = static_cast<Interface*>(curr);
      if (iface->isExcludedFromTopology())
         continue;

      for(int m = 0; m < iface->getIpAddressList()->size(); m++)
      {
         const InetAddress& a = iface->getIpAddressList()->get(m);
         if (a.isValidUnicast())
         {
            addrList.add(a);
         }
      }
   }
   unlockChildList();

   // Check if we have subnet bindings for all interfaces
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, L"Checking subnet bindings for node %s [%u]", m_name, m_id);
   for(int i = 0; i < addrList.size(); i++)
   {
      InetAddress addr = addrList.get(i);

      wchar_t buffer[64];
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, L"Node::checkSubnetBinding(%s [%u]): checking address %s/%d", m_name, m_id, addr.toString(buffer), addr.getMaskBits());

      shared_ptr<Interface> iface = findInterfaceByIP(addr);
      if (iface == nullptr)
      {
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_CONF_POLL, _T("Internal error: cannot find interface object in Node::checkSubnetBinding()"));
         continue;   // Something goes really wrong
      }

      // Is cluster interconnect interface?
      bool isSync = (cluster != nullptr) ? cluster->isSyncAddr(addr) : false;

      shared_ptr<Subnet> pSubnet = FindSubnetForNode(m_zoneUIN, addr);
      if (pSubnet != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("Node::checkSubnetBinding(%s [%d]): found subnet %s [%u]"), m_name, m_id, pSubnet->getName(), pSubnet->getId());
         if (isSync)
         {
            pSubnet.reset();   // No further checks on this subnet
         }
         else
         {
            if (pSubnet->isSyntheticMask() && !iface->isSyntheticMask() && (addr.getMaskBits() > 0))
            {
               nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 4, _T("Setting correct netmask for subnet %s [%u] from node %s [%u]"),
                        pSubnet->getName(), pSubnet->getId(), m_name, m_id);
               if ((addr.getHostBits() < 2) && (getParentCount() > 1))
               {
                  /* Delete subnet object if we try to change it's netmask to 255.255.255.255 or 255.255.255.254 and
                   node has more than one parent. hasIfaceForPrimaryIp parameter should prevent us from going in
                   loop (creating and deleting all the time subnet). */
                  pSubnet->deleteObject();
                  pSubnet.reset();   // prevent binding to deleted subnet
               }
               else
               {
                  pSubnet->setCorrectMask(addr.getSubnetAddress());
               }
            }

            // Check if node is linked to this subnet
            if ((pSubnet != nullptr) && !pSubnet->isDirectChild(m_id))
            {
               nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 4, L"Restored link between subnet %s [%u] and node %s [%u]",
                        pSubnet->getName(), pSubnet->getId(), m_name, m_id);
               pSubnet->addNode(self());
            }
         }
      }
      else if (!isSync)
      {
         wchar_t buffer[64];
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, L"Missing subnet for address %s/%d on interface %s [%u]",
            addr.toString(buffer), addr.getMaskBits(), iface->getName(), iface->getIfIndex());

         // Ignore masks 255.255.255.255 and 255.255.255.254 - some point-to-point interfaces can have such mask
         if (addr.getHostBits() > 1)
         {
            if (addr.getMaskBits() > 0)
            {
               pSubnet = createSubnet(addr, iface->isSyntheticMask());
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("Zero subnet mask on interface %s [%u]"), iface->getName(), iface->getIfIndex());
               addr.setMaskBits((addr.getFamily() == AF_INET) ? ConfigReadInt(_T("Objects.Subnets.DefaultSubnetMaskIPv4"), 24) : ConfigReadInt(_T("Objects.Subnets.DefaultSubnetMaskIPv6"), 64));
               pSubnet = createSubnet(addr, true);
            }
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("Subnet not required for address %s/%d on interface %s [%u]"),
               addr.toString(buffer), addr.getMaskBits(), iface->getName(), iface->getIfIndex());
         }
      }

      // Check if subnet mask is correct on interface
      if ((pSubnet != nullptr) && (pSubnet->getIpAddress().getMaskBits() != addr.getMaskBits()) && (addr.getHostBits() > 0))
      {
         shared_ptr<Interface> iface = findInterfaceByIP(addr);
         EventBuilder(EVENT_INCORRECT_NETMASK, m_id)
            .param(_T("interfaceObjectId"), iface->getId(), EventBuilder::OBJECT_ID_FORMAT)
            .param(_T("interfaceIndex"), iface->getIfIndex())
            .param(_T("interfaceName"), iface->getName())
            .param(_T("actualNetworkMask"), addr.getMaskBits())
            .param(_T("correctNetworkMask"), pSubnet->getIpAddress().getMaskBits())
            .post();
      }
   }

   // Some devices may report interface list, but without IP
   // To prevent such nodes from hanging at top of the tree, attempt
   // to find subnet for node's primary IP
   if ((getParentsCount(OBJECT_SUBNET) == 0) && m_ipAddress.isValidUnicast() && !(m_flags & NF_EXTERNAL_GATEWAY) && !addrList.hasAddress(m_ipAddress))
   {
      shared_ptr<Subnet> pSubnet = FindSubnetForNode(m_zoneUIN, m_ipAddress);
      if (pSubnet != nullptr)
      {
         // Check if node is linked to this subnet
         if (!pSubnet->isDirectChild(m_id))
         {
            nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 4, _T("Restored link between subnet %s [%u] and node %s [%u]"), pSubnet->getName(), pSubnet->getId(), m_name, m_id);
            pSubnet->addNode(self());
         }
      }
      else
      {
         InetAddress addr(m_ipAddress);
         addr.setMaskBits((addr.getFamily() == AF_INET) ? ConfigReadInt(_T("Objects.Subnets.DefaultSubnetMaskIPv4"), 24) : ConfigReadInt(_T("Objects.Subnets.DefaultSubnetMaskIPv6"), 64));
         createSubnet(addr, true);
      }
   }

   // Check for incorrect parent subnets
   readLockParentList();
   readLockChildList();
   ObjectArray<NetObj> unlinkList(getParentList().size());
   for(int i = 0; i < getParentList().size(); i++)
   {
      if (getParentList().get(i)->getObjectClass() == OBJECT_SUBNET)
      {
         Subnet *subnet = static_cast<Subnet*>(getParentList().get(i));
         if (subnet->getIpAddress().contains(m_ipAddress) && !(m_flags & NF_EXTERNAL_GATEWAY))
            continue;   // primary IP is in given subnet

         int j;
         for(j = 0; j < addrList.size(); j++)
         {
            const InetAddress& addr = addrList.get(j);
            if (subnet->getIpAddress().contains(addr))
            {
               if ((cluster != nullptr) && cluster->isSyncAddr(addr))
               {
                  j = addrList.size(); // Cause to unbind from this subnet
               }
               break;
            }
         }
         if (j == addrList.size())
         {
            nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 4, _T("Node::CheckSubnetBinding(): Subnet %s [%u] is incorrect for node %s [%u]"),
                  subnet->getName(), subnet->getId(), m_name, m_id);
            unlinkList.add(subnet);
         }
      }
   }
   unlockChildList();
   unlockParentList();

   // Unlink for incorrect subnet objects
   for(int n = 0; n < unlinkList.size(); n++)
   {
      NetObj *o = unlinkList.get(n);
      unlinkObjects(o, this);
      o->calculateCompoundStatus();
   }
}

/**
 * Update interface names
 */
void Node::updateInterfaceNames(ClientSession *pSession, UINT32 rqId)
{
   _pollerLock();
   if (m_isDeleteInitiated || IsShutdownInProgress())
   {
      _pollerUnlock();
      return;
   }

   m_pollRequestor = pSession;
   m_pollRequestId = rqId;

   if (m_status == STATUS_UNMANAGED)
   {
      sendPollerMsg(POLLER_WARNING _T("Node %s is unmanaged, interface names poll aborted\r\n"), m_name);
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("Node %s [%u] is unmanaged, interface names poll aborted"), m_name, m_id);
      _pollerUnlock();
      return;
   }

   sendPollerMsg(_T("Starting interface names poll of node %s\r\n"), m_name);
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 4, _T("Starting interface names poll of node %s (ID: %d)"), m_name, m_id);

   // Retrieve interface list
   InterfaceList *pIfList = getInterfaceList();
   if (pIfList != nullptr)
   {
      int useAliases = ConfigReadInt(_T("Objects.Interfaces.UseAliases"), 0);

      // Check names of existing interfaces
      for(int j = 0; j < pIfList->size(); j++)
      {
         InterfaceInfo *ifInfo = pIfList->get(j);

         readLockChildList();
         for(int i = 0; i < getChildList().size(); i++)
         {
            if (getChildList().get(i)->getObjectClass() == OBJECT_INTERFACE)
            {
               Interface *iface = static_cast<Interface*>(getChildList().get(i));

               if (ifInfo->index == iface->getIfIndex())
               {
                  sendPollerMsg(_T("   Checking interface %u (%s)\r\n"), iface->getIfIndex(), iface->getName());

                  TCHAR ifObjectName[MAX_OBJECT_NAME], expandedName[MAX_OBJECT_NAME];
                  BuildInterfaceObjectName(ifObjectName, *ifInfo, useAliases);
                  iface->expandName(ifObjectName, expandedName);
                  if (_tcscmp(expandedName, iface->getName()))
                  {
                     iface->setName(expandedName);
                     sendPollerMsg(POLLER_WARNING _T("   Object name of interface %u changed to %s\r\n"), iface->getIfIndex(), expandedName);
                  }
                  if (_tcscmp(ifInfo->name, iface->getIfName()))
                  {
                     iface->setIfName(ifInfo->name);
                     sendPollerMsg(POLLER_WARNING _T("   Name of interface %u changed to %s\r\n"), iface->getIfIndex(), ifInfo->name);
                  }
                  if (_tcscmp(ifInfo->description, iface->getDescription()))
                  {
                     iface->setDescription(ifInfo->description);
                     sendPollerMsg(POLLER_WARNING _T("   Description of interface %u changed to %s\r\n"), iface->getIfIndex(), ifInfo->description);
                  }
                  if (_tcscmp(ifInfo->alias, iface->getIfAlias()))
                  {
                     if (iface->getAlias().isEmpty() || !_tcscmp(iface->getIfAlias(), iface->getAlias()))
                     {
                        iface->setAlias(ifInfo->alias);
                        sendPollerMsg(POLLER_WARNING _T("   Alias of interface %u changed to %s\r\n"), iface->getIfIndex(), ifInfo->alias);
                     }
                     iface->setIfAlias(ifInfo->alias);
                     sendPollerMsg(POLLER_WARNING _T("   SNMP alias of interface %u changed to %s\r\n"), iface->getIfIndex(), ifInfo->alias);
                  }
                  break;
               }
            }
         }
         unlockChildList();
      }

      delete pIfList;
   }
   else     /* pIfList == nullptr */
   {
      sendPollerMsg(POLLER_ERROR _T("   Unable to get interface list from node\r\n"));
   }

   // Finish poll
   sendPollerMsg(_T("Finished interface names poll of node %s\r\n"), m_name);
   _pollerUnlock();
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 4, _T("Finished interface names poll of node %s (ID: %d)"), m_name, m_id);
}

/**
 * Get list of hardware components as JSON array
 */
json_t *Node::getHardwareComponentsAsJSON()
{
   json_t *componentsArray = nullptr;
   lockProperties();
   if (m_hardwareComponents != nullptr)
   {
      componentsArray = json_array();
      int hcSize = m_hardwareComponents->size();
      for (int i = 0; i < hcSize; i++)
      {
         json_array_append_new(componentsArray, m_hardwareComponents->get(i)->toJson());
      }
   }
   unlockProperties();
   return componentsArray;
}

/**
 * Get list of software packages as JSON array
 */
json_t *Node::getSoftwarePackagesAsJSON(const wchar_t *filter)
{
   json_t *packagesArray = nullptr;
   lockProperties();
   if (m_softwarePackages != nullptr)
   {
      packagesArray = json_array();
      int spSize = m_softwarePackages->size();
      for (int i = 0; i < spSize; i++)
      {
         SoftwarePackage *pkg = m_softwarePackages->get(i);
         if ((filter == nullptr) || (wcsistr(pkg->getName(), filter) != nullptr) || (wcsistr(pkg->getDescription(), filter) != nullptr))
            json_array_append_new(packagesArray, pkg->toJson());
      }
   }
   unlockProperties();
   return packagesArray;
}

/**
 * Get list of parent objects for NXSL script
 */
NXSL_Value *Node::getParentsForNXSL(NXSL_VM *vm)
{
   NXSL_Array *parents = new NXSL_Array(vm);
   int index = 0;

   readLockParentList();
   for(int i = 0; i < getParentList().size(); i++)
   {
      NetObj *object = getParentList().get(i);
      if ((object->getObjectClass() != OBJECT_TEMPLATE) && object->isTrustedObject(m_id))
      {
         parents->set(index++, object->createNXSLObject(vm));
      }
   }
   unlockParentList();

   return vm->createValue(parents);
}

/**
 * Get list of interface objects for NXSL script
 */
NXSL_Value *Node::getInterfacesForNXSL(NXSL_VM *vm)
{
   NXSL_Array *ifaces = new NXSL_Array(vm);
   int index = 0;

   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *curr = getChildList().get(i);
      if (curr->getObjectClass() == OBJECT_INTERFACE)
      {
         ifaces->set(index++, curr->createNXSLObject(vm));
      }
   }
   unlockChildList();

   return vm->createValue(ifaces);
}

/**
 * Get list of hardware components for NXSL script
 */
NXSL_Value *Node::getHardwareComponentsForNXSL(NXSL_VM* vm)
{
   NXSL_Array* a = new NXSL_Array(vm);
   lockProperties();
   if (m_hardwareComponents != nullptr)
   {
      int hcSize = m_hardwareComponents->size();
      for (int i = 0; i < hcSize; i++)
      {
         a->append(vm->createValue(vm->createObject(&g_nxslHardwareComponent, new HardwareComponent(*m_hardwareComponents->get(i)))));
      }
   }
   unlockProperties();
   return vm->createValue(a);
}

/**
 * Get list of software packages for NXSL script
 */
NXSL_Value* Node::getSoftwarePackagesForNXSL(NXSL_VM* vm)
{
   NXSL_Array *a = new NXSL_Array(vm);
   lockProperties();
   if (m_softwarePackages != nullptr)
   {
      int hcSize = m_softwarePackages->size();
      for (int i = 0; i < hcSize; i++)
      {
         a->append(vm->createValue(vm->createObject(&g_nxslSoftwarePackage, new SoftwarePackage(*m_softwarePackages->get(i)))));
      }
   }
   unlockProperties();
   return vm->createValue(a);
}

/**
 * Get list of OSPF areas for use within NXSL
 */
NXSL_Value *Node::getOSPFAreasForNXSL(NXSL_VM *vm)
{
   NXSL_Array *a = new NXSL_Array(vm);
   lockProperties();
   for (int i = 0; i < m_ospfAreas.size(); i++)
   {
      a->append(vm->createValue(vm->createObject(&g_nxslOSPFAreaClass, new OSPFArea(*m_ospfAreas.get(i)))));
   }
   unlockProperties();
   return vm->createValue(a);
}

/**
 * Get list of OSPF neighbors for use within NXSL
 */
NXSL_Value *Node::getOSPFNeighborsForNXSL(NXSL_VM *vm)
{
   NXSL_Array *a = new NXSL_Array(vm);
   lockProperties();
   for (int i = 0; i < m_ospfNeighbors.size(); i++)
   {
      a->append(vm->createValue(vm->createObject(&g_nxslOSPFNeighborClass, new OSPFNeighbor(*m_ospfNeighbors.get(i)))));
   }
   unlockProperties();
   return vm->createValue(a);
}

/**
 * Check and update last agent trap ID
 */
bool Node::checkAgentTrapId(uint64_t trapId)
{
   lockProperties();
   bool valid = (trapId > m_lastAgentTrapId);
   if (valid)
      m_lastAgentTrapId = trapId;
   unlockProperties();
   return valid;
}

/**
 * Check and update last agent SNMP trap ID
 */
bool Node::checkSNMPTrapId(uint32_t id)
{
   lockProperties();
   bool valid = (id > m_lastSNMPTrapId);
   if (valid)
      m_lastSNMPTrapId = id;
   unlockProperties();
   return valid;
}

/**
 * Check and update last syslog message ID
 */
bool Node::checkSyslogMessageId(uint64_t id)
{
   lockProperties();
   bool valid = (id > m_lastSyslogMessageId);
   if (valid)
      m_lastSyslogMessageId = id;
   unlockProperties();
   return valid;
}

/**
 * Check and update last windows log message ID
 */
bool Node::checkWindowsEventId(uint64_t id)
{
   lockProperties();
   bool valid = (id > m_lastWindowsEventId);
   if (valid)
      m_lastWindowsEventId = id;
   unlockProperties();
   return valid;
}

/**
 * Check and update last agent data push request ID
 */
bool Node::checkAgentPushRequestId(uint64_t requestId)
{
   lockProperties();
   bool valid = (requestId > m_lastAgentPushRequestId);
   if (valid)
      m_lastAgentPushRequestId = requestId;
   unlockProperties();
   return valid;
}

/**
 * Check if data collection is disabled
 */
bool Node::isDataCollectionDisabled()
{
   return (m_flags & DCF_DISABLE_DATA_COLLECT) != 0;
}

/**
 * Get LLDP local port info by LLDP local ID
 *
 * @param idType port ID type (value of lldpLocPortIdSubtype)
 * @param id port ID
 * @param idLen port ID length in bytes
 * @param buffer buffer for storing port information
 */
bool Node::getLldpLocalPortInfo(uint32_t idType, BYTE *id, size_t idLen, LLDP_LOCAL_PORT_INFO *port) const
{
   bool result = false;
   lockProperties();
   if (m_lldpLocalPortInfo != nullptr)
   {
      for(int i = 0; i < m_lldpLocalPortInfo->size(); i++)
      {
         LLDP_LOCAL_PORT_INFO *p = m_lldpLocalPortInfo->get(i);
         if ((idType == p->localIdSubtype) && (idLen == p->localIdLen) && !memcmp(id, p->localId, idLen))
         {
            memcpy(port, p, sizeof(LLDP_LOCAL_PORT_INFO));
            result = true;
            break;
         }
      }
   }
   unlockProperties();
   return result;
}

/**
 * Get LLDP local port info by lldpLocPortNum
 *
 * @param localPortNumber port number (value of lldpLocPortNum)
 * @param port buffer for storing port information
 */
bool Node::getLldpLocalPortInfo(uint32_t localPortNumber, LLDP_LOCAL_PORT_INFO *port) const
{
   bool result = false;
   lockProperties();
   if (m_lldpLocalPortInfo != nullptr)
   {
      for(int i = 0; i < m_lldpLocalPortInfo->size(); i++)
      {
         LLDP_LOCAL_PORT_INFO *p = m_lldpLocalPortInfo->get(i);
         if (localPortNumber == p->portNumber)
         {
            memcpy(port, p, sizeof(LLDP_LOCAL_PORT_INFO));
            result = true;
            break;
         }
      }
   }
   unlockProperties();
   return result;
}

/**
 * Show node LLDP information
 */
void Node::showLLDPInfo(ServerConsole *console) const
{
   TCHAR buffer[256];

   lockProperties();
   console->printf(_T("\x1b[1m*\x1b[0m Node LLDP ID: %s\n\n"), m_lldpNodeId);
   console->print(_T("\x1b[1m*\x1b[0m Local LLDP ports\n"));
   if (m_lldpLocalPortInfo != nullptr)
   {
      console->print(_T("   Port | ifIndex | ST | Len | Local ID                 | Description\n")
                     _T("   -----+---------+----+-----+--------------------------+--------------------------------------\n"));
      for(int i = 0; i < m_lldpLocalPortInfo->size(); i++)
      {
         LLDP_LOCAL_PORT_INFO *port = m_lldpLocalPortInfo->get(i);
         console->printf(_T("   %4u | %7u | %2u | %3d | %-24s | %s\n"), port->portNumber, port->ifIndex,
                  port->localIdSubtype, (int)port->localIdLen, BinToStr(port->localId, port->localIdLen, buffer), port->ifDescr);
      }
   }
   else
   {
      console->print(_T("   No local LLDP port info\n"));
   }
   unlockProperties();
}

/**
 * Fill NXCP message with software package list
 */
void Node::writePackageListToMessage(NXCPMessage *msg)
{
   lockProperties();
   if (m_softwarePackages != nullptr)
   {
      msg->setField(VID_NUM_ELEMENTS, static_cast<uint32_t>(m_softwarePackages->size()));
      uint32_t fieldId = VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < m_softwarePackages->size(); i++)
      {
         m_softwarePackages->get(i)->fillMessage(msg, fieldId);
         fieldId += 10;
      }
      msg->setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      msg->setField(VID_RCC, RCC_NO_SOFTWARE_PACKAGE_DATA);
   }
   unlockProperties();
}

/**
 * Fill NXCP message with hardware component list
 */
void Node::writeHardwareListToMessage(NXCPMessage *msg)
{
   lockProperties();
   if (m_hardwareComponents != nullptr)
   {
      msg->setField(VID_NUM_ELEMENTS, m_hardwareComponents->size());
      UINT32 varId = VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < m_hardwareComponents->size(); i++)
      {
         m_hardwareComponents->get(i)->fillMessage(msg, varId);
         varId += 64;
      }
      msg->setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      msg->setField(VID_RCC, RCC_NO_HARDWARE_DATA);
   }
   unlockProperties();
}

/**
 * Create DCI Table form list of registered wireless stations
 */
shared_ptr<Table> Node::wsListAsTable()
{
   shared_ptr<Table> result;
   lockProperties();
   if (m_wirelessStations != nullptr)
   {
      result = make_shared<Table>();
      result->addColumn(_T("MAC_ADDRESS"), DCI_DT_STRING, _T("Mac address"), true);
      result->addColumn(_T("IP_ADDRESS"), DCI_DT_STRING, _T("IP address"));
      result->addColumn(_T("NODE_ID"), DCI_DT_UINT, _T("Node id"));
      result->addColumn(_T("NODE_NAME"), DCI_DT_STRING, _T("Node name"));
      result->addColumn(_T("ZONE_UIN"), DCI_DT_INT, _T("Zone UIN"));
      result->addColumn(_T("ZONE_NAME"), DCI_DT_STRING, _T("Zone name"));
      result->addColumn(_T("AP_ID"), DCI_DT_STRING, _T("Access point id"));
      result->addColumn(_T("AP"), DCI_DT_STRING, _T("Access point"));
      result->addColumn(_T("RADIO"), DCI_DT_STRING, _T("Radio"));
      result->addColumn(_T("RADIO_INDEX"), DCI_DT_UINT, _T("Radio index"));
      result->addColumn(_T("SSID"), DCI_DT_STRING, _T("SSID"));
      result->addColumn(_T("VLAN"), DCI_DT_UINT, _T("VLAN"));

      for(int i = 0; i < m_wirelessStations->size(); i++)
      {
         result->addRow();
         WirelessStationInfo *ws = m_wirelessStations->get(i);

         TCHAR mac[64];
         result->set(0, MACToStr(ws->macAddr, mac));
         result->set(1, ws->ipAddr.toString());
         result->set(2, ws->nodeId);
         shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectById(ws->nodeId, OBJECT_NODE));
         if (node != nullptr)
         {
            result->set(3, node->getName());
            result->set(4, node->getZoneUIN());
            shared_ptr<Zone> zone = FindZoneByUIN(node->getZoneUIN());
            if (zone != nullptr)
            {
               result->set(5, zone->getName());
            }
            else
            {
               result->set(5, _T(""));
            }
         }
         else
         {
            result->set(3, _T(""));
            result->set(4, _T(""));
            result->set(5, _T(""));
         }
         result->set(6, ws->apObjectId);
         shared_ptr<NetObj> accessPoint = FindObjectById(ws->apObjectId, OBJECT_ACCESSPOINT);
         if (accessPoint != nullptr)
         {
            result->set(7, accessPoint->getName());
         }
         else
         {
            result->set(7, _T(""));
         }
         result->set(8, ws->rfName);
         result->set(9, (UINT32)ws->rfIndex);
         result->set(10, ws->ssid);
         result->set(11, (WORD)ws->vlan);
      }
   }
   unlockProperties();
   return result;
}

/**
 * Write list of registered wireless stations to NXCP message
 */
void Node::writeWsListToMessage(NXCPMessage *msg, uint32_t apId)
{
   lockProperties();
   if (m_wirelessStations != nullptr)
   {
      uint32_t fieldId = VID_ELEMENT_LIST_BASE;
      uint32_t count = 0;
      for(int i = 0; i < m_wirelessStations->size(); i++)
      {
         WirelessStationInfo *ws = m_wirelessStations->get(i);
         if ((apId == 0) || (apId == ws->apObjectId))
         {
            ws->fillMessage(msg, fieldId);
            fieldId += 10;
            count++;
         }
      }
      msg->setField(VID_NUM_ELEMENTS, count);
   }
   else
   {
      msg->setField(VID_NUM_ELEMENTS, static_cast<uint32_t>(0));
   }
   unlockProperties();
}

/**
 * Get wireless stations registered on this AP/controller.
 * Returned list must be destroyed by caller.
 */
ObjectArray<WirelessStationInfo> *Node::getWirelessStations(uint32_t apId) const
{
   ObjectArray<WirelessStationInfo> *wsList = nullptr;

   lockProperties();
   if ((m_wirelessStations != nullptr) && !m_wirelessStations->isEmpty())
   {
      wsList = new ObjectArray<WirelessStationInfo>((apId == 0) ? m_wirelessStations->size() : 0, 32, Ownership::True);
      for(int i = 0; i < m_wirelessStations->size(); i++)
      {
         WirelessStationInfo *ws = m_wirelessStations->get(i);
         if ((apId == 0) || (apId == ws->apObjectId))
         {
            wsList->add(new WirelessStationInfo(*ws));
         }
      }
   }
   unlockProperties();
   return wsList;
}

/**
 * Get wireless stations registered on this AP/controller as NXSL objects.
 */
NXSL_Value *Node::getWirelessStationsForNXSL(NXSL_VM *vm, uint32_t apId) const
{
   NXSL_Array *wsList = new NXSL_Array(vm);

   lockProperties();
   if ((m_wirelessStations != nullptr) && !m_wirelessStations->isEmpty())
   {
      for(int i = 0; i < m_wirelessStations->size(); i++)
      {
         WirelessStationInfo *ws = m_wirelessStations->get(i);
         if ((apId == 0) || (apId == ws->apObjectId))
         {
            wsList->append(vm->createValue(vm->createObject(&g_nxslWirelessStationClass, new WirelessStationInfo(*ws))));
         }
      }
   }
   unlockProperties();

   return vm->createValue(wsList);
}

/**
 * Get access points from wireless controller
 */
ObjectArray<AccessPointInfo> *Node::getAccessPoints()
{
   SNMP_Transport *snmp = createSnmpTransport();
   if (snmp == nullptr)
      return nullptr;
   ObjectArray<AccessPointInfo> *accessPoints = m_driver->getAccessPoints(snmp, this, m_driverData);
   if (accessPoints != nullptr)
   {
      for(AccessPointInfo *ap : *accessPoints)
         ap->setControllerId(m_id);
   }
   delete snmp;
   return accessPoints;
}

/**
 * Get number of access points managed by this controller
 */
uint32_t Node::getAccessPointCount() const
{
   shared_ptr<WirelessDomain> wirelessDomain = getWirelessDomain();
   if (wirelessDomain == nullptr)
      return 0;

   return wirelessDomain->getChildrenCount(
      [this] (const NetObj& object) -> bool
      {
         return (object.getObjectClass() == OBJECT_ACCESSPOINT) &&
            (static_cast<const AccessPoint&>(object).getControllerId() == m_id);
      });
}

/**
 * Get number of access points managed by this controller in given state
 */
uint32_t Node::getAccessPointCount(AccessPointState state) const
{
   shared_ptr<WirelessDomain> wirelessDomain = getWirelessDomain();
   if (wirelessDomain == nullptr)
      return 0;

   return wirelessDomain->getChildrenCount(
      [state, this] (const NetObj& object) -> bool
      {
         return (object.getObjectClass() == OBJECT_ACCESSPOINT) &&
            (static_cast<const AccessPoint&>(object).getControllerId() == m_id) &&
            (static_cast<const AccessPoint&>(object).getApState() == state);
      });
}

/**
 * Get radio name
 */
void Node::getRadioName(uint32_t rfIndex, TCHAR *buffer, size_t bufSize) const
{
   buffer[0] = 0;
   lockProperties();
   if (m_radioInterfaces != nullptr)
   {
      for(int i = 0; i < m_radioInterfaces->size(); i++)
      {
         RadioInterfaceInfo *rif = m_radioInterfaces->get(i);
         if (rif->index == rfIndex)
         {
            _tcslcpy(buffer, rif->name, bufSize);
            break;
         }
      }
   }
   unlockProperties();
}

/**
 * Synchronize data collection settings with agent
 */
void Node::syncDataCollectionWithAgent(AgentConnectionEx *conn)
{
   NXCPMessage msg(CMD_DATA_COLLECTION_CONFIG, conn->generateRequestId(), conn->getProtocolVersion());
   msg.setField(VID_EXTENDED_DCI_DATA, true);

   if (IsZoningEnabled())
   {
      shared_ptr<Zone> zone = FindZoneByProxyId(m_id);
      if (zone != nullptr)
      {
         msg.setField(VID_THIS_PROXY_ID, m_id);
         zone->fillAgentConfigurationMessage(&msg);
      }
   }

   if (m_status != STATUS_UNMANAGED)
   {
      uint32_t count = 0;
      uint32_t baseInfoFieldId = VID_ELEMENT_LIST_BASE;
      uint32_t extraInfoFieldId = VID_EXTRA_DCI_INFO_BASE;
      readLockDciAccess();
      for(int i = 0; i < m_dcObjects.size(); i++)
      {
         DCObject *dco = m_dcObjects.get(i);
         if ((dco->getStatus() != ITEM_STATUS_DISABLED) &&
             dco->hasValue() &&
             (dco->getAgentCacheMode() == AGENT_CACHE_ON) &&
             (dco->getSourceNode() == 0))
         {
            msg.setField(baseInfoFieldId++, dco->getId());
            msg.setField(baseInfoFieldId++, static_cast<int16_t>(dco->getType()));
            msg.setField(baseInfoFieldId++, static_cast<int16_t>(dco->getDataSource()));
            msg.setField(baseInfoFieldId++, dco->getName());
            msg.setField(baseInfoFieldId++, dco->getEffectivePollingInterval());
            msg.setFieldFromTime(baseInfoFieldId++, dco->getLastPollTime().asTime()); // convert to seconds for compatibility with older agents
            baseInfoFieldId += 4;
            msg.setField(extraInfoFieldId + 1, dco->getLastPollTime());
            extraInfoFieldId += 1000;
            if (dco->isAdvancedSchedule())
            {
               dco->fillSchedulingDataMessage(&msg, extraInfoFieldId);
            }
            extraInfoFieldId += 100;
            count++;
         }
      }
      unlockDciAccess();

      nxlog_debug_tag(DEBUG_TAG_DC_AGENT_CACHE, 6, _T("Node::syncDataCollectionWithAgent(%s [%u]): %d local metrics"), m_name, m_id, count);

      ProxyInfo data;
      data.proxyId = m_id;
      data.count = count;
      data.msg = &msg;
      data.baseInfoFieldId = baseInfoFieldId;
      data.extraInfoFieldId = extraInfoFieldId;
      data.nodeInfoCount = 0;
      data.nodeInfoFieldId = VID_NODE_INFO_LIST_BASE;

      g_idxAccessPointById.forEach(super::collectProxyInfoCallback, &data);
      g_idxChassisById.forEach(super::collectProxyInfoCallback, &data);
      g_idxNodeById.forEach(super::collectProxyInfoCallback, &data);

      nxlog_debug_tag(DEBUG_TAG_DC_AGENT_CACHE, 6, _T("Node::syncDataCollectionWithAgent(%s [%u]): %d proxied metrics"), m_name, m_id, data.count - count);

      msg.setField(VID_NUM_ELEMENTS, data.count);
      msg.setField(VID_NUM_NODES, data.nodeInfoCount);
   }
   else
   {
      msg.setField(VID_NUM_ELEMENTS, 0);
      msg.setField(VID_NUM_NODES, 0);
   }

   uint32_t rcc;
   NXCPMessage *response = conn->customRequest(&msg);
   if (response != nullptr)
   {
      rcc = response->getFieldAsUInt32(VID_RCC);
      delete response;
   }
   else
   {
      rcc = ERR_REQUEST_TIMEOUT;
   }

   if (rcc == ERR_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG_DC_AGENT_CACHE, 4, _T("Node::syncDataCollectionWithAgent: node %s [%u] synchronized"), m_name, m_id);
      m_state &= ~NSF_CACHE_MODE_NOT_SUPPORTED;
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_DC_AGENT_CACHE, 4, _T("Node::syncDataCollectionWithAgent: node %s [%u] not synchronized (%s)"), m_name, m_id, AgentErrorCodeToText(rcc));
      if ((rcc == ERR_UNKNOWN_COMMAND) || (rcc == ERR_NOT_IMPLEMENTED))
      {
         m_state |= NSF_CACHE_MODE_NOT_SUPPORTED;
      }
   }
}

/**
 * Fully removes all DCI configuration from node
 */
void Node::clearDataCollectionConfigFromAgent(AgentConnectionEx *conn)
{
   NXCPMessage msg(CMD_CLEAN_AGENT_DCI_CONF, conn->generateRequestId(), conn->getProtocolVersion());
   NXCPMessage *response = conn->customRequest(&msg);
   if (response != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_DC_AGENT_CACHE, 4, _T("Node::clearDataCollectionConfigFromAgent: DCI configuration successfully removed from node %s [%d]"), m_name, (int)m_id);
      delete response;
   }
}

/**
 * Callback for async data collection synchronization with agent
 */
void Node::dataCollectionSyncCallback()
{
   InterlockedDecrement(&m_pendingDataConfigurationSync);

   agentLock();
   bool newConnection;
   if (connectToAgent(nullptr, nullptr, &newConnection))
   {
      if (!newConnection)
         syncDataCollectionWithAgent(m_agentConnection.get());
   }
   agentUnlock();
}

/**
 * Schedule data collection synchronization with agent
 */
void Node::scheduleDataCollectionSyncWithAgent()
{
   if (!(m_capabilities & NC_IS_NATIVE_AGENT))
      return;

   if (InterlockedIncrement(&m_pendingDataConfigurationSync) == 1)
   {
      nxlog_debug_tag(DEBUG_TAG_DC_AGENT_CACHE, 5, _T("Node::onDataCollectionChange(%s [%u]): scheduling data collection sync"), m_name, m_id);
      ThreadPoolScheduleRelative(g_pollerThreadPool, 30000, self(), &Node::dataCollectionSyncCallback); // wait for possible subsequent update requests within 30 seconds
   }
   else
   {
      // data collection configuration update already scheduled
      InterlockedDecrement(&m_pendingDataConfigurationSync);
      nxlog_debug_tag(DEBUG_TAG_DC_AGENT_CACHE, 7, _T("Node::onDataCollectionChange(%s [%u]): configuration upload already scheduled"), m_name, m_id);
   }
}

/**
 * Update proxy node data collection configuration if required
 */
void Node::updateProxyDataCollectionConfiguration(uint32_t proxyId, const TCHAR *proxyName)
{
   if (proxyId == 0)
      return;

   shared_ptr<Node> proxy = static_pointer_cast<Node>(FindObjectById(proxyId, OBJECT_NODE));
   if (proxy == nullptr)
      return;

   nxlog_debug_tag(DEBUG_TAG_DC_AGENT_CACHE, 5, _T("Node::updateProxyDataCollectionConfiguration(%s [%u]): scheduling data collection sync for %s proxy %s [%u]"),
         m_name, m_id, proxyName, proxy->getName(), proxy->getId());
   scheduleDataCollectionSyncWithAgent();
}

/**
 * Called when data collection configuration changed
 */
void Node::onDataCollectionChange()
{
   super::onDataCollectionChange();

   scheduleDataCollectionSyncWithAgent();

   updateProxyDataCollectionConfiguration(getEffectiveSnmpProxy(ProxySelection::PRIMARY), _T("SNMP"));
   updateProxyDataCollectionConfiguration(getEffectiveSnmpProxy(ProxySelection::BACKUP), _T("backup SNMP"));
   updateProxyDataCollectionConfiguration(getEffectiveModbusProxy(ProxySelection::PRIMARY), _T("Modbus"));
   updateProxyDataCollectionConfiguration(getEffectiveModbusProxy(ProxySelection::BACKUP), _T("backup Modbus"));
}

/**
 * Update physical container (rack or chassis) binding
 */
void Node::updatePhysicalContainerBinding(uint32_t containerId)
{
   bool containerFound = false;
   SharedObjectArray<NetObj> deleteList;

   readLockParentList();
   for(int i = 0; i < getParentList().size(); i++)
   {
      NetObj *object = getParentList().get(i);
      if ((object->getObjectClass() != OBJECT_RACK) && (object->getObjectClass() != OBJECT_CHASSIS))
         continue;
      if (object->getId() == containerId)
      {
         containerFound = true;
         continue;
      }
      deleteList.add(getParentList().getShared(i));
   }
   unlockParentList();

   for(int n = 0; n < deleteList.size(); n++)
   {
      NetObj *container = deleteList.get(n);
      nxlog_debug(5, _T("Node::updatePhysicalContainerBinding(%s [%d]): delete incorrect binding %s [%d]"), m_name, m_id, container->getName(), container->getId());
      unlinkObjects(container, this);
   }

   if (containerId == 0)
      return;

   if (!containerFound)
   {
      shared_ptr<NetObj> container = FindObjectById(containerId);
      if (container == nullptr)
      {
         nxlog_debug(5, _T("Node::updatePhysicalContainerBinding(%s [%d]): object [%d] not found"),
                     m_name, m_id, containerId);
      }
      else if ((container->getObjectClass() != OBJECT_RACK) && (container->getObjectClass() != OBJECT_CHASSIS))
      {
         nxlog_debug(5, _T("Node::updatePhysicalContainerBinding(%s [%d]): incorrect object %s [%d] class"),
                     m_name, m_id, container->getName(), containerId);
      }
      else
      {
         nxlog_debug(5, _T("Node::updatePhysicalContainerBinding(%s [%d]): add binding %s [%d]"), m_name, m_id, container->getName(), container->getId());
         linkObjects(container, self());
      }
   }
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *Node::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(vm->createObject(&g_nxslNodeClass, new shared_ptr<Node>(self())));
}

/**
 * Increase number of received syslog messages
 */
void Node::incSyslogMessageCount()
{
   lockProperties();
   m_syslogMessageCount++;
   unlockProperties();
}

/**
 * Increase number of received SNMP traps
 */
void Node::incSnmpTrapCount()
{
   lockProperties();
   m_snmpTrapCount++;
   unlockProperties();
}

/**
 * Increase number of received SNMP traps
 */
bool Node::checkTrapShouldBeProcessed()
{
   lockProperties();
   bool dropSNMPTrap = m_state & NSF_SNMP_TRAP_FLOOD;
   unlockProperties();
   time_t now = time(nullptr);
   if ((g_snmpTrapStormCountThreshold != 0) && (now > m_snmpTrapStormLastCheckTime)) // If last check was more than second ago
   {
      m_snmpTrapStormLastCheckTime = time(nullptr);
      auto newDiff = static_cast<uint32_t>(m_snmpTrapCount - m_snmpTrapLastTotal);
      m_snmpTrapLastTotal = m_snmpTrapCount;
      if (newDiff > g_snmpTrapStormCountThreshold)
      {
         if (m_snmpTrapStormActualDuration >= g_snmpTrapStormDurationThreshold)
         {
            if (!dropSNMPTrap)
            {
               nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_SNMP_TRAP_FLOOD, _T("SNMP trap flood detected for node %s [%u]: threshold=%u eventsPerSecond=%d"), m_name, m_id, g_snmpTrapStormCountThreshold, newDiff);
               EventBuilder(EVENT_SNMP_TRAP_FLOOD_DETECTED, m_id)
                  .param(_T("snmpTrapsPerSecond"), newDiff)
                  .param(_T("duration"), g_snmpTrapStormDurationThreshold)
                  .param(_T("threshold"), g_snmpTrapStormCountThreshold)
                  .post();

               lockProperties();
               m_state |= NSF_SNMP_TRAP_FLOOD;
               setModified(MODIFY_NODE_PROPERTIES);
               unlockProperties();
            }
            dropSNMPTrap = true;
         }
         m_snmpTrapStormActualDuration++;
      }
      else if (m_snmpTrapStormActualDuration != 0)
      {
         nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_SNMP_TRAP_FLOOD, _T("SNMP trap flood condition cleared for node %s [%u]"), m_name, m_id);
         EventBuilder(EVENT_SNMP_TRAP_FLOOD_ENDED, m_id)
            .param(_T("snmpTrapsPerSecond"), newDiff)
            .param(_T("duration"), g_snmpTrapStormDurationThreshold)
            .param(_T("threshold"), g_snmpTrapStormCountThreshold)
            .post();
         m_snmpTrapStormActualDuration = 0;
         lockProperties();
         m_state &= ~NSF_SNMP_TRAP_FLOOD;
         setModified(MODIFY_NODE_PROPERTIES);
         unlockProperties();
         dropSNMPTrap = false;
      }
   }

   return !dropSNMPTrap;
}

/**
 * Collect info for SNMP proxy and DCI source (proxy) nodes
 */
void Node::collectProxyInfo(ProxyInfo *info)
{
   if ((m_status == STATUS_UNMANAGED) || (m_state & DCSF_UNREACHABLE))
      return;

   uint32_t primarySnmpProxy = getEffectiveSnmpProxy(ProxySelection::PRIMARY);
   bool snmpProxy = (primarySnmpProxy == info->proxyId);
   bool backupSnmpProxy = (getEffectiveSnmpProxy(ProxySelection::BACKUP) == info->proxyId);
   bool isTarget = false;

   uint32_t primaryModbusProxy = getEffectiveModbusProxy(ProxySelection::PRIMARY);
   bool modbusProxy = (primaryModbusProxy == info->proxyId);
   bool backupModbusProxy = (getEffectiveModbusProxy(ProxySelection::BACKUP) == info->proxyId);

   nxlog_debug_tag(DEBUG_TAG_DC_AGENT_CACHE, 5, _T("Node::collectProxyInfo(%s [%u]): proxyId=%u snmp=%s backup-snmp=%s modbus=%s backup-modbus=%s"),
      m_name, m_id, info->proxyId, BooleanToString(snmpProxy), BooleanToString(backupSnmpProxy), BooleanToString(modbusProxy), BooleanToString(backupModbusProxy));

   readLockDciAccess();
   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      DCObject *dco = m_dcObjects.get(i);
      if (dco->getStatus() == ITEM_STATUS_DISABLED)
         continue;

      if ((((snmpProxy || backupSnmpProxy) && (dco->getDataSource() == DS_SNMP_AGENT) && (dco->getSourceNode() == 0)) ||
           ((modbusProxy || backupModbusProxy) && (dco->getDataSource() == DS_MODBUS) && (dco->getSourceNode() == 0)) ||
           ((dco->getDataSource() == DS_NATIVE_AGENT) && (dco->getSourceNode() == info->proxyId))) &&
          dco->hasValue() && (dco->getAgentCacheMode() == AGENT_CACHE_ON))
      {
         switch(dco->getDataSource())
         {
            case DS_NATIVE_AGENT:
               addProxyDataCollectionElement(info, dco, 0);
               break;
            case DS_SNMP_AGENT:
               addProxyDataCollectionElement(info, dco, backupSnmpProxy ? primarySnmpProxy : 0);
               isTarget = true;
               break;
            case DS_MODBUS:
               addProxyDataCollectionElement(info, dco, backupModbusProxy ? primaryModbusProxy : 0);
               break;
         }
      }
   }
   unlockDciAccess();

   if (isTarget)
      addProxySnmpTarget(info, this);
}

/**
 * Get node type name from type
 */
const TCHAR *Node::typeName(NodeType type)
{
   static const TCHAR *names[] = { _T("Unknown"), _T("Physical"), _T("Virtual"), _T("Controller"), _T("Container") };
   return (((int)type >= 0) && ((int)type < sizeof(names) / sizeof(const TCHAR *))) ? names[type] : names[0];
}

/**
 * Set SSH credentials for node
 */
void Node::setSshCredentials(const TCHAR *login, const TCHAR *password)
{
   lockProperties();
   m_sshLogin = login;
   m_sshPassword = password;
   setModified(MODIFY_NODE_PROPERTIES);
   unlockProperties();
}

/**
 * Check if compression allowed for agent
 */
bool Node::isAgentCompressionAllowed()
{
   if (m_agentCompressionMode == NODE_AGENT_COMPRESSION_DEFAULT)
      return ConfigReadInt(_T("Agent.DefaultProtocolCompressionMode"), NODE_AGENT_COMPRESSION_ENABLED) == NODE_AGENT_COMPRESSION_ENABLED;
   return m_agentCompressionMode == NODE_AGENT_COMPRESSION_ENABLED;
}

/**
 * Set routing loop event information
 */
void Node::setRoutingLoopEvent(const InetAddress& address, uint32_t nodeId, uint64_t eventId)
{
   for(int i = 0; i < m_routingLoopEvents->size(); i++)
   {
      RoutingLoopEvent *e = m_routingLoopEvents->get(i);
      if ((e->getNodeId() == nodeId) || e->getAddress().equals(address))
      {
         m_routingLoopEvents->remove(i);
         break;
      }
   }
   m_routingLoopEvents->add(new RoutingLoopEvent(address, nodeId, eventId));
}

/**
 * Set tunnel ID
 */
void Node::setTunnelId(const uuid& tunnelId, const TCHAR *certSubject)
{
   bool updated = false;

   lockProperties();
   if (!m_tunnelId.equals(tunnelId) || _tcscmp(CHECK_NULL_EX(certSubject), CHECK_NULL_EX(m_agentCertSubject)))
   {
      m_tunnelId = tunnelId;
      MemFree(m_agentCertSubject);
      m_agentCertSubject = MemCopyString(certSubject);
      setModified(MODIFY_NODE_PROPERTIES, false);
      updated = true;
   }
   unlockProperties();

   if (updated)
   {
      if (!tunnelId.isNull())
      {
         TCHAR buffer[128];
         nxlog_debug_tag(DEBUG_TAG_AGENT, 4, _T("Tunnel ID for node %s [%d] set to %s"), m_name, m_id, tunnelId.toString(buffer));
      }
      if (certSubject != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_AGENT, 4, _T("Agent certificate subject for node %s [%d] set to %s"), m_name, m_id, certSubject);
      }
   }
}

/**
 * Checks if proxy has been set and links objects, if not set, creates new server link or edits existing
 */
bool Node::checkProxyAndLink(NetworkMapObjectList *map, uint32_t seedNode, uint32_t proxyId, uint32_t linkType, const TCHAR *linkName, bool checkAllProxies)
{
   if ((proxyId == 0) || (proxyId == g_dwMgmtNode))
      return false;

   if (IsZoningEnabled() && (m_zoneUIN != 0) && (linkType != LINK_TYPE_ZONE_PROXY))
   {
      shared_ptr<Zone> zone = FindZoneByUIN(m_zoneUIN);
      if ((zone != nullptr) && zone->isProxyNode(proxyId))
         return false;
   }

   shared_ptr<Node> proxy = static_pointer_cast<Node>(FindObjectById(proxyId, OBJECT_NODE));
   if (proxy != nullptr && proxy->getId() != m_id)
   {
      ObjLink *link = map->getLink(m_id, proxyId, linkType);
      if (link != nullptr)
         return true;

      map->addObject(proxyId);
      map->linkObjects(m_id, proxyId, linkType, linkName);
      proxy->populateInternalCommunicationTopologyMap(map, seedNode, !checkAllProxies, checkAllProxies);
   }
   return true;
}

/**
 * Build internal connection topology - internal function
 */
void Node::populateInternalCommunicationTopologyMap(NetworkMapObjectList *map, uint32_t currentObjectId, bool agentConnectionOnly, bool checkAllProxies)
{
   if ((map->getNumObjects() != 0) && (currentObjectId == m_id))
      return;

   map->addObject(m_id);

   if (IsZoningEnabled() && (m_zoneUIN != 0))
   {
      shared_ptr<Zone> zone = FindZoneByUIN(m_zoneUIN);
      if (zone != nullptr)
      {
         IntegerArray<uint32_t> proxies = zone->getAllProxyNodes();
         for(int i = 0; i < proxies.size(); i++)
         {
            checkProxyAndLink(map, currentObjectId, proxies.get(i), LINK_TYPE_ZONE_PROXY, _T("Zone proxy"), checkAllProxies);
         }
      }
   }

   StringBuffer protocols;
	if (!m_tunnelId.equals(uuid::NULL_UUID))
	{
	   map->addObject(FindLocalMgmtNode());
	   map->linkObjects(m_id, FindLocalMgmtNode(), LINK_TYPE_AGENT_TUNNEL, _T("Agent tunnel"));
	}
	else if (!checkProxyAndLink(map, currentObjectId, getEffectiveAgentProxy(), LINK_TYPE_AGENT_PROXY, _T("Agent proxy"), checkAllProxies))
	{
	   protocols.append(_T("Agent "));
	}

	// Only agent connection necessary for proxy nodes
	// Only if IP is set while using agent tunnel
	if (!agentConnectionOnly && _tcscmp(m_primaryHostName, _T("0.0.0.0")))
	{
      if (!checkProxyAndLink(map, currentObjectId, getEffectiveIcmpProxy(), LINK_TYPE_ICMP_PROXY, _T("ICMP proxy"), checkAllProxies))
         protocols.append(_T("ICMP "));
      if (!checkProxyAndLink(map, currentObjectId, getEffectiveSnmpProxy(), LINK_TYPE_SNMP_PROXY, _T("SNMP proxy"), checkAllProxies))
         protocols.append(_T("SNMP "));
      if (!checkProxyAndLink(map, currentObjectId, getEffectiveSshProxy(), LINK_TYPE_SSH_PROXY, _T("SSH proxy"), checkAllProxies))
         protocols.append(_T("SSH"));
	}

	if (!protocols.isEmpty() || agentConnectionOnly)
	{
	   bool inSameZone = true;
      if (IsZoningEnabled())
      {
         shared_ptr<Node> server = static_pointer_cast<Node>(FindObjectById(FindLocalMgmtNode(), OBJECT_NODE));
         if ((server != nullptr) && (server->getZoneUIN() != m_zoneUIN))
            inSameZone = false;
      }
      if (inSameZone)
      {
         map->addObject(FindLocalMgmtNode());
         map->linkObjects(m_id, FindLocalMgmtNode(), LINK_TYPE_NORMAL, (agentConnectionOnly ? _T("Direct") : (const TCHAR *)protocols));
      }
	}
}

/**
 * Node type names
 */
static const char *s_nodeTypeNames[] =
{
   "Unknown",
   "Physical",
   "Virtual",
   "Controller",
   "Container"
};

/**
 * Serialize object to JSON
 */
json_t *Node::toJson()
{
   json_t *root = super::toJson();
   lockProperties();
   json_object_set_new(root, "ipAddress", m_ipAddress.toJson());
   json_object_set_new(root, "primaryName", json_string_t(m_primaryHostName));
   json_object_set_new(root, "tunnelId", m_tunnelId.toJson());

   // Capability flags JSON object with boolean attributes
   json_t *capabilities = json_object();
   json_object_set_new(capabilities, "isSnmp", json_boolean(m_capabilities & NC_IS_SNMP));
   json_object_set_new(capabilities, "isNativeAgent", json_boolean(m_capabilities & NC_IS_NATIVE_AGENT));
   json_object_set_new(capabilities, "isBridge", json_boolean(m_capabilities & NC_IS_BRIDGE));
   json_object_set_new(capabilities, "isRouter", json_boolean(m_capabilities & NC_IS_ROUTER));
   json_object_set_new(capabilities, "isLocalMgmt", json_boolean(m_capabilities & NC_IS_LOCAL_MGMT));
   json_object_set_new(capabilities, "isPrinter", json_boolean(m_capabilities & NC_IS_PRINTER));
   json_object_set_new(capabilities, "isOspf", json_boolean(m_capabilities & NC_IS_OSPF));
   json_object_set_new(capabilities, "isSsh", json_boolean(m_capabilities & NC_IS_SSH));
   json_object_set_new(capabilities, "isCdp", json_boolean(m_capabilities & NC_IS_CDP));
   json_object_set_new(capabilities, "isNdp", json_boolean(m_capabilities & NC_IS_NDP));
   json_object_set_new(capabilities, "isLldp", json_boolean(m_capabilities & NC_IS_LLDP));
   json_object_set_new(capabilities, "isVrrp", json_boolean(m_capabilities & NC_IS_VRRP));
   json_object_set_new(capabilities, "hasVlans", json_boolean(m_capabilities & NC_HAS_VLANS));
   json_object_set_new(capabilities, "is8021x", json_boolean(m_capabilities & NC_IS_8021X));
   json_object_set_new(capabilities, "isStp", json_boolean(m_capabilities & NC_IS_STP));
   json_object_set_new(capabilities, "hasEntityMib", json_boolean(m_capabilities & NC_HAS_ENTITY_MIB));
   json_object_set_new(capabilities, "hasIfxTable", json_boolean(m_capabilities & NC_HAS_IFXTABLE));
   json_object_set_new(capabilities, "hasAgentIfxCounters", json_boolean(m_capabilities & NC_HAS_AGENT_IFXCOUNTERS));
   json_object_set_new(capabilities, "hasWinPdh", json_boolean(m_capabilities & NC_HAS_WINPDH));
   json_object_set_new(capabilities, "isWifiController", json_boolean(m_capabilities & NC_IS_WIFI_CONTROLLER));
   json_object_set_new(capabilities, "isSmclp", json_boolean(m_capabilities & NC_IS_SMCLP));
   json_object_set_new(capabilities, "isNewPolicyTypes", json_boolean(m_capabilities & NC_IS_NEW_POLICY_TYPES));
   json_object_set_new(capabilities, "hasUserAgent", json_boolean(m_capabilities & NC_HAS_USER_AGENT));
   json_object_set_new(capabilities, "isEthernetIp", json_boolean(m_capabilities & NC_IS_ETHERNET_IP));
   json_object_set_new(capabilities, "isModbusTcp", json_boolean(m_capabilities & NC_IS_MODBUS_TCP));
   json_object_set_new(capabilities, "isProfinet", json_boolean(m_capabilities & NC_IS_PROFINET));
   json_object_set_new(capabilities, "hasFileManager", json_boolean(m_capabilities & NC_HAS_FILE_MANAGER));
   json_object_set_new(capabilities, "lldpV2Mib", json_boolean(m_capabilities & NC_LLDP_V2_MIB));
   json_object_set_new(capabilities, "emulatedEntityMib", json_boolean(m_capabilities & NC_EMULATED_ENTITY_MIB));
   json_object_set_new(capabilities, "deviceView", json_boolean(m_capabilities & NC_DEVICE_VIEW));
   json_object_set_new(capabilities, "isWifiAp", json_boolean(m_capabilities & NC_IS_WIFI_AP));
   json_object_set_new(capabilities, "isVnc", json_boolean(m_capabilities & NC_IS_VNC));
   json_object_set_new(capabilities, "isLocalVnc", json_boolean(m_capabilities & NC_IS_LOCAL_VNC));
   json_object_set_new(capabilities, "registeredForBackup", json_boolean(m_capabilities & NC_REGISTERED_FOR_BACKUP));
   json_object_set_new(capabilities, "hasServiceManager", json_boolean(m_capabilities & NC_HAS_SERVICE_MANAGER));
   json_object_set_new(root, "capabilities", capabilities);

   // State flags JSON object with boolean attributes
   json_t *state = json_object();
   json_object_set_new(state, "agentUnreachable", json_boolean(m_state & NSF_AGENT_UNREACHABLE));
   json_object_set_new(state, "snmpUnreachable", json_boolean(m_state & NSF_SNMP_UNREACHABLE));
   json_object_set_new(state, "ethernetIpUnreachable", json_boolean(m_state & NSF_ETHERNET_IP_UNREACHABLE));
   json_object_set_new(state, "cacheModeNotSupported", json_boolean(m_state & NSF_CACHE_MODE_NOT_SUPPORTED));
   json_object_set_new(state, "snmpTrapFlood", json_boolean(m_state & NSF_SNMP_TRAP_FLOOD));
   json_object_set_new(state, "icmpUnreachable", json_boolean(m_state & NSF_ICMP_UNREACHABLE));
   json_object_set_new(state, "sshUnreachable", json_boolean(m_state & NSF_SSH_UNREACHABLE));
   json_object_set_new(state, "modbusUnreachable", json_boolean(m_state & NSF_MODBUS_UNREACHABLE));
   json_object_set_new(state, "unreachable", json_boolean(m_state & DCSF_UNREACHABLE));
   json_object_set_new(state, "networkPathProblem", json_boolean(m_state & DCSF_NETWORK_PATH_PROBLEM));
   json_object_set_new(root, "state", state);

   json_object_set_new(root, "capabilityFlags", json_integer(m_capabilities));
   json_object_set_new(root, "stateFlags", json_integer(m_state));
   json_object_set_new(root, "type", json_string(s_nodeTypeNames[(int)m_type]));
   json_object_set_new(root, "subType", json_string_t(m_subType));
   json_object_set_new(root, "pendingState", json_integer(m_pendingState));
   json_object_set_new(root, "pollCountSNMP", json_integer(m_pollCountSNMP));
   json_object_set_new(root, "agentCacheMode", json_integer(m_agentCacheMode));
   json_object_set_new(root, "pollCountAgent", json_integer(m_pollCountAgent));
   json_object_set_new(root, "requiredPollCount", json_integer(m_requiredPollCount));
   json_object_set_new(root, "zoneUIN", json_integer(m_zoneUIN));
   json_object_set_new(root, "agentPort", json_integer(m_agentPort));
   json_object_set_new(root, "agentCacheMode", json_integer(m_agentCacheMode));
   json_object_set_new(root, "agentCompressionMode", json_integer(m_agentCompressionMode));
   json_object_set_new(root, "agentSecret", json_string_t(m_agentSecret));
   json_object_set_new(root, "snmpVersion", json_integer(m_snmpVersion));
   json_object_set_new(root, "snmpPort", json_integer(m_snmpPort));
   json_object_set_new(root, "useIfXTable", json_integer(m_nUseIfXTable));
   json_object_set_new(root, "snmpSecurity", (m_snmpSecurity != nullptr) ? m_snmpSecurity->toJson() : json_object());
   json_object_set_new(root, "agentVersion", json_string_t(m_agentVersion));
   json_object_set_new(root, "platformName", json_string_t(m_platformName));
   json_object_set_new(root, "snmpObjectId", json_string_t(m_snmpObjectId.toString()));
   json_object_set_new(root, "sysDescription", json_string_t(m_sysDescription));
   json_object_set_new(root, "sysName", json_string_t(m_sysName));
   json_object_set_new(root, "sysLocation", json_string_t(m_sysLocation));
   json_object_set_new(root, "sysContact", json_string_t(m_sysContact));
   json_object_set_new(root, "lldpNodeId", json_string_t(m_lldpNodeId));
   json_object_set_new(root, "driverName", json_string_t(m_driver->getName()));
   json_object_set_new(root, "downSince", json_time_string(m_downSince));
   json_object_set_new(root, "bootTime", json_time_string(m_bootTime));
   json_object_set_new(root, "pollerNodeId", json_integer(m_pollerNode));
   json_object_set_new(root, "agentProxyNodeId", json_integer(m_agentProxy));
   json_object_set_new(root, "snmpProxyNodeId", json_integer(m_snmpProxy));
   json_object_set_new(root, "icmpProxyNodeId", json_integer(m_icmpProxy));
   json_object_set_new(root, "lastEvents", json_integer_array(m_lastEvents, MAX_LAST_EVENTS));
   char baseBridgeAddrText[64];
   json_object_set_new(root, "baseBridgeAddress", json_string_a(BinToStrA(m_baseBridgeAddress, MAC_ADDR_LENGTH, baseBridgeAddrText)));
   json_object_set_new(root, "rackHeight", json_integer(m_rackHeight));
   json_object_set_new(root, "rackPosition", json_integer(m_rackPosition));
   json_object_set_new(root, "rackOrientation", json_integer(m_rackOrientation));
   json_object_set_new(root, "physicalContainerId", json_integer(m_physicalContainer));
   json_object_set_new(root, "rackImageFront", m_rackImageFront.toJson());
   json_object_set_new(root, "rackImageRear", m_rackImageRear.toJson());
   json_object_set_new(root, "syslogMessageCount", json_integer(m_syslogMessageCount));
   json_object_set_new(root, "snmpTrapCount", json_integer(m_snmpTrapCount));
   json_object_set_new(root, "sshLogin", json_string_t(m_sshLogin));
   json_object_set_new(root, "sshPassword", json_string_t(m_sshPassword));
   json_object_set_new(root, "sshKeyId", json_integer(m_sshKeyId));
   json_object_set_new(root, "sshPort", json_integer(m_sshPort));
   json_object_set_new(root, "sshProxy", json_integer(m_sshProxy));
   json_object_set_new(root, "portNumberingScheme", json_integer(m_portNumberingScheme));
   json_object_set_new(root, "portRowCount", json_integer(m_portRowCount));
   json_object_set_new(root, "icmpStatCollectionMode", json_integer((int)m_icmpStatCollectionMode));
   json_object_set_new(root, "icmpTargets", m_icmpTargets.toJson());
   json_object_set_new(root, "chassisPlacementConfig", json_string_t(m_chassisPlacementConf));
   json_object_set_new(root, "syslogCodepage", json_string_a(m_syslogCodepage));
   json_object_set_new(root, "modbusUnitId", json_integer(m_modbusUnitId));
   json_object_set_new(root, "modbusTCPPort", json_integer(m_modbusTcpPort));
   json_object_set_new(root, "modbusProxyNodeId", json_integer(m_modbusProxy));
   json_object_set_new(root, "vncPassword", json_string_t(m_vncPassword));
   json_object_set_new(root, "vncPort", json_integer(m_vncPort));
   json_object_set_new(root, "vncProxy", json_integer(m_vncProxy));
   unlockProperties();
   return root;
}

/**
 * ICMP poll target
 */
struct IcmpPollTarget
{
   wchar_t name[MAX_OBJECT_NAME];
   InetAddress address;

   IcmpPollTarget(const wchar_t *category, const wchar_t *_name, const InetAddress& _address)
   {
      if (category != nullptr)
      {
         wcslcpy(name, category, MAX_OBJECT_NAME);
         wcslcat(name, L":", MAX_OBJECT_NAME);
         if (_name != nullptr)
            wcslcat(name, _name, MAX_OBJECT_NAME);
         else
            _address.toString(&name[wcslen(name)]);
      }
      else
      {
         if (_name != nullptr)
            wcslcpy(name, _name, MAX_OBJECT_NAME);
         else
            _address.toString(name);
      }
      address = _address;
   }
};

/**
 * ICMP poll
 */
void Node::icmpPoll(PollerInfo *poller)
{
   int64_t startTime = GetCurrentTimeMs();

   // Prepare poll list
   StructArray<IcmpPollTarget> targets;

   lockProperties();
   if (m_ipAddress.isValidUnicast())
      targets.add(IcmpPollTarget(nullptr, _T("PRI"), m_ipAddress));
   for(int i = 0; i < m_icmpTargets.size(); i++)
      targets.add(IcmpPollTarget(_T("A"), nullptr, m_icmpTargets.get(i)));
   unlockProperties();

   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *curr = getChildList().get(i);
      if (curr->getStatus() == STATUS_UNMANAGED)
         continue;

      if (((curr->getObjectClass() == OBJECT_INTERFACE) && static_cast<Interface*>(curr)->isIncludedInIcmpPoll()) ||
          ((curr->getObjectClass() == OBJECT_ACCESSPOINT) && static_cast<AccessPoint*>(curr)->isIncludedInIcmpPoll()))
      {
         InetAddress addr = curr->getPrimaryIpAddress();
         if (addr.isValidUnicast())
            targets.add(IcmpPollTarget(_T("N"), curr->getName(), addr));
      }
   }
   unlockChildList();

   shared_ptr<Node> proxyNode;
   shared_ptr<AgentConnection> conn;
   uint32_t icmpProxy = getEffectiveIcmpProxy();
   if (icmpProxy != 0)
   {
      nxlog_debug_tag(DEBUG_TAG_ICMP_POLL, 7, _T("Node::icmpPoll(%s [%u]): ping via proxy [%u]"), m_name, m_id, icmpProxy);
      proxyNode = static_pointer_cast<Node>(g_idxNodeById.get(icmpProxy));
      if ((proxyNode == nullptr) || !proxyNode->isNativeAgent() || proxyNode->isDown())
      {
         nxlog_debug_tag(DEBUG_TAG_ICMP_POLL, 7, _T("Node::icmpPoll(%s [%u]): proxy node not available"), m_name, m_id);
         proxyNode.reset();
         goto end_poll;
      }
      nxlog_debug_tag(DEBUG_TAG_ICMP_POLL, 7, _T("Node::icmpPoll(%s [%u]): proxy node found: \"%s\" [%u]"), m_name, m_id, proxyNode->getName(), proxyNode->getId());
      conn = proxyNode->acquireProxyConnection(ProxyType::ICMP_PROXY);
      if (conn == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_ICMP_POLL, 7, _T("Node::icmpPoll(%s [%u]): cannot acquire connection to agent on proxy node \"%s\" [%u]"), m_name, m_id, proxyNode->getName(), proxyNode->getId());
         goto end_poll;
      }
   }

   for(int i = 0; i < targets.size(); i++)
   {
      const IcmpPollTarget *t = targets.get(i);
      icmpPollAddress(conn.get(), t->name, t->address);
   }

end_poll:
   m_icmpPollState.complete(GetCurrentTimeMs() - startTime);
}

/**
 * Poll specific address with ICMP
 */
void Node::icmpPollAddress(AgentConnection *conn, const TCHAR *target, const InetAddress& addr)
{
   TCHAR debugPrefix[256], buffer[64];
   _sntprintf(debugPrefix, 256, _T("Node::icmpPollAddress(%s [%u], %s, %s):"), m_name, m_id, target, addr.toString(buffer));

   uint32_t status = ICMP_SEND_FAILED, rtt = 0;
   if (conn != nullptr)
   {
      TCHAR parameter[128];
      _sntprintf(parameter, 128, _T("Icmp.Ping(%s)"), addr.toString(buffer));
      uint32_t rcc = conn->getParameter(parameter, buffer, 64);
      if (rcc == ERR_SUCCESS)
      {
         nxlog_debug_tag(DEBUG_TAG_ICMP_POLL, 7, _T("%s: proxy response: \"%s\""), debugPrefix, buffer);
         TCHAR *eptr;
         rtt = _tcstol(buffer, &eptr, 10);
         if (*eptr == 0)
         {
            status = ICMP_SUCCESS;
         }
      }
      else if (rcc == ERR_REQUEST_TIMEOUT)
      {
         status = ICMP_TIMEOUT;
         rtt = 10000;
      }
      nxlog_debug_tag(DEBUG_TAG_ICMP_POLL, 7, _T("%s: response time %u"), debugPrefix, rtt);
   }
   else  // not using ICMP proxy
   {
      nxlog_debug_tag(DEBUG_TAG_ICMP_POLL, 7, _T("%s: calling IcmpPing(%s,3,%d,%d)"),
               debugPrefix, addr.toString(buffer), g_icmpPingTimeout, g_icmpPingSize);
      status = IcmpPing(addr, 1, g_icmpPingTimeout, &rtt, g_icmpPingSize, false);
      nxlog_debug_tag(DEBUG_TAG_ICMP_POLL, 7, _T("%s: ping status=%u RTT=%u"), debugPrefix, status, rtt);
   }

   if ((status == ICMP_SUCCESS) || (status == ICMP_TIMEOUT) || (status == ICMP_UNREACHABLE))
   {
      lockProperties();

      if (m_icmpStatCollectors == nullptr)
         m_icmpStatCollectors = new StringObjectMap<IcmpStatCollector>(Ownership::True);

      IcmpStatCollector *collector = m_icmpStatCollectors->get(target);
      if (collector == nullptr)
      {
         collector = new IcmpStatCollector(ConfigReadInt(_T("ICMP.StatisticPeriod"), 60));
         m_icmpStatCollectors->set(target, collector);
         nxlog_debug_tag(DEBUG_TAG_ICMP_POLL, 7, _T("%s: new collector object created"), debugPrefix);
      }

      if (!_tcscmp(target, _T("PRI")))
      {
         uint32_t requiredPolls = (m_requiredPollCount > 0) ? m_requiredPollCount : g_requiredPolls;
         if (status != ICMP_SUCCESS)
         {
            if (!(m_state & NSF_ICMP_UNREACHABLE))
            {
               m_pollCountICMP++;
               if (m_pollCountICMP >= requiredPolls)
               {
                  m_state |= NSF_ICMP_UNREACHABLE;
                  m_pollCountICMP = 0;
                  PostSystemEvent(EVENT_ICMP_UNREACHABLE, m_id);
                  setModified(MODIFY_NODE_PROPERTIES);
                  m_pollCountICMP = 0;
               }
            }
            else
            {
               m_pollCountICMP = 0;
            }
         }
         else
         {
            if (m_state & NSF_ICMP_UNREACHABLE)
            {
               m_pollCountICMP++;
               if (m_pollCountICMP >= requiredPolls)
               {
                  m_state &= ~NSF_ICMP_UNREACHABLE;
                  m_pollCountICMP = 0;
                  PostSystemEvent(EVENT_ICMP_OK, m_id);
                  setModified(MODIFY_NODE_PROPERTIES);
               }
            }
            else
            {
               m_pollCountICMP = 0;
            }
         }
      }

      collector->update((status == ICMP_SUCCESS) ? rtt : 10000);

      unlockProperties();
   }
}

/**
 * Get all ICMP statistics for given target
 */
bool Node::getIcmpStatistics(const TCHAR *target, uint32_t *last, uint32_t *min, uint32_t *max, uint32_t *avg, uint32_t *loss, uint32_t *jitter) const
{
   lockProperties();
   IcmpStatCollector *collector = (m_icmpStatCollectors != nullptr) ? m_icmpStatCollectors->get(target) : nullptr;
   if ((collector != nullptr) && collector->empty())
      collector = nullptr;
   if (last != nullptr)
      *last = (collector != nullptr) ? collector->last() : 0;
   if (min != nullptr)
      *min = (collector != nullptr) ? collector->min() : 0;
   if (max != nullptr)
      *max = (collector != nullptr) ? collector->max() : 0;
   if (avg != nullptr)
      *avg = (collector != nullptr) ? collector->average() : 0;
   if (loss != nullptr)
      *loss = (collector != nullptr) ? collector->packetLoss() : 0;
   unlockProperties();
   return collector != nullptr;
}

/**
 * Get all ICMP statistic collectors
 */
StringList *Node::getIcmpStatCollectors() const
{
   lockProperties();
   StringList *collectors = (m_icmpStatCollectors != nullptr) ? new StringList(m_icmpStatCollectors->keys()) : new StringList();
   unlockProperties();
   return collectors;
}

/**
 * Get ICMP poll statistic for given target and function
 */
DataCollectionError Node::getIcmpStatistic(const wchar_t *param, IcmpStatFunction function, wchar_t *value) const
{
   wchar_t key[MAX_OBJECT_NAME + 2];
   if (param != nullptr)
   {
      wchar_t target[MAX_OBJECT_NAME];
      if (!AgentGetParameterArgW(param, 1, target, MAX_OBJECT_NAME))
         return DataCollectionError::DCE_NOT_SUPPORTED;

      if ((wcslen(target) >= 2) && (target[1] == ':'))
      {
         wcslcpy(key, target, MAX_OBJECT_NAME + 2);
      }
      else
      {
         InetAddress a = InetAddress::parse(target);
         if (a.isValid())
         {
            key[0] = 'A';
            key[1] = ':';
            a.toString(&key[2]);
         }
         else
         {
            shared_ptr<NetObj> object = objectFromParameter(param);
            if ((object == nullptr) || ((object->getObjectClass() != OBJECT_INTERFACE) && (object->getObjectClass() != OBJECT_ACCESSPOINT)))
               return DataCollectionError::DCE_NO_SUCH_INSTANCE;

            key[0] = 'N';
            key[1] = ':';
            wcslcpy(&key[2], object->getName(), MAX_OBJECT_NAME);
         }
      }
   }
   else
   {
      wcscpy(key, L"PRI");
   }

   lockProperties();
   DataCollectionError rc;
   IcmpStatCollector *collector = (m_icmpStatCollectors != nullptr) ? m_icmpStatCollectors->get(key) : nullptr;
   if (collector != nullptr)
   {
      if (!collector->empty())
      {
         switch(function)
         {
            case IcmpStatFunction::AVERAGE:
               ret_uint(value, collector->average());
               break;
            case IcmpStatFunction::JITTER:
               ret_uint(value, collector->jitter());
               break;
            case IcmpStatFunction::LAST:
               ret_uint(value, collector->last());
               break;
            case IcmpStatFunction::LOSS:
               ret_uint(value, collector->packetLoss());
               break;
            case IcmpStatFunction::MAX:
               ret_uint(value, collector->max());
               break;
            case IcmpStatFunction::MIN:
               ret_uint(value, collector->min());
               break;
         }
         rc = DataCollectionError::DCE_SUCCESS;
      }
      else
      {
         rc = DataCollectionError::DCE_COLLECTION_ERROR;
      }
   }
   else
   {
      rc = DataCollectionError::DCE_NO_SUCH_INSTANCE;
   }
   unlockProperties();
   return rc;
}

/**
 * Update ICMP statistic period for already configured ICMP collectors
 */
void Node::updateIcmpStatisticPeriod(uint32_t period)
{
   lockProperties();
   if (m_icmpStatCollectors != nullptr)
   {
      m_icmpStatCollectors->forEach(
         [period](const TCHAR *key, const IcmpStatCollector *collector) -> EnumerationCallbackResult
         {
            const_cast<IcmpStatCollector*>(collector)->resize(period);
            return _CONTINUE;
         });
   }
   unlockProperties();
}

/**
 * Get values for virtual attributes
 */
bool Node::getObjectAttribute(const TCHAR *name, TCHAR **value, bool *isAllocated) const
{
   if (super::getObjectAttribute(name, value, isAllocated))
      return true;

   if (!_tcscmp(name, _T("ssh.login")))
   {
      lockProperties();
      *value = MemCopyString(m_sshLogin);
      *isAllocated = true;
      unlockProperties();
      return true;
   }
   else if (!_tcscmp(name, _T("ssh.password")))
   {
      lockProperties();
      *value = MemCopyString(m_sshPassword);
      *isAllocated = true;
      unlockProperties();
      return true;
   }
   else if (!_tcscmp(name, _T("ssh.port")))
   {
      lockProperties();
      TCHAR *port = MemAllocString(8);
      _sntprintf(port, 8, _T("%d"), m_sshPort);
      *value = port;
      *isAllocated = true;
      unlockProperties();
      return true;
   }
   return false;
}

/**
 * Update cluster membership
 */
void Node::updateClusterMembership()
{
   if (IsShutdownInProgress() || !(g_flags & AF_AUTOBIND_ON_CONF_POLL))
      return;

   sendPollerMsg(_T("Processing cluster autobind rules\r\n"));
   unique_ptr<SharedObjectArray<NetObj>> clusters = g_idxObjectById.getObjects(
      [] (NetObj *object, void *context)
      {
         return (object->getObjectClass() == OBJECT_CLUSTER) && !object->isDeleted() && static_cast<Cluster*>(object)->isAutoBindEnabled();
      });
   for(int i = 0; i < clusters->size(); i++)
   {
      Cluster *cluster = static_cast<Cluster*>(clusters->get(i));
      NXSL_VM *cachedFilterVM = nullptr;
      AutoBindDecision decision = cluster->isApplicable(&cachedFilterVM, self());
      if (decision == AutoBindDecision_Bind)
      {
         if (!cluster->isDirectChild(m_id))
         {
            sendPollerMsg(_T("   Binding to cluster %s\r\n"), cluster->getName());
            nxlog_debug_tag(_T("obj.bind"), 4, _T("Node::updateClusterMembership(): binding node %s [%u] to cluster %s [%u]"),
                      m_name, m_id, cluster->getName(), cluster->getId());
            linkObjects(cluster->self(), self());
            EventBuilder(EVENT_CLUSTER_AUTOADD, g_dwMgmtNode)
               .param(_T("nodeId"), m_id, EventBuilder::OBJECT_ID_FORMAT)
               .param(_T("nodeName"), m_name)
               .param(_T("clusterId"), cluster->getId(), EventBuilder::OBJECT_ID_FORMAT)
               .param(_T("clusterName"), cluster->getName())
               .post();
            cluster->calculateCompoundStatus();
         }
      }
      else if (decision == AutoBindDecision_Unbind)
      {
         if (cluster->isAutoUnbindEnabled() && cluster->isDirectChild(m_id))
         {
            sendPollerMsg(_T("   Removing from cluster %s\r\n"), cluster->getName());
            nxlog_debug_tag(_T("obj.bind"), 4, _T("Node::updateClusterMembership(): removing node %s [%u] from cluster %s [%u]"),
                      m_name, m_id, cluster->getName(), cluster->getId());
            unlinkObjects(cluster, this);
            EventBuilder(EVENT_CLUSTER_AUTOREMOVE, g_dwMgmtNode)
               .param(_T("nodeId"), m_id, EventBuilder::OBJECT_ID_FORMAT)
               .param(_T("nodeName"), m_name)
               .param(_T("clusterId"), cluster->getId(), EventBuilder::OBJECT_ID_FORMAT)
               .param(_T("clusterName"), cluster->getName())
               .post();
            cluster->calculateCompoundStatus();
         }
      }
      delete cachedFilterVM;
   }
}

/**
 * Create MODBUS transport
 */
ModbusTransport *Node::createModbusTransport()
{
#if WITH_MODBUS
   ModbusTransport *transport = nullptr;
   uint32_t modbusProxy = getEffectiveModbusProxy();
   if (modbusProxy == 0)
   {
      transport = new ModbusDirectTransport(m_ipAddress, m_modbusTcpPort, m_modbusUnitId);
   }
   else
   {
      shared_ptr<Node> proxyNode = (modbusProxy == m_id) ? self() : static_pointer_cast<Node>(g_idxNodeById.get(modbusProxy));
      if (proxyNode != nullptr)
      {
         shared_ptr<AgentConnectionEx> conn = proxyNode->acquireProxyConnection(MODBUS_PROXY);
         if (conn != nullptr)
         {
            // Use loopback address if node is MODBUS proxy for itself
            transport = new ModbusProxyTransport(conn, (modbusProxy == m_id) ? InetAddress::LOOPBACK : m_ipAddress, m_modbusTcpPort, m_modbusUnitId);
         }
      }
   }
   return transport;
#else
   return nullptr;
#endif
}
