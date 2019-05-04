/*
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Victor Kirhenshtein
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

#define DEBUG_TAG_DISCOVERY      _T("poll.discovery")
#define DEBUG_TAG_POLL_MANAGER   _T("poll.manager")

/**
 * Check if given address is in processing by new node poller
 */
bool IsNodePollerActiveAddress(const InetAddress& addr);

/**
 * Node poller queue (polls new nodes)
 */
ObjectQueue<DiscoveredAddress> g_nodePollerQueue;

/**
 * Thread pool for pollers
 */
ThreadPool *g_pollerThreadPool = NULL;

/**
 * Active pollers
 */
static HashMap<UINT64, PollerInfo> s_pollers(false);
static Mutex s_pollerLock;

/**
 * Poller info destructor - will unregister poller and decrease ref count on object
 */
PollerInfo::~PollerInfo()
{
   s_pollerLock.lock();
   s_pollers.remove(CAST_FROM_POINTER(this, UINT64));
   s_pollerLock.unlock();
   m_object->decRefCount();
}

/**
 * Register active poller
 */
PollerInfo *RegisterPoller(PollerType type, NetObj *object)
{
   PollerInfo *p = new PollerInfo(type, object);
   object->incRefCount();
   s_pollerLock.lock();
   s_pollers.set(CAST_FROM_POINTER(p, UINT64), p);
   s_pollerLock.unlock();
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
   ConsolePrintf(static_cast<CONSOLE_CTX>(arg), _T("%s | %9d | %-30s | %s\n"), pollerType[p->getType()], o->getId(), name, p->getStatus()); 

   return _CONTINUE;
}

/**
 * Get poller diagnostic
 */
void ShowPollers(CONSOLE_CTX console)
{
   ConsoleWrite(console, _T("Type | Object ID | Object name                    | Status\n")
                         _T("-----+-----------+--------------------------------+--------------------------\n"));
   s_pollerLock.lock();
   s_pollers.forEach(ShowPollerInfo, console);
   s_pollerLock.unlock();
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

	NewNodeData newNodeData(addr);
	Node *node = new Node(&newNodeData, 0);
	node->setCapabilities(NC_IS_LOCAL_MGMT);
   NetObjInsert(node, true, false);
	node->setName(GetLocalHostName(buffer, 256, false));

	node->configurationPollWorkerEntry(RegisterPoller(POLLER_TYPE_CONFIGURATION, node));

   node->unhide();
   g_dwMgmtNode = node->getId();   // Set local management node ID
   PostEvent(EVENT_NODE_ADDED, node->getId(), NULL);

	// Bind to the root of service tree
	g_pServiceRoot->addChild(node);
	node->addParent(g_pServiceRoot);
}

/**
 * Callback to clear incorrectly set local management node flag
 */
