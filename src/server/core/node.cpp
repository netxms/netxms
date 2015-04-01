/*
** NetXMS - Network Management System
** Copyright (C) 2003-2014 Victor Kirhenshtein
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

/**
 * Externals
 */
extern Queue g_statusPollQueue;
extern Queue g_configPollQueue;
extern Queue g_instancePollQueue;
extern Queue g_topologyPollQueue;
extern Queue g_routePollQueue;
extern Queue g_discoveryPollQueue;

/**
 * Node class default constructor
 */
Node::Node() : DataCollectionTarget()
{
	m_primaryName[0] = 0;
   m_iStatus = STATUS_UNKNOWN;
   m_dwFlags = 0;
   m_dwDynamicFlags = 0;
   m_zoneId = 0;
   m_agentPort = AGENT_LISTEN_PORT;
   m_agentAuthMethod = AUTH_NONE;
   m_szSharedSecret[0] = 0;
   m_iStatusPollType = POLL_ICMP_PING;
   m_snmpVersion = SNMP_VERSION_1;
   m_wSNMPPort = SNMP_DEFAULT_PORT;
	char community[MAX_COMMUNITY_LENGTH];
   ConfigReadStrA(_T("DefaultCommunityString"), community, MAX_COMMUNITY_LENGTH, "public");
	m_snmpSecurity = new SNMP_SecurityContext(community);
   m_szObjectId[0] = 0;
   m_lastDiscoveryPoll = 0;
   m_lastStatusPoll = 0;
   m_lastConfigurationPoll = 0;
   m_lastInstancePoll = 0;
	m_lastTopologyPoll = 0;
   m_lastRTUpdate = 0;
	m_downSince = 0;
   m_bootTime = 0;
   m_agentUpTime = 0;
   m_hPollerMutex = MutexCreate();
   m_hAgentAccessMutex = MutexCreate();
   m_hSmclpAccessMutex = MutexCreate();
   m_mutexRTAccess = MutexCreate();
	m_mutexTopoAccess = MutexCreate();
   m_pAgentConnection = NULL;
   m_smclpConnection = NULL;
	m_lastAgentTrapId = 0;
	m_lastSNMPTrapId = 0;
   m_lastAgentPushRequestId = 0;
   m_szAgentVersion[0] = 0;
   m_szPlatformName[0] = 0;
	m_sysDescription = NULL;
	m_sysName = NULL;
	m_lldpNodeId = NULL;
	m_lldpLocalPortInfo = NULL;
   m_paramList = NULL;
	m_tableList = NULL;
   m_pollerNode = 0;
   m_agentProxy = 0;
	m_snmpProxy = 0;
   m_icmpProxy = 0;
   memset(m_qwLastEvents, 0, sizeof(QWORD) * MAX_LAST_EVENTS);
   m_pRoutingTable = NULL;
   m_failTimeSNMP = 0;
   m_failTimeAgent = 0;
	m_linkLayerNeighbors = NULL;
	m_vrrpInfo = NULL;
	m_pTopology = NULL;
	m_topologyRebuildTimestamp = 0;
	m_iPendingStatus = -1;
	m_iPollCount = 0;
	m_iRequiredPollCount = 0;	// Use system default
	m_nUseIfXTable = IFXTABLE_DEFAULT;	// Use system default
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
	m_winPerfObjects = NULL;
	memset(m_baseBridgeAddress, 0, MAC_ADDR_LENGTH);
}

/**
 * Constructor for new node object
 */
Node::Node(const InetAddress& addr, UINT32 dwFlags, UINT32 agentProxy, UINT32 snmpProxy, UINT32 dwZone) : DataCollectionTarget()
{
   addr.toString(m_primaryName);
   m_iStatus = STATUS_UNKNOWN;
   m_ipAddress = addr;
   m_dwFlags = dwFlags;
   m_dwDynamicFlags = 0;
   m_zoneId = dwZone;
   m_agentPort = AGENT_LISTEN_PORT;
   m_agentAuthMethod = AUTH_NONE;
   m_szSharedSecret[0] = 0;
   m_iStatusPollType = POLL_ICMP_PING;
   m_snmpVersion = SNMP_VERSION_1;
   m_wSNMPPort = SNMP_DEFAULT_PORT;
	char community[MAX_COMMUNITY_LENGTH];
   ConfigReadStrA(_T("DefaultCommunityString"), community, MAX_COMMUNITY_LENGTH, "public");
	m_snmpSecurity = new SNMP_SecurityContext(community);
   addr.toString(m_name);    // Make default name from IP address
   m_szObjectId[0] = 0;
   m_lastDiscoveryPoll = 0;
   m_lastStatusPoll = 0;
   m_lastConfigurationPoll = 0;
   m_lastInstancePoll = 0;
	m_lastTopologyPoll = 0;
   m_lastRTUpdate = 0;
	m_downSince = 0;
   m_bootTime = 0;
   m_agentUpTime = 0;
   m_hPollerMutex = MutexCreate();
   m_hAgentAccessMutex = MutexCreate();
   m_hSmclpAccessMutex = MutexCreate();
   m_mutexRTAccess = MutexCreate();
	m_mutexTopoAccess = MutexCreate();
   m_pAgentConnection = NULL;
   m_smclpConnection = NULL;
	m_lastAgentTrapId = 0;
	m_lastSNMPTrapId = 0;
   m_lastAgentPushRequestId = 0;
   m_szAgentVersion[0] = 0;
   m_szPlatformName[0] = 0;
	m_sysDescription = NULL;
	m_sysName = NULL;
	m_lldpNodeId = NULL;
	m_lldpLocalPortInfo = NULL;
   m_paramList = NULL;
	m_tableList = NULL;
   m_pollerNode = 0;
   m_agentProxy = agentProxy;
	m_snmpProxy = snmpProxy;
   m_icmpProxy = 0;
   memset(m_qwLastEvents, 0, sizeof(QWORD) * MAX_LAST_EVENTS);
   m_isHidden = true;
   m_pRoutingTable = NULL;
   m_failTimeSNMP = 0;
   m_failTimeAgent = 0;
	m_linkLayerNeighbors = NULL;
	m_vrrpInfo = NULL;
	m_pTopology = NULL;
	m_topologyRebuildTimestamp = 0;
	m_iPendingStatus = -1;
	m_iPollCount = 0;
	m_iRequiredPollCount = 0;	// Use system default
	m_nUseIfXTable = IFXTABLE_DEFAULT;	// Use system default
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
	m_winPerfObjects = NULL;
	memset(m_baseBridgeAddress, 0, MAC_ADDR_LENGTH);
}

/**
 * Node destructor
 */
Node::~Node()
{
	delete m_driverData;
   MutexDestroy(m_hPollerMutex);
   MutexDestroy(m_hAgentAccessMutex);
   MutexDestroy(m_hSmclpAccessMutex);
   MutexDestroy(m_mutexRTAccess);
	MutexDestroy(m_mutexTopoAccess);
   delete m_pAgentConnection;
   delete m_smclpConnection;
   delete m_paramList;
	delete m_tableList;
	safe_free(m_sysDescription);
   DestroyRoutingTable(m_pRoutingTable);
	if (m_linkLayerNeighbors != NULL)
		m_linkLayerNeighbors->decRefCount();
	delete m_vrrpInfo;
	delete m_pTopology;
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
	delete m_winPerfObjects;
	safe_free(m_sysName);
}

/**
 * Create object from database data
 */
BOOL Node::loadFromDatabase(UINT32 dwId)
{
   int i, iNumRows;
   UINT32 dwSubnetId;
   NetObj *pObject;
   BOOL bResult = FALSE;

   m_id = dwId;

   if (!loadCommonProperties())
   {
      DbgPrintf(2, _T("Cannot load common properties for node object %d"), dwId);
      return FALSE;
   }

	DB_STATEMENT hStmt = DBPrepare(g_hCoreDB,
		_T("SELECT primary_name,primary_ip,node_flags,")
      _T("snmp_version,auth_method,secret,")
      _T("agent_port,status_poll_type,snmp_oid,agent_version,")
      _T("platform_name,poller_node_id,zone_guid,")
      _T("proxy_node,snmp_proxy,required_polls,uname,")
		_T("use_ifxtable,snmp_port,community,usm_auth_password,")
		_T("usm_priv_password,usm_methods,snmp_sys_name,bridge_base_addr,")
      _T("runtime_flags,down_since,boot_time,driver_name,icmp_proxy FROM nodes WHERE id=?"));
	if (hStmt == NULL)
		return FALSE;

	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, dwId);
	DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult == NULL)
	{
		DBFreeStatement(hStmt);
      return FALSE;     // Query failed
	}

   if (DBGetNumRows(hResult) == 0)
   {
      DBFreeResult(hResult);
		DBFreeStatement(hStmt);
      DbgPrintf(2, _T("Missing record in \"nodes\" table for node object %d"), dwId);
      return FALSE;
   }

   DBGetField(hResult, 0, 0, m_primaryName, MAX_DNS_NAME);
   m_ipAddress = DBGetFieldInetAddr(hResult, 0, 1);
   m_dwFlags = DBGetFieldULong(hResult, 0, 2);
   m_snmpVersion = DBGetFieldLong(hResult, 0, 3);
   m_agentAuthMethod = (WORD)DBGetFieldLong(hResult, 0, 4);
   DBGetField(hResult, 0, 5, m_szSharedSecret, MAX_SECRET_LENGTH);
   m_agentPort = (WORD)DBGetFieldLong(hResult, 0, 6);
   m_iStatusPollType = DBGetFieldLong(hResult, 0, 7);
   DBGetField(hResult, 0, 8, m_szObjectId, MAX_OID_LEN * 4);
   DBGetField(hResult, 0, 9, m_szAgentVersion, MAX_AGENT_VERSION_LEN);
   DBGetField(hResult, 0, 10, m_szPlatformName, MAX_PLATFORM_NAME_LEN);
   m_pollerNode = DBGetFieldULong(hResult, 0, 11);
   m_zoneId = DBGetFieldULong(hResult, 0, 12);
   m_agentProxy = DBGetFieldULong(hResult, 0, 13);
   m_snmpProxy = DBGetFieldULong(hResult, 0, 14);
   m_iRequiredPollCount = DBGetFieldLong(hResult, 0, 15);
   m_sysDescription = DBGetField(hResult, 0, 16, NULL, 0);
   m_nUseIfXTable = (BYTE)DBGetFieldLong(hResult, 0, 17);
	m_wSNMPPort = (WORD)DBGetFieldLong(hResult, 0, 18);

	// SNMP authentication parameters
	char snmpAuthObject[256], snmpAuthPassword[256], snmpPrivPassword[256];
	DBGetFieldA(hResult, 0, 19, snmpAuthObject, 256);
	DBGetFieldA(hResult, 0, 20, snmpAuthPassword, 256);
	DBGetFieldA(hResult, 0, 21, snmpPrivPassword, 256);
	int snmpMethods = DBGetFieldLong(hResult, 0, 22);
	delete m_snmpSecurity;
	m_snmpSecurity = new SNMP_SecurityContext(snmpAuthObject, snmpAuthPassword, snmpPrivPassword, snmpMethods & 0xFF, snmpMethods >> 8);
	m_snmpSecurity->setSecurityModel((m_snmpVersion == SNMP_VERSION_3) ? SNMP_SECURITY_MODEL_USM : SNMP_SECURITY_MODEL_V2C);

	m_sysName = DBGetField(hResult, 0, 23, NULL, 0);

	TCHAR baseAddr[16];
	TCHAR *value = DBGetField(hResult, 0, 24, baseAddr, 16);
	if (value != NULL)
		StrToBin(value, m_baseBridgeAddress, MAC_ADDR_LENGTH);

	m_dwDynamicFlags = DBGetFieldULong(hResult, 0, 25);
	m_dwDynamicFlags &= NDF_PERSISTENT;	// Clear out all non-persistent runtime flags

	m_downSince = DBGetFieldLong(hResult, 0, 26);
   m_bootTime = DBGetFieldLong(hResult, 0, 27);

	// Setup driver
	TCHAR driverName[34];
	DBGetField(hResult, 0, 28, driverName, 34);
	StrStrip(driverName);
	if (driverName[0] != 0)
		m_driver = FindDriverByName(driverName);

   m_icmpProxy = DBGetFieldULong(hResult, 0, 29);

   DBFreeResult(hResult);
	DBFreeStatement(hStmt);

   if (!m_isDeleted)
   {
      // Link node to subnets
		hStmt = DBPrepare(g_hCoreDB, _T("SELECT subnet_id FROM nsmap WHERE node_id=?"));
		if (hStmt == NULL)
			return FALSE;

		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
      hResult = DBSelectPrepared(hStmt);
      if (hResult == NULL)
		{
			DBFreeStatement(hStmt);
         return FALSE;     // Query failed
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
            pObject->AddChild(this);
            AddParent(pObject);
         }
      }

      DBFreeResult(hResult);
		DBFreeStatement(hStmt);

		loadItemsFromDB();
      loadACLFromDB();

      // Walk through all items in the node and load appropriate thresholds
		bResult = TRUE;
      for(i = 0; i < m_dcObjects->size(); i++)
         if (!m_dcObjects->get(i)->loadThresholdsFromDB())
         {
            DbgPrintf(3, _T("Cannot load thresholds for DCI %d of node %d (%s)"),
                      m_dcObjects->get(i)->getId(), dwId, m_name);
            bResult = FALSE;
         }
   }
   else
   {
      bResult = TRUE;
   }

   return bResult;
}

/**
 * Save object to database
 */
BOOL Node::saveToDatabase(DB_HANDLE hdb)
{
   // Lock object's access
   lockProperties();

   if (!saveCommonProperties(hdb))
	{
		unlockProperties();
		return FALSE;
	}

   // Form and execute INSERT or UPDATE query
	int snmpMethods = m_snmpSecurity->getAuthMethod() | (m_snmpSecurity->getPrivMethod() << 8);
	DB_STATEMENT hStmt;
   if (IsDatabaseRecordExist(hdb, _T("nodes"), _T("id"), m_id))
	{
		hStmt = DBPrepare(hdb,
			_T("UPDATE nodes SET primary_ip=?,primary_name=?,snmp_port=?,node_flags=?,snmp_version=?,community=?,")
         _T("status_poll_type=?,agent_port=?,auth_method=?,secret=?,snmp_oid=?,uname=?,agent_version=?,")
			_T("platform_name=?,poller_node_id=?,zone_guid=?,proxy_node=?,snmp_proxy=?,icmp_proxy=?,required_polls=?,")
			_T("use_ifxtable=?,usm_auth_password=?,usm_priv_password=?,usm_methods=?,snmp_sys_name=?,bridge_base_addr=?,")
			_T("runtime_flags=?,down_since=?,driver_name=?,rack_image=?,rack_position=?,rack_id=?,boot_time=? WHERE id=?"));
	}
   else
	{
		hStmt = DBPrepare(hdb,
		  _T("INSERT INTO nodes (primary_ip,primary_name,snmp_port,node_flags,snmp_version,community,status_poll_type,")
		  _T("agent_port,auth_method,secret,snmp_oid,uname,agent_version,platform_name,poller_node_id,zone_guid,")
		  _T("proxy_node,snmp_proxy,icmp_proxy,required_polls,use_ifxtable,usm_auth_password,usm_priv_password,usm_methods,")
		  _T("snmp_sys_name,bridge_base_addr,runtime_flags,down_since,driver_name,rack_image,rack_position,rack_id,boot_time,id) ")
		  _T("VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"));
	}
	if (hStmt == NULL)
	{
		unlockProperties();
		return FALSE;
	}

   TCHAR ipAddr[64], baseAddress[16];

	DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_ipAddress.toString(ipAddr), DB_BIND_STATIC);
	DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_primaryName, DB_BIND_STATIC);
	DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (LONG)m_wSNMPPort);
	DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_dwFlags);
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
	DBBind(hStmt, 11, DB_SQLTYPE_VARCHAR, m_szObjectId, DB_BIND_STATIC);
	DBBind(hStmt, 12, DB_SQLTYPE_VARCHAR, m_sysDescription, DB_BIND_STATIC);
	DBBind(hStmt, 13, DB_SQLTYPE_VARCHAR, m_szAgentVersion, DB_BIND_STATIC);
	DBBind(hStmt, 14, DB_SQLTYPE_VARCHAR, m_szPlatformName, DB_BIND_STATIC);
	DBBind(hStmt, 15, DB_SQLTYPE_INTEGER, m_pollerNode);
	DBBind(hStmt, 16, DB_SQLTYPE_INTEGER, m_zoneId);
	DBBind(hStmt, 17, DB_SQLTYPE_INTEGER, m_agentProxy);
	DBBind(hStmt, 18, DB_SQLTYPE_INTEGER, m_snmpProxy);
	DBBind(hStmt, 19, DB_SQLTYPE_INTEGER, m_icmpProxy);
	DBBind(hStmt, 20, DB_SQLTYPE_INTEGER, (LONG)m_iRequiredPollCount);
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
	DBBind(hStmt, 27, DB_SQLTYPE_INTEGER, m_dwDynamicFlags);
	DBBind(hStmt, 28, DB_SQLTYPE_INTEGER, (LONG)m_downSince);
	DBBind(hStmt, 29, DB_SQLTYPE_VARCHAR, (m_driver != NULL) ? m_driver->getName() : _T(""), DB_BIND_STATIC);
	DBBind(hStmt, 30, DB_SQLTYPE_VARCHAR, _T("00000000-0000-0000-0000-000000000000"), DB_BIND_STATIC);	// rack image
	DBBind(hStmt, 31, DB_SQLTYPE_INTEGER, (LONG)0);	// rack position
	DBBind(hStmt, 32, DB_SQLTYPE_INTEGER, (LONG)0);	// rack ID
	DBBind(hStmt, 33, DB_SQLTYPE_INTEGER, (LONG)m_bootTime);	// rack ID
	DBBind(hStmt, 34, DB_SQLTYPE_INTEGER, m_id);

	BOOL bResult = DBExecute(hStmt);
	DBFreeStatement(hStmt);

   // Save access list
   saveACLToDB(hdb);

   unlockProperties();

   // Save data collection items
   if (bResult)
   {
		lockDciAccess(false);
      for(int i = 0; i < m_dcObjects->size(); i++)
         m_dcObjects->get(i)->saveToDB(hdb);
		unlockDciAccess();
   }

	// Clear modifications flag
	lockProperties();
   m_isModified = false;
	unlockProperties();

	return bResult;
}

/**
 * Delete object from database
 */
bool Node::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = DataCollectionTarget::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM nodes WHERE id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM nsmap WHERE node_id=?"));
   return success;
}

/**
 * Get ARP cache from node
 */
ARP_CACHE *Node::getArpCache()
{
   ARP_CACHE *pArpCache = NULL;

   if (m_dwFlags & NF_IS_LOCAL_MGMT)
   {
      pArpCache = GetLocalArpCache();
   }
   else if (m_dwFlags & NF_IS_NATIVE_AGENT)
   {
      agentLock();
      if (connectToAgent())
         pArpCache = m_pAgentConnection->getArpCache();
      agentUnlock();
   }
   else if (m_dwFlags & NF_IS_SNMP)
   {
		SNMP_Transport *pTransport;

		pTransport = createSnmpTransport();
		if (pTransport != NULL)
		{
			pArpCache = SnmpGetArpCache(m_snmpVersion, pTransport);
			delete pTransport;
		}
   }

   return pArpCache;
}

/**
 * Get list of interfaces from node
 */
InterfaceList *Node::getInterfaceList()
{
   InterfaceList *pIfList = NULL;

   if ((m_dwFlags & NF_IS_NATIVE_AGENT) && (!(m_dwFlags & NF_DISABLE_NXCP)))
   {
      agentLock();
      if (connectToAgent())
      {
         pIfList = m_pAgentConnection->getInterfaceList();
      }
      agentUnlock();
   }
   if ((pIfList == NULL) && (m_dwFlags & NF_IS_LOCAL_MGMT))
   {
      pIfList = GetLocalInterfaceList();
   }
   if ((pIfList == NULL) && (m_dwFlags & NF_IS_SNMP) &&
       (!(m_dwFlags & NF_DISABLE_SNMP)) && (m_driver != NULL))
   {
		SNMP_Transport *pTransport = createSnmpTransport();
		if (pTransport != NULL)
		{
   		bool useIfXTable;
			if (m_nUseIfXTable == IFXTABLE_DEFAULT)
			{
				useIfXTable = (ConfigReadInt(_T("UseIfXTable"), 1) != 0) ? true : false;
			}
			else
			{
				useIfXTable = (m_nUseIfXTable == IFXTABLE_ENABLED) ? true : false;
			}

			int useAliases = ConfigReadInt(_T("UseInterfaceAliases"), 0);
			pIfList = m_driver->getInterfaces(pTransport, &m_customAttributes, m_driverData, useAliases, useIfXTable);

			if ((pIfList != NULL) && (m_dwFlags & NF_IS_BRIDGE))
			{
				BridgeMapPorts(m_snmpVersion, pTransport, pIfList);
			}
			delete pTransport;
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
				continue;	// Do not add interfaces if router is not in master state

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
 * Find interface by index and/or IP subnet. Interface IP considered matching
 * if it is from same IP subnet as hostAddr or if hostAddr set to INADDR_ANY.
 *
 * @param ifIndex interface index to match or INVALID_INDEX to select first matching interface
 * @param hostAddr IP address to match or INADDR_ANY to select first matching interface
 * @return pointer to interface object or NULL if appropriate interface couldn't be found
 */
Interface *Node::findInterfaceByIndex(UINT32 ifIndex)
{
   UINT32 i;
   Interface *pInterface;

   LockChildList(FALSE);
   for(i = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i]->getObjectClass() == OBJECT_INTERFACE)
      {
         pInterface = (Interface *)m_pChildList[i];
			if (pInterface->getIfIndex() == ifIndex)
         {
            UnlockChildList();
            return pInterface;
         }
      }
   UnlockChildList();
   return NULL;
}

/**
 * Find interface by name or description
 * Returns pointer to interface object or NULL if appropriate interface couldn't be found
 */
Interface *Node::findInterfaceByName(const TCHAR *name)
{
   UINT32 i;
   Interface *pInterface;

   LockChildList(FALSE);
   for(i = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i]->getObjectClass() == OBJECT_INTERFACE)
      {
         pInterface = (Interface *)m_pChildList[i];
			if (!_tcsicmp(pInterface->getName(), name) || !_tcsicmp(pInterface->getDescription(), name))
         {
            UnlockChildList();
            return pInterface;
         }
      }
   UnlockChildList();
   return NULL;
}

/**
 * Find interface by slot/port pair
 * Returns pointer to interface object or NULL if appropriate interface couldn't be found
 */
Interface *Node::findInterfaceBySlotAndPort(UINT32 slot, UINT32 port)
{
   UINT32 i;
   Interface *pInterface;

   LockChildList(FALSE);
   for(i = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i]->getObjectClass() == OBJECT_INTERFACE)
      {
         pInterface = (Interface *)m_pChildList[i];
			if (pInterface->isPhysicalPort() && (pInterface->getSlotNumber() == slot) && (pInterface->getPortNumber() == port))
         {
            UnlockChildList();
            return pInterface;
         }
      }
   UnlockChildList();
   return NULL;
}

