/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2025 Victor Kirhenshtein
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
   // Check if database is locked
   wchar_t lockStatus[MAX_DB_STRING];
   DBMgrConfigReadStr(L"DBLockStatus", lockStatus, MAX_DB_STRING, L"ERROR");
   int32_t lockFlag = DBMgrConfigReadInt32(L"DBLockFlag", 0);
   if ((wcscmp(lockStatus, L"UNLOCKED") != 0) || (lockFlag != 0))
   {
      wchar_t lockInfo[MAX_DB_STRING];
      DBMgrConfigReadStr(_T("DBLockInfo"), lockInfo, MAX_DB_STRING, L"<error>");
      if (GetYesNo(_T("Database is locked by server %s [%s]\nDo you wish to force database unlock?"), lockStatus, lockInfo))
      {
         if (SQLQuery(L"UPDATE config SET var_value='UNLOCKED' where var_name='DBLockStatus'") &&
             SQLQuery(L"UPDATE config SET var_value='' where var_name='DBLockInfo'") &&
             SQLQuery(L"UPDATE config SET var_value='0' where var_name='DBLockFlag'"))
         {
            WriteToTerminal(L"Database lock removed\n");
         }
      }
   }
   else
   {
      WriteToTerminal(L"Database is not locked\n");
   }
}
