/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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

#define DEBUG_TAG_POLL_MANAGER   _T("poll.manager")

void ActiveDiscoveryPoller();
void WakeupActiveDiscoveryThread();

/**
 * Stop node discovery poller
 */
void StopDiscoveryPoller();

/**
 * Thread pool for pollers
 */
ThreadPool *g_pollerThreadPool = nullptr;

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
PollerInfo *RegisterPoller(PollerType type, const shared_ptr<NetObj>& object)
{
   PollerInfo *p = new PollerInfo(type, object);
   s_pollerLock.lock();
   s_pollers.set(CAST_FROM_POINTER(p, uint64_t), p);
   s_pollerLock.unlock();
   return p;
}

/**
 * Show pollers on console
 */
void ShowPollers(ServerConsole *console)
{
   console->print(_T("Type | Object ID | Object name                    | Status\n")
                  _T("-----+-----------+--------------------------------+--------------------------\n"));

   s_pollerLock.lock();
   s_pollers.forEach(
      [console] (const uint64_t& key, PollerInfo *poller) -> EnumerationCallbackResult
      {
         static const TCHAR *pollerType[] = { _T("STAT"), _T("CONF"), _T("INST"), _T("ROUT"), _T("DISC"), _T("TOPO"), _T("ICMP"), _T("BIND"), _T("MAP ") };

         NetObj *o = poller->getObject();

         TCHAR name[32];
         _tcslcpy(name, o->getName(), 31);
         console->printf(_T("%s | %9d | %-30s | %s\n"), pollerType[static_cast<int>(poller->getType())], o->getId(), name, poller->getStatus());

         return _CONTINUE;
      });
   s_pollerLock.unlock();
}

/**
 * Show poller summary on console
 */
void ShowPollerSummary(ServerConsole *console)
{
   int pollerCount[9];
   memset(pollerCount, 0, sizeof(pollerCount));

   s_pollerLock.lock();
   int total = s_pollers.size();
   s_pollers.forEach(
      [&pollerCount] (const uint64_t& key, PollerInfo *poller) -> EnumerationCallbackResult
      {
         pollerCount[static_cast<int>(poller->getType())]++;
         return _CONTINUE;
      });
   s_pollerLock.unlock();

   console->printf(
      _T("\n")
      _T("Status .............. %d\n")
      _T("Configuration ....... %d\n")
      _T("Instance discovery .. %d\n")
      _T("Routing table ....... %d\n")
      _T("Discovery ........... %d\n")
      _T("Topology ............ %d\n")
      _T("ICMP ................ %d\n")
      _T("Auto bind ........... %d\n")
      _T("Map update .......... %d\n")
      _T("Total ............... %d\n\n"),
      pollerCount[0], pollerCount[1], pollerCount[2], pollerCount[3], pollerCount[4],
      pollerCount[5], pollerCount[6], pollerCount[7], pollerCount[8], total);
}

/**
 * Get total number of pollers
 */
int GetTotalPollerCount()
{
   s_pollerLock.lock();
   int count = s_pollers.size();
   s_pollerLock.unlock();
   return count;
}

/**
 * Get number of pollers of given type
 */
int GetPollerCount(PollerType type)
{
   int count = 0;
   s_pollerLock.lock();
   s_pollers.forEach(
      [&count, type] (const uint64_t& key, PollerInfo *poller) -> EnumerationCallbackResult
      {
         if (poller->getType() == type)
            count++;
         return _CONTINUE;
      });
   s_pollerLock.unlock();
   return count;
}

/**
 * Create management node object
 */
static void CreateManagementNode(const InetAddress& addr)
{
	NewNodeData newNodeData(addr);
	auto node = make_shared<Node>(&newNodeData, 0);
	node->setLocalMgmtFlag();
   NetObjInsert(node, true, false);
   TCHAR buffer[256];
	node->setName(GetLocalHostName(buffer, 256, false));
	node->setComments(_T("NetXMS server node (created automatically by server process)"));

   static_cast<Pollable&>(*node).doForcedConfigurationPoll(RegisterPoller(PollerType::CONFIGURATION, node));

   node->unhide();
   g_dwMgmtNode = node->getId();   // Set local management node ID
   PostSystemEvent(EVENT_NODE_ADDED, node->getId());

	// Bind to the root of service tree
   NetObj::linkObjects(g_infrastructureServiceRoot, node);
}

/**
 * Callback to clear incorrectly set local management node flag
 */
static EnumerationCallbackResult CheckMgmtFlagCallback(NetObj *object, void *data)
{
	if ((g_dwMgmtNode != object->getId()) && static_cast<Node*>(object)->isLocalManagement())
	{
	   static_cast<Node*>(object)->clearLocalMgmtFlag();
		nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 2, _T("Incorrectly set flag NC_IS_LOCAL_MGMT cleared from node %s [%d]"),
					 object->getName(), object->getId());
	}
	return _CONTINUE;
}

/**
 * Check if management server node presented in node list
 */
