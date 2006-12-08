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
   m_dwNumChilds = 0;
   m_ppChildList = NULL;
   m_dwNumParents = 0;
   m_ppParentList = NULL;
}


//
// Destructor
//

nxVertex::~nxVertex()
{
   safe_free(m_ppChildList);
   safe_free(m_ppParentList);
}


//
// Link another vertex as child
//

void nxVertex::LinkChild(nxVertex *pVtx)
{
   DWORD i;

   for(i = 0; i < m_dwNumChilds; i++)
      if (m_ppChildList[i] == pVtx)
         break;   // Already linked
   if (i == m_dwNumChilds)
   {
      m_dwNumChilds++;
      m_ppChildList = (nxVertex **)realloc(m_ppChildList, sizeof(nxVertex *) * m_dwNumChilds);
      m_ppChildList[i] = pVtx;
      pVtx->LinkParent(this);
   }
}


//
// Unlink child vertex
//

void nxVertex::UnlinkChild(nxVertex *pVtx)
{
   DWORD i;

   for(i = 0; i < m_dwNumChilds; i++)
      if (m_ppChildList[i] == pVtx)
      {
         m_dwNumChilds--;
         memmove(&m_ppChildList[i], &m_ppChildList[i + 1], (m_dwNumChilds - i) * sizeof(nxVertex *));
         pVtx->UnlinkParent(this);
         break;
      }
}


//
// Link another vertex as parent
//

void nxVertex::LinkParent(nxVertex *pVtx)
{
   DWORD i;

   for(i = 0; i < m_dwNumParents; i++)
      if (m_ppParentList[i] == pVtx)
         break;   // Already linked
   if (i == m_dwNumParents)
   {
      m_dwNumParents++;
      m_ppParentList = (nxVertex **)realloc(m_ppParentList, sizeof(nxVertex *) * m_dwNumParents);
      m_ppParentList[i] = pVtx;
   }
}


//
// Unlink parent vertex
//

void nxVertex::UnlinkParent(nxVertex *pVtx)
{
   DWORD i;

   for(i = 0; i < m_dwNumParents; i++)
      if (m_ppParentList[i] == pVtx)
      {
         m_dwNumParents--;
         memmove(&m_ppParentList[i], &m_ppParentList[i + 1], (m_dwNumParents - i) * sizeof(nxVertex *));
         break;
      }
}


//
// Check if link to given vertex exist
//

BOOL nxVertex::IsParentOf(nxVertex *pVtx)
{
   DWORD i;
   BOOL bRet = FALSE;

   for(i = 0; i < m_dwNumChilds; i++)
      if (m_ppChildList[i] == pVtx)
      {
         bRet = TRUE;
         break;
      }
   return bRet;
}
