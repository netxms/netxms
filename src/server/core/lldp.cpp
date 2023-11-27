/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Victor Kirhenshtein
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
 * Get LLDP chassis ID subtype value as text
 */
static const TCHAR *ChassisIdSubtypeAsText(uint32_t chassisIdSubtype)
{
   switch(chassisIdSubtype)
   {
      case 1:
         return _T("chassisComponent");
      case 2:
         return _T("interfaceAlias");
      case 3:
         return _T("portComponent");
      case 4:
         return _T("macAddress");
      case 5:
         return _T("networkAddress");
      case 6:
         return _T("interfaceName");
      case 7:
         return _T("local");
   }
   return _T("unknown");
}

/**
 * Get LLDP port ID subtype value as text
 */
static const TCHAR *PortIdSubtypeAsText(uint32_t portIdSubtype)
{
   switch(portIdSubtype)
   {
      case 1:
         return _T("interfaceAlias");
      case 2:
         return _T("portComponent");
      case 3:
         return _T("macAddress");
      case 4:
         return _T("networkAddress");
      case 5:
         return _T("interfaceName");
      case 6:
         return _T("agentCircuitId");
      case 7:
         return _T("local");
   }
   return _T("unknown");
}

/**
 * Handler for walking local port table
 */
static uint32_t PortLocalInfoHandler(SNMP_Variable *var, SNMP_Transport *transport, ObjectArray<LLDP_LOCAL_PORT_INFO> *ports)
{
	LLDP_LOCAL_PORT_INFO *port = new LLDP_LOCAL_PORT_INFO;
   port->portNumber = var->getName().getLastElement();
	port->localIdLen = var->getRawValue(port->localId, 256);

	const SNMP_ObjectId& oid = var->getName();
	uint32_t newOid[128];
	memcpy(newOid, oid.value(), oid.length() * sizeof(uint32_t));
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), transport->getSnmpVersion());

	newOid[oid.length() - 2] = 4;	// lldpLocPortDescr
	request.bindVariable(new SNMP_Variable(newOid, oid.length()));

   newOid[oid.length() - 2] = 2;   // lldpLocPortIdSubtype
   request.bindVariable(new SNMP_Variable(newOid, oid.length()));

	SNMP_PDU *responsePDU = nullptr;
   uint32_t rcc = transport->doRequest(&request, &responsePDU);
	if (rcc == SNMP_ERR_SUCCESS)
   {
	   if (responsePDU->getNumVariables() >= 2)
	   {
	      responsePDU->getVariable(0)->getValueAsString(port->ifDescr, 192);
	      port->localIdSubtype = responsePDU->getVariable(1)->getValueAsUInt();
	   }
		delete responsePDU;
	}
	else
	{
		_tcscpy(port->ifDescr, _T("###error###"));
	   nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("PortLocalInfoHandler: failed SNMP request for port information (%s)"), SnmpGetErrorText(rcc));
	}

	for(int i = 0; i < ports->size(); i++)
	{
	   LLDP_LOCAL_PORT_INFO *p = ports->get(i);
	   if (p->portNumber == port->portNumber)
	   {
	      // If port with same number already exist, ignore new entry unless information for existing entry is incomplete
	      if (!_tcscmp(p->ifDescr, _T("###error###")))
	         ports->remove(i);
	      else
	         delete_and_null(port);
	      break;
	   }
	}

	if (port != nullptr)
	   ports->add(port);

	return SNMP_ERR_SUCCESS;
}

/**
 * Get information about LLDP local ports
 */
