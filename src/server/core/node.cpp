/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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


//
// Node class default constructor
//

Node::Node() : Template()
{
	m_primaryName[0] = 0;
   m_iStatus = STATUS_UNKNOWN;
   m_dwFlags = 0;
   m_dwDynamicFlags = 0;
   m_zoneId = 0;
   m_wAgentPort = AGENT_LISTEN_PORT;
   m_wAuthMethod = AUTH_NONE;
   m_szSharedSecret[0] = 0;
   m_iStatusPollType = POLL_ICMP_PING;
   m_snmpVersion = SNMP_VERSION_1;
   m_wSNMPPort = SNMP_DEFAULT_PORT;
	char community[MAX_COMMUNITY_LENGTH];
   ConfigReadStrA(_T("DefaultCommunityString"), community, MAX_COMMUNITY_LENGTH, "public");
	m_snmpSecurity = new SNMP_SecurityContext(community);
   m_szObjectId[0] = 0;
   m_tLastDiscoveryPoll = 0;
   m_tLastStatusPoll = 0;
   m_tLastConfigurationPoll = 0;
	m_tLastTopologyPoll = 0;
   m_tLastRTUpdate = 0;
   m_hPollerMutex = MutexCreate();
   m_hAgentAccessMutex = MutexCreate();
   m_mutexRTAccess = MutexCreate();
	m_mutexTopoAccess = MutexCreate();
   m_pAgentConnection = NULL;
	m_lastAgentTrapId = 0;
   m_szAgentVersion[0] = 0;
   m_szPlatformName[0] = 0;
	m_sysDescription = NULL;
	m_sysName = NULL;
	m_lldpNodeId = NULL;
   m_paramList = NULL;
	m_tableList = NULL;
   m_dwPollerNode = 0;
   m_dwProxyNode = 0;
	m_dwSNMPProxy = 0;
   memset(m_qwLastEvents, 0, sizeof(QWORD) * MAX_LAST_EVENTS);
   m_pRoutingTable = NULL;
   m_tFailTimeSNMP = 0;
   m_tFailTimeAgent = 0;
	m_linkLayerNeighbors = NULL;
	m_vrrpInfo = NULL;
	m_tLastTopologyPoll = 0;
	m_pTopology = NULL;
	m_topologyRebuildTimestamp = 0;
	m_iPendingStatus = -1;
	m_iPollCount = 0;
	m_iRequiredPollCount = 0;	// Use system default
	m_nUseIfXTable = IFXTABLE_DEFAULT;	// Use system default
	m_jobQueue = new ServerJobQueue();
	m_fdb = NULL;
	m_vlans = NULL;
	m_driver = NULL;
	m_components = NULL;
	memset(m_baseBridgeAddress, 0, MAC_ADDR_LENGTH);
}


//
// Constructor for new node object
//

Node::Node(DWORD dwAddr, DWORD dwFlags, DWORD dwProxyNode, DWORD dwSNMPProxy, DWORD dwZone) : Template()
{
	IpToStr(dwAddr, m_primaryName);
   m_iStatus = STATUS_UNKNOWN;
   m_dwIpAddr = dwAddr;
   m_dwFlags = dwFlags;
   m_dwDynamicFlags = 0;
   m_zoneId = dwZone;
   m_wAgentPort = AGENT_LISTEN_PORT;
   m_wAuthMethod = AUTH_NONE;
   m_szSharedSecret[0] = 0;
   m_iStatusPollType = POLL_ICMP_PING;
   m_snmpVersion = SNMP_VERSION_1;
   m_wSNMPPort = SNMP_DEFAULT_PORT;
	char community[MAX_COMMUNITY_LENGTH];
   ConfigReadStrA(_T("DefaultCommunityString"), community, MAX_COMMUNITY_LENGTH, "public");
	m_snmpSecurity = new SNMP_SecurityContext(community);
   IpToStr(dwAddr, m_szName);    // Make default name from IP address
   m_szObjectId[0] = 0;
   m_tLastDiscoveryPoll = 0;
   m_tLastStatusPoll = 0;
   m_tLastConfigurationPoll = 0;
	m_tLastTopologyPoll = 0;
   m_tLastRTUpdate = 0;
   m_hPollerMutex = MutexCreate();
   m_hAgentAccessMutex = MutexCreate();
   m_mutexRTAccess = MutexCreate();
	m_mutexTopoAccess = MutexCreate();
   m_pAgentConnection = NULL;
	m_lastAgentTrapId = 0;
   m_szAgentVersion[0] = 0;
   m_szPlatformName[0] = 0;
	m_sysDescription = NULL;
	m_sysName = NULL;
	m_lldpNodeId = NULL;
   m_paramList = NULL;
	m_tableList = NULL;
   m_dwPollerNode = 0;
   m_dwProxyNode = dwProxyNode;
	m_dwSNMPProxy = dwSNMPProxy;
   memset(m_qwLastEvents, 0, sizeof(QWORD) * MAX_LAST_EVENTS);
   m_bIsHidden = TRUE;
   m_pRoutingTable = NULL;
   m_tFailTimeSNMP = 0;
   m_tFailTimeAgent = 0;
	m_linkLayerNeighbors = NULL;
	m_vrrpInfo = NULL;
	m_tLastTopologyPoll = 0;
	m_pTopology = NULL;
	m_topologyRebuildTimestamp = 0;
	m_iPendingStatus = -1;
	m_iPollCount = 0;
	m_iRequiredPollCount = 0;	// Use system default
	m_nUseIfXTable = IFXTABLE_DEFAULT;	// Use system default
	m_jobQueue = new ServerJobQueue();
	m_fdb = NULL;
	m_vlans = NULL;
	m_driver = NULL;
	m_components = NULL;
	memset(m_baseBridgeAddress, 0, MAC_ADDR_LENGTH);
}


//
// Node destructor
//

Node::~Node()
{
   MutexDestroy(m_hPollerMutex);
   MutexDestroy(m_hAgentAccessMutex);
   MutexDestroy(m_mutexRTAccess);
	MutexDestroy(m_mutexTopoAccess);
   delete m_pAgentConnection;
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
	delete m_components;
}


//
// Create object from database data
//

BOOL Node::CreateFromDB(DWORD dwId)
{
   TCHAR query[1024];
   DB_RESULT hResult;
   int i, iNumRows;
   DWORD dwSubnetId;
   NetObj *pObject;
   BOOL bResult = FALSE;

   m_dwId = dwId;

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
      _T("runtime_flags FROM nodes WHERE id=?"));
	if (hStmt == NULL)
		return FALSE;

	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, dwId);
	hResult = DBSelectPrepared(hStmt);
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
   m_dwIpAddr = DBGetFieldIPAddr(hResult, 0, 1);
   m_dwFlags = DBGetFieldULong(hResult, 0, 2);
   m_snmpVersion = DBGetFieldLong(hResult, 0, 3);
   m_wAuthMethod = (WORD)DBGetFieldLong(hResult, 0, 4);
   DBGetField(hResult, 0, 5, m_szSharedSecret, MAX_SECRET_LENGTH);
   m_wAgentPort = (WORD)DBGetFieldLong(hResult, 0, 6);
   m_iStatusPollType = DBGetFieldLong(hResult, 0, 7);
   DBGetField(hResult, 0, 8, m_szObjectId, MAX_OID_LEN * 4);
   DBGetField(hResult, 0, 9, m_szAgentVersion, MAX_AGENT_VERSION_LEN);
   DBGetField(hResult, 0, 10, m_szPlatformName, MAX_PLATFORM_NAME_LEN);
   m_dwPollerNode = DBGetFieldULong(hResult, 0, 11);
   m_zoneId = DBGetFieldULong(hResult, 0, 12);
   m_dwProxyNode = DBGetFieldULong(hResult, 0, 13);
   m_dwSNMPProxy = DBGetFieldULong(hResult, 0, 14);
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

   DBFreeResult(hResult);
	DBFreeStatement(hStmt);

   if (!m_bIsDeleted)
   {
      // Link node to subnets
      _sntprintf(query, 1024, _T("SELECT subnet_id FROM nsmap WHERE node_id=%d"), dwId);
      hResult = DBSelect(g_hCoreDB, query);
      if (hResult == NULL)
         return FALSE;     // Query failed

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
         else if (pObject->Type() != OBJECT_SUBNET)
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
      loadItemsFromDB();
      loadACLFromDB();

      // Walk through all items in the node and load appropriate thresholds
		bResult = TRUE;
      for(i = 0; i < m_dcObjects->size(); i++)
         if (!m_dcObjects->get(i)->loadThresholdsFromDB())
         {
            DbgPrintf(3, _T("Cannot load thresholds for DCI %d of node %d (%s)"),
                      m_dcObjects->get(i)->getId(), dwId, m_szName);
            bResult = FALSE;
         }
   }
   else
   {
      bResult = TRUE;
   }

   return bResult;
}


//
// Save object to database
//

BOOL Node::SaveToDB(DB_HANDLE hdb)
{
   TCHAR szQuery[4096], szIpAddr[16], baseAddress[16];
   BOOL bNewObject = TRUE;
   BOOL bResult;

   // Lock object's access
   LockData();

   saveCommonProperties(hdb);

   // Form and execute INSERT or UPDATE query
	int snmpMethods = m_snmpSecurity->getAuthMethod() | (m_snmpSecurity->getPrivMethod() << 8);
   if (IsDatabaseRecordExist(hdb, _T("nodes"), _T("id"), m_dwId))
	{
      _sntprintf(szQuery, 4096,
			_T("UPDATE nodes SET primary_ip='%s',primary_name=%s,snmp_port=%d,")
                 _T("node_flags=%d,snmp_version=%d,community=%s,")
                 _T("status_poll_type=%d,agent_port=%d,auth_method=%d,secret=%s,")
                 _T("snmp_oid=%s,uname=%s,agent_version=%s,platform_name=%s,poller_node_id=%d,")
                 _T("zone_guid=%d,proxy_node=%d,snmp_proxy=%d,")
					  _T("required_polls=%d,use_ifxtable=%d,usm_auth_password=%s,")
					  _T("usm_priv_password=%s,usm_methods=%d,snmp_sys_name=%s,bridge_base_addr='%s',")
					  _T("runtime_flags=%d WHERE id=%d"),
                 IpToStr(m_dwIpAddr, szIpAddr), (const TCHAR *)DBPrepareString(hdb, m_primaryName), m_wSNMPPort, 
                 m_dwFlags, m_snmpVersion, (const TCHAR *)DBPrepareStringA(hdb, m_snmpSecurity->getCommunity()),
                 m_iStatusPollType, m_wAgentPort, m_wAuthMethod,
					  (const TCHAR *)DBPrepareString(hdb, m_szSharedSecret), 
                 (const TCHAR *)DBPrepareString(hdb, m_szObjectId),
					  (const TCHAR *)DBPrepareString(hdb, m_sysDescription),
                 (const TCHAR *)DBPrepareString(hdb, m_szAgentVersion),
					  (const TCHAR *)DBPrepareString(hdb, m_szPlatformName), m_dwPollerNode, m_zoneId,
                 m_dwProxyNode, m_dwSNMPProxy, m_iRequiredPollCount,
					  m_nUseIfXTable, (const TCHAR *)DBPrepareStringA(hdb, m_snmpSecurity->getAuthPassword()),
					  (const TCHAR *)DBPrepareStringA(hdb, m_snmpSecurity->getPrivPassword()), snmpMethods,
					  (const TCHAR *)DBPrepareString(hdb, m_sysName), 
					  BinToStr(m_baseBridgeAddress, MAC_ADDR_LENGTH, baseAddress), (int)m_dwDynamicFlags, (int)m_dwId);
	}
   else
	{
      _sntprintf(szQuery, 4096,
                 _T("INSERT INTO nodes (id,primary_ip,primary_name,snmp_port,")
                 _T("node_flags,snmp_version,community,status_poll_type,")
                 _T("agent_port,auth_method,secret,snmp_oid,proxy_node,")
                 _T("agent_version,platform_name,uname,")
                 _T("poller_node_id,zone_guid,snmp_proxy,required_polls,")
		           _T("use_ifxtable,usm_auth_password,usm_priv_password,usm_methods,")
					  _T("snmp_sys_name,bridge_base_addr,runtime_flags) VALUES ")
		           _T("(%d,'%s',%s,%d,%d,%d,%s,%d,%d,%d,%s,%s,%d,%s,%s,%s,%d,%d,%d,%d,%d,%s,%s,%d,%s,'%s',%d)"),
                 (int)m_dwId, IpToStr(m_dwIpAddr, szIpAddr), (const TCHAR *)DBPrepareString(hdb, m_primaryName),
					  (int)m_wSNMPPort, m_dwFlags, m_snmpVersion, (
					  const TCHAR *)DBPrepareStringA(hdb, m_snmpSecurity->getCommunity()),
					  m_iStatusPollType, (int)m_wAgentPort, m_wAuthMethod, 
					  (const TCHAR *)DBPrepareString(hdb, m_szSharedSecret),
					  (const TCHAR *)DBPrepareString(hdb, m_szObjectId),
                 m_dwProxyNode, (const TCHAR *)DBPrepareString(hdb, m_szAgentVersion),
                 (const TCHAR *)DBPrepareString(hdb, m_szPlatformName),
					  (const TCHAR *)DBPrepareString(hdb, m_sysDescription),
		           m_dwPollerNode, m_zoneId, m_dwSNMPProxy, m_iRequiredPollCount, m_nUseIfXTable,
					  (const TCHAR *)DBPrepareStringA(hdb, m_snmpSecurity->getAuthPassword()),
					  (const TCHAR *)DBPrepareStringA(hdb, m_snmpSecurity->getPrivPassword()), snmpMethods,
					  (const TCHAR *)DBPrepareString(hdb, m_sysName),
					  BinToStr(m_baseBridgeAddress, MAC_ADDR_LENGTH, baseAddress), (int)m_dwDynamicFlags);
	}
   bResult = DBQuery(hdb, szQuery);

   // Save access list
   saveACLToDB(hdb);

   UnlockData();

   // Save data collection items
   if (bResult)
   {
		lockDciAccess();
      for(int i = 0; i < m_dcObjects->size(); i++)
         m_dcObjects->get(i)->saveToDB(hdb);
		unlockDciAccess();
   }

	// Clear modifications flag
	LockData();
   m_bIsModified = FALSE;
	UnlockData();

	return bResult;
}


//
// Delete object from database
//

BOOL Node::DeleteFromDB()
{
   TCHAR szQuery[256];
   BOOL bSuccess;

   bSuccess = Template::DeleteFromDB();
   if (bSuccess)
   {
      _sntprintf(szQuery, 256, _T("DELETE FROM nodes WHERE id=%d"), (int)m_dwId);
      QueueSQLRequest(szQuery);
      _sntprintf(szQuery, 256, _T("DELETE FROM nsmap WHERE node_id=%d"), (int)m_dwId);
      QueueSQLRequest(szQuery);
      _sntprintf(szQuery, 256, _T("DROP TABLE idata_%d"), (int)m_dwId);
      QueueSQLRequest(szQuery);
      _sntprintf(szQuery, 256, _T("DROP TABLE tdata_%d"), (int)m_dwId);
      QueueSQLRequest(szQuery);
   }
   return bSuccess;
}


//
// Get ARP cache from node
//

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


//
// Get list of interfaces from node
//

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
		SNMP_Transport *pTransport;
		bool useIfXTable;

		pTransport = createSnmpTransport();
		if (pTransport != NULL)
		{
			if (m_nUseIfXTable == IFXTABLE_DEFAULT)
			{
				useIfXTable = (ConfigReadInt(_T("UseIfXTable"), 1) != 0) ? true : false;
			}
			else
			{
				useIfXTable = (m_nUseIfXTable == IFXTABLE_ENABLED) ? true : false;
			}

			int useAliases = ConfigReadInt(_T("UseInterfaceAliases"), 0);
			pIfList = m_driver->getInterfaces(pTransport, &m_customAttributes, useAliases, useIfXTable);

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


//
// Add VRRP interfaces to interface list
//

void Node::addVrrpInterfaces(InterfaceList *ifList)
{
	int i, j, k;
	TCHAR buffer[32];

	LockData();
	if (m_vrrpInfo != NULL)
	{
		DbgPrintf(6, _T("Node::addVrrpInterfaces(node=%s [%d]): m_vrrpInfo->getSize()=%d"), m_szName, (int)m_dwId, m_vrrpInfo->getSize());

		for(i = 0; i < m_vrrpInfo->getSize(); i++)
		{
			VrrpRouter *router = m_vrrpInfo->getRouter(i);
			DbgPrintf(6, _T("Node::addVrrpInterfaces(node=%s [%d]): vrouter %d state=%d"), m_szName, (int)m_dwId, i, router->getState());
			if (router->getState() != VRRP_STATE_MASTER)
				continue;	// Do not add interfaces if router is not in master state

			// Get netmask for this VR
			DWORD netmask = 0;
			for(j = 0; j < ifList->getSize(); j++)
				if (ifList->get(j)->dwIndex == router->getIfIndex())
				{
					netmask = ifList->get(j)->dwIpNetMask;
					break;
				}

			// Walk through all VR virtual IPs
			for(j = 0; j < router->getVipCount(); j++)
			{
				DWORD vip = router->getVip(j);
				DbgPrintf(6, _T("Node::addVrrpInterfaces(node=%s [%d]): checking VIP %s@%d"), m_szName, (int)m_dwId, IpToStr(vip, buffer), i);
				if (vip != 0)
				{
					for(k = 0; k < ifList->getSize(); k++)
						if (ifList->get(k)->dwIpAddr == vip)
							break;
					if (k == ifList->getSize())
					{
						NX_INTERFACE_INFO iface;
						memset(&iface, 0, sizeof(NX_INTERFACE_INFO));
						_sntprintf(iface.szName, MAX_DB_STRING, _T("vrrp.%u.%u.%d"), router->getId(), router->getIfIndex(), j);
						memcpy(iface.bMacAddr, router->getVirtualMacAddr(), MAC_ADDR_LENGTH);
						iface.dwIpAddr = vip;
						iface.dwIpNetMask = netmask;
						ifList->add(&iface);
						DbgPrintf(6, _T("Node::addVrrpInterfaces(node=%s [%d]): added interface %s"), m_szName, (int)m_dwId, iface.szName);
					}
				}
			}
		}
	}
	UnlockData();
}


//
// Find interface by index and node IP
// Returns pointer to interface object or NULL if appropriate interface couldn't be found
//

Interface *Node::findInterface(DWORD dwIndex, DWORD dwHostAddr)
{
   DWORD i;
   Interface *pInterface;

   LockChildList(FALSE);
   for(i = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i]->Type() == OBJECT_INTERFACE)
      {
         pInterface = (Interface *)m_pChildList[i];
         if (pInterface->getIfIndex() == dwIndex)
         {
            if (((pInterface->IpAddr() & pInterface->getIpNetMask()) ==
                 (dwHostAddr & pInterface->getIpNetMask())) ||
                (dwHostAddr == INADDR_ANY))
            {
               UnlockChildList();
               return pInterface;
            }
         }
      }
   UnlockChildList();
   return NULL;
}


