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
         TCHAR szBuffer[16];

         _tprintf(_T("Database is locked by server %s [%s]\n"
                     "Do you wish to force database unlock? (Y/N)\n"),
                  szLockStatus, szLockInfo);
         _fgetts(szBuffer, 16, stdin);
         if ((szBuffer[0] == _T('y')) || (szBuffer[0] == _T('Y')))
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
