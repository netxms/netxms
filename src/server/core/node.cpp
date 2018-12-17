/*
** NetXMS - Network Management System
** Copyright (C) 2003-2018 Raden Solutions
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

#define DEBUG_TAG_CONF_POLL   _T("poll.conf")
#define DEBUG_TAG_AGENT       _T("node.agent")
#define DEBUG_TAG_STATUS_POLL _T("poll.status")

/**
 * Externals
 */
extern VolatileCounter64 g_syslogMessagesReceived;
extern VolatileCounter64 g_snmpTrapsReceived;

/**
 * Node class default constructor
 */
Node::Node() : super()
{
   m_primaryName[0] = 0;
   m_status = STATUS_UNKNOWN;
   m_type = NODE_TYPE_UNKNOWN;
   m_subType[0] = 0;
   m_hypervisorType[0] = 0;
   m_hypervisorInfo = NULL;
   m_capabilities = 0;
   m_zoneUIN = 0;
   m_agentPort = AGENT_LISTEN_PORT;
   m_agentAuthMethod = AUTH_NONE;
   m_agentCacheMode = AGENT_CACHE_DEFAULT;
   m_szSharedSecret[0] = 0;
   m_iStatusPollType = POLL_ICMP_PING;
   m_snmpVersion = SNMP_VERSION_2C;
   m_snmpPort = SNMP_DEFAULT_PORT;
   m_snmpSecurity = new SNMP_SecurityContext("public");
   m_snmpObjectId[0] = 0;
   m_lastDiscoveryPoll = 0;
   m_lastStatusPoll = 0;
   m_lastConfigurationPoll = 0;
   m_lastInstancePoll = 0;
   m_lastTopologyPoll = 0;
   m_lastRTUpdate = 0;
   m_downSince = 0;
   m_bootTime = 0;
   m_agentUpTime = 0;
   m_hAgentAccessMutex = MutexCreate();
   m_hSmclpAccessMutex = MutexCreate();
   m_mutexRTAccess = MutexCreate();
   m_mutexTopoAccess = MutexCreate();
   m_agentConnection = NULL;
   m_proxyConnections = new ProxyAgentConnection[MAX_PROXY_TYPE];
   m_smclpConnection = NULL;
   m_lastAgentTrapId = 0;
   m_lastSNMPTrapId = 0;
   m_lastSyslogMessageId = 0;
   m_lastAgentPushRequestId = 0;
   m_agentCertSubject = NULL;
   m_agentVersion[0] = 0;
   m_platformName[0] = 0;
   m_sysDescription = NULL;
   m_sysName = NULL;
   m_sysContact = NULL;
   m_sysLocation = NULL;
   m_lldpNodeId = NULL;
   m_lldpLocalPortInfo = NULL;
   m_agentParameters = NULL;
   m_agentTables = NULL;
   m_driverParameters = NULL;
   m_pollerNode = 0;
   m_agentProxy = 0;
   m_snmpProxy = 0;
   m_icmpProxy = 0;
   memset(m_lastEvents, 0, sizeof(QWORD) * MAX_LAST_EVENTS);
   m_routingLoopEvents = new ObjectArray<RoutingLoopEvent>(0, 16, true);
   m_pRoutingTable = NULL;
   m_arpCache = NULL;
   m_failTimeSNMP = 0;
   m_failTimeAgent = 0;
   m_lastAgentCommTime = 0;
   m_lastAgentConnectAttempt = 0;
   m_linkLayerNeighbors = NULL;
   m_vrrpInfo = NULL;
   m_topology = NULL;
   m_topologyRebuildTimestamp = 0;
   m_pendingState = -1;
   m_pollCountAgent = 0;
   m_pollCountSNMP = 0;
   m_pollCountAllDown = 0;
   m_requiredPollCount = 0; // Use system default
   m_nUseIfXTable = IFXTABLE_DEFAULT;  // Use system default
   m_jobQueue = new ServerJobQueue();
   m_fdb = NULL;
   m_vlans = NULL;
   m_wirelessStations = NULL;
   m_adoptedApCount = 0;
   m_totalApCount = 0;
   m_driver = NULL;
   m_driverData = NULL;
   m_components = NULL;
   m_softwarePackages = NULL;
   m_hardwareComponents = NULL;
   m_winPerfObjects = NULL;
   memset(m_baseBridgeAddress, 0, MAC_ADDR_LENGTH);
   m_fileUpdateConn = NULL;
   m_rackId = 0;
   m_rackPosition = 0;
   m_rackHeight = 1;
   m_chassisId = 0;
   m_syslogMessageCount = 0;
   m_snmpTrapCount = 0;
   m_sshLogin[0] = 0;
   m_sshPassword[0] = 0;
   m_sshProxy = 0;
   m_portNumberingScheme = NDD_PN_UNKNOWN;
   m_portRowCount = 0;
   m_agentCompressionMode = NODE_AGENT_COMPRESSION_DEFAULT;
   m_rackOrientation = FILL;
}

/**
 * Create new node from new node data
 */
Node::Node(const NewNodeData *newNodeData, UINT32 flags)  : super()
{
   m_runtimeFlags |= DCDF_CONFIGURATION_POLL_PENDING;
   newNodeData->ipAddr.toString(m_primaryName);
   m_status = STATUS_UNKNOWN;
   m_type = NODE_TYPE_UNKNOWN;
   m_subType[0] = 0;
   m_hypervisorType[0] = 0;
   m_hypervisorInfo = NULL;
   m_ipAddress = newNodeData->ipAddr;
   m_capabilities = 0;
   m_flags = flags;
   m_zoneUIN = newNodeData->zoneUIN;
   m_agentPort = newNodeData->agentPort;
   m_agentAuthMethod = AUTH_NONE;
   m_agentCacheMode = AGENT_CACHE_DEFAULT;
   m_szSharedSecret[0] = 0;
   m_iStatusPollType = POLL_ICMP_PING;
   m_snmpVersion = SNMP_VERSION_2C;
   m_snmpPort = newNodeData->snmpPort;
   if (newNodeData->snmpSecurity != NULL)
      m_snmpSecurity = new SNMP_SecurityContext(newNodeData->snmpSecurity);
   else
      m_snmpSecurity = new SNMP_SecurityContext("public");
   if (newNodeData->name[0] != 0)
      _tcslcpy(m_name, newNodeData->name, MAX_OBJECT_NAME);
   else
      newNodeData->ipAddr.toString(m_name);    // Make default name from IP address
   m_snmpObjectId[0] = 0;
   m_lastDiscoveryPoll = 0;
   m_lastStatusPoll = 0;
   m_lastConfigurationPoll = 0;
   m_lastInstancePoll = 0;
   m_lastTopologyPoll = 0;
   m_lastRTUpdate = 0;
   m_downSince = 0;
   m_bootTime = 0;
   m_agentUpTime = 0;
   m_hAgentAccessMutex = MutexCreate();
   m_hSmclpAccessMutex = MutexCreate();
   m_mutexRTAccess = MutexCreate();
   m_mutexTopoAccess = MutexCreate();
   m_agentConnection = NULL;
   m_proxyConnections = new ProxyAgentConnection[MAX_PROXY_TYPE];
   m_smclpConnection = NULL;
   m_lastAgentTrapId = 0;
   m_lastSNMPTrapId = 0;
   m_lastSyslogMessageId = 0;
   m_lastAgentPushRequestId = 0;
   m_agentCertSubject = NULL;
   m_agentVersion[0] = 0;
   m_platformName[0] = 0;
   m_sysDescription = NULL;
   m_sysName = NULL;
   m_sysContact = NULL;
   m_sysLocation = NULL;
   m_lldpNodeId = NULL;
   m_lldpLocalPortInfo = NULL;
   m_agentParameters = NULL;
   m_agentTables = NULL;
   m_driverParameters = NULL;
   m_pollerNode = 0;
   m_agentProxy = newNodeData->agentProxyId;
   m_snmpProxy = newNodeData->snmpProxyId;
   m_icmpProxy = newNodeData->icmpProxyId;
   memset(m_lastEvents, 0, sizeof(QWORD) * MAX_LAST_EVENTS);
   m_routingLoopEvents = new ObjectArray<RoutingLoopEvent>(0, 16, true);
   m_isHidden = true;
   m_pRoutingTable = NULL;
   m_arpCache = NULL;
   m_failTimeSNMP = 0;
   m_failTimeAgent = 0;
   m_lastAgentCommTime = 0;
   m_lastAgentConnectAttempt = 0;
   m_linkLayerNeighbors = NULL;
   m_vrrpInfo = NULL;
   m_topology = NULL;
   m_topologyRebuildTimestamp = 0;
   m_pendingState = -1;
   m_pollCountAgent = 0;
   m_pollCountSNMP = 0;
   m_pollCountAllDown = 0;
   m_requiredPollCount = 0; // Use system default
   m_nUseIfXTable = IFXTABLE_DEFAULT;  // Use system default
   m_jobQueue = new ServerJobQueue();
   m_fdb = NULL;
   m_vlans = NULL;
   m_wirelessStations = NULL;
   m_adoptedApCount = 0;
   m_totalApCount = 0;
   m_driver = NULL;
   m_driverData = NULL;
   m_components = NULL;
   m_softwarePackages = NULL;
   m_hardwareComponents = NULL;
   m_winPerfObjects = NULL;
   memset(m_baseBridgeAddress, 0, MAC_ADDR_LENGTH);
   m_fileUpdateConn = NULL;
   m_rackId = 0;
   m_rackPosition = 0;
   m_rackHeight = 1;
   m_chassisId = 0;
   m_syslogMessageCount = 0;
   m_snmpTrapCount = 0;
   _tcslcpy(m_sshLogin, newNodeData->sshLogin, MAX_SSH_LOGIN_LEN);
   _tcslcpy(m_sshPassword, newNodeData->sshPassword, MAX_SSH_PASSWORD_LEN);
   m_sshProxy = newNodeData->sshProxyId;
   m_portNumberingScheme = NDD_PN_UNKNOWN;
   m_portRowCount = 0;
   m_agentCompressionMode = NODE_AGENT_COMPRESSION_DEFAULT;
   m_rackOrientation = FILL;
   m_agentId = newNodeData->agentId;
}

/**
 * Node destructor
 */
Node::~Node()
{
   delete m_driverData;
   MutexDestroy(m_hAgentAccessMutex);
   MutexDestroy(m_hSmclpAccessMutex);
   MutexDestroy(m_mutexRTAccess);
   MutexDestroy(m_mutexTopoAccess);
   if (m_agentConnection != NULL)
      m_agentConnection->decRefCount();
   for(int i = 0; i < MAX_PROXY_TYPE; i++)
      if(m_proxyConnections[i].get() != NULL)
         m_proxyConnections[i].get()->decRefCount();
   delete[] m_proxyConnections;
   delete m_smclpConnection;
   delete m_agentParameters;
   delete m_agentTables;
   delete m_driverParameters;
   free(m_sysDescription);
   DestroyRoutingTable(m_pRoutingTable);
   if (m_arpCache != NULL)
      m_arpCache->decRefCount();
   if (m_linkLayerNeighbors != NULL)
      m_linkLayerNeighbors->decRefCount();
   delete m_vrrpInfo;
   delete m_topology;
   delete m_jobQueue;
   delete m_snmpSecurity;
   if (m_fdb != NULL)
      m_fdb->decRefCount();
   if (m_vlans != NULL)
      m_vlans->decRefCount();
   delete m_wirelessStations;
   if (m_components != NULL)
      m_components->decRefCount();
   delete m_lldpLocalPortInfo;
   delete m_softwarePackages;
   delete m_hardwareComponents;
   delete m_winPerfObjects;
   free(m_sysName);
   free(m_sysContact);
   free(m_sysLocation);
   delete m_routingLoopEvents;
   free(m_agentCertSubject);
   free(m_hypervisorInfo);
}

/**
 * Create object from database data
 */
bool Node::loadFromDatabase(DB_HANDLE hdb, UINT32 dwId)
{
   int i, iNumRows;
   UINT32 dwSubnetId;
   NetObj *pObject;
   bool bResult = false;

   m_id = dwId;

   if (!loadCommonProperties(hdb))
   {
      DbgPrintf(2, _T("Cannot load common properties for node object %d"), dwId);
      return false;
   }

   DB_STATEMENT hStmt = DBPrepare(hdb,
      _T("SELECT primary_name,primary_ip,")
      _T("snmp_version,auth_method,secret,")
      _T("agent_port,status_poll_type,snmp_oid,agent_version,")
      _T("platform_name,poller_node_id,zone_guid,")
      _T("proxy_node,snmp_proxy,required_polls,uname,")
      _T("use_ifxtable,snmp_port,community,usm_auth_password,")
      _T("usm_priv_password,usm_methods,snmp_sys_name,bridge_base_addr,")
      _T("down_since,boot_time,driver_name,icmp_proxy,")
      _T("agent_cache_mode,snmp_sys_contact,snmp_sys_location,")
      _T("rack_id,rack_image_front,rack_position,rack_height,")
      _T("last_agent_comm_time,syslog_msg_count,snmp_trap_count,")
      _T("node_type,node_subtype,ssh_login,ssh_password,ssh_proxy,")
      _T("port_rows,port_numbering_scheme,agent_comp_mode,")
      _T("tunnel_id,lldp_id,capabilities,fail_time_snmp,fail_time_agent,")
      _T("rack_orientation,rack_image_rear,agent_id,agent_cert_subject,")
      _T("hypervisor_type,hypervisor_info")
      _T(" FROM nodes WHERE id=?"));
   if (hStmt == NULL)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, dwId);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult == NULL)
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

   DBGetField(hResult, 0, 0, m_primaryName, MAX_DNS_NAME);
   m_ipAddress = DBGetFieldInetAddr(hResult, 0, 1);
   m_snmpVersion = DBGetFieldLong(hResult, 0, 2);
   m_agentAuthMethod = (WORD)DBGetFieldLong(hResult, 0, 3);
   DBGetField(hResult, 0, 4, m_szSharedSecret, MAX_SECRET_LENGTH);
   m_agentPort = (WORD)DBGetFieldLong(hResult, 0, 5);
   m_iStatusPollType = DBGetFieldLong(hResult, 0, 6);
   DBGetField(hResult, 0, 7, m_snmpObjectId, MAX_OID_LEN * 4);
   DBGetField(hResult, 0, 8, m_agentVersion, MAX_AGENT_VERSION_LEN);
   DBGetField(hResult, 0, 9, m_platformName, MAX_PLATFORM_NAME_LEN);
   m_pollerNode = DBGetFieldULong(hResult, 0, 10);
   m_zoneUIN = DBGetFieldULong(hResult, 0, 11);
   m_agentProxy = DBGetFieldULong(hResult, 0, 12);
   m_snmpProxy = DBGetFieldULong(hResult, 0, 13);
   m_requiredPollCount = DBGetFieldLong(hResult, 0, 14);
   m_sysDescription = DBGetField(hResult, 0, 15, NULL, 0);
   m_nUseIfXTable = (BYTE)DBGetFieldLong(hResult, 0, 16);
   m_snmpPort = (WORD)DBGetFieldLong(hResult, 0, 17);

   // SNMP authentication parameters
   char snmpAuthObject[256], snmpAuthPassword[256], snmpPrivPassword[256];
   DBGetFieldA(hResult, 0, 18, snmpAuthObject, 256);
   DBGetFieldA(hResult, 0, 19, snmpAuthPassword, 256);
   DBGetFieldA(hResult, 0, 20, snmpPrivPassword, 256);
   int snmpMethods = DBGetFieldLong(hResult, 0, 21);
   delete m_snmpSecurity;
   if (m_snmpVersion == SNMP_VERSION_3)
   {
      m_snmpSecurity = new SNMP_SecurityContext(snmpAuthObject, snmpAuthPassword, snmpPrivPassword, snmpMethods & 0xFF, snmpMethods >> 8);
   }
   else
   {
      // This will create security context with V2C security model
      // USM fields will be loaded but keys will not be calculated
      m_snmpSecurity = new SNMP_SecurityContext(snmpAuthObject);
      m_snmpSecurity->setAuthMethod(snmpMethods & 0xFF);
      m_snmpSecurity->setAuthPassword(snmpAuthPassword);
      m_snmpSecurity->setPrivMethod(snmpMethods >> 8);
      m_snmpSecurity->setPrivPassword(snmpPrivPassword);
   }

   m_sysName = DBGetField(hResult, 0, 22, NULL, 0);

   TCHAR baseAddr[16];
   TCHAR *value = DBGetField(hResult, 0, 23, baseAddr, 16);
   if (value != NULL)
      StrToBin(value, m_baseBridgeAddress, MAC_ADDR_LENGTH);

   m_downSince = DBGetFieldLong(hResult, 0, 24);
   m_bootTime = DBGetFieldLong(hResult, 0, 25);

   // Setup driver
   TCHAR driverName[34];
   DBGetField(hResult, 0, 26, driverName, 34);
   StrStrip(driverName);
   if (driverName[0] != 0)
      m_driver = FindDriverByName(driverName);

   m_icmpProxy = DBGetFieldULong(hResult, 0, 27);
   m_agentCacheMode = (INT16)DBGetFieldLong(hResult, 0, 28);
   if ((m_agentCacheMode != AGENT_CACHE_ON) && (m_agentCacheMode != AGENT_CACHE_OFF))
      m_agentCacheMode = AGENT_CACHE_DEFAULT;

   m_sysContact = DBGetField(hResult, 0, 29, NULL, 0);
   m_sysLocation = DBGetField(hResult, 0, 30, NULL, 0);

   m_rackId = DBGetFieldULong(hResult, 0, 31);
   m_rackImageFront = DBGetFieldGUID(hResult, 0, 32);
   m_rackPosition = (INT16)DBGetFieldLong(hResult, 0, 33);
   m_rackHeight = (INT16)DBGetFieldLong(hResult, 0, 34);
   m_lastAgentCommTime = DBGetFieldLong(hResult, 0, 35);
   m_syslogMessageCount = DBGetFieldInt64(hResult, 0, 36);
   m_snmpTrapCount = DBGetFieldInt64(hResult, 0, 37);
   m_type = (NodeType)DBGetFieldLong(hResult, 0, 38);
   DBGetField(hResult, 0, 39, m_subType, MAX_NODE_SUBTYPE_LENGTH);
   DBGetField(hResult, 0, 40, m_sshLogin, MAX_SSH_LOGIN_LEN);
   DBGetField(hResult, 0, 41, m_sshPassword, MAX_SSH_PASSWORD_LEN);
   m_sshProxy = DBGetFieldULong(hResult, 0, 42);
   m_portRowCount = DBGetFieldULong(hResult, 0, 43);
   m_portNumberingScheme = DBGetFieldULong(hResult, 0, 44);
   m_agentCompressionMode = (INT16)DBGetFieldLong(hResult, 0, 45);
   m_tunnelId = DBGetFieldGUID(hResult, 0, 46);
   m_lldpNodeId = DBGetField(hResult, 0, 47, NULL, 0);
   if ((m_lldpNodeId != NULL) && (*m_lldpNodeId == 0))
      MemFreeAndNull(m_lldpNodeId);
   m_capabilities = DBGetFieldULong(hResult, 0, 48);
   m_failTimeSNMP = DBGetFieldLong(hResult, 0, 49);
   m_failTimeAgent = DBGetFieldLong(hResult, 0, 50);
   m_rackOrientation = static_cast<RackOrientation>(DBGetFieldLong(hResult, 0, 51));
   m_rackImageRear = DBGetFieldGUID(hResult, 0, 52);
   m_agentId = DBGetFieldGUID(hResult, 0, 53);
   m_agentCertSubject = DBGetField(hResult, 0, 54, NULL, 0);
   if ((m_agentCertSubject != NULL) && (m_agentCertSubject[0] == 0))
      MemFreeAndNull(m_agentCertSubject);
   DBGetField(hResult, 0, 55, m_hypervisorType, MAX_HYPERVISOR_TYPE_LENGTH);
   m_hypervisorInfo = DBGetField(hResult, 0, 56, NULL, 0);

   DBFreeResult(hResult);
   DBFreeStatement(hStmt);

   if (m_isDeleted)
   {
      return true;
   }

   // Link node to subnets
   hStmt = DBPrepare(hdb, _T("SELECT subnet_id FROM nsmap WHERE node_id=?"));
   if (hStmt == NULL)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   hResult = DBSelectPrepared(hStmt);
   if (hResult == NULL)
   {
      DBFreeStatement(hStmt);
      return false;     // Query failed
   }

   iNumRows = DBGetNumRows(hResult);
   for(i = 0; i < iNumRows; i++)
   {
      dwSubnetId = DBGetFieldULong(hResult, i, 0);
      pObject = FindObjectById(dwSubnetId);
      if (pObject == NULL)
      {
         nxlog_write(MSG_INVALID_SUBNET_ID, EVENTLOG_ERROR_TYPE, "dd", dwId, dwSubnetId);
         break;
      }
      else if (pObject->getObjectClass() != OBJECT_SUBNET)
      {
         nxlog_write(MSG_SUBNET_NOT_SUBNET, EVENTLOG_ERROR_TYPE, "dd", dwId, dwSubnetId);
         break;
      }
      else
      {
         pObject->addChild(this);
         addParent(pObject);
      }
   }

   DBFreeResult(hResult);
   DBFreeStatement(hStmt);

   loadItemsFromDB(hdb);
   loadACLFromDB(hdb);

   // Walk through all items in the node and load appropriate thresholds
   bResult = true;
   for(i = 0; i < m_dcObjects->size(); i++)
   {
      if (!m_dcObjects->get(i)->loadThresholdsFromDB(hdb))
      {
         DbgPrintf(3, _T("Cannot load thresholds for DCI %d of node %d (%s)"),
                   m_dcObjects->get(i)->getId(), dwId, m_name);
         bResult = false;
      }
   }

   updatePhysicalContainerBinding(OBJECT_RACK, m_rackId);
   updatePhysicalContainerBinding(OBJECT_CHASSIS, m_chassisId);

   if (bResult)
   {
      // Load software packages
      hStmt = DBPrepare(hdb, _T("SELECT name,version,vendor,date,url,description FROM software_inventory WHERE node_id=?"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         hResult = DBSelectPrepared(hStmt);
         if (hResult != NULL)
         {
            int nRows = DBGetNumRows(hResult);
            m_softwarePackages = new ObjectArray<SoftwarePackage>(nRows, 16, true);

            for(int i = 0; i < nRows; i++)
               m_softwarePackages->add(new SoftwarePackage(hResult, i));

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
         DbgPrintf(3, _T("Cannot load software packages of node %d (%s)"), m_id, m_name);
   }

   if (bResult)
   {
      // Load hardware components
      hStmt = DBPrepare(hdb, _T("SELECT component_type,component_index,vendor,model,capacity,serial FROM hardware_inventory WHERE node_id=?"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         hResult = DBSelectPrepared(hStmt);
         if (hResult != NULL)
         {
            int nRows = DBGetNumRows(hResult);
            m_hardwareComponents = new ObjectArray<HardwareComponent>(nRows, 16, true);

            for(int i = 0; i < nRows; i++)
               m_hardwareComponents->add(new HardwareComponent(hResult, i));

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
         DbgPrintf(3, _T("Cannot load hardware information of node %d (%s)"), m_id, m_name);
   }

   return bResult;
}

/**
 * Save object to database
 */
bool Node::saveToDatabase(DB_HANDLE hdb)
{
   // Lock object's access
   lockProperties();

   bool success = saveCommonProperties(hdb);

   if (success && (m_modified & MODIFY_NODE_PROPERTIES))
   {
      static const TCHAR *columns[] = {
         _T("primary_ip"), _T("primary_name"), _T("snmp_port"), _T("capabilities"), _T("snmp_version"), _T("community"),
         _T("status_poll_type"), _T("agent_port"), _T("auth_method"), _T("secret"), _T("snmp_oid"), _T("uname"),
         _T("agent_version"), _T("platform_name"), _T("poller_node_id"), _T("zone_guid"), _T("proxy_node"), _T("snmp_proxy"),
         _T("icmp_proxy"), _T("required_polls"), _T("use_ifxtable"), _T("usm_auth_password"), _T("usm_priv_password"),
         _T("usm_methods"), _T("snmp_sys_name"), _T("bridge_base_addr"), _T("down_since"), _T("driver_name"),
         _T("rack_image_front"), _T("rack_position"), _T("rack_height"), _T("rack_id"), _T("boot_time"), _T("agent_cache_mode"),
         _T("snmp_sys_contact"), _T("snmp_sys_location"), _T("last_agent_comm_time"), _T("syslog_msg_count"),
         _T("snmp_trap_count"), _T("node_type"), _T("node_subtype"), _T("ssh_login"), _T("ssh_password"), _T("ssh_proxy"),
         _T("chassis_id"), _T("port_rows"), _T("port_numbering_scheme"), _T("agent_comp_mode"), _T("tunnel_id"), _T("lldp_id"),
         _T("fail_time_snmp"), _T("fail_time_agent"), _T("rack_orientation"), _T("rack_image_rear"), _T("agent_id"),
         _T("agent_cert_subject"), _T("hypervisor_type"), _T("hypervisor_info"),
         NULL
      };

      DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("nodes"), _T("id"), m_id, columns);
      if (hStmt != NULL)
      {
         int snmpMethods = m_snmpSecurity->getAuthMethod() | (m_snmpSecurity->getPrivMethod() << 8);
         TCHAR ipAddr[64], baseAddress[16], cacheMode[16], compressionMode[16];

         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_ipAddress.toString(ipAddr), DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_primaryName, DB_BIND_STATIC);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (LONG)m_snmpPort);
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_capabilities);
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (LONG)m_snmpVersion);
#ifdef UNICODE
         DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, WideStringFromMBString(m_snmpSecurity->getCommunity()), DB_BIND_DYNAMIC);
#else
         DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, m_snmpSecurity->getCommunity(), DB_BIND_STATIC);
#endif
         DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, (LONG)m_iStatusPollType);
         DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, (LONG)m_agentPort);
         DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, (LONG)m_agentAuthMethod);
         DBBind(hStmt, 10, DB_SQLTYPE_VARCHAR, m_szSharedSecret, DB_BIND_STATIC);
         DBBind(hStmt, 11, DB_SQLTYPE_VARCHAR, m_snmpObjectId, DB_BIND_STATIC);
         DBBind(hStmt, 12, DB_SQLTYPE_VARCHAR, m_sysDescription, DB_BIND_STATIC);
         DBBind(hStmt, 13, DB_SQLTYPE_VARCHAR, m_agentVersion, DB_BIND_STATIC);
         DBBind(hStmt, 14, DB_SQLTYPE_VARCHAR, m_platformName, DB_BIND_STATIC);
         DBBind(hStmt, 15, DB_SQLTYPE_INTEGER, m_pollerNode);
         DBBind(hStmt, 16, DB_SQLTYPE_INTEGER, m_zoneUIN);
         DBBind(hStmt, 17, DB_SQLTYPE_INTEGER, m_agentProxy);
         DBBind(hStmt, 18, DB_SQLTYPE_INTEGER, m_snmpProxy);
         DBBind(hStmt, 19, DB_SQLTYPE_INTEGER, m_icmpProxy);
         DBBind(hStmt, 20, DB_SQLTYPE_INTEGER, (LONG)m_requiredPollCount);
         DBBind(hStmt, 21, DB_SQLTYPE_INTEGER, (LONG)m_nUseIfXTable);
#ifdef UNICODE
         DBBind(hStmt, 22, DB_SQLTYPE_VARCHAR, WideStringFromMBString(m_snmpSecurity->getAuthPassword()), DB_BIND_DYNAMIC);
         DBBind(hStmt, 23, DB_SQLTYPE_VARCHAR, WideStringFromMBString(m_snmpSecurity->getPrivPassword()), DB_BIND_DYNAMIC);
#else
         DBBind(hStmt, 22, DB_SQLTYPE_VARCHAR, m_snmpSecurity->getAuthPassword(), DB_BIND_STATIC);
         DBBind(hStmt, 23, DB_SQLTYPE_VARCHAR, m_snmpSecurity->getPrivPassword(), DB_BIND_STATIC);
#endif
         DBBind(hStmt, 24, DB_SQLTYPE_INTEGER, (LONG)snmpMethods);
         DBBind(hStmt, 25, DB_SQLTYPE_VARCHAR, m_sysName, DB_BIND_STATIC);
         DBBind(hStmt, 26, DB_SQLTYPE_VARCHAR, BinToStr(m_baseBridgeAddress, MAC_ADDR_LENGTH, baseAddress), DB_BIND_STATIC);
         DBBind(hStmt, 27, DB_SQLTYPE_INTEGER, (LONG)m_downSince);
         DBBind(hStmt, 28, DB_SQLTYPE_VARCHAR, (m_driver != NULL) ? m_driver->getName() : _T(""), DB_BIND_STATIC);
         DBBind(hStmt, 29, DB_SQLTYPE_VARCHAR, m_rackImageFront);   // rack image front
         DBBind(hStmt, 30, DB_SQLTYPE_INTEGER, m_rackPosition); // rack position
         DBBind(hStmt, 31, DB_SQLTYPE_INTEGER, m_rackHeight);   // device height in rack units
         DBBind(hStmt, 32, DB_SQLTYPE_INTEGER, m_rackId);   // rack ID
         DBBind(hStmt, 33, DB_SQLTYPE_INTEGER, (LONG)m_bootTime);
         DBBind(hStmt, 34, DB_SQLTYPE_VARCHAR, _itot(m_agentCacheMode, cacheMode, 10), DB_BIND_STATIC, 1);
         DBBind(hStmt, 35, DB_SQLTYPE_VARCHAR, m_sysContact, DB_BIND_STATIC);
         DBBind(hStmt, 36, DB_SQLTYPE_VARCHAR, m_sysLocation, DB_BIND_STATIC);
         DBBind(hStmt, 37, DB_SQLTYPE_INTEGER, (LONG)m_lastAgentCommTime);
         DBBind(hStmt, 38, DB_SQLTYPE_BIGINT, m_syslogMessageCount);
         DBBind(hStmt, 39, DB_SQLTYPE_BIGINT, m_snmpTrapCount);
         DBBind(hStmt, 40, DB_SQLTYPE_INTEGER, (INT32)m_type);
         DBBind(hStmt, 41, DB_SQLTYPE_VARCHAR, m_subType, DB_BIND_STATIC);
         DBBind(hStmt, 42, DB_SQLTYPE_VARCHAR, m_sshLogin, DB_BIND_STATIC);
         DBBind(hStmt, 43, DB_SQLTYPE_VARCHAR, m_sshPassword, DB_BIND_STATIC);
         DBBind(hStmt, 44, DB_SQLTYPE_INTEGER, m_sshProxy);
         DBBind(hStmt, 45, DB_SQLTYPE_INTEGER, m_chassisId);
         DBBind(hStmt, 46, DB_SQLTYPE_INTEGER, m_portRowCount);
         DBBind(hStmt, 47, DB_SQLTYPE_INTEGER, m_portNumberingScheme);
         DBBind(hStmt, 48, DB_SQLTYPE_VARCHAR, _itot(m_agentCompressionMode, compressionMode, 10), DB_BIND_STATIC, 1);
         DBBind(hStmt, 49, DB_SQLTYPE_VARCHAR, m_tunnelId);
         DBBind(hStmt, 50, DB_SQLTYPE_VARCHAR, m_lldpNodeId, DB_BIND_STATIC);
         DBBind(hStmt, 51, DB_SQLTYPE_INTEGER, (LONG)m_failTimeSNMP);
         DBBind(hStmt, 52, DB_SQLTYPE_INTEGER, (LONG)m_failTimeAgent);
         DBBind(hStmt, 53, DB_SQLTYPE_INTEGER, m_rackOrientation);
         DBBind(hStmt, 54, DB_SQLTYPE_VARCHAR, m_rackImageRear);
         DBBind(hStmt, 55, DB_SQLTYPE_VARCHAR, m_agentId);
         DBBind(hStmt, 56, DB_SQLTYPE_VARCHAR, m_agentCertSubject, DB_BIND_STATIC);
         DBBind(hStmt, 57, DB_SQLTYPE_VARCHAR, m_hypervisorType, DB_BIND_STATIC);
         DBBind(hStmt, 58, DB_SQLTYPE_VARCHAR, m_hypervisorInfo, DB_BIND_STATIC);
         DBBind(hStmt, 59, DB_SQLTYPE_INTEGER, m_id);

         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }

   // Save access list
   if (success)
      success = saveACLToDB(hdb);

   if ((m_softwarePackages != NULL) && success && (m_modified & MODIFY_SOFTWARE_INV))
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM software_inventory WHERE node_id=?"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         if (DBExecute(hStmt))
         {
            for(int i = 0; success && i < m_softwarePackages->size(); i++)
               success = m_softwarePackages->get(i)->saveToDatabase(hdb, m_id);
         }
         else
         {
            success = false;
         }
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }

   if ((m_hardwareComponents != NULL) && success && (m_modified & MODIFY_HARDWARE))
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM hardware_inventory WHERE node_id=?"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         if (DBExecute(hStmt))
         {
            for(int i = 0; success && i < m_hardwareComponents->size(); i++)
               success = m_hardwareComponents->get(i)->saveToDatabase(hdb, m_id);
         }
         else
         {
            success = false;
         }
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }

   unlockProperties();

   // Save data collection items
   if (success && (m_modified & MODIFY_DATA_COLLECTION))
   {
      lockDciAccess(false);
      for(int i = 0; success && (i < m_dcObjects->size()); i++)
         success = m_dcObjects->get(i)->saveToDatabase(hdb);
      unlockDciAccess();
   }

   // Clear modifications flag
   lockProperties();
   m_modified = 0;
   unlockProperties();

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

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("UPDATE nodes SET last_agent_comm_time=?,syslog_msg_count=?,snmp_trap_count=? WHERE id=?"));
   if (hStmt == NULL)
      return false;

   lockProperties();
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (INT32)m_lastAgentCommTime);
   DBBind(hStmt, 2, DB_SQLTYPE_BIGINT, m_syslogMessageCount);
   DBBind(hStmt, 3, DB_SQLTYPE_BIGINT, m_snmpTrapCount);
   DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_id);
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
   return success;
}

/**
 * Get ARP cache from node
 */
ArpCache *Node::getArpCache(bool forceRead)
{
   ArpCache *arpCache = NULL;
   if (!forceRead)
   {
      lockProperties();
      if ((m_arpCache != NULL) && (m_arpCache->timestamp() > time(NULL) - 3600))
      {
         arpCache = m_arpCache;
         arpCache->incRefCount();
      }
      unlockProperties();
      if (arpCache != NULL)
         return arpCache;
   }

   if (m_capabilities & NC_IS_LOCAL_MGMT)
   {
      arpCache = GetLocalArpCache();
   }
   else if (m_capabilities & NC_IS_NATIVE_AGENT)
   {
      AgentConnectionEx *conn = getAgentConnection();
      if (conn != NULL)
      {
         arpCache = conn->getArpCache();
         conn->decRefCount();
      }
   }
   else if (m_capabilities & NC_IS_SNMP)
   {
      SNMP_Transport *transport = createSnmpTransport();
      if (transport != NULL)
      {
         if (m_driver != NULL)
         {
            arpCache = m_driver->getArpCache(transport, m_driverData);
         }
         delete transport;
      }
   }

   if (arpCache != NULL)
   {
      nxlog_debug_tag(DEBUG_TAG_TOPO_ARP, 6, _T("Read ARP cache from node %s [%u] (%d entries)"), m_name, m_id, arpCache->size());
      arpCache->dumpToLog();

      lockProperties();
      if (m_arpCache != NULL)
         m_arpCache->decRefCount();
      m_arpCache = arpCache;
      m_arpCache->incRefCount();
      unlockProperties();
   }
   return arpCache;
}

/**
 * Get list of interfaces from node
 */
