/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Victor Kirhenshtein
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
UniversalRoot::UniversalRoot() : super()
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
void UniversalRoot::linkObjects()
{
   super::linkObjects();

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   TCHAR szQuery[256];
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT object_id FROM container_members WHERE container_id=%d"), m_id);

   DB_RESULT hResult = DBSelect(hdb, szQuery);
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 dwObjectId = DBGetFieldULong(hResult, i, 0);
         NetObj *pObject = FindObjectById(dwObjectId);
         if (pObject != NULL)
            linkObject(pObject);
         else
            nxlog_write(NXLOG_ERROR, _T("Inconsistent database: %s object %s [%u] has reference to non-existent child object [%u]"),
                     getObjectClassName(), m_name, m_id, dwObjectId);
      }
      DBFreeResult(hResult);
   }

   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Save object to database
 */
bool UniversalRoot::saveToDatabase(DB_HANDLE hdb)
{
   lockProperties();

   bool success = saveCommonProperties(hdb);

   // Save access list
   if (success)
      success = saveACLToDB(hdb);

   unlockProperties();

   // Update members list
   if (success && (m_modified & MODIFY_RELATIONS))
   {
      success = executeQueryOnObject(hdb, _T("DELETE FROM container_members WHERE container_id=?"));
      lockChildList(false);
      if (success && !m_childList->isEmpty())
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO container_members (container_id,object_id) VALUES (?,?)"));
         if (hStmt != NULL)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            for(int i = 0; (i < m_childList->size()) && success; i++)
            {
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_childList->get(i)->getId());
               success = DBExecute(hStmt);
            }
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }
      unlockChildList();
   }

   return success;
}

/**
 * Load properties from database
 */
void UniversalRoot::loadFromDatabase(DB_HANDLE hdb)
{
   loadCommonProperties(hdb);
   loadACLFromDB(hdb);
}
