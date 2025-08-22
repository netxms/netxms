/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2025 Victor Kirhenshtein
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
#include <nxdbmgr_tools.h>

/**
 * Column identifier (table.column)
 */
struct COLUMN_IDENTIFIER
{
   const TCHAR *table;
   const char *column;
};

//
// Functions
//

DB_HANDLE ConnectToDatabase();
void CheckDatabase();
bool CreateDatabase(const char *driver, const TCHAR *dbName, const TCHAR *dbLogin, const TCHAR *dbPassword);
int InitDatabase(const char *initFile);
bool ClearDatabase(bool preMigration);
void ExportDatabase(const char *file, const StringList& excludedTables, const StringList& includedTables);
void ImportDatabase(const char *file, const StringList& excludedTables, const StringList& includedTables, bool ignoreDataMigrationErrors);
bool ConvertDatabase();
bool ConvertDataTables();
void MigrateDatabase(const TCHAR *sourceConfig, TCHAR *destConfFields, const StringList& excludedTables, const StringList& includedTables, bool ignoreDataMigrationErrors);
void UpgradeDatabase();
void UnlockDatabase();
void ReindexIData();

bool ExecSQLBatch(const char *pszFile, bool showOutput);
bool ValidateDatabase(bool allowLock = false);

bool SetMajorSchemaVersion(int32_t nextMajor, int32_t nextMinor);
bool SetMinorSchemaVersion(int32_t nextMinor);
INT32 GetSchemaLevelForMajorVersion(int32_t major);
bool SetSchemaLevelForMajorVersion(int32_t major, int32_t level);

void RegisterOnlineUpgrade(int major, int minor);
void UnregisterOnlineUpgrade(int major, int minor);
bool IsOnlineUpgradePending();
void RunPendingOnlineUpgrades();

IntegerArray<uint32_t> *GetDataCollectionTargets();
bool IsDataTableExist(const TCHAR *format, uint32_t id);

bool CreateIDataTable(uint32_t objectId);
bool CreateTDataTable(uint32_t objectId);
bool CreateTDataTable_preV281(uint32_t objectId);

void ResetSystemAccount();

bool LoadServerModules(wchar_t *moduleLoadList, bool quiet);
bool EnumerateModuleTables(bool (*handler)(const wchar_t*, void*), void *userData);
bool EnumerateModuleSchemas(bool (*handler)(const wchar_t*, void*), void *userData);
bool UpgradeModuleSchemas();
bool CheckModuleSchemas();
bool CheckModuleSchemaVersions();

bool IsColumnIntegerFixNeeded(const TCHAR *table, const char *name);
bool IsTimestampColumn(const TCHAR *table, const char *name);
bool IsTimestampConversionNeeded(const TCHAR* table);

bool LoadDataCollectionObjects();
const wchar_t *GetDCObjectStorageClass(uint32_t id);

/**
 * Global variables
 */
extern bool g_isGuiMode;
extern bool g_checkData;
extern bool g_checkDataTablesOnly;
extern bool g_dataOnlyMigration;
extern bool g_skipDataMigration;
extern bool g_skipDataSchemaMigration;
extern bool g_machineReadableOutput;
extern int g_migrationTxnSize;

#endif