//
// Find interface by name or description
// Returns pointer to interface object or NULL if appropriate interface couldn't be found
//

Interface *Node::findInterface(const TCHAR *name)
{
   DWORD i;
   Interface *pInterface;

   LockChildList(FALSE);
   for(i = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i]->Type() == OBJECT_INTERFACE)
      {
         pInterface = (Interface *)m_pChildList[i];
			if (!_tcsicmp(pInterface->Name(), name) || !_tcsicmp(pInterface->getDescription(), name))
         {
            UnlockChildList();
            return pInterface;
         }
      }
   UnlockChildList();
   return NULL;
}


//
// Find interface by slot/port pair
// Returns pointer to interface object or NULL if appropriate interface couldn't be found
//

Interface *Node::findInterfaceBySlotAndPort(DWORD slot, DWORD port)
{
   DWORD i;
   Interface *pInterface;

   LockChildList(FALSE);
   for(i = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i]->Type() == OBJECT_INTERFACE)
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


//
// Find interface by MAC address
// Returns pointer to interface object or NULL if appropriate interface couldn't be found
//

Interface *Node::findInterfaceByMAC(const BYTE *macAddr)
{
   DWORD i;
   Interface *pInterface;

   LockChildList(FALSE);
   for(i = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i]->Type() == OBJECT_INTERFACE)
      {
         pInterface = (Interface *)m_pChildList[i];
			if (!memcmp(pInterface->getMacAddr(), macAddr, MAC_ADDR_LENGTH))
         {
            UnlockChildList();
            return pInterface;
         }
      }
   UnlockChildList();
   return NULL;
}


//
// Find interface by IP address
// Returns pointer to interface object or NULL if appropriate interface couldn't be found
//

Interface *Node::findInterfaceByIP(DWORD ipAddr)
{
   DWORD i;
   Interface *pInterface;

	if (ipAddr == 0)
		return NULL;

   LockChildList(FALSE);
   for(i = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i]->Type() == OBJECT_INTERFACE)
      {
         pInterface = (Interface *)m_pChildList[i];
			if (pInterface->IpAddr() == ipAddr)
         {
            UnlockChildList();
            return pInterface;
         }
      }
   UnlockChildList();
   return NULL;
}


//
// Find interface by bridge port number
//

Interface *Node::findBridgePort(DWORD bridgePortNumber)
{
   DWORD i;
   Interface *pInterface;

   LockChildList(FALSE);
   for(i = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i]->Type() == OBJECT_INTERFACE)
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


//
// Find connection point for node
//

Interface *Node::findConnectionPoint(DWORD *localIfId, BYTE *localMacAddr)
{
	Interface *cp = NULL;
   LockChildList(FALSE);
   for(DWORD i = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i]->Type() == OBJECT_INTERFACE)
      {
         Interface *iface = (Interface *)m_pChildList[i];
			cp = FindInterfaceConnectionPoint(iface->getMacAddr());
			if (cp != NULL)
			{
				*localIfId = iface->Id();
				memcpy(localMacAddr, iface->getMacAddr(), MAC_ADDR_LENGTH);
				break;
			}
		}
   UnlockChildList();
   return cp;
}


//
// Check if given IP address is one of node's interfaces
//

BOOL Node::isMyIP(DWORD dwIpAddr)
{
   DWORD i;

   LockChildList(FALSE);
   for(i = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i]->Type() == OBJECT_INTERFACE)
      {
         if (((Interface *)m_pChildList[i])->IpAddr() == dwIpAddr)
         {
            UnlockChildList();
            return TRUE;
         }
      }
   UnlockChildList();
   return FALSE;
}


//
// Create new interface
//

Interface *Node::createNewInterface(DWORD dwIpAddr, DWORD dwNetMask, const TCHAR *name, const TCHAR *descr,
                                    DWORD dwIndex, DWORD dwType, BYTE *pbMacAddr, DWORD bridgePort, 
										      DWORD slot, DWORD port, bool physPort, bool manuallyCreated)
{
   Interface *pInterface;
   Subnet *pSubnet = NULL;
	Cluster *pCluster;
	bool bAddToSubnet, bSyntheticMask = false;

	DbgPrintf(5, _T("Node::createNewInterface(%08X, %08X, %s, %d, %d, bp=%d, slot=%d, port=%d) called for node %s [%d]"),
	          dwIpAddr, dwNetMask, CHECK_NULL(name), dwIndex, dwType, bridgePort, slot, port, m_szName, m_dwId);

   // Find subnet to place interface object to
	if ((dwIpAddr != 0) && (dwType != IFTYPE_SOFTWARE_LOOPBACK) && ((dwIpAddr & 0xFF000000) != 0x7F000000))
   {
		pCluster = getMyCluster();
		bAddToSubnet = (pCluster != NULL) ? !pCluster->isSyncAddr(dwIpAddr) : TRUE;
		DbgPrintf(5, _T("Node::createNewInterface: node=%s [%d] cluster=%s [%d] add=%d"),
		          m_szName, m_dwId, (pCluster != NULL) ? pCluster->Name() : _T("(null)"),
					 (pCluster != NULL) ? pCluster->Id() : 0, bAddToSubnet);
		if (bAddToSubnet)
		{
			pSubnet = FindSubnetForNode(m_zoneId, dwIpAddr);
			if (pSubnet == NULL)
			{
				// Check if netmask is 0 (detect), and if yes, create
				// new subnet with class mask
				if (dwNetMask == 0)
				{
					bSyntheticMask = TRUE;
					if (dwIpAddr < 0xE0000000)
					{
						dwNetMask = 0xFFFFFF00;   // Class A, B or C
					}
					else
					{
						TCHAR szBuffer[16];

						// Multicast address??
						DbgPrintf(2, _T("Attempt to create interface object with multicast address %s"), 
									 IpToStr(dwIpAddr, szBuffer));
					}
				}

				// Create new subnet object
				if (dwIpAddr < 0xE0000000)
				{
					pSubnet = new Subnet(dwIpAddr & dwNetMask, dwNetMask, m_zoneId, bSyntheticMask);
					NetObjInsert(pSubnet, TRUE);
					if (IsZoningEnabled())
					{
						Zone *zone = FindZoneByGUID(m_zoneId);
						if (zone != NULL)
						{
							zone->addSubnet(pSubnet);
						}
					}
					else
					{
						g_pEntireNet->AddSubnet(pSubnet);
					}
				}
			}
			else
			{
				// Set correct netmask if we was asked for it
				if (dwNetMask == 0)
				{
					dwNetMask = pSubnet->getIpNetMask();
					bSyntheticMask = pSubnet->isSyntheticMask();
				}
			}
		}
   }

   // Create interface object
   if (name != NULL)
		pInterface = new Interface(name, (descr != NULL) ? descr : name, dwIndex, dwIpAddr, dwNetMask, dwType, m_zoneId);
   else
      pInterface = new Interface(dwIpAddr, dwNetMask, m_zoneId, bSyntheticMask);
   if (pbMacAddr != NULL)
      pInterface->setMacAddr(pbMacAddr);
	pInterface->setBridgePortNumber(bridgePort);
	pInterface->setSlotNumber(slot);
	pInterface->setPortNumber(port);
	pInterface->setPhysicalPortFlag(physPort);
	pInterface->setManualCreationFlag(manuallyCreated);

   // Insert to objects' list and generate event
   NetObjInsert(pInterface, TRUE);
   addInterface(pInterface);
   if (!m_bIsHidden)
      pInterface->unhide();
   PostEvent(EVENT_INTERFACE_ADDED, m_dwId, "dsaad", pInterface->Id(),
             pInterface->Name(), pInterface->IpAddr(),
             pInterface->getIpNetMask(), pInterface->getIfIndex());

   // Bind node to appropriate subnet
   if (pSubnet != NULL)
   {
      pSubnet->AddNode(this);
      
      // Check if subnet mask is correct on interface
      if ((pSubnet->getIpNetMask() != pInterface->getIpNetMask()) && !pSubnet->isSyntheticMask())
		{
         PostEvent(EVENT_INCORRECT_NETMASK, m_dwId, "idsaa", pInterface->Id(),
                   pInterface->getIfIndex(), pInterface->Name(),
                   pInterface->getIpNetMask(), pSubnet->getIpNetMask());
		}
   }

	return pInterface;
}


//
// Delete interface from node
//

void Node::deleteInterface(Interface *pInterface)
{
   DWORD i;

	DbgPrintf(5, _T("Node::deleteInterface(node=%s [%d], interface=%s [%d])"), m_szName, m_dwId, pInterface->Name(), pInterface->Id());

   // Check if we should unlink node from interface's subnet
   if ((pInterface->IpAddr() != 0) && !pInterface->isExcludedFromTopology())
   {
      BOOL bUnlink = TRUE;

      LockChildList(FALSE);
      for(i = 0; i < m_dwChildCount; i++)
         if (m_pChildList[i]->Type() == OBJECT_INTERFACE)
            if (m_pChildList[i] != pInterface)
               if ((((Interface *)m_pChildList[i])->IpAddr() & ((Interface *)m_pChildList[i])->getIpNetMask()) ==
                   (pInterface->IpAddr() & pInterface->getIpNetMask()))
               {
                  bUnlink = FALSE;
                  break;
               }
      UnlockChildList();
      
      if (bUnlink)
      {
         // Last interface in subnet, should unlink node
         Subnet *pSubnet = FindSubnetByIP(m_zoneId, pInterface->IpAddr() & pInterface->getIpNetMask());
         if (pSubnet != NULL)
         {
            DeleteParent(pSubnet);
            pSubnet->DeleteChild(this);
         }
			DbgPrintf(5, _T("Node::deleteInterface(node=%s [%d], interface=%s [%d]): unlinked from subnet %s [%d]"),
			          m_szName, m_dwId, pInterface->Name(), pInterface->Id(),
						 (pSubnet != NULL) ? pSubnet->Name() : _T("(null)"), 
						 (pSubnet != NULL) ? pSubnet->Id() : 0);
      }
   }
	pInterface->deleteObject();
}


//
// Calculate node status based on child objects status
//

void Node::calculateCompoundStatus(BOOL bForcedRecalc)
{
   int iOldStatus = m_iStatus;
   static DWORD dwEventCodes[] = { EVENT_NODE_NORMAL, EVENT_NODE_MINOR,
      EVENT_NODE_WARNING, EVENT_NODE_MAJOR, EVENT_NODE_CRITICAL,
      EVENT_NODE_UNKNOWN, EVENT_NODE_UNMANAGED };

   NetObj::calculateCompoundStatus(bForcedRecalc);
   if (m_iStatus != iOldStatus)
      PostEvent(dwEventCodes[m_iStatus], m_dwId, "d", iOldStatus);
}


//
// Perform status poll on node
//

