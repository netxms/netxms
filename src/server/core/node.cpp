/* 
** NetXMS - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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

#include "nms_core.h"


//
// Node class default constructor
//

Node::Node()
     :NetObj()
{
   m_dwFlags = 0;
   m_dwDiscoveryFlags = 0;
   m_wAgentPort = AGENT_LISTEN_PORT;
   m_wAuthMethod = AUTH_NONE;
   m_szSharedSecret[0] = 0;
   m_iStatusPollType = POLL_ICMP_PING;
   m_iSNMPVersion = SNMP_VERSION_1;
   strcpy(m_szCommunityString, "public");
   m_szObjectId[0] = 0;
   m_tLastDiscoveryPoll = 0;
   m_tLastStatusPoll = 0;
   m_tLastConfigurationPoll = 0;
   m_iSnmpAgentFails = 0;
   m_iNativeAgentFails = 0;
   m_hPollerMutex = MutexCreate();
   m_hAgentAccessMutex = MutexCreate();
   m_dwNumItems = 0;
   m_ppItems = NULL;
   m_pAgentConnection = NULL;
   m_dwDCILockStatus = INVALID_INDEX;
}


//
// Constructor for new node object
//

Node::Node(DWORD dwAddr, DWORD dwFlags, DWORD dwDiscoveryFlags)
     :NetObj()
{
   m_dwIpAddr = dwAddr;
   m_dwFlags = dwFlags;
   m_dwDiscoveryFlags = dwDiscoveryFlags;
   m_wAgentPort = AGENT_LISTEN_PORT;
   m_wAuthMethod = AUTH_NONE;
   m_szSharedSecret[0] = 0;
   m_iStatusPollType = POLL_ICMP_PING;
   m_iSNMPVersion = SNMP_VERSION_1;
   strcpy(m_szCommunityString, "public");
   IpToStr(dwAddr, m_szName);    // Make default name from IP address
   m_szObjectId[0] = 0;
   m_tLastDiscoveryPoll = 0;
   m_tLastStatusPoll = 0;
   m_tLastConfigurationPoll = 0;
   m_iSnmpAgentFails = 0;
   m_iNativeAgentFails = 0;
   m_hPollerMutex = MutexCreate();
   m_hAgentAccessMutex = MutexCreate();
   m_dwNumItems = 0;
   m_ppItems = NULL;
   m_pAgentConnection = NULL;
   m_dwDCILockStatus = INVALID_INDEX;
}


//
// Node destructor
//

Node::~Node()
{
   MutexDestroy(m_hPollerMutex);
   MutexDestroy(m_hAgentAccessMutex);
   DestroyItems();
   if (m_pAgentConnection != NULL)
      delete m_pAgentConnection;
}


//
// Destroy all related data collection items
//

void Node::DestroyItems(void)
{
   DWORD i;

   for(i = 0; i < m_dwNumItems; i++)
      delete m_ppItems[i];
   safe_free(m_ppItems);
   m_dwNumItems = 0;
   m_ppItems = NULL;
}


//
// Create object from database data
//

BOOL Node::CreateFromDB(DWORD dwId)
{
   char szQuery[256];
   DB_RESULT hResult;
   int i, iNumRows;
   DWORD dwSubnetId;
   NetObj *pObject;
   BOOL bResult = FALSE;

   sprintf(szQuery, "SELECT id,name,status,primary_ip,is_snmp,is_agent,is_bridge,"
                    "is_router,snmp_version,discovery_flags,auth_method,secret,"
                    "agent_port,status_poll_type,community,snmp_oid,is_local_mgmt,"
                    "image_id,is_deleted FROM nodes WHERE id=%d", dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult == 0)
      return FALSE;     // Query failed

   if (DBGetNumRows(hResult) == 0)
   {
      DBFreeResult(hResult);
      return FALSE;
   }

   m_dwId = dwId;
   strncpy(m_szName, DBGetField(hResult, 0, 1), MAX_OBJECT_NAME);
   m_iStatus = DBGetFieldLong(hResult, 0, 2);
   m_dwIpAddr = DBGetFieldULong(hResult, 0, 3);

   // Flags
   if (DBGetFieldLong(hResult, 0, 4))
      m_dwFlags |= NF_IS_SNMP;
   if (DBGetFieldLong(hResult, 0, 5))
      m_dwFlags |= NF_IS_NATIVE_AGENT;
   if (DBGetFieldLong(hResult, 0, 6))
      m_dwFlags |= NF_IS_BRIDGE;
   if (DBGetFieldLong(hResult, 0, 7))
      m_dwFlags |= NF_IS_ROUTER;
   if (DBGetFieldLong(hResult, 0, 16))
      m_dwFlags |= NF_IS_LOCAL_MGMT;

   m_iSNMPVersion = DBGetFieldLong(hResult, 0, 8);
   m_dwDiscoveryFlags = DBGetFieldULong(hResult, 0, 9);
   m_wAuthMethod = (WORD)DBGetFieldLong(hResult, 0, 10);
   strncpy(m_szSharedSecret, DBGetField(hResult, 0, 11), MAX_SECRET_LENGTH);
   m_wAgentPort = (WORD)DBGetFieldLong(hResult, 0, 12);
   m_iStatusPollType = DBGetFieldLong(hResult, 0, 13);
   strncpy(m_szCommunityString, DBGetField(hResult, 0, 14), MAX_COMMUNITY_LENGTH);
   strncpy(m_szObjectId, DBGetField(hResult, 0, 15), MAX_OID_LEN * 4);
   m_dwImageId = DBGetFieldULong(hResult, 0, 17);
   m_bIsDeleted = DBGetFieldLong(hResult, 0, 18);

   DBFreeResult(hResult);

   if (!m_bIsDeleted)
   {
      // Link node to subnets
      sprintf(szQuery, "SELECT subnet_id FROM nsmap WHERE node_id=%d", dwId);
      hResult = DBSelect(g_hCoreDB, szQuery);
      if (hResult == NULL)
         return FALSE;     // Query failed

      if (DBGetNumRows(hResult) == 0)
      {
         DBFreeResult(hResult);
         return FALSE;     // No parents - it shouldn't happen if database isn't corrupted
      }

      iNumRows = DBGetNumRows(hResult);
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
            bResult = FALSE;
   }
   else
   {
      bResult = TRUE;
   }

   return bResult;
}


//
// Load data collection items from database
//

void Node::LoadItemsFromDB(void)
{
   char szQuery[256];
   DB_RESULT hResult;

   sprintf(szQuery, "SELECT item_id,name,source,datatype,polling_interval,retention_time,"
                    "status,delta_calculation,transformation,template_id,description "
                    "FROM items WHERE node_id=%d", m_dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);

   if (hResult != 0)
   {
      int i, iRows;

      iRows = DBGetNumRows(hResult);
      if (iRows > 0)
      {
         m_dwNumItems = iRows;
         m_ppItems = (DCItem **)malloc(sizeof(DCItem *) * iRows);
         for(i = 0; i < iRows; i++)
            m_ppItems[i] = new DCItem(hResult, i, this);
      }
      DBFreeResult(hResult);
   }
}


//
// Save object to database
//

BOOL Node::SaveToDB(void)
{
   char szQuery[4096];
   DB_RESULT hResult;
   BOOL bNewObject = TRUE;
   BOOL bResult;

   // Lock object's access
   Lock();

   // Check for object's existence in database
   sprintf(szQuery, "SELECT id FROM nodes WHERE id=%ld", m_dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != 0)
   {
      if (DBGetNumRows(hResult) > 0)
         bNewObject = FALSE;
      DBFreeResult(hResult);
   }

   // Form and execute INSERT or UPDATE query
   if (bNewObject)
      sprintf(szQuery, "INSERT INTO nodes (id,name,status,is_deleted,primary_ip,"
                       "is_snmp,is_agent,is_bridge,is_router,snmp_version,community,"
                       "discovery_flags,status_poll_type,agent_port,auth_method,secret,"
                       "snmp_oid,is_local_mgmt,image_id)"
                       " VALUES (%d,'%s',%d,%d,%d,%d,%d,%d,%d,%d,'%s',%d,%d,%d,%d,'%s','%s',%d,%ld)",
              m_dwId, m_szName, m_iStatus, m_bIsDeleted, m_dwIpAddr, 
              m_dwFlags & NF_IS_SNMP ? 1 : 0,
              m_dwFlags & NF_IS_NATIVE_AGENT ? 1 : 0,
              m_dwFlags & NF_IS_BRIDGE ? 1 : 0,
              m_dwFlags & NF_IS_ROUTER ? 1 : 0,
              m_iSNMPVersion, m_szCommunityString, m_dwDiscoveryFlags, m_iStatusPollType,
              m_wAgentPort,m_wAuthMethod,m_szSharedSecret, m_szObjectId,
              m_dwFlags & NF_IS_LOCAL_MGMT ? 1 : 0, m_dwImageId);
   else
      sprintf(szQuery, "UPDATE nodes SET name='%s',status=%d,is_deleted=%d,primary_ip=%d,"
                       "is_snmp=%d,is_agent=%d,is_bridge=%d,is_router=%d,snmp_version=%d,"
                       "community='%s',discovery_flags=%d,status_poll_type=%d,agent_port=%d,"
                       "auth_method=%d,secret='%s',snmp_oid='%s',is_local_mgmt=%d,image_id=%ld"
                       " WHERE id=%ld",
              m_szName, m_iStatus, m_bIsDeleted, m_dwIpAddr, 
              m_dwFlags & NF_IS_SNMP ? 1 : 0,
              m_dwFlags & NF_IS_NATIVE_AGENT ? 1 : 0,
              m_dwFlags & NF_IS_BRIDGE ? 1 : 0,
              m_dwFlags & NF_IS_ROUTER ? 1 : 0,
              m_iSNMPVersion, m_szCommunityString, m_dwDiscoveryFlags, 
              m_iStatusPollType, m_wAgentPort, m_wAuthMethod, m_szSharedSecret, 
              m_szObjectId, m_dwFlags & NF_IS_LOCAL_MGMT ? 1 : 0, m_dwImageId, m_dwId);
   bResult = DBQuery(g_hCoreDB, szQuery);

   // Save data collection items
   if (bResult)
   {
      DWORD i;

      for(i = 0; i < m_dwNumItems; i++)
         m_ppItems[i]->SaveToDB();
   }

   // Save access list
   SaveACLToDB();

   // Clear modifications flag and unlock object
   m_bIsModified = FALSE;
   Unlock();

   return bResult;
}


//
// Delete object from database
//

BOOL Node::DeleteFromDB(void)
{
   char szQuery[256];
   BOOL bSuccess;

   bSuccess = NetObj::DeleteFromDB();
   if (bSuccess)
   {
      sprintf(szQuery, "DELETE FROM nodes WHERE id=%ld", m_dwId);
      QueueSQLRequest(szQuery);
      sprintf(szQuery, "DELETE FROM nsmap WHERE node_id=%ld", m_dwId);
      QueueSQLRequest(szQuery);
      sprintf(szQuery, "DROP TABLE idata_%ld", m_dwId);
      QueueSQLRequest(szQuery);
   }
   return bSuccess;
}


//
// Poll newly discovered node
// Usually called once by node poller thread when new node is discovered
// and object for it is created
//

BOOL Node::NewNodePoll(DWORD dwNetMask)
{
   AgentConnection *pAgentConn;

   PollerLock();

   // Determine node's capabilities
   if (SnmpGet(m_dwIpAddr, m_szCommunityString, ".1.3.6.1.2.1.1.2.0", NULL, 0,
               m_szObjectId, MAX_OID_LEN * 4, FALSE, FALSE))
      m_dwFlags |= NF_IS_SNMP;

   pAgentConn = new AgentConnection(m_dwIpAddr, m_wAgentPort, m_wAuthMethod,
                                    m_szSharedSecret);
   if (pAgentConn->Connect())
      m_dwFlags |= NF_IS_NATIVE_AGENT;

   // Get interface list
   if ((m_dwFlags & NF_IS_SNMP) || (m_dwFlags & NF_IS_NATIVE_AGENT) ||
       (m_dwFlags & NF_IS_LOCAL_MGMT))
   {
      INTERFACE_LIST *pIfList = NULL;
      int i;

      if (m_dwFlags & NF_IS_LOCAL_MGMT)    // For local machine
         pIfList = GetLocalInterfaceList();
      else if (m_dwFlags & NF_IS_NATIVE_AGENT)    // For others prefer native agent
         pIfList = pAgentConn->GetInterfaceList();
      if ((pIfList == NULL) && (m_dwFlags & NF_IS_SNMP))  // Use SNMP if we cannot get interfaces via agent
         pIfList = SnmpGetInterfaceList(m_dwIpAddr, m_szCommunityString);

      if (pIfList != NULL)
      {
         for(i = 0; i < pIfList->iNumEntries; i++)
            CreateNewInterface(pIfList->pInterfaces[i].dwIpAddr, 
                               pIfList->pInterfaces[i].dwIpNetMask,
                               pIfList->pInterfaces[i].szName,
                               pIfList->pInterfaces[i].dwIndex,
                               pIfList->pInterfaces[i].dwType);
         DestroyInterfaceList(pIfList);
      }
      else
      {
         // We cannot get interface list from node for some reasons, create dummy one
         CreateNewInterface(m_dwIpAddr, dwNetMask);
      }
   }
   else  // No SNMP, no native agent - create pseudo interface object
   {
      CreateNewInterface(m_dwIpAddr, dwNetMask);
   }

   // Clean up agent connection
   if (m_dwFlags & NF_IS_NATIVE_AGENT)
      pAgentConn->Disconnect();
   delete pAgentConn;

   PollerUnlock();

   return TRUE;
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
      pArpCache = SnmpGetArpCache(m_dwIpAddr, m_szCommunityString);
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
   else if (m_dwFlags & NF_IS_NATIVE_AGENT)
   {
      AgentLock();
      if (ConnectToAgent())
      {
         CleanInterfaceList(pIfList);
         pIfList = m_pAgentConnection->GetInterfaceList();
      }
      AgentUnlock();
   }
   else if (m_dwFlags & NF_IS_SNMP)
   {
      pIfList = SnmpGetInterfaceList(m_dwIpAddr, m_szCommunityString);
   }

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

   for(i = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i]->Type() == OBJECT_INTERFACE)
      {
         pInterface = (Interface *)m_pChildList[i];
         if (pInterface->IfIndex() == dwIndex)
            if ((pInterface->IpAddr() & pInterface->IpNetMask()) ==
                (dwHostAddr & pInterface->IpNetMask()))
               return pInterface;
      }
   return NULL;
}


//
// Create new interface
//

void Node::CreateNewInterface(DWORD dwIpAddr, DWORD dwNetMask, char *szName, DWORD dwIndex, DWORD dwType)
{
   Interface *pInterface;
   Subnet *pSubnet;

   // Create interface object
   if (szName != NULL)
      pInterface = new Interface(szName, dwIndex, dwIpAddr, dwNetMask, dwType);
   else
      pInterface = new Interface(dwIpAddr, dwNetMask);

   // Insert to objects' list and generate event
   NetObjInsert(pInterface, TRUE);
   AddInterface(pInterface);
   PostEvent(EVENT_INTERFACE_ADDED, m_dwId, "dsaad", pInterface->Id(),
             pInterface->Name(), pInterface->IpAddr(),
             pInterface->IpNetMask(), pInterface->IfIndex());

   // Bind node to appropriate subnet
   if (pInterface->IpAddr() != 0)   // Do not link non-IP interfaces to 0.0.0.0 subnet
   {
      pSubnet = FindSubnetByIP(pInterface->IpAddr() & pInterface->IpNetMask());
      if (pSubnet == NULL)
      {
         // Create new subnet object
         pSubnet = new Subnet(pInterface->IpAddr() & pInterface->IpNetMask(), pInterface->IpNetMask());
         NetObjInsert(pSubnet, TRUE);
         g_pEntireNet->AddSubnet(pSubnet);
      }
      pSubnet->AddNode(this);
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
      for(i = 0; i < m_dwChildCount; i++)
         if (m_pChildList[i]->Type() == OBJECT_INTERFACE)
            if (m_pChildList[i] != pInterface)
               if ((((Interface *)m_pChildList[i])->IpAddr() & ((Interface *)m_pChildList[i])->IpNetMask()) ==
                   (pInterface->IpAddr() & pInterface->IpNetMask()))
                  break;
      if (i == m_dwChildCount)
      {
         // Last interface in subnet, should unlink node
         Subnet *pSubnet = FindSubnetByIP(pInterface->IpAddr() & pInterface->IpNetMask());
         DeleteParent(pSubnet);
         if (pSubnet != NULL)
         {
            pSubnet->DeleteChild(this);
            if ((pSubnet->IsEmpty()) && (g_dwFlags & AF_DELETE_EMPTY_SUBNETS))
            {
               PostEvent(EVENT_SUBNET_DELETED, pSubnet->Id(), NULL);
               pSubnet->Delete();
            }
         }
      }
   }
   pInterface->Delete();
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

void Node::StatusPoll(ClientSession *pSession, DWORD dwRqId)
{
   DWORD i;

   PollerLock();
   m_pPollRequestor = pSession;
   SendPollerMsg(dwRqId, _T("Starting status poll for node %s\r\n"), m_szName);
   for(i = 0; i < m_dwChildCount; i++)
      if ((m_pChildList[i]->Type() == OBJECT_INTERFACE) &&
          (m_pChildList[i]->Status() != STATUS_UNMANAGED))
         ((Interface *)m_pChildList[i])->StatusPoll(pSession, dwRqId);
   CalculateCompoundStatus();
   m_tLastStatusPoll = time(NULL);
   SendPollerMsg(dwRqId, "Finished status poll for node %s\r\n"
                         "Node status after poll is %s\r\n", m_szName, g_pszStatusName[m_iStatus]);
   m_pPollRequestor = NULL;
   PollerUnlock();
}


//
// Perform configuration poll on node
//

void Node::ConfigurationPoll(ClientSession *pSession, DWORD dwRqId)
{
   DWORD dwOldFlags = m_dwFlags;
   AgentConnection *pAgentConn;
   INTERFACE_LIST *pIfList;
   BOOL bHasChanges = FALSE;

   PollerLock();
   m_pPollRequestor = pSession;
   SendPollerMsg(dwRqId, _T("Starting status poll for node %s\r\n"), m_szName);
   DbgPrintf(AF_DEBUG_DISCOVERY, "Starting configuration poll for node %s (ID: %d)", m_szName, m_dwId);

   // Check node's capabilities
   SendPollerMsg(dwRqId, _T("Checking node's capabilities...\r\n"));
   if (SnmpGet(m_dwIpAddr, m_szCommunityString, ".1.3.6.1.2.1.1.2.0", NULL, 0,
               m_szObjectId, MAX_OID_LEN * 4, FALSE, FALSE))
   {
      m_dwFlags |= NF_IS_SNMP;
      m_iSnmpAgentFails = 0;
      SendPollerMsg(dwRqId, _T("   SNMP agent is active\r\n"));
   }
   else
   {
      if (m_dwFlags & NF_IS_SNMP)
      {
         if (m_iSnmpAgentFails == 0)
            PostEvent(EVENT_SNMP_FAIL, m_dwId, NULL);
         m_iSnmpAgentFails++;
      }
      SendPollerMsg(dwRqId, _T("   SNMP agent is not responding\r\n"));
   }

   pAgentConn = new AgentConnection(m_dwIpAddr, m_wAgentPort, m_wAuthMethod, m_szSharedSecret);
   if (pAgentConn->Connect())
   {
      m_dwFlags |= NF_IS_NATIVE_AGENT;
      m_iNativeAgentFails = 0;
      pAgentConn->Disconnect();
      SendPollerMsg(dwRqId, _T("   NetXMS native agent is active\r\n"));
   }
   else
   {
      if (m_dwFlags & NF_IS_NATIVE_AGENT)
      {
         if (m_iNativeAgentFails == 0)
            PostEvent(EVENT_AGENT_FAIL, m_dwId, NULL);
         m_iNativeAgentFails++;
      }
      SendPollerMsg(dwRqId, _T("   NetXMS native agent is not responding\r\n"));
   }
   delete pAgentConn;

   // Generate event if node flags has been changed
   if (dwOldFlags != m_dwFlags)
   {
      PostEvent(EVENT_NODE_FLAGS_CHANGED, m_dwId, "xx", dwOldFlags, m_dwFlags);
      bHasChanges = TRUE;
   }

   // Retrieve interface list
   SendPollerMsg(dwRqId, _T("Capability check finished\r\n"
                            "Checking interface configuration...\r\n"));
   pIfList = GetInterfaceList();
   if (pIfList != NULL)
   {
      DWORD i;
      int j;

      // Find non-existing interfaces
      for(i = 0; i < m_dwChildCount; i++)
         if (m_pChildList[i]->Type() == OBJECT_INTERFACE)
         {
            Interface *pInterface = (Interface *)m_pChildList[i];

            for(j = 0; j < pIfList->iNumEntries; j++)
               if ((pIfList->pInterfaces[j].dwIndex == pInterface->IfIndex()) &&
                   (pIfList->pInterfaces[j].dwIpAddr == pInterface->IpAddr()) &&
                   (pIfList->pInterfaces[j].dwIpNetMask == pInterface->IpNetMask()))
                  break;
            if (j == pIfList->iNumEntries)
            {
               // No such interface in current configuration, delete it
               SendPollerMsg(dwRqId, _T("   Interface \"%s\" is no longer exist\r\n"), 
                             pInterface->Name());
               PostEvent(EVENT_INTERFACE_DELETED, m_dwId, "dsaa", pInterface->IfIndex(),
                         pInterface->Name(), pInterface->IpAddr(), pInterface->IpNetMask());
               DeleteInterface(pInterface);
               i = 0;   // Restart loop
               bHasChanges = TRUE;
            }
         }

      // Add new interfaces
      for(j = 0; j < pIfList->iNumEntries; j++)
      {
         for(i = 0; i < m_dwChildCount; i++)
            if (m_pChildList[i]->Type() == OBJECT_INTERFACE)
            {
               Interface *pInterface = (Interface *)m_pChildList[i];

               if ((pIfList->pInterfaces[j].dwIndex == pInterface->IfIndex()) &&
                   (pIfList->pInterfaces[j].dwIpAddr == pInterface->IpAddr()) &&
                   (pIfList->pInterfaces[j].dwIpNetMask == pInterface->IpNetMask()))
                  break;
            }
         if (i == m_dwChildCount)
         {
            // New interface
            SendPollerMsg(dwRqId, _T("   Found new interface \"%s\"\r\n"), 
                          pIfList->pInterfaces[j].szName);
            CreateNewInterface(pIfList->pInterfaces[j].dwIpAddr, 
                               pIfList->pInterfaces[j].dwIpNetMask,
                               pIfList->pInterfaces[j].szName,
                               pIfList->pInterfaces[j].dwIndex,
                               pIfList->pInterfaces[j].dwType);
            bHasChanges = TRUE;
         }
      }

      DestroyInterfaceList(pIfList);
   }

   m_tLastConfigurationPoll = time(NULL);
   SendPollerMsg(dwRqId, _T("Interface configuration check finished\r\n"
                            "Finished configuration poll for node %s\r\n"
                            "Node configuration was%schanged after poll\r\n"),
                 m_szName, bHasChanges ? _T(" ") : _T(" not "));
   PollerUnlock();
   DbgPrintf(AF_DEBUG_DISCOVERY, "Finished configuration poll for node %s (ID: %d)", m_szName, m_dwId);

   if (bHasChanges)
   {
      Lock();
      Modify();
      Unlock();
   }
}


//
// Connect to native agent
//

BOOL Node::ConnectToAgent(void)
{
   // Create new agent connection object if needed
   if (m_pAgentConnection == NULL)
      m_pAgentConnection = new AgentConnection(m_dwIpAddr, m_wAgentPort, m_wAuthMethod, m_szSharedSecret);

   // Check if we already connected
   if (m_pAgentConnection->Nop() == ERR_SUCCESS)
      return TRUE;

   // Close current connection or clean up after broken connection
   m_pAgentConnection->Disconnect();
   return m_pAgentConnection->Connect();
}


//
// Get item's value via SNMP
//

DWORD Node::GetItemFromSNMP(const char *szParam, DWORD dwBufSize, char *szBuffer)
{
   return SnmpGet(m_dwIpAddr, m_szCommunityString, szParam, NULL, 0,
                  szBuffer, dwBufSize, FALSE, TRUE) ? DCE_SUCCESS : DCE_COMM_ERROR;
}


//
// Get item's value via native agent
//

DWORD Node::GetItemFromAgent(const char *szParam, DWORD dwBufSize, char *szBuffer)
{
   DWORD dwError, dwResult = DCE_COMM_ERROR;
   DWORD dwTries = 5;

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
   else
   {
      dwError = DCE_NOT_SUPPORTED;
   }

   return dwError;
}


//
// Put items which requires polling into the queue
//

void Node::QueueItemsForPolling(Queue *pPollerQueue)
{
   DWORD i;
   time_t currTime;

   currTime = time(NULL);

   Lock();
   for(i = 0; i < m_dwNumItems; i++)
   {
      if (m_ppItems[i]->ReadyForPolling(currTime))
      {
         m_ppItems[i]->SetBusyFlag(TRUE);
         pPollerQueue->Put(m_ppItems[i]);
      }
   }
   Unlock();
}


//
// Add item to node
//

BOOL Node::AddItem(DCItem *pItem)
{
   DWORD i;
   BOOL bResult = FALSE;

   Lock();
   // Check if that item exists
   for(i = 0; i < m_dwNumItems; i++)
      if (m_ppItems[i]->Id() == pItem->Id())
         break;   // Item with specified id already exist
   
   if (i == m_dwNumItems)     // Add new item
   {
      m_dwNumItems++;
      m_ppItems = (DCItem **)realloc(m_ppItems, sizeof(DCItem *) * m_dwNumItems);
      m_ppItems[i] = pItem;
      m_ppItems[i]->SetLastPollTime(0);    // Cause item to be polled immediatelly
      m_ppItems[i]->SetStatus(ITEM_STATUS_ACTIVE);
      m_ppItems[i]->SetBusyFlag(FALSE);
      Modify();
      bResult = TRUE;
   }

   Unlock();
   return bResult;
}


//
// Delete item from node
//

BOOL Node::DeleteItem(DWORD dwItemId)
{
   DWORD i;
   BOOL bResult = FALSE;

   Lock();
   // Check if that item exists
   for(i = 0; i < m_dwNumItems; i++)
      if (m_ppItems[i]->Id() == dwItemId)
      {
         // Destroy item
         m_ppItems[i]->DeleteFromDB();
         delete m_ppItems[i];
         m_dwNumItems--;
         memmove(&m_ppItems[i], &m_ppItems[i + 1], sizeof(DCItem *) * (m_dwNumItems - i));
         bResult = TRUE;
         break;
      }

   Unlock();
   return bResult;
}


//
// Modify data collection item from CSCP message
//

BOOL Node::UpdateItem(DWORD dwItemId, CSCPMessage *pMsg, DWORD *pdwNumMaps, 
                      DWORD **ppdwMapIndex, DWORD **ppdwMapId)
{
   DWORD i;
   BOOL bResult = FALSE;

   Lock();
   // Check if that item exists
   for(i = 0; i < m_dwNumItems; i++)
      if (m_ppItems[i]->Id() == dwItemId)
      {
         m_ppItems[i]->UpdateFromMessage(pMsg, pdwNumMaps, ppdwMapIndex, ppdwMapId);
         bResult = TRUE;
         m_bIsModified = TRUE;
         break;
      }

   Unlock();
   return bResult;
}


//
// Create CSCP message with object's data
//

void Node::CreateMessage(CSCPMessage *pMsg)
{
   NetObj::CreateMessage(pMsg);
   pMsg->SetVariable(VID_FLAGS, m_dwFlags);
   pMsg->SetVariable(VID_DISCOVERY_FLAGS, m_dwDiscoveryFlags);
   pMsg->SetVariable(VID_AGENT_PORT, m_wAgentPort);
   pMsg->SetVariable(VID_AUTH_METHOD, m_wAuthMethod);
   pMsg->SetVariable(VID_SHARED_SECRET, m_szSharedSecret);
   pMsg->SetVariable(VID_COMMUNITY_STRING, m_szCommunityString);
   pMsg->SetVariable(VID_SNMP_OID, m_szObjectId);
}


//
// Modify object from message
//

DWORD Node::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
   if (!bAlreadyLocked)
      Lock();

   // Change listen port of native agent
   if (pRequest->IsVariableExist(VID_AGENT_PORT))
      m_wAgentPort = pRequest->GetVariableShort(VID_AGENT_PORT);

   // Change authentication method of native agent
   if (pRequest->IsVariableExist(VID_AUTH_METHOD))
      m_wAuthMethod = pRequest->GetVariableShort(VID_AUTH_METHOD);

   // Change shared secret of native agent
   if (pRequest->IsVariableExist(VID_SHARED_SECRET))
      pRequest->GetVariableStr(VID_SHARED_SECRET, m_szSharedSecret, MAX_SECRET_LENGTH);

   // Change SNMP community string
   if (pRequest->IsVariableExist(VID_COMMUNITY_STRING))
      pRequest->GetVariableStr(VID_COMMUNITY_STRING, m_szCommunityString, MAX_COMMUNITY_LENGTH);

   return NetObj::ModifyFromMessage(pRequest, TRUE);
}


//
// Lock data collection items list
//

BOOL Node::LockDCIList(DWORD dwSessionId)
{
   BOOL bSuccess = FALSE;

   Lock();
   if (m_dwDCILockStatus == INVALID_INDEX)
   {
      m_dwDCILockStatus = dwSessionId;
      bSuccess = TRUE;
   }
   Unlock();
   return bSuccess;
}


//
// Unlock data collection items list
//

BOOL Node::UnlockDCIList(DWORD dwSessionId)
{
   BOOL bSuccess = FALSE;

   Lock();
   if (m_dwDCILockStatus == dwSessionId)
   {
      m_dwDCILockStatus = INVALID_INDEX;
      bSuccess = TRUE;
   }
   Unlock();
   return bSuccess;
}


//
// Send DCI list to client
//

void Node::SendItemsToClient(ClientSession *pSession, DWORD dwRqId)
{
   CSCPMessage msg;
   DWORD i;

   // Prepare message
   msg.SetId(dwRqId);
   msg.SetCode(CMD_NODE_DCI);

   // Walk through items list
   for(i = 0; i < m_dwNumItems; i++)
   {
      m_ppItems[i]->CreateMessage(&msg);
      pSession->SendMessage(&msg);
      msg.DeleteAllVariables();
   }

   // Send end-of-list indicator
   msg.SetCode(CMD_NODE_DCI_LIST_END);
   pSession->SendMessage(&msg);
}


//
// Get DCI item's type
//

int Node::GetItemType(DWORD dwItemId)
{
   DWORD i;
   int iType = -1;

   Lock();
   // Check if that item exists
   for(i = 0; i < m_dwNumItems; i++)
      if (m_ppItems[i]->Id() == dwItemId)
      {
         iType = m_ppItems[i]->DataType();
         break;
      }

   Unlock();
   return iType;
}
