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
** File: map.cpp
**
**/

#include "libnxmap.h"


//
// Default map object constructor
//

nxMap::nxMap()
{
   CommonInit();
}


//
// Constructor for new map object
//

nxMap::nxMap(DWORD dwObjectId, TCHAR *pszName, TCHAR *pszDescription)
{
   CommonInit();
   m_dwObjectId = dwObjectId;
   m_pszName = _tcsdup(pszName);
   m_pszDescription = _tcsdup(pszDescription);
}


//
// Create map object from NXCP message
//

nxMap::nxMap(CSCPMessage *pMsg)
{
   CommonInit();
   ModifyFromMessage(pMsg);
}


//
// Common initialization code
//

void nxMap::CommonInit(void)
{
   m_pszName = NULL;
   m_pszDescription = NULL;
   m_dwObjectId = 0;
   m_dwNumSubmaps = 0;
   m_ppSubmaps = NULL;
   m_dwACLSize = 0;
   m_pACL = NULL;
   m_mutex = MutexCreate();
}


//
// Destructor
//

nxMap::~nxMap()
{
   DWORD i;

   safe_free(m_pszName);
   safe_free(m_pszDescription);
   for(i = 0; i < m_dwNumSubmaps; i++)
      delete m_ppSubmaps[i];
   safe_free(m_ppSubmaps);
   safe_free(m_pACL);
   MutexDestroy(m_mutex);
}


//
// Get submap by object ID or create if one doesn't exist
//

nxSubmap *nxMap::GetSubmap(DWORD dwObjectId)
{
   DWORD i;
   nxSubmap *pSubmap;

   Lock();

   for(i = 0; i < m_dwNumSubmaps; i++)
   {
      if (m_ppSubmaps[i]->Id() == dwObjectId)
      {
         pSubmap = m_ppSubmaps[i];
         break;
      }
   }

   if (i == m_dwNumSubmaps)
   {
      // Create new submap
      pSubmap = new nxSubmap(dwObjectId);
      m_dwNumSubmaps++;
      m_ppSubmaps = (nxSubmap **)realloc(m_ppSubmaps, sizeof(nxSubmap *) * m_dwNumSubmaps);
      m_ppSubmaps[i] = pSubmap;
   }

   Unlock();
   return pSubmap;
}


//
// Fill NXCP message with map data
//

void nxMap::CreateMessage(CSCPMessage *pMsg)
{
   DWORD i, j, *pdwACL;

   Lock();

   pdwACL = (DWORD *)malloc(sizeof(DWORD) * m_dwACLSize * 2);
   for(i = 0, j = 0; i < m_dwACLSize; i++)
   {
      pdwACL[j++] = m_pACL[i].dwUserId;
      pdwACL[j++] = m_pACL[i].dwAccess;
   }
   
   pMsg->SetVariable(VID_NAME, m_pszName);
   pMsg->SetVariable(VID_DESCRIPTION, m_pszDescription);
   pMsg->SetVariable(VID_OBJECT_ID, m_dwObjectId);
   pMsg->SetVariable(VID_NUM_SUBMAPS, m_dwNumSubmaps);
   pMsg->SetVariable(VID_ACL_SIZE, m_dwACLSize);
   pMsg->SetVariableToInt32Array(VID_ACL, m_dwACLSize * 2, pdwACL);

   Unlock();

   safe_free(pdwACL);
}


//
// Modify map object from NXCP message
//

void nxMap::ModifyFromMessage(CSCPMessage *pMsg)
{
   Lock();

   Unlock();
}