ObjectArray<LLDP_LOCAL_PORT_INFO> *GetLLDPLocalPortInfo(const Node& node, SNMP_Transport *snmp)
{
   nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("Reading LLDP local port information from node %s [%u]"), node.getName(), node.getId());

   ObjectArray<LLDP_LOCAL_PORT_INFO> *ports = new ObjectArray<LLDP_LOCAL_PORT_INFO>(64, 64, Ownership::True);

   bool failure = false;
   if (SnmpWalk(snmp, LLDP_OID(_T("1.3.7.1.3"), false), PortLocalInfoHandler, ports) != SNMP_ERR_SUCCESS)
      failure = true;

	if (node.isLLDPV2MIBSupported())
	{
	   if (SnmpWalk(snmp, LLDP_OID(_T("1.3.7.1.3"), true), PortLocalInfoHandler, ports) == SNMP_ERR_SUCCESS)
	      failure = false;
	}

	if (failure)
	{
		delete ports;
		return nullptr;
	}

	if (nxlog_get_debug_level_tag(DEBUG_TAG_TOPO_LLDP) >= 6)
	{
	   nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 6, _T("GetLLDPLocalPortInfo(%s [%u]): %d ports"), node.getName(), node.getId(), ports->size());

	   TCHAR buffer[512];
	   for(int i = 0; i < ports->size(); i++)
	   {
	      LLDP_LOCAL_PORT_INFO *p = ports->get(i);
	      nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 6, _T("GetLLDPLocalPortInfo(%s [%u]): port=%u, idSubType=%s(%u), idLen=%d, id=%s, ifDescr=\"%s\""), node.getName(), node.getId(),
	               p->portNumber, PortIdSubtypeAsText(p->localIdSubtype), p->localIdSubtype, (int)p->localIdLen, BinToStr(p->localId, p->localIdLen, buffer), p->ifDescr);
	   }
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
static shared_ptr<Interface> FindRemoteInterface(Node *node, uint32_t idSubType, BYTE *id, size_t idLen)
{
	TCHAR buffer[256];
   nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("FindRemoteInterface(%s [%u]): idSubType=%s(%u) id=%s (%d)"), node->getName(), node->getId(), PortIdSubtypeAsText(idSubType), idSubType, BinToStr(id, idLen, buffer), (int)idLen);

	// Try local LLDP port info first
   shared_ptr<Interface> ifc;
   LLDP_LOCAL_PORT_INFO port;
   if (node->getLldpLocalPortInfo(idSubType, id, idLen, &port))
   {
      nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("FindRemoteInterface(%s [%u]): getLldpLocalPortInfo found port: %u \"%s\""), node->getName(), node->getId(), port.portNumber, port.ifDescr);
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

   nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("FindRemoteInterface(%s [%u]): interface not found by getLldpLocalPortInfo"), node->getName(), node->getId());
   TCHAR ifName[130];
	switch(idSubType)
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
			mbcp_to_wchar((char *)id, (int)idLen, ifName, 128, "ISO-8859-1");
			ifName[MIN(idLen, 127)] = 0;
#else
			{
				size_t len = MIN(idLen, 127);
				memcpy(ifName, id, len);
				ifName[len] = 0;
			}
#endif
		   nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("FindRemoteInterface(%s [%u]): ifName=\"%s\""), node->getName(), node->getId(), ifName);
			ifc = node->findInterfaceByName(ifName);	/* TODO: find by cached ifName value */
			if (ifc == nullptr)
			{
			   // Attempt to get alternative interface identification from driver
			   NetworkDeviceDriver *driver = node->getDriver();
			   if (driver != nullptr)
			   {
			      SNMP_Transport *snmp = node->createSnmpTransport();
			      InterfaceId iid;
			      if (driver->lldpNameToInterfaceId(snmp, node, node->getDriverData(), ifName, &iid))
			      {
		            nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("FindRemoteInterface(%s [%u]): alternative ID of type %d from driver for name \"%s\""),
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
			      delete snmp;
			   }
	         if (ifc == nullptr)
			   {
		         nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("FindRemoteInterface(%s [%u]): interface cannot be found by name"), node->getName(), node->getId());
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
static const SNMP_Variable *GetVariableFromCache(uint32_t *oid, size_t oidLen, const StringObjectMap<SNMP_Variable>& cache)
{
   TCHAR oidText[MAX_OID_LEN * 6];
   SnmpConvertOIDToText(oidLen, oid, oidText, MAX_OID_LEN * 6);
   return cache.get(oidText);
}

/**
 * Read LLDP remote device table
 */
static StringObjectMap<SNMP_Variable> *ReadLLDPRemoteTable(Node *node, bool lldpMibV2)
{
   // Entire table should be cached before processing because some devices (D-Link for example)
   // do not allow GET requests for table elements
   StringObjectMap<SNMP_Variable> *connections = nullptr;
   SNMP_Transport *snmp = node->createSnmpTransport();
   if (snmp != nullptr)
   {
      connections = new StringObjectMap<SNMP_Variable>(Ownership::True);
      SnmpWalk(snmp, LLDP_OID(_T("1.4.1.1"), lldpMibV2),
         [connections] (SNMP_Variable *v) -> uint32_t
         {
            TCHAR buffer[1024];
            connections->set(v->getName().toString(buffer, 1024), new SNMP_Variable(v));
            return SNMP_ERR_SUCCESS;
         });
      if (connections->size() > 0)
      {
         nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("%d entries in %s connection database for node %s [%u]"), connections->size(), LLDP_MIB_NAME(lldpMibV2), node->getName(), node->getId());
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("%s connection database empty for node %s [%u]"), LLDP_MIB_NAME(lldpMibV2), node->getName(), node->getId());
         delete_and_null(connections);
      }
      delete snmp;
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("Cannot create SNMP transport for node %s [%u]"), node->getName(), node->getId());
   }
   return connections;
}