InterfaceList *Node::getInterfaceList()
{
   InterfaceList *pIfList = NULL;

   if ((m_capabilities & NC_IS_NATIVE_AGENT) && (!(m_flags & NF_DISABLE_NXCP)))
   {
      AgentConnectionEx *conn = getAgentConnection();
      if (conn != NULL)
      {
         pIfList = conn->getInterfaceList();
         conn->decRefCount();
      }
   }
   if ((pIfList == NULL) && (m_capabilities & NC_IS_LOCAL_MGMT))
   {
      pIfList = GetLocalInterfaceList();
   }
   if ((pIfList == NULL) && (m_capabilities & NC_IS_SNMP) &&
       (!(m_flags & NF_DISABLE_SNMP)) && (m_driver != NULL))
   {
      SNMP_Transport *pTransport = createSnmpTransport();
      if (pTransport != NULL)
      {
         bool useIfXTable;
         if (m_nUseIfXTable == IFXTABLE_DEFAULT)
         {
            useIfXTable = (ConfigReadBoolean(_T("UseIfXTable"), true) != 0) ? true : false;
         }
         else
         {
            useIfXTable = (m_nUseIfXTable == IFXTABLE_ENABLED) ? true : false;
         }

         int useAliases = ConfigReadInt(_T("UseInterfaceAliases"), 0);
         DbgPrintf(6, _T("Node::getInterfaceList(node=%s [%d]): calling driver (useAliases=%d, useIfXTable=%d)"),
                  m_name, (int)m_id, useAliases, useIfXTable);
         pIfList = m_driver->getInterfaces(pTransport, &m_customAttributes, m_driverData, useAliases, useIfXTable);

         if ((pIfList != NULL) && (m_capabilities & NC_IS_BRIDGE))
         {
            BridgeMapPorts(pTransport, pIfList);
         }
         delete pTransport;
      }
      else
      {
         DbgPrintf(6, _T("Node::getInterfaceList(node=%s [%d]): cannot create SNMP transport"), m_name, (int)m_id);
      }
   }

   if (pIfList != NULL)
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
   if (m_vrrpInfo != NULL)
   {
      DbgPrintf(6, _T("Node::addVrrpInterfaces(node=%s [%d]): m_vrrpInfo->size()=%d"), m_name, (int)m_id, m_vrrpInfo->size());

      for(i = 0; i < m_vrrpInfo->size(); i++)
      {
         VrrpRouter *router = m_vrrpInfo->getRouter(i);
         DbgPrintf(6, _T("Node::addVrrpInterfaces(node=%s [%d]): vrouter %d state=%d"), m_name, (int)m_id, i, router->getState());
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
            UINT32 vip = router->getVip(j);
            DbgPrintf(6, _T("Node::addVrrpInterfaces(node=%s [%d]): checking VIP %s@%d"), m_name, (int)m_id, IpToStr(vip, buffer), i);
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
                  DbgPrintf(6, _T("Node::addVrrpInterfaces(node=%s [%d]): added interface %s"), m_name, (int)m_id, iface->name);
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
 * @return pointer to interface object or NULL if appropriate interface couldn't be found
 */
Interface *Node::findInterfaceByIndex(UINT32 ifIndex)
{
   lockChildList(false);
   for(int i = 0; i < m_childList->size(); i++)
      if (m_childList->get(i)->getObjectClass() == OBJECT_INTERFACE)
      {
         Interface *pInterface = (Interface *)m_childList->get(i);
         if (pInterface->getIfIndex() == ifIndex)
         {
            unlockChildList();
            return pInterface;
         }
      }
   unlockChildList();
   return NULL;
}

/**
 * Find interface by name or description
 * Returns pointer to interface object or NULL if appropriate interface couldn't be found
 */
Interface *Node::findInterfaceByName(const TCHAR *name)
{
   if ((name == NULL) || (name[0] == 0))
      return NULL;

   Interface *pInterface;

   lockChildList(false);
   for(int i = 0; i < m_childList->size(); i++)
      if (m_childList->get(i)->getObjectClass() == OBJECT_INTERFACE)
      {
         pInterface = (Interface *)m_childList->get(i);
         if (!_tcsicmp(pInterface->getName(), name) || !_tcsicmp(pInterface->getDescription(), name))
         {
            unlockChildList();
            return pInterface;
         }
      }
   unlockChildList();
   return NULL;
}

/**
 * Find interface by slot/port pair
 * Returns pointer to interface object or NULL if appropriate interface couldn't be found
 */
Interface *Node::findInterfaceBySlotAndPort(UINT32 slot, UINT32 port)
{
   lockChildList(false);
   for(int i = 0; i < m_childList->size(); i++)
      if (m_childList->get(i)->getObjectClass() == OBJECT_INTERFACE)
      {
         Interface *pInterface = (Interface *)m_childList->get(i);
         if (pInterface->isPhysicalPort() && (pInterface->getSlotNumber() == slot) && (pInterface->getPortNumber() == port))
         {
            unlockChildList();
            return pInterface;
         }
      }
   unlockChildList();
   return NULL;
}

/**
 * Find interface by MAC address
 * Returns pointer to interface object or NULL if appropriate interface couldn't be found
 */
Interface *Node::findInterfaceByMAC(const BYTE *macAddr)
{
   Interface *iface = NULL;
   lockChildList(false);
   for(int i = 0; i < m_childList->size(); i++)
   {
      NetObj *curr = m_childList->get(i);
      if (curr->getObjectClass() == OBJECT_INTERFACE)
      {
         if (!memcmp(((Interface *)curr)->getMacAddr(), macAddr, MAC_ADDR_LENGTH))
         {
            iface = (Interface *)curr;
            break;
         }
      }
   }
   unlockChildList();
   return iface;
}

/**
 * Find interface by IP address
 * Returns pointer to interface object or NULL if appropriate interface couldn't be found
 */
Interface *Node::findInterfaceByIP(const InetAddress& addr)
{
   if (!addr.isValid())
      return NULL;

   lockChildList(false);
   for(int i = 0; i < m_childList->size(); i++)
      if (m_childList->get(i)->getObjectClass() == OBJECT_INTERFACE)
      {
         Interface *pInterface = (Interface *)m_childList->get(i);
         if (pInterface->getIpAddressList()->hasAddress(addr))
         {
            unlockChildList();
            return pInterface;
         }
      }
   unlockChildList();
   return NULL;
}

/**
 * Find interface by bridge port number
 */
Interface *Node::findBridgePort(UINT32 bridgePortNumber)
{
   lockChildList(false);
   for(int i = 0; i < m_childList->size(); i++)
      if (m_childList->get(i)->getObjectClass() == OBJECT_INTERFACE)
      {
         Interface *pInterface = (Interface *)m_childList->get(i);
         if (pInterface->getBridgePortNumber() == bridgePortNumber)
         {
            unlockChildList();
            return pInterface;
         }
      }
   unlockChildList();
   return NULL;
}

/**
 * Find connection point for node
 */
NetObj *Node::findConnectionPoint(UINT32 *localIfId, BYTE *localMacAddr, int *type)
{
   NetObj *cp = NULL;
   lockChildList(false);
   for(int i = 0; i < m_childList->size(); i++)
   {
      if (m_childList->get(i)->getObjectClass() == OBJECT_INTERFACE)
      {
         Interface *iface = (Interface *)m_childList->get(i);
         cp = FindInterfaceConnectionPoint(iface->getMacAddr(), type);
         if (cp != NULL)
         {
            *localIfId = iface->getId();
            memcpy(localMacAddr, iface->getMacAddr(), MAC_ADDR_LENGTH);
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
AccessPoint *Node::findAccessPointByMAC(const BYTE *macAddr)
{
   AccessPoint *ap = NULL;
   lockChildList(false);
   for(int i = 0; i < m_childList->size(); i++)
   {
      NetObj *curr = m_childList->get(i);
      if (curr->getObjectClass() == OBJECT_ACCESSPOINT)
      {
         if (!memcmp(((AccessPoint *)curr)->getMacAddr(), macAddr, MAC_ADDR_LENGTH))
         {
            ap = (AccessPoint *)curr;
            break;
         }
      }
   }
   unlockChildList();
   return ap;
}

/**
 * Find access point by radio ID (radio interface index)
 */
AccessPoint *Node::findAccessPointByRadioId(int rfIndex)
{
   AccessPoint *ap = NULL;
   lockChildList(false);
   for(int i = 0; i < m_childList->size(); i++)
   {
      NetObj *curr = m_childList->get(i);
      if (curr->getObjectClass() == OBJECT_ACCESSPOINT)
      {
         if (((AccessPoint *)curr)->isMyRadio(rfIndex))
         {
            ap = (AccessPoint *)curr;
            break;
         }
      }
   }
   unlockChildList();
   return ap;
}

/**
 * Find attached access point by BSSID
 */
AccessPoint *Node::findAccessPointByBSSID(const BYTE *bssid)
{
   AccessPoint *ap = NULL;
   lockChildList(false);
   for(int i = 0; i < m_childList->size(); i++)
   {
      NetObj *curr = m_childList->get(i);
      if (curr->getObjectClass() == OBJECT_ACCESSPOINT)
      {
         if (!memcmp(((AccessPoint *)curr)->getMacAddr(), bssid, MAC_ADDR_LENGTH) ||
             ((AccessPoint *)curr)->isMyRadio(bssid))
         {
            ap = (AccessPoint *)curr;
            break;
         }
      }
   }
   unlockChildList();
   return ap;
}

/**
 * Check if given IP address is one of node's interfaces
 */
bool Node::isMyIP(const InetAddress& addr)
{
   lockChildList(false);
   for(int i = 0; i < m_childList->size(); i++)
      if (m_childList->get(i)->getObjectClass() == OBJECT_INTERFACE)
      {
         if (((Interface *)m_childList->get(i))->getIpAddressList()->hasAddress(addr))
         {
            unlockChildList();
            return true;
         }
      }
   unlockChildList();
   return false;
}

/**
 * Filter interface - should return true if system should proceed with interface creation
 */
bool Node::filterInterface(InterfaceInfo *info)
{
   NXSL_VM *vm = CreateServerScriptVM(_T("Hook::CreateInterface"), this);
   if (vm == NULL)
   {
      DbgPrintf(7, _T("Node::filterInterface(%s [%u]): hook script \"Hook::CreateInterface\" not found"), m_name, m_id);
      return true;
   }

   Interface *iface;
   if (info->name[0] != 0)
   {
      iface = new Interface(info->name, (info->description[0] != 0) ? info->description : info->name,
                                 info->index, info->ipAddrList, info->type, m_zoneUIN);
   }
   else
   {
      iface = new Interface(info->ipAddrList, m_zoneUIN, false);
   }
   iface->setMacAddr(info->macAddr, false);
   iface->setBridgePortNumber(info->bridgePort);
   iface->setSlotNumber(info->slot);
   iface->setPortNumber(info->port);
   iface->setPhysicalPortFlag(info->isPhysicalPort);
   iface->setManualCreationFlag(false);
   iface->setSystemFlag(info->isSystem);
   iface->setMTU(info->mtu);
   iface->setSpeed(info->speed);
   iface->setIfTableSuffix(info->ifTableSuffixLength, info->ifTableSuffix);

   bool pass = true;
   NXSL_Value *argv = vm->createValue(new NXSL_Object(vm, &g_nxslInterfaceClass, iface));
   if (vm->run(1, &argv))
   {
      NXSL_Value *result = vm->getResult();
      if ((result != NULL) && result->isInteger())
      {
         pass = (result->getValueAsInt32() != 0);
      }
   }
   else
   {
      DbgPrintf(4, _T("Node::filterInterface(%s [%u]): hook script execution error: %s"), m_name, m_id, vm->getErrorText());
   }
   delete vm;
   delete iface;

   DbgPrintf(6, _T("Node::filterInterface(%s [%u]): interface \"%s\" (ifIndex=%d) %s by filter"),
             m_name, m_id, info->name, info->index, pass ? _T("accepted") : _T("rejected"));
   return pass;
}

/**
 * Create new interface - convenience wrapper
 */
Interface *Node::createNewInterface(const InetAddress& ipAddr, BYTE *macAddr, bool fakeInterface)
{
   InterfaceInfo info(1);
   info.ipAddrList.add(ipAddr);
   if (macAddr != NULL)
      memcpy(info.macAddr, macAddr, MAC_ADDR_LENGTH);
   return createNewInterface(&info, false, fakeInterface);
}

/**
 * Create new interface
 */
Interface *Node::createNewInterface(InterfaceInfo *info, bool manuallyCreated, bool fakeInterface)
{
   bool bSyntheticMask = false;
   TCHAR buffer[64];

   DbgPrintf(5, _T("Node::createNewInterface(\"%s\", %d, %d, bp=%d, slot=%d, port=%d) called for node %s [%d]"),
      info->name, info->index, info->type, info->bridgePort, info->slot, info->port, m_name, m_id);
   for(int i = 0; i < info->ipAddrList.size(); i++)
   {
      const InetAddress& addr = info->ipAddrList.get(i);
      DbgPrintf(5, _T("Node::createNewInterface(%s): IP address %s/%d"), info->name, addr.toString(buffer), addr.getMaskBits());
   }

   // Find subnet to place interface object to
   if (info->type != IFTYPE_SOFTWARE_LOOPBACK)
   {
      Cluster *pCluster = getMyCluster();
      for(int i = 0; i < info->ipAddrList.size(); i++)
      {
         InetAddress addr = info->ipAddrList.get(i);
         bool addToSubnet = addr.isValidUnicast() && ((pCluster == NULL) || !pCluster->isSyncAddr(addr));
         DbgPrintf(5, _T("Node::createNewInterface: node=%s [%d] ip=%s/%d cluster=%s [%d] add=%s"),
                   m_name, m_id, addr.toString(buffer), addr.getMaskBits(),
                   (pCluster != NULL) ? pCluster->getName() : _T("(null)"),
                   (pCluster != NULL) ? pCluster->getId() : 0, addToSubnet ? _T("yes") : _T("no"));
         if (addToSubnet)
         {
            Subnet *pSubnet = FindSubnetForNode(m_zoneUIN, addr);
            if (pSubnet == NULL)
            {
               // Check if netmask is 0 (detect), and if yes, create
               // new subnet with default mask
               if (addr.getMaskBits() == 0)
               {
                  bSyntheticMask = true;
                  addr.setMaskBits((addr.getFamily() == AF_INET) ? ConfigReadInt(_T("DefaultSubnetMaskIPv4"), 24) : ConfigReadInt(_T("DefaultSubnetMaskIPv6"), 64));
                  info->ipAddrList.replace(addr);
               }

               // Create new subnet object
               if (addr.getHostBits() >= 2)
               {
                  pSubnet = createSubnet(addr, bSyntheticMask);
                  if (bSyntheticMask)
                  {
                     // createSubnet may adjust address mask bits
                     info->ipAddrList.replace(addr);
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
            if (pSubnet != NULL)
            {
               pSubnet->addNode(this);
            }
         }  // addToSubnet
      } // loop by address list
   }

   // Create interface object
   Interface *iface;
   if (info->name[0] != 0)
      iface = new Interface(info->name, (info->description[0] != 0) ? info->description : info->name,
                                 info->index, info->ipAddrList, info->type, m_zoneUIN);
   else
      iface = new Interface(info->ipAddrList, m_zoneUIN, bSyntheticMask);
   iface->setMacAddr(info->macAddr, false);
   iface->setBridgePortNumber(info->bridgePort);
   iface->setSlotNumber(info->slot);
   iface->setPortNumber(info->port);
   iface->setPhysicalPortFlag(info->isPhysicalPort);
   iface->setManualCreationFlag(manuallyCreated);
   iface->setSystemFlag(info->isSystem);
   iface->setMTU(info->mtu);
   iface->setSpeed(info->speed);
   iface->setIfTableSuffix(info->ifTableSuffixLength, info->ifTableSuffix);

   int defaultExpectedState = ConfigReadInt(_T("DefaultInterfaceExpectedState"), IF_DEFAULT_EXPECTED_STATE_UP);
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

   // Insert to objects' list and generate event
   NetObjInsert(iface, true, false);
   addInterface(iface);
   if (!m_isHidden)
      iface->unhide();
   if (!iface->isSystem())
   {
      const InetAddress& addr = iface->getFirstIpAddress();
      PostEvent(EVENT_INTERFACE_ADDED, m_id, "dsAdd", iface->getId(),
                iface->getName(), &addr, addr.getMaskBits(), iface->getIfIndex());
   }

   return iface;
}

/**
 * Delete interface from node
 */
void Node::deleteInterface(Interface *iface)
{
   DbgPrintf(5, _T("Node::deleteInterface(node=%s [%d], interface=%s [%d])"), m_name, m_id, iface->getName(), iface->getId());

   // Check if we should unlink node from interface's subnet
   if (!iface->isExcludedFromTopology())
   {
      const ObjectArray<InetAddress> *list = iface->getIpAddressList()->getList();
      for(int i = 0; i < list->size(); i++)
      {
         bool doUnlink = true;
         const InetAddress *addr = list->get(i);

         lockChildList(false);
         for(int j = 0; j < m_childList->size(); j++)
         {
            NetObj *curr = m_childList->get(j);
            if ((curr->getObjectClass() == OBJECT_INTERFACE) && (curr != iface) &&
                ((Interface *)curr)->getIpAddressList()->findSameSubnetAddress(*addr).isValid())
            {
               doUnlink = false;
               break;
            }
         }
         unlockChildList();

         if (doUnlink)
         {
            // Last interface in subnet, should unlink node
            Subnet *pSubnet = FindSubnetByIP(m_zoneUIN, addr->getSubnetAddress());
            if (pSubnet != NULL)
            {
               deleteParent(pSubnet);
               pSubnet->deleteChild(this);
            }
            DbgPrintf(5, _T("Node::deleteInterface(node=%s [%d], interface=%s [%d]): unlinked from subnet %s [%d]"),
                      m_name, m_id, iface->getName(), iface->getId(),
                      (pSubnet != NULL) ? pSubnet->getName() : _T("(null)"),
                      (pSubnet != NULL) ? pSubnet->getId() : 0);
         }
      }
   }
   iface->deleteObject();
}

/**
 * Calculate node status based on child objects status
 */
void Node::calculateCompoundStatus(BOOL bForcedRecalc)
{
   int iOldStatus = m_status;
   static UINT32 dwEventCodes[] = { EVENT_NODE_NORMAL, EVENT_NODE_WARNING,
      EVENT_NODE_MINOR, EVENT_NODE_MAJOR, EVENT_NODE_CRITICAL,
      EVENT_NODE_UNKNOWN, EVENT_NODE_UNMANAGED };

   super::calculateCompoundStatus(bForcedRecalc);
   if (m_status != iOldStatus)
      PostEvent(dwEventCodes[m_status], m_id, "d", iOldStatus);
}

/**
 * Perform status poll on node
 */
void Node::statusPoll(PollerInfo *poller, ClientSession *pSession, UINT32 rqId)
{
   if (m_runtimeFlags & DCDF_DELETE_IN_PROGRESS)
   {
      if (rqId == 0)
         m_runtimeFlags &= ~DCDF_QUEUED_FOR_STATUS_POLL;
      return;
   }

   if (IsShutdownInProgress())
      return;

   UINT32 oldCapabilities = m_capabilities;
   UINT32 oldState = m_state;
   NetObj *pPollerNode = NULL, **ppPollList;
   SNMP_Transport *pTransport;
   Cluster *pCluster;
   time_t tNow, tExpire;

   Queue *pQueue = new Queue;     // Delayed event queue
   poller->setStatus(_T("wait for lock"));
   pollerLock();

   if (IsShutdownInProgress())
   {
      delete pQueue;
      pollerUnlock();
      return;
   }

   poller->setStatus(_T("preparing"));
   m_pollRequestor = pSession;
   sendPollerMsg(rqId, _T("Starting status poll for node %s\r\n"), m_name);
   nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Starting status poll for node %s (ID: %d)"), m_name, m_id);

   // Read capability expiration time and current time
   tExpire = (time_t)ConfigReadULong(_T("CapabilityExpirationTime"), 604800);
   tNow = time(NULL);

   bool agentConnected = false;

   int retryCount = 5;

restart_agent_check:
   if (g_flags & AF_RESOLVE_IP_FOR_EACH_STATUS_POLL)
   {
      poller->setStatus(_T("updating primary IP"));
      updatePrimaryIpAddr();
   }

   UINT32 requiredPolls = (m_requiredPollCount > 0) ? m_requiredPollCount : g_requiredPolls;

   // Check SNMP agent connectivity
   if ((m_capabilities & NC_IS_SNMP) && (!(m_flags & NF_DISABLE_SNMP)) && m_ipAddress.isValidUnicast())
   {
      TCHAR szBuffer[256];
      UINT32 dwResult;

      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): check SNMP"), m_name);
      pTransport = createSnmpTransport();
      if (pTransport != NULL)
      {
         poller->setStatus(_T("check SNMP"));
         sendPollerMsg(rqId, _T("Checking SNMP agent connectivity\r\n"));
         const TCHAR *testOid = m_customAttributes.get(_T("snmp.testoid"));
         if (testOid == NULL)
         {
            testOid = _T(".1.3.6.1.2.1.1.2.0");
         }
         dwResult = SnmpGet(m_snmpVersion, pTransport, testOid, NULL, 0, szBuffer, sizeof(szBuffer), 0);
         if ((dwResult == SNMP_ERR_SUCCESS) || (dwResult == SNMP_ERR_NO_OBJECT))
         {
            if (m_state & NSF_SNMP_UNREACHABLE)
            {
               m_pollCountSNMP++;
               if (m_pollCountSNMP >= requiredPolls)
               {
                  m_state &= ~NSF_SNMP_UNREACHABLE;
                  PostEventEx(pQueue, EVENT_SNMP_OK, m_id, NULL);
                  sendPollerMsg(rqId, POLLER_INFO _T("Connectivity with SNMP agent restored\r\n"));
                  m_pollCountSNMP = 0;
               }

            }
            else
            {
               m_pollCountSNMP = 0;
            }

            // Update authoritative engine data for SNMPv3
            if ((pTransport->getSnmpVersion() == SNMP_VERSION_3) && (pTransport->getAuthoritativeEngine() != NULL))
            {
               lockProperties();
               m_snmpSecurity->setAuthoritativeEngine(*pTransport->getAuthoritativeEngine());
               unlockProperties();
            }
         }
         else if ((dwResult == SNMP_ERR_ENGINE_ID) && (m_snmpVersion == SNMP_VERSION_3) && (retryCount > 0))
         {
            // Reset authoritative engine data
            lockProperties();
            m_snmpSecurity->setAuthoritativeEngine(SNMP_Engine());
            unlockProperties();
            delete pTransport;
            retryCount--;
            goto restart_agent_check;
         }
         else
         {
            if (pTransport->isProxyTransport() && (dwResult == SNMP_ERR_COMM))
            {
               AgentConnectionEx *pconn = acquireProxyConnection(SNMP_PROXY, true);
               if (pconn != NULL)
               {
                  pconn->decRefCount();
                  if (retryCount > 0)
                  {
                     retryCount--;
                     delete pTransport;
                     goto restart_agent_check;
                  }
               }
            }

            nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): got communication error on proxy transport, checking connection to proxy. Poll count: %d of %d"), m_name, m_pollCountSNMP, m_requiredPollCount);
            sendPollerMsg(rqId, POLLER_ERROR _T("SNMP agent unreachable\r\n"));
            if (m_state & NSF_SNMP_UNREACHABLE)
            {
               if ((tNow > m_failTimeSNMP + tExpire) && (!(m_state & DCSF_UNREACHABLE)))
               {
                  m_capabilities &= ~NC_IS_SNMP;
                  m_state &= ~NSF_SNMP_UNREACHABLE;
                  m_snmpObjectId[0] = 0;
                  sendPollerMsg(rqId, POLLER_WARNING _T("Attribute isSNMP set to FALSE\r\n"));
               }
            }
            else
            {
               m_pollCountSNMP++;
               if (m_pollCountSNMP >= requiredPolls)
               {
                  m_state |= NSF_SNMP_UNREACHABLE;
                  PostEventEx(pQueue, EVENT_SNMP_FAIL, m_id, NULL);
                  m_failTimeSNMP = tNow;
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

   // Check native agent connectivity
   if ((m_capabilities & NC_IS_NATIVE_AGENT) && (!(m_flags & NF_DISABLE_NXCP)))
   {
      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): checking agent"), m_name);
      poller->setStatus(_T("check agent"));
      sendPollerMsg(rqId, _T("Checking NetXMS agent connectivity\r\n"));

      UINT32 error, socketError;
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
               PostEventEx(pQueue, EVENT_AGENT_OK, m_id, NULL);
               sendPollerMsg(rqId, POLLER_INFO _T("Connectivity with NetXMS agent restored\r\n"));
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
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): agent unreachable, error=%d, socketError=%d. Poll count %d of %d"), m_name, (int)error, (int)socketError, m_pollCountAgent, m_requiredPollCount);
         sendPollerMsg(rqId, POLLER_ERROR _T("NetXMS agent unreachable\r\n"));
         if (m_state & NSF_AGENT_UNREACHABLE)
         {
            if ((tNow > m_failTimeAgent + tExpire) && !(m_state & DCSF_UNREACHABLE))
            {
               m_capabilities &= ~NC_IS_NATIVE_AGENT;
               m_state &= ~NSF_AGENT_UNREACHABLE;
               m_platformName[0] = 0;
               m_agentVersion[0] = 0;
               sendPollerMsg(rqId, POLLER_WARNING _T("Attribute isNetXMSAgent set to FALSE\r\n"));
            }
         }
         else
         {
            m_pollCountAgent++;
            if (m_pollCountAgent >= requiredPolls)
            {
               m_state |= NSF_AGENT_UNREACHABLE;
               PostEventEx(pQueue, EVENT_AGENT_FAIL, m_id, NULL);
               m_failTimeAgent = tNow;
               //cancel file monitoring locally(on agent it is canceled if agent have fallen)
               g_monitoringList.removeDisconnectedNode(m_id);
               m_pollCountAgent = 0;
            }
         }
      }
      agentUnlock();
      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("StatusPoll(%s): agent check finished"), m_name);

      // If file update connection is active, send NOP command to prevent disconnection by idle timeout
      AgentConnection *fileUpdateConnection;
      lockProperties();
      fileUpdateConnection = m_fileUpdateConn;
      if (fileUpdateConnection != NULL)
         fileUpdateConnection->incRefCount();
      unlockProperties();
      if (fileUpdateConnection != NULL)
      {
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): sending keepalive command on file monitoring connection"), m_name);
         fileUpdateConnection->nop();
         fileUpdateConnection->decRefCount();
      }
   }

   poller->setStatus(_T("prepare polling list"));

   // Find service poller node object
   lockProperties();
   if (m_pollerNode != 0)
   {
      UINT32 id = m_pollerNode;
      unlockProperties();
      pPollerNode = FindObjectById(id);
      if (pPollerNode != NULL)
      {
         if (pPollerNode->getObjectClass() != OBJECT_NODE)
            pPollerNode = NULL;
      }
   }
   else
   {
      unlockProperties();
   }

   // If nothing found, use management server
   if (pPollerNode == NULL)
   {
      pPollerNode = FindObjectById(g_dwMgmtNode);
      if (pPollerNode != NULL)
         pPollerNode->incRefCount();
   }
   else
   {
      pPollerNode->incRefCount();
   }

   // Create polling list
   ppPollList = (NetObj **)malloc(sizeof(NetObj *) * m_childList->size());
   lockChildList(false);
   int pollListSize = 0;
   for(int i = 0; i < m_childList->size(); i++)
   {
      NetObj *curr = m_childList->get(i);
      if (curr->getStatus() != STATUS_UNMANAGED)
      {
         curr->incRefCount();
         ppPollList[pollListSize++] = curr;
      }
   }
   unlockChildList();

   // Poll interfaces and services
   poller->setStatus(_T("child poll"));
   DbgPrintf(7, _T("StatusPoll(%s): starting child object poll"), m_name);
   pCluster = getMyCluster();
   pTransport = createSnmpTransport();
   for(int i = 0; i < pollListSize; i++)
   {
      switch(ppPollList[i]->getObjectClass())
      {
         case OBJECT_INTERFACE:
            DbgPrintf(7, _T("StatusPoll(%s): polling interface %d [%s]"), m_name, ppPollList[i]->getId(), ppPollList[i]->getName());
            ((Interface *)ppPollList[i])->statusPoll(pSession, rqId, pQueue, pCluster, pTransport, m_icmpProxy);
            break;
         case OBJECT_NETWORKSERVICE:
            DbgPrintf(7, _T("StatusPoll(%s): polling network service %d [%s]"), m_name, ppPollList[i]->getId(), ppPollList[i]->getName());
            ((NetworkService *)ppPollList[i])->statusPoll(pSession, rqId, (Node *)pPollerNode, pQueue);
            break;
         case OBJECT_ACCESSPOINT:
            DbgPrintf(7, _T("StatusPoll(%s): polling access point %d [%s]"), m_name, ppPollList[i]->getId(), ppPollList[i]->getName());
            ((AccessPoint *)ppPollList[i])->statusPollFromController(pSession, rqId, pQueue, this, pTransport);
            break;
         default:
            DbgPrintf(7, _T("StatusPoll(%s): skipping object %d [%s] class %d"), m_name, ppPollList[i]->getId(), ppPollList[i]->getName(), ppPollList[i]->getObjectClass());
            break;
      }
      ppPollList[i]->decRefCount();
   }
   delete pTransport;
   free(ppPollList);
   DbgPrintf(7, _T("StatusPoll(%s): finished child object poll"), m_name);

   // Check if entire node is down
   // This check is disabled for nodes without IP address
   // The only exception is node without valid address connected via agent tunnel
   if (m_ipAddress.isValidUnicast() || agentConnected)
   {
      bool allDown = true;
      lockChildList(false);
      for(int i = 0; i < m_childList->size(); i++)
      {
         NetObj *curr = m_childList->get(i);
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
      if (allDown && (m_capabilities & NC_IS_NATIVE_AGENT) &&
          !(m_flags & NF_DISABLE_NXCP) && !(m_state & NSF_AGENT_UNREACHABLE))
      {
         allDown = false;
      }
      if (allDown && (m_capabilities & NC_IS_SNMP) &&
          !(m_flags & NF_DISABLE_SNMP) && !(m_state & NSF_SNMP_UNREACHABLE))
      {
         allDown = false;
      }

      DbgPrintf(6, _T("StatusPoll(%s): allDown=%s, statFlags=0x%08X"), m_name, allDown ? _T("true") : _T("false"), m_state);
      if (allDown)
      {
         if (!(m_state & DCSF_UNREACHABLE))
         {
            m_state |= DCSF_UNREACHABLE;
            m_downSince = time(NULL);
            poller->setStatus(_T("check network path"));
            if (checkNetworkPath(rqId))
            {
               m_state |= DCSF_NETWORK_PATH_PROBLEM;

               // Set interfaces and network services to UNKNOWN state
               lockChildList(false);
               for(int i = 0; i < m_childList->size(); i++)
               {
                  NetObj *curr = m_childList->get(i);
                  if ((curr->getObjectClass() == OBJECT_INTERFACE) || (curr->getObjectClass() == OBJECT_NETWORKSERVICE))
                  {
                     curr->resetStatus();
                  }
               }
               unlockChildList();

               // Clear delayed event queue
               while(true)
               {
                  Event *e = (Event *)pQueue->get();
                  if (e == NULL)
                     break;
                  delete e;
               }
               delete_and_null(pQueue);

               PostEvent(EVENT_NODE_UNREACHABLE, m_id, NULL);
            }
            else
            {
               PostEvent(EVENT_NODE_DOWN, m_id, NULL);
            }
            g_monitoringList.removeDisconnectedNode(m_id);
            sendPollerMsg(rqId, POLLER_ERROR _T("Node is unreachable\r\n"));
         }
         else
         {
            if((m_state & DCSF_NETWORK_PATH_PROBLEM) && !checkNetworkPath(rqId))
            {
               PostEvent(EVENT_NODE_DOWN, m_id, NULL);
               m_state &= ~DCSF_NETWORK_PATH_PROBLEM;
            }
            sendPollerMsg(rqId, POLLER_WARNING _T("Node is still unreachable\r\n"));
         }
      }
      else
      {
         m_downSince = 0;
         if (m_state & DCSF_UNREACHABLE)
         {
            int reason = (m_state & DCSF_NETWORK_PATH_PROBLEM) ? 1 : 0;
            m_state &= ~(DCSF_UNREACHABLE | DCSF_NETWORK_PATH_PROBLEM);
            PostEvent(EVENT_NODE_UP, m_id, "d", reason);
            sendPollerMsg(rqId, POLLER_INFO _T("Node recovered from unreachable state\r\n"));
            goto restart_agent_check;
         }
         else
         {
            sendPollerMsg(rqId, POLLER_INFO _T("Node is connected\r\n"));
         }
      }
   }

   // Get uptime and update boot time
   if (!(m_state & DCSF_UNREACHABLE))
   {
      TCHAR buffer[MAX_RESULT_LENGTH];
      if (getItemFromAgent(_T("System.Uptime"), MAX_RESULT_LENGTH, buffer) == DCE_SUCCESS)
      {
         m_bootTime = time(NULL) - _tcstol(buffer, NULL, 0);
         DbgPrintf(5, _T("StatusPoll(%s [%d]): boot time set to %u from agent"), m_name, m_id, (UINT32)m_bootTime);
      }
      else if (getItemFromSNMP(m_snmpPort, _T(".1.3.6.1.2.1.1.3.0"), MAX_RESULT_LENGTH, buffer, SNMP_RAWTYPE_NONE) == DCE_SUCCESS)
      {
         m_bootTime = time(NULL) - _tcstol(buffer, NULL, 0) / 100;   // sysUpTime is in hundredths of a second
         DbgPrintf(5, _T("StatusPoll(%s [%d]): boot time set to %u from SNMP"), m_name, m_id, (UINT32)m_bootTime);
      }
      else
      {
         DbgPrintf(5, _T("StatusPoll(%s [%d]): unable to get system uptime"), m_name, m_id);
      }
   }
   else
   {
      m_bootTime = 0;
   }

   // Get agent uptime to check if it was restared
   if (!(m_state & DCSF_UNREACHABLE) && isNativeAgent())
   {
      TCHAR buffer[MAX_RESULT_LENGTH];
      if (getItemFromAgent(_T("Agent.Uptime"), MAX_RESULT_LENGTH, buffer) == DCE_SUCCESS)
      {
         time_t oldAgentuptime = m_agentUpTime;
         m_agentUpTime = _tcstol(buffer, NULL, 0);
         if ((UINT32)oldAgentuptime > (UINT32)m_agentUpTime)
         {
            //cancel file monitoring locally(on agent it is canceled if agent have fallen)
            g_monitoringList.removeDisconnectedNode(m_id);
         }
      }
      else
      {
         DbgPrintf(5, _T("StatusPoll(%s [%d]): unable to get agent uptime"), m_name, m_id);
         g_monitoringList.removeDisconnectedNode(m_id);
         m_agentUpTime = 0;
      }
   }
   else
   {
      m_agentUpTime = 0;
   }

   // Get geolocation
   if (!(m_state & DCSF_UNREACHABLE) && isNativeAgent())
   {
      TCHAR buffer[MAX_RESULT_LENGTH];
      if (getItemFromAgent(_T("GPS.LocationData"), MAX_RESULT_LENGTH, buffer) == DCE_SUCCESS)
      {
         GeoLocation loc = GeoLocation::parseAgentData(buffer);
         if (loc.getType() != GL_UNSET)
         {
            DbgPrintf(5, _T("StatusPoll(%s [%d]): location set to %s, %s from agent"), m_name, m_id, loc.getLatitudeAsString(), loc.getLongitudeAsString());
            setGeoLocation(loc);
         }
      }
      else
      {
         DbgPrintf(5, _T("StatusPoll(%s [%d]): unable to get system location"), m_name, m_id);
      }
   }

   // Get agent log and agent local database status
   if (!(m_state & DCSF_UNREACHABLE) && isNativeAgent())
   {
      TCHAR buffer[MAX_RESULT_LENGTH];
      if (getItemFromAgent(_T("Agent.LogFile.Status"), MAX_RESULT_LENGTH, buffer) == DCE_SUCCESS)
      {
         UINT32 status = _tcstol(buffer, NULL, 0);
         if (status != 0)
            PostEvent(EVENT_AGENT_LOG_PROBLEM, m_id, "ds", status, _T("could not open"));
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("StatusPoll(%s [%d]): unable to get agent log status"), m_name, m_id);
      }

      if (getItemFromAgent(_T("Agent.LocalDatabase.Status"), MAX_RESULT_LENGTH, buffer) == DCE_SUCCESS)
      {
         UINT32 status = _tcstol(buffer, NULL, 0);
         const TCHAR *statusDescription[3]= {
                                       _T("normal"),
                                       _T("could not open database"),
                                       _T("could not update database"),
                                       };
         if (status != 0)
            PostEvent(EVENT_AGENT_LOCAL_DATABASE_PROBLEM, m_id, "ds", status, statusDescription[status]);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("StatusPoll(%s [%d]): unable to get agent local database status"), m_name, m_id);
      }
   }

   // Send delayed events and destroy delayed event queue
   if (pQueue != NULL)
   {
      ResendEvents(pQueue);
      delete pQueue;
   }

   // Call hooks in loaded modules
   for(UINT32 i = 0; i < g_dwNumModules; i++)
   {
      if (g_pModuleList[i].pfStatusPollHook != NULL)
      {
         DbgPrintf(5, _T("StatusPoll(%s [%d]): calling hook in module %s"), m_name, m_id, g_pModuleList[i].szName);
         g_pModuleList[i].pfStatusPollHook(this, pSession, rqId, poller);
      }
   }

   // Execute hook script
   poller->setStatus(_T("hook"));
   executeHookScript(_T("StatusPoll"));

   poller->setStatus(_T("cleanup"));
   if (pPollerNode != NULL)
      pPollerNode->decRefCount();

   if (oldCapabilities != m_capabilities)
      PostEvent(EVENT_NODE_CAPABILITIES_CHANGED, m_id, "xx", oldCapabilities, m_capabilities);

   if (oldState != m_state || oldCapabilities != m_capabilities)
   {
      lockProperties();
      setModified(MODIFY_NODE_PROPERTIES);
      unlockProperties();
   }

   calculateCompoundStatus();
   m_lastStatusPoll = time(NULL);
   sendPollerMsg(rqId, _T("Finished status poll for node %s\r\n"), m_name);
   sendPollerMsg(rqId, _T("Node status after poll is %s\r\n"), GetStatusAsText(m_status, true));
   m_pollRequestor = NULL;
   if (rqId == 0)
      m_runtimeFlags &= ~DCDF_QUEUED_FOR_STATUS_POLL;
   pollerUnlock();
   DbgPrintf(5, _T("Finished status poll for node %s (ID: %d)"), m_name, m_id);

   // Check if the node has to be deleted due to long downtime
   if (rqId == 0)
   {
      time_t unreachableDeleteDays = (time_t)ConfigReadInt(_T("DeleteUnreachableNodesPeriod"), 0);
      if ((unreachableDeleteDays > 0) && (m_downSince > 0) &&
          (time(NULL) - m_downSince > unreachableDeleteDays * 24 * 3600))
      {
         deleteObject();
      }
   }
}

/**
 * Check single element of network path
 */
bool Node::checkNetworkPathElement(UINT32 nodeId, const TCHAR *nodeType, bool isProxy, UINT32 requestId, bool secondPass)
{
   Node *node = (Node *)FindObjectById(nodeId, OBJECT_NODE);
   if (node == NULL)
      return false;

   nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("Node::checkNetworkPathElement(%s [%d]): found %s: %s [%d]"), m_name, m_id, nodeType, node->getName(), node->getId());

   if (secondPass && (node->m_lastStatusPoll < time(NULL) - 1))
   {
      DbgPrintf(6, _T("Node::checkNetworkPathElement(%s [%d]): forced status poll on node %s [%d]"),
                m_name, m_id, node->getName(), node->getId());
      PollerInfo *poller = RegisterPoller(POLLER_TYPE_STATUS, node);
      poller->startExecution();
      node->statusPoll(poller, NULL, 0);
      delete poller;
   }

   if (node->isDown())
   {
      DbgPrintf(5, _T("Node::checkNetworkPathElement(%s [%d]): %s %s [%d] is down"),
                m_name, m_id, nodeType, node->getName(), node->getId());
      sendPollerMsg(requestId, POLLER_WARNING _T("   %s %s is down\r\n"), nodeType, node->getName());
      return true;
   }
   if (isProxy && node->isNativeAgent() && (node->getState() & NSF_AGENT_UNREACHABLE))
   {
      DbgPrintf(5, _T("Node::checkNetworkPathElement(%s [%d]): agent on %s %s [%d] is down"),
                m_name, m_id, nodeType, node->getName(), node->getId());
      sendPollerMsg(requestId, POLLER_WARNING _T("   Agent on %s %s is down\r\n"), nodeType, node->getName());
      return true;
   }
   return false;
}

/**
 * Check network path between node and management server to detect possible intermediate node failure - layer 2
 *
 * @return true if network path problems found
 */
bool Node::checkNetworkPathLayer2(UINT32 requestId, bool secondPass)
{
   time_t now = time(NULL);

   // Check proxy node(s)
   if (IsZoningEnabled() && (m_zoneUIN != 0))
   {
      Zone *zone = FindZoneByUIN(m_zoneUIN);
      if ((zone != NULL) && (zone->getProxyNodeId() != 0) && (zone->getProxyNodeId() != m_id))
      {
         if (checkNetworkPathElement(zone->getProxyNodeId(), _T("zone proxy"), true, requestId, secondPass))
            return true;
      }
   }

   // Check directly connected switch
   sendPollerMsg(requestId, _T("Checking ethernet connectivity...\r\n"));
   Interface *iface = findInterfaceByIP(m_ipAddress);
   if (iface != NULL)
   {
      if  (iface->getPeerNodeId() != 0)
      {
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("Node::checkNetworkPath(%s [%d]): found interface object for primary IP: %s [%d]"), m_name, m_id, iface->getName(), iface->getId());
         if (checkNetworkPathElement(iface->getPeerNodeId(), _T("upstream switch"), false, requestId, secondPass))
            return true;

         Node *switchNode = (Node *)FindObjectById(iface->getPeerNodeId(), OBJECT_NODE);
         Interface *switchIface = (Interface *)FindObjectById(iface->getPeerInterfaceId(), OBJECT_INTERFACE);
         if ((switchNode != NULL) && (switchIface != NULL) && (switchIface->getExpectedState() != IF_EXPECTED_STATE_IGNORE) &&
             ((switchIface->getAdminState() == IF_ADMIN_STATE_DOWN) || (switchIface->getAdminState() == IF_ADMIN_STATE_TESTING) ||
              (switchIface->getOperState() == IF_OPER_STATE_DOWN) || (switchIface->getOperState() == IF_OPER_STATE_TESTING)))
         {
            nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Node::checkNetworkPath(%s [%d]): upstream interface %s [%d] on switch %s [%d] is down"),
                        m_name, m_id, switchIface->getName(), switchIface->getId(), switchNode->getName(), switchNode->getId());
            sendPollerMsg(requestId, POLLER_WARNING _T("   Upstream interface %s on node %s is down\r\n"), switchIface->getName(), switchNode->getName());
            return true;
         }
      }
      else
      {
         BYTE localMacAddr[MAC_ADDR_LENGTH];
         memcpy(localMacAddr, iface->getMacAddr(), MAC_ADDR_LENGTH);
         int type = 0;
         NetObj *cp = FindInterfaceConnectionPoint(localMacAddr, &type);
         if (cp != NULL)
         {
            nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("Node::checkNetworkPath(%s [%d]): found connection point: %s [%d]"), m_name, m_id, cp->getName(), cp->getId());
            if (secondPass)
            {
               Node *node = (cp->getObjectClass() == OBJECT_INTERFACE) ? ((Interface *)cp)->getParentNode() : ((AccessPoint *)cp)->getParentNode();
               if ((node != NULL) && !node->isDown() && (node->m_lastStatusPoll < now - 1))
               {
                  nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("Node::checkNetworkPath(%s [%d]): forced status poll on node %s [%d]"),
                              m_name, m_id, node->getName(), node->getId());
                  node->statusPollWorkerEntry(RegisterPoller(POLLER_TYPE_STATUS, node), NULL, 0);
               }
            }

            if (cp->getObjectClass() == OBJECT_INTERFACE)
            {
               Interface *iface = (Interface *)cp;
               if ((iface->getExpectedState() != IF_EXPECTED_STATE_IGNORE) &&
                   ((iface->getAdminState() == IF_ADMIN_STATE_DOWN) || (iface->getAdminState() == IF_ADMIN_STATE_TESTING) ||
                    (iface->getOperState() == IF_OPER_STATE_DOWN) || (iface->getOperState() == IF_OPER_STATE_TESTING)))
               {
                  nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Node::checkNetworkPath(%s [%d]): upstream interface %s [%d] on switch %s [%d] is down"),
                              m_name, m_id, iface->getName(), iface->getId(), iface->getParentNode()->getName(), iface->getParentNode()->getId());
                  sendPollerMsg(requestId, POLLER_WARNING _T("   Upstream interface %s on node %s is down\r\n"),
                                iface->getName(), iface->getParentNode()->getName());
                  return true;
               }
            }
            else if (cp->getObjectClass() == OBJECT_ACCESSPOINT)
            {
               AccessPoint *ap = (AccessPoint *)cp;
               if (ap->getStatus() == STATUS_CRITICAL)   // FIXME: how to correctly determine if AP is down?
               {
                  nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Node::checkNetworkPath(%s [%d]): wireless access point %s [%d] is down"),
                              m_name, m_id, ap->getName(), ap->getId());
                  sendPollerMsg(requestId, POLLER_WARNING _T("   Wireless access point %s is down\r\n"), ap->getName());
                  return true;
               }
            }
         }
      }
   }
   else
   {
      DbgPrintf(5, _T("Node::checkNetworkPath(%s [%d]): cannot find interface object for primary IP"), m_name, m_id);
   }
   return false;
}

