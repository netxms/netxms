/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
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
ThreadPool *g_pollerThreadPool = nullptr;

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
static HashMap<uint64_t, PollerInfo> s_pollers(Ownership::False);
static Mutex s_pollerLock;

/**
 * Poller info destructor - will unregister poller and decrease ref count on object
 */
PollerInfo::~PollerInfo()
{
   s_pollerLock.lock();
   s_pollers.remove(CAST_FROM_POINTER(this, uint64_t));
   s_pollerLock.unlock();
}

/**
 * Register active poller
 */
PollerInfo *RegisterPoller(PollerType type, const shared_ptr<NetObj>& object, bool objectCreation)
{
   PollerInfo *p = new PollerInfo(type, object, objectCreation);
   s_pollerLock.lock();
   s_pollers.set(CAST_FROM_POINTER(p, uint64_t), p);
   s_pollerLock.unlock();
   return p;
}

/**
 * Show poller information on console
 */
static EnumerationCallbackResult ShowPollerInfo(const uint64_t& key, PollerInfo *poller, ServerConsole *console)
{
   static const TCHAR *pollerType[] = { _T("STAT"), _T("CONF"), _T("INST"), _T("ROUT"), _T("DISC"), _T("BSVC"), _T("COND"), _T("TOPO"), _T("ZONE"), _T("ICMP") };

   NetObj *o = poller->getObject();

   TCHAR name[32];
   _tcslcpy(name, o->getName(), 31);
   console->printf(_T("%s | %9d | %-30s | %s\n"), pollerType[static_cast<int>(poller->getType())], o->getId(), name, poller->getStatus());

   return _CONTINUE;
}

/**
 * Get poller diagnostic
 */