void Node::statusPoll(ClientSession *pSession, DWORD dwRqId, int nPoller)
{
   DWORD i, dwPollListSize, dwOldFlags = m_dwFlags;
   NetObj *pPollerNode = NULL, **ppPollList;
   BOOL bAllDown;
   Queue *pQueue;    // Delayed event queue
	SNMP_Transport *pTransport;
	Cluster *pCluster;
   time_t tNow, tExpire;

   pQueue = new Queue;
   SetPollerInfo(nPoller, _T("wait for lock"));
   pollerLock();
   m_pPollRequestor = pSession;
   SendPollerMsg(dwRqId, _T("Starting status poll for node %s\r\n"), m_szName);
   DbgPrintf(5, _T("Starting status poll for node %s (ID: %d)"), m_szName, m_dwId);

   // Read capability expiration time and current time
   tExpire = (time_t)ConfigReadULong(_T("CapabilityExpirationTime"), 604800);
   tNow = time(NULL);

   // Check SNMP agent connectivity
restart_agent_check:
   if ((m_dwFlags & NF_IS_SNMP) && (!(m_dwFlags & NF_DISABLE_SNMP)) && (m_dwIpAddr != 0))
   {
      TCHAR szBuffer[256];
      DWORD dwResult;

      DbgPrintf(6, _T("StatusPoll(%s): check SNMP"), m_szName);
		pTransport = createSnmpTransport();
		if (pTransport == NULL)
		{
	      DbgPrintf(6, _T("StatusPoll(%s): cannot create SNMP transport"), m_szName);
			goto skip_snmp_check;
		}

      SetPollerInfo(nPoller, _T("check SNMP"));
      SendPollerMsg(dwRqId, _T("Checking SNMP agent connectivity\r\n"));
      dwResult = SnmpGet(m_snmpVersion, pTransport, _T(".1.3.6.1.2.1.1.2.0"), NULL, 0, szBuffer, 256, 0);
      if ((dwResult == SNMP_ERR_SUCCESS) || (dwResult == SNMP_ERR_NO_OBJECT))
      {
         if (m_dwDynamicFlags & NDF_SNMP_UNREACHABLE)
         {
            m_dwDynamicFlags &= ~NDF_SNMP_UNREACHABLE;
            PostEventEx(pQueue, EVENT_SNMP_OK, m_dwId, NULL);
            SendPollerMsg(dwRqId, POLLER_INFO _T("Connectivity with SNMP agent restored\r\n"));
         }
      }
      else
      {
         SendPollerMsg(dwRqId, POLLER_ERROR _T("SNMP agent unreachable\r\n"));
         if (m_dwDynamicFlags & NDF_SNMP_UNREACHABLE)
         {
            if ((tNow > m_tFailTimeSNMP + tExpire) &&
                (!(m_dwDynamicFlags & NDF_UNREACHABLE)))
            {
               m_dwFlags &= ~NF_IS_SNMP;
               m_dwDynamicFlags &= ~NDF_SNMP_UNREACHABLE;
               m_szObjectId[0] = 0;
               SendPollerMsg(dwRqId, POLLER_WARNING _T("Attribute isSNMP set to FALSE\r\n"));
            }
         }
         else
         {
            m_dwDynamicFlags |= NDF_SNMP_UNREACHABLE;
            PostEventEx(pQueue, EVENT_SNMP_FAIL, m_dwId, NULL);
            m_tFailTimeSNMP = tNow;
         }
      }
		delete pTransport;
      DbgPrintf(6, _T("StatusPoll(%s): SNMP check finished"), m_szName);
   }

skip_snmp_check:
   // Check native agent connectivity
   if ((m_dwFlags & NF_IS_NATIVE_AGENT) && (!(m_dwFlags & NF_DISABLE_NXCP)) && (m_dwIpAddr != 0))
   {
      DbgPrintf(6, _T("StatusPoll(%s): checking agent"), m_szName);
      SetPollerInfo(nPoller, _T("check agent"));
      SendPollerMsg(dwRqId, _T("Checking NetXMS agent connectivity\r\n"));

		DWORD error, socketError;
		agentLock();
      if (connectToAgent(&error, &socketError))
      {
         DbgPrintf(7, _T("StatusPoll(%s): connected to agent"), m_szName);
         if (m_dwDynamicFlags & NDF_AGENT_UNREACHABLE)
         {
            m_dwDynamicFlags &= ~NDF_AGENT_UNREACHABLE;
            PostEventEx(pQueue, EVENT_AGENT_OK, m_dwId, NULL);
            SendPollerMsg(dwRqId, POLLER_INFO _T("Connectivity with NetXMS agent restored\r\n"));
         }
      }
      else
      {
         DbgPrintf(6, _T("StatusPoll(%s): agent unreachable, error=%d, socketError=%d"), m_szName, (int)error, (int)socketError);
         SendPollerMsg(dwRqId, POLLER_ERROR _T("NetXMS agent unreachable\r\n"));
         if (m_dwDynamicFlags & NDF_AGENT_UNREACHABLE)
         {
            if ((tNow > m_tFailTimeAgent + tExpire) &&
                (!(m_dwDynamicFlags & NDF_UNREACHABLE)))
            {
               m_dwFlags &= ~NF_IS_NATIVE_AGENT;
               m_dwDynamicFlags &= ~NDF_AGENT_UNREACHABLE;
               m_szPlatformName[0] = 0;
               m_szAgentVersion[0] = 0;
               SendPollerMsg(dwRqId, POLLER_WARNING _T("Attribute isNetXMSAgent set to FALSE\r\n"));
            }
         }
         else
         {
            m_dwDynamicFlags |= NDF_AGENT_UNREACHABLE;
            PostEventEx(pQueue, EVENT_AGENT_FAIL, m_dwId, NULL);
            m_tFailTimeAgent = tNow;
         }
      }
		agentUnlock();
      DbgPrintf(7, _T("StatusPoll(%s): agent check finished"), m_szName);
   }

   SetPollerInfo(nPoller, _T("prepare polling list"));

   // Find service poller node object
   LockData();
   if (m_dwPollerNode != 0)
   {
		DWORD id = m_dwPollerNode;
		UnlockData();
      pPollerNode = FindObjectById(id);
      if (pPollerNode != NULL)
      {
         if (pPollerNode->Type() != OBJECT_NODE)
            pPollerNode = NULL;
      }
   }
	else
	{
		UnlockData();
	}

   // If nothing found, use management server
   if (pPollerNode == NULL)
   {
      pPollerNode = FindObjectById(g_dwMgmtNode);
      if (pPollerNode != NULL)
         pPollerNode->IncRefCount();
   }
   else
   {
      pPollerNode->IncRefCount();
   }

   // Create polling list
   ppPollList = (NetObj **)malloc(sizeof(NetObj *) * m_dwChildCount);
   LockChildList(FALSE);
   for(i = 0, dwPollListSize = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i]->Status() != STATUS_UNMANAGED)
      {
         m_pChildList[i]->IncRefCount();
         ppPollList[dwPollListSize++] = m_pChildList[i];
      }
   UnlockChildList();

   // Poll interfaces and services
   SetPollerInfo(nPoller, _T("child poll"));
   DbgPrintf(7, _T("StatusPoll(%s): starting child object poll"), m_szName);
	pCluster = getMyCluster();
	pTransport = createSnmpTransport();
   for(i = 0; i < dwPollListSize; i++)
   {
      switch(ppPollList[i]->Type())
      {
         case OBJECT_INTERFACE:
			   DbgPrintf(7, _T("StatusPoll(%s): polling interface %d [%s]"), m_szName, ppPollList[i]->Id(), ppPollList[i]->Name());
            ((Interface *)ppPollList[i])->StatusPoll(pSession, dwRqId, pQueue,
					(pCluster != NULL) ? pCluster->isSyncAddr(((Interface *)ppPollList[i])->IpAddr()) : FALSE,
					pTransport);
            break;
         case OBJECT_NETWORKSERVICE:
			   DbgPrintf(7, _T("StatusPoll(%s): polling network service %d [%s]"), m_szName, ppPollList[i]->Id(), ppPollList[i]->Name());
            ((NetworkService *)ppPollList[i])->StatusPoll(pSession, dwRqId,
                                                          (Node *)pPollerNode, pQueue);
            break;
         default:
            break;
      }
      ppPollList[i]->DecRefCount();
   }
	delete pTransport;
   safe_free(ppPollList);
   DbgPrintf(7, _T("StatusPoll(%s): finished child object poll"), m_szName);

   // Check if entire node is down
	// This check is disabled for nodes without IP address
	if (m_dwIpAddr != 0)
	{
		LockChildList(FALSE);
		if (m_dwChildCount > 0)
		{
			for(i = 0, bAllDown = TRUE; i < m_dwChildCount; i++)
				if ((m_pChildList[i]->Type() == OBJECT_INTERFACE) &&
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
		if (bAllDown)
		{
		   if (!(m_dwDynamicFlags & NDF_UNREACHABLE))
		   {
		      m_dwDynamicFlags |= NDF_UNREACHABLE;
		      PostEvent(EVENT_NODE_DOWN, m_dwId, NULL);
		      SendPollerMsg(dwRqId, POLLER_ERROR _T("Node is unreachable\r\n"));
		   }
		   else
		   {
		      SendPollerMsg(dwRqId, POLLER_WARNING _T("Node is still unreachable\r\n"));
		   }
		}
		else
		{
		   if (m_dwDynamicFlags & NDF_UNREACHABLE)
		   {
		      m_dwDynamicFlags &= ~(NDF_UNREACHABLE | NDF_SNMP_UNREACHABLE | NDF_AGENT_UNREACHABLE);
		      PostEvent(EVENT_NODE_UP, m_dwId, NULL);
		      SendPollerMsg(dwRqId, POLLER_INFO _T("Node recovered from unreachable state\r\n"));
				goto restart_agent_check;
		   }
		   else
		   {
		      SendPollerMsg(dwRqId, POLLER_INFO _T("Node is connected\r\n"));
		   }
		}
	}

   // Send delayed events and destroy delayed event queue
   ResendEvents(pQueue);
   delete pQueue;
   
   SetPollerInfo(nPoller, _T("cleanup"));
   if (pPollerNode != NULL)
      pPollerNode->DecRefCount();

   if (dwOldFlags != m_dwFlags)
   {
      PostEvent(EVENT_NODE_FLAGS_CHANGED, m_dwId, "xx", dwOldFlags, m_dwFlags);
      LockData();
      Modify();
      UnlockData();
   }

   calculateCompoundStatus();
   m_tLastStatusPoll = time(NULL);
   SendPollerMsg(dwRqId, _T("Finished status poll for node %s\r\n"), m_szName);
   SendPollerMsg(dwRqId, _T("Node status after poll is %s\r\n"), g_szStatusText[m_iStatus]);
   m_pPollRequestor = NULL;
   if (dwRqId == 0)
      m_dwDynamicFlags &= ~NDF_QUEUED_FOR_STATUS_POLL;
   pollerUnlock();
   DbgPrintf(5, _T("Finished status poll for node %s (ID: %d)"), m_szName, m_dwId);
}


//
// Check agent policy binding
// Intended to be called only from configuration poller
//

void Node::checkAgentPolicyBinding(AgentConnection *conn)
{
	AgentPolicyInfo *ap;
	DWORD rcc = conn->getPolicyInventory(&ap);
	if (rcc == ERR_SUCCESS)
	{
		// Check for unbound but installed policies
		for(int i = 0; i < ap->getSize(); i++)
		{
			uuid_t guid;
			ap->getGuid(i, guid);
			NetObj *object = FindObjectByGUID(guid, -1);
			if ((object != NULL) && (!object->IsChild(m_dwId)))
			{
				object->AddChild(this);
				AddParent(object);
				DbgPrintf(5, _T("ConfPoll(%s): bound to policy object %s [%d]"), m_szName, object->Name(), object->Id());
			}
		}

		// Check for bound but not installed policies
		LockParentList(FALSE);
		NetObj **unbindList = (NetObj **)malloc(sizeof(NetObj *) * m_dwParentCount);
		int unbindListSize = 0;
		for(DWORD i = 0; i < m_dwParentCount; i++)
		{
			if (IsAgentPolicyObject(m_pParentList[i]))
			{
				uuid_t guid1, guid2;
				int j;

				m_pParentList[i]->getGuid(guid1);
				for(j = 0; j < ap->getSize(); j++)
				{
					ap->getGuid(j, guid2);
					if (!uuid_compare(guid1, guid2))
						break;
				}
				if (j == ap->getSize())
					unbindList[unbindListSize++] = m_pParentList[i];
			}
		}
		UnlockParentList();

		for(int i = 0; i < unbindListSize; i++)
		{
			unbindList[i]->DeleteChild(this);
			DeleteParent(unbindList[i]);
			DbgPrintf(5, _T("ConfPoll(%s): unbound from policy object %s [%d]"), m_szName, unbindList[i]->Name(), unbindList[i]->Id());
		}
		safe_free(unbindList);

		delete ap;
	}
	else
	{
	   DbgPrintf(5, _T("ConfPoll(%s): AgentConnection::getPolicyInventory() failed: rcc=%d"), m_szName, rcc);
	}
}


//
// Update primary IP address from primary name
//

void Node::updatePrimaryIpAddr()
{
	if (m_primaryName[0] == 0)
		return;

	DWORD ipAddr = ntohl(ResolveHostName(m_primaryName));
	if ((ipAddr != m_dwIpAddr) && (ipAddr != INADDR_ANY) && (ipAddr != INADDR_NONE))
	{
		TCHAR buffer1[32], buffer2[32];

		DbgPrintf(4, _T("IP address for node %s [%d] changed from %s to %s"), 
			m_szName, (int)m_dwId, IpToStr(m_dwIpAddr, buffer1), IpToStr(ipAddr, buffer2));
		PostEvent(EVENT_IP_ADDRESS_CHANGED, m_dwId, "aa", ipAddr, m_dwIpAddr);
		m_dwIpAddr = ipAddr;
	}
}


//
// Walker callback for print MIB
//

static DWORD PrintMIBWalkerCallback(DWORD version, SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
	(*((int *)arg))++;
	return SNMP_ERR_SUCCESS;
}


//
// Perform configuration poll on node
//

void Node::configurationPoll(ClientSession *pSession, DWORD dwRqId,
                             int nPoller, DWORD dwNetMask)
{
   DWORD dwOldFlags = m_dwFlags, dwAddr, rcc;
   AgentConnection *pAgentConn;
   TCHAR szBuffer[4096];
	SNMP_Transport *pTransport;
   BOOL bHasChanges = FALSE;

   SetPollerInfo(nPoller, _T("wait for lock"));
   pollerLock();
   m_pPollRequestor = pSession;
   SendPollerMsg(dwRqId, _T("Starting configuration poll for node %s\r\n"), m_szName);
   DbgPrintf(4, _T("Starting configuration poll for node %s (ID: %d)"), m_szName, m_dwId);

   // Check for forced capabilities recheck
   if (m_dwDynamicFlags & NDF_RECHECK_CAPABILITIES)
   {
      m_dwFlags &= ~(NF_IS_NATIVE_AGENT | NF_IS_SNMP | NF_IS_CPSNMP |
                     NF_IS_BRIDGE | NF_IS_ROUTER | NF_IS_OSPF | NF_IS_PRINTER |
							NF_IS_CDP | NF_IS_LLDP | NF_IS_SONMP);
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
      SendPollerMsg(dwRqId, POLLER_WARNING _T("Node is marked as unreachable, configuration poll aborted\r\n"));
      DbgPrintf(4, _T("Node is marked as unreachable, configuration poll aborted"));
      m_tLastConfigurationPoll = time(NULL);
   }
   else
   {
		updatePrimaryIpAddr();

      // Check node's capabilities
      SetPollerInfo(nPoller, _T("capability check"));
      SendPollerMsg(dwRqId, _T("Checking node's capabilities...\r\n"));
      
		// ***** NetXMS agent check *****
      DbgPrintf(5, _T("ConfPoll(%s): checking for NetXMS agent Flags={%08X} DynamicFlags={%08X}"), m_szName, m_dwFlags, m_dwDynamicFlags);
      if ((!((m_dwFlags & NF_IS_NATIVE_AGENT) && (m_dwDynamicFlags & NDF_AGENT_UNREACHABLE))) &&
          (!(m_dwFlags & NF_DISABLE_NXCP)) && (m_dwIpAddr != 0))
      {
	      SendPollerMsg(dwRqId, _T("   Checking NetXMS agent...\r\n"));
         pAgentConn = new AgentConnection(htonl(m_dwIpAddr), m_wAgentPort, m_wAuthMethod, m_szSharedSecret);
         setAgentProxy(pAgentConn);
         DbgPrintf(5, _T("ConfPoll(%s): checking for NetXMS agent - connecting"), m_szName);
         if (pAgentConn->connect(g_pServerKey))
         {
            DbgPrintf(5, _T("ConfPoll(%s): checking for NetXMS agent - connected"), m_szName);
            LockData();
            m_dwFlags |= NF_IS_NATIVE_AGENT;
            if (m_dwDynamicFlags & NDF_AGENT_UNREACHABLE)
            {
               m_dwDynamicFlags &= ~NDF_AGENT_UNREACHABLE;
               PostEvent(EVENT_AGENT_OK, m_dwId, NULL);
               SendPollerMsg(dwRqId, POLLER_INFO _T("   Connectivity with NetXMS agent restored\r\n"));
            }
				else
				{
	            SendPollerMsg(dwRqId, POLLER_INFO _T("   NetXMS native agent is active\r\n"));
				}
            UnlockData();
      
            if (pAgentConn->getParameter(_T("Agent.Version"), MAX_AGENT_VERSION_LEN, szBuffer) == ERR_SUCCESS)
            {
               LockData();
               if (_tcscmp(m_szAgentVersion, szBuffer))
               {
                  _tcscpy(m_szAgentVersion, szBuffer);
                  bHasChanges = TRUE;
                  SendPollerMsg(dwRqId, _T("   NetXMS agent version changed to %s\r\n"), m_szAgentVersion);
               }
               UnlockData();
            }

            if (pAgentConn->getParameter(_T("System.PlatformName"), MAX_PLATFORM_NAME_LEN, szBuffer) == ERR_SUCCESS)
            {
               LockData();
               if (_tcscmp(m_szPlatformName, szBuffer))
               {
                  _tcscpy(m_szPlatformName, szBuffer);
                  bHasChanges = TRUE;
                  SendPollerMsg(dwRqId, _T("   Platform name changed to %s\r\n"), m_szPlatformName);
               }
               UnlockData();
            }

            // Check IP forwarding status
            if (pAgentConn->getParameter(_T("Net.IP.Forwarding"), 16, szBuffer) == ERR_SUCCESS)
            {
               if (_tcstoul(szBuffer, NULL, 10) != 0)
                  m_dwFlags |= NF_IS_ROUTER;
               else
                  m_dwFlags &= ~NF_IS_ROUTER;
            }

				// Get uname
				if (pAgentConn->getParameter(_T("System.Uname"), MAX_DB_STRING, szBuffer) == ERR_SUCCESS)
				{
					TranslateStr(szBuffer, _T("\r\n"), _T(" "));
					TranslateStr(szBuffer, _T("\n"), _T(" "));
					TranslateStr(szBuffer, _T("\r"), _T(" "));
               LockData();
               if ((m_sysDescription == NULL) || _tcscmp(m_sysDescription, szBuffer))
               {
						safe_free(m_sysDescription);
                  m_sysDescription = _tcsdup(szBuffer);
                  bHasChanges = TRUE;
                  SendPollerMsg(dwRqId, _T("   System description changed to %s\r\n"), m_sysDescription);
               }
               UnlockData();
				}

				StructArray<NXC_AGENT_PARAM> *plist;
				StructArray<NXC_AGENT_TABLE> *tlist;
            rcc = pAgentConn->getSupportedParameters(&plist, &tlist);
				if (rcc == ERR_SUCCESS)
				{
					LockData();
					delete m_paramList;
					delete m_tableList;
					m_paramList = plist;
					m_tableList = tlist;
					UnlockData();
				}
				else
				{
				   DbgPrintf(5, _T("ConfPoll(%s): AgentConnection::getSupportedParameters() failed: rcc=%d"), m_szName, rcc);
				}

				checkAgentPolicyBinding(pAgentConn);

            pAgentConn->disconnect();
         }
			else
			{
	         DbgPrintf(5, _T("ConfPoll(%s): checking for NetXMS agent - failed to connect"), m_szName);
			}
         delete pAgentConn;
         DbgPrintf(5, _T("ConfPoll(%s): checking for NetXMS agent - finished"), m_szName);
      }

		// ***** SNMP check *****
		if ((!((m_dwFlags & NF_IS_SNMP) && (m_dwDynamicFlags & NDF_SNMP_UNREACHABLE))) &&
          (!(m_dwFlags & NF_DISABLE_SNMP)) && (m_dwIpAddr != 0))
      {
	      SendPollerMsg(dwRqId, _T("   Checking SNMP...\r\n"));
         DbgPrintf(5, _T("ConfPoll(%s): calling SnmpCheckCommSettings()"), m_szName);
			pTransport = createSnmpTransport();
			if (pTransport == NULL)
			{
				DbgPrintf(5, _T("ConfPoll(%s): unable to create SNMP transport"), m_szName);
				goto skip_snmp_checks;
			}

			SNMP_SecurityContext *newCtx = SnmpCheckCommSettings(pTransport, &m_snmpVersion, m_snmpSecurity);
			if (newCtx != NULL)
         {
            LockData();
				delete m_snmpSecurity;
				m_snmpSecurity = newCtx;
            m_dwFlags |= NF_IS_SNMP;
            if (m_dwDynamicFlags & NDF_SNMP_UNREACHABLE)
            {
               m_dwDynamicFlags &= ~NDF_SNMP_UNREACHABLE;
               PostEvent(EVENT_SNMP_OK, m_dwId, NULL);
               SendPollerMsg(dwRqId, POLLER_INFO _T("   Connectivity with SNMP agent restored\r\n"));
            }
				UnlockData();
            SendPollerMsg(dwRqId, _T("   SNMP agent is active (version %s)\r\n"), 
					(m_snmpVersion == SNMP_VERSION_3) ? _T("3") : ((m_snmpVersion == SNMP_VERSION_2C) ? _T("2c") : _T("1")));

				if (SnmpGet(m_snmpVersion, pTransport,
								_T(".1.3.6.1.2.1.1.2.0"), NULL, 0, szBuffer, 4096, SG_STRING_RESULT) == SNMP_ERR_SUCCESS)
				{
					LockData();
					if (_tcscmp(m_szObjectId, szBuffer))
					{
						nx_strncpy(m_szObjectId, szBuffer, MAX_OID_LEN * 4);
						bHasChanges = TRUE;
					}
					UnlockData();
				}

				// Get system description
				if (SnmpGet(m_snmpVersion, pTransport,
				            _T(".1.3.6.1.2.1.1.1.0"), NULL, 0, szBuffer, MAX_DB_STRING, SG_STRING_RESULT) == SNMP_ERR_SUCCESS)
				{
					TranslateStr(szBuffer, _T("\r\n"), _T(" "));
					TranslateStr(szBuffer, _T("\n"), _T(" "));
					TranslateStr(szBuffer, _T("\r"), _T(" "));
					LockData();
               if ((m_sysDescription == NULL) || _tcscmp(m_sysDescription, szBuffer))
               {
						safe_free(m_sysDescription);
                  m_sysDescription = _tcsdup(szBuffer);
                  bHasChanges = TRUE;
                  SendPollerMsg(dwRqId, _T("   System description changed to %s\r\n"), m_sysDescription);
               }
					UnlockData();
				}

				// Select device driver
				NetworkDeviceDriver *driver = FindDriverForNode(this, pTransport);
				DbgPrintf(5, _T("ConfPoll(%s): selected device driver %s"), m_szName, driver->getName());
				LockData();
				if (driver != m_driver)
				{
					m_driver = driver;
					SendPollerMsg(dwRqId, _T("   New network device driver selected: %s\r\n"), m_driver->getName());
				}
				UnlockData();

				// Allow driver to gather additional info
				m_driver->analyzeDevice(pTransport, m_szObjectId, &m_customAttributes);

				// Get sysName
				if (SnmpGet(m_snmpVersion, pTransport,
				            _T(".1.3.6.1.2.1.1.5.0"), NULL, 0, szBuffer, MAX_DB_STRING, SG_STRING_RESULT) == SNMP_ERR_SUCCESS)
				{
					LockData();
					if ((m_sysName == NULL) || _tcscmp(m_sysName, szBuffer))
               {
						safe_free(m_sysName);
                  m_sysName = _tcsdup(szBuffer);
                  bHasChanges = TRUE;
                  SendPollerMsg(dwRqId, _T("   System name changed to %s\r\n"), m_sysName);
               }
					UnlockData();
				}

            // Check IP forwarding
            if (CheckSNMPIntegerValue(pTransport, _T(".1.3.6.1.2.1.4.1.0"), 1))
            {
					LockData();
               m_dwFlags |= NF_IS_ROUTER;
					UnlockData();
            }
            else
            {
					LockData();
               m_dwFlags &= ~NF_IS_ROUTER;
					UnlockData();
            }

            // Check for bridge MIB support
            if (SnmpGet(m_snmpVersion, pTransport, _T(".1.3.6.1.2.1.17.1.1.0"), NULL, 0, szBuffer, 4096, SG_RAW_RESULT) == SNMP_ERR_SUCCESS)
            {
					LockData();
               m_dwFlags |= NF_IS_BRIDGE;
					memcpy(m_baseBridgeAddress, szBuffer, 6);
					UnlockData();

					// Check for Spanning Tree (IEEE 802.1d) MIB support
					if (CheckSNMPIntegerValue(pTransport, _T(".1.3.6.1.2.1.17.2.1.0"), 3))
					{
						LockData();
						m_dwFlags |= NF_IS_STP;
						UnlockData();
					}
					else
					{
						LockData();
						m_dwFlags &= ~NF_IS_STP;
						UnlockData();
					}
            }
            else
            {
					LockData();
               m_dwFlags &= ~(NF_IS_BRIDGE | NF_IS_STP);
					UnlockData();
            }

            // Check for ENTITY-MIB support
            if (SnmpGet(m_snmpVersion, pTransport, _T(".1.3.6.1.2.1.47.1.4.1.0"), NULL, 0, szBuffer, 4096, SG_RAW_RESULT) == SNMP_ERR_SUCCESS)
            {
					LockData();
               m_dwFlags |= NF_HAS_ENTITY_MIB;
					UnlockData();

					ComponentTree *components = BuildComponentTree(this, pTransport);
					LockData();
					if (m_components != NULL)
						m_components->decRefCount();
					m_components = components;
					UnlockData();
            }
            else
            {
					LockData();
               m_dwFlags &= ~NF_HAS_ENTITY_MIB;
					if (m_components != NULL)
					{
						m_components->decRefCount();
						m_components = NULL;
					}
					UnlockData();
            }

            // Check for printer MIB support
				int count = 0;
				SnmpEnumerate(m_snmpVersion, pTransport, _T(".1.3.6.1.2.1.43.5.1.1.17"), PrintMIBWalkerCallback, &count, FALSE);
            if (count > 0)
            {
					LockData();
               m_dwFlags |= NF_IS_PRINTER;
					UnlockData();
            }
            else
            {
					LockData();
               m_dwFlags &= ~NF_IS_PRINTER;
					UnlockData();
            }

            // Check for CDP (Cisco Discovery Protocol) support
            if (CheckSNMPIntegerValue(pTransport, _T(".1.3.6.1.4.1.9.9.23.1.3.1.0"), 1))
            {
					LockData();
               m_dwFlags |= NF_IS_CDP;
					UnlockData();
            }
            else
            {
					LockData();
               m_dwFlags &= ~NF_IS_CDP;
					UnlockData();
            }

            // Check for NDP (Nortel Discovery Protocol) support
            if (CheckSNMPIntegerValue(pTransport, _T(".1.3.6.1.4.1.45.1.6.13.1.2.0"), 1))
            {
					LockData();
               m_dwFlags |= NF_IS_NDP;
					UnlockData();
            }
            else
            {
					LockData();
               m_dwFlags &= ~NF_IS_NDP;
					UnlockData();
            }

		      // Check for LLDP (Link Layer Discovery Protocol) support
				if (SnmpGet(m_snmpVersion, pTransport, _T(".1.0.8802.1.1.2.1.3.2.0"), NULL, 0, szBuffer, 4096, 0) == SNMP_ERR_SUCCESS)
				{
					LockData();
					m_dwFlags |= NF_IS_LLDP;
					UnlockData();

					if (SnmpGet(m_snmpVersion, pTransport, _T(".1.0.8802.1.1.2.1.3.1.0"), NULL, 0, szBuffer, 4096, SG_STRING_RESULT) == SNMP_ERR_SUCCESS)
					{
						_tcscat(szBuffer, _T("@"));
						int len = (int)_tcslen(szBuffer);
						if (SnmpGet(m_snmpVersion, pTransport, _T(".1.0.8802.1.1.2.1.3.2.0"), NULL, 0, &szBuffer[len], 4096 - len, SG_HSTRING_RESULT) == SNMP_ERR_SUCCESS)
						{
							LockData();
							if ((m_lldpNodeId == NULL) || _tcscmp(m_lldpNodeId, szBuffer))
							{
								safe_free(m_lldpNodeId);
								m_lldpNodeId = _tcsdup(szBuffer);
								bHasChanges = TRUE;
								SendPollerMsg(dwRqId, _T("   LLDP node ID changed to %s\r\n"), m_lldpNodeId);
							}
							UnlockData();
						}
					}
				}
				else
				{
					LockData();
					m_dwFlags &= ~NF_IS_LLDP;
					UnlockData();
				}

            // Check for 802.1x support
            if (CheckSNMPIntegerValue(pTransport, _T(".1.0.8802.1.1.1.1.1.1.0"), 1))
            {
					LockData();
               m_dwFlags |= NF_IS_8021X;
					UnlockData();
            }
            else
            {
					LockData();
					m_dwFlags &= ~NF_IS_8021X;
					UnlockData();
            }

            CheckOSPFSupport(pTransport);

				// Get VRRP information
				VrrpInfo *vrrpInfo = GetVRRPInfo(this);
				if (vrrpInfo != NULL)
				{
					LockData();
					m_dwFlags |= NF_IS_VRRP;
					delete m_vrrpInfo;
					m_vrrpInfo = vrrpInfo;
					UnlockData();
				}
				else
				{
					LockData();
					m_dwFlags &= ~NF_IS_VRRP;
					UnlockData();
				}
         }
         else
         {
            // Check for CheckPoint SNMP agent on port 161
            DbgPrintf(5, _T("ConfPoll(%s): checking for CheckPoint SNMP"), m_szName);
				if (SnmpGet(SNMP_VERSION_1, pTransport, _T(".1.3.6.1.4.1.2620.1.1.10.0"), NULL, 0, szBuffer, 4096, 0) == SNMP_ERR_SUCCESS)
            {
               LockData();
               if (_tcscmp(m_szObjectId, _T(".1.3.6.1.4.1.2620.1.1")))
               {
                  nx_strncpy(m_szObjectId, _T(".1.3.6.1.4.1.2620.1.1"), MAX_OID_LEN * 4);
                  bHasChanges = TRUE;
               }

               m_dwFlags |= NF_IS_SNMP | NF_IS_ROUTER;
               m_dwDynamicFlags &= ~NDF_SNMP_UNREACHABLE;
               UnlockData();
               SendPollerMsg(dwRqId, POLLER_INFO _T("   CheckPoint SNMP agent on port 161 is active\r\n"));
            }
         }
			delete pTransport;
      }

skip_snmp_checks:
      // Check for CheckPoint SNMP agent on port 260
      DbgPrintf(5, _T("ConfPoll(%s): checking for CheckPoint SNMP on port 260"), m_szName);
      if (!((m_dwFlags & NF_IS_CPSNMP) && (m_dwDynamicFlags & NDF_CPSNMP_UNREACHABLE)) && (m_dwIpAddr != 0))
      {
			pTransport = new SNMP_UDPTransport;
			((SNMP_UDPTransport *)pTransport)->createUDPTransport(NULL, htonl(m_dwIpAddr), CHECKPOINT_SNMP_PORT);
         if (SnmpGet(SNMP_VERSION_1, pTransport,
                     _T(".1.3.6.1.4.1.2620.1.1.10.0"), NULL, 0, szBuffer, 4096, 0) == SNMP_ERR_SUCCESS)
         {
            LockData();
            m_dwFlags |= NF_IS_CPSNMP | NF_IS_ROUTER;
            m_dwDynamicFlags &= ~NDF_CPSNMP_UNREACHABLE;
            UnlockData();
            SendPollerMsg(dwRqId, POLLER_INFO _T("   CheckPoint SNMP agent on port 260 is active\r\n"));
         }
			delete pTransport;
      }

      // Generate event if node flags has been changed
      if (dwOldFlags != m_dwFlags)
      {
         PostEvent(EVENT_NODE_FLAGS_CHANGED, m_dwId, "xx", dwOldFlags, m_dwFlags);
         bHasChanges = TRUE;
      }

      // Retrieve interface list
      SetPollerInfo(nPoller, _T("interface check"));
      SendPollerMsg(dwRqId, _T("Capability check finished\r\n"));

		if (updateInterfaceConfiguration(dwRqId, dwNetMask))
			bHasChanges = TRUE;

      m_tLastConfigurationPoll = time(NULL);

		// Check node name
		SendPollerMsg(dwRqId, _T("Checking node name\r\n"));
		dwAddr = ntohl(_t_inet_addr(m_szName));
		if ((g_dwFlags & AF_RESOLVE_NODE_NAMES) &&
			 (dwAddr != INADDR_NONE) && 
			 (dwAddr != INADDR_ANY) &&
			 isMyIP(dwAddr))
		{
			SendPollerMsg(dwRqId, _T("Node name is an IP address and need to be resolved\r\n"));
	      SetPollerInfo(nPoller, _T("resolving name"));
			if (resolveName(FALSE))
			{
				SendPollerMsg(dwRqId, POLLER_INFO _T("Node name resolved to %s\r\n"), m_szName);
				bHasChanges = TRUE;
			}
			else
			{
				SendPollerMsg(dwRqId, POLLER_WARNING _T("Node name cannot be resolved\r\n"));
			}
		}
		else
		{
			if (g_dwFlags & AF_SYNC_NODE_NAMES_WITH_DNS)
			{
				SendPollerMsg(dwRqId, _T("Syncing node name with DNS\r\n"));
		      SetPollerInfo(nPoller, _T("resolving name"));
				if (resolveName(TRUE))
				{
					SendPollerMsg(dwRqId, POLLER_INFO _T("Node name resolved to %s\r\n"), m_szName);
					bHasChanges = TRUE;
				}
			}
			else
			{
				SendPollerMsg(dwRqId, _T("Node name is OK\r\n"));
			}
		}

		// Apply templates
		ApplySystemTemplates();
		ApplyUserTemplates();

		updateContainerMembership();

		SendPollerMsg(dwRqId, _T("Finished configuration poll for node %s\r\n"), m_szName);
		SendPollerMsg(dwRqId, _T("Node configuration was%schanged after poll\r\n"), bHasChanges ? _T(" ") : _T(" not "));

		m_dwDynamicFlags |= NDF_CONFIGURATION_POLL_PASSED;
   }

   // Finish configuration poll
   SetPollerInfo(nPoller, _T("cleanup"));
   if (dwRqId == 0)
      m_dwDynamicFlags &= ~NDF_QUEUED_FOR_CONFIG_POLL;
   m_dwDynamicFlags &= ~NDF_RECHECK_CAPABILITIES;
   pollerUnlock();
   DbgPrintf(4, _T("Finished configuration poll for node %s (ID: %d)"), m_szName, m_dwId);

   if (bHasChanges)
   {
      LockData();
      Modify();
      UnlockData();
   }
}


//
// Update interface configuration
//

BOOL Node::updateInterfaceConfiguration(DWORD dwRqId, DWORD dwNetMask)
{
   InterfaceList *pIfList;
   Interface **ppDeleteList;
   int i, j, iDelCount;
	BOOL hasChanges = FALSE;
	Cluster *pCluster = getMyCluster();

   SendPollerMsg(dwRqId, _T("Checking interface configuration...\r\n"));
   pIfList = getInterfaceList();
   if (pIfList != NULL)
   {
		// Remove cluster virtual interfaces from list
		if (pCluster != NULL)
		{
			for(i = 0; i < pIfList->getSize(); i++)
			{
				if (pCluster->isVirtualAddr(pIfList->get(i)->dwIpAddr))
				{
					pIfList->remove(i);
					i--;
				}
			}
		}

      // Find non-existing interfaces
      LockChildList(FALSE);
      ppDeleteList = (Interface **)malloc(sizeof(Interface *) * m_dwChildCount);
      for(i = 0, iDelCount = 0; i < (int)m_dwChildCount; i++)
      {
         if (m_pChildList[i]->Type() == OBJECT_INTERFACE)
         {
            Interface *pInterface = (Interface *)m_pChildList[i];
				if (!pInterface->isManuallyCreated())
				{
					for(j = 0; j < pIfList->getSize(); j++)
					{
						if ((pIfList->get(j)->dwIndex == pInterface->getIfIndex()) &&
							 (pIfList->get(j)->dwIpAddr == pInterface->IpAddr()))
							break;
					}

					if (j == pIfList->getSize())
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
            SendPollerMsg(dwRqId, POLLER_WARNING _T("   Interface \"%s\" is no longer exist\r\n"), 
                          ppDeleteList[j]->Name());
            PostEvent(EVENT_INTERFACE_DELETED, m_dwId, "dsaa", ppDeleteList[j]->getIfIndex(),
                      ppDeleteList[j]->Name(), ppDeleteList[j]->IpAddr(), ppDeleteList[j]->getIpNetMask());
            deleteInterface(ppDeleteList[j]);
         }
         hasChanges = TRUE;
      }
      safe_free(ppDeleteList);

      // Add new interfaces and check configuration of existing
      for(j = 0; j < pIfList->getSize(); j++)
      {
			NX_INTERFACE_INFO *ifInfo = pIfList->get(j);
         BOOL bNewInterface = TRUE;

         LockChildList(FALSE);
         for(i = 0; i < (int)m_dwChildCount; i++)
         {
            if (m_pChildList[i]->Type() == OBJECT_INTERFACE)
            {
               Interface *pInterface = (Interface *)m_pChildList[i];

               if ((ifInfo->dwIndex == pInterface->getIfIndex()) &&
                   (ifInfo->dwIpAddr == pInterface->IpAddr()))
               {
                  // Existing interface, check configuration
                  if (memcmp(ifInfo->bMacAddr, pInterface->getMacAddr(), MAC_ADDR_LENGTH))
                  {
                     TCHAR szOldMac[16], szNewMac[16];

                     BinToStr((BYTE *)pInterface->getMacAddr(), MAC_ADDR_LENGTH, szOldMac);
                     BinToStr(ifInfo->bMacAddr, MAC_ADDR_LENGTH, szNewMac);
                     PostEvent(EVENT_MAC_ADDR_CHANGED, m_dwId, "idsss",
                               pInterface->Id(), pInterface->getIfIndex(),
                               pInterface->Name(), szOldMac, szNewMac);
                     pInterface->setMacAddr(ifInfo->bMacAddr);
                  }
                  if (_tcscmp(ifInfo->szName, pInterface->Name()))
                  {
                     pInterface->setName(ifInfo->szName);
                  }
                  if (_tcscmp(ifInfo->szDescription, pInterface->getDescription()))
                  {
                     pInterface->setDescription(ifInfo->szDescription);
                  }
						if (ifInfo->dwBridgePortNumber != pInterface->getBridgePortNumber())
						{
							pInterface->setBridgePortNumber(ifInfo->dwBridgePortNumber);
						}
						if (ifInfo->dwSlotNumber != pInterface->getSlotNumber())
						{
							pInterface->setSlotNumber(ifInfo->dwSlotNumber);
						}
						if (ifInfo->dwPortNumber != pInterface->getPortNumber())
						{
							pInterface->setPortNumber(ifInfo->dwPortNumber);
						}
						if (ifInfo->isPhysicalPort != pInterface->isPhysicalPort())
						{
							pInterface->setPhysicalPortFlag(ifInfo->isPhysicalPort);
						}
						if ((ifInfo->dwIpNetMask != 0) && (ifInfo->dwIpNetMask != pInterface->getIpNetMask()))
						{
							pInterface->setIpNetMask(ifInfo->dwIpNetMask);
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
            SendPollerMsg(dwRqId, POLLER_INFO _T("   Found new interface \"%s\"\r\n"), ifInfo->szName);
            createNewInterface(ifInfo->dwIpAddr, 
                               ifInfo->dwIpNetMask,
                               ifInfo->szName,
										 ifInfo->szDescription,
                               ifInfo->dwIndex,
                               ifInfo->dwType,
                               ifInfo->bMacAddr,
										 ifInfo->dwBridgePortNumber,
										 ifInfo->dwSlotNumber,
										 ifInfo->dwPortNumber,
										 ifInfo->isPhysicalPort);
            hasChanges = TRUE;
         }
      }

      // Check if address we are using to communicate with node
      // is configured on one of node's interfaces
      for(i = 0; i < pIfList->getSize(); i++)
         if (pIfList->get(i)->dwIpAddr == m_dwIpAddr)
            break;

      if (i == (DWORD)pIfList->getSize())
      {
         // Node is behind NAT
         if (!(m_dwFlags & NF_BEHIND_NAT))
         {
				m_dwFlags |= NF_BEHIND_NAT;
				hasChanges = TRUE;
			}
      }
      else
      {
         // Check if NF_BEHIND_NAT flag set incorrectly
         if (m_dwFlags & NF_BEHIND_NAT)
         {
            m_dwFlags &= ~NF_BEHIND_NAT;
            hasChanges = TRUE;
         }
      }

		checkSubnetBinding(pIfList);

		delete pIfList;
   }
   else     /* pIfList == NULL */
   {
      Interface *pInterface;
      DWORD dwCount;

      SendPollerMsg(dwRqId, POLLER_ERROR _T("Unable to get interface list from node\r\n"));

      // Delete all existing interfaces in case of forced capability recheck
      if (m_dwDynamicFlags & NDF_RECHECK_CAPABILITIES)
      {
         LockChildList(FALSE);
         ppDeleteList = (Interface **)malloc(sizeof(Interface *) * m_dwChildCount);
         for(i = 0, iDelCount = 0; i < (int)m_dwChildCount; i++)
         {
				if ((m_pChildList[i]->Type() == OBJECT_INTERFACE) && !((Interface *)m_pChildList[i])->isManuallyCreated())
               ppDeleteList[iDelCount++] = (Interface *)m_pChildList[i];
         }
         UnlockChildList();
         for(j = 0; j < iDelCount; j++)
         {
            SendPollerMsg(dwRqId, POLLER_WARNING _T("   Interface \"%s\" is no longer exist\r\n"), 
                          ppDeleteList[j]->Name());
            PostEvent(EVENT_INTERFACE_DELETED, m_dwId, "dsaa", ppDeleteList[j]->getIfIndex(),
                      ppDeleteList[j]->Name(), ppDeleteList[j]->IpAddr(), ppDeleteList[j]->getIpNetMask());
            deleteInterface(ppDeleteList[j]);
         }
         safe_free(ppDeleteList);
      }

      // Check if we have pseudo-interface object
      dwCount = getInterfaceCount(&pInterface);
      if (dwCount == 1)
      {
         if (pInterface->isFake())
         {
            // Check if primary IP is different from interface's IP
            if (pInterface->IpAddr() != m_dwIpAddr)
            {
               deleteInterface(pInterface);
					if (m_dwIpAddr != 0)
                  createNewInterface(m_dwIpAddr, dwNetMask);
            }
         }
      }
      else if (dwCount == 0)
      {
         // No interfaces at all, create pseudo-interface
			if (m_dwIpAddr != 0)
         	createNewInterface(m_dwIpAddr, dwNetMask);
      }
   }

	SendPollerMsg(dwRqId, _T("Interface configuration check finished\r\n"));
	return hasChanges;
}


//
// Apply system templates
//

void Node::ApplySystemTemplates()
{
	Template *pTemplate;

	pTemplate = FindTemplateByName(_T("@System.Agent"));
	if (pTemplate != NULL)
	{
		if (isNativeAgent())
		{
			if (!pTemplate->IsChild(m_dwId))
			{
				DbgPrintf(4, _T("Node::ApplySystemTemplates(): applying template %d \"%s\" to node %d \"%s\""),
				          pTemplate->Id(), pTemplate->Name(), m_dwId, m_szName);
				pTemplate->ApplyToNode(this);
			}
		}
		else
		{
			if (pTemplate->IsChild(m_dwId))
			{
				DbgPrintf(4, _T("Node::ApplySystemTemplates(): removing template %d \"%s\" from node %d \"%s\""),
				          pTemplate->Id(), pTemplate->Name(), m_dwId, m_szName);
				pTemplate->DeleteChild(this);
				DeleteParent(pTemplate);
				pTemplate->queueRemoveFromNode(m_dwId, TRUE);
			}
		}
	}
}


//
// Apply user templates
//

static void ApplyTemplate(NetObj *object, void *node)
{
   if ((object->Type() == OBJECT_TEMPLATE) && !object->isDeleted())
   {
      Template *pTemplate = (Template *)object;
		if (pTemplate->isApplicable((Node *)node))
		{
			if (!pTemplate->IsChild(((Node *)node)->Id()))
			{
				DbgPrintf(4, _T("Node::ApplyUserTemplates(): applying template %d \"%s\" to node %d \"%s\""),
				          pTemplate->Id(), pTemplate->Name(), ((Node *)node)->Id(), ((Node *)node)->Name());
				pTemplate->ApplyToNode((Node *)node);
			}
		}
		else
		{
			if (pTemplate->isAutoApplyEnabled() && pTemplate->IsChild(((Node *)node)->Id()))
			{
				DbgPrintf(4, _T("Node::ApplyUserTemplates(): removing template %d \"%s\" from node %d \"%s\""),
				          pTemplate->Id(), pTemplate->Name(), ((Node *)node)->Id(), ((Node *)node)->Name());
				pTemplate->DeleteChild((Node *)node);
				((Node *)node)->DeleteParent(pTemplate);
				pTemplate->queueRemoveFromNode(((Node *)node)->Id(), TRUE);
			}
		}
   }
}

void Node::ApplyUserTemplates()
{
	g_idxObjectById.forEach(ApplyTemplate, this);
}


//
// Update container membership
//

static void UpdateContainerBinding(NetObj *object, void *node)
{
   if ((object->Type() == OBJECT_CONTAINER) && !object->isDeleted())
   {
      Container *pContainer = (Container *)object;
		if (pContainer->isSuitableForNode((Node *)node))
		{
			if (!pContainer->IsChild(((Node *)node)->Id()))
			{
				DbgPrintf(4, _T("Node::UpdateContainerMembership(): binding node %d \"%s\" to container %d \"%s\""),
				          ((Node *)node)->Id(), ((Node *)node)->Name(), pContainer->Id(), pContainer->Name());
				pContainer->AddChild((Node *)node);
				((Node *)node)->AddParent(pContainer);
			}
		}
		else
		{
			if (pContainer->isAutoBindEnabled() && pContainer->IsChild(((Node *)node)->Id()))
			{
				DbgPrintf(4, _T("Node::UpdateContainerMembership(): removing node %d \"%s\" from container %d \"%s\""),
				          ((Node *)node)->Id(), ((Node *)node)->Name(), pContainer->Id(), pContainer->Name());
				pContainer->DeleteChild((Node *)node);
				((Node *)node)->DeleteParent(pContainer);
			}
		}
   }
}

void Node::updateContainerMembership()
{
	g_idxObjectById.forEach(UpdateContainerBinding, this);
}


//
// Connect to native agent
//

BOOL Node::connectToAgent(DWORD *error, DWORD *socketError)
{
   BOOL bRet;

   // Create new agent connection object if needed
   if (m_pAgentConnection == NULL)
      m_pAgentConnection = new AgentConnectionEx(htonl(m_dwIpAddr), m_wAgentPort, m_wAuthMethod, m_szSharedSecret);

   // Check if we already connected
   if (m_pAgentConnection->nop() == ERR_SUCCESS)
      return TRUE;

   // Close current connection or clean up after broken connection
   m_pAgentConnection->disconnect();
   m_pAgentConnection->setPort(m_wAgentPort);
#ifdef UNICODE
	char mbSecret[MAX_SECRET_LENGTH];
	WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, m_szSharedSecret, -1, mbSecret, MAX_SECRET_LENGTH, NULL, NULL);
	mbSecret[MAX_SECRET_LENGTH - 1] = 0;
   m_pAgentConnection->setAuthData(m_wAuthMethod, mbSecret);
#else
   m_pAgentConnection->setAuthData(m_wAuthMethod, m_szSharedSecret);
#endif
   setAgentProxy(m_pAgentConnection);
   bRet = m_pAgentConnection->connect(g_pServerKey, FALSE, error, socketError);
   if (bRet)
	{
		m_pAgentConnection->setCommandTimeout(g_dwAgentCommandTimeout);
      m_pAgentConnection->enableTraps();
	}
   return bRet;
}


//
// Get item's value via SNMP
//

DWORD Node::getItemFromSNMP(WORD port, const TCHAR *szParam, DWORD dwBufSize, TCHAR *szBuffer, int interpretRawValue)
{
   DWORD dwResult;

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
				dwResult = SnmpGet(m_snmpVersion, pTransport, szParam, NULL, 0, szBuffer, dwBufSize, SG_PSTRING_RESULT);
			}
			else
			{
				BYTE rawValue[1024];
				memset(rawValue, 0, 1024);
				dwResult = SnmpGet(m_snmpVersion, pTransport, szParam, NULL, 0, rawValue, 1024, SG_RAW_RESULT);
				if (dwResult == SNMP_ERR_SUCCESS)
				{
					switch(interpretRawValue)
					{
						case SNMP_RAWTYPE_INT32:
							_sntprintf(szBuffer, dwBufSize, _T("%d"), ntohl(*((LONG *)rawValue)));
							break;
						case SNMP_RAWTYPE_UINT32:
							_sntprintf(szBuffer, dwBufSize, _T("%u"), ntohl(*((DWORD *)rawValue)));
							break;
						case SNMP_RAWTYPE_INT64:
							_sntprintf(szBuffer, dwBufSize, INT64_FMT, ntohq(*((INT64 *)rawValue)));
							break;
						case SNMP_RAWTYPE_UINT64:
							_sntprintf(szBuffer, dwBufSize, UINT64_FMT, ntohq(*((QWORD *)rawValue)));
							break;
						case SNMP_RAWTYPE_DOUBLE:
							_sntprintf(szBuffer, dwBufSize, _T("%f"), ntohd(*((double *)rawValue)));
							break;
						case SNMP_RAWTYPE_IP_ADDR:
							IpToStr(ntohl(*((DWORD *)rawValue)), szBuffer);
							break;
						case SNMP_RAWTYPE_MAC_ADDR:
							MACToStr(rawValue, szBuffer);
							break;
						default:
							szBuffer[0] = 0;
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
   DbgPrintf(7, _T("Node(%s)->GetItemFromSNMP(%s): dwResult=%d"), m_szName, szParam, dwResult);
   return (dwResult == SNMP_ERR_SUCCESS) ? DCE_SUCCESS : 
      ((dwResult == SNMP_ERR_NO_OBJECT) ? DCE_NOT_SUPPORTED : DCE_COMM_ERROR);
}


//
// Get item's value via SNMP from CheckPoint's agent
//

DWORD Node::GetItemFromCheckPointSNMP(const TCHAR *szParam, DWORD dwBufSize, TCHAR *szBuffer)
{
   DWORD dwResult;

   if ((m_dwDynamicFlags & NDF_CPSNMP_UNREACHABLE) ||
       (m_dwDynamicFlags & NDF_UNREACHABLE))
   {
      dwResult = SNMP_ERR_COMM;
   }
   else
   {
		SNMP_Transport *pTransport;

		pTransport = new SNMP_UDPTransport;
		((SNMP_UDPTransport *)pTransport)->createUDPTransport(NULL, htonl(m_dwIpAddr), CHECKPOINT_SNMP_PORT);
      dwResult = SnmpGet(SNMP_VERSION_1, pTransport, szParam, NULL, 0, szBuffer, dwBufSize, SG_STRING_RESULT);
		delete pTransport;
   }
   DbgPrintf(7, _T("Node(%s)->GetItemFromCheckPointSNMP(%s): dwResult=%d"), m_szName, szParam, dwResult);
   return (dwResult == SNMP_ERR_SUCCESS) ? DCE_SUCCESS : 
      ((dwResult == SNMP_ERR_NO_OBJECT) ? DCE_NOT_SUPPORTED : DCE_COMM_ERROR);
}


//
// Get item's value via native agent
//

DWORD Node::GetItemFromAgent(const TCHAR *szParam, DWORD dwBufSize, TCHAR *szBuffer)
{
   DWORD dwError = ERR_NOT_CONNECTED, dwResult = DCE_COMM_ERROR;
   DWORD dwTries = 3;

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
				DbgPrintf(7, _T("Node(%s)->GetItemFromAgent(%s): timeout; resetting connection to agent..."), m_szName, szParam);
				delete_and_null(m_pAgentConnection);
            if (!connectToAgent())
               goto end_loop;
				DbgPrintf(7, _T("Node(%s)->GetItemFromAgent(%s): connection to agent restored successfully"), m_szName, szParam);
            break;
      }
   }

end_loop:
   agentUnlock();
   DbgPrintf(7, _T("Node(%s)->GetItemFromAgent(%s): dwError=%d dwResult=%d"), m_szName, szParam, dwError, dwResult);
   return dwResult;
}


//
// Get table from agent
//

DWORD Node::getTableFromAgent(const TCHAR *name, Table **table)
{
   DWORD dwError = ERR_NOT_CONNECTED, dwResult = DCE_COMM_ERROR;
   DWORD dwTries = 3;

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
				DbgPrintf(7, _T("Node(%s)->getTableFromAgent(%s): timeout; resetting connection to agent..."), m_szName, name);
				delete_and_null(m_pAgentConnection);
            if (!connectToAgent())
               goto end_loop;
				DbgPrintf(7, _T("Node(%s)->getTableFromAgent(%s): connection to agent restored successfully"), m_szName, name);
            break;
      }
   }

end_loop:
   agentUnlock();
   DbgPrintf(7, _T("Node(%s)->getTableFromAgent(%s): dwError=%d dwResult=%d"), m_szName, name, dwError, dwResult);
   return dwResult;
}


//
// Get value for server's internal parameter
//

DWORD Node::GetInternalItem(const TCHAR *szParam, DWORD dwBufSize, TCHAR *szBuffer)
{
   DWORD dwError = DCE_SUCCESS;

   if (!_tcsicmp(szParam, _T("Status")))
   {
      _sntprintf(szBuffer, dwBufSize, _T("%d"), m_iStatus);
   }
   else if (!_tcsicmp(szParam, _T("Dummy")))
   {
      _tcscpy(szBuffer, _T("0"));
   }
   else if (MatchString(_T("Net.IP.NextHop(*)"), szParam, FALSE))
   {
      if ((m_dwFlags & NF_IS_NATIVE_AGENT) || (m_dwFlags & NF_IS_SNMP))
		{
			TCHAR arg[256] = _T("");
			DWORD destAddr, nextHop, ifIndex;
			BOOL isVpn;

	      AgentGetParameterArg(szParam, 1, arg, 256);
			destAddr = ntohl(_t_inet_addr(arg));
			if ((destAddr > 0) && (destAddr < 0xE0000000))
			{
				if (getNextHop(m_dwIpAddr, destAddr, &nextHop, &ifIndex, &isVpn))
				{
					IpToStr(nextHop, szBuffer);
				}
				else
				{
					_tcscpy(szBuffer, _T("UNREACHABLE"));
				}
			}
			else
			{
	         dwError = DCE_NOT_SUPPORTED;
			}
		}
		else
		{
         dwError = DCE_NOT_SUPPORTED;
		}
   }
   else if (!_tcsicmp(szParam, _T("AgentStatus")))
   {
      if (m_dwFlags & NF_IS_NATIVE_AGENT)
      {
         szBuffer[0] = (m_dwDynamicFlags & NDF_AGENT_UNREACHABLE) ? _T('1') : _T('0');
         szBuffer[1] = 0;
      }
      else
      {
         dwError = DCE_NOT_SUPPORTED;
      }
   }
   else if (MatchString(_T("ChildStatus(*)"), szParam, FALSE))
   {
      TCHAR *pEnd, szArg[256];
      DWORD i, dwId;
      NetObj *pObject = NULL;

      AgentGetParameterArg(szParam, 1, szArg, 256);
      dwId = _tcstoul(szArg, &pEnd, 0);
      if (*pEnd != 0)
      {
         // Argument is object's name
         dwId = 0;
      }

      // Find child object with requested ID or name
      LockChildList(FALSE);
      for(i = 0; i < m_dwChildCount; i++)
      {
         if (((dwId == 0) && (!_tcsicmp(m_pChildList[i]->Name(), szArg))) ||
             (dwId == m_pChildList[i]->Id()))
         {
            pObject = m_pChildList[i];
            break;
         }
      }
      UnlockChildList();

      if (pObject != NULL)
      {
         _sntprintf(szBuffer, dwBufSize, _T("%d"), pObject->Status());
      }
      else
      {
         dwError = DCE_NOT_SUPPORTED;
      }
   }
   else if (MatchString(_T("ConditionStatus(*)"), szParam, FALSE))
   {
      TCHAR *pEnd, szArg[256];
      DWORD dwId;
      NetObj *pObject = NULL;

      AgentGetParameterArg(szParam, 1, szArg, 256);
      dwId = _tcstoul(szArg, &pEnd, 0);
      if (*pEnd == 0)
		{
			pObject = FindObjectById(dwId);
			if (pObject != NULL)
				if (pObject->Type() != OBJECT_CONDITION)
					pObject = NULL;
		}
		else
      {
         // Argument is object's name
			pObject = FindObjectByName(szArg, OBJECT_CONDITION);
      }

      if (pObject != NULL)
      {
			if (pObject->IsTrustedNode(m_dwId))
			{
				_sntprintf(szBuffer, dwBufSize, _T("%d"), pObject->Status());
			}
			else
			{
	         dwError = DCE_NOT_SUPPORTED;
			}
      }
      else
      {
         dwError = DCE_NOT_SUPPORTED;
      }
   }
   else if (m_dwFlags & NF_IS_LOCAL_MGMT)
   {
      if (!_tcsicmp(szParam, _T("Server.AverageDCPollerQueueSize")))
      {
         _sntprintf(szBuffer, dwBufSize, _T("%f"), g_dAvgPollerQueueSize);
      }
      else if (!_tcsicmp(szParam, _T("Server.AverageDBWriterQueueSize")))
      {
         _sntprintf(szBuffer, dwBufSize, _T("%f"), g_dAvgDBAndIDataWriterQueueSize);
      }
      else if (!_tcsicmp(szParam, _T("Server.AverageDBWriterQueueSize.Other")))
      {
         _sntprintf(szBuffer, dwBufSize, _T("%f"), g_dAvgDBWriterQueueSize);
      }
      else if (!_tcsicmp(szParam, _T("Server.AverageDBWriterQueueSize.IData")))
      {
         _sntprintf(szBuffer, dwBufSize, _T("%f"), g_dAvgIDataWriterQueueSize);
      }
      else if (!_tcsicmp(szParam, _T("Server.AverageStatusPollerQueueSize")))
      {
         _sntprintf(szBuffer, dwBufSize, _T("%f"), g_dAvgStatusPollerQueueSize);
      }
      else if (!_tcsicmp(szParam, _T("Server.AverageConfigurationPollerQueueSize")))
      {
         _sntprintf(szBuffer, dwBufSize, _T("%f"), g_dAvgConfigPollerQueueSize);
      }
      else if (!_tcsicmp(szParam, _T("Server.AverageDCIQueuingTime")))
      {
         _sntprintf(szBuffer, dwBufSize, _T("%u"), g_dwAvgDCIQueuingTime);
      }
      else if (!_tcsicmp(szParam, _T("Server.TotalEventsProcessed")))
      {
         _sntprintf(szBuffer, dwBufSize, INT64_FMT, g_totalEventsProcessed);
      }
      else
      {
         dwError = DCE_NOT_SUPPORTED;
      }
   }
   else
   {
      dwError = DCE_NOT_SUPPORTED;
   }

   return dwError;
}


//
// Translate DCI error code into RCC
//

static DWORD RCCFromDCIError(DWORD error)
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


//
// Get item's value for client
//

DWORD Node::getItemForClient(int iOrigin, const TCHAR *pszParam, TCHAR *pszBuffer, DWORD dwBufSize)
{
   DWORD dwResult = 0, dwRetCode;

   // Get data from node
   switch(iOrigin)
   {
      case DS_INTERNAL:
         dwRetCode = GetInternalItem(pszParam, dwBufSize, pszBuffer);
         break;
      case DS_NATIVE_AGENT:
         dwRetCode = GetItemFromAgent(pszParam, dwBufSize, pszBuffer);
         break;
      case DS_SNMP_AGENT:
			dwRetCode = getItemFromSNMP(0, pszParam, dwBufSize, pszBuffer, SNMP_RAWTYPE_NONE);
         break;
      case DS_CHECKPOINT_AGENT:
         dwRetCode = GetItemFromCheckPointSNMP(pszParam, dwBufSize, pszBuffer);
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


//
// Get table for client
//

DWORD Node::getTableForClient(const TCHAR *name, Table **table)
{
	DWORD dwRetCode = getTableFromAgent(name, table);
	return RCCFromDCIError(dwRetCode);
}


//
// Put items which requires polling into the queue
//

void Node::queueItemsForPolling(Queue *pPollerQueue)
{
   if ((m_iStatus == STATUS_UNMANAGED) ||
	    (m_dwFlags & NF_DISABLE_DATA_COLLECT) ||
		 (m_bIsDeleted))
      return;  // Do not collect data for unmanaged nodes or if data collection is disabled

   time_t currTime = time(NULL);

   lockDciAccess();
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
		DCObject *object = m_dcObjects->get(i);
      if (object->isReadyForPolling(currTime))
      {
         object->setBusyFlag(TRUE);
         IncRefCount();   // Increment reference count for each queued DCI
         pPollerQueue->Put(object);
			DbgPrintf(8, _T("Node(%s)->QueueItemsForPolling(): item %d \"%s\" added to queue"), m_szName, object->getId(), object->getName());
      }
   }
   unlockDciAccess();
}


//
// Create CSCP message with object's data
//

void Node::CreateMessage(CSCPMessage *pMsg)
{
   Template::CreateMessage(pMsg);
	pMsg->SetVariable(VID_PRIMARY_NAME, m_primaryName);
   pMsg->SetVariable(VID_FLAGS, m_dwFlags);
   pMsg->SetVariable(VID_RUNTIME_FLAGS, m_dwDynamicFlags);
   pMsg->SetVariable(VID_AGENT_PORT, m_wAgentPort);
   pMsg->SetVariable(VID_AUTH_METHOD, m_wAuthMethod);
   pMsg->SetVariable(VID_SHARED_SECRET, m_szSharedSecret);
	pMsg->SetVariableFromMBString(VID_SNMP_AUTH_OBJECT, m_snmpSecurity->getCommunity());
	pMsg->SetVariableFromMBString(VID_SNMP_AUTH_PASSWORD, m_snmpSecurity->getAuthPassword());
	pMsg->SetVariableFromMBString(VID_SNMP_PRIV_PASSWORD, m_snmpSecurity->getPrivPassword());
	pMsg->SetVariable(VID_SNMP_USM_METHODS, (WORD)((WORD)m_snmpSecurity->getAuthMethod() | ((WORD)m_snmpSecurity->getPrivMethod() << 8)));
   pMsg->SetVariable(VID_SNMP_OID, m_szObjectId);
   pMsg->SetVariable(VID_SNMP_PORT, m_wSNMPPort);
   pMsg->SetVariable(VID_SNMP_VERSION, (WORD)m_snmpVersion);
   pMsg->SetVariable(VID_AGENT_VERSION, m_szAgentVersion);
   pMsg->SetVariable(VID_PLATFORM_NAME, m_szPlatformName);
   pMsg->SetVariable(VID_POLLER_NODE_ID, m_dwPollerNode);
   pMsg->SetVariable(VID_ZONE_ID, m_zoneId);
   pMsg->SetVariable(VID_AGENT_PROXY, m_dwProxyNode);
   pMsg->SetVariable(VID_SNMP_PROXY, m_dwSNMPProxy);
	pMsg->SetVariable(VID_REQUIRED_POLLS, (WORD)m_iRequiredPollCount);
	pMsg->SetVariable(VID_SYS_NAME, CHECK_NULL_EX(m_sysName));
	pMsg->SetVariable(VID_SYS_DESCRIPTION, CHECK_NULL_EX(m_sysDescription));
	pMsg->SetVariable(VID_BRIDGE_BASE_ADDRESS, m_baseBridgeAddress, 6);
	if (m_lldpNodeId != NULL)
		pMsg->SetVariable(VID_LLDP_NODE_ID, m_lldpNodeId);
	pMsg->SetVariable(VID_USE_IFXTABLE, (WORD)m_nUseIfXTable);
	if (m_vrrpInfo != NULL)
	{
		pMsg->SetVariable(VID_VRRP_VERSION, (WORD)m_vrrpInfo->getVersion());
		pMsg->SetVariable(VID_VRRP_VR_COUNT, (WORD)m_vrrpInfo->getSize());
	}
	if (m_driver != NULL)
	{
		pMsg->SetVariable(VID_DRIVER_NAME, m_driver->getName());
		pMsg->SetVariable(VID_DRIVER_VERSION, m_driver->getVersion());
	}
}


//
// Modify object from message
//

DWORD Node::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
   if (!bAlreadyLocked)
      LockData();

   // Change primary IP address
   if (pRequest->IsVariableExist(VID_IP_ADDRESS))
   {
      DWORD i, dwIpAddr;
      
      dwIpAddr = pRequest->GetVariableLong(VID_IP_ADDRESS);

      // Check if received IP address is one of node's interface addresses
      LockChildList(FALSE);
      for(i = 0; i < m_dwChildCount; i++)
         if ((m_pChildList[i]->Type() == OBJECT_INTERFACE) &&
             (m_pChildList[i]->IpAddr() == dwIpAddr))
            break;
      UnlockChildList();
      if (i == m_dwChildCount)
      {
         UnlockData();
         return RCC_INVALID_IP_ADDR;
      }

      m_dwIpAddr = dwIpAddr;

		// Update primary name if it is not set with the same message
		if (!pRequest->IsVariableExist(VID_PRIMARY_NAME))
		{
			IpToStr(m_dwIpAddr, m_primaryName);
		}
   }

   // Change primary host name
   if (pRequest->IsVariableExist(VID_PRIMARY_NAME))
   {
		pRequest->GetVariableStr(VID_PRIMARY_NAME, m_primaryName, MAX_DNS_NAME);
		m_dwDynamicFlags |= NDF_FORCE_CONFIGURATION_POLL | NDF_RECHECK_CAPABILITIES;
	}

   // Poller node ID
   if (pRequest->IsVariableExist(VID_POLLER_NODE_ID))
   {
      DWORD dwNodeId;
      NetObj *pObject;
      
      dwNodeId = pRequest->GetVariableLong(VID_POLLER_NODE_ID);
		if (dwNodeId != 0)
		{
			pObject = FindObjectById(dwNodeId);

			// Check if received id is a valid node id
			if (pObject == NULL)
			{
				UnlockData();
				return RCC_INVALID_OBJECT_ID;
			}
			if (pObject->Type() != OBJECT_NODE)
			{
				UnlockData();
				return RCC_INVALID_OBJECT_ID;
			}
		}
		m_dwPollerNode = dwNodeId;
   }

   // Change listen port of native agent
   if (pRequest->IsVariableExist(VID_AGENT_PORT))
      m_wAgentPort = pRequest->GetVariableShort(VID_AGENT_PORT);

   // Change authentication method of native agent
   if (pRequest->IsVariableExist(VID_AUTH_METHOD))
      m_wAuthMethod = pRequest->GetVariableShort(VID_AUTH_METHOD);

   // Change shared secret of native agent
   if (pRequest->IsVariableExist(VID_SHARED_SECRET))
      pRequest->GetVariableStr(VID_SHARED_SECRET, m_szSharedSecret, MAX_SECRET_LENGTH);

   // Change SNMP protocol version
   if (pRequest->IsVariableExist(VID_SNMP_VERSION))
	{
      m_snmpVersion = pRequest->GetVariableShort(VID_SNMP_VERSION);
		m_snmpSecurity->setSecurityModel((m_snmpVersion == SNMP_VERSION_3) ? SNMP_SECURITY_MODEL_USM : SNMP_SECURITY_MODEL_V2C);
	}

   // Change SNMP port
   if (pRequest->IsVariableExist(VID_SNMP_PORT))
		m_wSNMPPort = pRequest->GetVariableShort(VID_SNMP_PORT);

   // Change SNMP authentication data
   if (pRequest->IsVariableExist(VID_SNMP_AUTH_OBJECT))
	{
		char mbBuffer[256];

      pRequest->GetVariableStrA(VID_SNMP_AUTH_OBJECT, mbBuffer, 256);
		m_snmpSecurity->setAuthName(mbBuffer);

		pRequest->GetVariableStrA(VID_SNMP_AUTH_PASSWORD, mbBuffer, 256);
		m_snmpSecurity->setAuthPassword(mbBuffer);

		pRequest->GetVariableStrA(VID_SNMP_PRIV_PASSWORD, mbBuffer, 256);
		m_snmpSecurity->setPrivPassword(mbBuffer);

		WORD methods = pRequest->GetVariableShort(VID_SNMP_USM_METHODS);
		m_snmpSecurity->setAuthMethod((int)(methods & 0xFF));
		m_snmpSecurity->setPrivMethod((int)(methods >> 8));
	}

   // Change proxy node
   if (pRequest->IsVariableExist(VID_AGENT_PROXY))
      m_dwProxyNode = pRequest->GetVariableLong(VID_AGENT_PROXY);

   // Change SNMP proxy node
   if (pRequest->IsVariableExist(VID_SNMP_PROXY))
      m_dwSNMPProxy = pRequest->GetVariableLong(VID_SNMP_PROXY);

   // Number of required polls
   if (pRequest->IsVariableExist(VID_REQUIRED_POLLS))
      m_iRequiredPollCount = (int)pRequest->GetVariableShort(VID_REQUIRED_POLLS);

   // Enable/disable usage of ifXTable
   if (pRequest->IsVariableExist(VID_USE_IFXTABLE))
      m_nUseIfXTable = (BYTE)pRequest->GetVariableShort(VID_USE_IFXTABLE);

   // Change flags
   if (pRequest->IsVariableExist(VID_FLAGS))
   {
      m_dwFlags &= NF_SYSTEM_FLAGS;
      m_dwFlags |= pRequest->GetVariableLong(VID_FLAGS) & NF_USER_FLAGS;
   }

   return Template::ModifyFromMessage(pRequest, TRUE);
}


//
// Wakeup node using magic packet
//

DWORD Node::wakeUp()
{
   DWORD i, dwResult = RCC_NO_WOL_INTERFACES;

   LockChildList(FALSE);

   for(i = 0; i < m_dwChildCount; i++)
      if ((m_pChildList[i]->Type() == OBJECT_INTERFACE) &&
          (m_pChildList[i]->Status() != STATUS_UNMANAGED) &&
          (m_pChildList[i]->IpAddr() != 0))
      {
         dwResult = ((Interface *)m_pChildList[i])->wakeUp();
         break;
      }

   UnlockChildList();
   return dwResult;
}


//
// Get status of interface with given index from SNMP agent
//

int Node::getInterfaceStatusFromSNMP(SNMP_Transport *pTransport, DWORD dwIndex)
{
   return SnmpGetInterfaceStatus(m_snmpVersion, pTransport, dwIndex);
}


//
// Get status of interface with given index from native agent
//

int Node::getInterfaceStatusFromAgent(DWORD dwIndex)
{
   TCHAR szParam[128], szBuffer[32];
   DWORD dwAdminStatus, dwLinkState;
   int iStatus;

   // Get administrative status
   _sntprintf(szParam, 128, _T("Net.Interface.AdminStatus(%u)"), dwIndex);
   if (GetItemFromAgent(szParam, 32, szBuffer) == DCE_SUCCESS)
   {
      dwAdminStatus = _tcstoul(szBuffer, NULL, 0);

      switch(dwAdminStatus)
      {
         case 3:
            iStatus = STATUS_TESTING;
            break;
         case 2:
         case 0:     // Agents before 0.2.1 may return 0 instead of 2
            iStatus = STATUS_DISABLED;
            break;
         case 1:     // Interface administratively up, check link state
            _sntprintf(szParam, 128, _T("Net.Interface.Link(%u)"), dwIndex);
            if (GetItemFromAgent(szParam, 32, szBuffer) == DCE_SUCCESS)
            {
               dwLinkState = _tcstoul(szBuffer, NULL, 0);
               iStatus = (dwLinkState == 0) ? STATUS_CRITICAL : STATUS_NORMAL;
            }
            else
            {
               iStatus = STATUS_UNKNOWN;
            }
            break;
         default:
            iStatus = STATUS_UNKNOWN;
            break;
      }
   }
   else
   {
      iStatus = STATUS_UNKNOWN;
   }

   return iStatus;
}


//
// Put list of supported parameters into CSCP message
//

void Node::writeParamListToMessage(CSCPMessage *pMsg)
{
   LockData();

	if (m_paramList != NULL)
   {
      pMsg->SetVariable(VID_NUM_PARAMETERS, (DWORD)m_paramList->size());

		int i;
		DWORD dwId;
      for(i = 0, dwId = VID_PARAM_LIST_BASE; i < m_paramList->size(); i++)
      {
			NXC_AGENT_PARAM *p = m_paramList->get(i);
         pMsg->SetVariable(dwId++, p->szName);
         pMsg->SetVariable(dwId++, p->szDescription);
         pMsg->SetVariable(dwId++, (WORD)p->iDataType);
      }
		DbgPrintf(6, _T("Node[%s]::writeParamListToMessage(): sending %d parameters"), m_szName, m_paramList->size());
   }
   else
   {
		DbgPrintf(6, _T("Node[%s]::writeParamListToMessage(): m_paramList == NULL"), m_szName);
      pMsg->SetVariable(VID_NUM_PARAMETERS, (DWORD)0);
   }

	if (m_tableList != NULL)
   {
		pMsg->SetVariable(VID_NUM_TABLES, (DWORD)m_tableList->size());

		int i;
		DWORD dwId;
      for(i = 0, dwId = VID_TABLE_LIST_BASE; i < m_tableList->size(); i++)
      {
			NXC_AGENT_TABLE *t = m_tableList->get(i);
         pMsg->SetVariable(dwId++, t->name);
         pMsg->SetVariable(dwId++, t->instanceColumn);
         pMsg->SetVariable(dwId++, t->description);
      }
		DbgPrintf(6, _T("Node[%s]::writeParamListToMessage(): sending %d tables"), m_szName, m_tableList->size());
   }
   else
   {
		DbgPrintf(6, _T("Node[%s]::writeParamListToMessage(): m_tableList == NULL"), m_szName);
      pMsg->SetVariable(VID_NUM_TABLES, (DWORD)0);
   }

	UnlockData();
}


//
// Open list of supported parameters for reading
//

void Node::openParamList(StructArray<NXC_AGENT_PARAM> **paramList)
{
   LockData();
   *paramList = m_paramList;
}


//
// Check status of network service
//

DWORD Node::CheckNetworkService(DWORD *pdwStatus, DWORD dwIpAddr, int iServiceType,
                                WORD wPort, WORD wProto, TCHAR *pszRequest,
                                TCHAR *pszResponse)
{
   DWORD dwError = ERR_NOT_CONNECTED;

   if ((m_dwFlags & NF_IS_NATIVE_AGENT) &&
       (!(m_dwDynamicFlags & NDF_AGENT_UNREACHABLE)) &&
       (!(m_dwDynamicFlags & NDF_UNREACHABLE)))
   {
      AgentConnection *pConn;

      pConn = createAgentConnection();
      if (pConn != NULL)
      {
         dwError = pConn->checkNetworkService(pdwStatus, dwIpAddr, iServiceType,
                                              wPort, wProto, pszRequest, pszResponse);
         pConn->disconnect();
         delete pConn;
      }
   }
   return dwError;
}


//
// Handler for object deletion
//

void Node::OnObjectDelete(DWORD dwObjectId)
{
	LockData();
   if (dwObjectId == m_dwPollerNode)
   {
      // If deleted object is our poller node, change it to default
      m_dwPollerNode = 0;
      Modify();
      DbgPrintf(3, _T("Node \"%s\": poller node %d deleted"), m_szName, dwObjectId);
   }
	UnlockData();
}


//
// Check node for OSPF support
//

void Node::CheckOSPFSupport(SNMP_Transport *pTransport)
{
   LONG nAdminStatus;

   if (SnmpGet(m_snmpVersion, pTransport,
               _T(".1.3.6.1.2.1.14.1.2.0"), NULL, 0, &nAdminStatus, sizeof(LONG), 0) == SNMP_ERR_SUCCESS)
   {
		LockData();
      if (nAdminStatus)
      {
         m_dwFlags |= NF_IS_OSPF;
      }
      else
      {
         m_dwFlags &= ~NF_IS_OSPF;
      }
		UnlockData();
   }
}


//
// Create ready to use agent connection
//

AgentConnectionEx *Node::createAgentConnection()
{
   AgentConnectionEx *conn;

   if ((!(m_dwFlags & NF_IS_NATIVE_AGENT)) ||
	    (m_dwFlags & NF_DISABLE_NXCP) ||
       (m_dwDynamicFlags & NDF_AGENT_UNREACHABLE) ||
       (m_dwDynamicFlags & NDF_UNREACHABLE))
      return NULL;

   DbgPrintf(6, _T("Node::createAgentConnection(%s [%d])"), m_szName, (int)m_dwId);
   conn = new AgentConnectionEx(htonl(m_dwIpAddr), m_wAgentPort, m_wAuthMethod, m_szSharedSecret);
   setAgentProxy(conn);
   if (!conn->connect(g_pServerKey))
   {
      delete conn;
      conn = NULL;
   }
	DbgPrintf(6, _T("Node::createAgentConnection(%s [%d]): conn=%p"), m_szName, (int)m_dwId, conn);
   return conn;
}


//
// Get last collected values of all DCIs
//

DWORD Node::getLastValues(CSCPMessage *pMsg)
{
   lockDciAccess();

	DWORD dwId = VID_DCI_VALUES_BASE, dwCount = 0;
   for(int i = 0; i < m_dcObjects->size(); i++)
	{
		DCObject *object = m_dcObjects->get(i);
		if ((object->getType() == DCO_TYPE_ITEM) && _tcsnicmp(object->getDescription(), _T("@system."), 8))
		{
			((DCItem *)object)->getLastValue(pMsg, dwId);
			dwId += 50;
			dwCount++;
		}
	}
   pMsg->SetVariable(VID_NUM_ITEMS, dwCount);

   unlockDciAccess();
   return RCC_SUCCESS;
}


//
// Clean expired DCI data
//

void Node::cleanDCIData()
{
   lockDciAccess();
   for(int i = 0; i < m_dcObjects->size(); i++)
      m_dcObjects->get(i)->deleteExpiredData();
   unlockDciAccess();
}


//
// Apply DCI from template
// pItem passed to this method should be a template's DCI
//

bool Node::applyTemplateItem(DWORD dwTemplateId, DCObject *dcObject)
{
   bool bResult = true;

   lockDciAccess();	// write lock

   DbgPrintf(5, _T("Applying DCO \"%s\" to node \"%s\""), dcObject->getName(), m_szName);

   // Check if that template item exists
	int i;
   for(i = 0; i < m_dcObjects->size(); i++)
      if ((m_dcObjects->get(i)->getTemplateId() == dwTemplateId) &&
          (m_dcObjects->get(i)->getTemplateItemId() == dcObject->getId()))
         break;   // Item with specified id already exist

   if (i == m_dcObjects->size())
   {
      // New item from template, just add it
		DCObject *newObject;
		switch(dcObject->getType())
		{
			case DCO_TYPE_ITEM:
				newObject = new DCItem((DCItem *)dcObject);
				break;
			case DCO_TYPE_TABLE:
				newObject = new DCTable((DCTable *)dcObject);
				break;
			default:
				newObject = NULL;
				break;
		}
		if (newObject != NULL)
		{
			newObject->setTemplateId(dwTemplateId, dcObject->getId());
			newObject->changeBinding(CreateUniqueId(IDG_ITEM), this, TRUE);
			bResult = addDCObject(newObject, true);
		}
   }
   else
   {
      // Update existing item
      m_dcObjects->get(i)->updateFromTemplate(dcObject);
   }

   unlockDciAccess();

	if (bResult)
	{
		LockData();
		m_bIsModified = TRUE;
		UnlockData();
	}
   return bResult;
}


//
// Clean deleted template items from node's DCI list
// Arguments is template id and list of valid template item ids.
// all items related to given template and not presented in list should be deleted.
//

void Node::cleanDeletedTemplateItems(DWORD dwTemplateId, DWORD dwNumItems, DWORD *pdwItemList)
{
   DWORD i, j, dwNumDeleted, *pdwDeleteList;

   lockDciAccess();  // write lock

   pdwDeleteList = (DWORD *)malloc(sizeof(DWORD) * m_dcObjects->size());
   dwNumDeleted = 0;

   for(i = 0; i < (DWORD)m_dcObjects->size(); i++)
      if (m_dcObjects->get(i)->getTemplateId() == dwTemplateId)
      {
         for(j = 0; j < dwNumItems; j++)
            if (m_dcObjects->get(i)->getTemplateItemId() == pdwItemList[j])
               break;

         // Delete DCI if it's not in list
         if (j == dwNumItems)
            pdwDeleteList[dwNumDeleted++] = m_dcObjects->get(i)->getId();
      }

   for(i = 0; i < dwNumDeleted; i++)
      deleteDCObject(pdwDeleteList[i], false);

   unlockDciAccess();
   free(pdwDeleteList);
}


//
// Unbind node from template, i.e either remove DCI association with template
// or remove these DCIs at all
//

void Node::unbindFromTemplate(DWORD dwTemplateId, BOOL bRemoveDCI)
{
   DWORD i;

   if (bRemoveDCI)
   {
      lockDciAccess();  // write lock

		DWORD *pdwDeleteList = (DWORD *)malloc(sizeof(DWORD) * m_dcObjects->size());
		DWORD dwNumDeleted = 0;

      for(i = 0; i < (DWORD)m_dcObjects->size(); i++)
         if (m_dcObjects->get(i)->getTemplateId() == dwTemplateId)
         {
            pdwDeleteList[dwNumDeleted++] = m_dcObjects->get(i)->getId();
         }

		for(i = 0; i < dwNumDeleted; i++)
			deleteDCObject(pdwDeleteList[i], false);

      unlockDciAccess();

		safe_free(pdwDeleteList);
   }
   else
   {
      lockDciAccess();

      for(int i = 0; i < m_dcObjects->size(); i++)
         if (m_dcObjects->get(i)->getTemplateId() == dwTemplateId)
         {
            m_dcObjects->get(i)->setTemplateId(0, 0);
         }

      unlockDciAccess();
   }
}


//
// Change node's IP address
//

void Node::changeIPAddress(DWORD dwIpAddr)
{
   DWORD i;

   pollerLock();

   LockData();

   m_dwIpAddr = dwIpAddr;
   m_dwDynamicFlags |= NDF_FORCE_CONFIGURATION_POLL | NDF_RECHECK_CAPABILITIES;

   // Change status of node and all it's childs to UNKNOWN
   m_iStatus = STATUS_UNKNOWN;
   LockChildList(FALSE);
   for(i = 0; i < m_dwChildCount; i++)
   {
      m_pChildList[i]->resetStatus();
      if (m_pChildList[i]->Type() == OBJECT_INTERFACE)
      {
         if (((Interface *)m_pChildList[i])->isFake())
         {
            ((Interface *)m_pChildList[i])->setIpAddr(dwIpAddr);
         }
      }
   }
   UnlockChildList();

   Modify();
   UnlockData();

   agentLock();
   delete_and_null(m_pAgentConnection);
   agentUnlock();

   pollerUnlock();
}


//
// Change node's zone
//

void Node::changeZone(DWORD newZone)
{
   DWORD i;

   pollerLock();

   LockData();

   m_zoneId = newZone;
   m_dwDynamicFlags |= NDF_FORCE_CONFIGURATION_POLL;
	m_tLastConfigurationPoll = 0;

	// Remove from subnets
	LockParentList(FALSE);
	NetObj **subnets = (NetObj **)malloc(sizeof(NetObj *) * m_dwParentCount);
	DWORD count = 0;
	for(i = 0; i < m_dwParentCount; i++)
		if (m_pParentList[i]->Type() == OBJECT_SUBNET)
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
		if (m_pChildList[i]->Type() == OBJECT_INTERFACE)
			((Interface *)m_pChildList[i])->updateZoneId();
	UnlockChildList();

   Modify();
   UnlockData();

   agentLock();
   delete_and_null(m_pAgentConnection);
   agentUnlock();

   pollerUnlock();
}


//
// Get number of interface objects and pointer to the last one
//

DWORD Node::getInterfaceCount(Interface **ppInterface)
{
   DWORD i, dwCount;

   LockChildList(FALSE);
   for(i = 0, dwCount = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i]->Type() == OBJECT_INTERFACE)
      {
         dwCount++;
         *ppInterface = (Interface *)m_pChildList[i];
      }
   UnlockChildList();
   return dwCount;
}


//
// Update cache for all DCI's
//

void Node::updateDciCache()
{
	lockDciAccess();
   for(int i = 0; i < m_dcObjects->size(); i++)
	{
		if (m_dcObjects->get(i)->getType() == DCO_TYPE_ITEM)
		{
			((DCItem *)m_dcObjects->get(i))->updateCacheSize();
		}
	}
	unlockDciAccess();
}


//
// Get routing table from node
//

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


//
// Get next hop for given destination address
//

BOOL Node::getNextHop(DWORD dwSrcAddr, DWORD dwDestAddr, DWORD *pdwNextHop,
                      DWORD *pdwIfIndex, BOOL *pbIsVPN)
{
   DWORD i;
   BOOL bResult = FALSE;

	// Check directly connected networks and VPN connectors
   LockChildList(FALSE);
   for(i = 0; i < m_dwChildCount; i++)
	{
      if (m_pChildList[i]->Type() == OBJECT_VPNCONNECTOR)
      {
         if (((VPNConnector *)m_pChildList[i])->IsRemoteAddr(dwDestAddr) &&
             ((VPNConnector *)m_pChildList[i])->IsLocalAddr(dwSrcAddr))
         {
            *pdwNextHop = ((VPNConnector *)m_pChildList[i])->GetPeerGatewayAddr();
            *pdwIfIndex = m_pChildList[i]->Id();
            *pbIsVPN = TRUE;
            bResult = TRUE;
            break;
         }
      }
      else if ((m_pChildList[i]->Type() == OBJECT_INTERFACE) && 
		         (m_pChildList[i]->IpAddr() != 0) &&
					(m_pChildList[i]->Status() == SEVERITY_NORMAL))
      {
			DWORD mask = ((Interface *)m_pChildList[i])->getIpNetMask();
			if ((dwDestAddr & mask) == (m_pChildList[i]->IpAddr() & mask))
			{
            *pdwNextHop = dwDestAddr;
				*pdwIfIndex = ((Interface *)m_pChildList[i])->getIfIndex();
            *pbIsVPN = FALSE;
            bResult = TRUE;
            break;
			}
		}
	}
   UnlockChildList();

   // Check routing table
   if (!bResult)
   {
      routingTableLock();
      if (m_pRoutingTable != NULL)
      {
         for(i = 0; i < (DWORD)m_pRoutingTable->iNumEntries; i++)
            if ((dwDestAddr & m_pRoutingTable->pRoutes[i].dwDestMask) == m_pRoutingTable->pRoutes[i].dwDestAddr)
            {
               *pdwNextHop = m_pRoutingTable->pRoutes[i].dwNextHop;
               *pdwIfIndex = m_pRoutingTable->pRoutes[i].dwIfIndex;
               *pbIsVPN = FALSE;
               bResult = TRUE;
               break;
            }
      }
      routingTableUnlock();
   }

   return bResult;
}


//
// Update cached routing table
//

void Node::updateRoutingTable()
{
   ROUTING_TABLE *pRT;

   pRT = getRoutingTable();
   if (pRT != NULL)
   {
      routingTableLock();
      DestroyRoutingTable(m_pRoutingTable);
      m_pRoutingTable = pRT;
      routingTableUnlock();
   }
   m_tLastRTUpdate = time(NULL);
   m_dwDynamicFlags &= ~NDF_QUEUED_FOR_ROUTE_POLL;
}


//
// Call SNMP Enumerate with node's SNMP parameters
//

DWORD Node::CallSnmpEnumerate(const TCHAR *pszRootOid, 
                              DWORD (* pHandler)(DWORD, SNMP_Variable *, SNMP_Transport *, void *),
                              void *pArg)
{
   if ((m_dwFlags & NF_IS_SNMP) && 
       (!(m_dwDynamicFlags & NDF_SNMP_UNREACHABLE)) &&
       (!(m_dwDynamicFlags & NDF_UNREACHABLE)))
	{
		SNMP_Transport *pTransport;
		DWORD dwResult;

		pTransport = createSnmpTransport();
		if (pTransport != NULL)
		{
			dwResult = SnmpEnumerate(m_snmpVersion, pTransport,
											 pszRootOid, pHandler, pArg, FALSE);
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


//
// Set proxy information for agent's connection
//

void Node::setAgentProxy(AgentConnection *pConn)
{
	DWORD proxyNode = m_dwProxyNode;
	
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
      Node *pNode;

      pNode = (Node *)FindObjectById(proxyNode);
      if (pNode != NULL)
      {
         pConn->setProxy(htonl(pNode->m_dwIpAddr), pNode->m_wAgentPort,
                         pNode->m_wAuthMethod, pNode->m_szSharedSecret);
      }
   }
}


//
// Prepare node object for deletion
//

void Node::PrepareForDeletion()
{
   // Prevent node from being queued for polling
   LockData();
   m_dwDynamicFlags |= NDF_POLLING_DISABLED;
   UnlockData();

   // Wait for all pending polls
   while(1)
   {
      LockData();
      if ((m_dwDynamicFlags & 
            (NDF_QUEUED_FOR_STATUS_POLL | NDF_QUEUED_FOR_CONFIG_POLL |
             NDF_QUEUED_FOR_DISCOVERY_POLL | NDF_QUEUED_FOR_ROUTE_POLL)) == 0)
      {
         UnlockData();
         break;
      }
      UnlockData();
      ThreadSleepMs(100);
   }
	Template::PrepareForDeletion();
}


//
// Check if specified SNMP variable set to specified value.
// If variable doesn't exist at all, will return FALSE
//

BOOL Node::CheckSNMPIntegerValue(SNMP_Transport *pTransport, const TCHAR *pszOID, int nValue)
{
   DWORD dwTemp;

   if (SnmpGet(m_snmpVersion, pTransport, pszOID, NULL, 0, &dwTemp, sizeof(DWORD), 0) == SNMP_ERR_SUCCESS)
      return (int)dwTemp == nValue;
   return FALSE;
}


//
// Check and update if needed interface names
//

void Node::checkInterfaceNames(InterfaceList *pIfList)
{
   // Cut interface names to MAX_OBJECT_NAME and check for unnamed interfaces
   for(int i = 0; i < pIfList->getSize(); i++)
   {
      pIfList->get(i)->szName[MAX_OBJECT_NAME - 1] = 0;
      if (pIfList->get(i)->szName[0] == 0)
         _sntprintf(pIfList->get(i)->szName, MAX_OBJECT_NAME, _T("%d"), pIfList->get(i)->dwIndex);
   }
}


//
// Get cluster object this node belongs to, if any
//

Cluster *Node::getMyCluster()
{
	DWORD i;
	Cluster *pCluster = NULL;

	LockParentList(FALSE);
	for(i = 0; i < m_dwParentCount; i++)
		if (m_pParentList[i]->Type() == OBJECT_CLUSTER)
		{
			pCluster = (Cluster *)m_pParentList[i];
			break;
		}
	UnlockParentList();
	return pCluster;
}


//
// Create SNMP transport
//

SNMP_Transport *Node::createSnmpTransport(WORD port)
{
	SNMP_Transport *pTransport = NULL;
	DWORD snmpProxy = m_dwSNMPProxy;

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
		((SNMP_UDPTransport *)pTransport)->createUDPTransport(NULL, htonl(m_dwIpAddr), (port != 0) ? port : m_wSNMPPort);
	}
	else
	{
		NetObj *pObject;

		pObject = FindObjectById(snmpProxy);
		if (pObject != NULL)
		{
			if (pObject->Type() == OBJECT_NODE)
			{
				AgentConnection *pConn;

				pConn = ((Node *)pObject)->createAgentConnection();
				if (pConn != NULL)
				{
					pTransport = new SNMP_ProxyTransport(pConn, m_dwIpAddr, (port != 0) ? port : m_wSNMPPort);
				}
			}
		}
	}

	// Set security
	if (pTransport != NULL)
	{
		LockData();
		pTransport->setSecurityContext(new SNMP_SecurityContext(m_snmpSecurity));
		pTransport->setSnmpVersion(m_snmpVersion);
		UnlockData();
	}
	return pTransport;
}


//
// Get SNMP security context
// ATTENTION: This method returns new copy of security context
// which must be destroyed by the caller
//

SNMP_SecurityContext *Node::getSnmpSecurityContext()
{
	LockData();
	SNMP_SecurityContext *ctx = new SNMP_SecurityContext(m_snmpSecurity);
	UnlockData();
	return ctx;
}


//
// Resolve node's name
//

BOOL Node::resolveName(BOOL useOnlyDNS)
{
	BOOL bSuccess = FALSE;
	HOSTENT *hs;
	DWORD i, dwAddr;
	TCHAR szBuffer[256];

	DbgPrintf(4, _T("Resolving name for node %d [%s]..."), m_dwId, m_szName);

	// Try to resolve primary IP
	dwAddr = htonl(m_dwIpAddr);
	hs = gethostbyaddr((const char *)&dwAddr, 4, AF_INET);
	if (hs != NULL)
	{
#ifdef UNICODE
		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, hs->h_name, -1, m_szName, MAX_OBJECT_NAME);
		m_szName[MAX_OBJECT_NAME - 1] = 0;
#else
		nx_strncpy(m_szName, hs->h_name, MAX_OBJECT_NAME);
#endif
		bSuccess = TRUE;
	}
	else
	{
		// Try to resolve each interface's IP address
		LockChildList(FALSE);
		for(i = 0; i < m_dwChildCount; i++)
		{
			if (m_pChildList[i]->Type() == OBJECT_INTERFACE)
			{
				dwAddr = htonl(m_pChildList[i]->IpAddr());
				if (dwAddr != 0)
				{
					hs = gethostbyaddr((const char *)&dwAddr, 4, AF_INET);
					if (hs != NULL)
					{
#ifdef UNICODE
						MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, hs->h_name, -1, m_szName, MAX_OBJECT_NAME);
						m_szName[MAX_OBJECT_NAME - 1] = 0;
#else
						nx_strncpy(m_szName, hs->h_name, MAX_OBJECT_NAME);
#endif
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
			DbgPrintf(4, _T("Resolving name for node %d [%s] via agent..."), m_dwId, m_szName);
			if (GetItemFromAgent(_T("System.Hostname"), 256, szBuffer) == DCE_SUCCESS)
			{
				StrStrip(szBuffer);
				if (szBuffer[0] != 0)
				{
					nx_strncpy(m_szName, szBuffer, MAX_OBJECT_NAME);
					bSuccess = TRUE;
				}
			}
		}

		// Try to get hostname from SNMP if other methods fails
		if (!(bSuccess || useOnlyDNS))
		{
			DbgPrintf(4, _T("Resolving name for node %d [%s] via SNMP..."), m_dwId, m_szName);
			if (getItemFromSNMP(0, _T(".1.3.6.1.2.1.1.5.0"), 256, szBuffer, SNMP_RAWTYPE_NONE) == DCE_SUCCESS)
			{
				StrStrip(szBuffer);
				if (szBuffer[0] != 0)
				{
					nx_strncpy(m_szName, szBuffer, MAX_OBJECT_NAME);
					bSuccess = TRUE;
				}
			}
		}
	}

	if (bSuccess)
		DbgPrintf(4, _T("Name for node %d was resolved to %s"), m_dwId, m_szName);
	else
		DbgPrintf(4, _T("Name for node %d was not resolved"), m_dwId, m_szName);
	return bSuccess;
}


//
// Send list of system DCIs
//

DWORD Node::getPerfTabDCIList(CSCPMessage *pMsg)
{
	lockDciAccess();

	DWORD dwId = VID_SYSDCI_LIST_BASE, dwCount = 0;
   for(int i = 0; i < m_dcObjects->size(); i++)
	{
		DCObject *object = m_dcObjects->get(i);
		if (object->getPerfTabSettings() != NULL)
		{
			pMsg->SetVariable(dwId++, object->getId());
			pMsg->SetVariable(dwId++, object->getDescription());
			pMsg->SetVariable(dwId++, (WORD)object->getStatus());
			pMsg->SetVariable(dwId++, object->getPerfTabSettings());
			pMsg->SetVariable(dwId++, (WORD)object->getType());
			dwId += 5;
			dwCount++;
		}
	}
   pMsg->SetVariable(VID_NUM_ITEMS, dwCount);

	unlockDciAccess();
   return RCC_SUCCESS;
}


//
// Get current layer 2 topology (as dynamically created list which should be destroyed by caller)
// Will return NULL if there are no topology information or it is expired
//

nxmap_ObjList *Node::GetL2Topology()
{
	nxmap_ObjList *pResult;
	DWORD dwExpTime;

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


//
// Rebuild layer 2 topology and return it as dynamically reated list which should be destroyed by caller
//

nxmap_ObjList *Node::BuildL2Topology(DWORD *pdwStatus)
{
	nxmap_ObjList *pResult;
	int nDepth = ConfigReadInt(_T("TopologyDiscoveryRadius"), 5);

	MutexLock(m_mutexTopoAccess);
	if ((m_linkLayerNeighbors != NULL) && (m_linkLayerNeighbors->getSize() > 0))
	{
		MutexUnlock(m_mutexTopoAccess);

		pResult = new nxmap_ObjList;
		::BuildL2Topology(*pResult, this, nDepth);

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


//
// Topology poller
//

void Node::topologyPoll(ClientSession *pSession, DWORD dwRqId, int nPoller)
{
	pollerLock();
   m_pPollRequestor = pSession;

   SendPollerMsg(dwRqId, _T("Starting topology poll for node %s\r\n"), m_szName);
	DbgPrintf(4, _T("Started topology poll for node %s [%d]"), m_szName, m_dwId);

	ForwardingDatabase *fdb = GetSwitchForwardingDatabase(this);
	MutexLock(m_mutexTopoAccess);
	if (m_fdb != NULL)
		m_fdb->decRefCount();
	m_fdb = fdb;
	MutexUnlock(m_mutexTopoAccess);
	if (fdb != NULL)
	{
		DbgPrintf(4, _T("Switch forwarding database retrieved for node %s [%d]"), m_szName, m_dwId);
	   SendPollerMsg(dwRqId, POLLER_INFO _T("Switch forwarding database retrieved\r\n"));
	}
	else
	{
		DbgPrintf(4, _T("Failed to get switch forwarding database from node %s [%d]"), m_szName, m_dwId);
	   SendPollerMsg(dwRqId, POLLER_WARNING _T("Failed to get switch forwarding database\r\n"));
	}

	LinkLayerNeighbors *nbs = BuildLinkLayerNeighborList(this);
	if (nbs != NULL)
	{
		SendPollerMsg(dwRqId, POLLER_INFO _T("Link layer topology retrieved (%d connections found)\r\n"), nbs->getSize());
		DbgPrintf(4, _T("Link layer topology retrieved for node %s [%d] (%d connections found)"), m_szName, (int)m_dwId, nbs->getSize());

		MutexLock(m_mutexTopoAccess);
		if (m_linkLayerNeighbors != NULL)
			m_linkLayerNeighbors->decRefCount();
		m_linkLayerNeighbors = nbs;
		MutexUnlock(m_mutexTopoAccess);

		// Walk through interfaces and update peers
	   SendPollerMsg(dwRqId, _T("Updating peer information on interfaces\r\n"));
		for(int i = 0; i < nbs->getSize(); i++)
		{
			LL_NEIGHBOR_INFO *ni = nbs->getConnection(i);
			NetObj *object = FindObjectById(ni->objectId);
			if ((object != NULL) && (object->Type() == OBJECT_NODE))
			{
				DbgPrintf(5, _T("Node::topologyPoll(%s [%d]): found peer node %s [%d], localIfIndex=%d remoteIfIndex=%d"),
				          m_szName, m_dwId, object->Name(), object->Id(), ni->ifLocal, ni->ifRemote);
				Interface *ifLocal = findInterface(ni->ifLocal, INADDR_ANY);
				Interface *ifRemote = ((Node *)object)->findInterface(ni->ifRemote, INADDR_ANY);
				DbgPrintf(5, _T("Node::topologyPoll(%s [%d]): localIfObject=%s remoteIfObject=%s"), m_szName, m_dwId, 
				          (ifLocal != NULL) ? ifLocal->Name() : _T("(null)"),
				          (ifRemote != NULL) ? ifRemote->Name() : _T("(null)"));
				if ((ifLocal != NULL) && (ifRemote != NULL))
				{
					ifLocal->setPeer(ni->objectId, ifRemote->Id());
					ifRemote->setPeer(m_dwId, ifLocal->Id());
					SendPollerMsg(dwRqId, _T("   Local interface %s linked to remote interface %s:%s\r\n"),
					              ifLocal->Name(), object->Name(), ifRemote->Name());
					DbgPrintf(5, _T("Local interface %s:%s linked to remote interface %s:%s"),
					          m_szName, ifLocal->Name(), object->Name(), ifRemote->Name());
				}
			}
		}
	   SendPollerMsg(dwRqId, _T("Link layer topology processed\r\n"));
		DbgPrintf(4, _T("Link layer topology processed for node %s [%d]"), m_szName, m_dwId);
	}
	else
	{
	   SendPollerMsg(dwRqId, POLLER_ERROR _T("Link layer topology retrieved\r\n"));
	}

	if (m_driver != NULL)
	{
		SNMP_Transport *snmp = createSnmpTransport();
		if (snmp != NULL)
		{
			VlanList *vlanList = m_driver->getVlans(snmp, &m_customAttributes);
			delete snmp;

			MutexLock(m_mutexTopoAccess);
			if (vlanList != NULL)
			{
				resolveVlanPorts(vlanList);
				SendPollerMsg(dwRqId, POLLER_INFO _T("VLAN list successfully retrieved from node\r\n"));
				DbgPrintf(4, _T("VLAN list retrieved from node %s [%d]"), m_szName, m_dwId);
				if (m_vlans != NULL)
					m_vlans->decRefCount();
				m_vlans = vlanList;
			}
			else
			{
				SendPollerMsg(dwRqId, POLLER_WARNING _T("Cannot get VLAN list\r\n"));
				DbgPrintf(4, _T("Cannot retrieve VLAN list from node %s [%d]"), m_szName, m_dwId);
				if (m_vlans != NULL)
					m_vlans->decRefCount();
				m_vlans = NULL;
			}
			MutexUnlock(m_mutexTopoAccess);

			LockData();
			DWORD oldFlags = m_dwFlags;
			if (vlanList != NULL)
				m_dwFlags |= NF_HAS_VLANS;
			else
				m_dwFlags &= ~NF_HAS_VLANS;
			if (oldFlags != m_dwFlags)
				Modify();
			UnlockData();
		}
	}

   SendPollerMsg(dwRqId, _T("Finished topology poll for node %s\r\n"), m_szName);

	m_tLastTopologyPoll = time(NULL);
   m_dwDynamicFlags &= ~NDF_QUEUED_FOR_TOPOLOGY_POLL;
	pollerUnlock();

	DbgPrintf(4, _T("Finished topology poll for node %s [%d]"), m_szName, m_dwId);
}


//
// Update host connections using forwarding database information
//

void Node::addHostConnections(LinkLayerNeighbors *nbs)
{
	ForwardingDatabase *fdb = getSwitchForwardingDatabase();
	if (fdb == NULL)
		return;

	LockChildList(FALSE);
	for(int i = 0; i < (int)m_dwChildCount; i++)
	{
		if (m_pChildList[i]->Type() != OBJECT_INTERFACE)
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
					info.objectId = peerNode->Id();
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


//
// Add existing connections to link layer neighbours table
//

void Node::addExistingConnections(LinkLayerNeighbors *nbs)
{
	LockChildList(FALSE);
	for(int i = 0; i < (int)m_dwChildCount; i++)
	{
		if (m_pChildList[i]->Type() != OBJECT_INTERFACE)
			continue;

		Interface *ifLocal = (Interface *)m_pChildList[i];
		if ((ifLocal->getPeerNodeId() != 0) && (ifLocal->getPeerInterfaceId() != 0))
		{
			LL_NEIGHBOR_INFO info;

			info.ifLocal = ifLocal->getIfIndex();
			info.ifRemote = ifLocal->getPeerInterfaceId();
			info.objectId = ifLocal->getPeerNodeId();
			info.isPtToPt = true;
			info.protocol = LL_PROTO_FDB;
			nbs->addConnection(&info);
		}
	}
	UnlockChildList();
}


//
// Resolve port indexes in VLAN list
//

void Node::resolveVlanPorts(VlanList *vlanList)
{
	for(int i = 0; i < vlanList->getSize(); i++)
	{
		VlanInfo *vlan = vlanList->get(i);
		vlan->prepareForResolve();
		for(int j = 0; j < vlan->getNumPorts(); j++)
		{
			DWORD portId = vlan->getPorts()[j];
			Interface *iface = NULL;
			switch(vlan->getPortReferenceMode())
			{
				case VLAN_PRM_IFINDEX:
					iface = findInterface(portId, INADDR_ANY);
					break;
				case VLAN_PRM_SLOTPORT:
					iface = findInterfaceBySlotAndPort(portId >> 16, portId & 0xFFFF);
					break;
			}
			if (iface != NULL)
				vlan->resolvePort(j, (iface->getSlotNumber() << 16) | iface->getPortNumber(), iface->getIfIndex(), iface->Id());
		}
	}
}


//
// Check subnet bindings
//

void Node::checkSubnetBinding(InterfaceList *pIfList)
{
	Subnet *pSubnet;
	Interface *pInterface;
	Cluster *pCluster;
	NetObj **ppUnlinkList;
	int i, j, count;
	BOOL isSync;

	pCluster = getMyCluster();

	// Check if we have subnet bindings for all interfaces
	DbgPrintf(5, _T("Checking subnet bindings for node %s [%d]"), m_szName, m_dwId);
	for(i = 0; i < pIfList->getSize(); i++)
	{
		NX_INTERFACE_INFO *iface = pIfList->get(i);
		if (iface->dwIpAddr != 0)
		{
			pInterface = findInterface(iface->dwIndex, iface->dwIpAddr);
			if (pInterface == NULL)
			{
				nxlog_write(MSG_INTERNAL_ERROR, EVENTLOG_WARNING_TYPE, "s", _T("Cannot find interface object in Node::CheckSubnetBinding()"));
				break;	// Something goes really wrong
			}
			if (pInterface->isExcludedFromTopology())
				continue;

			// Is cluster interconnect interface?
			isSync = (pCluster != NULL) ? pCluster->isSyncAddr(pInterface->IpAddr()) : FALSE;

			pSubnet = FindSubnetForNode(m_zoneId, iface->dwIpAddr);
			if (pSubnet != NULL)
			{
				if (isSync)
				{
					pSubnet = NULL;	// No further checks on this subnet
				}
				else
				{
					if (pSubnet->isSyntheticMask())
					{
						DbgPrintf(4, _T("Setting correct netmask for subnet %s [%d] from node %s [%d]"),
									 pSubnet->Name(), pSubnet->Id(), m_szName, m_dwId);
						pSubnet->setCorrectMask(pInterface->IpAddr() & pInterface->getIpNetMask(), pInterface->getIpNetMask());
					}

					// Check if node is linked to this subnet
					if (!pSubnet->IsChild(m_dwId))
					{
						DbgPrintf(4, _T("Restored link between subnet %s [%d] and node %s [%d]"),
									 pSubnet->Name(), pSubnet->Id(), m_szName, m_dwId);
						pSubnet->AddNode(this);
					}
				}
			}
			else if (!isSync)
			{
				// Create subnet
				pSubnet = new Subnet(iface->dwIpAddr & iface->dwIpNetMask, iface->dwIpNetMask, m_zoneId, FALSE);
				NetObjInsert(pSubnet, TRUE);
				if (g_dwFlags & AF_ENABLE_ZONING)
				{
					Zone *zone = FindZoneByGUID(m_zoneId);
					if (zone != NULL)
					{
						zone->addSubnet(pSubnet);
					}
					else
					{
						DbgPrintf(1, _T("Inconsistent configuration - zone %d does not exist"), (int)m_zoneId);
					}
				}
				else
				{
					g_pEntireNet->AddSubnet(pSubnet);
				}
				pSubnet->AddNode(this);
				DbgPrintf(4, _T("Node::CheckSubnetBinding(): Creating new subnet %s [%d] for node %s [%d]"),
				          pSubnet->Name(), pSubnet->Id(), m_szName, m_dwId);
			}

			// Check if subnet mask is correct on interface
			if ((pSubnet != NULL) && (pSubnet->getIpNetMask() != pInterface->getIpNetMask()))
			{
				PostEvent(EVENT_INCORRECT_NETMASK, m_dwId, "idsaa", pInterface->Id(),
							 pInterface->getIfIndex(), pInterface->Name(),
							 pInterface->getIpNetMask(), pSubnet->getIpNetMask());
			}
		}
	}
	
	// Check if we have incorrect subnets as parents
	LockParentList(FALSE);
	LockChildList(FALSE);
	ppUnlinkList = (NetObj **)malloc(sizeof(NetObj *) * m_dwParentCount);
	for(i = 0, count = 0; i < (int)m_dwParentCount; i++)
	{
		if (m_pParentList[i]->Type() == OBJECT_SUBNET)
		{
			pSubnet = (Subnet *)m_pParentList[i];
			for(j = 0; j < (int)m_dwChildCount; j++)
			{
				if (m_pChildList[j]->Type() == OBJECT_INTERFACE)
				{
					if (((Interface *)m_pChildList[j])->isExcludedFromTopology())
						continue;

					if (pSubnet->IpAddr() == (m_pChildList[j]->IpAddr() & pSubnet->getIpNetMask()))
					{
						if (pCluster != NULL)
						{
							if (pCluster->isSyncAddr(m_pChildList[j]->IpAddr()))
							{
								j = (int)m_dwChildCount;	// Cause to unbind from this subnet
							}
						}
						break;
					}
				}
			}
			if (j == (int)m_dwChildCount)
			{
				DbgPrintf(4, _T("Node::CheckSubnetBinding(): Subnet %s [%d] is incorrect for node %s [%d]"),
							 pSubnet->Name(), pSubnet->Id(), m_szName, m_dwId);
				ppUnlinkList[count++] = pSubnet;	
			}
		}
	}
	UnlockChildList();
	UnlockParentList();
	
	// Unlink for incorrect subnet objects
	for(i = 0; i < count; i++)
	{
		ppUnlinkList[i]->DeleteChild(this);
		DeleteParent(ppUnlinkList[i]);
	}
	safe_free(ppUnlinkList);
}


//
// Update interface names
//

void Node::updateInterfaceNames(ClientSession *pSession, DWORD dwRqId)
{
	InterfaceList *pIfList;
	DWORD i;
	int j;
	
   pollerLock();
   m_pPollRequestor = pSession;
   SendPollerMsg(dwRqId, _T("Starting interface names poll for node %s\r\n"), m_szName);
   DbgPrintf(4, _T("Starting interface names poll for node %s (ID: %d)"), m_szName, m_dwId);

   // Retrieve interface list
   pIfList = getInterfaceList();
   if (pIfList != NULL)
   {
      // Check names of existing interfaces
      for(j = 0; j < pIfList->getSize(); j++)
      {
         LockChildList(FALSE);
         for(i = 0; i < m_dwChildCount; i++)
         {
            if (m_pChildList[i]->Type() == OBJECT_INTERFACE)
            {
               Interface *pInterface = (Interface *)m_pChildList[i];

               if (pIfList->get(j)->dwIndex == pInterface->getIfIndex())
               {
						SendPollerMsg(dwRqId, _T("   Checking interface %d (%s)\r\n"), pInterface->getIfIndex(), pInterface->Name());
                  if (_tcscmp(pIfList->get(j)->szName, pInterface->Name()))
                  {
                     pInterface->setName(pIfList->get(j)->szName);
							SendPollerMsg(dwRqId, POLLER_WARNING _T("   Name of interface %d changed to %s\r\n"), pInterface->getIfIndex(), pIfList->get(j)->szName);
                  }
                  if (_tcscmp(pIfList->get(j)->szDescription, pInterface->getDescription()))
                  {
                     pInterface->setDescription(pIfList->get(j)->szDescription);
							SendPollerMsg(dwRqId, POLLER_WARNING _T("   Description of interface %d changed to %s\r\n"), pInterface->getIfIndex(), pIfList->get(j)->szDescription);
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
      SendPollerMsg(dwRqId, POLLER_ERROR _T("   Unable to get interface list from node\r\n"));
   }

   // Finish poll
	SendPollerMsg(dwRqId, _T("Finished interface names poll for node %s\r\n"), m_szName);
   pollerUnlock();
   DbgPrintf(4, _T("Finished interface names poll for node %s (ID: %d)"), m_szName, m_dwId);
}


//
// Process new DCI value
//

void Node::processNewDCValue(DCObject *dco, time_t currTime, void *value)
{
	lockDciAccess();
	dco->processNewValue(currTime, value);
	unlockDciAccess();
}


//
// Get list of parent objects for NXSL script
//

NXSL_Array *Node::getParentsForNXSL()
{
	NXSL_Array *parents = new NXSL_Array;
	int index = 0;

	LockParentList(FALSE);
	for(DWORD i = 0; i < m_dwParentCount; i++)
	{
		if (((m_pParentList[i]->Type() == OBJECT_CONTAINER) ||
		     (m_pParentList[i]->Type() == OBJECT_CLUSTER) ||
			  (m_pParentList[i]->Type() == OBJECT_SUBNET) ||
			  (m_pParentList[i]->Type() == OBJECT_SERVICEROOT)) &&
		    m_pParentList[i]->IsTrustedNode(m_dwId))
		{
			parents->set(index++, new NXSL_Value(new NXSL_Object(&g_nxslNetObjClass, m_pParentList[i])));
		}
	}
	UnlockParentList();

	return parents;
}


//
// Get list of interface objects for NXSL script
//

NXSL_Array *Node::getInterfacesForNXSL()
{
	NXSL_Array *ifaces = new NXSL_Array;
	int index = 0;

	LockChildList(FALSE);
	for(DWORD i = 0; i < m_dwChildCount; i++)
	{
		if (m_pChildList[i]->Type() == OBJECT_INTERFACE)
		{
			ifaces->set(index++, new NXSL_Value(new NXSL_Object(&g_nxslInterfaceClass, m_pChildList[i])));
		}
	}
	UnlockChildList();

	return ifaces;
}


//
// Get switch forwarding database
//

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


//
// Get link layer neighbors
//

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


//
// Get link layer neighbors
//

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


//
// Substitute % macros in given text with actual values
//

TCHAR *Node::expandText(const TCHAR *pszTemplate)
{
   const TCHAR *pCurr;
   DWORD dwPos, dwSize;
   TCHAR *pText, scriptName[256];
	int i;

   dwSize = (DWORD)_tcslen(pszTemplate) + 1;
   pText = (TCHAR *)malloc(dwSize * sizeof(TCHAR));
   for(pCurr = pszTemplate, dwPos = 0; *pCurr != 0; pCurr++)
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
                  dwSize += (DWORD)_tcslen(m_szName);
                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
                  _tcscpy(&pText[dwPos], m_szName);
                  dwPos += (DWORD)_tcslen(m_szName);
                  break;
               case 'a':   // IP address of the node
                  dwSize += 16;
                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
						IpToStr(m_dwIpAddr, &pText[dwPos]);
                  dwPos = (DWORD)_tcslen(pText);
                  break;
               case 'i':   // Node identifier
                  dwSize += 10;
                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
                  _sntprintf(&pText[dwPos], 11, _T("0x%08X"), m_dwId);
                  dwPos = (DWORD)_tcslen(pText);
                  break;
               case 'v':   // NetXMS server version
                  dwSize += (DWORD)_tcslen(NETXMS_VERSION_STRING);
                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
                  _tcscpy(&pText[dwPos], NETXMS_VERSION_STRING);
                  dwPos += (DWORD)_tcslen(NETXMS_VERSION_STRING);
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
							NXSL_Program *script;
							NXSL_ServerEnv *pEnv;

							scriptName[i] = 0;
							StrStrip(scriptName);

							g_pScriptLibrary->lock();
							script = g_pScriptLibrary->findScript(scriptName);
							if (script != NULL)
							{
								pEnv = new NXSL_ServerEnv;
								script->setGlobalVariable(_T("$node"), new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, this)));

								if (script->run(pEnv) == 0)
								{
									NXSL_Value *result = script->getResult();
									if (result != NULL)
									{
										const TCHAR *temp = result->getValueAsCString();
										if (temp != NULL)
										{
											dwSize += (DWORD)_tcslen(temp);
											pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
											_tcscpy(&pText[dwPos], temp);
											dwPos += (DWORD)_tcslen(temp);
											DbgPrintf(4, _T("Node::expandText(\"%s\"): Script %s executed successfully"),
														 pszTemplate, scriptName);
										}
									}
								}
								else
								{
									DbgPrintf(4, _T("Node::expandText(\"%s\"): Script %s execution error: %s"),
												 pszTemplate, scriptName, script->getErrorText());
									PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", scriptName, script->getErrorText(), 0);
								}
							}
							else
							{
								DbgPrintf(4, _T("Node::expandText(\"%s\"): Cannot find script %s"), pszTemplate, scriptName);
							}
							g_pScriptLibrary->unlock();
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
							const TCHAR *temp = GetCustomAttribute(scriptName);
							if (temp != NULL)
							{
								dwSize += (DWORD)_tcslen(temp);
								pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
								_tcscpy(&pText[dwPos], temp);
								dwPos += (DWORD)_tcslen(temp);
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


//
// Check and update last agent trap ID
//

bool Node::checkAgentTrapId(QWORD trapId)
{
	LockData();
	bool valid = (trapId > m_lastAgentTrapId);
	if (valid)
		m_lastAgentTrapId = trapId;
	UnlockData();
	return valid;
}


//
// Get node's physical components
//

ComponentTree *Node::getComponents()
{
	LockData();
	ComponentTree *components = m_components;
	if (components != NULL)
		components->incRefCount();
	UnlockData();
	return components;
}
