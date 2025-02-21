/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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

#define MAX_OWNER_INFO     256

/**
 * Lock information structure
 */
struct LOCK_INFO
{
   session_id_t lockStatus;
   TCHAR ownerInfo[MAX_OWNER_INFO];
};

/**
 * EPP lock
 */
static Mutex s_mutex(MutexType::FAST);
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
      GetLocalIPAddress().toString(buffer);
      ConfigWriteStr(_T("DBLockStatus"), buffer, true, false);
      GetSysInfoStr(buffer, sizeof(buffer));
      ConfigWriteStr(_T("DBLockInfo"), buffer, true, false);
      ConfigWriteULong(_T("DBLockPID"), GetCurrentProcessId(), true, false);
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
BOOL LockEPP(session_id_t sessionId, const TCHAR *ownerInfo, session_id_t *currentOwner, TCHAR *pszCurrentOwnerInfo)
{
   TCHAR szBuffer[256];
   BOOL bSuccess = FALSE;
   session_id_t dwTemp;

   if (currentOwner == NULL)
      currentOwner = &dwTemp;
   if (pszCurrentOwnerInfo == NULL)
      pszCurrentOwnerInfo = szBuffer;

   nxlog_debug_tag(_T("event.policy"), 5, _T("Attempting to lock Event Processing Policy by %d (%s)"),
             sessionId, ownerInfo != NULL ? ownerInfo : _T("NULL"));
   s_mutex.lock();
   if (s_eppLock.lockStatus == UNLOCKED)
   {
      s_eppLock.lockStatus = sessionId;
      _tcslcpy(s_eppLock.ownerInfo, ownerInfo, MAX_OWNER_INFO);
      bSuccess = TRUE;
      nxlog_debug_tag(_T("event.policy"), 5, _T("Event Processing Policy successfully locked by %d (%s)"),
                sessionId, ownerInfo != NULL ? ownerInfo : _T("NULL"));
   }
   else
   {
      *currentOwner = s_eppLock.lockStatus;
      _tcscpy(pszCurrentOwnerInfo, s_eppLock.ownerInfo);
      nxlog_debug_tag(_T("event.policy"), 5, _T("Event Processing Policy cannot be locked by %d (%s) - already locked by \"%s\""),
                sessionId, ownerInfo != NULL ? ownerInfo : _T("NULL"),
                s_eppLock.ownerInfo);
   }
   s_mutex.unlock();
   return bSuccess;
}

/**
 * Unlock component
 */
void UnlockEPP()
{
   s_mutex.lock();
   s_eppLock.lockStatus = UNLOCKED;
   s_eppLock.ownerInfo[0] = 0;
   s_mutex.unlock();
   nxlog_debug_tag(_T("event.policy"), 5, _T("Event Processing Policy unlocked"));
}

/**
 * Unlock all locks for specific session
 */
void RemoveAllSessionLocks(session_id_t sessionId)
{
   s_mutex.lock();
   if (s_eppLock.lockStatus == sessionId)
   {
      s_eppLock.lockStatus = UNLOCKED;
      s_eppLock.ownerInfo[0] = 0;
   }
   s_mutex.unlock();
   nxlog_debug_tag(_T("client.session"), 5, _T("All locks for session %d removed"), sessionId);
}
