/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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
** File: layer2.cpp
**
**/

#include "nxcore.h"


//
// Build layer 2 topology for switch
//

void BuildL2Topology(nxmap_ObjList &topology, Node *root, int nDepth)
{
	topology.AddObject(root->Id());

	LinkLayerNeighbors *nbs = root->getLinkLayerNeighbors();
	if (nbs == NULL)
		return;

	for(int i = 0; i < nbs->getSize(); i++)
	{
		LL_NEIGHBOR_INFO *info = nbs->getConnection(i);
		if (info != NULL)
		{
			Node *node = (Node *)FindObjectById(info->objectId);
			if ((node != NULL) && (nDepth > 0))
			{
				BuildL2Topology(topology, node, nDepth - 1);
				Interface *ifLocal = root->findInterface(info->ifLocal, INADDR_ANY);
				Interface *ifRemote = node->findInterface(info->ifRemote, INADDR_ANY);
				DbgPrintf(5, _T("BuildL2Topology: root=%s [%d], node=%s [%d], ifLocal=%d %p, ifRemote=%d %p"),
				          root->Name(), root->Id(), node->Name(), node->Id(), info->ifLocal, ifLocal, info->ifRemote, ifRemote);
				topology.LinkObjectsEx(root->Id(), node->Id(),
					(ifLocal != NULL) ? ifLocal->Name() : _T("N/A"),
					(ifRemote != NULL) ? ifRemote->Name() : _T("N/A"),
					info->ifLocal, info->ifRemote);
			}
		}
	}
	nbs->decRefCount();
}


//
// Find connection point for interface
//

Interface *FindInterfaceConnectionPoint(const BYTE *macAddr, bool *exactMatch)
{
	TCHAR macAddrText[32];
	DbgPrintf(6, _T("Called FindInterfaceConnectionPoint(%s)"), MACToStr(macAddr, macAddrText));

	Interface *iface = NULL;
	ObjectArray<NetObj> *nodes = g_idxNodeById.getObjects();

	Node *bestMatchNode = NULL;
	DWORD bestMatchIfIndex = 0;
	int bestMatchCount = 0x7FFFFFFF;

	for(int i = 0; (i < nodes->size()) && (iface == NULL); i++)
	{
		Node *node = (Node *)nodes->get(i);
		ForwardingDatabase *fdb = node->getSwitchForwardingDatabase();
		if (fdb != NULL)
		{
			DbgPrintf(6, _T("FindInterfaceConnectionPoint(%s): FDB obtained for node %s [%d]"),
			          macAddrText, node->Name(), (int)node->Id());
			DWORD ifIndex = fdb->findMacAddress(macAddr);
			if (ifIndex != 0)
			{
				int count = fdb->getMacCountOnPort(ifIndex);
				if (count == 1)
				{
					iface = node->findInterface(ifIndex, INADDR_ANY);
					if (iface != NULL)
					{
						DbgPrintf(4, _T("FindInterfaceConnectionPoint(%s): found interface %s [%d] on node %s [%d]"), macAddrText,
									 iface->Name(), (int)iface->Id(), iface->getParentNode()->Name(), (int)iface->getParentNode()->Id());
					}
					else
					{
						DbgPrintf(4, _T("FindInterfaceConnectionPoint(%s): cannot find interface object for node %s [%d] ifIndex %d"),
									 macAddrText, node->Name(), node->Id(), ifIndex);
					}
				}
				else if (count < bestMatchCount)
				{
					bestMatchCount = count;
					bestMatchNode = node;
					bestMatchIfIndex = ifIndex;
					DbgPrintf(4, _T("FindInterfaceConnectionPoint(%s): found potential interface [ifIndex=%d] on node %s [%d], count %d"), 
					          macAddrText, ifIndex, node->Name(), (int)node->Id(), count);
				}
			}
			fdb->decRefCount();
		}
	}
	delete nodes;

	if (iface != NULL)
	{
		*exactMatch = true;
	}
	else if (bestMatchNode != NULL)
	{
		*exactMatch = false;
		iface = bestMatchNode->findInterface(bestMatchIfIndex, INADDR_ANY);
	}
	return iface;
}
