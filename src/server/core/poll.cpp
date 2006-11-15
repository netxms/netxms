/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006 Victor Kirhenshtein
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
** File: poll.cpp
**
**/

#include "nxcore.h"


//
// Poller state structure
//

struct __poller_state
{
   int iType;
   time_t timestamp;
   char szMsg[128];
   char szInfo[128];
};


//
// Global data
//

Queue g_statusPollQueue;
Queue g_configPollQueue;
Queue g_routePollQueue;
Queue g_discoveryPollQueue;
Queue g_nodePollerQueue;
Queue g_conditionPollerQueue;


//
// Static data
//

static __poller_state *m_pPollerState = NULL;
static int m_iNumPollers = 0;
static DWORD m_dwNewNodeId = 1;


//
// Check if management server node presented in node list
//

void CheckForMgmtNode(void)
{
   INTERFACE_LIST *pIfList;
   Node *pNode;
   int i;

   pIfList = GetLocalInterfaceList();
   if (pIfList != NULL)
   {
      for(i = 0; i < pIfList->iNumEntries; i++)
         if ((pNode = FindNodeByIP(pIfList->pInterfaces[i].dwIpAddr)) != NULL)
         {
            // Check management node flag
            if (!(pNode->Flags() & NF_IS_LOCAL_MGMT))
            {
               pNode->SetLocalMgmtFlag();
               DbgPrintf(AF_DEBUG_OBJECTS, _T("Local management node %s [%d] was not have NF_IS_LOCAL_MGMT flag set"), pNode->Name(), pNode->Id());
            }
            g_dwMgmtNode = pNode->Id();   // Set local management node ID
            break;
         }
      if (i == pIfList->iNumEntries)   // No such node
      {
         // Find interface with IP address
         for(i = 0; i < pIfList->iNumEntries; i++)
            if (pIfList->pInterfaces[i].dwIpAddr != 0)
            {
               pNode = new Node(pIfList->pInterfaces[i].dwIpAddr, NF_IS_LOCAL_MGMT, 0, 0);
               NetObjInsert(pNode, TRUE);
               pNode->ConfigurationPoll(NULL, 0, -1, pIfList->pInterfaces[i].dwIpNetMask);
               pNode->Unhide();
               g_dwMgmtNode = pNode->Id();   // Set local management node ID
               PostEvent(EVENT_NODE_ADDED, pNode->Id(), NULL);
               
               // Add default data collection items
               pNode->AddItem(new DCItem(CreateUniqueId(IDG_ITEM), "Status", 
                                         DS_INTERNAL, DCI_DT_INT, 60, 30, pNode));
               pNode->AddItem(new DCItem(CreateUniqueId(IDG_ITEM), 
                                         "Server.AverageDCPollerQueueSize", 
                                         DS_INTERNAL, DCI_DT_FLOAT, 60, 30, pNode,
                                         "Average length of data collection poller's request queue for last minute"));
               pNode->AddItem(new DCItem(CreateUniqueId(IDG_ITEM), 
                                         "Server.AverageDBWriterQueueSize", 
                                         DS_INTERNAL, DCI_DT_FLOAT, 60, 30, pNode,
                                         "Average length of database writer's request queue for last minute"));
               pNode->AddItem(new DCItem(CreateUniqueId(IDG_ITEM), 
                                         "Server.AverageDCIQueuingTime", 
                                         DS_INTERNAL, DCI_DT_UINT, 60, 30, pNode,
                                         "Average time to queue DCI for polling for last minute"));
               pNode->AddItem(new DCItem(CreateUniqueId(IDG_ITEM), 
                                         "Server.AverageStatusPollerQueueSize", 
                                         DS_INTERNAL, DCI_DT_FLOAT, 60, 30, pNode,
                                         "Average length of status poller queue for last minute"));
               pNode->AddItem(new DCItem(CreateUniqueId(IDG_ITEM), 
                                         "Server.AverageConfigurationPollerQueueSize", 
                                         DS_INTERNAL, DCI_DT_FLOAT, 60, 30, pNode,
                                         "Average length of configuration poller queue for last minute"));
               break;
            }
      }
      DestroyInterfaceList(pIfList);
   }
}


//
// Set poller's state
//

static void SetPollerState(int nIdx, char *pszMsg)
{
   nx_strncpy(m_pPollerState[nIdx].szMsg, pszMsg, 128);
   m_pPollerState[nIdx].szInfo[0] = 0;
   m_pPollerState[nIdx].timestamp = time(NULL);
}


//
// Set poller's info
//