static void CheckMgmtFlagCallback(NetObj *object, void *data)
{
	if ((g_dwMgmtNode != object->getId()) && ((Node *)object)->isLocalManagement())
	{
		((Node *)object)->clearLocalMgmtFlag();
		nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 2, _T("Incorrectly set flag NC_IS_LOCAL_MGMT cleared from node %s [%d]"),
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
            if (!(node->getCapabilities() & NC_IS_LOCAL_MGMT))
            {
               node->setLocalMgmtFlag();
               nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 1, _T("Local management node %s [%d] was not have NC_IS_LOCAL_MGMT flag set"), node->getName(), node->getId());
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
		// Check that other nodes does not have NC_IS_LOCAL_MGMT flag set
		g_idxNodeById.forEach(CheckMgmtFlagCallback, NULL);
	}
	else
	{
		// Management node cannot be found or created. This can happen
		// if management node currently does not have IP addresses (for example,
		// it's a Windows machine which is disconnected from the network).
		// In this case, try to find any node with NC_IS_LOCAL_MGMT flag, or create
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
static bool PollerQueueElementComparator(const void *key, const DiscoveredAddress *element)
{
   return static_cast<const InetAddress*>(key)->equals(element->ipAddr);
}

/**
 * Check potential new node from sysog, SNMP trap, or address range scan
 */
void CheckPotentialNode(const InetAddress& ipAddr, UINT32 zoneUIN, DiscoveredAddressSourceType sourceType, UINT32 sourceNodeId)
{
	TCHAR buffer[64];
	nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Checking address %s in zone %d"), ipAddr.toString(buffer), zoneUIN);
   if (!ipAddr.isValid() || ipAddr.isBroadcast() || ipAddr.isLoopback() || ipAddr.isMulticast())
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Potential node %s rejected (IP address is not a valid unicast address)"), ipAddr.toString(buffer));
      return;
   }

   Node *curr = FindNodeByIP(zoneUIN, ipAddr);
   if (curr != NULL)
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Potential node %s rejected (IP address already known at node %s [%d])"),
               ipAddr.toString(buffer), curr->getName(), curr->getId());
      return;
   }

   if (IsClusterIP(zoneUIN, ipAddr))
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Potential node %s rejected (IP address is known as cluster resource address)"), ipAddr.toString(buffer));
      return;
   }

   if (IsNodePollerActiveAddress(ipAddr) || (g_nodePollerQueue.find(&ipAddr, PollerQueueElementComparator) != NULL))
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Potential node %s rejected (IP address already queued for polling)"), ipAddr.toString(buffer));
      return;
   }

   Subnet *subnet = FindSubnetForNode(zoneUIN, ipAddr);
   if (subnet != NULL)
   {
      if (!subnet->getIpAddress().equals(ipAddr) && !ipAddr.isSubnetBroadcast(subnet->getIpAddress().getMaskBits()))
      {
         DiscoveredAddress *nodeInfo = MemAllocStruct<DiscoveredAddress>();
         nodeInfo->ipAddr = ipAddr;
         nodeInfo->ipAddr.setMaskBits(subnet->getIpAddress().getMaskBits());
         nodeInfo->zoneUIN = zoneUIN;
         nodeInfo->ignoreFilter = FALSE;
         memset(nodeInfo->bMacAddr, 0, MAC_ADDR_LENGTH);
         nodeInfo->sourceType = sourceType;
         nodeInfo->sourceNodeId = sourceNodeId;
         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 5, _T("New node queued: %s/%d"), nodeInfo->ipAddr.toString(buffer), nodeInfo->ipAddr.getMaskBits());
         g_nodePollerQueue.put(nodeInfo);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Potential node %s rejected (IP address is a base or broadcast address of existing subnet)"), ipAddr.toString(buffer));
      }
   }
   else
   {
      DiscoveredAddress *nodeInfo = MemAllocStruct<DiscoveredAddress>();
      nodeInfo->ipAddr = ipAddr;
      nodeInfo->zoneUIN = zoneUIN;
      nodeInfo->ignoreFilter = FALSE;
      memset(nodeInfo->bMacAddr, 0, MAC_ADDR_LENGTH);
      nodeInfo->sourceType = sourceType;
      nodeInfo->sourceNodeId = sourceNodeId;
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 5, _T("New node queued: %s/%d"), nodeInfo->ipAddr.toString(buffer), nodeInfo->ipAddr.getMaskBits());
      g_nodePollerQueue.put(nodeInfo);
   }
}

/**
 * Check potential new node from ARP cache or routing table
 */
