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
** File: lln.cpp
**
**/

#include "nxcore.h"

/**
 * Check if given information is duplicate
 */
bool LinkLayerNeighbors::isDuplicate(const LL_NEIGHBOR_INFO& info)
{
	for(int i = 0; i < m_connections.size(); i++)
	{
	   LL_NEIGHBOR_INFO *n = m_connections.get(i);
		if (n->ifLocal == info.ifLocal)
		{
		   if ((n->ifRemote != info.ifRemote) || (n->objectId != info.objectId))
		   {
		      nxlog_debug_tag(DEBUG_TAG_TOPO_LINK, 5, _T("LinkLayerNeighbors::isDuplicate: inconsistent data: %s(ifLocal=%d remote=%d/%d) %s(ifLocal=%d remote=%d/%d)"),
		                  GetLinkLayerProtocolName(n->protocol), n->ifLocal, n->objectId, n->ifRemote,
                        GetLinkLayerProtocolName(info.protocol), info.ifLocal, info.objectId, info.ifRemote);
		   }
         return true;
		}
      if ((n->ifRemote == info.ifRemote) && (n->objectId == info.objectId))
      {
         if (n->ifLocal != info.ifLocal)
         {
            nxlog_debug_tag(DEBUG_TAG_TOPO_LINK, 5, _T("LinkLayerNeighbors::isDuplicate: inconsistent data: %s(ifLocal=%d remote=%d/%d) %s(ifLocal=%d remote=%d/%d)"),
                        GetLinkLayerProtocolName(n->protocol), n->ifLocal, n->objectId, n->ifRemote,
                        GetLinkLayerProtocolName(info.protocol), info.ifLocal, info.objectId, info.ifRemote);
         }
         return true;
      }
	}
	return false;
}

/**
 * Add neighbors reported by driver. Returns false if standard MIBs should be skipped.
 */
static bool AddDriverNeighbors(Node *node, LinkLayerNeighbors *nbs)
{
   if (!node->isSNMPSupported())
      return true;

   SNMP_Transport *snmp = node->createSnmpTransport();
   if (snmp == nullptr)
      return true;

   nxlog_debug_tag(DEBUG_TAG_TOPO_DRIVER, 5, _T("Collecting topology information from driver %s for node %s [%u]"), node->getDriver()->getName(), node->getName(), node->getId());
   bool ignoreStandardMibs = false;
   ObjectArray<LinkLayerNeighborInfo> *neighbors = node->getDriver()->getLinkLayerNeighbors(snmp, node->getDriverData(), &ignoreStandardMibs);
   if (neighbors != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_TOPO_DRIVER, 5, _T("%d link layer neighbors reported by driver for node %s [%u]"), neighbors->size(), node->getName(), node->getId());
      for(LinkLayerNeighborInfo *n : *neighbors)
      {
         shared_ptr<Node> remoteNode;
         if (n->remoteIP.isValidUnicast())
         {
            remoteNode = FindNodeByIP(node->getZoneUIN(), false, n->remoteIP);
         }
         else if (n->remoteMAC.isValid())
         {
            remoteNode = FindNodeByMAC(n->remoteMAC);
         }
         else if (n->remoteName[0] != 0)
         {
            remoteNode = FindNodeBySysName(n->remoteName);
            if (remoteNode == nullptr)
            {
               InetAddress a = ResolveHostName(node->getZoneUIN(), n->remoteName);
               if (a.isValidUnicast())
                  remoteNode = FindNodeByIP(node->getZoneUIN(), a);
            }
         }

         TCHAR ipAddrText[64], macAddrText[64];
         if (remoteNode != nullptr)
         {
            nxlog_debug_tag(DEBUG_TAG_TOPO_DRIVER, 5, _T("AddDriverNeighbors(%s [%u]): found remote node %s [%u] for entry %s / %s"),
               node->getName(), node->getId(), remoteNode->getName(), remoteNode->getId(), n->remoteIP.toString(ipAddrText), n->remoteMAC.toString(macAddrText));

            shared_ptr<Interface> ifRemote;
            switch(n->ifRemote.type)
            {
               case InterfaceIdType::INDEX:
                  ifRemote = remoteNode->findInterfaceByIndex(n->ifRemote.value.ifIndex);
                  break;
               case InterfaceIdType::NAME:
                  ifRemote = remoteNode->findInterfaceByName(n->ifRemote.value.ifName);
                  break;
            }

            if (ifRemote != nullptr)
            {
               nxlog_debug_tag(DEBUG_TAG_TOPO_DRIVER, 5, _T("AddDriverNeighbors(%s [%u]): found remote interface %s [%u] on remote node %s [%u]"),
                  node->getName(), node->getId(), ifRemote->getName(), ifRemote->getId(), remoteNode->getName(), remoteNode->getId());

               LL_NEIGHBOR_INFO info;
               info.ifLocal = n->ifLocal;
               info.objectId = remoteNode->getId();
               info.ifRemote = ifRemote->getIfIndex();
               info.isPtToPt = n->isPtToPt;
               info.protocol = n->protocol;
               info.isCached = false;
               nbs->addConnection(info);
            }
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG_TOPO_DRIVER, 5, _T("AddDriverNeighbors(%s [%u]): cannot find remote node (name=\"%s\", ip=%s, mac=%s)"),
               node->getName(), node->getId(), n->remoteName, n->remoteIP.toString(ipAddrText), n->remoteMAC.toString(macAddrText));
         }
      }
      delete neighbors;
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_TOPO_DRIVER, 5, _T("Driver for node %s [%u] cannot provide link layer topology information"), node->getName(), node->getId());
   }
   delete snmp;
   return !ignoreStandardMibs;
}