/**
 * Find interface by MAC address
 * Returns pointer to interface object or NULL if appropriate interface couldn't be found
 */
Interface *Node::findInterfaceByMAC(const BYTE *macAddr)
{
   Interface *iface = NULL;
   LockChildList(FALSE);
   for(UINT32 i = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i]->getObjectClass() == OBJECT_INTERFACE)
      {
			if (!memcmp(((Interface *)m_pChildList[i])->getMacAddr(), macAddr, MAC_ADDR_LENGTH))
         {
            iface = (Interface *)m_pChildList[i];
            break;
         }
      }
   UnlockChildList();
   return iface;
}

/**
 * Find interface by IP address
 * Returns pointer to interface object or NULL if appropriate interface couldn't be found
 */
Interface *Node::findInterfaceByIP(const InetAddress& addr)
{
   UINT32 i;
   Interface *pInterface;

   if (!addr.isValid())
		return NULL;

   LockChildList(FALSE);
   for(i = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i]->getObjectClass() == OBJECT_INTERFACE)
      {
         pInterface = (Interface *)m_pChildList[i];
         if (pInterface->getIpAddressList()->hasAddress(addr))
         {
            UnlockChildList();
            return pInterface;
         }
      }
   UnlockChildList();
   return NULL;
}

/**
 * Find interface by bridge port number
 */
Interface *Node::findBridgePort(UINT32 bridgePortNumber)
{
   UINT32 i;
   Interface *pInterface;

   LockChildList(FALSE);
   for(i = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i]->getObjectClass() == OBJECT_INTERFACE)
      {
         pInterface = (Interface *)m_pChildList[i];
			if (pInterface->getBridgePortNumber() == bridgePortNumber)
         {
            UnlockChildList();
            return pInterface;
         }
      }
   UnlockChildList();
   return NULL;
}

/**
 * Find connection point for node
 */
NetObj *Node::findConnectionPoint(UINT32 *localIfId, BYTE *localMacAddr, int *type)
{
	NetObj *cp = NULL;
   LockChildList(FALSE);
   for(UINT32 i = 0; i < m_dwChildCount; i++)
   {
      if (m_pChildList[i]->getObjectClass() == OBJECT_INTERFACE)
      {
         Interface *iface = (Interface *)m_pChildList[i];
			cp = FindInterfaceConnectionPoint(iface->getMacAddr(), type);
			if (cp != NULL)
			{
				*localIfId = iface->getId();
				memcpy(localMacAddr, iface->getMacAddr(), MAC_ADDR_LENGTH);
				break;
			}
		}
   }
   UnlockChildList();
   return cp;
}

/**
 * Find attached access point by MAC address
 */
AccessPoint *Node::findAccessPointByMAC(const BYTE *macAddr)
{
   AccessPoint *ap = NULL;
   LockChildList(FALSE);
   for(UINT32 i = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i]->getObjectClass() == OBJECT_ACCESSPOINT)
      {
         if (!memcmp(((AccessPoint *)m_pChildList[i])->getMacAddr(), macAddr, MAC_ADDR_LENGTH))
         {
            ap = (AccessPoint *)m_pChildList[i];
            break;
         }
      }
   UnlockChildList();
   return ap;
}

/**
 * Find access point by radio ID (radio interface index)
 */
AccessPoint *Node::findAccessPointByRadioId(int rfIndex)
{
   AccessPoint *ap = NULL;
   LockChildList(FALSE);
   for(UINT32 i = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i]->getObjectClass() == OBJECT_ACCESSPOINT)
      {
         if (((AccessPoint *)m_pChildList[i])->isMyRadio(rfIndex))
         {
            ap = (AccessPoint *)m_pChildList[i];
            break;
         }
      }
   UnlockChildList();
   return ap;
}

/**
 * Find attached access point by BSSID
 */
AccessPoint *Node::findAccessPointByBSSID(const BYTE *bssid)
{
   AccessPoint *ap = NULL;
   LockChildList(FALSE);
   for(UINT32 i = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i]->getObjectClass() == OBJECT_ACCESSPOINT)
      {
         if (!memcmp(((AccessPoint *)m_pChildList[i])->getMacAddr(), bssid, MAC_ADDR_LENGTH) ||
             ((AccessPoint *)m_pChildList[i])->isMyRadio(bssid))
         {
            ap = (AccessPoint *)m_pChildList[i];
            break;
         }
      }
   UnlockChildList();
   return ap;
}

/**
 * Check if given IP address is one of node's interfaces
 */
bool Node::isMyIP(const InetAddress& addr)
{
   UINT32 i;

   LockChildList(FALSE);
   for(i = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i]->getObjectClass() == OBJECT_INTERFACE)
      {
         if (((Interface *)m_pChildList[i])->getIpAddressList()->hasAddress(addr))
         {
            UnlockChildList();
            return true;
         }
      }
   UnlockChildList();
   return false;
}

/**
 * Create new interface - convenience wrapper
 */
Interface *Node::createNewInterface(const InetAddress& ipAddr, BYTE *macAddr)
{
   InterfaceInfo info(1);
   info.ipAddrList.add(ipAddr);
   if (macAddr != NULL)
      memcpy(info.macAddr, macAddr, MAC_ADDR_LENGTH);
   return createNewInterface(&info, false);
}

/**
 * Create new interface
 */
Interface *Node::createNewInterface(InterfaceInfo *info, bool manuallyCreated)
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
			   Subnet *pSubnet = FindSubnetForNode(m_zoneId, addr);
			   if (pSubnet == NULL)
			   {
				   // Check if netmask is 0 (detect), and if yes, create
				   // new subnet with class mask
               if (addr.getMaskBits() == 0)
				   {
					   bSyntheticMask = true;
                  addr.setMaskBits((addr.getFamily() == AF_INET) ? 24 : 64);
                  info->ipAddrList.replace(addr);
				   }

				   // Create new subnet object
               if (addr.getHostBits() >= 2)
				   {
					   pSubnet = createSubnet(addr, bSyntheticMask);
				   }
			   }
			   else
			   {
				   // Set correct netmask if we was asked for it
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
   Interface *pInterface;
   if (info->name[0] != 0)
		pInterface = new Interface(info->name, (info->description[0] != 0) ? info->description : info->name,
                                 info->index, info->ipAddrList, info->type, m_zoneId);
   else
      pInterface = new Interface(info->ipAddrList, m_zoneId, bSyntheticMask);
   pInterface->setMacAddr(info->macAddr);
	pInterface->setBridgePortNumber(info->bridgePort);
	pInterface->setSlotNumber(info->slot);
	pInterface->setPortNumber(info->port);
   pInterface->setPhysicalPortFlag(info->isPhysicalPort);
	pInterface->setManualCreationFlag(manuallyCreated);
   pInterface->setSystemFlag(info->isSystem);

   // Insert to objects' list and generate event
   NetObjInsert(pInterface, TRUE);
   addInterface(pInterface);
   if (!m_isHidden)
      pInterface->unhide();
   if (!pInterface->isSystem())
   {
      const InetAddress& addr = pInterface->getFirstIpAddress();
      PostEvent(EVENT_INTERFACE_ADDED, m_id, "dsAdd", pInterface->getId(),
                pInterface->getName(), &addr, addr.getMaskBits(), pInterface->getIfIndex());
   }

	return pInterface;
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

         LockChildList(FALSE);
         for(UINT32 j = 0; j < m_dwChildCount; j++)
            if ((m_pChildList[j]->getObjectClass() == OBJECT_INTERFACE) && (m_pChildList[j] != iface) &&
                ((Interface *)m_pChildList[j])->getIpAddressList()->findSameSubnetAddress(*addr).isValid())
            {
               doUnlink = false;
               break;
            }
         UnlockChildList();

         if (doUnlink)
         {
            // Last interface in subnet, should unlink node
            Subnet *pSubnet = FindSubnetByIP(m_zoneId, addr->getSubnetAddress());
            if (pSubnet != NULL)
            {
               DeleteParent(pSubnet);
               pSubnet->DeleteChild(this);
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
   int iOldStatus = m_iStatus;
   static UINT32 dwEventCodes[] = { EVENT_NODE_NORMAL, EVENT_NODE_WARNING,
      EVENT_NODE_MINOR, EVENT_NODE_MAJOR, EVENT_NODE_CRITICAL,
      EVENT_NODE_UNKNOWN, EVENT_NODE_UNMANAGED };

   DataCollectionTarget::calculateCompoundStatus(bForcedRecalc);
   if (m_iStatus != iOldStatus)
      PostEvent(dwEventCodes[m_iStatus], m_id, "d", iOldStatus);
}

/**
 * Perform status poll on node
 */
void Node::statusPoll(ClientSession *pSession, UINT32 dwRqId, int nPoller)
{
	if (m_dwDynamicFlags & NDF_DELETE_IN_PROGRESS)
	{
		if (dwRqId == 0)
			m_dwDynamicFlags &= ~NDF_QUEUED_FOR_STATUS_POLL;
		return;
	}

   UINT32 i, dwPollListSize, dwOldFlags = m_dwFlags;
   NetObj *pPollerNode = NULL, **ppPollList;
   BOOL bAllDown;
	SNMP_Transport *pTransport;
	Cluster *pCluster;
   time_t tNow, tExpire;

   Queue *pQueue = new Queue;     // Delayed event queue
   SetPollerInfo(nPoller, _T("wait for lock"));
   pollerLock();
   m_pollRequestor = pSession;
   sendPollerMsg(dwRqId, _T("Starting status poll for node %s\r\n"), m_name);
   DbgPrintf(5, _T("Starting status poll for node %s (ID: %d)"), m_name, m_id);

   // Read capability expiration time and current time
   tExpire = (time_t)ConfigReadULong(_T("CapabilityExpirationTime"), 604800);
   tNow = time(NULL);

restart_agent_check:
   if (g_flags & AF_RESOLVE_IP_FOR_EACH_STATUS_POLL)
   {
      updatePrimaryIpAddr();
   }

   // Check SNMP agent connectivity
   if ((m_dwFlags & NF_IS_SNMP) && (!(m_dwFlags & NF_DISABLE_SNMP)) && m_ipAddress.isValidUnicast())
   {
      TCHAR szBuffer[256];
      UINT32 dwResult;

      DbgPrintf(6, _T("StatusPoll(%s): check SNMP"), m_name);
		pTransport = createSnmpTransport();
		if (pTransport != NULL)
      {
         SetPollerInfo(nPoller, _T("check SNMP"));
         sendPollerMsg(dwRqId, _T("Checking SNMP agent connectivity\r\n"));
         const TCHAR *testOid = m_customAttributes.get(_T("snmp.testoid"));
         if (testOid == NULL)
         {
            testOid = _T(".1.3.6.1.2.1.1.2.0");
         }
		   dwResult = SnmpGet(m_snmpVersion, pTransport, testOid, NULL, 0, szBuffer, sizeof(szBuffer), 0);
         if ((dwResult == SNMP_ERR_SUCCESS) || (dwResult == SNMP_ERR_NO_OBJECT))
         {
            if (m_dwDynamicFlags & NDF_SNMP_UNREACHABLE)
            {
               m_dwDynamicFlags &= ~NDF_SNMP_UNREACHABLE;
               PostEventEx(pQueue, EVENT_SNMP_OK, m_id, NULL);
               sendPollerMsg(dwRqId, POLLER_INFO _T("Connectivity with SNMP agent restored\r\n"));
            }

            // Update authoritative engine data for SNMPv3
            if ((pTransport->getSnmpVersion() == SNMP_VERSION_3) && (pTransport->getAuthoritativeEngine() != NULL))
            {
         		lockProperties();
               m_snmpSecurity->setAuthoritativeEngine(*pTransport->getAuthoritativeEngine());
               unlockProperties();
            }
         }
         else
         {
            sendPollerMsg(dwRqId, POLLER_ERROR _T("SNMP agent unreachable\r\n"));
            if (m_dwDynamicFlags & NDF_SNMP_UNREACHABLE)
            {
               if ((tNow > m_failTimeSNMP + tExpire) &&
                   (!(m_dwDynamicFlags & NDF_UNREACHABLE)))
               {
                  m_dwFlags &= ~NF_IS_SNMP;
                  m_dwDynamicFlags &= ~NDF_SNMP_UNREACHABLE;
                  m_szObjectId[0] = 0;
                  sendPollerMsg(dwRqId, POLLER_WARNING _T("Attribute isSNMP set to FALSE\r\n"));
               }
            }
            else
            {
               m_dwDynamicFlags |= NDF_SNMP_UNREACHABLE;
               PostEventEx(pQueue, EVENT_SNMP_FAIL, m_id, NULL);
               m_failTimeSNMP = tNow;
            }
         }
		   delete pTransport;
      }
      else
      {
	      DbgPrintf(6, _T("StatusPoll(%s): cannot create SNMP transport"), m_name);
      }
      DbgPrintf(6, _T("StatusPoll(%s): SNMP check finished"), m_name);
   }

   // Check native agent connectivity
   if ((m_dwFlags & NF_IS_NATIVE_AGENT) && (!(m_dwFlags & NF_DISABLE_NXCP)) && m_ipAddress.isValidUnicast())
   {
      DbgPrintf(6, _T("StatusPoll(%s): checking agent"), m_name);
      SetPollerInfo(nPoller, _T("check agent"));
      sendPollerMsg(dwRqId, _T("Checking NetXMS agent connectivity\r\n"));

		UINT32 error, socketError;
		agentLock();
      if (connectToAgent(&error, &socketError))
      {
         DbgPrintf(7, _T("StatusPoll(%s): connected to agent"), m_name);
         if (m_dwDynamicFlags & NDF_AGENT_UNREACHABLE)
         {
            m_dwDynamicFlags &= ~NDF_AGENT_UNREACHABLE;
            PostEventEx(pQueue, EVENT_AGENT_OK, m_id, NULL);
            sendPollerMsg(dwRqId, POLLER_INFO _T("Connectivity with NetXMS agent restored\r\n"));
         }
      }
      else
      {
         DbgPrintf(6, _T("StatusPoll(%s): agent unreachable, error=%d, socketError=%d"), m_name, (int)error, (int)socketError);
         sendPollerMsg(dwRqId, POLLER_ERROR _T("NetXMS agent unreachable\r\n"));
         if (m_dwDynamicFlags & NDF_AGENT_UNREACHABLE)
         {
            if ((tNow > m_failTimeAgent + tExpire) && !(m_dwDynamicFlags & NDF_UNREACHABLE))
            {
               m_dwFlags &= ~NF_IS_NATIVE_AGENT;
               m_dwDynamicFlags &= ~NDF_AGENT_UNREACHABLE;
               m_szPlatformName[0] = 0;
               m_szAgentVersion[0] = 0;
               sendPollerMsg(dwRqId, POLLER_WARNING _T("Attribute isNetXMSAgent set to FALSE\r\n"));
            }
         }
         else
         {
            m_dwDynamicFlags |= NDF_AGENT_UNREACHABLE;
            PostEventEx(pQueue, EVENT_AGENT_FAIL, m_id, NULL);
            m_failTimeAgent = tNow;
            //cancel file monitoring locally(on agent it is canceled if agent have fallen)
            g_monitoringList.removeDisconectedNode(m_id);
         }
      }
		agentUnlock();
      DbgPrintf(7, _T("StatusPoll(%s): agent check finished"), m_name);
   }

   SetPollerInfo(nPoller, _T("prepare polling list"));

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
   ppPollList = (NetObj **)malloc(sizeof(NetObj *) * m_dwChildCount);
   LockChildList(FALSE);
   for(i = 0, dwPollListSize = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i]->Status() != STATUS_UNMANAGED)
      {
         m_pChildList[i]->incRefCount();
         ppPollList[dwPollListSize++] = m_pChildList[i];
      }
   UnlockChildList();

   // Poll interfaces and services
   SetPollerInfo(nPoller, _T("child poll"));
   DbgPrintf(7, _T("StatusPoll(%s): starting child object poll"), m_name);
	pCluster = getMyCluster();
	pTransport = createSnmpTransport();
   for(i = 0; i < dwPollListSize; i++)
   {
      switch(ppPollList[i]->getObjectClass())
      {
         case OBJECT_INTERFACE:
			   DbgPrintf(7, _T("StatusPoll(%s): polling interface %d [%s]"), m_name, ppPollList[i]->getId(), ppPollList[i]->getName());
            ((Interface *)ppPollList[i])->statusPoll(pSession, dwRqId, pQueue, pCluster, pTransport, m_icmpProxy);
            break;
         case OBJECT_NETWORKSERVICE:
			   DbgPrintf(7, _T("StatusPoll(%s): polling network service %d [%s]"), m_name, ppPollList[i]->getId(), ppPollList[i]->getName());
            ((NetworkService *)ppPollList[i])->statusPoll(pSession, dwRqId,
                                                          (Node *)pPollerNode, pQueue);
            break;
         case OBJECT_ACCESSPOINT:
			   DbgPrintf(7, _T("StatusPoll(%s): polling access point %d [%s]"), m_name, ppPollList[i]->getId(), ppPollList[i]->getName());
            ((AccessPoint *)ppPollList[i])->statusPoll(pSession, dwRqId, pQueue, this);
            break;
         default:
            DbgPrintf(7, _T("StatusPoll(%s): skipping object %d [%s] class %d"), m_name, ppPollList[i]->getId(), ppPollList[i]->getName(), ppPollList[i]->getObjectClass());
            break;
      }
      ppPollList[i]->decRefCount();
   }
	delete pTransport;
   safe_free(ppPollList);
   DbgPrintf(7, _T("StatusPoll(%s): finished child object poll"), m_name);

   // Check if entire node is down
	// This check is disabled for nodes without IP address
	if (m_ipAddress.isValidUnicast())
	{
		LockChildList(FALSE);
		if (m_dwChildCount > 0)
		{
			for(i = 0, bAllDown = TRUE; i < m_dwChildCount; i++)
				if ((m_pChildList[i]->getObjectClass() == OBJECT_INTERFACE) &&
					 (m_pChildList[i]->Status() != STATUS_CRITICAL) &&
					 (m_pChildList[i]->Status() != STATUS_UNKNOWN) &&
					 (m_pChildList[i]->Status() != STATUS_UNMANAGED) &&
					 (m_pChildList[i]->Status() != STATUS_DISABLED))
				{
					bAllDown = FALSE;
					break;
				}
		}
		else
		{
			bAllDown = FALSE;
		}
		UnlockChildList();
		if (bAllDown && (m_dwFlags & NF_IS_NATIVE_AGENT) &&
		    (!(m_dwFlags & NF_DISABLE_NXCP)))
		   if (!(m_dwDynamicFlags & NDF_AGENT_UNREACHABLE))
		      bAllDown = FALSE;
		if (bAllDown && (m_dwFlags & NF_IS_SNMP) &&
		    (!(m_dwFlags & NF_DISABLE_SNMP)))
		   if (!(m_dwDynamicFlags & NDF_SNMP_UNREACHABLE))
		      bAllDown = FALSE;

		DbgPrintf(6, _T("StatusPoll(%s): bAllDown=%s, dynFlags=0x%08X"), m_name, bAllDown ? _T("true") : _T("false"), m_dwDynamicFlags);
		if (bAllDown)
		{
		   if (!(m_dwDynamicFlags & NDF_UNREACHABLE))
		   {
		      m_dwDynamicFlags |= NDF_UNREACHABLE;
				m_downSince = time(NULL);
			   SetPollerInfo(nPoller, _T("check network path"));
				if (checkNetworkPath(dwRqId))
				{
			      m_dwDynamicFlags |= NDF_NETWORK_PATH_PROBLEM;

					// Set interfaces and network services to UNKNOWN state
					LockChildList(FALSE);
					for(i = 0, bAllDown = TRUE; i < m_dwChildCount; i++)
						if (((m_pChildList[i]->getObjectClass() == OBJECT_INTERFACE) || (m_pChildList[i]->getObjectClass() == OBJECT_NETWORKSERVICE)) &&
							 (m_pChildList[i]->Status() == STATUS_CRITICAL))
						{
							m_pChildList[i]->resetStatus();
						}
					UnlockChildList();

					// Clear delayed event queue
					while(1)
					{
						Event *pEvent = (Event *)pQueue->Get();
						if (pEvent == NULL)
							break;
						delete pEvent;
					}
					delete_and_null(pQueue);

					PostEvent(EVENT_NODE_UNREACHABLE, m_id, NULL);
				}
				else
				{
					PostEvent(EVENT_NODE_DOWN, m_id, NULL);
				}
		      sendPollerMsg(dwRqId, POLLER_ERROR _T("Node is unreachable\r\n"));
		   }
		   else
		   {
		      sendPollerMsg(dwRqId, POLLER_WARNING _T("Node is still unreachable\r\n"));
		   }
		}
		else
		{
			m_downSince = 0;
		   if (m_dwDynamicFlags & NDF_UNREACHABLE)
		   {
            int reason = (m_dwDynamicFlags & NDF_NETWORK_PATH_PROBLEM) ? 1 : 0;
		      m_dwDynamicFlags &= ~(NDF_UNREACHABLE | NDF_SNMP_UNREACHABLE | NDF_AGENT_UNREACHABLE | NDF_NETWORK_PATH_PROBLEM);
		      PostEvent(EVENT_NODE_UP, m_id, "d", reason);
		      sendPollerMsg(dwRqId, POLLER_INFO _T("Node recovered from unreachable state\r\n"));
				goto restart_agent_check;
		   }
		   else
		   {
		      sendPollerMsg(dwRqId, POLLER_INFO _T("Node is connected\r\n"));
		   }
		}
	}

   // Get uptime and update boot time
   if (!(m_dwDynamicFlags & NDF_UNREACHABLE))
   {
      TCHAR buffer[MAX_RESULT_LENGTH];
      if (getItemFromAgent(_T("System.Uptime"), MAX_RESULT_LENGTH, buffer) == DCE_SUCCESS)
      {
         m_bootTime = time(NULL) - _tcstol(buffer, NULL, 0);
			DbgPrintf(5, _T("StatusPoll(%s [%d]): boot time set to %u from agent"), m_name, m_id, (UINT32)m_bootTime);
      }
      else if (getItemFromSNMP(m_wSNMPPort, _T(".1.3.6.1.2.1.1.3.0"), MAX_RESULT_LENGTH, buffer, SNMP_RAWTYPE_NONE) == DCE_SUCCESS)
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
   if (!(m_dwDynamicFlags & NDF_UNREACHABLE))
   {
      TCHAR buffer[MAX_RESULT_LENGTH];
      if (getItemFromAgent(_T("Agent.Uptime"), MAX_RESULT_LENGTH, buffer) == DCE_SUCCESS)
      {
         time_t oldAgentuptime = m_agentUpTime;
         m_agentUpTime = _tcstol(buffer, NULL, 0);
         if((UINT32)oldAgentuptime > (UINT32)m_agentUpTime)
         {
            //cancel file monitoring locally(on agent it is canceled if agent have fallen)
            g_monitoringList.removeDisconectedNode(m_id);
         }
      }
      else
      {
         DbgPrintf(5, _T("StatusPoll(%s [%d]): unable to get agent uptime"), m_name, m_id);
         g_monitoringList.removeDisconectedNode(m_id);
         m_agentUpTime = 0;
      }
   }
   else
   {
      g_monitoringList.removeDisconectedNode(m_id);
      m_agentUpTime = 0;
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
			g_pModuleList[i].pfStatusPollHook(this, pSession, dwRqId, nPoller);
		}
	}

	// Execute hook script
   SetPollerInfo(nPoller, _T("hook"));
	executeHookScript(_T("StatusPoll"));

   SetPollerInfo(nPoller, _T("cleanup"));
   if (pPollerNode != NULL)
      pPollerNode->decRefCount();

   if (dwOldFlags != m_dwFlags)
   {
      PostEvent(EVENT_NODE_FLAGS_CHANGED, m_id, "xx", dwOldFlags, m_dwFlags);
      lockProperties();
      setModified();
      unlockProperties();
   }

   calculateCompoundStatus();
   m_lastStatusPoll = time(NULL);
   sendPollerMsg(dwRqId, _T("Finished status poll for node %s\r\n"), m_name);
   sendPollerMsg(dwRqId, _T("Node status after poll is %s\r\n"), GetStatusAsText(m_iStatus, true));
   m_pollRequestor = NULL;
   if (dwRqId == 0)
      m_dwDynamicFlags &= ~NDF_QUEUED_FOR_STATUS_POLL;
   pollerUnlock();
   DbgPrintf(5, _T("Finished status poll for node %s (ID: %d)"), m_name, m_id);
}

/**
 * Check single element of network path
 */
bool Node::checkNetworkPathElement(UINT32 nodeId, const TCHAR *nodeType, bool isProxy, UINT32 dwRqId)
{
	Node *node = (Node *)FindObjectById(nodeId, OBJECT_NODE);
	if (node == NULL)
		return false;

	DbgPrintf(6, _T("Node::checkNetworkPathElement(%s [%d]): found %s: %s [%d]"), m_name, m_id, nodeType, node->getName(), node->getId());
	if (node->isDown())
	{
		DbgPrintf(5, _T("Node::checkNetworkPathElement(%s [%d]): %s %s [%d] is down"),
					 m_name, m_id, nodeType, node->getName(), node->getId());
		sendPollerMsg(dwRqId, POLLER_WARNING _T("   %s %s is down\r\n"), nodeType, node->getName());
		return true;
	}
	if (isProxy && node->isNativeAgent() && (node->getRuntimeFlags() & NDF_AGENT_UNREACHABLE))
	{
		DbgPrintf(5, _T("Node::checkNetworkPathElement(%s [%d]): agent on %s %s [%d] is down"),
					 m_name, m_id, nodeType, node->getName(), node->getId());
		sendPollerMsg(dwRqId, POLLER_WARNING _T("   Agent on %s %s is down\r\n"), nodeType, node->getName());
		return true;
	}
	if (node->m_lastStatusPoll < time(NULL) - 1)
	{
		DbgPrintf(6, _T("Node::checkNetworkPathElement(%s [%d]): forced status poll on node %s [%d]"),
					 m_name, m_id, node->getName(), node->getId());
		node->statusPoll(NULL, 0, 0);
		if (node->isDown())
		{
			DbgPrintf(5, _T("Node::checkNetworkPathElement(%s [%d]): %s %s [%d] is down"),
						 m_name, m_id, nodeType, node->getName(), node->getId());
			sendPollerMsg(dwRqId, POLLER_WARNING _T("   %s %s is down\r\n"), nodeType, node->getName());
			return true;
		}
		if (isProxy && node->isNativeAgent() && (node->getRuntimeFlags() & NDF_AGENT_UNREACHABLE))
		{
			DbgPrintf(5, _T("Node::checkNetworkPathElement(%s [%d]): agent on %s %s [%d] is down"),
						 m_name, m_id, nodeType, node->getName(), node->getId());
			sendPollerMsg(dwRqId, POLLER_WARNING _T("   Agent on %s %s is down\r\n"), nodeType, node->getName());
			return true;
		}
	}
	return false;
}

/**
 * Check network path between node and management server to detect possible intermediate node failure
 *
 * @return true if network path problems found
 */
bool Node::checkNetworkPath(UINT32 dwRqId)
{
	time_t now = time(NULL);

	// Check proxy node(s)
	if (IsZoningEnabled() && (m_zoneId != 0))
	{
		Zone *zone = (Zone *)g_idxZoneByGUID.get(m_zoneId);
		if ((zone != NULL) && ((zone->getAgentProxy() != 0) || (zone->getSnmpProxy() != 0) || (zone->getIcmpProxy() != 0)))
		{
			bool allProxyDown = true;
			if (zone->getIcmpProxy() != 0)
				allProxyDown = checkNetworkPathElement(zone->getIcmpProxy(), _T("ICMP proxy"), true, dwRqId);
			if (allProxyDown && (zone->getSnmpProxy() != 0) && (zone->getSnmpProxy() != zone->getIcmpProxy()))
				allProxyDown = checkNetworkPathElement(zone->getSnmpProxy(), _T("SNMP proxy"), true, dwRqId);
			if (allProxyDown && (zone->getAgentProxy() != 0) && (zone->getAgentProxy() != zone->getIcmpProxy()) && (zone->getAgentProxy() != zone->getSnmpProxy()))
				allProxyDown = checkNetworkPathElement(zone->getAgentProxy(), _T("agent proxy"), true, dwRqId);
			if (allProxyDown)
				return true;
		}
	}

	// Check directly connected switch
   sendPollerMsg(dwRqId, _T("Checking ethernet connectivity...\r\n"));
	Interface *iface = findInterfaceByIP(m_ipAddress);
	if ((iface != NULL) && (iface->getPeerNodeId() != 0))
	{
		DbgPrintf(6, _T("Node::checkNetworkPath(%s [%d]): found interface object for primary IP: %s [%d]"), m_name, m_id, iface->getName(), iface->getId());
		if (checkNetworkPathElement(iface->getPeerNodeId(), _T("upstream switch"), false, dwRqId))
			return true;
	}
	else
	{
		DbgPrintf(5, _T("Node::checkNetworkPath(%s [%d]): cannot find interface object for primary IP"), m_name, m_id);
	}

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
   sendPollerMsg(dwRqId, _T("Checking network path...\r\n"));
	bool secondPass = false;
	bool pathProblemFound = false;
restart:
   for(int i = 0; i < trace->getHopCount(); i++)
   {
		HOP_INFO *hop = trace->getHopInfo(i);
      if ((hop->object == NULL) || (hop->object == this) || (hop->object->getObjectClass() != OBJECT_NODE))
			continue;

		DbgPrintf(6, _T("Node::checkNetworkPath(%s [%d]): checking upstream node %s [%d]"),
		          m_name, m_id, hop->object->getName(), hop->object->getId());
      if (secondPass && !((Node *)hop->object)->isDown() && (((Node *)hop->object)->m_lastStatusPoll < now - 1))
		{
			DbgPrintf(6, _T("Node::checkNetworkPath(%s [%d]): forced status poll on node %s [%d]"),
			          m_name, m_id, hop->object->getName(), hop->object->getId());
			((Node *)hop->object)->statusPoll(NULL, 0, 0);
		}

      if (((Node *)hop->object)->isDown())
      {
			DbgPrintf(5, _T("Node::checkNetworkPath(%s [%d]): upstream node %s [%d] is down"),
			          m_name, m_id, hop->object->getName(), hop->object->getId());
			sendPollerMsg(dwRqId, POLLER_WARNING _T("   Upstream node %s is down\r\n"), hop->object->getName());
			pathProblemFound = true;
			break;
      }
   }
	if (!secondPass && !pathProblemFound)
	{
		DbgPrintf(5, _T("Node::checkNetworkPath(%s [%d]): will do second pass"), m_name, m_id);
		secondPass = true;
		goto restart;
	}
   delete trace;
	return pathProblemFound;
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
			uuid_t guid;
			ap->getGuid(i, guid);
			NetObj *object = FindObjectByGUID(guid, -1);
			if ((object != NULL) && (!object->isChild(m_id)))
			{
				object->AddChild(this);
				AddParent(object);
				DbgPrintf(5, _T("ConfPoll(%s): bound to policy object %s [%d]"), m_name, object->getName(), object->getId());
			}
		}

		// Check for bound but not installed policies
		LockParentList(FALSE);
		NetObj **unbindList = (NetObj **)malloc(sizeof(NetObj *) * m_dwParentCount);
		int unbindListSize = 0;
		for(UINT32 i = 0; i < m_dwParentCount; i++)
		{
			if (IsAgentPolicyObject(m_pParentList[i]))
			{
				uuid_t guid1, guid2;
				int j;

				m_pParentList[i]->getGuid(guid1);
				for(j = 0; j < ap->size(); j++)
				{
					ap->getGuid(j, guid2);
					if (!uuid_compare(guid1, guid2))
						break;
				}
				if (j == ap->size())
					unbindList[unbindListSize++] = m_pParentList[i];
			}
		}
		UnlockParentList();

		for(int i = 0; i < unbindListSize; i++)
		{
			unbindList[i]->DeleteChild(this);
			DeleteParent(unbindList[i]);
			DbgPrintf(5, _T("ConfPoll(%s): unbound from policy object %s [%d]"), m_name, unbindList[i]->getName(), unbindList[i]->getId());
		}
		safe_free(unbindList);

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

   InetAddress ipAddr = InetAddress::parse(m_primaryName);
   if (!ipAddr.isValid() && (m_zoneId != 0))
   {
      // resolve address through proxy agent
      Zone *zone = FindZoneByGUID(m_zoneId);
      if (zone != NULL)
      {
         Node *proxy = (Node *)FindObjectById(zone->getAgentProxy(), OBJECT_NODE);
         if (proxy != NULL)
         {
            TCHAR query[256], buffer[128];
            _sntprintf(query, 256, _T("Net.Resolver.AddressByName(%s)"), m_primaryName);
            if (proxy->getItemFromAgent(query, 128, buffer) == ERR_SUCCESS)
            {
               ipAddr = InetAddress::parse(buffer);
            }
         }
      }
   }

   // Resolve address through local resolver
   if (!ipAddr.isValid())
   {
      ipAddr = InetAddress::resolveHostName(m_primaryName);
   }

   if (!ipAddr.equals(m_ipAddress) && (ipAddr.isValidUnicast() || !_tcscmp(m_primaryName, _T("0.0.0.0"))))
	{
		TCHAR buffer1[64], buffer2[64];

		DbgPrintf(4, _T("IP address for node %s [%d] changed from %s to %s"),
         m_name, (int)m_id, m_ipAddress.toString(buffer1), ipAddr.toString(buffer2));
		PostEvent(EVENT_IP_ADDRESS_CHANGED, m_id, "AA", &ipAddr, &m_ipAddress);

      if (m_dwFlags & NF_REMOTE_AGENT)
      {
         lockProperties();
         m_ipAddress = ipAddr;
         setModified();
         unlockProperties();
      }
      else
      {
		   setPrimaryIPAddress(ipAddr);
      }

		agentLock();
		delete_and_null(m_pAgentConnection);
		agentUnlock();
	}
}

