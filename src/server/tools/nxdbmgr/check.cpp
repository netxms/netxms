/* 
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004 Victor Kirhenshtein
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
** $module: check.cpp
**
**/

#include "nxdbmgr.h"


//
// Static data
//

static int m_iNumErrors = 0;
static int m_iNumFixes = 0;


//
// Check node objects
//

static void CheckNodes(void)
{
   DB_RESULT hResult, hResult2;
   DWORD i, dwNumObjects, dwId;
   TCHAR szQuery[256], szName[MAX_OBJECT_NAME];
   BOOL bResult, bIsDeleted;

   _tprintf(_T("Checking node objects...\n"));
   hResult = SQLSelect(_T("SELECT id,primary_ip FROM nodes"));
   if (hResult != NULL)
   {
      dwNumObjects = DBGetNumRows(hResult);
      for(i = 0; i < dwNumObjects; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);

         // Check appropriate record in object_properties table
         _sntprintf(szQuery, 256, _T("SELECT name,is_deleted FROM object_properties WHERE object_id=%ld"), dwId);
         hResult2 = SQLSelect(szQuery);
         if (hResult2 != NULL)
         {
            if (DBGetNumRows(hResult2) == 0)
            {
               m_iNumErrors++;
               _tprintf(_T("Missing node object %ld properties. Create? (Y/N) "), dwId);
               if (GetYesNo())
               {
                  _sntprintf(szQuery, 256, 
                             _T("INSERT INTO object_properties (object_id,name,"
                                "status,is_deleted,image_id,inherit_access_rights,"
                                "last_access) VALUES(%ld,'lost_node_%ld',5,0,0,1,0)"), dwId, dwId);
                  if (SQLQuery(szQuery))
                     m_iNumFixes++;
               }
            }
            else
            {
               _tcsncpy(szName, DBGetField(hResult2, 0, 0), MAX_OBJECT_NAME);
               bIsDeleted = DBGetFieldLong(hResult2, 0, 1) ? TRUE : FALSE;
            }
            DBFreeResult(hResult2);
         }

         if (!bIsDeleted)
         {
            _sntprintf(szQuery, 256, _T("SELECT subnet_id FROM nsmap WHERE node_id=%ld"), dwId);
            hResult2 = SQLSelect(szQuery);
            if (hResult2 != NULL)
            {
               if ((DBGetNumRows(hResult2) == 0) && (DBGetFieldIPAddr(hResult, i, 1) != 0))
               {
                  m_iNumErrors++;
                  _tprintf(_T("Unlinked node object %ld (\"%s\"). Delete? (Y/N) "),
                           dwId, szName);
                  if (GetYesNo())
                  {
                     _sntprintf(szQuery, 256, _T("DELETE FROM nodes WHERE id=%ld"), dwId);
                     bResult = SQLQuery(szQuery);
                     _sntprintf(szQuery, 256, _T("DELETE FROM acl WHERE object_id=%ld"), dwId);
                     bResult = bResult && SQLQuery(szQuery);
                     _sntprintf(szQuery, 256, _T("DELETE FROM object_properties WHERE object_id=%ld"), dwId);
                     if (SQLQuery(szQuery) && bResult)
                        m_iNumFixes++;
                  }
               }
               DBFreeResult(hResult2);
            }
         }
      }
      DBFreeResult(hResult);
   }
}


//
// Check interface objects
//

static void CheckInterfaces(void)
{
   DB_RESULT hResult, hResult2;
   DWORD i, dwNumObjects, dwId;
   TCHAR szQuery[256], szName[MAX_OBJECT_NAME];
   BOOL bIsDeleted;

   _tprintf(_T("Checking interface objects...\n"));
   hResult = SQLSelect(_T("SELECT id,node_id FROM interfaces"));
   if (hResult != NULL)
   {
      dwNumObjects = DBGetNumRows(hResult);
      for(i = 0; i < dwNumObjects; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);

         // Check appropriate record in object_properties table
         _sntprintf(szQuery, 256, _T("SELECT name,is_deleted FROM object_properties WHERE object_id=%ld"), dwId);
         hResult2 = SQLSelect(szQuery);
         if (hResult2 != NULL)
         {
            if (DBGetNumRows(hResult2) == 0)
            {
               m_iNumErrors++;
               _tprintf(_T("Missing interface object %ld properties. Create? (Y/N) "), dwId);
               if (GetYesNo())
               {
                  _sntprintf(szQuery, 256, 
                             _T("INSERT INTO object_properties (object_id,name,"
                                "status,is_deleted,image_id,inherit_access_rights,"
                                "last_access) VALUES(%ld,'lost_node_%ld',5,0,0,1,0)"), dwId, dwId);
                  if (SQLQuery(szQuery))
                     m_iNumFixes++;
               }
            }
            else
            {
               _tcsncpy(szName, DBGetField(hResult2, 0, 0), MAX_OBJECT_NAME);
               bIsDeleted = DBGetFieldLong(hResult2, 0, 1) ? TRUE : FALSE;
            }
            DBFreeResult(hResult2);
         }

         // Check if referred node exists
         _sntprintf(szQuery, 256, _T("SELECT name FROM object_properties WHERE object_id=%ld AND is_deleted=0"),
                    DBGetFieldULong(hResult, i, 1));
         hResult2 = SQLSelect(szQuery);
         if (hResult2 != NULL)
         {
            if (DBGetNumRows(hResult2) == 0)
            {
               m_iNumErrors++;
               dwId = DBGetFieldULong(hResult, i, 0);
               _tprintf(_T("Unlinked interface object %ld (\"%s\"). Delete? (Y/N) "),
                        dwId, DBGetField(hResult, i, 1));
               if (GetYesNo())
               {
                  _sntprintf(szQuery, 256, _T("DELETE FROM interfaces WHERE id=%ld"), dwId);
                  if (SQLQuery(szQuery))
                     m_iNumFixes++;
               }
            }
            DBFreeResult(hResult2);
         }
      }
      DBFreeResult(hResult);
   }
}


