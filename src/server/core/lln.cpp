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
 * Constructor
 */
LinkLayerNeighbors::LinkLayerNeighbors() : m_connections(0, 16)
{
	memset(m_data, 0, sizeof(m_data));
}

/**
 * Check if given information is duplicate
 */
bool LinkLayerNeighbors::isDuplicate(LL_NEIGHBOR_INFO *info)
{
	for(int i = 0; i < m_connections.size(); i++)
	{
	   LL_NEIGHBOR_INFO *n = m_connections.get(i);
		if (n->ifLocal == info->ifLocal)
		{
		   if ((n->ifRemote != info->ifRemote) || (n->objectId != info->objectId))
		   {
		      nxlog_debug(5, _T("LinkLayerNeighbors::isDuplicate: inconsistent data: %s(ifLocal=%d remote=%d/%d) %s(ifLocal=%d remote=%d/%d)"),
		                  GetLinkLayerProtocolName(n->protocol), n->ifLocal, n->objectId, n->ifRemote,
                        GetLinkLayerProtocolName(info->protocol), info->ifLocal, info->objectId, info->ifRemote);
		   }
         return true;
		}
	}
	return false;
}

/**
 * Add neighbor
 */
void LinkLayerNeighbors::addConnection(LL_NEIGHBOR_INFO *info)
{
	if ((info->ifLocal == 0) || (info->ifRemote == 0))
		return;		// Do not add incomplete information

	if (isDuplicate(info))
		return;

	m_connections.add(info);
}

/**
 * Gather link layer connectivity information from node
 */
shared_ptr<LinkLayerNeighbors> BuildLinkLayerNeighborList(Node *node)
{
	LinkLayerNeighbors *nbs = new LinkLayerNeighbors();

	if (node->getCapabilities() & NC_IS_LLDP)
	{
		AddLLDPNeighbors(node, nbs);
	}
	if (node->getCapabilities() & NC_IS_CDP)
	{
		AddCDPNeighbors(node, nbs);
	}
	if (node->getCapabilities() & NC_IS_NDP)
	{
		AddNDPNeighbors(node, nbs);
	}

	// For bridges get STP data and scan forwarding database
	if (node->isBridge())
	{
      AddSTPNeighbors(node, nbs);
		node->addHostConnections(nbs);
	}

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
      case LL_PROTO_NDP:
         return _T("NDP");
      case LL_PROTO_EDP:
         return _T("EDP");
      case LL_PROTO_STP:
         return _T("STP");
      default:
         return _T("UNKNOWN");
   }
}