static void CheckPotentialNode(Node *node, const InetAddress& ipAddr, UINT32 ifIndex, const BYTE *macAddr,
         DiscoveredAddressSourceType sourceType, UINT32 sourceNodeId)
{
   TCHAR buffer[64];
   nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Checking potential node %s at %s:%d"), ipAddr.toString(buffer), node->getName(), ifIndex);
   if (!ipAddr.isValidUnicast())
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Potential node %s rejected (IP address is not a valid unicast address)"), ipAddr.toString(buffer));
      return;
   }

   Node *curr = FindNodeByIP(node->getZoneUIN(), ipAddr);
   if (curr != NULL)
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Potential node %s rejected (IP address already known at node %s [%d])"),
               ipAddr.toString(buffer), curr->getName(), curr->getId());
      return;
   }

   if (IsClusterIP(node->getZoneUIN(), ipAddr))
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Potential node %s rejected (IP address is known as cluster resource address)"), ipAddr.toString(buffer));
      return;
   }

   if (IsNodePollerActiveAddress(ipAddr) || (g_nodePollerQueue.find(&ipAddr, PollerQueueElementComparator) != NULL))
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Potential node %s rejected (IP address already queued for polling)"), ipAddr.toString(buffer));
      return;
   }

   Interface *pInterface = node->findInterfaceByIndex(ifIndex);
   if (pInterface != NULL)
   {
      const InetAddress& interfaceAddress = pInterface->getIpAddressList()->findSameSubnetAddress(ipAddr);
      if (interfaceAddress.isValidUnicast())
      {
         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Interface found: %s [%d] addr=%s/%d ifIndex=%d"),
            pInterface->getName(), pInterface->getId(), interfaceAddress.toString(buffer), interfaceAddress.getMaskBits(), pInterface->getIfIndex());
         if (!ipAddr.isSubnetBroadcast(interfaceAddress.getMaskBits()))
         {
            DiscoveredAddress *pInfo = MemAllocStruct<DiscoveredAddress>();
            pInfo->ipAddr = ipAddr;
            pInfo->ipAddr.setMaskBits(interfaceAddress.getMaskBits());
            pInfo->zoneUIN = node->getZoneUIN();
            pInfo->ignoreFilter = FALSE;
            if (macAddr == NULL)
               memset(pInfo->bMacAddr, 0, MAC_ADDR_LENGTH);
            else
               memcpy(pInfo->bMacAddr, macAddr, MAC_ADDR_LENGTH);
            pInfo->sourceType = sourceType;
            pInfo->sourceNodeId = sourceNodeId;
            nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 5, _T("New node queued: %s/%d"),
                      pInfo->ipAddr.toString(buffer), pInfo->ipAddr.getMaskBits());
            g_nodePollerQueue.put(pInfo);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Potential node %s rejected - broadcast/multicast address"), ipAddr.toString(buffer));
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Interface object found but IP address not found"));
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Interface object not found"));
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

	nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Checking host route %s at %d"), IpToStr(route->dwDestAddr, buffer), route->dwIfIndex);
	iface = node->findInterfaceByIndex(route->dwIfIndex);
	if ((iface != NULL) && iface->getIpAddressList()->findSameSubnetAddress(route->dwDestAddr).isValidUnicast())
	{
		CheckPotentialNode(node, route->dwDestAddr, route->dwIfIndex, NULL, DA_SRC_ROUTING_TABLE, node->getId());
	}
	else
	{
		nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Interface object not found for host route"));
	}
}

/**
 * Discovery poller
 */
static void DiscoveryPoller(void *arg)
{
   PollerInfo *poller = (PollerInfo *)arg;
   poller->startExecution();

   if (IsShutdownInProgress())
   {
      delete poller;
      return;
   }

   Node *node = (Node *)poller->getObject();
	if (node->getRuntimeFlags() & DCDF_DELETE_IN_PROGRESS)
	{
      node->setDiscoveryPollTimeStamp();
      delete poller;
      return;
	}

   nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("Starting discovery poll for node %s (%s) in zone %d"),
	          node->getName(), (const TCHAR *)node->getIpAddress().toString(), (int)node->getZoneUIN());

   // Retrieve and analyze node's ARP cache
   ArpCache *pArpCache = node->getArpCache(true);
   if (pArpCache != NULL)
   {
      for(int i = 0; i < pArpCache->size(); i++)
      {
         const ArpEntry *e = pArpCache->get(i);
			if (!e->macAddr.isBroadcast())	// Ignore broadcast addresses
				CheckPotentialNode(node, e->ipAddr, e->ifIndex, e->macAddr.value(), DA_SRC_ARP_CACHE, node->getId());
      }
      pArpCache->decRefCount();
   }

	// Retrieve and analyze node's routing table
   nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 5, _T("Discovery poll for node %s (%s) - reading routing table"),
             node->getName(), (const TCHAR *)node->getIpAddress().toString());
	ROUTING_TABLE *rt = node->getRoutingTable();
	if (rt != NULL)
	{
		for(int i = 0; i < rt->iNumEntries; i++)
		{
			CheckPotentialNode(node, rt->pRoutes[i].dwNextHop, rt->pRoutes[i].dwIfIndex, NULL, DA_SRC_ROUTING_TABLE, node->getId());
			if ((rt->pRoutes[i].dwDestMask == 0xFFFFFFFF) && (rt->pRoutes[i].dwDestAddr != 0))
				CheckHostRoute(node, &rt->pRoutes[i]);
		}
		DestroyRoutingTable(rt);
	}

	node->executeHookScript(_T("DiscoveryPoll"));

   nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("Finished discovery poll for node %s (%s)"),
             node->getName(), (const TCHAR *)node->getIpAddress().toString());
   node->setDiscoveryPollTimeStamp();
   delete poller;
}