void ShowPollers(ServerConsole *console)
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
	shared_ptr<Node> node = MakeSharedNObject<Node>(&newNodeData, 0);
	node->setCapabilities(NC_IS_LOCAL_MGMT);
   NetObjInsert(node, true, false);
	node->setName(GetLocalHostName(buffer, 256, false));

	node->startForcedConfigurationPoll();
	node->configurationPollWorkerEntry(RegisterPoller(PollerType::CONFIGURATION, node->self()));

   node->unhide();
   g_dwMgmtNode = node->getId();   // Set local management node ID
   PostSystemEvent(EVENT_NODE_ADDED, node->getId(), nullptr);

	// Bind to the root of service tree
	g_infrastructureServiceRoot->addChild(node);
	node->addParent(g_infrastructureServiceRoot);
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
   InterfaceList *pIfList = GetLocalInterfaceList();
   if (pIfList != nullptr)
   {
      int i;
      for(i = 0; i < pIfList->size(); i++)
      {
         InterfaceInfo *iface = pIfList->get(i);
         if (iface->type == IFTYPE_SOFTWARE_LOOPBACK)
            continue;

         shared_ptr<Node> node = FindNodeByIP(0, &iface->ipAddrList);
         if (node != nullptr)
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
		g_idxNodeById.forEach(CheckMgmtFlagCallback, nullptr);
	}
	else
	{
		// Management node cannot be found or created. This can happen
		// if management node currently does not have IP addresses (for example,
		// it's a Windows machine which is disconnected from the network).
		// In this case, try to find any node with NC_IS_LOCAL_MGMT flag, or create
		// new one without interfaces
		shared_ptr<NetObj> mgmtNode = g_idxNodeById.find(LocalMgmtNodeComparator, nullptr);
		if (mgmtNode != nullptr)
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
void CheckPotentialNode(const InetAddress& ipAddr, int32_t zoneUIN, DiscoveredAddressSourceType sourceType, UINT32 sourceNodeId)
{
	TCHAR buffer[64];
	nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Checking address %s in zone %d (source: %s)"),
	         ipAddr.toString(buffer), zoneUIN, g_discoveredAddrSourceTypeAsText[sourceType]);
   if (!ipAddr.isValid() || ipAddr.isBroadcast() || ipAddr.isLoopback() || ipAddr.isMulticast())
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Potential node %s rejected (IP address is not a valid unicast address)"), ipAddr.toString(buffer));
      return;
   }

   shared_ptr<Node> curr = FindNodeByIP(zoneUIN, ipAddr);
   if (curr != nullptr)
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

   if (IsNodePollerActiveAddress(ipAddr) || (g_nodePollerQueue.find(&ipAddr, PollerQueueElementComparator) != nullptr))
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Potential node %s rejected (IP address already queued for polling)"), ipAddr.toString(buffer));
      return;
   }

   shared_ptr<Subnet> subnet = FindSubnetForNode(zoneUIN, ipAddr);
   if (subnet != nullptr)
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

   shared_ptr<Node> curr = FindNodeByIP(node->getZoneUIN(), ipAddr);
   if (curr != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Potential node %s rejected (IP address already known at node %s [%d])"),
               ipAddr.toString(buffer), curr->getName(), curr->getId());

      // Check for duplicate IP address
      shared_ptr<Interface> iface = curr->findInterfaceByIP(ipAddr);
      if ((iface != nullptr) && macAddr.isValid() && !iface->getMacAddr().equals(macAddr))
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

   if (IsNodePollerActiveAddress(ipAddr) || (g_nodePollerQueue.find(&ipAddr, PollerQueueElementComparator) != nullptr))
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Potential node %s rejected (IP address already queued for polling)"), ipAddr.toString(buffer));
      return;
   }

   shared_ptr<Interface> pInterface = node->findInterfaceByIndex(ifIndex);
   if (pInterface != nullptr)
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
static void CheckHostRoute(Node *node, const ROUTE *route)
{
	TCHAR buffer[16];
	nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Checking host route %s at %d"), IpToStr(route->dwDestAddr, buffer), route->dwIfIndex);
	shared_ptr<Interface> iface = node->findInterfaceByIndex(route->dwIfIndex);
	if ((iface != nullptr) && iface->getIpAddressList()->findSameSubnetAddress(route->dwDestAddr).isValidUnicast())
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
   if (pArpCache != nullptr)
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
	RoutingTable *rt = node->getRoutingTable();
	if (rt != nullptr)
	{
		for(int i = 0; i < rt->size(); i++)
		{
         ROUTE *route = rt->get(i);
			CheckPotentialNode(node, route->dwNextHop, route->dwIfIndex, MacAddress::NONE, DA_SRC_ROUTING_TABLE, node->getId());
			if ((route->dwDestMask == 0xFFFFFFFF) && (route->dwDestAddr != 0))
				CheckHostRoute(node, route);
		}
		delete rt;
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
void RangeScanCallback(const InetAddress& addr, int32_t zoneUIN, const Node *proxy, uint32_t rtt, ServerConsole *console, void *context)
{
   TCHAR ipAddrText[64];
   if (proxy != nullptr)
   {
      ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 5, _T("Active discovery - node %s responded to ICMP ping via proxy %s [%u]"),
            addr.toString(ipAddrText), proxy->getName(), proxy->getId());
      CheckPotentialNode(addr, zoneUIN, DA_SRC_ACTIVE_DISCOVERY, proxy->getId());
   }
   else
   {
      ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 5, _T("Active discovery - node %s responded to ICMP ping"), addr.toString(ipAddrText));
      CheckPotentialNode(addr, zoneUIN, DA_SRC_ACTIVE_DISCOVERY, 0);
   }
}

/**
 * Check given address range with ICMP ping for new nodes
 */
