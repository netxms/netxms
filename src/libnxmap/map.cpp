/* 
** NetXMS - Network Management System
** Network Maps Library
** Copyright (C) 2006, 2007, 2008 Victor Kirhenshtein
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
// Default submap creation function
//

static nxSubmap *CreateSubmap(DWORD dwObjectId, nxMap *pMap)
{
   return new nxSubmap(dwObjectId);
}


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

nxMap::nxMap(DWORD dwMapId, DWORD dwObjectId, const TCHAR *pszName, const TCHAR *pszDescription)
{
   CommonInit();
   m_dwMapId = dwMapId;
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
   m_dwMapId = 0;
   m_pszName = NULL;
   m_pszDescription = NULL;
   m_dwObjectId = 0;
   m_dwNumSubmaps = 0;
   m_ppSubmaps = NULL;
   m_dwACLSize = 0;
   m_pACL = NULL;
   m_mutex = MutexCreate();
   m_pfCreateSubmap = CreateSubmap;
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
      pSubmap = m_pfCreateSubmap(dwObjectId, this);
      m_dwNumSubmaps++;
      m_ppSubmaps = (nxSubmap **)realloc(m_ppSubmaps, sizeof(nxSubmap *) * m_dwNumSubmaps);
      m_ppSubmaps[i] = pSubmap;
   }

   Unlock();
   return pSubmap;
}


//
// Add new or replace existing submap
//

void nxMap::AddSubmap(nxSubmap *pSubmap)
{
   DWORD i;

   Lock();

   // Check if submap with given ID exist
   for(i = 0; i < m_dwNumSubmaps; i++)
   {
      if (m_ppSubmaps[i]->Id() == pSubmap->Id())
      {
         break;
      }
   }

   if (i == m_dwNumSubmaps)
   {
      // Add new submap
      m_dwNumSubmaps++;
      m_ppSubmaps = (nxSubmap **)realloc(m_ppSubmaps, sizeof(nxSubmap *) * m_dwNumSubmaps);
      m_ppSubmaps[i] = pSubmap;
   }
   else
   {
      // Replace existing submap
      delete m_ppSubmaps[i];
      m_ppSubmaps[i] = pSubmap;
   }

   Unlock();
}


//
// Check if submap for given object exists
//

BOOL nxMap::IsSubmapExist(DWORD dwObjectId, BOOL bLock)
{
   BOOL bRet = FALSE;
   DWORD i;

   if (bLock)
      Lock();

   for(i = 0; i < m_dwNumSubmaps; i++)
   {
      if (m_ppSubmaps[i]->Id() == dwObjectId)
      {
         bRet = TRUE;
         break;
      }
   }

   if (bLock)
      Unlock();
   return bRet;
}


//
// Fill NXCP message with map data
//

void nxMap::CreateMessage(CSCPMessage *pMsg)
{
   DWORD i, j, *pdwACL, *pdwSubmapList;

   pdwACL = (DWORD *)malloc(sizeof(DWORD) * m_dwACLSize * 2);
   for(i = 0, j = 0; i < m_dwACLSize; i++)
   {
      pdwACL[j++] = m_pACL[i].dwUserId;
      pdwACL[j++] = m_pACL[i].dwAccess;
   }

   pdwSubmapList = (DWORD *)malloc(sizeof(DWORD) * m_dwNumSubmaps);
   for(i = 0; i < m_dwNumSubmaps; i++)
      pdwSubmapList[i] = m_ppSubmaps[i]->Id();
   
   pMsg->SetVariable(VID_MAP_ID, m_dwMapId);
   pMsg->SetVariable(VID_NAME, m_pszName);
   pMsg->SetVariable(VID_DESCRIPTION, m_pszDescription);
   pMsg->SetVariable(VID_OBJECT_ID, m_dwObjectId);
   pMsg->SetVariable(VID_NUM_SUBMAPS, m_dwNumSubmaps);
   pMsg->SetVariableToInt32Array(VID_SUBMAP_LIST, m_dwNumSubmaps, pdwSubmapList);
   pMsg->SetVariable(VID_ACL_SIZE, m_dwACLSize);
   pMsg->SetVariableToInt32Array(VID_ACL, m_dwACLSize * 2, pdwACL);

   safe_free(pdwACL);
   safe_free(pdwSubmapList);
}


//
// Modify map object from NXCP message
//

void nxMap::ModifyFromMessage(CSCPMessage *pMsg)
{
   DWORD i, j, dwNumSubmaps, *pdwTemp;

   // Update simple attributes
   m_dwMapId = pMsg->GetVariableLong(VID_MAP_ID);
   m_dwObjectId = pMsg->GetVariableLong(VID_OBJECT_ID);
   safe_free(m_pszName);
   m_pszName = pMsg->GetVariableStr(VID_NAME);
   safe_free(m_pszDescription);
   m_pszDescription = pMsg->GetVariableStr(VID_DESCRIPTION);

   // Delete submaps which are no longer exist
   dwNumSubmaps = pMsg->GetVariableLong(VID_NUM_SUBMAPS);
   pdwTemp = (DWORD *)malloc(sizeof(DWORD) * dwNumSubmaps);
   pMsg->GetVariableInt32Array(VID_SUBMAP_LIST, dwNumSubmaps, pdwTemp);
   for(i = 0; i < m_dwNumSubmaps; i++)
   {
      for(j = 0; j < dwNumSubmaps; j++)
         if (pdwTemp[j] == m_ppSubmaps[i]->Id())
            break;
      if (j == dwNumSubmaps)
      {
         delete m_ppSubmaps[i];
         m_dwNumSubmaps--;
         memmove(&m_ppSubmaps[i], &m_ppSubmaps[i + 1], sizeof(nxSubmap *) * (m_dwNumSubmaps - i));
         i--;
      }
   }
   safe_free(pdwTemp);

   // Update ACL
   m_dwACLSize = pMsg->GetVariableLong(VID_ACL_SIZE);
   pdwTemp = (DWORD *)malloc(sizeof(DWORD) * m_dwACLSize * 2);
   pMsg->GetVariableInt32Array(VID_ACL, m_dwACLSize * 2, pdwTemp);
   safe_free(m_pACL);
   m_pACL = (MAP_ACL_ENTRY *)malloc(sizeof(MAP_ACL_ENTRY) * m_dwACLSize);
   for(i = 0, j = 0; i < m_dwACLSize; i++)
   {
      m_pACL[i].dwUserId = pdwTemp[j++];
      m_pACL[i].dwAccess = pdwTemp[j++];
   }
}
