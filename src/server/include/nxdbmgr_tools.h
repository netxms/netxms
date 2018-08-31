/*
** NetXMS - Network Management System
** Database manager library
** Copyright (C) 2003-2018 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: nxdbmgr_tools.h
**
**/

#ifndef _nxdbmgr_tools_h_
#define _nxdbmgr_tools_h_

#ifdef LIBNXDBMGR_EXPORTS
#define LIBNXDBMGR_EXPORTABLE __EXPORT
#else
#define LIBNXDBMGR_EXPORTABLE __IMPORT
#endif

#include <nms_util.h>
#include <nms_agent.h>
#include <nxdbapi.h>

/**
 * Pre-defined GUID mapping for GenerateGUID
 */
struct GUID_MAPPING
{
   UINT32 id;
   const TCHAR *guid;
};

// API for nxdbmgr
void LIBNXDBMGR_EXPORTABLE SetQueryTraceMode(bool enabled);
bool LIBNXDBMGR_EXPORTABLE IsQueryTraceEnabled();
void LIBNXDBMGR_EXPORTABLE SetTableSuffix(const TCHAR *s);
void LIBNXDBMGR_EXPORTABLE SetDBMgrGUIMode(bool guiMode);
void LIBNXDBMGR_EXPORTABLE SetDBMgrForcedConfirmationMode(bool forced);

// SQL query tools
void LIBNXDBMGR_EXPORTABLE ShowQuery(const TCHAR *query);
DB_RESULT LIBNXDBMGR_EXPORTABLE SQLSelect(const TCHAR *query);
DB_UNBUFFERED_RESULT LIBNXDBMGR_EXPORTABLE SQLSelectUnbuffered(const TCHAR *query);
bool LIBNXDBMGR_EXPORTABLE SQLExecute(DB_STATEMENT hStmt);
bool LIBNXDBMGR_EXPORTABLE SQLQuery(const TCHAR *query);
bool LIBNXDBMGR_EXPORTABLE SQLBatch(const TCHAR *batch);

// Confirmations
bool LIBNXDBMGR_EXPORTABLE GetYesNo(const TCHAR *format, ...);
bool LIBNXDBMGR_EXPORTABLE GetYesNoEx(const TCHAR *format, ...);
void LIBNXDBMGR_EXPORTABLE ResetBulkYesNo();
void LIBNXDBMGR_EXPORTABLE SetOperationInProgress(bool inProgress);

// Config and metadata API
bool LIBNXDBMGR_EXPORTABLE DBMgrMetaDataReadStr(const TCHAR *variable, TCHAR *buffer, size_t bufferSize, const TCHAR *defaultValue);
INT32 LIBNXDBMGR_EXPORTABLE DBMgrMetaDataReadInt32(const TCHAR *variable, INT32 defaultValue);
bool LIBNXDBMGR_EXPORTABLE DBMgrMetaDataWriteStr(const TCHAR *variable, const TCHAR *value);
bool LIBNXDBMGR_EXPORTABLE DBMgrMetaDataWriteInt32(const TCHAR *variable, INT32 value);
bool LIBNXDBMGR_EXPORTABLE DBMgrConfigReadStr(const TCHAR *variable, TCHAR *buffer, size_t bufferSize, const TCHAR *defaultValue);
INT32 LIBNXDBMGR_EXPORTABLE DBMgrConfigReadInt32(const TCHAR *variable, INT32 defaultValue);
UINT32 LIBNXDBMGR_EXPORTABLE DBMgrConfigReadUInt32(const TCHAR *variable, UINT32 defaultValue);

// Upgrade tools
bool LIBNXDBMGR_EXPORTABLE GenerateGUID(const TCHAR *table, const TCHAR *idColumn, const TCHAR *guidColumn, const GUID_MAPPING *mapping);

bool LIBNXDBMGR_EXPORTABLE CreateTable(const TCHAR *pszQuery);

