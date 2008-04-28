/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Victor Kirhenshtein
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
// Constructor for new empty nxSubmapSrv object
//

nxSubmapSrv::nxSubmapSrv(DWORD dwObjectId, DWORD dwMapId)
            :nxSubmap(dwObjectId)
{
   m_dwMapId = dwMapId;
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
   _sntprintf(szQuery, 1024, _T("SELECT object_id1,object_id2,link_type,port1,port2 FROM submap_links WHERE map_id=%d AND submap_id=%d"),
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
				DBGetField(hResult, i, 3, m_pLinkList[i].szPort1, MAX_CONNECTOR_NAME);
				DecodeSQLString(m_pLinkList[i].szPort1);
				DBGetField(hResult, i, 4, m_pLinkList[i].szPort2, MAX_CONNECTOR_NAME);
				DecodeSQLString(m_pLinkList[i].szPort2);
         }
      }
      DBFreeResult(hResult);
   }
}


//
// Save submap to database
// Will return appropriate RCC, ready for sending to client
// Intended to be called only from nxMapSrv::SaveToDB()
//

DWORD nxSubmapSrv::SaveToDB(void)
{
   TCHAR szQuery[2048], *pszEscPort1, *pszEscPort2;
   DB_RESULT hResult;
   BOOL bExist;
   DWORD i, dwResult = RCC_DB_FAILURE;

   // Check if submap record exist in database
   _sntprintf(szQuery, 256, _T("SELECT submap_id FROM submaps WHERE map_id=%d AND submap_id=%d"),
              m_dwMapId, m_dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult == NULL)
      goto exit_save;

   bExist = (DBGetNumRows(hResult) > 0);
   DBFreeResult(hResult);

   if (bExist)
   {
      _sntprintf(szQuery, 256, _T("UPDATE submaps SET attributes=%d WHERE map_id=%d AND submap_id=%d"),
                 m_dwAttr, m_dwMapId, m_dwId);
   }
   else
   {
      _sntprintf(szQuery, 256, _T("INSERT INTO submaps (map_id,submap_id,attributes) VALUES (%d,%d,%d)"),
                 m_dwMapId, m_dwId, m_dwAttr);
   }
   if (!DBQuery(g_hCoreDB, szQuery))
      goto exit_save;

   // Save object positions
   // INSERT is used because old records was deleted by nxMapSrv::SaveToDB()
   for(i = 0; i < m_dwNumObjects; i++)
   {
      _sntprintf(szQuery, 256, _T("INSERT INTO submap_object_positions (map_id,submap_id,object_id,x,y) VALUES (%d,%d,%d,%d,%d)"),
                 m_dwMapId, m_dwId, m_pObjectList[i].dwId, m_pObjectList[i].x,
                 m_pObjectList[i].y);
      if (!DBQuery(g_hCoreDB, szQuery))
         goto exit_save;
   }

   // Save links between objects
   // INSERT is used because old records was deleted by nxMapSrv::SaveToDB()
   for(i = 0; i < m_dwNumLinks; i++)
   {
		pszEscPort1 = EncodeSQLString(m_pLinkList[i].szPort1);
		pszEscPort2 = EncodeSQLString(m_pLinkList[i].szPort2);
      _sntprintf(szQuery, 2048, _T("INSERT INTO submap_links (map_id,submap_id,object_id1,object_id2,link_type,port1,port2) VALUES (%d,%d,%d,%d,%d,'%s','%s')"),
                 m_dwMapId, m_dwId, m_pLinkList[i].dwId1, m_pLinkList[i].dwId2,
                 m_pLinkList[i].nType, pszEscPort1, pszEscPort2);
		free(pszEscPort1);
		free(pszEscPort2);
      if (!DBQuery(g_hCoreDB, szQuery))
         goto exit_save;
   }

   dwResult = RCC_SUCCESS;

exit_save:
   return dwResult;
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
   m_pszName = DBGetField(hData, nRow, 1, NULL, 0);
   DecodeSQLString(m_pszName);
   m_pszDescription = DBGetField(hData, nRow, 2, NULL, 0);
   DecodeSQLString(m_pszDescription);
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
// Save map to database
// Will return appropriate RCC, ready for sending to client
//

DWORD nxMapSrv::SaveToDB(void)
{
   DWORD i, dwNumRows, dwId, dwResult = RCC_DB_FAILURE;
   DB_RESULT hResult;
   TCHAR szQuery[1024], *pszEscName, *pszEscDescr;
   BOOL bExist;

   Lock();
	DBBegin(g_hCoreDB);

   // Check if map record exist in database
   _sntprintf(szQuery, 256, _T("SELECT map_id FROM maps WHERE map_id=%d"), m_dwMapId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult == NULL)
      goto exit_save;

   bExist = (DBGetNumRows(hResult) > 0);
   DBFreeResult(hResult);

   pszEscName = EncodeSQLString(m_pszName);
   pszEscDescr = EncodeSQLString(m_pszDescription);
   if (bExist)
   {
      _sntprintf(szQuery, 1024, _T("UPDATE maps SET map_name='%s',description='%s',root_object_id=%d WHERE map_id=%d"),
                 pszEscName, pszEscDescr, m_dwObjectId, m_dwMapId);
   }
   else
   {
      _sntprintf(szQuery, 1024, _T("INSERT INTO maps (map_id,map_name,description,root_object_id) VALUES (%d,'%s','%s',%d)"),
                 m_dwMapId, pszEscName, pszEscDescr, m_dwObjectId);
   }
   free(pszEscName);
   free(pszEscDescr);
   if (!DBQuery(g_hCoreDB, szQuery))
      goto exit_save;

   // Save ACL
   _sntprintf(szQuery, 256, _T("DELETE FROM map_access_lists WHERE map_id=%d"), m_dwMapId);
   if (!DBQuery(g_hCoreDB, szQuery))
      goto exit_save;

   for(i = 0; i < m_dwACLSize; i++)
   {
      _sntprintf(szQuery, 1024, _T("INSERT INTO map_access_lists (map_id,user_id,access_rights) VALUES (%d,%d,%d)"),
                 m_dwMapId, m_pACL[i].dwUserId, m_pACL[i].dwAccess);
      if (!DBQuery(g_hCoreDB, szQuery))
         goto exit_save;
   }

   // Delete non-existing submaps
   _sntprintf(szQuery, 1024, _T("SELECT submap_id FROM submaps WHERE map_id=%d"), m_dwMapId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult == NULL)
      goto exit_save;
   dwNumRows = DBGetNumRows(hResult);
   for(i = 0; i < dwNumRows; i++)
   {
      dwId = DBGetFieldULong(hResult, i, 0);
      if (!IsSubmapExist(dwId, FALSE))
      {
         _sntprintf(szQuery, 1024, _T("DELETE FROM submaps WHERE map_id=%d AND submap_id=%d"),
                    m_dwMapId, dwId);
         DBQuery(g_hCoreDB, szQuery);
      }
   }
   DBFreeResult(hResult);

   // Save submaps
   _sntprintf(szQuery, 1024, _T("DELETE FROM submap_object_positions WHERE map_id=%d"), m_dwMapId);
   DBQuery(g_hCoreDB, szQuery);

   _sntprintf(szQuery, 1024, _T("DELETE FROM submap_links WHERE map_id=%d"), m_dwMapId);
   DBQuery(g_hCoreDB, szQuery);

   dwResult = RCC_SUCCESS;
   for(i = 0; (i < m_dwNumSubmaps) && (dwResult == RCC_SUCCESS); i++)
      dwResult = ((nxSubmapSrv *)m_ppSubmaps[i])->SaveToDB();

exit_save:
   Unlock();

	if (dwResult == RCC_SUCCESS)
		DBCommit(g_hCoreDB);
	else
		DBRollback(g_hCoreDB);

   return dwResult;
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


//
// Create NXCP message with map list
//

void CreateMapListMessage(CSCPMessage &msg, DWORD dwUserId)
{
	DWORD i, id, count;

	if (!LockMaps())
	{
		msg.SetVariable(VID_RCC, RCC_INTERNAL_ERROR);
		return;
	}

   for(i = 0, id = VID_MAP_LIST_BASE, count = 0; i < m_dwNumMaps; i++)
	{
		if (m_ppMapList[i]->CheckUserRights(dwUserId, MAP_ACCESS_READ))
		{
			msg.SetVariable(id++, m_ppMapList[i]->MapId());
			msg.SetVariable(id++, m_ppMapList[i]->ObjectId());
			msg.SetVariable(id++, (DWORD)MAP_ACCESS_READ);
			msg.SetVariable(id++, m_ppMapList[i]->Name());
         id += 6;  // Reserved ids for future use
			count++;
		}
	}

	UnlockMaps();
	msg.SetVariable(VID_NUM_MAPS, count);
	msg.SetVariable(VID_RCC, RCC_SUCCESS);
}


//
// Create new map
//

DWORD CreateNewMap(DWORD rootObj, const TCHAR *name, DWORD *newId)
{
	DWORD id, rcc;
	nxMapSrv *map;

	id = CreateUniqueId(IDG_MAP);

	map = new nxMapSrv(id, rootObj, name, _T(""));
	rcc = map->SaveToDB();
	if (rcc == RCC_SUCCESS)
	{
		LockMaps();
		m_dwNumMaps++;
		m_ppMapList = (nxMapSrv **)realloc(m_ppMapList, sizeof(nxMapSrv *) * m_dwNumMaps);
		m_ppMapList[m_dwNumMaps - 1] = map;
		UnlockMaps();
		*newId = id;
	}
	else
	{
		delete map;
	}
	return rcc;
}
