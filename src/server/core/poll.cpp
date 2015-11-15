/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2015 Victor Kirhenshtein
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
 * Node poller queue (polls new nodes)
 */
Queue g_nodePollerQueue;

/**
 * Thread pool for pollers
 */
ThreadPool *g_pollerThreadPool = NULL;

/**
 * Active pollers
 */
static HashMap<UINT64, PollerInfo> s_pollers(false);
static MUTEX s_pollerLock = MutexCreate();

/**
 * Poller info destructor - will unregister poller and decrease ref count on object
 */
PollerInfo::~PollerInfo()
{
   MutexLock(s_pollerLock);
   s_pollers.remove(CAST_FROM_POINTER(this, UINT64));
   MutexUnlock(s_pollerLock);
   m_object->decRefCount();
}

/**
 * Register active poller
 */
PollerInfo *RegisterPoller(PollerType type, NetObj *object)
{
   PollerInfo *p = new PollerInfo(type, object);
   object->incRefCount();
   MutexLock(s_pollerLock);
   s_pollers.set(CAST_FROM_POINTER(p, UINT64), p);
   MutexUnlock(s_pollerLock);
   return p;
}

/**
 * Show poller information on console
 */
static EnumerationCallbackResult ShowPollerInfo(const void *key, const void *object, void *arg)
{
   static const TCHAR *pollerType[] = { _T("STAT"), _T("CONF"), _T("INST"), _T("ROUT"), _T("DISC"), _T("BSVC"), _T("COND"), _T("TOPO") };

   PollerInfo *p = (PollerInfo *)object;
   NetObj *o = p->getObject();

   TCHAR name[32];
   nx_strncpy(name, o->getName(), 31);
   ConsolePrintf((CONSOLE_CTX)arg, _T("%s | %9d | %-30s | %s\n"), pollerType[p->getType()], o->getId(), name, p->getStatus()); 

   return _CONTINUE;
}

/**
 * Get poller diagnostic
 */
void ShowPollers(CONSOLE_CTX console)
{
   ConsoleWrite(console, _T("Type | Object ID | Object name                    | Status\n")
                         _T("-----+-----------+--------------------------------+--------------------------\n"));
   MutexLock(s_pollerLock);
   s_pollers.forEach(ShowPollerInfo, console);
   MutexUnlock(s_pollerLock);
}

/**
 * Helper for AddThreadPoolMonitoringParameters
 */
inline TCHAR *BuildParamName(const TCHAR *format, const TCHAR *pool, TCHAR *buffer)
{
   _sntprintf(buffer, 256, format, pool);
   return buffer;
}

/**
 * Create management node object
 */
static void CreateManagementNode(const InetAddress& addr)
{
	TCHAR buffer[256];

	Node *node = new Node(addr, NF_IS_LOCAL_MGMT, 0, 0, 0);
   NetObjInsert(node, true, false);
	node->setName(GetLocalHostName(buffer, 256));

   PollerInfo *p = RegisterPoller(POLLER_TYPE_CONFIGURATION, node);
   p->startExecution();
   node->configurationPoll(NULL, 0, p, addr.getMaskBits());
   delete p;

   node->unhide();
   g_dwMgmtNode = node->getId();   // Set local management node ID
   PostEvent(EVENT_NODE_ADDED, node->getId(), NULL);

	// Bind to the root of service tree
	g_pServiceRoot->AddChild(node);
	node->AddParent(g_pServiceRoot);
}

/**
 * Callback to clear incorrectly set local management node flag
 */
