/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004, 2005, 2006 Victor Kirhenshtein
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
** File: unlock.cpp
**
**/

#include "nxdbmgr.h"

/**
 * Unlock database
 */
void UnlockDatabase()
{
   DB_RESULT hResult;
   TCHAR szLockStatus[MAX_DB_STRING], szLockInfo[MAX_DB_STRING];
   BOOL bLocked = FALSE;

   // Check if database is locked
   hResult = DBSelect(g_hCoreDB, _T("SELECT var_value FROM config WHERE var_name='DBLockStatus'"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         DBGetField(hResult, 0, 0, szLockStatus, MAX_CONFIG_VALUE);
         DecodeSQLString(szLockStatus);
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
               DBGetField(hResult, 0, 0, szLockInfo, MAX_CONFIG_VALUE);
               DecodeSQLString(szLockInfo);
            }
            DBFreeResult(hResult);
         }
      }
   }

   if (bLocked)
   {
      if (GetYesNo(_T("Database is locked by server %s [%s]\nDo you wish to force database unlock?"), szLockStatus, szLockInfo))
      {
         if (SQLQuery(_T("UPDATE config SET var_value='UNLOCKED' where var_name='DBLockStatus'")))
         {
            bLocked = FALSE;
            _tprintf(_T("Database lock removed\n"));
         }
      }
   }
   else
   {
      _tprintf(_T("Database is not locked\n"));
   }
}
