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
** File: graph.cpp
**
**/

#include "libnxmap.h"


//
// Default constructor
//

nxGraph::nxGraph()
{
   m_dwVertexCount = 0;
   m_ppVertexList = NULL;
   m_dwVertexCount = 0;
   m_ppVertexList = NULL;
   m_pRoot = NULL;
}


//
// Create graph from list of objects and links
//

nxGraph::nxGraph(DWORD dwNumObjects, DWORD *pdwObjectList,
                 DWORD dwNumLinks, OBJLINK *pLinkList)
{
   DWORD i;
   nxVertex *pVtx1, *pVtx2;

   m_dwVertexCount = dwNumObjects;
   m_ppVertexList = (nxVertex **)malloc(sizeof(nxVertex *) * dwNumObjects);
   for(i = 0; i < dwNumObjects; i++)
   {
      m_ppVertexList[i] = new nxVertex(pdwObjectList[i]);
   }

   // Create links between vertexes
   for(i = 0; i < dwNumLinks; i++)
   {
      pVtx1 = FindVertex(pLinkList[i].dwId1);
      pVtx2 = FindVertex(pLinkList[i].dwId2);
      if ((pVtx1 != NULL) && (pVtx2 != NULL))
         pVtx1->LinkChild(pVtx2);
   }

   m_pRoot = NULL;   // No selected root by default
}


//
// Destructor
//

nxGraph::~nxGraph()
{
   DWORD i;

   for(i = 0; i < m_dwVertexCount; i++)
      delete m_ppVertexList[i];
   safe_free(m_ppVertexList);
}


//
// Set all vertexes as unprocessed
//

void nxGraph::SetAsUnprocessed(void)
{
   DWORD i;

   for(i = 0; i < m_dwVertexCount; i++)
      m_ppVertexList[i]->SetAsUnprocessed();
}


//
// Find vertex by ID
//

nxVertex *nxGraph::FindVertex(DWORD dwId)
{
   DWORD i;

   for(i = 0; i < m_dwVertexCount; i++)
      if (m_ppVertexList[i]->GetId() == dwId)
         return m_ppVertexList[i];
   return NULL;
}


//
// Get internal index of given vertex
//

DWORD nxGraph::GetVertexIndex(nxVertex *pVertex)
{
   DWORD i;

   for(i = 0; i < m_dwVertexCount; i++)
      if (m_ppVertexList[i]->GetId() == pVertex->GetId())
         return i;
   return INVALID_INDEX;
}


//
// Normalize vertex positions
//

void nxGraph::NormalizeVertexPositions(void)
{
   DWORD i;
   int x, y, dx, dy;

   // Calculate required offset
   for(i = 0, dx = 0, dy = 0; i < m_dwVertexCount; i++)
   {
      x = m_ppVertexList[i]->GetPosX();
      if (x < MAP_LEFT_MARGIN)
         if (dx < MAP_LEFT_MARGIN - x)
            dx = MAP_LEFT_MARGIN - x;
      y = m_ppVertexList[i]->GetPosY();
      if (y < MAP_TOP_MARGIN)
         if (dy < MAP_TOP_MARGIN - y)
            dy = MAP_TOP_MARGIN - y;
   }

   // Change coordinates
   if ((dx != 0) || (dy != 0))
   {
      for(i = 0; i < m_dwVertexCount; i++)
         m_ppVertexList[i]->SetPosition(m_ppVertexList[i]->GetPosX() + dx,
                                        m_ppVertexList[i]->GetPosY() + dy);
   }
}


//
// Set root vertex
//

void nxGraph::SetRootVertex(DWORD dwId)
{
   DWORD i;

   for(i = 0; i < m_dwVertexCount; i++)
      if (m_ppVertexList[i]->GetId() == dwId)
      {
         m_pRoot = m_ppVertexList[i];
         return;
      }
   m_pRoot = NULL;
}


//
// Normalize links for given vertex
//

void nxGraph::NormalizeVertexLinks(nxVertex *pRoot)
{
   DWORD i;

   for(i = 0; i < m_dwVertexCount; i++)
   {
      if (!m_ppVertexList[i]->IsProcessed() && m_ppVertexList[i]->IsParentOf(pRoot))
      {
         m_ppVertexList[i]->UnlinkChild(pRoot);
         pRoot->LinkChild(m_ppVertexList[i]);
      }
      m_ppVertexList[i]->SetAsProcessed();
   }

   for(i = 0; i < pRoot->GetNumChilds(); i++)
      NormalizeVertexLinks(pRoot->GetChild(i));
}


//
// Normalize links - make all links to go from root when possible
//

void nxGraph::NormalizeLinks(void)
{
   if (m_pRoot != NULL)
   {
      SetAsUnprocessed();
      m_pRoot->SetAsProcessed();
      NormalizeVertexLinks(m_pRoot);
   }
}
