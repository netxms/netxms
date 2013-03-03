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
** File: ndp.cpp
**
**/

#include "nxcore.h"

/**
 * Read remote slot and port from s5EnMsTopNmmEosTable
 */
static WORD ReadRemoteSlotAndPort(Node *node, SNMP_ObjectId *oid, DWORD snmpVersion, SNMP_Transport *transport)
{
	// Read data from appropriate entry in s5EnMsTopNmmEosTable
	DWORD eosEntryOID[64];
	memcpy(eosEntryOID, oid->getValue(), oid->getLength() * sizeof(DWORD));
	eosEntryOID[11] = 3;
	eosEntryOID[12] = 1;
	eosEntryOID[13] = 1;

   SNMP_PDU *pRqPDU = new SNMP_PDU(SNMP_GET_REQUEST, SnmpNewRequestId(), snmpVersion);
	pRqPDU->bindVariable(new SNMP_Variable(eosEntryOID, oid->getLength()));

	WORD result = 0;
	SNMP_PDU *pRespPDU = NULL;
   DWORD rcc = transport->doRequest(pRqPDU, &pRespPDU, g_dwSNMPTimeout, 3);
	delete pRqPDU;
	if ((rcc == SNMP_ERR_SUCCESS) && (pRespPDU->getNumVariables() > 0) && (pRespPDU->getVariable(0)->GetType() == ASN_OCTET_STRING))
   {
		BYTE eosEntry[128];
		pRespPDU->getVariable(0)->getRawValue(eosEntry, 128);
		result = (((WORD)eosEntry[7]) << 8) | (WORD)eosEntry[8];	// Slot in byte 7 and port in byte 8
	}
	delete pRespPDU;
	return result;
}

/**
 * Topology table walker's callback for NDP topology table
 */
static DWORD NDPTopoHandler(DWORD snmpVersion, SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
	Node *node = (Node *)((LinkLayerNeighbors *)arg)->getData();
	SNMP_ObjectId *oid = var->GetName();

	// Entries indexed by slot, port, IP address, and segment ID
	DWORD slot = oid->getValue()[14];
	DWORD port = oid->getValue()[15];

	// Table always contains record with slot=0 and port=0 which
	// represents local chassis. We should ignore this record.
	if ((slot == 0) && (port == 0))
		return SNMP_ERR_SUCCESS;

	DWORD remoteIp;
	var->getRawValue((BYTE *)&remoteIp, sizeof(DWORD));
	remoteIp = ntohl(remoteIp);
	TCHAR ipAddrText[32];
	DbgPrintf(6, _T("NDP(%s [%d]): found peer at %d.%d IP address %s"), node->Name(), node->Id(), slot, port, IpToStr(remoteIp, ipAddrText));
	Node *remoteNode = FindNodeByIP(node->getZoneId(), remoteIp);
	if (remoteNode == NULL)
	{
		DbgPrintf(6, _T("NDP(%s [%d]): node object for IP %s not found"), node->Name(), node->Id(), ipAddrText);
		return SNMP_ERR_SUCCESS;
	}

	Interface *ifLocal = node->findInterfaceBySlotAndPort(slot, port);
	DbgPrintf(6, _T("NDP(%s [%d]): remote node is %s [%d], local interface object \"%s\""), node->Name(), node->Id(),
	          remoteNode->Name(), remoteNode->Id(), (ifLocal != NULL) ? ifLocal->Name() : _T("(null)"));
	if (ifLocal != NULL)
	{
		WORD rport = ReadRemoteSlotAndPort(node, oid, snmpVersion, transport);
		DbgPrintf(6, _T("NDP(%s [%d]): remote slot/port is %04X"), node->Name(), node->Id(), rport);
		if (rport != 0)
		{
			Interface *ifRemote = remoteNode->findInterfaceBySlotAndPort(rport >> 8, rport & 0xFF);
			if (ifRemote != NULL)
			{
				LL_NEIGHBOR_INFO info;

				info.objectId = remoteNode->Id();
				info.ifRemote = ifRemote->getIfIndex();
				info.ifLocal = ifLocal->getIfIndex();
				info.isPtToPt = true;
				info.protocol = LL_PROTO_NDP;
				((LinkLayerNeighbors *)arg)->addConnection(&info);
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
	if (!(node->getFlags() & NF_IS_NDP))
		return;

	DbgPrintf(5, _T("NDP: collecting topology information for node %s [%d]"), node->Name(), node->Id());
	nbs->setData(node);
	node->callSnmpEnumerate(_T(".1.3.6.1.4.1.45.1.6.13.2.1.1.3"), NDPTopoHandler, nbs);
	DbgPrintf(5, _T("NDP: finished collecting topology information for node %s [%d]"), node->Name(), node->Id());
}
