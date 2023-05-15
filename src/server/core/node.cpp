/*
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Raden Solutions
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
#include <asset_management.h>
#include <ncdrv.h>

#define DEBUG_TAG_DC_AGENT_CACHE    _T("dc.agent.cache")
#define DEBUG_TAG_ICMP_POLL         _T("poll.icmp")
#define DEBUG_TAG_NODE_INTERFACES   _T("node.iface")
#define DEBUG_TAG_ROUTES_POLL       _T("poll.routes")
#define DEBUG_TAG_TOPOLOGY_POLL     _T("poll.topology")
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
DataCollectionError GetPerfDataStorageDriverMetric(const TCHAR *driver, const TCHAR *metric, TCHAR *value);

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
   m_zoneUIN = 0;
   m_agentPort = AGENT_LISTEN_PORT;
   m_agentCacheMode = AGENT_CACHE_DEFAULT;
   m_agentSecret[0] = 0;
   m_snmpVersion = SNMP_VERSION_2C;
   m_snmpPort = SNMP_DEFAULT_PORT;
   m_snmpSecurity = new SNMP_SecurityContext("public");
   m_snmpObjectId = nullptr;
   m_snmpCodepage[0] = 0;
   m_ospfRouterId = 0;
   m_downSince = 0;
   m_savedDownSince = 0;
   m_bootTime = 0;
   m_agentUpTime = 0;
   m_proxyConnections = new ProxyAgentConnection[MAX_PROXY_TYPE];
   m_pendingDataConfigurationSync = 0;
   m_smclpConnection = nullptr;
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
   m_pollerNode = 0;
   m_agentProxy = 0;
   m_snmpProxy = 0;
   m_eipProxy = 0;
   m_mqttProxy = 0;
   m_icmpProxy = 0;
   memset(m_lastEvents, 0, sizeof(m_lastEvents));
   m_routingLoopEvents = new ObjectArray<RoutingLoopEvent>(0, 16, Ownership::True);
   m_routingTable = nullptr;
   m_failTimeAgent = TIMESTAMP_NEVER;
   m_failTimeSNMP = TIMESTAMP_NEVER;
   m_failTimeSSH = TIMESTAMP_NEVER;
   m_failTimeEtherNetIP = TIMESTAMP_NEVER;
   m_recoveryTime = TIMESTAMP_NEVER;
   m_lastAgentCommTime = TIMESTAMP_NEVER;
   m_lastAgentConnectAttempt = TIMESTAMP_NEVER;
   m_agentRestartTime = TIMESTAMP_NEVER;
   m_vrrpInfo = nullptr;
   m_topologyRebuildTimestamp = TIMESTAMP_NEVER;
   m_pendingState = -1;
   m_pollCountAgent = 0;
   m_pollCountSNMP = 0;
   m_pollCountSSH = 0;
   m_pollCountEtherNetIP = 0;
   m_pollCountICMP = 0;
   m_requiredPollCount = 0; // Use system default
   m_pollsAfterIpUpdate = 0;
   m_nUseIfXTable = IFXTABLE_DEFAULT;  // Use system default
   m_wirelessStations = nullptr;
   m_adoptedApCount = 0;
   m_totalApCount = 0;
   m_driver = nullptr;
   m_driverData = nullptr;
   m_softwarePackages = nullptr;
   m_hardwareComponents = nullptr;
   m_winPerfObjects = nullptr;
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
}

/**
 * Create new node from new node data
 */
Node::Node(const NewNodeData *newNodeData, uint32_t flags) : super(Pollable::STATUS | Pollable::CONFIGURATION | Pollable::DISCOVERY | Pollable::TOPOLOGY | Pollable::ROUTING_TABLE | Pollable::ICMP),
         m_ipAddress(newNodeData->ipAddr), m_primaryHostName(newNodeData->ipAddr.toString()), m_routingTableMutex(MutexType::FAST), m_topologyMutex(MutexType::FAST),
         m_sshLogin(newNodeData->sshLogin), m_sshPassword(newNodeData->sshPassword)
{
   m_runtimeFlags |= ODF_CONFIGURATION_POLL_PENDING;
   m_status = STATUS_UNKNOWN;
   m_type = NODE_TYPE_UNKNOWN;
   m_subType[0] = 0;
   m_hypervisorType[0] = 0;
   m_capabilities = 0;
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
   m_snmpObjectId = nullptr;
   m_snmpCodepage[0] = 0;
   m_ospfRouterId = 0;
   m_downSince = 0;
   m_savedDownSince = 0;
   m_bootTime = 0;
   m_agentUpTime = 0;
   m_proxyConnections = new ProxyAgentConnection[MAX_PROXY_TYPE];
   m_pendingDataConfigurationSync = 0;
   m_smclpConnection = nullptr;
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
   m_pollerNode = 0;
   m_agentProxy = newNodeData->agentProxyId;
   m_snmpProxy = newNodeData->snmpProxyId;
   m_eipProxy = newNodeData->eipProxyId;
   m_mqttProxy = newNodeData->mqttProxyId;
   m_icmpProxy = newNodeData->icmpProxyId;
   memset(m_lastEvents, 0, sizeof(m_lastEvents));
   m_routingLoopEvents = new ObjectArray<RoutingLoopEvent>(0, 16, Ownership::True);
   m_isHidden = true;
   m_routingTable = nullptr;
   m_failTimeAgent = TIMESTAMP_NEVER;
   m_failTimeSNMP = TIMESTAMP_NEVER;
   m_failTimeSSH = TIMESTAMP_NEVER;
   m_failTimeEtherNetIP = TIMESTAMP_NEVER;
   m_recoveryTime = TIMESTAMP_NEVER;
   m_lastAgentCommTime = TIMESTAMP_NEVER;
   m_lastAgentConnectAttempt = TIMESTAMP_NEVER;
   m_agentRestartTime = TIMESTAMP_NEVER;
   m_vrrpInfo = nullptr;
   m_topologyRebuildTimestamp = TIMESTAMP_NEVER;
   m_pendingState = -1;
   m_pollCountAgent = 0;
   m_pollCountSNMP = 0;
   m_pollCountSSH = 0;
   m_pollCountEtherNetIP = 0;
   m_pollCountICMP = 0;
   m_requiredPollCount = 0; // Use system default
   m_pollsAfterIpUpdate = 0;
   m_nUseIfXTable = IFXTABLE_DEFAULT;  // Use system default
   m_wirelessStations = nullptr;
   m_adoptedApCount = 0;
   m_totalApCount = 0;
   m_driver = nullptr;
   m_driverData = nullptr;
   m_softwarePackages = nullptr;
   m_hardwareComponents = nullptr;
   m_winPerfObjects = nullptr;
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
}

/**
 * Node destructor
 */
Node::~Node()
{
   delete m_driverData;
   delete[] m_proxyConnections;
   delete m_smclpConnection;
   delete m_agentParameters;
   delete m_agentTables;
   delete m_driverParameters;
   MemFree(m_snmpObjectId);
   MemFree(m_sysDescription);
   delete m_routingTable;
   delete m_vrrpInfo;
   delete m_snmpSecurity;
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
bool Node::loadFromDatabase(DB_HANDLE hdb, UINT32 dwId)
{
   int i, iNumRows;
   bool bResult = false;

   m_id = dwId;

   if (!loadCommonProperties(hdb) || !super::loadFromDatabase(hdb, dwId))
   {
      DbgPrintf(2, _T("Cannot load common properties for node object %d"), dwId);
      return false;
   }

   if (Pollable::loadFromDatabase(hdb, m_id))
   {
      if (static_cast<uint32_t>(time(nullptr) - m_configurationPollState.getLastCompleted()) < g_configurationPollingInterval)
         m_runtimeFlags |= ODF_CONFIGURATION_POLL_PASSED;
   }

   DB_STATEMENT hStmt = DBPrepare(hdb,
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
      _T("mqtt_proxy FROM nodes WHERE id=?"));
   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, dwId);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult == nullptr)
   {
      DBFreeStatement(hStmt);
      return false;     // Query failed
   }

   if (DBGetNumRows(hResult) == 0)
   {
      DBFreeResult(hResult);
      DBFreeStatement(hStmt);
      DbgPrintf(2, _T("Missing record in \"nodes\" table for node object %d"), dwId);
      return false;
   }

   m_primaryHostName = DBGetFieldAsSharedString(hResult, 0, 0);
   m_ipAddress = DBGetFieldInetAddr(hResult, 0, 1);
   m_snmpVersion = static_cast<SNMP_Version>(DBGetFieldLong(hResult, 0, 2));
   DBGetField(hResult, 0, 3, m_agentSecret, MAX_SECRET_LENGTH);
   m_agentPort = static_cast<uint16_t>(DBGetFieldLong(hResult, 0, 4));
   m_snmpObjectId = DBGetField(hResult, 0, 5, nullptr, 0);
   if ((m_snmpObjectId != nullptr) && (*m_snmpObjectId == 0))
      MemFreeAndNull(m_snmpObjectId);
   DBGetField(hResult, 0, 6, m_agentVersion, MAX_AGENT_VERSION_LEN);
   DBGetField(hResult, 0, 7, m_platformName, MAX_PLATFORM_NAME_LEN);
   m_pollerNode = DBGetFieldULong(hResult, 0, 8);
   m_zoneUIN = DBGetFieldULong(hResult, 0, 9);
   m_agentProxy = DBGetFieldULong(hResult, 0, 10);
   m_snmpProxy = DBGetFieldULong(hResult, 0, 11);
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

   m_icmpProxy = DBGetFieldULong(hResult, 0, 25);
   m_agentCacheMode = static_cast<int16_t>(DBGetFieldLong(hResult, 0, 26));
   if ((m_agentCacheMode != AGENT_CACHE_ON) && (m_agentCacheMode != AGENT_CACHE_OFF))
      m_agentCacheMode = AGENT_CACHE_DEFAULT;

   m_sysContact = DBGetField(hResult, 0, 27, nullptr, 0);
   m_sysLocation = DBGetField(hResult, 0, 28, nullptr, 0);

   m_physicalContainer = DBGetFieldULong(hResult, 0, 29);
   m_rackImageFront = DBGetFieldGUID(hResult, 0, 30);
   m_rackPosition = (INT16)DBGetFieldLong(hResult, 0, 31);
   m_rackHeight = (INT16)DBGetFieldLong(hResult, 0, 32);
   m_lastAgentCommTime = DBGetFieldLong(hResult, 0, 33);
   m_syslogMessageCount = DBGetFieldInt64(hResult, 0, 34);
   m_snmpTrapCount = DBGetFieldInt64(hResult, 0, 35);
   m_snmpTrapLastTotal = m_snmpTrapCount;
   m_type = (NodeType)DBGetFieldLong(hResult, 0, 36);
   DBGetField(hResult, 0, 37, m_subType, MAX_NODE_SUBTYPE_LENGTH);
   m_sshLogin = DBGetFieldAsSharedString(hResult, 0, 38);
   m_sshPassword = DBGetFieldAsSharedString(hResult, 0, 39);
   m_sshProxy = DBGetFieldULong(hResult, 0, 40);
   m_portRowCount = DBGetFieldULong(hResult, 0, 41);
   m_portNumberingScheme = DBGetFieldULong(hResult, 0, 42);
   m_agentCompressionMode = (INT16)DBGetFieldLong(hResult, 0, 43);
   m_tunnelId = DBGetFieldGUID(hResult, 0, 44);
   m_lldpNodeId = DBGetField(hResult, 0, 45, nullptr, 0);
   if ((m_lldpNodeId != nullptr) && (*m_lldpNodeId == 0))
      MemFreeAndNull(m_lldpNodeId);
   m_capabilities = DBGetFieldULong(hResult, 0, 46);
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
   m_cipDeviceType = static_cast<uint16_t>(DBGetFieldULong(hResult, 0, 63));
   m_cipStatus = static_cast<uint16_t>(DBGetFieldULong(hResult, 0, 64));
   m_cipState = static_cast<uint8_t>(DBGetFieldULong(hResult, 0, 65));
   m_eipProxy = DBGetFieldULong(hResult, 0, 66);
   m_eipPort = static_cast<uint16_t>(DBGetFieldULong(hResult, 0, 67));
   BYTE hardwareId[HARDWARE_ID_LENGTH];
   DBGetFieldByteArray2(hResult, 0, 68, hardwareId, HARDWARE_ID_LENGTH, 0);
   m_hardwareId = NodeHardwareId(hardwareId);
   m_cipVendorCode = static_cast<uint16_t>(DBGetFieldULong(hResult, 0, 69));
   m_agentCertMappingMethod = static_cast<CertificateMappingMethod>(DBGetFieldLong(hResult, 0, 70));
   m_agentCertMappingData = DBGetField(hResult, 0, 71, nullptr, 0);
   if ((m_agentCertMappingData != nullptr) && (m_agentCertMappingData[0] == 0))
      MemFreeAndNull(m_agentCertMappingData);

   m_sshPort = static_cast<uint16_t>(DBGetFieldLong(hResult, 0, 73));
   m_sshKeyId = DBGetFieldLong(hResult, 0, 74);

   DBGetFieldUTF8(hResult, 0, 75, m_syslogCodepage, 16);
   DBGetFieldUTF8(hResult, 0, 76, m_snmpCodepage, 16);
   m_ospfRouterId = DBGetFieldIPAddr(hResult, 0, 77);
   m_mqttProxy = DBGetFieldIPAddr(hResult, 0, 78);

   DBFreeResult(hResult);
   DBFreeStatement(hStmt);

   if (m_isDeleted)
   {
      return true;
   }

   // Link node to subnets
   hStmt = DBPrepare(hdb, _T("SELECT subnet_id FROM nsmap WHERE node_id=?"));
   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   hResult = DBSelectPrepared(hStmt);
   if (hResult == nullptr)
   {
      DBFreeStatement(hStmt);
      return false;     // Query failed
   }

   iNumRows = DBGetNumRows(hResult);
   for(i = 0; i < iNumRows; i++)
   {
      uint32_t subnetId = DBGetFieldULong(hResult, i, 0);
      shared_ptr<NetObj> subnet = FindObjectById(subnetId, OBJECT_SUBNET);
      if (subnet != nullptr)
      {
         subnet->addChild(self());
         addParent(subnet);
      }
      else
      {
         nxlog_write(NXLOG_ERROR, _T("Inconsistent database: node %s [%u] linked to non-existing subnet [%u]"), m_name, m_id, subnetId);
      }
   }

   DBFreeResult(hResult);
   DBFreeStatement(hStmt);

   loadItemsFromDB(hdb);
   loadACLFromDB(hdb);

   // Walk through all items in the node and load appropriate thresholds
   bResult = true;
   for(i = 0; i < m_dcObjects.size(); i++)
   {
      if (!m_dcObjects.get(i)->loadThresholdsFromDB(hdb))
      {
         DbgPrintf(3, _T("Cannot load thresholds for DCI %d of node %d (%s)"),
                   m_dcObjects.get(i)->getId(), dwId, m_name);
         bResult = false;
      }
   }
   loadDCIListForCleanup(hdb);

   updatePhysicalContainerBinding(m_physicalContainer);

   if (bResult)
   {
      // Load components
      hStmt = DBPrepare(hdb, _T("SELECT component_index,parent_index,position,component_class,if_index,name,description,model,serial_number,vendor,firmware FROM node_components WHERE node_id=?"));
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
                           DBGetFieldULong(hResult, i, 0), // index
                           DBGetFieldULong(hResult, i, 3), // class
                           DBGetFieldULong(hResult, i, 1), // parent index
                           DBGetFieldULong(hResult, i, 2), // position
                           DBGetFieldULong(hResult, i, 4), // ifIndex
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
                  nxlog_debug(6, _T("Node::loadFromDatabase(%s [%u]): root element for component tree not found"), m_name, m_id);
                  elements.setOwner(Ownership::True);   // cause element destruction on exit
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
         nxlog_debug(3, _T("Cannot load components for node %s [%u]"), m_name, m_id);
   }

   if (bResult)
   {
      // Load software packages
      hStmt = DBPrepare(hdb, _T("SELECT name,version,vendor,install_date,url,description FROM software_inventory WHERE node_id=?"));
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
         DBFreeStatement(hStmt);
      }
      else
      {
         bResult = false;
      }

      if (!bResult)
         nxlog_debug(3, _T("Cannot load software package list for node %s [%u]"), m_name, m_id);
   }

   if (bResult)
   {
      // Load hardware components
      hStmt = DBPrepare(hdb, _T("SELECT category,component_index,hw_type,vendor,model,location,capacity,part_number,serial_number,description FROM hardware_inventory WHERE node_id=?"));
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
         DBFreeStatement(hStmt);
      }
      else
      {
         bResult = false;
      }

      if (!bResult)
         nxlog_debug(3, _T("Cannot load hardware information for node %s [%u]"), m_name, m_id);
   }

   if (bResult)
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
         nxlog_debug(3, _T("Cannot load OSPF area information for node %s [%u]"), m_name, m_id);
   }

   if (bResult)
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
                  n->nodeId = DBGetFieldULong(hResult, i, 3);
                  n->ifIndex = DBGetFieldULong(hResult, i, 4);
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
         nxlog_debug(3, _T("Cannot load OSPF area information for node %s [%u]"), m_name, m_id);
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
                  nxlog_debug(3, _T("Cannot load ICMP statistic collector %s for node %s [%u]"), target, m_name, m_id);
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

   return bResult;
}

/**
 * Link related objects after loading from database
 */
void Node::linkObjects()
{
   super::linkObjects();

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

   const ObjectArray<Component> *children = component->getChildren();
   for(int i = 0; i < children->size(); i++)
      if (!SaveComponent(hStmt, children->get(i)))
         return false;
   return true;
}

/**
 * Save ICMP statistics collector
 */
static EnumerationCallbackResult SaveIcmpStatCollector(const TCHAR *target, const IcmpStatCollector *collector, std::pair<uint32_t, DB_HANDLE> *context)
{
   return collector->saveToDatabase(context->second, context->first, target) ? _CONTINUE : _STOP;
}

/**
 * Save object to database
 */
bool Node::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);

   if (success && (m_modified & MODIFY_NODE_PROPERTIES))
   {
      static const TCHAR *columns[] = {
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
         _T("agent_cert_mapping_method"), _T("agent_cert_mapping_data"), _T("snmp_engine_id"), _T("syslog_codepage"), _T("snmp_codepage"),
         _T("ospf_router_id"), _T("mqtt_proxy"),
         nullptr
      };

      DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("nodes"), _T("id"), m_id, columns);
      if (hStmt != nullptr)
      {
         lockProperties();

         int snmpMethods = m_snmpSecurity->getAuthMethod() | (m_snmpSecurity->getPrivMethod() << 8);
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

         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_ipAddress.toString(ipAddr), DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_primaryHostName, DB_BIND_TRANSIENT);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_snmpPort));
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_capabilities);
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_snmpVersion));
#ifdef UNICODE
         DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, WideStringFromMBString(m_snmpSecurity->getCommunity()), DB_BIND_DYNAMIC);
#else
         DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, m_snmpSecurity->getCommunity(), DB_BIND_STATIC);
#endif
         DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_agentPort));
         DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, m_agentSecret, DB_BIND_STATIC);
         DBBind(hStmt, 9, DB_SQLTYPE_VARCHAR, m_snmpObjectId, DB_BIND_STATIC);
         DBBind(hStmt, 10, DB_SQLTYPE_VARCHAR, m_sysDescription, DB_BIND_STATIC);
         DBBind(hStmt, 11, DB_SQLTYPE_VARCHAR, m_agentVersion, DB_BIND_STATIC);
         DBBind(hStmt, 12, DB_SQLTYPE_VARCHAR, m_platformName, DB_BIND_STATIC);
         DBBind(hStmt, 13, DB_SQLTYPE_INTEGER, m_pollerNode);
         DBBind(hStmt, 14, DB_SQLTYPE_INTEGER, m_zoneUIN);
         DBBind(hStmt, 15, DB_SQLTYPE_INTEGER, m_agentProxy);
         DBBind(hStmt, 16, DB_SQLTYPE_INTEGER, m_snmpProxy);
         DBBind(hStmt, 17, DB_SQLTYPE_INTEGER, m_icmpProxy);
         DBBind(hStmt, 18, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_requiredPollCount));
         DBBind(hStmt, 19, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_nUseIfXTable));
