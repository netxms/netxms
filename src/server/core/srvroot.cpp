/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: srvroot.cpp
**
**/

#include "nms_core.h"


//
// Service root class default constructor
//

ServiceRoot::ServiceRoot()
            :NetObj()
{
   m_dwId = 2;
   strcpy(m_szName, "All Services");
}


//
// Network class destructor
//

ServiceRoot::~ServiceRoot()
{
}


//
// Save object to database
//

BOOL ServiceRoot::SaveToDB(void)
{
   char szQuery[1024];
   DWORD i;

   Lock();

   // Save name
   ConfigWriteStr("ServiceRootObjectName", m_szName, TRUE);

   // Update members list
   sprintf(szQuery, "DELETE FROM container_members WHERE container_id=%d", m_dwId);
   DBQuery(g_hCoreDB, szQuery);
   for(i = 0; i < m_dwChildCount; i++)
   {
      sprintf(szQuery, "INSERT INTO container_members (container_id,object_id) VALUES (%ld,%ld)", m_dwId, m_pChildList[i]->Id());
      DBQuery(g_hCoreDB, szQuery);
   }

   // Save access list
   SaveACLToDB();

   // Unlock object and clear modification flag
   Unlock();
   m_bIsModified = FALSE;
   return TRUE;
}


//
// Load properties from database
//

void ServiceRoot::LoadFromDB(void)
{
   Lock();
   ConfigReadStr("ServiceRootObjectName", m_szName, MAX_OBJECT_NAME, "All Services");
   LoadACLFromDB();
   Unlock();
}


//
// Link child objects
//

void ServiceRoot::LinkChildObjects(void)
{
   DWORD i, dwNumChilds, dwObjectId;
   NetObj *pObject;
   char szQuery[256];
   DB_RESULT hResult;

   Lock();

   // Load child list and link objects
   sprintf(szQuery, "SELECT object_id FROM container_members WHERE container_id=%d", m_dwId);
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
            WriteLog(MSG_SRVROOT_INVALID_CHILD_ID, EVENTLOG_WARNING_TYPE, "d", dwObjectId);
      }
      DBFreeResult(hResult);
   }

   Unlock();
}
