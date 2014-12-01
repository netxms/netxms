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
** File: layer2.cpp
**
**/

#include "nxcore.h"

/**
 * Build layer 2 topology for switch
 */
void BuildL2Topology(nxmap_ObjList &topology, Node *root, int nDepth, bool includeEndNodes)
{
	if (topology.isObjectExist(root->getId()))
		return;  // loop in object connections

	topology.addObject(root->getId());

	LinkLayerNeighbors *nbs = root->getLinkLayerNeighbors();
	if (nbs == NULL)
		return;

	for(int i = 0; i < nbs->size(); i++)
	{
		LL_NEIGHBOR_INFO *info = nbs->getConnection(i);
		if (info != NULL)
		{
			Node *node = (Node *)FindObjectById(info->objectId);
			if ((node != NULL) && (nDepth > 0) && (node->isBridge() || includeEndNodes))
			{
				BuildL2Topology(topology, node, nDepth - 1, includeEndNodes);
				Interface *ifLocal = root->findInterface(info->ifLocal, INADDR_ANY);
				Interface *ifRemote = node->findInterface(info->ifRemote, INADDR_ANY);
				DbgPrintf(5, _T("BuildL2Topology: root=%s [%d], node=%s [%d], ifLocal=%d %p, ifRemote=%d %p"),
				          root->getName(), root->getId(), node->getName(), node->getId(), info->ifLocal, ifLocal, info->ifRemote, ifRemote);
				topology.linkObjectsEx(root->getId(), node->getId(),
					(ifLocal != NULL) ? ifLocal->getName() : _T("N/A"),
					(ifRemote != NULL) ? ifRemote->getName() : _T("N/A"),
					info->ifLocal, info->ifRemote);
			}
		}
	}
	nbs->decRefCount();
}

/**
 * Find connection point for interface
 */
NetObj *FindInterfaceConnectionPoint(const BYTE *macAddr, int *type)
{
	TCHAR macAddrText[32];
	DbgPrintf(6, _T("Called FindInterfaceConnectionPoint(%s)"), MACToStr(macAddr, macAddrText));

   *type = CP_TYPE_INDIRECT;

	NetObj *cp = NULL;
	ObjectArray<NetObj> *nodes = g_idxNodeById.getObjects(true);

	Node *bestMatchNode = NULL;
	UINT32 bestMatchIfIndex = 0;
	int bestMatchCount = 0x7FFFFFFF;

	for(int i = 0; (i < nodes->size()) && (cp == NULL); i++)
	{
		Node *node = (Node *)nodes->get(i);
		
      ForwardingDatabase *fdb = node->getSwitchForwardingDatabase();
		if (fdb != NULL)
		{
			DbgPrintf(6, _T("FindInterfaceConnectionPoint(%s): FDB obtained for node %s [%d]"),
			          macAddrText, node->getName(), (int)node->getId());
			UINT32 ifIndex = fdb->findMacAddress(macAddr);
			if (ifIndex != 0)
			{
				int count = fdb->getMacCountOnPort(ifIndex);
				if (count == 1)
				{
					Interface *iface = node->findInterface(ifIndex, INADDR_ANY);
					if (iface != NULL)
					{
						DbgPrintf(4, _T("FindInterfaceConnectionPoint(%s): found interface %s [%d] on node %s [%d]"), macAddrText,
									 iface->getName(), (int)iface->getId(), iface->getParentNode()->getName(), (int)iface->getParentNode()->getId());
                  cp = iface;
                  *type = CP_TYPE_DIRECT;
					}
					else
					{
						DbgPrintf(4, _T("FindInterfaceConnectionPoint(%s): cannot find interface object for node %s [%d] ifIndex %d"),
									 macAddrText, node->getName(), node->getId(), ifIndex);
					}
				}
				else if (count < bestMatchCount)
				{
					bestMatchCount = count;
					bestMatchNode = node;
					bestMatchIfIndex = ifIndex;
					DbgPrintf(4, _T("FindInterfaceConnectionPoint(%s): found potential interface [ifIndex=%d] on node %s [%d], count %d"), 
					          macAddrText, ifIndex, node->getName(), (int)node->getId(), count);
				}
			}
			fdb->decRefCount();
		}

      if (node->isWirelessController())
      {
			DbgPrintf(6, _T("FindInterfaceConnectionPoint(%s): node %s [%d] is a wireless controller, checking associated stations"),
			          macAddrText, node->getName(), (int)node->getId());
         ObjectArray<WirelessStationInfo> *wsList = node->getWirelessStations();
         if (wsList != NULL)
         {
			   DbgPrintf(6, _T("FindInterfaceConnectionPoint(%s): %d wireless stations registered on node %s [%d]"),
                      macAddrText, wsList->size(), node->getName(), (int)node->getId());

            for(int i = 0; i < wsList->size(); i++)
            {
               WirelessStationInfo *ws = wsList->get(i);
               if (!memcmp(ws->macAddr, macAddr, MAC_ADDR_LENGTH))
               {
                  AccessPoint *ap = (AccessPoint *)FindObjectById(ws->apObjectId, OBJECT_ACCESSPOINT);
                  if (ap != NULL)
                  {
						   DbgPrintf(4, _T("FindInterfaceConnectionPoint(%s): found matching wireless station on node %s [%d] AP %s"), macAddrText,
									    node->getName(), (int)node->getId(), ap->getName());
                     cp = ap;
                     *type = CP_TYPE_WIRELESS;
                  }
                  else
                  {
                     Interface *iface = node->findInterface(ws->rfIndex, INADDR_ANY);
                     if (iface != NULL)
                     {
						      DbgPrintf(4, _T("FindInterfaceConnectionPoint(%s): found matching wireless station on node %s [%d] interface %s"),
                           macAddrText, node->getName(), (int)node->getId(), iface->getName());
                        cp = iface;
                        *type = CP_TYPE_WIRELESS;
                     }
                     else
                     {
						      DbgPrintf(4, _T("FindInterfaceConnectionPoint(%s): found matching wireless station on node %s [%d] but cannot determine AP or interface"), 
                           macAddrText, node->getName(), (int)node->getId());
                     }
                  }
                  break;
               }
            }

            delete wsList;
         }
         else
         {
			   DbgPrintf(6, _T("FindInterfaceConnectionPoint(%s): unable to get wireless stations from node %s [%d]"),
			             macAddrText, node->getName(), (int)node->getId());
         }
      }

      node->decRefCount();
	}
	delete nodes;

	if ((cp == NULL) && (bestMatchNode != NULL))
	{
		cp = bestMatchNode->findInterface(bestMatchIfIndex, INADDR_ANY);
	}
	return cp;
}
