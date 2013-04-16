/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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

/**
 * Poller state structure
 */
struct __poller_state
{
   int iType;
   time_t timestamp;
   TCHAR szMsg[128];
   TCHAR szInfo[128];
};

/**
 * Poller queues
 */
Queue g_statusPollQueue;
Queue g_configPollQueue;
Queue g_topologyPollQueue;
Queue g_routePollQueue;
Queue g_discoveryPollQueue;
Queue g_nodePollerQueue;
Queue g_conditionPollerQueue;
Queue g_businessServicePollerQueue;

/**
 * Static data
 */
static __poller_state *m_pPollerState = NULL;
static int m_iNumPollers = 0;

/**
 * Create management node object
 */
static void CreateManagementNode(DWORD ipAddr, DWORD netMask)
{
	TCHAR buffer[256];

	Node *pNode = new Node(ipAddr, NF_IS_LOCAL_MGMT, 0, 0, 0);
   NetObjInsert(pNode, TRUE);
	pNode->setName(GetLocalHostName(buffer, 256));
   pNode->configurationPoll(NULL, 0, -1, netMask);
   pNode->unhide();
   g_dwMgmtNode = pNode->Id();   // Set local management node ID
   PostEvent(EVENT_NODE_ADDED, pNode->Id(), NULL);

	// Bind to the root of service tree
	g_pServiceRoot->AddChild(pNode);
	pNode->AddParent(g_pServiceRoot);
   
   // Add default data collection items
	int pollingInterval = ConfigReadInt(_T("DefaultDCIPollingInterval"), 60);
	int retentionTime = ConfigReadInt(_T("DefaultDCIRetentionTime"), 30);
   pNode->addDCObject(new DCItem(CreateUniqueId(IDG_ITEM), _T("Status"), 
                                 DS_INTERNAL, DCI_DT_INT, pollingInterval, retentionTime, pNode));
   pNode->addDCObject(new DCItem(CreateUniqueId(IDG_ITEM), 
                                 _T("Server.AverageDCPollerQueueSize"), 
                                 DS_INTERNAL, DCI_DT_FLOAT, pollingInterval, retentionTime, pNode,
                                 _T("Data collection poller's request queue for last minute")));
   pNode->addDCObject(new DCItem(CreateUniqueId(IDG_ITEM), 
                                 _T("Server.AverageDBWriterQueueSize"), 
                                 DS_INTERNAL, DCI_DT_FLOAT, pollingInterval, retentionTime, pNode,
                                 _T("Database writer's request queue for last minute")));
   pNode->addDCObject(new DCItem(CreateUniqueId(IDG_ITEM), 
                                 _T("Server.AverageDBWriterQueueSize.IData"), 
                                 DS_INTERNAL, DCI_DT_FLOAT, pollingInterval, retentionTime, pNode,
                                 _T("Database writer's request queue (DCI data) for last minute")));
   pNode->addDCObject(new DCItem(CreateUniqueId(IDG_ITEM), 
                                 _T("Server.AverageDBWriterQueueSize.Other"), 
                                 DS_INTERNAL, DCI_DT_FLOAT, pollingInterval, retentionTime, pNode,
                                 _T("Database writer's request queue (other queries) for last minute")));
   pNode->addDCObject(new DCItem(CreateUniqueId(IDG_ITEM), 
                                 _T("Server.AverageDCIQueuingTime"), 
                                 DS_INTERNAL, DCI_DT_UINT, pollingInterval, retentionTime, pNode,
                                 _T("Average time to queue DCI for polling for last minute")));
   pNode->addDCObject(new DCItem(CreateUniqueId(IDG_ITEM), 
                                 _T("Server.AverageStatusPollerQueueSize"), 
                                 DS_INTERNAL, DCI_DT_FLOAT, pollingInterval, retentionTime, pNode,
                                 _T("Status poller queue for last minute")));
   pNode->addDCObject(new DCItem(CreateUniqueId(IDG_ITEM), 
                                 _T("Server.AverageConfigurationPollerQueueSize"), 
                                 DS_INTERNAL, DCI_DT_FLOAT, pollingInterval, retentionTime, pNode,
                                 _T("Configuration poller queue for last minute")));
}

