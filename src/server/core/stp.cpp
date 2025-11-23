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
** File: stp.cpp
**
**/

#include "nxcore.h"

#define DEBUG_TAG _T("topology.stp")

/**
 * Spanning tree state as text
 */
const wchar_t NXCORE_EXPORTABLE *STPPortStateToText(SpanningTreePortState state)
{
   static const wchar_t *stpStateText[] =
   {
      L"UNKNOWN",
      L"DISABLED",
      L"BLOCKING",
      L"LISTENING",
      L"LEARNING",
      L"FORWARDING",
      L"BROKEN"
   };
   int index = static_cast<int>(state);
   return (index <= static_cast<int>(SpanningTreePortState::BROKEN)) && (index >= 0) ? stpStateText[index] : stpStateText[0];
}

/**
 * STP port table walker's callback
 */
static uint32_t STPPortListHandler(SNMP_Variable *var, SNMP_Transport *transport, LinkLayerNeighbors *neighbors)
{
	int state = var->getValueAsInt();
	if ((state != 2) && (state != 5))
		return SNMP_ERR_SUCCESS;  // port state not "blocked" or "forwarding"

	Node *node = static_cast<Node*>(neighbors->getData());
	uint32_t oid[64];
   memcpy(oid, var->getName().value(), var->getName().length() * sizeof(UINT32));

   // Get designated bridge and designated port for this port
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), transport->getSnmpVersion());

   oid[10] = 8;   // dot1dStpPortDesignatedBridge
   request.bindVariable(new SNMP_Variable(oid, var->getName().length()));

   oid[10] = 9;   // dot1dStpPortDesignatedPort
   request.bindVariable(new SNMP_Variable(oid, var->getName().length()));

	SNMP_PDU *response = nullptr;
   uint32_t rcc = transport->doRequest(&request, &response);
	if (rcc == SNMP_ERR_SUCCESS)
   {
      if (response->getNumVariables() >= 2)
      {
         BYTE designatedBridge[16], designatedPort[4];
         size_t dblen = response->getVariable(0)->getRawValue(designatedBridge, 16);
         size_t dplen = response->getVariable(1)->getRawValue(designatedPort, 4);

         if ((dblen >= 8) && (dplen >= 2) &&
                  memcmp(designatedBridge, "\x00\x00\x00\x00\x00\x00\x00\x00", 8) &&
                  memcmp(designatedPort, "\x00\x00", 2))
         {
            // Usually bridge ID is a MAC address of one of bridge interfaces
            // If this will not work, try to find bridge by bridge ID value
            shared_ptr<Node> bridge = FindNodeByMAC(&designatedBridge[2]);
            if (bridge == nullptr)
               bridge = FindNodeByBridgeId(&designatedBridge[2]);
            if ((bridge != nullptr) && (bridge->getId() != node->getId()))
            {
               nxlog_debug_tag(DEBUG_TAG, 6, _T("Found designated STP bridge %s [%u] for node %s [%u] port %u"),
                     bridge->getName(), bridge->getId(), node->getName(), node->getId(), oid[11]);
               shared_ptr<Interface> ifLocal = node->findBridgePort(oid[11]);
               if (ifLocal != nullptr)
               {
                  nxlog_debug_tag(DEBUG_TAG, 6, _T("Found local port %s [%u] for node %s [%u]"), ifLocal->getName(), ifLocal->getId(), node->getName(), node->getId());
                  shared_ptr<Interface> ifRemote = bridge->findBridgePort((uint32_t)designatedPort[1]);
                  if (ifRemote != nullptr)
                  {
                     nxlog_debug_tag(DEBUG_TAG, 6, _T("Found remote port %s [%u] on node %s [%u]"), ifRemote->getName(), ifRemote->getId(), bridge->getName(), bridge->getId());

                     LL_NEIGHBOR_INFO info;
                     info.ifLocal = ifLocal->getIfIndex();
                     info.ifRemote = ifRemote->getIfIndex();
                     info.objectId = bridge->getId();
                     info.isPtToPt = true;
                     info.protocol = LL_PROTO_STP;
                     info.isCached = false;
                     neighbors->addConnection(info);
                  }
                  else
                  {
                     nxlog_debug_tag(DEBUG_TAG, 6, _T("Bridge port number %u is invalid for node %s [%u]"), (uint32_t)designatedPort[1], bridge->getName(), bridge->getId());
                  }
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG, 6, _T("Bridge port number %u is invalid for node %s [%u]"), oid[11], node->getName(), node->getId());
               }
            }
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 6, _T("Designated bridge or designated port is invalid for port %d on node %s [%u]"), oid[11], node->getName(), node->getId());
         }
      }
		delete response;
	}
	else
	{
      nxlog_debug_tag(DEBUG_TAG, 6, _T("SNMP failure reading additional STP data for node %s [%u]"), node->getName(), node->getId());
	}

	return SNMP_ERR_SUCCESS;
}

/**
 * Add STP-discovered neighbors
 */
void AddSTPNeighbors(Node *node, LinkLayerNeighbors *nbs)
{
	if (!(node->getCapabilities() & NC_IS_STP))
		return;

	nxlog_debug_tag(DEBUG_TAG, 5, _T("Collecting STP topology information for node %s [%u]"), node->getName(), node->getId());
	nbs->setData(node);
	node->callSnmpEnumerate(_T(".1.3.6.1.2.1.17.2.15.1.3"), STPPortListHandler, nbs);
	nxlog_debug_tag(DEBUG_TAG, 5, _T("Finished collecting STP topology information for node %s [%u]"), node->getName(), node->getId());
}
