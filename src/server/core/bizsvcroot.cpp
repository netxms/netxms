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
** File: bizsvcroot.cpp
**
**/

#include "nxcore.h"

/**
 * Business service tree root class default constructor
 */
BusinessServiceRoot::BusinessServiceRoot() : super()
{
	m_id = BUILTIN_OID_BUSINESSSERVICEROOT;
	_tcscpy(m_name, _T("Business Services"));
   m_guid = uuid::generate();
	m_status = STATUS_NORMAL;
}

/**
 * Business service root class destructor
 */
BusinessServiceRoot::~BusinessServiceRoot()
{
}

/**
 * Save object to database
 */
bool BusinessServiceRoot::saveToDatabase(DB_HANDLE hdb)
{
   lockProperties();

   bool success = saveCommonProperties(hdb);
   if (success)
   {
      // Update members list
      success = executeQueryOnObject(hdb, _T("DELETE FROM container_members WHERE container_id=?"));
      lockChildList(false);
      if (success && !m_childList->isEmpty())
      {
         TCHAR szQuery[1024];
         for(int i = 0; (i < m_childList->size()) && success; i++)
         {
            _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("INSERT INTO container_members (container_id,object_id) VALUES (%d,%d)"), m_id, m_childList->get(i)->getId());
            success = DBQuery(hdb, szQuery);
         }
      }
      unlockChildList();
   }

   if (success)
      success = saveACLToDB(hdb);

   unlockProperties();
   return success;
}

/**
 * Load properties from database
 */
void BusinessServiceRoot::loadFromDatabase(DB_HANDLE hdb)
{
   loadCommonProperties(hdb);
   loadACLFromDB(hdb);
	initUptimeStats();
}

/**
 * Link child objects
 * This method is expected to be called only at startup, so we don't lock
 */
void BusinessServiceRoot::linkObjects()
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
            nxlog_write(NXLOG_WARNING, _T("Inconsistent database: %s object %s [%u] has reference to non-existent child object [%u]"),
                     getObjectClassName(), m_name, m_id, dwObjectId);
      }
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);
}