/**
 * Check network path between node and management server to detect possible intermediate node failure - layer 3
 *
 * @return true if network path problems found
 */
bool Node::checkNetworkPathLayer3(UINT32 requestId, bool secondPass)
{
   Node *mgmtNode = (Node *)FindObjectById(g_dwMgmtNode);
   if (mgmtNode == NULL)
   {
      DbgPrintf(5, _T("Node::checkNetworkPath(%s [%d]): cannot find management node"), m_name, m_id);
      return false;
   }

   NetworkPath *trace = TraceRoute(mgmtNode, this);
   if (trace == NULL)
   {
      DbgPrintf(5, _T("Node::checkNetworkPath(%s [%d]): trace not available"), m_name, m_id);
      return false;
   }
   DbgPrintf(5, _T("Node::checkNetworkPath(%s [%d]): trace available, %d hops, %s"),
             m_name, m_id, trace->getHopCount(), trace->isComplete() ? _T("complete") : _T("incomplete"));

   // We will do path check in two passes
   // If unreachable intermediate node will be found on first pass,
   // then method will just return true. Otherwise, we will do
   // second pass, this time forcing status poll on each node in the path.
   sendPollerMsg(requestId, _T("Checking network path (%s pass)...\r\n"), secondPass ? _T("second") : _T("first"));
   bool pathProblemFound = false;
   for(int i = 0; i < trace->getHopCount(); i++)
   {
      HOP_INFO *hop = trace->getHopInfo(i);
      if ((hop->object == NULL) || (hop->object == this) || (hop->object->getObjectClass() != OBJECT_NODE))
         continue;

      // Check for loops
      if (i > 0)
      {
         for(int j = i - 1; j >= 0; j--)
         {
            HOP_INFO *prevHop = trace->getHopInfo(j);
            if (prevHop->object == hop->object)
            {
               prevHop = trace->getHopInfo(i - 1);
               nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Node::checkNetworkPath(%s [%d]): routing loop detected on upstream node %s [%d]"),
                           m_name, m_id, prevHop->object->getName(), prevHop->object->getId());
               sendPollerMsg(requestId, POLLER_WARNING _T("   Routing loop detected on upstream node %s\r\n"), prevHop->object->getName());

               static const TCHAR *names[] =
                        { _T("protocol"), _T("destNodeId"), _T("destAddress"),
                          _T("sourceNodeId"), _T("sourceAddress"), _T("prefix"),
                          _T("prefixLength"), _T("nextHopNodeId"), _T("nextHopAddress")
                        };
               PostEventWithNames(EVENT_ROUTING_LOOP_DETECTED, prevHop->object->getId(), "siAiAAdiA", names,
                     (trace->getSourceAddress().getFamily() == AF_INET6) ? _T("IPv6") : _T("IPv4"),
                     m_id, &m_ipAddress, g_dwMgmtNode, &(trace->getSourceAddress()),
                     &prevHop->route, prevHop->route.getMaskBits(), hop->object->getId(), &prevHop->nextHop);

               pathProblemFound = true;
               break;
            }
         }
         if (pathProblemFound)
            break;
      }

      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("Node::checkNetworkPath(%s [%d]): checking upstream router %s [%d]"),
                  m_name, m_id, hop->object->getName(), hop->object->getId());
      if (checkNetworkPathElement(hop->object->getId(), _T("upstream router"), false, requestId, secondPass))
      {
         pathProblemFound = true;
         break;
      }

      if (hop->isVpn)
      {
         // Next hop is behind VPN tunnel
         VPNConnector *vpnConn = (VPNConnector *)FindObjectById(hop->ifIndex, OBJECT_VPNCONNECTOR);
         if ((vpnConn != NULL) && (vpnConn->getStatus() == STATUS_CRITICAL))
         {
            /* TODO: mark as path problem */
         }
      }
      else
      {
         Interface *iface = ((Node *)hop->object)->findInterfaceByIndex(hop->ifIndex);
         if ((iface != NULL) && (iface->getExpectedState() != IF_EXPECTED_STATE_IGNORE) &&
             ((iface->getAdminState() == IF_ADMIN_STATE_DOWN) || (iface->getAdminState() == IF_ADMIN_STATE_TESTING) ||
              (iface->getOperState() == IF_OPER_STATE_DOWN) || (iface->getOperState() == IF_OPER_STATE_TESTING)))
         {
            nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Node::checkNetworkPath(%s [%d]): upstream interface %s [%d] on node %s [%d] is down"),
                        m_name, m_id, iface->getName(), iface->getId(), hop->object->getName(), hop->object->getId());
            sendPollerMsg(requestId, POLLER_WARNING _T("   Upstream interface %s on node %s is down\r\n"), iface->getName(), hop->object->getName());
            break;
         }
      }
   }

   delete trace;
   return pathProblemFound;
}

/**
 * Check network path between node and management server to detect possible intermediate node failure
 *
 * @return true if network path problems found
 */
bool Node::checkNetworkPath(UINT32 requestId)
{
   if (checkNetworkPathLayer2(requestId, false))
      return true;

   if (checkNetworkPathLayer3(requestId, false))
      return true;

   nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Node::checkNetworkPath(%s [%d]): will do second pass"), m_name, m_id);

   if (checkNetworkPathLayer2(requestId, true))
      return true;

   if (checkNetworkPathLayer3(requestId, true))
      return true;

   return false;
}

/**
 * Check agent policy binding
 * Intended to be called only from configuration poller
 */
void Node::checkAgentPolicyBinding(AgentConnection *conn)
{
   AgentPolicyInfo *ap;
   UINT32 rcc = conn->getPolicyInventory(&ap);
   if (rcc == ERR_SUCCESS)
   {
      // Check for unbound but installed policies
      for(int i = 0; i < ap->size(); i++)
      {
         uuid guid = ap->getGuid(i);
         bool found = false;
         lockParentList(false);
         for(int i = 0; i < m_parentList->size(); i++)
         {
            if (m_parentList->get(i)->getObjectClass() == OBJECT_TEMPLATE)
            {
                if(((Template*)m_parentList->get(i))->hasPolicy(guid))
                {
                   found = false;
                   break;
                }
            }
         }

         if(!found)
         {
            ServerJob *job = new PolicyUninstallJob(this, ap->getType(i), guid, 0);
            if(!AddJob(job))
               delete job;
         }

         unlockParentList();
      }

      // Check for bound but not installed policies and schedule it's installation again
      //Job will be unbound if it was not possible to add job
      lockParentList(false);
      NetObj **unbindList = (NetObj **)malloc(sizeof(NetObj *) * m_parentList->size());
      int unbindListSize = 0;
      for(int i = 0; i < m_parentList->size(); i++)
      {
         if (m_parentList->get(i)->getObjectClass() == OBJECT_TEMPLATE)
         {
            ((Template *)m_parentList->get(i))->checkPolicyBind(this, ap, unbindList, &unbindListSize);
         }
      }
      unlockParentList();

      for(int i = 0; i < unbindListSize; i++)
      {
         unbindList[i]->deleteChild(this);
         deleteParent(unbindList[i]);
         DbgPrintf(5, _T("ConfPoll(%s): unbound from policy object %s [%d]"), m_name, unbindList[i]->getName(), unbindList[i]->getId());
      }
      MemFree(unbindList);

      m_capabilities |= ap->isNewPolicyType() ? NC_IS_NEW_POLICY_TYPES : 0;
      delete ap;
   }
   else
   {
      DbgPrintf(5, _T("ConfPoll(%s): AgentConnection::getPolicyInventory() failed: rcc=%d"), m_name, rcc);
   }
}

/**
 * Update primary IP address from primary name
 */
void Node::updatePrimaryIpAddr()
{
   if (m_primaryName[0] == 0)
      return;

   InetAddress ipAddr = ResolveHostName(m_zoneUIN, m_primaryName);
   if (!ipAddr.equals(m_ipAddress) && (ipAddr.isValidUnicast() || !_tcscmp(m_primaryName, _T("0.0.0.0"))))
   {
      TCHAR buffer1[64], buffer2[64];

      DbgPrintf(4, _T("IP address for node %s [%d] changed from %s to %s"),
         m_name, (int)m_id, m_ipAddress.toString(buffer1), ipAddr.toString(buffer2));
      PostEvent(EVENT_IP_ADDRESS_CHANGED, m_id, "AA", &ipAddr, &m_ipAddress);

      if (m_flags & NF_REMOTE_AGENT)
      {
         lockProperties();
         m_ipAddress = ipAddr;
         setModified(MODIFY_NODE_PROPERTIES);
         unlockProperties();
      }
      else
      {
         setPrimaryIPAddress(ipAddr);
      }

      agentLock();
      deleteAgentConnection();
      agentUnlock();
   }
}

/**
 * Comparator for package names and versions
 */
static int PackageNameVersionComparator(const SoftwarePackage **p1, const SoftwarePackage **p2)
{
   int rc = _tcscmp((*p1)->getName(), (*p2)->getName());
   if (rc == 0)
      rc = _tcscmp((*p1)->getVersion(), (*p2)->getVersion());
   return rc;
}

/**
 * Comparator for package names
 */
static int PackageNameComparator(const SoftwarePackage **p1, const SoftwarePackage **p2)
{
   return _tcscmp((*p1)->getName(), (*p2)->getName());
}

/**
 * Calculate package changes
 */
static ObjectArray<SoftwarePackage> *CalculatePackageChanges(ObjectArray<SoftwarePackage> *oldSet, ObjectArray<SoftwarePackage> *newSet)
{
   ObjectArray<SoftwarePackage> *changes = new ObjectArray<SoftwarePackage>(32, 32, false);
   for(int i = 0; i < oldSet->size(); i++)
   {
      SoftwarePackage *p = oldSet->get(i);
      SoftwarePackage *np = (SoftwarePackage *)newSet->find(p, PackageNameComparator);
      if (np == NULL)
      {
         p->setChangeCode(CHANGE_REMOVED);
         changes->add(p);
         continue;
      }

      if (!_tcscmp(p->getVersion(), np->getVersion()))
         continue;

      if (newSet->find(p, PackageNameVersionComparator) != NULL)
         continue;

      // multiple versions of same package could be installed
      // (example is gpg-pubkey package on RedHat)
      // if this is the case consider all version changes
      // to be either install or remove
      SoftwarePackage *prev = oldSet->get(i - 1);
      SoftwarePackage *next = oldSet->get(i + 1);
      bool multipleVersions =
               ((prev != NULL) && !_tcscmp(prev->getName(), p->getName())) ||
               ((next != NULL) && !_tcscmp(next->getName(), p->getName()));

      if (multipleVersions)
      {
         p->setChangeCode(CHANGE_REMOVED);
      }
      else
      {
         p->setChangeCode(CHANGE_UPDATED);
         np->setChangeCode(CHANGE_UPDATED);
         changes->add(np);
      }
      changes->add(p);
   }

   // Check for new packages
   for(int i = 0; i < newSet->size(); i++)
   {
      SoftwarePackage *p = newSet->get(i);
      if (p->getChangeCode() == CHANGE_UPDATED)
         continue;   // already marked as upgrade for some existig package
      if (oldSet->find(p, PackageNameVersionComparator) != NULL)
         continue;

      p->setChangeCode(CHANGE_ADDED);
      changes->add(p);
   }

   return changes;
}

static int HardwareSerialComparator(const HardwareComponent **c1, const HardwareComponent **c2)
{
   return _tcscmp((*c1)->getSerial(), (*c2)->getSerial());
}

/**
 * Calculate hardware changes
 */
static ObjectArray<HardwareComponent> *CalculateHardwareChanges(ObjectArray<HardwareComponent> *oldSet, ObjectArray<HardwareComponent> *newSet)
{
   HardwareComponent *nc = NULL, *oc = NULL;
   int i;
   ObjectArray<HardwareComponent> *changes = new ObjectArray<HardwareComponent>(16, 16);

   for(i = 0; i < newSet->size(); i++)
   {
      nc = newSet->get(i);
      oc = oldSet->find(nc, HardwareSerialComparator);
      if (oc == NULL)
      {
         nc->setChangeCode(CHANGE_ADDED);
         changes->add(nc);
      }
   }

   for(i = 0; i < oldSet->size(); i++)
   {
      oc = oldSet->get(i);
      nc = newSet->find(oc, HardwareSerialComparator);
      if (nc == NULL)
      {
         oc->setChangeCode(CHANGE_REMOVED);
         changes->add(oc);
      }
   }

   return changes;
}

/*
 * Update list of hardware components for node
 */
bool Node::updateHardwareComponents(PollerInfo *poller, UINT32 requestId)
{
   static const TCHAR *eventParamNames[] = { _T("type"), _T("serial") };

   poller->setStatus(_T("hardware check"));
   sendPollerMsg(requestId, _T("Reading list of installed hardware components\r\n"));

   Table *tableMem, *tableProc;
   if ((getTableFromAgent(_T("Hardware.MemoryDevices"), &tableMem) != DCE_SUCCESS) ||
       (getTableFromAgent(_T("Hardware.Processors"), &tableProc) != DCE_SUCCESS))
   {
      sendPollerMsg(requestId, POLLER_WARNING _T("Unable to get installed hardware information\r\n"));
      return false;
   }

   ObjectArray<HardwareComponent> *components = new ObjectArray<HardwareComponent>((tableMem->getNumRows() + tableProc->getNumRows()), 16, true);
   for(int i = 0; i < tableMem->getNumRows(); i++)
      components->add(new HardwareComponent(tableMem, i));
   for(int i = 0; i < tableProc->getNumRows(); i++)
      components->add(new HardwareComponent(tableProc, i));
   delete tableMem;
   delete tableProc;

   sendPollerMsg(requestId, POLLER_INFO _T("Received information on %d installed hardware components\r\n"), components->size());

   lockProperties();
   if (m_hardwareComponents != NULL)
   {
      ObjectArray<HardwareComponent> *changes = CalculateHardwareChanges(m_hardwareComponents, components);
      for(int i = 0; i < changes->size(); i++)
      {
         HardwareComponent *c = changes->get(i);
         switch(c->getChangeCode())
         {
            case CHANGE_NONE:
               break;
            case CHANGE_ADDED:
               nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): new hardware of type: %s, serial: %s added"), m_name, c->getType(), c->getSerial());
               sendPollerMsg(requestId, _T("   New hardware component, type: %s, serial: %s\r\n"), c->getType(), c->getSerial());
               PostEventWithNames(EVENT_HARDWARE_ADDED, m_id, "ss", eventParamNames, c->getType(), c->getSerial());
               break;
            case CHANGE_REMOVED:
               nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): hardware of type: %s, serial: %s removed"), m_name, c->getType(), c->getSerial());
               sendPollerMsg(requestId, _T("   Hardware component, type: %s, serial: %s removed\r\n"), c->getType(), c->getSerial());
               PostEventWithNames(EVENT_HARDWARE_ADDED, m_id, "ss", eventParamNames, c->getType(), c->getSerial());
               break;
         }
      }
      delete changes;
      delete m_hardwareComponents;
      setModified(MODIFY_HARDWARE);
   }
   m_hardwareComponents = components;
   unlockProperties();
   return true;
}

/**
 * Update list of software packages for node
 */
bool Node::updateSoftwarePackages(PollerInfo *poller, UINT32 requestId)
{
   static const TCHAR *eventParamNames[] = { _T("name"), _T("version"), _T("previousVersion") };

   if (!(m_capabilities & NC_IS_NATIVE_AGENT))
      return false;

   poller->setStatus(_T("software check"));
   sendPollerMsg(requestId, _T("Reading list of installed software packages\r\n"));

   Table *table;
   if (getTableFromAgent(_T("System.InstalledProducts"), &table) != DCE_SUCCESS)
   {
      sendPollerMsg(requestId, POLLER_WARNING _T("Unable to get information about installed software packages\r\n"));
      return false;
   }

   ObjectArray<SoftwarePackage> *packages = new ObjectArray<SoftwarePackage>(table->getNumRows(), 16, true);
   for(int i = 0; i < table->getNumRows(); i++)
   {
      SoftwarePackage *pkg = SoftwarePackage::createFromTableRow(table, i);
      if (pkg != NULL)
         packages->add(pkg);
   }
   packages->sort(PackageNameVersionComparator);
   delete table;
   sendPollerMsg(requestId, POLLER_INFO _T("Got information about %d installed software packages\r\n"), packages->size());

   lockProperties();
   if (m_softwarePackages != NULL)
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
               sendPollerMsg(requestId, _T("   New package %s version %s\r\n"), p->getName(), p->getVersion());
               PostEventWithNames(EVENT_PACKAGE_INSTALLED, m_id, "ss", eventParamNames, p->getName(), p->getVersion());
               break;
            case CHANGE_REMOVED:
               nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): package %s version %s removed"), m_name, p->getName(), p->getVersion());
               sendPollerMsg(requestId, _T("   Package %s version %s removed\r\n"), p->getName(), p->getVersion());
               PostEventWithNames(EVENT_PACKAGE_REMOVED, m_id, "ss", eventParamNames, p->getName(), p->getVersion());
               break;
            case CHANGE_UPDATED:
               SoftwarePackage *prev = changes->get(++i);   // next entry contains previous package version
               nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): package %s updated (%s -> %s)"), m_name, p->getName(), prev->getVersion(), p->getVersion());
               sendPollerMsg(requestId, _T("   Package %s updated (%s -> %s)\r\n"), p->getName(), prev->getVersion(), p->getVersion());
               PostEventWithNames(EVENT_PACKAGE_UPDATED, m_id, "sss", eventParamNames, p->getName(), p->getVersion(), prev->getVersion());
               break;
         }
      }
      delete changes;
      delete m_softwarePackages;
      setModified(MODIFY_SOFTWARE_INV);
   }
   m_softwarePackages = packages;
   unlockProperties();
   return true;
}

/**
 * Perform configuration poll on node
 */