void CheckForMgmtNode()
{
   bool roaming = ConfigReadBoolean(_T("Server.RoamingMode"), false);

   if (roaming)
   {
      shared_ptr<NetObj> mgmtNode = g_idxNodeById.find([](NetObj *object, void *) { return static_cast<Node*>(object)->isLocalManagement(); }, nullptr);
      if (mgmtNode != nullptr)
      {
         g_dwMgmtNode = mgmtNode->getId();
         if (!mgmtNode->getPrimaryIpAddress().isLoopback())
            static_cast<Node&>(*mgmtNode).setPrimaryHostName(_T("127.0.0.1"));
      }
      else
      {
         CreateManagementNode(InetAddress(0x7F000001));
      }
   }
   else
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
               if (!node->isLocalManagement())
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
		shared_ptr<NetObj> mgmtNode = g_idxNodeById.find([](NetObj *object, void *) { return static_cast<Node*>(object)->isLocalManagement(); }, nullptr);
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
 * Callback for queuing objects for polling
 */
static EnumerationCallbackResult QueueForPolling(NetObj *object, void *data)
{
   WatchdogNotify(*static_cast<uint32_t*>(data));
   if (IsShutdownInProgress())
      return _STOP;

   // Only objects that are not yet completed construction or being
   // prepared for deletion are hidden, so any kind of polling should not be scheduled
   if (object->isHidden())
      return _CONTINUE;

   Pollable* pollableObject = object->getAsPollable();
   if (pollableObject == nullptr)
      return _CONTINUE;

   TCHAR threadKey[32];
   _sntprintf(threadKey, 32, _T("POLL_%u"), object->getId());

   if (pollableObject->isStatusPollAvailable() && pollableObject->lockForStatusPoll())
   {
      nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("%s %s [%u] queued for status poll"), object->getObjectClassName(), object->getName(), object->getId());
      ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, pollableObject, &Pollable::doStatusPoll, RegisterPoller(PollerType::STATUS, object->self()));
   }

   if (pollableObject->isConfigurationPollAvailable() && pollableObject->lockForConfigurationPoll())
   {
      nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("%s %s [%u] queued for configuration poll"), object->getObjectClassName(), object->getName(), object->getId());
      ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, pollableObject, &Pollable::doConfigurationPoll, RegisterPoller(PollerType::CONFIGURATION, object->self()));
   }

   if (pollableObject->isInstanceDiscoveryPollAvailable() && pollableObject->lockForInstanceDiscoveryPoll())
   {
      nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("%s %s [%u] queued for instance discovery poll"), object->getObjectClassName(), object->getName(), object->getId());
      ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, pollableObject, &Pollable::doInstanceDiscoveryPoll, RegisterPoller(PollerType::INSTANCE_DISCOVERY, object->self()));
   }

   if (pollableObject->isTopologyPollAvailable() && pollableObject->lockForTopologyPoll())
   {
      nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("%s %s [%u] queued for topology poll"), object->getObjectClassName(), object->getName(), object->getId());
      ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, pollableObject, &Pollable::doTopologyPoll, RegisterPoller(PollerType::TOPOLOGY, object->self()));
   }

   if (pollableObject->isRoutingTablePollAvailable() && pollableObject->lockForRoutingTablePoll())
   {
      nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("%s %s [%u] queued for routing table poll"), object->getObjectClassName(), object->getName(), object->getId());
      ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, pollableObject, &Pollable::doRoutingTablePoll, RegisterPoller(PollerType::ROUTING_TABLE, object->self()));
   }

   if (pollableObject->isDiscoveryPollAvailable() && pollableObject->lockForDiscoveryPoll())
   {
      nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("%s %s [%u] queued for discovery poll"), object->getObjectClassName(), object->getName(), object->getId());
      ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, pollableObject, &Pollable::doDiscoveryPoll, RegisterPoller(PollerType::DISCOVERY, object->self()));
   }

   if (pollableObject->isIcmpPollAvailable() && pollableObject->lockForIcmpPoll())
   {
      nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("%s %s [%u] queued for ICMP poll"), object->getObjectClassName(), object->getName(), object->getId());
      ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, pollableObject, &Pollable::doIcmpPoll, RegisterPoller(PollerType::ICMP, object->self()));
   }

   if (pollableObject->isAutobindPollAvailable() && pollableObject->lockForAutobindPoll())
   {
      nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("%s %s [%u] queued for autobind poll"), object->getObjectClassName(), object->getName(), object->getId());
      ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, pollableObject, &Pollable::doAutobindPoll, RegisterPoller(PollerType::AUTOBIND, object->self()));
   }

   if (pollableObject->isMapUpdatePollAvailable() && pollableObject->lockForMapUpdatePoll())
   {
      nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 6, _T("%s %s [%u] queued for map update poll"), object->getObjectClassName(), object->getName(), object->getId());
      ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, pollableObject, &Pollable::doMapUpdatePoll, RegisterPoller(PollerType::MAP_UPDATE, object->self()));
   }

   return _CONTINUE;
}

/**
 * Node and condition queuing thread
 */
void PollManager(Condition *startCondition)
{
   ThreadSetName("PollManager");

   // Start active discovery poller
   THREAD activeDiscoveryPollerThread = ThreadCreateEx(ActiveDiscoveryPoller);

   uint32_t watchdogId = WatchdogAddThread(_T("Poll Manager"), 5);
   int counter = 0;

   startCondition->set();

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

   WakeupActiveDiscoveryThread();
   ThreadJoin(activeDiscoveryPollerThread);

   StopDiscoveryPoller();

   nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 2, _T("Waiting for outstanding poll requests"));
   ThreadPoolDestroy(g_pollerThreadPool);

   nxlog_debug_tag(DEBUG_TAG_POLL_MANAGER, 1, _T("Poll manager main thread terminated"));
}
