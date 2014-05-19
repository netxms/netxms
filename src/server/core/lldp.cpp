/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
** File: lldp.cpp
**
**/

#include "nxcore.h"

/**
 * Handler for walking local port table
 */
static UINT32 PortLocalInfoHandler(UINT32 snmpVersion, SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
	LLDP_LOCAL_PORT_INFO *port = new LLDP_LOCAL_PORT_INFO;
   port->portNumber = var->getName()->getValue()[11];
	port->localIdLen = var->getRawValue(port->localId, 256);

	SNMP_ObjectId *oid = var->getName();
	UINT32 newOid[128];
	memcpy(newOid, oid->getValue(), oid->getLength() * sizeof(UINT32));
   SNMP_PDU *pRqPDU = new SNMP_PDU(SNMP_GET_REQUEST, SnmpNewRequestId(), snmpVersion);

	newOid[oid->getLength() - 2] = 4;	// lldpLocPortDescr
	pRqPDU->bindVariable(new SNMP_Variable(newOid, oid->getLength()));

	SNMP_PDU *pRespPDU = NULL;
   UINT32 rcc = transport->doRequest(pRqPDU, &pRespPDU, g_dwSNMPTimeout, 3);
	delete pRqPDU;
	if (rcc == SNMP_ERR_SUCCESS)
   {
		pRespPDU->getVariable(0)->getValueAsString(port->ifDescr, 192);
		delete pRespPDU;
	}
	else
	{
		_tcscpy(port->ifDescr, _T("###error###"));
	}

	((ObjectArray<LLDP_LOCAL_PORT_INFO> *)arg)->add(port);
	return SNMP_ERR_SUCCESS;
}

/**
 * Get information about LLDP local ports
 */
ObjectArray<LLDP_LOCAL_PORT_INFO> *GetLLDPLocalPortInfo(SNMP_Transport *snmp)
{
	ObjectArray<LLDP_LOCAL_PORT_INFO> *ports = new ObjectArray<LLDP_LOCAL_PORT_INFO>(64, 64, true);
	if (SnmpWalk(snmp->getSnmpVersion(), snmp, _T(".1.0.8802.1.1.2.1.3.7.1.3"), PortLocalInfoHandler, ports, FALSE) != SNMP_ERR_SUCCESS)
	{
		delete ports;
		return NULL;
	}
	return ports;
}

/**
 * Find remote interface
 *
 * @param node remote node
 * @param idType port ID type (value of lldpRemPortIdSubtype)
 * @param id port ID
 * @param idLen port ID length in bytes
 * @param nbs link layer neighbors list which is being built
 */
static Interface *FindRemoteInterface(Node *node, UINT32 idType, BYTE *id, size_t idLen, LinkLayerNeighbors *nbs)
{
   LLDP_LOCAL_PORT_INFO port;
	TCHAR ifName[130];
	Interface *ifc;

	switch(idType)
	{
		case 3:	// MAC address
			return node->findInterfaceByMAC(id);
		case 4:	// Network address
			if (id[0] == 1)	// IPv4
			{
				UINT32 ipAddr;
				memcpy(&ipAddr, &id[1], sizeof(UINT32));
				return node->findInterfaceByIP(ntohl(ipAddr));
			}
			return NULL;
		case 5:	// Interface name
#ifdef UNICODE
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (char *)id, (int)idLen, ifName, 128);
			ifName[min(idLen, 127)] = 0;
#else
			{
				int len = min(idLen, 127);
				memcpy(ifName, id, len);
				ifName[len] = 0;
			}
#endif
			ifc = node->findInterface(ifName);	/* TODO: find by cached ifName value */
			if ((ifc == NULL) && !_tcsncmp(node->getObjectId(), _T(".1.3.6.1.4.1.1916.2"), 19))
			{
				// Hack for Extreme Networks switches
				// Must be moved into driver
				memmove(&ifName[2], ifName, (_tcslen(ifName) + 1) * sizeof(TCHAR));
				ifName[0] = _T('1');
				ifName[1] = _T(':');
				ifc = node->findInterface(ifName);
			}
			return ifc;
		case 7:	// local identifier
			if (node->getLldpLocalPortInfo(id, idLen, &port))
			{
            if (node->isBridge())
               ifc = node->findBridgePort(port.portNumber);
            else
               ifc = node->findInterface(port.portNumber, INADDR_ANY);
            if (ifc == NULL)  // unable to find interface by bridge port number or interface index, try description
               ifc = node->findInterface(port.ifDescr);	/* TODO: find by cached ifName value */
			}
			else
			{
				ifc = NULL;
			}
			return ifc;
		default:
			return NULL;
	}
}

/**
 * Topology table walker's callback for LLDP topology table
 */
