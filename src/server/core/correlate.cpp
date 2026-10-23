/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: correlate.cpp
**
**/

#include "nxcore.h"

#define DEBUG_TAG _T("event.corr")

/**
 * Static data
 */
static uint64_t m_networkLostEventId = 0;

/**
 * Correlate current event to agent unreachable event
 */
static bool CheckAgentDown(Node *currNode, Event *event, uint32_t nodeId, const wchar_t *nodeType)
{
	shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectById(nodeId, OBJECT_NODE));
	if ((node != nullptr) && node->isNativeAgent() && (node->getState() & NSF_AGENT_UNREACHABLE))
	{
		event->setRootId(node->getLastEventId(LAST_EVENT_AGENT_DOWN));
		nxlog_debug_tag(DEBUG_TAG, 5, L"CheckAgentDown: agent on %s %s [%u] for current node %s [%u] is down",
		          nodeType, node->getName(), node->getId(), currNode->getName(), currNode->getId());
		return true;
	}
	return false;
}

/**
 * Correlate SYS_NODE_DOWN event
 */
static void C_SysNodeDown(Node *node, Event *event)
{
   if (!ConfigReadBoolean(L"Events.Correlation.TopologyBased", true))
   {
      nxlog_debug_tag(DEBUG_TAG, 6, L"C_SysNodeDown: topology based event correlation disabled");
      return;
   }

	// Check for NetXMS server network connectivity
	if (g_flags & AF_NO_NETWORK_CONNECTIVITY)
	{
		event->setRootId(m_networkLostEventId);
		return;
	}

	// Check proxy nodes
	if (IsZoningEnabled() && (node->getZoneUIN() != 0))
	{
		shared_ptr<Zone> zone = FindZoneByUIN(node->getZoneUIN());
		if ((zone != nullptr) && !zone->isProxyNode(node->getId()) && (node->getAssignedZoneProxyId() != 0))
		{
		   if (CheckAgentDown(node, event, node->getAssignedZoneProxyId(), L"zone proxy"))
		      return;
			event->setRootId(0);
		}
	}
}

/**
 * Correlate SYS_NODE_UNREACHABLE event
 */
static void C_SysNodeUnreachable(Node *node, Event *event)
{
   if (!ConfigReadBoolean(L"Events.Correlation.TopologyBased", true))
   {
      nxlog_debug_tag(DEBUG_TAG, 6, L"C_SysNodeUnreachable: topology based event correlation disabled");
      return;
   }

   shared_ptr<NetObj> object;
   NetworkPathFailureReason reason = NetworkPathFailureReasonFromInt(event->getParameterAsInt32(0, 0));
   switch(reason)
   {
      case NetworkPathFailureReason::PROXY_NODE_DOWN:
      case NetworkPathFailureReason::ROUTER_DOWN:
      case NetworkPathFailureReason::SWITCH_DOWN:
         object = FindObjectById(event->getParameterAsUInt32(2, 0), OBJECT_NODE);
         if (object != nullptr)
         {
            event->setRootId(static_cast<const Node&>(*object).getLastEventId(LAST_EVENT_NODE_DOWN));
         }
         break;
      case NetworkPathFailureReason::PROXY_AGENT_UNREACHABLE:
         object = FindObjectById(event->getParameterAsUInt32(2, 0), OBJECT_NODE);
         if (object != nullptr)
         {
            event->setRootId(static_cast<const Node&>(*object).getLastEventId(LAST_EVENT_AGENT_DOWN));
         }
         break;
      case NetworkPathFailureReason::AGENT_TUNNEL_DOWN:
         // Check for NetXMS server network connectivity (tunnel-only nodes have no
         // intermediate path objects, so the beacon check is the only correlation source)
         if (g_flags & AF_NO_NETWORK_CONNECTIVITY)
         {
            event->setRootId(m_networkLostEventId);
         }
         break;
      default:
         break;
   }
}