void SetPollerInfo(int nIdx, char *pszMsg)
{
   if (nIdx != -1)
   {
      nx_strncpy(m_pPollerState[nIdx].szInfo, pszMsg, 128);
      m_pPollerState[nIdx].timestamp = time(NULL);
   }
}


//
// Display current poller threads state
//

void ShowPollerState(CONSOLE_CTX pCtx)
{
   int i;
   char szTime[64];
   struct tm *ltm;

   ConsolePrintf(pCtx, "PT  TIME                   STATE\n");
   for(i = 0; i < m_iNumPollers; i++)
   {
      ltm = localtime(&m_pPollerState[i].timestamp);
      if (ltm != NULL)
         _tcsftime(szTime, 64, _T("%d/%b/%Y %H:%M:%S"), ltm);
      else
         _tcscpy(szTime, _T("<error>             "));
      if (m_pPollerState[i].szInfo[0] != 0)
         ConsolePrintf(pCtx, "%c   %s   %s - %s\n", m_pPollerState[i].iType, 
                       szTime, m_pPollerState[i].szMsg, m_pPollerState[i].szInfo);
      else
         ConsolePrintf(pCtx, "%c   %s   %s\n", m_pPollerState[i].iType, 
                       szTime, m_pPollerState[i].szMsg);
   }
   ConsolePrintf(pCtx, "\n");
}


//
// Status poll thread
//