/**
 * Callback to clear incorrectly set local management node flag
 */
static void CheckMgmtFlagCallback(NetObj *object, void *data)
{
	if ((g_dwMgmtNode != object->Id()) && ((Node *)object)->isLocalManagement())
	{
		((Node *)object)->clearLocalMgmtFlag();
		DbgPrintf(2, _T("Incorrectly set flag NF_IS_LOCAL_MGMT cleared from node %s [%d]"),
					 object->Name(), object->Id());
	}
}

/**
 * Comparator to find management node object in existing nodes
 */
static bool LocalMgmtNodeComparator(NetObj *object, void *data)
{
	return ((Node *)object)->isLocalManagement();
}

/**
 * Check if management server node presented in node list
 */
void CheckForMgmtNode()
{
   InterfaceList *pIfList;
   Node *pNode;
   int i;

   pIfList = GetLocalInterfaceList();
   if (pIfList != NULL)
   {
      for(i = 0; i < pIfList->getSize(); i++)
         if ((pNode = FindNodeByIP(0, pIfList->get(i)->dwIpAddr)) != NULL)
         {
            // Check management node flag
            if (!(pNode->getFlags() & NF_IS_LOCAL_MGMT))
            {
               pNode->setLocalMgmtFlag();
               DbgPrintf(1, _T("Local management node %s [%d] was not have NF_IS_LOCAL_MGMT flag set"), pNode->Name(), pNode->Id());
            }
            g_dwMgmtNode = pNode->Id();   // Set local management node ID
            break;
         }
      if (i == pIfList->getSize())   // No such node
      {
         // Find interface with IP address
         for(i = 0; i < pIfList->getSize(); i++)
            if (pIfList->get(i)->dwIpAddr != 0)
            {
					CreateManagementNode(pIfList->get(i)->dwIpAddr, pIfList->get(i)->dwIpNetMask);
               break;
            }
      }
      delete pIfList;
   }

	if (g_dwMgmtNode != 0)
	{
		// Check that other nodes does not have NF_IS_LOCAL_MGMT flag set
		g_idxNodeById.forEach(CheckMgmtFlagCallback, NULL);
	}
	else
	{
		// Management node cannot be found or created. This can happen
		// if management node currently does not have IP addresses (for example,
		// it's a Windows machine which is disconnected from the network).
		// In this case, try to find any node with NF_IS_LOCAL_MGMT flag, or create
		// new one without interfaces
		NetObj *mgmtNode = g_idxNodeById.find(LocalMgmtNodeComparator, NULL);
		if (mgmtNode != NULL)
		{
			g_dwMgmtNode = mgmtNode->Id();
		}
		else
		{
			CreateManagementNode(0, 0);
		}
	}
}

/**
 * Set poller's state
 */
static void SetPollerState(int nIdx, const TCHAR *pszMsg)
{
   nx_strncpy(m_pPollerState[nIdx].szMsg, pszMsg, 128);
   m_pPollerState[nIdx].szInfo[0] = 0;
   m_pPollerState[nIdx].timestamp = time(NULL);
}

/**
 * Set poller's info
 */
void SetPollerInfo(int nIdx, const TCHAR *pszMsg)
{
   if (nIdx != -1)
   {
      nx_strncpy(m_pPollerState[nIdx].szInfo, pszMsg, 128);
      m_pPollerState[nIdx].timestamp = time(NULL);
   }
}

/**
 * Display current poller threads state
 */
void ShowPollerState(CONSOLE_CTX pCtx)
{
   int i;
   TCHAR szTime[64];
   struct tm *ltm;

   ConsolePrintf(pCtx, _T("PT  TIME                   STATE\n"));
   for(i = 0; i < m_iNumPollers; i++)
   {
      ltm = localtime(&m_pPollerState[i].timestamp);
      if (ltm != NULL)
         _tcsftime(szTime, 64, _T("%d/%b/%Y %H:%M:%S"), ltm);
      else
         _tcscpy(szTime, _T("<error>             "));
      if (m_pPollerState[i].szInfo[0] != 0)
         ConsolePrintf(pCtx, _T("%c   %s   %s - %s\n"), m_pPollerState[i].iType, 
                       szTime, m_pPollerState[i].szMsg, m_pPollerState[i].szInfo);
      else
         ConsolePrintf(pCtx, _T("%c   %s   %s\n"), m_pPollerState[i].iType, 
                       szTime, m_pPollerState[i].szMsg);
   }
   ConsolePrintf(pCtx, _T("\n"));
}

