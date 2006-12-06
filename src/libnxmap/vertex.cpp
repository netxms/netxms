/* 
** NetXMS - Network Management System
** Network Maps Library
** Copyright (C) 2006 Victor Kirhenshtein
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
** File: vertex.cpp
**
**/

#include "libnxmap.h"


//
// Constructor
//

nxVertex::nxVertex(DWORD dwId)
{
   m_dwId = dwId;
   m_posX = 0;
   m_posY = 0;
   m_dwNumLinks = 0;
   m_ppLinkList = NULL;
}


//
// Destructor
//

nxVertex::~nxVertex()
{
   safe_free(m_ppLinkList);
}


//
// Link another vertex
//

void nxVertex::Link(nxVertex *pVtx)
{
   DWORD i;

   for(i = 0; i < m_dwNumLinks; i++)
      if (m_ppLinkList[i] == pVtx)
         break;   // Already linked
   if (i == m_dwNumLinks)
   {
      m_dwNumLinks++;
      m_ppLinkList = (nxVertex **)realloc(m_ppLinkList, sizeof(nxVertex *) * m_dwNumLinks);
      m_ppLinkList[i] = pVtx;
   }
}
