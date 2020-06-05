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
static bool CheckAgentDown(const shared_ptr<Node>& currNode, Event *pEvent, uint32_t nodeId, const TCHAR *nodeType)
{
	shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectById(nodeId, OBJECT_NODE));
	if ((node != nullptr) && node->isNativeAgent() && (node->getState() & NSF_AGENT_UNREACHABLE))
	{
		pEvent->setRootId(node->getLastEventId(LAST_EVENT_AGENT_DOWN));
		nxlog_debug_tag(DEBUG_TAG, 5, _T("C_SysNodeDown: agent on %s %s [%d] for current node %s [%d] is down"),
		          nodeType, node->getName(), node->getId(), currNode->getName(), currNode->getId());
		return true;
	}
	return false;
}

/**
 * Correlate SYS_NODE_DOWN event
 */
static void C_SysNodeDown(const shared_ptr<Node>& pNode, Event *event)
{
   if (!ConfigReadBoolean(_T("Events.Correlation.TopologyBased"), true))
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("C_SysNodeUnreachable: topology based event correlation disabled"));
      return;
   }

	// Check for NetXMS server network connectivity
	if (g_flags & AF_NO_NETWORK_CONNECTIVITY)
	{
		event->setRootId(m_networkLostEventId);
		return;
	}

	// Check proxy nodes
	if (IsZoningEnabled() && (pNode->getZoneUIN() != 0))
	{
		shared_ptr<Zone> zone = FindZoneByUIN(pNode->getZoneUIN());
		if ((zone != nullptr) && !zone->isProxyNode(pNode->getId()) && (pNode->getAssignedZoneProxyId() != 0))
		{
		   if (CheckAgentDown(pNode, event, pNode->getAssignedZoneProxyId(), _T("zone proxy")))
		      return;
			event->setRootId(0);
		}
	}
}

/**
 * Correlate SYS_NODE_UNREACHABLE event
 */
static void C_SysNodeUnreachable(const shared_ptr<Node>& pNode, Event *event)
{
   if (!ConfigReadBoolean(_T("Events.Correlation.TopologyBased"), true))
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("C_SysNodeUnreachable: topology based event correlation disabled"));
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
   }
}

/**
 * Correlate event
 */
void CorrelateEvent(Event *pEvent)
{
   shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectById(pEvent->getSourceId(), OBJECT_NODE));
   if (node == nullptr)
		return;

	nxlog_debug_tag(DEBUG_TAG, 6, _T("CorrelateEvent: event %s id ") UINT64_FMT _T(" source %s [%d]"),
	          pEvent->getName(), pEvent->getId(), node->getName(), node->getId());

   UINT32 eventCode = pEvent->getCode();
   if (eventCode == EVENT_MAINTENANCE_MODE_ENTERED || eventCode == EVENT_MAINTENANCE_MODE_LEFT)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("CorrelateEvent: finished, maintenance events ignored"));
      return;
   }

   if (node->isInMaintenanceMode() && (eventCode != EVENT_NODE_ADDED))
   {
      pEvent->setRootId(node->getMaintenanceEventId());
      nxlog_debug_tag(DEBUG_TAG, 6, _T("CorrelateEvent: finished, rootId=") UINT64_FMT, pEvent->getRootId());
      return;
   }

   switch(eventCode)
   {
      case EVENT_INTERFACE_DISABLED:
         {
            shared_ptr<Interface> pInterface = node->findInterfaceByIndex(pEvent->getParameterAsUInt32(4));
            if (pInterface != nullptr)
            {
               pInterface->setLastDownEventId(pEvent->getId());
            }
         }
         break;
      case EVENT_INTERFACE_DOWN:
      case EVENT_INTERFACE_EXPECTED_DOWN:
         {
            shared_ptr<Interface> pInterface = node->findInterfaceByIndex(pEvent->getParameterAsUInt32(4));
            if (pInterface != nullptr)
            {
               pInterface->setLastDownEventId(pEvent->getId());
            }
         }
         // there are intentionally no break
      case EVENT_SERVICE_DOWN:
      case EVENT_SNMP_FAIL:
         if (node->getState() & DCSF_UNREACHABLE)
         {
            pEvent->setRootId(node->getLastEventId(LAST_EVENT_NODE_DOWN));
         }
         break;
      case EVENT_AGENT_FAIL:
         node->setLastEventId(LAST_EVENT_AGENT_DOWN, pEvent->getId());
         if (node->getState() & DCSF_UNREACHABLE)
         {
            pEvent->setRootId(node->getLastEventId(LAST_EVENT_NODE_DOWN));
         }
         break;
      case EVENT_AGENT_OK:
         node->setLastEventId(LAST_EVENT_AGENT_DOWN, 0);
         break;
      case EVENT_NODE_DOWN:
         node->setLastEventId(LAST_EVENT_NODE_DOWN, pEvent->getId());
         C_SysNodeDown(node, pEvent);
         break;
      case EVENT_NODE_UNREACHABLE:
         node->setLastEventId(LAST_EVENT_NODE_DOWN, pEvent->getId());
         C_SysNodeUnreachable(node, pEvent);
         break;
      case EVENT_NODE_UP:
         node->setLastEventId(LAST_EVENT_NODE_DOWN, 0);
         break;
      case EVENT_NETWORK_CONNECTION_LOST:
         m_networkLostEventId = pEvent->getId();
         break;
      case EVENT_ROUTING_LOOP_DETECTED:
         node->setRoutingLoopEvent(InetAddress::parse(pEvent->getNamedParameter(_T("destAddress"), _T(""))), pEvent->getNamedParameterAsUInt32(_T("destNodeId")), pEvent->getId());
         break;
      default:
         break;
   }

	nxlog_debug_tag(DEBUG_TAG, 6, _T("CorrelateEvent: finished, rootId=") UINT64_FMT, pEvent->getRootId());
}