bool LIBNXDBMGR_EXPORTABLE CreateConfigParam(const TCHAR *name, const TCHAR *value, const TCHAR *description, const TCHAR *units, char dataType, bool isVisible, bool needRestart, bool isPublic, bool forceUpdate = false);
bool LIBNXDBMGR_EXPORTABLE CreateConfigParam(const TCHAR *name, const TCHAR *value, bool isVisible, bool needRestart, bool forceUpdate = false);

bool LIBNXDBMGR_EXPORTABLE ConvertStrings(const TCHAR *table, const TCHAR *idColumn, const TCHAR *idColumn2, const TCHAR *column, bool isStringId);
bool LIBNXDBMGR_EXPORTABLE ConvertStrings(const TCHAR *table, const TCHAR *idColumn, const TCHAR *column);

bool LIBNXDBMGR_EXPORTABLE CreateEventTemplate(int code, const TCHAR *name, int severity, int flags, const TCHAR *guid, const TCHAR *message, const TCHAR *description);
bool LIBNXDBMGR_EXPORTABLE IsEventPairInUse(UINT32 code1, UINT32 code2);
int LIBNXDBMGR_EXPORTABLE NextFreeEPPruleID();
bool LIBNXDBMGR_EXPORTABLE AddEventToEPPRule(const TCHAR *guid, UINT32 eventCode);

bool LIBNXDBMGR_EXPORTABLE CreateLibraryScript(UINT32 id, const TCHAR *guid, const TCHAR *name, const TCHAR *code);

// Check tools
void LIBNXDBMGR_EXPORTABLE StartStage(const TCHAR *pszMsg, int workTotal = 1);
void LIBNXDBMGR_EXPORTABLE SetStageWorkTotal(int workTotal);
void LIBNXDBMGR_EXPORTABLE UpdateStageProgress(int installment);
void LIBNXDBMGR_EXPORTABLE EndStage();

bool LIBNXDBMGR_EXPORTABLE DBMgrExecuteQueryOnObject(UINT32 objectId, const TCHAR *query);

TCHAR LIBNXDBMGR_EXPORTABLE *DBMgrGetObjectName(UINT32 objectId, TCHAR *buffer);


// Global variables
extern int LIBNXDBMGR_EXPORTABLE g_dbSyntax;
extern bool LIBNXDBMGR_EXPORTABLE g_ignoreErrors;
extern DB_HANDLE LIBNXDBMGR_EXPORTABLE g_dbHandle;
extern int LIBNXDBMGR_EXPORTABLE g_dbCheckErrors;
extern int LIBNXDBMGR_EXPORTABLE g_dbCheckFixes;

/**
 * Create save point
 */
inline INT64 CreateSavePoint()
{
   if (g_dbSyntax != DB_SYNTAX_PGSQL)
      return 0;
   INT64 id = GetCurrentTimeMs();
   TCHAR query[64];
   _sntprintf(query, 64, _T("SAVEPOINT nxdbmgr_") INT64_FMT, id);
   SQLQuery(query);
   return id;
}

/**
 * Release save point
 */
inline void ReleaseSavePoint(INT64 id)
{
   if (g_dbSyntax != DB_SYNTAX_PGSQL)
      return;
   TCHAR query[64];
   _sntprintf(query, 64, _T("RELEASE SAVEPOINT nxdbmgr_") INT64_FMT, id);
   SQLQuery(query);
}

/**
 * Rollback to save point
 */
inline void RollbackToSavePoint(INT64 id)
{
   if (g_dbSyntax != DB_SYNTAX_PGSQL)
      return;
   TCHAR query[64];
   _sntprintf(query, 64, _T("ROLLBACK TO SAVEPOINT nxdbmgr_") INT64_FMT, id);
   SQLQuery(query);
}

/**
 * Execute with error check
 */
#define CHK_EXEC(x) do { INT64 s = CreateSavePoint(); if (!(x)) { RollbackToSavePoint(s); if (!g_ignoreErrors) return false; } ReleaseSavePoint(s); } while (0)
#define CHK_EXEC_NO_SP(x) do { if (!(x)) { if (!g_ignoreErrors) return false; } } while (0)

#endif   /* _nxdbmgr_tools_h_ */
