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
** File: cdp.cpp
**
**/

#include "nxcore.h"

/**
 * Topology table walker's callback for CDP topology table
 */
static UINT32 CDPTopoHandler(UINT32 snmpVersion, SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
	Node *node = (Node *)((LinkLayerNeighbors *)arg)->getData();
	SNMP_ObjectId *oid = var->getName();

	UINT32 remoteIp;
	var->getRawValue((BYTE *)&remoteIp, sizeof(UINT32));
	remoteIp = ntohl(remoteIp);
	
   TCHAR ipAddrText[16];
	DbgPrintf(6, _T("CDP(%s [%d]): remote IP address %s"), node->getName(), node->getId(), IpToStr(remoteIp, ipAddrText));
	Node *remoteNode = FindNodeByIP(node->getZoneId(), remoteIp);
	if (remoteNode == NULL)
	{
		DbgPrintf(6, _T("CDP(%s [%d]): node object for remote IP %s not found"), node->getName(), node->getId(), ipAddrText);
		return SNMP_ERR_SUCCESS;
	}
	DbgPrintf(6, _T("CDP(%s [%d]): remote node is %s [%d]"), node->getName(), node->getId(), remoteNode->getName(), remoteNode->getId());

	// Get additional info for current record
	UINT32 newOid[128];
	memcpy(newOid, oid->getValue(), oid->getLength() * sizeof(UINT32));
   SNMP_PDU *pRqPDU = new SNMP_PDU(SNMP_GET_REQUEST, SnmpNewRequestId(), snmpVersion);

	newOid[13] = 7;	// cdpCacheDevicePort
	pRqPDU->bindVariable(new SNMP_Variable(newOid, oid->getLength()));

   SNMP_PDU *pRespPDU = NULL;
   UINT32 rcc = transport->doRequest(pRqPDU, &pRespPDU, g_snmpTimeout, 3);
	delete pRqPDU;

	if (rcc == SNMP_ERR_SUCCESS)
   {
		if (pRespPDU->getNumVariables() >= 1)
		{
			TCHAR ifName[MAX_CONNECTOR_NAME] = _T("");
			pRespPDU->getVariable(0)->getValueAsString(ifName, MAX_CONNECTOR_NAME);
			DbgPrintf(6, _T("CDP(%s [%d]): remote port is \"%s\""), node->getName(), node->getId(), ifName);
			Interface *ifRemote = remoteNode->findInterface(ifName);
			if (ifRemote != NULL)
			{
				DbgPrintf(6, _T("CDP(%s [%d]): remote interface object is %s [%d]"), node->getName(), node->getId(), ifRemote->getName(), ifRemote->getId());
		
				LL_NEIGHBOR_INFO info;

				// Index for cdpCacheTable is cdpCacheIfIndex, cdpCacheDeviceIndex
				info.ifLocal = oid->getValue()[oid->getLength() - 2];
				info.ifRemote = ifRemote->getIfIndex();
				info.objectId = remoteNode->getId();
				info.isPtToPt = true;
				info.protocol = LL_PROTO_CDP;
            info.isCached = false;

				((LinkLayerNeighbors *)arg)->addConnection(&info);
			}
		}
      delete pRespPDU;
	}

	return rcc;
}

/**
 * Add CDP-discovered neighbors
 */
void AddCDPNeighbors(Node *node, LinkLayerNeighbors *nbs)
{
	if (!(node->getFlags() & NF_IS_CDP))
		return;

	DbgPrintf(5, _T("CDP: collecting topology information for node %s [%d]"), node->getName(), node->getId());
	nbs->setData(node);
	node->callSnmpEnumerate(_T(".1.3.6.1.4.1.9.9.23.1.2.1.1.4"), CDPTopoHandler, nbs);
	DbgPrintf(5, _T("CDP: finished collecting topology information for node %s [%d]"), node->getName(), node->getId());
}
