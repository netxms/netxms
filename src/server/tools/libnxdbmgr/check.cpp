/*
** NetXMS database manager library
** Copyright (C) 2004-2019 Victor Kirhenshtein
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
** File: check.cpp
**
**/

#include "libnxdbmgr.h"

/**
 * Global error/fix count
 */
int LIBNXDBMGR_EXPORTABLE g_dbCheckErrors = 0;
int LIBNXDBMGR_EXPORTABLE g_dbCheckFixes = 0;

/**
 * Stage data
 */
static int s_stageErrors;
static int s_stageErrorsUpdate;
static int s_stageFixes;
static TCHAR *s_stageMessage = nullptr;
static int s_stageWorkTotal = 0;
static int s_stageWorkDone = 0;

/**
 * Start stage
 */
void LIBNXDBMGR_EXPORTABLE StartStage(const TCHAR *message, int workTotal)
{
   if (message != nullptr)
   {
      MemFree(s_stageMessage);
      s_stageMessage = MemCopyString(message);
      s_stageErrors = g_dbCheckErrors;
      s_stageErrorsUpdate = g_dbCheckErrors;
      s_stageFixes = g_dbCheckFixes;
      s_stageWorkTotal = workTotal;
      s_stageWorkDone = 0;
      ResetBulkYesNo();
   }
   WriteToTerminalEx(_T("\x1b[1m*\x1b[0m %-67s  \x1b[37;1m[\x1b[0m   0%% \x1b[37;1m]\x1b[0m\b\b\b"), s_stageMessage);
#ifndef _WIN32
   fflush(stdout);
#endif
   SetOperationInProgress(true);
}

/**
 * Set total work for stage
 */
void LIBNXDBMGR_EXPORTABLE SetStageWorkTotal(int workTotal)
{
   s_stageWorkTotal = workTotal;
   if (s_stageWorkDone > s_stageWorkTotal)
      s_stageWorkDone = s_stageWorkTotal;
}

/**
 * Update stage progress
 */
void LIBNXDBMGR_EXPORTABLE UpdateStageProgress(int installment)
{
   if (g_dbCheckErrors - s_stageErrorsUpdate > 0)
   {
      StartStage(nullptr); // redisplay stage message
      s_stageErrorsUpdate = g_dbCheckErrors;
   }

   s_stageWorkDone += installment;
   if (s_stageWorkDone > s_stageWorkTotal)
      s_stageWorkDone = s_stageWorkTotal;
   WriteToTerminalEx(_T("\b\b\b%3d"), s_stageWorkDone * 100 / s_stageWorkTotal);
#ifndef _WIN32
   fflush(stdout);
#endif
}

/**
 * End stage
 */
void LIBNXDBMGR_EXPORTABLE EndStage()
{
   static const TCHAR *pszStatus[] = { _T("PASSED"), _T("FIXED "), _T("ERROR ") };
   static int nColor[] = { 32, 33, 31 };
   int nCode, nErrors;

   nErrors = g_dbCheckErrors - s_stageErrors;
   if (nErrors > 0)
   {
      nCode = (g_dbCheckFixes - s_stageFixes == nErrors) ? 1 : 2;
      if (g_dbCheckErrors - s_stageErrorsUpdate > 0)
         StartStage(nullptr); // redisplay stage message
   }
   else
   {
      nCode = 0;
   }
   WriteToTerminalEx(_T("\b\b\b\b\b\x1b[37;1m[\x1b[%d;1m%s\x1b[37;1m]\x1b[0m\n"), nColor[nCode], pszStatus[nCode]);
   SetOperationInProgress(false);
   ResetBulkYesNo();
}

/**
 * Prepare and execute SQL query with single binding - object ID.
 */
bool LIBNXDBMGR_EXPORTABLE DBMgrExecuteQueryOnObject(uint32_t objectId, const TCHAR *query)
{
   DB_STATEMENT hStmt = DBPrepare(g_dbHandle, query);
   if (hStmt == nullptr)
      return false;
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, objectId);
   bool success = DBExecute(hStmt) ? true : false;
   DBFreeStatement(hStmt);
   return success;
}

/**
 * Get object name from object_properties table
 */
TCHAR LIBNXDBMGR_EXPORTABLE *DBMgrGetObjectName(uint32_t objectId, TCHAR *buffer, bool useIdIfMissing)
{
   bool success = false;
   TCHAR query[256];
   _sntprintf(query, 256, _T("SELECT name FROM object_properties WHERE object_id=%u"), objectId);
   DB_RESULT hResult = SQLSelect(query);
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         DBGetField(hResult, 0, 0, buffer, MAX_OBJECT_NAME);
         success = true;
      }
      else if (useIdIfMissing)
      {
         _sntprintf(buffer, MAX_OBJECT_NAME, _T("[%u]"), objectId);
      }
      DBFreeResult(hResult);
   }
   else if (useIdIfMissing)
   {
      _sntprintf(buffer, MAX_OBJECT_NAME, _T("[%u]"), objectId);
   }
   return (success || useIdIfMissing) ? buffer : nullptr;
}