void CheckRange(const InetAddressListElement& range, void (*callback)(const InetAddress&, int32_t, const Node *, uint32_t, ServerConsole *, void *), ServerConsole *console, void *context)
{
   if (range.getBaseAddress().getFamily() != AF_INET)
   {
      ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, _T("Active discovery on range %s skipped - only IPv4 ranges supported"), (const TCHAR *)range.toString());
      return;
   }

   uint32_t from = range.getBaseAddress().getAddressV4();
   uint32_t to;
   if (range.getType() == InetAddressListElement_SUBNET)
   {
      from++;
      to = range.getBaseAddress().getSubnetBroadcast().getAddressV4() - 1;
   }
   else
   {
      to = range.getEndAddress().getAddressV4();
   }

   if (from > to)
   {
      ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, _T("Invalid address range %s"), range.toString().cstr());
      return;
   }

   uint32_t blockSize = ConfigReadULong(_T("NetworkDiscovery.ActiveDiscovery.BlockSize"), 1024);
   uint32_t interBlockDelay = ConfigReadULong(_T("NetworkDiscovery.ActiveDiscovery.InterBlockDelay"), 0);

   if ((range.getZoneUIN() != 0) || (range.getProxyId() != 0))
   {
      uint32_t proxyId;
      if (range.getProxyId() != 0)
      {
         proxyId = range.getProxyId();
      }
      else
      {
         shared_ptr<Zone> zone = FindZoneByUIN(range.getZoneUIN());
         if (zone == nullptr)
         {
            ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, _T("Invalid zone UIN for address range %s"), range.toString().cstr());
            return;
         }
         proxyId = zone->getProxyNodeId(nullptr);
      }

      shared_ptr<Node> proxy = static_pointer_cast<Node>(FindObjectById(proxyId, OBJECT_NODE));
      if (proxy == nullptr)
      {
         ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, _T("Cannot find zone proxy node for address range %s"), range.toString().cstr());
         return;
      }

      shared_ptr<AgentConnectionEx> conn = proxy->createAgentConnection();
      if (conn == nullptr)
      {
         ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, _T("Cannot connect to proxy agent for address range %s"), (const TCHAR *)range.toString());
         return;
      }
      conn->setCommandTimeout(10000);

      TCHAR ipAddr1[64], ipAddr2[64], rangeText[128];
      _sntprintf(rangeText, 128, _T("%s - %s"), IpToStr(from, ipAddr1), IpToStr(to, ipAddr2));
      ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, _T("Starting active discovery check on range %s via proxy %s [%u]"),
               rangeText, proxy->getName(), proxy->getId());
      while((from <= to) && !IsShutdownInProgress())
      {
         if (interBlockDelay > 0)
            ThreadSleepMs(interBlockDelay);

         TCHAR request[256];
         _sntprintf(request, 256, _T("ICMP.ScanRange(%s,%s)"), IpToStr(from, ipAddr1), IpToStr(std::min(to, from + blockSize - 1), ipAddr2));
         StringList *list;
         if (conn->getList(request, &list) == ERR_SUCCESS)
         {
            for(int i = 0; i < list->size(); i++)
            {
               ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 5, _T("Active discovery - node %s responded to ICMP ping via proxy %s [%u]"),
                        list->get(i), proxy->getName(), proxy->getId());
               callback(InetAddress::parse(list->get(i)), range.getZoneUIN(), proxy.get(), 0, console, nullptr);
            }
            delete list;
         }
         from += blockSize;
      }
      ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, _T("Finished active discovery check on range %s via proxy %s [%u]"),
               rangeText, proxy->getName(), proxy->getId());
   }
   else
   {
      TCHAR ipAddr1[16], ipAddr2[16];
      ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, _T("Starting active discovery check on range %s - %s"), IpToStr(from, ipAddr1), IpToStr(to, ipAddr2));
      while((from <= to) && !IsShutdownInProgress())
      {
         if (interBlockDelay > 0)
            ThreadSleepMs(interBlockDelay);
         ScanAddressRange(from, std::min(to, from + blockSize - 1), callback, console, nullptr);
         from += blockSize;
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
static void ActiveDiscoveryPoller()
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

      time_t now = time(nullptr);

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
            if (!MatchSchedule(schedule, NULL, ltm, now))
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
      if (addressList != nullptr)
      {
         for(int i = 0; (i < addressList->size()) && !IsShutdownInProgress(); i++)
         {
            CheckRange(*addressList->get(i), RangeScanCallback, nullptr, nullptr);
         }
         delete addressList;
      }

      interval = ConfigReadInt(_T("NetworkDiscovery.ActiveDiscovery.Interval"), 7200);
      sleepTime = (interval > 0) ? interval * 1000 : 60000;
   }

   nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 2, _T("Active discovery thread terminated"));
}