/**
 * Perform configuration poll on node
 */
void Node::configurationPoll(ClientSession *pSession, UINT32 dwRqId, int nPoller, int maskBits)
{
	if (m_dwDynamicFlags & NDF_DELETE_IN_PROGRESS)
	{
		if (dwRqId == 0)
			m_dwDynamicFlags &= ~NDF_QUEUED_FOR_CONFIG_POLL;
		return;
	}

   UINT32 dwOldFlags = m_dwFlags;
   TCHAR szBuffer[4096];
   bool hasChanges = false;

   SetPollerInfo(nPoller, _T("wait for lock"));
   pollerLock();
   m_pollRequestor = pSession;
   sendPollerMsg(dwRqId, _T("Starting configuration poll for node %s\r\n"), m_name);
   DbgPrintf(4, _T("Starting configuration poll for node %s (ID: %d)"), m_name, m_id);

   // Check for forced capabilities recheck
   if (m_dwDynamicFlags & NDF_RECHECK_CAPABILITIES)
   {
      sendPollerMsg(dwRqId, POLLER_WARNING _T("Capability reset\r\n"));
      m_dwFlags &= ~(NF_IS_NATIVE_AGENT | NF_IS_SNMP | NF_IS_CPSNMP |
                     NF_IS_BRIDGE | NF_IS_ROUTER | NF_IS_OSPF | NF_IS_PRINTER |
							NF_IS_CDP | NF_IS_LLDP | NF_IS_SONMP | NF_IS_VRRP | NF_HAS_VLANS |
							NF_IS_8021X | NF_IS_STP | NF_HAS_ENTITY_MIB | NF_HAS_IFXTABLE |
							NF_HAS_WINPDH);
		m_dwDynamicFlags &= ~NDF_CONFIGURATION_POLL_PASSED;
      m_szObjectId[0] = 0;
      m_szPlatformName[0] = 0;
      m_szAgentVersion[0] = 0;
		safe_free_and_null(m_sysDescription);
		safe_free_and_null(m_sysName);
		safe_free_and_null(m_lldpNodeId);
   }

   // Check if node is marked as unreachable
   if ((m_dwDynamicFlags & NDF_UNREACHABLE) && !(m_dwDynamicFlags & NDF_RECHECK_CAPABILITIES))
   {
      sendPollerMsg(dwRqId, POLLER_WARNING _T("Node is marked as unreachable, configuration poll aborted\r\n"));
      DbgPrintf(4, _T("Node is marked as unreachable, configuration poll aborted"));
      m_lastConfigurationPoll = time(NULL);
   }
   else
   {
		updatePrimaryIpAddr();

      SetPollerInfo(nPoller, _T("capability check"));
      sendPollerMsg(dwRqId, _T("Checking node's capabilities...\r\n"));

		if (confPollAgent(dwRqId))
			hasChanges = true;
		if (confPollSnmp(dwRqId))
			hasChanges = true;

      // Check for CheckPoint SNMP agent on port 260
      if (ConfigReadInt(_T("EnableCheckPointSNMP"), 0))
      {
         DbgPrintf(5, _T("ConfPoll(%s): checking for CheckPoint SNMP on port 260"), m_name);
         if (!((m_dwFlags & NF_IS_CPSNMP) && (m_dwDynamicFlags & NDF_CPSNMP_UNREACHABLE)) && m_ipAddress.isValidUnicast())
         {
			   SNMP_Transport *pTransport = new SNMP_UDPTransport;
			   ((SNMP_UDPTransport *)pTransport)->createUDPTransport(m_ipAddress, CHECKPOINT_SNMP_PORT);
            if (SnmpGet(SNMP_VERSION_1, pTransport,
                        _T(".1.3.6.1.4.1.2620.1.1.10.0"), NULL, 0, szBuffer, sizeof(szBuffer), 0) == SNMP_ERR_SUCCESS)
            {
               lockProperties();
               m_dwFlags |= NF_IS_CPSNMP | NF_IS_ROUTER;
               m_dwDynamicFlags &= ~NDF_CPSNMP_UNREACHABLE;
               unlockProperties();
               sendPollerMsg(dwRqId, POLLER_INFO _T("   CheckPoint SNMP agent on port 260 is active\r\n"));
            }
			   delete pTransport;
         }
      }

      // Generate event if node flags has been changed
      if (dwOldFlags != m_dwFlags)
      {
         PostEvent(EVENT_NODE_FLAGS_CHANGED, m_id, "xx", dwOldFlags, m_dwFlags);
         hasChanges = true;
      }

      // Retrieve interface list
      SetPollerInfo(nPoller, _T("interface check"));
      sendPollerMsg(dwRqId, _T("Capability check finished\r\n"));

		if (updateInterfaceConfiguration(dwRqId, maskBits))
			hasChanges = true;

      m_lastConfigurationPoll = time(NULL);

		// Check node name
		sendPollerMsg(dwRqId, _T("Checking node name\r\n"));
		UINT32 dwAddr = ntohl(_t_inet_addr(m_name));
		if ((g_flags & AF_RESOLVE_NODE_NAMES) &&
			 (dwAddr != INADDR_NONE) &&
			 (dwAddr != INADDR_ANY) &&
			 isMyIP(dwAddr))
		{
			sendPollerMsg(dwRqId, _T("Node name is an IP address and need to be resolved\r\n"));
	      SetPollerInfo(nPoller, _T("resolving name"));
			if (resolveName(FALSE))
			{
				sendPollerMsg(dwRqId, POLLER_INFO _T("Node name resolved to %s\r\n"), m_name);
				hasChanges = true;
			}
			else
			{
				sendPollerMsg(dwRqId, POLLER_WARNING _T("Node name cannot be resolved\r\n"));
			}
		}
		else
		{
			if (g_flags & AF_SYNC_NODE_NAMES_WITH_DNS)
			{
				sendPollerMsg(dwRqId, _T("Syncing node name with DNS\r\n"));
		      SetPollerInfo(nPoller, _T("resolving name"));
				if (resolveName(TRUE))
				{
					sendPollerMsg(dwRqId, POLLER_INFO _T("Node name resolved to %s\r\n"), m_name);
					hasChanges = true;
				}
			}
			else
			{
				sendPollerMsg(dwRqId, _T("Node name is OK\r\n"));
			}
		}

		applyUserTemplates();
		updateContainerMembership();

		// Get list of installed products
		if (m_dwFlags & NF_IS_NATIVE_AGENT)
		{
			SetPollerInfo(nPoller, _T("software check"));
			sendPollerMsg(dwRqId, _T("Reading list of installed software packages\r\n"));

			Table *table;
			if (getTableFromAgent(_T("System.InstalledProducts"), &table) == DCE_SUCCESS)
			{
				lockProperties();
				delete m_softwarePackages;
				m_softwarePackages = new ObjectArray<SoftwarePackage>(table->getNumRows(), 16, true);
				for(int i = 0; i < table->getNumRows(); i++)
					m_softwarePackages->add(new SoftwarePackage(table, i));
				unlockProperties();
				delete table;
				sendPollerMsg(dwRqId, POLLER_INFO _T("Got information about %d installed software packages\r\n"), m_softwarePackages->size());
			}
			else
			{
				delete_and_null(m_softwarePackages);
				sendPollerMsg(dwRqId, POLLER_WARNING _T("Unable to get information about installed software packages\r\n"));
			}
		}

		sendPollerMsg(dwRqId, _T("Finished configuration poll for node %s\r\n"), m_name);
		sendPollerMsg(dwRqId, _T("Node configuration was%schanged after poll\r\n"), hasChanges ? _T(" ") : _T(" not "));

		// Call hooks in loaded modules
		for(UINT32 i = 0; i < g_dwNumModules; i++)
		{
			if (g_pModuleList[i].pfConfPollHook != NULL)
			{
				DbgPrintf(5, _T("ConfigurationPoll(%s [%d]): calling hook in module %s"), m_name, m_id, g_pModuleList[i].szName);
				if (g_pModuleList[i].pfConfPollHook(this, pSession, dwRqId, nPoller))
               hasChanges = true;
			}
		}

		// Execute hook script
		SetPollerInfo(nPoller, _T("hook"));
		executeHookScript(_T("ConfigurationPoll"));

		m_dwDynamicFlags |= NDF_CONFIGURATION_POLL_PASSED;
   }

   // Finish configuration poll
   SetPollerInfo(nPoller, _T("cleanup"));
   if (dwRqId == 0)
      m_dwDynamicFlags &= ~NDF_QUEUED_FOR_CONFIG_POLL;
   m_dwDynamicFlags &= ~NDF_RECHECK_CAPABILITIES;
   pollerUnlock();
   DbgPrintf(4, _T("Finished configuration poll for node %s (ID: %d)"), m_name, m_id);

   if (hasChanges)
   {
      lockProperties();
      setModified();
      unlockProperties();
   }
}

/**
 * Configuration poll: check for NetXMS agent
 */
