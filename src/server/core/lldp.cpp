/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
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
static UINT32 PortLocalInfoHandler(SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
	LLDP_LOCAL_PORT_INFO *port = new LLDP_LOCAL_PORT_INFO;
   port->portNumber = var->getName().getElement(11);
	port->localIdLen = var->getRawValue(port->localId, 256);

	const SNMP_ObjectId& oid = var->getName();
	UINT32 newOid[128];
	memcpy(newOid, oid.value(), oid.length() * sizeof(UINT32));
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), transport->getSnmpVersion());

	newOid[oid.length() - 2] = 4;	// lldpLocPortDescr
	request.bindVariable(new SNMP_Variable(newOid, oid.length()));

   newOid[oid.length() - 2] = 2;   // lldpLocPortIdSubtype
   request.bindVariable(new SNMP_Variable(newOid, oid.length()));

	SNMP_PDU *pRespPDU = nullptr;
   UINT32 rcc = transport->doRequest(&request, &pRespPDU, SnmpGetDefaultTimeout(), 3);
	if (rcc == SNMP_ERR_SUCCESS)
   {
	   if (pRespPDU->getNumVariables() >= 2)
	   {
	      pRespPDU->getVariable(0)->getValueAsString(port->ifDescr, 192);
	      port->localIdSubtype = pRespPDU->getVariable(1)->getValueAsUInt();
	   }
		delete pRespPDU;
	}
	else
	{
		_tcscpy(port->ifDescr, _T("###error###"));
	   nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("PortLocalInfoHandler: failed SNMP request for port information (%s)"), SNMPGetErrorText(rcc));
	}

	((ObjectArray<LLDP_LOCAL_PORT_INFO> *)arg)->add(port);
	return SNMP_ERR_SUCCESS;
}

/**
 * Get information about LLDP local ports
 */
ObjectArray<LLDP_LOCAL_PORT_INFO> *GetLLDPLocalPortInfo(SNMP_Transport *snmp)
{
   nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("Reading LLDP local port information"));
	ObjectArray<LLDP_LOCAL_PORT_INFO> *ports = new ObjectArray<LLDP_LOCAL_PORT_INFO>(64, 64, Ownership::True);
	if (SnmpWalk(snmp, _T(".1.0.8802.1.1.2.1.3.7.1.3"), PortLocalInfoHandler, ports) != SNMP_ERR_SUCCESS)
	{
		delete ports;
		return nullptr;
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
 */
static shared_ptr<Interface> FindRemoteInterface(Node *node, SNMP_Transport *snmp, uint32_t idType, BYTE *id, size_t idLen)
{
	TCHAR buffer[256];
   nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("LLDPTopoHandler(%s [%d]): FindRemoteInterface: idType=%d id=%s (%d)"), node->getName(), node->getId(), idType, BinToStr(id, idLen, buffer), (int)idLen);

	// Try local LLDP port info first
   shared_ptr<Interface> ifc;
   LLDP_LOCAL_PORT_INFO port;
   if (node->getLldpLocalPortInfo(idType, id, idLen, &port))
   {
      nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("LLDPTopoHandler(%s [%d]): FindRemoteInterface: getLldpLocalPortInfo found port: %d \"%s\""), node->getName(), node->getId(), port.portNumber, port.ifDescr);
      if (node->isBridge())
         ifc = node->findBridgePort(port.portNumber);
      // Node: some Juniper devices may have bridge port numbers for certain interfaces but still use ifIndex in LLDP local port list
      if (ifc == nullptr)  // unable to find interface by bridge port number or device is not a bridge
         ifc = node->findInterfaceByIndex(port.portNumber);
      if (ifc == nullptr)  // unable to find interface by bridge port number or interface index, try description
         ifc = node->findInterfaceByName(port.ifDescr);  /* TODO: find by cached ifName value */
      if (ifc == nullptr)  // some devices may report interface alias as description in LLDP local port list
         ifc = node->findInterfaceByAlias(port.ifDescr);
      if (ifc != nullptr)
         return ifc;
   }

   nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("LLDPTopoHandler(%s [%d]): FindRemoteInterface: interface not found by getLldpLocalPortInfo"), node->getName(), node->getId());
   TCHAR ifName[130];
	switch(idType)
	{
		case 3:	// MAC address
			return node->findInterfaceByMAC(MacAddress(id, idLen));
		case 4:	// Network address
			if (id[0] == 1)	// IPv4
			{
				uint32_t ipAddr;
				memcpy(&ipAddr, &id[1], sizeof(uint32_t));
				return node->findInterfaceByIP(ntohl(ipAddr));
			}
			return shared_ptr<Interface>();
		case 5:	// Interface name
#ifdef UNICODE
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (char *)id, (int)idLen, ifName, 128);
			ifName[MIN(idLen, 127)] = 0;
#else
			{
				size_t len = MIN(idLen, 127);
				memcpy(ifName, id, len);
				ifName[len] = 0;
			}
