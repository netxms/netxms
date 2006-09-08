/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: node.cpp
**
**/

#include "nxcore.h"


//
// Node class default constructor
//

Node::Node()
     :Template()
{
   m_dwFlags = 0;
   m_dwDynamicFlags = 0;
   m_dwZoneGUID = 0;
   m_dwNodeType = NODE_TYPE_GENERIC;
   m_wAgentPort = AGENT_LISTEN_PORT;
   m_wAuthMethod = AUTH_NONE;
   m_szSharedSecret[0] = 0;
   m_iStatusPollType = POLL_ICMP_PING;
   m_iSNMPVersion = SNMP_VERSION_1;
   m_wSNMPPort = SNMP_DEFAULT_PORT;
   ConfigReadStr("DefaultCommunityString", m_szCommunityString,
                 MAX_COMMUNITY_LENGTH, "public");
   m_szObjectId[0] = 0;
   m_tLastDiscoveryPoll = 0;
   m_tLastStatusPoll = 0;
   m_tLastConfigurationPoll = 0;
   m_tLastRTUpdate = 0;
   m_hPollerMutex = MutexCreate();
   m_hAgentAccessMutex = MutexCreate();
   m_mutexRTAccess = MutexCreate();
   m_pAgentConnection = NULL;
   m_szAgentVersion[0] = 0;
   m_szPlatformName[0] = 0;
   m_dwNumParams = 0;
   m_pParamList = NULL;
   m_dwPollerNode = 0;
   m_dwProxyNode = 0;
   memset(m_qwLastEvents, 0, sizeof(QWORD) * MAX_LAST_EVENTS);
   m_pRoutingTable = NULL;
   m_tFailTimeSNMP = 0;
   m_tFailTimeAgent = 0;
}


//
// Constructor for new node object
//

Node::Node(DWORD dwAddr, DWORD dwFlags, DWORD dwProxyNode, DWORD dwZone)
     :Template()
{
   m_dwIpAddr = dwAddr;
   m_dwFlags = dwFlags;
   m_dwDynamicFlags = 0;
   m_dwZoneGUID = dwZone;
   m_dwNodeType = NODE_TYPE_GENERIC;
   m_wAgentPort = AGENT_LISTEN_PORT;
   m_wAuthMethod = AUTH_NONE;
   m_szSharedSecret[0] = 0;
   m_iStatusPollType = POLL_ICMP_PING;
   m_iSNMPVersion = SNMP_VERSION_1;
   m_wSNMPPort = SNMP_DEFAULT_PORT;
   ConfigReadStr("DefaultCommunityString", m_szCommunityString,
                 MAX_COMMUNITY_LENGTH, "public");
   IpToStr(dwAddr, m_szName);    // Make default name from IP address
   m_szObjectId[0] = 0;
   m_tLastDiscoveryPoll = 0;
   m_tLastStatusPoll = 0;
   m_tLastConfigurationPoll = 0;
   m_tLastRTUpdate = 0;
   m_hPollerMutex = MutexCreate();
   m_hAgentAccessMutex = MutexCreate();
   m_mutexRTAccess = MutexCreate();
   m_pAgentConnection = NULL;
   m_szAgentVersion[0] = 0;
   m_szPlatformName[0] = 0;
   m_dwNumParams = 0;
   m_pParamList = NULL;
   m_dwPollerNode = 0;
   m_dwProxyNode = dwProxyNode;
   memset(m_qwLastEvents, 0, sizeof(QWORD) * MAX_LAST_EVENTS);
   m_bIsHidden = TRUE;
   m_pRoutingTable = NULL;
   m_tFailTimeSNMP = 0;
   m_tFailTimeAgent = 0;
}


//
// Node destructor
//

Node::~Node()
{
   MutexDestroy(m_hPollerMutex);
   MutexDestroy(m_hAgentAccessMutex);
   MutexDestroy(m_mutexRTAccess);
   if (m_pAgentConnection != NULL)
      delete m_pAgentConnection;
   safe_free(m_pParamList);
   DestroyRoutingTable(m_pRoutingTable);
}


//
// Create object from database data
//