#ifdef UNICODE
         DBBind(hStmt, 20, DB_SQLTYPE_VARCHAR, WideStringFromMBString(m_snmpSecurity->getAuthPassword()), DB_BIND_DYNAMIC);
         DBBind(hStmt, 21, DB_SQLTYPE_VARCHAR, WideStringFromMBString(m_snmpSecurity->getPrivPassword()), DB_BIND_DYNAMIC);
#else
         DBBind(hStmt, 20, DB_SQLTYPE_VARCHAR, m_snmpSecurity->getAuthPassword(), DB_BIND_STATIC);
         DBBind(hStmt, 21, DB_SQLTYPE_VARCHAR, m_snmpSecurity->getPrivPassword(), DB_BIND_STATIC);
#endif
         DBBind(hStmt, 22, DB_SQLTYPE_INTEGER, static_cast<int32_t>(snmpMethods));
         DBBind(hStmt, 23, DB_SQLTYPE_VARCHAR, m_sysName, DB_BIND_STATIC, 127);
         DBBind(hStmt, 24, DB_SQLTYPE_VARCHAR, BinToStr(m_baseBridgeAddress, MAC_ADDR_LENGTH, baseAddress), DB_BIND_STATIC);
         DBBind(hStmt, 25, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_downSince));
         DBBind(hStmt, 26, DB_SQLTYPE_VARCHAR, (m_driver != nullptr) ? m_driver->getName() : _T(""), DB_BIND_STATIC);
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
         DBBind(hStmt, 43, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_sshPort));
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
         }
         else
         {
            DBBind(hStmt, 75, DB_SQLTYPE_VARCHAR, _T(""), DB_BIND_STATIC);
         }
         DBBind(hStmt, 76, DB_SQLTYPE_VARCHAR, DB_CTYPE_UTF8_STRING, m_syslogCodepage, DB_BIND_STATIC);
         DBBind(hStmt, 77, DB_SQLTYPE_VARCHAR, DB_CTYPE_UTF8_STRING, m_snmpCodepage, DB_BIND_STATIC);
         if (m_ospfRouterId != 0)
            DBBind(hStmt, 78, DB_SQLTYPE_VARCHAR, IpToStr(m_ospfRouterId, routerId), DB_BIND_STATIC);
         else
            DBBind(hStmt, 78, DB_SQLTYPE_VARCHAR, _T(""), DB_BIND_STATIC);
         DBBind(hStmt, 79, DB_SQLTYPE_INTEGER, m_mqttProxy);
         DBBind(hStmt, 80, DB_SQLTYPE_INTEGER, m_id);

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
                  _T("INSERT INTO software_inventory (node_id,name,version,vendor,install_date,url,description) VALUES (?,?,?,?,?,?,?)"),
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

      success = executeQueryOnObject(hdb, _T("DELETE FROM icmp_statistics WHERE object_id=?"));
      if (success && isIcmpStatCollectionEnabled() && (m_icmpStatCollectors != nullptr) && !m_icmpStatCollectors->isEmpty())
      {
         std::pair<uint32_t, DB_HANDLE> context(m_id, hdb);
         success = (m_icmpStatCollectors->forEach(SaveIcmpStatCollector, &context) == _CONTINUE);
      }

      if (success)
         success = executeQueryOnObject(hdb, _T("DELETE FROM icmp_target_address_list WHERE node_id=?"));

      if (success && !m_icmpTargets.isEmpty())
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO icmp_target_address_list (node_id,ip_addr) VALUES (?,?)"), m_icmpTargets.size() > 1);
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
      std::pair<UINT32, DB_HANDLE> context(m_id, hdb);
      if (m_icmpStatCollectors->forEach(SaveIcmpStatCollector, &context) == _STOP)
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

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("UPDATE nodes SET last_agent_comm_time=?,syslog_msg_count=?,snmp_trap_count=?,snmp_engine_id=?,down_since=? WHERE id=?"));
   if (hStmt == nullptr)
      return false;

   lockProperties();
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_lastAgentCommTime));
   DBBind(hStmt, 2, DB_SQLTYPE_BIGINT, m_syslogMessageCount);
   DBBind(hStmt, 3, DB_SQLTYPE_BIGINT, m_snmpTrapCount);
   if (m_snmpSecurity != nullptr)
      DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_snmpSecurity->getAuthoritativeEngine().toString(), DB_BIND_TRANSIENT);
   else
      DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, _T(""), DB_BIND_STATIC);
   DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_downSince));
   DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_id);
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
 * Get ARP cache from node
 */
shared_ptr<ArpCache> Node::getArpCache(bool forceRead)
{
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
         return arpCache;
   }

   if (m_capabilities & NC_IS_LOCAL_MGMT)
   {
      arpCache = GetLocalArpCache();
   }
   else if (m_capabilities & NC_IS_NATIVE_AGENT)
   {
      shared_ptr<AgentConnectionEx> conn = getAgentConnection();
      if (conn != nullptr)
      {
         arpCache = conn->getArpCache();
      }
   }
   else if ((m_capabilities & NC_IS_SNMP) && (m_driver != nullptr))
   {
      SNMP_Transport *transport = createSnmpTransport();
      if (transport != nullptr)
      {
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
   return arpCache;
}

/**
 * Get list of interfaces from node
 * @return Dynamically allocated list of interfaces. Must be freed by the caller.
 */
InterfaceList *Node::getInterfaceList()
{
   InterfaceList *pIfList = nullptr;

   if ((m_capabilities & NC_IS_NATIVE_AGENT) && (!(m_flags & NF_DISABLE_NXCP)))
   {
      shared_ptr<AgentConnectionEx> conn = getAgentConnection();
      if (conn != nullptr)
      {
         pIfList = conn->getInterfaceList();
      }
   }
   if ((pIfList == nullptr) && (m_capabilities & NC_IS_LOCAL_MGMT))
   {
      pIfList = GetLocalInterfaceList();
   }
   if ((pIfList == nullptr) && (m_capabilities & NC_IS_SNMP) &&
       (!(m_flags & NF_DISABLE_SNMP)) && (m_driver != nullptr))
   {
      SNMP_Transport *pTransport = createSnmpTransport();
      if (pTransport != nullptr)
      {
         bool useIfXTable;
         if (m_nUseIfXTable == IFXTABLE_DEFAULT)
         {
            useIfXTable = ConfigReadBoolean(_T("Objects.Interfaces.UseIfXTable"), true);
         }
         else
         {
            useIfXTable = (m_nUseIfXTable == IFXTABLE_ENABLED) ? true : false;
         }

         int useAliases = ConfigReadInt(_T("Objects.Interfaces.UseAliases"), 0);
         nxlog_debug_tag(DEBUG_TAG_NODE_INTERFACES, 6, _T("Node::getInterfaceList(node=%s [%u]): calling driver (useAliases=%d, useIfXTable=%s)"),
                  m_name, m_id, useAliases, useIfXTable ? _T("true") : _T("false"));
         pIfList = m_driver->getInterfaces(pTransport, this, m_driverData, useAliases, useIfXTable);

         if ((pIfList != nullptr) && (m_capabilities & NC_IS_BRIDGE))
         {
            BridgeMapPorts(pTransport, pIfList);
         }
         delete pTransport;
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_NODE_INTERFACES, 6, _T("Node::getInterfaceList(node=%s [%u]): cannot create SNMP transport"), m_name, m_id);
      }
   }

   if (pIfList != nullptr)
   {
      checkInterfaceNames(pIfList);
      addVrrpInterfaces(pIfList);
   }

   return pIfList;
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
                  if (addr.getSubnetAddress().contain(router->getVip(0)))
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
shared_ptr<Interface> Node::findInterfaceByIndex(UINT32 ifIndex) const
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
         if (!_tcsicmp(iface->getName(), name) || !_tcsicmp(iface->getDescription(), name))
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
          static_cast<Interface*>(curr)->getMacAddr().equals(macAddr))
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
         if (subnet.contain(addrList->get(j)))
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
shared_ptr<Interface> Node::findBridgePort(UINT32 bridgePortNumber) const
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
         cp = FindInterfaceConnectionPoint(iface->getMacAddr(), type);
         if (cp != nullptr)
         {
            *localIfId = iface->getId();
            memcpy(localMacAddr, iface->getMacAddr().value(), MAC_ADDR_LENGTH);
            break;
         }
      }
   }
   unlockChildList();
   return cp;
}

/**
 * Find attached access point by MAC address
 */
shared_ptr<AccessPoint> Node::findAccessPointByMAC(const MacAddress& macAddr) const
{
   shared_ptr<AccessPoint> ap;
   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *curr = getChildList().get(i);
      if ((curr->getObjectClass() == OBJECT_ACCESSPOINT) &&
          static_cast<AccessPoint*>(curr)->getMacAddr().equals(macAddr))
      {
         ap = static_pointer_cast<AccessPoint>(getChildList().getShared(i));
         break;
      }
   }
   unlockChildList();
   return ap;
}

/**
 * Find access point by radio ID (radio interface index)
 */
shared_ptr<AccessPoint> Node::findAccessPointByRadioId(uint32_t rfIndex) const
{
   shared_ptr<AccessPoint> ap;
   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *curr = getChildList().get(i);
      if ((curr->getObjectClass() == OBJECT_ACCESSPOINT) &&
          static_cast<AccessPoint*>(curr)->isMyRadio(rfIndex))
      {
         ap = static_pointer_cast<AccessPoint>(getChildList().getShared(i));
         break;
      }
   }
   unlockChildList();
   return ap;
}

/**
 * Find attached access point by BSSID
 */
shared_ptr<AccessPoint> Node::findAccessPointByBSSID(const BYTE *bssid) const
{
   shared_ptr<AccessPoint> ap;
   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *curr = getChildList().get(i);
      if ((curr->getObjectClass() == OBJECT_ACCESSPOINT) &&
          (static_cast<AccessPoint*>(curr)->getMacAddr().equals(bssid) || static_cast<AccessPoint*>(curr)->isMyRadio(bssid)))
      {
         ap = static_pointer_cast<AccessPoint>(getChildList().getShared(i));
         break;
      }
   }
   unlockChildList();
   return ap;
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
 * Create interface object. Can return nullptr if interface creation hook
 * blocks interface creation.
 */
