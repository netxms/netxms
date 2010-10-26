/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005 Victor Kirhenshtein
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
** $module: tracert.cpp
**
**/

#include "nxcore.h"


//
// Trace route between two nodes
//

NETWORK_PATH_TRACE *TraceRoute(Node *pSrc, Node *pDest)
{
   DWORD dwNextHop, dwIfIndex, dwHopCount;
   Node *pCurr, *pNext;
   NETWORK_PATH_TRACE *pTrace;
   BOOL bIsVPN;

   pTrace = (NETWORK_PATH_TRACE *)malloc(sizeof(NETWORK_PATH_TRACE));
   memset(pTrace, 0, sizeof(NETWORK_PATH_TRACE));
   
   for(pCurr = pSrc, dwHopCount = 0; (pCurr != pDest) && (pCurr != NULL) && (dwHopCount < 30); pCurr = pNext, dwHopCount++)
   {
      if (pCurr->getNextHop(pSrc->IpAddr(), pDest->IpAddr(), &dwNextHop, &dwIfIndex, &bIsVPN))
      {
         pNext = FindNodeByIP(dwNextHop);
         pTrace->pHopList = (HOP_INFO *)realloc(pTrace->pHopList, sizeof(HOP_INFO) * (pTrace->iNumHops + 1));
         pTrace->pHopList[pTrace->iNumHops].dwNextHop = dwNextHop;
         pTrace->pHopList[pTrace->iNumHops].dwIfIndex = dwIfIndex;
         pTrace->pHopList[pTrace->iNumHops].bIsVPN = bIsVPN;
         pTrace->pHopList[pTrace->iNumHops].pObject = pCurr;
         pTrace->iNumHops++;
         if ((pNext == pCurr) || (dwNextHop == 0))
            pNext = NULL;     // Directly connected subnet or too many hops, stop trace
      }
      else
      {
         pNext = NULL;
      }
   }

   return pTrace;
}


//
// Destroy trace information
//

void DestroyTraceData(NETWORK_PATH_TRACE *pTrace)
{
   if (pTrace != NULL)
   {
      safe_free(pTrace->pHopList);
      free(pTrace);
   }
}
