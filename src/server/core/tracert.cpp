/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Victor Kirhenshtein
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
** File: tracert.cpp
**/

#include "nxcore.h"

/**
 * Network path constructor
 */
NetworkPath::NetworkPath(const InetAddress& srcAddr)
{
   m_sourceAddress = srcAddr;
	m_hopCount = 0;
	m_allocated = 0;
	m_path = NULL;
	m_complete = false;
}

/**
 * Network path destructor
 */
NetworkPath::~NetworkPath()
{
	for(int i = 0; i < m_hopCount; i++)
		if (m_path[i].object != NULL)
			m_path[i].object->decRefCount();
	free(m_path);
}

/**
 * Add hop to path
 */
void NetworkPath::addHop(const InetAddress& nextHop, const InetAddress& route, NetObj *currentObject, UINT32 ifIndex, bool isVpn, const TCHAR *name)
{
	if (m_hopCount == m_allocated)
	{
		m_allocated += 16;
		m_path = (HOP_INFO *)realloc(m_path, sizeof(HOP_INFO) * m_allocated);
	}
	m_path[m_hopCount].nextHop = nextHop;
   m_path[m_hopCount].route = route;
	m_path[m_hopCount].object = currentObject;
	m_path[m_hopCount].ifIndex = ifIndex;
	m_path[m_hopCount].isVpn = isVpn;
   nx_strncpy(m_path[m_hopCount].name, name, MAX_OBJECT_NAME);
	m_hopCount++;
	if (currentObject != NULL)
		currentObject->incRefCount();
}

/**
 * Fill NXCP message with trace data
 */
void NetworkPath::fillMessage(NXCPMessage *msg)
{
	msg->setField(VID_HOP_COUNT, (WORD)m_hopCount);
	msg->setField(VID_IS_COMPLETE, (WORD)(m_complete ? 1 : 0));
	UINT32 varId = VID_NETWORK_PATH_BASE;
	for(int i = 0; i < m_hopCount; i++, varId += 5)
	{
		msg->setField(varId++, m_path[i].object->getId());
		msg->setField(varId++, m_path[i].nextHop);
		msg->setField(varId++, m_path[i].ifIndex);
		msg->setField(varId++, (WORD)(m_path[i].isVpn ? 1 : 0));
		msg->setField(varId++, m_path[i].name);
	}
}

/**
 * Trace route between two nodes
 */
NetworkPath *TraceRoute(Node *pSrc, Node *pDest)
{
   UINT32 srcIfIndex;
   InetAddress srcAddr;
   if (!pSrc->getOutwardInterface(pDest->getIpAddress(), &srcAddr, &srcIfIndex))
      srcAddr = pSrc->getIpAddress();

   NetworkPath *path = new NetworkPath(srcAddr);

   int hopCount = 0;
   Node *pCurr, *pNext;
   for(pCurr = pSrc; (pCurr != pDest) && (pCurr != NULL) && (hopCount < 30); pCurr = pNext, hopCount++)
   {
      UINT32 dwIfIndex;
      InetAddress nextHop;
      InetAddress route;
      bool isVpn;
      TCHAR name[MAX_OBJECT_NAME];
      if (pCurr->getNextHop(srcAddr, pDest->getIpAddress(), &nextHop, &route, &dwIfIndex, &isVpn, name))
      {
			pNext = FindNodeByIP(pSrc->getZoneUIN(), nextHop);
			path->addHop(nextHop, route, pCurr, dwIfIndex, isVpn, name);
         if ((pNext == pCurr) || !nextHop.isValid())
            pNext = NULL;     // Directly connected subnet or too many hops, stop trace
      }
      else
      {
         pNext = NULL;
      }
   }
	if (pCurr == pDest)
	{
      path->addHop(InetAddress::INVALID, InetAddress::INVALID, pCurr, 0, false, _T(""));
		path->setComplete();
	}

   return path;
}
