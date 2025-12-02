/*
** NetXMS - Network Management System
** Database manager library
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
#include <nxsrvapi.h>

// Non-standard SQL type codes
#define SQL_TYPE_TEXT      0
#define SQL_TYPE_TEXT4K    1
#define SQL_TYPE_INT64     2
#define SQL_TYPE_BLOB      3

/**
 * Pre-defined GUID mapping for GenerateGUID
 */
struct GUID_MAPPING
{
   uint32_t id;
   const TCHAR *guid;
};

// API for nxdbmgr
void LIBNXDBMGR_EXPORTABLE SetQueryTraceMode(bool enabled);
bool LIBNXDBMGR_EXPORTABLE IsQueryTraceEnabled();
void LIBNXDBMGR_EXPORTABLE SetTableSuffix(const TCHAR *s);
void LIBNXDBMGR_EXPORTABLE SetDBMgrGUIMode(bool guiMode);
void LIBNXDBMGR_EXPORTABLE SetDBMgrForcedConfirmationMode(bool forced);
void LIBNXDBMGR_EXPORTABLE SetDBMgrFailOnFixMode(bool fail);

// SQL query tools
void LIBNXDBMGR_EXPORTABLE ShowQuery(const TCHAR *query);
DB_RESULT LIBNXDBMGR_EXPORTABLE SQLSelect(const TCHAR *query);
DB_RESULT LIBNXDBMGR_EXPORTABLE SQLSelectEx(DB_HANDLE hdb, const TCHAR *query);
DB_UNBUFFERED_RESULT LIBNXDBMGR_EXPORTABLE SQLSelectUnbuffered(const TCHAR *query);
bool LIBNXDBMGR_EXPORTABLE SQLExecute(DB_STATEMENT hStmt);
bool LIBNXDBMGR_EXPORTABLE SQLQueryFormatted(const wchar_t *query, ...);
bool LIBNXDBMGR_EXPORTABLE SQLQuery(const wchar_t *query, bool showOutput = false);
bool LIBNXDBMGR_EXPORTABLE SQLBatch(const wchar_t *batch);
const wchar_t LIBNXDBMGR_EXPORTABLE *GetSQLTypeName(int type);

// Confirmations
bool LIBNXDBMGR_EXPORTABLE GetYesNo(const TCHAR *format, ...);
bool LIBNXDBMGR_EXPORTABLE GetYesNoEx(const TCHAR *format, ...);
void LIBNXDBMGR_EXPORTABLE ResetBulkYesNo();
void LIBNXDBMGR_EXPORTABLE SetOperationInProgress(bool inProgress);

// Config and metadata API
bool LIBNXDBMGR_EXPORTABLE DBMgrMetaDataReadStr(const TCHAR *variable, TCHAR *buffer, size_t bufferSize, const TCHAR *defaultValue);
bool LIBNXDBMGR_EXPORTABLE DBMgrMetaDataReadStrEx(DB_HANDLE hdb, const TCHAR *variable, TCHAR *buffer, size_t bufferSize, const TCHAR *defaultValue);
int32_t LIBNXDBMGR_EXPORTABLE DBMgrMetaDataReadInt32(const TCHAR *variable, int32_t defaultValue);
int32_t LIBNXDBMGR_EXPORTABLE DBMgrMetaDataReadInt32Ex(DB_HANDLE hdb, const TCHAR *variable, int32_t defaultValue);
bool LIBNXDBMGR_EXPORTABLE DBMgrMetaDataWriteStr(const wchar_t *variable, const wchar_t *value);
bool LIBNXDBMGR_EXPORTABLE DBMgrMetaDataWriteInt32(const wchar_t *variable, int32_t value);
bool LIBNXDBMGR_EXPORTABLE DBMgrConfigReadStr(const TCHAR *variable, TCHAR *buffer, size_t bufferSize, const TCHAR *defaultValue);
int32_t LIBNXDBMGR_EXPORTABLE DBMgrConfigReadInt32(const TCHAR *variable, int32_t defaultValue);
uint32_t LIBNXDBMGR_EXPORTABLE DBMgrConfigReadUInt32(const TCHAR *variable, uint32_t defaultValue);
StringMap LIBNXDBMGR_EXPORTABLE *DBMgrGetConfigurationVariables(const TCHAR *pattern);

// Upgrade tools
bool LIBNXDBMGR_EXPORTABLE GenerateGUID(const TCHAR *table, const TCHAR *idColumn, const TCHAR *guidColumn, const GUID_MAPPING *mapping);

bool LIBNXDBMGR_EXPORTABLE CreateTable(const TCHAR *pszQuery);

bool LIBNXDBMGR_EXPORTABLE CreateConfigParam(const wchar_t *name, const wchar_t *value, const wchar_t *description, const wchar_t *units, char dataType, bool isVisible, bool needRestart, bool isPublic, bool forceUpdate = false);
bool LIBNXDBMGR_EXPORTABLE CreateConfigParam(const wchar_t *name, const wchar_t *value, bool isVisible, bool needRestart, bool forceUpdate = false);