shared_ptr<Interface> Node::createInterfaceObject(InterfaceInfo *info, bool manuallyCreated, bool fakeInterface, bool syntheticMask)
{
   shared_ptr<Interface> iface;
   if (info->name[0] != 0)
   {
      iface = make_shared<Interface>(info->name, (info->description[0] != 0) ? info->description : info->name,
               info->index, info->ipAddrList, info->type, m_zoneUIN);
   }
   else
   {
      iface = make_shared<Interface>(info->ipAddrList, m_zoneUIN, syntheticMask);
   }
   iface->setAlias(info->alias);
   iface->setIfAlias(info->alias);
   iface->setMacAddr(MacAddress(info->macAddr, MAC_ADDR_LENGTH), false);
   iface->setBridgePortNumber(info->bridgePort);
   iface->setPhysicalLocation(info->location);
   iface->setPhysicalPortFlag(info->isPhysicalPort);
   iface->setManualCreationFlag(manuallyCreated);
   iface->setSystemFlag(info->isSystem);
   iface->setMTU(info->mtu);
   iface->setSpeed(info->speed);
   iface->setIfTableSuffix(info->ifTableSuffixLength, info->ifTableSuffix);

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
         if (vm.failureReason() == ScriptVMFailureReason::SCRIPT_IS_EMPTY)
            nxlog_debug_tag(DEBUG_TAG_NODE_INTERFACES, 7, _T("Node::createInterfaceObject(%s [%u]): hook script \"Hook::CreateInterface\" is empty"), m_name, m_id);
         else
            nxlog_debug_tag(DEBUG_TAG_NODE_INTERFACES, 7, _T("Node::createInterfaceObject(%s [%u]): hook script \"Hook::CreateInterface\" not found"), m_name, m_id);
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
                m_name, m_id, iface->getName(), info->index, pass ? _T("accepted") : _T("rejected"));
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

   nxlog_debug_tag(DEBUG_TAG_NODE_INTERFACES, 5, _T("Node::createNewInterface(\"%s\", %d, %d, bp=%d, chassis=%u, module=%u, pic=%u, port=%u) called for node %s [%d]"),
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
      shared_ptr<Cluster> pCluster = getMyCluster();
      for(int i = 0; i < info->ipAddrList.size(); i++)
      {
         InetAddress addr = info->ipAddrList.get(i);
         bool addToSubnet = addr.isValidUnicast() && ((pCluster == nullptr) || !pCluster->isSyncAddr(addr));
         nxlog_debug_tag(DEBUG_TAG_NODE_INTERFACES, 5, _T("Node::createNewInterface: node=%s [%d] ip=%s/%d cluster=%s [%d] add=%s"),
                   m_name, m_id, addr.toString(buffer), addr.getMaskBits(),
                   (pCluster != nullptr) ? pCluster->getName() : _T("(null)"),
                   (pCluster != nullptr) ? pCluster->getId() : 0, addToSubnet ? _T("yes") : _T("no"));
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
               if (addr.getHostBits() > 0)
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
   shared_ptr<Interface>  iface = createInterfaceObject(info, manuallyCreated, fakeInterface, bSyntheticMask);
   if (iface == nullptr)
      return iface;

   NetObjInsert(iface, true, false);
   addInterface(iface);
   if (!m_isHidden)
      iface->unhide();
   if (!iface->isSystem())
   {
      const InetAddress& addr = iface->getFirstIpAddress();
      PostSystemEvent(EVENT_INTERFACE_ADDED, m_id, "dsAdd", iface->getId(),
                iface->getName(), &addr, addr.getMaskBits(), iface->getIfIndex());
   }

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
   nxlog_debug_tag(DEBUG_TAG_NODE_INTERFACES, 5, _T("Node::deleteInterface(node=%s [%d], interface=%s [%d])"), m_name, m_id, iface->getName(), iface->getId());

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
               deleteParent(*subnet);
               subnet->deleteChild(*this);
            }
            nxlog_debug_tag(DEBUG_TAG_NODE_INTERFACES, 5, _T("Node::deleteInterface(node=%s [%d], interface=%s [%d]): unlinked from subnet %s [%d]"),
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
bool Node::setMgmtStatus(bool isManaged)
{
   if (!super::setMgmtStatus(isManaged))
      return false;

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
   return true;
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
      PostSystemEvent(eventCodes[m_status], m_id, "d", oldStatus);
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
               (getMyCluster() == nullptr) &&
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
       (static_cast<uint32_t>(time(nullptr) - m_routingPollState.getLastCompleted()) > getCustomAttributeAsUInt32(_T("SysConfig:Topology.RoutingTableUpdateInterval"), g_routingTableUpdateInterval)) &&
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

   uint32_t oldCapabilities = m_capabilities;
   uint32_t oldState = m_state;

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
   time_t capabilityExpirationGracePeriod = static_cast<time_t>(ConfigReadULong(_T("Objects.Nodes.CapabilityExpirationTime"), 3600));
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
                  PostSystemEventEx(eventQueue, EVENT_SNMP_OK, m_id, nullptr);
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
               unlockProperties();
            }
         }
         else if ((snmpErr == SNMP_ERR_ENGINE_ID) && (m_snmpVersion == SNMP_VERSION_3) && (retryCount > 0))
         {
            // Reset authoritative engine data
            lockProperties();
            m_snmpSecurity->setAuthoritativeEngine(SNMP_Engine());
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
               if ((now > m_failTimeSNMP + capabilityExpirationTime) && (!(m_state & DCSF_UNREACHABLE)) && (now > m_recoveryTime + capabilityExpirationGracePeriod))
               {
                  m_capabilities &= ~NC_IS_SNMP;
                  m_state &= ~NSF_SNMP_UNREACHABLE;
                  MemFreeAndNull(m_snmpObjectId);
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
                  PostSystemEventEx(eventQueue, EVENT_SNMP_FAIL, m_id, nullptr);
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
               PostSystemEventEx(eventQueue, EVENT_SSH_OK, m_id, nullptr);
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
            if ((now > m_failTimeSSH + capabilityExpirationTime) && !(m_state & DCSF_UNREACHABLE) && (now > m_recoveryTime + capabilityExpirationGracePeriod))
            {
               m_capabilities &= ~NC_IS_SSH;
               m_state &= ~NSF_AGENT_UNREACHABLE;
               sendPollerMsg(POLLER_WARNING _T("Attribute isSSH set to FALSE\r\n"));
            }
         }
         else
         {
            m_pollCountSSH++;
            if (m_pollCountSSH >= requiredPolls)
            {
               m_state |= NSF_SSH_UNREACHABLE;
               PostSystemEventEx(eventQueue, EVENT_SSH_UNREACHABLE, m_id, nullptr);
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
               PostSystemEventEx(eventQueue, EVENT_AGENT_OK, m_id, nullptr);
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
         }
         else
         {
            m_pollCountAgent = 0;
         }
         agentConnected = true;
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): agent unreachable, error=%d, socketError=%d. Poll count %d of %d"),
                  m_name, (int)error, (int)socketError, m_pollCountAgent, requiredPolls);
         sendPollerMsg(POLLER_ERROR _T("Cannot connect to NetXMS agent (%s)\r\n"), AgentErrorCodeToText(error));
         if (m_state & NSF_AGENT_UNREACHABLE)
         {
            if ((now > m_failTimeAgent + capabilityExpirationTime) && !(m_state & DCSF_UNREACHABLE) && (now > m_recoveryTime + capabilityExpirationGracePeriod))
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
               PostSystemEventEx(eventQueue, EVENT_AGENT_FAIL, m_id, nullptr);
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
               PostSystemEventEx(eventQueue, EVENT_ETHERNET_IP_OK, m_id, nullptr);
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
            if ((now > m_failTimeEtherNetIP + capabilityExpirationTime) && !(m_state & DCSF_UNREACHABLE) && (now > m_recoveryTime + capabilityExpirationGracePeriod))
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
               PostSystemEventEx(eventQueue, EVENT_ETHERNET_IP_UNREACHABLE, m_id, nullptr);
               m_failTimeEtherNetIP = now;
               m_pollCountEtherNetIP = 0;
            }
         }
      }

      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("StatusPoll(%s): EtherNet/IP check finished"), m_name);
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
   DbgPrintf(7, _T("StatusPoll(%s): starting child object poll"), m_name);
   shared_ptr<Cluster> cluster = getMyCluster();
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
         case OBJECT_ACCESSPOINT:
            if (snmp != nullptr)
            {
               nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("StatusPoll(%s): polling access point %d [%s]"), m_name, curr->getId(), curr->getName());
               static_cast<AccessPoint*>(curr)->statusPollFromController(pSession, rqId, eventQueue, this, snmp);
            }
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
             (((Interface *)curr)->getAdminState() != IF_ADMIN_STATE_DOWN) &&
             (((Interface *)curr)->getConfirmedOperState() == IF_OPER_STATE_UP) &&
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
            if (m_ipAddress.isValidUnicast())
            {
               nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): using TCP ping on primary IP address"), m_name);
               sendPollerMsg(_T("Checking primary IP address with TCP ping on agent's port\r\n"));
               TcpPingResult r = TcpPing(m_ipAddress, m_agentPort, 1000);
               if ((r == TCP_PING_SUCCESS) || (r == TCP_PING_REJECT))
               {
                  nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): agent is unreachable but IP address is likely reachable (TCP ping returns %d)"), m_name, r);
                  sendPollerMsg(POLLER_INFO _T("   Primary IP address is responding to TCP ping\r\n"));
                  allDown = false;
               }
               else
               {
                  sendPollerMsg(POLLER_ERROR _T("   Primary IP address is not responding to TCP ping\r\n"));
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
            if (m_ipAddress.isValidUnicast())
            {
               nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): using TCP ping on primary IP address"), m_name);
               sendPollerMsg(_T("Checking primary IP address with TCP ping on EtherNet/IP port\r\n"));
               TcpPingResult r = TcpPing(m_ipAddress, m_eipPort, 1000);
               if ((r == TCP_PING_SUCCESS) || (r == TCP_PING_REJECT))
               {
                  nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): EtherNet/IP is unreachable but IP address is likely reachable (TCP ping returns %d)"), m_name, r);
                  sendPollerMsg(POLLER_INFO _T("   Primary IP address is responding to TCP ping\r\n"));
                  allDown = false;
               }
               else
               {
                  sendPollerMsg(POLLER_ERROR _T("   Primary IP address is not responding to TCP ping\r\n"));
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

      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): allDown=%s, statFlags=0x%08X"), m_name, allDown ? _T("true") : _T("false"), m_state);
      if (allDown)
      {
         if (!(m_state & DCSF_UNREACHABLE))
         {
            m_state |= DCSF_UNREACHABLE;
            m_downSince = time(nullptr);
            poller->setStatus(_T("check network path"));
            NetworkPathCheckResult patchCheckResult = checkNetworkPath(rqId);
            nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): network path check result: (rootCauseFound=%s, reason=%d, node=%u, iface=%u)"),
                     m_name, patchCheckResult.rootCauseFound ? _T("true") : _T("false"), static_cast<int>(patchCheckResult.reason),
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

               static const TCHAR *pnames[] = {
                        _T("reasonCode"), _T("reason"), _T("rootCauseNodeId"), _T("rootCauseNodeName"),
                        _T("rootCauseInterfaceId"), _T("rootCauseInterfaceName"), _T("description")
               };
               static const TCHAR *reasonNames[] = {
                        _T("None"), _T("Router down"), _T("Switch down"), _T("Wireless AP down"),
                        _T("Proxy node down"), _T("Proxy agent unreachable"), _T("VPN tunnel down"),
                        _T("Routing loop"), _T("Interface disabled")
               };

               TCHAR description[1024];
               switch(patchCheckResult.reason)
               {
                  case NetworkPathFailureReason::INTERFACE_DISABLED:
                     _sntprintf(description, 1024, _T("Interface %s on node %s is disabled"),
                              GetObjectName(patchCheckResult.rootCauseInterfaceId, _T("UNKNOWN")),
                              GetObjectName(patchCheckResult.rootCauseNodeId, _T("UNKNOWN")));
                     break;
                  case NetworkPathFailureReason::PROXY_AGENT_UNREACHABLE:
                     _sntprintf(description, 1024, _T("Agent on proxy node %s is unreachable"), GetObjectName(patchCheckResult.rootCauseNodeId, _T("UNKNOWN")));
                     break;
                  case NetworkPathFailureReason::PROXY_NODE_DOWN:
                     _sntprintf(description, 1024, _T("Proxy node %s is down"), GetObjectName(patchCheckResult.rootCauseNodeId, _T("UNKNOWN")));
                     break;
                  case NetworkPathFailureReason::ROUTER_DOWN:
                     _sntprintf(description, 1024, _T("Intermediate router %s is down"), GetObjectName(patchCheckResult.rootCauseNodeId, _T("UNKNOWN")));
                     break;
                  case NetworkPathFailureReason::ROUTING_LOOP:
                     _sntprintf(description, 1024, _T("Routing loop detected on intermediate router %s"), GetObjectName(patchCheckResult.rootCauseNodeId, _T("UNKNOWN")));
                     break;
                  case NetworkPathFailureReason::SWITCH_DOWN:
                     _sntprintf(description, 1024, _T("Intermediate switch %s is down"), GetObjectName(patchCheckResult.rootCauseNodeId, _T("UNKNOWN")));
                     break;
                  case NetworkPathFailureReason::VPN_TUNNEL_DOWN:
                     _sntprintf(description, 1024, _T("VPN tunnel %s on node %s is down"),
                              GetObjectName(patchCheckResult.rootCauseInterfaceId, _T("UNKNOWN")),
                              GetObjectName(patchCheckResult.rootCauseNodeId, _T("UNKNOWN")));
                     break;
                  case NetworkPathFailureReason::WIRELESS_AP_DOWN:
                     _sntprintf(description, 1024, _T("Wireless access point %s is down"), GetObjectName(patchCheckResult.rootCauseNodeId, _T("UNKNOWN")));
                     break;
                  default:
                     _tcscpy(description, reasonNames[static_cast<int32_t>(patchCheckResult.reason)]);
                     break;
               }

               PostSystemEventWithNames(EVENT_NODE_UNREACHABLE, m_id, "dsisiss", pnames, static_cast<int32_t>(patchCheckResult.reason),
                        reasonNames[static_cast<int32_t>(patchCheckResult.reason)], patchCheckResult.rootCauseNodeId,
                        GetObjectName(patchCheckResult.rootCauseNodeId, _T("")), patchCheckResult.rootCauseInterfaceId,
                        GetObjectName(patchCheckResult.rootCauseInterfaceId, _T("")), description);
               sendPollerMsg(POLLER_WARNING _T("Detected network path problem (%s)\r\n"), description);
            }
            else if ((m_flags & (NF_DISABLE_NXCP | NF_DISABLE_SNMP | NF_DISABLE_ICMP | NF_DISABLE_ETHERNET_IP)) == (NF_DISABLE_NXCP | NF_DISABLE_SNMP | NF_DISABLE_ICMP | NF_DISABLE_ETHERNET_IP))
            {
               sendPollerMsg(POLLER_WARNING _T("All poller methods (NetXMS agent, SNMP, ICMP and EtherNet/IP) are currently disabled\r\n"));
            }
            else
            {
               PostSystemEvent(EVENT_NODE_DOWN, m_id, nullptr);
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
                  PostSystemEvent(EVENT_NODE_DOWN, m_id, nullptr);
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
            PostSystemEvent(EVENT_NODE_UP, m_id, "d", reason);
            sendPollerMsg(POLLER_INFO _T("Node recovered from unreachable state\r\n"));
            resyncDataCollectionConfiguration = true; // Will cause addition of all remotely collected DCIs on proxy
            // Set recovery time to provide grace period for capability expiration
            m_recoveryTime = now;
            goto restart_status_poll;
         }
         else
         {
            sendPollerMsg(POLLER_INFO _T("Node is connected\r\n"));
         }
      }
   }

   POLL_CANCELLATION_CHECKPOINT_EX(delete eventQueue);

   // Get uptime and update boot time
   if (!(m_state & DCSF_UNREACHABLE))
   {
      TCHAR buffer[MAX_RESULT_LENGTH];
      if (getMetricFromAgent(_T("System.Uptime"), buffer, MAX_RESULT_LENGTH) == DCE_SUCCESS)
      {
         m_bootTime = time(nullptr) - _tcstol(buffer, nullptr, 0);
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("StatusPoll(%s [%d]): boot time set to %u from agent"), m_name, m_id, (UINT32)m_bootTime);
      }
      else if (getMetricFromSNMP(m_snmpPort, SNMP_VERSION_DEFAULT, _T(".1.3.6.1.2.1.1.3.0"), buffer, MAX_RESULT_LENGTH, SNMP_RAWTYPE_NONE) == DCE_SUCCESS)
      {
         m_bootTime = time(nullptr) - _tcstol(buffer, nullptr, 0) / 100;   // sysUpTime is in hundredths of a second
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("StatusPoll(%s [%d]): boot time set to %u from SNMP"), m_name, m_id, (UINT32)m_bootTime);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("StatusPoll(%s [%d]): unable to get system uptime"), m_name, m_id);
      }
   }
   else
   {
      m_bootTime = 0;
   }

   // Get agent uptime to check if it was restarted
   if (!(m_state & DCSF_UNREACHABLE) && isNativeAgent())
   {
      TCHAR buffer[MAX_RESULT_LENGTH];
      if (getMetricFromAgent(_T("Agent.Uptime"), buffer, MAX_RESULT_LENGTH) == DCE_SUCCESS)
      {
         time_t oldAgentuptime = m_agentUpTime;
         m_agentUpTime = _tcstol(buffer, nullptr, 0);
         if (oldAgentuptime > m_agentUpTime)
         {
            // agent was restarted, cancel existing file monitors (on agent side they should have been cancelled by restart)
            RemoveFileMonitorsByNodeId(m_id);
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("StatusPoll(%s [%d]): unable to get agent uptime"), m_name, m_id);
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
      if (!geoLocationRetrieved && isSNMPSupported() && (m_driver != nullptr))
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
            PostSystemEvent(EVENT_AGENT_LOG_PROBLEM, m_id, "ds", status, _T("could not open"));
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
            PostSystemEvent(EVENT_AGENT_LOCAL_DATABASE_PROBLEM, m_id, "ds", status, statusDescription[status]);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("StatusPoll(%s [%d]): unable to get agent local database status"), m_name, m_id);
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
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("StatusPoll(%s [%d]): cannot get user agent information"), m_name, m_id);
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
      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("StatusPoll(%s [%d]): calling hook in module %s"), m_name, m_id, CURRENT_MODULE.szName);
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
      PostSystemEvent(EVENT_NODE_CAPABILITIES_CHANGED, m_id, "xx", oldCapabilities, m_capabilities);

   if (oldState != m_state)
   {
      markAsModified(MODIFY_RUNTIME); // only notify clients
   }

   if (oldCapabilities != m_capabilities)
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
         NetworkPathCheckResult result = checkNetworkPathElement(zone->getProxyNodeId(this), _T("zone proxy"), true, false, requestId, secondPass);
         if (result.rootCauseFound)
            return result;
      }
   }

   // Check directly connected switch
   sendPollerMsg(_T("Checking ethernet connectivity...\r\n"));
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
         shared_ptr<NetObj> cp = FindInterfaceConnectionPoint(iface->getMacAddr(), &type);
         if (cp != nullptr)
         {
            nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6,
                     _T("Node::checkNetworkPath(%s [%d]): found connection point: %s [%d]"), m_name, m_id, cp->getName(), cp->getId());
            if (secondPass)
            {
               shared_ptr<Node> node = (cp->getObjectClass() == OBJECT_INTERFACE) ? static_cast<Interface*>(cp.get())->getParentNode() : static_cast<AccessPoint*>(cp.get())->getParentNode();
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
               if (ap->getStatus() == STATUS_CRITICAL)   // FIXME: how to correctly determine if AP is down?
               {
                  nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Node::checkNetworkPath(%s [%d]): wireless access point %s [%d] is down"),
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

               static const TCHAR *names[] =
                        { _T("protocol"), _T("destNodeId"), _T("destAddress"),
                          _T("sourceNodeId"), _T("sourceAddress"), _T("prefix"),
                          _T("prefixLength"), _T("nextHopNodeId"), _T("nextHopAddress")
                        };
               PostSystemEventWithNames(EVENT_ROUTING_LOOP_DETECTED, prevHop->object->getId(), "siAiAAdiA", names,
                     (trace->getSourceAddress().getFamily() == AF_INET6) ? _T("IPv6") : _T("IPv4"),
                     m_id, &m_ipAddress, g_dwMgmtNode, &(trace->getSourceAddress()),
                     &prevHop->route, prevHop->route.getMaskBits(), hop->object->getId(), &prevHop->nextHop);

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

   nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Node::checkNetworkPath(%s [%d]): will do second pass"), m_name, m_id);

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
   sendPollerMsg(_T("   Checking agent policy deployment\r\n"));

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
            _sntprintf(data->debugId, 256, _T("%s [%u] from %s/%s"), getName(), getId(), _T("unknown"), guid.toString().cstr());

            TCHAR key[64];
            _sntprintf(key, 64, _T("AgentPolicyDeployment_%u"), m_id);
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
      PostSystemEvent(EVENT_IP_ADDRESS_CHANGED, m_id, "AA", &ipAddr, &m_ipAddress);

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
      if (!success && (m_capabilities & NC_HAS_ENTITY_MIB))
      {
         // Try to get hardware information from ENTITY MIB
         lockProperties();
         if (m_components != nullptr)
         {
            const Component *root = m_components->getRoot();
            if (root != nullptr)
            {
               // Device could be reported as but consist from single chassis only
               if ((root->getClass() == 11) && (root->getChildren()->size() == 1) && (root->getChildren()->get(0)->getClass() == 3))
               {
                  root = root->getChildren()->get(0);
               }
               if ((root->getClass() == 3) || (root->getClass() == 11)) // Chassis or stack
               {
                  _tcslcpy(hwInfo.vendor, root->getVendor(), 128);
                  _tcslcpy(hwInfo.productName, (*root->getModel() != 0) ? root->getModel() : root->getDescription(), 128);
                  _tcslcpy(hwInfo.productVersion, root->getFirmware(), 16);
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

   static const TCHAR *eventParamNames[] =
            { _T("category"), _T("type"), _T("vendor"), _T("model"),
              _T("location"), _T("partNumber"), _T("serialNumber"),
              _T("capacity"), _T("description") };
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
               PostSystemEventWithNames(EVENT_HARDWARE_COMPONENT_ADDED, m_id, "sssssssDs", eventParamNames, categoryNames[c->getCategory()],
                        c->getType(), c->getVendor(), c->getModel(), c->getLocation(), c->getPartNumber(),
                        c->getSerialNumber(), c->getCapacity(), c->getDescription());
               break;
            case CHANGE_REMOVED:
               nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): hardware component \"%s %s (%s)\" serial number %s removed"),
                        m_name, categoryNames[c->getCategory()], c->getModel(), c->getType(), c->getSerialNumber());
               sendPollerMsg(_T("   %s %s (%s) removed, serial number %s\r\n"),
                        categoryNames[c->getCategory()], c->getModel(), c->getType(), c->getSerialNumber());
               PostSystemEventWithNames(EVENT_HARDWARE_COMPONENT_REMOVED, m_id, "sssssssDs", eventParamNames, categoryNames[c->getCategory()],
                        c->getType(), c->getVendor(), c->getModel(), c->getLocation(), c->getPartNumber(),
                        c->getSerialNumber(), c->getCapacity(), c->getDescription());
               break;
         }
      }
      delete changes;
      delete m_hardwareComponents;
      setModified(MODIFY_HARDWARE_INVENTORY);
   }
   else if (components != nullptr)
      setModified(MODIFY_HARDWARE_INVENTORY);

   m_hardwareComponents = components;
   unlockProperties();
   return true;
}

/**
 * Update list of software packages for node
 */
bool Node::updateSoftwarePackages(PollerInfo *poller, uint32_t requestId)
{
   static const TCHAR *eventParamNames[] = { _T("name"), _T("version"), _T("previousVersion") };

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
               PostSystemEventWithNames(EVENT_PACKAGE_INSTALLED, m_id, "ss", eventParamNames, p->getName(), p->getVersion());
               break;
            case CHANGE_REMOVED:
               nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): package %s version %s removed"), m_name, p->getName(), p->getVersion());
               sendPollerMsg(_T("   Package %s version %s removed\r\n"), p->getName(), p->getVersion());
               PostSystemEventWithNames(EVENT_PACKAGE_REMOVED, m_id, "ss", eventParamNames, p->getName(), p->getVersion());
               break;
            case CHANGE_UPDATED:
               SoftwarePackage *prev = changes->get(++i);   // next entry contains previous package version
               nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): package %s updated (%s -> %s)"), m_name, p->getName(), prev->getVersion(), p->getVersion());
               sendPollerMsg(_T("   Package %s updated (%s -> %s)\r\n"), p->getName(), prev->getVersion(), p->getVersion());
               PostSystemEventWithNames(EVENT_PACKAGE_UPDATED, m_id, "sss", eventParamNames, p->getName(), p->getVersion(), prev->getVersion());
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
   PostSystemEvent(EVENT_DUPLICATE_NODE_DELETED, g_dwMgmtNode, "dssdsss",
            data->originalNode->getId(), data->originalNode->getName(), data->originalNode->getPrimaryHostName().cstr(),
            data->duplicateNode->getId(), data->duplicateNode->getName(), data->duplicateNode->getPrimaryHostName().cstr(),
            data->reason);
   data->duplicateNode->deleteObject();
   // Calling updateObjectIndexes will update all indexes that could be broken
   // by deleting duplicate IP address entries
   data->originalNode->updateObjectIndexes();
   delete data;
}

/**
 * Exit from "unreachable" state f full configuration poll was successful on any communication method (agent, SNMP, etc.)
 */
#define ExitFromUnreachableState() do { \
   if ((m_runtimeFlags & NDF_RECHECK_CAPABILITIES) && (m_state & DCSF_UNREACHABLE)) \
   { \
      int reason = (m_state & DCSF_NETWORK_PATH_PROBLEM) ? 1 : 0; \
      m_state &= ~(DCSF_UNREACHABLE | DCSF_NETWORK_PATH_PROBLEM); \
      PostSystemEvent(EVENT_NODE_UP, m_id, "d", reason); \
      sendPollerMsg(POLLER_INFO _T("Node recovered from unreachable state\r\n")); \
      m_recoveryTime = time(nullptr); \
   } \
} while(0)

/**
 * Start forced configuration poll on node
 */
