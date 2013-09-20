/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2010 Victor Kirhenshtein
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
** File: bizsvcroot.cpp
**
**/

#include "nxcore.h"


//
// Business service tree root class default constructor
//

BusinessServiceRoot::BusinessServiceRoot() : ServiceContainer()
{
	m_dwId = BUILTIN_OID_BUSINESSSERVICEROOT;
	_tcscpy(m_szName, _T("Business Services"));
	uuid_generate(m_guid);
	m_iStatus = STATUS_NORMAL;
}


//
// Business service root class destructor
//

BusinessServiceRoot::~BusinessServiceRoot()
{
}


//
// Save object to database
//

BOOL BusinessServiceRoot::SaveToDB(DB_HANDLE hdb)
{
   TCHAR szQuery[1024];
   UINT32 i;

   LockData();

   saveCommonProperties(hdb);

   // Update members list
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM container_members WHERE container_id=%d"), m_dwId);
   DBQuery(hdb, szQuery);
   LockChildList(FALSE);
   for(i = 0; i < m_dwChildCount; i++)
   {
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("INSERT INTO container_members (container_id,object_id) VALUES (%d,%d)"), m_dwId, m_pChildList[i]->Id());
      DBQuery(hdb, szQuery);
   }
   UnlockChildList();

   // Save access list
   saveACLToDB(hdb);

   // Unlock object and clear modification flag
   UnlockData();
   m_isModified = false;
   return TRUE;
}

/**
 * Load properties from database
 */
void BusinessServiceRoot::LoadFromDB()
{
   loadCommonProperties();
   loadACLFromDB();
	initUptimeStats();
}


//
// Link child objects
// This method is expected to be called only at startup, so we don't lock
//

void BusinessServiceRoot::LinkChildObjects()
{
   UINT32 i, dwNumChilds, dwObjectId;
   NetObj *pObject;
   TCHAR szQuery[256];
   DB_RESULT hResult;

   // Load child list and link objects
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT object_id FROM container_members WHERE container_id=%d"), m_dwId);
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
                        dwObjectId, g_szClassName[Type()]);
      }
      DBFreeResult(hResult);
   }
}

