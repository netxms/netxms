/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2010 Victor Kirhenshtein
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
** File: locks.cpp
**
**/

#include "nxcore.h"


//
// Constants
//

#define MAX_OWNER_INFO     256
#define NUMBER_OF_LOCKS    7


//
// Lock structure
//

struct LOCK_INFO
{
   DWORD dwLockStatus;
   const TCHAR *pszName;
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
	{ UNLOCKED, _T("deprecated: Event Configuration Database"), _T("") },
	{ UNLOCKED, _T("deprecated: Action Configuration Database"), _T("") },
   { UNLOCKED, _T("SNMP Trap Configuration"), _T("") },
   { UNLOCKED, _T("Package Database"), _T("") },
   { UNLOCKED, _T("Object Tools Configuration"), _T("") }
};


//
// Lock entire database and clear all other locks
// Will return FALSE if someone already locked database
//

BOOL InitLocks(DWORD *pdwIpAddr, TCHAR *pszInfo)
{
   BOOL bSuccess = FALSE;
   TCHAR szBuffer[256];

   *pdwIpAddr = UNLOCKED;
   pszInfo[0] = 0;

   // Check current database lock status
   ConfigReadStr(_T("DBLockStatus"), szBuffer, 256, _T("ERROR"));
	DbgPrintf(6, _T("DBLockStatus=\"%s\""), szBuffer);
   if (!_tcscmp(szBuffer, _T("UNLOCKED")))
   {
      IpToStr(GetLocalIpAddr(), szBuffer);
      ConfigWriteStr(_T("DBLockStatus"), szBuffer, FALSE);
      GetSysInfoStr(szBuffer, sizeof(szBuffer));
      ConfigWriteStr(_T("DBLockInfo"), szBuffer, TRUE);
      ConfigWriteULong(_T("DBLockPID"), GetCurrentProcessId(), TRUE);
      m_hMutexLockerAccess = MutexCreate();
      bSuccess = TRUE;
   }
   else
   {
      if (_tcscmp(szBuffer, _T("ERROR")))
      {
         *pdwIpAddr = ntohl(_t_inet_addr(szBuffer));
         ConfigReadStr(_T("DBLockInfo"), pszInfo, 256, _T("<error>"));
      }
   }

   return bSuccess;
}


//
// Unlock database
//

void NXCORE_EXPORTABLE UnlockDB()
{
   ConfigWriteStr(_T("DBLockStatus"), _T("UNLOCKED"), FALSE);
   ConfigWriteStr(_T("DBLockInfo"), _T(""), FALSE);
   ConfigWriteULong(_T("DBLockPID"), 0, FALSE);
}


//
// Lock component
// Function will try to lock specified component. On success, will return TRUE.
// On failure, will return FALSE and pdwCurrentOwner will be set to the value of lock_status
// field, and pszCurrentOwnerInfo will be filled with the value of  owner_info field.
//

BOOL LockComponent(DWORD dwId, DWORD dwLockBy, const TCHAR *pszOwnerInfo, 
                   DWORD *pdwCurrentOwner, TCHAR *pszCurrentOwnerInfo)
{
   TCHAR szBuffer[256];
   BOOL bSuccess = FALSE;
   DWORD dwTemp;

   if (pdwCurrentOwner == NULL)
      pdwCurrentOwner = &dwTemp;
   if (pszCurrentOwnerInfo == NULL)
      pszCurrentOwnerInfo = szBuffer;

   if (dwId >= NUMBER_OF_LOCKS)
   {
      *pdwCurrentOwner = UNLOCKED;
      _tcscpy(pszCurrentOwnerInfo, _T("Unknown component"));
      return FALSE;
   }

   DbgPrintf(5, _T("*Locks* Attempting to lock component \"%s\" by %d (%s)"),
             m_locks[dwId].pszName, dwLockBy, pszOwnerInfo != NULL ? pszOwnerInfo : _T("NULL"));
   MutexLock(m_hMutexLockerAccess, INFINITE);
   if (m_locks[dwId].dwLockStatus == UNLOCKED)
   {
      m_locks[dwId].dwLockStatus = dwLockBy;
      nx_strncpy(m_locks[dwId].szOwnerInfo, pszOwnerInfo, MAX_OWNER_INFO);
      bSuccess = TRUE;
      DbgPrintf(5, _T("*Locks* Component \"%s\" successfully locked by %d (%s)"),
                m_locks[dwId].pszName, dwLockBy, pszOwnerInfo != NULL ? pszOwnerInfo : _T("NULL"));
   }
   else
   {
      *pdwCurrentOwner = m_locks[dwId].dwLockStatus;
      _tcscpy(pszCurrentOwnerInfo, m_locks[dwId].szOwnerInfo);
      DbgPrintf(5, _T("*Locks* Component \"%s\" cannot be locked by %d (%s) - already locked by \"%s\""),
                m_locks[dwId].pszName, dwLockBy, pszOwnerInfo != NULL ? pszOwnerInfo : _T("NULL"),
                m_locks[dwId].szOwnerInfo);
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
   DbgPrintf(5, _T("*Locks* Component \"%s\" unlocked"), m_locks[dwId].pszName);
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
   DbgPrintf(5, _T("*Locks* All locks for session %d removed"), dwSessionId);
}
