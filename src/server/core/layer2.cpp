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
				topology.LinkObjectsEx(root->Id(), node->Id(),
					(TCHAR *)((ifLocal != NULL) ? ifLocal->Name() : _T("N/A")),
					(TCHAR *)((ifRemote != NULL) ? ifRemote->Name() : _T("N/A")));
			}
		}
	}
}


//
// Find connection point for interface
//

Interface *FindInterfaceConnectionPoint(const BYTE *macAddr)
{
	TCHAR macAddrText[32];
	DbgPrintf(6, _T("Called FindInterfaceConnectionPoint(%s)"), MACToStr(macAddr, macAddrText));

	if (g_pNodeIndexByAddr == NULL)
      return NULL;

	Interface *iface = NULL;
   RWLockReadLock(g_rwlockNodeIndex, INFINITE);
   for (DWORD i = 0; (i < g_dwNodeAddrIndexSize) && (iface == NULL); i++)
	{
		ForwardingDatabase *fdb = ((Node *)g_pNodeIndexByAddr[i].pObject)->getSwitchForwardingDatabase();
		if (fdb != NULL)
		{
			DbgPrintf(6, _T("FindInterfaceConnectionPoint(%s): FDB obtained for node %s [%d]"), macAddrText,
				((Node *)g_pNodeIndexByAddr[i].pObject)->Name(), (int)((Node *)g_pNodeIndexByAddr[i].pObject)->Id());
			DWORD ifIndex = fdb->findMacAddress(macAddr);
			if ((ifIndex != 0) && fdb->isSingleMacOnPort(ifIndex))
			{
				iface = ((Node *)g_pNodeIndexByAddr[i].pObject)->findInterface(ifIndex, INADDR_ANY);
				if (iface != NULL)
				{
					DbgPrintf(4, _T("FindInterfaceConnectionPoint(%s): found interface %s [%d] on node %s [%d]"), macAddrText,
						iface->Name(), (int)iface->Id(), iface->getParentNode()->Name(), (int)iface->getParentNode()->Id());
				}
				else
				{
					DbgPrintf(4, _T("FindInterfaceConnectionPoint(%s): cannot find interface object for node %s [%d] ifIndex %d"),
						macAddrText, ((Node *)g_pNodeIndexByAddr[i].pObject)->Name(), ((Node *)g_pNodeIndexByAddr[i].pObject)->Id(), ifIndex);
				}
			}
			fdb->decRefCount();
		}
	}
   RWLockUnlock(g_rwlockNodeIndex);
	return iface;
}