/**
 * Status poll thread
 */
static THREAD_RESULT THREAD_CALL StatusPoller(void *arg)
{
   NetObj *pObject;
   TCHAR szBuffer[MAX_OBJECT_NAME + 64];

	int unreachableDeleteDays = ConfigReadInt(_T("DeleteUnreachableNodesPeriod"), 0);
	if (unreachableDeleteDays > 10000)
	{
		DbgPrintf(1, _T("Extremely high value of DeleteUnreachableNodesPeriod (%d), using 10000 instead)"), unreachableDeleteDays);
		unreachableDeleteDays = 10000;
	}

   // Initialize state info
   m_pPollerState[(long)arg].iType = 'S';
   SetPollerState((long)arg, _T("init"));

   // Main loop
   while(!IsShutdownInProgress())
   {
      SetPollerState((long)arg, _T("wait"));
      pObject = (NetObj *)g_statusPollQueue.GetOrBlock();
      if (pObject == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator

      _sntprintf(szBuffer, MAX_OBJECT_NAME + 64, _T("poll: %s [%d]"), pObject->Name(), pObject->Id());
      SetPollerState((long)arg, szBuffer);
		if (pObject->Type() == OBJECT_NODE)
		{
			((Node *)pObject)->statusPoll(NULL, 0, (long)arg);
			// Check if the node has to be deleted due to long downtime
			if ((unreachableDeleteDays > 0) && (((Node *)pObject)->getDownTime() > 0) && 
				 (time(NULL) - ((Node *)pObject)->getDownTime() > unreachableDeleteDays * 24 * 3600))
			{
				((Node*)pObject)->deleteObject();
			}
		}
		else if (pObject->Type() == OBJECT_CLUSTER)
		{
			((Cluster *)pObject)->statusPoll(NULL, 0, (long)arg);
		}
      pObject->decRefCount();
   }
   SetPollerState((long)arg, _T("finished"));
   return THREAD_OK;
}

/**
 * Configuration poller
 */
static THREAD_RESULT THREAD_CALL ConfigurationPoller(void *arg)
{
   Node *pNode;
   TCHAR szBuffer[MAX_OBJECT_NAME + 64];

   // Initialize state info
   m_pPollerState[(long)arg].iType = 'C';
   SetPollerState((long)arg, _T("init"));

   // Wait one minute to give status pollers chance to run first
   ThreadSleep(60);

   // Main loop
   while(!IsShutdownInProgress())
   {
      SetPollerState((long)arg, _T("wait"));
      pNode = (Node *)g_configPollQueue.GetOrBlock();
      if (pNode == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator

      _sntprintf(szBuffer, MAX_OBJECT_NAME + 64, _T("poll: %s [%d]"), pNode->Name(), pNode->Id());
      SetPollerState((long)arg, szBuffer);
      ObjectTransactionStart();
      pNode->configurationPoll(NULL, 0, (long)arg, 0);
      ObjectTransactionEnd();
      pNode->decRefCount();
   }
   SetPollerState((long)arg, _T("finished"));
   return THREAD_OK;
}

/**
 * Routing table poller
 */
static THREAD_RESULT THREAD_CALL RoutePoller(void *arg)
{
   Node *pNode;
   TCHAR szBuffer[MAX_OBJECT_NAME + 64];

   // Initialize state info
   m_pPollerState[(long)arg].iType = 'R';
   SetPollerState((long)arg, _T("init"));

   // Wait two minutes to give status and configuration pollers chance to run first
   ThreadSleep(120);

   // Main loop
   while(!IsShutdownInProgress())
   {
      SetPollerState((long)arg, _T("wait"));
      pNode = (Node *)g_routePollQueue.GetOrBlock();
      if (pNode == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator

      _sntprintf(szBuffer, MAX_OBJECT_NAME + 64, _T("poll: %s [%d]"), pNode->Name(), pNode->Id());
      SetPollerState((long)arg, szBuffer);
      pNode->updateRoutingTable();
      pNode->decRefCount();
   }
   SetPollerState((long)arg, _T("finished"));
   return THREAD_OK;
}

/**
 * Comparator for poller queue elements
 */
static bool PollerQueueElementComparator(void *key, void *element)
{
	return CAST_FROM_POINTER(key, DWORD) == ((NEW_NODE *)element)->dwIpAddr;
}

/**
 * Check potential new node from ARP cache or routing table
 */
static void CheckPotentialNode(Node *node, DWORD ipAddr, DWORD ifIndex, BYTE *macAddr = NULL)
{
	TCHAR buffer[32];

	DbgPrintf(6, _T("DiscoveryPoller(): checking potential node %s at %d"), IpToStr(ipAddr, buffer), ifIndex);
	if ((ipAddr != 0) && (ipAddr != 0xFFFFFFFF) && ((ipAddr & 0xFF000000) != 0x7F000000) &&
	    (FindNodeByIP(node->getZoneId(), ipAddr) == NULL) && !IsClusterIP(node->getZoneId(), ipAddr) && 
		 (g_nodePollerQueue.find(CAST_TO_POINTER(ipAddr, void *), PollerQueueElementComparator) == NULL))
   {
      Interface *pInterface = node->findInterface(ifIndex, ipAddr);
      if (pInterface != NULL)
		{
			DbgPrintf(6, _T("DiscoveryPoller(): interface found: %s [%d] addr=%s mask=%s ifIndex=%d"),
			          pInterface->Name(), pInterface->Id(), IpToStr(pInterface->IpAddr(), buffer),
			          IpToStr(pInterface->getIpNetMask(), &buffer[16]), pInterface->getIfIndex());
         if ((ipAddr < 0xE0000000) && !IsBroadcastAddress(ipAddr, pInterface->getIpNetMask()))
         {
            NEW_NODE *pInfo;
				TCHAR buf1[16], buf2[16];

            pInfo = (NEW_NODE *)malloc(sizeof(NEW_NODE));
            pInfo->dwIpAddr = ipAddr;
            pInfo->dwNetMask = pInterface->getIpNetMask();
				pInfo->zoneId = node->getZoneId();
				pInfo->ignoreFilter = FALSE;
				if (macAddr == NULL)
					memset(pInfo->bMacAddr, 0, MAC_ADDR_LENGTH);
				else
					memcpy(pInfo->bMacAddr, macAddr, MAC_ADDR_LENGTH);
				DbgPrintf(5, _T("DiscoveryPoller(): new node queued: %s/%s"),
				          IpToStr(pInfo->dwIpAddr, buf1), 
				          IpToStr(pInfo->dwNetMask, buf2));
            g_nodePollerQueue.Put(pInfo);
         }
			else
			{
				DbgPrintf(6, _T("DiscoveryPoller(): potential node %s rejected - broadcast/multicast address"), IpToStr(ipAddr, buffer));
			}
		}
		else
		{
			DbgPrintf(6, _T("DiscoveryPoller(): interface object not found"));
		}
   }
	else
	{
		DbgPrintf(6, _T("DiscoveryPoller(): potential node %s rejected"), IpToStr(ipAddr, buffer));
	}
}

/**
 * Check host route
 * Host will be added if it is directly connected
 */
static void CheckHostRoute(Node *node, ROUTE *route)
{
	TCHAR buffer[16];
	Interface *iface;

	DbgPrintf(6, _T("DiscoveryPoller(): checking host route %s at %d"), IpToStr(route->dwDestAddr, buffer), route->dwIfIndex);
	iface = node->findInterface(route->dwIfIndex, route->dwDestAddr);
	if (iface != NULL)
	{
		CheckPotentialNode(node, route->dwDestAddr, route->dwIfIndex);
	}
	else
	{
		DbgPrintf(6, _T("DiscoveryPoller(): interface object not found for host route"));
	}
}

/**
 * Discovery poller
 */
static THREAD_RESULT THREAD_CALL DiscoveryPoller(void *arg)
{
   Node *pNode;
   TCHAR szBuffer[MAX_OBJECT_NAME + 64], szIpAddr[16];
   ARP_CACHE *pArpCache;
	ROUTING_TABLE *rt;
	DWORD i;

   // Initialize state info
   m_pPollerState[(long)arg].iType = 'D';
   SetPollerState((long)arg, _T("init"));

   // Wait two minutes to give status and configuration pollers chance to run first
   ThreadSleep(120);

   // Main loop
   while(!IsShutdownInProgress())
   {
      SetPollerState((long)arg, _T("wait"));
      pNode = (Node *)g_discoveryPollQueue.GetOrBlock();
      if (pNode == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator

		if (pNode->getRuntimeFlags() & NDF_DELETE_IN_PROGRESS)
		{
	      pNode->setDiscoveryPollTimeStamp();
	      pNode->decRefCount();
			continue;
		}

      _sntprintf(szBuffer, MAX_OBJECT_NAME + 64, _T("poll: %s [%d]"), pNode->Name(), pNode->Id());
      SetPollerState((long)arg, szBuffer);

      DbgPrintf(4, _T("Starting discovery poll for node %s (%s) in zone %d"),
		          pNode->Name(), IpToStr(pNode->IpAddr(), szIpAddr), (int)pNode->getZoneId());

      // Retrieve and analize node's ARP cache
      pArpCache = pNode->getArpCache();
      if (pArpCache != NULL)
      {
         for(i = 0; i < pArpCache->dwNumEntries; i++)
				if (memcmp(pArpCache->pEntries[i].bMacAddr, _T("\xFF\xFF\xFF\xFF\xFF\xFF"), 6))	// Ignore broadcast addresses
					CheckPotentialNode(pNode, pArpCache->pEntries[i].dwIpAddr, pArpCache->pEntries[i].dwIndex, pArpCache->pEntries[i].bMacAddr);
         DestroyArpCache(pArpCache);
      }

		// Retrieve and analize node's routing table
      DbgPrintf(5, _T("Discovery poll for node %s (%s) - reading routing table"),
                pNode->Name(), IpToStr(pNode->IpAddr(), szIpAddr));
		rt = pNode->getRoutingTable();
		if (rt != NULL)
		{
			for(i = 0; i < (DWORD)rt->iNumEntries; i++)
			{
				CheckPotentialNode(pNode, rt->pRoutes[i].dwNextHop, rt->pRoutes[i].dwIfIndex);
				if ((rt->pRoutes[i].dwDestMask == 0xFFFFFFFF) && (rt->pRoutes[i].dwDestAddr != 0))
					CheckHostRoute(pNode, &rt->pRoutes[i]);
			}
			DestroyRoutingTable(rt);
		}

      DbgPrintf(4, _T("Finished discovery poll for node %s (%s)"),
                pNode->Name(), IpToStr(pNode->IpAddr(), szIpAddr));
      pNode->setDiscoveryPollTimeStamp();
      pNode->decRefCount();
   }
   g_nodePollerQueue.Clear();
   g_nodePollerQueue.Put(INVALID_POINTER_VALUE);
   SetPollerState((long)arg, _T("finished"));
   return THREAD_OK;
}

/**
 * Condition poller
 */
static THREAD_RESULT THREAD_CALL ConditionPoller(void *arg)
{
   Condition *pCond;
   TCHAR szBuffer[MAX_OBJECT_NAME + 64];

   // Initialize state info
   m_pPollerState[(long)arg].iType = 'N';
   SetPollerState((long)arg, _T("init"));

   // Main loop
   while(!IsShutdownInProgress())
   {
      SetPollerState((long)arg, _T("wait"));
      pCond = (Condition *)g_conditionPollerQueue.GetOrBlock();
      if (pCond == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator

      _sntprintf(szBuffer, MAX_OBJECT_NAME + 64, _T("poll: %s [%d]"), pCond->Name(), pCond->Id());
      SetPollerState((long)arg, szBuffer);
      pCond->check();
      pCond->EndPoll();
   }
   SetPollerState((long)arg, _T("finished"));
   return THREAD_OK;
}

/**
 * Topology poller
 */
static THREAD_RESULT THREAD_CALL TopologyPoller(void *arg)
{
   TCHAR szBuffer[MAX_OBJECT_NAME + 64];

   // Initialize state info
   m_pPollerState[(long)arg].iType = 'T';
   SetPollerState((long)arg, _T("init"));

   // Wait two minutes to give configuration pollers chance to run first
   ThreadSleep(120);

   // Main loop
   while(!IsShutdownInProgress())
   {
      SetPollerState((long)arg, _T("wait"));
      Node *node = (Node *)g_topologyPollQueue.GetOrBlock();
      if (node == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator

      _sntprintf(szBuffer, MAX_OBJECT_NAME + 64, _T("poll: %s [%d]"), node->Name(), node->Id());
      SetPollerState((long)arg, szBuffer);
		node->topologyPoll(NULL, 0, CAST_FROM_POINTER(arg, int));
      node->decRefCount();
   }
   SetPollerState((long)arg, _T("finished"));
   return THREAD_OK;
}

/**
 * Business service poller
 */
static THREAD_RESULT THREAD_CALL BusinessServicePoller(void *arg)
{
   TCHAR szBuffer[MAX_OBJECT_NAME + 64];

   // Initialize state info
   m_pPollerState[(long)arg].iType = 'B';
   SetPollerState((long)arg, _T("init"));

   // Wait two minutes to give configuration pollers chance to run first
   ThreadSleep(120);

   // Main loop
   while(!IsShutdownInProgress())
   {
      SetPollerState((long)arg, _T("wait"));
		BusinessService *service = (BusinessService *)g_businessServicePollerQueue.GetOrBlock();
      if (service == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator

      _sntprintf(szBuffer, MAX_OBJECT_NAME + 64, _T("poll: %s [%d]"), service->Name(), service->Id());
      SetPollerState((long)arg, szBuffer);
		service->poll(NULL, 0, CAST_FROM_POINTER(arg, int));
      service->decRefCount();
   }
   SetPollerState((long)arg, _T("finished"));
   return THREAD_OK;
}

/**
 * Check given address range with ICMP ping for new nodes
 */
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
   DbgPrintf(4, _T("Starting active discovery check on range %s - %s"),
             IpToStr(dwFrom, szIpAddr1), IpToStr(dwTo, szIpAddr2));

   for(dwAddr = dwFrom; dwAddr <= dwTo; dwAddr++)
   {
      if (IcmpPing(htonl(dwAddr), 3, 2000, NULL, g_dwPingSize) == ICMP_SUCCESS)
      {
         DbgPrintf(5, _T("Active discovery - node %s responds to ICMP ping"),
                   IpToStr(dwAddr, szIpAddr1));
         if (FindNodeByIP(0, dwAddr) == NULL)
         {
            Subnet *pSubnet;

            pSubnet = FindSubnetForNode(0, dwAddr);
            if (pSubnet != NULL)
            {
               if ((pSubnet->IpAddr() != dwAddr) && 
                   !IsBroadcastAddress(dwAddr, pSubnet->getIpNetMask()))
               {
                  NEW_NODE *pInfo;

                  pInfo = (NEW_NODE *)malloc(sizeof(NEW_NODE));
                  pInfo->dwIpAddr = dwAddr;
                  pInfo->dwNetMask = pSubnet->getIpNetMask();
						pInfo->zoneId = 0;	/* FIXME: add correct zone ID */
						pInfo->ignoreFilter = FALSE;
						memset(pInfo->bMacAddr, 0, MAC_ADDR_LENGTH);
                  g_nodePollerQueue.Put(pInfo);
               }
            }
            else
            {
               NEW_NODE *pInfo;

               pInfo = (NEW_NODE *)malloc(sizeof(NEW_NODE));
               pInfo->dwIpAddr = dwAddr;
               pInfo->dwNetMask = 0;
					pInfo->zoneId = 0;	/* FIXME: add correct zone ID */
					pInfo->ignoreFilter = FALSE;
					memset(pInfo->bMacAddr, 0, MAC_ADDR_LENGTH);
               g_nodePollerQueue.Put(pInfo);
            }
         }
      }
   }

   DbgPrintf(4, _T("Finished active discovery check on range %s - %s"),
             IpToStr(dwFrom, szIpAddr1), IpToStr(dwTo, szIpAddr2));
}

/**
 * Active discovery poller thread
 */
static THREAD_RESULT THREAD_CALL ActiveDiscoveryPoller(void *arg)
{
   int i, nRows, nInterval;
   DB_RESULT hResult;

   // Initialize state info
   m_pPollerState[(long)arg].iType = 'A';
   SetPollerState((long)arg, _T("init"));

   nInterval = ConfigReadInt(_T("ActiveDiscoveryInterval"), 7200);

   // Main loop
   while(!IsShutdownInProgress())
   {
      SetPollerState((long)arg, _T("wait"));
      if (SleepAndCheckForShutdown(nInterval))
         break;

      if (!(g_dwFlags & AF_ACTIVE_NETWORK_DISCOVERY))
         continue;

      SetPollerState((long)arg, _T("check"));
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
   SetPollerState((long)arg, _T("finished"));
   return THREAD_OK;
}

/**
 * Callback for queueing objects for polling
 */
static void QueueForPolling(NetObj *object, void *data)
{
	switch(object->Type())
	{
		case OBJECT_NODE:
			{
				Node *node = (Node *)object;
				if (node->isReadyForConfigurationPoll())
				{
					node->incRefCount();
					node->lockForConfigurationPoll();
					DbgPrintf(6, _T("Node %d \"%s\" queued for configuration poll"), (int)node->Id(), node->Name());
					g_configPollQueue.Put(node);
				}
				if (node->isReadyForStatusPoll())
				{
					node->incRefCount();
					node->lockForStatusPoll();
					DbgPrintf(6, _T("Node %d \"%s\" queued for status poll"), (int)node->Id(), node->Name());
					g_statusPollQueue.Put(node);
				}
				if (node->isReadyForRoutePoll())
				{
					node->incRefCount();
					node->lockForRoutePoll();
					DbgPrintf(6, _T("Node %d \"%s\" queued for routing table poll"), (int)node->Id(), node->Name());
					g_routePollQueue.Put(node);
				}
				if (node->isReadyForDiscoveryPoll())
				{
					node->incRefCount();
					node->lockForDiscoveryPoll();
					DbgPrintf(6, _T("Node %d \"%s\" queued for discovery poll"), (int)node->Id(), node->Name());
					g_discoveryPollQueue.Put(node);
				}
				if (node->isReadyForTopologyPoll())
				{
					node->incRefCount();
					node->lockForTopologyPoll();
					DbgPrintf(6, _T("Node %d \"%s\" queued for topology poll"), (int)node->Id(), node->Name());
					g_topologyPollQueue.Put(node);
				}
			}
			break;
		case OBJECT_CONDITION:
			{
				Condition *cond = (Condition *)object;
				if (cond->ReadyForPoll())
				{
					cond->LockForPoll();
					DbgPrintf(6, _T("Condition %d \"%s\" queued for poll"), (int)object->Id(), object->Name());
					g_conditionPollerQueue.Put(cond);
				}
			}
			break;
		case OBJECT_CLUSTER:
			{
				Cluster *cluster = (Cluster *)object;
				if (cluster->isReadyForStatusPoll())
				{
					cluster->incRefCount();
					cluster->lockForStatusPoll();
					DbgPrintf(6, _T("Cluster %d \"%s\" queued for status poll"), (int)cluster->Id(), cluster->Name());
					g_statusPollQueue.Put(cluster);
				}
			}
			break;
		case OBJECT_BUSINESSSERVICE:
			{
				BusinessService *service = (BusinessService *)object;
				if (service->isReadyForPolling())
				{
					service->incRefCount();
					service->lockForPolling();
					DbgPrintf(6, _T("Business service %d \"%s\" queued for poll"), (int)object->Id(), object->Name());
					g_businessServicePollerQueue.Put(service);
				}
			}
			break;
		default:
			break;
	}
}

/**
 * Node and condition queuing thread
 */
THREAD_RESULT THREAD_CALL PollManager(void *pArg)
{
   DWORD dwWatchdogId;
   int i, iCounter, iNumStatusPollers, iNumConfigPollers;
   int nIndex, iNumDiscoveryPollers, iNumRoutePollers;
   int iNumConditionPollers, iNumTopologyPollers, iNumBusinessServicePollers;

   // Read configuration
   iNumConditionPollers = ConfigReadInt(_T("NumberOfConditionPollers"), 10);
   iNumStatusPollers = ConfigReadInt(_T("NumberOfStatusPollers"), 25);
   iNumConfigPollers = ConfigReadInt(_T("NumberOfConfigurationPollers"), 10);
   iNumRoutePollers = ConfigReadInt(_T("NumberOfRoutingTablePollers"), 10);
   iNumDiscoveryPollers = ConfigReadInt(_T("NumberOfDiscoveryPollers"), 1);
   iNumTopologyPollers = ConfigReadInt(_T("NumberOfTopologyPollers"), 10);
   iNumBusinessServicePollers = ConfigReadInt(_T("NumberOfBusinessServicePollers"), 10);
   m_iNumPollers = iNumStatusPollers + iNumConfigPollers + 
                   iNumDiscoveryPollers + iNumRoutePollers + iNumConditionPollers + 
						 iNumTopologyPollers + iNumBusinessServicePollers + 1;
   DbgPrintf(2, _T("PollManager: %d pollers to start"), m_iNumPollers);

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

   // Start topology pollers
   for(i = 0; i < iNumTopologyPollers; i++, nIndex++)
      ThreadCreate(TopologyPoller, 0, CAST_TO_POINTER(nIndex, void *));

   // Start business service pollers
   for(i = 0; i < iNumBusinessServicePollers; i++, nIndex++)
      ThreadCreate(BusinessServicePoller, 0, CAST_TO_POINTER(nIndex, void *));

   // Start active discovery poller
   ThreadCreate(ActiveDiscoveryPoller, 0, CAST_TO_POINTER(nIndex, void *));

   dwWatchdogId = WatchdogAddThread(_T("Poll Manager"), 60);
   iCounter = 0;

   while(!IsShutdownInProgress())
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

      // Walk through objects and queue them for status 
      // and/or configuration poll
		g_idxObjectById.forEach(QueueForPolling, NULL);
   }

   // Send stop signal to all pollers
   g_statusPollQueue.Clear();
   g_configPollQueue.Clear();
   g_discoveryPollQueue.Clear();
   g_routePollQueue.Clear();
   g_conditionPollerQueue.Clear();
	g_topologyPollQueue.Clear();
	g_businessServicePollerQueue.Clear();
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
   for(i = 0; i < iNumTopologyPollers; i++)
      g_topologyPollQueue.Put(INVALID_POINTER_VALUE);
   for(i = 0; i < iNumBusinessServicePollers; i++)
      g_businessServicePollerQueue.Put(INVALID_POINTER_VALUE);

   DbgPrintf(1, _T("PollManager: main thread terminated"));
   return THREAD_OK;
}

/**
 * Reset discovery poller after configuration change
 */
void ResetDiscoveryPoller()
{
   Node *pNode;
   NEW_NODE *pInfo;

   // Clear queues
   while((pNode = (Node *)g_discoveryPollQueue.Get()) != NULL)
   {
      if (pNode != INVALID_POINTER_VALUE)
      {
         pNode->setDiscoveryPollTimeStamp();
         pNode->decRefCount();
      }
   }
   while((pInfo = (NEW_NODE *)g_nodePollerQueue.Get()) != NULL)
   {
      if (pInfo != INVALID_POINTER_VALUE)
         free(pInfo);
   }

   // Reload discovery parameters
   g_dwDiscoveryPollingInterval = ConfigReadInt(_T("DiscoveryPollingInterval"), 900);
   if (ConfigReadInt(_T("RunNetworkDiscovery"), 0))
      g_dwFlags |= AF_ENABLE_NETWORK_DISCOVERY;
   else
      g_dwFlags &= ~AF_ENABLE_NETWORK_DISCOVERY;

   if (ConfigReadInt(_T("ActiveNetworkDiscovery"), 0))
      g_dwFlags |= AF_ACTIVE_NETWORK_DISCOVERY;
   else
      g_dwFlags &= ~AF_ACTIVE_NETWORK_DISCOVERY;
}
