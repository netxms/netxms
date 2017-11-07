/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2016 Victor Kirhenshtein
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
** File: nxdbmgr.h
**
**/

#ifndef _nxdbmgr_h_
#define _nxdbmgr_h_

#include <nms_common.h>
#include <nms_util.h>
#include <uuid.h>
#include <nxsrvapi.h>
#include <nxdbapi.h>
#include <netxmsdb.h>


//
// Non-standard data type codes
//

#define SQL_TYPE_TEXT      0
#define SQL_TYPE_TEXT4K    1
#define SQL_TYPE_INT64     2

/**
 * Pre-defined GUID mapping for GenerateGUID
 */
struct GUID_MAPPING
{
   UINT32 id;
   const TCHAR *guid;
};

//
// Functions
//

DB_HANDLE ConnectToDatabase();
void CheckDatabase();
void InitDatabase(const char *pszInitFile);
bool ClearDatabase(bool preMigration);
void ExportDatabase(char *file, bool skipAudit, bool skipAlarms, bool skipEvent, bool skipSysLog, bool skipTrapLog);
void ImportDatabase(const char *file);
void MigrateDatabase(const TCHAR *sourceConfig, TCHAR *destConfFields);
void UpgradeDatabase();
void UnlockDatabase();
void ReindexIData();
DB_RESULT SQLSelect(const TCHAR *pszQuery);
DB_UNBUFFERED_RESULT SQLSelectUnbuffered(const TCHAR *pszQuery);
bool SQLExecute(DB_STATEMENT hStmt);
bool SQLQuery(const TCHAR *pszQuery);
bool SQLBatch(const TCHAR *pszBatch);
bool GetYesNo(const TCHAR *format, ...);
bool GetYesNoEx(const TCHAR *format, ...);
void ResetBulkYesNo();
void SetOperationInProgress(bool inProgress);
void ShowQuery(const TCHAR *pszQuery);
bool ExecSQLBatch(const char *pszFile);
bool ValidateDatabase();

bool IsDatabaseRecordExist(const TCHAR *table, const TCHAR *idColumn, UINT32 id);

BOOL MetaDataReadStr(const TCHAR *pszVar, TCHAR *pszBuffer, int iBufSize, const TCHAR *pszDefault);
int MetaDataReadInt(const TCHAR *pszVar, int iDefault);
BOOL ConfigReadStr(const TCHAR *pszVar, TCHAR *pszBuffer, int iBufSize, const TCHAR *pszDefault);
int ConfigReadInt(const TCHAR *pszVar, int iDefault);
DWORD ConfigReadULong(const TCHAR *pszVar, DWORD dwDefault);
bool CreateConfigParam(const TCHAR *name, const TCHAR *value, bool isVisible, bool needRestart, bool forceUpdate = false);
bool CreateConfigParam(const TCHAR *name, const TCHAR *value, const TCHAR *description, char dataType, bool isVisible, bool needRestart, bool isPublic, bool forceUpdate = false);
BOOL ConvertStrings(const TCHAR *table, const TCHAR *idColumn, const TCHAR *idColumn2, const TCHAR *column, bool isStringId);
BOOL ConvertStrings(const TCHAR *table, const TCHAR *idColumn, const TCHAR *column);
BOOL CreateEventTemplate(int code, const TCHAR *name, int severity, int flags, const TCHAR *guid, const TCHAR *message, const TCHAR *description);
bool CreateTable(const TCHAR *pszQuery);
bool GenerateGUID(const TCHAR *table, const TCHAR *idColumn, const TCHAR *guidColumn, const GUID_MAPPING *mapping);
bool SetMajorSchemaVersion(INT32 nextMajor, INT32 nextMinor);
bool SetMinorSchemaVersion(INT32 nextMinor);
INT32 GetSchemaLevelForMajorVersion(INT32 major);
bool SetSchemaLevelForMajorVersion(INT32 major, INT32 level);

bool IsEventPairInUse(UINT32 code1, UINT32 code2);
int NextFreeEPPruleID();
bool AddEventToEPPRule(const TCHAR *guid, UINT32 eventCode);

IntegerArray<UINT32> *GetDataCollectionTargets();
bool IsDataTableExist(const TCHAR *format, UINT32 id);

BOOL CreateIDataTable(DWORD nodeId);
BOOL CreateTDataTable(DWORD nodeId);
BOOL CreateTDataTable_preV281(DWORD nodeId);

void ResetSystemAccount();

/**
 * Global variables
 */
extern DB_HANDLE g_hCoreDB;
extern BOOL g_bIgnoreErrors;
extern BOOL g_bTrace;
extern bool g_isGuiMode;
extern bool g_checkData;
extern bool g_checkDataTablesOnly;
extern bool g_dataOnlyMigration;
extern bool g_skipDataMigration;
extern bool g_skipDataSchemaMigration;
extern int g_migrationTxnSize;
extern int g_dbSyntax;
extern const TCHAR *g_pszTableSuffix;
extern const TCHAR *g_pszSqlType[6][3];

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
#define CHK_EXEC(x) do { INT64 s = CreateSavePoint(); if (!(x)) { RollbackToSavePoint(s); if (!g_bIgnoreErrors) return false; } ReleaseSavePoint(s); } while (0)

#endif
