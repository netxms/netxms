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
 * Discovery source type names
 */
const TCHAR *g_discoveredAddrSourceTypeAsText[] = {
         _T("ARP Cache"),
         _T("Routing Table"),
         _T("Agent Registration"),
         _T("SNMP Trap"),
         _T("Syslog"),
         _T("Active Discovery"),
};

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
PollerInfo *RegisterPoller(PollerType type, NetObj *object, bool objectCreation)
{
   PollerInfo *p = new PollerInfo(type, object, objectCreation);
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
   static const TCHAR *pollerType[] = { _T("STAT"), _T("CONF"), _T("INST"), _T("ROUT"), _T("DISC"), _T("BSVC"), _T("COND"), _T("TOPO"), _T("ZONE"), _T("ICMP") };

   PollerInfo *p = (PollerInfo *)object;
   NetObj *o = p->getObject();

   TCHAR name[32];
   _tcslcpy(name, o->getName(), 31);
   ConsolePrintf(static_cast<CONSOLE_CTX>(arg), _T("%s | %9d | %-30s | %s\n"),
            pollerType[static_cast<int>(p->getType())], o->getId(), name, p->getStatus());

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

	node->startForcedConfigurationPoll();
	node->configurationPollWorkerEntry(RegisterPoller(PollerType::CONFIGURATION, node));

   node->unhide();
   g_dwMgmtNode = node->getId();   // Set local management node ID
   PostSystemEvent(EVENT_NODE_ADDED, node->getId(), NULL);

	// Bind to the root of service tree
	g_pServiceRoot->addChild(node);
	node->addParent(g_pServiceRoot);
}

/**
 * Callback to clear incorrectly set local management node flag
 */
static void CheckMgmtFlagCallback(NetObj *object, void *data)
{
	if ((g_dwMgmtNode != object->getId()) && static_cast<Node*>(object)->isLocalManagement())
	{
	   static_cast<Node*>(object)->clearLocalMgmtFlag();
		nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 2, _T("Incorrectly set flag NC_IS_LOCAL_MGMT cleared from node %s [%d]"),
					 object->getName(), object->getId());
	}
}

/**
 * Comparator to find management node object in existing nodes
 */
static bool LocalMgmtNodeComparator(NetObj *object, void *data)
{
	return static_cast<Node*>(object)->isLocalManagement();
}

/**
 * Check if management server node presented in node list
 */
void CheckForMgmtNode()
{
   Node *node;
   int i;

   InterfaceList *pIfList = GetLocalInterfaceList();
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
            if ((iface->type == IFTYPE_SOFTWARE_LOOPBACK) || iface->ipAddrList.isEmpty())
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
static bool PollerQueueElementComparator(const InetAddress *key, const DiscoveredAddress *element)
{
   return key->equals(element->ipAddr);
}

/**
 * Check potential new node from sysog, SNMP trap, or address range scan
 */
void CheckPotentialNode(const InetAddress& ipAddr, UINT32 zoneUIN, DiscoveredAddressSourceType sourceType, UINT32 sourceNodeId)
{
	TCHAR buffer[64];
	nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Checking address %s in zone %d (source: %s)"),
	         ipAddr.toString(buffer), zoneUIN, g_discoveredAddrSourceTypeAsText[sourceType]);
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
static void CheckPotentialNode(Node *node, const InetAddress& ipAddr, UINT32 ifIndex, const MacAddress& macAddr,
         DiscoveredAddressSourceType sourceType, UINT32 sourceNodeId)
{
   TCHAR buffer[64];
   nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Checking potential node %s at %s:%d (source: %s)"),
            ipAddr.toString(buffer), node->getName(), ifIndex, g_discoveredAddrSourceTypeAsText[sourceType]);
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

      // Check for duplicate IP address
      Interface *iface = curr->findInterfaceByIP(ipAddr);
      if ((iface != NULL) && macAddr.isValid() && !iface->getMacAddr().equals(macAddr))
      {
         MacAddress knownMAC = iface->getMacAddr();
         PostSystemEvent(EVENT_DUPLICATE_IP_ADDRESS, g_dwMgmtNode, "AdssHHdss",
                  &ipAddr, curr->getId(), curr->getName(), iface->getName(),
                  &knownMAC, &macAddr, node->getId(), node->getName(),
                  g_discoveredAddrSourceTypeAsText[sourceType]);
      }
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
            if (macAddr.isValid() && (macAddr.length() == MAC_ADDR_LENGTH))
               memcpy(pInfo->bMacAddr, macAddr.value(), MAC_ADDR_LENGTH);
            else
               memset(pInfo->bMacAddr, 0, MAC_ADDR_LENGTH);
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
		CheckPotentialNode(node, route->dwDestAddr, route->dwIfIndex, MacAddress::NONE, DA_SRC_ROUTING_TABLE, node->getId());
	}
	else
	{
		nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Interface object not found for host route"));
	}
}