/**
 * Find remote node
 */
static shared_ptr<Node> FindRemoteNode(Node *node, const SNMP_Variable *lldpRemChassisId, const SNMP_Variable *lldpRemChassisIdSubtype, const SNMP_Variable *lldpRemSysName)
{
   // Build LLDP ID for remote system
   uint32_t idSubType = lldpRemChassisIdSubtype->getValueAsUInt();
   String remoteId = BuildLldpId(idSubType, lldpRemChassisId->getValue(), lldpRemChassisId->getValueLength());
   shared_ptr<Node> remoteNode = FindNodeByLLDPId(remoteId);

   // Try to find node by interface MAC address if chassis ID type is "MAC address"
   if ((remoteNode == nullptr) && (lldpRemChassisIdSubtype->getValueAsInt() == 4) && (lldpRemChassisId->getValueLength() >= 6))
   {
      // Some devices (definitely seen on Mikrotik) report lldpRemChassisIdSubtype as 4 (MAC address)
      // but actually encode it not as 6 bytes value but in textual form (like 00:04:F2:E7:05:47).
      TCHAR buffer[64];
      MacAddress macAddr = (lldpRemChassisId->getValueLength() > 8) ? MacAddress::parse(lldpRemChassisId->getValueAsString(buffer, 64)) : lldpRemChassisId->getValueAsMACAddr();
      nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("FindRemoteNode(%s [%u]): remoteIdSubType=%s(%u) remoteId=%s: FindNodeByLLDPId failed, fallback to interface MAC address (\"%s\")"),
               node->getName(), node->getId(), ChassisIdSubtypeAsText(idSubType), idSubType, remoteId.cstr(), macAddr.toString().cstr());
      remoteNode = FindNodeByMAC(macAddr);
   }

   // Try to find node by sysName as fallback
   if (remoteNode == nullptr)
   {
      TCHAR sysName[256] = _T("");
      lldpRemSysName->getValueAsString(sysName, 256);
      Trim(sysName);
      nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("FindRemoteNode(%s [%u]): remoteIdSubType=%s(%u) remoteId=%s: FindNodeByLLDPId and FindNodeByMAC failed, fallback to sysName (\"%s\")"),
               node->getName(), node->getId(), ChassisIdSubtypeAsText(idSubType), idSubType, remoteId.cstr(), sysName);
      remoteNode = FindNodeBySysName(sysName);
   }

   return remoteNode;
}

/**
 * Find local node's interface in LLDP remote table on remote node
 */