//
// Check common object properties
//

static void CheckObjectProperties(void)
{
   DB_RESULT hResult;
   TCHAR szQuery[1024];
   DWORD i, dwNumRows, dwObjectId;

   _tprintf(_T("Checking object properties...\n"));
   hResult = SQLSelect(_T("SELECT object_id,name,last_modified FROM object_properties"));
   if (hResult != NULL)
   {
      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwObjectId = DBGetFieldULong(hResult, i, 0);

         // Check last change time
         if (DBGetFieldULong(hResult, i, 2) == 0)
         {
            m_iNumErrors++;
            _tprintf(_T("Object %ld [%s] has invalid timestamp. Correct? (Y/N) "),
                     dwObjectId, DBGetField(hResult, i, 1));
            if (GetYesNo())
            {
               _sntprintf(szQuery, 1024, _T("UPDATE object_properties SET last_modified=%ld WHERE object_id=%ld"),
                          time(NULL), dwObjectId);
               if (SQLQuery(szQuery))
                  m_iNumFixes++;
            }
         }
      }
      DBFreeResult(hResult);
   }
}


//
// Check database for errors
//

void CheckDatabase(BOOL bForce)
{
   DB_RESULT hResult;
   long iVersion = 0;
   BOOL bCompleted = FALSE;

   _tprintf(_T("Checking database...\n"));

   // Get database format version
   hResult = DBSelect(g_hCoreDB, _T("SELECT var_value FROM config WHERE var_name='DBFormatVersion'"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         iVersion = DBGetFieldLong(hResult, 0, 0);
      DBFreeResult(hResult);
   }
   if (iVersion < DB_FORMAT_VERSION)
   {
      _tprintf(_T("Your database has format version %d, this tool is compiled for version %d.\nUse \"upgrade\" command to upgrade your database first.\n"),
               iVersion, DB_FORMAT_VERSION);
   }
   else if (iVersion > DB_FORMAT_VERSION)
   {
       _tprintf(_T("Your database has format version %d, this tool is compiled for version %d.\n"
                   "You need to upgrade your server before using this database.\n"),
                iVersion, DB_FORMAT_VERSION);

   }
   else
   {
      TCHAR szLockStatus[MAX_DB_STRING], szLockInfo[MAX_DB_STRING];
      BOOL bLocked;

      // Check if database is locked
      hResult = DBSelect(g_hCoreDB, _T("SELECT var_value FROM config WHERE var_name='DBLockStatus'"));
      if (hResult != NULL)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            _tcsncpy(szLockStatus, DBGetField(hResult, 0, 0), MAX_DB_STRING);
            bLocked = _tcscmp(szLockStatus, _T("UNLOCKED"));
         }
         DBFreeResult(hResult);

         if (bLocked)
         {
            hResult = DBSelect(g_hCoreDB, _T("SELECT var_value FROM config WHERE var_name='DBLockInfo'"));
            if (hResult != NULL)
            {
               if (DBGetNumRows(hResult) > 0)
               {
                  _tcsncpy(szLockInfo, DBGetField(hResult, 0, 0), MAX_DB_STRING);
               }
               DBFreeResult(hResult);
            }
         }
      }

      if (bLocked)
      {
         _tprintf(_T("Database is locked by server %s [%s]\n"
                     "Do you wish to force database unlock? (Y/N) "),
                  szLockStatus, szLockInfo);
         if (GetYesNo())
         {
            if (SQLQuery(_T("UPDATE config SET var_value='UNLOCKED' where var_name='DBLockStatus'")))
            {
               bLocked = FALSE;
               _tprintf(_T("Database lock removed\n"));
            }
         }
      }

      if (!bLocked)
      {
         CheckNodes();
         CheckInterfaces();
         CheckObjectProperties();

         if (m_iNumErrors == 0)
         {
            _tprintf(_T("Database doesn't contain any errors\n"));
         }
         else
         {
            _tprintf(_T("%d errors was found, %d errors was corrected\n"), m_iNumErrors, m_iNumFixes);
            if (m_iNumFixes == m_iNumErrors)
               _tprintf(_T("All errors in database was fixed\n"));
            else
               _tprintf(_T("Database still contain errors\n"));
         }
         bCompleted = TRUE;
      }
   }

   _tprintf(_T("Database check %s\n"), bCompleted ? _T("completed") : _T("aborted"));
}