/**
 * Callback for address range scan
 */
static void RangeScanCallback(const InetAddress& addr, UINT32 rtt, void *context)
{
   nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 5, _T("Active discovery - node %s responded to ICMP ping"), (const TCHAR *)addr.toString());
   CheckPotentialNode(addr, 0, DA_SRC_ACTIVE_DISCOVERY, 0);
}

/**
 * Check given address range with ICMP ping for new nodes
 */
static void CheckRange(const InetAddressListElement& range)
{
   if (range.getBaseAddress().getFamily() != AF_INET)
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("Active discovery on range %s skipped - only IPv4 ranges supported"), (const TCHAR *)range.toString());
      return;
   }

   UINT32 from = range.getBaseAddress().getAddressV4();
   UINT32 to;
   if (range.getType() == InetAddressListElement_SUBNET)
   {
      from++;
      to = range.getBaseAddress().getSubnetBroadcast().getAddressV4() - 1;
   }
   else
   {
      to = range.getEndAddress().getAddressV4();
   }

   if (from >= to)
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("Invalid address range %s"), (const TCHAR *)range.toString());
      return;
   }

   if ((range.getZoneUIN() != 0) || (range.getProxyId() != 0))
   {
      UINT32 proxyId;
      if (range.getProxyId() != 0)
      {
         proxyId = range.getProxyId();
      }
      else
      {
         Zone *zone = FindZoneByUIN(range.getZoneUIN());
         if (zone == NULL)
         {
            nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("Invalid zone UIN for address range %s"), (const TCHAR *)range.toString());
            return;
         }
         proxyId = zone->getProxyNodeId(NULL);
      }

      Node *proxy = static_cast<Node*>(FindObjectById(proxyId, OBJECT_NODE));
      if (proxy == NULL)
      {
         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("Cannot find zone proxy node for address range %s"), (const TCHAR *)range.toString());
         return;
      }

      AgentConnectionEx *conn = proxy->createAgentConnection();
      if (conn == NULL)
      {
         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("Cannot connect to proxy agent for address range %s"), (const TCHAR *)range.toString());
         return;
      }
      conn->setCommandTimeout(10000);

      TCHAR ipAddr1[16], ipAddr2[16];
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("Starting active discovery check on range %s - %s via proxy %s [%u]"),
               IpToStr(from, ipAddr1), IpToStr(to, ipAddr2), proxy->getName(), proxy->getId());
      while((from < to) && !IsShutdownInProgress())
      {
         TCHAR request[256];
         _sntprintf(request, 256, _T("ICMP.ScanRange(%s,%s)"), IpToStr(from, ipAddr1), IpToStr(std::min(to, from + 255), ipAddr2));
         StringList *list;
         if (conn->getList(request, &list) == ERR_SUCCESS)
         {
            for(int i = 0; i < list->size(); i++)
            {
               nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 5, _T("Active discovery - node %s responded to ICMP ping via proxy %s [%u]"),
                        list->get(i), proxy->getName(), proxy->getId());
               CheckPotentialNode(InetAddress::parse(list->get(i)), range.getZoneUIN(), DA_SRC_ACTIVE_DISCOVERY, proxy->getId());
            }
            delete list;
         }
         from += 256;
      }
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("Finished active discovery check on range %s - %s via proxy %s [%u]"),
               IpToStr(from, ipAddr1), IpToStr(to, ipAddr2), proxy->getName(), proxy->getId());

      conn->decRefCount();
   }
   else
   {
      TCHAR ipAddr1[16], ipAddr2[16];
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("Starting active discovery check on range %s - %s"), IpToStr(from, ipAddr1), IpToStr(to, ipAddr2));
      while((from < to) && !IsShutdownInProgress())
      {
         ScanAddressRange(from, std::min(to, from + 1023), RangeScanCallback, NULL);
         from += 1024;
      }
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("Finished active discovery check on range %s - %s"), ipAddr1, ipAddr2);
   }
}