/**
 * Discovery poller
 */
void DiscoveryPoller(PollerInfo *poller)
{
   poller->startExecution();
   INT64 startTime = GetCurrentTimeMs();

   Node *node = static_cast<Node*>(poller->getObject());
	if (node->isDeleteInitiated() || IsShutdownInProgress())
	{
	   nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Discovery poll for node %s (%s) in zone %d aborted"),
	             node->getName(), (const TCHAR *)node->getIpAddress().toString(), (int)node->getZoneUIN());
      node->completeDiscoveryPoll(GetCurrentTimeMs() - startTime);
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
				CheckPotentialNode(node, e->ipAddr, e->ifIndex, e->macAddr, DA_SRC_ARP_CACHE, node->getId());
      }
      pArpCache->decRefCount();
   }

   if (node->isDeleteInitiated() || IsShutdownInProgress())
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Discovery poll for node %s (%s) in zone %d aborted"),
                node->getName(), (const TCHAR *)node->getIpAddress().toString(), (int)node->getZoneUIN());
      node->completeDiscoveryPoll(GetCurrentTimeMs() - startTime);
      delete poller;
      return;
   }

	// Retrieve and analyze node's routing table
   nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 5, _T("Discovery poll for node %s (%s) - reading routing table"),
             node->getName(), (const TCHAR *)node->getIpAddress().toString());
	ROUTING_TABLE *rt = node->getRoutingTable();
	if (rt != NULL)
	{
		for(int i = 0; i < rt->iNumEntries; i++)
		{
			CheckPotentialNode(node, rt->pRoutes[i].dwNextHop, rt->pRoutes[i].dwIfIndex, MacAddress::NONE, DA_SRC_ROUTING_TABLE, node->getId());
			if ((rt->pRoutes[i].dwDestMask == 0xFFFFFFFF) && (rt->pRoutes[i].dwDestAddr != 0))
				CheckHostRoute(node, &rt->pRoutes[i]);
		}
		DestroyRoutingTable(rt);
	}

	node->executeHookScript(_T("DiscoveryPoll"));

   nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("Finished discovery poll for node %s (%s)"),
             node->getName(), (const TCHAR *)node->getIpAddress().toString());
   node->completeDiscoveryPoll(GetCurrentTimeMs() - startTime);
   delete poller;
}

/**
 * Callback for address range scan
 */
void RangeScanCallback(const InetAddress& addr, UINT32 zoneUIN, Node *proxy, UINT32 rtt, ServerConsole *console, void *context)
{
   if (proxy != NULL)
   {
      ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 5, _T("Active discovery - node %s responded to ICMP ping via proxy %s [%u]"),
            (const TCHAR *)addr.toString(), proxy->getName(), proxy->getId());
      CheckPotentialNode(addr, zoneUIN, DA_SRC_ACTIVE_DISCOVERY, proxy->getId());
   }
   else
   {
      ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 5, _T("Active discovery - node %s responded to ICMP ping"), (const TCHAR *)addr.toString());
      CheckPotentialNode(addr, zoneUIN, DA_SRC_ACTIVE_DISCOVERY, 0);
   }
}

