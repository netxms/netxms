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
** $module: nms_topo.h
**
**/

#ifndef _nms_topo_h_
#define _nms_topo_h_

#include <netxms_maps.h>


//
// Hop information structure
//

struct HOP_INFO
{
   DWORD dwNextHop;     // Next hop address
   NetObj *pObject;     // Current hop object
   DWORD dwIfIndex;     // Interface index or VPN connector object ID
   BOOL bIsVPN;         // TRUE if next hop is behind VPN tunnel
};


//
// Network path trace
//

struct NETWORK_PATH_TRACE
{
   int iNumHops;
   HOP_INFO *pHopList;
};


//
// Topology functions
//

NETWORK_PATH_TRACE *TraceRoute(Node *pSrc, Node *pDest);
void DestroyTraceData(NETWORK_PATH_TRACE *pTrace);
DWORD BuildL2Topology(nxmap_ObjList &topology, Node *pRoot, Node *pParent,
							 int nDepth, TCHAR *pszParentIfName);


#endif   /* _nms_topo_h_ */