bool LIBNXDBMGR_EXPORTABLE ConvertStrings(const TCHAR *table, const TCHAR *idColumn, const TCHAR *idColumn2, const TCHAR *column, bool isStringId);
bool LIBNXDBMGR_EXPORTABLE ConvertStrings(const TCHAR *table, const TCHAR *idColumn, const TCHAR *column);

bool LIBNXDBMGR_EXPORTABLE ConvertColumnToInt64(const wchar_t *table, const wchar_t *column);

bool LIBNXDBMGR_EXPORTABLE CreateEventTemplate(int code, const TCHAR *name, int severity, int flags, const TCHAR *guid, const TCHAR *message, const TCHAR *description);
bool LIBNXDBMGR_EXPORTABLE IsEventPairInUse(UINT32 code1, UINT32 code2);
int LIBNXDBMGR_EXPORTABLE NextFreeEPPruleID();
bool LIBNXDBMGR_EXPORTABLE AddEventToEPPRule(const TCHAR *guid, uint32_t eventCode);

bool LIBNXDBMGR_EXPORTABLE CreateLibraryScript(uint32_t id, const TCHAR *guid, const TCHAR *name, const TCHAR *code);

void LIBNXDBMGR_EXPORTABLE DecodeSQLString(TCHAR *str);

// Check tools
void LIBNXDBMGR_EXPORTABLE StartStage(const TCHAR *pszMsg, int workTotal = 1);
void LIBNXDBMGR_EXPORTABLE SetStageWorkTotal(int workTotal);
void LIBNXDBMGR_EXPORTABLE UpdateStageProgress(int installment);
void LIBNXDBMGR_EXPORTABLE EndStage();

bool LIBNXDBMGR_EXPORTABLE DBMgrExecuteQueryOnObject(uint32_t objectId, const TCHAR *query);

TCHAR LIBNXDBMGR_EXPORTABLE *DBMgrGetObjectName(uint32_t objectId, TCHAR *buffer, bool useIdIfMissing = true);

bool LIBNXDBMGR_EXPORTABLE ConvertXmlToJson(const TCHAR *table, const TCHAR *idColumn1, const TCHAR *idColumn2, const TCHAR *dataColumn);

// Global variables
extern bool LIBNXDBMGR_EXPORTABLE g_ignoreErrors;
extern DB_HANDLE LIBNXDBMGR_EXPORTABLE g_dbHandle;
extern int LIBNXDBMGR_EXPORTABLE g_dbCheckErrors;
extern int LIBNXDBMGR_EXPORTABLE g_dbCheckFixes;

/**
 * Create save point
 */
static inline int64_t CreateSavePoint()
{
   if ((g_dbSyntax != DB_SYNTAX_PGSQL) && (g_dbSyntax != DB_SYNTAX_TSDB))
      return 0;
   int64_t id = GetCurrentTimeMs();
   TCHAR query[64];
   _sntprintf(query, 64, _T("SAVEPOINT nxdbmgr_") INT64_FMT, id);
   SQLQuery(query);
   return id;
}

/**
 * Release save point
 */
static inline void ReleaseSavePoint(int64_t id)
{
   if ((g_dbSyntax != DB_SYNTAX_PGSQL) && (g_dbSyntax != DB_SYNTAX_TSDB))
      return;
   TCHAR query[64];
   _sntprintf(query, 64, _T("RELEASE SAVEPOINT nxdbmgr_") INT64_FMT, id);
   SQLQuery(query);
}

/**
 * Rollback to save point
 */
static inline void RollbackToSavePoint(int64_t id)
{
   if ((g_dbSyntax != DB_SYNTAX_PGSQL) && (g_dbSyntax != DB_SYNTAX_TSDB))
      return;
   TCHAR query[64];
   _sntprintf(query, 64, _T("ROLLBACK TO SAVEPOINT nxdbmgr_") INT64_FMT, id);
   SQLQuery(query);
}

/**
 * Execute with error check
 */
#define CHK_EXEC(x) do { int64_t s = CreateSavePoint(); if (!(x)) { RollbackToSavePoint(s); if (!g_ignoreErrors) return false; } ReleaseSavePoint(s); } while (0)
#define CHK_EXEC_RB(x) do { int64_t s = CreateSavePoint(); if (!(x)) { RollbackToSavePoint(s); if (!g_ignoreErrors) { DBRollback(g_dbHandle); return false; } } ReleaseSavePoint(s); } while (0)
#define CHK_EXEC_NO_SP(x) do { if (!(x)) { if (!g_ignoreErrors) return false; } } while (0)
#define CHK_EXEC_NO_SP_WITH_HOOK(x, hook) do { if (!(x)) { if (!g_ignoreErrors) { hook; return false; } } } while (0)

#endif   /* _nxdbmgr_tools_h_ */