void Node::startForcedConfigurationPoll()
{
   lockProperties();
   m_runtimeFlags |= NDF_IGNORE_UNREACHABLE_STATE;
   unlockProperties();
   Pollable::startForcedConfigurationPoll();
}

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

   uint32_t oldCapabilities = m_capabilities;
   uint32_t modified = 0;

   sendPollerMsg(_T("Starting configuration of for node %s\r\n"), m_name);
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 4, _T("Starting configuration poll of node %s (ID: %d)"), m_name, m_id);

   // Check for forced capabilities recheck
   if (m_runtimeFlags & NDF_RECHECK_CAPABILITIES)
   {
      sendPollerMsg(POLLER_WARNING _T("Capability reset\r\n"));
      lockProperties();
      m_capabilities &= NC_IS_LOCAL_MGMT; // reset all except "local management" flag
      m_runtimeFlags &= ~ODF_CONFIGURATION_POLL_PASSED;
      MemFreeAndNull(m_snmpObjectId);
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

   // Check if node is marked as unreachable
   if (!(m_state & DCSF_UNREACHABLE) || (m_runtimeFlags & (NDF_RECHECK_CAPABILITIES | NDF_IGNORE_UNREACHABLE_STATE)))
   {
      updatePrimaryIpAddr();

      poller->setStatus(_T("capability check"));
      sendPollerMsg(_T("Checking node's capabilities...\r\n"));

      if (confPollAgent(rqId))
         modified |= MODIFY_NODE_PROPERTIES;

      if ((oldCapabilities & NC_IS_NATIVE_AGENT) && !(m_capabilities & NC_IS_NATIVE_AGENT))
         m_lastAgentCommTime = TIMESTAMP_NEVER;

      POLL_CANCELLATION_CHECKPOINT();

      // Setup permanent connection to agent if not present (needed for proper configuration re-sync)
      if (m_capabilities & NC_IS_NATIVE_AGENT)
      {
         ExitFromUnreachableState();

         uint32_t error, socketError;
         bool newConnection;
         agentLock();
         connectToAgent(&error, &socketError, &newConnection, true);
         agentUnlock();

         // Check if agent was marked as unreachable before full poll
         if ((m_state & NSF_AGENT_UNREACHABLE) && (m_runtimeFlags & NDF_RECHECK_CAPABILITIES))
         {
            m_state &= ~NSF_AGENT_UNREACHABLE;
            PostSystemEvent(EVENT_AGENT_OK, m_id, nullptr);
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
      }

      POLL_CANCELLATION_CHECKPOINT();

      if (confPollSnmp(rqId))
         modified |= MODIFY_NODE_PROPERTIES;

      ExitFromUnreachableState();

      // Check if SNMP was marked as unreachable before full poll
      if ((m_capabilities & NC_IS_SNMP) && (m_state & NSF_SNMP_UNREACHABLE) && (m_runtimeFlags & NDF_RECHECK_CAPABILITIES))
      {
         m_state &= ~NSF_SNMP_UNREACHABLE;
         PostSystemEvent(EVENT_SNMP_OK, m_id, nullptr);
         sendPollerMsg(POLLER_INFO _T("   Connectivity with SNMP agent restored\r\n"));
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("ConfPoll(%s): Connectivity with SNMP agent restored"), m_name);
         m_pollCountSNMP = 0;
      }

      POLL_CANCELLATION_CHECKPOINT();

      if (confPollSsh(rqId))
         modified |= MODIFY_NODE_PROPERTIES;

      ExitFromUnreachableState();

      // Check if SSH was marked as unreachable before full poll
      if ((m_capabilities & NC_IS_SSH) && (m_state & NSF_SSH_UNREACHABLE) && (m_runtimeFlags & NDF_RECHECK_CAPABILITIES))
      {
         m_state &= ~NSF_SSH_UNREACHABLE;
         PostSystemEvent(EVENT_SSH_OK, m_id, nullptr);
         sendPollerMsg(POLLER_INFO _T("   SSH connectivity restored\r\n"));
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("ConfPoll(%s): SSH connectivity restored"), m_name);
         m_pollCountSSH = 0;
      }

      POLL_CANCELLATION_CHECKPOINT();

      if (confPollEthernetIP(rqId))
         modified |= MODIFY_NODE_PROPERTIES;

      ExitFromUnreachableState();

      // Check if Ethernet/IP was marked as unreachable before full poll
      if ((m_capabilities & NC_IS_ETHERNET_IP) && (m_state & NSF_ETHERNET_IP_UNREACHABLE) && (m_runtimeFlags & NDF_RECHECK_CAPABILITIES))
      {
         m_state &= ~NSF_ETHERNET_IP_UNREACHABLE;
         PostSystemEvent(EVENT_ETHERNET_IP_OK, m_id, nullptr);
         sendPollerMsg(POLLER_INFO _T("   EtherNet/IP connectivity restored\r\n"));
         m_pollCountEtherNetIP = 0;
      }

      POLL_CANCELLATION_CHECKPOINT();

      // Generate event if node flags has been changed
      if (oldCapabilities != m_capabilities)
      {
         PostSystemEvent(EVENT_NODE_CAPABILITIES_CHANGED, m_id, "xx", oldCapabilities, m_capabilities);
         modified |= MODIFY_NODE_PROPERTIES;
      }

      // Update system description
      TCHAR buffer[MAX_RESULT_LENGTH] = { 0 };
      if ((m_capabilities & NC_IS_NATIVE_AGENT) && !(m_flags & NF_DISABLE_NXCP))
      {
         getMetricFromAgent(_T("System.Uname"), buffer, MAX_RESULT_LENGTH);
      }
      else if ((m_capabilities & NC_IS_SNMP) && !(m_flags & NF_DISABLE_SNMP))
      {
         getMetricFromSNMP(m_snmpPort, SNMP_VERSION_DEFAULT, _T(".1.3.6.1.2.1.1.1.0"), buffer, MAX_RESULT_LENGTH, SNMP_RAWTYPE_NONE);
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
            m_runtimeFlags &= ~(ODF_CONFIGURATION_POLL_PENDING | NDF_RECHECK_CAPABILITIES | NDF_IGNORE_UNREACHABLE_STATE);
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

      // Call hooks in loaded modules
      ENUMERATE_MODULES(pfConfPollHook)
      {
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfigurationPoll(%s [%d]): calling hook in module %s"), m_name, m_id, CURRENT_MODULE.szName);
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
   }
   else
   {
      sendPollerMsg(POLLER_WARNING _T("Node is marked as unreachable, configuration poll aborted\r\n"));
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 4, _T("Node is marked as unreachable, configuration poll aborted"));
   }

   UpdateAssetLinkage(this);
   autoFillAssetProperties();

   // Finish configuration poll
   poller->setStatus(_T("cleanup"));
   lockProperties();
   m_runtimeFlags &= ~(NDF_RECHECK_CAPABILITIES | NDF_IGNORE_UNREACHABLE_STATE);
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
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("Node::detectNodeType(%s [%d]): SNMP node, driver name is %s"),
               m_name, m_id, (m_driver != nullptr) ? m_driver->getName() : _T("(not set)"));

      bool vtypeReportedByDevice = false;
      if (m_driver != nullptr)
      {
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
      }

      if (!vtypeReportedByDevice)
      {
         // Assume physical device if it supports SNMP and driver is not "GENERIC" nor "NET-SNMP"
         if ((m_driver != nullptr) && _tcscmp(m_driver->getName(), _T("GENERIC")) && _tcscmp(m_driver->getName(), _T("NET-SNMP")))
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
bool Node::confPollAgent(UINT32 rqId)
{
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): checking for NetXMS agent Flags={%08X} StateFlags={%08X} RuntimeFlags={%08X}"), m_name, m_flags, m_state, m_runtimeFlags);
   if (((m_capabilities & NC_IS_NATIVE_AGENT) && (m_state & NSF_AGENT_UNREACHABLE)) || (m_flags & NF_DISABLE_NXCP))
   {
      sendPollerMsg(_T("   NetXMS agent polling is %s\r\n"), (m_flags & NF_DISABLE_NXCP) ? _T("disabled") : _T("not possible"));
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
      if (!m_ipAddress.isValidUnicast() && !((m_capabilities & NC_IS_LOCAL_MGMT) && m_ipAddress.isLoopback()))
      {
         sendPollerMsg(POLLER_ERROR _T("   Node primary IP is invalid and there are no active tunnels\r\n"));
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): node primary IP is invalid and there are no active tunnels"), m_name);
         return false;
      }
      pAgentConn = make_shared<AgentConnectionEx>(m_id, m_ipAddress, m_agentPort, m_agentSecret, isAgentCompressionAllowed());//direct connection to agent without tunnel
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
         if (hStmt != NULL)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_zoneUIN);

            DB_RESULT hResult = DBSelectPrepared(hStmt);
            if (hResult != NULL)
            {
               int count = DBGetNumRows(hResult);
               for(int i = 0; i < count; i++)
                  secrets.addPreallocated(DBGetField(hResult, i, 0, NULL, 0));
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
         PostSystemEvent(EVENT_AGENT_OK, m_id, nullptr);
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
            PostSystemEvent(EVENT_AGENT_ID_CHANGED, m_id, "GG", &m_agentId, &agentId);
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
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): AgentConnection::getSupportedParameters() failed: rcc=%d"), m_name, rcc);
      }

      // Get supported Windows Performance Counters
      if (!_tcsncmp(m_platformName, _T("windows-"), 8) && (!(m_flags & NF_DISABLE_PERF_COUNT)))
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
bool Node::confPollEthernetIP(uint32_t requestId)
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
         PostSystemEvent(EVENT_ETHERNET_IP_OK, m_id, nullptr);
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
      _sntprintf(buffer, 64, _T("%u.%u"), identity->productRevisionMajor, identity->productRevisionMinor);
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
 * SNMP walker callback which sets indicator to true after first varbind and aborts walk
 */
static UINT32 IndicatorSnmpWalkerCallback(SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
   *static_cast<bool*>(arg) = true;
   return SNMP_ERR_COMM;
}

/**
 * Configuration poll: check for SNMP
 */
