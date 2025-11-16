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
   wchar_t ownerInfo[MAX_OWNER_INFO];
};

/**
 * EPP lock
 */
static Mutex s_mutex(MutexType::FAST);
static LOCK_INFO s_eppLock = { UNLOCKED, L"" };

/**
 * Read lock flag from database. We cannot use ConfigReadInt here because it uses cached values.
 */
static int32_t ReadLockFlag()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, L"SELECT var_value FROM config WHERE var_name='DBLockFlag'");
   if (hResult == nullptr)
      return -1;

   int32_t lockFlag = DBGetNumRows(hResult) > 0 ? DBGetFieldInt32(hResult, 0, 0) : 0;
   DBFreeResult(hResult);
   DBConnectionPoolReleaseConnection(hdb);
   return lockFlag;
}

/**
 * Read lock status from database. We cannot use ConfigReadStr here because it uses cached values.
 */
static void ReadLockStatus(wchar_t *buffer)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, L"SELECT var_value FROM config WHERE var_name='DBLockStatus'");
   if (hResult == nullptr)
   {
      wcscpy(buffer, L"ERROR");
   }
   else
   {
      if (DBGetNumRows(hResult) > 0)
      {
         DBGetField(hResult, 0, 0, buffer, 256);
      }
      else
      {
         wcscpy(buffer, L"UNLOCKED");
      }
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Lock entire database and clear all other locks
 * Will return FALSE if someone already locked database
 */
bool LockDatabase(InetAddress *lockAddr, wchar_t *lockInfo)
{
   bool success = false;

   *lockAddr = InetAddress();
   lockInfo[0] = 0;

   // Check current database lock status
   int retryCount = 10;
retry:
   wchar_t buffer[256];
   ReadLockStatus(buffer);
	nxlog_debug_tag(L"db.lock", 6, L"DBLockStatus=\"%s\"", buffer);

	if (!wcscmp(buffer, L"UNLOCKED"))
   {
	   if (--retryCount < 0)
	   {
	      wcscpy(lockInfo, L"LOCK ERROR");
	      nxlog_debug_tag(L"db.lock", 1, L"Failed to acquire database lock after multiple attempts");
         return false;
	   }

      if (ReadLockFlag() != 0)
      {
         nxlog_debug_tag(L"db.lock", 6, L"Another instance is in the process of acquiring lock");
         ThreadSleepMs(1000); // Wait for other instance to complete lock acquisition or exit
         goto retry;
      }

      // First phase of double lock - set temporary flag
      int32_t lockFlag;
      RAND_bytes((BYTE *)&lockFlag, sizeof(int32_t));
      lockFlag &= 0x7FFFFFFF;  // Ensure positive value
      ConfigWriteInt(L"DBLockFlag", lockFlag, true, false, false);
      ThreadSleepMs(500);  // Small delay to ensure that if other instance also writes flag, we will read updated value
      int32_t checkFlag = ReadLockFlag();
      if (checkFlag != lockFlag)
      {
         nxlog_debug_tag(L"db.lock", 6, L"Double lock first phase failed: written flag=%d, read flag=%d", lockFlag, checkFlag);
         ThreadSleepMs(1000); // Wait for other instance to complete lock acquisition or exit
         goto retry;  // Another instance is in the process of acquiring lock
      }

      nxlog_debug_tag(L"db.lock", 6, L"Double lock first phase succeeded");

      GetLocalIPAddress().toString(buffer);
      ConfigWriteStr(L"DBLockStatus", buffer, true, false);
      GetSysInfoStr(buffer, sizeof(buffer));
      ConfigWriteStr(L"DBLockInfo", buffer, true, false);
      ConfigWriteULong(L"DBLockPID", GetCurrentProcessId(), true, false);
      ConfigWriteInt(L"DBLockFlag", 0, true, false, false);
      success = true;
   }
   else if (!wcsncmp(buffer, L"NXDBMGR", 7))
   {
      wcslcpy(lockInfo, buffer, 256);
   }
   else if (wcscmp(buffer, L"ERROR"))
   {
      *lockAddr = InetAddress::parse(buffer);
      ConfigReadStr(L"DBLockInfo", lockInfo, 256, L"<error>");
   }

   return success;
}

/**
 * Unlock database
 */
void NXCORE_EXPORTABLE UnlockDatabase()
{
   ConfigWriteStr(L"DBLockStatus", L"UNLOCKED", false);
   ConfigWriteStr(L"DBLockInfo", L"", false);
   ConfigWriteULong(L"DBLockPID", 0, false);
   ConfigWriteInt(L"DBLockFlag", 0, false);
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

   if (currentOwner == nullptr)
      currentOwner = &dwTemp;
   if (pszCurrentOwnerInfo == nullptr)
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
   nxlog_debug_tag(L"event.policy", 5, L"Event Processing Policy unlocked");
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
   nxlog_debug_tag(L"client.session", 5, L"All locks for session %d removed", sessionId);
}