/**
 * Correlate node event
 */
static void CorrelateNodeEvent(Event *event, Node *node)
{
   switch(event->getCode())
   {
      case EVENT_INTERFACE_DISABLED:
         {
            shared_ptr<Interface> pInterface = node->findInterfaceByIndex(event->getParameterAsUInt32(4));
            if (pInterface != nullptr)
            {
               pInterface->setLastDownEventId(event->getId());
            }
         }
         break;
      case EVENT_INTERFACE_DOWN:
      case EVENT_INTERFACE_EXPECTED_DOWN:
         {
            shared_ptr<Interface> pInterface = node->findInterfaceByIndex(event->getParameterAsUInt32(4));
            if (pInterface != nullptr)
            {
               pInterface->setLastDownEventId(event->getId());
            }
         }
         // there are intentionally no break
      case EVENT_SERVICE_DOWN:
         if (node->getState() & DCSF_UNREACHABLE)
         {
            event->setRootId(node->getLastEventId(LAST_EVENT_NODE_DOWN));
         }
         break;
      case EVENT_SNMP_FAIL:
         if (node->getState() & DCSF_UNREACHABLE)
         {
            event->setRootId(node->getLastEventId(LAST_EVENT_NODE_DOWN));
         }
         else
         {
            uint32_t proxyId = node->getEffectiveSnmpProxy();
            if (proxyId != 0)
               CheckAgentDown(node, event, proxyId, L"SNMP proxy");
         }
         break;
      case EVENT_ICMP_UNREACHABLE:
         if (node->getState() & DCSF_UNREACHABLE)
         {
            event->setRootId(node->getLastEventId(LAST_EVENT_NODE_DOWN));
         }
         else
         {
            uint32_t proxyId = node->getEffectiveIcmpProxy();
            if (proxyId != 0)
               CheckAgentDown(node, event, proxyId, L"ICMP proxy");
         }
         break;
      case EVENT_ETHERNET_IP_UNREACHABLE:
         if (node->getState() & DCSF_UNREACHABLE)
         {
            event->setRootId(node->getLastEventId(LAST_EVENT_NODE_DOWN));
         }
         else
         {
            uint32_t proxyId = node->getEffectiveEtherNetIPProxy();
            if (proxyId != 0)
               CheckAgentDown(node, event, proxyId, L"EtherNet/IP proxy");
         }
         break;
      case EVENT_SSH_UNREACHABLE:
         if (node->getState() & DCSF_UNREACHABLE)
         {
            event->setRootId(node->getLastEventId(LAST_EVENT_NODE_DOWN));
         }
         else
         {
            uint32_t proxyId = node->getEffectiveSshProxy();
            if (proxyId != 0)
               CheckAgentDown(node, event, proxyId, L"SSH proxy");
         }
         break;
      case EVENT_MODBUS_UNREACHABLE:
         if (node->getState() & DCSF_UNREACHABLE)
         {
            event->setRootId(node->getLastEventId(LAST_EVENT_NODE_DOWN));
         }
         else
         {
            uint32_t proxyId = node->getEffectiveModbusProxy();
            if (proxyId != 0)
               CheckAgentDown(node, event, proxyId, L"Modbus proxy");
         }
         break;
      case EVENT_AGENT_FAIL:
         node->setLastEventId(LAST_EVENT_AGENT_DOWN, event->getId());
         if (node->getState() & DCSF_UNREACHABLE)
         {
            event->setRootId(node->getLastEventId(LAST_EVENT_NODE_DOWN));
         }
         break;
      case EVENT_AGENT_OK:
         node->setLastEventId(LAST_EVENT_AGENT_DOWN, 0);
         break;
      case EVENT_NODE_DOWN:
         node->setLastEventId(LAST_EVENT_NODE_DOWN, event->getId());
         C_SysNodeDown(node, event);
         break;
      case EVENT_NODE_UNREACHABLE:
         node->setLastEventId(LAST_EVENT_NODE_DOWN, event->getId());
         C_SysNodeUnreachable(node, event);
         break;
      case EVENT_NODE_UP:
         node->setLastEventId(LAST_EVENT_NODE_DOWN, 0);
         break;
      case EVENT_NETWORK_CONNECTION_LOST:
         m_networkLostEventId = event->getId();
         break;
      case EVENT_ROUTING_LOOP_DETECTED:
         node->setRoutingLoopEvent(InetAddress::parse(event->getNamedParameter(L"destAddress", L"")), event->getNamedParameterAsUInt32(L"destNodeId"), event->getId());
         break;
      default:
         break;
   }
}

