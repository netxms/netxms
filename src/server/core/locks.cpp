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
** $module: locks.cpp
**
**/

#include "nms_core.h"


//
// Lock entire database and clear all other locks
// Will return FALSE if someone already locked database
//

BOOL InitLocks(DWORD *pdwIpAddr, char *pszInfo)
{
   DB_RESULT hResult;
   BOOL bSuccess = FALSE;
   DWORD dwStatus;

   *pdwIpAddr = UNLOCKED;
   pszInfo[0] = 0;

   hResult = DBSelect(g_hCoreDB, "SELECT lock_status,owner_info FROM locks WHERE component_id=0");
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         dwStatus = DBGetFieldULong(hResult, 0, 0);
         if (dwStatus != UNLOCKED)
         {
            // Database is locked by someone else, fetch owner info
            *pdwIpAddr = dwStatus;
            strcpy(pszInfo, DBGetField(hResult, 0, 1));
         }
         else
         {
            char szQuery[1024], szSysInfo[512];

            // Lock database
            GetSysInfoStr(szSysInfo);
            sprintf(szQuery, "UPDATE locks SET lock_status=%ld,owner_info='%s' WHERE component_id=0",
                    GetLocalIpAddr(), szSysInfo);
            DBQuery(g_hCoreDB, szQuery);
            bSuccess = TRUE;
         }
      }
      DBFreeResult(hResult);
   }

   // Clear all locks if we was successfully locked the database
   if (bSuccess)
      DBQuery(g_hCoreDB, "UPDATE locks SET lock_status=-1,owner_info='' WHERE COMPONENT_id<>0");

   return bSuccess;
}


//
// Lock component
//

BOOL LockComponent(DWORD dwId, DWORD dwLockBy, char *pszOwnerInfo)
{
   return FALSE;
}


//
// Unlock component
//

void UnlockComponent(DWORD dwId)
{
}