/**
 * Callback for queuing objects for polling
 */
static void QueueForPolling(NetObj *object, void *data)
{
   WatchdogNotify(*static_cast<uint32_t*>(data));
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
         ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, target, &DataCollectionTarget::statusPollWorkerEntry, RegisterPoller(PollerType::STATUS, target->self()));
      }
      if (target->lockForConfigurationPoll())
      {
         nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("Data collection target %s [%u] queued for configuration poll"), target->getName(), target->getId());
         ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, target, &DataCollectionTarget::configurationPollWorkerEntry, RegisterPoller(PollerType::CONFIGURATION, target->self()));
      }
      if (target->lockForInstancePoll())
      {
         nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("Data collection target %s [%u] queued for instance discovery poll"), target->getName(), target->getId());
         ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, target, &DataCollectionTarget::instanceDiscoveryPollWorkerEntry, RegisterPoller(PollerType::INSTANCE_DISCOVERY, target->self()));
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
					ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, node, &Node::routingTablePollWorkerEntry, RegisterPoller(PollerType::ROUTING_TABLE, node->self()));
				}
				if (node->lockForDiscoveryPoll())
				{
					nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("Node %s [%u] queued for discovery poll"), node->getName(), node->getId());
					ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, DiscoveryPoller, RegisterPoller(PollerType::DISCOVERY, node->self()));
				}
				if (node->lockForTopologyPoll())
				{
					nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("Node %s [%u] queued for topology poll"), node->getName(), node->getId());
					ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, node, &Node::topologyPollWorkerEntry, RegisterPoller(PollerType::TOPOLOGY, node->self()));
				}
		      if (node->lockForIcmpPoll())
		      {
		         nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("Node %s [%u] queued for ICMP poll"), node->getName(), node->getId());
		         ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, node, &Node::icmpPollWorkerEntry, RegisterPoller(PollerType::ICMP, node->self()));
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
					ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, cond, &ConditionObject::doPoll, RegisterPoller(PollerType::CONDITION, cond->self()));
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
					ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, service, &BusinessService::poll, RegisterPoller(PollerType::BUSINESS_SERVICE, service->self()));
				}
			}
			break;
		case OBJECT_ZONE:
         if (static_cast<Zone*>(object)->lockForHealthCheck())
         {
            nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("Zone %s [%u] queued for health check"), object->getName(), object->getId());
            ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, static_cast<Zone*>(object), &Zone::healthCheck, RegisterPoller(PollerType::ZONE_HEALTH, object->self()));
         }
		   break;
		default:
			break;
	}
}

/**
 * Node and condition queuing thread
 */
void PollManager(CONDITION startCondition)
{
   ThreadSetName("PollManager");
   g_pollerThreadPool = ThreadPoolCreate( _T("POLLERS"),
         ConfigReadInt(_T("ThreadPool.Poller.BaseSize"), 10),
         ConfigReadInt(_T("ThreadPool.Poller.MaxSize"), 250),
         256 * 1024);

   // Start active discovery poller
   THREAD activeDiscoveryPollerThread = ThreadCreateEx(ActiveDiscoveryPoller);

   uint32_t watchdogId = WatchdogAddThread(_T("Poll Manager"), 5);
   int counter = 0;

   ConditionSet(startCondition);

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
   while((nodeInfo = g_nodePollerQueue.get()) != nullptr)
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
}

/**
 * Reset discovery poller after configuration change
 */
void ResetDiscoveryPoller()
{
   // Clear node poller queue
   DiscoveredAddress *nodeInfo;
   while((nodeInfo = g_nodePollerQueue.get()) != nullptr)
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
      CheckRange(*addressList->get(i), RangeScanCallback, nullptr, nullptr);
   }
   delete addressList;
}
