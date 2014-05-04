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
** File: stp.cpp
**
**/

#include "nxcore.h"

/**
 * STP port table walker's callback
 */
static UINT32 STPPortListHandler(UINT32 snmpVersion, SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
	int state = var->getValueAsInt();
	if ((state != 2) && (state != 5))
		return SNMP_ERR_SUCCESS;  // port state not "blocked" or "forwarding"

	Node *node = (Node *)((LinkLayerNeighbors *)arg)->getData();
	UINT32 oid[64];
   memcpy(oid, var->getName()->getValue(), var->getName()->getLength() * sizeof(UINT32));

   // Get designated bridge and designated port for this port
   SNMP_PDU *request = new SNMP_PDU(SNMP_GET_REQUEST, SnmpNewRequestId(), snmpVersion);

   oid[10] = 8;   // dot1dStpPortDesignatedBridge
   request->bindVariable(new SNMP_Variable(oid, var->getName()->getLength()));

   oid[10] = 9;   // dot1dStpPortDesignatedPort
   request->bindVariable(new SNMP_Variable(oid, var->getName()->getLength()));

	SNMP_PDU *response = NULL;
   UINT32 rcc = transport->doRequest(request, &response, g_dwSNMPTimeout, 3);
	delete request;
	if (rcc == SNMP_ERR_SUCCESS)
   {
      if (response->getNumVariables() >= 2)
      {
         BYTE designatedBridge[16], designatedPort[4];
         response->getVariable(0)->getRawValue(designatedBridge, 16);
         response->getVariable(1)->getRawValue(designatedPort, 4);

         // Usually bridge ID is a MAC address of one of bridge interfaces
         // If this will not work, try to find bridge by bridge ID value
         Node *bridge = FindNodeByMAC(&designatedBridge[2]);
         if (bridge == NULL)
            bridge = FindNodeByBridgeId(&designatedBridge[2]);
         if ((bridge != NULL) && (bridge != node))
         {
            DbgPrintf(6, _T("STP: found designated bridge %s [%d] for node %s [%d] port %d"), 
               bridge->Name(), bridge->Id(), node->Name(), node->Id(), oid[11]);
            Interface *ifLocal = node->findBridgePort(oid[11]);
            if (ifLocal != NULL)
            {
               DbgPrintf(6, _T("STP: found local port %s [%d] for node %s [%d]"), 
                  ifLocal->Name(), ifLocal->Id(), node->Name(), node->Id());
               Interface *ifRemote = bridge->findBridgePort((UINT32)designatedPort[1]);
               if (ifRemote != NULL)
               {
                  DbgPrintf(6, _T("STP: found remote port %s [%d] on node %s [%d]"), 
                     ifRemote->Name(), ifRemote->Id(), bridge->Name(), bridge->Id());
                  
                  LL_NEIGHBOR_INFO info;
                  info.ifLocal = ifLocal->getIfIndex();
                  info.ifRemote = ifRemote->getIfIndex();
                  info.objectId = bridge->Id();
                  info.isPtToPt = true;
                  info.protocol = LL_PROTO_STP;
                  ((LinkLayerNeighbors *)arg)->addConnection(&info);
               }
               else
               {
                  DbgPrintf(6, _T("STP: bridge port number %d is invalid for node %s [%d]"), (UINT32)designatedPort[1], bridge->Name(), bridge->Id());
               }
            }
            else
            {
               DbgPrintf(6, _T("STP: bridge port number %d is invalid for node %s [%d]"), oid[11], node->Name(), node->Id());
            }
         }
      }
		delete response;
	}
	else
	{
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

	DbgPrintf(5, _T("STP: collecting topology information for node %s [%d]"), node->Name(), node->Id());
	nbs->setData(node);
	node->callSnmpEnumerate(_T(".1.3.6.1.2.1.17.2.15.1.3"), STPPortListHandler, nbs);
	DbgPrintf(5, _T("STP: finished collecting topology information for node %s [%d]"), node->Name(), node->Id());
}