bool Node::confPollAgent(UINT32 dwRqId)
{
   DbgPrintf(5, _T("ConfPoll(%s): checking for NetXMS agent Flags={%08X} DynamicFlags={%08X}"), m_name, m_dwFlags, m_dwDynamicFlags);
	if (((m_dwFlags & NF_IS_NATIVE_AGENT) && (m_dwDynamicFlags & NDF_AGENT_UNREACHABLE)) ||
	    !m_ipAddress.isValidUnicast() || (m_dwFlags & NF_DISABLE_NXCP))
		return false;

	bool hasChanges = false;

	sendPollerMsg(dwRqId, _T("   Checking NetXMS agent...\r\n"));
   AgentConnection *pAgentConn = new AgentConnectionEx(m_id, m_ipAddress, m_agentPort, m_agentAuthMethod, m_szSharedSecret);
   setAgentProxy(pAgentConn);
   DbgPrintf(5, _T("ConfPoll(%s): checking for NetXMS agent - connecting"), m_name);

   // Try to connect to agent
   UINT32 rcc;
   if (!pAgentConn->connect(g_pServerKey, FALSE, &rcc))
   {
      // If there are authentication problem, try default shared secret
      if ((rcc == ERR_AUTH_REQUIRED) || (rcc == ERR_AUTH_FAILED))
      {
         TCHAR secret[MAX_SECRET_LENGTH];
         ConfigReadStr(_T("AgentDefaultSharedSecret"), secret, MAX_SECRET_LENGTH, _T("netxms"));
         pAgentConn->setAuthData(AUTH_SHA1_HASH, secret);
         if (pAgentConn->connect(g_pServerKey, FALSE, &rcc))
         {
            m_agentAuthMethod = AUTH_SHA1_HASH;
            nx_strncpy(m_szSharedSecret, secret, MAX_SECRET_LENGTH);
            DbgPrintf(5, _T("ConfPoll(%s): checking for NetXMS agent - shared secret changed to system default"), m_name);
         }
      }
   }

   if (rcc == ERR_SUCCESS)
   {
      DbgPrintf(5, _T("ConfPoll(%s): checking for NetXMS agent - connected"), m_name);
      lockProperties();
      m_dwFlags |= NF_IS_NATIVE_AGENT;
      if (m_dwDynamicFlags & NDF_AGENT_UNREACHABLE)
      {
         m_dwDynamicFlags &= ~NDF_AGENT_UNREACHABLE;
         PostEvent(EVENT_AGENT_OK, m_id, NULL);
         sendPollerMsg(dwRqId, POLLER_INFO _T("   Connectivity with NetXMS agent restored\r\n"));
      }
		else
		{
         sendPollerMsg(dwRqId, POLLER_INFO _T("   NetXMS native agent is active\r\n"));
		}
      unlockProperties();

		TCHAR buffer[MAX_RESULT_LENGTH];
      if (pAgentConn->getParameter(_T("Agent.Version"), MAX_AGENT_VERSION_LEN, buffer) == ERR_SUCCESS)
      {
         lockProperties();
         if (_tcscmp(m_szAgentVersion, buffer))
         {
            _tcscpy(m_szAgentVersion, buffer);
            hasChanges = true;
            sendPollerMsg(dwRqId, _T("   NetXMS agent version changed to %s\r\n"), m_szAgentVersion);
         }
         unlockProperties();
      }

      if (pAgentConn->getParameter(_T("System.PlatformName"), MAX_PLATFORM_NAME_LEN, buffer) == ERR_SUCCESS)
      {
         lockProperties();
         if (_tcscmp(m_szPlatformName, buffer))
         {
            _tcscpy(m_szPlatformName, buffer);
            hasChanges = true;
            sendPollerMsg(dwRqId, _T("   Platform name changed to %s\r\n"), m_szPlatformName);
         }
         unlockProperties();
      }

      // Check IP forwarding status
      if (pAgentConn->getParameter(_T("Net.IP.Forwarding"), 16, buffer) == ERR_SUCCESS)
      {
         if (_tcstoul(buffer, NULL, 10) != 0)
            m_dwFlags |= NF_IS_ROUTER;
         else
            m_dwFlags &= ~NF_IS_ROUTER;
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
				safe_free(m_sysDescription);
            m_sysDescription = _tcsdup(buffer);
            hasChanges = true;
            sendPollerMsg(dwRqId, _T("   System description changed to %s\r\n"), m_sysDescription);
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
			delete m_paramList;
			delete m_tableList;
			m_paramList = plist;
			m_tableList = tlist;

			// Check for 64-bit interface counters
			m_dwFlags &= ~NF_HAS_AGENT_IFXCOUNTERS;
         if (netIf64bitCounters)
         {
			   for(int i = 0; i < plist->size(); i++)
			   {
				   if (!_tcsicmp(plist->get(i)->getName(), _T("Net.Interface.BytesIn64(*)")))
				   {
					   m_dwFlags |= NF_HAS_AGENT_IFXCOUNTERS;
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
		if (!_tcsncmp(m_szPlatformName, _T("windows-"), 8))
		{
         sendPollerMsg(dwRqId, _T("   Reading list of available Windows Performance Counters...\r\n"));
			ObjectArray<WinPerfObject> *perfObjects = WinPerfObject::getWinPerfObjectsFromNode(this, pAgentConn);
			lockProperties();
			delete m_winPerfObjects;
			m_winPerfObjects = perfObjects;
			if (m_winPerfObjects != NULL)
			{
				sendPollerMsg(dwRqId, POLLER_INFO _T("   %d counters read\r\n"), m_winPerfObjects->size());
				if (!(m_dwFlags & NF_HAS_WINPDH))
				{
					m_dwFlags |= NF_HAS_WINPDH;
					hasChanges = true;
				}
			}
			else
			{
				sendPollerMsg(dwRqId, POLLER_ERROR _T("   unable to get Windows Performance Counters list\r\n"));
				if (m_dwFlags & NF_HAS_WINPDH)
				{
					m_dwFlags &= ~NF_HAS_WINPDH;
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
      DbgPrintf(5, _T("ConfPoll(%s): checking for NetXMS agent - failed to connect (error %d)"), m_name, rcc);
	}
   delete pAgentConn;
   DbgPrintf(5, _T("ConfPoll(%s): checking for NetXMS agent - finished"), m_name);
	return hasChanges;
}

/**
 * SNMP walker callback which just counts number of varbinds
 */
static UINT32 CountingSnmpWalkerCallback(UINT32 version, SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
	(*((int *)arg))++;
	return SNMP_ERR_SUCCESS;
}

/**
 * Configuration poll: check for SNMP
 */
bool Node::confPollSnmp(UINT32 dwRqId)
{
	if (((m_dwFlags & NF_IS_SNMP) && (m_dwDynamicFlags & NDF_SNMP_UNREACHABLE)) ||
	    !m_ipAddress.isValidUnicast() || (m_dwFlags & NF_DISABLE_SNMP))
		return false;

	bool hasChanges = false;

   sendPollerMsg(dwRqId, _T("   Checking SNMP...\r\n"));
   DbgPrintf(5, _T("ConfPoll(%s): calling SnmpCheckCommSettings()"), m_name);
	SNMP_Transport *pTransport = createSnmpTransport();
	if (pTransport == NULL)
	{
		DbgPrintf(5, _T("ConfPoll(%s): unable to create SNMP transport"), m_name);
		return false;
	}

   StringList oids;
   const TCHAR *customOid = m_customAttributes.get(_T("snmp.testoid"));
   if (customOid != NULL)
      oids.add(customOid);
   oids.add(_T(".1.3.6.1.2.1.1.2.0"));
   oids.add(_T(".1.3.6.1.2.1.1.1.0"));
   AddDriverSpecificOids(&oids);
   SNMP_SecurityContext *newCtx = SnmpCheckCommSettings(pTransport, &m_snmpVersion, m_snmpSecurity, &oids);
	if (newCtx != NULL)
   {
      lockProperties();
		delete m_snmpSecurity;
		m_snmpSecurity = newCtx;
      m_dwFlags |= NF_IS_SNMP;
      if (m_dwDynamicFlags & NDF_SNMP_UNREACHABLE)
      {
         m_dwDynamicFlags &= ~NDF_SNMP_UNREACHABLE;
         PostEvent(EVENT_SNMP_OK, m_id, NULL);
         sendPollerMsg(dwRqId, POLLER_INFO _T("   Connectivity with SNMP agent restored\r\n"));
      }
		unlockProperties();
      sendPollerMsg(dwRqId, _T("   SNMP agent is active (version %s)\r\n"),
			(m_snmpVersion == SNMP_VERSION_3) ? _T("3") : ((m_snmpVersion == SNMP_VERSION_2C) ? _T("2c") : _T("1")));

		TCHAR szBuffer[4096];
		if (SnmpGet(m_snmpVersion, pTransport, _T(".1.3.6.1.2.1.1.2.0"), NULL, 0, szBuffer, sizeof(szBuffer), SG_STRING_RESULT) != SNMP_ERR_SUCCESS)
		{
			// Set snmp object ID to .0.0 if it cannot be read
			_tcscpy(szBuffer, _T(".0.0"));
		}
		lockProperties();
		if (_tcscmp(m_szObjectId, szBuffer))
		{
			nx_strncpy(m_szObjectId, szBuffer, MAX_OID_LEN * 4);
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
				safe_free(m_sysDescription);
            m_sysDescription = _tcsdup(szBuffer);
            hasChanges = true;
            sendPollerMsg(dwRqId, _T("   System description changed to %s\r\n"), m_sysDescription);
         }
			unlockProperties();
		}

		// Select device driver
		NetworkDeviceDriver *driver = FindDriverForNode(this, pTransport);
		DbgPrintf(5, _T("ConfPoll(%s): selected device driver %s"), m_name, driver->getName());
		lockProperties();
		if (driver != m_driver)
		{
			m_driver = driver;
			sendPollerMsg(dwRqId, _T("   New network device driver selected: %s\r\n"), m_driver->getName());
		}
		unlockProperties();

		// Allow driver to gather additional info
		m_driver->analyzeDevice(pTransport, m_szObjectId, &m_customAttributes, &m_driverData);

		// Get sysName
		if (SnmpGet(m_snmpVersion, pTransport,
		            _T(".1.3.6.1.2.1.1.5.0"), NULL, 0, szBuffer, sizeof(szBuffer), SG_STRING_RESULT) == SNMP_ERR_SUCCESS)
		{
			lockProperties();
			if ((m_sysName == NULL) || _tcscmp(m_sysName, szBuffer))
         {
				safe_free(m_sysName);
            m_sysName = _tcsdup(szBuffer);
            hasChanges = true;
            sendPollerMsg(dwRqId, _T("   System name changed to %s\r\n"), m_sysName);
         }
			unlockProperties();
		}

      // Check IP forwarding
      if (checkSNMPIntegerValue(pTransport, _T(".1.3.6.1.2.1.4.1.0"), 1))
      {
			lockProperties();
         m_dwFlags |= NF_IS_ROUTER;
			unlockProperties();
      }
      else
      {
			lockProperties();
         m_dwFlags &= ~NF_IS_ROUTER;
			unlockProperties();
      }

		checkIfXTable(pTransport);
		checkBridgeMib(pTransport);

      // Check for ENTITY-MIB support
      if (SnmpGet(m_snmpVersion, pTransport, _T(".1.3.6.1.2.1.47.1.4.1.0"), NULL, 0, szBuffer, sizeof(szBuffer), SG_RAW_RESULT) == SNMP_ERR_SUCCESS)
      {
			lockProperties();
         m_dwFlags |= NF_HAS_ENTITY_MIB;
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
         m_dwFlags &= ~NF_HAS_ENTITY_MIB;
			if (m_components != NULL)
			{
				m_components->decRefCount();
				m_components = NULL;
			}
			unlockProperties();
      }

      // Check for printer MIB support
		int count = 0;
		SnmpWalk(m_snmpVersion, pTransport, _T(".1.3.6.1.2.1.43.5.1.1.17"), CountingSnmpWalkerCallback, &count, FALSE);
      if (count > 0)
      {
			lockProperties();
         m_dwFlags |= NF_IS_PRINTER;
			unlockProperties();
      }
      else
      {
			lockProperties();
         m_dwFlags &= ~NF_IS_PRINTER;
			unlockProperties();
      }

      // Check for CDP (Cisco Discovery Protocol) support
      if (checkSNMPIntegerValue(pTransport, _T(".1.3.6.1.4.1.9.9.23.1.3.1.0"), 1))
      {
			lockProperties();
         m_dwFlags |= NF_IS_CDP;
			unlockProperties();
      }
      else
      {
			lockProperties();
         m_dwFlags &= ~NF_IS_CDP;
			unlockProperties();
      }

      // Check for NDP (Nortel Discovery Protocol) support
      if (checkSNMPIntegerValue(pTransport, _T(".1.3.6.1.4.1.45.1.6.13.1.2.0"), 1))
      {
			lockProperties();
         m_dwFlags |= NF_IS_NDP;
			unlockProperties();
      }
      else
      {
			lockProperties();
         m_dwFlags &= ~NF_IS_NDP;
			unlockProperties();
      }

      // Check for LLDP (Link Layer Discovery Protocol) support
		if (SnmpGet(m_snmpVersion, pTransport, _T(".1.0.8802.1.1.2.1.3.2.0"), NULL, 0, szBuffer, sizeof(szBuffer), 0) == SNMP_ERR_SUCCESS)
		{
			lockProperties();
			m_dwFlags |= NF_IS_LLDP;
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
					safe_free(m_lldpNodeId);
					m_lldpNodeId = _tcsdup(szBuffer);
					hasChanges = true;
					sendPollerMsg(dwRqId, _T("   LLDP node ID changed to %s\r\n"), m_lldpNodeId);
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
			m_dwFlags &= ~NF_IS_LLDP;
			unlockProperties();
		}

      // Check for 802.1x support
      if (checkSNMPIntegerValue(pTransport, _T(".1.0.8802.1.1.1.1.1.1.0"), 1))
      {
			lockProperties();
         m_dwFlags |= NF_IS_8021X;
			unlockProperties();
      }
      else
      {
			lockProperties();
			m_dwFlags &= ~NF_IS_8021X;
			unlockProperties();
      }

      checkOSPFSupport(pTransport);

		// Get VRRP information
		VrrpInfo *vrrpInfo = GetVRRPInfo(this);
		if (vrrpInfo != NULL)
		{
			lockProperties();
			m_dwFlags |= NF_IS_VRRP;
			delete m_vrrpInfo;
			m_vrrpInfo = vrrpInfo;
			unlockProperties();
		}
		else
		{
			lockProperties();
			m_dwFlags &= ~NF_IS_VRRP;
			unlockProperties();
		}

		// Get wireless controller data
		if ((m_driver != NULL) && m_driver->isWirelessController(pTransport, &m_customAttributes, m_driverData))
		{
			DbgPrintf(5, _T("ConfPoll(%s): node is wireless controller, reading access point information"), m_name);
         sendPollerMsg(dwRqId, _T("   Reading wireless access point information\r\n"));
			lockProperties();
			m_dwFlags |= NF_IS_WIFI_CONTROLLER;
			unlockProperties();

         int clusterMode = m_driver->getClusterMode(pTransport, &m_customAttributes, m_driverData);

			ObjectArray<AccessPointInfo> *aps = m_driver->getAccessPoints(pTransport, &m_customAttributes, m_driverData);
			if (aps != NULL)
			{
				sendPollerMsg(dwRqId, POLLER_INFO _T("   %d wireless access points found\r\n"), aps->size());
				DbgPrintf(5, _T("ConfPoll(%s): got information about %d access points"), m_name, aps->size());
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
						ap = new AccessPoint((const TCHAR *)name, info->getMacAddr());
						NetObjInsert(ap, TRUE);
						DbgPrintf(5, _T("ConfPoll(%s): created new access point object %s [%d]"), m_name, ap->getName(), ap->getId());
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
				DbgPrintf(5, _T("ConfPoll(%s): failed to read access point information"), m_name);
				sendPollerMsg(dwRqId, POLLER_ERROR _T("   Failed to read access point information\r\n"));
			}
		}
		else
		{
			lockProperties();
			m_dwFlags &= ~NF_IS_WIFI_CONTROLLER;
			unlockProperties();
		}
   }
   else if (ConfigReadInt(_T("EnableCheckPointSNMP"), 0))
   {
      // Check for CheckPoint SNMP agent on port 161
      DbgPrintf(5, _T("ConfPoll(%s): checking for CheckPoint SNMP"), m_name);
		TCHAR szBuffer[4096];
		if (SnmpGet(SNMP_VERSION_1, pTransport, _T(".1.3.6.1.4.1.2620.1.1.10.0"), NULL, 0, szBuffer, sizeof(szBuffer), 0) == SNMP_ERR_SUCCESS)
      {
         lockProperties();
         if (_tcscmp(m_szObjectId, _T(".1.3.6.1.4.1.2620.1.1")))
         {
            nx_strncpy(m_szObjectId, _T(".1.3.6.1.4.1.2620.1.1"), MAX_OID_LEN * 4);
            hasChanges = true;
         }

         m_dwFlags |= NF_IS_SNMP | NF_IS_ROUTER;
         m_dwDynamicFlags &= ~NDF_SNMP_UNREACHABLE;
         unlockProperties();
         sendPollerMsg(dwRqId, POLLER_INFO _T("   CheckPoint SNMP agent on port 161 is active\r\n"));
      }
   }
	delete pTransport;
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
      m_dwFlags |= NF_IS_BRIDGE;
		memcpy(m_baseBridgeAddress, szBuffer, 6);
		unlockProperties();

		// Check for Spanning Tree (IEEE 802.1d) MIB support
		if (checkSNMPIntegerValue(pTransport, _T(".1.3.6.1.2.1.17.2.1.0"), 3))
		{
			lockProperties();
			m_dwFlags |= NF_IS_STP;
			unlockProperties();
		}
		else
		{
			lockProperties();
			m_dwFlags &= ~NF_IS_STP;
			unlockProperties();
		}
   }
   else
   {
		lockProperties();
      m_dwFlags &= ~(NF_IS_BRIDGE | NF_IS_STP);
		unlockProperties();
   }
}

/**
 * Configuration poll: check for ifXTable
 */
void Node::checkIfXTable(SNMP_Transport *pTransport)
{
	int count = 0;
	SnmpWalk(m_snmpVersion, pTransport, _T(".1.3.6.1.2.1.31.1.1.1.1"), CountingSnmpWalkerCallback, &count, FALSE);
   if (count > 0)
   {
		lockProperties();
      m_dwFlags |= NF_HAS_IFXTABLE;
		unlockProperties();
   }
   else
   {
		lockProperties();
      m_dwFlags &= ~NF_HAS_IFXTABLE;
		unlockProperties();
   }
}

/**
 * Update interface configuration
 */
BOOL Node::updateInterfaceConfiguration(UINT32 dwRqId, int maskBits)
{
   InterfaceList *pIfList;
   Interface **ppDeleteList;
   int i, j, iDelCount;
	BOOL hasChanges = FALSE;
	Cluster *pCluster = getMyCluster();

   sendPollerMsg(dwRqId, _T("Checking interface configuration...\r\n"));
   pIfList = getInterfaceList();
   if (pIfList != NULL)
   {
		DbgPrintf(6, _T("Node::updateInterfaceConfiguration(%s [%u]): got %d interfaces"), m_name, m_id, pIfList->size());

      // Find non-existing interfaces
      LockChildList(FALSE);
      ppDeleteList = (Interface **)malloc(sizeof(Interface *) * m_dwChildCount);
      for(i = 0, iDelCount = 0; i < (int)m_dwChildCount; i++)
      {
         if (m_pChildList[i]->getObjectClass() == OBJECT_INTERFACE)
         {
            Interface *pInterface = (Interface *)m_pChildList[i];
				if (!pInterface->isManuallyCreated())
				{
					for(j = 0; j < pIfList->size(); j++)
					{
						if (pIfList->get(j)->index == pInterface->getIfIndex())
							break;
					}

					if (j == pIfList->size())
					{
						// No such interface in current configuration, add it to delete list
						ppDeleteList[iDelCount++] = pInterface;
					}
				}
         }
      }
      UnlockChildList();

      // Delete non-existent interfaces
      if (iDelCount > 0)
      {
         for(j = 0; j < iDelCount; j++)
         {
            sendPollerMsg(dwRqId, POLLER_WARNING _T("   Interface \"%s\" is no longer exist\r\n"), ppDeleteList[j]->getName());
            const InetAddress& addr = ppDeleteList[j]->getFirstIpAddress();
            PostEvent(EVENT_INTERFACE_DELETED, m_id, "dsAd", ppDeleteList[j]->getIfIndex(), ppDeleteList[j]->getName(), &addr, addr.getMaskBits());
            deleteInterface(ppDeleteList[j]);
         }
         hasChanges = TRUE;
      }
      safe_free(ppDeleteList);

      // Add new interfaces and check configuration of existing
      for(j = 0; j < pIfList->size(); j++)
      {
			InterfaceInfo *ifInfo = pIfList->get(j);
         BOOL bNewInterface = TRUE;

         LockChildList(FALSE);
         for(i = 0; i < (int)m_dwChildCount; i++)
         {
            if (m_pChildList[i]->getObjectClass() == OBJECT_INTERFACE)
            {
               Interface *pInterface = (Interface *)m_pChildList[i];

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
                     pInterface->setMacAddr(ifInfo->macAddr);
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
                        }
                     }
                     else
                     {
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
                     }
                  }

                  bNewInterface = FALSE;
                  break;
               }
            }
         }
         UnlockChildList();

         if (bNewInterface)
         {
            // New interface
            sendPollerMsg(dwRqId, POLLER_INFO _T("   Found new interface \"%s\"\r\n"), ifInfo->name);
            createNewInterface(ifInfo, false);
            hasChanges = TRUE;
         }
      }
   }
   else if (!(m_dwFlags & NF_REMOTE_AGENT))    /* pIfList == NULL */
   {
      Interface *pInterface;
      UINT32 dwCount;

      sendPollerMsg(dwRqId, POLLER_ERROR _T("Unable to get interface list from node\r\n"));
		DbgPrintf(6, _T("Node::updateInterfaceConfiguration(%s [%u]): Unable to get interface list from node"), m_name, m_id);

      // Delete all existing interfaces in case of forced capability recheck
      if (m_dwDynamicFlags & NDF_RECHECK_CAPABILITIES)
      {
         LockChildList(FALSE);
         ppDeleteList = (Interface **)malloc(sizeof(Interface *) * m_dwChildCount);
         for(i = 0, iDelCount = 0; i < (int)m_dwChildCount; i++)
         {
				if ((m_pChildList[i]->getObjectClass() == OBJECT_INTERFACE) && !((Interface *)m_pChildList[i])->isManuallyCreated())
               ppDeleteList[iDelCount++] = (Interface *)m_pChildList[i];
         }
         UnlockChildList();
         for(j = 0; j < iDelCount; j++)
         {
            sendPollerMsg(dwRqId, POLLER_WARNING _T("   Interface \"%s\" is no longer exist\r\n"),
                          ppDeleteList[j]->getName());
            const InetAddress& addr = ppDeleteList[j]->getIpAddressList()->getFirstUnicastAddress();
            PostEvent(EVENT_INTERFACE_DELETED, m_id, "dsAd", ppDeleteList[j]->getIfIndex(),
                      ppDeleteList[j]->getName(), &addr, addr.getMaskBits());
            deleteInterface(ppDeleteList[j]);
         }
         safe_free(ppDeleteList);
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
						Subnet *pSubnet = FindSubnetForNode(m_zoneId, m_ipAddress);
						if (pSubnet != NULL)
							pSubnet->findMacAddress(m_ipAddress, macAddr);
						pMacAddr = !memcmp(macAddr, "\x00\x00\x00\x00\x00\x00", MAC_ADDR_LENGTH) ? NULL : macAddr;
						TCHAR szMac[20];
						MACToStr(macAddr, szMac);
						DbgPrintf(5, _T("Node::updateInterfaceConfiguration(%s [%u]): got MAC for unknown interface: %s"), m_name, m_id, szMac);
                  InetAddress ifaceAddr = m_ipAddress;
                  ifaceAddr.setMaskBits(maskBits);
                  createNewInterface(ifaceAddr, pMacAddr);
					}
            }
				else
				{
					// check MAC address
					memset(macAddr, 0, MAC_ADDR_LENGTH);
					Subnet *pSubnet = FindSubnetForNode(m_zoneId, m_ipAddress);
					if (pSubnet != NULL)
						pSubnet->findMacAddress(m_ipAddress, macAddr);
					if (memcmp(macAddr, "\x00\x00\x00\x00\x00\x00", MAC_ADDR_LENGTH) && memcmp(macAddr, pInterface->getMacAddr(), MAC_ADDR_LENGTH))
					{
                  TCHAR szOldMac[16], szNewMac[16];

                  BinToStr((BYTE *)pInterface->getMacAddr(), MAC_ADDR_LENGTH, szOldMac);
                  BinToStr(macAddr, MAC_ADDR_LENGTH, szNewMac);
						DbgPrintf(5, _T("Node::updateInterfaceConfiguration(%s [%u]): MAC change for unknown interface: %s to %s"),
						          m_name, m_id, szOldMac, szNewMac);
                  PostEvent(EVENT_MAC_ADDR_CHANGED, m_id, "idsss",
                            pInterface->getId(), pInterface->getIfIndex(),
                            pInterface->getName(), szOldMac, szNewMac);
                  pInterface->setMacAddr(macAddr);
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
				Subnet *pSubnet = FindSubnetForNode(m_zoneId, m_ipAddress);
				if (pSubnet != NULL)
					pSubnet->findMacAddress(m_ipAddress, macAddr);
				pMacAddr = !memcmp(macAddr, "\x00\x00\x00\x00\x00\x00", MAC_ADDR_LENGTH) ? NULL : macAddr;
				TCHAR szMac[20];
				MACToStr(macAddr, szMac);
				DbgPrintf(5, _T("Node::updateInterfaceConfiguration(%s [%u]): got MAC for unknown interface: %s"), m_name, m_id, szMac);
            InetAddress ifaceAddr = m_ipAddress;
            ifaceAddr.setMaskBits(maskBits);
         	createNewInterface(ifaceAddr, pMacAddr);
			}
      }
		DbgPrintf(6, _T("Node::updateInterfaceConfiguration(%s [%u]): pflist == NULL, dwCount = %u"), m_name, m_id, dwCount);
   }

	delete pIfList;
   checkSubnetBinding();

	sendPollerMsg(dwRqId, _T("Interface configuration check finished\r\n"));
	return hasChanges;
}