bool Node::confPollSnmp(uint32_t requestId)
{
   if (((m_capabilities & NC_IS_SNMP) && (m_state & NSF_SNMP_UNREACHABLE)) ||
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
   m_capabilities |= NC_IS_SNMP;
   if (m_state & NSF_SNMP_UNREACHABLE)
   {
      m_state &= ~NSF_SNMP_UNREACHABLE;
      PostSystemEvent(EVENT_SNMP_OK, m_id, nullptr);
      sendPollerMsg(POLLER_INFO _T("   Connectivity with SNMP agent restored\r\n"));
   }
   unlockProperties();
   sendPollerMsg(_T("   SNMP agent is active (version %s)\r\n"),
            (m_snmpVersion == SNMP_VERSION_3) ? _T("3") : ((m_snmpVersion == SNMP_VERSION_2C) ? _T("2c") : _T("1")));
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): SNMP agent detected (version %s)"), m_name,
            (m_snmpVersion == SNMP_VERSION_3) ? _T("3") : ((m_snmpVersion == SNMP_VERSION_2C) ? _T("2c") : _T("1")));

   TCHAR szBuffer[4096];
   if (SnmpGet(m_snmpVersion, pTransport, _T(".1.3.6.1.2.1.1.2.0"), nullptr, 0, szBuffer, sizeof(szBuffer), SG_STRING_RESULT) != SNMP_ERR_SUCCESS)
   {
      // Set snmp object ID to .0.0 if it cannot be read
      _tcscpy(szBuffer, _T(".0.0"));
   }
   lockProperties();
   if (_tcscmp(CHECK_NULL_EX(m_snmpObjectId), szBuffer))
   {
      MemFree(m_snmpObjectId);
      m_snmpObjectId = MemCopyString(szBuffer);
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
   m_driver->analyzeDevice(pTransport, CHECK_NULL_EX(m_snmpObjectId), this, &m_driverData);
   if (m_driverData != nullptr)
   {
      m_driverData->attachToNode(m_id, m_guid, m_name);
   }

   NDD_MODULE_LAYOUT layout;
   m_driver->getModuleLayout(pTransport, this, m_driverData, 1, &layout); // TODO module set to 1
   if (layout.numberingScheme == NDD_PN_UNKNOWN)
   {
      // Try to find port numbering information in database
      LookupDevicePortLayout(SNMP_ObjectId::parse(CHECK_NULL_EX(m_snmpObjectId)), &layout);
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
   if (CheckSNMPIntegerValue(pTransport, _T(".1.3.6.1.2.1.4.1.0"), 1))
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
   if ((SnmpGet(m_snmpVersion, pTransport, _T(".1.3.6.1.2.1.47.1.4.1.0"), nullptr, 0, szBuffer, sizeof(szBuffer), SG_RAW_RESULT) == SNMP_ERR_SUCCESS) ||
       (SnmpGet(m_snmpVersion, pTransport, _T(".1.3.6.1.2.1.47.1.1.1.1.5"), nullptr, 0, szBuffer, sizeof(szBuffer), SG_GET_NEXT_REQUEST | SG_RAW_RESULT) == SNMP_ERR_SUCCESS))
   {
      lockProperties();
      m_capabilities |= NC_HAS_ENTITY_MIB;
      unlockProperties();

      TCHAR debugInfo[256];
      _sntprintf(debugInfo, 256, _T("%s [%u]"), m_name, m_id);
      shared_ptr<ComponentTree> components = BuildComponentTree(pTransport, debugInfo);
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
      unlockProperties();
   }
   else
   {
      lockProperties();
      m_capabilities &= ~NC_HAS_ENTITY_MIB;
      unlockProperties();
   }

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
   bool lldpMIB = (SnmpGet(m_snmpVersion, pTransport, _T(".1.0.8802.1.1.2.1.3.2.0"), nullptr, 0, szBuffer, sizeof(szBuffer), 0) == SNMP_ERR_SUCCESS) ||
                  (SnmpGet(m_snmpVersion, pTransport, _T(".1.0.8802.1.1.2.1.3.6.0"), nullptr, 0, szBuffer, sizeof(szBuffer), 0) == SNMP_ERR_SUCCESS);
   bool lldpV2MIB = (SnmpGet(m_snmpVersion, pTransport, _T(".1.3.111.2.802.1.1.13.1.3.2.0"), nullptr, 0, szBuffer, sizeof(szBuffer), 0) == SNMP_ERR_SUCCESS);
   if (lldpMIB || lldpV2MIB)
   {
      lockProperties();
      m_capabilities |= NC_IS_LLDP;
      if (lldpV2MIB)
         m_capabilities |= NC_LLDP_V2_MIB;
      else
         m_capabilities &= ~NC_LLDP_V2_MIB;
      unlockProperties();

      int32_t type;
      BYTE data[256];
      uint32_t dataLen;
      if ((SnmpGetEx(pTransport, LLDP_OID(_T("1.3.1.0"), lldpV2MIB), nullptr, 0, &type, sizeof(int32_t), 0, nullptr) == SNMP_ERR_SUCCESS) &&
          (SnmpGetEx(pTransport, LLDP_OID(_T("1.3.2.0"), lldpV2MIB), nullptr, 0, data, 256, SG_RAW_RESULT, &dataLen) == SNMP_ERR_SUCCESS))
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
   if (CheckSNMPIntegerValue(pTransport, _T(".1.0.8802.1.1.1.1.1.1.0"), 1))
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
   if ((m_driver != nullptr) && m_driver->isWirelessController(pTransport, this, m_driverData))
   {
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): node is wireless controller, reading access point information"), m_name);
      sendPollerMsg(_T("   Reading wireless access point information\r\n"));
      lockProperties();
      m_capabilities |= NC_IS_WIFI_CONTROLLER;
      unlockProperties();

      int clusterMode = m_driver->getClusterMode(pTransport, this, m_driverData);

      ObjectArray<AccessPointInfo> *aps = m_driver->getAccessPoints(pTransport, this, m_driverData);
      if (aps != nullptr)
      {
         sendPollerMsg(POLLER_INFO _T("   %d wireless access points found\r\n"), aps->size());
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): got information about %d access points"), m_name, aps->size());
         int adopted = 0;
         for(int i = 0; i < aps->size(); i++)
         {
            AccessPointInfo *info = aps->get(i);
            if (info->getState() == AP_ADOPTED)
               adopted++;

            bool newAp = false;
            shared_ptr<AccessPoint> ap;
            if (info->getMacAddr().isValid())
            {
               ap = (clusterMode == CLUSTER_MODE_STANDALONE) ?
                        findAccessPointByMAC(info->getMacAddr()) : FindAccessPointByMAC(info->getMacAddr());
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): Invalid MAC address on access point %s"), m_name, info->getName());
               if (clusterMode == CLUSTER_MODE_STANDALONE)
               {
                  ap = static_pointer_cast<AccessPoint>(findChildObject(info->getName(), OBJECT_ACCESSPOINT));
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): Cannot find access point in cluster mode without MAC address"), m_name);
                  continue;   // Ignore this record
               }
            }
            if (ap == nullptr)
            {
               StringBuffer name;
               if (info->getName() != nullptr)
               {
                  name = info->getName();
               }
               else
               {
                  for(int j = 0; j < info->getRadioInterfaces()->size(); j++)
                  {
                     if (j > 0)
                        name += _T("/");
                     name += info->getRadioInterfaces()->get(j)->name;
                  }
               }
               ap = make_shared<AccessPoint>(name.cstr(), info->getIndex(), info->getMacAddr());
               NetObjInsert(ap, true, false);
               nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): created new access point object %s [%d]"), m_name, ap->getName(), ap->getId());
               newAp = true;
            }
            ap->attachToNode(m_id);
            ap->setIpAddress(info->getIpAddr());
            if ((info->getState() == AP_ADOPTED) || newAp)
            {
               ap->updateRadioInterfaces(info->getRadioInterfaces());
               ap->updateInfo(info->getVendor(), info->getModel(), info->getSerial());
            }
            ap->unhide();
            ap->updateState(info->getState());
         }

         if (clusterMode == CLUSTER_MODE_STANDALONE)
         {
            // Delete access points no longer reported by controller
            unique_ptr<SharedObjectArray<NetObj>> apList = getChildren(OBJECT_ACCESSPOINT);
            for(int i = 0; i < apList->size(); i++)
            {
               auto ap = static_cast<AccessPoint*>(apList->get(i));
               bool found = false;
               for(int j = 0; j < aps->size(); j++)
               {
                  AccessPointInfo *info = aps->get(j);
                  if (ap->getMacAddr().equals(info->getMacAddr()) && !_tcscmp(ap->getName(), info->getName()))
                  {
                     found = true;
                     break;
                  }
               }
               if (!found)
               {
                  nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): deleting non-existent access point %s [%d]"),
                           m_name, ap->getName(), ap->getId());
                  ap->deleteObject();
               }
            }
         }

         lockProperties();
         m_adoptedApCount = adopted;
         m_totalApCount = aps->size();
         unlockProperties();

         delete aps;
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): failed to read access point information"), m_name);
         sendPollerMsg(POLLER_ERROR _T("   Failed to read access point information\r\n"));
      }
   }
   else
   {
      lockProperties();
      m_capabilities &= ~NC_IS_WIFI_CONTROLLER;
      unlockProperties();
   }

   if (ConfigReadBoolean(_T("EnableCheckPointSNMP"), false))
   {
      // Check for CheckPoint SNMP agent on port 161
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): checking for CheckPoint SNMP"), m_name);
      TCHAR szBuffer[4096];
      if (SnmpGet(SNMP_VERSION_1, pTransport, _T(".1.3.6.1.4.1.2620.1.1.10.0"), nullptr, 0, szBuffer, sizeof(szBuffer), 0) == SNMP_ERR_SUCCESS)
      {
         lockProperties();
         if (_tcscmp(CHECK_NULL_EX(m_snmpObjectId), _T(".1.3.6.1.4.1.2620.1.1")))
         {
            MemFree(m_snmpObjectId);
            m_snmpObjectId = MemCopyString(_T(".1.3.6.1.4.1.2620.1.1"));
            hasChanges = true;
         }

         m_capabilities |= NC_IS_SNMP | NC_IS_ROUTER;
         m_state &= ~NSF_SNMP_UNREACHABLE;
         unlockProperties();
         sendPollerMsg(POLLER_INFO _T("   CheckPoint SNMP agent on port 161 is active\r\n"));
      }
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
bool Node::confPollSsh(uint32_t requestId)
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
   }
   else
   {
      sendPollerMsg(_T("   Cannot connect to SSH\r\n"));
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("ConfPoll(%s): SSH unreachable"), m_name);
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
void Node::checkBridgeMib(SNMP_Transport *pTransport)
{
   // Older Cisco Nexus software (7.x probably) does not return base bridge address but still
   // support some parts of bridge MIB. We do mark such devices as isBridge to allow more correct
   // L2 topology maps and device visualization.
   BYTE baseBridgeAddress[64];
   memset(baseBridgeAddress, 0, sizeof(baseBridgeAddress));
   uint32_t portCount = 0;
   if ((SnmpGet(m_snmpVersion, pTransport, _T(".1.3.6.1.2.1.17.1.1.0"), nullptr, 0, baseBridgeAddress, sizeof(baseBridgeAddress), SG_RAW_RESULT) == SNMP_ERR_SUCCESS) ||
       (SnmpGet(m_snmpVersion, pTransport, _T(".1.3.6.1.2.1.17.1.2.0"), nullptr, 0, &portCount, sizeof(uint32_t), 0) == SNMP_ERR_SUCCESS))
   {
      lockProperties();
      m_capabilities |= NC_IS_BRIDGE;
      memcpy(m_baseBridgeAddress, baseBridgeAddress, 6);
      unlockProperties();

      // Check for Spanning Tree (IEEE 802.1d) MIB support
      uint32_t stpVersion;
      if (SnmpGet(m_snmpVersion, pTransport, _T(".1.3.6.1.2.1.17.2.1.0"), nullptr, 0, &stpVersion, sizeof(uint32_t), 0) == SNMP_ERR_SUCCESS)
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
void Node::checkIfXTable(SNMP_Transport *pTransport)
{
   bool present = false;
   SnmpWalk(pTransport, _T(".1.3.6.1.2.1.31.1.1.1.1"), IndicatorSnmpWalkerCallback, &present);
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
      sendPollerMsg(POLLER_WARNING _T("   Duplicate interface \"%s\" deleted\r\n"), iface->getName());
      deleteInterface(iface);
   }

   return deleteList.size() > 0;
}

/**
 * Update interface configuration
 */
bool Node::updateInterfaceConfiguration(uint32_t requestId)
{
   sendPollerMsg(_T("Checking interface configuration...\r\n"));

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
            PostSystemEvent(EVENT_INTERFACE_DELETED, m_id, "dsAd", iface->getIfIndex(), iface->getName(), &addr, addr.getMaskBits());
            deleteInterface(iface);
         }
         hasChanges = true;
      }

      // Add new interfaces and check configuration of existing
      for(int j = 0; j < ifList->size(); j++)
      {
         InterfaceInfo *ifInfo = ifList->get(j);
         shared_ptr<Interface> pInterface;
         bool isNewInterface = true;
         bool interfaceUpdated = false;

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
                      !pInterface->getMacAddr().equals(ifInfo->macAddr, MAC_ADDR_LENGTH))
                  {
                     TCHAR szOldMac[64], szNewMac[64];
                     pInterface->getMacAddr().toString(szOldMac);
                     MACToStr(ifInfo->macAddr, szNewMac);
                     PostSystemEvent(EVENT_MAC_ADDR_CHANGED, m_id, "idsss",
                               pInterface->getId(), pInterface->getIfIndex(),
                               pInterface->getName(), szOldMac, szNewMac);
                     pInterface->setMacAddr(MacAddress(ifInfo->macAddr, MAC_ADDR_LENGTH), true);
                     interfaceUpdated = true;
                  }
                  TCHAR expandedName[MAX_OBJECT_NAME];
                  pInterface->expandName(ifInfo->name, expandedName);
                  if (_tcscmp(expandedName, pInterface->getName()))
                  {
                     pInterface->setName(expandedName);
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
                  if (ifInfo->speed != pInterface->getSpeed())
                  {
                     pInterface->setSpeed(ifInfo->speed);
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

                  // Check for deleted IPs and changed masks
                  const InetAddressList *ifList = pInterface->getIpAddressList();
                  for(int n = 0; n < ifList->size(); n++)
                  {
                     const InetAddress& ifAddr = ifList->get(n);
                     const InetAddress& addr = ifInfo->ipAddrList.findAddress(ifAddr);
                     if (addr.isValid())
                     {
                        if (addr.getMaskBits() != ifAddr.getMaskBits())
                        {
                           PostSystemEvent(EVENT_IF_MASK_CHANGED, m_id, "dsAddd", pInterface->getId(), pInterface->getName(),
                                     &addr, addr.getMaskBits(), pInterface->getIfIndex(), ifAddr.getMaskBits());
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
                        PostSystemEvent(EVENT_IF_IPADDR_DELETED, m_id, "dsAdd", pInterface->getId(), pInterface->getName(),
                                  &ifAddr, ifAddr.getMaskBits(), pInterface->getIfIndex());
                        pInterface->deleteIpAddress(ifAddr);
                        interfaceUpdated = true;
                     }
                  }

                  // Check for added IPs
                  for(int m = 0; m < ifInfo->ipAddrList.size(); m++)
                  {
                     const InetAddress& addr = ifInfo->ipAddrList.get(m);
                     if (!ifList->hasAddress(addr))
                     {
                        pInterface->addIpAddress(addr);
                        PostSystemEvent(EVENT_IF_IPADDR_ADDED, m_id, "dsAdd", pInterface->getId(), pInterface->getName(),
                                  &addr, addr.getMaskBits(), pInterface->getIfIndex());
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
            PostSystemEvent(EVENT_INTERFACE_DELETED, m_id, "dsAd", iface->getIfIndex(), iface->getName(), &addr, addr.getMaskBits());
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
               if (macAddr.isValid() && !macAddr.equals(iface->getMacAddr()))
               {
                  TCHAR oldMAC[32], newMAC[32];
                  iface->getMacAddr().toString(oldMAC);
                  macAddr.toString(newMAC);
                  nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("Node::updateInterfaceConfiguration(%s [%u]): MAC change for unknown interface: %s to %s"), m_name, m_id, oldMAC, newMAC);
                  PostSystemEvent(EVENT_MAC_ADDR_CHANGED, m_id, "idsss",
                            iface->getId(), iface->getIfIndex(),
                            iface->getName(), oldMAC, newMAC);
                  iface->setMacAddr(macAddr, true);
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
   if (dco->getInstanceDiscoveryData() == nullptr)
      return nullptr;

   shared_ptr<Node> node;
   if (dco->getSourceNode() != 0)
   {
      node = static_pointer_cast<Node>(FindObjectById(dco->getSourceNode(), OBJECT_NODE));
      if (node == nullptr)
      {
         nxlog_debug(6, _T("Node::getInstanceList(%s [%d]): source node [%d] not found"), dco->getName().cstr(), dco->getId(), dco->getSourceNode());
         return nullptr;
      }
      if (!node->isTrustedObject(m_id))
      {
         nxlog_debug(6, _T("Node::getInstanceList(%s [%d]): this node (%s [%d]) is not trusted by source node %s [%d]"),
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
   switch(dco->getInstanceDiscoveryMethod())
   {
      case IDM_AGENT_LIST:
         node->getListFromAgent(dco->getInstanceDiscoveryData(), &instances);
         break;
      case IDM_AGENT_TABLE:
      case IDM_INTERNAL_TABLE:
         if (dco->getInstanceDiscoveryMethod() == IDM_AGENT_TABLE)
            node->getTableFromAgent(dco->getInstanceDiscoveryData(), &instanceTable);
         else
            node->getInternalTable(dco->getInstanceDiscoveryData(), &instanceTable);
         if (instanceTable != nullptr)
         {
            TCHAR buffer[1024];
            instances = new StringList();
            for(int i = 0; i < instanceTable->getNumRows(); i++)
            {
               instanceTable->buildInstanceString(i, buffer, 1024);
               instances->add(buffer);
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
         TCHAR query[256];
         _sntprintf(query, 256, _T("PDH.ObjectInstances(\"%s\")"), EscapeStringForAgent(dco->getInstanceDiscoveryData()).cstr());
         node->getListFromAgent(query, &instances);
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
 * Connect to SM-CLP agent. Assumes that access to SM-CLP connection is already locked.
 */
bool Node::connectToSMCLP()
{
   // Create new connection object if needed
   if (m_smclpConnection == nullptr)
   {
      m_smclpConnection = new SMCLP_Connection(m_ipAddress.getAddressV4(), 23);
      DbgPrintf(7, _T("Node::connectToSMCLP(%s [%d]): new connection created"), m_name, m_id);
   }
   else
   {
      // Check if we already connected
      if (m_smclpConnection->checkConnection())
      {
         DbgPrintf(7, _T("Node::connectToSMCLP(%s [%d]): already connected"), m_name, m_id);
         return true;
      }

      // Close current connection or clean up after broken connection
      m_smclpConnection->disconnect();
      delete m_smclpConnection;
      m_smclpConnection = new SMCLP_Connection(m_ipAddress.getAddressV4(), 23);
      DbgPrintf(7, _T("Node::connectToSMCLP(%s [%d]): existing connection reset"), m_name, m_id);
   }

   TCHAR login[64], password[64];
   if ((getCustomAttribute(_T("iLO.login"), login, 64) != nullptr) &&
       (getCustomAttribute(_T("iLO.password"), password, 64) != nullptr))
      return m_smclpConnection->connect(login, password);
   return false;
}

/**
 * Connect to native agent. Assumes that access to agent connection is already locked.
 */
bool Node::connectToAgent(UINT32 *error, UINT32 *socketError, bool *newConnection, bool forceConnect)
{
   if ((g_flags & AF_SHUTDOWN) || m_isDeleteInitiated)
      return false;

   if (!forceConnect && (m_agentConnection == nullptr) && (time(nullptr) - m_lastAgentConnectAttempt < 30))
   {
      nxlog_debug_tag(DEBUG_TAG_AGENT, 7, _T("Node::connectToAgent(%s [%d]): agent is unreachable, will not retry connection"), m_name, m_id);
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
      nxlog_debug_tag(DEBUG_TAG_AGENT, 7, _T("Node::connectToAgent(%s [%d]): %s and there are no active tunnels"), m_name, m_id,
               (m_flags & NF_AGENT_OVER_TUNNEL_ONLY) ? _T("direct agent connections are disabled") : _T("node primary IP is invalid"));
      return false;
   }

   // Create new agent connection object if needed
   if (m_agentConnection == nullptr)
   {
      m_agentConnection = (tunnel != nullptr) ?
               make_shared<AgentConnectionEx>(m_id, tunnel, m_agentSecret, isAgentCompressionAllowed()) :
               make_shared<AgentConnectionEx>(m_id, m_ipAddress, m_agentPort, m_agentSecret, isAgentCompressionAllowed());
      nxlog_debug_tag(DEBUG_TAG_AGENT, 7, _T("Node::connectToAgent(%s [%d]): new agent connection created"), m_name, m_id);
   }
   else
   {
      // Check if we already connected
      if (m_agentConnection->nop() == ERR_SUCCESS)
      {
         DbgPrintf(7, _T("Node::connectToAgent(%s [%d]): already connected"), m_name, m_id);
         if (newConnection != nullptr)
            *newConnection = false;
         setLastAgentCommTime();
         return true;
      }

      // Close current connection or clean up after broken connection
      m_agentConnection->disconnect();
      m_agentConnection->setTunnel(tunnel);
      nxlog_debug_tag(DEBUG_TAG_AGENT, 7, _T("Node::connectToAgent(%s [%d]): existing connection reset"), m_name, m_id);
   }
   if (newConnection != nullptr)
      *newConnection = true;
   m_agentConnection->setPort(m_agentPort);
   m_agentConnection->setSharedSecret(m_agentSecret);
   if (tunnel == nullptr)
      setAgentProxy(m_agentConnection.get());
   m_agentConnection->setCommandTimeout(g_agentCommandTimeout);
   DbgPrintf(7, _T("Node::connectToAgent(%s [%d]): calling connect on port %d"), m_name, m_id, (int)m_agentPort);
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
         DbgPrintf(5, _T("Node::connectToAgent(%s [%d]): cannot set server ID on agent (%s)"), m_name, m_id, AgentErrorCodeToText(rcc));
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
      DbgPrintf(7, _T("Node(%s)->getMetricFromSNMP(%s): snmpResult=%d"), m_name, name, SNMP_ERR_COMM);
      return DCErrorFromSNMPError(SNMP_ERR_COMM);
   }

   if (size < 64)
   {
      DbgPrintf(7, _T("Node(%s)->getMetricFromSNMP(%s): result buffer is too small"), m_name, name);
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
         snmpResult = SnmpGetEx(snmp, name, nullptr, 0, rawValue, 1024, SG_RAW_RESULT, nullptr);
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
                  IpToStr(ntohl(*reinterpret_cast<uint32_t*>(rawValue)), buffer);
                  break;
               case SNMP_RAWTYPE_MAC_ADDR:
                  MACToStr(rawValue, buffer);
                  break;
               default:
                  buffer[0] = 0;
                  break;
            }
         }
      }
      delete snmp;
   }
   else
   {
      snmpResult = SNMP_ERR_COMM;
   }
   DbgPrintf(7, _T("Node(%s)->getMetricFromSNMP(%s): snmpResult=%u"), m_name, name, snmpResult);
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
 * Callback for SnmpWalk in Node::getTableFromSNMP
 */
static UINT32 SNMPGetTableCallback(SNMP_Variable *varbind, SNMP_Transport *snmp, void *arg)
{
   ((ObjectArray<SNMP_ObjectId> *)arg)->add(new SNMP_ObjectId(varbind->getName()));
   return SNMP_ERR_SUCCESS;
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
   uint32_t rc = SnmpWalk(snmp, oid, SNMPGetTableCallback, &oidList);
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
static uint32_t SNMPGetListCallback(SNMP_Variable *varbind, SNMP_Transport *snmp, void *arg)
{
   bool convert = false;
   TCHAR buffer[256];
   static_cast<StringList*>(arg)->add(varbind->getValueAsPrintableString(buffer, 256, &convert));
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
 * Information for SNMPOIDSuffixListCallback
 */
struct SNMPOIDSuffixListCallback_Data
{
   size_t oidLen;
   StringMap *values;
};

/**
 * Callback for SnmpWalk in Node::getOIDSuffixListFromSNMP
 */
static uint32_t SNMPOIDSuffixListCallback(SNMP_Variable *varbind, SNMP_Transport *snmp, void *arg)
{
   SNMPOIDSuffixListCallback_Data *data = (SNMPOIDSuffixListCallback_Data *)arg;
   const SNMP_ObjectId& oid = varbind->getName();
   if (oid.length() <= data->oidLen)
      return SNMP_ERR_SUCCESS;
   TCHAR buffer[256];
   SnmpConvertOIDToText(oid.length() - data->oidLen, &(oid.value()[data->oidLen]), buffer, 256);

   const TCHAR *key = (buffer[0] == _T('.')) ? &buffer[1] : buffer;

   TCHAR value[256] = _T("");
   bool convert = false;
   varbind->getValueAsPrintableString(value, 256, &convert);
   data->values->set(key, (value[0] != 0) ? value : key);
   return SNMP_ERR_SUCCESS;
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

   SNMPOIDSuffixListCallback_Data data;
   uint32_t oidBin[256];
   data.oidLen = SnmpParseOID(oid, oidBin, 256);
   if (data.oidLen == 0)
   {
      delete snmp;
      return DCE_NOT_SUPPORTED;
   }

   data.values = new StringMap;
   uint32_t rc = SnmpWalk(snmp, oid, SNMPOIDSuffixListCallback, &data);
   delete snmp;
   if (rc == SNMP_ERR_SUCCESS)
   {
      *values = data.values;
   }
   else
   {
      delete data.values;
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
   nxlog_debug(7, _T("Node(%s)->getMetricFromAgent(%s): dwError=%d dwResult=%d"), m_name, name, agentError, rc);
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
   DbgPrintf(7, _T("Node(%s)->getTableFromAgent(%s): dwError=%d dwResult=%d"), m_name, name, error, result);
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
   DbgPrintf(7, _T("Node(%s)->getListFromAgent(%s): dwError=%d dwResult=%d"), m_name, name, agentError, rc);
   return rc;
}

/**
 * Get item's value via SM-CLP protocol
 */
DataCollectionError Node::getMetricFromSMCLP(const TCHAR *param, TCHAR *buffer, size_t size)
{
   DataCollectionError result = DCE_COMM_ERROR;
   int tries = 3;

   if (m_state & DCSF_UNREACHABLE)
      return DCE_COMM_ERROR;

   smclpLock();

   // Establish connection if needed
   if (m_smclpConnection == nullptr)
      if (!connectToSMCLP())
         goto end_loop;

   while(tries-- > 0)
   {
      // Get parameter
      TCHAR path[MAX_PARAM_NAME];
      _tcslcpy(path, param, MAX_PARAM_NAME);
      TCHAR *attr = _tcsrchr(path, _T('/'));
      if (attr != nullptr)
      {
         *attr = 0;
         attr++;
      }
      TCHAR *value = m_smclpConnection->get(path, attr);
      if (value != nullptr)
      {
         _tcslcpy(buffer, value, size);
         free(value);
         result = DCE_SUCCESS;
         break;
      }
      else
      {
         if (!connectToSMCLP())
            result = DCE_COMM_ERROR;
         else
            result = DCE_NOT_SUPPORTED;
      }
   }

end_loop:
   smclpUnlock();
   DbgPrintf(7, _T("Node(%s)->GetItemFromSMCLP(%s): result=%d"), m_name, param, result);
   return result;
}

/**
 * Get metric value from network device driver
 */
DataCollectionError Node::getMetricFromDeviceDriver(const TCHAR *param, TCHAR *buffer, size_t size)
{
   lockProperties();
   NetworkDeviceDriver *driver = m_driver;
   unlockProperties();

   if ((driver == nullptr) || !driver->hasMetrics())
      return DCE_NOT_SUPPORTED;

   SNMP_Transport *transport = createSnmpTransport();
   if (transport == nullptr)
      return DCE_COMM_ERROR;
   DataCollectionError rc = driver->getMetric(transport, this, m_driverData, param, buffer, size);
   delete transport;
   return rc;
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
      RoutingTable *rt = getRoutingTable();
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

         for(int i = 0; i < rt->size(); i++)
         {
            auto *r = rt->get(i);
            table->addRow();
            table->set(0, i + 1);
            InetAddress arrd = InetAddress(r->dwDestAddr);
            table->set(1, arrd.toString());
            table->set(2, static_cast<uint32_t>(BitsInMask(r->dwDestMask)));
            arrd = InetAddress(r->dwNextHop);
            table->set(3, arrd.toString());
            table->set(4, r->dwIfIndex);
            auto iface = findInterfaceByIndex(r->dwIfIndex);
            table->set(5, (iface != nullptr) ? iface->getName() : _T(""));
            table->set(6, r->dwRouteType);
         }
         delete rt;
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
   TCHAR channel[64];
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
   else if (!_tcsicmp(_T("ICMP.PacketLoss"), name))
   {
      rc = getIcmpStatistic(nullptr, IcmpStatFunction::LOSS, buffer);
   }
   else if (MatchString(_T("ICMP.PacketLoss(*)"), name, FALSE))
   {
      rc = getIcmpStatistic(name, IcmpStatFunction::LOSS, buffer);
   }
   else if (!_tcsicmp(_T("ICMP.ResponseTime.Average"), name))
   {
      rc = getIcmpStatistic(nullptr, IcmpStatFunction::AVERAGE, buffer);
   }
   else if (MatchString(_T("ICMP.ResponseTime.Average(*)"), name, FALSE))
   {
      rc = getIcmpStatistic(name, IcmpStatFunction::AVERAGE, buffer);
   }
   else if (!_tcsicmp(_T("ICMP.ResponseTime.Last"), name))
   {
      rc = getIcmpStatistic(nullptr, IcmpStatFunction::LAST, buffer);
   }
   else if (MatchString(_T("ICMP.ResponseTime.Last(*)"), name, FALSE))
   {
      rc = getIcmpStatistic(name, IcmpStatFunction::LAST, buffer);
   }
   else if (!_tcsicmp(_T("ICMP.ResponseTime.Max"), name))
   {
      rc = getIcmpStatistic(nullptr, IcmpStatFunction::MAX, buffer);
   }
   else if (MatchString(_T("ICMP.ResponseTime.Max(*)"), name, FALSE))
   {
      rc = getIcmpStatistic(name, IcmpStatFunction::MAX, buffer);
   }
   else if (!_tcsicmp(_T("ICMP.ResponseTime.Min"), name))
   {
      rc = getIcmpStatistic(nullptr, IcmpStatFunction::MIN, buffer);
   }
   else if (MatchString(_T("ICMP.ResponseTime.Min(*)"), name, FALSE))
   {
      rc = getIcmpStatistic(name, IcmpStatFunction::MIN, buffer);
   }
   else if (MatchString(_T("Net.IP.NextHop(*)"), name, FALSE))
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
   else if (MatchString(_T("NetSvc.ResponseTime(*)"), name, FALSE))
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
   else if (!_tcsicmp(name, _T("WirelessController.AdoptedAPCount")))
   {
      if (m_capabilities & NC_IS_WIFI_CONTROLLER)
      {
         IntegerToString(m_adoptedApCount, buffer);
      }
      else
      {
         rc = DCE_NOT_SUPPORTED;
      }
   }
   else if (!_tcsicmp(name, _T("WirelessController.TotalAPCount")))
   {
      if (m_capabilities & NC_IS_WIFI_CONTROLLER)
      {
         IntegerToString(m_totalApCount, buffer);
      }
      else
      {
         rc = DCE_NOT_SUPPORTED;
      }
   }
   else if (!_tcsicmp(name, _T("WirelessController.UnadoptedAPCount")))
   {
      if (m_capabilities & NC_IS_WIFI_CONTROLLER)
      {
         IntegerToString(m_totalApCount - m_adoptedApCount, buffer);
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
         ret_int(buffer, GetAlarmCount());
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
         TCHAR loginName[256];
         AgentGetParameterArg(name, 1, loginName, 256);
         IntegerToString(GetSessionCount(true, false, -1, loginName), buffer);
      }
      else if (!_tcsicmp(name, _T("Server.ClientSessions.Desktop")))
      {
         IntegerToString(GetSessionCount(true, true, CLIENT_TYPE_DESKTOP, nullptr), buffer);
      }
      else if (MatchString(_T("Server.ClientSessions.Desktop(*)"), name, false))
      {
         TCHAR loginName[256];
         AgentGetParameterArg(name, 1, loginName, 256);
         IntegerToString(GetSessionCount(true, false, CLIENT_TYPE_DESKTOP, loginName), buffer);
      }
      else if (!_tcsicmp(name, _T("Server.ClientSessions.Mobile")))
      {
         IntegerToString(GetSessionCount(true, true, CLIENT_TYPE_MOBILE, nullptr), buffer);
      }
      else if (MatchString(_T("Server.ClientSessions.Mobile(*)"), name, false))
      {
         TCHAR loginName[256];
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
         g_idxObjectById.forEach([&dciCount](NetObj *object) {
            if (object->isDataCollectionTarget())
               dciCount += static_cast<DataCollectionTarget*>(object)->getItemCount();
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
      else if (!_tcsicmp(name, _T("Server.ObjectCount.Clusters")))
      {
         ret_uint(buffer, static_cast<uint32_t>(g_idxClusterById.size()));
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
         TCHAR driver[64], metric[64];
         AgentGetParameterArg(name, 1, driver, 64);
         AgentGetParameterArg(name, 2, metric, 64);
         rc = GetPerfDataStorageDriverMetric(driver, metric, buffer);
      }
      else if (MatchString(_T("Server.QueueSize.Average(*)"), name, false))
      {
         rc = GetQueueStatistic(name, StatisticType::AVERAGE, buffer);
      }
      else if (MatchString(_T("Server.QueueSize.Current(*)"), name, false))
      {
         rc = GetQueueStatistic(name, StatisticType::CURRENT, buffer);
      }
      else if (MatchString(_T("Server.QueueSize.Max(*)"), name, false))
      {
         rc = GetQueueStatistic(name, StatisticType::MAX, buffer);
      }
      else if (MatchString(_T("Server.QueueSize.Min(*)"), name, false))
      {
         rc = GetQueueStatistic(name, StatisticType::MIN, buffer);
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
 * Translate DCI error code into RCC
 */
static inline uint32_t RCCFromDCIError(DataCollectionError error)
{
   switch(error)
   {
      case DCE_SUCCESS:
         return RCC_SUCCESS;
      case DCE_COMM_ERROR:
         return RCC_COMM_FAILURE;
      case DCE_NO_SUCH_INSTANCE:
         return RCC_NO_SUCH_INSTANCE;
      case DCE_NOT_SUPPORTED:
         return RCC_DCI_NOT_SUPPORTED;
      case DCE_COLLECTION_ERROR:
         return RCC_AGENT_ERROR;
      case DCE_ACCESS_DENIED:
         return RCC_ACCESS_DENIED;
      default:
         return RCC_SYSTEM_FAILURE;
   }
}

/**
 * Get item's value for client
 */
uint32_t Node::getMetricForClient(int origin, uint32_t userId, const TCHAR *name, TCHAR *buffer, size_t size)
{
   DataCollectionError rc = DCE_ACCESS_DENIED;

   // Get data from node
   switch(origin)
   {
      case DS_INTERNAL:
         if (checkAccessRights(userId, OBJECT_ACCESS_READ))
            rc = getInternalMetric(name, buffer, size);
         break;
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
      default:
         return RCC_INVALID_ARGUMENT;
   }

   return RCCFromDCIError(rc);
}

/**
 * Get table for client
 */
uint32_t Node::getTableForClient(const TCHAR *name, shared_ptr<Table> *table)
{
   return RCCFromDCIError(getTableFromAgent(name, table));
}

/**
 * Create NXCP message with object's data
 */
void Node::fillMessageInternal(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageInternal(msg, userId);
   msg->setField(VID_IP_ADDRESS, m_ipAddress);
   msg->setField(VID_PRIMARY_NAME, m_primaryHostName);
   msg->setField(VID_NODE_TYPE, (INT16)m_type);
   msg->setField(VID_NODE_SUBTYPE, m_subType);
   msg->setField(VID_HYPERVISOR_TYPE, m_hypervisorType);
   msg->setField(VID_HYPERVISOR_INFO, m_hypervisorInfo);
   msg->setField(VID_VENDOR, m_vendor);
   msg->setField(VID_PRODUCT_CODE, m_productCode);
   msg->setField(VID_PRODUCT_NAME, m_productName);
   msg->setField(VID_PRODUCT_VERSION, m_productVersion);
   msg->setField(VID_SERIAL_NUMBER, m_serialNumber);
   msg->setField(VID_STATE_FLAGS, m_state);
   msg->setField(VID_CAPABILITIES, m_capabilities);
   msg->setField(VID_AGENT_PORT, m_agentPort);
   msg->setField(VID_AGENT_CACHE_MODE, m_agentCacheMode);
   msg->setField(VID_SHARED_SECRET, m_agentSecret);
   msg->setFieldFromMBString(VID_SNMP_AUTH_OBJECT, m_snmpSecurity->getCommunity());
   msg->setFieldFromMBString(VID_SNMP_AUTH_PASSWORD, m_snmpSecurity->getAuthPassword());
   msg->setFieldFromMBString(VID_SNMP_PRIV_PASSWORD, m_snmpSecurity->getPrivPassword());
   msg->setField(VID_SNMP_USM_METHODS, (WORD)((WORD)m_snmpSecurity->getAuthMethod() | ((WORD)m_snmpSecurity->getPrivMethod() << 8)));
   msg->setField(VID_SNMP_OID, CHECK_NULL_EX(m_snmpObjectId));
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
   if (m_driver != nullptr)
   {
      msg->setField(VID_DRIVER_NAME, m_driver->getName());
      msg->setField(VID_DRIVER_VERSION, m_driver->getVersion());
   }
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
      msg->setField(VID_CIP_DEVICE_TYPE_NAME, CIP_DeviceTypeNameFromCode(m_cipDeviceType));
      msg->setField(VID_CIP_STATUS, m_cipStatus);
      msg->setField(VID_CIP_STATUS_TEXT, CIP_DecodeDeviceStatus(m_cipStatus));
      msg->setField(VID_CIP_EXT_STATUS_TEXT, CIP_DecodeExtendedDeviceStatus(m_cipStatus));
      msg->setField(VID_CIP_STATE, static_cast<uint16_t>(m_cipState));
      msg->setField(VID_CIP_STATE_TEXT, CIP_DeviceStateTextFromCode(m_cipState));
      msg->setField(VID_CIP_VENDOR_CODE, m_cipVendorCode);
   }

   msg->setField(VID_CERT_MAPPING_METHOD, static_cast<int16_t>(m_agentCertMappingMethod));
   msg->setField(VID_CERT_MAPPING_DATA, m_agentCertMappingData);
   msg->setField(VID_AGENT_CERT_SUBJECT, m_agentCertSubject);
   msg->setFieldFromUtf8String(VID_SYSLOG_CODEPAGE, m_syslogCodepage);
   msg->setFieldFromUtf8String(VID_SNMP_CODEPAGE, m_snmpCodepage);
   msg->setField(VID_OSPF_ROUTER_ID, InetAddress(m_ospfRouterId));

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
uint32_t Node::modifyFromMessageInternal(const NXCPMessage& msg)
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
      m_snmpSecurity->setAuthName(mbBuffer);

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

   return super::modifyFromMessageInternal(msg);
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
      DbgPrintf(4, _T("Node::onSnmpProxyChange(%s [%d]): data collection sync needed for %s [%d]"), m_name, m_id, node->getName(), node->getId());
      static_cast<Node*>(node.get())->forceSyncDataCollectionConfig();
   }

   // resync data collection configuration with old proxy
   node = FindObjectById(oldProxy, OBJECT_NODE);
   if (node != nullptr)
   {
      DbgPrintf(4, _T("Node::onSnmpProxyChange(%s [%d]): data collection sync needed for %s [%d]"), m_name, m_id, node->getName(), node->getId());
      static_cast<Node*>(node.get())->forceSyncDataCollectionConfig();
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
 * Get status of interface with given index from SNMP agent
 */
void Node::getInterfaceStatusFromSNMP(SNMP_Transport *pTransport, uint32_t index, int ifTableSuffixLen, uint32_t *ifTableSuffix, InterfaceAdminState *adminState, InterfaceOperState *operState)
{
   if (m_driver != nullptr)
   {
      m_driver->getInterfaceState(pTransport, this, m_driverData, index, ifTableSuffixLen, ifTableSuffix, adminState, operState);
   }
   else
   {
      *adminState = IF_ADMIN_STATE_UNKNOWN;
      *operState = IF_OPER_STATE_UNKNOWN;
   }
}

/**
 * Get status of interface with given index from native agent
 */
void Node::getInterfaceStatusFromAgent(UINT32 index, InterfaceAdminState *adminState, InterfaceOperState *operState)
{
   TCHAR szParam[128], szBuffer[32];

   // Get administrative status
   _sntprintf(szParam, 128, _T("Net.Interface.AdminStatus(%u)"), index);
   if (getMetricFromAgent(szParam, szBuffer, 32) == DCE_SUCCESS)
   {
      *adminState = (InterfaceAdminState)_tcstol(szBuffer, nullptr, 0);

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
            _sntprintf(szParam, 128, _T("Net.Interface.Link(%u)"), index);
            if (getMetricFromAgent(szParam, szBuffer, 32) == DCE_SUCCESS)
            {
               UINT32 dwLinkState = _tcstoul(szBuffer, nullptr, 0);
               *operState = (dwLinkState == 0) ? IF_OPER_STATE_DOWN : IF_OPER_STATE_UP;
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
      DbgPrintf(6, _T("Node[%s]::writeParamListToMessage(): sending %d parameters (origin=%d)"), m_name, parameters->size(), origin);
   }
   else
   {
      DbgPrintf(6, _T("Node[%s]::writeParamListToMessage(): parameter list is missing (origin=%d)"), m_name, origin);
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
      DbgPrintf(6, _T("Node[%s]::writeParamListToMessage(): sending %d tables (origin=%d)"), m_name, tables->size(), origin);
   }
   else
   {
      DbgPrintf(6, _T("Node[%s]::writeParamListToMessage(): table list is missing (origin=%d)"), m_name, origin);
      pMsg->setField(VID_NUM_TABLES, (UINT32)0);
   }

   unlockProperties();
}

/**
 * Put list of supported Windows performance counters into NXCP message
 */
void Node::writeWinPerfObjectsToMessage(NXCPMessage *msg)
{
   lockProperties();

   if (m_winPerfObjects != nullptr)
   {
      msg->setField(VID_NUM_OBJECTS, (UINT32)m_winPerfObjects->size());

      UINT32 id = VID_PARAM_LIST_BASE;
      for(int i = 0; i < m_winPerfObjects->size(); i++)
      {
         WinPerfObject *o = m_winPerfObjects->get(i);
         id = o->fillMessage(msg, id);
      }
      DbgPrintf(6, _T("Node[%s]::writeWinPerfObjectsToMessage(): sending %d objects"), m_name, m_winPerfObjects->size());
   }
   else
   {
      DbgPrintf(6, _T("Node[%s]::writeWinPerfObjectsToMessage(): m_winPerfObjects == nullptr"), m_name);
      msg->setField(VID_NUM_OBJECTS, (UINT32)0);
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
   if (object.getId() == m_pollerNode)
   {
      // If deleted object is our poller node, change it to default
      m_pollerNode = 0;
      setModified(MODIFY_NODE_PROPERTIES);
      DbgPrintf(3, _T("Node::onObjectDelete(%s [%u]): poller node %s [%u] deleted"), m_name, m_id, object.getName(), object.getId());
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
   if (SnmpGet(m_snmpVersion, snmp, _T(".1.3.6.1.2.1.14.1.2.0"), nullptr, 0, &adminStatus, sizeof(int32_t), 0) == SNMP_ERR_SUCCESS)
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
      if (SnmpGet(m_snmpVersion, snmp, _T(".1.3.6.1.2.1.14.1.1.0"), nullptr, 0, &routerId, sizeof(int32_t), 0) == SNMP_ERR_SUCCESS)
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
       (m_state & NSF_AGENT_UNREACHABLE) ||
       (m_state & DCSF_UNREACHABLE) ||
       (m_status == STATUS_UNMANAGED) ||
       m_isDeleteInitiated)
      return shared_ptr<AgentConnectionEx>();

   shared_ptr<AgentConnectionEx> conn;
   shared_ptr<AgentTunnel> tunnel = GetTunnelForNode(m_id);
   if (tunnel != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_AGENT, 6, _T("Node::createAgentConnection(%s [%d]): using agent tunnel"), m_name, (int)m_id);
      conn = make_shared<AgentConnectionEx>(m_id, tunnel, m_agentSecret, isAgentCompressionAllowed());
   }
   else
   {
      if ((!m_ipAddress.isValidUnicast() && !((m_capabilities & NC_IS_LOCAL_MGMT) && m_ipAddress.isLoopback())) || (m_flags & NF_AGENT_OVER_TUNNEL_ONLY))
      {
         nxlog_debug_tag(DEBUG_TAG_AGENT, 7, _T("Node::createAgentConnection(%s [%d]): %s and there are no active tunnels"), m_name, m_id,
                  (m_flags & NF_AGENT_OVER_TUNNEL_ONLY) ? _T("direct agent connections are disabled") : _T("node primary IP is invalid"));
         return shared_ptr<AgentConnectionEx>();
      }
      conn = make_shared<AgentConnectionEx>(m_id, m_ipAddress, m_agentPort, m_agentSecret, isAgentCompressionAllowed());
      if (!setAgentProxy(conn.get()))
      {
         return shared_ptr<AgentConnectionEx>();
      }
   }
   conn->setCommandTimeout(g_agentCommandTimeout);
   if (!conn->connect(g_serverKey, nullptr, nullptr, sendServerId ? g_serverId : 0))
   {
      conn.reset();
   }
   else
   {
      setLastAgentCommTime();
   }
   nxlog_debug_tag(DEBUG_TAG_AGENT, 6, _T("Node::createAgentConnection(%s [%d]): conn=%p"), m_name, (int)m_id, conn.get());
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
   m_proxyConnections[type].lock();

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
         unlockProperties(); //removing possible deadlock
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
      deleteParent(*subnets[i]);
      subnets[i]->deleteChild(*this);
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
 * Get routing table from node
 */
RoutingTable *Node::getRoutingTable()
{
   RoutingTable *routingTable = nullptr;

   if ((m_capabilities & NC_IS_NATIVE_AGENT) && (!(m_flags & NF_DISABLE_NXCP)))
   {
      shared_ptr<AgentConnectionEx> conn = getAgentConnection();
      if (conn != nullptr)
      {
         routingTable = conn->getRoutingTable();
      }
   }
   if ((routingTable == nullptr) && (m_capabilities & NC_IS_SNMP) && (!(m_flags & NF_DISABLE_SNMP)))
   {
      SNMP_Transport *pTransport = createSnmpTransport();
      if (pTransport != nullptr)
      {
         routingTable = SnmpGetRoutingTable(pTransport);
         delete pTransport;
      }
   }

   if (routingTable != nullptr)
   {
      SortRoutingTable(routingTable);
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
      for(int i = 0; i < m_routingTable->size(); i++)
      {
         ROUTE *route = m_routingTable->get(i);
         if ((destAddr.getAddressV4() & route->dwDestMask) == route->dwDestAddr)
         {
            *srcIfIndex = route->dwIfIndex;
            shared_ptr<Interface> iface = findInterfaceByIndex(route->dwIfIndex);
            if (iface != nullptr)
            {
               *srcAddr = iface->getIpAddressList()->getFirstUnicastAddressV4();
            }
            else
            {
               *srcAddr = m_ipAddress;  // use primary IP if outward interface does not have IP address or cannot be found
            }
            found = true;
            break;
         }
      }
   }
   else
   {
      DbgPrintf(6, _T("Node::getOutwardInterface(%s [%d]): no routing table"), m_name, m_id);
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
      for(int i = 0; i < m_routingTable->size(); i++)
      {
         ROUTE *entry = m_routingTable->get(i);
         if ((!nextHopFound || (entry->dwDestMask == 0xFFFFFFFF)) &&
             ((destAddr.getAddressV4() & entry->dwDestMask) == entry->dwDestAddr))
         {
            shared_ptr<Interface> iface = findInterfaceByIndex(entry->dwIfIndex);
            if ((entry->dwNextHop == 0) && (iface != nullptr) &&
                (iface->getIpAddressList()->getFirstUnicastAddressV4().getHostBits() == 0))
            {
               // On Linux XEN VMs can be pointed by individual host routes to virtual interfaces
               // where each vif has netmask 255.255.255.255 and next hop in routing table set to 0.0.0.0
               *nextHop = destAddr;
            }
            else
            {
               *nextHop = entry->dwNextHop;
            }
            *route = entry->dwDestAddr;
            route->setMaskBits(BitsInMask(entry->dwDestMask));
            *ifIndex = entry->dwIfIndex;
            *isVpn = false;
            if (iface != nullptr)
            {
               _tcslcpy(name, iface->getName(), MAX_OBJECT_NAME);
            }
            else
            {
               _sntprintf(name, MAX_OBJECT_NAME, _T("%d"), entry->dwIfIndex);
            }
            nextHopFound = true;
            break;
         }
      }
   }
   else
   {
      DbgPrintf(6, _T("Node::getNextHop(%s [%d]): no routing table"), m_name, m_id);
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
   auto routingTable = getRoutingTable();
   if (routingTable != nullptr)
   {
      routingTableLock();
      delete m_routingTable;
      m_routingTable = routingTable;
      routingTableUnlock();
      nxlog_debug_tag(DEBUG_TAG_ROUTES_POLL, 5, _T("Routing table updated for node %s [%d]"), m_name, m_id);
   }
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
         proxyNode = zone->getProxyNodeId(this);
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
   nxlog_debug(4, _T("Node::PrepareForDeletion(%s [%u]): waiting for outstanding polls to finish"), m_name, m_id);
   while (m_statusPollState.isPending() || m_configurationPollState.isPending() ||
          m_discoveryPollState.isPending() || m_routingPollState.isPending() ||
          m_topologyPollState.isPending() || m_instancePollState.isPending() ||
          m_icmpPollState.isPending())
   {
      ThreadSleepMs(100);
   }
   nxlog_debug(4, _T("Node::PrepareForDeletion(%s [%u]): no outstanding polls left"), m_name, m_id);

   UnbindAgentTunnel(m_id, 0);

   // Clear possible references to self and other nodes
   m_lastKnownNetworkPath.reset();

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
shared_ptr<Cluster> Node::getMyCluster()
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
 * Generic function for calculating effective proxy node ID
 */
static uint32_t GetEffectiveProtocolProxy(Node *node, uint32_t configuredProxy, uint32_t zoneUIN, bool backup = false, uint32_t defaultProxy = 0)
{
   uint32_t effectiveProxy = backup ? 0 : configuredProxy;
   if (IsZoningEnabled() && (effectiveProxy == 0) && (zoneUIN != 0))
   {
      // Use zone default proxy if set
      shared_ptr<Zone> zone = FindZoneByUIN(zoneUIN);
      if (zone != nullptr)
      {
         effectiveProxy = zone->isProxyNode(node->getId()) ? node->getId() : zone->getProxyNodeId(node, backup);
      }
   }
   return (effectiveProxy != 0) ? effectiveProxy : defaultProxy;
}

/**
 * Get effective SNMP proxy for this node
 */
uint32_t Node::getEffectiveSnmpProxy(bool backup)
{
   return GetEffectiveProtocolProxy(this, m_snmpProxy, m_zoneUIN, backup);
}

/**
 * Get effective EtherNet/IP proxy for this node
 */
uint32_t Node::getEffectiveEtherNetIPProxy(bool backup)
{
   return GetEffectiveProtocolProxy(this, m_eipProxy, m_zoneUIN, backup);
}

/**
 * Get effective MQTT proxy for this node
 */
uint32_t Node::getEffectiveMqttProxy()
{
   return GetEffectiveProtocolProxy(this, m_mqttProxy, m_zoneUIN, false, g_dwMgmtNode);
}

/**
 * Get effective SSH proxy for this node
 */
uint32_t Node::getEffectiveSshProxy()
{
   return GetEffectiveProtocolProxy(this, m_sshProxy, m_zoneUIN, false, g_dwMgmtNode);
}

/**
 * Get effective TCP proxy for this node
 */
uint32_t Node::getEffectiveTcpProxy()
{
   return GetEffectiveProtocolProxy(this, 0, m_zoneUIN, false, g_dwMgmtNode);
}

/**
 * Get effective ICMP proxy for this node
 */
uint32_t Node::getEffectiveIcmpProxy()
{
   return GetEffectiveProtocolProxy(this, 0, m_zoneUIN);
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
         agentProxy = zone->getProxyNodeId(this);
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

   SNMP_Transport *pTransport = nullptr;
   uint32_t snmpProxy = getEffectiveSnmpProxy();
   if (snmpProxy == 0)
   {
      pTransport = new SNMP_UDPTransport;
      static_cast<SNMP_UDPTransport*>(pTransport)->createUDPTransport(m_ipAddress, (port != 0) ? port : m_snmpPort);
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
            pTransport = new SNMP_ProxyTransport(conn, (snmpProxy == m_id) ? InetAddress::LOOPBACK : m_ipAddress, (port != 0) ? port : m_snmpPort);
         }
      }
   }

   // Set security
   if (pTransport != nullptr)
   {
      lockProperties();
      SNMP_Version effectiveVersion = (version != SNMP_VERSION_DEFAULT) ? version : m_snmpVersion;
      pTransport->setSnmpVersion(effectiveVersion);
      if (m_snmpCodepage[0] != 0)
      {
         pTransport->setCodepage(m_snmpCodepage);
      }
      else if (g_snmpCodepage[0] != 0)
      {
         pTransport->setCodepage(g_snmpCodepage);
      }

      if (context == nullptr)
      {
         if (community == nullptr)
            pTransport->setSecurityContext(new SNMP_SecurityContext(m_snmpSecurity));
         else
            pTransport->setSecurityContext(new SNMP_SecurityContext(community));
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
            pTransport->setSecurityContext(new SNMP_SecurityContext(fullCommunity));
         }
         else
         {
            SNMP_SecurityContext *securityContext = new SNMP_SecurityContext(m_snmpSecurity);
            securityContext->setContextName(context);
            pTransport->setSecurityContext(securityContext);
         }
      }
      unlockProperties();
   }
   return pTransport;
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
 * Get current layer 2 topology (as dynamically created list which should be destroyed by caller)
 * Will return nullptr if there are no topology information or it is expired
 */
shared_ptr<NetworkMapObjectList> Node::getL2Topology()
{
   shared_ptr<NetworkMapObjectList> result;
   time_t expTime = ConfigReadULong(_T("Topology.AdHocRequest.ExpirationTime"), 900);
   m_topologyMutex.lock();
   if ((m_topology != nullptr) && (m_topologyRebuildTimestamp + expTime >= time(nullptr)))
   {
      result = m_topology;
   }
   m_topologyMutex.unlock();
   return result;
}

/**
 * Rebuild layer 2 topology and return it as dynamically reated list which should be destroyed by caller
 */
shared_ptr<NetworkMapObjectList> Node::buildL2Topology(uint32_t *status, int radius, bool includeEndNodes, bool useL1Topology)
{
   shared_ptr<NetworkMapObjectList> result;
   int nDepth = (radius < 0) ? ConfigReadInt(_T("Topology.DefaultDiscoveryRadius"), 5) : radius;

   m_topologyMutex.lock();
   if (m_linkLayerNeighbors != nullptr)
   {
      m_topologyMutex.unlock();

      result = make_shared<NetworkMapObjectList>();
      BuildL2Topology(*result, this, nDepth, includeEndNodes, useL1Topology);

      m_topologyMutex.lock();
      m_topology = result;
      m_topologyRebuildTimestamp = time(nullptr);
   }
   else
   {
      m_topology.reset();
      *status = RCC_NO_L2_TOPOLOGY_SUPPORT;
   }
   m_topologyMutex.unlock();
   return result;
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

   if (m_driver != nullptr)
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
         uint32_t oldCaps = m_capabilities;
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

   poller->setStatus(_T("reading FDB"));
   shared_ptr<ForwardingDatabase> fdb = GetSwitchForwardingDatabase(this);
   m_topologyMutex.lock();
   m_fdb = fdb;
   m_topologyMutex.unlock();
   if (fdb != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 4, _T("Switch forwarding database retrieved for node %s [%d]"), m_name, m_id);
      sendPollerMsg(POLLER_INFO _T("Switch forwarding database retrieved\r\n"));
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 4, _T("Failed to get switch forwarding database from node %s [%d]"), m_name, m_id);
      sendPollerMsg(POLLER_WARNING _T("Failed to get switch forwarding database\r\n"));
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
         if ((object != nullptr) && (object->getObjectClass() == OBJECT_NODE))
         {
            nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 5, _T("Node::topologyPoll(%s [%d]): found peer node %s [%d], localIfIndex=%d remoteIfIndex=%d"),
                      m_name, m_id, object->getName(), object->getId(), ni->ifLocal, ni->ifRemote);
            shared_ptr<Interface> ifLocal = findInterfaceByIndex(ni->ifLocal);
            shared_ptr<Interface> ifRemote = static_cast<Node*>(object.get())->findInterfaceByIndex(ni->ifRemote);
            nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 5, _T("Node::topologyPoll(%s [%d]): localIfObject=%s remoteIfObject=%s"), m_name, m_id,
                      (ifLocal != nullptr) ? ifLocal->getName() : _T("(null)"),
                      (ifRemote != nullptr) ? ifRemote->getName() : _T("(null)"));
            if ((ifLocal != nullptr) && (ifRemote != nullptr))
            {
               // Update old peers for local and remote interfaces, if any
               if ((ifRemote->getPeerInterfaceId() != 0) && (ifRemote->getPeerInterfaceId() != ifLocal->getId()))
               {
                  shared_ptr<Interface> ifOldPeer = static_pointer_cast<Interface>(FindObjectById(ifRemote->getPeerInterfaceId(), OBJECT_INTERFACE));
                  if (ifOldPeer != nullptr)
                  {
                     ifOldPeer->clearPeer();
                  }
               }
               if ((ifLocal->getPeerInterfaceId() != 0) && (ifLocal->getPeerInterfaceId() != ifRemote->getId()))
               {
                  shared_ptr<Interface> ifOldPeer = static_pointer_cast<Interface>(FindObjectById(ifLocal->getPeerInterfaceId(), OBJECT_INTERFACE));
                  if (ifOldPeer != nullptr)
                  {
                     ifOldPeer->clearPeer();
                  }
               }

               ifLocal->setPeer(static_cast<Node*>(object.get()), ifRemote.get(), ni->protocol, false);
               ifRemote->setPeer(this, ifLocal.get(), ni->protocol, true);
               sendPollerMsg(_T("   Local interface %s linked to remote interface %s:%s\r\n"),
                             ifLocal->getName(), object->getName(), ifRemote->getName());
               nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 5, _T("Local interface %s:%s linked to remote interface %s:%s"),
                         m_name, ifLocal->getName(), object->getName(), ifRemote->getName());
            }
         }
      }

      readLockChildList();
      for(int i = 0; i < getChildList().size(); i++)
      {
         if (getChildList().get(i)->getObjectClass() != OBJECT_INTERFACE)
            continue;

         Interface *iface = (Interface *)getChildList().get(i);

         // Clear self-linked interfaces caused by bug in previous release
         if ((iface->getPeerNodeId() == m_id) && (iface->getPeerInterfaceId() == iface->getId()))
         {
            iface->clearPeer();
            nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 6, _T("Node::topologyPoll(%s [%d]): Self-linked interface %s [%d] fixed"), m_name, m_id, iface->getName(), iface->getId());
         }
         // Remove outdated peer information
         else if (iface->getPeerNodeId() != 0)
         {
            shared_ptr<Node> peerNode = static_pointer_cast<Node>(FindObjectById(iface->getPeerNodeId(), OBJECT_NODE));
            if (peerNode == nullptr)
            {
               nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 6, _T("Node::topologyPoll(%s [%d]): peer node set but node object does not exist"), m_name, m_id);
               iface->clearPeer();
               continue;
            }

            // Remove peer information if more than one MAC address detected on port
            // Do not change information when interface is down
            if ((iface->getConfirmedOperState() == IF_OPER_STATE_UP) && (iface->getPeerDiscoveryProtocol() == LL_PROTO_FDB) && nbs->isMultipointInterface(iface->getIfIndex()))
            {
               shared_ptr<Interface> ifPeer = static_pointer_cast<Interface>(FindObjectById(iface->getPeerInterfaceId(), OBJECT_INTERFACE));
               if (ifPeer != nullptr)
               {
                  ifPeer->clearPeer();
               }
               iface->clearPeer();
               nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 6, _T("Node::topologyPoll(%s [%d]): Removed outdated peer information from interface %s [%d]"), m_name, m_id, iface->getName(), iface->getId());
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
   if ((m_driver != nullptr) && (m_capabilities & NC_IS_WIFI_CONTROLLER))
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
            nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 6, _T("%d wireless stations found on controller node %s [%d]"), stations->size(), m_name, m_id);

            for(int i = 0; i < stations->size(); i++)
            {
               WirelessStationInfo *ws = stations->get(i);

               shared_ptr<AccessPoint> ap = (ws->apMatchPolicy == AP_MATCH_BY_BSSID) ? findAccessPointByBSSID(ws->bssid) : findAccessPointByRadioId(ws->rfIndex);
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
      nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 5, _T("TopologyPoll(%s [%d]): calling hook in module %s"), m_name, m_id, CURRENT_MODULE.szName);
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
         TCHAR buffer[64];
         nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 6, _T("Node::addHostConnections(%s [%u]): found single MAC %s on interface %s"),
            m_name, m_id, macAddr.toString(buffer), ifLocal->getName());
         shared_ptr<Interface> ifRemote = FindInterfaceByMAC(macAddr);
         if (ifRemote != nullptr)
         {
            nxlog_debug_tag(DEBUG_TAG_TOPOLOGY_POLL, 6, _T("Node::addHostConnections(%s [%u]): found remote interface %s [%u]"),
               m_name, m_id, ifRemote->getName(), ifRemote->getId());
            shared_ptr<Node> peerNode = ifRemote->getParentNode();
            if (peerNode != nullptr)
            {
               LL_NEIGHBOR_INFO info;
               info.ifLocal = ifLocal->getIfIndex();
               info.ifRemote = ifRemote->getIfIndex();
               info.objectId = peerNode->getId();
               info.isPtToPt = true;
               info.protocol = LL_PROTO_FDB;
               info.isCached = false;
               nbs->addConnection(&info);
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
            nbs->addConnection(&info);
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
         VlanPortInfo *port = &(vlan->getPorts()[j]);
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
               iface = findInterfaceByLocation(vlan->getPorts()[j].location);
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
      if (vm.failureReason() == ScriptVMFailureReason::SCRIPT_IS_EMPTY)
         nxlog_debug(7, _T("Node::createSubnet(%s [%u]): hook script \"Hook::CreateSubnet\" is empty"), m_name, m_id);
      else
         nxlog_debug(7, _T("Node::createSubnet(%s [%u]): hook script \"Hook::CreateSubnet\" not found"), m_name, m_id);
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
               zone->addSubnet(subnet);
            }
            else
            {
               nxlog_debug(1, _T("Node::createSubnet(): Inconsistent configuration - zone %d does not exist"), (int)m_zoneUIN);
            }
         }
         else
         {
            g_entireNetwork->addSubnet(subnet);
         }
         nxlog_debug(4, _T("Node::createSubnet(): Created new subnet %s [%d] for node %s [%d]"),
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
   TCHAR buffer[64];

   shared_ptr<Cluster> pCluster = getMyCluster();

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
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("Checking subnet bindings for node %s [%d]"), m_name, m_id);
   for(int i = 0; i < addrList.size(); i++)
   {
      InetAddress addr = addrList.get(i);
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("Node::checkSubnetBinding(%s [%d]): checking address %s/%d"), m_name, m_id, addr.toString(buffer), addr.getMaskBits());

      shared_ptr<Interface> iface = findInterfaceByIP(addr);
      if (iface == nullptr)
      {
         nxlog_write(NXLOG_WARNING, _T("Internal error: cannot find interface object in Node::checkSubnetBinding()"));
         continue;   // Something goes really wrong
      }

      // Is cluster interconnect interface?
      bool isSync = (pCluster != nullptr) ? pCluster->isSyncAddr(addr) : false;

      shared_ptr<Subnet> pSubnet = FindSubnetForNode(m_zoneUIN, addr);
      if (pSubnet != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("Node::checkSubnetBinding(%s [%d]): found subnet %s [%d]"), m_name, m_id, pSubnet->getName(), pSubnet->getId());
         if (isSync)
         {
            pSubnet.reset();   // No further checks on this subnet
         }
         else
         {
            if (pSubnet->isSyntheticMask() && !iface->isSyntheticMask() && (addr.getMaskBits() > 0))
            {
               nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 4, _T("Setting correct netmask for subnet %s [%d] from node %s [%d]"),
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
               nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 4, _T("Restored link between subnet %s [%d] and node %s [%d]"),
                        pSubnet->getName(), pSubnet->getId(), m_name, m_id);
               pSubnet->addNode(self());
            }
         }
      }
      else if (!isSync)
      {
         DbgPrintf(6, _T("Missing subnet for address %s/%d on interface %s [%d]"),
            addr.toString(buffer), addr.getMaskBits(), iface->getName(), iface->getIfIndex());

         // Ignore mask 255.255.255.255 - some point-to-point interfaces can have such mask
         if (addr.getHostBits() > 0)
         {
            if (addr.getMaskBits() > 0)
            {
               pSubnet = createSubnet(addr, iface->isSyntheticMask());
            }
            else
            {
               DbgPrintf(6, _T("Zero subnet mask on interface %s [%d]"), iface->getName(), iface->getIfIndex());
               addr.setMaskBits((addr.getFamily() == AF_INET) ? ConfigReadInt(_T("Objects.Subnets.DefaultSubnetMaskIPv4"), 24) : ConfigReadInt(_T("Objects.Subnets.DefaultSubnetMaskIPv6"), 64));
               pSubnet = createSubnet(addr, true);
            }
         }
         else
         {
            DbgPrintf(6, _T("Subnet not required for address %s/%d on interface %s [%d]"),
               addr.toString(buffer), addr.getMaskBits(), iface->getName(), iface->getIfIndex());
         }
      }

      // Check if subnet mask is correct on interface
      if ((pSubnet != nullptr) && (pSubnet->getIpAddress().getMaskBits() != addr.getMaskBits()) && (addr.getHostBits() > 0))
      {
         shared_ptr<Interface> iface = findInterfaceByIP(addr);
         PostSystemEvent(EVENT_INCORRECT_NETMASK, m_id, "idsdd", iface->getId(),
                   iface->getIfIndex(), iface->getName(),
                   addr.getMaskBits(), pSubnet->getIpAddress().getMaskBits());
      }
   }

   // Some devices may report interface list, but without IP
   // To prevent such nodes from hanging at top of the tree, attempt
   // to find subnet node primary IP
   if ((getParentsCount(OBJECT_SUBNET) == 0) && m_ipAddress.isValidUnicast() && !(m_flags & NF_EXTERNAL_GATEWAY) && !addrList.hasAddress(m_ipAddress))
   {
      shared_ptr<Subnet> pSubnet = FindSubnetForNode(m_zoneUIN, m_ipAddress);
      if (pSubnet != nullptr)
      {
         // Check if node is linked to this subnet
         if (!pSubnet->isDirectChild(m_id))
         {
            DbgPrintf(4, _T("Restored link between subnet %s [%d] and node %s [%d]"),
                      pSubnet->getName(), pSubnet->getId(), m_name, m_id);
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
         Subnet *pSubnet = (Subnet *)getParentList().get(i);
         if (pSubnet->getIpAddress().contain(m_ipAddress) && !(m_flags & NF_EXTERNAL_GATEWAY))
            continue;   // primary IP is in given subnet

         int j;
         for(j = 0; j < addrList.size(); j++)
         {
            const InetAddress& addr = addrList.get(j);
            if (pSubnet->getIpAddress().contain(addr))
            {
               if ((pCluster != nullptr) && pCluster->isSyncAddr(addr))
               {
                  j = addrList.size(); // Cause to unbind from this subnet
               }
               break;
            }
         }
         if (j == addrList.size())
         {
            DbgPrintf(4, _T("Node::CheckSubnetBinding(): Subnet %s [%d] is incorrect for node %s [%d]"),
                      pSubnet->getName(), pSubnet->getId(), m_name, m_id);
            unlinkList.add(pSubnet);
         }
      }
   }
   unlockChildList();
   unlockParentList();

   // Unlink for incorrect subnet objects
   for(int n = 0; n < unlinkList.size(); n++)
   {
      NetObj *o = unlinkList.get(n);
      o->deleteChild(*this);
      deleteParent(*o);
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
      // Check names of existing interfaces
      for(int j = 0; j < pIfList->size(); j++)
      {
         InterfaceInfo *ifInfo = pIfList->get(j);

         readLockChildList();
         for(int i = 0; i < getChildList().size(); i++)
         {
            if (getChildList().get(i)->getObjectClass() == OBJECT_INTERFACE)
            {
               Interface *pInterface = (Interface *)getChildList().get(i);

               if (ifInfo->index == pInterface->getIfIndex())
               {
                  sendPollerMsg(_T("   Checking interface %d (%s)\r\n"), pInterface->getIfIndex(), pInterface->getName());
                  if (_tcscmp(ifInfo->name, pInterface->getName()))
                  {
                     pInterface->setName(ifInfo->name);
                     sendPollerMsg(POLLER_WARNING _T("   Name of interface %u changed to %s\r\n"), pInterface->getIfIndex(), ifInfo->name);
                  }
                  if (_tcscmp(ifInfo->description, pInterface->getDescription()))
                  {
                     pInterface->setDescription(ifInfo->description);
                     sendPollerMsg(POLLER_WARNING _T("   Description of interface %u changed to %s\r\n"), pInterface->getIfIndex(), ifInfo->description);
                  }
                  if (_tcscmp(ifInfo->alias, pInterface->getIfAlias()))
                  {
                     if (pInterface->getAlias().isEmpty() || !_tcscmp(pInterface->getIfAlias(), pInterface->getAlias()))
                     {
                        pInterface->setAlias(ifInfo->alias);
                        sendPollerMsg(POLLER_WARNING _T("   Alias of interface %u changed to %s\r\n"), pInterface->getIfIndex(), ifInfo->alias);
                     }
                     pInterface->setIfAlias(ifInfo->alias);
                     sendPollerMsg(POLLER_WARNING _T("   SNMP alias of interface %u changed to %s\r\n"), pInterface->getIfIndex(), ifInfo->alias);
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
bool Node::checkAgentPushRequestId(UINT64 requestId)
{
   lockProperties();
   bool valid = (requestId > m_lastAgentPushRequestId);
   if (valid)
      m_lastAgentPushRequestId = requestId;
   unlockProperties();
   return valid;
}

/**
 * Get node's physical components
 */
shared_ptr<ComponentTree> Node::getComponents()
{
   lockProperties();
   shared_ptr<ComponentTree> components = m_components;
   unlockProperties();
   return components;
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
bool Node::getLldpLocalPortInfo(uint32_t idType, BYTE *id, size_t idLen, LLDP_LOCAL_PORT_INFO *buffer)
{
   bool result = false;
   lockProperties();
   if (m_lldpLocalPortInfo != nullptr)
   {
      for(int i = 0; i < m_lldpLocalPortInfo->size(); i++)
      {
         LLDP_LOCAL_PORT_INFO *port = m_lldpLocalPortInfo->get(i);
         if ((idType == port->localIdSubtype) && (idLen == port->localIdLen) && !memcmp(id, port->localId, idLen))
         {
            memcpy(buffer, port, sizeof(LLDP_LOCAL_PORT_INFO));
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
void Node::showLLDPInfo(ServerConsole *console)
{
   TCHAR buffer[256];

   lockProperties();
   console->printf(_T("\x1b[1m*\x1b[0m Node LLDP ID: %s\n\n"), m_lldpNodeId);
   console->print(_T("\x1b[1m*\x1b[0m Local LLDP ports\n"));
   if (m_lldpLocalPortInfo != nullptr)
   {
      console->print(_T("   Port | ST | Len | Local ID                 | Description\n")
                     _T("   -----+----+-----+--------------------------+--------------------------------------\n"));
      for(int i = 0; i < m_lldpLocalPortInfo->size(); i++)
      {
         LLDP_LOCAL_PORT_INFO *port = m_lldpLocalPortInfo->get(i);
         console->printf(_T("   %4u | %2u | %3d | %-24s | %s\n"), port->portNumber,
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
void Node::writeWsListToMessage(NXCPMessage *msg)
{
   lockProperties();
   if (m_wirelessStations != nullptr)
   {
      msg->setField(VID_NUM_ELEMENTS, (UINT32)m_wirelessStations->size());
      UINT32 varId = VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < m_wirelessStations->size(); i++)
      {
         WirelessStationInfo *ws = m_wirelessStations->get(i);
         msg->setField(varId++, ws->macAddr, MAC_ADDR_LENGTH);
         msg->setField(varId++, ws->ipAddr);
         msg->setField(varId++, ws->ssid);
         msg->setField(varId++, (WORD)ws->vlan);
         msg->setField(varId++, ws->apObjectId);
         msg->setField(varId++, (UINT32)ws->rfIndex);
         msg->setField(varId++, ws->rfName);
         msg->setField(varId++, ws->nodeId);
         varId += 2;
      }
   }
   else
   {
      msg->setField(VID_NUM_ELEMENTS, (UINT32)0);
   }
   unlockProperties();
}

/**
 * Get wireless stations registered on this AP/controller.
 * Returned list must be destroyed by caller.
 */
ObjectArray<WirelessStationInfo> *Node::getWirelessStations() const
{
   ObjectArray<WirelessStationInfo> *ws = nullptr;

   lockProperties();
   if ((m_wirelessStations != nullptr) && (m_wirelessStations->size() > 0))
   {
      ws = new ObjectArray<WirelessStationInfo>(m_wirelessStations->size(), 16, Ownership::True);
      for(int i = 0; i < m_wirelessStations->size(); i++)
      {
         WirelessStationInfo *wsi = new WirelessStationInfo;
         memcpy(wsi, m_wirelessStations->get(i), sizeof(WirelessStationInfo));
         ws->add(wsi);
      }
   }
   unlockProperties();
   return ws;
}

/**
 * Get access point state via driver
 */
AccessPointState Node::getAccessPointState(AccessPoint *ap, SNMP_Transport *snmpTransport, const ObjectArray<RadioInterfaceInfo> *radioInterfaces)
{
   if (m_driver == nullptr)
      return AP_UNKNOWN;
   return m_driver->getAccessPointState(snmpTransport, this, m_driverData, ap->getIndex(), ap->getMacAddr(), ap->getIpAddress(), radioInterfaces);
}

/**
 * Synchronize data collection settings with agent
 */
void Node::syncDataCollectionWithAgent(AgentConnectionEx *conn)
{
   NXCPMessage msg(conn->getProtocolVersion());
   msg.setCode(CMD_DATA_COLLECTION_CONFIG);
   msg.setId(conn->generateRequestId());
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
            msg.setField(baseInfoFieldId++, (INT16)dco->getType());
            msg.setField(baseInfoFieldId++, (INT16)dco->getDataSource());
            msg.setField(baseInfoFieldId++, dco->getName());
            msg.setField(baseInfoFieldId++, (INT32)dco->getEffectivePollingInterval());
            msg.setFieldFromTime(baseInfoFieldId++, dco->getLastPollTime());
            baseInfoFieldId += 4;
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
      nxlog_debug_tag(DEBUG_TAG_DC_AGENT_CACHE, 4, _T("Node::syncDataCollectionWithAgent: node %s [%d] synchronized"), m_name, (int)m_id);
      m_state &= ~NSF_CACHE_MODE_NOT_SUPPORTED;
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_DC_AGENT_CACHE, 4, _T("Node::syncDataCollectionWithAgent: node %s [%d] not synchronized (%s)"), m_name, (int)m_id, AgentErrorCodeToText(rcc));
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
   NXCPMessage msg(conn->getProtocolVersion());
   msg.setCode(CMD_CLEAN_AGENT_DCI_CONF);
   msg.setId(conn->generateRequestId());
   NXCPMessage *response = conn->customRequest(&msg);
   if (response != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_DC_AGENT_CACHE, 4, _T("Node::clearDataCollectionConfigFromAgent: DCI configuration successfully removed from node %s [%d]"), m_name, (int)m_id);
      delete response;
   }
}

/**
 * Callback for async handling of data collection change notification
 */
void Node::onDataCollectionChangeAsyncCallback()
{
   if (InterlockedIncrement(&m_pendingDataConfigurationSync) == 1)
   {
      SleepAndCheckForShutdown(30);  // wait for possible subsequent update requests within 30 seconds
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
   else
   {
      // data collection configuration update already scheduled
      InterlockedDecrement(&m_pendingDataConfigurationSync);
      nxlog_debug_tag(DEBUG_TAG_DC_AGENT_CACHE, 7, _T("Node::onDataCollectionChangeAsyncCallback(%s [%u]): configuration upload already scheduled"), m_name, m_id);
   }
}

/**
 * Called when data collection configuration changed
 */
void Node::onDataCollectionChange()
{
   super::onDataCollectionChange();

   if (m_capabilities & NC_IS_NATIVE_AGENT)
   {
      nxlog_debug_tag(DEBUG_TAG_DC_AGENT_CACHE, 5, _T("Node::onDataCollectionChange(%s [%d]): executing data collection sync"), m_name, m_id);
      ThreadPoolExecute(g_mainThreadPool, self(), &Node::onDataCollectionChangeAsyncCallback);
   }

   uint32_t snmpProxyId = getEffectiveSnmpProxy(false);
   if (snmpProxyId != 0)
   {
      shared_ptr<Node> snmpProxy = static_pointer_cast<Node>(FindObjectById(snmpProxyId, OBJECT_NODE));
      if (snmpProxy != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_DC_AGENT_CACHE, 5, _T("Node::onDataCollectionChange(%s [%d]): executing data collection sync for SNMP proxy %s [%d]"),
               m_name, m_id, snmpProxy->getName(), snmpProxy->getId());
         ThreadPoolExecute(g_mainThreadPool, snmpProxy, &Node::onDataCollectionChangeAsyncCallback);
      }
   }

   snmpProxyId = getEffectiveSnmpProxy(true);
   if (snmpProxyId != 0)
   {
      shared_ptr<Node> snmpProxy = static_pointer_cast<Node>(FindObjectById(snmpProxyId, OBJECT_NODE));
      if (snmpProxy != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_DC_AGENT_CACHE, 5, _T("Node::onDataCollectionChange(%s [%d]): executing data collection sync for backup SNMP proxy %s [%d]"),
               m_name, m_id, snmpProxy->getName(), snmpProxy->getId());
         ThreadPoolExecute(g_mainThreadPool, snmpProxy, &Node::onDataCollectionChangeAsyncCallback);
      }
   }
}

/**
 * Force data collection configuration synchronization with agent
 */
void Node::forceSyncDataCollectionConfig()
{
   ThreadPoolExecute(g_mainThreadPool, self(), &Node::onDataCollectionChangeAsyncCallback);
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
      container->deleteChild(*this);
      deleteParent(*container);
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
         container->addChild(self());
         addParent(container);
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
               PostSystemEvent(EVENT_SNMP_TRAP_FLOOD_DETECTED, m_id, "ddd", newDiff, g_snmpTrapStormDurationThreshold, g_snmpTrapStormCountThreshold);

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
         PostSystemEvent(EVENT_SNMP_TRAP_FLOOD_ENDED, m_id, "DdD", newDiff, g_snmpTrapStormDurationThreshold, g_snmpTrapStormCountThreshold);
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

   uint32_t primarySnmpProxy = getEffectiveSnmpProxy(false);
   bool snmpProxy = (primarySnmpProxy == info->proxyId);
   bool backupSnmpProxy = (getEffectiveSnmpProxy(true) == info->proxyId);
   bool isTarget = false;

   readLockDciAccess();
   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      DCObject *dco = m_dcObjects.get(i);
      if (dco->getStatus() == ITEM_STATUS_DISABLED)
         continue;

      if ((((snmpProxy || backupSnmpProxy) && (dco->getDataSource() == DS_SNMP_AGENT) && (dco->getSourceNode() == 0)) ||
           ((dco->getDataSource() == DS_NATIVE_AGENT) && (dco->getSourceNode() == info->proxyId))) &&
          dco->hasValue() && (dco->getAgentCacheMode() == AGENT_CACHE_ON))
      {
         addProxyDataCollectionElement(info, dco, backupSnmpProxy && (dco->getDataSource() == DS_SNMP_AGENT) ? primarySnmpProxy : 0);
         if (dco->getDataSource() == DS_SNMP_AGENT)
            isTarget = true;
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
bool Node::checkProxyAndLink(NetworkMapObjectList *topology, uint32_t seedNode, uint32_t proxyId, uint32_t linkType, const TCHAR *linkName, bool checkAllProxies)
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
      ObjLink *link = topology->getLink(m_id, proxyId, linkType);
      if (link != nullptr)
         return true;

      topology->addObject(proxyId);
      topology->linkObjects(m_id, proxyId, linkType, linkName);
      proxy->buildInternalCommunicationTopologyInternal(topology, seedNode, !checkAllProxies, checkAllProxies);
   }
   return true;
}

/**
 * Build internal connection topology - internal function
 */
void Node::buildInternalCommunicationTopologyInternal(NetworkMapObjectList *topology, uint32_t seedNode, bool agentConnectionOnly, bool checkAllProxies)
{
   if (topology->getNumObjects() != 0 && seedNode == m_id)
      return;
   topology->addObject(m_id);

   if (IsZoningEnabled() && (m_zoneUIN != 0))
   {
      shared_ptr<Zone> zone = FindZoneByUIN(m_zoneUIN);
      if (zone != nullptr)
      {
         IntegerArray<uint32_t> proxies = zone->getAllProxyNodes();
         for(int i = 0; i < proxies.size(); i++)
         {
            checkProxyAndLink(topology, seedNode, proxies.get(i), LINK_TYPE_ZONE_PROXY, _T("Zone proxy"), checkAllProxies);
         }
      }
   }

   StringBuffer protocols;
	if (!m_tunnelId.equals(uuid::NULL_UUID))
	{
	   topology->addObject(FindLocalMgmtNode());
	   topology->linkObjects(m_id, FindLocalMgmtNode(), LINK_TYPE_AGENT_TUNNEL, _T("Agent tunnel"));
	}
	else if (!checkProxyAndLink(topology, seedNode, getEffectiveAgentProxy(), LINK_TYPE_AGENT_PROXY, _T("Agent proxy"), checkAllProxies))
	{
	   protocols.append(_T("Agent "));
	}

	// Only agent connection necessary for proxy nodes
	// Only if IP is set while using agent tunnel
	if (!agentConnectionOnly && _tcscmp(m_primaryHostName, _T("0.0.0.0")))
	{
      if (!checkProxyAndLink(topology, seedNode, getEffectiveIcmpProxy(), LINK_TYPE_ICMP_PROXY, _T("ICMP proxy"), checkAllProxies))
         protocols.append(_T("ICMP "));
      if (!checkProxyAndLink(topology, seedNode, getEffectiveSnmpProxy(), LINK_TYPE_SNMP_PROXY, _T("SNMP proxy"), checkAllProxies))
         protocols.append(_T("SNMP "));
      if (!checkProxyAndLink(topology, seedNode, getEffectiveSshProxy(), LINK_TYPE_SSH_PROXY, _T("SSH proxy"), checkAllProxies))
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
         topology->addObject(FindLocalMgmtNode());
         topology->linkObjects(m_id, FindLocalMgmtNode(), LINK_TYPE_NORMAL, (agentConnectionOnly ? _T("Direct") : (const TCHAR *)protocols));
      }
	}
}

/**
 * Build internal communication topology
 */
unique_ptr<NetworkMapObjectList> Node::buildInternalCommunicationTopology()
{
   auto topology = make_unique<NetworkMapObjectList>();
   topology->setAllowDuplicateLinks(true);
   buildInternalCommunicationTopologyInternal(topology.get(), m_id, false, false);
   return topology;
}

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
   json_object_set_new(root, "stateFlags", json_integer(m_state));
   json_object_set_new(root, "type", json_integer(m_type));
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
   json_object_set_new(root, "snmpObjectId", json_string_t(m_snmpObjectId));
   json_object_set_new(root, "sysDescription", json_string_t(m_sysDescription));
   json_object_set_new(root, "sysName", json_string_t(m_sysName));
   json_object_set_new(root, "sysLocation", json_string_t(m_sysLocation));
   json_object_set_new(root, "sysContact", json_string_t(m_sysContact));
   json_object_set_new(root, "lldpNodeId", json_string_t(m_lldpNodeId));
   json_object_set_new(root, "driverName", (m_driver != nullptr) ? json_string_t(m_driver->getName()) : json_null());
   json_object_set_new(root, "downSince", json_integer(m_downSince));
   json_object_set_new(root, "bootTime", json_integer(m_bootTime));
   json_object_set_new(root, "pollerNode", json_integer(m_pollerNode));
   json_object_set_new(root, "agentProxy", json_integer(m_agentProxy));
   json_object_set_new(root, "snmpProxy", json_integer(m_snmpProxy));
   json_object_set_new(root, "icmpProxy", json_integer(m_icmpProxy));
   json_object_set_new(root, "lastEvents", json_integer_array(m_lastEvents, MAX_LAST_EVENTS));
   json_object_set_new(root, "adoptedApCount", json_integer(m_adoptedApCount));
   json_object_set_new(root, "totalApCount", json_integer(m_totalApCount));
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
   unlockProperties();
   return root;
}

/**
 * ICMP poll target
 */
struct IcmpPollTarget
{
   TCHAR name[MAX_OBJECT_NAME];
   InetAddress address;

   IcmpPollTarget(const TCHAR *category, const TCHAR *_name, const InetAddress& _address)
   {
      if (category != nullptr)
      {
         _tcslcpy(name, category, MAX_OBJECT_NAME);
         _tcslcat(name, _T(":"), MAX_OBJECT_NAME);
         if (_name != nullptr)
            _tcslcat(name, _name, MAX_OBJECT_NAME);
         else
            _address.toString(&name[_tcslen(name)]);
      }
      else
      {
         if (_name != nullptr)
            _tcslcpy(name, _name, MAX_OBJECT_NAME);
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
      nxlog_debug_tag(DEBUG_TAG_ICMP_POLL, 7, _T("Node::icmpPoll(%s [%u]): proxy node found: %s"), m_name, m_id, proxyNode->getName());
      conn = proxyNode->createAgentConnection();
      if (conn == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_ICMP_POLL, 7, _T("Node::icmpPoll(%s [%u]): cannot connect to agent on proxy node"), m_name, m_id);
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
                  PostSystemEvent(EVENT_ICMP_UNREACHABLE, m_id, nullptr);
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
                  PostSystemEvent(EVENT_ICMP_OK, m_id, nullptr);
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
bool Node::getIcmpStatistics(const TCHAR *target, uint32_t *last, uint32_t *min, uint32_t *max, uint32_t *avg, uint32_t *loss) const
{
   lockProperties();
   IcmpStatCollector *collector = (m_icmpStatCollectors != nullptr) ? m_icmpStatCollectors->get(target) : nullptr;
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
   StringList *collectors = (m_icmpStatCollectors != nullptr) ? m_icmpStatCollectors->keys() : new StringList();
   unlockProperties();
   return collectors;
}

/**
 * Get ICMP poll statistic for given target and function
 */
DataCollectionError Node::getIcmpStatistic(const TCHAR *param, IcmpStatFunction function, TCHAR *value) const
{
   TCHAR key[MAX_OBJECT_NAME + 2];
   if (param != nullptr)
   {
      TCHAR target[MAX_OBJECT_NAME];
      if (!AgentGetParameterArg(param, 1, target, MAX_OBJECT_NAME))
         return DataCollectionError::DCE_NOT_SUPPORTED;

      if ((_tcslen(target) >= 2) && (target[1] == ':'))
      {
         _tcslcpy(key, target, MAX_OBJECT_NAME + 2);
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
            _tcslcpy(&key[2], object->getName(), MAX_OBJECT_NAME);
         }
      }
   }
   else
   {
      _tcscpy(key, _T("PRI"));
   }

   lockProperties();
   DataCollectionError rc;
   IcmpStatCollector *collector = (m_icmpStatCollectors != nullptr) ? m_icmpStatCollectors->get(key) : nullptr;
   if (collector != nullptr)
   {
      switch(function)
      {
         case IcmpStatFunction::AVERAGE:
            ret_uint(value, collector->average());
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
            cluster->addChild(self());
            addParent(cluster->self());
            PostSystemEvent(EVENT_CLUSTER_AUTOADD, g_dwMgmtNode, "isis", m_id, m_name, cluster->getId(), cluster->getName());
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
            cluster->deleteChild(*this);
            deleteParent(*cluster);
            PostSystemEvent(EVENT_CLUSTER_AUTOREMOVE, g_dwMgmtNode, "isis", m_id, m_name, cluster->getId(), cluster->getName());
            cluster->calculateCompoundStatus();
         }
      }
      delete cachedFilterVM;
   }
}
