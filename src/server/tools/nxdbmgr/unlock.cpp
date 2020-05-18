/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2020 Victor Kirhenshtein
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
   TCHAR lockStatus[MAX_DB_STRING], lockInfo[MAX_DB_STRING];
   bool locked = false;

   // Check if database is locked
   DB_RESULT hResult = DBSelect(g_dbHandle, _T("SELECT var_value FROM config WHERE var_name='DBLockStatus'"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         DBGetField(hResult, 0, 0, lockStatus, MAX_DB_STRING);
         locked = (_tcscmp(lockStatus, _T("UNLOCKED")) != 0);
      }
      DBFreeResult(hResult);

      if (locked)
      {
         hResult = DBSelect(g_dbHandle, _T("SELECT var_value FROM config WHERE var_name='DBLockInfo'"));
         if (hResult != nullptr)
         {
            if (DBGetNumRows(hResult) > 0)
            {
               DBGetField(hResult, 0, 0, lockInfo, MAX_DB_STRING);
            }
            DBFreeResult(hResult);
         }
      }
   }

   if (locked)
   {
      if (GetYesNo(_T("Database is locked by server %s [%s]\nDo you wish to force database unlock?"), lockStatus, lockInfo))
      {
         if (SQLQuery(_T("UPDATE config SET var_value='UNLOCKED' where var_name='DBLockStatus'")))
         {
            _tprintf(_T("Database lock removed\n"));
         }
      }
   }
   else
   {
      _tprintf(_T("Database is not locked\n"));
   }
}
