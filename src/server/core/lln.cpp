/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 Victor Kirhenshtein
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
** File: lln.cpp
**
**/

#include "nxcore.h"


//
// Constructor
//

LinkLayerNeighbors::LinkLayerNeighbors()
{
	m_connections = NULL;
	m_count = 0;
	m_allocated = 0;
	m_data = NULL;
}


//
// Destructor
//

LinkLayerNeighbors::~LinkLayerNeighbors()
{
	safe_free(m_connections);
}


//
// Check if given information is duplicate
//

bool LinkLayerNeighbors::isDuplicate(LL_NEIGHBOR_INFO *info)
{
	for(int i = 0; i < m_count; i++)
		if ((m_connections[i].ifLocal == info->ifLocal) &&
		    (m_connections[i].ifRemote == info->ifRemote) &&
		    (m_connections[i].objectId == info->objectId))
		return true;
	return false;
}


//
// Add neighbor
//

void LinkLayerNeighbors::addConnection(LL_NEIGHBOR_INFO *info)
{
	if ((info->ifLocal == 0) || (info->ifRemote == 0))
		return;		// Do not add incomplete information

	if (isDuplicate(info))
		return;

	if (m_count == m_allocated)
	{
		m_allocated += 32;
		m_connections = (LL_NEIGHBOR_INFO *)realloc(m_connections, sizeof(LL_NEIGHBOR_INFO) * m_allocated);
	}
	memcpy(&m_connections[m_count], info, sizeof(LL_NEIGHBOR_INFO));
	m_count++;
}


//
// Topology table walker's callback for NDP topology table
//

static DWORD NDPTopoHandler(DWORD snmpVersion, SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
	DWORD remoteIp;
	var->getRawValue((BYTE *)&remoteIp, sizeof(DWORD));
	remoteIp = ntohl(remoteIp);
	Node *remoteNode = FindNodeByIP(remoteIp);
	if (remoteNode == NULL)
		return SNMP_ERR_SUCCESS;

	SNMP_ObjectId *oid = var->GetName();

	// Entries indexed by slot, port, IP address, and segment ID
	DWORD slot = oid->GetValue()[14];
	DWORD port = oid->GetValue()[15];

	// Table always contains record with slot=0 and port=0 which
	// represents local chassis. We should ignore this record.
	if ((slot != 0) && (port != 0))
	{
		Node *node = (Node *)((LinkLayerNeighbors *)arg)->getData();
		Interface *ifLocal = node->findInterfaceBySlotAndPort(slot, port);
		if (ifLocal != NULL)
		{
		}
	}

	return SNMP_ERR_SUCCESS;
}


//
// Topology table walker's callback for CDP topology table
//

static DWORD CDPTopoHandler(DWORD dwVersion, SNMP_Variable *pVar, SNMP_Transport *pTransport, void *arg)
{
   TCHAR szOid[MAX_OID_LEN * 4], szSuffix[MAX_OID_LEN * 4], ipAddr[16];

	DWORD remoteIp = ntohl(_t_inet_addr(pVar->GetValueAsIPAddr(ipAddr)));
	Node *remoteNode = FindNodeByIP(remoteIp);
	if (remoteNode == NULL)
		return SNMP_ERR_SUCCESS;

   SNMP_ObjectId *pOid = pVar->GetName();
   SNMPConvertOIDToText(pOid->Length() - 14, (DWORD *)&(pOid->GetValue())[14], szSuffix, MAX_OID_LEN * 4);

	// Get interfaces
   SNMP_PDU *pRqPDU = new SNMP_PDU(SNMP_GET_REQUEST, SnmpNewRequestId(), dwVersion);

	_tcscpy(szOid, _T(".1.3.6.1.4.1.9.9.23.1.2.1.1.7"));	// Remote port name
   _tcscat(szOid, szSuffix);
	pRqPDU->bindVariable(new SNMP_Variable(szOid));

	_tcscpy(szOid, _T(".1.3.6.1.4.1.9.9.23.1.2.1.1.1"));	// Local interface index
   _tcscat(szOid, szSuffix);
	pRqPDU->bindVariable(new SNMP_Variable(szOid));

   SNMP_PDU *pRespPDU;
   DWORD rcc = pTransport->doRequest(pRqPDU, &pRespPDU, g_dwSNMPTimeout, 3);
	delete pRqPDU;

	if (rcc == SNMP_ERR_SUCCESS)
   {
		if (pRespPDU->getNumVariables() >= 2)
		{
			TCHAR ifName[MAX_CONNECTOR_NAME] = _T("");
			pRespPDU->getVariable(0)->GetValueAsString(ifName, 64);
			Interface *ifRemote = remoteNode->findInterface(ifName);
			if (ifRemote != NULL)
			{
				LL_NEIGHBOR_INFO info;

				info.ifLocal = pRespPDU->getVariable(1)->GetValueAsUInt();
				info.ifRemote = ifRemote->getIfIndex();
				info.objectId = remoteNode->Id();
				info.isPtToPt = true;
				info.protocol = LL_PROTO_CDP;

				((LinkLayerNeighbors *)arg)->addConnection(&info);
			}
		}
      delete pRespPDU;
	}

	return rcc;
}


//
// Gather link layer connectivity information from node
//

LinkLayerNeighbors *BuildLinkLayerNeighborList(Node *node)
{
	LinkLayerNeighbors *nbs = new LinkLayerNeighbors();

	if (node->getFlags() & NF_IS_LLDP)
	{
		AddLLDPNeighbors(node, nbs);
	}
	if (node->getFlags() & NF_IS_CDP)
	{
		node->CallSnmpEnumerate(_T(".1.3.6.1.4.1.9.9.23.1.2.1.1.4"), CDPTopoHandler, nbs);
	}
	if (node->getFlags() & NF_IS_NDP)
	{
		nbs->setData(node);
		node->CallSnmpEnumerate(_T(".1.3.6.1.4.1.45.1.6.13.2.1.1.3"), NDPTopoHandler, nbs);
	}

	return nbs;
}
