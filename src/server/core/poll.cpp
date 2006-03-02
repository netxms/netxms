/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005 Victor Kirhenshtein
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
** $module: poll.cpp
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
            g_dwMgmtNode = pNode->Id();   // Set local management node ID
            break;
         }
      if (i == pIfList->iNumEntries)   // No such node
      {
         // Find interface with IP address
         for(i = 0; i < pIfList->iNumEntries; i++)
            if (pIfList->pInterfaces[i].dwIpAddr != 0)
            {
               pNode = new Node(pIfList->pInterfaces[i].dwIpAddr, NF_IS_LOCAL_MGMT, DF_DEFAULT, 0);
               NetObjInsert(pNode, TRUE);
               pNode->NewNodePoll(0);
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
      pNode->ConfigurationPoll(NULL, 0, (long)arg);
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
                     pInfo->dwFlags = DF_DEFAULT;
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
// Node queuing thread
//

THREAD_RESULT THREAD_CALL NodePollManager(void *pArg)
{
   Node *pNode;
   DWORD dwWatchdogId;
   int i, iCounter, iNumStatusPollers, iNumConfigPollers;
   int nIndex, iNumDiscoveryPollers, iNumRoutePollers;
   BOOL bDoNetworkDiscovery;

   // Read configuration
   iNumStatusPollers = ConfigReadInt("NumberOfStatusPollers", 10);
   iNumConfigPollers = ConfigReadInt("NumberOfConfigurationPollers", 4);
   iNumRoutePollers = ConfigReadInt("NumberOfRoutingTablePollers", 5);
   bDoNetworkDiscovery = ConfigReadInt("RunNetworkDiscovery", 1);
   if (bDoNetworkDiscovery)
      iNumDiscoveryPollers = ConfigReadInt("NumberOfDiscoveryPollers", 1);
   else
      iNumDiscoveryPollers = 0;
   m_iNumPollers = iNumStatusPollers + iNumConfigPollers + iNumDiscoveryPollers + iNumRoutePollers;

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

   dwWatchdogId = WatchdogAddThread("Node Poll Manager", 60);
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
      for(DWORD i = 0; i < g_dwNodeAddrIndexSize; i++)
      {
         pNode = (Node *)g_pNodeIndexByAddr[i].pObject;
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
         if (bDoNetworkDiscovery && pNode->ReadyForDiscoveryPoll())
         {
            pNode->IncRefCount();
            pNode->LockForDiscoveryPoll();
            g_discoveryPollQueue.Put(pNode);
         }
      }
      RWLockUnlock(g_rwlockNodeIndex);
   }

   // Send stop signal to all pollers
   g_statusPollQueue.Clear();
   g_configPollQueue.Clear();
   g_discoveryPollQueue.Clear();
   for(i = 0; i < iNumStatusPollers; i++)
      g_statusPollQueue.Put(INVALID_POINTER_VALUE);
   for(i = 0; i < iNumConfigPollers; i++)
      g_configPollQueue.Put(INVALID_POINTER_VALUE);
   for(i = 0; i < iNumRoutePollers; i++)
      g_routePollQueue.Put(INVALID_POINTER_VALUE);
   for(i = 0; i < iNumDiscoveryPollers; i++)
      g_discoveryPollQueue.Put(INVALID_POINTER_VALUE);

   DbgPrintf(AF_DEBUG_MISC, "NodePollManager: main thread terminated");
   return THREAD_OK;
}
