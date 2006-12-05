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
** File: objlist.cpp
**
**/

#include "libnxmap.h"


//
// Constructor
//

nxObjList::nxObjList()
{
   m_dwNumObjects = 0;
   m_pdwObjectList = NULL;
   m_dwNumLinks = 0;
   m_pLinkList = NULL;
}


//
// Destructor
//

nxObjList::~nxObjList()
{
   safe_free(m_pdwObjectList);
   safe_free(m_pLinkList);
}


//
// Clear list
//

void nxObjList::Clear(void)
{
   safe_free_and_null(m_pdwObjectList);
   safe_free_and_null(m_pLinkList);
   m_dwNumObjects = 0;
   m_dwNumLinks = 0;
}


//
// Add object to list
//

void nxObjList::AddObject(DWORD dwId)
{
   DWORD i;

   for(i = 0; i < m_dwNumObjects; i++)
   {
      if (m_pdwObjectList[i] == dwId)
         break;
   }

   if (i == m_dwNumObjects)
   {
      m_dwNumObjects++;
      m_pdwObjectList = (DWORD *)realloc(m_pdwObjectList, sizeof(DWORD) * m_dwNumObjects);
      m_pdwObjectList[i] = dwId;
   }
}


//
// Link two objects
//

void nxObjList::LinkObjects(DWORD dwId1, DWORD dwId2)
{
   DWORD i;
   int nCount;

   // Validate object IDs
   for(i = 0, nCount = 0; (i < m_dwNumObjects) && (nCount < 2); i++)
   {
      if ((m_pdwObjectList[i] == dwId1) ||
          (m_pdwObjectList[i] == dwId2))
         nCount++;
   }

   if (nCount == 2)  // Both objects exist?
   {
      // Check for duplicate links
      for(i = 0; i < m_dwNumLinks; i++)
      {
         if (((m_pLinkList[i].dwId1 == dwId1) && (m_pLinkList[i].dwId2 == dwId2)) ||
             ((m_pLinkList[i].dwId2 == dwId1) && (m_pLinkList[i].dwId1 == dwId2)))
            break;
      }
      if (i == m_dwNumLinks)
      {
         m_dwNumLinks++;
         m_pLinkList = (OBJLINK *)realloc(m_pLinkList, sizeof(OBJLINK) * m_dwNumLinks);
         m_pLinkList[i].dwId1 = dwId1;
         m_pLinkList[i].dwId2 = dwId2;
         m_pLinkList[i].nType = LINK_TYPE_NORMAL;
      }
   }
}