#endif
			ifc = node->findInterfaceByName(ifName);	/* TODO: find by cached ifName value */
			if (ifc == nullptr)
			{
			   // Attempt to get alternative interface identification from driver
			   NetworkDeviceDriver *driver = node->getDriver();
			   if (driver != nullptr)
			   {
			      InterfaceId iid;
			      if (driver->lldpNameToInterfaceId(snmp, node, node->getDriverData(), ifName, &iid))
			      {
		            nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("LLDPTopoHandler(%s [%d]): FindRemoteInterface: alternative ID of type %d from driver for name \"%s\""),
		                     node->getName(), node->getId(), static_cast<int>(iid.type), ifName);
			         switch(iid.type)
			         {
			            case InterfaceIdType::INDEX:
		                  ifc = node->findInterfaceByIndex(iid.value.ifIndex);
		                  break;
                     case InterfaceIdType::NAME:
                        ifc = node->findInterfaceByName(iid.value.ifName);
                        break;
			         }
			      }
			   }
			}
			return ifc;
		case 7:	// local identifier
			return shared_ptr<Interface>();   // already tried to find port using local info
		default:
			return shared_ptr<Interface>();
	}
}

/**
 * Get variable from cache
 */
static SNMP_Variable *GetVariableFromCache(uint32_t *oid, size_t oidLen, StringObjectMap<SNMP_Variable> *cache)
{
   TCHAR oidText[MAX_OID_LEN * 6];
   SNMPConvertOIDToText(oidLen, oid, oidText, MAX_OID_LEN * 6);
   return cache->get(oidText);
}

/**
 * Process LLDP connection database entry
 */
static void ProcessLLDPConnectionEntry(Node *node, SNMP_Transport *snmp, StringObjectMap<SNMP_Variable> *connections, SNMP_Variable *var, LinkLayerNeighbors *nbs)
{
	const SNMP_ObjectId& oid = var->getName();

	// Get additional info for current record
	uint32_t newOid[128];
	memcpy(newOid, oid.value(), oid.length() * sizeof(uint32_t));

	newOid[oid.length() - 4] = 4;	// lldpRemChassisIdSubtype
	SNMP_Variable *lldpRemChassisIdSubtype = GetVariableFromCache(newOid, oid.length(), connections);

	newOid[oid.length() - 4] = 7;	// lldpRemPortId
   SNMP_Variable *lldpRemPortId = GetVariableFromCache(newOid, oid.length(), connections);

	newOid[oid.length() - 4] = 6;	// lldpRemPortIdSubtype
   SNMP_Variable *lldpRemPortIdSubtype = GetVariableFromCache(newOid, oid.length(), connections);

   newOid[oid.length() - 4] = 9;   // lldpRemSysName
   SNMP_Variable *lldpRemSysName = GetVariableFromCache(newOid, oid.length(), connections);

	if ((lldpRemChassisIdSubtype != nullptr) && (lldpRemPortId != nullptr) && (lldpRemPortIdSubtype != nullptr) && (lldpRemSysName != nullptr))
   {
		// Build LLDP ID for remote system
		TCHAR remoteId[256];
      BuildLldpId(lldpRemChassisIdSubtype->getValueAsInt(), var->getValue(), (int)var->getValueLength(), remoteId, 256);
		shared_ptr<Node> remoteNode = FindNodeByLLDPId(remoteId);

		// Try to find node by interface MAC address if chassis ID type is "MAC address"
      if ((remoteNode == nullptr) && (lldpRemChassisIdSubtype->getValueAsInt() == 4) && (var->getValueLength() >= 6))
      {
         auto macAddr = MacAddress(var->getValue(), var->getValueLength());
         nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("ProcessLLDPConnectionEntry(%s [%d]): remoteId=%s: FindNodeByLLDPId failed, fallback to interface MAC address (\"%s\")"),
                  node->getName(), node->getId(), remoteId, macAddr.toString().cstr());
         remoteNode = FindNodeByMAC(macAddr);
      }

		// Try to find node by sysName as fallback
		if (remoteNode == nullptr)
		{
		   TCHAR sysName[256] = _T("");
		   lldpRemSysName->getValueAsString(sysName, 256);
		   Trim(sysName);
         nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("ProcessLLDPConnectionEntry(%s [%d]): remoteId=%s: FindNodeByLLDPId and FindNodeByMAC failed, fallback to sysName (\"%s\")"),
                  node->getName(), node->getId(), remoteId, sysName);
         remoteNode = FindNodeBySysName(sysName);
		}

		if (remoteNode != nullptr)
		{
	      nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("ProcessLLDPConnectionEntry(%s [%d]): remoteId=%s remoteNode=%s [%d]"), node->getName(), node->getId(), remoteId, remoteNode->getName(), remoteNode->getId());

			BYTE remoteIfId[1024];
			size_t remoteIfIdLen = lldpRemPortId->getRawValue(remoteIfId, 1024);
			shared_ptr<Interface> ifRemote = FindRemoteInterface(remoteNode.get(), snmp, lldpRemPortIdSubtype->getValueAsUInt(), remoteIfId, remoteIfIdLen);
         if (ifRemote == nullptr)
         {
            // Try to find remote interface by description
            newOid[oid.length() - 4] = 8; // lldpRemPortDesc
            SNMP_Variable *lldpRemPortDesc = GetVariableFromCache(newOid, oid.length(), connections);
            if (lldpRemPortDesc != nullptr)
            {
               TCHAR *ifDescr = lldpRemPortDesc->getValueAsString((TCHAR *)remoteIfId, 1024 / sizeof(TCHAR));
               Trim(ifDescr);
               nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("ProcessLLDPConnectionEntry(%s [%d]): FindRemoteInterface failed, lookup by description (\"%s\")"), node->getName(), node->getId(), CHECK_NULL(ifDescr));
               if (ifDescr != nullptr)
                  ifRemote = remoteNode->findInterfaceByName(ifDescr);
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("ProcessLLDPConnectionEntry(%s [%d]): FindRemoteInterface failed but lldpRemPortDesc is not available"), node->getName(), node->getId());
            }
         }

			LL_NEIGHBOR_INFO info;
			info.objectId = remoteNode->getId();
			info.ifRemote = (ifRemote != nullptr) ? ifRemote->getIfIndex() : 0;
			info.isPtToPt = true;
			info.protocol = LL_PROTO_LLDP;
         info.isCached = false;

			// Index to lldpRemTable is lldpRemTimeMark, lldpRemLocalPortNum, lldpRemIndex
			uint32_t localPort = oid.getElement(oid.length() - 2);

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
				shared_ptr<Interface> localIf = node->findBridgePort(localPort);
				if (localIf != nullptr)
					info.ifLocal = localIf->getIfIndex();
            nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("ProcessLLDPConnectionEntry(%s [%d]): lookup bridge port: localPort=%d iface=%s"), node->getName(), node->getId(), localPort, (localIf != nullptr) ? localIf->getName() : _T("(null)"));
			}
			else
			{
				info.ifLocal = localPort;
			}

			nbs->addConnection(&info);
         nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("ProcessLLDPConnectionEntry(%s [%d]): added connection: objectId=%d ifRemote=%d ifLocal=%d"), node->getName(), node->getId(), info.objectId, info.ifRemote, info.ifLocal);
		}
		else
		{
         nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("ProcessLLDPConnectionEntry(%s [%d]): remoteId=%s: remote node not found"), node->getName(), node->getId(), remoteId);
		}
	}
	else
	{
      TCHAR remoteId[256];
      BinToStr(var->getValue(), var->getValueLength(), remoteId);
	   nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("ProcessLLDPConnectionEntry(%s [%d]): SNMP get failed for remote ID %s"), node->getName(), node->getId(), remoteId);
	}
}