static THREAD_RESULT THREAD_CALL StatusPoller(void *arg)
{
   Node *pNode;
   char szBuffer[MAX_OBJECT_NAME + 64];

   // Initialize state info
   m_pPollerState[(long)arg].iType = 'S';
   SetPollerState((long)arg, "init");

   // Main loop
   while(!ShutdownInProgress())
   {
      SetPollerState((long)arg, "wait");
      pNode = (Node *)g_statusPollQueue.GetOrBlock();
      if (pNode == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator

      snprintf(szBuffer, MAX_OBJECT_NAME + 64, "poll: %s [%d]",
               pNode->Name(), pNode->Id());
      SetPollerState((long)arg, szBuffer);
      pNode->StatusPoll(NULL, 0, (long)arg);
      pNode->DecRefCount();
   }
   SetPollerState((long)arg, "finished");
   return THREAD_OK;
}


//
// Configuration poll thread
//

static THREAD_RESULT THREAD_CALL ConfigurationPoller(void *arg)
{
   Node *pNode;
   char szBuffer[MAX_OBJECT_NAME + 64];

   // Initialize state info
   m_pPollerState[(long)arg].iType = 'C';
   SetPollerState((long)arg, "init");

   // Wait one minute to give status pollers chance to run first
   ThreadSleep(60);

   // Main loop
   while(!ShutdownInProgress())
   {
      SetPollerState((long)arg, "wait");
      pNode = (Node *)g_configPollQueue.GetOrBlock();
      if (pNode == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator

      snprintf(szBuffer, MAX_OBJECT_NAME + 64, "poll: %s [%d]",
               pNode->Name(), pNode->Id());
      SetPollerState((long)arg, szBuffer);
      pNode->ConfigurationPoll(NULL, 0, (long)arg, 0);
      pNode->DecRefCount();
   }
   SetPollerState((long)arg, "finished");
   return THREAD_OK;
}


//
// Routing table poll thread
//

static THREAD_RESULT THREAD_CALL RoutePoller(void *arg)
{
   Node *pNode;
   char szBuffer[MAX_OBJECT_NAME + 64];

   // Initialize state info
   m_pPollerState[(long)arg].iType = 'R';
   SetPollerState((long)arg, "init");

   // Main loop
   while(!ShutdownInProgress())
   {
      SetPollerState((long)arg, "wait");
      pNode = (Node *)g_routePollQueue.GetOrBlock();
      if (pNode == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator

      snprintf(szBuffer, MAX_OBJECT_NAME + 64, "poll: %s [%d]",
               pNode->Name(), pNode->Id());
      SetPollerState((long)arg, szBuffer);
      pNode->UpdateRoutingTable();
      pNode->DecRefCount();
   }
   SetPollerState((long)arg, "finished");
   return THREAD_OK;
}


//
// Discovery poll thread
//

static THREAD_RESULT THREAD_CALL DiscoveryPoller(void *arg)
{
   Node *pNode;
   TCHAR szBuffer[MAX_OBJECT_NAME + 64], szIpAddr[16];
   ARP_CACHE *pArpCache;

   // Initialize state info
   m_pPollerState[(long)arg].iType = 'D';
   SetPollerState((long)arg, "init");

   // Main loop
   while(!ShutdownInProgress())
   {
      SetPollerState((long)arg, "wait");
      pNode = (Node *)g_discoveryPollQueue.GetOrBlock();
      if (pNode == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator

      snprintf(szBuffer, MAX_OBJECT_NAME + 64, "poll: %s [%d]",
               pNode->Name(), pNode->Id());
      SetPollerState((long)arg, szBuffer);

      DbgPrintf(AF_DEBUG_DISCOVERY, _T("Starting discovery poll for node %s (%s)"),
                pNode->Name(), IpToStr(pNode->IpAddr(), szIpAddr));

      // Retrieve and analize node's ARP cache
      pArpCache = pNode->GetArpCache();
      if (pArpCache != NULL)
      {
         for(DWORD j = 0; j < pArpCache->dwNumEntries; j++)
         {
            if (FindNodeByIP(pArpCache->pEntries[j].dwIpAddr) == NULL)
            {
               Interface *pInterface = pNode->FindInterface(pArpCache->pEntries[j].dwIndex,
                                                            pArpCache->pEntries[j].dwIpAddr);
               if (pInterface != NULL)
                  if (!IsBroadcastAddress(pArpCache->pEntries[j].dwIpAddr,
                                          pInterface->IpNetMask()))
                  {
                     NEW_NODE *pInfo;

                     pInfo = (NEW_NODE *)malloc(sizeof(NEW_NODE));
                     pInfo->dwIpAddr = pArpCache->pEntries[j].dwIpAddr;
                     pInfo->dwNetMask = pInterface->IpNetMask();
                     g_nodePollerQueue.Put(pInfo);
                  }
            }
         }
         DestroyArpCache(pArpCache);
      }

      DbgPrintf(AF_DEBUG_DISCOVERY, _T("Finished discovery poll for node %s (%s)"),
                pNode->Name(), IpToStr(pNode->IpAddr(), szIpAddr));
      pNode->SetDiscoveryPollTimeStamp();
      pNode->DecRefCount();
   }
   g_nodePollerQueue.Clear();
   g_nodePollerQueue.Put(INVALID_POINTER_VALUE);
   SetPollerState((long)arg, "finished");
   return THREAD_OK;
}


//
// Condition poll thread
//

static THREAD_RESULT THREAD_CALL ConditionPoller(void *arg)
{
   Condition *pCond;
   char szBuffer[MAX_OBJECT_NAME + 64];

   // Initialize state info
   m_pPollerState[(long)arg].iType = 'N';
   SetPollerState((long)arg, "init");

   // Main loop
   while(!ShutdownInProgress())
   {
      SetPollerState((long)arg, "wait");
      pCond = (Condition *)g_conditionPollerQueue.GetOrBlock();
      if (pCond == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator

      snprintf(szBuffer, MAX_OBJECT_NAME + 64, "poll: %s [%d]",
               pCond->Name(), pCond->Id());
      SetPollerState((long)arg, szBuffer);
      pCond->Check();
      pCond->EndPoll();
   }
   SetPollerState((long)arg, "finished");
   return THREAD_OK;
}


//
// Check address range
//

static void CheckRange(int nType, DWORD dwAddr1, DWORD dwAddr2)
{
   DWORD dwAddr, dwFrom, dwTo;
   TCHAR szIpAddr1[16], szIpAddr2[16];

   if (nType == 0)
   {
      dwFrom = (dwAddr1 & dwAddr2) + 1;
      dwTo = dwFrom | ~dwAddr2 - 1;
   }
   else
   {
      dwFrom = dwAddr1;
      dwTo = dwAddr2;
   }
   DbgPrintf(AF_DEBUG_DISCOVERY, _T("Starting active discovery check on range %s - %s"),
             IpToStr(dwFrom, szIpAddr1), IpToStr(dwTo, szIpAddr2));

   for(dwAddr = dwFrom; dwAddr <= dwTo; dwAddr++)
   {
      if (IcmpPing(htonl(dwAddr), 3, 2000, NULL, g_dwPingSize) == ICMP_SUCCESS)
      {
         DbgPrintf(AF_DEBUG_DISCOVERY,
                   _T("Active discovery - node %s responds to ICMP ping"),
                   IpToStr(dwAddr, szIpAddr1));
         if (FindNodeByIP(dwAddr) == NULL)
         {
            Subnet *pSubnet;

            pSubnet = FindSubnetForNode(dwAddr);
            if (pSubnet != NULL)
            {
               if ((pSubnet->IpAddr() != dwAddr) && 
                   !IsBroadcastAddress(dwAddr, pSubnet->IpNetMask()))
               {
                  NEW_NODE *pInfo;

                  pInfo = (NEW_NODE *)malloc(sizeof(NEW_NODE));
                  pInfo->dwIpAddr = dwAddr;
                  pInfo->dwNetMask = pSubnet->IpNetMask();
                  g_nodePollerQueue.Put(pInfo);
               }
            }
            else
            {
               NEW_NODE *pInfo;

               pInfo = (NEW_NODE *)malloc(sizeof(NEW_NODE));
               pInfo->dwIpAddr = dwAddr;
               pInfo->dwNetMask = 0;
               g_nodePollerQueue.Put(pInfo);
            }
         }
      }
   }

   DbgPrintf(AF_DEBUG_DISCOVERY, _T("Finished active discovery check on range %s - %s"),
             IpToStr(dwFrom, szIpAddr1), IpToStr(dwTo, szIpAddr2));
}


//
// Active discovery poll thread
//

static THREAD_RESULT THREAD_CALL ActiveDiscoveryPoller(void *arg)
{
   int i, nRows, nInterval;
   DB_RESULT hResult;

   // Initialize state info
   m_pPollerState[(long)arg].iType = 'A';
   SetPollerState((long)arg, "init");

   nInterval = ConfigReadInt(_T("ActiveDiscoveryInterval"), 7200);

   // Main loop
   while(!ShutdownInProgress())
   {
      SetPollerState((long)arg, "wait");
      if (SleepAndCheckForShutdown(nInterval))
         break;

      if (!(g_dwFlags & AF_ACTIVE_NETWORK_DISCOVERY))
         continue;

      SetPollerState((long)arg, "check");
      hResult = DBSelect(g_hCoreDB, _T("SELECT addr_type,addr1,addr2 FROM address_lists WHERE list_type=1"));
      if (hResult != NULL)
      {
         nRows = DBGetNumRows(hResult);
         for(i = 0; i < nRows; i++)
         {
            CheckRange(DBGetFieldLong(hResult, i, 0),
                       DBGetFieldIPAddr(hResult, i, 1),
                       DBGetFieldIPAddr(hResult, i, 2));
         }
         DBFreeResult(hResult);
      }
   }
   SetPollerState((long)arg, "finished");
   return THREAD_OK;
}


//
// Node and condition queuing thread
//

THREAD_RESULT THREAD_CALL PollManager(void *pArg)
{
   Node *pNode;
   Condition *pCond;
   DWORD j, dwWatchdogId;
   int i, iCounter, iNumStatusPollers, iNumConfigPollers;
   int nIndex, iNumDiscoveryPollers, iNumRoutePollers;
   int iNumConditionPollers;

   // Read configuration
   iNumConditionPollers = ConfigReadInt("NumberOfConditionPollers", 10);
   iNumStatusPollers = ConfigReadInt("NumberOfStatusPollers", 10);
   iNumConfigPollers = ConfigReadInt("NumberOfConfigurationPollers", 4);
   iNumRoutePollers = ConfigReadInt("NumberOfRoutingTablePollers", 5);
   iNumDiscoveryPollers = ConfigReadInt("NumberOfDiscoveryPollers", 1);
   m_iNumPollers = iNumStatusPollers + iNumConfigPollers + 
                   iNumDiscoveryPollers + iNumRoutePollers + iNumConditionPollers + 1;
   DbgPrintf(AF_DEBUG_MISC, "PollManager: %d pollers to start", m_iNumPollers);

   // Prepare static data
   m_pPollerState = (__poller_state *)malloc(sizeof(__poller_state) * m_iNumPollers);

   // Start status pollers
   for(i = 0, nIndex = 0; i < iNumStatusPollers; i++, nIndex++)
      ThreadCreate(StatusPoller, 0, CAST_TO_POINTER(nIndex, void *));

   // Start configuration pollers
   for(i = 0; i < iNumConfigPollers; i++, nIndex++)
      ThreadCreate(ConfigurationPoller, 0, CAST_TO_POINTER(nIndex, void *));

   // Start routing table pollers
   for(i = 0; i < iNumRoutePollers; i++, nIndex++)
      ThreadCreate(RoutePoller, 0, CAST_TO_POINTER(nIndex, void *));

   // Start discovery pollers
   for(i = 0; i < iNumDiscoveryPollers; i++, nIndex++)
      ThreadCreate(DiscoveryPoller, 0, CAST_TO_POINTER(nIndex, void *));

   // Start condition pollers
   for(i = 0; i < iNumConditionPollers; i++, nIndex++)
      ThreadCreate(ConditionPoller, 0, CAST_TO_POINTER(nIndex, void *));

   // Start active discovery poller
   ThreadCreate(ActiveDiscoveryPoller, 0, CAST_TO_POINTER(nIndex, void *));

   dwWatchdogId = WatchdogAddThread("Poll Manager", 60);
   iCounter = 0;

   while(!ShutdownInProgress())
   {
      if (SleepAndCheckForShutdown(5))
         break;      // Shutdown has arrived
      WatchdogNotify(dwWatchdogId);

      // Check for management node every 10 minutes
      iCounter++;
      if (iCounter % 120 == 0)
      {
         iCounter = 0;
         CheckForMgmtNode();
      }

      // Walk through nodes and queue them for status 
      // and/or configuration poll
      RWLockReadLock(g_rwlockNodeIndex, INFINITE);
      for(j = 0; j < g_dwNodeAddrIndexSize; j++)
      {
         pNode = (Node *)g_pNodeIndexByAddr[j].pObject;
         if (pNode->ReadyForConfigurationPoll())
         {
            pNode->IncRefCount();
            pNode->LockForConfigurationPoll();
            g_configPollQueue.Put(pNode);
         }
         if (pNode->ReadyForStatusPoll())
         {
            pNode->IncRefCount();
            pNode->LockForStatusPoll();
            g_statusPollQueue.Put(pNode);
         }
         if (pNode->ReadyForRoutePoll())
         {
            pNode->IncRefCount();
            pNode->LockForRoutePoll();
            g_routePollQueue.Put(pNode);
         }
         if (pNode->ReadyForDiscoveryPoll())
         {
            pNode->IncRefCount();
            pNode->LockForDiscoveryPoll();
            g_discoveryPollQueue.Put(pNode);
         }
      }
      RWLockUnlock(g_rwlockNodeIndex);

      // Walk through condition objects and queue them for poll
      RWLockReadLock(g_rwlockConditionIndex, INFINITE);
      for(j = 0; j < g_dwConditionIndexSize; j++)
      {
         pCond = (Condition *)g_pConditionIndex[j].pObject;
         if (pCond->ReadyForPoll())
         {
            pCond->LockForPoll();
            g_conditionPollerQueue.Put(pCond);
         }
      }
      RWLockUnlock(g_rwlockConditionIndex);
   }

   // Send stop signal to all pollers
   g_statusPollQueue.Clear();
   g_configPollQueue.Clear();
   g_discoveryPollQueue.Clear();
   g_routePollQueue.Clear();
   g_conditionPollerQueue.Clear();
   for(i = 0; i < iNumStatusPollers; i++)
      g_statusPollQueue.Put(INVALID_POINTER_VALUE);
   for(i = 0; i < iNumConfigPollers; i++)
      g_configPollQueue.Put(INVALID_POINTER_VALUE);
   for(i = 0; i < iNumRoutePollers; i++)
      g_routePollQueue.Put(INVALID_POINTER_VALUE);
   for(i = 0; i < iNumDiscoveryPollers; i++)
      g_discoveryPollQueue.Put(INVALID_POINTER_VALUE);
   for(i = 0; i < iNumConditionPollers; i++)
      g_conditionPollerQueue.Put(INVALID_POINTER_VALUE);

   DbgPrintf(AF_DEBUG_MISC, "PollManager: main thread terminated");
   return THREAD_OK;
}


//
// Reset discovery poller after configuration change
//

void ResetDiscoveryPoller(void)
{
   Node *pNode;
   NEW_NODE *pInfo;

   // Clear queues
   while((pNode = (Node *)g_discoveryPollQueue.Get()) != NULL)
   {
      if (pNode != INVALID_POINTER_VALUE)
      {
         pNode->SetDiscoveryPollTimeStamp();
         pNode->DecRefCount();
      }
   }
   while((pInfo = (NEW_NODE *)g_nodePollerQueue.Get()) != NULL)
   {
      if (pInfo != INVALID_POINTER_VALUE)
         free(pInfo);
   }

   // Reload discovery parameters
   g_dwDiscoveryPollingInterval = ConfigReadInt("DiscoveryPollingInterval", 900);
   if (ConfigReadInt("RunNetworkDiscovery", 0))
      g_dwFlags |= AF_ENABLE_NETWORK_DISCOVERY;
   else
      g_dwFlags &= ~AF_ENABLE_NETWORK_DISCOVERY;

   if (ConfigReadInt("ActiveNetworkDiscovery", 0))
      g_dwFlags |= AF_ACTIVE_NETWORK_DISCOVERY;
   else
      g_dwFlags &= ~AF_ACTIVE_NETWORK_DISCOVERY;
}