static uint32_t FindLocalInterfaceOnRemoteNode(Node *thisNode, Node *remoteNode, bool lldpv2mib)
{
   StringObjectMap<SNMP_Variable> *connections = ReadLLDPRemoteTable(remoteNode, lldpv2mib);
   if (connections == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("FindLocalInterfaceOnRemoteNode(%s [%u]): cannot get %s remote table from node %s [%u]"),
               thisNode->getName(), thisNode->getId(), LLDP_MIB_NAME(lldpv2mib), remoteNode->getName(), remoteNode->getId());
      return 0;
   }

   const TCHAR *oidPrefix = lldpv2mib ? _T(".1.3.111.2.802.1.1.13.1.4.1.1.6.") : _T(".1.0.8802.1.1.2.1.4.1.1.5.");
   size_t oidPrefixLen = _tcslen(oidPrefix);

   uint32_t localIfIndex = 0;
   StringList *oids = connections->keys();
   for(int i = 0; i < oids->size(); i++)
   {
      const TCHAR *oid = oids->get(i);
      if (_tcsncmp(oid, oidPrefix, oidPrefixLen))
         continue;

      SNMP_Variable *lldpRemChassisId = connections->get(oid);

      // Get additional info for current record
      const SNMP_ObjectId& name = lldpRemChassisId->getName();
      uint32_t newOid[128];
      memcpy(newOid, name.value(), name.length() * sizeof(uint32_t));

      if (lldpv2mib)
         newOid[12] = 5;   // lldpV2RemChassisIdSubtype
      else
         newOid[10] = 4;   // lldpRemChassisIdSubtype
      const SNMP_Variable *lldpRemChassisIdSubtype = GetVariableFromCache(newOid, name.length(), *connections);

      if (lldpv2mib)
         newOid[12] = 8;   // lldpV2RemPortId
      else
         newOid[10] = 7;   // lldpRemPortId
      const SNMP_Variable *lldpRemPortId = GetVariableFromCache(newOid, name.length(), *connections);

      if (lldpv2mib)
         newOid[12] = 7;   // lldpV2RemPortIdSubtype
      else
         newOid[10] = 6;   // lldpRemPortIdSubtype
      const SNMP_Variable *lldpRemPortIdSubtype = GetVariableFromCache(newOid, name.length(), *connections);

      if (lldpv2mib)
         newOid[12] = 10;   // lldpV2RemSysName
      else
         newOid[10] = 9;   // lldpRemSysName
      const SNMP_Variable *lldpRemSysName = GetVariableFromCache(newOid, name.length(), *connections);

      if ((lldpRemChassisIdSubtype == nullptr) || (lldpRemPortId == nullptr) || (lldpRemPortIdSubtype == nullptr) || (lldpRemSysName == nullptr))
      {
         nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("FindLocalInterfaceOnRemoteNode(%s [%u]): some OIDs are missing for base OID=\"%s\":%s%s%s%s"),
                  thisNode->getName(), thisNode->getId(), oid, (lldpRemChassisIdSubtype == nullptr) ? _T(" lldpRemChassisIdSubtype") : _T(""),
                  (lldpRemPortId == nullptr) ? _T(" lldpRemPortId") : _T(""), (lldpRemPortIdSubtype == nullptr) ? _T(" lldpRemPortIdSubtype") : _T(""),
                  (lldpRemSysName == nullptr) ? _T(" lldpRemSysName") : _T(""));
         continue;
      }

      shared_ptr<Node> node = FindRemoteNode(remoteNode, lldpRemChassisId, lldpRemChassisIdSubtype, lldpRemSysName);
      if ((node == nullptr) || (node->getId() != thisNode->getId()))
         continue;

      nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("FindLocalInterfaceOnRemoteNode(%s [%u]): found matching record in %s remote table on node %s [%u]"),
               thisNode->getName(), thisNode->getId(), LLDP_MIB_NAME(lldpv2mib), remoteNode->getName(), remoteNode->getId());

      BYTE localIfId[1024];
      size_t localIfIdLen = lldpRemPortId->getRawValue(localIfId, 1024);
      shared_ptr<Interface> ifLocal = FindRemoteInterface(thisNode, lldpRemPortIdSubtype->getValueAsUInt(), localIfId, localIfIdLen);
      if (ifLocal == nullptr)
      {
         // Try to find remote interface by description
         if (lldpv2mib)
            newOid[12] = 9;   // lldpV2RemPortDesc
         else
            newOid[10] = 8; // lldpRemPortDesc
         const SNMP_Variable *lldpRemPortDesc = GetVariableFromCache(newOid, name.length(), *connections);
         if (lldpRemPortDesc != nullptr)
         {
            TCHAR *ifDescr = lldpRemPortDesc->getValueAsString((TCHAR *)localIfId, 1024 / sizeof(TCHAR));
            Trim(ifDescr);
            nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("FindLocalInterfaceOnRemoteNode(%s [%u]): FindRemoteInterface failed, lookup by description (\"%s\")"), node->getName(), node->getId(), CHECK_NULL(ifDescr));
            if (ifDescr != nullptr)
               ifLocal = thisNode->findInterfaceByName(ifDescr);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("FindLocalInterfaceOnRemoteNode(%s [%u]): FindRemoteInterface failed and lldpRemPortDesc is not available in %s on node %s [%u]"),
                  node->getName(), node->getId(), LLDP_MIB_NAME(lldpv2mib), remoteNode->getName(), remoteNode->getId());
         }
      }

      if (ifLocal != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("FindLocalInterfaceOnRemoteNode(%s [%u]): found interface %s in %s remote table on node %s [%u]"),
                  node->getName(), node->getId(), ifLocal->getName(), LLDP_MIB_NAME(lldpv2mib), remoteNode->getName(), remoteNode->getId());
         localIfIndex = ifLocal->getIfIndex();
         break;
      }
   }
   delete oids;
   delete connections;
   return localIfIndex;
}

