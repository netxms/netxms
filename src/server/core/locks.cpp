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
// Constants
//

#define MAX_OWNER_INFO     256
#define NUMBER_OF_LOCKS    5


//
// Lock structure
//

struct LOCK_INFO
{
   DWORD dwLockStatus;
   TCHAR *pszName;
   TCHAR szOwnerInfo[MAX_OWNER_INFO];
};


//
// Static data
//

static MUTEX m_hMutexLockerAccess = NULL;
static LOCK_INFO m_locks[NUMBER_OF_LOCKS] =
{
   { UNLOCKED, _T("Event Processing Policy"), _T("") },
   { UNLOCKED, _T("User Database"), _T("") },
   { UNLOCKED, _T("Event Configuration Database"), _T("") },
   { UNLOCKED, _T("Action Configuration Database"), _T("") },
   { UNLOCKED, _T("SNMP Trap Configuration"), _T("") }
};


//
// Lock entire database and clear all other locks
// Will return FALSE if someone already locked database
//

BOOL InitLocks(DWORD *pdwIpAddr, char *pszInfo)
{
   BOOL bSuccess = FALSE;
   char szBuffer[256];

   *pdwIpAddr = UNLOCKED;
   pszInfo[0] = 0;

   // Check current database lock status
   ConfigReadStr("DBLockStatus", szBuffer, 256, "ERROR");
   if (!strcmp(szBuffer, "UNLOCKED"))
   {
      IpToStr(GetLocalIpAddr(), szBuffer);
      ConfigWriteStr("DBLockStatus", szBuffer, FALSE);
      GetSysInfoStr(szBuffer);
      ConfigWriteStr("DBLockInfo", szBuffer, TRUE);
      m_hMutexLockerAccess = MutexCreate();
      bSuccess = TRUE;
   }
   else
   {
      if (strcmp(szBuffer, "ERROR"))
      {
         *pdwIpAddr = ntohl(inet_addr(szBuffer));
         ConfigReadStr("DBLockInfo", pszInfo, 256, "<error>");
      }
   }

   return bSuccess;
}


//
// Unlock database
//

void UnlockDB(void)
{
   ConfigWriteStr("DBLockStatus", "UNLOCKED", FALSE);
   ConfigWriteStr("DBLockInfo", "", FALSE);
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
   char szBuffer[256];
   BOOL bSuccess = FALSE;
   DWORD dwTemp;

   if (pdwCurrentOwner == NULL)
      pdwCurrentOwner = &dwTemp;
   if (pszCurrentOwnerInfo == NULL)
      pszCurrentOwnerInfo = szBuffer;

   if (dwId >= NUMBER_OF_LOCKS)
   {
      *pdwCurrentOwner = UNLOCKED;
      strcpy(pszCurrentOwnerInfo, "Unknown component");
      return FALSE;
   }

   DbgPrintf(AF_DEBUG_LOCKS, "*Locks* Attempting to lock component \"%s\" by %d (%s)",
             m_locks[dwId].pszName, dwLockBy, pszOwnerInfo != NULL ? pszOwnerInfo : "NULL");
   MutexLock(m_hMutexLockerAccess, INFINITE);
   if (m_locks[dwId].dwLockStatus == UNLOCKED)
   {
      m_locks[dwId].dwLockStatus = dwLockBy;
      strncpy(m_locks[dwId].szOwnerInfo, pszOwnerInfo, MAX_OWNER_INFO);
      bSuccess = TRUE;
   }
   else
   {
      *pdwCurrentOwner = m_locks[dwId].dwLockStatus;
      strcpy(pszCurrentOwnerInfo, m_locks[dwId].szOwnerInfo);
   }
   MutexUnlock(m_hMutexLockerAccess);
   return bSuccess;
}


//
// Unlock component
//

void UnlockComponent(DWORD dwId)
{
   MutexLock(m_hMutexLockerAccess, INFINITE);
   m_locks[dwId].dwLockStatus = UNLOCKED;
   m_locks[dwId].szOwnerInfo[0] = 0;
   MutexUnlock(m_hMutexLockerAccess);
   DbgPrintf(AF_DEBUG_LOCKS, "*Locks* Component \"%s\" unlocked", m_locks[dwId].pszName);
}


//
// Unlock all locks for specific session
//

void RemoveAllSessionLocks(DWORD dwSessionId)
{
   DWORD i;

   MutexLock(m_hMutexLockerAccess, INFINITE);
   for(i = 0; i < NUMBER_OF_LOCKS; i++)
      if (m_locks[i].dwLockStatus == dwSessionId)
      {
         m_locks[i].dwLockStatus = UNLOCKED;
         m_locks[i].szOwnerInfo[0] = 0;
      }
   MutexUnlock(m_hMutexLockerAccess);
   DbgPrintf(AF_DEBUG_LOCKS, "*Locks* All locks for session %d removed", dwSessionId);
}
