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
** $module: container.cpp
**
**/

#include "nxcore.h"


//
// Find container category by id
//

CONTAINER_CATEGORY NXCORE_EXPORTABLE *FindContainerCategory(DWORD dwId)
{
   DWORD i;

   for(i = 0; i < g_dwNumCategories; i++)
      if (g_pContainerCatList[i].dwCatId == dwId)
         return &g_pContainerCatList[i];
   return NULL;
}


//
// Default container class constructor
//

Container::Container()
          :NetObj()
{
   m_pszDescription = NULL;
   m_pdwChildIdList = NULL;
   m_dwChildIdListSize = 0;
   m_dwCategory = 1;
}


//
// "Normal" container class constructor
//

Container::Container(char *pszName, DWORD dwCategory, char *pszDescription)
          :NetObj()
{
   nx_strncpy(m_szName, pszName, MAX_OBJECT_NAME);
   m_pszDescription = strdup(pszDescription);
   m_pdwChildIdList = NULL;
   m_dwChildIdListSize = 0;
   m_dwCategory = dwCategory;
   m_bIsHidden = TRUE;
}


//
// Container class destructor
//

Container::~Container()
{
   safe_free(m_pszDescription);
   safe_free(m_pdwChildIdList);
}


//
// Create object from database data
//

BOOL Container::CreateFromDB(DWORD dwId)
{
   char szQuery[256];
   DB_RESULT hResult;
   DWORD i;

   m_dwId = dwId;

   if (!LoadCommonProperties())
      return FALSE;

   sprintf(szQuery, "SELECT category,description FROM containers WHERE id=%d", dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult == NULL)
      return FALSE;     // Query failed

   if (DBGetNumRows(hResult) == 0)
   {
      // No object with given ID in database
      DBFreeResult(hResult);
      return FALSE;
   }

   m_dwCategory = DBGetFieldULong(hResult, 0, 0);
   m_pszDescription = strdup(CHECK_NULL(DBGetField(hResult, 0, 1)));

   DBFreeResult(hResult);

   // Load access list
   LoadACLFromDB();

   // Load child list for later linkage
   if (!m_bIsDeleted)
   {
      sprintf(szQuery, "SELECT object_id FROM container_members WHERE container_id=%d", m_dwId);
      hResult = DBSelect(g_hCoreDB, szQuery);
      if (hResult != NULL)
      {
         m_dwChildIdListSize = DBGetNumRows(hResult);
         if (m_dwChildIdListSize > 0)
         {
            m_pdwChildIdList = (DWORD *)malloc(sizeof(DWORD) * m_dwChildIdListSize);
            for(i = 0; i < m_dwChildIdListSize; i++)
               m_pdwChildIdList[i] = DBGetFieldULong(hResult, i, 0);
         }
         DBFreeResult(hResult);
      }
   }

   return TRUE;
}


//
// Save object to database
//

BOOL Container::SaveToDB(DB_HANDLE hdb)
{
   char szQuery[1024];
   DB_RESULT hResult;
   DWORD i;
   BOOL bNewObject = TRUE;

   // Lock object's access
   LockData();

   SaveCommonProperties(hdb);

   // Check for object's existence in database
   sprintf(szQuery, "SELECT id FROM containers WHERE id=%d", m_dwId);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         bNewObject = FALSE;
      DBFreeResult(hResult);
   }

   // Form and execute INSERT or UPDATE query
   if (bNewObject)
      sprintf(szQuery, "INSERT INTO containers (id,category,"
                       "description,object_class) VALUES (%d,%d,'%s',%d)",
              m_dwId, m_dwCategory, CHECK_NULL(m_pszDescription), Type());
   else
      sprintf(szQuery, "UPDATE containers SET category=%d,"
                       "description='%s',object_class=%d WHERE id=%d",
              m_dwCategory, CHECK_NULL(m_pszDescription), Type(), m_dwId);
   DBQuery(hdb, szQuery);

   // Update members list
   sprintf(szQuery, "DELETE FROM container_members WHERE container_id=%d", m_dwId);
   DBQuery(hdb, szQuery);
   LockChildList(FALSE);
   for(i = 0; i < m_dwChildCount; i++)
   {
      sprintf(szQuery, "INSERT INTO container_members (container_id,object_id) VALUES (%d,%d)", m_dwId, m_pChildList[i]->Id());
      DBQuery(hdb, szQuery);
   }
   UnlockChildList();

   // Save access list
   SaveACLToDB(hdb);

   // Clear modifications flag and unlock object
   m_bIsModified = FALSE;
   UnlockData();

   return TRUE;
}


//
// Delete object from database
//

BOOL Container::DeleteFromDB(void)
{
   char szQuery[256];
   BOOL bSuccess;

   bSuccess = NetObj::DeleteFromDB();
   if (bSuccess)
   {
      sprintf(szQuery, "DELETE FROM containers WHERE id=%d", m_dwId);
      QueueSQLRequest(szQuery);
      sprintf(szQuery, "DELETE FROM container_members WHERE container_id=%d", m_dwId);
      QueueSQLRequest(szQuery);
   }
   return bSuccess;
}


//
// Link child objects after loading from database
// This method is expected to be called only at startup, so we don't lock
//

void Container::LinkChildObjects(void)
{
   NetObj *pObject;
   DWORD i;

   if (m_dwChildIdListSize > 0)
   {
      // Find and link child objects
      for(i = 0; i < m_dwChildIdListSize; i++)
      {
         pObject = FindObjectById(m_pdwChildIdList[i]);
         if (pObject != NULL)
            LinkObject(pObject);
         else
            WriteLog(MSG_INVALID_CONTAINER_MEMBER, EVENTLOG_ERROR_TYPE, "dd", m_dwId, m_pdwChildIdList[i]);
      }

      // Cleanup
      free(m_pdwChildIdList);
      m_pdwChildIdList = NULL;
      m_dwChildIdListSize = 0;
   }
}


//
// Create CSCP message with object's data
//

void Container::CreateMessage(CSCPMessage *pMsg)
{
   NetObj::CreateMessage(pMsg);
   pMsg->SetVariable(VID_CATEGORY, m_dwCategory);
   pMsg->SetVariable(VID_DESCRIPTION, CHECK_NULL(m_pszDescription));
}