/**
 * Process LLDP connection database entry
 */
static void ProcessLLDPConnectionEntry(Node *node, const StringObjectMap<SNMP_Variable>& connections, bool lldpMibVersion2, SNMP_Variable *lldpRemChassisId, LinkLayerNeighbors *nbs)
{
	const SNMP_ObjectId& oid = lldpRemChassisId->getName();

	// Get additional info for current record
	uint32_t newOid[128];
	memcpy(newOid, oid.value(), oid.length() * sizeof(uint32_t));

   if (lldpMibVersion2)
      newOid[12] = 5;   // lldpV2RemChassisIdSubtype
   else
      newOid[10] = 4;	// lldpRemChassisIdSubtype
	const SNMP_Variable *lldpRemChassisIdSubtype = GetVariableFromCache(newOid, oid.length(), connections);

   if (lldpMibVersion2)
      newOid[12] = 8;   // lldpV2RemPortId
   else
      newOid[10] = 7;	// lldpRemPortId
   const SNMP_Variable *lldpRemPortId = GetVariableFromCache(newOid, oid.length(), connections);

   if (lldpMibVersion2)
      newOid[12] = 7;   // lldpV2RemPortIdSubtype
   else
      newOid[10] = 6;	// lldpRemPortIdSubtype
	const SNMP_Variable *lldpRemPortIdSubtype = GetVariableFromCache(newOid, oid.length(), connections);

   if (lldpMibVersion2)
      newOid[12] = 10;   // lldpV2RemSysName
   else
      newOid[10] = 9;   // lldpRemSysName
   const SNMP_Variable *lldpRemSysName = GetVariableFromCache(newOid, oid.length(), connections);

	if ((lldpRemChassisIdSubtype != nullptr) && (lldpRemPortId != nullptr) && (lldpRemPortIdSubtype != nullptr) && (lldpRemSysName != nullptr))
   {
	   shared_ptr<Node> remoteNode = FindRemoteNode(node, lldpRemChassisId, lldpRemChassisIdSubtype, lldpRemSysName);
		if (remoteNode != nullptr)
		{
	      nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("ProcessLLDPConnectionEntry(%s [%u], %s): remoteNode=%s [%u]"), node->getName(), node->getId(), LLDP_MIB_NAME(lldpMibVersion2), remoteNode->getName(), remoteNode->getId());

			BYTE remoteIfId[1024];
			size_t remoteIfIdLen = lldpRemPortId->getRawValue(remoteIfId, 1024);
			shared_ptr<Interface> ifRemote = FindRemoteInterface(remoteNode.get(), lldpRemPortIdSubtype->getValueAsUInt(), remoteIfId, remoteIfIdLen);
         if (ifRemote == nullptr)
         {
            // Try to find remote interface by description
            if (lldpMibVersion2)
               newOid[13] = 9;   // lldpV2RemPortDesc
            else
               newOid[10] = 8; // lldpRemPortDesc
            const SNMP_Variable *lldpRemPortDesc = GetVariableFromCache(newOid, oid.length(), connections);
            if (lldpRemPortDesc != nullptr)
            {
               TCHAR *ifDescr = lldpRemPortDesc->getValueAsString((TCHAR *)remoteIfId, 1024 / sizeof(TCHAR));
               Trim(ifDescr);
               nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("ProcessLLDPConnectionEntry(%s [%u], %s): FindRemoteInterface failed, lookup by description (\"%s\")"),
                     node->getName(), node->getId(), LLDP_MIB_NAME(lldpMibVersion2), CHECK_NULL(ifDescr));
               if (ifDescr != nullptr)
                  ifRemote = remoteNode->findInterfaceByName(ifDescr);
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("ProcessLLDPConnectionEntry(%s [%u], %s): FindRemoteInterface failed and lldpRemPortDesc is not available"), node->getName(), node->getId(), LLDP_MIB_NAME(lldpMibVersion2));
            }
         }

			LL_NEIGHBOR_INFO info;
			info.objectId = remoteNode->getId();
			info.ifRemote = (ifRemote != nullptr) ? ifRemote->getIfIndex() : 0;
			info.isPtToPt = true;
			info.protocol = LL_PROTO_LLDP;
         info.isCached = false;

			// Index to lldpRemTable is lldpRemTimeMark, lldpRemLocalPortNum, lldpRemIndex
         // Normally lldpRemLocalPortNum should not be zero, but many (if not all)
         // Mikrotik RouterOS versions always report zero for any port. The only way to find
         // correct port number in that case is to try and find matching information on remote node.
         // Some devices are known to be returning invalid values for lldpRemLocalPortNum (some TP-Link models for example).
         // In that case do not attempt local port search by interface index or bridge port number.
			uint32_t localPort = oid.getElement(oid.length() - 2);
			bool doRemoteLookup = (localPort == 0) || !node->getDriver()->isValidLldpRemLocalPortNum(node, node->getDriverData());
			if (localPort != 0)
			{
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
            if (node->isBridge() && !node->getDriver()->isLldpRemTableUsingIfIndex(node, node->getDriverData()))
            {
               shared_ptr<Interface> localIf = node->findBridgePort(localPort);
               if (localIf != nullptr)
               {
                  info.ifLocal = localIf->getIfIndex();
                  nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("ProcessLLDPConnectionEntry(%s [%u], %s): lookup bridge port: localPort=%u iface=%s"),
                        node->getName(), node->getId(), LLDP_MIB_NAME(lldpMibVersion2), localPort, localIf->getName());
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("ProcessLLDPConnectionEntry(%s [%u], %s): cannot find interface with dot1dBasePort=%u"),
                        node->getName(), node->getId(), LLDP_MIB_NAME(lldpMibVersion2), localPort);
                  doRemoteLookup = true;
               }
            }
            else if (node->findInterfaceByIndex(localPort) != nullptr)
            {
               info.ifLocal = localPort;
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("ProcessLLDPConnectionEntry(%s [%u], %s): cannot find interface with ifIndex=%u"),
                     node->getName(), node->getId(), LLDP_MIB_NAME(lldpMibVersion2), localPort);
               doRemoteLookup = true;
            }
			}

			if (doRemoteLookup)
			{
            if ((remoteNode != nullptr) && (ifRemote != nullptr) && remoteNode->isLLDPSupported())
            {
               nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("ProcessLLDPConnectionEntry(%s [%u], %s): lldpRemLocalPortNum is invalid, attempt to find matching information on remote node %s [%u]"),
                        node->getName(), node->getId(), LLDP_MIB_NAME(lldpMibVersion2), remoteNode->getName(), remoteNode->getId());
               info.ifLocal = FindLocalInterfaceOnRemoteNode(node, remoteNode.get(), false);
               if ((info.ifLocal == 0) && remoteNode->isLLDPV2MIBSupported())
                  info.ifLocal = FindLocalInterfaceOnRemoteNode(node, remoteNode.get(), true);
               nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("ProcessLLDPConnectionEntry(%s [%u], %s): ifLocal=%u after lookup on remote node"), node->getName(), node->getId(), LLDP_MIB_NAME(lldpMibVersion2), info.ifLocal);
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("ProcessLLDPConnectionEntry(%s [%u], %s): lldpRemLocalPortNum is invalid and remote interface is not known"), node->getName(), node->getId(), LLDP_MIB_NAME(lldpMibVersion2));
               info.ifLocal = 0;
            }
			}

			nbs->addConnection(&info);
         nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("ProcessLLDPConnectionEntry(%s [%u], %s): added connection: objectId=%u ifRemote=%u ifLocal=%u"), node->getName(), node->getId(), LLDP_MIB_NAME(lldpMibVersion2), info.objectId, info.ifRemote, info.ifLocal);
		}
		else
		{
         nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("ProcessLLDPConnectionEntry(%s [%u], %s): remote node not found"), node->getName(), node->getId(), LLDP_MIB_NAME(lldpMibVersion2));
		}
	}
	else
	{
      TCHAR remoteId[256];
      BinToStr(lldpRemChassisId->getValue(), lldpRemChassisId->getValueLength(), remoteId);
	   nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("ProcessLLDPConnectionEntry(%s [%u], %s): SNMP get failed for remote ID %s"), node->getName(), node->getId(), LLDP_MIB_NAME(lldpMibVersion2), remoteId);
	}
}