void Node::configurationPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId)
{
   if (m_runtimeFlags & DCDF_DELETE_IN_PROGRESS)
   {
      if (rqId == 0)
         m_runtimeFlags &= ~DCDF_QUEUED_FOR_CONFIGURATION_POLL;
      return;
   }

   if (IsShutdownInProgress())
      return;

   UINT32 oldCapabilities = m_capabilities;
   TCHAR szBuffer[4096];
   UINT32 modified = 0;

   poller->setStatus(_T("wait for lock"));
   pollerLock();

   if (IsShutdownInProgress())
   {
      pollerUnlock();
      return;
   }

   m_pollRequestor = session;
   sendPollerMsg(rqId, _T("Starting configuration poll for node %s\r\n"), m_name);
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 4, _T("Starting configuration poll for node %s (ID: %d)"), m_name, m_id);

   // Check for forced capabilities recheck
   if (m_runtimeFlags & NDF_RECHECK_CAPABILITIES)
   {
      sendPollerMsg(rqId, POLLER_WARNING _T("Capability reset\r\n"));
      m_capabilities = 0;
      m_runtimeFlags &= ~DCDF_CONFIGURATION_POLL_PASSED;
      m_snmpObjectId[0] = 0;
      m_platformName[0] = 0;
      m_agentVersion[0] = 0;
      MemFreeAndNull(m_sysDescription);
      MemFreeAndNull(m_sysName);
      MemFreeAndNull(m_sysContact);
      MemFreeAndNull(m_sysLocation);
      MemFreeAndNull(m_lldpNodeId);
   }

   // Check if node is marked as unreachable
   if ((m_state & DCSF_UNREACHABLE) && !(m_runtimeFlags & NDF_RECHECK_CAPABILITIES))
   {
      sendPollerMsg(rqId, POLLER_WARNING _T("Node is marked as unreachable, configuration poll aborted\r\n"));
      DbgPrintf(4, _T("Node is marked as unreachable, configuration poll aborted"));
      m_lastConfigurationPoll = time(NULL);
   }
   else
   {
      updatePrimaryIpAddr();

      poller->setStatus(_T("capability check"));
      sendPollerMsg(rqId, _T("Checking node's capabilities...\r\n"));

      if (confPollAgent(rqId))
         modified |= MODIFY_NODE_PROPERTIES;
      if (confPollSnmp(rqId))
         modified |= MODIFY_NODE_PROPERTIES;

      // Check for CheckPoint SNMP agent on port 260
      if (ConfigReadBoolean(_T("EnableCheckPointSNMP"), false))
      {
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): checking for CheckPoint SNMP on port 260"), m_name);
         if (!((m_capabilities & NC_IS_CPSNMP) && (m_state & NSF_CPSNMP_UNREACHABLE)) && m_ipAddress.isValidUnicast())
         {
            SNMP_Transport *pTransport = new SNMP_UDPTransport;
            ((SNMP_UDPTransport *)pTransport)->createUDPTransport(m_ipAddress, CHECKPOINT_SNMP_PORT);
            if (SnmpGet(SNMP_VERSION_1, pTransport,
                        _T(".1.3.6.1.4.1.2620.1.1.10.0"), NULL, 0, szBuffer, sizeof(szBuffer), 0) == SNMP_ERR_SUCCESS)
            {
               lockProperties();
               m_capabilities |= NC_IS_CPSNMP | NC_IS_ROUTER;
               m_state &= ~NSF_CPSNMP_UNREACHABLE;
               unlockProperties();
               sendPollerMsg(rqId, POLLER_INFO _T("   CheckPoint SNMP agent on port 260 is active\r\n"));
            }
            delete pTransport;
         }
      }

      // Generate event if node flags has been changed
      if (oldCapabilities != m_capabilities)
      {
         PostEvent(EVENT_NODE_CAPABILITIES_CHANGED, m_id, "xx", oldCapabilities, m_capabilities);
         modified |= MODIFY_NODE_PROPERTIES;
      }

      // Retrieve interface list
      poller->setStatus(_T("interface check"));
      sendPollerMsg(rqId, _T("Capability check finished\r\n"));

      if (updateInterfaceConfiguration(rqId, 0)) // maskBits
         modified |= MODIFY_NODE_PROPERTIES;

      m_lastConfigurationPoll = time(NULL);

      // Check node name
      sendPollerMsg(rqId, _T("Checking node name\r\n"));
      UINT32 dwAddr = ntohl(_t_inet_addr(m_name));
      if ((g_flags & AF_RESOLVE_NODE_NAMES) &&
          (dwAddr != INADDR_NONE) &&
          (dwAddr != INADDR_ANY) &&
          isMyIP(dwAddr))
      {
         sendPollerMsg(rqId, _T("Node name is an IP address and need to be resolved\r\n"));
         poller->setStatus(_T("resolving name"));
         if (resolveName(FALSE))
         {
            sendPollerMsg(rqId, POLLER_INFO _T("Node name resolved to %s\r\n"), m_name);
            modified |= MODIFY_COMMON_PROPERTIES;
         }
         else
         {
            sendPollerMsg(rqId, POLLER_WARNING _T("Node name cannot be resolved\r\n"));
         }
      }
      else
      {
         if (g_flags & AF_SYNC_NODE_NAMES_WITH_DNS)
         {
            sendPollerMsg(rqId, _T("Syncing node name with DNS\r\n"));
            poller->setStatus(_T("resolving name"));
            if (resolveName(TRUE))
            {
               sendPollerMsg(rqId, POLLER_INFO _T("Node name resolved to %s\r\n"), m_name);
               modified |= MODIFY_COMMON_PROPERTIES;
            }
         }
         else
         {
            sendPollerMsg(rqId, _T("Node name is OK\r\n"));
         }
      }

      updateSoftwarePackages(poller, rqId);
      updateHardwareComponents(poller, rqId);

      applyUserTemplates();
      updateContainerMembership();

      // Call hooks in loaded modules
      for(UINT32 i = 0; i < g_dwNumModules; i++)
      {
         if (g_pModuleList[i].pfConfPollHook != NULL)
         {
            DbgPrintf(5, _T("ConfigurationPoll(%s [%d]): calling hook in module %s"), m_name, m_id, g_pModuleList[i].szName);
            if (g_pModuleList[i].pfConfPollHook(this, session, rqId, poller))
               modified |= MODIFY_ALL;   // FIXME: change module call to get exact modifications
         }
      }

      // Setup permanent connection to agent if not present (needed for proper configuration re-sync)
      if (m_capabilities & NC_IS_NATIVE_AGENT)
      {
         agentLock();
         connectToAgent();
         agentUnlock();
      }

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
         sendPollerMsg(rqId, _T("   Node type changed to %s\r\n"), typeName(type));

         if (*hypervisorType != 0)
            sendPollerMsg(rqId, _T("   Hypervisor type set to %s\r\n"), hypervisorType);
         _tcslcpy(m_hypervisorType, hypervisorType, MAX_HYPERVISOR_TYPE_LENGTH);

         if (*hypervisorInfo != 0)
            sendPollerMsg(rqId, _T("   Hypervisor information set to %s\r\n"), hypervisorInfo);
         MemFree(m_hypervisorInfo);
         m_hypervisorInfo = MemCopyString(hypervisorInfo);
      }
      unlockProperties();

      // Execute hook script
      poller->setStatus(_T("hook"));
      executeHookScript(_T("ConfigurationPoll"));

      sendPollerMsg(rqId, _T("Finished configuration poll for node %s\r\n"), m_name);
      sendPollerMsg(rqId, _T("Node configuration was%schanged after poll\r\n"), (modified != 0) ? _T(" ") : _T(" not "));

      m_runtimeFlags &= ~DCDF_CONFIGURATION_POLL_PENDING;
      m_runtimeFlags |= DCDF_CONFIGURATION_POLL_PASSED;
   }

   // Finish configuration poll
   poller->setStatus(_T("cleanup"));
   if (rqId == 0)
      m_runtimeFlags &= ~DCDF_QUEUED_FOR_CONFIGURATION_POLL;
   m_runtimeFlags &= ~NDF_RECHECK_CAPABILITIES;
   pollerUnlock();
   DbgPrintf(4, _T("Finished configuration poll for node %s (ID: %d)"), m_name, m_id);

   if (modified != 0)
   {
      lockProperties();
      setModified(modified);
      unlockProperties();
   }
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
               m_name, m_id, (m_driver != NULL) ? m_driver->getName() : _T("(not set)"));

      // Assume physical device if it supports SNMP and driver is not "GENERIC" nor "NET-SNMP"
      // FIXME: add driver method to determine node type
      if ((m_driver != NULL) && _tcscmp(m_driver->getName(), _T("GENERIC")) && _tcscmp(m_driver->getName(), _T("NET-SNMP")))
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
   if (m_capabilities & NC_IS_NATIVE_AGENT)
   {
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("Node::detectNodeType(%s [%d]): NetXMS agent node"), m_name, m_id);

      AgentConnection *conn = getAgentConnection();
      if (conn != NULL)
      {
         TCHAR buffer[MAX_RESULT_LENGTH];
         if (conn->getParameter(_T("System.IsVirtual"), MAX_RESULT_LENGTH, buffer) == ERR_SUCCESS)
         {
            VirtualizationType vt = static_cast<VirtualizationType>(_tcstol(buffer, NULL, 10));
            if (vt != VTYPE_NONE)
            {
               type = (vt == VTYPE_FULL) ? NODE_TYPE_VIRTUAL : NODE_TYPE_CONTAINER;
               if (conn->getParameter(_T("Hypervisor.Type"), MAX_RESULT_LENGTH, buffer) == ERR_SUCCESS)
               {
                  _tcslcpy(hypervisorType, buffer, MAX_HYPERVISOR_TYPE_LENGTH);
                  if (conn->getParameter(_T("Hypervisor.Version"), MAX_RESULT_LENGTH, buffer) == ERR_SUCCESS)
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
         conn->decRefCount();
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
      return false;

   bool hasChanges = false;

   sendPollerMsg(rqId, _T("   Checking NetXMS agent...\r\n"));
   AgentTunnel *tunnel = GetTunnelForNode(m_id);
   AgentConnectionEx *pAgentConn;
   if (tunnel != NULL)
   {
      pAgentConn = new AgentConnectionEx(m_id, tunnel, m_agentAuthMethod, m_szSharedSecret, isAgentCompressionAllowed());
      tunnel->decRefCount();
   }
   else
   {
      if (!m_ipAddress.isValidUnicast())
      {
         sendPollerMsg(rqId, POLLER_ERROR _T("   Node primary IP is invalid and there are no active tunnels\r\n"));
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): node primary IP is invalid and there are no active tunnels"), m_name);
         return false;
      }
      pAgentConn = new AgentConnectionEx(m_id, m_ipAddress, m_agentPort, m_agentAuthMethod, m_szSharedSecret, isAgentCompressionAllowed());
      setAgentProxy(pAgentConn);
   }
   pAgentConn->setCommandTimeout(g_agentCommandTimeout);
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): checking for NetXMS agent - connecting"), m_name);

   // Try to connect to agent
   UINT32 rcc;
   if (!pAgentConn->connect(g_pServerKey, &rcc))
   {
      // If there are authentication problem, try default shared secret
      if ((rcc == ERR_AUTH_REQUIRED) || (rcc == ERR_AUTH_FAILED))
      {
         TCHAR secret[MAX_SECRET_LENGTH];
         ConfigReadStr(_T("AgentDefaultSharedSecret"), secret, MAX_SECRET_LENGTH, _T("netxms"));
         DecryptPassword(_T("netxms"), secret, secret, MAX_SECRET_LENGTH);
         pAgentConn->setAuthData(AUTH_SHA1_HASH, secret);
         if (pAgentConn->connect(g_pServerKey, &rcc))
         {
            m_agentAuthMethod = AUTH_SHA1_HASH;
            nx_strncpy(m_szSharedSecret, secret, MAX_SECRET_LENGTH);
            nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): checking for NetXMS agent - shared secret changed to system default"), m_name);
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
         PostEvent(EVENT_AGENT_OK, m_id, NULL);
         sendPollerMsg(rqId, POLLER_INFO _T("   Connectivity with NetXMS agent restored\r\n"));
      }
      else
      {
         sendPollerMsg(rqId, POLLER_INFO _T("   NetXMS native agent is active\r\n"));
      }
      unlockProperties();

      TCHAR buffer[MAX_RESULT_LENGTH];
      if (pAgentConn->getParameter(_T("Agent.Version"), MAX_AGENT_VERSION_LEN, buffer) == ERR_SUCCESS)
      {
         lockProperties();
         if (_tcscmp(m_agentVersion, buffer))
         {
            _tcscpy(m_agentVersion, buffer);
            hasChanges = true;
            sendPollerMsg(rqId, _T("   NetXMS agent version changed to %s\r\n"), m_agentVersion);
         }
         unlockProperties();
      }

      if (pAgentConn->getParameter(_T("Agent.ID"), MAX_RESULT_LENGTH, buffer) == ERR_SUCCESS)
      {
         uuid agentId = uuid::parse(buffer);
         lockProperties();
         if (!m_agentId.equals(agentId))
         {
            PostEvent(EVENT_AGENT_ID_CHANGED, m_id, "GG", &m_agentId, &agentId);
            m_agentId = agentId;
            hasChanges = true;
            sendPollerMsg(rqId, _T("   NetXMS agent ID changed to %s\r\n"), m_agentId.toString(buffer));
         }
         unlockProperties();
      }

      if (pAgentConn->getParameter(_T("System.PlatformName"), MAX_PLATFORM_NAME_LEN, buffer) == ERR_SUCCESS)
      {
         lockProperties();
         if (_tcscmp(m_platformName, buffer))
         {
            _tcscpy(m_platformName, buffer);
            hasChanges = true;
            sendPollerMsg(rqId, _T("   Platform name changed to %s\r\n"), m_platformName);
         }
         unlockProperties();
      }

      // Check IP forwarding status
      if (pAgentConn->getParameter(_T("Net.IP.Forwarding"), 16, buffer) == ERR_SUCCESS)
      {
         if (_tcstoul(buffer, NULL, 10) != 0)
            m_capabilities |= NC_IS_ROUTER;
         else
            m_capabilities &= ~NC_IS_ROUTER;
      }

      // Get uname
      if (pAgentConn->getParameter(_T("System.Uname"), MAX_DB_STRING, buffer) == ERR_SUCCESS)
      {
         TranslateStr(buffer, _T("\r\n"), _T(" "));
         TranslateStr(buffer, _T("\n"), _T(" "));
         TranslateStr(buffer, _T("\r"), _T(" "));
         lockProperties();
         if ((m_sysDescription == NULL) || _tcscmp(m_sysDescription, buffer))
         {
            free(m_sysDescription);
            m_sysDescription = _tcsdup(buffer);
            hasChanges = true;
            sendPollerMsg(rqId, _T("   System description changed to %s\r\n"), m_sysDescription);
         }
         unlockProperties();
      }

      // Check for 64 bit counter support.
      // if Net.Interface.64BitCounters not supported by agent then use
      // only presence of 64 bit parameters as indicator
      bool netIf64bitCounters = true;
      if (pAgentConn->getParameter(_T("Net.Interface.64BitCounters"), MAX_DB_STRING, buffer) == ERR_SUCCESS)
      {
         netIf64bitCounters = _tcstol(buffer, NULL, 10) ? true : false;
      }

      ObjectArray<AgentParameterDefinition> *plist;
      ObjectArray<AgentTableDefinition> *tlist;
      UINT32 rcc = pAgentConn->getSupportedParameters(&plist, &tlist);
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
         DbgPrintf(5, _T("ConfPoll(%s): AgentConnection::getSupportedParameters() failed: rcc=%d"), m_name, rcc);
      }

      // Get supported Windows Performance Counters
      if (!_tcsncmp(m_platformName, _T("windows-"), 8))
      {
         sendPollerMsg(rqId, _T("   Reading list of available Windows Performance Counters...\r\n"));
         ObjectArray<WinPerfObject> *perfObjects = WinPerfObject::getWinPerfObjectsFromNode(this, pAgentConn);
         lockProperties();
         delete m_winPerfObjects;
         m_winPerfObjects = perfObjects;
         if (m_winPerfObjects != NULL)
         {
            sendPollerMsg(rqId, POLLER_INFO _T("   %d counters read\r\n"), m_winPerfObjects->size());
            if (!(m_capabilities & NC_HAS_WINPDH))
            {
               m_capabilities |= NC_HAS_WINPDH;
               hasChanges = true;
            }
         }
         else
         {
            sendPollerMsg(rqId, POLLER_ERROR _T("   unable to get Windows Performance Counters list\r\n"));
            if (m_capabilities & NC_HAS_WINPDH)
            {
               m_capabilities &= ~NC_HAS_WINPDH;
               hasChanges = true;
            }
         }
         unlockProperties();
      }

      checkAgentPolicyBinding(pAgentConn);

      pAgentConn->disconnect();
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): checking for NetXMS agent - failed to connect (error %d)"), m_name, rcc);
   }
   pAgentConn->decRefCount();
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): checking for NetXMS agent - finished"), m_name);
   return hasChanges;
}

/**
 * SNMP walker callback which sets indicator to true after first varbind and aborts walk
 */
static UINT32 IndicatorSnmpWalkerCallback(SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
   (*((bool *)arg)) = true;
   return SNMP_ERR_COMM;
}

/**
 * Configuration poll: check for SNMP
 */
bool Node::confPollSnmp(UINT32 rqId)
{
   if (((m_capabilities & NC_IS_SNMP) && (m_state & NSF_SNMP_UNREACHABLE)) ||
       !m_ipAddress.isValidUnicast() || (m_flags & NF_DISABLE_SNMP))
      return false;

   bool hasChanges = false;

   sendPollerMsg(rqId, _T("   Checking SNMP...\r\n"));
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): calling SnmpCheckCommSettings()"), m_name);
   StringList oids;
   const TCHAR *customOid = m_customAttributes.get(_T("snmp.testoid"));
   if (customOid != NULL)
      oids.add(customOid);
   oids.add(_T(".1.3.6.1.2.1.1.2.0"));
   oids.add(_T(".1.3.6.1.2.1.1.1.0"));
   AddDriverSpecificOids(&oids);

   // Check current SNMP settings first
   SNMP_Transport *pTransport = createSnmpTransport();
   if ((pTransport != NULL) && !SnmpTestRequest(pTransport, &oids))
   {
      delete_and_null(pTransport);
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): node is not responding to SNMP using current settings"), m_name);
      if (m_flags & NF_SNMP_SETTINGS_LOCKED)
      {
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): SNMP settings are locked, skip SnmpCheckCommSettings()"), m_name);
         return false;
      }
   }
   if (pTransport == NULL)
      pTransport = SnmpCheckCommSettings(getEffectiveSnmpProxy(), (getEffectiveSnmpProxy() == m_id) ? InetAddress::LOOPBACK : m_ipAddress, &m_snmpVersion, m_snmpPort, m_snmpSecurity, &oids, m_zoneUIN);
   if (pTransport == NULL)
   {
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
      PostEvent(EVENT_SNMP_OK, m_id, NULL);
      sendPollerMsg(rqId, POLLER_INFO _T("   Connectivity with SNMP agent restored\r\n"));
   }
   unlockProperties();
   sendPollerMsg(rqId, _T("   SNMP agent is active (version %s)\r\n"),
            (m_snmpVersion == SNMP_VERSION_3) ? _T("3") : ((m_snmpVersion == SNMP_VERSION_2C) ? _T("2c") : _T("1")));
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): SNMP agent detected (version %s)"), m_name,
            (m_snmpVersion == SNMP_VERSION_3) ? _T("3") : ((m_snmpVersion == SNMP_VERSION_2C) ? _T("2c") : _T("1")));

   TCHAR szBuffer[4096];
   if (SnmpGet(m_snmpVersion, pTransport, _T(".1.3.6.1.2.1.1.2.0"), NULL, 0, szBuffer, sizeof(szBuffer), SG_STRING_RESULT) != SNMP_ERR_SUCCESS)
   {
      // Set snmp object ID to .0.0 if it cannot be read
      _tcscpy(szBuffer, _T(".0.0"));
   }
   lockProperties();
   if (_tcscmp(m_snmpObjectId, szBuffer))
   {
      _tcslcpy(m_snmpObjectId, szBuffer, MAX_OID_LEN * 4);
      hasChanges = true;
   }
   unlockProperties();

   // Get system description
   if (SnmpGet(m_snmpVersion, pTransport, _T(".1.3.6.1.2.1.1.1.0"), NULL, 0, szBuffer, sizeof(szBuffer), SG_STRING_RESULT) == SNMP_ERR_SUCCESS)
   {
      TranslateStr(szBuffer, _T("\r\n"), _T(" "));
      TranslateStr(szBuffer, _T("\n"), _T(" "));
      TranslateStr(szBuffer, _T("\r"), _T(" "));
      lockProperties();
      if ((m_sysDescription == NULL) || _tcscmp(m_sysDescription, szBuffer))
      {
         free(m_sysDescription);
         m_sysDescription = _tcsdup(szBuffer);
         hasChanges = true;
         sendPollerMsg(rqId, _T("   System description changed to %s\r\n"), m_sysDescription);
      }
      unlockProperties();
   }

   // Select device driver
   NetworkDeviceDriver *driver = FindDriverForNode(this, pTransport);
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): selected device driver %s"), m_name, driver->getName());
   lockProperties();
   if (driver != m_driver)
   {
      m_driver = driver;
      sendPollerMsg(rqId, _T("   New network device driver selected: %s\r\n"), m_driver->getName());
   }
   unlockProperties();

   // Allow driver to gather additional info
   m_driver->analyzeDevice(pTransport, m_snmpObjectId, &m_customAttributes, &m_driverData);
   if (m_driverData != NULL)
   {
      m_driverData->attachToNode(m_id, m_guid, m_name);
   }

   NDD_MODULE_LAYOUT layout;
   m_driver->getModuleLayout(pTransport, &m_customAttributes, m_driverData, 1, &layout); // TODO module set to 1
   if (layout.numberingScheme == NDD_PN_UNKNOWN)
   {
      // Try to find port numbering information in database
      LookupDevicePortLayout(SNMP_ObjectId::parse(m_snmpObjectId), &layout);
   }
   m_portRowCount = layout.rows;
   m_portNumberingScheme = layout.numberingScheme;

   if (m_driver->hasMetrics())
   {
      ObjectArray<AgentParameterDefinition> *metrics = m_driver->getAvailableMetrics(pTransport, &m_customAttributes, m_driverData);
      if (metrics != NULL)
      {
         lockProperties();
         delete m_driverParameters;
         m_driverParameters = metrics;
         unlockProperties();
      }
   }

   // Get sysName, sysContact, sysLocation
   if (querySnmpSysProperty(pTransport, _T(".1.3.6.1.2.1.1.5.0"), _T("name"), rqId, &m_sysName))
      hasChanges = true;
   if (querySnmpSysProperty(pTransport, _T(".1.3.6.1.2.1.1.4.0"), _T("contact"), rqId, &m_sysContact))
      hasChanges = true;
   if (querySnmpSysProperty(pTransport, _T(".1.3.6.1.2.1.1.6.0"), _T("location"), rqId, &m_sysLocation))
      hasChanges = true;

   // Check IP forwarding
   if (checkSNMPIntegerValue(pTransport, _T(".1.3.6.1.2.1.4.1.0"), 1))
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
   if (SnmpGet(m_snmpVersion, pTransport, _T(".1.3.6.1.2.1.47.1.4.1.0"), NULL, 0, szBuffer, sizeof(szBuffer), SG_RAW_RESULT) == SNMP_ERR_SUCCESS)
   {
      lockProperties();
      m_capabilities |= NC_HAS_ENTITY_MIB;
      unlockProperties();

      ComponentTree *components = BuildComponentTree(this, pTransport);
      lockProperties();
      if (m_components != NULL)
         m_components->decRefCount();
      m_components = components;
      unlockProperties();
   }
   else
   {
      lockProperties();
      m_capabilities &= ~NC_HAS_ENTITY_MIB;
      if (m_components != NULL)
      {
         m_components->decRefCount();
         m_components = NULL;
      }
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
   if (checkSNMPIntegerValue(pTransport, _T(".1.3.6.1.4.1.9.9.23.1.3.1.0"), 1))
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
   if (checkSNMPIntegerValue(pTransport, _T(".1.3.6.1.4.1.45.1.6.13.1.2.0"), 1))
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
   if (SnmpGet(m_snmpVersion, pTransport, _T(".1.0.8802.1.1.2.1.3.2.0"), NULL, 0, szBuffer, sizeof(szBuffer), 0) == SNMP_ERR_SUCCESS)
   {
      lockProperties();
      m_capabilities |= NC_IS_LLDP;
      unlockProperties();

      INT32 type;
      BYTE data[256];
      UINT32 dataLen;
      if ((SnmpGetEx(pTransport, _T(".1.0.8802.1.1.2.1.3.1.0"), NULL, 0, &type, sizeof(INT32), 0, NULL) == SNMP_ERR_SUCCESS) &&
          (SnmpGetEx(pTransport, _T(".1.0.8802.1.1.2.1.3.2.0"), NULL, 0, data, 256, SG_RAW_RESULT, &dataLen) == SNMP_ERR_SUCCESS))
      {
         BuildLldpId(type, data, dataLen, szBuffer, 1024);
         lockProperties();
         if ((m_lldpNodeId == NULL) || _tcscmp(m_lldpNodeId, szBuffer))
         {
            free(m_lldpNodeId);
            m_lldpNodeId = _tcsdup(szBuffer);
            hasChanges = true;
            sendPollerMsg(rqId, _T("   LLDP node ID changed to %s\r\n"), m_lldpNodeId);
         }
         unlockProperties();
      }

      ObjectArray<LLDP_LOCAL_PORT_INFO> *lldpPorts = GetLLDPLocalPortInfo(pTransport);
      lockProperties();
      delete m_lldpLocalPortInfo;
      m_lldpLocalPortInfo = lldpPorts;
      unlockProperties();
   }
   else
   {
      lockProperties();
      m_capabilities &= ~NC_IS_LLDP;
      unlockProperties();
   }

   // Check for 802.1x support
   if (checkSNMPIntegerValue(pTransport, _T(".1.0.8802.1.1.1.1.1.1.0"), 1))
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
   if (vrrpInfo != NULL)
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
   if ((m_driver != NULL) && m_driver->isWirelessController(pTransport, &m_customAttributes, m_driverData))
   {
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): node is wireless controller, reading access point information"), m_name);
      sendPollerMsg(rqId, _T("   Reading wireless access point information\r\n"));
      lockProperties();
      m_capabilities |= NC_IS_WIFI_CONTROLLER;
      unlockProperties();

      int clusterMode = m_driver->getClusterMode(pTransport, &m_customAttributes, m_driverData);

      ObjectArray<AccessPointInfo> *aps = m_driver->getAccessPoints(pTransport, &m_customAttributes, m_driverData);
      if (aps != NULL)
      {
         sendPollerMsg(rqId, POLLER_INFO _T("   %d wireless access points found\r\n"), aps->size());
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): got information about %d access points"), m_name, aps->size());
         int adopted = 0;
         for(int i = 0; i < aps->size(); i++)
         {
            AccessPointInfo *info = aps->get(i);
            if (info->getState() == AP_ADOPTED)
               adopted++;

            bool newAp = false;
            AccessPoint *ap = (clusterMode == CLUSTER_MODE_STANDALONE) ? findAccessPointByMAC(info->getMacAddr()) : FindAccessPointByMAC(info->getMacAddr());
            if (ap == NULL)
            {
               String name;

               if (info->getName() != NULL)
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
               ap = new AccessPoint((const TCHAR *)name, info->getIndex(), info->getMacAddr());
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

         lockProperties();
         m_adoptedApCount = adopted;
         m_totalApCount = aps->size();
         unlockProperties();

         delete aps;
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): failed to read access point information"), m_name);
         sendPollerMsg(rqId, POLLER_ERROR _T("   Failed to read access point information\r\n"));
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
      if (SnmpGet(SNMP_VERSION_1, pTransport, _T(".1.3.6.1.4.1.2620.1.1.10.0"), NULL, 0, szBuffer, sizeof(szBuffer), 0) == SNMP_ERR_SUCCESS)
      {
         lockProperties();
         if (_tcscmp(m_snmpObjectId, _T(".1.3.6.1.4.1.2620.1.1")))
         {
            _tcslcpy(m_snmpObjectId, _T(".1.3.6.1.4.1.2620.1.1"), MAX_OID_LEN * 4);
            hasChanges = true;
         }

         m_capabilities |= NC_IS_SNMP | NC_IS_ROUTER;
         m_state &= ~NSF_SNMP_UNREACHABLE;
         unlockProperties();
         sendPollerMsg(rqId, POLLER_INFO _T("   CheckPoint SNMP agent on port 161 is active\r\n"));
      }
   }
   delete pTransport;
   return hasChanges;
}

/**
 * Query SNMP sys property (sysName, sysLocation, etc.)
 */
bool Node::querySnmpSysProperty(SNMP_Transport *snmp, const TCHAR *oid, const TCHAR *propName, UINT32 pollRqId, TCHAR **value)
{
   TCHAR buffer[256];
   bool hasChanges = false;

   if (SnmpGet(m_snmpVersion, snmp, oid, NULL, 0, buffer, sizeof(buffer), SG_STRING_RESULT) == SNMP_ERR_SUCCESS)
   {
      lockProperties();
      if ((*value == NULL) || _tcscmp(*value, buffer))
      {
         MemFree(*value);
         *value = _tcsdup(buffer);
         hasChanges = true;
         sendPollerMsg(pollRqId, _T("   System %s changed to %s\r\n"), propName, *value);
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
   TCHAR szBuffer[4096];
   if (SnmpGet(m_snmpVersion, pTransport, _T(".1.3.6.1.2.1.17.1.1.0"), NULL, 0, szBuffer, sizeof(szBuffer), SG_RAW_RESULT) == SNMP_ERR_SUCCESS)
   {
      lockProperties();
      m_capabilities |= NC_IS_BRIDGE;
      memcpy(m_baseBridgeAddress, szBuffer, 6);
      unlockProperties();

      // Check for Spanning Tree (IEEE 802.1d) MIB support
      if (checkSNMPIntegerValue(pTransport, _T(".1.3.6.1.2.1.17.2.1.0"), 3))
      {
         lockProperties();
         m_capabilities |= NC_IS_STP;
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
 * Delete duplicate interfaces
 * (find and delete multiple interfaces with same ifIndex value created by version prior to 2.0-M3)
 */
bool Node::deleteDuplicateInterfaces(UINT32 rqid)
{
   ObjectArray<Interface> deleteList(16, 16, false);

   lockChildList(false);
   for(int i = 0; i < m_childList->size(); i++)
   {
      NetObj *curr = m_childList->get(i);

      if ((curr->getObjectClass() != OBJECT_INTERFACE) ||
          ((Interface *)curr)->isManuallyCreated())
         continue;
      Interface *iface = (Interface *)curr;
      for(int j = i + 1; j < m_childList->size(); j++)
      {
         NetObj *next = m_childList->get(j);

         if ((next->getObjectClass() != OBJECT_INTERFACE) ||
             ((Interface *)next)->isManuallyCreated() ||
             (deleteList.contains((Interface *)next)))
            continue;
         if (iface->getIfIndex() == ((Interface *)next)->getIfIndex())
         {
            deleteList.add((Interface *)next);
            DbgPrintf(6, _T("Node::deleteDuplicateInterfaces(%s [%d]): found duplicate interface %s [%d], original %s [%d], ifIndex=%d"),
               m_name, m_id, next->getName(), next->getId(), iface->getName(), iface->getId(), iface->getIfIndex());
         }
      }
   }
   unlockChildList();

   for(int i = 0; i < deleteList.size(); i++)
   {
      Interface *iface = deleteList.get(i);
      sendPollerMsg(rqid, POLLER_WARNING _T("   Duplicate interface \"%s\" deleted\r\n"), iface->getName());
      deleteInterface(iface);
   }

   return deleteList.size() > 0;
}

/**
 * Update interface configuration
 */
bool Node::updateInterfaceConfiguration(UINT32 rqid, int maskBits)
{
   sendPollerMsg(rqid, _T("Checking interface configuration...\r\n"));

   bool hasChanges = deleteDuplicateInterfaces(rqid);

   InterfaceList *ifList = getInterfaceList();
   if (ifList != NULL)
   {
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("Node::updateInterfaceConfiguration(%s [%u]): got %d interfaces"), m_name, m_id, ifList->size());

      // Find non-existing interfaces
      lockChildList(false);
      ObjectArray<Interface> deleteList(m_childList->size(), 8, false);
      for(int i = 0; i < m_childList->size(); i++)
      {
         if (m_childList->get(i)->getObjectClass() == OBJECT_INTERFACE)
         {
            Interface *iface = (Interface *)m_childList->get(i);
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
            sendPollerMsg(rqid, POLLER_WARNING _T("   Interface \"%s\" is no longer exist\r\n"), iface->getName());
            const InetAddress& addr = iface->getFirstIpAddress();
            PostEvent(EVENT_INTERFACE_DELETED, m_id, "dsAd", iface->getIfIndex(), iface->getName(), &addr, addr.getMaskBits());
            deleteInterface(iface);
         }
         hasChanges = true;
      }

      // Add new interfaces and check configuration of existing
      for(int j = 0; j < ifList->size(); j++)
      {
         InterfaceInfo *ifInfo = ifList->get(j);
         bool isNewInterface = true;

         lockChildList(false);
         for(int i = 0; i < m_childList->size(); i++)
         {
            if (m_childList->get(i)->getObjectClass() == OBJECT_INTERFACE)
            {
               Interface *pInterface = (Interface *)m_childList->get(i);

               if (ifInfo->index == pInterface->getIfIndex())
               {
                  // Existing interface, check configuration
                  if (memcmp(ifInfo->macAddr, "\x00\x00\x00\x00\x00\x00", MAC_ADDR_LENGTH) &&
                      memcmp(ifInfo->macAddr, pInterface->getMacAddr(), MAC_ADDR_LENGTH))
                  {
                     TCHAR szOldMac[16], szNewMac[16];

                     BinToStr((BYTE *)pInterface->getMacAddr(), MAC_ADDR_LENGTH, szOldMac);
                     BinToStr(ifInfo->macAddr, MAC_ADDR_LENGTH, szNewMac);
                     PostEvent(EVENT_MAC_ADDR_CHANGED, m_id, "idsss",
                               pInterface->getId(), pInterface->getIfIndex(),
                               pInterface->getName(), szOldMac, szNewMac);
                     pInterface->setMacAddr(ifInfo->macAddr, true);
                  }
                  if (_tcscmp(ifInfo->name, pInterface->getName()))
                  {
                     pInterface->setName(ifInfo->name);
                  }
                  if (_tcscmp(ifInfo->description, pInterface->getDescription()))
                  {
                     pInterface->setDescription(ifInfo->description);
                  }
                  if (_tcscmp(ifInfo->alias, pInterface->getAlias()))
                  {
                     pInterface->setAlias(ifInfo->alias);
                  }
                  if (ifInfo->bridgePort != pInterface->getBridgePortNumber())
                  {
                     pInterface->setBridgePortNumber(ifInfo->bridgePort);
                  }
                  if (ifInfo->slot != pInterface->getSlotNumber())
                  {
                     pInterface->setSlotNumber(ifInfo->slot);
                  }
                  if (ifInfo->port != pInterface->getPortNumber())
                  {
                     pInterface->setPortNumber(ifInfo->port);
                  }
                  if (ifInfo->isPhysicalPort != pInterface->isPhysicalPort())
                  {
                     pInterface->setPhysicalPortFlag(ifInfo->isPhysicalPort);
                  }
                  if (ifInfo->mtu != pInterface->getMTU())
                  {
                     pInterface->setMTU(ifInfo->mtu);
                  }
                  if (ifInfo->speed != pInterface->getSpeed())
                  {
                     pInterface->setSpeed(ifInfo->speed);
                  }
                  if ((ifInfo->ifTableSuffixLength != pInterface->getIfTableSuffixLen()) ||
                      memcmp(ifInfo->ifTableSuffix, pInterface->getIfTableSuffix(),
                         std::min(ifInfo->ifTableSuffixLength, pInterface->getIfTableSuffixLen())))
                  {
                     pInterface->setIfTableSuffix(ifInfo->ifTableSuffixLength, ifInfo->ifTableSuffix);
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
                           PostEvent(EVENT_IF_MASK_CHANGED, m_id, "dsAddd", pInterface->getId(), pInterface->getName(),
                                     &addr, addr.getMaskBits(), pInterface->getIfIndex(), ifAddr.getMaskBits());
                           pInterface->setNetMask(addr);
                           sendPollerMsg(rqid, POLLER_INFO _T("   IP network mask changed to /%d on interface \"%s\" address %s\r\n"),
                              addr.getMaskBits(), pInterface->getName(), (const TCHAR *)ifAddr.toString());
                        }
                     }
                     else
                     {
                        sendPollerMsg(rqid, POLLER_WARNING _T("   IP address %s removed from interface \"%s\"\r\n"),
                           (const TCHAR *)ifAddr.toString(), pInterface->getName());
                        PostEvent(EVENT_IF_IPADDR_DELETED, m_id, "dsAdd", pInterface->getId(), pInterface->getName(),
                                  &ifAddr, ifAddr.getMaskBits(), pInterface->getIfIndex());
                        pInterface->deleteIpAddress(ifAddr);
                     }
                  }

                  // Check for added IPs
                  for(int m = 0; m < ifInfo->ipAddrList.size(); m++)
                  {
                     const InetAddress& addr = ifInfo->ipAddrList.get(m);
                     if (!ifList->hasAddress(addr))
                     {
                        pInterface->addIpAddress(addr);
                        PostEvent(EVENT_IF_IPADDR_ADDED, m_id, "dsAdd", pInterface->getId(), pInterface->getName(),
                                  &addr, addr.getMaskBits(), pInterface->getIfIndex());
                        sendPollerMsg(rqid, POLLER_INFO _T("   IP address %s added to interface \"%s\"\r\n"),
                           (const TCHAR *)addr.toString(), pInterface->getName());
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
            sendPollerMsg(rqid, POLLER_INFO _T("   Found new interface \"%s\"\r\n"), ifInfo->name);
            if (filterInterface(ifInfo))
            {
               createNewInterface(ifInfo, false, false);
               hasChanges = true;
            }
            else
            {
               sendPollerMsg(rqid, POLLER_WARNING _T("   Creation of interface object \"%s\" blocked by filter\r\n"), ifInfo->name);
            }
         }
      }

      // Set parent interfaces
      for(int j = 0; j < ifList->size(); j++)
      {
         InterfaceInfo *ifInfo = ifList->get(j);
         if (ifInfo->parentIndex != 0)
         {
            Interface *parent = findInterfaceByIndex(ifInfo->parentIndex);
            if (parent != NULL)
            {
               Interface *iface = findInterfaceByIndex(ifInfo->index);
               if (iface != NULL)
               {
                  iface->setParentInterface(parent->getId());
                  nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("Node::updateInterfaceConfiguration(%s [%u]): set sub-interface: %s [%d] -> %s [%d]"),
                           m_name, m_id, parent->getName(), parent->getId(), iface->getName(), iface->getId());
               }
            }
         }
         else
         {
            Interface *iface = findInterfaceByIndex(ifInfo->index);
            if ((iface != NULL) && (iface->getParentInterfaceId() != 0))
            {
               iface->setParentInterface(0);
               nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("Node::updateInterfaceConfiguration(%s [%u]): removed reference to parent interface from %s [%d]"),
                        m_name, m_id, iface->getName(), iface->getId());
            }
         }
      }
   }
   else if (!(m_flags & NF_REMOTE_AGENT))    /* pIfList == NULL */
   {
      Interface *pInterface;
      UINT32 dwCount;

      sendPollerMsg(rqid, POLLER_ERROR _T("Unable to get interface list from node\r\n"));
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("Node::updateInterfaceConfiguration(%s [%u]): Unable to get interface list from node"), m_name, m_id);

      // Delete all existing interfaces in case of forced capability recheck
      if (m_runtimeFlags & NDF_RECHECK_CAPABILITIES)
      {
         lockChildList(false);
         Interface **ppDeleteList = (Interface **)malloc(sizeof(Interface *) * m_childList->size());
         int delCount = 0;
         for(int i = 0; i < m_childList->size(); i++)
         {
            NetObj *curr = m_childList->get(i);
            if ((curr->getObjectClass() == OBJECT_INTERFACE) && !((Interface *)curr)->isManuallyCreated())
               ppDeleteList[delCount++] = (Interface *)curr;
         }
         unlockChildList();
         for(int j = 0; j < delCount; j++)
         {
            sendPollerMsg(rqid, POLLER_WARNING _T("   Interface \"%s\" is no longer exist\r\n"),
                          ppDeleteList[j]->getName());
            const InetAddress& addr = ppDeleteList[j]->getIpAddressList()->getFirstUnicastAddress();
            PostEvent(EVENT_INTERFACE_DELETED, m_id, "dsAd", ppDeleteList[j]->getIfIndex(),
                      ppDeleteList[j]->getName(), &addr, addr.getMaskBits());
            deleteInterface(ppDeleteList[j]);
         }
         free(ppDeleteList);
      }

      // Check if we have pseudo-interface object
      BYTE macAddr[MAC_ADDR_LENGTH];
      BYTE *pMacAddr;
      dwCount = getInterfaceCount(&pInterface);
      if (dwCount == 1)
      {
         if (pInterface->isFake())
         {
            // Check if primary IP is different from interface's IP
            if (!pInterface->getIpAddressList()->hasAddress(m_ipAddress))
            {
               deleteInterface(pInterface);
               if (m_ipAddress.isValidUnicast())
               {
                  memset(macAddr, 0, MAC_ADDR_LENGTH);
                  Subnet *pSubnet = FindSubnetForNode(m_zoneUIN, m_ipAddress);
                  if (pSubnet != NULL)
                     pSubnet->findMacAddress(m_ipAddress, macAddr);
                  pMacAddr = !memcmp(macAddr, "\x00\x00\x00\x00\x00\x00", MAC_ADDR_LENGTH) ? NULL : macAddr;
                  TCHAR szMac[20];
                  MACToStr(macAddr, szMac);
                  nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("Node::updateInterfaceConfiguration(%s [%u]): got MAC for unknown interface: %s"), m_name, m_id, szMac);
                  InetAddress ifaceAddr = m_ipAddress;
                  ifaceAddr.setMaskBits(maskBits);
                  createNewInterface(ifaceAddr, pMacAddr, true);
               }
            }
            else
            {
               // check MAC address
               memset(macAddr, 0, MAC_ADDR_LENGTH);
               Subnet *pSubnet = FindSubnetForNode(m_zoneUIN, m_ipAddress);
               if (pSubnet != NULL)
               {
                  nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("Node::updateInterfaceConfiguration(%s [%u]): found subnet %s [%u]"),
                            m_name, m_id, pSubnet->getName(), pSubnet->getId());
                  pSubnet->findMacAddress(m_ipAddress, macAddr);
               }
               if (memcmp(macAddr, "\x00\x00\x00\x00\x00\x00", MAC_ADDR_LENGTH) && memcmp(macAddr, pInterface->getMacAddr(), MAC_ADDR_LENGTH))
               {
                  TCHAR szOldMac[16], szNewMac[16];

                  BinToStr((BYTE *)pInterface->getMacAddr(), MAC_ADDR_LENGTH, szOldMac);
                  BinToStr(macAddr, MAC_ADDR_LENGTH, szNewMac);
                  nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("Node::updateInterfaceConfiguration(%s [%u]): MAC change for unknown interface: %s to %s"),
                            m_name, m_id, szOldMac, szNewMac);
                  PostEvent(EVENT_MAC_ADDR_CHANGED, m_id, "idsss",
                            pInterface->getId(), pInterface->getIfIndex(),
                            pInterface->getName(), szOldMac, szNewMac);
                  pInterface->setMacAddr(macAddr, true);
               }
            }
         }
      }
      else if (dwCount == 0)
      {
         // No interfaces at all, create pseudo-interface
         if (m_ipAddress.isValidUnicast())
         {
            memset(macAddr, 0, MAC_ADDR_LENGTH);
            Subnet *pSubnet = FindSubnetForNode(m_zoneUIN, m_ipAddress);
            if (pSubnet != NULL)
            {
               nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("Node::updateInterfaceConfiguration(%s [%u]): found subnet %s [%u]"),
                         m_name, m_id, pSubnet->getName(), pSubnet->getId());
               pSubnet->findMacAddress(m_ipAddress, macAddr);
            }
            pMacAddr = !memcmp(macAddr, "\x00\x00\x00\x00\x00\x00", MAC_ADDR_LENGTH) ? NULL : macAddr;
            TCHAR szMac[20];
            MACToStr(macAddr, szMac);
            nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("Node::updateInterfaceConfiguration(%s [%u]): got MAC for unknown interface: %s"), m_name, m_id, szMac);
            InetAddress ifaceAddr = m_ipAddress;
            ifaceAddr.setMaskBits(maskBits);
            createNewInterface(ifaceAddr, pMacAddr, true);
         }
      }
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("Node::updateInterfaceConfiguration(%s [%u]): pIfList == NULL, dwCount = %u"), m_name, m_id, dwCount);
   }

   delete ifList;
   checkSubnetBinding();

   sendPollerMsg(rqid, _T("Interface configuration check finished\r\n"));
   return hasChanges;
}