/**
 * Callback: apply template to nodes
 */
static void ApplyTemplate(NetObj *object, void *node)
{
   if ((object->getObjectClass() == OBJECT_TEMPLATE) && !object->isDeleted())
   {
      Template *pTemplate = (Template *)object;
		if (pTemplate->isApplicable((Node *)node))
		{
			if (!pTemplate->isChild(((Node *)node)->getId()))
			{
				DbgPrintf(4, _T("Node::ApplyUserTemplates(): applying template %d \"%s\" to node %d \"%s\""),
				          pTemplate->getId(), pTemplate->getName(), ((Node *)node)->getId(), ((Node *)node)->getName());
				pTemplate->applyToTarget((Node *)node);
				PostEvent(EVENT_TEMPLATE_AUTOAPPLY, g_dwMgmtNode, "isis", ((Node *)node)->getId(), ((Node *)node)->getName(), pTemplate->getId(), pTemplate->getName());
			}
		}
		else
		{
			if (pTemplate->isAutoRemoveEnabled() && pTemplate->isChild(((Node *)node)->getId()))
			{
				DbgPrintf(4, _T("Node::ApplyUserTemplates(): removing template %d \"%s\" from node %d \"%s\""),
				          pTemplate->getId(), pTemplate->getName(), ((Node *)node)->getId(), ((Node *)node)->getName());
				pTemplate->DeleteChild((Node *)node);
				((Node *)node)->DeleteParent(pTemplate);
				pTemplate->queueRemoveFromTarget(((Node *)node)->getId(), TRUE);
				PostEvent(EVENT_TEMPLATE_AUTOREMOVE, g_dwMgmtNode, "isis", ((Node *)node)->getId(), ((Node *)node)->getName(), pTemplate->getId(), pTemplate->getName());
			}
		}
   }
}

/**
 * Apply user templates
 */
void Node::applyUserTemplates()
{
	g_idxObjectById.forEach(ApplyTemplate, this);
}

/**
 * Filter for selecting containers from objects
 */
static bool ContainerSelectionFilter(NetObj *object, void *userData)
{
   return (object->getObjectClass() == OBJECT_CONTAINER) && !object->isDeleted() && ((Container *)object)->isAutoBindEnabled();
}

/**
 * Update container membership
 */
void Node::updateContainerMembership()
{
   ObjectArray<NetObj> *containers = g_idxObjectById.getObjects(true, ContainerSelectionFilter);
   for(int i = 0; i < containers->size(); i++)
   {
      Container *pContainer = (Container *)containers->get(i);
		if (pContainer->isSuitableForNode(this))
		{
			if (!pContainer->isChild(m_id))
			{
				DbgPrintf(4, _T("Node::updateContainerMembership(): binding node %d \"%s\" to container %d \"%s\""),
				          m_id, m_name, pContainer->getId(), pContainer->getName());
				pContainer->AddChild(this);
				AddParent(pContainer);
				PostEvent(EVENT_CONTAINER_AUTOBIND, g_dwMgmtNode, "isis", m_id, m_name, pContainer->getId(), pContainer->getName());
            pContainer->calculateCompoundStatus();
			}
		}
		else
		{
			if (pContainer->isAutoUnbindEnabled() && pContainer->isChild(m_id))
			{
				DbgPrintf(4, _T("Node::updateContainerMembership(): removing node %d \"%s\" from container %d \"%s\""),
				          m_id, m_name, pContainer->getId(), pContainer->getName());
				pContainer->DeleteChild(this);
				DeleteParent(pContainer);
				PostEvent(EVENT_CONTAINER_AUTOUNBIND, g_dwMgmtNode, "isis", m_id, m_name, pContainer->getId(), pContainer->getName());
            pContainer->calculateCompoundStatus();
			}
		}
      pContainer->decRefCount();
   }
   delete containers;
}

/**
 * Perform instance discovery poll on node
 */
void Node::instanceDiscoveryPoll(ClientSession *session, UINT32 requestId, int pollerId)
{
	if (m_dwDynamicFlags & NDF_DELETE_IN_PROGRESS)
	{
		if (requestId == 0)
			m_dwDynamicFlags &= ~NDF_QUEUED_FOR_INSTANCE_POLL;
		return;
	}

   SetPollerInfo(pollerId, _T("wait for lock"));
   pollerLock();
   m_pollRequestor = session;
   sendPollerMsg(requestId, _T("Starting instance discovery poll for node %s\r\n"), m_name);
   DbgPrintf(4, _T("Starting instance discovery poll for node %s (ID: %d)"), m_name, m_id);

   // Check if node is marked as unreachable
   if (!(m_dwDynamicFlags & NDF_UNREACHABLE))
   {
		SetPollerInfo(pollerId, _T("instance discovery"));
      doInstanceDiscovery(requestId);

		// Execute hook script
		SetPollerInfo(pollerId, _T("hook"));
		executeHookScript(_T("InstancePoll"));
   }
   else
   {
      sendPollerMsg(requestId, POLLER_WARNING _T("Node is marked as unreachable, instance discovery poll aborted\r\n"));
      DbgPrintf(4, _T("Node is marked as unreachable, instance discovery poll aborted"));
   }

   m_lastInstancePoll = time(NULL);

   // Finish instance discovery poll
   SetPollerInfo(pollerId, _T("cleanup"));
   if (requestId == 0)
      m_dwDynamicFlags &= ~NDF_QUEUED_FOR_INSTANCE_POLL;
   pollerUnlock();
   DbgPrintf(4, _T("Finished instance discovery poll for node %s (ID: %d)"), m_name, m_id);
}

/**
 * Do instance discovery
 */
void Node::doInstanceDiscovery(UINT32 requestId)
{
   sendPollerMsg(requestId, _T("Running DCI instance discovery\r\n"));

	// collect instance discovery DCIs
	ObjectArray<DCItem> rootItems;
   lockDciAccess(false);
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
		DCObject *object = m_dcObjects->get(i);
		if ((object->getType() == DCO_TYPE_ITEM) && (((DCItem *)object)->getInstanceDiscoveryMethod() != IDM_NONE))
      {
			object->setBusyFlag(TRUE);
			rootItems.add((DCItem *)object);
      }
   }
   unlockDciAccess();

	// process instance discovery DCIs
	// it should be done that way to prevent DCI list lock for long time
	for(int i = 0; i < rootItems.size(); i++)
	{
		DCItem *dci = rootItems.get(i);
		DbgPrintf(5, _T("Node::doInstanceDiscovery(%s [%u]): Updating instances for instance discovery DCI %s [%d]"),
		          m_name, m_id, dci->getName(), dci->getId());
      sendPollerMsg(requestId, _T("   Updating instances for %s [%d]\r\n"), dci->getName(), dci->getId());
		StringMap *instances = getInstanceList(dci);
		if (instances != NULL)
		{
			DbgPrintf(5, _T("Node::doInstanceDiscovery(%s [%u]): read %d values"), m_name, m_id, instances->size());
			dci->filterInstanceList(instances);
         updateInstances(dci, instances, requestId);
			delete instances;
		}
		else
		{
			DbgPrintf(5, _T("Node::doInstanceDiscovery(%s [%u]): failed to get instance list for DCI %s [%d]"),
						 m_name, m_id, dci->getName(), dci->getId());
         sendPollerMsg(requestId, POLLER_ERROR _T("      Failed to get instance list\r\n"));
		}
		dci->setBusyFlag(FALSE);
	}
}

/**
 * Get instances for instance discovery DCI
 */