/**
 * Topology table walker's callback for LLDP topology table
 */
static UINT32 LLDPTopoHandler(SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
   TCHAR buffer[1024];
   ((StringObjectMap<SNMP_Variable> *)arg)->set(var->getName().toString(buffer, 1024), new SNMP_Variable(var));
   return SNMP_ERR_SUCCESS;
}

/**
 * Add LLDP-discovered neighbors
 */
void AddLLDPNeighbors(Node *node, LinkLayerNeighbors *nbs)
{
	if (!(node->getCapabilities() & NC_IS_LLDP))
		return;

	nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("Collecting LLDP topology information for node %s [%d]"), node->getName(), node->getId());

	// Entire table should be cached before processing because some devices (D-Link for example)
	// do not allow GET requests for table elements
	SNMP_Transport *snmp = node->createSnmpTransport();
	if (snmp != nullptr)
	{
      StringObjectMap<SNMP_Variable> connections(Ownership::True);
      SnmpWalk(snmp, _T(".1.0.8802.1.1.2.1.4.1.1"), LLDPTopoHandler, &connections, false, false);
      if (connections.size() > 0)
      {
         nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("%d entries in LLDP connection database for node %s [%d]"), connections.size(), node->getName(), node->getId());
         StringList *oids = connections.keys();
         for(int i = 0; i < oids->size(); i++)
         {
            const TCHAR *oid = oids->get(i);
            if (_tcsncmp(oid, _T(".1.0.8802.1.1.2.1.4.1.1.5."), 26))
               continue;
            SNMP_Variable *var = connections.get(oid);
            ProcessLLDPConnectionEntry(node, snmp, &connections, var, nbs);
         }
         delete oids;
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("LLDP connection database empty for node %s [%d]"), node->getName(), node->getId());
      }
      delete snmp;
	}
	else
	{
      nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("Cannot create SNMP transport for node %s [%d]"), node->getName(), node->getId());
	}
	nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("Finished collecting LLDP topology information for node %s [%d]"), node->getName(), node->getId());
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
