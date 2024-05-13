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
** File: ndp.cpp
**
**/

#include "nxcore.h"

/**
 * Read remote slot and port from s5EnMsTopNmmEosTable
 */
static uint16_t ReadRemoteSlotAndPort(Node *node, const SNMP_ObjectId& oid, SNMP_Transport *transport)
{
	// Read data from appropriate entry in s5EnMsTopNmmEosTable
	UINT32 eosEntryOID[64];
	memcpy(eosEntryOID, oid.value(), oid.length() * sizeof(UINT32));
	eosEntryOID[11] = 3;
	eosEntryOID[12] = 1;
	eosEntryOID[13] = 1;

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), transport->getSnmpVersion());
	request.bindVariable(new SNMP_Variable(eosEntryOID, oid.length()));

	uint16_t result = 0;
	SNMP_PDU *response = nullptr;
   uint32_t rcc = transport->doRequest(&request, &response);
	if ((rcc == SNMP_ERR_SUCCESS) && (response->getNumVariables() > 0) && (response->getVariable(0)->getType() == ASN_OCTET_STRING))
   {
		BYTE eosEntry[128];
		response->getVariable(0)->getRawValue(eosEntry, 128);
		result = (((uint16_t)eosEntry[7]) << 8) | (uint16_t)eosEntry[8];	// Slot in byte 7 and port in byte 8
	}
	delete response;
	return result;
}

/**
 * Topology table walker's callback for NDP topology table
 */
static uint32_t NDPTopoHandler(SNMP_Variable *var, SNMP_Transport *transport, LinkLayerNeighbors *nbs)
{
	Node *node = static_cast<Node*>(nbs->getData());
	const SNMP_ObjectId& oid = var->getName();

	// Entries indexed by slot, port, IP address, and segment ID
	uint32_t slot = oid.getElement(14);
	uint32_t port = oid.getElement(15);

	// Table always contains record with slot=0 and port=0 which
	// represents local chassis. We should ignore this record.
	if ((slot == 0) && (port == 0))
		return SNMP_ERR_SUCCESS;

	uint32_t remoteIp;
	var->getRawValue((BYTE *)&remoteIp, sizeof(uint32_t));
	remoteIp = ntohl(remoteIp);
	TCHAR ipAddrText[32];
	nxlog_debug_tag(DEBUG_TAG_TOPO_NDP, 6, _T("NDP(%s [%d]): found peer at %d.%d IP address %s"), node->getName(), node->getId(), slot, port, IpToStr(remoteIp, ipAddrText));
	shared_ptr<Node> remoteNode = FindNodeByIP(node->getZoneUIN(), remoteIp);
	if (remoteNode == nullptr)
	{
		nxlog_debug_tag(DEBUG_TAG_TOPO_NDP, 6, _T("NDP(%s [%d]): node object for IP %s not found"), node->getName(), node->getId(), ipAddrText);
		return SNMP_ERR_SUCCESS;
	}

	shared_ptr<Interface> ifLocal = node->findInterfaceByLocation(InterfacePhysicalLocation(0, slot, 0, port));
	nxlog_debug_tag(DEBUG_TAG_TOPO_NDP, 6, _T("NDP(%s [%d]): remote node is %s [%d], local interface object \"%s\""), node->getName(), node->getId(),
	          remoteNode->getName(), remoteNode->getId(), (ifLocal != nullptr) ? ifLocal->getName() : _T("(null)"));
	if (ifLocal != nullptr)
	{
		uint16_t rport = ReadRemoteSlotAndPort(node, oid, transport);
		nxlog_debug_tag(DEBUG_TAG_TOPO_NDP, 6, _T("NDP(%s [%d]): remote slot/port is %04X"), node->getName(), node->getId(), rport);
		if (rport != 0)
		{
			shared_ptr<Interface> ifRemote = remoteNode->findInterfaceByLocation(InterfacePhysicalLocation(0, rport >> 8, 0, rport & 0xFF));
			if (ifRemote != nullptr)
			{
				LL_NEIGHBOR_INFO info;
				info.objectId = remoteNode->getId();
				info.ifRemote = ifRemote->getIfIndex();
				info.ifLocal = ifLocal->getIfIndex();
				info.isPtToPt = true;
				info.protocol = LL_PROTO_NDP;
            info.isCached = false;
				nbs->addConnection(info);
			}
		}
	}

	return SNMP_ERR_SUCCESS;
}

/**
 * Add NDP-discovered neighbors
 */
void AddNDPNeighbors(Node *node, LinkLayerNeighbors *nbs)
{
	if (!(node->getCapabilities() & NC_IS_NDP))
		return;

	nxlog_debug_tag(DEBUG_TAG_TOPO_NDP, 5, _T("NDP: collecting topology information for node %s [%d]"), node->getName(), node->getId());
	nbs->setData(node);
	node->callSnmpEnumerate(_T(".1.3.6.1.4.1.45.1.6.13.2.1.1.3"), NDPTopoHandler, nbs);
	nxlog_debug_tag(DEBUG_TAG_TOPO_NDP, 5, _T("NDP: finished collecting topology information for node %s [%d]"), node->getName(), node->getId());
}