/**
 * Check given address range with ICMP ping for new nodes
 */
void CheckRange(const InetAddressListElement& range, void (*callback)(const InetAddress&, UINT32, Node *, UINT32, ServerConsole *, void *), ServerConsole *console, void *context)
{
   if (range.getBaseAddress().getFamily() != AF_INET)
   {
      ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, _T("Active discovery on range %s skipped - only IPv4 ranges supported"), (const TCHAR *)range.toString());
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
      ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, _T("Invalid address range %s"), (const TCHAR *)range.toString());
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
            ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, _T("Invalid zone UIN for address range %s"), (const TCHAR *)range.toString());
            return;
         }
         proxyId = zone->getProxyNodeId(NULL);
      }

      Node *proxy = static_cast<Node*>(FindObjectById(proxyId, OBJECT_NODE));
      if (proxy == NULL)
      {
         ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, _T("Cannot find zone proxy node for address range %s"), (const TCHAR *)range.toString());
         return;
      }

      AgentConnectionEx *conn = proxy->createAgentConnection();
      if (conn == NULL)
      {
         ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, _T("Cannot connect to proxy agent for address range %s"), (const TCHAR *)range.toString());
         return;
      }
      conn->setCommandTimeout(10000);

      TCHAR ipAddr1[64], ipAddr2[64], rangeText[128];
      _sntprintf(rangeText, 128, _T("%s - %s"), IpToStr(from, ipAddr1), IpToStr(to, ipAddr2));
      ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, _T("Starting active discovery check on range %s via proxy %s [%u]"),
               rangeText, proxy->getName(), proxy->getId());
      while((from < to) && !IsShutdownInProgress())
      {
         TCHAR request[256];
         _sntprintf(request, 256, _T("ICMP.ScanRange(%s,%s)"), IpToStr(from, ipAddr1), IpToStr(std::min(to, from + 255), ipAddr2));
         StringList *list;
         if (conn->getList(request, &list) == ERR_SUCCESS)
         {
            for(int i = 0; i < list->size(); i++)
            {
               ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 5, _T("Active discovery - node %s responded to ICMP ping via proxy %s [%u]"),
                        list->get(i), proxy->getName(), proxy->getId());
               callback(InetAddress::parse(list->get(i)), range.getZoneUIN(), proxy, 0, console, NULL);
            }
            delete list;
         }
         from += 256;
      }
      ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, _T("Finished active discovery check on range %s via proxy %s [%u]"),
               rangeText, proxy->getName(), proxy->getId());

      conn->decRefCount();
   }
   else
   {
      TCHAR ipAddr1[16], ipAddr2[16];
      ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, _T("Starting active discovery check on range %s - %s"), IpToStr(from, ipAddr1), IpToStr(to, ipAddr2));
      while((from < to) && !IsShutdownInProgress())
      {
         ScanAddressRange(from, std::min(to, from + 1023), callback, console, NULL);
         from += 1024;
      }
      ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, _T("Finished active discovery check on range %s - %s"), ipAddr1, ipAddr2);
   }
}

/**
 * Active discovery thread wakeup condition
 */
static CONDITION s_activeDiscoveryWakeup = ConditionCreate(false);

/**
 * Active discovery poller thread
 */
