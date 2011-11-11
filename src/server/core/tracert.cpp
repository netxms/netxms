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
** File: tracert.cpp
**/

#include "nxcore.h"


//
// Network path constructor
//

NetworkPath::NetworkPath()
{
	m_hopCount = 0;
	m_allocated = 0;
	m_path = NULL;
	m_complete = false;
}


//
// Network path destructor
//

NetworkPath::~NetworkPath()
{
	for(int i = 0; i < m_hopCount; i++)
		if (m_path[i].object != NULL)
			m_path[i].object->DecRefCount();
	safe_free(m_path);
}


//
// Add hop to path
//

void NetworkPath::addHop(DWORD nextHop, NetObj *currentObject, DWORD ifIndex, bool isVpn)
{
	if (m_hopCount == m_allocated)
	{
		m_allocated += 16;
		m_path = (HOP_INFO *)realloc(m_path, sizeof(HOP_INFO) * m_allocated);
	}
	m_path[m_hopCount].nextHop = nextHop;
	m_path[m_hopCount].object = currentObject;
	m_path[m_hopCount].ifIndex = ifIndex;
	m_path[m_hopCount].isVpn = isVpn;
	m_hopCount++;
	if (currentObject != NULL)
		currentObject->IncRefCount();
}


//
// Fill NXCP message with trace data
//

void NetworkPath::fillMessage(CSCPMessage *msg)
{
	msg->SetVariable(VID_HOP_COUNT, (WORD)m_hopCount);
	msg->SetVariable(VID_IS_COMPLETE, (WORD)(m_complete ? 1 : 0));
	DWORD varId = VID_NETWORK_PATH_BASE;
	for(int i = 0; i < m_hopCount; i++, varId += 6)
	{
		msg->SetVariable(varId++, m_path[i].object->Id());
		msg->SetVariable(varId++, m_path[i].nextHop);
		msg->SetVariable(varId++, m_path[i].ifIndex);
		msg->SetVariable(varId++, (WORD)(m_path[i].isVpn ? 1 : 0));
	}
}


//
// Trace route between two nodes
//

NetworkPath *TraceRoute(Node *pSrc, Node *pDest)
{
   DWORD dwNextHop, dwIfIndex, dwHopCount;
   Node *pCurr, *pNext;
   NetworkPath *pTrace;
   BOOL bIsVPN;

   pTrace = new NetworkPath;
   
   for(pCurr = pSrc, dwHopCount = 0; (pCurr != pDest) && (pCurr != NULL) && (dwHopCount < 30); pCurr = pNext, dwHopCount++)
   {
      if (pCurr->getNextHop(pSrc->IpAddr(), pDest->IpAddr(), &dwNextHop, &dwIfIndex, &bIsVPN))
      {
			pNext = FindNodeByIP(pSrc->getZoneId(), dwNextHop);
			pTrace->addHop(dwNextHop, pCurr, dwIfIndex, bIsVPN ? true : false);
         if ((pNext == pCurr) || (dwNextHop == 0))
            pNext = NULL;     // Directly connected subnet or too many hops, stop trace
      }
      else
      {
         pNext = NULL;
      }
   }
	if (pCurr == pDest)
	{
		pTrace->addHop(0, pCurr, 0, false);
		pTrace->setComplete();
	}

   return pTrace;
}
