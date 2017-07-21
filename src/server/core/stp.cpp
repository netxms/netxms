/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2016 Victor Kirhenshtein
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

/**
 * STP port table walker's callback
 */
static UINT32 STPPortListHandler(SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
	int state = var->getValueAsInt();
	if ((state != 2) && (state != 5))
		return SNMP_ERR_SUCCESS;  // port state not "blocked" or "forwarding"

	Node *node = (Node *)((LinkLayerNeighbors *)arg)->getData();
	UINT32 oid[64];
   memcpy(oid, var->getName().value(), var->getName().length() * sizeof(UINT32));

   // Get designated bridge and designated port for this port
   SNMP_PDU *request = new SNMP_PDU(SNMP_GET_REQUEST, SnmpNewRequestId(), transport->getSnmpVersion());

   oid[10] = 8;   // dot1dStpPortDesignatedBridge
   request->bindVariable(new SNMP_Variable(oid, var->getName().length()));

   oid[10] = 9;   // dot1dStpPortDesignatedPort
   request->bindVariable(new SNMP_Variable(oid, var->getName().length()));

	SNMP_PDU *response = NULL;
   UINT32 rcc = transport->doRequest(request, &response, SnmpGetDefaultTimeout(), 3);
	delete request;
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
            Node *bridge = FindNodeByMAC(&designatedBridge[2]);
            if (bridge == NULL)
               bridge = FindNodeByBridgeId(&designatedBridge[2]);
            if ((bridge != NULL) && (bridge != node))
            {
               nxlog_debug(6, _T("STP: found designated bridge %s [%d] for node %s [%d] port %d"),
                  bridge->getName(), bridge->getId(), node->getName(), node->getId(), oid[11]);
               Interface *ifLocal = node->findBridgePort(oid[11]);
               if (ifLocal != NULL)
               {
                  nxlog_debug(6, _T("STP: found local port %s [%d] for node %s [%d]"),
                     ifLocal->getName(), ifLocal->getId(), node->getName(), node->getId());
                  Interface *ifRemote = bridge->findBridgePort((UINT32)designatedPort[1]);
                  if (ifRemote != NULL)
                  {
                     nxlog_debug(6, _T("STP: found remote port %s [%d] on node %s [%d]"),
                        ifRemote->getName(), ifRemote->getId(), bridge->getName(), bridge->getId());

                     LL_NEIGHBOR_INFO info;
                     info.ifLocal = ifLocal->getIfIndex();
                     info.ifRemote = ifRemote->getIfIndex();
                     info.objectId = bridge->getId();
                     info.isPtToPt = true;
                     info.protocol = LL_PROTO_STP;
                     info.isCached = false;
                     ((LinkLayerNeighbors *)arg)->addConnection(&info);
                  }
                  else
                  {
                     nxlog_debug(6, _T("STP: bridge port number %d is invalid for node %s [%d]"), (UINT32)designatedPort[1], bridge->getName(), bridge->getId());
                  }
               }
               else
               {
                  nxlog_debug(6, _T("STP: bridge port number %d is invalid for node %s [%d]"), oid[11], node->getName(), node->getId());
               }
            }
         }
         else
         {
            nxlog_debug(6, _T("STP: designated bridge or designated port is invalid for port %d on node %s [%d]"), oid[11], node->getName(), node->getId());
         }
      }
		delete response;
	}
	else
	{
      nxlog_debug(6, _T("STP: SNMP failure reading additional data for node %s [%d]"), node->getName(), node->getId());
	}

	return SNMP_ERR_SUCCESS;
}

/**
 * Add STP-discovered neighbors
 */
void AddSTPNeighbors(Node *node, LinkLayerNeighbors *nbs)
{
	if (!(node->getFlags() & NF_IS_STP))
		return;

	DbgPrintf(5, _T("STP: collecting topology information for node %s [%d]"), node->getName(), node->getId());
	nbs->setData(node);
	node->callSnmpEnumerate(_T(".1.3.6.1.2.1.17.2.15.1.3"), STPPortListHandler, nbs);
	DbgPrintf(5, _T("STP: finished collecting topology information for node %s [%d]"), node->getName(), node->getId());
}