/**
 * Get instances for instance discovery DCO
 */
StringMap *Node::getInstanceList(DCObject *dco)
{
   if (dco->getInstanceDiscoveryData() == NULL)
      return NULL;

   Node *node;
   if (dco->getSourceNode() != 0)
   {
      node = (Node *)FindObjectById(dco->getSourceNode(), OBJECT_NODE);
      if (node == NULL)
      {
         DbgPrintf(6, _T("Node::getInstanceList(%s [%d]): source node [%d] not found"), dco->getName(), dco->getId(), dco->getSourceNode());
         return NULL;
      }
      if (!node->isTrustedNode(m_id))
      {
         DbgPrintf(6, _T("Node::getInstanceList(%s [%d]): this node (%s [%d]) is not trusted by source node %s [%d]"),
                  dco->getName(), dco->getId(), m_name, m_id, node->getName(), node->getId());
         return NULL;
      }
   }
   else
   {
      node = this;
   }

   StringList *instances = NULL;
   StringMap *instanceMap = NULL;
   switch(dco->getInstanceDiscoveryMethod())
   {
      case IDM_AGENT_LIST:
         node->getListFromAgent(dco->getInstanceDiscoveryData(), &instances);
         break;
      case IDM_SCRIPT:
         node->getStringMapFromScript(dco->getInstanceDiscoveryData(), &instanceMap, this);
         break;
      case IDM_SNMP_WALK_VALUES:
         node->getListFromSNMP(dco->getSnmpPort(), dco->getInstanceDiscoveryData(), &instances);
         break;
      case IDM_SNMP_WALK_OIDS:
         node->getOIDSuffixListFromSNMP(dco->getSnmpPort(), dco->getInstanceDiscoveryData(), &instanceMap);
         break;
      default:
         instances = NULL;
         break;
   }
   if ((instances == NULL) && (instanceMap == NULL))
      return NULL;

   if (instanceMap == NULL)
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
   if (m_smclpConnection == NULL)
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
   if ((getCustomAttribute(_T("iLO.login"), login, 64) != NULL) &&
       (getCustomAttribute(_T("iLO.password"), password, 64) != NULL))
      return m_smclpConnection->connect(login, password);
   return false;
}

/**
 * Connect to native agent. Assumes that access to agent connection is already locked.
 */