static UINT32 LLDPTopoHandler(UINT32 snmpVersion, SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
	LinkLayerNeighbors *nbs = (LinkLayerNeighbors *)arg;
	Node *node = (Node *)nbs->getData();
	SNMP_ObjectId *oid = var->getName();

	// Get additional info for current record
	UINT32 newOid[128];
	memcpy(newOid, oid->getValue(), oid->getLength() * sizeof(UINT32));
   SNMP_PDU *pRqPDU = new SNMP_PDU(SNMP_GET_REQUEST, SnmpNewRequestId(), snmpVersion);

	newOid[oid->getLength() - 4] = 4;	// lldpRemChassisIdSubtype
	pRqPDU->bindVariable(new SNMP_Variable(newOid, oid->getLength()));

	newOid[oid->getLength() - 4] = 7;	// lldpRemPortId
	pRqPDU->bindVariable(new SNMP_Variable(newOid, oid->getLength()));

	newOid[oid->getLength() - 4] = 6;	// lldpRemPortIdSubtype
	pRqPDU->bindVariable(new SNMP_Variable(newOid, oid->getLength()));

	newOid[oid->getLength() - 4] = 8;	// lldpRemPortDesc
	pRqPDU->bindVariable(new SNMP_Variable(newOid, oid->getLength()));

	SNMP_PDU *pRespPDU = NULL;
   UINT32 rcc = transport->doRequest(pRqPDU, &pRespPDU, g_dwSNMPTimeout, 3);
	delete pRqPDU;
	if (rcc == SNMP_ERR_SUCCESS)
   {
		// Build LLDP ID for remote system
		TCHAR remoteId[256];
      BuildLldpId(pRespPDU->getVariable(0)->getValueAsInt(), var->getValue(), (int)var->getValueLength(), remoteId, 256);
		Node *remoteNode = FindNodeByLLDPId(remoteId);
		if (remoteNode != NULL)
		{
			nbs->setData(2, transport);

			BYTE remoteIfId[1024];
			size_t remoteIfIdLen = pRespPDU->getVariable(1)->getRawValue(remoteIfId, 1024);
			Interface *ifRemote = FindRemoteInterface(remoteNode, pRespPDU->getVariable(2)->getValueAsUInt(), remoteIfId, remoteIfIdLen, nbs);
         if (ifRemote == NULL)
         {
            // Try to find remote interface by description
            TCHAR *ifDescr = pRespPDU->getVariable(3)->getValueAsString((TCHAR *)remoteIfId, 1024 / sizeof(TCHAR));
            if (ifDescr != NULL)
               ifRemote = remoteNode->findInterface(ifDescr);
         }

			LL_NEIGHBOR_INFO info;

			info.objectId = remoteNode->Id();
			info.ifRemote = (ifRemote != NULL) ? ifRemote->getIfIndex() : 0;
			info.isPtToPt = true;
			info.protocol = LL_PROTO_LLDP;

			// Index to lldpRemTable is lldpRemTimeMark, lldpRemLocalPortNum, lldpRemIndex
			UINT32 localPort = oid->getValue()[oid->getLength() - 2];

			// Determine interface index from local port number. It can be
			// either ifIndex or dot1dBasePort, as described in LLDP MIB:
			//         A port number has no mandatory relationship to an
			//         InterfaceIndex object (of the interfaces MIB, IETF RFC 2863).
			//         If the LLDP agent is a IEEE 802.1D, IEEE 802.1Q bridge, the
			//         LldpPortNumber will have the same value as the dot1dBasePort
			//         object (defined in IETF RFC 1493) associated corresponding
			//         bridge port.  If the system hosting LLDP agent is not an
			//         IEEE 802.1D or an IEEE 802.1Q bridge, the LldpPortNumber
			//         will have the same value as the corresponding interface's
			//         InterfaceIndex object.
			if (node->isBridge())
			{
				Interface *localIf = node->findBridgePort(localPort);
				if (localIf != NULL)
					info.ifLocal = localIf->getIfIndex();
			}
			else
			{
				info.ifLocal = localPort;
			}

			nbs->addConnection(&info);
		}

		delete pRespPDU;
	}
	return SNMP_ERR_SUCCESS;
}

/**
 * Add LLDP-discovered neighbors
 */
void AddLLDPNeighbors(Node *node, LinkLayerNeighbors *nbs)
{
	if (!(node->getFlags() & NF_IS_LLDP))
		return;

	DbgPrintf(5, _T("LLDP: collecting topology information for node %s [%d]"), node->Name(), node->Id());
	nbs->setData(0, node);
	nbs->setData(1, NULL);	// local port info cache
	node->callSnmpEnumerate(_T(".1.0.8802.1.1.2.1.4.1.1.5"), LLDPTopoHandler, nbs);
	DbgPrintf(5, _T("LLDP: finished collecting topology information for node %s [%d]"), node->Name(), node->Id());
}

/**
 * Parse MAC address. Could be without separators or with any separator char.
 */
static bool ParseMACAddress(const char *text, int length, BYTE *mac, int *macLength)
{
   bool withSeparator = false;
   char separator = 0;
   int p = 0;
   bool hi = true;
   for(int i = 0; (i < length) && (p < 64); i++)
   {
      char c = toupper(text[i]);
      if ((i % 3 == 2) && withSeparator)
      {
         if (c != separator)
            return false;
         continue;
      }
      if (!isdigit(c) && ((c < 'A') || (c > 'F')))
      {
         if (i == 2)
         {
            withSeparator = true;
            separator = c;
            continue;
         }
         return false;
      }
      if (hi)
      {
         mac[p] = (isdigit(c) ? (c - '0') : (c - 'A' + 10)) << 4;
         hi = false;
      }
      else
      {
         mac[p] |= (isdigit(c) ? (c - '0') : (c - 'A' + 10));
         p++;
         hi = true;
      }
   }
   *macLength = p;
   return true;
}

/**
 * Build LLDP ID for node
 */
void BuildLldpId(int type, const BYTE *data, int length, TCHAR *id, int idLen)
{
	_sntprintf(id, idLen, _T("%d@"), type);
   if (type == 4)
   {
      // Some D-Link switches returns MAC address for ID type 4 as formatted text instead of raw bytes
      BYTE macAddr[64];
      int macLength;
      if ((length >= MAC_ADDR_LENGTH * 2) && ParseMACAddress((const char *)data, length, macAddr, &macLength))
      {
   	   BinToStr(macAddr, macLength, &id[_tcslen(id)]);
      }
      else
      {
   	   BinToStr(data, length, &id[_tcslen(id)]);
      }
   }
   else
   {
	   BinToStr(data, length, &id[_tcslen(id)]);
   }
}