/**
 * Gather link layer connectivity information from node
 */
shared_ptr<LinkLayerNeighbors> BuildLinkLayerNeighborList(Node *node)
{
   nxlog_debug_tag(DEBUG_TAG_TOPO_LINK, 5, _T("BuildLinkLayerNeighborList(%s [%u]): building link level topology"), node->getName(), node->getId());

	LinkLayerNeighbors *nbs = new LinkLayerNeighbors();

	bool processStandardMibs = AddDriverNeighbors(node, nbs);
	if (processStandardMibs)
	{
      AddLLDPNeighbors(node, nbs);
      AddCDPNeighbors(node, nbs);
      AddNDPNeighbors(node, nbs);
	}
	else
	{
	   nxlog_debug_tag(DEBUG_TAG_TOPO_LINK, 5, _T("BuildLinkLayerNeighborList(%s [%u]): standard MIB processing is disabled for this node"), node->getName(), node->getId());
	}

	// For bridges get STP data and scan forwarding database
	if (node->isBridge())
	{
	   if (processStandardMibs)
	      AddSTPNeighbors(node, nbs);
		node->addHostConnections(nbs);
	}

   nxlog_debug_tag(DEBUG_TAG_TOPO_LINK, 5, _T("BuildLinkLayerNeighborList(%s [%u]): %d connections found"), node->getName(), node->getId(), nbs->size());

   // Add existing connections from interfaces. Mostly useful
   // for end nodes, because interfaces of end nodes should be linked to switches already,
   // but can be useful in other situations (for example, STP topology data can be obtained only on one side),
   // so we just walk node's interfaces and copy connection point information
   node->addExistingConnections(nbs);
	return shared_ptr<LinkLayerNeighbors>(nbs);
}

/**
 * Return protocol name
 */
const TCHAR *GetLinkLayerProtocolName(LinkLayerProtocol p)
{
   switch(p)
   {
      case LL_PROTO_FDB:
         return _T("FDB");
      case LL_PROTO_CDP:
         return _T("CDP");
      case LL_PROTO_LLDP:
         return _T("LLDP");
      case LL_PROTO_MANUAL:
         return _T("MANUAL");
      case LL_PROTO_NDP:
         return _T("NDP");
      case LL_PROTO_EDP:
         return _T("EDP");
      case LL_PROTO_STP:
         return _T("STP");
      case LL_PROTO_OTHER:
         return _T("OTHER");
      default:
         return _T("UNKNOWN");
   }
}