StringMap *Node::getInstanceList(DCItem *dci)
{
	if (dci->getInstanceDiscoveryData() == NULL)
		return NULL;

   Node *node;
   if (dci->getProxyNode() != 0)
   {
      node = (Node *)FindObjectById(dci->getProxyNode(), OBJECT_NODE);
      if (node == NULL)
      {
         DbgPrintf(6, _T("Node::getInstanceList(%s [%d]): proxy node [%d] not found"), dci->getName(), dci->getId(), dci->getProxyNode());
         return NULL;
      }
      if (!node->isTrustedNode(m_id))
      {
         DbgPrintf(6, _T("Node::getInstanceList(%s [%d]): this node (%s [%d]) is not trusted by proxy node %s [%d] not found"),
                   dci->getName(), dci->getId(), m_name, m_id, node->getName(), node->getId());
         return NULL;
      }
   }
   else
   {
      node = this;
   }

	StringList *instances = NULL;
   StringMap *instanceMap = NULL;
	switch(dci->getInstanceDiscoveryMethod())
	{
		case IDM_AGENT_LIST:
			node->getListFromAgent(dci->getInstanceDiscoveryData(), &instances);
			break;
		case IDM_SNMP_WALK_VALUES:
		   node->getListFromSNMP(dci->getSnmpPort(), dci->getInstanceDiscoveryData(), &instances);
		   break;
      case IDM_SNMP_WALK_OIDS:
         node->getOIDSuffixListFromSNMP(dci->getSnmpPort(), dci->getInstanceDiscoveryData(), &instanceMap);
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
 * Callback for finding instance
 */
static bool FindInstanceCallback(const TCHAR *key, const void *value, void *data)
{
   return _tcscmp((const TCHAR *)data, key) != 0;  // return false if instance found - it will stop enumeration
}

/**
 * Data for CreateInstanceDCI
 */
struct CreateInstanceDCIData
{
   DCItem *root;
   Node *object;
   UINT32 requestId;
};

/**
 * Callback for creating instance DCIs
 */
static bool CreateInstanceDCI(const TCHAR *key, const void *value, void *data)
{
   Node *object = ((CreateInstanceDCIData *)data)->object;
   DCItem *root = ((CreateInstanceDCIData *)data)->root;

	DbgPrintf(5, _T("Node::updateInstances(%s [%u], %s [%u]): creating new DCI for instance \"%s\""),
	          object->getName(), object->getId(), root->getName(), root->getId(), key);
   object->sendPollerMsg(((CreateInstanceDCIData *)data)->requestId, _T("      Creating new DCI for instance \"%s\"\r\n"), key);

	DCItem *dci = new DCItem(root);
   dci->setTemplateId(object->getId(), root->getId());
	dci->setInstance((const TCHAR *)value);
	dci->setInstanceDiscoveryMethod(IDM_NONE);
	dci->setInstanceDiscoveryData(key);
	dci->setInstanceFilter(NULL);
   dci->expandInstance();
	dci->changeBinding(CreateUniqueId(IDG_ITEM), object, FALSE);
	object->addDCObject(dci, true);
   return true;
}

/**
 * Update instance DCIs created from instance discovery DCI
 */
void Node::updateInstances(DCItem *root, StringMap *instances, UINT32 requestId)
{
   lockDciAccess(true);

	// Delete DCIs for missing instances and update existing
	IntegerArray<UINT32> deleteList;
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
		DCObject *object = m_dcObjects->get(i);
		if ((object->getType() != DCO_TYPE_ITEM) ||
			 (object->getTemplateId() != m_id) ||
			 (object->getTemplateItemId() != root->getId()))
			continue;

      const TCHAR *dciInstance = ((DCItem *)object)->getInstanceDiscoveryData();
      if (!instances->forEach(FindInstanceCallback, (void *)dciInstance))
		{
			// found, remove value from instances
			DbgPrintf(5, _T("Node::updateInstances(%s [%u], %s [%u]): instance \"%s\" found"),
			          m_name, m_id, root->getName(), root->getId(), dciInstance);
			instances->remove(dciInstance);
		}
		else
		{
			// not found, delete DCI
			DbgPrintf(5, _T("Node::updateInstances(%s [%u], %s [%u]): instance \"%s\" not found, instance DCI will be deleted"),
			          m_name, m_id, root->getName(), root->getId(), dciInstance);
         sendPollerMsg(requestId, _T("      Existing instance \"%s\" not found and will be deleted\r\n"), dciInstance);
			deleteList.add(object->getId());
		}
   }

	for(int i = 0; i < deleteList.size(); i++)
		deleteDCObject(deleteList.get(i), false);

	// Create new instances
   CreateInstanceDCIData data;
   data.root = root;
   data.object = this;
   data.requestId = requestId;
   instances->forEach(CreateInstanceDCI, &data);

   unlockDciAccess();
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

   const TCHAR *login = getCustomAttribute(_T("iLO.login"));
   const TCHAR *password = getCustomAttribute(_T("iLO.password"));

   if ((login != NULL) && (password != NULL))
      return m_smclpConnection->connect(login, password);
   return false;
}

/**
 * Connect to native agent. Assumes that access to agent connection is already locked.
 */
BOOL Node::connectToAgent(UINT32 *error, UINT32 *socketError)
{
   BOOL bRet;

   // Create new agent connection object if needed
   if (m_pAgentConnection == NULL)
	{
      m_pAgentConnection = new AgentConnectionEx(m_id, m_ipAddress, m_agentPort, m_agentAuthMethod, m_szSharedSecret);
		DbgPrintf(7, _T("Node::connectToAgent(%s [%d]): new agent connection created"), m_name, m_id);
	}
	else
	{
		// Check if we already connected
		if (m_pAgentConnection->nop() == ERR_SUCCESS)
		{
			DbgPrintf(7, _T("Node::connectToAgent(%s [%d]): already connected"), m_name, m_id);
			return TRUE;
		}

		// Close current connection or clean up after broken connection
		m_pAgentConnection->disconnect();
		DbgPrintf(7, _T("Node::connectToAgent(%s [%d]): existing connection reset"), m_name, m_id);
	}
   m_pAgentConnection->setPort(m_agentPort);
   m_pAgentConnection->setAuthData(m_agentAuthMethod, m_szSharedSecret);
   setAgentProxy(m_pAgentConnection);
	DbgPrintf(7, _T("Node::connectToAgent(%s [%d]): calling connect on port %d"), m_name, m_id, (int)m_agentPort);
   bRet = m_pAgentConnection->connect(g_pServerKey, FALSE, error, socketError);
   if (bRet)
	{
		m_pAgentConnection->setCommandTimeout(g_agentCommandTimeout);
      m_pAgentConnection->enableTraps();
	}
   return bRet;
}

/**
 * Get DCI value via SNMP
 */
UINT32 Node::getItemFromSNMP(WORD port, const TCHAR *param, size_t bufSize, TCHAR *buffer, int interpretRawValue)
{
   UINT32 dwResult;

   if ((((m_dwDynamicFlags & NDF_SNMP_UNREACHABLE) || !(m_dwFlags & NF_IS_SNMP)) && (port == 0)) ||
       (m_dwDynamicFlags & NDF_UNREACHABLE) ||
		 (m_dwFlags & NF_DISABLE_SNMP))
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
   return (dwResult == SNMP_ERR_SUCCESS) ? DCE_SUCCESS :
      ((dwResult == SNMP_ERR_NO_OBJECT) ? DCE_NOT_SUPPORTED : DCE_COMM_ERROR);
}

/**
 * Read one row for SNMP table
 */
static UINT32 ReadSNMPTableRow(SNMP_Transport *snmp, SNMP_ObjectId *rowOid, size_t baseOidLen, UINT32 index,
                               ObjectArray<DCTableColumn> *columns, Table *table)
{
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   for(int i = 0; i < columns->size(); i++)
   {
      DCTableColumn *c = columns->get(i);
      if (c->getSnmpOid() != NULL)
      {
         UINT32 oid[MAX_OID_LEN];
         size_t oidLen = c->getSnmpOid()->getLength();
         memcpy(oid, c->getSnmpOid()->getValue(), oidLen * sizeof(UINT32));
         if (rowOid != NULL)
         {
            size_t suffixLen = rowOid->getLength() - baseOidLen;
            memcpy(&oid[oidLen], rowOid->getValue() + baseOidLen, suffixLen * sizeof(UINT32));
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
   UINT32 rc = snmp->doRequest(&request, &response, g_snmpTimeout, 3);
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
               bool convert = false;
               TCHAR buffer[256];
               table->set((int)i, v->getValueAsPrintableString(buffer, 256, &convert));
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
static UINT32 SNMPGetTableCallback(UINT32 snmpVersion, SNMP_Variable *varbind, SNMP_Transport *snmp, void *arg)
{
   ((ObjectArray<SNMP_ObjectId> *)arg)->add(new SNMP_ObjectId(varbind->getName()));
   return SNMP_ERR_SUCCESS;
}

/**
 * Get table from SNMP
 */
UINT32 Node::getTableFromSNMP(WORD port, const TCHAR *oid, ObjectArray<DCTableColumn> *columns, Table **table)
{
   *table = NULL;

   SNMP_Transport *snmp = createSnmpTransport(port);
   if (snmp == NULL)
      return DCE_COMM_ERROR;

   ObjectArray<SNMP_ObjectId> oidList(64, 64, true);
   UINT32 rc = SnmpWalk(snmp->getSnmpVersion(), snmp, oid, SNMPGetTableCallback, &oidList, FALSE);
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
            break;
      }
   }
   delete snmp;
   return (rc == SNMP_ERR_SUCCESS) ? DCE_SUCCESS : ((rc == SNMP_ERR_NO_OBJECT) ? DCE_NOT_SUPPORTED : DCE_COMM_ERROR);
}

/**
 * Callback for SnmpWalk in Node::getListFromSNMP
 */
static UINT32 SNMPGetListCallback(UINT32 snmpVersion, SNMP_Variable *varbind, SNMP_Transport *snmp, void *arg)
{
   bool convert = false;
   TCHAR buffer[256];
   ((StringList *)arg)->add(varbind->getValueAsPrintableString(buffer, 256, &convert));
   return SNMP_ERR_SUCCESS;
}

/**
 * Get list of values from SNMP
 */
UINT32 Node::getListFromSNMP(WORD port, const TCHAR *oid, StringList **list)
{
   *list = NULL;
   SNMP_Transport *snmp = createSnmpTransport(port);
   if (snmp == NULL)
      return DCE_COMM_ERROR;

   *list = new StringList;
   UINT32 rc = SnmpWalk(snmp->getSnmpVersion(), snmp, oid, SNMPGetListCallback, *list, FALSE);
   delete snmp;
   if (rc != SNMP_ERR_SUCCESS)
   {
      delete *list;
      *list = NULL;
   }
   return (rc == SNMP_ERR_SUCCESS) ? DCE_SUCCESS : ((rc == SNMP_ERR_NO_OBJECT) ? DCE_NOT_SUPPORTED : DCE_COMM_ERROR);
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
static UINT32 SNMPOIDSuffixListCallback(UINT32 snmpVersion, SNMP_Variable *varbind, SNMP_Transport *snmp, void *arg)
{
   SNMPOIDSuffixListCallback_Data *data = (SNMPOIDSuffixListCallback_Data *)arg;
   SNMP_ObjectId *oid = varbind->getName();
   if (oid->getLength() <= data->oidLen)
      return SNMP_ERR_SUCCESS;
   TCHAR buffer[256];
   SNMPConvertOIDToText(oid->getLength() - data->oidLen, &(oid->getValue()[data->oidLen]), buffer, 256);

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
UINT32 Node::getOIDSuffixListFromSNMP(WORD port, const TCHAR *oid, StringMap **values)
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
   UINT32 rc = SnmpWalk(snmp->getSnmpVersion(), snmp, oid, SNMPOIDSuffixListCallback, &data, FALSE);
   delete snmp;
   if (rc == SNMP_ERR_SUCCESS)
   {
      *values = data.values;
   }
   else
   {
      delete data.values;
   }
   return (rc == SNMP_ERR_SUCCESS) ? DCE_SUCCESS : ((rc == SNMP_ERR_NO_OBJECT) ? DCE_NOT_SUPPORTED : DCE_COMM_ERROR);
}

/**
 * Get item's value via SNMP from CheckPoint's agent
 */
UINT32 Node::getItemFromCheckPointSNMP(const TCHAR *szParam, UINT32 dwBufSize, TCHAR *szBuffer)
{
   UINT32 dwResult;

   if ((m_dwDynamicFlags & NDF_CPSNMP_UNREACHABLE) ||
       (m_dwDynamicFlags & NDF_UNREACHABLE))
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
   return (dwResult == SNMP_ERR_SUCCESS) ? DCE_SUCCESS :
      ((dwResult == SNMP_ERR_NO_OBJECT) ? DCE_NOT_SUPPORTED : DCE_COMM_ERROR);
}

/**
 * Get item's value via native agent
 */
UINT32 Node::getItemFromAgent(const TCHAR *szParam, UINT32 dwBufSize, TCHAR *szBuffer)
{
   UINT32 dwError = ERR_NOT_CONNECTED, dwResult = DCE_COMM_ERROR;
   UINT32 dwTries = 3;

   if ((m_dwDynamicFlags & NDF_AGENT_UNREACHABLE) ||
       (m_dwDynamicFlags & NDF_UNREACHABLE) ||
		 (m_dwFlags & NF_DISABLE_NXCP) ||
		 !(m_dwFlags & NF_IS_NATIVE_AGENT))
      return DCE_COMM_ERROR;

   agentLock();

   // Establish connection if needed
   if (m_pAgentConnection == NULL)
      if (!connectToAgent())
         goto end_loop;

   // Get parameter from agent
   while(dwTries-- > 0)
   {
      dwError = m_pAgentConnection->getParameter(szParam, dwBufSize, szBuffer);
      switch(dwError)
      {
         case ERR_SUCCESS:
            dwResult = DCE_SUCCESS;
            goto end_loop;
         case ERR_UNKNOWN_PARAMETER:
            dwResult = DCE_NOT_SUPPORTED;
            goto end_loop;
         case ERR_NOT_CONNECTED:
         case ERR_CONNECTION_BROKEN:
            if (!connectToAgent())
               goto end_loop;
            break;
         case ERR_REQUEST_TIMEOUT:
				// Reset connection to agent after timeout
				DbgPrintf(7, _T("Node(%s)->GetItemFromAgent(%s): timeout; resetting connection to agent..."), m_name, szParam);
				delete_and_null(m_pAgentConnection);
            if (!connectToAgent())
               goto end_loop;
				DbgPrintf(7, _T("Node(%s)->GetItemFromAgent(%s): connection to agent restored successfully"), m_name, szParam);
            break;
      }
   }

end_loop:
   agentUnlock();
   DbgPrintf(7, _T("Node(%s)->GetItemFromAgent(%s): dwError=%d dwResult=%d"), m_name, szParam, dwError, dwResult);
   return dwResult;
}

/**
 * Get table from agent
 */
UINT32 Node::getTableFromAgent(const TCHAR *name, Table **table)
{
   UINT32 dwError = ERR_NOT_CONNECTED, dwResult = DCE_COMM_ERROR;
   UINT32 dwTries = 3;

	*table = NULL;

   if ((m_dwDynamicFlags & NDF_AGENT_UNREACHABLE) ||
       (m_dwDynamicFlags & NDF_UNREACHABLE) ||
		 (m_dwFlags & NF_DISABLE_NXCP) ||
		 !(m_dwFlags & NF_IS_NATIVE_AGENT))
      return DCE_COMM_ERROR;

   agentLock();

   // Establish connection if needed
   if (m_pAgentConnection == NULL)
      if (!connectToAgent())
         goto end_loop;

   // Get parameter from agent
   while(dwTries-- > 0)
   {
      dwError = m_pAgentConnection->getTable(name, table);
      switch(dwError)
      {
         case ERR_SUCCESS:
            dwResult = DCE_SUCCESS;
            goto end_loop;
         case ERR_UNKNOWN_PARAMETER:
            dwResult = DCE_NOT_SUPPORTED;
            goto end_loop;
         case ERR_NOT_CONNECTED:
         case ERR_CONNECTION_BROKEN:
            if (!connectToAgent())
               goto end_loop;
            break;
         case ERR_REQUEST_TIMEOUT:
				// Reset connection to agent after timeout
				DbgPrintf(7, _T("Node(%s)->getTableFromAgent(%s): timeout; resetting connection to agent..."), m_name, name);
				delete_and_null(m_pAgentConnection);
            if (!connectToAgent())
               goto end_loop;
				DbgPrintf(7, _T("Node(%s)->getTableFromAgent(%s): connection to agent restored successfully"), m_name, name);
            break;
      }
   }

end_loop:
   agentUnlock();
   DbgPrintf(7, _T("Node(%s)->getTableFromAgent(%s): dwError=%d dwResult=%d"), m_name, name, dwError, dwResult);
   return dwResult;
}

/**
 * Get list from agent
 */
UINT32 Node::getListFromAgent(const TCHAR *name, StringList **list)
{
   UINT32 dwError = ERR_NOT_CONNECTED, dwResult = DCE_COMM_ERROR;
   UINT32 i, dwTries = 3;

	*list = NULL;

   if ((m_dwDynamicFlags & NDF_AGENT_UNREACHABLE) ||
       (m_dwDynamicFlags & NDF_UNREACHABLE) ||
		 (m_dwFlags & NF_DISABLE_NXCP) ||
		 !(m_dwFlags & NF_IS_NATIVE_AGENT))
      return DCE_COMM_ERROR;

   agentLock();

   // Establish connection if needed
   if (m_pAgentConnection == NULL)
      if (!connectToAgent())
         goto end_loop;

   // Get parameter from agent
   while(dwTries-- > 0)
   {
		dwError = m_pAgentConnection->getList(name);
      switch(dwError)
      {
         case ERR_SUCCESS:
            dwResult = DCE_SUCCESS;
				*list = new StringList;
				for(i = 0; i < m_pAgentConnection->getNumDataLines(); i++)
					(*list)->add(m_pAgentConnection->getDataLine(i));
            goto end_loop;
         case ERR_UNKNOWN_PARAMETER:
            dwResult = DCE_NOT_SUPPORTED;
            goto end_loop;
         case ERR_NOT_CONNECTED:
         case ERR_CONNECTION_BROKEN:
            if (!connectToAgent())
               goto end_loop;
            break;
         case ERR_REQUEST_TIMEOUT:
				// Reset connection to agent after timeout
				DbgPrintf(7, _T("Node(%s)->getListFromAgent(%s): timeout; resetting connection to agent..."), m_name, name);
				delete_and_null(m_pAgentConnection);
            if (!connectToAgent())
               goto end_loop;
				DbgPrintf(7, _T("Node(%s)->getListFromAgent(%s): connection to agent restored successfully"), m_name, name);
            break;
      }
   }

end_loop:
   agentUnlock();
   DbgPrintf(7, _T("Node(%s)->getListFromAgent(%s): dwError=%d dwResult=%d"), m_name, name, dwError, dwResult);
   return dwResult;
}

/**
 * Get item's value via SM-CLP protocol
 */
UINT32 Node::getItemFromSMCLP(const TCHAR *param, UINT32 bufSize, TCHAR *buffer)
{
   UINT32 result = DCE_COMM_ERROR;
   int tries = 3;

   if (m_dwDynamicFlags & NDF_UNREACHABLE)
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
         nx_strncpy(buffer, value, bufSize);
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
 * Get value for server's internal parameter
 */
UINT32 Node::getInternalItem(const TCHAR *param, size_t bufSize, TCHAR *buffer)
{
	UINT32 rc = DataCollectionTarget::getInternalItem(param, bufSize, buffer);
	if (rc != DCE_NOT_SUPPORTED)
		return rc;
	rc = DCE_SUCCESS;

   if (!_tcsicmp(param, _T("AgentStatus")))
   {
      if (m_dwFlags & NF_IS_NATIVE_AGENT)
      {
         buffer[0] = (m_dwDynamicFlags & NDF_AGENT_UNREACHABLE) ? _T('1') : _T('0');
         buffer[1] = 0;
      }
      else
      {
         rc = DCE_NOT_SUPPORTED;
      }
   }
   else if (MatchString(_T("Net.IP.NextHop(*)"), param, FALSE))
   {
      if ((m_dwFlags & NF_IS_NATIVE_AGENT) || (m_dwFlags & NF_IS_SNMP))
		{
			TCHAR arg[256] = _T("");
	      AgentGetParameterArg(param, 1, arg, 256);
         InetAddress destAddr = InetAddress::parse(arg);
			if (destAddr.isValidUnicast())
			{
			   bool isVpn;
   			UINT32 ifIndex;
            InetAddress nextHop;
            TCHAR name[MAX_OBJECT_NAME];
				if (getNextHop(m_ipAddress, destAddr, &nextHop, &ifIndex, &isVpn, name))
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
            rc = DCE_COMM_ERROR;
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
         LockChildList(FALSE);
         for(int i = 0; i < (int)m_dwChildCount; i++)
         {
            if ((m_pChildList[i]->getObjectClass() == OBJECT_INTERFACE) && ((Interface *)m_pChildList[i])->getIpAddressList()->hasAddress(m_ipAddress))
            {
               iface = (Interface *)m_pChildList[i];
               break;
            }
         }
         UnlockChildList();

         UINT32 value = 10000;
         if (iface != NULL)
         {
            value = iface->getPingTime();
         }
         else
         {
            value = getPingTime();
         }
         if (value == 10000)
            rc = DCE_COMM_ERROR;
         else
            _sntprintf(buffer, bufSize, _T("%d"), value);
      }
      else
      {
         rc = DCE_NOT_SUPPORTED;
      }
   }
   else if (!_tcsicmp(param, _T("WirelessController.AdoptedAPCount")))
   {
      if (m_dwFlags & NF_IS_WIFI_CONTROLLER)
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
      if (m_dwFlags & NF_IS_WIFI_CONTROLLER)
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
      if (m_dwFlags & NF_IS_WIFI_CONTROLLER)
      {
			_sntprintf(buffer, bufSize, _T("%d"), m_totalApCount - m_adoptedApCount);
      }
      else
      {
         rc = DCE_NOT_SUPPORTED;
      }
   }
   else if (m_dwFlags & NF_IS_LOCAL_MGMT)
   {
      if (!_tcsicmp(param, _T("Server.AverageDCPollerQueueSize")))
      {
         _sntprintf(buffer, bufSize, _T("%f"), g_dAvgPollerQueueSize);
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
      else if (!_tcsicmp(param, _T("Server.AverageStatusPollerQueueSize")))
      {
         _sntprintf(buffer, bufSize, _T("%f"), g_dAvgStatusPollerQueueSize);
      }
      else if (!_tcsicmp(param, _T("Server.AverageConfigurationPollerQueueSize")))
      {
         _sntprintf(buffer, bufSize, _T("%f"), g_dAvgConfigPollerQueueSize);
      }
      else if (!_tcsicmp(param, _T("Server.AverageDCIQueuingTime")))
      {
         _sntprintf(buffer, bufSize, _T("%u"), g_dwAvgDCIQueuingTime);
      }
      else if (!_tcsicmp(param, _T("Server.AverageSyslogProcessingQueueSize")))
      {
         _sntprintf(buffer, bufSize, _T("%f"), g_dAvgSyslogProcessingQueueSize);
      }
      else if (!_tcsicmp(param, _T("Server.AverageSyslogWriterQueueSize")))
      {
         _sntprintf(buffer, bufSize, _T("%f"), g_dAvgSyslogWriterQueueSize);
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
static UINT32 RCCFromDCIError(UINT32 error)
{
   switch(error)
   {
      case DCE_SUCCESS:
         return RCC_SUCCESS;
      case DCE_COMM_ERROR:
         return RCC_COMM_FAILURE;
      case DCE_NOT_SUPPORTED:
         return RCC_DCI_NOT_SUPPORTED;
      default:
         return RCC_SYSTEM_FAILURE;
   }
}

/**
 * Get item's value for client
 */
UINT32 Node::getItemForClient(int iOrigin, const TCHAR *pszParam, TCHAR *pszBuffer, UINT32 dwBufSize)
{
   UINT32 dwResult = 0, dwRetCode;

   // Get data from node
   switch(iOrigin)
   {
      case DS_INTERNAL:
         dwRetCode = getInternalItem(pszParam, dwBufSize, pszBuffer);
         break;
      case DS_NATIVE_AGENT:
         dwRetCode = getItemFromAgent(pszParam, dwBufSize, pszBuffer);
         break;
      case DS_SNMP_AGENT:
			dwRetCode = getItemFromSNMP(0, pszParam, dwBufSize, pszBuffer, SNMP_RAWTYPE_NONE);
         break;
      case DS_CHECKPOINT_AGENT:
         dwRetCode = getItemFromCheckPointSNMP(pszParam, dwBufSize, pszBuffer);
         break;
      default:
         dwResult = RCC_INVALID_ARGUMENT;
         break;
   }

   // Translate return code to RCC
   if (dwResult != RCC_INVALID_ARGUMENT)
		dwResult = RCCFromDCIError(dwRetCode);

   return dwResult;
}

/**
 * Get table for client
 */
UINT32 Node::getTableForClient(const TCHAR *name, Table **table)
{
	UINT32 dwRetCode = getTableFromAgent(name, table);
	return RCCFromDCIError(dwRetCode);
}

/**
 * Create CSCP message with object's data
 */
void Node::fillMessage(NXCPMessage *pMsg, BOOL alreadyLocked)
{
   if (!alreadyLocked)
		lockProperties();

   DataCollectionTarget::fillMessage(pMsg, TRUE);
   pMsg->setField(VID_IP_ADDRESS, m_ipAddress);
	pMsg->setField(VID_PRIMARY_NAME, m_primaryName);
   pMsg->setField(VID_FLAGS, m_dwFlags);
   pMsg->setField(VID_RUNTIME_FLAGS, m_dwDynamicFlags);
   pMsg->setField(VID_AGENT_PORT, m_agentPort);
   pMsg->setField(VID_AUTH_METHOD, m_agentAuthMethod);
   pMsg->setField(VID_SHARED_SECRET, m_szSharedSecret);
	pMsg->setFieldFromMBString(VID_SNMP_AUTH_OBJECT, m_snmpSecurity->getCommunity());
	pMsg->setFieldFromMBString(VID_SNMP_AUTH_PASSWORD, m_snmpSecurity->getAuthPassword());
	pMsg->setFieldFromMBString(VID_SNMP_PRIV_PASSWORD, m_snmpSecurity->getPrivPassword());
	pMsg->setField(VID_SNMP_USM_METHODS, (WORD)((WORD)m_snmpSecurity->getAuthMethod() | ((WORD)m_snmpSecurity->getPrivMethod() << 8)));
   pMsg->setField(VID_SNMP_OID, m_szObjectId);
   pMsg->setField(VID_SNMP_PORT, m_wSNMPPort);
   pMsg->setField(VID_SNMP_VERSION, (WORD)m_snmpVersion);
   pMsg->setField(VID_AGENT_VERSION, m_szAgentVersion);
   pMsg->setField(VID_PLATFORM_NAME, m_szPlatformName);
   pMsg->setField(VID_POLLER_NODE_ID, m_pollerNode);
   pMsg->setField(VID_ZONE_ID, m_zoneId);
   pMsg->setField(VID_AGENT_PROXY, m_agentProxy);
   pMsg->setField(VID_SNMP_PROXY, m_snmpProxy);
   pMsg->setField(VID_ICMP_PROXY, m_icmpProxy);
	pMsg->setField(VID_REQUIRED_POLLS, (WORD)m_iRequiredPollCount);
	pMsg->setField(VID_SYS_NAME, CHECK_NULL_EX(m_sysName));
	pMsg->setField(VID_SYS_DESCRIPTION, CHECK_NULL_EX(m_sysDescription));
   pMsg->setField(VID_BOOT_TIME, (UINT32)m_bootTime);
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

	if(!alreadyLocked)
      unlockProperties();
}

/**
 * Modify object from NXCP message
 */
UINT32 Node::modifyFromMessage(NXCPMessage *pRequest, BOOL bAlreadyLocked)
{
   if (!bAlreadyLocked)
      lockProperties();

   // Change flags
   if (pRequest->isFieldExist(VID_FLAGS))
   {
      bool wasRemoteAgent = ((m_dwFlags & NF_REMOTE_AGENT) != 0);
      m_dwFlags &= NF_SYSTEM_FLAGS;
      m_dwFlags |= pRequest->getFieldAsUInt32(VID_FLAGS) & NF_USER_FLAGS;
      if (wasRemoteAgent && !(m_dwFlags & NF_REMOTE_AGENT) && m_ipAddress.isValidUnicast())
      {
         if (IsZoningEnabled())
         {
		      Zone *zone = (Zone *)g_idxZoneByGUID.get(m_zoneId);
		      if (zone != NULL)
		      {
			      zone->addToIndex(this);
		      }
		      else
		      {
			      DbgPrintf(2, _T("Cannot find zone object with GUID=%d for node object %s [%d]"), (int)m_zoneId, m_name, (int)m_id);
		      }
         }
         else
         {
				g_idxNodeByAddr.put(m_ipAddress, this);
         }
      }
      else if (!wasRemoteAgent && (m_dwFlags & NF_REMOTE_AGENT) && m_ipAddress.isValidUnicast())
      {
         if (IsZoningEnabled())
         {
		      Zone *zone = (Zone *)g_idxZoneByGUID.get(m_zoneId);
		      if (zone != NULL)
		      {
               zone->removeFromIndex(this);
		      }
		      else
		      {
			      DbgPrintf(2, _T("Cannot find zone object with GUID=%d for node object %s [%d]"), (int)m_zoneId, m_name, (int)m_id);
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

      // Check if received IP address is one of node's interface addresses
      LockChildList(FALSE);
      UINT32 i;
      for(i = 0; i < m_dwChildCount; i++)
         if ((m_pChildList[i]->getObjectClass() == OBJECT_INTERFACE) &&
             ((Interface *)m_pChildList[i])->getIpAddressList()->hasAddress(ipAddr))
            break;
      UnlockChildList();
      if (i == m_dwChildCount)
      {
         unlockProperties();
         return RCC_INVALID_IP_ADDR;
      }

      setPrimaryIPAddress(ipAddr);

		// Update primary name if it is not set with the same message
		if (!pRequest->isFieldExist(VID_PRIMARY_NAME))
		{
			m_ipAddress.toString(m_primaryName);
		}

		agentLock();
		delete_and_null(m_pAgentConnection);
		agentUnlock();
	}

   // Change primary host name
   if (pRequest->isFieldExist(VID_PRIMARY_NAME))
   {
		pRequest->getFieldAsString(VID_PRIMARY_NAME, m_primaryName, MAX_DNS_NAME);
		m_dwDynamicFlags |= NDF_FORCE_CONFIGURATION_POLL | NDF_RECHECK_CAPABILITIES;
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
				unlockProperties();
				return RCC_INVALID_OBJECT_ID;
			}
			if (pObject->getObjectClass() != OBJECT_NODE)
			{
				unlockProperties();
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
      m_agentAuthMethod = pRequest->getFieldAsUInt16(VID_AUTH_METHOD);

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
		m_wSNMPPort = pRequest->getFieldAsUInt16(VID_SNMP_PORT);

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
      m_snmpProxy = pRequest->getFieldAsUInt32(VID_SNMP_PROXY);

   // Change ICMP proxy node
   if (pRequest->isFieldExist(VID_ICMP_PROXY))
      m_icmpProxy = pRequest->getFieldAsUInt32(VID_ICMP_PROXY);

   // Number of required polls
   if (pRequest->isFieldExist(VID_REQUIRED_POLLS))
      m_iRequiredPollCount = (int)pRequest->getFieldAsUInt16(VID_REQUIRED_POLLS);

   // Enable/disable usage of ifXTable
   if (pRequest->isFieldExist(VID_USE_IFXTABLE))
      m_nUseIfXTable = (BYTE)pRequest->getFieldAsUInt16(VID_USE_IFXTABLE);

   return DataCollectionTarget::modifyFromMessage(pRequest, TRUE);
}

/**
 * Wakeup node using magic packet
 */
UINT32 Node::wakeUp()
{
   UINT32 i, dwResult = RCC_NO_WOL_INTERFACES;

   LockChildList(FALSE);

   for(i = 0; i < m_dwChildCount; i++)
      if ((m_pChildList[i]->getObjectClass() == OBJECT_INTERFACE) &&
          (m_pChildList[i]->Status() != STATUS_UNMANAGED) &&
          ((Interface *)m_pChildList[i])->getIpAddressList()->getFirstUnicastAddressV4().isValid())
      {
         dwResult = ((Interface *)m_pChildList[i])->wakeUp();
         if (dwResult == RCC_SUCCESS)
            break;
      }

   // If no interface found try to find interface in unmanaged state
   if (dwResult != RCC_SUCCESS)
   {
      for(i = 0; i < m_dwChildCount; i++)
         if ((m_pChildList[i]->getObjectClass() == OBJECT_INTERFACE) &&
             (m_pChildList[i]->Status() == STATUS_UNMANAGED) &&
             ((Interface *)m_pChildList[i])->getIpAddressList()->getFirstUnicastAddressV4().isValid())
         {
            dwResult = ((Interface *)m_pChildList[i])->wakeUp();
            if (dwResult == RCC_SUCCESS)
               break;
         }
   }

   UnlockChildList();
   return dwResult;
}

/**
 * Get status of interface with given index from SNMP agent
 */
void Node::getInterfaceStatusFromSNMP(SNMP_Transport *pTransport, UINT32 index, InterfaceAdminState *adminState, InterfaceOperState *operState)
{
	if (m_driver != NULL)
	{
      m_driver->getInterfaceState(pTransport, &m_customAttributes, m_driverData, index, adminState, operState);
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
void Node::writeParamListToMessage(NXCPMessage *pMsg, WORD flags)
{
   lockProperties();

	if ((flags & 0x01) && (m_paramList != NULL))
   {
      pMsg->setField(VID_NUM_PARAMETERS, (UINT32)m_paramList->size());

		int i;
		UINT32 dwId;
      for(i = 0, dwId = VID_PARAM_LIST_BASE; i < m_paramList->size(); i++)
      {
         dwId += m_paramList->get(i)->fillMessage(pMsg, dwId);
      }
		DbgPrintf(6, _T("Node[%s]::writeParamListToMessage(): sending %d parameters"), m_name, m_paramList->size());
   }
   else
   {
		DbgPrintf(6, _T("Node[%s]::writeParamListToMessage(): m_paramList == NULL"), m_name);
      pMsg->setField(VID_NUM_PARAMETERS, (UINT32)0);
   }

	if ((flags & 0x02) && (m_tableList != NULL))
   {
		pMsg->setField(VID_NUM_TABLES, (UINT32)m_tableList->size());

		int i;
		UINT32 dwId;
      for(i = 0, dwId = VID_TABLE_LIST_BASE; i < m_tableList->size(); i++)
      {
         dwId += m_tableList->get(i)->fillMessage(pMsg, dwId);
      }
		DbgPrintf(6, _T("Node[%s]::writeParamListToMessage(): sending %d tables"), m_name, m_tableList->size());
   }
   else
   {
		DbgPrintf(6, _T("Node[%s]::writeParamListToMessage(): m_tableList == NULL"), m_name);
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
void Node::openParamList(ObjectArray<AgentParameterDefinition> **paramList)
{
   lockProperties();
   *paramList = m_paramList;
}

/**
 * Open list of supported tables for reading
 */
void Node::openTableList(ObjectArray<AgentTableDefinition> **tableList)
{
   lockProperties();
   *tableList = m_tableList;
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

   if ((m_dwFlags & NF_IS_NATIVE_AGENT) &&
       (!(m_dwDynamicFlags & NDF_AGENT_UNREACHABLE)) &&
       (!(m_dwDynamicFlags & NDF_UNREACHABLE)))
   {
      AgentConnection *pConn;

      pConn = createAgentConnection();
      if (pConn != NULL)
      {
         dwError = pConn->checkNetworkService(pdwStatus, ipAddr, iServiceType, wPort, wProto, pszRequest, pszResponse, responseTime);
         pConn->disconnect();
         delete pConn;
      }
   }
   return dwError;
}

/**
 * Handler for object deletion
 */
void Node::onObjectDelete(UINT32 dwObjectId)
{
	lockProperties();
   if (dwObjectId == m_pollerNode)
   {
      // If deleted object is our poller node, change it to default
      m_pollerNode = 0;
      setModified();
      DbgPrintf(3, _T("Node \"%s\": poller node %d deleted"), m_name, dwObjectId);
   }
	unlockProperties();
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
         m_dwFlags |= NF_IS_OSPF;
      }
      else
      {
         m_dwFlags &= ~NF_IS_OSPF;
      }
		unlockProperties();
   }
}

/**
 * Create ready to use agent connection
 */
AgentConnectionEx *Node::createAgentConnection()
{
   AgentConnectionEx *conn;

   if ((!(m_dwFlags & NF_IS_NATIVE_AGENT)) ||
	    (m_dwFlags & NF_DISABLE_NXCP) ||
       (m_dwDynamicFlags & NDF_AGENT_UNREACHABLE) ||
       (m_dwDynamicFlags & NDF_UNREACHABLE))
      return NULL;

   DbgPrintf(6, _T("Node::createAgentConnection(%s [%d])"), m_name, (int)m_id);
   conn = new AgentConnectionEx(m_id, m_ipAddress, m_agentPort, m_agentAuthMethod, m_szSharedSecret);
   setAgentProxy(conn);
   if (!conn->connect(g_pServerKey))
   {
      delete conn;
      conn = NULL;
   }
	DbgPrintf(6, _T("Node::createAgentConnection(%s [%d]): conn=%p"), m_name, (int)m_id, conn);
   return conn;
}

/**
 * Set node's primary IP address.
 * Assumed that all necessary locks already in place
 */
void Node::setPrimaryIPAddress(const InetAddress& addr)
{
   if (addr.equals(m_ipAddress) || (m_dwFlags & NF_REMOTE_AGENT))
		return;

	if (IsZoningEnabled())
	{
		Zone *zone = FindZoneByGUID(m_zoneId);
		if (zone == NULL)
		{
			DbgPrintf(1, _T("Internal error: zone is NULL in Node::setPrimaryIPAddress (zone ID = %d)"), (int)m_zoneId);
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
   UINT32 i;

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
		m_dwDynamicFlags |= NDF_FORCE_CONFIGURATION_POLL | NDF_RECHECK_CAPABILITIES;

		// Change status of node and all it's childs to UNKNOWN
		m_iStatus = STATUS_UNKNOWN;
		LockChildList(FALSE);
		for(i = 0; i < m_dwChildCount; i++)
		{
			m_pChildList[i]->resetStatus();
			if (m_pChildList[i]->getObjectClass() == OBJECT_INTERFACE)
			{
				if (((Interface *)m_pChildList[i])->isFake())
				{
					((Interface *)m_pChildList[i])->setIpAddress(ipAddr);
				}
			}
		}
		UnlockChildList();

		setModified();
	}
   unlockProperties();

   agentLock();
   delete_and_null(m_pAgentConnection);
   agentUnlock();

   pollerUnlock();
}

/**
 * Change node's zone
 */
void Node::changeZone(UINT32 newZone)
{
   UINT32 i;

   pollerLock();

   lockProperties();

   m_zoneId = newZone;
   m_dwDynamicFlags |= NDF_FORCE_CONFIGURATION_POLL | NDF_RECHECK_CAPABILITIES;
	m_lastConfigurationPoll = 0;

	// Remove from subnets
	LockParentList(FALSE);
	NetObj **subnets = (NetObj **)malloc(sizeof(NetObj *) * m_dwParentCount);
	UINT32 count = 0;
	for(i = 0; i < m_dwParentCount; i++)
		if (m_pParentList[i]->getObjectClass() == OBJECT_SUBNET)
			subnets[count++] = m_pParentList[i];
	UnlockParentList();

	for(i = 0; i < count; i++)
	{
		DeleteParent(subnets[i]);
		subnets[i]->DeleteChild(this);
	}
	safe_free(subnets);

	// Change zone ID on interfaces
	LockChildList(FALSE);
	for(i = 0; i < m_dwChildCount; i++)
		if (m_pChildList[i]->getObjectClass() == OBJECT_INTERFACE)
			((Interface *)m_pChildList[i])->updateZoneId();
	UnlockChildList();

   setModified();
   unlockProperties();

   agentLock();
   delete_and_null(m_pAgentConnection);
   agentUnlock();

   pollerUnlock();
}

/**
 * Get number of interface objects and pointer to the last one
 */
UINT32 Node::getInterfaceCount(Interface **ppInterface)
{
   UINT32 i, dwCount;

   LockChildList(FALSE);
   for(i = 0, dwCount = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i]->getObjectClass() == OBJECT_INTERFACE)
      {
         dwCount++;
         *ppInterface = (Interface *)m_pChildList[i];
      }
   UnlockChildList();
   return dwCount;
}

/**
 * Get routing table from node
 */
ROUTING_TABLE *Node::getRoutingTable()
{
   ROUTING_TABLE *pRT = NULL;

   if ((m_dwFlags & NF_IS_NATIVE_AGENT) && (!(m_dwFlags & NF_DISABLE_NXCP)))
   {
      agentLock();
      if (connectToAgent())
      {
         pRT = m_pAgentConnection->getRoutingTable();
      }
      agentUnlock();
   }
   if ((pRT == NULL) && (m_dwFlags & NF_IS_SNMP) && (!(m_dwFlags & NF_DISABLE_SNMP)))
   {
		SNMP_Transport *pTransport;

		pTransport = createSnmpTransport();
		if (pTransport != NULL)
		{
			pRT = SnmpGetRoutingTable(m_snmpVersion, pTransport);
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
bool Node::getNextHop(const InetAddress& srcAddr, const InetAddress& destAddr, InetAddress *nextHop, UINT32 *ifIndex, bool *isVpn, TCHAR *name)
{
   UINT32 i;
   bool nextHopFound = false;
   *name = 0;

	// Check directly connected networks and VPN connectors
	bool nonFunctionalInterfaceFound = false;
	LockChildList(FALSE);
	for(i = 0; i < m_dwChildCount; i++)
	{
		if (m_pChildList[i]->getObjectClass() == OBJECT_VPNCONNECTOR)
		{
			if (((VPNConnector *)m_pChildList[i])->isRemoteAddr(destAddr) &&
				 ((VPNConnector *)m_pChildList[i])->isLocalAddr(srcAddr))
			{
				*nextHop = ((VPNConnector *)m_pChildList[i])->getPeerGatewayAddr();
				*ifIndex = m_pChildList[i]->getId();
				*isVpn = true;
            nx_strncpy(name, m_pChildList[i]->getName(), MAX_OBJECT_NAME);
				nextHopFound = true;
				break;
			}
		}
		else if ((m_pChildList[i]->getObjectClass() == OBJECT_INTERFACE) &&
               ((Interface *)m_pChildList[i])->getIpAddressList()->findSameSubnetAddress(destAddr).isValid())
		{
			*nextHop = destAddr;
			*ifIndex = ((Interface *)m_pChildList[i])->getIfIndex();
			*isVpn = false;
         nx_strncpy(name, m_pChildList[i]->getName(), MAX_OBJECT_NAME);
			if (m_pChildList[i]->Status() == SEVERITY_NORMAL)  /* TODO: use separate link status */
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
	UnlockChildList();

	// Check routing table
	// If directly connected subnet found, only check host routes
	nextHopFound = nextHopFound || nonFunctionalInterfaceFound;
   routingTableLock();
   if (m_pRoutingTable != NULL)
   {
      for(i = 0; i < (UINT32)m_pRoutingTable->iNumEntries; i++)
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
 * Update cached routing table
 */
void Node::updateRoutingTable()
{
	if (m_dwDynamicFlags & NDF_DELETE_IN_PROGRESS)
	{
		m_dwDynamicFlags &= ~NDF_QUEUED_FOR_ROUTE_POLL;
		return;
	}

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
   m_dwDynamicFlags &= ~NDF_QUEUED_FOR_ROUTE_POLL;
}

/**
 * Call SNMP Enumerate with node's SNMP parameters
 */
UINT32 Node::callSnmpEnumerate(const TCHAR *pszRootOid,
                              UINT32 (* pHandler)(UINT32, SNMP_Variable *, SNMP_Transport *, void *),
                              void *pArg, const TCHAR *context)
{
   if ((m_dwFlags & NF_IS_SNMP) &&
       (!(m_dwDynamicFlags & NDF_SNMP_UNREACHABLE)) &&
       (!(m_dwDynamicFlags & NDF_UNREACHABLE)))
	{
		SNMP_Transport *pTransport;
		UINT32 dwResult;

		pTransport = createSnmpTransport(0, context);
		if (pTransport != NULL)
		{
			dwResult = SnmpWalk(m_snmpVersion, pTransport, pszRootOid, pHandler, pArg, FALSE);
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
void Node::setAgentProxy(AgentConnection *pConn)
{
	UINT32 proxyNode = m_agentProxy;

	if (IsZoningEnabled() && (proxyNode == 0) && (m_zoneId != 0))
	{
		Zone *zone = (Zone *)g_idxZoneByGUID.get(m_zoneId);
		if (zone != NULL)
		{
			proxyNode = zone->getAgentProxy();
		}
	}

   if (proxyNode != 0)
   {
		Node *node = (Node *)g_idxNodeById.get(proxyNode);
      if (node != NULL)
      {
         pConn->setProxy(node->m_ipAddress, node->m_agentPort, node->m_agentAuthMethod, node->m_szSharedSecret);
      }
   }
}

/**
 * Callback for removing node from queue
 */
static bool NodeQueueComparator(void *key, void *element)
{
	return key == element;
}

/**
 * Prepare node object for deletion
 */
void Node::prepareForDeletion()
{
   // Prevent node from being queued for polling
   lockProperties();
   m_dwDynamicFlags |= NDF_POLLING_DISABLED | NDF_DELETE_IN_PROGRESS;
   unlockProperties();

	if (g_statusPollQueue.remove(this, NodeQueueComparator))
	{
		m_dwDynamicFlags &= ~NDF_QUEUED_FOR_STATUS_POLL;
		DbgPrintf(4, _T("Node::PrepareForDeletion(%s [%d]): removed from status poller queue"), m_name, (int)m_id);
		decRefCount();
	}

	if (g_configPollQueue.remove(this, NodeQueueComparator))
	{
		m_dwDynamicFlags &= ~NDF_QUEUED_FOR_CONFIG_POLL;
		DbgPrintf(4, _T("Node::PrepareForDeletion(%s [%d]): removed from configuration poller queue"), m_name, (int)m_id);
		decRefCount();
	}

	if (g_instancePollQueue.remove(this, NodeQueueComparator))
	{
		m_dwDynamicFlags &= ~NDF_QUEUED_FOR_INSTANCE_POLL;
		DbgPrintf(4, _T("Node::PrepareForDeletion(%s [%d]): removed from instance discovery poller queue"), m_name, (int)m_id);
		decRefCount();
	}

	if (g_discoveryPollQueue.remove(this, NodeQueueComparator))
	{
		m_dwDynamicFlags &= ~NDF_QUEUED_FOR_DISCOVERY_POLL;
		DbgPrintf(4, _T("Node::PrepareForDeletion(%s [%d]): removed from discovery poller queue"), m_name, (int)m_id);
		decRefCount();
	}

	if (g_routePollQueue.remove(this, NodeQueueComparator))
	{
		m_dwDynamicFlags &= ~NDF_QUEUED_FOR_ROUTE_POLL;
		DbgPrintf(4, _T("Node::PrepareForDeletion(%s [%d]): removed from routing table poller queue"), m_name, (int)m_id);
		decRefCount();
	}

	if (g_topologyPollQueue.remove(this, NodeQueueComparator))
	{
		m_dwDynamicFlags &= ~NDF_QUEUED_FOR_TOPOLOGY_POLL;
		DbgPrintf(4, _T("Node::PrepareForDeletion(%s [%d]): removed from topology poller queue"), m_name, (int)m_id);
		decRefCount();
	}

   // Wait for all pending polls
	DbgPrintf(4, _T("Node::PrepareForDeletion(%s [%d]): waiting for outstanding polls to finish"), m_name, (int)m_id);
   while(1)
   {
      lockProperties();
      if ((m_dwDynamicFlags &
            (NDF_QUEUED_FOR_STATUS_POLL | NDF_QUEUED_FOR_CONFIG_POLL |
             NDF_QUEUED_FOR_DISCOVERY_POLL | NDF_QUEUED_FOR_ROUTE_POLL |
             NDF_QUEUED_FOR_TOPOLOGY_POLL | NDF_QUEUED_FOR_INSTANCE_POLL)) == 0)
      {
         unlockProperties();
         break;
      }
      unlockProperties();
      ThreadSleepMs(100);
   }
	DbgPrintf(4, _T("Node::PrepareForDeletion(%s [%d]): no outstanding polls left"), m_name, (int)m_id);
	DataCollectionTarget::prepareForDeletion();
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
	UINT32 i;
	Cluster *pCluster = NULL;

	LockParentList(FALSE);
	for(i = 0; i < m_dwParentCount; i++)
		if (m_pParentList[i]->getObjectClass() == OBJECT_CLUSTER)
		{
			pCluster = (Cluster *)m_pParentList[i];
			break;
		}
	UnlockParentList();
	return pCluster;
}

/**
 * Create SNMP transport
 */
SNMP_Transport *Node::createSnmpTransport(WORD port, const TCHAR *context)
{
	SNMP_Transport *pTransport = NULL;
	UINT32 snmpProxy = m_snmpProxy;

	if (IsZoningEnabled() && (snmpProxy == 0) && (m_zoneId != 0))
	{
		// Use zone default proxy if set
		Zone *zone = (Zone *)g_idxZoneByGUID.get(m_zoneId);
		if (zone != NULL)
		{
			snmpProxy = zone->getSnmpProxy();
		}
	}

	if (snmpProxy == 0)
	{
		pTransport = new SNMP_UDPTransport;
		((SNMP_UDPTransport *)pTransport)->createUDPTransport(m_ipAddress, (port != 0) ? port : m_wSNMPPort);
	}
	else
	{
		Node *proxyNode = (Node *)g_idxNodeById.get(snmpProxy);
		if (proxyNode != NULL)
		{
			AgentConnection *pConn;

			pConn = proxyNode->createAgentConnection();
			if (pConn != NULL)
			{
				pTransport = new SNMP_ProxyTransport(pConn, m_ipAddress, (port != 0) ? port : m_wSNMPPort);
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
SNMP_SecurityContext *Node::getSnmpSecurityContext()
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

	// Try to resolve primary IP
   TCHAR name[MAX_OBJECT_NAME];
	if (m_ipAddress.getHostByAddr(name, MAX_OBJECT_NAME) != NULL)
	{
		nx_strncpy(m_name, name, MAX_OBJECT_NAME);
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
		LockChildList(FALSE);
		for(UINT32 i = 0; i < m_dwChildCount; i++)
		{
			if ((m_pChildList[i]->getObjectClass() == OBJECT_INTERFACE) && !((Interface *)m_pChildList[i])->isLoopback())
			{
            const InetAddressList *list = ((Interface *)m_pChildList[i])->getIpAddressList();
            for(int n = 0; n < list->size(); n++)
            {
               const InetAddress& a = list->get(i);
               if (a.isValidUnicast() && (a.getHostByAddr(name, MAX_OBJECT_NAME) != NULL))
               {
					   nx_strncpy(m_name, name, MAX_OBJECT_NAME);
					   bSuccess = TRUE;
					   break;
				   }
            }
			}
		}
		UnlockChildList();

		// Try to get hostname from agent if address resolution fails
		if (!(bSuccess || useOnlyDNS))
		{
			DbgPrintf(4, _T("Resolving name for node %d [%s] via agent..."), m_id, m_name);
			if (getItemFromAgent(_T("System.Hostname"), 256, szBuffer) == DCE_SUCCESS)
			{
				StrStrip(szBuffer);
				if (szBuffer[0] != 0)
				{
					nx_strncpy(m_name, szBuffer, MAX_OBJECT_NAME);
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
					nx_strncpy(m_name, szBuffer, MAX_OBJECT_NAME);
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
nxmap_ObjList *Node::getL2Topology()
{
	nxmap_ObjList *pResult;
	UINT32 dwExpTime;

	dwExpTime = ConfigReadULong(_T("TopologyExpirationTime"), 900);
	MutexLock(m_mutexTopoAccess);
	if ((m_pTopology == NULL) || (m_topologyRebuildTimestamp + (time_t)dwExpTime < time(NULL)))
	{
		pResult = NULL;
	}
	else
	{
		pResult = new nxmap_ObjList(m_pTopology);
	}
	MutexUnlock(m_mutexTopoAccess);
	return pResult;
}

/**
 * Rebuild layer 2 topology and return it as dynamically reated list which should be destroyed by caller
 */
nxmap_ObjList *Node::buildL2Topology(UINT32 *pdwStatus, int radius, bool includeEndNodes)
{
	nxmap_ObjList *pResult;
	int nDepth = (radius < 0) ? ConfigReadInt(_T("TopologyDiscoveryRadius"), 5) : radius;

	MutexLock(m_mutexTopoAccess);
	if (m_linkLayerNeighbors != NULL)
	{
		MutexUnlock(m_mutexTopoAccess);

		pResult = new nxmap_ObjList;
		BuildL2Topology(*pResult, this, nDepth, includeEndNodes);

		MutexLock(m_mutexTopoAccess);
		delete m_pTopology;
		m_pTopology = new nxmap_ObjList(pResult);
		m_topologyRebuildTimestamp = time(NULL);
	}
	else
	{
		pResult = NULL;
		delete_and_null(m_pTopology);
		*pdwStatus = RCC_NO_L2_TOPOLOGY_SUPPORT;
	}
	MutexUnlock(m_mutexTopoAccess);
	return pResult;
}

/**
 * Build IP topology
 */
nxmap_ObjList *Node::buildIPTopology(UINT32 *pdwStatus, int radius, bool includeEndNodes)
{
	int nDepth = (radius < 0) ? ConfigReadInt(_T("TopologyDiscoveryRadius"), 5) : radius;
	nxmap_ObjList *pResult = new nxmap_ObjList;
	buildIPTopologyInternal(*pResult, nDepth, 0, false, includeEndNodes);
	return pResult;
}

/**
 * Build IP topology
 */
void Node::buildIPTopologyInternal(nxmap_ObjList &topology, int nDepth, UINT32 seedSubnet, bool vpnLink, bool includeEndNodes)
{
	topology.addObject(m_id);
	if (seedSubnet != 0)
      topology.linkObjects(seedSubnet, m_id, vpnLink ? LINK_TYPE_VPN : LINK_TYPE_NORMAL);

	if (nDepth > 0)
	{
      UINT32 i;
      int j;

		ObjectArray<Subnet> subnets;
		LockParentList(FALSE);
		for(i = 0; i < m_dwParentCount; i++)
		{
			if ((m_pParentList[i]->getId() == seedSubnet) || (m_pParentList[i]->getObjectClass() != OBJECT_SUBNET))
				continue;

			topology.addObject(m_pParentList[i]->getId());
			topology.linkObjects(m_id, m_pParentList[i]->getId());
			m_pParentList[i]->incRefCount();
			subnets.add((Subnet *)m_pParentList[i]);
		}
		UnlockParentList();

		for(j = 0; j < subnets.size(); j++)
		{
			Subnet *s = subnets.get(j);
			s->buildIPTopologyInternal(topology, nDepth, m_id, includeEndNodes);
			s->decRefCount();
		}

      ObjectArray<Node> peers;
		LockChildList(FALSE);
		for(i = 0; i < m_dwChildCount; i++)
		{
			if (m_pChildList[i]->getObjectClass() != OBJECT_VPNCONNECTOR)
				continue;

         Node *node = (Node *)FindObjectById(((VPNConnector *)m_pChildList[i])->getPeerGatewayId(), OBJECT_NODE);
         if ((node != NULL) && (node->getId() != seedSubnet) && !topology.isObjectExist(node->getId()))
         {
            node->incRefCount();
			   peers.add(node);
         }
		}
		UnlockChildList();

		for(j = 0; j < peers.size(); j++)
		{
			Node *n = peers.get(j);
			n->buildIPTopologyInternal(topology, nDepth - 1, m_id, true, includeEndNodes);
			n->decRefCount();
		}
   }
}

/**
 * Topology poller
 */
void Node::topologyPoll(ClientSession *pSession, UINT32 dwRqId, int nPoller)
{
	if (m_dwDynamicFlags & NDF_DELETE_IN_PROGRESS)
	{
		if (dwRqId == 0)
			m_dwDynamicFlags &= ~NDF_QUEUED_FOR_TOPOLOGY_POLL;
		return;
	}

	pollerLock();
   m_pollRequestor = pSession;

   sendPollerMsg(dwRqId, _T("Starting topology poll for node %s\r\n"), m_name);
	DbgPrintf(4, _T("Started topology poll for node %s [%d]"), m_name, m_id);

	if (m_driver != NULL)
	{
		SNMP_Transport *snmp = createSnmpTransport();
		if (snmp != NULL)
		{
			VlanList *vlanList = m_driver->getVlans(snmp, &m_customAttributes, m_driverData);
			delete snmp;

			MutexLock(m_mutexTopoAccess);
			if (vlanList != NULL)
			{
				resolveVlanPorts(vlanList);
				sendPollerMsg(dwRqId, POLLER_INFO _T("VLAN list successfully retrieved from node\r\n"));
				DbgPrintf(4, _T("VLAN list retrieved from node %s [%d]"), m_name, m_id);
				if (m_vlans != NULL)
					m_vlans->decRefCount();
				m_vlans = vlanList;
			}
			else
			{
				sendPollerMsg(dwRqId, POLLER_WARNING _T("Cannot get VLAN list\r\n"));
				DbgPrintf(4, _T("Cannot retrieve VLAN list from node %s [%d]"), m_name, m_id);
				if (m_vlans != NULL)
					m_vlans->decRefCount();
				m_vlans = NULL;
			}
			MutexUnlock(m_mutexTopoAccess);

			lockProperties();
			UINT32 oldFlags = m_dwFlags;
			if (vlanList != NULL)
				m_dwFlags |= NF_HAS_VLANS;
			else
				m_dwFlags &= ~NF_HAS_VLANS;
			if (oldFlags != m_dwFlags)
				setModified();
			unlockProperties();
		}
	}

	ForwardingDatabase *fdb = GetSwitchForwardingDatabase(this);
	MutexLock(m_mutexTopoAccess);
	if (m_fdb != NULL)
		m_fdb->decRefCount();
	m_fdb = fdb;
	MutexUnlock(m_mutexTopoAccess);
	if (fdb != NULL)
	{
		DbgPrintf(4, _T("Switch forwarding database retrieved for node %s [%d]"), m_name, m_id);
	   sendPollerMsg(dwRqId, POLLER_INFO _T("Switch forwarding database retrieved\r\n"));
	}
	else
	{
		DbgPrintf(4, _T("Failed to get switch forwarding database from node %s [%d]"), m_name, m_id);
	   sendPollerMsg(dwRqId, POLLER_WARNING _T("Failed to get switch forwarding database\r\n"));
	}

	LinkLayerNeighbors *nbs = BuildLinkLayerNeighborList(this);
	if (nbs != NULL)
	{
		sendPollerMsg(dwRqId, POLLER_INFO _T("Link layer topology retrieved (%d connections found)\r\n"), nbs->size());
		DbgPrintf(4, _T("Link layer topology retrieved for node %s [%d] (%d connections found)"), m_name, (int)m_id, nbs->size());

		MutexLock(m_mutexTopoAccess);
		if (m_linkLayerNeighbors != NULL)
			m_linkLayerNeighbors->decRefCount();
		m_linkLayerNeighbors = nbs;
		MutexUnlock(m_mutexTopoAccess);

		// Walk through interfaces and update peers
	   sendPollerMsg(dwRqId, _T("Updating peer information on interfaces\r\n"));
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
					sendPollerMsg(dwRqId, _T("   Local interface %s linked to remote interface %s:%s\r\n"),
					              ifLocal->getName(), object->getName(), ifRemote->getName());
					DbgPrintf(5, _T("Local interface %s:%s linked to remote interface %s:%s"),
					          m_name, ifLocal->getName(), object->getName(), ifRemote->getName());
				}
			}
		}

      LockChildList(FALSE);
      for(DWORD i = 0; i < m_dwChildCount; i++)
      {
         if (m_pChildList[i]->getObjectClass() != OBJECT_INTERFACE)
            continue;

         Interface *iface = (Interface *)m_pChildList[i];

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
      UnlockChildList();

	   sendPollerMsg(dwRqId, _T("Link layer topology processed\r\n"));
		DbgPrintf(4, _T("Link layer topology processed for node %s [%d]"), m_name, m_id);
	}
	else
	{
	   sendPollerMsg(dwRqId, POLLER_ERROR _T("Link layer topology retrieved\r\n"));
	}

	// Read list of associated wireless stations
	if ((m_driver != NULL) && (m_dwFlags & NF_IS_WIFI_CONTROLLER))
	{
		SNMP_Transport *snmp = createSnmpTransport();
		if (snmp != NULL)
		{
			ObjectArray<WirelessStationInfo> *stations = m_driver->getWirelessStations(snmp, &m_customAttributes, m_driverData);
			delete snmp;
			if (stations != NULL)
			{
				sendPollerMsg(dwRqId, _T("   %d wireless stations found\r\n"), stations->size());
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
   for(UINT32 i = 0; i < g_dwNumModules; i++)
	{
		if (g_pModuleList[i].pfTopologyPollHook != NULL)
		{
			DbgPrintf(5, _T("TopologyPoll(%s [%d]): calling hook in module %s"), m_name, m_id, g_pModuleList[i].szName);
			g_pModuleList[i].pfTopologyPollHook(this, pSession, dwRqId, nPoller);
		}
	}

	// Execute hook script
   SetPollerInfo(nPoller, _T("hook"));
	executeHookScript(_T("TopologyPoll"));

   sendPollerMsg(dwRqId, _T("Finished topology poll for node %s\r\n"), m_name);

	m_lastTopologyPoll = time(NULL);
   m_dwDynamicFlags &= ~NDF_QUEUED_FOR_TOPOLOGY_POLL;
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

	LockChildList(FALSE);
	for(int i = 0; i < (int)m_dwChildCount; i++)
	{
		if (m_pChildList[i]->getObjectClass() != OBJECT_INTERFACE)
			continue;

		Interface *ifLocal = (Interface *)m_pChildList[i];
		BYTE macAddr[MAC_ADDR_LENGTH];
		if (fdb->isSingleMacOnPort(ifLocal->getIfIndex(), macAddr))
		{
			Interface *ifRemote = FindInterfaceByMAC(macAddr);
			if (ifRemote != NULL)
			{
				Node *peerNode = ifRemote->getParentNode();
				if (peerNode != NULL)
				{
					LL_NEIGHBOR_INFO info;

					info.ifLocal = ifLocal->getIfIndex();
					info.ifRemote = ifRemote->getIfIndex();
					info.objectId = peerNode->getId();
					info.isPtToPt = true;
					info.protocol = LL_PROTO_FDB;
					nbs->addConnection(&info);
				}
			}
		}
	}
	UnlockChildList();

	fdb->decRefCount();
}

/**
 * Add existing connections to link layer neighbours table
 */
void Node::addExistingConnections(LinkLayerNeighbors *nbs)
{
	LockChildList(FALSE);
	for(int i = 0; i < (int)m_dwChildCount; i++)
	{
		if (m_pChildList[i]->getObjectClass() != OBJECT_INTERFACE)
			continue;

		Interface *ifLocal = (Interface *)m_pChildList[i];
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
	UnlockChildList();
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
				case VLAN_PRM_IFINDEX:	// interface index
               iface = findInterfaceByIndex(portId);
					break;
				case VLAN_PRM_BPORT:		// bridge port number
					iface = findBridgePort(portId);
					break;
				case VLAN_PRM_SLOTPORT:	// slot/port pair
					iface = findInterfaceBySlotAndPort(portId >> 16, portId & 0xFFFF);
					break;
			}
			if (iface != NULL)
				vlan->resolvePort(j, (iface->getSlotNumber() << 16) | iface->getPortNumber(), iface->getIfIndex(), iface->getId());
		}
	}
}

/**
 * Create new subnet and bins to this node
 */
Subnet *Node::createSubnet(const InetAddress& baseAddr, bool syntheticMask)
{
   InetAddress addr = baseAddr.getSubnetAddress();
   if (syntheticMask)
   {
      while(FindSubnetByIP(m_zoneId, addr) != NULL)
      {
         addr.setMaskBits(addr.getMaskBits() + 1);
      }
   }
   Subnet *s = new Subnet(addr, m_zoneId, syntheticMask);
   NetObjInsert(s, TRUE);
   if (g_flags & AF_ENABLE_ZONING)
   {
	   Zone *zone = FindZoneByGUID(m_zoneId);
	   if (zone != NULL)
	   {
		   zone->addSubnet(s);
	   }
	   else
	   {
		   DbgPrintf(1, _T("Node::createSubnet(): Inconsistent configuration - zone %d does not exist"), (int)m_zoneId);
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
   LockChildList(FALSE);
   for(UINT32 n = 0; n < m_dwChildCount; n++)
   {
      if (m_pChildList[n]->getObjectClass() != OBJECT_INTERFACE)
         continue;
      Interface *iface = (Interface *)m_pChildList[n];
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
   UnlockChildList();

   // Check if we have subnet bindings for all interfaces
   DbgPrintf(5, _T("Checking subnet bindings for node %s [%d]"), m_name, m_id);
   for(int i = 0; i < addrList.size(); i++)
   {
      const InetAddress& addr = addrList.get(i);
      DbgPrintf(5, _T("Node::checkSubnetBinding(%s [%d]): checking address %s/%d"), m_name, m_id, addr.toString(buffer), addr.getMaskBits());

	   Interface *iface = findInterfaceByIP(addr);
      if (iface == NULL)
	   {
		   nxlog_write(MSG_INTERNAL_ERROR, EVENTLOG_WARNING_TYPE, "s", _T("Cannot find interface object in Node::checkSubnetBinding()"));
		   continue;	// Something goes really wrong
	   }

	   // Is cluster interconnect interface?
	   bool isSync = (pCluster != NULL) ? pCluster->isSyncAddr(addr) : false;

	   Subnet *pSubnet = FindSubnetForNode(m_zoneId, addr);
	   if (pSubnet != NULL)
	   {
         DbgPrintf(5, _T("Node::checkSubnetBinding(%s [%d]): found subnet %s [%d]"), m_name, m_id, pSubnet->getName(), pSubnet->getId());
		   if (isSync)
		   {
			   pSubnet = NULL;	// No further checks on this subnet
		   }
		   else
		   {
            if (pSubnet->isSyntheticMask() && !iface->isSyntheticMask())
			   {
				   DbgPrintf(4, _T("Setting correct netmask for subnet %s [%d] from node %s [%d]"),
							    pSubnet->getName(), pSubnet->getId(), m_name, m_id);
               if ((addr.getHostBits() < 2) && (getParentCount() > 1))
               {
                  /* Delete subnet object if we try to change it's netmask to 255.255.255.255 or 255.255.255.254 and
                   node has more than one parent. hasIfaceForPrimaryIp paramteter should prevent us from going in
                   loop(creating and deleting all the time subnet). */
                  pSubnet->deleteObject();
                  pSubnet = NULL;   // prevent binding to deleted subnet
               }
               else
               {
                  pSubnet->setCorrectMask(addr.getSubnetAddress());
               }
			   }

			   // Check if node is linked to this subnet
			   if ((pSubnet != NULL) && !pSubnet->isChild(m_id))
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
            pSubnet = createSubnet(addr, false);
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
   if (m_ipAddress.isValidUnicast() && !(m_dwFlags & NF_REMOTE_AGENT) && !addrList.hasAddress(m_ipAddress))
   {
	   Subnet *pSubnet = FindSubnetForNode(m_zoneId, m_ipAddress);
      if (pSubnet != NULL)
      {
		   // Check if node is linked to this subnet
		   if (!pSubnet->isChild(m_id))
		   {
			   DbgPrintf(4, _T("Restored link between subnet %s [%d] and node %s [%d]"),
						    pSubnet->getName(), pSubnet->getId(), m_name, m_id);
			   pSubnet->addNode(this);
		   }
      }
      else
      {
         InetAddress addr(m_ipAddress);
         addr.setMaskBits((addr.getFamily() == AF_INET) ? 24 : 64);
		   pSubnet = createSubnet(addr, true);
      }
   }

	// Check for incorrect parent subnets
	LockParentList(FALSE);
	LockChildList(FALSE);
   ObjectArray<NetObj> unlinkList(m_dwParentCount, 8, false);
	for(UINT32 i = 0; i < m_dwParentCount; i++)
	{
		if (m_pParentList[i]->getObjectClass() == OBJECT_SUBNET)
		{
			Subnet *pSubnet = (Subnet *)m_pParentList[i];
			if (pSubnet->getIpAddress().contain(m_ipAddress) && !(m_dwFlags & NF_REMOTE_AGENT))
            continue;   // primary IP is in given subnet

         int j;
         for(j = 0; j < addrList.size(); j++)
			{
            const InetAddress& addr = addrList.get(j);
				if (pSubnet->getIpAddress().contain(addr))
				{
					if ((pCluster != NULL) && pCluster->isSyncAddr(addr))
               {
						j = addrList.size();	// Cause to unbind from this subnet
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
	UnlockChildList();
	UnlockParentList();

	// Unlink for incorrect subnet objects
   for(int n = 0; n < unlinkList.size(); n++)
	{
      NetObj *o = unlinkList.get(n);
      o->DeleteChild(this);
		DeleteParent(o);
      o->calculateCompoundStatus();
	}
}

/**
 * Update interface names
 */
void Node::updateInterfaceNames(ClientSession *pSession, UINT32 dwRqId)
{
   pollerLock();
   m_pollRequestor = pSession;
   sendPollerMsg(dwRqId, _T("Starting interface names poll for node %s\r\n"), m_name);
   DbgPrintf(4, _T("Starting interface names poll for node %s (ID: %d)"), m_name, m_id);

   // Retrieve interface list
   InterfaceList *pIfList = getInterfaceList();
   if (pIfList != NULL)
   {
      // Check names of existing interfaces
      for(int j = 0; j < pIfList->size(); j++)
      {
         InterfaceInfo *ifInfo = pIfList->get(j);

         LockChildList(FALSE);
         for(UINT32 i = 0; i < m_dwChildCount; i++)
         {
            if (m_pChildList[i]->getObjectClass() == OBJECT_INTERFACE)
            {
               Interface *pInterface = (Interface *)m_pChildList[i];

               if (ifInfo->index == pInterface->getIfIndex())
               {
						sendPollerMsg(dwRqId, _T("   Checking interface %d (%s)\r\n"), pInterface->getIfIndex(), pInterface->getName());
                  if (_tcscmp(ifInfo->name, pInterface->getName()))
                  {
                     pInterface->setName(ifInfo->name);
							sendPollerMsg(dwRqId, POLLER_WARNING _T("   Name of interface %d changed to %s\r\n"), pInterface->getIfIndex(), ifInfo->name);
                  }
                  if (_tcscmp(ifInfo->description, pInterface->getDescription()))
                  {
                     pInterface->setDescription(ifInfo->description);
							sendPollerMsg(dwRqId, POLLER_WARNING _T("   Description of interface %d changed to %s\r\n"), pInterface->getIfIndex(), ifInfo->description);
                  }
                  if (_tcscmp(ifInfo->alias, pInterface->getAlias()))
                  {
                     pInterface->setAlias(ifInfo->alias);
							sendPollerMsg(dwRqId, POLLER_WARNING _T("   Alias of interface %d changed to %s\r\n"), pInterface->getIfIndex(), ifInfo->alias);
                  }
                  break;
               }
            }
         }
         UnlockChildList();
      }

      delete pIfList;
   }
   else     /* pIfList == NULL */
   {
      sendPollerMsg(dwRqId, POLLER_ERROR _T("   Unable to get interface list from node\r\n"));
   }

   // Finish poll
	sendPollerMsg(dwRqId, _T("Finished interface names poll for node %s\r\n"), m_name);
   pollerUnlock();
   DbgPrintf(4, _T("Finished interface names poll for node %s (ID: %d)"), m_name, m_id);
}

/**
 * Get list of parent objects for NXSL script
 */
NXSL_Array *Node::getParentsForNXSL()
{
	NXSL_Array *parents = new NXSL_Array;
	int index = 0;

	LockParentList(FALSE);
	for(UINT32 i = 0; i < m_dwParentCount; i++)
	{
		if (((m_pParentList[i]->getObjectClass() == OBJECT_CONTAINER) ||
		     (m_pParentList[i]->getObjectClass() == OBJECT_CLUSTER) ||
			  (m_pParentList[i]->getObjectClass() == OBJECT_SUBNET) ||
			  (m_pParentList[i]->getObjectClass() == OBJECT_SERVICEROOT)) &&
		    m_pParentList[i]->isTrustedNode(m_id))
		{
			parents->set(index++, new NXSL_Value(new NXSL_Object(&g_nxslNetObjClass, m_pParentList[i])));
		}
	}
	UnlockParentList();

	return parents;
}

/**
 * Get list of template type parent objects for NXSL script
 */
NXSL_Array *Node::getTemplatesForNXSL()
{
	NXSL_Array *parents = new NXSL_Array;
	int index = 0;

	LockParentList(FALSE);
	for(UINT32 i = 0; i < m_dwParentCount; i++)
	{
		if ((m_pParentList[i]->getObjectClass() == OBJECT_TEMPLATE) &&
		    m_pParentList[i]->isTrustedNode(m_id))
		{
			parents->set(index++, new NXSL_Value(new NXSL_Object(&g_nxslNetObjClass, m_pParentList[i])));
		}
	}
	UnlockParentList();

	return parents;
}

/**
 * Get list of interface objects for NXSL script
 */
NXSL_Array *Node::getInterfacesForNXSL()
{
	NXSL_Array *ifaces = new NXSL_Array;
	int index = 0;

	LockChildList(FALSE);
	for(UINT32 i = 0; i < m_dwChildCount; i++)
	{
		if (m_pChildList[i]->getObjectClass() == OBJECT_INTERFACE)
		{
			ifaces->set(index++, new NXSL_Value(new NXSL_Object(&g_nxslInterfaceClass, m_pChildList[i])));
		}
	}
	UnlockChildList();

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
	LinkLayerNeighbors *nbs;

	MutexLock(m_mutexTopoAccess);
	if (m_linkLayerNeighbors != NULL)
		m_linkLayerNeighbors->incRefCount();
	nbs = m_linkLayerNeighbors;
	MutexUnlock(m_mutexTopoAccess);
	return nbs;
}

/**
 * Get VLANs
 */
VlanList *Node::getVlans()
{
	VlanList *vlans;

	MutexLock(m_mutexTopoAccess);
	if (m_vlans != NULL)
		m_vlans->incRefCount();
	vlans = m_vlans;
	MutexUnlock(m_mutexTopoAccess);
	return vlans;
}

/**
 * Substitute % macros in given text with actual values
 */
TCHAR *Node::expandText(const TCHAR *textTemplate)
{
   const TCHAR *pCurr;
   UINT32 dwPos, dwSize;
   TCHAR *pText, scriptName[256];
	int i;

   dwSize = (UINT32)_tcslen(textTemplate) + 1;
   pText = (TCHAR *)malloc(dwSize * sizeof(TCHAR));
   for(pCurr = textTemplate, dwPos = 0; *pCurr != 0; pCurr++)
   {
      switch(*pCurr)
      {
         case '%':   // Metacharacter
            pCurr++;
            if (*pCurr == 0)
            {
               pCurr--;
               break;   // Abnormal loop termination
            }
            switch(*pCurr)
            {
               case '%':
                  pText[dwPos++] = '%';
                  break;
               case 'n':   // Name of the node
                  dwSize += (UINT32)_tcslen(m_name);
                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
                  _tcscpy(&pText[dwPos], m_name);
                  dwPos += (UINT32)_tcslen(m_name);
                  break;
               case 'a':   // IP address of the node
                  dwSize += 48;
                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
						m_ipAddress.toString(&pText[dwPos]);
                  dwPos = (UINT32)_tcslen(pText);
                  break;
               case 'i':   // Node identifier
                  dwSize += 10;
                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
                  _sntprintf(&pText[dwPos], 11, _T("0x%08X"), m_id);
                  dwPos = (UINT32)_tcslen(pText);
                  break;
               case 'v':   // NetXMS server version
                  dwSize += (UINT32)_tcslen(NETXMS_VERSION_STRING);
                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
                  _tcscpy(&pText[dwPos], NETXMS_VERSION_STRING);
                  dwPos += (UINT32)_tcslen(NETXMS_VERSION_STRING);
                  break;
					case '[':	// Script
						for(i = 0, pCurr++; (*pCurr != ']') && (*pCurr != 0) && (i < 255); pCurr++)
						{
							scriptName[i++] = *pCurr;
						}
						if (*pCurr == 0)	// no terminating ]
						{
							pCurr--;
						}
						else
						{
							scriptName[i] = 0;
							StrStrip(scriptName);

                     NXSL_VM *vm = g_pScriptLibrary->createVM(scriptName, new NXSL_ServerEnv);
							if (vm != NULL)
							{
								vm->setGlobalVariable(_T("$node"), new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, this)));

								if (vm->run())
								{
									NXSL_Value *result = vm->getResult();
									if (result != NULL)
									{
										const TCHAR *temp = result->getValueAsCString();
										if (temp != NULL)
										{
											dwSize += (UINT32)_tcslen(temp);
											pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
											_tcscpy(&pText[dwPos], temp);
											dwPos += (UINT32)_tcslen(temp);
											DbgPrintf(4, _T("Node::expandText(\"%s\"): Script %s executed successfully"),
														 textTemplate, scriptName);
										}
									}
								}
								else
								{
									DbgPrintf(4, _T("Node::expandText(\"%s\"): Script %s execution error: %s"),
												 textTemplate, scriptName, vm->getErrorText());
									PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", scriptName, vm->getErrorText(), 0);
								}
                        delete vm;
							}
							else
							{
								DbgPrintf(4, _T("Node::expandText(\"%s\"): Cannot find script %s"), textTemplate, scriptName);
							}
						}
						break;
					case '{':	// Custom attribute
						for(i = 0, pCurr++; (*pCurr != '}') && (*pCurr != 0) && (i < 255); pCurr++)
						{
							scriptName[i++] = *pCurr;
						}
						if (*pCurr == 0)	// no terminating }
						{
							pCurr--;
						}
						else
						{
							scriptName[i] = 0;
							StrStrip(scriptName);
							const TCHAR *temp = getCustomAttribute(scriptName);
							if (temp != NULL)
							{
								dwSize += (UINT32)_tcslen(temp);
								pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
								_tcscpy(&pText[dwPos], temp);
								dwPos += (UINT32)_tcslen(temp);
							}
						}
						break;
               default:    // All other characters are invalid, ignore
                  break;
            }
            break;
         case '\\':  // Escape character
            pCurr++;
            if (*pCurr == 0)
            {
               pCurr--;
               break;   // Abnormal loop termination
            }
            switch(*pCurr)
            {
               case 't':
                  pText[dwPos++] = '\t';
                  break;
               case 'n':
                  pText[dwPos++] = 0x0D;
                  pText[dwPos++] = 0x0A;
                  break;
               default:
                  pText[dwPos++] = *pCurr;
                  break;
            }
            break;
         default:
            pText[dwPos++] = *pCurr;
            break;
      }
   }
   pText[dwPos] = 0;
   return pText;
}

/**
 * Check and update last agent trap ID
 */
bool Node::checkAgentTrapId(QWORD trapId)
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
bool Node::checkSNMPTrapId(UINT32 trapId)
{
	lockProperties();
	bool valid = (trapId > m_lastSNMPTrapId);
	if (valid)
		m_lastSNMPTrapId = trapId;
	unlockProperties();
	return valid;
}

/**
 * Check and update last agent data push request ID
 */
bool Node::checkAgentPushRequestId(QWORD requestId)
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
 * Execute hook script
 *
 * @param hookName hook name. Will find and excute script named Hook::hookName
 */
void Node::executeHookScript(const TCHAR *hookName)
{
	TCHAR scriptName[MAX_PATH] = _T("Hook::");
	nx_strncpy(&scriptName[6], hookName, MAX_PATH - 6);
	NXSL_VM *vm = g_pScriptLibrary->createVM(scriptName, new NXSL_ServerEnv);
	if (vm == NULL)
	{
		DbgPrintf(7, _T("Node::executeHookScript(%s [%u]): hook script \"%s\" not found"), m_name, m_id, scriptName);
		return;
	}

	vm->setGlobalVariable(_T("$node"), new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, this)));
	if (!vm->run())
	{
		DbgPrintf(4, _T("Node::executeHookScript(%s [%u]): hook script \"%s\" execution error: %s"),
		          m_name, m_id, scriptName, vm->getErrorText());
	}
   delete vm;
}

/**
 * Check if data collection is disabled
 */
bool Node::isDataCollectionDisabled()
{
	return (m_dwFlags & NF_DISABLE_DATA_COLLECT) != 0;
}

/**
 * Get LLDP local port info by LLDP local ID
 *
 * @param id port ID
 * @param idLen port ID length in bytes
 * @param buffer buffer for storing port information
 */
bool Node::getLldpLocalPortInfo(BYTE *id, size_t idLen, LLDP_LOCAL_PORT_INFO *buffer)
{
	bool result = false;
	lockProperties();
	if (m_lldpLocalPortInfo != NULL)
	{
		for(int i = 0; i < m_lldpLocalPortInfo->size(); i++)
		{
			LLDP_LOCAL_PORT_INFO *port = m_lldpLocalPortInfo->get(i);
			if ((idLen == port->localIdLen) && !memcmp(id, port->localId, idLen))
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
   if (IsZoningEnabled() && (m_zoneId != 0) && (icmpProxy == 0))
   {
      Zone *zone = (Zone *)g_idxZoneByGUID.get(m_zoneId);
      if (zone != NULL)
      {
         icmpProxy = zone->getIcmpProxy();
      }
   }

   if (icmpProxy != 0)
   {
      DbgPrintf(7, _T("Node::updatePingData: ping via proxy [%u]"), icmpProxy);
      Node *proxyNode = (Node *)g_idxNodeById.get(icmpProxy);
      if ((proxyNode != NULL) && proxyNode->isNativeAgent() && !proxyNode->isDown())
      {
         DbgPrintf(7, _T("Node::updatePingData: proxy node found: %s"), proxyNode->getName());
         AgentConnection *conn = proxyNode->createAgentConnection();
         if (conn != NULL)
         {
            TCHAR parameter[128], buffer[64];

            _sntprintf(parameter, 128, _T("Icmp.Ping(%s)"), m_ipAddress.toString(buffer));
            if (conn->getParameter(parameter, 64, buffer) == ERR_SUCCESS)
            {
               DbgPrintf(7, _T("Node::updatePingData:  proxy response: \"%s\""), buffer);
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
                  DbgPrintf(7, _T("Node::updatePingData: incorrect value: %d or error while parsing: %s"), value, eptr);
               }
            }
            conn->disconnect();
            delete conn;
         }
         else
         {
            DbgPrintf(7, _T("Node::updatePingData: cannot connect to agent on proxy node [%u]"), icmpProxy);
         }
      }
      else
      {
         DbgPrintf(7, _T("Node::updatePingData: proxy node not available [%u]"), icmpProxy);
      }
   }
   else	// not using ICMP proxy
   {
      UINT32 dwPingStatus = IcmpPing(m_ipAddress, 3, g_icmpPingTimeout, &m_pingTime, g_icmpPingSize);
      if (dwPingStatus != ICMP_SUCCESS)
      {
         DbgPrintf(7, _T("Node::updatePingData: error getting ping %d"), dwPingStatus);
         m_pingTime = PING_TIME_TIMEOUT;
      }
      m_pingLastTimeStamp = time(NULL);
   }
}
