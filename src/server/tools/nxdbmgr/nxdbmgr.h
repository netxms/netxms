/* 
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2012 Victor Kirhenshtein
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

//
// Execute with error check
//

#define CHK_EXEC(x) do { if (!(x)) if (!g_bIgnoreErrors) return FALSE; } while (0)


//
// Functions
//

void CheckDatabase();
void InitDatabase(const char *pszInitFile);
void ReindexDatabase();
void ReindexIData();
BOOL ClearDatabase();
void ExportDatabase(const char *file);
void ImportDatabase(const char *file);
void MigrateDatabase(const TCHAR *sourceConfig);
void UpgradeDatabase();
void UnlockDatabase();
DB_RESULT SQLSelect(const TCHAR *pszQuery);
DB_ASYNC_RESULT SQLAsyncSelect(const TCHAR *pszQuery);
BOOL SQLExecute(DB_STATEMENT hStmt);
BOOL SQLQuery(const TCHAR *pszQuery);
BOOL SQLBatch(const TCHAR *pszBatch);
BOOL SQLDropColumn(const TCHAR *table, const TCHAR *column);
bool GetYesNo(const TCHAR *format, ...);
void ShowQuery(const TCHAR *pszQuery);
BOOL ExecSQLBatch(const char *pszFile);
BOOL ValidateDatabase();

BOOL IsNodeExist(DWORD dwId);

BOOL MetaDataReadStr(const TCHAR *pszVar, TCHAR *pszBuffer, int iBufSize, const TCHAR *pszDefault);
BOOL ConfigReadStr(const TCHAR *pszVar, TCHAR *pszBuffer, int iBufSize, const TCHAR *pszDefault);
int ConfigReadInt(const TCHAR *pszVar, int iDefault);
DWORD ConfigReadULong(const TCHAR *pszVar, DWORD dwDefault);
BOOL CreateConfigParam(const TCHAR *pszName, const TCHAR *pszValue,
                       int iVisible, int iNeedRestart, BOOL bForceUpdate = FALSE);

BOOL CreateIDataTable(DWORD nodeId);
BOOL CreateTDataTables(DWORD nodeId);
BOOL CreateTDataTable_preV281(DWORD nodeId);

void ResetAdmin();

//
// Global variables
//

extern DB_HANDLE g_hCoreDB;
extern BOOL g_bIgnoreErrors;
extern BOOL g_bTrace;
extern bool g_isGuiMode;
extern bool g_checkData;
extern int g_iSyntax;
extern const TCHAR *g_pszTableSuffix;
extern const TCHAR *g_pszSqlType[6][3];


#endif