/**
 * Add LLDP-discovered neighbors using specific LLDP MIB version
 */
static void AddLLDPNeighbors(Node *node, LinkLayerNeighbors *nbs, bool lldpMibVersion2)
{
   StringObjectMap<SNMP_Variable> *connections = ReadLLDPRemoteTable(node, lldpMibVersion2);
   if (connections == nullptr)
      return;

   const TCHAR *oidPrefix = lldpMibVersion2 ? _T(".1.3.111.2.802.1.1.13.1.4.1.1.6.") : _T(".1.0.8802.1.1.2.1.4.1.1.5.");
   size_t oidPrefixLen = _tcslen(oidPrefix);

   StringList *oids = connections->keys();
   for(int i = 0; i < oids->size(); i++)
   {
      const TCHAR *oid = oids->get(i);
      if (_tcsncmp(oid, oidPrefix, oidPrefixLen))
         continue;
      SNMP_Variable *var = connections->get(oid);
      ProcessLLDPConnectionEntry(node, *connections, lldpMibVersion2, var, nbs);
   }
   delete oids;
   delete connections;
}

/**
 * Add LLDP-discovered neighbors
 */
void AddLLDPNeighbors(Node *node, LinkLayerNeighbors *nbs)
{
	if (!node->isLLDPSupported())
		return;

	nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("Collecting LLDP topology information for node %s [%u]"), node->getName(), node->getId());

	AddLLDPNeighbors(node, nbs, false);
	if (node->isLLDPV2MIBSupported())
	   AddLLDPNeighbors(node, nbs, true);

	nxlog_debug_tag(DEBUG_TAG_TOPO_LLDP, 5, _T("Finished collecting LLDP topology information for node %s [%u]"), node->getName(), node->getId());
}

/**
 * Parse MAC address. Could be without separators or with any separator char.
 */
static bool ParseMACAddress(const char *text, size_t length, BYTE *mac, size_t *macLength)
{
   bool withSeparator = false;
   char separator = 0;
   int p = 0;
   bool hi = true;
   for(size_t i = 0; (i < length) && (p < 64); i++)
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
String BuildLldpId(uint32_t type, const BYTE *data, size_t length)
{
   StringBuffer sb;
   sb.append(type);
   sb.append(_T('@'));
   if (type == 4)
   {
      // Some D-Link switches returns MAC address for ID type 4 as formatted text instead of raw bytes
      BYTE macAddr[64];
      size_t macLength;
      if ((length >= MAC_ADDR_LENGTH * 2) && (length <= MAC_ADDR_LENGTH * 3) && ParseMACAddress(reinterpret_cast<const char*>(data), length, macAddr, &macLength))
      {
         sb.appendAsHexString(macAddr, macLength);
      }
      else
      {
         sb.appendAsHexString(data, length);
      }
   }
   else
   {
      sb.appendAsHexString(data, length);
   }
   return sb;
}
