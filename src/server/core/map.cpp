/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006 Victor Kirhenshtein
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

#include "nxcore.h"


//
// Static data
//

static DWORD m_dwNumMaps = 0;
static nxMapSrv **m_ppMapList = NULL;
static MUTEX m_mutexMapList = INVALID_MUTEX_HANDLE;


//
// Submap creation function for ...Srv classes
//

static nxSubmap *CreateSubmapSrv(DWORD dwObjectId, nxMap *pMap)
{
   return new nxSubmapSrv(dwObjectId, pMap->MapId());
}


//
// Constructor for creating submap object from database
// Expected field order:
//     submap_id,attributes
//

nxSubmapSrv::nxSubmapSrv(DB_RESULT hData, int nRow, DWORD dwMapId)
{
   DB_RESULT hResult;
   DWORD i;
   TCHAR szQuery[1024];

   CommonInit();
   m_dwMapId = dwMapId;
   m_dwId = DBGetFieldULong(hData, nRow, 0);
   m_dwAttr = DBGetFieldULong(hData, nRow, 1);
   
   // Load object positions
   _sntprintf(szQuery, 1024, _T("SELECT object_id,x,y FROM submap_object_positions WHERE map_id=%d AND submap_id=%d"),
              m_dwMapId, m_dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      m_dwNumObjects = DBGetNumRows(hResult);
      if (m_dwNumObjects > 0)
      {
         m_pObjectList = (MAP_OBJECT *)malloc(sizeof(MAP_OBJECT) * m_dwNumObjects);
         for(i = 0; i < m_dwNumObjects; i++)
         {
            m_pObjectList[i].dwId = DBGetFieldULong(hResult, i, 0);
            m_pObjectList[i].x = DBGetFieldLong(hResult, i, 1);
            m_pObjectList[i].y = DBGetFieldLong(hResult, i, 2);
            m_pObjectList[i].dwState = 0;
         }
      }
      DBFreeResult(hResult);
   }

   // Load links between objects
   _sntprintf(szQuery, 1024, _T("SELECT object_id1,object_id2,link_type FROM submap_links WHERE map_id=%d AND submap_id=%d"),
              m_dwMapId, m_dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      m_dwNumLinks = DBGetNumRows(hResult);
      if (m_dwNumLinks > 0)
      {
         m_pLinkList = (OBJLINK *)malloc(sizeof(OBJLINK) * m_dwNumLinks);
         for(i = 0; i < m_dwNumLinks; i++)
         {
            m_pLinkList[i].dwId1 = DBGetFieldULong(hResult, i, 0);
            m_pLinkList[i].dwId2 = DBGetFieldULong(hResult, i, 1);
            m_pLinkList[i].nType = DBGetFieldLong(hResult, i, 2);
         }
      }
      DBFreeResult(hResult);
   }
}


//
// Constructor for new empty nxSubmapSrv object
//

nxSubmapSrv::nxSubmapSrv(DWORD dwObjectId, DWORD dwMapId)
            :nxSubmap(dwObjectId)
{
   m_dwMapId = dwMapId;
}


//
// Constructor for creaing map object from database
// Expected field order:
//     map_id,map_name,description,root_object_id
//

nxMapSrv::nxMapSrv(DB_RESULT hData, int nRow)
{
   DB_RESULT hResult;
   DWORD i;
   TCHAR szQuery[1024];

   CommonInit();
   m_dwMapId = DBGetFieldULong(hData, nRow, 0);
   m_pszName = _tcsdup(DBGetField(hData, nRow, 1));
   m_pszDescription = _tcsdup(DBGetField(hData, nRow, 2));
   m_dwObjectId = DBGetFieldULong(hData, nRow, 3);

   // Load ACL
   _sntprintf(szQuery, 1024, _T("SELECT user_id,access_rights FROM map_access_lists WHERE map_id=%d"), m_dwMapId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      m_dwACLSize = DBGetNumRows(hResult);
      if (m_dwACLSize > 0)
      {
         m_pACL = (MAP_ACL_ENTRY *)malloc(sizeof(MAP_ACL_ENTRY) * m_dwACLSize);
         for(i = 0; i < m_dwACLSize; i++)
         {
            m_pACL[i].dwUserId = DBGetFieldULong(hResult, i, 0);
            m_pACL[i].dwAccess = DBGetFieldULong(hResult, i, 1);
         }
      }
      DBFreeResult(hResult);
   }

   // Load submaps
   _sntprintf(szQuery, 1024, _T("SELECT submap_id,attributes FROM submaps WHERE map_id=%d"), m_dwMapId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      m_dwNumSubmaps = DBGetNumRows(hResult);
      if (m_dwNumSubmaps > 0)
      {
         m_ppSubmaps = (nxSubmap **)malloc(sizeof(nxSubmap *) * m_dwNumSubmaps);
         for(i = 0; i < m_dwNumSubmaps; i++)
            m_ppSubmaps[i] = new nxSubmapSrv(hResult, i, m_dwMapId);
      }
      DBFreeResult(hResult);
   }
}


