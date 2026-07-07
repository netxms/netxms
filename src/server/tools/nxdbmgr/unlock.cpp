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
 * Clear HA cluster lease if one is recorded. Clearing keeps the monotonic
 * term (next acquisition increments it) and only expires the lease - the
 * same state a graceful release leaves behind - so a surviving cluster node
 * can acquire immediately instead of waiting out the validity window.
 */
static void ClearClusterLease()
{
   DB_RESULT hResult = SQLSelect(L"SELECT term,holder_name,expires_at FROM ha_lease WHERE lease_id=1");
   if (hResult == nullptr)
      return;

   bool leaseHeld = false;
   wchar_t holderName[64] = L"";
   if (DBGetNumRows(hResult) > 0)
   {
      leaseHeld = (DBGetFieldInt64(hResult, 0, 2) > 0);
      DBGetField(hResult, 0, 1, holderName, 64);
   }
   DBFreeResult(hResult);

   if (!leaseHeld)
      return;

   if (GetYesNo(L"HA cluster lease is held by node %s and may be stale\nDo you wish to clear the lease?", holderName))
   {
      if (SQLQuery(L"UPDATE ha_lease SET holder_guid=NULL,holder_incarnation=0,holder_name=NULL,acquired_at=0,expires_at=0 WHERE lease_id=1"))
         WriteToTerminal(L"HA cluster lease cleared\n");
   }
}

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
         if (!wcscmp(lockStatus, L"CLUSTER"))
            ClearClusterLease();
      }
   }
   else
   {
      WriteToTerminal(L"Database is not locked\n");
   }
}
