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
** File: correlate.cpp
**
**/

#include "nxcore.h"

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
		DbgPrintf(5, _T("C_SysNodeDown: %s %s [%d] for current node %s [%d] is down"),
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
	if ((node != NULL) && node->isNativeAgent() && (node->getRuntimeFlags() & NDF_AGENT_UNREACHABLE))
	{
		pEvent->setRootId(node->getLastEventId(LAST_EVENT_AGENT_DOWN));
		DbgPrintf(5, _T("C_SysNodeDown: agent on %s %s [%d] for current node %s [%d] is down"),
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
	// Check for NetXMS server netwok connectivity
	if (g_flags & AF_NO_NETWORK_CONNECTIVITY)
	{
		pEvent->setRootId(m_networkLostEventId);
		return;
	}

	// Check proxy nodes
	if (IsZoningEnabled() && (pNode->getZoneId() != 0))
	{
		Zone *zone = (Zone *)g_idxZoneByGUID.get(pNode->getZoneId());
		if ((zone != NULL) && ((zone->getAgentProxy() != 0) || (zone->getSnmpProxy() != 0) || (zone->getIcmpProxy() != 0)))
		{
			bool allProxyDown = true;
			if (zone->getAgentProxy() != 0)
				allProxyDown = CheckAgentDown(pNode, pEvent, zone->getAgentProxy(), _T("agent proxy"));
			if (allProxyDown && (zone->getSnmpProxy() != 0) && (zone->getSnmpProxy() != zone->getAgentProxy()))
				allProxyDown = CheckAgentDown(pNode, pEvent, zone->getSnmpProxy(), _T("SNMP proxy"));
			if (allProxyDown && (zone->getIcmpProxy() != 0) && (zone->getIcmpProxy() != zone->getAgentProxy()) && (zone->getIcmpProxy() != zone->getSnmpProxy()))
				allProxyDown = CheckAgentDown(pNode, pEvent, zone->getSnmpProxy(), _T("ICMP proxy"));
			if (allProxyDown)
				return;
			pEvent->setRootId(0);
		}
	}

	// Check directly connected switch
	Interface *iface = pNode->findInterfaceByIP(pNode->getIpAddress());
	if ((iface != NULL) && (iface->getPeerNodeId() != 0))
	{
		if (CheckNodeDown(pNode, pEvent, iface->getPeerNodeId(), _T("upstream switch")))
			return;
	}

   // Trace route from management station to failed node and
   // check for failed intermediate nodes or interfaces
   Node *pMgmtNode = (Node *)FindObjectById(g_dwMgmtNode);
   if (pMgmtNode == NULL)
	{
		DbgPrintf(5, _T("C_SysNodeDown: cannot find management node"));
		return;
	}

	NetworkPath *trace = TraceRoute(pMgmtNode, pNode);
	if (trace == NULL)
	{
		DbgPrintf(5, _T("C_SysNodeDown: trace to node %s [%d] not available"), pNode->getName(), pNode->getId());
		return;
	}

   for(int i = 0; i < trace->getHopCount(); i++)
   {
		HOP_INFO *hop = trace->getHopInfo(i);
      if ((hop->object == NULL) || (hop->object == pNode) || (hop->object->getObjectClass() != OBJECT_NODE))
			continue;

      if (((Node *)hop->object)->isDown())
      {
         pEvent->setRootId(((Node *)hop->object)->getLastEventId(LAST_EVENT_NODE_DOWN));
			DbgPrintf(5, _T("C_SysNodeDown: upstream node %s [%d] for current node %s [%d] is down"),
			          hop->object->getName(), hop->object->getId(), pNode->getName(), pNode->getId());
			break;
      }

      if (hop->isVpn)
      {
         // Next hop is behind VPN tunnel
         VPNConnector *vpnConn = (VPNConnector *)FindObjectById(hop->ifIndex, OBJECT_VPNCONNECTOR);
         if ((vpnConn != NULL) &&
             (vpnConn->Status() == STATUS_CRITICAL))
         {
            /* TODO: set root id */
         }
      }
      else
      {
         Interface *pInterface = ((Node *)hop->object)->findInterfaceByIndex(hop->ifIndex);
         if ((pInterface != NULL) && ((pInterface->Status() == STATUS_CRITICAL) || (pInterface->Status() == STATUS_DISABLED)))
         {
				DbgPrintf(5, _T("C_SysNodeDown: upstream interface %s [%d] on node %s [%d] for current node %s [%d] is down"),
				          pInterface->getName(), pInterface->getId(), hop->object->getName(), hop->object->getId(), pNode->getName(), pNode->getId());
            pEvent->setRootId(pInterface->getLastDownEventId());
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

	DbgPrintf(6, _T("CorrelateEvent: event %s id ") UINT64_FMT _T(" source %s [%d]"),
	          pEvent->getName(), pEvent->getId(), node->getName(), node->getId());

   if (node->isInMaintenanceMode())
   {
      pEvent->setRootId(node->getMaintenanceEventId());
   }
   else
   {
      switch(pEvent->getCode())
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
            if (node->getRuntimeFlags() & NDF_UNREACHABLE)
            {
               pEvent->setRootId(node->getLastEventId(LAST_EVENT_NODE_DOWN));
            }
            break;
         case EVENT_AGENT_FAIL:
            node->setLastEventId(LAST_EVENT_AGENT_DOWN, pEvent->getId());
            if (node->getRuntimeFlags() & NDF_UNREACHABLE)
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
         default:
            break;
      }
   }

	DbgPrintf(6, _T("CorrelateEvent: finished, rootId=") UINT64_FMT, pEvent->getRootId());
}