static THREAD_RESULT THREAD_CALL ActiveDiscoveryPoller(void *arg)
{
   ThreadSetName("ActiveDiscovery");

   nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 2, _T("Active discovery thread started"));

   time_t lastRun = 0;
   UINT32 sleepTime = 60000;

   // Main loop
   while(!IsShutdownInProgress())
   {
      ConditionWait(s_activeDiscoveryWakeup, sleepTime);
      if (IsShutdownInProgress())
         break;

      if (!(g_flags & AF_ACTIVE_NETWORK_DISCOVERY))
      {
         sleepTime = INFINITE;
         continue;
      }

      time_t now = time(NULL);

      UINT32 interval = ConfigReadULong(_T("NetworkDiscovery.ActiveDiscovery.Interval"), 7200);
      if (interval != 0)
      {
         if (static_cast<UINT32>(now - lastRun) < interval)
         {
            sleepTime = (interval - static_cast<UINT32>(lastRun - now)) * 1000;
            continue;
         }
      }
      else
      {
         TCHAR schedule[256];
         ConfigReadStr(_T("NetworkDiscovery.ActiveDiscovery.Schedule"), schedule, 256, _T(""));
         if (schedule[0] != 0)
         {
#if HAVE_LOCALTIME_R
            struct tm tmbuffer;
            localtime_r(&now, &tmbuffer);
            struct tm *ltm = &tmbuffer;
#else
            struct tm *ltm = localtime(&now);
#endif
            if (!MatchSchedule(schedule, ltm, now))
            {
               sleepTime = 60000;
               continue;
            }
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 4, _T("Empty active discovery schedule"));
            sleepTime = INFINITE;
            continue;
         }
      }

      ObjectArray<InetAddressListElement> *addressList = LoadServerAddressList(1);
      if (addressList != NULL)
      {
         for(int i = 0; (i < addressList->size()) && !IsShutdownInProgress(); i++)
         {
            CheckRange(*addressList->get(i), RangeScanCallback, NULL, NULL);
         }
         delete addressList;
      }

      interval = ConfigReadInt(_T("NetworkDiscovery.ActiveDiscovery.Interval"), 7200);
      sleepTime = (interval > 0) ? interval * 1000 : 60000;
   }

   nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 2, _T("Active discovery thread terminated"));
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

   // Only objects that are not yet completed construction or being
   // prepared for deletion are hidden, so any kind of polling should not be scheduled
   if (object->isHidden())
      return;

   TCHAR threadKey[32];
   _sntprintf(threadKey, 32, _T("POLL_%u"), object->getId());

   if (object->isDataCollectionTarget())
   {
      auto target = static_cast<DataCollectionTarget*>(object);
      if (target->lockForStatusPoll())
      {
         nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("Data collection target %s [%u] queued for status poll"), target->getName(), target->getId());
         ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, target, &DataCollectionTarget::statusPollWorkerEntry, RegisterPoller(PollerType::STATUS, target));
      }
      if (target->lockForConfigurationPoll())
      {
         nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("Data collection target %s [%u] queued for configuration poll"), target->getName(), target->getId());
         ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, target, &DataCollectionTarget::configurationPollWorkerEntry, RegisterPoller(PollerType::CONFIGURATION, target));
      }
      if (target->lockForInstancePoll())
      {
         nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("Data collection target %s [%u] queued for instance discovery poll"), target->getName(), target->getId());
         ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, target, &DataCollectionTarget::instanceDiscoveryPollWorkerEntry, RegisterPoller(PollerType::INSTANCE_DISCOVERY, target));
      }
   }

	switch(object->getObjectClass())
	{
		case OBJECT_NODE:
			{
				auto node = static_cast<Node*>(object);
				if (node->lockForRoutePoll())
				{
					nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("Node %s [%u] queued for routing table poll"), node->getName(), node->getId());
					ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, node, &Node::routingTablePollWorkerEntry, RegisterPoller(PollerType::ROUTING_TABLE, node));
				}
				if (node->lockForDiscoveryPoll())
				{
					nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("Node %s [%u] queued for discovery poll"), node->getName(), node->getId());
					ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, DiscoveryPoller, RegisterPoller(PollerType::DISCOVERY, node));
				}
				if (node->lockForTopologyPoll())
				{
					nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("Node %s [%u] queued for topology poll"), node->getName(), node->getId());
					ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, node, &Node::topologyPollWorkerEntry, RegisterPoller(PollerType::TOPOLOGY, node));
				}
		      if (node->lockForIcmpPoll())
		      {
		         nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("Node %s [%u] queued for ICMP poll"), node->getName(), node->getId());
		         ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, node, &Node::icmpPollWorkerEntry, RegisterPoller(PollerType::ICMP, node));
		      }
			}
			break;
		case OBJECT_CONDITION:
			{
			   auto cond = static_cast<ConditionObject*>(object);
				if (cond->isReadyForPoll())
				{
					cond->lockForPoll();
					nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("Condition %d \"%s\" queued for poll"), (int)object->getId(), object->getName());
					ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, cond, &ConditionObject::doPoll, RegisterPoller(PollerType::CONDITION, cond));
				}
			}
			break;
		case OBJECT_BUSINESSSERVICE:
			{
				auto service = static_cast<BusinessService*>(object);
				if (service->isReadyForPolling())
				{
					service->lockForPolling();
					nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("Business service %d \"%s\" queued for poll"), (int)object->getId(), object->getName());
					ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, service, &BusinessService::poll, RegisterPoller(PollerType::BUSINESS_SERVICE, service));
				}
			}
			break;
		case OBJECT_ZONE:
         if (static_cast<Zone*>(object)->lockForHealthCheck())
         {
            nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("Zone %s [%u] queued for health check"), object->getName(), object->getId());
            ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, static_cast<Zone*>(object), &Zone::healthCheck, RegisterPoller(PollerType::ZONE_HEALTH, object));
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
   THREAD activeDiscoveryPollerThread = ThreadCreateEx(ActiveDiscoveryPoller, 0, NULL);

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

   ConditionSet(s_activeDiscoveryWakeup);
   ThreadJoin(activeDiscoveryPollerThread);

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

   ConditionDestroy(s_activeDiscoveryWakeup);
   s_activeDiscoveryWakeup = INVALID_CONDITION_HANDLE;

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
   g_discoveryPollingInterval = ConfigReadInt(_T("NetworkDiscovery.PassiveDiscovery.Interval"), 900);

   switch(ConfigReadInt(_T("NetworkDiscovery.Type"), 0))
   {
      case 0:  // Disabled
         g_flags &= ~(AF_PASSIVE_NETWORK_DISCOVERY | AF_ACTIVE_NETWORK_DISCOVERY);
         break;
      case 1:  // Passive only
         g_flags |= AF_PASSIVE_NETWORK_DISCOVERY;
         g_flags &= ~AF_ACTIVE_NETWORK_DISCOVERY;
         break;
      case 2:  // Active only
         g_flags &= ~AF_PASSIVE_NETWORK_DISCOVERY;
         g_flags |= AF_ACTIVE_NETWORK_DISCOVERY;
         break;
      case 3:  // Active and passive
         g_flags |= AF_PASSIVE_NETWORK_DISCOVERY | AF_ACTIVE_NETWORK_DISCOVERY;
         break;
      default:
         break;
   }

   if (ConfigReadBoolean(_T("UseSNMPTrapsForDiscovery"), false))
      g_flags |= AF_SNMP_TRAP_DISCOVERY;
   else
      g_flags &= ~AF_SNMP_TRAP_DISCOVERY;

   if (ConfigReadBoolean(_T("UseSyslogForDiscovery"), false))
      g_flags |= AF_SYSLOG_DISCOVERY;
   else
      g_flags &= ~AF_SYSLOG_DISCOVERY;

   ConditionSet(s_activeDiscoveryWakeup);
}

/**
 * Wakeup active discovery thread
 */
void WakeupActiveDiscoveryThread()
{
   ConditionSet(s_activeDiscoveryWakeup);
}

/**
 * Manual active discovery starter
 */
void StartManualActiveDiscovery(ObjectArray<InetAddressListElement> *addressList)
{
   for(int i = 0; (i < addressList->size()) && !IsShutdownInProgress(); i++)
   {
      CheckRange(*addressList->get(i), RangeScanCallback, NULL, NULL);
   }
   delete addressList;
}
