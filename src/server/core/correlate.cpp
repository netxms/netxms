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
** File: correlate.cpp
**
**/

#include "nxcore.h"

#define DEBUG_TAG _T("event.corr")

/**
 * Static data
 */
static QWORD m_networkLostEventId = 0;

/**
 * Correlate current event to another node down event
 */
static bool CheckNodeDown(Node *currNode, Event *pEvent, UINT32 nodeId, const TCHAR *nodeType)
{
	Node *node = (Node *)FindObjectById(nodeId, OBJECT_NODE);
	if ((node != NULL) && node->isDown())
	{
		pEvent->setRootId(node->getLastEventId(LAST_EVENT_NODE_DOWN));
		nxlog_debug_tag(DEBUG_TAG, 5, _T("C_SysNodeDown: %s %s [%d] for current node %s [%d] is down"),
		          nodeType, node->getName(), node->getId(), currNode->getName(), currNode->getId());
		return true;
	}
	return false;
}

/**
 * Correlate current event to agent unreachable event
 */
static bool CheckAgentDown(Node *currNode, Event *pEvent, UINT32 nodeId, const TCHAR *nodeType)
{
	Node *node = (Node *)FindObjectById(nodeId, OBJECT_NODE);
	if ((node != NULL) && node->isNativeAgent() && (node->getState() & NSF_AGENT_UNREACHABLE))
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
static void C_SysNodeDown(Node *pNode, Event *pEvent)
{
   if (!ConfigReadBoolean(_T("Events.Correlation.TopologyBased"), true))
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("C_SysNodeDown: topology based event correlation disabled"));
      return;
   }

	// Check for NetXMS server network connectivity
	if (g_flags & AF_NO_NETWORK_CONNECTIVITY)
	{
		pEvent->setRootId(m_networkLostEventId);
		return;
	}

	// Check proxy nodes
	if (IsZoningEnabled() && (pNode->getZoneUIN() != 0))
	{
		Zone *zone = FindZoneByUIN(pNode->getZoneUIN());
		if ((zone != NULL) && !zone->isProxyNode(pNode->getId()) && (pNode->getAssignedZoneProxyId() != 0))
		{
		   if (CheckAgentDown(pNode, pEvent, pNode->getAssignedZoneProxyId(), _T("zone proxy")))
		      return;
			pEvent->setRootId(0);
		}
	}

	// Check directly connected switch
	Interface *iface = pNode->findInterfaceByIP(pNode->getIpAddress());
	if (iface != NULL)
	{
	   if  (iface->getPeerNodeId() != 0)
	   {
         if (CheckNodeDown(pNode, pEvent, iface->getPeerNodeId(), _T("upstream switch")))
            return;

         Node *switchNode = (Node *)FindObjectById(iface->getPeerNodeId(), OBJECT_NODE);
         Interface *switchIface = (Interface *)FindObjectById(iface->getPeerInterfaceId(), OBJECT_INTERFACE);
         if ((switchNode != NULL) && (switchIface != NULL) &&
             ((switchIface->getAdminState() == IF_ADMIN_STATE_DOWN) || (switchIface->getAdminState() == IF_ADMIN_STATE_TESTING) ||
              (switchIface->getOperState() == IF_OPER_STATE_DOWN) || (switchIface->getOperState() == IF_OPER_STATE_TESTING)))
         {
            nxlog_debug(5, _T("C_SysNodeDown: upstream interface %s [%d] on switch %s [%d] for current node %s [%d] is down"),
                        switchIface->getName(), switchIface->getId(), switchNode->getName(), switchNode->getId(), pNode->getName(), pNode->getId());
            pEvent->setRootId(switchIface->getLastDownEventId());
            return;
         }
	   }
	   else
	   {
         BYTE localMacAddr[MAC_ADDR_LENGTH];
         memcpy(localMacAddr, iface->getMacAddr(), MAC_ADDR_LENGTH);
         int type = 0;
         NetObj *cp = FindInterfaceConnectionPoint(localMacAddr, &type);
         if (cp != NULL)
         {
            if (cp->getObjectClass() == OBJECT_INTERFACE)
            {
               Interface *iface = (Interface *)cp;
               if ((iface->getAdminState() == IF_ADMIN_STATE_DOWN) || (iface->getAdminState() == IF_ADMIN_STATE_TESTING) ||
                    (iface->getOperState() == IF_OPER_STATE_DOWN) || (iface->getOperState() == IF_OPER_STATE_TESTING))
               {
                  nxlog_debug(5, _T("C_SysNodeDown: upstream interface %s [%d] on switch %s [%d] for current node %s [%d] is down"),
                              iface->getName(), iface->getId(), iface->getParentNode()->getName(), iface->getParentNode()->getId(),
                              pNode->getName(), pNode->getId());
                  return;
               }
            }
            else if (cp->getObjectClass() == OBJECT_ACCESSPOINT)
            {
               AccessPoint *ap = (AccessPoint *)cp;
               if (ap->getStatus() == STATUS_CRITICAL)   // FIXME: how to correctly determine if AP is down?
               {
                  nxlog_debug(5, _T("Node::checkNetworkPath(%s [%d]): wireless access point %s [%d] for current node %s [%d] is down"),
                              ap->getName(), ap->getId(), pNode->getName(), pNode->getId());
                  return;
               }
            }
         }
	   }
	}

   // Trace route from management station or proxy to failed node and
   // check for failed intermediate nodes or interfaces
	UINT32 sourceNodeId;
	if (!(pNode->getFlags() & NF_DISABLE_ICMP))
	   sourceNodeId = pNode->getIcmpProxy();
   if ((sourceNodeId == 0) && !(pNode->getFlags() & NF_DISABLE_NXCP))
      sourceNodeId = pNode->getAgentProxy();
   if ((sourceNodeId == 0) && !(pNode->getFlags() & NF_DISABLE_SNMP))
      sourceNodeId = pNode->getSNMPProxy();
   if (sourceNodeId == 0)
      sourceNodeId = pNode->getAssignedZoneProxyId();
   Node *sourceNode = static_cast<Node*>(FindObjectById((sourceNodeId != 0) ? sourceNodeId : g_dwMgmtNode, OBJECT_NODE));
   if (sourceNode == NULL)
	{
		nxlog_debug_tag(DEBUG_TAG, 5, _T("C_SysNodeDown: cannot find source node for network path trace"));
		return;
	}

   nxlog_debug_tag(DEBUG_TAG, 5, _T("C_SysNodeDown: tracing network path from node %s [%d] to node %s [%d]"),
            sourceNode->getName(), sourceNode->getId(), pNode->getName(), pNode->getId());
	NetworkPath *trace = TraceRoute(sourceNode, pNode);
	if (trace == NULL)
	{
		nxlog_debug_tag(DEBUG_TAG, 5, _T("C_SysNodeDown: trace to node %s [%d] not available"), pNode->getName(), pNode->getId());
		return;
	}

   for(int i = 0; i < trace->getHopCount(); i++)
   {
		HOP_INFO *hop = trace->getHopInfo(i);
      if ((hop->object == NULL) || (hop->object == pNode) || (hop->object->getObjectClass() != OBJECT_NODE))
			continue;

      if (static_cast<Node*>(hop->object)->isDown())
      {
         pEvent->setRootId(static_cast<Node*>(hop->object)->getLastEventId(LAST_EVENT_NODE_DOWN));
			nxlog_debug_tag(DEBUG_TAG, 5, _T("C_SysNodeDown: upstream node %s [%d] for current node %s [%d] is down"),
			          hop->object->getName(), hop->object->getId(), pNode->getName(), pNode->getId());
			break;
      }

      if (hop->isVpn)
      {
         // Next hop is behind VPN tunnel
         VPNConnector *vpnConn = (VPNConnector *)FindObjectById(hop->ifIndex, OBJECT_VPNCONNECTOR);
         if ((vpnConn != NULL) && (vpnConn->getStatus() == STATUS_CRITICAL))
         {
            /* TODO: set root id */
         }
      }
      else
      {
         iface = static_cast<Node*>(hop->object)->findInterfaceByIndex(hop->ifIndex);
         if ((iface != NULL) &&
             ((iface->getAdminState() == IF_ADMIN_STATE_DOWN) || (iface->getAdminState() == IF_ADMIN_STATE_TESTING) ||
              (iface->getOperState() == IF_OPER_STATE_DOWN) || (iface->getOperState() == IF_OPER_STATE_TESTING)))
         {
				nxlog_debug(5, _T("C_SysNodeDown: upstream interface %s [%d] on node %s [%d] for current node %s [%d] is down"),
				            iface->getName(), iface->getId(), hop->object->getName(), hop->object->getId(), pNode->getName(), pNode->getId());
            pEvent->setRootId(iface->getLastDownEventId());
				break;
         }
      }
   }
   delete trace;
}