/**
 * Correlate SYS_AP_DOWN event
 */
static void C_SysApDown(AccessPoint *ap, Event *event)
{
   if (!ConfigReadBoolean(L"Events.Correlation.TopologyBased", true))
   {
      nxlog_debug_tag(DEBUG_TAG, 6, L"C_SysApDown: topology based event correlation disabled");
      return;
   }

   // Check for NetXMS server network connectivity
   if (g_flags & AF_NO_NETWORK_CONNECTIVITY)
   {
      event->setRootId(m_networkLostEventId);
      return;
   }

   // Check switch this AP is connected to
   if (ap->getPeerNodeId() != 0)
   {
      shared_ptr<NetObj> peerNode = FindObjectById(ap->getPeerNodeId(), OBJECT_NODE);
      if ((peerNode != nullptr) && ((peerNode->getState() & (DCSF_UNREACHABLE | DCSF_NETWORK_PATH_PROBLEM)) == DCSF_UNREACHABLE))
      {
         event->setRootId(static_cast<Node&>(*peerNode).getLastEventId(LAST_EVENT_NODE_DOWN));
      }
   }
}

/**
 * Correlate access point event
 */
static void CorrelateAccessPointEvent(Event *event, AccessPoint *ap)
{
   switch(event->getCode())
   {
      case EVENT_AP_DOWN:
         C_SysApDown(ap, event);
         break;
      default:
         break;
   }
}

/**
 * Correlate event
 */
void CorrelateEvent(Event *event)
{
   shared_ptr<NetObj> source = FindObjectById(event->getSourceId());
   if (source == nullptr)
		return;

	nxlog_debug_tag(DEBUG_TAG, 6, _T("CorrelateEvent: event %s id ") UINT64_FMT _T(" source %s [%u]"),
	          event->getName(), event->getId(), source->getName(), source->getId());

   uint32_t eventCode = event->getCode();
   if (eventCode == EVENT_MAINTENANCE_MODE_ENTERED || eventCode == EVENT_MAINTENANCE_MODE_LEFT)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("CorrelateEvent: finished, maintenance events ignored"));
      return;
   }

   if (source->isInMaintenanceMode() && (eventCode != EVENT_NODE_ADDED))
   {
      if (event->getTimestamp() >= source->getMaintenanceStartTime())
      {
         event->setRootId(source->getMaintenanceEventId());
         nxlog_debug_tag(DEBUG_TAG, 6, _T("CorrelateEvent: finished, rootId=") UINT64_FMT, event->getRootId());
         return;
      }
      nxlog_debug_tag(DEBUG_TAG, 6, _T("CorrelateEvent: event timestamp precedes maintenance mode entry, skipping maintenance correlation"));
   }

   switch(source->getObjectClass())
   {
      case OBJECT_ACCESSPOINT:
         CorrelateAccessPointEvent(event, static_cast<AccessPoint*>(source.get()));
         break;
      case OBJECT_NODE:
         CorrelateNodeEvent(event, static_cast<Node*>(source.get()));
         break;
   }

	nxlog_debug_tag(DEBUG_TAG, 6, _T("CorrelateEvent: finished, rootId=") UINT64_FMT, event->getRootId());
}