bool Node::connectToAgent(UINT32 *error, UINT32 *socketError, bool *newConnection, bool forceConnect)
{
   if (g_flags & AF_SHUTDOWN)
      return false;

   if (!forceConnect && (m_agentConnection == NULL) && (time(NULL) - m_lastAgentConnectAttempt < 30))
   {
      nxlog_debug_tag(DEBUG_TAG_AGENT, 7, _T("Node::connectToAgent(%s [%d]): agent is unreachable, will not retry connection"), m_name, m_id);
      if (error != NULL)
         *error = ERR_CONNECT_FAILED;
      if (socketError != NULL)
         *socketError = 0;
      return false;
   }

   // Check if tunnel is available
   AgentTunnel *tunnel = GetTunnelForNode(m_id);
   if ((tunnel == NULL) && (!m_ipAddress.isValidUnicast() || (m_flags & NF_AGENT_OVER_TUNNEL_ONLY)))
   {
      nxlog_debug_tag(DEBUG_TAG_AGENT, 7, _T("Node::connectToAgent(%s [%d]): %s and there are no active tunnels"), m_name, m_id,
               (m_flags & NF_AGENT_OVER_TUNNEL_ONLY) ? _T("direct agent connections are disabled") : _T("node primary IP is invalid"));
      return false;
   }

   // Create new agent connection object if needed
   if (m_agentConnection == NULL)
   {
      m_agentConnection = (tunnel != NULL) ?
               new AgentConnectionEx(m_id, tunnel, m_agentAuthMethod, m_szSharedSecret, isAgentCompressionAllowed()) :
               new AgentConnectionEx(m_id, m_ipAddress, m_agentPort, m_agentAuthMethod, m_szSharedSecret, isAgentCompressionAllowed());
      nxlog_debug_tag(DEBUG_TAG_AGENT, 7, _T("Node::connectToAgent(%s [%d]): new agent connection created"), m_name, m_id);
   }
   else
   {
      // Check if we already connected
      if (m_agentConnection->nop() == ERR_SUCCESS)
      {
         DbgPrintf(7, _T("Node::connectToAgent(%s [%d]): already connected"), m_name, m_id);
         if (newConnection != NULL)
            *newConnection = false;
         setLastAgentCommTime();
         if (tunnel != NULL)
            tunnel->decRefCount();
         return true;
      }

      // Close current connection or clean up after broken connection
      m_agentConnection->disconnect();
      m_agentConnection->setTunnel(tunnel);
      nxlog_debug_tag(DEBUG_TAG_AGENT, 7, _T("Node::connectToAgent(%s [%d]): existing connection reset"), m_name, m_id);
   }
   if (newConnection != NULL)
      *newConnection = true;
   m_agentConnection->setPort(m_agentPort);
   m_agentConnection->setAuthData(m_agentAuthMethod, m_szSharedSecret);
   if (tunnel == NULL)
      setAgentProxy(m_agentConnection);
   m_agentConnection->setCommandTimeout(g_agentCommandTimeout);
   DbgPrintf(7, _T("Node::connectToAgent(%s [%d]): calling connect on port %d"), m_name, m_id, (int)m_agentPort);
   bool success = m_agentConnection->connect(g_pServerKey, error, socketError, g_serverId);
   if (success)
   {
      UINT32 rcc = m_agentConnection->setServerId(g_serverId);
      if (rcc == ERR_SUCCESS)
      {
         syncDataCollectionWithAgent(m_agentConnection);
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
      setFileUpdateConnection(NULL);
      setLastAgentCommTime();
      CALL_ALL_MODULES(pfOnConnectToAgent, (this, m_agentConnection));
   }
   else
   {
      deleteAgentConnection();
      m_lastAgentConnectAttempt = time(NULL);
   }
   if (tunnel != NULL)
      tunnel->decRefCount();
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
 * Get DCI value via SNMP
 */
DataCollectionError Node::getItemFromSNMP(WORD port, const TCHAR *param, size_t bufSize, TCHAR *buffer, int interpretRawValue)
{
   UINT32 dwResult;

   if ((((m_state & NSF_SNMP_UNREACHABLE) || !(m_capabilities & NC_IS_SNMP)) && (port == 0)) ||
       (m_state & DCSF_UNREACHABLE) ||
       (m_flags & NF_DISABLE_SNMP))
   {
      dwResult = SNMP_ERR_COMM;
   }
   else
   {
      SNMP_Transport *pTransport;

      pTransport = createSnmpTransport(port);
      if (pTransport != NULL)
      {
         if (interpretRawValue == SNMP_RAWTYPE_NONE)
         {
            dwResult = SnmpGet(m_snmpVersion, pTransport, param, NULL, 0, buffer, bufSize * sizeof(TCHAR), SG_PSTRING_RESULT);
         }
         else
         {
            BYTE rawValue[1024];
            memset(rawValue, 0, 1024);
            dwResult = SnmpGet(m_snmpVersion, pTransport, param, NULL, 0, rawValue, 1024, SG_RAW_RESULT);
            if (dwResult == SNMP_ERR_SUCCESS)
            {
               switch(interpretRawValue)
               {
                  case SNMP_RAWTYPE_INT32:
                     _sntprintf(buffer, bufSize, _T("%d"), ntohl(*((LONG *)rawValue)));
                     break;
                  case SNMP_RAWTYPE_UINT32:
                     _sntprintf(buffer, bufSize, _T("%u"), ntohl(*((UINT32 *)rawValue)));
                     break;
                  case SNMP_RAWTYPE_INT64:
                     _sntprintf(buffer, bufSize, INT64_FMT, (INT64)ntohq(*((INT64 *)rawValue)));
                     break;
                  case SNMP_RAWTYPE_UINT64:
                     _sntprintf(buffer, bufSize, UINT64_FMT, ntohq(*((QWORD *)rawValue)));
                     break;
                  case SNMP_RAWTYPE_DOUBLE:
                     _sntprintf(buffer, bufSize, _T("%f"), ntohd(*((double *)rawValue)));
                     break;
                  case SNMP_RAWTYPE_IP_ADDR:
                     IpToStr(ntohl(*((UINT32 *)rawValue)), buffer);
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
         delete pTransport;
      }
      else
      {
         dwResult = SNMP_ERR_COMM;
      }
   }
   DbgPrintf(7, _T("Node(%s)->GetItemFromSNMP(%s): dwResult=%d"), m_name, param, dwResult);
   return DCErrorFromSNMPError(dwResult);
}

/**
 * Read one row for SNMP table
 */
static UINT32 ReadSNMPTableRow(SNMP_Transport *snmp, const SNMP_ObjectId *rowOid, size_t baseOidLen, UINT32 index,
                               ObjectArray<DCTableColumn> *columns, Table *table)
{
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   for(int i = 0; i < columns->size(); i++)
   {
      DCTableColumn *c = columns->get(i);
      if (c->getSnmpOid() != NULL)
      {
         UINT32 oid[MAX_OID_LEN];
         size_t oidLen = c->getSnmpOid()->length();
         memcpy(oid, c->getSnmpOid()->value(), oidLen * sizeof(UINT32));
         if (rowOid != NULL)
         {
            size_t suffixLen = rowOid->length() - baseOidLen;
            memcpy(&oid[oidLen], rowOid->value() + baseOidLen, suffixLen * sizeof(UINT32));
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
   UINT32 rc = snmp->doRequest(&request, &response, SnmpGetDefaultTimeout(), 3);
   if (rc == SNMP_ERR_SUCCESS)
   {
      if (((int)response->getNumVariables() >= columns->size()) &&
          (response->getErrorCode() == SNMP_PDU_ERR_SUCCESS))
      {
         table->addRow();
         for(int i = 0; i < response->getNumVariables(); i++)
         {
            SNMP_Variable *v = response->getVariable(i);
            if ((v != NULL) && (v->getType() != ASN_NO_SUCH_OBJECT) && (v->getType() != ASN_NO_SUCH_INSTANCE))
            {
               DCTableColumn *c = columns->get(i);
               if ((c != NULL) && c->isConvertSnmpStringToHex())
               {
                  size_t size = v->getValueLength();
                  TCHAR *buffer = (TCHAR *)malloc((size * 2 + 1) * sizeof(TCHAR));
                  BinToStr(v->getValue(), size, buffer);
                  table->set(i, buffer);
               }
               else
               {
                  bool convert = false;
                  TCHAR buffer[256];
                  table->set(i, v->getValueAsPrintableString(buffer, 256, &convert));
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
DataCollectionError Node::getTableFromSNMP(WORD port, const TCHAR *oid, ObjectArray<DCTableColumn> *columns, Table **table)
{
   *table = NULL;

   SNMP_Transport *snmp = createSnmpTransport(port);
   if (snmp == NULL)
      return DCE_COMM_ERROR;

   ObjectArray<SNMP_ObjectId> oidList(64, 64, true);
   UINT32 rc = SnmpWalk(snmp, oid, SNMPGetTableCallback, &oidList, FALSE);
   if (rc == SNMP_ERR_SUCCESS)
   {
      *table = new Table;
      for(int i = 0; i < columns->size(); i++)
      {
         DCTableColumn *c = columns->get(i);
         if (c->getSnmpOid() != NULL)
            (*table)->addColumn(c->getName(), c->getDataType(), c->getDisplayName(), c->isInstanceColumn());
      }

      size_t baseOidLen = SNMPGetOIDLength(oid);
      for(int i = 0; i < oidList.size(); i++)
      {
         rc = ReadSNMPTableRow(snmp, oidList.get(i), baseOidLen, 0, columns, *table);
         if (rc != SNMP_ERR_SUCCESS)
         {
            delete_and_null(*table);
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
static UINT32 SNMPGetListCallback(SNMP_Variable *varbind, SNMP_Transport *snmp, void *arg)
{
   bool convert = false;
   TCHAR buffer[256];
   ((StringList *)arg)->add(varbind->getValueAsPrintableString(buffer, 256, &convert));
   return SNMP_ERR_SUCCESS;
}

/**
 * Get list of values from SNMP
 */
DataCollectionError Node::getListFromSNMP(WORD port, const TCHAR *oid, StringList **list)
{
   *list = NULL;
   SNMP_Transport *snmp = createSnmpTransport(port);
   if (snmp == NULL)
      return DCE_COMM_ERROR;

   *list = new StringList;
   UINT32 rc = SnmpWalk(snmp, oid, SNMPGetListCallback, *list);
   delete snmp;
   if (rc != SNMP_ERR_SUCCESS)
   {
      delete *list;
      *list = NULL;
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
static UINT32 SNMPOIDSuffixListCallback(SNMP_Variable *varbind, SNMP_Transport *snmp, void *arg)
{
   SNMPOIDSuffixListCallback_Data *data = (SNMPOIDSuffixListCallback_Data *)arg;
   const SNMP_ObjectId& oid = varbind->getName();
   if (oid.length() <= data->oidLen)
      return SNMP_ERR_SUCCESS;
   TCHAR buffer[256];
   SNMPConvertOIDToText(oid.length() - data->oidLen, &(oid.value()[data->oidLen]), buffer, 256);

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
DataCollectionError Node::getOIDSuffixListFromSNMP(WORD port, const TCHAR *oid, StringMap **values)
{
   *values = NULL;
   SNMP_Transport *snmp = createSnmpTransport(port);
   if (snmp == NULL)
      return DCE_COMM_ERROR;

   SNMPOIDSuffixListCallback_Data data;
   UINT32 oidBin[256];
   data.oidLen = SNMPParseOID(oid, oidBin, 256);
   if (data.oidLen == 0)
   {
      delete snmp;
      return DCE_NOT_SUPPORTED;
   }

   data.values = new StringMap;
   UINT32 rc = SnmpWalk(snmp, oid, SNMPOIDSuffixListCallback, &data);
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
 * Get item's value via SNMP from CheckPoint's agent
 */
DataCollectionError Node::getItemFromCheckPointSNMP(const TCHAR *szParam, UINT32 dwBufSize, TCHAR *szBuffer)
{
   UINT32 dwResult;

   if ((m_state & NSF_CPSNMP_UNREACHABLE) ||
       (m_state & DCSF_UNREACHABLE))
   {
      dwResult = SNMP_ERR_COMM;
   }
   else
   {
      SNMP_Transport *pTransport;

      pTransport = new SNMP_UDPTransport;
      ((SNMP_UDPTransport *)pTransport)->createUDPTransport(m_ipAddress, CHECKPOINT_SNMP_PORT);
      dwResult = SnmpGet(SNMP_VERSION_1, pTransport, szParam, NULL, 0, szBuffer, dwBufSize * sizeof(TCHAR), SG_STRING_RESULT);
      delete pTransport;
   }
   DbgPrintf(7, _T("Node(%s)->GetItemFromCheckPointSNMP(%s): dwResult=%d"), m_name, szParam, dwResult);
   return DCErrorFromSNMPError(dwResult);
}

/**
 * Get item's value via native agent
 */
DataCollectionError Node::getItemFromAgent(const TCHAR *szParam, UINT32 dwBufSize, TCHAR *szBuffer)
{
   if ((m_state & NSF_AGENT_UNREACHABLE) ||
       (m_state & DCSF_UNREACHABLE) ||
       (m_flags & NF_DISABLE_NXCP) ||
       !(m_capabilities & NC_IS_NATIVE_AGENT))
      return DCE_COMM_ERROR;

   UINT32 dwError = ERR_NOT_CONNECTED;
   DataCollectionError rc = DCE_COMM_ERROR;
   int retry = 3;

   AgentConnectionEx *conn = getAgentConnection();
   if (conn == NULL)
      goto end_loop;

   // Get parameter from agent
   while(retry-- > 0)
   {
      dwError = conn->getParameter(szParam, dwBufSize, szBuffer);
      switch(dwError)
      {
         case ERR_SUCCESS:
            rc = DCE_SUCCESS;
            setLastAgentCommTime();
            goto end_loop;
         case ERR_UNKNOWN_PARAMETER:
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
            conn->decRefCount();
            conn = getAgentConnection();
            if (conn == NULL)
               goto end_loop;
            break;
         case ERR_INTERNAL_ERROR:
            rc = DCE_COLLECTION_ERROR;
            setLastAgentCommTime();
            goto end_loop;
      }
   }

end_loop:
   if (conn != NULL)
      conn->decRefCount();
   nxlog_debug(7, _T("Node(%s)->GetItemFromAgent(%s): dwError=%d dwResult=%d"), m_name, szParam, dwError, rc);
   return rc;
}

/**
 * Get table from agent
 */
DataCollectionError Node::getTableFromAgent(const TCHAR *name, Table **table)
{
   UINT32 dwError = ERR_NOT_CONNECTED;
   DataCollectionError result = DCE_COMM_ERROR;
   UINT32 dwTries = 3;

   *table = NULL;

   if ((m_state & NSF_AGENT_UNREACHABLE) ||
       (m_state & DCSF_UNREACHABLE) ||
       (m_flags & NF_DISABLE_NXCP) ||
       !(m_capabilities & NC_IS_NATIVE_AGENT))
      return DCE_COMM_ERROR;

   AgentConnectionEx *conn = getAgentConnection();
   if (conn == NULL)
      goto end_loop;

   // Get parameter from agent
   while(dwTries-- > 0)
   {
      dwError = conn->getTable(name, table);
      switch(dwError)
      {
         case ERR_SUCCESS:
            result = DCE_SUCCESS;
            setLastAgentCommTime();
            goto end_loop;
         case ERR_UNKNOWN_PARAMETER:
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
            conn->decRefCount();
            conn = getAgentConnection();
            if (conn == NULL)
               goto end_loop;
            break;
         case ERR_INTERNAL_ERROR:
            result = DCE_COLLECTION_ERROR;
            setLastAgentCommTime();
            goto end_loop;
      }
   }

end_loop:
   if (conn != NULL)
      conn->decRefCount();
   DbgPrintf(7, _T("Node(%s)->getTableFromAgent(%s): dwError=%d dwResult=%d"), m_name, name, dwError, result);
   return result;
}

/**
 * Get list from agent
 */
DataCollectionError Node::getListFromAgent(const TCHAR *name, StringList **list)
{
   UINT32 dwError = ERR_NOT_CONNECTED;
   DataCollectionError rc = DCE_COMM_ERROR;
   UINT32 dwTries = 3;

   *list = NULL;

   if ((m_state & NSF_AGENT_UNREACHABLE) ||
       (m_state & DCSF_UNREACHABLE) ||
       (m_flags & NF_DISABLE_NXCP) ||
       !(m_capabilities & NC_IS_NATIVE_AGENT))
      return DCE_COMM_ERROR;

   AgentConnectionEx *conn = getAgentConnection();
   if (conn == NULL)
      goto end_loop;

   // Get parameter from agent
   while(dwTries-- > 0)
   {
      dwError = conn->getList(name, list);
      switch(dwError)
      {
         case ERR_SUCCESS:
            rc = DCE_SUCCESS;
            setLastAgentCommTime();
            goto end_loop;
         case ERR_UNKNOWN_PARAMETER:
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
            conn->decRefCount();
            conn = getAgentConnection();
            if (conn == NULL)
               goto end_loop;
            break;
         case ERR_INTERNAL_ERROR:
            rc = DCE_COLLECTION_ERROR;
            setLastAgentCommTime();
            goto end_loop;
      }
   }

end_loop:
   if (conn != NULL)
      conn->decRefCount();
   DbgPrintf(7, _T("Node(%s)->getListFromAgent(%s): dwError=%d dwResult=%d"), m_name, name, dwError, rc);
   return rc;
}

/**
 * Get item's value via SM-CLP protocol
 */
DataCollectionError Node::getItemFromSMCLP(const TCHAR *param, TCHAR *buffer, size_t size)
{
   DataCollectionError result = DCE_COMM_ERROR;
   int tries = 3;

   if (m_state & DCSF_UNREACHABLE)
      return DCE_COMM_ERROR;

   smclpLock();

   // Establish connection if needed
   if (m_smclpConnection == NULL)
      if (!connectToSMCLP())
         goto end_loop;

   while(tries-- > 0)
   {
      // Get parameter
      TCHAR path[MAX_PARAM_NAME];
      nx_strncpy(path, param, MAX_PARAM_NAME);
      TCHAR *attr = _tcsrchr(path, _T('/'));
      if (attr != NULL)
      {
         *attr = 0;
         attr++;
      }
      TCHAR *value = m_smclpConnection->get(path, attr);
      if (value != NULL)
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
DataCollectionError Node::getItemFromDeviceDriver(const TCHAR *param, TCHAR *buffer, size_t size)
{
   lockProperties();
   NetworkDeviceDriver *driver = m_driver;
   unlockProperties();

   if ((driver == NULL) || !driver->hasMetrics())
      return DCE_NOT_SUPPORTED;

   SNMP_Transport *transport = createSnmpTransport();
   if (transport == NULL)
      return DCE_COMM_ERROR;
   DataCollectionError rc = driver->getMetric(transport, &m_customAttributes, m_driverData, param, buffer, size);
   delete transport;
   return rc;
}

/**
 * Get value for server's internal parameter
 */
DataCollectionError Node::getInternalItem(const TCHAR *param, size_t bufSize, TCHAR *buffer)
{
   DataCollectionError rc = super::getInternalItem(param, bufSize, buffer);
   if (rc != DCE_NOT_SUPPORTED)
      return rc;
   rc = DCE_SUCCESS;

   if (!_tcsicmp(param, _T("AgentStatus")))
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
   else if (MatchString(_T("Net.IP.NextHop(*)"), param, FALSE))
   {
      if ((m_capabilities & NC_IS_NATIVE_AGENT) || (m_capabilities & NC_IS_SNMP))
      {
         TCHAR arg[256] = _T("");
         AgentGetParameterArg(param, 1, arg, 256);
         InetAddress destAddr = InetAddress::parse(arg);
         if (destAddr.isValidUnicast())
         {
            bool isVpn;
            UINT32 ifIndex;
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
   else if (MatchString(_T("NetSvc.ResponseTime(*)"), param, FALSE))
   {
      NetObj *object = objectFromParameter(param);
      if ((object != NULL) && (object->getObjectClass() == OBJECT_NETWORKSERVICE))
      {
         _sntprintf(buffer, bufSize, _T("%u"), ((NetworkService *)object)->getResponseTime());
      }
      else
      {
         rc = DCE_NOT_SUPPORTED;
      }
   }
   else if (MatchString(_T("PingTime(*)"), param, FALSE))
   {
      NetObj *object = objectFromParameter(param);
      if ((object != NULL) && (object->getObjectClass() == OBJECT_INTERFACE))
      {
         UINT32 value = ((Interface *)object)->getPingTime();
         if (value == 10000)
            rc = DCE_COLLECTION_ERROR;
         else
            _sntprintf(buffer, bufSize, _T("%d"), value);
      }
      else
      {
         rc = DCE_NOT_SUPPORTED;
      }
   }
   else if (!_tcsicmp(_T("PingTime"), param))
   {
      if (m_ipAddress.isValid())
      {
         Interface *iface = NULL;

         // Find interface for primary IP
         lockChildList(false);
         for(int i = 0; i < m_childList->size(); i++)
         {
            NetObj *curr = m_childList->get(i);
            if ((curr->getObjectClass() == OBJECT_INTERFACE) && ((Interface *)curr)->getIpAddressList()->hasAddress(m_ipAddress))
            {
               iface = (Interface *)curr;
               break;
            }
         }
         unlockChildList();

         UINT32 value = PING_TIME_TIMEOUT;
         if (iface != NULL)
         {
            value = iface->getPingTime();
         }
         else
         {
            value = getPingTime();
         }
         if (value == PING_TIME_TIMEOUT)
            rc = DCE_COLLECTION_ERROR;
         else
            _sntprintf(buffer, bufSize, _T("%d"), value);
      }
      else
      {
         rc = DCE_NOT_SUPPORTED;
      }
   }
   else if (!_tcsicmp(param, _T("ReceivedSNMPTraps")))
   {
      lockProperties();
      _sntprintf(buffer, bufSize, INT64_FMT, m_snmpTrapCount);
      unlockProperties();
   }
   else if (!_tcsicmp(param, _T("ReceivedSyslogMessages")))
   {
      lockProperties();
      _sntprintf(buffer, bufSize, INT64_FMT, m_syslogMessageCount);
      unlockProperties();
   }
   else if (!_tcsicmp(param, _T("WirelessController.AdoptedAPCount")))
   {
      if (m_capabilities & NC_IS_WIFI_CONTROLLER)
      {
         _sntprintf(buffer, bufSize, _T("%d"), m_adoptedApCount);
      }
      else
      {
         rc = DCE_NOT_SUPPORTED;
      }
   }
   else if (!_tcsicmp(param, _T("WirelessController.TotalAPCount")))
   {
      if (m_capabilities & NC_IS_WIFI_CONTROLLER)
      {
         _sntprintf(buffer, bufSize, _T("%d"), m_totalApCount);
      }
      else
      {
         rc = DCE_NOT_SUPPORTED;
      }
   }
   else if (!_tcsicmp(param, _T("WirelessController.UnadoptedAPCount")))
   {
      if (m_capabilities & NC_IS_WIFI_CONTROLLER)
      {
         _sntprintf(buffer, bufSize, _T("%d"), m_totalApCount - m_adoptedApCount);
      }
      else
      {
         rc = DCE_NOT_SUPPORTED;
      }
   }
   else if (m_capabilities & NC_IS_LOCAL_MGMT)
   {
      if (!_tcsicmp(param, _T("Server.AverageDataCollectorQueueSize")))
      {
         _sntprintf(buffer, bufSize, _T("%f"), g_dAvgDataCollectorQueueSize);
      }
      else if (!_tcsicmp(param, _T("Server.AverageDBWriterQueueSize")))
      {
         _sntprintf(buffer, bufSize, _T("%f"), g_dAvgDBAndIDataWriterQueueSize);
      }
      else if (!_tcsicmp(param, _T("Server.AverageDBWriterQueueSize.Other")))
      {
         _sntprintf(buffer, bufSize, _T("%f"), g_dAvgDBWriterQueueSize);
      }
      else if (!_tcsicmp(param, _T("Server.AverageDBWriterQueueSize.IData")))
      {
         _sntprintf(buffer, bufSize, _T("%f"), g_dAvgIDataWriterQueueSize);
      }
      else if (!_tcsicmp(param, _T("Server.AverageDBWriterQueueSize.RawData")))
      {
         _sntprintf(buffer, bufSize, _T("%f"), g_dAvgRawDataWriterQueueSize);
      }
      else if (!_tcsicmp(param, _T("Server.AverageDCIQueuingTime")))
      {
         _sntprintf(buffer, bufSize, _T("%u"), g_dwAvgDCIQueuingTime);
      }
      else if (!_tcsicmp(param, _T("Server.AverageEventLogWriterQueueSize")))
      {
         _sntprintf(buffer, bufSize, _T("%f"), g_dAvgEventLogWriterQueueSize);
      }
      else if (!_tcsicmp(param, _T("Server.AveragePollerQueueSize")))
      {
         _sntprintf(buffer, bufSize, _T("%f"), g_dAvgPollerQueueSize);
      }
      else if (!_tcsicmp(param, _T("Server.AverageSyslogProcessingQueueSize")))
      {
         _sntprintf(buffer, bufSize, _T("%f"), g_dAvgSyslogProcessingQueueSize);
      }
      else if (!_tcsicmp(param, _T("Server.AverageSyslogWriterQueueSize")))
      {
         _sntprintf(buffer, bufSize, _T("%f"), g_dAvgSyslogWriterQueueSize);
      }
      else if (!_tcsicmp(param, _T("Server.DB.Queries.Failed")))
      {
         LIBNXDB_PERF_COUNTERS counters;
         DBGetPerfCounters(&counters);
         _sntprintf(buffer, bufSize, UINT64_FMT, counters.failedQueries);
      }
      else if (!_tcsicmp(param, _T("Server.DB.Queries.LongRunning")))
      {
         LIBNXDB_PERF_COUNTERS counters;
         DBGetPerfCounters(&counters);
         _sntprintf(buffer, bufSize, UINT64_FMT, counters.longRunningQueries);
      }
      else if (!_tcsicmp(param, _T("Server.DB.Queries.NonSelect")))
      {
         LIBNXDB_PERF_COUNTERS counters;
         DBGetPerfCounters(&counters);
         _sntprintf(buffer, bufSize, UINT64_FMT, counters.nonSelectQueries);
      }
      else if (!_tcsicmp(param, _T("Server.DB.Queries.Select")))
      {
         LIBNXDB_PERF_COUNTERS counters;
         DBGetPerfCounters(&counters);
         _sntprintf(buffer, bufSize, UINT64_FMT, counters.selectQueries);
      }
      else if (!_tcsicmp(param, _T("Server.DB.Queries.Total")))
      {
         LIBNXDB_PERF_COUNTERS counters;
         DBGetPerfCounters(&counters);
         _sntprintf(buffer, bufSize, UINT64_FMT, counters.totalQueries);
      }
      else if (!_tcsicmp(param, _T("Server.DBWriter.Requests.IData")))
      {
         _sntprintf(buffer, bufSize, UINT64_FMT, g_idataWriteRequests);
      }
      else if (!_tcsicmp(param, _T("Server.DBWriter.Requests.Other")))
      {
         _sntprintf(buffer, bufSize, UINT64_FMT, g_otherWriteRequests);
      }
      else if (!_tcsicmp(param, _T("Server.DBWriter.Requests.RawData")))
      {
         _sntprintf(buffer, bufSize, UINT64_FMT, g_rawDataWriteRequests);
      }
      else if (!_tcsicmp(param, _T("Server.Heap.Active")))
      {
         INT64 bytes = GetActiveHeapMemory();
         if (bytes != -1)
            _sntprintf(buffer, bufSize, UINT64_FMT, bytes);
         else
            rc = DCE_NOT_SUPPORTED;
      }
      else if (!_tcsicmp(param, _T("Server.Heap.Allocated")))
      {
         INT64 bytes = GetAllocatedHeapMemory();
         if (bytes != -1)
            _sntprintf(buffer, bufSize, UINT64_FMT, bytes);
         else
            rc = DCE_NOT_SUPPORTED;
      }
      else if (!_tcsicmp(param, _T("Server.Heap.Mapped")))
      {
         INT64 bytes = GetMappedHeapMemory();
         if (bytes != -1)
            _sntprintf(buffer, bufSize, UINT64_FMT, bytes);
         else
            rc = DCE_NOT_SUPPORTED;
      }
      else if (!_tcsicmp(param, _T("Server.ReceivedSNMPTraps")))
      {
         _sntprintf(buffer, bufSize, UINT64_FMT, g_snmpTrapsReceived);
      }
      else if (!_tcsicmp(param, _T("Server.ReceivedSyslogMessages")))
      {
         _sntprintf(buffer, bufSize, UINT64_FMT, g_syslogMessagesReceived);
      }
      else if (MatchString(_T("Server.ThreadPool.ActiveRequests(*)"), param, FALSE))
      {
         rc = GetThreadPoolStat(THREAD_POOL_ACTIVE_REQUESTS, param, buffer);
      }
      else if (MatchString(_T("Server.ThreadPool.CurrSize(*)"), param, FALSE))
      {
         rc = GetThreadPoolStat(THREAD_POOL_CURR_SIZE, param, buffer);
      }
      else if (MatchString(_T("Server.ThreadPool.Load(*)"), param, FALSE))
      {
         rc = GetThreadPoolStat(THREAD_POOL_LOAD, param, buffer);
      }
      else if (MatchString(_T("Server.ThreadPool.LoadAverage(*)"), param, FALSE))
      {
         rc = GetThreadPoolStat(THREAD_POOL_LOADAVG_1, param, buffer);
      }
      else if (MatchString(_T("Server.ThreadPool.LoadAverage5(*)"), param, FALSE))
      {
         rc = GetThreadPoolStat(THREAD_POOL_LOADAVG_5, param, buffer);
      }
      else if (MatchString(_T("Server.ThreadPool.LoadAverage15(*)"), param, FALSE))
      {
         rc = GetThreadPoolStat(THREAD_POOL_LOADAVG_15, param, buffer);
      }
      else if (MatchString(_T("Server.ThreadPool.MaxSize(*)"), param, FALSE))
      {
         rc = GetThreadPoolStat(THREAD_POOL_MAX_SIZE, param, buffer);
      }
      else if (MatchString(_T("Server.ThreadPool.MinSize(*)"), param, FALSE))
      {
         rc = GetThreadPoolStat(THREAD_POOL_MIN_SIZE, param, buffer);
      }
      else if (MatchString(_T("Server.ThreadPool.ScheduledRequests(*)"), param, FALSE))
      {
         rc = GetThreadPoolStat(THREAD_POOL_SCHEDULED_REQUESTS, param, buffer);
      }
      else if (MatchString(_T("Server.ThreadPool.Usage(*)"), param, FALSE))
      {
         rc = GetThreadPoolStat(THREAD_POOL_USAGE, param, buffer);
      }
      else if (!_tcsicmp(param, _T("Server.TotalEventsProcessed")))
      {
         _sntprintf(buffer, bufSize, INT64_FMT, g_totalEventsProcessed);
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
inline UINT32 RCCFromDCIError(DataCollectionError error)
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
UINT32 Node::getItemForClient(int iOrigin, UINT32 userId, const TCHAR *pszParam, TCHAR *pszBuffer, UINT32 dwBufSize)
{
   DataCollectionError rc = DCE_ACCESS_DENIED;

   // Get data from node
   switch(iOrigin)
   {
      case DS_INTERNAL:
         if (checkAccessRights(userId, OBJECT_ACCESS_READ))
            rc = getInternalItem(pszParam, dwBufSize, pszBuffer);
         break;
      case DS_NATIVE_AGENT:
         if (checkAccessRights(userId, OBJECT_ACCESS_READ_AGENT))
            rc = getItemFromAgent(pszParam, dwBufSize, pszBuffer);
         break;
      case DS_SNMP_AGENT:
         if (checkAccessRights(userId, OBJECT_ACCESS_READ_SNMP))
            rc = getItemFromSNMP(0, pszParam, dwBufSize, pszBuffer, SNMP_RAWTYPE_NONE);
         break;
      case DS_CHECKPOINT_AGENT:
         if (checkAccessRights(userId, OBJECT_ACCESS_READ_SNMP))
            rc = getItemFromCheckPointSNMP(pszParam, dwBufSize, pszBuffer);
         break;
      case DS_DEVICE_DRIVER:
         if (checkAccessRights(userId, OBJECT_ACCESS_READ_SNMP))
            rc = getItemFromDeviceDriver(pszParam, pszBuffer, dwBufSize);
         break;
      default:
         return RCC_INVALID_ARGUMENT;
   }

   return RCCFromDCIError(rc);
}

/**
 * Get table for client
 */
UINT32 Node::getTableForClient(const TCHAR *name, Table **table)
{
   return RCCFromDCIError(getTableFromAgent(name, table));
}

/**
 * Create NXCP message with object's data
 */
void Node::fillMessageInternal(NXCPMessage *pMsg, UINT32 userId)
{
   super::fillMessageInternal(pMsg, userId);
   pMsg->setField(VID_IP_ADDRESS, m_ipAddress);
   pMsg->setField(VID_PRIMARY_NAME, m_primaryName);
   pMsg->setField(VID_NODE_TYPE, (INT16)m_type);
   pMsg->setField(VID_NODE_SUBTYPE, m_subType);
   pMsg->setField(VID_HYPERVISOR_TYPE, m_hypervisorType);
   pMsg->setField(VID_HYPERVISOR_INFO, CHECK_NULL_EX(m_hypervisorInfo));
   pMsg->setField(VID_STATE_FLAGS, m_state);
   pMsg->setField(VID_CAPABILITIES, m_capabilities);
   pMsg->setField(VID_AGENT_PORT, m_agentPort);
   pMsg->setField(VID_AUTH_METHOD, m_agentAuthMethod);
   pMsg->setField(VID_AGENT_CACHE_MODE, m_agentCacheMode);
   pMsg->setField(VID_SHARED_SECRET, m_szSharedSecret);
   pMsg->setFieldFromMBString(VID_SNMP_AUTH_OBJECT, m_snmpSecurity->getCommunity());
   pMsg->setFieldFromMBString(VID_SNMP_AUTH_PASSWORD, m_snmpSecurity->getAuthPassword());
   pMsg->setFieldFromMBString(VID_SNMP_PRIV_PASSWORD, m_snmpSecurity->getPrivPassword());
   pMsg->setField(VID_SNMP_USM_METHODS, (WORD)((WORD)m_snmpSecurity->getAuthMethod() | ((WORD)m_snmpSecurity->getPrivMethod() << 8)));
   pMsg->setField(VID_SNMP_OID, m_snmpObjectId);
   pMsg->setField(VID_SNMP_PORT, m_snmpPort);
   pMsg->setField(VID_SNMP_VERSION, (WORD)m_snmpVersion);
   pMsg->setField(VID_AGENT_VERSION, m_agentVersion);
   pMsg->setField(VID_AGENT_ID, m_agentId);
   pMsg->setField(VID_PLATFORM_NAME, m_platformName);
   pMsg->setField(VID_POLLER_NODE_ID, m_pollerNode);
   pMsg->setField(VID_ZONE_UIN, m_zoneUIN);
   pMsg->setField(VID_AGENT_PROXY, m_agentProxy);
   pMsg->setField(VID_SNMP_PROXY, m_snmpProxy);
   pMsg->setField(VID_ICMP_PROXY, m_icmpProxy);
   pMsg->setField(VID_REQUIRED_POLLS, (WORD)m_requiredPollCount);
   pMsg->setField(VID_SYS_NAME, CHECK_NULL_EX(m_sysName));
   pMsg->setField(VID_SYS_DESCRIPTION, CHECK_NULL_EX(m_sysDescription));
   pMsg->setField(VID_SYS_CONTACT, CHECK_NULL_EX(m_sysContact));
   pMsg->setField(VID_SYS_LOCATION, CHECK_NULL_EX(m_sysLocation));
   pMsg->setFieldFromTime(VID_BOOT_TIME, m_bootTime);
   pMsg->setFieldFromTime(VID_AGENT_COMM_TIME, m_lastAgentCommTime);
   pMsg->setField(VID_BRIDGE_BASE_ADDRESS, m_baseBridgeAddress, 6);
   if (m_lldpNodeId != NULL)
      pMsg->setField(VID_LLDP_NODE_ID, m_lldpNodeId);
   pMsg->setField(VID_USE_IFXTABLE, (WORD)m_nUseIfXTable);
   if (m_vrrpInfo != NULL)
   {
      pMsg->setField(VID_VRRP_VERSION, (WORD)m_vrrpInfo->getVersion());
      pMsg->setField(VID_VRRP_VR_COUNT, (WORD)m_vrrpInfo->size());
   }
   if (m_driver != NULL)
   {
      pMsg->setField(VID_DRIVER_NAME, m_driver->getName());
      pMsg->setField(VID_DRIVER_VERSION, m_driver->getVersion());
   }
   pMsg->setField(VID_RACK_ID, m_rackId);
   pMsg->setField(VID_RACK_IMAGE_FRONT, m_rackImageFront);
   pMsg->setField(VID_RACK_IMAGE_REAR, m_rackImageRear);
   pMsg->setField(VID_RACK_POSITION, m_rackPosition);
   pMsg->setField(VID_RACK_HEIGHT, m_rackHeight);
   pMsg->setField(VID_SSH_PROXY, m_sshProxy);
   pMsg->setField(VID_SSH_LOGIN, m_sshLogin);
   pMsg->setField(VID_SSH_PASSWORD, m_sshPassword);
   pMsg->setField(VID_PORT_ROW_COUNT, m_portRowCount);
   pMsg->setField(VID_PORT_NUMBERING_SCHEME, m_portNumberingScheme);
   pMsg->setField(VID_AGENT_COMPRESSION_MODE, m_agentCompressionMode);
   pMsg->setField(VID_RACK_ORIENTATION, static_cast<INT16>(m_rackOrientation));
}

/**
 * Modify object from NXCP message
 */
UINT32 Node::modifyFromMessageInternal(NXCPMessage *pRequest)
{
   // Change flags
   if (pRequest->isFieldExist(VID_FLAGS))
   {
      bool wasRemoteAgent = ((m_flags & NF_REMOTE_AGENT) != 0);
      m_flags = pRequest->getFieldAsUInt32(VID_FLAGS);
      if (wasRemoteAgent && !(m_flags & NF_REMOTE_AGENT) && m_ipAddress.isValidUnicast())
      {
         if (IsZoningEnabled())
         {
            Zone *zone = FindZoneByUIN(m_zoneUIN);
            if (zone != NULL)
            {
               zone->addToIndex(this);
            }
            else
            {
               DbgPrintf(2, _T("Cannot find zone object with GUID=%d for node object %s [%d]"), (int)m_zoneUIN, m_name, (int)m_id);
            }
         }
         else
         {
            g_idxNodeByAddr.put(m_ipAddress, this);
         }
      }
      else if (!wasRemoteAgent && (m_flags & NF_REMOTE_AGENT) && m_ipAddress.isValidUnicast())
      {
         if (IsZoningEnabled())
         {
            Zone *zone = FindZoneByUIN(m_zoneUIN);
            if (zone != NULL)
            {
               zone->removeFromIndex(this);
            }
            else
            {
               DbgPrintf(2, _T("Cannot find zone object with GUID=%d for node object %s [%d]"), (int)m_zoneUIN, m_name, (int)m_id);
            }
         }
         else
         {
            g_idxNodeByAddr.remove(m_ipAddress);
         }
      }
   }

   // Change primary IP address
   if (pRequest->isFieldExist(VID_IP_ADDRESS))
   {
      InetAddress ipAddr = pRequest->getFieldAsInetAddress(VID_IP_ADDRESS);

      if (m_flags & NF_REMOTE_AGENT)
      {
         lockProperties();
         m_ipAddress = ipAddr;
         setModified(MODIFY_NODE_PROPERTIES);
         unlockProperties();
      }
      else
      {
         // Check if received IP address is one of node's interface addresses
         lockChildList(false);
         int i, count = m_childList->size();
         for(i = 0; i < count; i++)
            if ((m_childList->get(i)->getObjectClass() == OBJECT_INTERFACE) &&
                ((Interface *)m_childList->get(i))->getIpAddressList()->hasAddress(ipAddr))
               break;
         unlockChildList();
         if (i == count)
         {
            return RCC_INVALID_IP_ADDR;
         }

         // Check that there is no node with same IP as we try to change
         if ((FindNodeByIP(m_zoneUIN, ipAddr) != NULL) || (FindSubnetByIP(m_zoneUIN, ipAddr) != NULL))
         {
            return RCC_ALREADY_EXIST;
         }

         setPrimaryIPAddress(ipAddr);
      }

      // Update primary name if it is not set with the same message
      if (!pRequest->isFieldExist(VID_PRIMARY_NAME))
      {
         m_ipAddress.toString(m_primaryName);
      }

      agentLock();
      deleteAgentConnection();
      agentUnlock();
   }

   // Change primary host name
   if (pRequest->isFieldExist(VID_PRIMARY_NAME))
   {
      TCHAR primaryName[MAX_DNS_NAME];
      pRequest->getFieldAsString(VID_PRIMARY_NAME, primaryName, MAX_DNS_NAME);

      InetAddress ipAddr = ResolveHostName(m_zoneUIN, primaryName);
      if (ipAddr.isValid() && !(m_flags & NF_REMOTE_AGENT))
      {
         // Check if received IP address is one of node's interface addresses
         lockChildList(false);
         int i, count = m_childList->size();
         for(i = 0; i < count; i++)
            if ((m_childList->get(i)->getObjectClass() == OBJECT_INTERFACE) &&
                ((Interface *)m_childList->get(i))->getIpAddressList()->hasAddress(ipAddr))
               break;
         unlockChildList();
         if (i == count)
         {
            // Check that there is no node with same IP as we try to change
            if ((FindNodeByIP(m_zoneUIN, ipAddr) != NULL) || (FindSubnetByIP(m_zoneUIN, ipAddr) != NULL))
            {
               return RCC_ALREADY_EXIST;
            }
         }
      }

      _tcscpy(m_primaryName, primaryName);
      m_runtimeFlags |= DCDF_FORCE_CONFIGURATION_POLL | NDF_RECHECK_CAPABILITIES;
   }

   // Poller node ID
   if (pRequest->isFieldExist(VID_POLLER_NODE_ID))
   {
      UINT32 dwNodeId;
      NetObj *pObject;

      dwNodeId = pRequest->getFieldAsUInt32(VID_POLLER_NODE_ID);
      if (dwNodeId != 0)
      {
         pObject = FindObjectById(dwNodeId);

         // Check if received id is a valid node id
         if (pObject == NULL)
         {
            return RCC_INVALID_OBJECT_ID;
         }
         if (pObject->getObjectClass() != OBJECT_NODE)
         {
            return RCC_INVALID_OBJECT_ID;
         }
      }
      m_pollerNode = dwNodeId;
   }

   // Change listen port of native agent
   if (pRequest->isFieldExist(VID_AGENT_PORT))
      m_agentPort = pRequest->getFieldAsUInt16(VID_AGENT_PORT);

   // Change authentication method of native agent
   if (pRequest->isFieldExist(VID_AUTH_METHOD))
      m_agentAuthMethod = pRequest->getFieldAsInt16(VID_AUTH_METHOD);

   // Change cache mode for agent DCI
   if (pRequest->isFieldExist(VID_AGENT_CACHE_MODE))
      m_agentCacheMode = pRequest->getFieldAsInt16(VID_AGENT_CACHE_MODE);

   // Change shared secret of native agent
   if (pRequest->isFieldExist(VID_SHARED_SECRET))
      pRequest->getFieldAsString(VID_SHARED_SECRET, m_szSharedSecret, MAX_SECRET_LENGTH);

   // Change SNMP protocol version
   if (pRequest->isFieldExist(VID_SNMP_VERSION))
   {
      m_snmpVersion = pRequest->getFieldAsUInt16(VID_SNMP_VERSION);
      m_snmpSecurity->setSecurityModel((m_snmpVersion == SNMP_VERSION_3) ? SNMP_SECURITY_MODEL_USM : SNMP_SECURITY_MODEL_V2C);
   }

   // Change SNMP port
   if (pRequest->isFieldExist(VID_SNMP_PORT))
      m_snmpPort = pRequest->getFieldAsUInt16(VID_SNMP_PORT);

   // Change SNMP authentication data
   if (pRequest->isFieldExist(VID_SNMP_AUTH_OBJECT))
   {
      char mbBuffer[256];

      pRequest->getFieldAsMBString(VID_SNMP_AUTH_OBJECT, mbBuffer, 256);
      m_snmpSecurity->setAuthName(mbBuffer);

      pRequest->getFieldAsMBString(VID_SNMP_AUTH_PASSWORD, mbBuffer, 256);
      m_snmpSecurity->setAuthPassword(mbBuffer);

      pRequest->getFieldAsMBString(VID_SNMP_PRIV_PASSWORD, mbBuffer, 256);
      m_snmpSecurity->setPrivPassword(mbBuffer);

      WORD methods = pRequest->getFieldAsUInt16(VID_SNMP_USM_METHODS);
      m_snmpSecurity->setAuthMethod((int)(methods & 0xFF));
      m_snmpSecurity->setPrivMethod((int)(methods >> 8));
   }

   // Change proxy node
   if (pRequest->isFieldExist(VID_AGENT_PROXY))
      m_agentProxy = pRequest->getFieldAsUInt32(VID_AGENT_PROXY);

   // Change SNMP proxy node
   if (pRequest->isFieldExist(VID_SNMP_PROXY))
   {
      UINT32 oldProxy = m_snmpProxy;
      m_snmpProxy = pRequest->getFieldAsUInt32(VID_SNMP_PROXY);
      if (oldProxy != m_snmpProxy)
      {
         ThreadPoolExecute(g_mainThreadPool, this, &Node::onSnmpProxyChange, oldProxy);
      }
   }

   // Change ICMP proxy node
   if (pRequest->isFieldExist(VID_ICMP_PROXY))
      m_icmpProxy = pRequest->getFieldAsUInt32(VID_ICMP_PROXY);

   // Number of required polls
   if (pRequest->isFieldExist(VID_REQUIRED_POLLS))
      m_requiredPollCount = (int)pRequest->getFieldAsUInt16(VID_REQUIRED_POLLS);

   // Enable/disable usage of ifXTable
   if (pRequest->isFieldExist(VID_USE_IFXTABLE))
      m_nUseIfXTable = (BYTE)pRequest->getFieldAsUInt16(VID_USE_IFXTABLE);

   // Rack settings
   if (pRequest->isFieldExist(VID_RACK_ID))
   {
      m_rackId = pRequest->getFieldAsUInt32(VID_RACK_ID);
      updatePhysicalContainerBinding(OBJECT_RACK, m_rackId);
   }
   if (pRequest->isFieldExist(VID_RACK_IMAGE_FRONT))
      m_rackImageFront = pRequest->getFieldAsGUID(VID_RACK_IMAGE_FRONT);
   if (pRequest->isFieldExist(VID_RACK_IMAGE_REAR))
      m_rackImageRear = pRequest->getFieldAsGUID(VID_RACK_IMAGE_REAR);
   if (pRequest->isFieldExist(VID_RACK_POSITION))
      m_rackPosition = pRequest->getFieldAsInt16(VID_RACK_POSITION);
   if (pRequest->isFieldExist(VID_RACK_HEIGHT))
      m_rackHeight = pRequest->getFieldAsInt16(VID_RACK_HEIGHT);

   // Chassis
   if (pRequest->isFieldExist(VID_CHASSIS_ID))
   {
      m_rackId = pRequest->getFieldAsUInt32(VID_CHASSIS_ID);
      updatePhysicalContainerBinding(OBJECT_CHASSIS, m_chassisId);
   }

   if (pRequest->isFieldExist(VID_SSH_PROXY))
      m_sshProxy = pRequest->getFieldAsUInt32(VID_SSH_PROXY);

   if (pRequest->isFieldExist(VID_SSH_LOGIN))
      pRequest->getFieldAsString(VID_SSH_LOGIN, m_sshLogin, MAX_SSH_LOGIN_LEN);

   if (pRequest->isFieldExist(VID_SSH_PASSWORD))
      pRequest->getFieldAsString(VID_SSH_PASSWORD, m_sshPassword, MAX_SSH_PASSWORD_LEN);

   if (pRequest->isFieldExist(VID_AGENT_COMPRESSION_MODE))
      m_agentCompressionMode = pRequest->getFieldAsInt16(VID_AGENT_COMPRESSION_MODE);

   if (pRequest->isFieldExist(VID_RACK_ORIENTATION))
      m_rackOrientation = static_cast<RackOrientation>(pRequest->getFieldAsUInt16(VID_RACK_ORIENTATION));

   return super::modifyFromMessageInternal(pRequest);
}

/**
 * Thread pool callback executed when SNMP proxy changes
 */
void Node::onSnmpProxyChange(UINT32 oldProxy)
{
   // resync data collection configuration with new proxy
   Node *node = (Node *)FindObjectById(m_snmpProxy, OBJECT_NODE);
   if (node != NULL)
   {
      DbgPrintf(4, _T("Node::onSnmpProxyChange(%s [%d]): data collection sync needed for %s [%d]"), m_name, m_id, node->m_name, node->m_id);
      node->agentLock();
      bool newConnection;
      if (node->connectToAgent(NULL, NULL, &newConnection))
      {
         if (!newConnection)
         {
            DbgPrintf(4, _T("Node::onSnmpProxyChange(%s [%d]): initiating data collection sync for %s [%d]"), m_name, m_id, node->m_name, node->m_id);
            node->syncDataCollectionWithAgent(node->m_agentConnection);
         }
      }
      node->agentUnlock();
   }

   // resync data collection configuration with old proxy
   node = (Node *)FindObjectById(oldProxy, OBJECT_NODE);
   if (node != NULL)
   {
      DbgPrintf(4, _T("Node::onSnmpProxyChange(%s [%d]): data collection sync needed for %s [%d]"), m_name, m_id, node->m_name, node->m_id);
      node->agentLock();
      bool newConnection;
      if (node->connectToAgent(NULL, NULL, &newConnection))
      {
         if (!newConnection)
         {
            DbgPrintf(4, _T("Node::onSnmpProxyChange(%s [%d]): initiating data collection sync for %s [%d]"), m_name, m_id, node->m_name, node->m_id);
            node->syncDataCollectionWithAgent(node->m_agentConnection);
         }
      }
      node->agentUnlock();
   }
}

/**
 * Wakeup node using magic packet
 */
UINT32 Node::wakeUp()
{
   UINT32 dwResult = RCC_NO_WOL_INTERFACES;

   lockChildList(false);

   for(int i = 0; i < m_childList->size(); i++)
   {
      NetObj *object = m_childList->get(i);
      if ((object->getObjectClass() == OBJECT_INTERFACE) &&
          (object->getStatus() != STATUS_UNMANAGED) &&
          ((Interface *)object)->getIpAddressList()->getFirstUnicastAddressV4().isValid())
      {
         dwResult = ((Interface *)object)->wakeUp();
         if (dwResult == RCC_SUCCESS)
            break;
      }
   }

   // If no interface found try to find interface in unmanaged state
   if (dwResult != RCC_SUCCESS)
   {
      for(int i = 0; i < m_childList->size(); i++)
      {
         NetObj *object = m_childList->get(i);
         if ((object->getObjectClass() == OBJECT_INTERFACE) &&
             (object->getStatus() == STATUS_UNMANAGED) &&
             ((Interface *)object)->getIpAddressList()->getFirstUnicastAddressV4().isValid())
         {
            dwResult = ((Interface *)object)->wakeUp();
            if (dwResult == RCC_SUCCESS)
               break;
         }
      }
   }

   unlockChildList();
   return dwResult;
}

/**
 * Get status of interface with given index from SNMP agent
 */
void Node::getInterfaceStatusFromSNMP(SNMP_Transport *pTransport, UINT32 index, int ifTableSuffixLen, UINT32 *ifTableSuffix, InterfaceAdminState *adminState, InterfaceOperState *operState)
{
   if (m_driver != NULL)
   {
      m_driver->getInterfaceState(pTransport, &m_customAttributes, m_driverData, index, ifTableSuffixLen, ifTableSuffix, adminState, operState);
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
   if (getItemFromAgent(szParam, 32, szBuffer) == DCE_SUCCESS)
   {
      *adminState = (InterfaceAdminState)_tcstol(szBuffer, NULL, 0);

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
            if (getItemFromAgent(szParam, 32, szBuffer) == DCE_SUCCESS)
            {
               UINT32 dwLinkState = _tcstoul(szBuffer, NULL, 0);
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

   ObjectArray<AgentParameterDefinition> *parameters = ((origin == DS_NATIVE_AGENT) ? m_agentParameters : ((origin == DS_DEVICE_DRIVER) ? m_driverParameters : NULL));
   if ((flags & 0x01) && (parameters != NULL))
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

   ObjectArray<AgentTableDefinition> *tables = ((origin == DS_NATIVE_AGENT) ? m_agentTables : NULL);
   if ((flags & 0x02) && (tables != NULL))
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

   if (m_winPerfObjects != NULL)
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
      DbgPrintf(6, _T("Node[%s]::writeWinPerfObjectsToMessage(): m_winPerfObjects == NULL"), m_name);
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
   return (origin == DS_NATIVE_AGENT) ? m_agentParameters : ((origin == DS_DEVICE_DRIVER) ? m_driverParameters : NULL);
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
      AgentConnection *conn = createAgentConnection();
      if (conn != NULL)
      {
         dwError = conn->checkNetworkService(pdwStatus, ipAddr, iServiceType, wPort, wProto, pszRequest, pszResponse, responseTime);
         conn->decRefCount();
      }
   }
   return dwError;
}

/**
 * Handler for object deletion
 */
void Node::onObjectDelete(UINT32 objectId)
{
   lockProperties();
   if (objectId == m_pollerNode)
   {
      // If deleted object is our poller node, change it to default
      m_pollerNode = 0;
      setModified(MODIFY_NODE_PROPERTIES);
      DbgPrintf(3, _T("Node::onObjectDelete(%s [%u]): poller node %u deleted"), m_name, m_id, objectId);
   }
   unlockProperties();

   super::onObjectDelete(objectId);
}

/**
 * Check node for OSPF support
 */
void Node::checkOSPFSupport(SNMP_Transport *pTransport)
{
   LONG nAdminStatus;

   if (SnmpGet(m_snmpVersion, pTransport,
               _T(".1.3.6.1.2.1.14.1.2.0"), NULL, 0, &nAdminStatus, sizeof(LONG), 0) == SNMP_ERR_SUCCESS)
   {
      lockProperties();
      if (nAdminStatus)
      {
         m_capabilities |= NC_IS_OSPF;
      }
      else
      {
         m_capabilities &= ~NC_IS_OSPF;
      }
      unlockProperties();
   }
}

/**
 * Create ready to use agent connection
 */
AgentConnectionEx *Node::createAgentConnection(bool sendServerId)
{
   if ((!(m_capabilities & NC_IS_NATIVE_AGENT)) ||
       (m_flags & NF_DISABLE_NXCP) ||
       (m_state & NSF_AGENT_UNREACHABLE) ||
       (m_state & DCSF_UNREACHABLE) ||
       (m_status == STATUS_UNMANAGED))
      return NULL;

   AgentTunnel *tunnel = GetTunnelForNode(m_id);
   AgentConnectionEx *conn;
   if (tunnel != NULL)
   {
      nxlog_debug_tag(DEBUG_TAG_AGENT, 6, _T("Node::createAgentConnection(%s [%d]): using agent tunnel"), m_name, (int)m_id);
      conn = new AgentConnectionEx(m_id, tunnel, m_agentAuthMethod, m_szSharedSecret, isAgentCompressionAllowed());
      tunnel->decRefCount();
   }
   else
   {
      if (!m_ipAddress.isValidUnicast() || (m_flags & NF_AGENT_OVER_TUNNEL_ONLY))
      {
         nxlog_debug_tag(DEBUG_TAG_AGENT, 7, _T("Node::createAgentConnection(%s [%d]): %s and there are no active tunnels"), m_name, m_id,
                  (m_flags & NF_AGENT_OVER_TUNNEL_ONLY) ? _T("direct agent connections are disabled") : _T("node primary IP is invalid"));
         return NULL;
      }
      conn = new AgentConnectionEx(m_id, m_ipAddress, m_agentPort, m_agentAuthMethod, m_szSharedSecret, isAgentCompressionAllowed());
      if (!setAgentProxy(conn))
      {
         conn->decRefCount();
         return NULL;
      }
   }
   conn->setCommandTimeout(g_agentCommandTimeout);
   if (!conn->connect(g_pServerKey, NULL, NULL, sendServerId ? g_serverId : 0))
   {
      conn->decRefCount();
      conn = NULL;
   }
   else
   {
      setLastAgentCommTime();
   }
   nxlog_debug_tag(DEBUG_TAG_AGENT, 6, _T("Node::createAgentConnection(%s [%d]): conn=%p"), m_name, (int)m_id, conn);
   return conn;
}

/**
 * Get agent connection. It may or may not be primary connection. Caller should call decRefCount
 * when connection is no longer needed. File transfers and other long running oprations should
 * be avoided.
 */
AgentConnectionEx *Node::getAgentConnection(bool forcePrimary)
{
   if (m_status == STATUS_UNMANAGED)
      return NULL;

   AgentConnectionEx *conn = NULL;

   bool success;
   int retryCount = 5;
   while(--retryCount >= 0)
   {
      success = MutexTryLock(m_hAgentAccessMutex);
      if (success)
      {
         if (connectToAgent())
         {
            conn = m_agentConnection;
            conn->incRefCount();
         }
         MutexUnlock(m_hAgentAccessMutex);
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
AgentConnectionEx *Node::acquireProxyConnection(ProxyType type, bool validate)
{
   m_proxyConnections[type].lock();

   AgentConnectionEx *conn = m_proxyConnections[type].get();
   if ((conn != NULL) && !conn->isConnected())
   {
      conn->decRefCount();
      conn = NULL;
      m_proxyConnections[type].set(NULL);
      nxlog_debug_tag(DEBUG_TAG_AGENT, 4, _T("Node::acquireProxyConnection(%s [%d] type=%d): existing agent connection dropped"), m_name, (int)m_id, (int)type);
   }

   if ((conn != NULL) && validate)
   {
      UINT32 rcc = conn->nop();
      if (rcc != ERR_SUCCESS)
      {
         conn->decRefCount();
         conn = NULL;
         m_proxyConnections[type].set(NULL);
         nxlog_debug_tag(DEBUG_TAG_AGENT, 4, _T("Node::acquireProxyConnection(%s [%d] type=%d): existing agent connection failed validation (RCC=%u) and dropped"), m_name, (int)m_id, (int)type, rcc);
      }
   }

   if ((conn == NULL) && (time(NULL) - m_proxyConnections[type].getLastConnectTime() > 60))
   {
      conn = createAgentConnection();
      if (conn != NULL)
      {
         if (conn->isMasterServer())
         {
            m_proxyConnections[type].set(conn);
            nxlog_debug_tag(DEBUG_TAG_AGENT, 4, _T("Node::acquireProxyConnection(%s [%d] type=%d): new agent connection created"), m_name, (int)m_id, (int)type);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG_AGENT, 6, _T("Node::acquireProxyConnection(%s [%d] type=%d): server does not have master access to agent"), m_name, (int)m_id, (int)type);
            conn->decRefCount();
            conn = NULL;
         }
      }
      m_proxyConnections[type].setLastConnectTime(time(NULL));
   }

   if (conn != NULL)
      conn->incRefCount();

   m_proxyConnections[type].unlock();
   return conn;
}

/**
 * Get connection to zone proxy node of this node
 */
AgentConnectionEx *Node::getConnectionToZoneNodeProxy(bool validate)
{
   Zone *zone = FindZoneByUIN(m_zoneUIN);
   if (zone == NULL)
   {
     DbgPrintf(1, _T("Internal error: zone is NULL in Node::getZoneProxyConnection (zone ID = %d)"), (int)m_zoneUIN);
     return NULL;
   }

   UINT32 zoneProxyNodeId = zone->getProxyNodeId();
   Node *zoneNode = (Node *)FindObjectById(zoneProxyNodeId, OBJECT_NODE);
   if(zoneNode == NULL)
   {
      DbgPrintf(1, _T("Internal error: zone proxy node is NULL in Node::getZoneProxyConnection (zone ID = %d, node ID = %d)"), (int)m_zoneUIN, (int)zoneProxyNodeId);
      return NULL;
   }

   return zoneNode->acquireProxyConnection(ZONE_PROXY, validate);
}

/**
 * Set node's primary IP address.
 * Assumed that all necessary locks already in place
 */
void Node::setPrimaryIPAddress(const InetAddress& addr)
{
   if (addr.equals(m_ipAddress) || (m_flags & NF_REMOTE_AGENT))
      return;

   if (IsZoningEnabled())
   {
      Zone *zone = FindZoneByUIN(m_zoneUIN);
      if (zone == NULL)
      {
         DbgPrintf(1, _T("Internal error: zone is NULL in Node::setPrimaryIPAddress (zone ID = %d)"), (int)m_zoneUIN);
         return;
      }
      if (m_ipAddress.isValid())
         zone->removeFromIndex(this);
      m_ipAddress = addr;
      if (m_ipAddress.isValid())
         zone->addToIndex(this);
   }
   else
   {
      if (m_ipAddress.isValid())
         g_idxNodeByAddr.remove(m_ipAddress);
      m_ipAddress = addr;
      if (m_ipAddress.isValid())
         g_idxNodeByAddr.put(m_ipAddress, this);
   }
}

/**
 * Change node's IP address.
 *
 * @param ipAddr new IP address
 */
void Node::changeIPAddress(const InetAddress& ipAddr)
{
   pollerLock();

   lockProperties();

   // check if primary name is an IP address
   if (InetAddress::resolveHostName(m_primaryName).equals(m_ipAddress))
   {
      TCHAR ipAddrText[64];
      m_ipAddress.toString(ipAddrText);
      if (!_tcscmp(ipAddrText, m_primaryName))
         ipAddr.toString(m_primaryName);

      setPrimaryIPAddress(ipAddr);
      m_runtimeFlags |= DCDF_FORCE_CONFIGURATION_POLL | NDF_RECHECK_CAPABILITIES;

      // Change status of node and all it's children to UNKNOWN
      m_status = STATUS_UNKNOWN;
      lockChildList(false);
      for(int i = 0; i < m_childList->size(); i++)
      {
         NetObj *object = m_childList->get(i);
         object->resetStatus();
         if (object->getObjectClass() == OBJECT_INTERFACE)
         {
            if (((Interface *)object)->isFake())
            {
               ((Interface *)object)->setIpAddress(ipAddr);
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

   pollerUnlock();
}

/**
 * Change node's zone
 */
void Node::changeZone(UINT32 newZone)
{
   int i;

   pollerLock();

   lockProperties();
   m_zoneUIN = newZone;
   m_runtimeFlags |= DCDF_FORCE_CONFIGURATION_POLL | NDF_RECHECK_CAPABILITIES;
   m_lastConfigurationPoll = 0;
   unlockProperties();

   // Remove from subnets
   lockParentList(false);
   NetObj **subnets = (NetObj **)malloc(sizeof(NetObj *) * m_parentList->size());
   int count = 0;
   for(i = 0; i < m_parentList->size(); i++)
      if (m_parentList->get(i)->getObjectClass() == OBJECT_SUBNET)
         subnets[count++] = m_parentList->get(i);
   unlockParentList();

   for(i = 0; i < count; i++)
   {
      deleteParent(subnets[i]);
      subnets[i]->deleteChild(this);
   }
   free(subnets);

   // Change zone ID on interfaces
   lockChildList(false);
   for(i = 0; i < m_childList->size(); i++)
      if (m_childList->get(i)->getObjectClass() == OBJECT_INTERFACE)
         ((Interface *)m_childList->get(i))->updateZoneUIN();
   unlockChildList();

   lockProperties();
   setModified(MODIFY_RELATIONS | MODIFY_NODE_PROPERTIES);
   unlockProperties();

   agentLock();
   deleteAgentConnection();
   agentUnlock();

   pollerUnlock();
}

/**
 * Set connection for file update messages
 */
void Node::setFileUpdateConnection(AgentConnection *conn)
{
   lockProperties();
   nxlog_debug(6, _T("Changing file tracking connection for node %s [%d]: %p -> %p"), m_name, m_id, m_fileUpdateConn, conn);
   if (m_fileUpdateConn != NULL)
      m_fileUpdateConn->decRefCount();
   m_fileUpdateConn = conn;
   if (m_fileUpdateConn != NULL)
      m_fileUpdateConn->incRefCount();
   unlockProperties();
}

/**
 * Get number of interface objects and pointer to the last one
 */
UINT32 Node::getInterfaceCount(Interface **ppInterface)
{
   lockChildList(false);
   UINT32 count = 0;
   for(int i = 0; i < m_childList->size(); i++)
      if (m_childList->get(i)->getObjectClass() == OBJECT_INTERFACE)
      {
         count++;
         *ppInterface = (Interface *)m_childList->get(i);
      }
   unlockChildList();
   return count;
}

/**
 * Get routing table from node
 */
ROUTING_TABLE *Node::getRoutingTable()
{
   ROUTING_TABLE *pRT = NULL;

   if ((m_capabilities & NC_IS_NATIVE_AGENT) && (!(m_flags & NF_DISABLE_NXCP)))
   {
      AgentConnectionEx *conn = getAgentConnection();
      if (conn != NULL)
      {
         pRT = conn->getRoutingTable();
         conn->decRefCount();
      }
   }
   if ((pRT == NULL) && (m_capabilities & NC_IS_SNMP) && (!(m_flags & NF_DISABLE_SNMP)))
   {
      SNMP_Transport *pTransport = createSnmpTransport();
      if (pTransport != NULL)
      {
         pRT = SnmpGetRoutingTable(m_capabilities, pTransport);
         delete pTransport;
      }
   }

   if (pRT != NULL)
   {
      SortRoutingTable(pRT);
   }
   return pRT;
}

/**
 * Get outward interface for routing to given destination address
 */
bool Node::getOutwardInterface(const InetAddress& destAddr, InetAddress *srcAddr, UINT32 *srcIfIndex)
{
   bool found = false;
   routingTableLock();
   if (m_pRoutingTable != NULL)
   {
      for(int i = 0; i < m_pRoutingTable->iNumEntries; i++)
      {
         if ((destAddr.getAddressV4() & m_pRoutingTable->pRoutes[i].dwDestMask) == m_pRoutingTable->pRoutes[i].dwDestAddr)
         {
            *srcIfIndex = m_pRoutingTable->pRoutes[i].dwIfIndex;
            Interface *iface = findInterfaceByIndex(m_pRoutingTable->pRoutes[i].dwIfIndex);
            if (iface != NULL)
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
bool Node::getNextHop(const InetAddress& srcAddr, const InetAddress& destAddr, InetAddress *nextHop, InetAddress *route, UINT32 *ifIndex, bool *isVpn, TCHAR *name)
{
   bool nextHopFound = false;
   *name = 0;

   // Check directly connected networks and VPN connectors
   bool nonFunctionalInterfaceFound = false;
   lockChildList(false);
   for(int i = 0; i < m_childList->size(); i++)
   {
      NetObj *object = m_childList->get(i);
      if (object->getObjectClass() == OBJECT_VPNCONNECTOR)
      {
         if (((VPNConnector *)object)->isRemoteAddr(destAddr) &&
             ((VPNConnector *)object)->isLocalAddr(srcAddr))
         {
            *nextHop = ((VPNConnector *)object)->getPeerGatewayAddr();
            *route = InetAddress::INVALID;
            *ifIndex = object->getId();
            *isVpn = true;
            nx_strncpy(name, object->getName(), MAX_OBJECT_NAME);
            nextHopFound = true;
            break;
         }
      }
      else if ((object->getObjectClass() == OBJECT_INTERFACE) &&
               ((Interface *)object)->getIpAddressList()->findSameSubnetAddress(destAddr).isValid())
      {
         *nextHop = destAddr;
         *route = InetAddress::INVALID;
         *ifIndex = ((Interface *)object)->getIfIndex();
         *isVpn = false;
         nx_strncpy(name, object->getName(), MAX_OBJECT_NAME);
         if ((((Interface *)object)->getAdminState() == IF_ADMIN_STATE_UP) &&
             (((Interface *)object)->getOperState() == IF_OPER_STATE_UP))
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
   if (m_pRoutingTable != NULL)
   {
      for(int i = 0; i < m_pRoutingTable->iNumEntries; i++)
      {
         if ((!nextHopFound || (m_pRoutingTable->pRoutes[i].dwDestMask == 0xFFFFFFFF)) &&
             ((destAddr.getAddressV4() & m_pRoutingTable->pRoutes[i].dwDestMask) == m_pRoutingTable->pRoutes[i].dwDestAddr))
         {
            Interface *iface = findInterfaceByIndex(m_pRoutingTable->pRoutes[i].dwIfIndex);
            if ((m_pRoutingTable->pRoutes[i].dwNextHop == 0) && (iface != NULL) &&
                (iface->getIpAddressList()->getFirstUnicastAddressV4().getHostBits() == 0))
            {
               // On Linux XEN VMs can be pointed by individual host routes to virtual interfaces
               // where each vif has netmask 255.255.255.255 and next hop in routing table set to 0.0.0.0
               *nextHop = destAddr;
            }
            else
            {
               *nextHop = m_pRoutingTable->pRoutes[i].dwNextHop;
            }
            *route = m_pRoutingTable->pRoutes[i].dwDestAddr;
            route->setMaskBits(BitsInMask(m_pRoutingTable->pRoutes[i].dwDestMask));
            *ifIndex = m_pRoutingTable->pRoutes[i].dwIfIndex;
            *isVpn = false;
            if (iface != NULL)
            {
               nx_strncpy(name, iface->getName(), MAX_OBJECT_NAME);
            }
            else
            {
               _sntprintf(name, MAX_OBJECT_NAME, _T("%d"), m_pRoutingTable->pRoutes[i].dwIfIndex);
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
 * Entry point for routing table poller
 */
void Node::routingTablePollWorkerEntry(PollerInfo *poller)
{
   routingTablePollWorkerEntry(poller, NULL, 0);
}

/**
 * Entry point for routing table poller
 */
void Node::routingTablePollWorkerEntry(PollerInfo *poller, ClientSession *session, UINT32 rqId)
{
   poller->startExecution();
   routingTablePoll(poller, session, rqId);
   delete poller;
}

/**
 * Routing table poll
 */
void Node::routingTablePoll(PollerInfo *poller, ClientSession *session, UINT32 rqId)
{
   if (m_runtimeFlags & DCDF_DELETE_IN_PROGRESS)
   {
      m_runtimeFlags &= ~NDF_QUEUED_FOR_ROUTE_POLL;
      return;
   }

   if (IsShutdownInProgress())
      return;

   ROUTING_TABLE *pRT = getRoutingTable();
   if (pRT != NULL)
   {
      routingTableLock();
      DestroyRoutingTable(m_pRoutingTable);
      m_pRoutingTable = pRT;
      routingTableUnlock();
      DbgPrintf(5, _T("Routing table updated for node %s [%d]"), m_name, m_id);
   }
   m_lastRTUpdate = time(NULL);
   m_runtimeFlags &= ~NDF_QUEUED_FOR_ROUTE_POLL;
}

/**
 * Call SNMP Enumerate with node's SNMP parameters
 */
UINT32 Node::callSnmpEnumerate(const TCHAR *pszRootOid,
                              UINT32 (* pHandler)(SNMP_Variable *, SNMP_Transport *, void *),
                              void *pArg, const TCHAR *context)
{
   if ((m_capabilities & NC_IS_SNMP) &&
       (!(m_state & NSF_SNMP_UNREACHABLE)) &&
       (!(m_state & DCSF_UNREACHABLE)))
   {
      SNMP_Transport *pTransport;
      UINT32 dwResult;

      pTransport = createSnmpTransport(0, context);
      if (pTransport != NULL)
      {
         dwResult = SnmpWalk(pTransport, pszRootOid, pHandler, pArg);
         delete pTransport;
      }
      else
      {
         dwResult = SNMP_ERR_COMM;
      }
      return dwResult;
   }
   else
   {
      return SNMP_ERR_COMM;
   }
}

/**
 * Set proxy information for agent's connection
 */
bool Node::setAgentProxy(AgentConnectionEx *conn)
{
   UINT32 proxyNode = m_agentProxy;

   if (IsZoningEnabled() && (proxyNode == 0) && (m_zoneUIN != 0))
   {
      Zone *zone = FindZoneByUIN(m_zoneUIN);
      if ((zone != NULL) && (zone->getProxyNodeId() != m_id))
      {
         proxyNode = zone->getProxyNodeId();
      }
   }

   if (proxyNode == 0)
      return true;

   Node *node = (Node *)g_idxNodeById.get(proxyNode);
   if (node == NULL)
   {
      nxlog_debug_tag(DEBUG_TAG_AGENT, 4, _T("Node::setAgentProxy(%s [%d]): cannot find proxy node [%d]"), m_name, m_id, proxyNode);
      return false;
   }

   AgentTunnel *tunnel = GetTunnelForNode(proxyNode);
   if (tunnel != NULL)
   {
      conn->setProxy(tunnel, node->m_agentAuthMethod, node->m_szSharedSecret);
      tunnel->decRefCount();
   }
   else
   {
      conn->setProxy(node->m_ipAddress, node->m_agentPort, node->m_agentAuthMethod, node->m_szSharedSecret);
   }
   return true;
}

/**
 * Prepare node object for deletion
 */
void Node::prepareForDeletion()
{
   // Prevent node from being queued for polling
   lockProperties();
   m_runtimeFlags |= DCDF_DELETE_IN_PROGRESS;
   unlockProperties();

   // Wait for all pending polls
   DbgPrintf(4, _T("Node::PrepareForDeletion(%s [%d]): waiting for outstanding polls to finish"), m_name, (int)m_id);
   while(1)
   {
      lockProperties();
      if ((m_runtimeFlags &
            (DCDF_QUEUED_FOR_STATUS_POLL | DCDF_QUEUED_FOR_CONFIGURATION_POLL |
             NDF_QUEUED_FOR_DISCOVERY_POLL | NDF_QUEUED_FOR_ROUTE_POLL |
             NDF_QUEUED_FOR_TOPOLOGY_POLL | DCDF_QUEUED_FOR_INSTANCE_POLL)) == 0)
      {
         unlockProperties();
         break;
      }
      unlockProperties();
      ThreadSleepMs(100);
   }
   DbgPrintf(4, _T("Node::PrepareForDeletion(%s [%d]): no outstanding polls left"), m_name, (int)m_id);
   super::prepareForDeletion();
}

/**
 * Check if specified SNMP variable set to specified value.
 * If variable doesn't exist at all, will return FALSE
 */
BOOL Node::checkSNMPIntegerValue(SNMP_Transport *pTransport, const TCHAR *pszOID, int nValue)
{
   UINT32 dwTemp;

   if (SnmpGet(m_snmpVersion, pTransport, pszOID, NULL, 0, &dwTemp, sizeof(UINT32), 0) == SNMP_ERR_SUCCESS)
      return (int)dwTemp == nValue;
   return FALSE;
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
         _sntprintf(pIfList->get(i)->name, MAX_OBJECT_NAME, _T("%d"), pIfList->get(i)->index);
   }
}

/**
 * Get cluster object this node belongs to, if any
 */
Cluster *Node::getMyCluster()
{
   Cluster *pCluster = NULL;

   lockParentList(false);
   for(int i = 0; i < m_parentList->size(); i++)
      if (m_parentList->get(i)->getObjectClass() == OBJECT_CLUSTER)
      {
         pCluster = (Cluster *)m_parentList->get(i);
         break;
      }
   unlockParentList();
   return pCluster;
}

/**
 * Get effective SNMP proxy for this node
 */
UINT32 Node::getEffectiveSnmpProxy() const
{
   UINT32 snmpProxy = m_snmpProxy;
   if (IsZoningEnabled() && (snmpProxy == 0) && (m_zoneUIN != 0))
   {
      // Use zone default proxy if set
      Zone *zone = FindZoneByUIN(m_zoneUIN);
      if (zone != NULL)
      {
         snmpProxy = zone->getProxyNodeId();
      }
   }
   return snmpProxy;
}

/**
 * Get effective SSH proxy for this node
 */
UINT32 Node::getEffectiveSshProxy() const
{
   UINT32 sshProxy = m_sshProxy;
   if (IsZoningEnabled() && (sshProxy == 0) && (m_zoneUIN != 0))
   {
      // Use zone default proxy if set
      Zone *zone = FindZoneByUIN(m_zoneUIN);
      if (zone != NULL)
      {
         sshProxy = zone->getProxyNodeId();
      }
   }
   return sshProxy;
}

/**
 * Get effective ICMP proxy for this node
 */
UINT32 Node::getEffectiveIcmpProxy() const
{
   UINT32 icmpProxy = m_icmpProxy;
   if (IsZoningEnabled() && (icmpProxy == 0) && (m_zoneUIN != 0))
   {
      // Use zone default proxy if set
      Zone *zone = FindZoneByUIN(m_zoneUIN);
      if (zone != NULL)
      {
         icmpProxy = zone->getProxyNodeId();
      }
   }
   return icmpProxy;
}

/**
 * Get effective Agent proxy for this node
 */
UINT32 Node::getEffectiveAgentProxy() const
{
   UINT32 agentProxy = m_agentProxy;
   if (IsZoningEnabled() && (agentProxy == 0) && (m_zoneUIN != 0))
   {
      // Use zone default proxy if set
      Zone *zone = FindZoneByUIN(m_zoneUIN);
      if (zone != NULL)
      {
         agentProxy = zone->getProxyNodeId();
      }
   }
   return agentProxy;
}

/**
 * Get effective Zone proxy for this node
 */
UINT32 Node::getEffectiveZoneProxy() const
{
   UINT32 zoneProxy = 0;
   if (IsZoningEnabled() && (m_zoneUIN != 0))
   {
      // Use zone default proxy if set
      Zone *zone = FindZoneByUIN(m_zoneUIN);
      if (zone != NULL)
      {
         zoneProxy = zone->getProxyNodeId();
      }
   }
   return zoneProxy;
}

/**
 * Create SNMP transport
 */
SNMP_Transport *Node::createSnmpTransport(WORD port, const TCHAR *context)
{
   if ((m_flags & NF_DISABLE_SNMP) || (m_status == STATUS_UNMANAGED))
      return NULL;

   SNMP_Transport *pTransport = NULL;
   UINT32 snmpProxy = getEffectiveSnmpProxy();
   if (snmpProxy == 0)
   {
      pTransport = new SNMP_UDPTransport;
      ((SNMP_UDPTransport *)pTransport)->createUDPTransport(m_ipAddress, (port != 0) ? port : m_snmpPort);
   }
   else
   {
      Node *proxyNode = (snmpProxy == m_id) ? this : (Node *)g_idxNodeById.get(snmpProxy);
      if (proxyNode != NULL)
      {
         AgentConnection *conn = proxyNode->acquireProxyConnection(SNMP_PROXY);
         if (conn != NULL)
         {
            // Use loopback address if node is SNMP proxy for itself
            pTransport = new SNMP_ProxyTransport(conn, (snmpProxy == m_id) ? InetAddress::LOOPBACK : m_ipAddress, (port != 0) ? port : m_snmpPort);
         }
      }
   }

   // Set security
   if (pTransport != NULL)
   {
      lockProperties();
      pTransport->setSnmpVersion(m_snmpVersion);
      if (context == NULL)
      {
         pTransport->setSecurityContext(new SNMP_SecurityContext(m_snmpSecurity));
      }
      else
      {
         if (m_snmpVersion < SNMP_VERSION_3)
         {
            char community[128];
#ifdef UNICODE
            char mbContext[64];
            WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, context, -1, mbContext, 64, NULL, NULL);
            snprintf(community, 128, "%s@%s", m_snmpSecurity->getCommunity(), mbContext);
#else
            snprintf(community, 128, "%s@%s", m_snmpSecurity->getCommunity(), context);
#endif
            pTransport->setSecurityContext(new SNMP_SecurityContext(community));
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
BOOL Node::resolveName(BOOL useOnlyDNS)
{
   BOOL bSuccess = FALSE;
   BOOL bNameTruncated = FALSE;
   TCHAR szBuffer[256];

   DbgPrintf(4, _T("Resolving name for node %d [%s]..."), m_id, m_name);

   TCHAR name[MAX_OBJECT_NAME];
   bool nameResolved = false;
   if(m_zoneUIN != 0)
   {
      AgentConnectionEx *conn = getConnectionToZoneNodeProxy();
      if (conn != NULL)
      {
         nameResolved = (conn->getHostByAddr(m_ipAddress, name, MAX_DNS_NAME) != NULL);
      }
   }
   else
   {
      nameResolved = (m_ipAddress.getHostByAddr(name, MAX_OBJECT_NAME) != NULL);
   }

   // Try to resolve primary IP
   if (nameResolved)
   {
      _tcslcpy(m_name, name, MAX_OBJECT_NAME);
      if (!(g_flags & AF_USE_FQDN_FOR_NODE_NAMES))
      {
         TCHAR *pPoint = _tcschr(m_name, _T('.'));
         if (pPoint != NULL)
         {
            *pPoint = _T('\0');
            bNameTruncated = TRUE;
         }
      }
      bSuccess = TRUE;
   }
   else
   {
      // Try to resolve each interface's IP address
      lockChildList(false);
      for(int i = 0; i < m_childList->size(); i++)
      {
         if ((m_childList->get(i)->getObjectClass() == OBJECT_INTERFACE) && !((Interface *)m_childList->get(i))->isLoopback())
         {
            const InetAddressList *list = ((Interface *)m_childList->get(i))->getIpAddressList();
            for(int n = 0; n < list->size(); n++)
            {
               const InetAddress& a = list->get(i);
               if (a.isValidUnicast() && (a.getHostByAddr(name, MAX_OBJECT_NAME) != NULL))
               {
                  _tcslcpy(m_name, name, MAX_OBJECT_NAME);
                  bSuccess = TRUE;
                  break;
               }
            }
         }
      }
      unlockChildList();

      // Try to get hostname from agent if address resolution fails
      if (!(bSuccess || useOnlyDNS))
      {
         DbgPrintf(4, _T("Resolving name for node %d [%s] via agent..."), m_id, m_name);
         if (getItemFromAgent(_T("System.Hostname"), 256, szBuffer) == DCE_SUCCESS)
         {
            StrStrip(szBuffer);
            if (szBuffer[0] != 0)
            {
               _tcslcpy(m_name, szBuffer, MAX_OBJECT_NAME);
               bSuccess = TRUE;
            }
         }
      }

      // Try to get hostname from SNMP if other methods fails
      if (!(bSuccess || useOnlyDNS))
      {
         DbgPrintf(4, _T("Resolving name for node %d [%s] via SNMP..."), m_id, m_name);
         if (getItemFromSNMP(0, _T(".1.3.6.1.2.1.1.5.0"), 256, szBuffer, SNMP_RAWTYPE_NONE) == DCE_SUCCESS)
         {
            StrStrip(szBuffer);
            if (szBuffer[0] != 0)
            {
               _tcslcpy(m_name, szBuffer, MAX_OBJECT_NAME);
               bSuccess = TRUE;
            }
         }
      }
   }

   if (bSuccess)
      DbgPrintf(4, _T("Name for node %d was resolved to %s%s"), m_id, m_name,
         bNameTruncated ? _T(" (truncated to host)") : _T(""));
   else
      DbgPrintf(4, _T("Name for node %d was not resolved"), m_id);
   return bSuccess;
}

/**
 * Get current layer 2 topology (as dynamically created list which should be destroyed by caller)
 * Will return NULL if there are no topology information or it is expired
 */
NetworkMapObjectList *Node::getL2Topology()
{
   NetworkMapObjectList *pResult;
   UINT32 dwExpTime;

   dwExpTime = ConfigReadULong(_T("TopologyExpirationTime"), 900);
   MutexLock(m_mutexTopoAccess);
   if ((m_topology == NULL) || (m_topologyRebuildTimestamp + (time_t)dwExpTime < time(NULL)))
   {
      pResult = NULL;
   }
   else
   {
      pResult = new NetworkMapObjectList(m_topology);
   }
   MutexUnlock(m_mutexTopoAccess);
   return pResult;
}

/**
 * Rebuild layer 2 topology and return it as dynamically reated list which should be destroyed by caller
 */
NetworkMapObjectList *Node::buildL2Topology(UINT32 *pdwStatus, int radius, bool includeEndNodes)
{
   NetworkMapObjectList *result;
   int nDepth = (radius < 0) ? ConfigReadInt(_T("TopologyDiscoveryRadius"), 5) : radius;

   MutexLock(m_mutexTopoAccess);
   if (m_linkLayerNeighbors != NULL)
   {
      MutexUnlock(m_mutexTopoAccess);

      result = new NetworkMapObjectList();
      BuildL2Topology(*result, this, nDepth, includeEndNodes);

      MutexLock(m_mutexTopoAccess);
      delete m_topology;
      m_topology = new NetworkMapObjectList(result);
      m_topologyRebuildTimestamp = time(NULL);
   }
   else
   {
      result = NULL;
      delete_and_null(m_topology);
      *pdwStatus = RCC_NO_L2_TOPOLOGY_SUPPORT;
   }
   MutexUnlock(m_mutexTopoAccess);
   return result;
}

/**
 * Build IP topology
 */
NetworkMapObjectList *Node::buildIPTopology(UINT32 *pdwStatus, int radius, bool includeEndNodes)
{
   int nDepth = (radius < 0) ? ConfigReadInt(_T("TopologyDiscoveryRadius"), 5) : radius;
   NetworkMapObjectList *pResult = new NetworkMapObjectList();
   buildIPTopologyInternal(*pResult, nDepth, 0, false, includeEndNodes);
   return pResult;
}

/**
 * Build IP topology
 */
void Node::buildIPTopologyInternal(NetworkMapObjectList &topology, int nDepth, UINT32 seedSubnet, bool vpnLink, bool includeEndNodes)
{
   if (topology.isObjectExist(m_id))
   {
      // this node was processed already
      if (seedSubnet != 0)
         topology.linkObjects(seedSubnet, m_id, vpnLink ? LINK_TYPE_VPN : LINK_TYPE_NORMAL);
      return;
   }

   topology.addObject(m_id);
   if (seedSubnet != 0)
      topology.linkObjects(seedSubnet, m_id, vpnLink ? LINK_TYPE_VPN : LINK_TYPE_NORMAL);

   if (nDepth > 0)
   {
      ObjectArray<Subnet> subnets;
      lockParentList(false);
      for(int i = 0; i < m_parentList->size(); i++)
      {
         NetObj *object = m_parentList->get(i);

         if ((object->getId() == seedSubnet) || (object->getObjectClass() != OBJECT_SUBNET))
            continue;

         if (!topology.isObjectExist(object->getId()))
         {
            topology.addObject(object->getId());
            object->incRefCount();
            subnets.add((Subnet *)object);
         }
         topology.linkObjects(m_id, object->getId());
      }
      unlockParentList();

      for(int i = 0; i < subnets.size(); i++)
      {
         Subnet *s = subnets.get(i);
         s->buildIPTopologyInternal(topology, nDepth, m_id, includeEndNodes);
         s->decRefCount();
      }

      ObjectArray<Node> peers;
      lockChildList(false);
      for(int i = 0; i < m_childList->size(); i++)
      {
         NetObj *object = m_childList->get(i);

         if (object->getObjectClass() != OBJECT_VPNCONNECTOR)
            continue;

         Node *node = (Node *)FindObjectById(((VPNConnector *)object)->getPeerGatewayId(), OBJECT_NODE);
         if ((node != NULL) && (node->getId() != seedSubnet) && !topology.isObjectExist(node->getId()))
         {
            node->incRefCount();
            peers.add(node);
         }
      }
      unlockChildList();

      for(int i = 0; i < peers.size(); i++)
      {
         Node *n = peers.get(i);
         n->buildIPTopologyInternal(topology, nDepth - 1, m_id, true, includeEndNodes);
         n->decRefCount();
      }
   }
}

/**
 * Entry point for topology poller
 */
void Node::topologyPollWorkerEntry(PollerInfo *poller)
{
   topologyPollWorkerEntry(poller, NULL, 0);
}

/**
 * Entry point for topology poller
 */
void Node::topologyPollWorkerEntry(PollerInfo *poller, ClientSession *session, UINT32 rqId)
{
   poller->startExecution();
   topologyPoll(poller, session, rqId);
   delete poller;
}

/**
 * Topology poll
 */
void Node::topologyPoll(PollerInfo *poller, ClientSession *pSession, UINT32 rqId)
{
   if (m_runtimeFlags & DCDF_DELETE_IN_PROGRESS)
   {
      if (rqId == 0)
         m_runtimeFlags &= ~NDF_QUEUED_FOR_TOPOLOGY_POLL;
      return;
   }

   if (IsShutdownInProgress())
      return;

   poller->setStatus(_T("wait for lock"));
   pollerLock();

   if (IsShutdownInProgress())
   {
      pollerUnlock();
      return;
   }

   m_pollRequestor = pSession;
   sendPollerMsg(rqId, _T("Starting topology poll for node %s\r\n"), m_name);
   DbgPrintf(4, _T("Started topology poll for node %s [%d]"), m_name, m_id);

   if (m_driver != NULL)
   {
      poller->setStatus(_T("reading VLANs"));
      SNMP_Transport *snmp = createSnmpTransport();
      if (snmp != NULL)
      {
         VlanList *vlanList = m_driver->getVlans(snmp, &m_customAttributes, m_driverData);
         delete snmp;

         if (vlanList != NULL)
         {
            lockChildList(false);
            for(int i = 0; i < m_childList->size(); i++)
            {
               NetObj *object = m_childList->get(i);
               if (object->getObjectClass() == OBJECT_INTERFACE)
               {
                  static_cast<Interface*>(object)->clearVlanList();
               }
            }
            unlockChildList();
         }

         MutexLock(m_mutexTopoAccess);
         if (vlanList != NULL)
         {
            resolveVlanPorts(vlanList);
            sendPollerMsg(rqId, POLLER_INFO _T("VLAN list successfully retrieved from node\r\n"));
            DbgPrintf(4, _T("VLAN list retrieved from node %s [%d]"), m_name, m_id);
            if (m_vlans != NULL)
               m_vlans->decRefCount();
            m_vlans = vlanList;
         }
         else
         {
            sendPollerMsg(rqId, POLLER_WARNING _T("Cannot get VLAN list\r\n"));
            DbgPrintf(4, _T("Cannot retrieve VLAN list from node %s [%d]"), m_name, m_id);
            if (m_vlans != NULL)
               m_vlans->decRefCount();
            m_vlans = NULL;
         }
         MutexUnlock(m_mutexTopoAccess);

         lockProperties();
         UINT32 oldCaps = m_capabilities;
         if (vlanList != NULL)
            m_capabilities |= NC_HAS_VLANS;
         else
            m_capabilities &= ~NC_HAS_VLANS;
         if (oldCaps != m_capabilities)
            setModified(MODIFY_NODE_PROPERTIES);
         unlockProperties();
      }
   }

   poller->setStatus(_T("reading FDB"));
   ForwardingDatabase *fdb = GetSwitchForwardingDatabase(this);
   MutexLock(m_mutexTopoAccess);
   if (m_fdb != NULL)
      m_fdb->decRefCount();
   m_fdb = fdb;
   MutexUnlock(m_mutexTopoAccess);
   if (fdb != NULL)
   {
      DbgPrintf(4, _T("Switch forwarding database retrieved for node %s [%d]"), m_name, m_id);
      sendPollerMsg(rqId, POLLER_INFO _T("Switch forwarding database retrieved\r\n"));
   }
   else
   {
      DbgPrintf(4, _T("Failed to get switch forwarding database from node %s [%d]"), m_name, m_id);
      sendPollerMsg(rqId, POLLER_WARNING _T("Failed to get switch forwarding database\r\n"));
   }

   poller->setStatus(_T("building neighbor list"));
   LinkLayerNeighbors *nbs = BuildLinkLayerNeighborList(this);
   if (nbs != NULL)
   {
      sendPollerMsg(rqId, POLLER_INFO _T("Link layer topology retrieved (%d connections found)\r\n"), nbs->size());
      DbgPrintf(4, _T("Link layer topology retrieved for node %s [%d] (%d connections found)"), m_name, (int)m_id, nbs->size());

      MutexLock(m_mutexTopoAccess);
      if (m_linkLayerNeighbors != NULL)
         m_linkLayerNeighbors->decRefCount();
      m_linkLayerNeighbors = nbs;
      MutexUnlock(m_mutexTopoAccess);

      // Walk through interfaces and update peers
      sendPollerMsg(rqId, _T("Updating peer information on interfaces\r\n"));
      for(int i = 0; i < nbs->size(); i++)
      {
         LL_NEIGHBOR_INFO *ni = nbs->getConnection(i);
         if (ni->isCached)
            continue;   // ignore cached information

         NetObj *object = FindObjectById(ni->objectId);
         if ((object != NULL) && (object->getObjectClass() == OBJECT_NODE))
         {
            DbgPrintf(5, _T("Node::topologyPoll(%s [%d]): found peer node %s [%d], localIfIndex=%d remoteIfIndex=%d"),
                      m_name, m_id, object->getName(), object->getId(), ni->ifLocal, ni->ifRemote);
            Interface *ifLocal = findInterfaceByIndex(ni->ifLocal);
            Interface *ifRemote = ((Node *)object)->findInterfaceByIndex(ni->ifRemote);
            DbgPrintf(5, _T("Node::topologyPoll(%s [%d]): localIfObject=%s remoteIfObject=%s"), m_name, m_id,
                      (ifLocal != NULL) ? ifLocal->getName() : _T("(null)"),
                      (ifRemote != NULL) ? ifRemote->getName() : _T("(null)"));
            if ((ifLocal != NULL) && (ifRemote != NULL))
            {
               // Update old peers for local and remote interfaces, if any
               if ((ifRemote->getPeerInterfaceId() != 0) && (ifRemote->getPeerInterfaceId() != ifLocal->getId()))
               {
                  Interface *ifOldPeer = (Interface *)FindObjectById(ifRemote->getPeerInterfaceId(), OBJECT_INTERFACE);
                  if (ifOldPeer != NULL)
                  {
                     ifOldPeer->clearPeer();
                  }
               }
               if ((ifLocal->getPeerInterfaceId() != 0) && (ifLocal->getPeerInterfaceId() != ifRemote->getId()))
               {
                  Interface *ifOldPeer = (Interface *)FindObjectById(ifLocal->getPeerInterfaceId(), OBJECT_INTERFACE);
                  if (ifOldPeer != NULL)
                  {
                     ifOldPeer->clearPeer();
                  }
               }

               ifLocal->setPeer((Node *)object, ifRemote, ni->protocol, false);
               ifRemote->setPeer(this, ifLocal, ni->protocol, true);
               sendPollerMsg(rqId, _T("   Local interface %s linked to remote interface %s:%s\r\n"),
                             ifLocal->getName(), object->getName(), ifRemote->getName());
               DbgPrintf(5, _T("Local interface %s:%s linked to remote interface %s:%s"),
                         m_name, ifLocal->getName(), object->getName(), ifRemote->getName());
            }
         }
      }

      lockChildList(false);
      for(int i = 0; i < m_childList->size(); i++)
      {
         if (m_childList->get(i)->getObjectClass() != OBJECT_INTERFACE)
            continue;

         Interface *iface = (Interface *)m_childList->get(i);

         // Clear self-linked interfaces caused by bug in previous release
         if ((iface->getPeerNodeId() == m_id) && (iface->getPeerInterfaceId() == iface->getId()))
         {
            iface->clearPeer();
            DbgPrintf(6, _T("Node::topologyPoll(%s [%d]): Self-linked interface %s [%d] fixed"), m_name, m_id, iface->getName(), iface->getId());
         }
         // Remove outdated peer information
         else if (iface->getPeerNodeId() != 0)
         {
            Node *peerNode = (Node *)FindObjectById(iface->getPeerNodeId(), OBJECT_NODE);
            if (peerNode == NULL)
            {
               DbgPrintf(6, _T("Node::topologyPoll(%s [%d]): peer node set but node object does not exist"), m_name, m_id);
               iface->clearPeer();
               continue;
            }

            if (peerNode->isDown())
               continue; // Don't change information about down peers

            bool ifaceFound = false;
            for(int j = 0; j < nbs->size(); j++)
            {
               LL_NEIGHBOR_INFO *ni = nbs->getConnection(j);
               if ((ni->objectId == iface->getPeerNodeId()) && (ni->ifLocal == iface->getIfIndex()))
               {
                  bool reflection = (iface->getFlags() & IF_PEER_REFLECTION) ? true : false;
                  ifaceFound = !ni->isCached || (((ni->protocol == LL_PROTO_FDB) || (ni->protocol == LL_PROTO_STP)) && reflection);
                  break;
               }
            }

            if (!ifaceFound)
            {
               Interface *ifPeer = (Interface *)FindObjectById(iface->getPeerInterfaceId(), OBJECT_INTERFACE);
               if (ifPeer != NULL)
               {
                  ifPeer->clearPeer();
               }
               iface->clearPeer();
               DbgPrintf(6, _T("Node::topologyPoll(%s [%d]): Removed outdated peer information from interface %s [%d]"), m_name, m_id, iface->getName(), iface->getId());
            }
         }
      }
      unlockChildList();

      sendPollerMsg(rqId, _T("Link layer topology processed\r\n"));
      DbgPrintf(4, _T("Link layer topology processed for node %s [%d]"), m_name, m_id);
   }
   else
   {
      sendPollerMsg(rqId, POLLER_ERROR _T("Cannot get link layer topology\r\n"));
   }

   // Read list of associated wireless stations
   if ((m_driver != NULL) && (m_capabilities & NC_IS_WIFI_CONTROLLER))
   {
      poller->setStatus(_T("reading wireless stations"));
      SNMP_Transport *snmp = createSnmpTransport();
      if (snmp != NULL)
      {
         ObjectArray<WirelessStationInfo> *stations = m_driver->getWirelessStations(snmp, &m_customAttributes, m_driverData);
         delete snmp;
         if (stations != NULL)
         {
            sendPollerMsg(rqId, _T("   %d wireless stations found\r\n"), stations->size());
            DbgPrintf(6, _T("%d wireless stations found on controller node %s [%d]"), stations->size(), m_name, m_id);

            for(int i = 0; i < stations->size(); i++)
            {
               WirelessStationInfo *ws = stations->get(i);

               AccessPoint *ap = (ws->apMatchPolicy == AP_MATCH_BY_BSSID) ? findAccessPointByBSSID(ws->bssid) : findAccessPointByRadioId(ws->rfIndex);
               if (ap != NULL)
               {
                  ws->apObjectId = ap->getId();
                  ap->getRadioName(ws->rfIndex, ws->rfName, MAX_OBJECT_NAME);
               }
               else
               {
                  ws->apObjectId = 0;
                  ws->rfName[0] = 0;
               }

               Node *node = FindNodeByMAC(ws->macAddr);
               ws->nodeId = (node != NULL) ? node->getId() : 0;
               if ((node != NULL) && (ws->ipAddr == 0))
               {
                  Interface *iface = node->findInterfaceByMAC(ws->macAddr);
                  if ((iface != NULL) && iface->getIpAddressList()->getFirstUnicastAddressV4().isValid())
                     ws->ipAddr = iface->getIpAddressList()->getFirstUnicastAddressV4().getAddressV4();
                  else
                     ws->ipAddr = node->getIpAddress().getAddressV4();
               }
            }
         }

         lockProperties();
         delete m_wirelessStations;
         m_wirelessStations = stations;
         unlockProperties();
      }
   }

   // Call hooks in loaded modules
   poller->setStatus(_T("calling modules"));
   for(UINT32 i = 0; i < g_dwNumModules; i++)
   {
      if (g_pModuleList[i].pfTopologyPollHook != NULL)
      {
         DbgPrintf(5, _T("TopologyPoll(%s [%d]): calling hook in module %s"), m_name, m_id, g_pModuleList[i].szName);
         g_pModuleList[i].pfTopologyPollHook(this, pSession, rqId, poller);
      }
   }

   // Execute hook script
   poller->setStatus(_T("hook"));
   executeHookScript(_T("TopologyPoll"));

   sendPollerMsg(rqId, _T("Finished topology poll for node %s\r\n"), m_name);

   m_lastTopologyPoll = time(NULL);
   m_runtimeFlags &= ~NDF_QUEUED_FOR_TOPOLOGY_POLL;
   pollerUnlock();

   DbgPrintf(4, _T("Finished topology poll for node %s [%d]"), m_name, m_id);
}

/**
 * Update host connections using forwarding database information
 */
void Node::addHostConnections(LinkLayerNeighbors *nbs)
{
   ForwardingDatabase *fdb = getSwitchForwardingDatabase();
   if (fdb == NULL)
      return;

   DbgPrintf(5, _T("Node::addHostConnections(%s [%d]): FDB retrieved"), m_name, (int)m_id);

   lockChildList(false);
   for(int i = 0; i < (int)m_childList->size(); i++)
   {
      if (m_childList->get(i)->getObjectClass() != OBJECT_INTERFACE)
         continue;

      Interface *ifLocal = (Interface *)m_childList->get(i);
      BYTE macAddr[MAC_ADDR_LENGTH];
      if (fdb->isSingleMacOnPort(ifLocal->getIfIndex(), macAddr))
      {
         TCHAR buffer[64];
         DbgPrintf(6, _T("Node::addHostConnections(%s [%d]): found single MAC %s on interface %s"),
            m_name, (int)m_id, MACToStr(macAddr, buffer), ifLocal->getName());
         Interface *ifRemote = FindInterfaceByMAC(macAddr);
         if (ifRemote != NULL)
         {
            DbgPrintf(6, _T("Node::addHostConnections(%s [%d]): found remote interface %s [%d]"),
               m_name, (int)m_id, ifRemote->getName(), ifRemote->getId());
            Node *peerNode = ifRemote->getParentNode();
            if (peerNode != NULL)
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
   }
   unlockChildList();

   fdb->decRefCount();
}

/**
 * Add existing connections to link layer neighbours table
 */
void Node::addExistingConnections(LinkLayerNeighbors *nbs)
{
   lockChildList(false);
   for(int i = 0; i < (int)m_childList->size(); i++)
   {
      if (m_childList->get(i)->getObjectClass() != OBJECT_INTERFACE)
         continue;

      Interface *ifLocal = (Interface *)m_childList->get(i);
      if ((ifLocal->getPeerNodeId() != 0) && (ifLocal->getPeerInterfaceId() != 0))
      {
         Interface *ifRemote = (Interface *)FindObjectById(ifLocal->getPeerInterfaceId(), OBJECT_INTERFACE);
         if (ifRemote != NULL)
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
   for(int i = 0; i < vlanList->size(); i++)
   {
      VlanInfo *vlan = vlanList->get(i);
      vlan->prepareForResolve();
      for(int j = 0; j < vlan->getNumPorts(); j++)
      {
         UINT32 portId = vlan->getPorts()[j];
         Interface *iface = NULL;
         switch(vlan->getPortReferenceMode())
         {
            case VLAN_PRM_IFINDEX:  // interface index
               iface = findInterfaceByIndex(portId);
               break;
            case VLAN_PRM_BPORT:    // bridge port number
               iface = findBridgePort(portId);
               break;
            case VLAN_PRM_SLOTPORT: // slot/port pair
               iface = findInterfaceBySlotAndPort(portId >> 16, portId & 0xFFFF);
               break;
         }
         if (iface != NULL)
         {
            vlan->resolvePort(j, (iface->getSlotNumber() << 16) | iface->getPortNumber(), iface->getIfIndex(), iface->getId());
            iface->addVlan(vlan->getVlanId());
         }
      }
   }
}

/**
 * Create new subnet and binds to this node
 */
Subnet *Node::createSubnet(InetAddress& baseAddr, bool syntheticMask)
{
   InetAddress addr = baseAddr.getSubnetAddress();
   if (syntheticMask)
   {
      while(FindSubnetByIP(m_zoneUIN, addr) != NULL)
      {
         baseAddr.setMaskBits(baseAddr.getMaskBits() + 1);
         addr = baseAddr.getSubnetAddress();
      }

      // Do not create subnet if there are no address space for it
      if (baseAddr.getHostBits() < 2)
         return NULL;
   }

   Subnet *s = new Subnet(addr, m_zoneUIN, syntheticMask);
   NetObjInsert(s, true, false);
   if (g_flags & AF_ENABLE_ZONING)
   {
      Zone *zone = FindZoneByUIN(m_zoneUIN);
      if (zone != NULL)
      {
         zone->addSubnet(s);
      }
      else
      {
         DbgPrintf(1, _T("Node::createSubnet(): Inconsistent configuration - zone %d does not exist"), (int)m_zoneUIN);
      }
   }
   else
   {
      g_pEntireNet->AddSubnet(s);
   }
   s->addNode(this);
   DbgPrintf(4, _T("Node::createSubnet(): Created new subnet %s [%d] for node %s [%d]"),
             s->getName(), s->getId(), m_name, m_id);
   return s;
}

/**
 * Check subnet bindings
 */
void Node::checkSubnetBinding()
{
   TCHAR buffer[64];

   Cluster *pCluster = getMyCluster();

   // Build consolidated IP address list
   InetAddressList addrList;
   lockChildList(false);
   for(int n = 0; n < m_childList->size(); n++)
   {
      if (m_childList->get(n)->getObjectClass() != OBJECT_INTERFACE)
         continue;
      Interface *iface = (Interface *)m_childList->get(n);
      if (iface->isLoopback() || iface->isExcludedFromTopology())
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
   DbgPrintf(5, _T("Checking subnet bindings for node %s [%d]"), m_name, m_id);
   for(int i = 0; i < addrList.size(); i++)
   {
      InetAddress addr = addrList.get(i);
      DbgPrintf(5, _T("Node::checkSubnetBinding(%s [%d]): checking address %s/%d"), m_name, m_id, addr.toString(buffer), addr.getMaskBits());

      Interface *iface = findInterfaceByIP(addr);
      if (iface == NULL)
      {
         nxlog_write(MSG_INTERNAL_ERROR, EVENTLOG_WARNING_TYPE, "s", _T("Cannot find interface object in Node::checkSubnetBinding()"));
         continue;   // Something goes really wrong
      }

      // Is cluster interconnect interface?
      bool isSync = (pCluster != NULL) ? pCluster->isSyncAddr(addr) : false;

      Subnet *pSubnet = FindSubnetForNode(m_zoneUIN, addr);
      if (pSubnet != NULL)
      {
         DbgPrintf(5, _T("Node::checkSubnetBinding(%s [%d]): found subnet %s [%d]"), m_name, m_id, pSubnet->getName(), pSubnet->getId());
         if (isSync)
         {
            pSubnet = NULL;   // No further checks on this subnet
         }
         else
         {
            if (pSubnet->isSyntheticMask() && !iface->isSyntheticMask() && (addr.getMaskBits() > 0))
            {
               DbgPrintf(4, _T("Setting correct netmask for subnet %s [%d] from node %s [%d]"),
                         pSubnet->getName(), pSubnet->getId(), m_name, m_id);
               if ((addr.getHostBits() < 2) && (getParentCount() > 1))
               {
                  /* Delete subnet object if we try to change it's netmask to 255.255.255.255 or 255.255.255.254 and
                   node has more than one parent. hasIfaceForPrimaryIp parameter should prevent us from going in
                   loop (creating and deleting all the time subnet). */
                  pSubnet->deleteObject();
                  pSubnet = NULL;   // prevent binding to deleted subnet
               }
               else
               {
                  pSubnet->setCorrectMask(addr.getSubnetAddress());
               }
            }

            // Check if node is linked to this subnet
            if ((pSubnet != NULL) && !pSubnet->isDirectChild(m_id))
            {
               DbgPrintf(4, _T("Restored link between subnet %s [%d] and node %s [%d]"),
                         pSubnet->getName(), pSubnet->getId(), m_name, m_id);
               pSubnet->addNode(this);
            }
         }
      }
      else if (!isSync)
      {
         DbgPrintf(6, _T("Missing subnet for address %s/%d on interface %s [%d]"),
            addr.toString(buffer), addr.getMaskBits(), iface->getName(), iface->getIfIndex());

         // Ignore mask 255.255.255.255 - some point-to-point interfaces can have such mask
         // Ignore mask 255.255.255.254 - it's invalid
         if (addr.getHostBits() >= 2)
         {
            if (addr.getMaskBits() > 0)
            {
               pSubnet = createSubnet(addr, false);
            }
            else
            {
               DbgPrintf(6, _T("Zero subnet mask on interface %s [%d]"), iface->getName(), iface->getIfIndex());
               addr.setMaskBits((addr.getFamily() == AF_INET) ? ConfigReadInt(_T("DefaultSubnetMaskIPv4"), 24) : ConfigReadInt(_T("DefaultSubnetMaskIPv6"), 64));
               pSubnet = createSubnet(addr, true);
            }
            pSubnet->addNode(this);
         }
         else
         {
            DbgPrintf(6, _T("Subnet not required for address %s/%d on interface %s [%d]"),
               addr.toString(buffer), addr.getMaskBits(), iface->getName(), iface->getIfIndex());
         }
      }

      // Check if subnet mask is correct on interface
      if ((pSubnet != NULL) && (pSubnet->getIpAddress().getMaskBits() != addr.getMaskBits()) && (addr.getHostBits() > 0))
      {
         Interface *iface = findInterfaceByIP(addr);
         PostEvent(EVENT_INCORRECT_NETMASK, m_id, "idsdd", iface->getId(),
                   iface->getIfIndex(), iface->getName(),
                   addr.getMaskBits(), pSubnet->getIpAddress().getMaskBits());
      }
   }

   // Some devices may report interface list, but without IP
   // To prevent such nodes from hanging at top of the tree, attempt
   // to find subnet node primary IP
   if (m_ipAddress.isValidUnicast() && !(m_flags & NF_REMOTE_AGENT) && !addrList.hasAddress(m_ipAddress))
   {
      Subnet *pSubnet = FindSubnetForNode(m_zoneUIN, m_ipAddress);
      if (pSubnet != NULL)
      {
         // Check if node is linked to this subnet
         if (!pSubnet->isDirectChild(m_id))
         {
            DbgPrintf(4, _T("Restored link between subnet %s [%d] and node %s [%d]"),
                      pSubnet->getName(), pSubnet->getId(), m_name, m_id);
            pSubnet->addNode(this);
         }
      }
      else
      {
         InetAddress addr(m_ipAddress);
         addr.setMaskBits((addr.getFamily() == AF_INET) ? ConfigReadInt(_T("DefaultSubnetMaskIPv4"), 24) : ConfigReadInt(_T("DefaultSubnetMaskIPv6"), 64));
         createSubnet(addr, true);
      }
   }

   // Check for incorrect parent subnets
   lockParentList(false);
   lockChildList(false);
   ObjectArray<NetObj> unlinkList(m_parentList->size(), 8, false);
   for(int i = 0; i < m_parentList->size(); i++)
   {
      if (m_parentList->get(i)->getObjectClass() == OBJECT_SUBNET)
      {
         Subnet *pSubnet = (Subnet *)m_parentList->get(i);
         if (pSubnet->getIpAddress().contain(m_ipAddress) && !(m_flags & NF_REMOTE_AGENT))
            continue;   // primary IP is in given subnet

         int j;
         for(j = 0; j < addrList.size(); j++)
         {
            const InetAddress& addr = addrList.get(j);
            if (pSubnet->getIpAddress().contain(addr))
            {
               if ((pCluster != NULL) && pCluster->isSyncAddr(addr))
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
      o->deleteChild(this);
      deleteParent(o);
      o->calculateCompoundStatus();
   }
}

/**
 * Update interface names
 */
void Node::updateInterfaceNames(ClientSession *pSession, UINT32 rqId)
{
   pollerLock();
   if (IsShutdownInProgress())
   {
      pollerUnlock();
      return;
   }

   m_pollRequestor = pSession;
   sendPollerMsg(rqId, _T("Starting interface names poll for node %s\r\n"), m_name);
   DbgPrintf(4, _T("Starting interface names poll for node %s (ID: %d)"), m_name, m_id);

   // Retrieve interface list
   InterfaceList *pIfList = getInterfaceList();
   if (pIfList != NULL)
   {
      // Check names of existing interfaces
      for(int j = 0; j < pIfList->size(); j++)
      {
         InterfaceInfo *ifInfo = pIfList->get(j);

         lockChildList(false);
         for(int i = 0; i < m_childList->size(); i++)
         {
            if (m_childList->get(i)->getObjectClass() == OBJECT_INTERFACE)
            {
               Interface *pInterface = (Interface *)m_childList->get(i);

               if (ifInfo->index == pInterface->getIfIndex())
               {
                  sendPollerMsg(rqId, _T("   Checking interface %d (%s)\r\n"), pInterface->getIfIndex(), pInterface->getName());
                  if (_tcscmp(ifInfo->name, pInterface->getName()))
                  {
                     pInterface->setName(ifInfo->name);
                     sendPollerMsg(rqId, POLLER_WARNING _T("   Name of interface %d changed to %s\r\n"), pInterface->getIfIndex(), ifInfo->name);
                  }
                  if (_tcscmp(ifInfo->description, pInterface->getDescription()))
                  {
                     pInterface->setDescription(ifInfo->description);
                     sendPollerMsg(rqId, POLLER_WARNING _T("   Description of interface %d changed to %s\r\n"), pInterface->getIfIndex(), ifInfo->description);
                  }
                  if (_tcscmp(ifInfo->alias, pInterface->getAlias()))
                  {
                     pInterface->setAlias(ifInfo->alias);
                     sendPollerMsg(rqId, POLLER_WARNING _T("   Alias of interface %d changed to %s\r\n"), pInterface->getIfIndex(), ifInfo->alias);
                  }
                  break;
               }
            }
         }
         unlockChildList();
      }

      delete pIfList;
   }
   else     /* pIfList == NULL */
   {
      sendPollerMsg(rqId, POLLER_ERROR _T("   Unable to get interface list from node\r\n"));
   }

   // Finish poll
   sendPollerMsg(rqId, _T("Finished interface names poll for node %s\r\n"), m_name);
   pollerUnlock();
   DbgPrintf(4, _T("Finished interface names poll for node %s (ID: %d)"), m_name, m_id);
}

/**
 * Get list of parent objects for NXSL script
 */
NXSL_Array *Node::getParentsForNXSL(NXSL_VM *vm)
{
   NXSL_Array *parents = new NXSL_Array(vm);
   int index = 0;

   lockParentList(FALSE);
   for(int i = 0; i < m_parentList->size(); i++)
   {
      NetObj *object = m_parentList->get(i);
      if (((object->getObjectClass() == OBJECT_CONTAINER) ||
           (object->getObjectClass() == OBJECT_CLUSTER) ||
           (object->getObjectClass() == OBJECT_SUBNET) ||
           (object->getObjectClass() == OBJECT_SERVICEROOT)) &&
          object->isTrustedNode(m_id))
      {
         parents->set(index++, object->createNXSLObject(vm));
      }
   }
   unlockParentList();

   return parents;
}

/**
 * Get list of template type parent objects for NXSL script
 */
NXSL_Array *Node::getTemplatesForNXSL(NXSL_VM *vm)
{
   NXSL_Array *parents = new NXSL_Array(vm);
   int index = 0;

   lockParentList(false);
   for(int i = 0; i < m_parentList->size(); i++)
   {
      NetObj *object = m_parentList->get(i);
      if ((object->getObjectClass() == OBJECT_TEMPLATE) && object->isTrustedNode(m_id))
      {
         parents->set(index++, object->createNXSLObject(vm));
      }
   }
   unlockParentList();

   return parents;
}

/**
 * Get list of interface objects for NXSL script
 */
NXSL_Array *Node::getInterfacesForNXSL(NXSL_VM *vm)
{
   NXSL_Array *ifaces = new NXSL_Array(vm);
   int index = 0;

   lockChildList(false);
   for(int i = 0; i < m_childList->size(); i++)
   {
      if (m_childList->get(i)->getObjectClass() == OBJECT_INTERFACE)
      {
         ifaces->set(index++, m_childList->get(i)->createNXSLObject(vm));
      }
   }
   unlockChildList();

   return ifaces;
}

/**
 * Get switch forwarding database
 */
ForwardingDatabase *Node::getSwitchForwardingDatabase()
{
   ForwardingDatabase *fdb;

   MutexLock(m_mutexTopoAccess);
   if (m_fdb != NULL)
      m_fdb->incRefCount();
   fdb = m_fdb;
   MutexUnlock(m_mutexTopoAccess);
   return fdb;
}

/**
 * Get link layer neighbors
 */
LinkLayerNeighbors *Node::getLinkLayerNeighbors()
{
   MutexLock(m_mutexTopoAccess);
   if (m_linkLayerNeighbors != NULL)
      m_linkLayerNeighbors->incRefCount();
   LinkLayerNeighbors *nbs = m_linkLayerNeighbors;
   MutexUnlock(m_mutexTopoAccess);
   return nbs;
}

/**
 * Get VLANs
 */
VlanList *Node::getVlans()
{
   MutexLock(m_mutexTopoAccess);
   if (m_vlans != NULL)
      m_vlans->incRefCount();
   VlanList *vlans = m_vlans;
   MutexUnlock(m_mutexTopoAccess);
   return vlans;
}

/**
 * Check and update last agent trap ID
 */
bool Node::checkAgentTrapId(UINT64 trapId)
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
bool Node::checkSNMPTrapId(UINT32 id)
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
bool Node::checkSyslogMessageId(UINT64 id)
{
   lockProperties();
   bool valid = (id > m_lastSyslogMessageId);
   if (valid)
      m_lastSyslogMessageId = id;
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
ComponentTree *Node::getComponents()
{
   lockProperties();
   ComponentTree *components = m_components;
   if (components != NULL)
      components->incRefCount();
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
bool Node::getLldpLocalPortInfo(UINT32 idType, BYTE *id, size_t idLen, LLDP_LOCAL_PORT_INFO *buffer)
{
   bool result = false;
   lockProperties();
   if (m_lldpLocalPortInfo != NULL)
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
void Node::showLLDPInfo(CONSOLE_CTX console)
{
   TCHAR buffer[256];

   lockProperties();
   ConsolePrintf(console, _T("\x1b[1m*\x1b[0m Node LLDP ID: %s\n\n"), m_lldpNodeId);
   ConsolePrintf(console, _T("\x1b[1m*\x1b[0m Local LLDP ports\n"));
   if (m_lldpLocalPortInfo != NULL)
   {
      ConsolePrintf(console, _T("   Port | ST | Len | Local ID                 | Description\n")
                             _T("   -----+----+-----+--------------------------+--------------------------------------\n"));
      for(int i = 0; i < m_lldpLocalPortInfo->size(); i++)
      {
         LLDP_LOCAL_PORT_INFO *port = m_lldpLocalPortInfo->get(i);
         ConsolePrintf(console, _T("   %4d | %2d | %3d | %-24s | %s\n"),
                       port->portNumber, port->localIdSubtype, (int)port->localIdLen,
                       BinToStr(port->localId, port->localIdLen, buffer), port->ifDescr);
      }
   }
   else
   {
      ConsolePrintf(console, _T("   No local LLDP port info\n"));
   }
   unlockProperties();
}

/**
 * Fill NXCP message with software package list
 */
void Node::writePackageListToMessage(NXCPMessage *msg)
{
   lockProperties();
   if (m_softwarePackages != NULL)
   {
      msg->setField(VID_NUM_ELEMENTS, (UINT32)m_softwarePackages->size());
      UINT32 varId = VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < m_softwarePackages->size(); i++)
      {
         m_softwarePackages->get(i)->fillMessage(msg, varId);
         varId += 10;
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
   if (m_hardwareComponents != NULL)
   {
      msg->setField(VID_NUM_ELEMENTS, m_hardwareComponents->size());
      UINT32 varId = VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < m_hardwareComponents->size(); i++)
      {
         m_hardwareComponents->get(i)->fillMessage(msg, varId);
         varId += 10;
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
 * Write list of registered wireless stations to NXCP message
 */
void Node::writeWsListToMessage(NXCPMessage *msg)
{
   lockProperties();
   if (m_wirelessStations != NULL)
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
ObjectArray<WirelessStationInfo> *Node::getWirelessStations()
{
   ObjectArray<WirelessStationInfo> *ws = NULL;

   lockProperties();
   if ((m_wirelessStations != NULL) && (m_wirelessStations->size() > 0))
   {
      ws = new ObjectArray<WirelessStationInfo>(m_wirelessStations->size(), 16, true);
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
 * Update ping data
 */
void Node::updatePingData()
{
   UINT32 icmpProxy = m_icmpProxy;
   if (IsZoningEnabled() && (m_zoneUIN != 0) && (icmpProxy == 0))
   {
      Zone *zone = FindZoneByUIN(m_zoneUIN);
      if (zone != NULL)
      {
         icmpProxy = zone->getProxyNodeId();
      }
   }

   if (icmpProxy != 0)
   {
      nxlog_debug(7, _T("Node::updatePingData: ping via proxy [%u]"), icmpProxy);
      Node *proxyNode = (Node *)g_idxNodeById.get(icmpProxy);
      if ((proxyNode != NULL) && proxyNode->isNativeAgent() && !proxyNode->isDown())
      {
         nxlog_debug(7, _T("Node::updatePingData: proxy node found: %s"), proxyNode->getName());
         AgentConnection *conn = proxyNode->createAgentConnection();
         if (conn != NULL)
         {
            TCHAR parameter[128], buffer[64];

            _sntprintf(parameter, 128, _T("Icmp.Ping(%s)"), m_ipAddress.toString(buffer));
            if (conn->getParameter(parameter, 64, buffer) == ERR_SUCCESS)
            {
               nxlog_debug(7, _T("Node::updatePingData:  proxy response: \"%s\""), buffer);
               TCHAR *eptr;
               long value = _tcstol(buffer, &eptr, 10);
               m_pingLastTimeStamp = time(NULL);
               if ((*eptr == 0) && (value >= 0) && (value < 10000))
               {
                  m_pingTime = value;
               }
               else
               {
                  m_pingTime = PING_TIME_TIMEOUT;
                  nxlog_debug(7, _T("Node::updatePingData: incorrect value: %d or error while parsing: %s"), value, eptr);
               }
            }
            conn->decRefCount();
         }
         else
         {
            nxlog_debug(7, _T("Node::updatePingData: cannot connect to agent on proxy node [%u]"), icmpProxy);
            m_pingTime = PING_TIME_TIMEOUT;
         }
      }
      else
      {
         nxlog_debug(7, _T("Node::updatePingData: proxy node not available [%u]"), icmpProxy);
         m_pingTime = PING_TIME_TIMEOUT;
      }
   }
   else  // not using ICMP proxy
   {
      UINT32 dwPingStatus = IcmpPing(m_ipAddress, 3, g_icmpPingTimeout, &m_pingTime, g_icmpPingSize, false);
      if (dwPingStatus != ICMP_SUCCESS)
      {
         nxlog_debug(7, _T("Node::updatePingData: error getting ping %d"), dwPingStatus);
         m_pingTime = PING_TIME_TIMEOUT;
      }
      m_pingLastTimeStamp = time(NULL);
   }
}

/**
 * Get access point state via driver
 */
AccessPointState Node::getAccessPointState(AccessPoint *ap, SNMP_Transport *snmpTransport)
{
   if (m_driver == NULL)
      return AP_UNKNOWN;
   return m_driver->getAccessPointState(snmpTransport, &m_customAttributes, m_driverData, ap->getIndex(), ap->getMacAddr(), ap->getIpAddress());
}

/**
 * Synchronize data collection settings with agent
 */
void Node::syncDataCollectionWithAgent(AgentConnectionEx *conn)
{
   NXCPMessage msg(conn->getProtocolVersion());
   msg.setCode(CMD_DATA_COLLECTION_CONFIG);
   msg.setId(conn->generateRequestId());

   UINT32 count = 0;
   UINT32 fieldId = VID_ELEMENT_LIST_BASE;
   lockDciAccess(false);
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      DCObject *dco = m_dcObjects->get(i);
      if ((dco->getStatus() != ITEM_STATUS_DISABLED) &&
          dco->hasValue() &&
          (dco->getAgentCacheMode() == AGENT_CACHE_ON) &&
          (dco->getSourceNode() == 0))
      {
         msg.setField(fieldId++, dco->getId());
         msg.setField(fieldId++, (INT16)dco->getType());
         msg.setField(fieldId++, (INT16)dco->getDataSource());
         msg.setField(fieldId++, dco->getName());
         msg.setField(fieldId++, (INT32)dco->getEffectivePollingInterval());
         msg.setFieldFromTime(fieldId++, dco->getLastPollTime());
         fieldId += 4;
         count++;
      }
   }
   unlockDciAccess();

   ProxyInfo data;
   data.proxyId = m_id;
   data.count = count;
   data.msg = &msg;
   data.fieldId = fieldId;
   data.nodeInfoCount = 0;
   data.nodeInfoFieldId = VID_NODE_INFO_LIST_BASE;

   g_idxAccessPointById.forEach(super::collectProxyInfoCallback, &data);
   g_idxChassisById.forEach(super::collectProxyInfoCallback, &data);
   g_idxNodeById.forEach(super::collectProxyInfoCallback, &data);

   msg.setField(VID_NUM_ELEMENTS, data.count);
   msg.setField(VID_NUM_NODES, data.nodeInfoCount);

   UINT32 rcc;
   NXCPMessage *response = conn->customRequest(&msg);
   if (response != NULL)
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
      DbgPrintf(4, _T("SyncDataCollection: node %s [%d] synchronized"), m_name, (int)m_id);
      m_state &= ~NSF_CACHE_MODE_NOT_SUPPORTED;
   }
   else
   {
      DbgPrintf(4, _T("SyncDataCollection: node %s [%d] not synchronized (%s)"), m_name, (int)m_id, AgentErrorCodeToText(rcc));
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
   if (response != NULL)
   {
      DbgPrintf(4, _T("ClearDataCollectionConfigFromAgent: DCI configuration successfully removed from node %s [%d]"), m_name, (int)m_id);
      delete response;
   }
}

/**
 * Callback for async handling of data collection change notification
 */
void Node::onDataCollectionChangeAsyncCallback(void *arg)
{
   Node *node = (Node *)arg;
   node->agentLock();
   bool newConnection;
   if (node->connectToAgent(NULL, NULL, &newConnection))
   {
      if (!newConnection)
         node->syncDataCollectionWithAgent(node->m_agentConnection);
   }
   node->agentUnlock();
}

/**
 * Called when data collection configuration changed
 */
void Node::onDataCollectionChange()
{
   super::onDataCollectionChange();
   if (m_capabilities & NC_IS_NATIVE_AGENT)
   {
      DbgPrintf(5, _T("Node::onDataCollectionChange(%s [%d]): executing data collection sync"), m_name, m_id);
      ThreadPoolExecute(g_mainThreadPool, Node::onDataCollectionChangeAsyncCallback, this);
   }

   UINT32 snmpProxyId = getEffectiveSnmpProxy();
   if (snmpProxyId != 0)
   {
      Node *snmpProxy = (Node *)FindObjectById(snmpProxyId, OBJECT_NODE);
      if (snmpProxy != NULL)
      {
         DbgPrintf(5, _T("Node::onDataCollectionChange(%s [%d]): executing data collection sync for SNMP proxy %s [%d]"),
                   m_name, m_id, snmpProxy->getName(), snmpProxy->getId());
         ThreadPoolExecute(g_mainThreadPool, Node::onDataCollectionChangeAsyncCallback, snmpProxy);
      }
   }
}

/**
 * Force data collection configuration synchronization with agent
 */
void Node::forceSyncDataCollectionConfig()
{
   ThreadPoolExecute(g_mainThreadPool, Node::onDataCollectionChangeAsyncCallback, this);
}

/**
 * Update physical container (rack or chassis) binding
 */
void Node::updatePhysicalContainerBinding(int containerClass, UINT32 containerId)
{
   bool containerFound = false;
   ObjectArray<NetObj> deleteList(16, 16, false);

   lockParentList(true);
   for(int i = 0; i < m_parentList->size(); i++)
   {
      NetObj *object = m_parentList->get(i);
      if (object->getObjectClass() != containerClass)
         continue;
      if (object->getId() == containerId)
      {
         containerFound = true;
         continue;
      }
      object->incRefCount();
      deleteList.add(object);
   }
   unlockParentList();

   for(int n = 0; n < deleteList.size(); n++)
   {
      NetObj *container = deleteList.get(n);
      nxlog_debug(5, _T("Node::updatePhysicalContainerBinding(%s [%d]): delete incorrect binding %s [%d]"), m_name, m_id, container->getName(), container->getId());
      container->deleteChild(this);
      deleteParent(container);
      container->decRefCount();
   }

   if (!containerFound && (containerId != 0))
   {
      NetObj *container = FindObjectById(containerId, containerClass);
      if (container != NULL)
      {
         nxlog_debug(5, _T("Node::updatePhysicalContainerBinding(%s [%d]): add binding %s [%d]"), m_name, m_id, container->getName(), container->getId());
         container->addChild(this);
         addParent(container);
      }
      else
      {
         nxlog_debug(5, _T("Node::updatePhysicalContainerBinding(%s [%d]): object [%d] of class %d (%s) not found"),
                     m_name, m_id, containerId, containerClass, NetObj::getObjectClassName(containerClass));
      }
   }
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *Node::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(new NXSL_Object(vm, &g_nxslNodeClass, this));
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
 * Collect info for SNMP proxy and DCI source (proxy) nodes
 */
void Node::collectProxyInfo(ProxyInfo *info)
{
   bool snmpProxy = (getEffectiveSnmpProxy() == info->proxyId);
   bool isTarget = false;

   lockDciAccess(false);
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      DCObject *dco = m_dcObjects->get(i);
      if (dco->getStatus() == ITEM_STATUS_DISABLED)
         continue;

      if (((snmpProxy && (dco->getDataSource() == DS_SNMP_AGENT) && (dco->getSourceNode() == 0)) ||
           ((dco->getDataSource() == DS_NATIVE_AGENT) && (dco->getSourceNode() == info->proxyId))) &&
          dco->hasValue() && (dco->getAgentCacheMode() == AGENT_CACHE_ON))
      {
         addProxyDataCollectionElement(info, dco);
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
 * Set node's chassis
 */
void Node::setChassis(UINT32 chassisId)
{
   lockProperties();
   if (chassisId == m_chassisId)
   {
      unlockProperties();
      return;
   }

   m_chassisId = chassisId;
   unlockProperties();

   updatePhysicalContainerBinding(OBJECT_CHASSIS, chassisId);
}

/**
 * Set SSH credentials for node
 */
void Node::setSshCredentials(const TCHAR *login, const TCHAR *password)
{
   lockProperties();
   if (login != NULL)
      nx_strncpy(m_sshLogin, login, MAX_SSH_LOGIN_LEN);
   if (password != NULL)
      nx_strncpy(m_sshPassword, password, MAX_SSH_PASSWORD_LEN);
   setModified(MODIFY_NODE_PROPERTIES);
   unlockProperties();
}

/**
 * Check if compression allowed for agent
 */
bool Node::isAgentCompressionAllowed()
{
   if (m_agentCompressionMode == NODE_AGENT_COMPRESSION_DEFAULT)
      return ConfigReadInt(_T("DefaultAgentProtocolCompressionMode"), NODE_AGENT_COMPRESSION_ENABLED) == NODE_AGENT_COMPRESSION_ENABLED;
   return m_agentCompressionMode == NODE_AGENT_COMPRESSION_ENABLED;
}

/**
 * Set routing loop event information
 */
void Node::setRoutingLoopEvent(const InetAddress& address, UINT32 nodeId, UINT64 eventId)
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
   lockProperties();
   m_tunnelId = tunnelId;
   free(m_agentCertSubject);
   m_agentCertSubject = MemCopyString(certSubject);
   setModified(MODIFY_NODE_PROPERTIES, false);
   unlockProperties();

   TCHAR buffer[128];
   nxlog_debug_tag(DEBUG_TAG_AGENT, 4, _T("Tunnel ID for node %s [%d] set to %s"), m_name, m_id, tunnelId.toString(buffer));
}

/**
 * Build internal connection topology
 */
NetworkMapObjectList *Node::buildInternalConnectionTopology()
{
   NetworkMapObjectList *topology = new NetworkMapObjectList();
   topology->setAllowDuplicateLinks(true);
   buildInternalConnectionTopologyInternal(topology, m_id, false, false);

   return topology;
}

/**
 * Checks if proxy has been set and links objects, if not set, creates new server link or edits existing
 */
bool Node::checkProxyAndLink(NetworkMapObjectList *topology, UINT32 seedNode, UINT32 proxyId, UINT32 linkType, const TCHAR *linkName, bool checkAllProxies)
{
   if ((proxyId == 0) || (IsZoningEnabled() && (getEffectiveZoneProxy() == proxyId) && (linkType != LINK_TYPE_ZONE_PROXY)))
      return false;

   Node *proxy = reinterpret_cast<Node *>(FindObjectById(proxyId, OBJECT_NODE));
   if (proxy != NULL && proxy->getId() != m_id)
   {
      ObjLink *link = topology->getLink(m_id, proxyId, linkType);
      if (link != NULL)
         return true;

      topology->addObject(proxyId);
      topology->linkObjects(m_id, proxyId, linkType, linkName);
      proxy->buildInternalConnectionTopologyInternal(topology, seedNode, !checkAllProxies, checkAllProxies);
   }
   return true;
}

/**
 * Build internal connection topology - internal function
 */
void Node::buildInternalConnectionTopologyInternal(NetworkMapObjectList *topology, UINT32 seedNode, bool agentConnectionOnly, bool checkAllProxies)
{
   if (topology->getNumObjects() != 0 && seedNode == m_id)
      return;
   topology->addObject(m_id);

   if (IsZoningEnabled())
      checkProxyAndLink(topology, seedNode, getEffectiveZoneProxy(), LINK_TYPE_ZONE_PROXY, _T("Zone proxy"), checkAllProxies);

   String protocols;
	if (!m_tunnelId.equals(uuid::NULL_UUID))
	{
	   topology->addObject(FindLocalMgmtNode());
	   topology->linkObjects(m_id, FindLocalMgmtNode(), LINK_TYPE_AGENT_TUNNEL, _T("Agent tunnel"));
	}
	else if (!checkProxyAndLink(topology, seedNode, getEffectiveAgentProxy(), LINK_TYPE_AGENT_PROXY, _T("Agent proxy"), checkAllProxies))
	   protocols.append(_T("Agent "));

	// Only agent connection necessary for proxy nodes
	// Only if IP is set while using agent tunnel
	if (!agentConnectionOnly && _tcscmp(m_primaryName, _T("0.0.0.0")))
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
         Node *server = reinterpret_cast<Node *>(FindObjectById(FindLocalMgmtNode(), OBJECT_NODE));
         if (server != NULL && server->getZoneUIN() != m_zoneUIN)
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
 * Build entire network map
 */
NetworkMapObjectList *Node::buildInternalCommunicationTopology()
{
   NetworkMapObjectList *topology = new NetworkMapObjectList();
   topology->setAllowDuplicateLinks(true);
   buildInternalCommunicationTopologyInternal(topology);

   return topology;
}

/**
 * Build entire network map - internal function
 */
void Node::buildInternalCommunicationTopologyInternal(NetworkMapObjectList *topology)
{
   ObjectArray<NetObj> *objects = g_idxObjectById.getObjects(true);
   for(int i = 0; i < objects->size(); i++)
   {
      NetObj *obj = objects->get(i);
      if (obj != NULL && !obj->isDeleted())
      {
         if (obj->getId() == m_id)
            continue;

         if (obj->getObjectClass() == OBJECT_NODE)
            static_cast<Node *>(obj)->buildInternalConnectionTopologyInternal(topology, m_id, false, true);
         else if (obj->getObjectClass() == OBJECT_SENSOR)
            static_cast<Sensor *>(obj)->buildInternalConnectionTopologyInternal(topology, true);
      }
   }
}

/**
 * Serialize object to JSON
 */
json_t *Node::toJson()
{
   json_t *root = super::toJson();
   json_object_set_new(root, "ipAddress", m_ipAddress.toJson());
   json_object_set_new(root, "primaryName", json_string_t(m_primaryName));
   json_object_set_new(root, "tunnelId", m_tunnelId.toJson());
   json_object_set_new(root, "stateFlags", json_integer(m_state));
   json_object_set_new(root, "type", json_integer(m_type));
   json_object_set_new(root, "subType", json_string_t(m_subType));
   json_object_set_new(root, "pendingState", json_integer(m_pendingState));
   json_object_set_new(root, "pollCountSNMP", json_integer(m_pollCountSNMP));
   json_object_set_new(root, "pollCountAgent", json_integer(m_pollCountAgent));
   json_object_set_new(root, "pollCountAllDown", json_integer(m_pollCountAllDown));
   json_object_set_new(root, "requiredPollCount", json_integer(m_requiredPollCount));
   json_object_set_new(root, "zoneUIN", json_integer(m_zoneUIN));
   json_object_set_new(root, "agentPort", json_integer(m_agentPort));
   json_object_set_new(root, "agentAuthMethod", json_integer(m_agentAuthMethod));
   json_object_set_new(root, "agentCacheMode", json_integer(m_agentCacheMode));
   json_object_set_new(root, "agentCompressionMode", json_integer(m_agentCompressionMode));
   json_object_set_new(root, "sharedSecret", json_string_t(m_szSharedSecret));
   json_object_set_new(root, "statusPollType", json_integer(m_iStatusPollType));
   json_object_set_new(root, "snmpVersion", json_integer(m_snmpVersion));
   json_object_set_new(root, "snmpPort", json_integer(m_snmpPort));
   json_object_set_new(root, "nUseIfXTable", json_integer(m_nUseIfXTable));
   json_object_set_new(root, "snmpSecurity", (m_snmpSecurity != NULL) ? m_snmpSecurity->toJson() : json_object());
   json_object_set_new(root, "agentVersion", json_string_t(m_agentVersion));
   json_object_set_new(root, "platformName", json_string_t(m_platformName));
   json_object_set_new(root, "snmpObjectId", json_string_t(m_snmpObjectId));
   json_object_set_new(root, "sysDescription", json_string_t(m_sysDescription));
   json_object_set_new(root, "sysName", json_string_t(m_sysName));
   json_object_set_new(root, "sysLocation", json_string_t(m_sysLocation));
   json_object_set_new(root, "sysContact", json_string_t(m_sysContact));
   json_object_set_new(root, "lldpNodeId", json_string_t(m_lldpNodeId));
   json_object_set_new(root, "driverName", (m_driver != NULL) ? json_string_t(m_driver->getName()) : json_null());
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
   json_object_set_new(root, "rackId", json_integer(m_rackId));
   json_object_set_new(root, "rackImageFront", m_rackImageFront.toJson());
   json_object_set_new(root, "rackImageRear", m_rackImageRear.toJson());
   json_object_set_new(root, "chassisId", json_integer(m_chassisId));
   json_object_set_new(root, "syslogMessageCount", json_integer(m_syslogMessageCount));
   json_object_set_new(root, "snmpTrapCount", json_integer(m_snmpTrapCount));
   json_object_set_new(root, "sshLogin", json_string_t(m_sshLogin));
   json_object_set_new(root, "sshPassword", json_string_t(m_sshPassword));
   json_object_set_new(root, "sshProxy", json_integer(m_sshProxy));
   json_object_set_new(root, "portNumberingScheme", json_integer(m_portNumberingScheme));
   json_object_set_new(root, "portRowCount", json_integer(m_portRowCount));
   return root;
}
