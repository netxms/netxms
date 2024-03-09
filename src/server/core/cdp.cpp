/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
** File: cdp.cpp
**/

#include "nxcore.h"
#include <netxms_maps.h>

/**
 * Topology table walker's callback for CDP topology table
 */
static uint32_t CDPTopoHandler(SNMP_Variable *var, SNMP_Transport *transport, LinkLayerNeighbors *nbs)
{
	Node *node = static_cast<Node*>(nbs->getData());
	const SNMP_ObjectId& oid = var->getName();

	uint32_t remoteIp;
	var->getRawValue((BYTE *)&remoteIp, sizeof(UINT32));
	remoteIp = ntohl(remoteIp);
	
   TCHAR ipAddrText[16];
	nxlog_debug_tag(DEBUG_TAG_TOPO_CDP, 6, _T("CDP(%s [%u]): remote IP address %s"), node->getName(), node->getId(), IpToStr(remoteIp, ipAddrText));
	shared_ptr<Node> remoteNode = FindNodeByIP(node->getZoneUIN(), remoteIp);
	if (remoteNode == nullptr)
	{
		nxlog_debug_tag(DEBUG_TAG_TOPO_CDP, 6, _T("CDP(%s [%u]): node object for remote IP %s not found"), node->getName(), node->getId(), ipAddrText);
		return SNMP_ERR_SUCCESS;
	}
	nxlog_debug_tag(DEBUG_TAG_TOPO_CDP, 6, _T("CDP(%s [%u]): remote node is %s [%u]"), node->getName(), node->getId(), remoteNode->getName(), remoteNode->getId());

	// Get additional info for current record
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), transport->getSnmpVersion());

   SNMP_ObjectId newOid(oid);
	newOid.changeElement(13, 7);	// cdpCacheDevicePort
	request.bindVariable(new SNMP_Variable(newOid));

   SNMP_PDU *response = nullptr;
   uint32_t rcc = transport->doRequest(&request, &response);
	if (rcc == SNMP_ERR_SUCCESS)
   {
		if (response->getNumVariables() >= 1)
		{
			TCHAR ifName[MAX_CONNECTOR_NAME] = _T("");
			response->getVariable(0)->getValueAsString(ifName, MAX_CONNECTOR_NAME);
			nxlog_debug_tag(DEBUG_TAG_TOPO_CDP, 6, _T("CDP(%s [%u]): remote port is \"%s\""), node->getName(), node->getId(), ifName);
			shared_ptr<Interface> ifRemote = remoteNode->findInterfaceByName(ifName);
			if (ifRemote != nullptr)
			{
				nxlog_debug_tag(DEBUG_TAG_TOPO_CDP, 6, _T("CDP(%s [%u]): remote interface object is %s [%d]"), node->getName(), node->getId(), ifRemote->getName(), ifRemote->getId());
		
				// Index for cdpCacheTable is cdpCacheIfIndex, cdpCacheDeviceIndex
            LL_NEIGHBOR_INFO info;
				info.ifLocal = oid.value()[oid.length() - 2];
				info.ifRemote = ifRemote->getIfIndex();
				info.objectId = remoteNode->getId();
				info.isPtToPt = true;
				info.protocol = LL_PROTO_CDP;
            info.isCached = false;

				nbs->addConnection(info);
			}
		}
      delete response;
	}

	return rcc;
}

/**
 * Add CDP-discovered neighbors
 */
void AddCDPNeighbors(Node *node, LinkLayerNeighbors *nbs)
{
	if (!(node->getCapabilities() & NC_IS_CDP))
		return;

	nxlog_debug_tag(DEBUG_TAG_TOPO_CDP, 5, _T("CDP: collecting topology information for node %s [%u]"), node->getName(), node->getId());
	nbs->setData(node);
	node->callSnmpEnumerate(_T(".1.3.6.1.4.1.9.9.23.1.2.1.1.4"), CDPTopoHandler, nbs);
	nxlog_debug_tag(DEBUG_TAG_TOPO_CDP, 5, _T("CDP: finished collecting topology information for node %s [%u]"), node->getName(), node->getId());
}