BOOL Node::CreateFromDB(DWORD dwId)
{
   char szQuery[512];
   DB_RESULT hResult;
   int i, iNumRows;
   DWORD dwSubnetId;
   NetObj *pObject;
   BOOL bResult = FALSE;

   m_dwId = dwId;

   if (!LoadCommonProperties())
   {
      DbgPrintf(AF_DEBUG_OBJECTS, "Cannot load common properties for node object %d", dwId);
      return FALSE;
   }

   _sntprintf(szQuery, 512, "SELECT primary_ip,node_flags,"
                            "snmp_version,auth_method,secret,"
                            "agent_port,status_poll_type,community,snmp_oid,"
                            "description,node_type,agent_version,"
                            "platform_name,poller_node_id,zone_guid,"
                            "proxy_node FROM nodes WHERE id=%d", dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult == 0)
      return FALSE;     // Query failed

   if (DBGetNumRows(hResult) == 0)
   {
      DBFreeResult(hResult);
      DbgPrintf(AF_DEBUG_OBJECTS, "Missing record in \"nodes\" table for node object %d", dwId);
      return FALSE;
   }

   m_dwIpAddr = DBGetFieldIPAddr(hResult, 0, 0);
   m_dwFlags = DBGetFieldULong(hResult, 0, 1);
   m_iSNMPVersion = DBGetFieldLong(hResult, 0, 2);
   m_wAuthMethod = (WORD)DBGetFieldLong(hResult, 0, 3);
   nx_strncpy(m_szSharedSecret, DBGetField(hResult, 0, 4), MAX_SECRET_LENGTH);
   m_wAgentPort = (WORD)DBGetFieldLong(hResult, 0, 5);
   m_iStatusPollType = DBGetFieldLong(hResult, 0, 6);
   nx_strncpy(m_szCommunityString, DBGetField(hResult, 0, 7), MAX_COMMUNITY_LENGTH);
   nx_strncpy(m_szObjectId, DBGetField(hResult, 0, 8), MAX_OID_LEN * 4);
   m_pszDescription = _tcsdup(CHECK_NULL_EX(DBGetField(hResult, 0, 9)));
   DecodeSQLString(m_pszDescription);
   m_dwNodeType = DBGetFieldULong(hResult, 0, 10);
   nx_strncpy(m_szAgentVersion, CHECK_NULL_EX(DBGetField(hResult, 0, 11)), MAX_AGENT_VERSION_LEN);
   DecodeSQLString(m_szAgentVersion);
   nx_strncpy(m_szPlatformName, CHECK_NULL_EX(DBGetField(hResult, 0, 12)), MAX_PLATFORM_NAME_LEN);
   DecodeSQLString(m_szPlatformName);
   m_dwPollerNode = DBGetFieldULong(hResult, 0, 13);
   m_dwZoneGUID = DBGetFieldULong(hResult, 0, 14);
   m_dwProxyNode = DBGetFieldULong(hResult, 0, 15);

   DBFreeResult(hResult);

   if (!m_bIsDeleted)
   {
      // Link node to subnets
      sprintf(szQuery, "SELECT subnet_id FROM nsmap WHERE node_id=%d", dwId);
      hResult = DBSelect(g_hCoreDB, szQuery);
      if (hResult == NULL)
         return FALSE;     // Query failed

      iNumRows = DBGetNumRows(hResult);
      if (iNumRows == 0)
      {
         DBFreeResult(hResult);
         DbgPrintf(AF_DEBUG_OBJECTS, "Unbound node object %d (%s)", dwId, m_szName);
         return FALSE;     // No parents - it shouldn't happen if database isn't corrupted
      }

      for(i = 0; i < iNumRows; i++)
      {
         dwSubnetId = DBGetFieldULong(hResult, i, 0);
         pObject = FindObjectById(dwSubnetId);
         if (pObject == NULL)
         {
            WriteLog(MSG_INVALID_SUBNET_ID, EVENTLOG_ERROR_TYPE, "dd", dwId, dwSubnetId);
            break;
         }
         else if (pObject->Type() != OBJECT_SUBNET)
         {
            WriteLog(MSG_SUBNET_NOT_SUBNET, EVENTLOG_ERROR_TYPE, "dd", dwId, dwSubnetId);
            break;
         }
         else
         {
            pObject->AddChild(this);
            AddParent(pObject);
            bResult = TRUE;
         }
      }

      DBFreeResult(hResult);
      LoadItemsFromDB();
      LoadACLFromDB();

      // Walk through all items in the node and load appropriate thresholds
      for(i = 0; i < (int)m_dwNumItems; i++)
         if (!m_ppItems[i]->LoadThresholdsFromDB())
         {
            DbgPrintf(AF_DEBUG_OBJECTS, "Cannot load thresholds for DCI %d of node %d (%s)",
                      m_ppItems[i]->Id(), dwId, m_szName);
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
   TCHAR *pszEscDescr, *pszEscVersion, *pszEscPlatform;
   TCHAR szQuery[4096], szIpAddr[16];
   DB_RESULT hResult;
   BOOL bNewObject = TRUE;
   BOOL bResult;

   // Lock object's access
   LockData();

   SaveCommonProperties(hdb);

   // Check for object's existence in database
   sprintf(szQuery, "SELECT id FROM nodes WHERE id=%d", m_dwId);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != 0)
   {
      if (DBGetNumRows(hResult) > 0)
         bNewObject = FALSE;
      DBFreeResult(hResult);
   }

   // Form and execute INSERT or UPDATE query
   pszEscDescr = EncodeSQLString(CHECK_NULL_EX(m_pszDescription));
   pszEscVersion = EncodeSQLString(m_szAgentVersion);
   pszEscPlatform = EncodeSQLString(m_szPlatformName);
   if (bNewObject)
      snprintf(szQuery, 4096,
               "INSERT INTO nodes (id,primary_ip,"
               "node_flags,snmp_version,community,status_poll_type,"
               "agent_port,auth_method,secret,snmp_oid,proxy_node,"
               "description,node_type,agent_version,platform_name,"
               "poller_node_id,zone_guid) VALUES (%d,'%s',%d,%d,'%s',%d,%d,%d,"
               "'%s','%s',%d,'%s',%d,'%s','%s',%d,%d)",
               m_dwId, IpToStr(m_dwIpAddr, szIpAddr), m_dwFlags,
               m_iSNMPVersion, m_szCommunityString, m_iStatusPollType,
               m_wAgentPort, m_wAuthMethod, m_szSharedSecret, m_szObjectId,
               m_dwProxyNode, pszEscDescr, m_dwNodeType, pszEscVersion,
               pszEscPlatform, m_dwPollerNode, m_dwZoneGUID);
   else
      snprintf(szQuery, 4096,
               "UPDATE nodes SET primary_ip='%s',"
               "node_flags=%d,snmp_version=%d,community='%s',"
               "status_poll_type=%d,agent_port=%d,auth_method=%d,secret='%s',"
               "snmp_oid='%s',description='%s',node_type=%d,"
               "agent_version='%s',platform_name='%s',poller_node_id=%d,"
               "zone_guid=%d,proxy_node=%d WHERE id=%d",
               IpToStr(m_dwIpAddr, szIpAddr), 
               m_dwFlags, m_iSNMPVersion, m_szCommunityString,
               m_iStatusPollType, m_wAgentPort, m_wAuthMethod, m_szSharedSecret, 
               m_szObjectId, pszEscDescr, m_dwNodeType, 
               pszEscVersion, pszEscPlatform, m_dwPollerNode, m_dwZoneGUID,
               m_dwProxyNode, m_dwId);
   bResult = DBQuery(hdb, szQuery);
   free(pszEscDescr);
   free(pszEscVersion);
   free(pszEscPlatform);

   // Save data collection items
   if (bResult)
   {
      DWORD i;

      for(i = 0; i < m_dwNumItems; i++)
         m_ppItems[i]->SaveToDB(hdb);
   }

   // Save access list
   SaveACLToDB(hdb);

   // Clear modifications flag and unlock object
   m_bIsModified = FALSE;
   UnlockData();

   return bResult;
}


//
// Delete object from database
//

BOOL Node::DeleteFromDB(void)
{
   char szQuery[256];
   BOOL bSuccess;

   bSuccess = Template::DeleteFromDB();
   if (bSuccess)
   {
      sprintf(szQuery, "DELETE FROM nodes WHERE id=%d", m_dwId);
      QueueSQLRequest(szQuery);
      sprintf(szQuery, "DELETE FROM nsmap WHERE node_id=%d", m_dwId);
      QueueSQLRequest(szQuery);
      sprintf(szQuery, "DROP TABLE idata_%d", m_dwId);
      QueueSQLRequest(szQuery);
   }
   return bSuccess;
}


//
// Get ARP cache from node
//

ARP_CACHE *Node::GetArpCache(void)
{
   ARP_CACHE *pArpCache = NULL;

   if (m_dwFlags & NF_IS_LOCAL_MGMT)
   {
      pArpCache = GetLocalArpCache();
   }
   else if (m_dwFlags & NF_IS_NATIVE_AGENT)
   {
      AgentLock();
      if (ConnectToAgent())
         pArpCache = m_pAgentConnection->GetArpCache();
      AgentUnlock();
   }
   else if (m_dwFlags & NF_IS_SNMP)
   {
      pArpCache = SnmpGetArpCache(m_iSNMPVersion, m_dwIpAddr, m_wSNMPPort,
                                  m_szCommunityString);
   }

   return pArpCache;
}


//
// Get list of interfaces from node
//

INTERFACE_LIST *Node::GetInterfaceList(void)
{
   INTERFACE_LIST *pIfList = NULL;

   if (m_dwFlags & NF_IS_LOCAL_MGMT)
   {
      pIfList = GetLocalInterfaceList();
   }
   if ((pIfList == NULL) && (m_dwFlags & NF_IS_NATIVE_AGENT) &&
       (!(m_dwFlags & NF_DISABLE_NXCP)))
   {
      AgentLock();
      if (ConnectToAgent())
      {
         pIfList = m_pAgentConnection->GetInterfaceList();
         CleanInterfaceList(pIfList);
      }
      AgentUnlock();
   }
   if ((pIfList == NULL) && (m_dwFlags & NF_IS_SNMP) &&
       (!(m_dwFlags & NF_DISABLE_SNMP)))
   {
      pIfList = SnmpGetInterfaceList(m_iSNMPVersion, m_dwIpAddr, m_wSNMPPort,
                                     m_szCommunityString, m_dwNodeType);
   }

   if (pIfList != NULL)
      CheckInterfaceNames(pIfList);

   return pIfList;
}


//
// Find interface by index and node IP
// Returns pointer to interface object or NULL if appropriate interface couldn't be found
//

Interface *Node::FindInterface(DWORD dwIndex, DWORD dwHostAddr)
{
   DWORD i;
   Interface *pInterface;

   LockChildList(FALSE);
   for(i = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i]->Type() == OBJECT_INTERFACE)
      {
         pInterface = (Interface *)m_pChildList[i];
         if (pInterface->IfIndex() == dwIndex)
         {
            if (((pInterface->IpAddr() & pInterface->IpNetMask()) ==
                 (dwHostAddr & pInterface->IpNetMask())) ||
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
// Create new interface
//

void Node::CreateNewInterface(DWORD dwIpAddr, DWORD dwNetMask, char *szName, 
                              DWORD dwIndex, DWORD dwType, BYTE *pbMacAddr)
{
   Interface *pInterface;
   Subnet *pSubnet = NULL;

   // Find subnet to place interface object to
   if (dwIpAddr != 0)
   {
      pSubnet = FindSubnetForNode(dwIpAddr);
      if (pSubnet == NULL)
      {
         // Check if netmask is 0 (detect), and if yes, create
         // new subnet with class mask
         if (dwNetMask == 0)
         {
            if (dwIpAddr < 0xE0000000)
            {
               dwNetMask = 0xFFFFFF00;   // Class A, B or C
            }
            else
            {
               TCHAR szBuffer[16];

               // Multicast address??
               DbgPrintf(AF_DEBUG_DISCOVERY, 
                         "Attempt to create interface object with multicast address %s", 
                         IpToStr(dwIpAddr, szBuffer));
            }
         }

         // Create new subnet object
         if (dwIpAddr < 0xE0000000)
         {
            pSubnet = new Subnet(dwIpAddr & dwNetMask, dwNetMask, m_dwZoneGUID);
            NetObjInsert(pSubnet, TRUE);
            g_pEntireNet->AddSubnet(pSubnet);
         }
      }
      else
      {
         // Set correct netmask if we was asked for it
         if (dwNetMask == 0)
         {
            dwNetMask = pSubnet->IpNetMask();
         }
      }
   }

   // Create interface object
   if (szName != NULL)
      pInterface = new Interface(szName, dwIndex, dwIpAddr, dwNetMask, dwType);
   else
      pInterface = new Interface(dwIpAddr, dwNetMask);
   if (pbMacAddr != NULL)
      pInterface->SetMacAddr(pbMacAddr);

   // Insert to objects' list and generate event
   NetObjInsert(pInterface, TRUE);
   AddInterface(pInterface);
   if (!m_bIsHidden)
      pInterface->Unhide();
   PostEvent(EVENT_INTERFACE_ADDED, m_dwId, "dsaad", pInterface->Id(),
             pInterface->Name(), pInterface->IpAddr(),
             pInterface->IpNetMask(), pInterface->IfIndex());

   // Bind node to appropriate subnet
   if (pSubnet != NULL)
   {
      pSubnet->AddNode(this);
      
      // Check if subnet mask is correct on interface
      if (pSubnet->IpNetMask() != dwNetMask)
         PostEvent(EVENT_INCORRECT_NETMASK, m_dwId, "idsaa", pInterface->Id(),
                   pInterface->IfIndex(), pInterface->Name(),
                   pInterface->IpNetMask(), pSubnet->IpNetMask());
   }
}


//
// Delete interface from node
//

void Node::DeleteInterface(Interface *pInterface)
{
   DWORD i;

   // Check if we should unlink node from interface's subnet
   if (pInterface->IpAddr() != 0)
   {
      BOOL bUnlink = TRUE;

      LockChildList(FALSE);
      for(i = 0; i < m_dwChildCount; i++)
         if (m_pChildList[i]->Type() == OBJECT_INTERFACE)
            if (m_pChildList[i] != pInterface)
               if ((((Interface *)m_pChildList[i])->IpAddr() & ((Interface *)m_pChildList[i])->IpNetMask()) ==
                   (pInterface->IpAddr() & pInterface->IpNetMask()))
               {
                  bUnlink = FALSE;
                  break;
               }
      UnlockChildList();
      
      if (bUnlink)
      {
         // Last interface in subnet, should unlink node
         Subnet *pSubnet = FindSubnetByIP(pInterface->IpAddr() & pInterface->IpNetMask());
         if (pSubnet != NULL)
         {
            DeleteParent(pSubnet);
            pSubnet->DeleteChild(this);
         }
      }
   }
   pInterface->Delete(FALSE);
}


//
// Calculate node status based on child objects status
//

void Node::CalculateCompoundStatus(void)
{
   int iOldStatus = m_iStatus;
   static DWORD dwEventCodes[] = { EVENT_NODE_NORMAL, EVENT_NODE_MINOR,
      EVENT_NODE_WARNING, EVENT_NODE_MAJOR, EVENT_NODE_CRITICAL,
      EVENT_NODE_UNKNOWN, EVENT_NODE_UNMANAGED };

   NetObj::CalculateCompoundStatus();
   if (m_iStatus != iOldStatus)
      PostEvent(dwEventCodes[m_iStatus], m_dwId, "d", iOldStatus);
}


//
// Perform status poll on node
//

void Node::StatusPoll(ClientSession *pSession, DWORD dwRqId, int nPoller)
{
   DWORD i, dwPollListSize, dwOldFlags = m_dwFlags;
   NetObj *pPollerNode = NULL, **ppPollList;
   BOOL bAllDown;
   Queue *pQueue;    // Delayed event queue
   time_t tNow, tExpire;

   pQueue = new Queue;
   SetPollerInfo(nPoller, "wait for lock");
   PollerLock();
   m_pPollRequestor = pSession;
   SendPollerMsg(dwRqId, "Starting status poll for node %s\r\n", m_szName);

   // Read capability expiration time and current time
   tExpire = (time_t)ConfigReadULong(_T("CapabilityExpirationTime"), 604800);
   tNow = time(NULL);

   // Check SNMP agent connectivity
   if ((m_dwFlags & NF_IS_SNMP) && (!(m_dwFlags & NF_DISABLE_SNMP)))
   {
      TCHAR szBuffer[256];
      DWORD dwResult;

      SetPollerInfo(nPoller, "check SNMP");
      SendPollerMsg(dwRqId, "Checking SNMP agent connectivity\r\n");
      dwResult = SnmpGet(m_iSNMPVersion, m_dwIpAddr, m_wSNMPPort, m_szCommunityString,
                         ".1.3.6.1.2.1.1.2.0", NULL, 0, szBuffer, 256, 
                         FALSE, FALSE);
      if ((dwResult == SNMP_ERR_SUCCESS) || (dwResult == SNMP_ERR_NO_OBJECT))
      {
         if (m_dwDynamicFlags & NDF_SNMP_UNREACHABLE)
         {
            m_dwDynamicFlags &= ~NDF_SNMP_UNREACHABLE;
            PostEventEx(pQueue, EVENT_SNMP_OK, m_dwId, NULL);
            SendPollerMsg(dwRqId, "Connectivity with SNMP agent restored\r\n");
         }
      }
      else
      {
         SendPollerMsg(dwRqId, "SNMP agent unreachable\r\n");
         if (m_dwDynamicFlags & NDF_SNMP_UNREACHABLE)
         {
            if ((tNow > m_tFailTimeSNMP + tExpire) &&
                (!(m_dwDynamicFlags & NDF_UNREACHABLE)))
            {
               m_dwFlags &= ~NF_IS_SNMP;
               m_dwDynamicFlags &= ~NDF_SNMP_UNREACHABLE;
               m_szObjectId[0] = 0;
               SendPollerMsg(dwRqId, "Attribute isSNMP set to FALSE\r\n");
            }
         }
         else
         {
            m_dwDynamicFlags |= NDF_SNMP_UNREACHABLE;
            PostEventEx(pQueue, EVENT_SNMP_FAIL, m_dwId, NULL);
            m_tFailTimeSNMP = tNow;
         }
      }
   }

   // Check native agent connectivity
   if ((m_dwFlags & NF_IS_NATIVE_AGENT) && (!(m_dwFlags & NF_DISABLE_NXCP)))
   {
      AgentConnection *pAgentConn;

      SetPollerInfo(nPoller, "check agent");
      SendPollerMsg(dwRqId, "Checking NetXMS agent connectivity\r\n");
      pAgentConn = new AgentConnection(htonl(m_dwIpAddr), m_wAgentPort,
                                       m_wAuthMethod, m_szSharedSecret);
      SetAgentProxy(pAgentConn);
      if (pAgentConn->Connect(g_pServerKey))
      {
         if (m_dwDynamicFlags & NDF_AGENT_UNREACHABLE)
         {
            m_dwDynamicFlags &= ~NDF_AGENT_UNREACHABLE;
            PostEventEx(pQueue, EVENT_AGENT_OK, m_dwId, NULL);
            SendPollerMsg(dwRqId, "Connectivity with NetXMS agent restored\r\n");
         }
         pAgentConn->Disconnect();
      }
      else
      {
         SendPollerMsg(dwRqId, "NetXMS agent unreachable\r\n");
         if (m_dwDynamicFlags & NDF_AGENT_UNREACHABLE)
         {
            if ((tNow > m_tFailTimeAgent + tExpire) &&
                (!(m_dwDynamicFlags & NDF_UNREACHABLE)))
            {
               m_dwFlags &= ~NF_IS_NATIVE_AGENT;
               m_dwDynamicFlags &= ~NDF_AGENT_UNREACHABLE;
               m_szPlatformName[0] = 0;
               m_szAgentVersion[0] = 0;
               SendPollerMsg(dwRqId, "Attribute isNetXMSAgent set to FALSE\r\n");
            }
         }
         else
         {
            m_dwDynamicFlags |= NDF_AGENT_UNREACHABLE;
            PostEventEx(pQueue, EVENT_AGENT_FAIL, m_dwId, NULL);
            m_tFailTimeAgent = tNow;
         }
      }
      delete pAgentConn;
   }

   SetPollerInfo(nPoller, "prepare polling list");

   // Find service poller node object
   LockData();
   if (m_dwPollerNode != 0)
   {
      pPollerNode = FindObjectById(m_dwPollerNode);
      if (pPollerNode != NULL)
      {
         if (pPollerNode->Type() != OBJECT_NODE)
            pPollerNode = NULL;
      }
   }
   UnlockData();

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
   SetPollerInfo(nPoller, "child poll");
   for(i = 0; i < dwPollListSize; i++)
   {
      switch(ppPollList[i]->Type())
      {
         case OBJECT_INTERFACE:
            ((Interface *)ppPollList[i])->StatusPoll(pSession, dwRqId, pQueue);
            break;
         case OBJECT_NETWORKSERVICE:
            ((NetworkService *)ppPollList[i])->StatusPoll(pSession, dwRqId,
                                                          (Node *)pPollerNode, pQueue);
            break;
         default:
            break;
      }
      ppPollList[i]->DecRefCount();
   }
   safe_free(ppPollList);

   // Check if entire node is down
   LockChildList(FALSE);
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
         SendPollerMsg(dwRqId, "Node is unreachable\r\n");
      }
      else
      {
         SendPollerMsg(dwRqId, "Node is still unreachable\r\n");
      }
   }
   else
   {
      if (m_dwDynamicFlags & NDF_UNREACHABLE)
      {
         m_dwDynamicFlags &= ~NDF_UNREACHABLE;
         PostEvent(EVENT_NODE_UP, m_dwId, NULL);
         SendPollerMsg(dwRqId, "Node recovered from unreachable state\r\n");
      }
      else
      {
         SendPollerMsg(dwRqId, "Node is connected\r\n");
      }
   }

   // Send delayed events and destroy delayed event queue
   ResendEvents(pQueue);
   delete pQueue;
   
   SetPollerInfo(nPoller, "cleanup");
   if (pPollerNode != NULL)
      pPollerNode->DecRefCount();

   if (dwOldFlags != m_dwFlags)
   {
      PostEvent(EVENT_NODE_FLAGS_CHANGED, m_dwId, "xx", dwOldFlags, m_dwFlags);
      LockData();
      Modify();
      UnlockData();
   }

   CalculateCompoundStatus();
   m_tLastStatusPoll = time(NULL);
   SendPollerMsg(dwRqId, "Finished status poll for node %s\r\n"
                         "Node status after poll is %s\r\n", m_szName, g_pszStatusName[m_iStatus]);
   m_pPollRequestor = NULL;
   if (dwRqId == 0)
      m_dwDynamicFlags &= ~NDF_QUEUED_FOR_STATUS_POLL;
   PollerUnlock();
}


//
// Perform configuration poll on node
//

void Node::ConfigurationPoll(ClientSession *pSession, DWORD dwRqId,
                             int nPoller, DWORD dwNetMask)
{
   DWORD i, dwOldFlags = m_dwFlags;
   Interface **ppDeleteList;
   int j, iDelCount;
   AgentConnection *pAgentConn;
   INTERFACE_LIST *pIfList;
   char szBuffer[4096];
   BOOL bHasChanges = FALSE;

   SetPollerInfo(nPoller, "wait for lock");
   PollerLock();
   m_pPollRequestor = pSession;
   SendPollerMsg(dwRqId, _T("Starting configuration poll for node %s\r\n"), m_szName);
   DbgPrintf(AF_DEBUG_DISCOVERY, "Starting configuration poll for node %s (ID: %d)", m_szName, m_dwId);

   // Check for forced capabilities recheck
   if (m_dwDynamicFlags & NDF_RECHECK_CAPABILITIES)
   {
      m_dwDynamicFlags &= ~(NDF_UNREACHABLE | NDF_SNMP_UNREACHABLE |
                            NDF_CPSNMP_UNREACHABLE | NDF_AGENT_UNREACHABLE);
      m_dwFlags &= ~(NF_IS_NATIVE_AGENT | NF_IS_SNMP | NF_IS_CPSNMP |
                     NF_IS_BRIDGE | NF_IS_ROUTER | NF_IS_OSPF);
      m_szObjectId[0] = 0;
      m_szPlatformName[0] = 0;
      m_szAgentVersion[0] = 0;
   }

   // Check if node is marked as unreachable
   if (m_dwDynamicFlags & NDF_UNREACHABLE)
   {
      SendPollerMsg(dwRqId, _T("Node is marked as unreachable, configuration poll aborted\r\n"));
      DbgPrintf(AF_DEBUG_DISCOVERY, "Node is marked as unreachable, configuration poll aborted");
      m_tLastConfigurationPoll = time(NULL);
   }
   else
   {
      // Check node's capabilities
      SetPollerInfo(nPoller, "capability check");
      SendPollerMsg(dwRqId, _T("Checking node's capabilities...\r\n"));
      if ((!((m_dwFlags & NF_IS_SNMP) && (m_dwDynamicFlags & NDF_SNMP_UNREACHABLE))) &&
          (!(m_dwFlags & NF_DISABLE_SNMP)))
      {
         DbgPrintf(AF_DEBUG_DISCOVERY, "ConfPoll(%s): trying SNMP GET", m_szName);
         if (SnmpGet(m_iSNMPVersion, m_dwIpAddr, m_wSNMPPort, m_szCommunityString,
                     ".1.3.6.1.2.1.1.2.0", NULL, 0, szBuffer, 4096,
                     FALSE, FALSE) == SNMP_ERR_SUCCESS)
         {
            DWORD dwNodeFlags, dwNodeType;

            LockData();

            if (strcmp(m_szObjectId, szBuffer))
            {
               nx_strncpy(m_szObjectId, szBuffer, MAX_OID_LEN * 4);
               bHasChanges = TRUE;
            }

            m_dwFlags |= NF_IS_SNMP;
            m_dwDynamicFlags &= ~NDF_SNMP_UNREACHABLE;
            SendPollerMsg(dwRqId, _T("   SNMP agent is active\r\n"));

            // Check node type
            dwNodeType = OidToType(m_szObjectId, &dwNodeFlags);
            if (m_dwNodeType != dwNodeType)
            {
               m_dwFlags |= dwNodeFlags;
               m_dwNodeType = dwNodeType;
               SendPollerMsg(dwRqId, _T("   Node type has been changed to %d\r\n"), m_dwNodeType);
               bHasChanges = TRUE;
            }

            // Check IP forwarding
            if (CheckSNMPIntegerValue(".1.3.6.1.2.1.4.1.0", 1))
            {
               m_dwFlags |= NF_IS_ROUTER;
            }
            else
            {
               m_dwFlags &= ~NF_IS_ROUTER;
            }

            // Check for bridge MIB support
            if (SnmpGet(m_iSNMPVersion, m_dwIpAddr, m_wSNMPPort, m_szCommunityString,
                        ".1.3.6.1.2.1.17.1.1.0", NULL, 0, szBuffer, 4096,
                        FALSE, FALSE) == SNMP_ERR_SUCCESS)
            {
               m_dwFlags |= NF_IS_BRIDGE;
            }
            else
            {
               m_dwFlags &= ~NF_IS_BRIDGE;
            }

            // Check for CDP (Cisco Discovery Protocol) support
            if (CheckSNMPIntegerValue(".1.3.6.1.4.1.9.9.23.1.3.1.0", 1))
            {
               m_dwFlags |= NF_IS_CDP;
            }
            else
            {
               m_dwFlags &= ~NF_IS_CDP;
            }

            // Check for Nortel topology discovery support
            if (CheckSNMPIntegerValue(".1.3.6.1.4.1.45.1.6.13.1.2.0", 1))
            {
               m_dwFlags |= NF_IS_NORTEL_TOPO;
            }
            else
            {
               m_dwFlags &= ~NF_IS_NORTEL_TOPO;
            }

            UnlockData();

            CheckOSPFSupport();
         }
         else
         {
            // Check for CheckPoint SNMP agent on port 161
            DbgPrintf(AF_DEBUG_DISCOVERY, "ConfPoll(%s): checking for CheckPoint SNMP", m_szName);
            if (SnmpGet(m_iSNMPVersion, m_dwIpAddr, m_wSNMPPort, m_szCommunityString,
                        ".1.3.6.1.4.1.2620.1.1.10.0", NULL, 0,
                        szBuffer, 4096, FALSE, FALSE) == SNMP_ERR_SUCCESS)
            {
               LockData();
               if (strcmp(m_szObjectId, ".1.3.6.1.4.1.2620.1.1"))
               {
                  nx_strncpy(m_szObjectId, ".1.3.6.1.4.1.2620.1.1", MAX_OID_LEN * 4);
                  bHasChanges = TRUE;
               }

               m_dwFlags |= NF_IS_SNMP | NF_IS_ROUTER;
               m_dwDynamicFlags &= ~NDF_SNMP_UNREACHABLE;
               UnlockData();
               SendPollerMsg(dwRqId, _T("   CheckPoint SNMP agent on port 161 is active\r\n"));
            }
         }
      }

      // Check for CheckPoint SNMP agent on port 260
      DbgPrintf(AF_DEBUG_DISCOVERY, "ConfPoll(%s): checking for CheckPoint SNMP on port 260", m_szName);
      if (!((m_dwFlags & NF_IS_CPSNMP) && (m_dwDynamicFlags & NDF_CPSNMP_UNREACHABLE)))
      {
         if (SnmpGet(SNMP_VERSION_1, m_dwIpAddr, CHECKPOINT_SNMP_PORT, m_szCommunityString,
                     ".1.3.6.1.4.1.2620.1.1.10.0", NULL, 0,
                     szBuffer, 4096, FALSE, FALSE) == SNMP_ERR_SUCCESS)
         {
            LockData();
            m_dwFlags |= NF_IS_CPSNMP | NF_IS_ROUTER;
            m_dwDynamicFlags &= ~NDF_CPSNMP_UNREACHABLE;
            UnlockData();
            SendPollerMsg(dwRqId, _T("   CheckPoint SNMP agent on port 260 is active\r\n"));
         }
      }

      DbgPrintf(AF_DEBUG_DISCOVERY, "ConfPoll(%s): checking for NetXMS agent Flags={%08X} DynamicFlags={%08X}", m_szName, m_dwFlags, m_dwDynamicFlags);
      if ((!((m_dwFlags & NF_IS_NATIVE_AGENT) && (m_dwDynamicFlags & NDF_AGENT_UNREACHABLE))) &&
          (!(m_dwFlags & NF_DISABLE_NXCP)))
      {
         pAgentConn = new AgentConnection(htonl(m_dwIpAddr), m_wAgentPort,
                                          m_wAuthMethod, m_szSharedSecret);
         SetAgentProxy(pAgentConn);
         DbgPrintf(AF_DEBUG_DISCOVERY, "ConfPoll(%s): checking for NetXMS agent - connecting", m_szName);
         if (pAgentConn->Connect(g_pServerKey))
         {
            DbgPrintf(AF_DEBUG_DISCOVERY, "ConfPoll(%s): checking for NetXMS agent - connected", m_szName);
            LockData();
            m_dwFlags |= NF_IS_NATIVE_AGENT;
            m_dwDynamicFlags &= ~NDF_AGENT_UNREACHABLE;
            UnlockData();
      
            if (pAgentConn->GetParameter("Agent.Version", MAX_AGENT_VERSION_LEN, szBuffer) == ERR_SUCCESS)
            {
               LockData();
               if (strcmp(m_szAgentVersion, szBuffer))
               {
                  strcpy(m_szAgentVersion, szBuffer);
                  bHasChanges = TRUE;
                  SendPollerMsg(dwRqId, _T("   NetXMS agent version changed to %s\r\n"), m_szAgentVersion);
               }
               UnlockData();
            }

            if (pAgentConn->GetParameter("System.PlatformName", MAX_PLATFORM_NAME_LEN, szBuffer) == ERR_SUCCESS)
            {
               LockData();
               if (strcmp(m_szPlatformName, szBuffer))
               {
                  strcpy(m_szPlatformName, szBuffer);
                  bHasChanges = TRUE;
                  SendPollerMsg(dwRqId, _T("   Platform name changed to %s\r\n"), m_szPlatformName);
               }
               UnlockData();
            }

            // Check IP forwarding status
            if (pAgentConn->GetParameter("Net.IP.Forwarding", 16, szBuffer) == ERR_SUCCESS)
            {
               if (_tcstoul(szBuffer, NULL, 10) != 0)
                  m_dwFlags |= NF_IS_ROUTER;
               else
                  m_dwFlags &= ~NF_IS_ROUTER;
            }

            /* LOCK? */
            safe_free(m_pParamList);
            pAgentConn->GetSupportedParameters(&m_dwNumParams, &m_pParamList);

            pAgentConn->Disconnect();
            SendPollerMsg(dwRqId, _T("   NetXMS native agent is active\r\n"));
         }
         delete pAgentConn;
         DbgPrintf(AF_DEBUG_DISCOVERY, "ConfPoll(%s): checking for NetXMS agent - finished", m_szName);
      }

      // Generate event if node flags has been changed
      if (dwOldFlags != m_dwFlags)
      {
         PostEvent(EVENT_NODE_FLAGS_CHANGED, m_dwId, "xx", dwOldFlags, m_dwFlags);
         bHasChanges = TRUE;
      }

      // Retrieve interface list
      SetPollerInfo(nPoller, "interface check");
      SendPollerMsg(dwRqId, _T("Capability check finished\r\n"
                               "Checking interface configuration...\r\n"));
      pIfList = GetInterfaceList();
      if (pIfList != NULL)
      {
         // Find non-existing interfaces
         LockChildList(FALSE);
         ppDeleteList = (Interface **)malloc(sizeof(Interface *) * m_dwChildCount);
         for(i = 0, iDelCount = 0; i < m_dwChildCount; i++)
         {
            if (m_pChildList[i]->Type() == OBJECT_INTERFACE)
            {
               Interface *pInterface = (Interface *)m_pChildList[i];

               if (pInterface->IfType() != IFTYPE_NETXMS_NAT_ADAPTER)
               {
                  for(j = 0; j < pIfList->iNumEntries; j++)
                  {
                     if ((pIfList->pInterfaces[j].dwIndex == pInterface->IfIndex()) &&
                         (pIfList->pInterfaces[j].dwIpAddr == pInterface->IpAddr()) &&
                         (pIfList->pInterfaces[j].dwIpNetMask == pInterface->IpNetMask()))
                        break;
                  }

                  if (j == pIfList->iNumEntries)
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
               SendPollerMsg(dwRqId, _T("   Interface \"%s\" is no longer exist\r\n"), 
                             ppDeleteList[j]->Name());
               PostEvent(EVENT_INTERFACE_DELETED, m_dwId, "dsaa", ppDeleteList[j]->IfIndex(),
                         ppDeleteList[j]->Name(), ppDeleteList[j]->IpAddr(), ppDeleteList[j]->IpNetMask());
               DeleteInterface(ppDeleteList[j]);
            }
            bHasChanges = TRUE;
         }
         safe_free(ppDeleteList);

         // Add new interfaces and check configuration of existing
         for(j = 0; j < pIfList->iNumEntries; j++)
         {
            BOOL bNewInterface = TRUE;

            LockChildList(FALSE);
            for(i = 0; i < m_dwChildCount; i++)
            {
               if (m_pChildList[i]->Type() == OBJECT_INTERFACE)
               {
                  Interface *pInterface = (Interface *)m_pChildList[i];

                  if ((pIfList->pInterfaces[j].dwIndex == pInterface->IfIndex()) &&
                      (pIfList->pInterfaces[j].dwIpAddr == pInterface->IpAddr()) &&
                      (pIfList->pInterfaces[j].dwIpNetMask == pInterface->IpNetMask()))
                  {
                     // Existing interface, check configuration
                     if (memcmp(pIfList->pInterfaces[j].bMacAddr, pInterface->MacAddr(), MAC_ADDR_LENGTH))
                     {
                        char szOldMac[16], szNewMac[16];

                        BinToStr((BYTE *)pInterface->MacAddr(), MAC_ADDR_LENGTH, szOldMac);
                        BinToStr(pIfList->pInterfaces[j].bMacAddr, MAC_ADDR_LENGTH, szNewMac);
                        PostEvent(EVENT_MAC_ADDR_CHANGED, m_dwId, "idsss",
                                  pInterface->Id(), pInterface->IfIndex(),
                                  pInterface->Name(), szOldMac, szNewMac);
                        pInterface->SetMacAddr(pIfList->pInterfaces[j].bMacAddr);
                     }
                     if (strcmp(pIfList->pInterfaces[j].szName, pInterface->Name()))
                     {
                        pInterface->SetName(pIfList->pInterfaces[j].szName);
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
               SendPollerMsg(dwRqId, _T("   Found new interface \"%s\"\r\n"), 
                             pIfList->pInterfaces[j].szName);
               CreateNewInterface(pIfList->pInterfaces[j].dwIpAddr, 
                                  pIfList->pInterfaces[j].dwIpNetMask,
                                  pIfList->pInterfaces[j].szName,
                                  pIfList->pInterfaces[j].dwIndex,
                                  pIfList->pInterfaces[j].dwType,
                                  pIfList->pInterfaces[j].bMacAddr);
               bHasChanges = TRUE;
            }
         }

         // Check if address we are using to communicate with node
         // is configured on one of node's interfaces
         for(i = 0; i < (DWORD)pIfList->iNumEntries; i++)
            if (pIfList->pInterfaces[i].dwIpAddr == m_dwIpAddr)
               break;

         if (i == (DWORD)pIfList->iNumEntries)
         {
            BOOL bCreate = TRUE;

            // Node is behind NAT
            m_dwFlags |= NF_BEHIND_NAT;

            // Check if we already have NAT interface
            LockChildList(FALSE);
            for(i = 0; i < m_dwChildCount; i++)
               if (m_pChildList[i]->Type() == OBJECT_INTERFACE)
               {
                  if (((Interface *)m_pChildList[i])->IfType() == IFTYPE_NETXMS_NAT_ADAPTER)
                  {
                     bCreate = FALSE;
                     break;
                  }
               }
            UnlockChildList();

            if (bCreate)
            {
               char szBuffer[MAX_OBJECT_NAME];

               // Create pseudo interface for NAT
               ConfigReadStr("NATAdapterName", szBuffer, MAX_OBJECT_NAME, "NetXMS NAT Adapter");
               CreateNewInterface(m_dwIpAddr, 0, szBuffer,
                                  0x7FFFFFFF, IFTYPE_NETXMS_NAT_ADAPTER);
               bHasChanges = TRUE;
            }
         }
         else
         {
            // Check if NF_BEHIND_NAT flag set incorrectly
            if (m_dwFlags & NF_BEHIND_NAT)
            {
               Interface *pIfNat;

               // Remove NAT interface
               LockChildList(FALSE);
               for(i = 0, pIfNat = NULL; i < m_dwChildCount; i++)
                  if (m_pChildList[i]->Type() == OBJECT_INTERFACE)
                  {
                     if (((Interface *)m_pChildList[i])->IfType() == IFTYPE_NETXMS_NAT_ADAPTER)
                     {
                        pIfNat = (Interface *)m_pChildList[i];
                        break;
                     }
                  }
               UnlockChildList();

               if (pIfNat != NULL)
                  DeleteInterface(pIfNat);

               m_dwFlags &= ~NF_BEHIND_NAT;
               bHasChanges = TRUE;
            }
         }

         DestroyInterfaceList(pIfList);
      }
      else     /* pIfList == NULL */
      {
         Interface *pInterface;
         DWORD dwCount;

         SendPollerMsg(dwRqId, _T("   Unable to get interface list from node\r\n"));

         // Delete all existing interfaces in case of forced capability recheck
         if (m_dwDynamicFlags & NDF_RECHECK_CAPABILITIES)
         {
            LockChildList(FALSE);
            ppDeleteList = (Interface **)malloc(sizeof(Interface *) * m_dwChildCount);
            for(i = 0, iDelCount = 0; i < m_dwChildCount; i++)
            {
               if (m_pChildList[i]->Type() == OBJECT_INTERFACE)
                  ppDeleteList[iDelCount++] = (Interface *)m_pChildList[i];
            }
            UnlockChildList();
            for(j = 0; j < iDelCount; j++)
            {
               SendPollerMsg(dwRqId, _T("   Interface \"%s\" is no longer exist\r\n"), 
                             ppDeleteList[j]->Name());
               PostEvent(EVENT_INTERFACE_DELETED, m_dwId, "dsaa", ppDeleteList[j]->IfIndex(),
                         ppDeleteList[j]->Name(), ppDeleteList[j]->IpAddr(), ppDeleteList[j]->IpNetMask());
               DeleteInterface(ppDeleteList[j]);
            }
            safe_free(ppDeleteList);
         }

         // Check if we have pseudo-interface object
         dwCount = GetInterfaceCount(&pInterface);
         if (dwCount == 1)
         {
            if (pInterface->IsFake())
            {
               // Check if primary IP is different from interface's IP
               if (pInterface->IpAddr() != m_dwIpAddr)
               {
                  DeleteInterface(pInterface);
                  CreateNewInterface(m_dwIpAddr, dwNetMask);
               }
            }
         }
         else if (dwCount == 0)
         {
            // No interfaces at all, create pseudo-interface
            CreateNewInterface(m_dwIpAddr, dwNetMask);
         }
      }

      m_tLastConfigurationPoll = time(NULL);
      SendPollerMsg(dwRqId, _T("Interface configuration check finished\r\n"
                               "Finished configuration poll for node %s\r\n"
                               "Node configuration was%schanged after poll\r\n"),
                    m_szName, bHasChanges ? _T(" ") : _T(" not "));
   }

   // Finish configuration poll
   SetPollerInfo(nPoller, "cleanup");
   if (dwRqId == 0)
      m_dwDynamicFlags &= ~NDF_QUEUED_FOR_CONFIG_POLL;
   m_dwDynamicFlags &= ~NDF_RECHECK_CAPABILITIES;
   PollerUnlock();
   DbgPrintf(AF_DEBUG_DISCOVERY, "Finished configuration poll for node %s (ID: %d)", m_szName, m_dwId);

   if (bHasChanges)
   {
      LockData();
      Modify();
      UnlockData();
   }
}


//
// Connect to native agent
//

BOOL Node::ConnectToAgent(void)
{
   // Create new agent connection object if needed
   if (m_pAgentConnection == NULL)
      m_pAgentConnection = new AgentConnectionEx(htonl(m_dwIpAddr), m_wAgentPort, m_wAuthMethod, m_szSharedSecret);

   // Check if we already connected
   if (m_pAgentConnection->Nop() == ERR_SUCCESS)
      return TRUE;

   // Close current connection or clean up after broken connection
   m_pAgentConnection->Disconnect();
   m_pAgentConnection->SetPort(m_wAgentPort);
   m_pAgentConnection->SetAuthData(m_wAuthMethod, m_szSharedSecret);
   SetAgentProxy(m_pAgentConnection);
   return m_pAgentConnection->Connect(g_pServerKey);
}


//
// Get item's value via SNMP
//

DWORD Node::GetItemFromSNMP(const char *szParam, DWORD dwBufSize, char *szBuffer)
{
   DWORD dwResult;

   if ((m_dwDynamicFlags & NDF_SNMP_UNREACHABLE) ||
       (m_dwDynamicFlags & NDF_UNREACHABLE))
   {
      dwResult = SNMP_ERR_COMM;
   }
   else
   {
      dwResult = SnmpGet(m_iSNMPVersion, m_dwIpAddr, m_wSNMPPort, m_szCommunityString,
                         szParam, NULL, 0, szBuffer, dwBufSize, FALSE, TRUE);
   }
   return (dwResult == SNMP_ERR_SUCCESS) ? DCE_SUCCESS : 
      ((dwResult == SNMP_ERR_NO_OBJECT) ? DCE_NOT_SUPPORTED : DCE_COMM_ERROR);
}


//
// Get item's value via SNMP from CheckPoint's agent
//

DWORD Node::GetItemFromCheckPointSNMP(const char *szParam, DWORD dwBufSize, char *szBuffer)
{
   DWORD dwResult;

   if ((m_dwDynamicFlags & NDF_CPSNMP_UNREACHABLE) ||
       (m_dwDynamicFlags & NDF_UNREACHABLE))
   {
      dwResult = SNMP_ERR_COMM;
   }
   else
   {
      dwResult = SnmpGet(SNMP_VERSION_1, m_dwIpAddr, CHECKPOINT_SNMP_PORT,
                         m_szCommunityString, szParam, NULL, 0, szBuffer,
                         dwBufSize, FALSE, TRUE);
   }
   return (dwResult == SNMP_ERR_SUCCESS) ? DCE_SUCCESS : 
      ((dwResult == SNMP_ERR_NO_OBJECT) ? DCE_NOT_SUPPORTED : DCE_COMM_ERROR);
}


//
// Get item's value via native agent
//

DWORD Node::GetItemFromAgent(const char *szParam, DWORD dwBufSize, char *szBuffer)
{
   DWORD dwError, dwResult = DCE_COMM_ERROR;
   DWORD dwTries = 5;

   if ((m_dwDynamicFlags & NDF_AGENT_UNREACHABLE) ||
       (m_dwDynamicFlags & NDF_UNREACHABLE))
      return DCE_COMM_ERROR;

   AgentLock();

   // Establish connection if needed
   if (m_pAgentConnection == NULL)
      if (!ConnectToAgent())
         goto end_loop;

   // Get parameter from agent
   while(dwTries-- > 0)
   {
      dwError = m_pAgentConnection->GetParameter((char *)szParam, dwBufSize, szBuffer);
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
            if (!ConnectToAgent())
               goto end_loop;
            break;
         case ERR_REQUEST_TIMEOUT:
            break;
      }
   }

end_loop:
   AgentUnlock();
   DbgPrintf(AF_DEBUG_DC, "Node(%s)->GetItemFromAgent(%s): dwError=%d dwResult=%d",
             m_szName, szParam, dwError, dwResult);
   return dwResult;
}


//
// Get value for server's internal parameter
//

DWORD Node::GetInternalItem(const char *szParam, DWORD dwBufSize, char *szBuffer)
{
   DWORD dwError = DCE_SUCCESS;

   if (!stricmp(szParam, "status"))
   {
      sprintf(szBuffer, "%d", m_iStatus);
   }
   else if (!stricmp(szParam, "AgentStatus"))
   {
      if (m_dwFlags & NF_IS_NATIVE_AGENT)
      {
         szBuffer[0] = (m_dwDynamicFlags & NDF_AGENT_UNREACHABLE) ? '1' : '0';
         szBuffer[1] = 0;
      }
      else
      {
         dwError = DCE_NOT_SUPPORTED;
      }
   }
   else if (MatchString("ChildStatus(*)", szParam, FALSE))
   {
      char *pEnd, szArg[256];
      DWORD i, dwId;
      NetObj *pObject = NULL;

      NxGetParameterArg((char *)szParam, 1, szArg, 256);
      dwId = strtoul(szArg, &pEnd, 0);
      if (*pEnd != 0)
      {
         // Argument is object's name
         dwId = 0;
      }

      // Find child object with requested ID or name
      LockChildList(FALSE);
      for(i = 0; i < m_dwChildCount; i++)
      {
         if (((dwId == 0) && (!stricmp(m_pChildList[i]->Name(), szArg))) ||
             (dwId == m_pChildList[i]->Id()))
         {
            pObject = m_pChildList[i];
            break;
         }
      }
      UnlockChildList();

      if (pObject != NULL)
      {
         sprintf(szBuffer, "%d", pObject->Status());
      }
      else
      {
         dwError = DCE_NOT_SUPPORTED;
      }
   }
   else if (m_dwFlags & NF_IS_LOCAL_MGMT)
   {
      if (!stricmp(szParam, "Server.AverageDCPollerQueueSize"))
      {
         sprintf(szBuffer, "%f", g_dAvgPollerQueueSize);
      }
      else if (!stricmp(szParam, "Server.AverageDBWriterQueueSize"))
      {
         sprintf(szBuffer, "%f", g_dAvgDBWriterQueueSize);
      }
      else if (!stricmp(szParam, "Server.AverageStatusPollerQueueSize"))
      {
         sprintf(szBuffer, "%f", g_dAvgStatusPollerQueueSize);
      }
      else if (!stricmp(szParam, "Server.AverageConfigurationPollerQueueSize"))
      {
         sprintf(szBuffer, "%f", g_dAvgConfigPollerQueueSize);
      }
      else if (!stricmp(szParam, "Server.AverageDCIQueuingTime"))
      {
         sprintf(szBuffer, "%u", g_dwAvgDCIQueuingTime);
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
// Get item's value for client
//

DWORD Node::GetItemForClient(int iOrigin, const char *pszParam, char *pszBuffer, DWORD dwBufSize)
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
         dwRetCode = GetItemFromSNMP(pszParam, dwBufSize, pszBuffer);
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
   {
      switch(dwRetCode)
      {
         case DCE_SUCCESS:
            dwResult = RCC_SUCCESS;
            break;
         case DCE_COMM_ERROR:
            dwResult = RCC_COMM_FAILURE;
            break;
         case DCE_NOT_SUPPORTED:
            dwResult = RCC_DCI_NOT_SUPPORTED;
            break;
         default:
            dwResult = RCC_SYSTEM_FAILURE;
            break;
      }
   }

   return dwResult;
}


//
// Put items which requires polling into the queue
//

void Node::QueueItemsForPolling(Queue *pPollerQueue)
{
   DWORD i;
   time_t currTime;

   if (m_iStatus == STATUS_UNMANAGED)
      return;  // Do not collect data for unmanaged nodes

   currTime = time(NULL);

   LockData();
   for(i = 0; i < m_dwNumItems; i++)
   {
      if (m_ppItems[i]->ReadyForPolling(currTime))
      {
         m_ppItems[i]->SetBusyFlag(TRUE);
         IncRefCount();   // Increment reference count for each queued DCI
         pPollerQueue->Put(m_ppItems[i]);
      }
   }
   UnlockData();
}


//
// Create CSCP message with object's data
//

void Node::CreateMessage(CSCPMessage *pMsg)
{
   Template::CreateMessage(pMsg);
   pMsg->SetVariable(VID_FLAGS, m_dwFlags);
   pMsg->SetVariable(VID_AGENT_PORT, m_wAgentPort);
   pMsg->SetVariable(VID_AUTH_METHOD, m_wAuthMethod);
   pMsg->SetVariable(VID_SHARED_SECRET, m_szSharedSecret);
   pMsg->SetVariable(VID_COMMUNITY_STRING, m_szCommunityString);
   pMsg->SetVariable(VID_SNMP_OID, m_szObjectId);
   pMsg->SetVariable(VID_NODE_TYPE, m_dwNodeType);
   pMsg->SetVariable(VID_SNMP_VERSION, (WORD)m_iSNMPVersion);
   pMsg->SetVariable(VID_AGENT_VERSION, m_szAgentVersion);
   pMsg->SetVariable(VID_PLATFORM_NAME, m_szPlatformName);
   pMsg->SetVariable(VID_POLLER_NODE_ID, m_dwPollerNode);
   pMsg->SetVariable(VID_ZONE_GUID, m_dwZoneGUID);
   pMsg->SetVariable(VID_PROXY_NODE, m_dwProxyNode);
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

      UpdateNodeIndex(m_dwIpAddr, dwIpAddr, this);
      m_dwIpAddr = dwIpAddr;
   }

   // Poller node ID
   if (pRequest->IsVariableExist(VID_POLLER_NODE_ID))
   {
      DWORD dwNodeId;
      NetObj *pObject;
      
      dwNodeId = pRequest->GetVariableLong(VID_POLLER_NODE_ID);
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
      m_iSNMPVersion = pRequest->GetVariableShort(VID_SNMP_VERSION);

   // Change SNMP community string
   if (pRequest->IsVariableExist(VID_COMMUNITY_STRING))
      pRequest->GetVariableStr(VID_COMMUNITY_STRING, m_szCommunityString, MAX_COMMUNITY_LENGTH);

   // Change proxy node
   if (pRequest->IsVariableExist(VID_PROXY_NODE))
      m_dwProxyNode = pRequest->GetVariableLong(VID_PROXY_NODE);

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

DWORD Node::WakeUp(void)
{
   DWORD i, dwResult = RCC_NO_WOL_INTERFACES;

   LockChildList(FALSE);

   for(i = 0; i < m_dwChildCount; i++)
      if ((m_pChildList[i]->Type() == OBJECT_INTERFACE) &&
          (m_pChildList[i]->Status() != STATUS_UNMANAGED) &&
          (m_pChildList[i]->IpAddr() != 0))
      {
         dwResult = ((Interface *)m_pChildList[i])->WakeUp();
         break;
      }

   UnlockChildList();
   return dwResult;
}


//
// Get status of interface with given index from SNMP agent
//

int Node::GetInterfaceStatusFromSNMP(DWORD dwIndex)
{
   return SnmpGetInterfaceStatus(m_iSNMPVersion, m_dwIpAddr, m_wSNMPPort,
                                 m_szCommunityString, dwIndex);
}


//
// Get status of interface with given index from native agent
//

int Node::GetInterfaceStatusFromAgent(DWORD dwIndex)
{
   char szParam[128], szBuffer[32];
   DWORD dwAdminStatus, dwLinkState;
   int iStatus;

   // Get administrative status
   sprintf(szParam, "Net.Interface.AdminStatus(%u)", dwIndex);
   if (GetItemFromAgent(szParam, 32, szBuffer) == DCE_SUCCESS)
   {
      dwAdminStatus = strtoul(szBuffer, NULL, 0);

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
            sprintf(szParam, "Net.Interface.Link(%u)", dwIndex);
            if (GetItemFromAgent(szParam, 32, szBuffer) == DCE_SUCCESS)
            {
               dwLinkState = strtoul(szBuffer, NULL, 0);
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

void Node::WriteParamListToMessage(CSCPMessage *pMsg)
{
   DWORD i, dwId;

   LockData();
   if (m_pParamList != NULL)
   {
      pMsg->SetVariable(VID_NUM_PARAMETERS, m_dwNumParams);
      for(i = 0, dwId = VID_PARAM_LIST_BASE; i < m_dwNumParams; i++)
      {
         pMsg->SetVariable(dwId++, m_pParamList[i].szName);
         pMsg->SetVariable(dwId++, m_pParamList[i].szDescription);
         pMsg->SetVariable(dwId++, (WORD)m_pParamList[i].iDataType);
      }
   }
   else
   {
      pMsg->SetVariable(VID_NUM_PARAMETERS, (DWORD)0);
   }
   UnlockData();
}


//
// Open list of supported parameters for reading
//

void Node::OpenParamList(DWORD *pdwNumParams, NXC_AGENT_PARAM **ppParamList)
{
   LockData();
   *pdwNumParams = m_dwNumParams;
   *ppParamList = m_pParamList;
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

      pConn = CreateAgentConnection();
      if (pConn != NULL)
      {
         dwError = pConn->CheckNetworkService(pdwStatus, dwIpAddr, iServiceType,
                                              wPort, wProto, pszRequest, pszResponse);
         pConn->Disconnect();
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
   if (dwObjectId == m_dwPollerNode)
   {
      // If deleted object is our poller node, change it to default
      /* LOCK? */
      m_dwPollerNode = 0;
      Modify();
      DbgPrintf(AF_DEBUG_MISC, _T("Node \"%s\": poller node %d deleted"), m_szName, dwObjectId);
   }
}


//
// Check node for OSPF support
//

void Node::CheckOSPFSupport(void)
{
   LONG nAdminStatus;

   if (SnmpGet(m_iSNMPVersion, m_dwIpAddr, m_wSNMPPort, m_szCommunityString,
               ".1.3.6.1.2.1.14.1.2.0", NULL, 0, &nAdminStatus, sizeof(LONG),
               FALSE, FALSE) == SNMP_ERR_SUCCESS)
   {
      if (nAdminStatus)
      {
         m_dwFlags |= NF_IS_OSPF;
      }
      else
      {
         m_dwFlags &= ~NF_IS_OSPF;
      }
   }
}


//
// Create ready to use agent connection
//

AgentConnection *Node::CreateAgentConnection(void)
{
   AgentConnection *pConn;

   if ((!(m_dwFlags & NF_IS_NATIVE_AGENT)) ||
       (m_dwDynamicFlags & NDF_AGENT_UNREACHABLE) ||
       (m_dwDynamicFlags & NDF_UNREACHABLE))
      return NULL;

   pConn = new AgentConnection(htonl(m_dwIpAddr), m_wAgentPort,
                               m_wAuthMethod, m_szSharedSecret);
   SetAgentProxy(pConn);
   if (!pConn->Connect(g_pServerKey))
   {
      delete pConn;
      pConn = NULL;
   }
   return pConn;
}


//
// Get last collected values of all DCIs
//

DWORD Node::GetLastValues(CSCPMessage *pMsg)
{
   DWORD i, dwId;

   LockData();

   pMsg->SetVariable(VID_NUM_ITEMS, m_dwNumItems);
   for(i = 0, dwId = VID_DCI_VALUES_BASE; i < m_dwNumItems; i++, dwId += 7)
      m_ppItems[i]->GetLastValue(pMsg, dwId);

   UnlockData();
   return RCC_SUCCESS;
}


//
// Clean expired DCI data
//

void Node::CleanDCIData(void)
{
   DWORD i;

   LockData();
   for(i = 0; i < m_dwNumItems; i++)
      m_ppItems[i]->CleanData();
   UnlockData();
}


//
// Apply DCI from template
// pItem passed to this method should be a template's DCI
//

BOOL Node::ApplyTemplateItem(DWORD dwTemplateId, DCItem *pItem)
{
   BOOL bResult = TRUE;
   DWORD i;
   DCItem *pNewItem;

   LockData();

   DbgPrintf(AF_DEBUG_DC, "Applying item \"%s\" to node \"%s\"", pItem->Name(), m_szName);

   // Check if that template item exists
   for(i = 0; i < m_dwNumItems; i++)
      if ((m_ppItems[i]->TemplateId() == dwTemplateId) &&
          (m_ppItems[i]->TemplateItemId() == pItem->Id()))
         break;   // Item with specified id already exist

   if (i == m_dwNumItems)
   {
      // New item from template, just add it
      pNewItem = new DCItem(pItem);
      pNewItem->SetTemplateId(dwTemplateId, pItem->Id());
      pNewItem->ChangeBinding(CreateUniqueId(IDG_ITEM), this);
      bResult = AddItem(pNewItem, TRUE);
   }
   else
   {
      // Update existing item
      m_ppItems[i]->UpdateFromTemplate(pItem);
      Modify();
   }

   UnlockData();
   return bResult;
}


//
// Clean deleted template items from node's DCI list
// Arguments is template id and list of valid template item ids.
// all items related to given template and not presented in list should be deleted.
//

void Node::CleanDeletedTemplateItems(DWORD dwTemplateId, DWORD dwNumItems, DWORD *pdwItemList)
{
   DWORD i, j, dwNumDeleted, *pdwDeleteList;

   pdwDeleteList = (DWORD *)malloc(sizeof(DWORD) * m_dwNumItems);
   dwNumDeleted = 0;

   LockData();

   for(i = 0; i < m_dwNumItems; i++)
      if (m_ppItems[i]->TemplateId() == dwTemplateId)
      {
         for(j = 0; j < dwNumItems; j++)
            if (m_ppItems[i]->TemplateItemId() == pdwItemList[j])
               break;

         // Delete DCI if it's not in list
         if (j == dwNumItems)
            pdwDeleteList[dwNumDeleted++] = m_ppItems[i]->Id();
      }

   UnlockData();

   for(i = 0; i < dwNumDeleted; i++)
      DeleteItem(pdwDeleteList[i]);
   free(pdwDeleteList);
}


//
// Unbind node from template, i.e either remove DCI association with template
// or remove these DCIs at all
//

void Node::UnbindFromTemplate(DWORD dwTemplateId, BOOL bRemoveDCI)
{
   DWORD i;

   if (bRemoveDCI)
   {
      DWORD dwNumDeleted, *pdwDeleteList;

      pdwDeleteList = (DWORD *)malloc(sizeof(DWORD) * m_dwNumItems);
      dwNumDeleted = 0;

      LockData();

      for(i = 0; i < m_dwNumItems; i++)
         if (m_ppItems[i]->TemplateId() == dwTemplateId)
         {
            pdwDeleteList[dwNumDeleted++] = m_ppItems[i]->Id();
         }

      UnlockData();

      for(i = 0; i < dwNumDeleted; i++)
         DeleteItem(pdwDeleteList[i]);
      free(pdwDeleteList);
   }
   else
   {
      LockData();

      for(i = 0; i < m_dwNumItems; i++)
         if (m_ppItems[i]->TemplateId() == dwTemplateId)
         {
            m_ppItems[i]->SetTemplateId(0, 0);
         }

      UnlockData();
   }
}


//
// Change node's IP address
//

void Node::ChangeIPAddress(DWORD dwIpAddr)
{
   DWORD i;

   PollerLock();

   LockData();

   UpdateNodeIndex(m_dwIpAddr, dwIpAddr, this);
   m_dwIpAddr = dwIpAddr;
   m_dwDynamicFlags |= NDF_FORCE_CONFIGURATION_POLL | NDF_RECHECK_CAPABILITIES;

   // Change status of node and all it's childs to UNKNOWN
   m_iStatus = STATUS_UNKNOWN;
   LockChildList(FALSE);
   for(i = 0; i < m_dwChildCount; i++)
   {
      m_pChildList[i]->ResetStatus();
      if (m_pChildList[i]->Type() == OBJECT_INTERFACE)
      {
         if (((Interface *)m_pChildList[i])->IsFake())
         {
            ((Interface *)m_pChildList[i])->SetIpAddr(dwIpAddr);
         }
      }
   }
   UnlockChildList();

   Modify();
   UnlockData();

   AgentLock();
   delete_and_null(m_pAgentConnection);
   AgentUnlock();

   PollerUnlock();
}


//
// Get number of interface objects and pointer to the last one
//

DWORD Node::GetInterfaceCount(Interface **ppInterface)
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

void Node::UpdateDCICache(void)
{
   DWORD i;

   /* LOCK? */
   for(i = 0; i < m_dwNumItems; i++)
      m_ppItems[i]->UpdateCacheSize();
}


//
// Get routing table from node
//

ROUTING_TABLE *Node::GetRoutingTable(void)
{
   ROUTING_TABLE *pRT = NULL;

   if ((m_dwFlags & NF_IS_NATIVE_AGENT) && (!(m_dwFlags & NF_DISABLE_NXCP)))
   {
      AgentLock();
      if (ConnectToAgent())
      {
         pRT = m_pAgentConnection->GetRoutingTable();
      }
      AgentUnlock();
   }
   if ((pRT == NULL) && (m_dwFlags & NF_IS_SNMP) && (!(m_dwFlags & NF_DISABLE_SNMP)))
   {
      pRT = SnmpGetRoutingTable(m_iSNMPVersion, m_dwIpAddr, m_wSNMPPort, m_szCommunityString);
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

BOOL Node::GetNextHop(DWORD dwSrcAddr, DWORD dwDestAddr, DWORD *pdwNextHop,
                      DWORD *pdwIfIndex, BOOL *pbIsVPN)
{
   DWORD i;
   BOOL bResult = FALSE;

   // Check VPN connectors
   LockChildList(FALSE);
   for(i = 0; i < m_dwChildCount; i++)
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
   UnlockChildList();

   // Check routing table
   if (!bResult)
   {
      RTLock();
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
      RTUnlock();
   }

   return bResult;
}


//
// Update cached routing table
//

void Node::UpdateRoutingTable(void)
{
   ROUTING_TABLE *pRT;

   pRT = GetRoutingTable();
   if (pRT != NULL)
   {
      RTLock();
      DestroyRoutingTable(m_pRoutingTable);
      m_pRoutingTable = pRT;
      RTUnlock();
   }
   m_tLastRTUpdate = time(NULL);
   m_dwDynamicFlags &= ~NDF_QUEUED_FOR_ROUTE_POLL;
}


//
// Call SNMP Enumerate with node's SNMP parameters
//

DWORD Node::CallSnmpEnumerate(char *pszRootOid, 
                              DWORD (* pHandler)(DWORD, DWORD, WORD, const char *, SNMP_Variable *, SNMP_Transport *, void *),
                              void *pArg)
{
   if ((m_dwFlags & NF_IS_SNMP) && 
       (!(m_dwDynamicFlags & NDF_SNMP_UNREACHABLE)) &&
       (!(m_dwDynamicFlags & NDF_UNREACHABLE)))
      return SnmpEnumerate(m_iSNMPVersion, m_dwIpAddr, m_wSNMPPort,
                           m_szCommunityString, pszRootOid, pHandler, pArg, FALSE);
   else
      return SNMP_ERR_COMM;
}


//
// Set proxy information for agent's connection
//

void Node::SetAgentProxy(AgentConnection *pConn)
{
   if (m_dwProxyNode != 0)
   {
      Node *pNode;

      pNode = (Node *)FindObjectById(m_dwProxyNode);
      if (pNode != NULL)
      {
         pConn->SetProxy(htonl(pNode->m_dwIpAddr), pNode->m_wAgentPort,
                         pNode->m_wAuthMethod, pNode->m_szSharedSecret);
      }
   }
}


//
// Prepare node object for deletion
//

void Node::PrepareForDeletion(void)
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
}


//
// Check if specified SNMP variable set to specified value.
// If variable doesn't exist at all, will return FALSE
//

BOOL Node::CheckSNMPIntegerValue(char *pszOID, int nValue)
{
   DWORD dwTemp;

   if (SnmpGet(m_iSNMPVersion, m_dwIpAddr, m_wSNMPPort, m_szCommunityString,
               pszOID, NULL, 0, &dwTemp, sizeof(DWORD), FALSE, FALSE) == SNMP_ERR_SUCCESS)
      return (int)dwTemp == nValue;
   return FALSE;
}


//
// Check and update if needed interface names
//

void Node::CheckInterfaceNames(INTERFACE_LIST *pIfList)
{
   int i;
   TCHAR *ptr;

   if (m_dwNodeType == NODE_TYPE_NORTEL_BAYSTACK)
   {
      // Translate interface names
      for(i = 0; i < pIfList->iNumEntries; i++)
      {
         if ((_tcsstr(pIfList->pInterfaces[i].szName, _T("BayStack")) != NULL) ||
             (_tcsstr(pIfList->pInterfaces[i].szName, _T("Nortel Ethernet Switch")) != NULL))
         {
            ptr = _tcsrchr(pIfList->pInterfaces[i].szName, _T('-'));
            if (ptr != NULL)
            {
               ptr++;
               while(*ptr == _T(' '))
                  ptr++;
               memmove(pIfList->pInterfaces[i].szName, ptr, _tcslen(ptr) + 1);
            }
         }
      }
   }

   // Cut interface names to MAX_OBJECT_NAME and check for unnamed interfaces
   for(i = 0; i < pIfList->iNumEntries; i++)
   {
      pIfList->pInterfaces[i].szName[MAX_OBJECT_NAME - 1] = 0;
      if (pIfList->pInterfaces[i].szName[0] == 0)
         _stprintf(pIfList->pInterfaces[i].szName, _T("%d"), pIfList->pInterfaces[i].dwIndex);
   }
}