/**
 * Correlate event
 */
void CorrelateEvent(Event *pEvent)
{
   Node *node = (Node *)FindObjectById(pEvent->getSourceId(), OBJECT_NODE);
   if (node == NULL)
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
            Interface *pInterface = node->findInterfaceByIndex(pEvent->getParameterAsULong(4));
            if (pInterface != NULL)
            {
               pInterface->setLastDownEventId(pEvent->getId());
            }
         }
         break;
      case EVENT_INTERFACE_DOWN:
      case EVENT_INTERFACE_EXPECTED_DOWN:
         {
            Interface *pInterface = node->findInterfaceByIndex(pEvent->getParameterAsULong(4));
            if (pInterface != NULL)
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
      case EVENT_NODE_UNREACHABLE:
         node->setLastEventId(LAST_EVENT_NODE_DOWN, pEvent->getId());
         C_SysNodeDown(node, pEvent);
         break;
      case EVENT_NODE_UP:
         node->setLastEventId(LAST_EVENT_NODE_DOWN, 0);
         break;
      case EVENT_NETWORK_CONNECTION_LOST:
         m_networkLostEventId = pEvent->getId();
         break;
      case EVENT_ROUTING_LOOP_DETECTED:
         node->setRoutingLoopEvent(InetAddress::parse(pEvent->getNamedParameter(_T("destAddress"))), pEvent->getNamedParameterAsULong(_T("destNodeId")), pEvent->getId());
         break;
      default:
         break;
   }

	nxlog_debug_tag(DEBUG_TAG, 6, _T("CorrelateEvent: finished, rootId=") UINT64_FMT, pEvent->getRootId());
}