/**
 * Active discovery poller thread
 */
static THREAD_RESULT THREAD_CALL ActiveDiscoveryPoller(void *arg)
{
   ThreadSetName("ActiveDiscovery");
   int nInterval = ConfigReadInt(_T("ActiveDiscoveryInterval"), 7200);

   // Main loop
   while(!IsShutdownInProgress())
   {
      if (SleepAndCheckForShutdown(nInterval))
         break;

      if (!(g_flags & AF_ACTIVE_NETWORK_DISCOVERY))
         continue;

      ObjectArray<InetAddressListElement> *addressList = LoadServerAddressList(1);
      if (addressList != NULL)
      {
         for(int i = 0; (i < addressList->size()) && !IsShutdownInProgress(); i++)
         {
            CheckRange(*addressList->get(i));
         }
         delete addressList;
      }
   }
   return THREAD_OK;
}

/**
 * Callback for queuing objects for polling
 */
static void QueueForPolling(NetObj *object, void *data)
{
   WatchdogNotify(*static_cast<UINT32*>(data));
   if (IsShutdownInProgress())
      return;

   TCHAR threadKey[32];
   _sntprintf(threadKey, 32, _T("POLL_%u"), object->getId());

   if (object->isDataCollectionTarget())
   {
      DataCollectionTarget *target = (DataCollectionTarget *)object;
      if (target->isReadyForStatusPoll())
      {
         target->lockForStatusPoll();
         nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("Data Collection Target %d \"%s\" queued for status poll"), (int)target->getId(), target->getName());
         ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, target, &DataCollectionTarget::statusPollWorkerEntry, RegisterPoller(POLLER_TYPE_STATUS, target));
      }
      if (target->isReadyForConfigurationPoll())
      {
         target->lockForConfigurationPoll();
         nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("Data Collection Target %d \"%s\" queued for configuration poll"), (int)target->getId(), target->getName());
         ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, target, &DataCollectionTarget::configurationPollWorkerEntry, RegisterPoller(POLLER_TYPE_CONFIGURATION, target));
      }
      if (target->isReadyForInstancePoll())
      {
         target->lockForInstancePoll();
         nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("Data Collection Target %d \"%s\" queued for instance discovery poll"), (int)target->getId(), target->getName());
         ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, target, &DataCollectionTarget::instanceDiscoveryPollWorkerEntry, RegisterPoller(POLLER_TYPE_INSTANCE_DISCOVERY, target));
      }
   }

	switch(object->getObjectClass())
	{
		case OBJECT_NODE:
			{
				Node *node = (Node *)object;
				if (node->isReadyForRoutePoll())
				{
					node->lockForRoutePoll();
					nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("Node %d \"%s\" queued for routing table poll"), (int)node->getId(), node->getName());
					ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, node, &Node::routingTablePollWorkerEntry, RegisterPoller(POLLER_TYPE_ROUTING_TABLE, node));
				}
				if (node->isReadyForDiscoveryPoll())
				{
					node->lockForDiscoveryPoll();
					nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("Node %d \"%s\" queued for discovery poll"), (int)node->getId(), node->getName());
					ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, DiscoveryPoller, RegisterPoller(POLLER_TYPE_DISCOVERY, node));
				}
				if (node->isReadyForTopologyPoll())
				{
					node->lockForTopologyPoll();
					nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("Node %d \"%s\" queued for topology poll"), (int)node->getId(), node->getName());
					ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, node, &Node::topologyPollWorkerEntry, RegisterPoller(POLLER_TYPE_TOPOLOGY, node));
				}
			}
			break;
		case OBJECT_CONDITION:
			{
			   ConditionObject *cond = (ConditionObject *)object;
				if (cond->isReadyForPoll())
				{
					cond->lockForPoll();
					nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("Condition %d \"%s\" queued for poll"), (int)object->getId(), object->getName());
					ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, cond, &ConditionObject::doPoll, RegisterPoller(POLLER_TYPE_CONDITION, cond));
				}
			}
			break;
		case OBJECT_BUSINESSSERVICE:
			{
				BusinessService *service = (BusinessService *)object;
				if (service->isReadyForPolling())
				{
					service->lockForPolling();
					nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("Business service %d \"%s\" queued for poll"), (int)object->getId(), object->getName());
					ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, service, &BusinessService::poll, RegisterPoller(POLLER_TYPE_BUSINESS_SERVICE, service));
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
THREAD_RESULT THREAD_CALL PollManager(void *arg)
{
   ThreadSetName("PollManager");
   g_pollerThreadPool = ThreadPoolCreate( _T("POLLERS"),
         ConfigReadInt(_T("ThreadPool.Poller.BaseSize"), 10),
         ConfigReadInt(_T("ThreadPool.Poller.MaxSize"), 250),
         256 * 1024);

   // Start active discovery poller
   ThreadCreate(ActiveDiscoveryPoller, 0, NULL);

   UINT32 watchdogId = WatchdogAddThread(_T("Poll Manager"), 5);
   int counter = 0;

   ConditionSet(static_cast<CONDITION>(arg));

   WatchdogStartSleep(watchdogId);
   while(!SleepAndCheckForShutdown(5))
   {
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
		g_idxObjectById.forEach(QueueForPolling, &watchdogId);
	   WatchdogStartSleep(watchdogId);
   }

   // Clear node poller queue
   DiscoveredAddress *nodeInfo;
   while((nodeInfo = g_nodePollerQueue.get()) != NULL)
   {
      if (nodeInfo != INVALID_POINTER_VALUE)
         MemFree(nodeInfo);
   }
   g_nodePollerQueue.put(INVALID_POINTER_VALUE);

   nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 2, _T("Waiting for outstanding poll requests"));
   ThreadPoolDestroy(g_pollerThreadPool);
   nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 1, _T("Poll manager main thread terminated"));
   return THREAD_OK;
}