//
// Common initialization for nxMapSrv constructors
//

void nxMapSrv::CommonInit(void)
{
   nxMap::CommonInit();
   m_nRefCount = 0;
   m_pfCreateSubmap = CreateSubmapSrv;
}


//
// Check user's access level
//

BOOL nxMapSrv::CheckUserRights(DWORD dwUserId, DWORD dwDesiredAccess)
{
   DWORD i, dwRights;
   BOOL bRet = FALSE, bFound = FALSE;

   if (dwUserId == 0)
      return TRUE;   // Superuser always has all rights

   Lock();

   // Check direct user assignment first
   for(i = 0; i < m_dwACLSize; i++)
   {
      if (m_pACL[i].dwUserId == dwUserId)
      {
         bRet = ((m_pACL[i].dwAccess & dwDesiredAccess) == dwDesiredAccess)? TRUE : FALSE;
         bFound = TRUE;
         break;
      }
   }

   // Check rights assignment through the group
   if (!bFound)
   {
      dwRights = 0;
      for(i = 0; i < m_dwACLSize; i++)
      {
         if (m_pACL[i].dwUserId & GROUP_FLAG)
         {
            if (CheckUserMembership(dwUserId, m_pACL[i].dwUserId))
            {
               dwRights |= m_pACL[i].dwAccess;
            }
         }
      }
      bRet = ((dwRights & dwDesiredAccess) == dwDesiredAccess)? TRUE : FALSE;
   }

   Unlock();
   return bRet;
}


//
// Load all maps on startup
//

void LoadMaps(void)
{
   DB_RESULT hResult;
   DWORD i;

   hResult = DBSelect(g_hCoreDB, "SELECT map_id,map_name,description,root_object_id FROM maps");
   if (hResult != NULL)
   {
      m_dwNumMaps = DBGetNumRows(hResult);
      if (m_dwNumMaps > 0)
      {
         m_ppMapList = (nxMapSrv **)malloc(sizeof(nxMapSrv *) * m_dwNumMaps);
         for(i = 0; i < m_dwNumMaps; i++)
            m_ppMapList[i] = new nxMapSrv(hResult, i);
      }
      DBFreeResult(hResult);
   }
   m_mutexMapList = MutexCreate();
}


//
// Resolve map name to ID
// Will return RCC_SUCCESS on success or appropriate RCC on failure
//

DWORD GetMapIdFromName(TCHAR *pszName, DWORD *pdwMapId)
{
   DWORD i, dwResult = RCC_INTERNAL_ERROR;

   if (MutexLock(m_mutexMapList, g_dwLockTimeout))
   {
      for(i = 0; i < m_dwNumMaps; i++)
      {
         if (!_tcsicmp(pszName, m_ppMapList[i]->Name()))
         {
            *pdwMapId = m_ppMapList[i]->MapId();
            dwResult = RCC_SUCCESS;
            break;
         }

         // Map with given name not found
         *pdwMapId = 0;
         dwResult = RCC_UNKNOWN_MAP_NAME;
      }
      MutexUnlock(m_mutexMapList);
   }
   return dwResult;
}


//
// Lock access to maps
//

BOOL LockMaps(void)
{
   return MutexLock(m_mutexMapList, g_dwLockTimeout);
}


//
// Unlock access to maps
//

void UnlockMaps(void)
{
   MutexUnlock(m_mutexMapList);
}


//
// Find map by ID (assuming that map list already locked)
//

nxMapSrv *FindMapByID(DWORD dwMapId)
{
   DWORD i;

   for(i = 0; i < m_dwNumMaps; i++)
      if (m_ppMapList[i]->MapId() == dwMapId)
         return m_ppMapList[i];
   return NULL;
}