static void CheckMgmtFlagCallback(NetObj *object, void *data)
{
	if ((g_dwMgmtNode != object->getId()) && ((Node *)object)->isLocalManagement())
	{
		((Node *)object)->clearLocalMgmtFlag();
		DbgPrintf(2, _T("Incorrectly set flag NF_IS_LOCAL_MGMT cleared from node %s [%d]"),
					 object->getName(), object->getId());
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
   Node *node;
   int i;

   pIfList = GetLocalInterfaceList();
   if (pIfList != NULL)
   {
      for(i = 0; i < pIfList->size(); i++)
      {
         InterfaceInfo *iface = pIfList->get(i);
         if (iface->type == IFTYPE_SOFTWARE_LOOPBACK)
            continue;
         if ((node = FindNodeByIP(0, &iface->ipAddrList)) != NULL)
         {
            // Check management node flag
            if (!(node->getFlags() & NF_IS_LOCAL_MGMT))
            {
               node->setLocalMgmtFlag();
               DbgPrintf(1, _T("Local management node %s [%d] was not have NF_IS_LOCAL_MGMT flag set"), node->getName(), node->getId());
            }
            g_dwMgmtNode = node->getId();   // Set local management node ID
            break;
         }
      }
      if (i == pIfList->size())   // No such node
      {
         // Find interface with IP address
         for(i = 0; i < pIfList->size(); i++)
         {
            InterfaceInfo *iface = pIfList->get(i);
            if ((iface->type == IFTYPE_SOFTWARE_LOOPBACK) || (iface->ipAddrList.size() == 0))
               continue;

            for(int j = 0; j < iface->ipAddrList.size(); j++)
            {
               const InetAddress& addr = iface->ipAddrList.get(j);
               if (addr.isValidUnicast())
               {
   				   CreateManagementNode(addr);
                  i = pIfList->size();    // stop walking interface list
                  break;
               }
            }
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
			g_dwMgmtNode = mgmtNode->getId();
		}
		else
		{
			CreateManagementNode(InetAddress());
		}
	}
}

/**
 * Comparator for poller queue elements
 */
static bool PollerQueueElementComparator(void *key, void *element)
{
   return ((InetAddress *)key)->equals(((NEW_NODE *)element)->ipAddr);
}

/**
 * Check potential new node from ARP cache or routing table
 */
static void CheckPotentialNode(Node *node, const InetAddress& ipAddr, UINT32 ifIndex, BYTE *macAddr = NULL)
{
	TCHAR buffer[64];

	DbgPrintf(6, _T("DiscoveryPoller(): checking potential node %s at %d"), ipAddr.toString(buffer), ifIndex);
   if (ipAddr.isValid() && !ipAddr.isBroadcast() && !ipAddr.isLoopback() && !ipAddr.isMulticast() &&
	    (FindNodeByIP(node->getZoneId(), ipAddr) == NULL) && !IsClusterIP(node->getZoneId(), ipAddr) && 
		 (g_nodePollerQueue.find((void *)&ipAddr, PollerQueueElementComparator) == NULL))
   {
      Interface *pInterface = node->findInterfaceByIndex(ifIndex);
      if (pInterface != NULL)
		{
         const InetAddress& interfaceAddress = pInterface->getIpAddressList()->findSameSubnetAddress(ipAddr);
         if (interfaceAddress.isValidUnicast())
         {
			   DbgPrintf(6, _T("DiscoveryPoller(): interface found: %s [%d] addr=%s/%d ifIndex=%d"),
               pInterface->getName(), pInterface->getId(), interfaceAddress.toString(buffer), interfaceAddress.getMaskBits(), pInterface->getIfIndex());
            if (!ipAddr.isSubnetBroadcast(interfaceAddress.getMaskBits()))
            {
               NEW_NODE *pInfo;
				   TCHAR buffer[64];

               pInfo = (NEW_NODE *)malloc(sizeof(NEW_NODE));
               pInfo->ipAddr = ipAddr;
               pInfo->ipAddr.setMaskBits(interfaceAddress.getMaskBits());
				   pInfo->zoneId = node->getZoneId();
				   pInfo->ignoreFilter = FALSE;
				   if (macAddr == NULL)
					   memset(pInfo->bMacAddr, 0, MAC_ADDR_LENGTH);
				   else
					   memcpy(pInfo->bMacAddr, macAddr, MAC_ADDR_LENGTH);
				   DbgPrintf(5, _T("DiscoveryPoller(): new node queued: %s/%d"),
				             pInfo->ipAddr.toString(buffer), pInfo->ipAddr.getMaskBits());
               g_nodePollerQueue.put(pInfo);
            }
			   else
			   {
               DbgPrintf(6, _T("DiscoveryPoller(): potential node %s rejected - broadcast/multicast address"), ipAddr.toString(buffer));
			   }
         }
         else
         {
   			DbgPrintf(6, _T("DiscoveryPoller(): interface object found but IP address not found"));
         }
		}
		else
		{
			DbgPrintf(6, _T("DiscoveryPoller(): interface object not found"));
		}
   }
	else
	{
		DbgPrintf(6, _T("DiscoveryPoller(): potential node %s rejected"), ipAddr.toString(buffer));
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
	iface = node->findInterfaceByIndex(route->dwIfIndex);
	if ((iface != NULL) && iface->getIpAddressList()->findSameSubnetAddress(route->dwDestAddr).isValidUnicast())
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
static void DiscoveryPoller(void *arg)
{
   PollerInfo *poller = (PollerInfo *)arg;
   poller->startExecution();

   Node *node = (Node *)poller->getObject();
	if (node->getRuntimeFlags() & NDF_DELETE_IN_PROGRESS)
	{
      node->setDiscoveryPollTimeStamp();
      delete poller;
      return;
	}

   DbgPrintf(4, _T("Starting discovery poll for node %s (%s) in zone %d"),
	          node->getName(), (const TCHAR *)node->getIpAddress().toString(), (int)node->getZoneId());

   // Retrieve and analize node's ARP cache
   ARP_CACHE *pArpCache = node->getArpCache();
   if (pArpCache != NULL)
   {
      for(UINT32 i = 0; i < pArpCache->dwNumEntries; i++)
			if (memcmp(pArpCache->pEntries[i].bMacAddr, "\xFF\xFF\xFF\xFF\xFF\xFF", 6))	// Ignore broadcast addresses
				CheckPotentialNode(node, pArpCache->pEntries[i].ipAddr, pArpCache->pEntries[i].dwIndex, pArpCache->pEntries[i].bMacAddr);
      DestroyArpCache(pArpCache);
   }

	// Retrieve and analize node's routing table
   DbgPrintf(5, _T("Discovery poll for node %s (%s) - reading routing table"),
             node->getName(), (const TCHAR *)node->getIpAddress().toString());
	ROUTING_TABLE *rt = node->getRoutingTable();
	if (rt != NULL)
	{
		for(int i = 0; i < rt->iNumEntries; i++)
		{
			CheckPotentialNode(node, rt->pRoutes[i].dwNextHop, rt->pRoutes[i].dwIfIndex);
			if ((rt->pRoutes[i].dwDestMask == 0xFFFFFFFF) && (rt->pRoutes[i].dwDestAddr != 0))
				CheckHostRoute(node, &rt->pRoutes[i]);
		}
		DestroyRoutingTable(rt);
	}

   DbgPrintf(4, _T("Finished discovery poll for node %s (%s)"),
             node->getName(), (const TCHAR *)node->getIpAddress().toString());
   node->setDiscoveryPollTimeStamp();
   delete poller;
}

/**
 * Check given address range with ICMP ping for new nodes
 */
static void CheckRange(int nType, UINT32 addr1, UINT32 addr2)
{
   UINT32 from, to;
   if (nType == 0)
   {
      from = (addr1 & addr2) + 1;
      to = from | ~addr2 - 1;
   }
   else
   {
      from = addr1;
      to = addr2;
   }

   TCHAR ipAddr1[16], ipAddr2[16];
   DbgPrintf(4, _T("Starting active discovery check on range %s - %s"), IpToStr(from, ipAddr1), IpToStr(to, ipAddr2));

   for(UINT32 curr = from; curr <= to; curr++)
   {
      InetAddress addr = InetAddress(curr);
      if (IcmpPing(addr, 3, g_icmpPingTimeout, NULL, g_icmpPingSize) == ICMP_SUCCESS)
      {
         DbgPrintf(5, _T("Active discovery - node %s responds to ICMP ping"), addr.toString(ipAddr1));
         if (FindNodeByIP(0, addr) == NULL)
         {
            Subnet *pSubnet;

            pSubnet = FindSubnetForNode(0, addr);
            if (pSubnet != NULL)
            {
               if (!pSubnet->getIpAddress().equals(addr) && 
                   !addr.isSubnetBroadcast(pSubnet->getIpAddress().getMaskBits()))
               {
                  NEW_NODE *pInfo;

                  pInfo = (NEW_NODE *)malloc(sizeof(NEW_NODE));
                  pInfo->ipAddr = addr;
                  pInfo->ipAddr.setMaskBits(pSubnet->getIpAddress().getMaskBits());
						pInfo->zoneId = 0;	/* FIXME: add correct zone ID */
						pInfo->ignoreFilter = FALSE;
						memset(pInfo->bMacAddr, 0, MAC_ADDR_LENGTH);
                  g_nodePollerQueue.put(pInfo);
               }
            }
            else
            {
               NEW_NODE *pInfo;

               pInfo = (NEW_NODE *)malloc(sizeof(NEW_NODE));
               pInfo->ipAddr = addr;
					pInfo->zoneId = 0;	/* FIXME: add correct zone ID */
					pInfo->ignoreFilter = FALSE;
					memset(pInfo->bMacAddr, 0, MAC_ADDR_LENGTH);
               g_nodePollerQueue.put(pInfo);
            }
         }
      }
   }

   DbgPrintf(4, _T("Finished active discovery check on range %s - %s"), IpToStr(from, ipAddr1), IpToStr(to, ipAddr2));
}

/**
 * Active discovery poller thread
 */
static THREAD_RESULT THREAD_CALL ActiveDiscoveryPoller(void *arg)
{
   int nInterval = ConfigReadInt(_T("ActiveDiscoveryInterval"), 7200);

   // Main loop
   while(!IsShutdownInProgress())
   {
      if (SleepAndCheckForShutdown(nInterval))
         break;

      if (!(g_flags & AF_ACTIVE_NETWORK_DISCOVERY))
         continue;

      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      DB_RESULT hResult = DBSelect(hdb, _T("SELECT addr_type,addr1,addr2 FROM address_lists WHERE list_type=1"));
      DBConnectionPoolReleaseConnection(hdb);
      if (hResult != NULL)
      {
         int nRows = DBGetNumRows(hResult);
         for(int i = 0; i < nRows; i++)
         {
            CheckRange(DBGetFieldLong(hResult, i, 0),
                       DBGetFieldIPAddr(hResult, i, 1),
                       DBGetFieldIPAddr(hResult, i, 2));
         }
         DBFreeResult(hResult);
      }
   }
   return THREAD_OK;
}

/**
 * Callback for queueing objects for polling
 */
static void QueueForPolling(NetObj *object, void *data)
{
	switch(object->getObjectClass())
	{
		case OBJECT_NODE:
			{
				Node *node = (Node *)object;
				if (node->isReadyForConfigurationPoll())
				{
					node->lockForConfigurationPoll();
					DbgPrintf(6, _T("Node %d \"%s\" queued for configuration poll"), (int)node->getId(), node->getName());
               ThreadPoolExecute(g_pollerThreadPool, node, &Node::configurationPoll, RegisterPoller(POLLER_TYPE_CONFIGURATION, node));
				}
				if (node->isReadyForInstancePoll())
				{
					node->lockForInstancePoll();
					DbgPrintf(6, _T("Node %d \"%s\" queued for instance discovery poll"), (int)node->getId(), node->getName());
               ThreadPoolExecute(g_pollerThreadPool, node, &Node::instanceDiscoveryPoll, RegisterPoller(POLLER_TYPE_INSTANCE_DISCOVERY, node));
				}
				if (node->isReadyForStatusPoll())
				{
					node->lockForStatusPoll();
					DbgPrintf(6, _T("Node %d \"%s\" queued for status poll"), (int)node->getId(), node->getName());
               ThreadPoolExecute(g_pollerThreadPool, node, &Node::statusPoll, RegisterPoller(POLLER_TYPE_STATUS, node));
				}
				if (node->isReadyForRoutePoll())
				{
					node->lockForRoutePoll();
					DbgPrintf(6, _T("Node %d \"%s\" queued for routing table poll"), (int)node->getId(), node->getName());
               ThreadPoolExecute(g_pollerThreadPool, node, &Node::routingTablePoll, RegisterPoller(POLLER_TYPE_ROUTING_TABLE, node));
				}
				if (node->isReadyForDiscoveryPoll())
				{
					node->lockForDiscoveryPoll();
					DbgPrintf(6, _T("Node %d \"%s\" queued for discovery poll"), (int)node->getId(), node->getName());
               ThreadPoolExecute(g_pollerThreadPool, DiscoveryPoller, RegisterPoller(POLLER_TYPE_DISCOVERY, node));
				}
				if (node->isReadyForTopologyPoll())
				{
					node->lockForTopologyPoll();
					DbgPrintf(6, _T("Node %d \"%s\" queued for topology poll"), (int)node->getId(), node->getName());
               ThreadPoolExecute(g_pollerThreadPool, node, &Node::topologyPoll, RegisterPoller(POLLER_TYPE_TOPOLOGY, node));
				}
			}
			break;
		case OBJECT_CONDITION:
			{
				Condition *cond = (Condition *)object;
				if (cond->isReadyForPoll())
				{
					cond->lockForPoll();
					DbgPrintf(6, _T("Condition %d \"%s\" queued for poll"), (int)object->getId(), object->getName());
               ThreadPoolExecute(g_pollerThreadPool, cond, &Condition::doPoll, RegisterPoller(POLLER_TYPE_CONDITION, cond));
				}
			}
			break;
		case OBJECT_CLUSTER:
			{
				Cluster *cluster = (Cluster *)object;
				if (cluster->isReadyForStatusPoll())
				{
					cluster->lockForStatusPoll();
					DbgPrintf(6, _T("Cluster %d \"%s\" queued for status poll"), (int)cluster->getId(), cluster->getName());
               ThreadPoolExecute(g_pollerThreadPool, cluster, &Cluster::statusPoll, RegisterPoller(POLLER_TYPE_STATUS, cluster));
				}
			}
			break;
		case OBJECT_BUSINESSSERVICE:
			{
				BusinessService *service = (BusinessService *)object;
				if (service->isReadyForPolling())
				{
					service->lockForPolling();
					DbgPrintf(6, _T("Business service %d \"%s\" queued for poll"), (int)object->getId(), object->getName());
               ThreadPoolExecute(g_pollerThreadPool, service, &BusinessService::poll, RegisterPoller(POLLER_TYPE_BUSINESS_SERVICE, service));
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
   g_pollerThreadPool = ThreadPoolCreate(ConfigReadInt(_T("PollerThreadPoolBaseSize"), 10), ConfigReadInt(_T("PollerThreadPoolMaxSize"), 250), _T("POLLERS"));

   // Start active discovery poller
   ThreadCreate(ActiveDiscoveryPoller, 0, NULL);

   UINT32 watchdogId = WatchdogAddThread(_T("Poll Manager"), 60);
   int counter = 0;

   while(!IsShutdownInProgress())
   {
      if (SleepAndCheckForShutdown(5))
         break;      // Shutdown has arrived
      WatchdogNotify(watchdogId);

      // Check for management node every 10 minutes
      counter++;
      if (counter % 120 == 0)
      {
         counter = 0;
         CheckForMgmtNode();
      }

      // Walk through objects and queue them for status 
      // and/or configuration poll
		g_idxObjectById.forEach(QueueForPolling, NULL);
   }

   g_nodePollerQueue.clear();
   g_nodePollerQueue.put(INVALID_POINTER_VALUE);

   ThreadPoolDestroy(g_pollerThreadPool);
   DbgPrintf(1, _T("PollManager: main thread terminated"));
   return THREAD_OK;
}

/**
 * Reset discovery poller after configuration change
 */
void ResetDiscoveryPoller()
{
   NEW_NODE *pInfo;

   // Clear node poller queue
   while((pInfo = (NEW_NODE *)g_nodePollerQueue.get()) != NULL)
   {
      if (pInfo != INVALID_POINTER_VALUE)
         free(pInfo);
   }

   // Reload discovery parameters
   g_dwDiscoveryPollingInterval = ConfigReadInt(_T("DiscoveryPollingInterval"), 900);
   if (ConfigReadInt(_T("RunNetworkDiscovery"), 0))
      g_flags |= AF_ENABLE_NETWORK_DISCOVERY;
   else
      g_flags &= ~AF_ENABLE_NETWORK_DISCOVERY;

   if (ConfigReadInt(_T("ActiveNetworkDiscovery"), 0))
      g_flags |= AF_ACTIVE_NETWORK_DISCOVERY;
   else
      g_flags &= ~AF_ACTIVE_NETWORK_DISCOVERY;

   if (ConfigReadInt(_T("UseSNMPTrapsForDiscovery"), 0))
      g_flags |= AF_SNMP_TRAP_DISCOVERY;
   else
      g_flags &= ~AF_SNMP_TRAP_DISCOVERY;
}