/**
 * Reset discovery poller after configuration change
 */
void ResetDiscoveryPoller()
{
   // Clear node poller queue
   DiscoveredAddress *nodeInfo;
   while((nodeInfo = g_nodePollerQueue.get()) != NULL)
   {
      if (nodeInfo != INVALID_POINTER_VALUE)
         MemFree(nodeInfo);
   }

   // Reload discovery parameters
   g_dwDiscoveryPollingInterval = ConfigReadInt(_T("DiscoveryPollingInterval"), 900);
   if (ConfigReadBoolean(_T("RunNetworkDiscovery"), false))
      g_flags |= AF_ENABLE_NETWORK_DISCOVERY;
   else
      g_flags &= ~AF_ENABLE_NETWORK_DISCOVERY;

   if (ConfigReadBoolean(_T("ActiveNetworkDiscovery"), false))
      g_flags |= AF_ACTIVE_NETWORK_DISCOVERY;
   else
      g_flags &= ~AF_ACTIVE_NETWORK_DISCOVERY;

   if (ConfigReadBoolean(_T("UseSNMPTrapsForDiscovery"), false))
      g_flags |= AF_SNMP_TRAP_DISCOVERY;
   else
      g_flags &= ~AF_SNMP_TRAP_DISCOVERY;

   if (ConfigReadBoolean(_T("UseSyslogForDiscovery"), false))
      g_flags |= AF_SYSLOG_DISCOVERY;
   else
      g_flags &= ~AF_SYSLOG_DISCOVERY;
}
