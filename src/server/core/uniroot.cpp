/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2015 Victor Kirhenshtein
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
** File: uniroot.cpp
**
**/

#include "nxcore.h"

/**
 * Constructor
 */
UniversalRoot::UniversalRoot() : NetObj()
{
   m_guid = uuid::generate();
}

/**
 * Destructor
 */
UniversalRoot::~UniversalRoot()
{
}

/**
 * Link child objects
 * This method is expected to be called only at startup, so we don't lock
 */
void UniversalRoot::LinkChildObjects()
{
   UINT32 i, dwNumChilds, dwObjectId;
   NetObj *pObject;
   TCHAR szQuery[256];
   DB_RESULT hResult;

   // Load child list and link objects
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT object_id FROM container_members WHERE container_id=%d"), m_id);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      dwNumChilds = DBGetNumRows(hResult);
      for(i = 0; i < dwNumChilds; i++)
      {
         dwObjectId = DBGetFieldULong(hResult, i, 0);
         pObject = FindObjectById(dwObjectId);
         if (pObject != NULL)
            LinkObject(pObject);
         else
            nxlog_write(MSG_ROOT_INVALID_CHILD_ID, EVENTLOG_WARNING_TYPE, "ds", 
                        dwObjectId, g_szClassName[getObjectClass()]);
      }
      DBFreeResult(hResult);
   }
}

/**
 * Save object to database
 */
BOOL UniversalRoot::saveToDatabase(DB_HANDLE hdb)
{
   TCHAR szQuery[1024];
   UINT32 i;

   lockProperties();

   saveCommonProperties(hdb);

   // Update members list
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM container_members WHERE container_id=%d"), m_id);
   DBQuery(hdb, szQuery);
   LockChildList(FALSE);
   for(i = 0; i < m_dwChildCount; i++)
   {
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("INSERT INTO container_members (container_id,object_id) VALUES (%d,%d)"), m_id, m_pChildList[i]->getId());
      DBQuery(hdb, szQuery);
   }
   UnlockChildList();

   // Save access list
   saveACLToDB(hdb);

   // Unlock object and clear modification flag
   unlockProperties();
   m_isModified = false;
   return TRUE;
}

/**
 * Load properties from database
 */
void UniversalRoot::LoadFromDB()
{
   loadCommonProperties();
   loadACLFromDB();
}
