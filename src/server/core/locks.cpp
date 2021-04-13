/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2021 Victor Kirhenshtein
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

/**
 * Lock information structure
 */
struct LOCK_INFO
{
   UINT32 dwLockStatus;
   TCHAR szOwnerInfo[MAX_OWNER_INFO];
};

/**
 * EPP lock
 */
static MUTEX m_hMutexLockerAccess = NULL;
static LOCK_INFO s_eppLock = { UNLOCKED, _T("")};

/**
 * Lock entire database and clear all other locks
 * Will return FALSE if someone already locked database
 */
bool LockDatabase(InetAddress *lockAddr, TCHAR *lockInfo)
{
   bool success = false;

   *lockAddr = InetAddress();
   lockInfo[0] = 0;

   // Check current database lock status
   TCHAR buffer[256];
   ConfigReadStr(_T("DBLockStatus"), buffer, 256, _T("ERROR"));
	nxlog_debug_tag(_T("db.lock"), 6, _T("DBLockStatus=\"%s\""), buffer);
   if (!_tcscmp(buffer, _T("UNLOCKED")))
   {
      GetLocalIpAddr().toString(buffer);
      ConfigWriteStr(_T("DBLockStatus"), buffer, true, false);
      GetSysInfoStr(buffer, sizeof(buffer));
      ConfigWriteStr(_T("DBLockInfo"), buffer, true, false);
      ConfigWriteULong(_T("DBLockPID"), GetCurrentProcessId(), true, false);
      m_hMutexLockerAccess = MutexCreate();
      success = true;
   }
   else if (!_tcsncmp(buffer, _T("NXDBMGR"), 7))
   {
      _tcslcpy(lockInfo, buffer, 256);
   }
   else if (_tcscmp(buffer, _T("ERROR")))
   {
      *lockAddr = InetAddress::parse(buffer);
      ConfigReadStr(_T("DBLockInfo"), lockInfo, 256, _T("<error>"));
   }

   return success;
}

/**
 * Unlock database
 */
void NXCORE_EXPORTABLE UnlockDatabase()
{
   ConfigWriteStr(_T("DBLockStatus"), _T("UNLOCKED"), false);
   ConfigWriteStr(_T("DBLockInfo"), _T(""), false);
   ConfigWriteULong(_T("DBLockPID"), 0, false);
}

/**
 * Lock component
 * Function will try to lock specified component. On success, will return TRUE.
 * On failure, will return FALSE and pdwCurrentOwner will be set to the value of lock_status
 * field, and pszCurrentOwnerInfo will be filled with the value of  owner_info field.
 */
BOOL LockEPP(int sessionId, const TCHAR *pszOwnerInfo, UINT32 *pdwCurrentOwner, TCHAR *pszCurrentOwnerInfo)
{
   TCHAR szBuffer[256];
   BOOL bSuccess = FALSE;
   UINT32 dwTemp;

   if (pdwCurrentOwner == NULL)
      pdwCurrentOwner = &dwTemp;
   if (pszCurrentOwnerInfo == NULL)
      pszCurrentOwnerInfo = szBuffer;

   DbgPrintf(5, _T("*Locks* Attempting to lock Event Processing Policy by %d (%s)"),
             sessionId, pszOwnerInfo != NULL ? pszOwnerInfo : _T("NULL"));
   MutexLock(m_hMutexLockerAccess);
   if (s_eppLock.dwLockStatus == UNLOCKED)
   {
      s_eppLock.dwLockStatus = (UINT32)sessionId;
      nx_strncpy(s_eppLock.szOwnerInfo, pszOwnerInfo, MAX_OWNER_INFO);
      bSuccess = TRUE;
      DbgPrintf(5, _T("*Locks* Event Processing Policy successfully locked by %d (%s)"),
                sessionId, pszOwnerInfo != NULL ? pszOwnerInfo : _T("NULL"));
   }
   else
   {
      *pdwCurrentOwner = s_eppLock.dwLockStatus;
      _tcscpy(pszCurrentOwnerInfo, s_eppLock.szOwnerInfo);
      DbgPrintf(5, _T("*Locks* Event Processing Policy cannot be locked by %d (%s) - already locked by \"%s\""),
                sessionId, pszOwnerInfo != NULL ? pszOwnerInfo : _T("NULL"),
                s_eppLock.szOwnerInfo);
   }
   MutexUnlock(m_hMutexLockerAccess);
   return bSuccess;
}

/**
 * Unlock component
 */
void UnlockEPP()
{
   MutexLock(m_hMutexLockerAccess);
   s_eppLock.dwLockStatus = UNLOCKED;
   s_eppLock.szOwnerInfo[0] = 0;
   MutexUnlock(m_hMutexLockerAccess);
   DbgPrintf(5, _T("*Locks* Event Processing Policy unlocked"));
}

/**
 * Unlock all locks for specific session
 */
void RemoveAllSessionLocks(int sessionId)
{
   MutexLock(m_hMutexLockerAccess);
   if (s_eppLock.dwLockStatus == (UINT32)sessionId)
   {
      s_eppLock.dwLockStatus = UNLOCKED;
      s_eppLock.szOwnerInfo[0] = 0;
   }
   MutexUnlock(m_hMutexLockerAccess);
   DbgPrintf(5, _T("*Locks* All locks for session %d removed"), sessionId);
}
