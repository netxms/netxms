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
// Static data
//

static MUTEX m_hMutexLockerAccess;


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

   // Clear all locks and create mutex if we was successfully locked the database
   if (bSuccess)
   {
      DBQuery(g_hCoreDB, "UPDATE locks SET lock_status=-1,owner_info='' WHERE COMPONENT_id<>0");
      m_hMutexLockerAccess = MutexCreate();
   }

   return bSuccess;
}


//
// Lock component
// Function will try to lock specified component. On success, will return TRUE.
// On failure, will return FALSE and pdwCurrentOwner will be set to the value of lock_status
// field, and pszCurrentOwnerInfo will be filled with the value of  owner_info field.
//

BOOL LockComponent(DWORD dwId, DWORD dwLockBy, char *pszOwnerInfo, 
                   DWORD *pdwCurrentOwner, char *pszCurrentOwnerInfo)
{
   char szQuery[256], szBuffer[256];
   DB_RESULT hResult;
   BOOL bSuccess = FALSE;
   DWORD dwTemp;

   if (pdwCurrentOwner == NULL)
      pdwCurrentOwner = &dwTemp;
   if (pszCurrentOwnerInfo == NULL)
      pszCurrentOwnerInfo = szBuffer;

   DbgPrintf(AF_DEBUG_LOCKS, "*Locks* Attempting to lock component %d by %d (%s)\n",
             dwId, dwLockBy, pszOwnerInfo != NULL ? pszOwnerInfo : "NULL");
   MutexLock(m_hMutexLockerAccess, INFINITE);
   sprintf(szQuery, "SELECT lock_status,owner_info FROM locks WHERE component_id=%ld", dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         *pdwCurrentOwner = DBGetFieldULong(hResult, 0, 0);
         strcpy(pszCurrentOwnerInfo, DBGetField(hResult, 0, 1));
         DBFreeResult(hResult);
         if (*pdwCurrentOwner == UNLOCKED)
         {
            sprintf(szQuery, "UPDATE locks SET lock_status=%d,owner_info='%s' WHERE component_id=%ld",
                    dwLockBy, pszOwnerInfo != NULL ? pszOwnerInfo : "", dwId);
            DBQuery(g_hCoreDB, szQuery);
            bSuccess = TRUE;
         }
      }
      else
      {
         *pdwCurrentOwner = UNLOCKED;
         strcpy(pszCurrentOwnerInfo, "Unknown component");
      }
   }
   else
   {
      *pdwCurrentOwner = UNLOCKED;
      strcpy(pszCurrentOwnerInfo, "SQL query failed");
   }
   MutexUnlock(m_hMutexLockerAccess);
   return bSuccess;
}


//
// Unlock component
//

void UnlockComponent(DWORD dwId)
{
   char szQuery[256];

   MutexLock(m_hMutexLockerAccess, INFINITE);
   sprintf(szQuery, "UPDATE locks SET lock_status=-1 WHERE component_id=%ld", dwId);
   DBQuery(g_hCoreDB, szQuery);
   MutexUnlock(m_hMutexLockerAccess);
   DbgPrintf(AF_DEBUG_LOCKS, "*Locks* Component %d unlocked\n", dwId);
}
