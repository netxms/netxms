/* 
** NetXMS - Network Management System
** DB Library
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: nxdbapi.h
**
**/

#ifndef _nxdbapi_h_
#define _nxdbapi_h_

#ifdef LIBNXDB_EXPORTS
#define LIBNXDB_EXPORTABLE __EXPORT
#else
#define LIBNXDB_EXPORTABLE __IMPORT
#endif

#include <dbdrv.h>
#include <uuid.h>


//
// DB-related constants
//

#define MAX_DB_LOGIN          64
#define MAX_DB_PASSWORD       64
#define MAX_DB_NAME           256

/**
 * DB events
 */
#define DBEVENT_CONNECTION_LOST        0
#define DBEVENT_CONNECTION_RESTORED    1
#define DBEVENT_QUERY_FAILED           2


/**
 * Database syntax codes
 */
#define DB_SYNTAX_MYSQL    0
#define DB_SYNTAX_PGSQL    1
#define DB_SYNTAX_MSSQL    2
#define DB_SYNTAX_ORACLE   3
#define DB_SYNTAX_SQLITE   4
#define DB_SYNTAX_DB2      5
#define DB_SYNTAX_INFORMIX 6
#define DB_SYNTAX_TSDB     7
#define DB_SYNTAX_UNKNOWN	-1

/**
 * Database connection structures
 */
struct db_driver_t;
typedef db_driver_t * DB_DRIVER;

struct db_handle_t;
typedef db_handle_t * DB_HANDLE;

struct db_statement_t;
typedef db_statement_t * DB_STATEMENT;

struct db_result_t;
typedef db_result_t * DB_RESULT;

struct db_unbuffered_result_t;
typedef db_unbuffered_result_t * DB_UNBUFFERED_RESULT;

/**
 * Pool connection information
 */
struct PoolConnectionInfo
{
   DB_HANDLE handle;
   bool inUse;
   bool resetOnRelease;
   time_t lastAccessTime;
   time_t connectTime;
   uint32_t usageCount;
   char srcFile[128];
   int srcLine;
};

/**
 * DB library performance counters
 */
struct LIBNXDB_PERF_COUNTERS
{
   uint64_t selectQueries;
   uint64_t nonSelectQueries;
   uint64_t totalQueries;
   uint64_t longRunningQueries;
   uint64_t failedQueries;
};

/**
 * Functions
 */
bool LIBNXDB_EXPORTABLE DBInit();
DB_DRIVER LIBNXDB_EXPORTABLE DBLoadDriver(const TCHAR *module, const TCHAR *initParameters,
         void (*eventHandler)(uint32_t, const WCHAR *, const WCHAR *, bool, void *), void *context);
void LIBNXDB_EXPORTABLE DBUnloadDriver(DB_DRIVER driver);
const char LIBNXDB_EXPORTABLE *DBGetDriverName(DB_DRIVER driver);
void LIBNXDB_EXPORTABLE DBSetDefaultPrefetchLimit(DB_DRIVER driver, int limit);
void LIBNXDB_EXPORTABLE DBEnableQueryTrace(bool enabled);
bool LIBNXDB_EXPORTABLE DBIsQueryTraceEnabled();

DB_HANDLE LIBNXDB_EXPORTABLE DBConnect(DB_DRIVER driver, const TCHAR *server, const TCHAR *dbName,
                                       const TCHAR *login, const TCHAR *password, const TCHAR *schema, TCHAR *errorText);
void LIBNXDB_EXPORTABLE DBDisconnect(DB_HANDLE hConn);
void LIBNXDB_EXPORTABLE DBEnableReconnect(DB_HANDLE hConn, bool enabled);
bool LIBNXDB_EXPORTABLE DBSetPrefetchLimit(DB_HANDLE hConn, int limit);
void LIBNXDB_EXPORTABLE DBSetSessionInitCallback(void (*cb)(DB_HANDLE));
DB_DRIVER LIBNXDB_EXPORTABLE DBGetDriver(DB_HANDLE hConn);

DB_STATEMENT LIBNXDB_EXPORTABLE DBPrepare(DB_HANDLE hConn, const TCHAR *query, bool optimizeForReuse = false);
DB_STATEMENT LIBNXDB_EXPORTABLE DBPrepareEx(DB_HANDLE hConn, const TCHAR *query, bool optimizeForReuse, TCHAR *errorText);
void LIBNXDB_EXPORTABLE DBFreeStatement(DB_STATEMENT hStmt);
const TCHAR LIBNXDB_EXPORTABLE *DBGetStatementSource(DB_STATEMENT hStmt);
bool LIBNXDB_EXPORTABLE DBOpenBatch(DB_STATEMENT hStmt);
void LIBNXDB_EXPORTABLE DBNextBatchRow(DB_STATEMENT hStmt);
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, int cType, const void *buffer, int allocType);
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, const TCHAR *value, int allocType);
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, const TCHAR *value, int allocType, int maxLen);
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, int16_t value);
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, uint16_t value);
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, int32_t value);
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, uint32_t value);
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, int64_t value);
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, uint64_t value);
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, double value);
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, const uuid& value);
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, const InetAddress& value);
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, const MacAddress& value);
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, Timestamp value);
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, json_t *value, int allocType);
bool LIBNXDB_EXPORTABLE DBExecute(DB_STATEMENT hStmt);
bool LIBNXDB_EXPORTABLE DBExecuteEx(DB_STATEMENT hStmt, TCHAR *errorText);
DB_RESULT LIBNXDB_EXPORTABLE DBSelectPrepared(DB_STATEMENT hStmt);
DB_RESULT LIBNXDB_EXPORTABLE DBSelectPreparedEx(DB_STATEMENT hStmt, TCHAR *errorText);
DB_UNBUFFERED_RESULT LIBNXDB_EXPORTABLE DBSelectPreparedUnbuffered(DB_STATEMENT hStmt);
DB_UNBUFFERED_RESULT LIBNXDB_EXPORTABLE DBSelectPreparedUnbufferedEx(DB_STATEMENT hStmt, TCHAR *errorText);

bool LIBNXDB_EXPORTABLE DBQuery(DB_HANDLE hConn, const TCHAR *szQuery);
bool LIBNXDB_EXPORTABLE DBQueryEx(DB_HANDLE hConn, const TCHAR *szQuery, TCHAR *errorText);

DB_RESULT LIBNXDB_EXPORTABLE DBSelect(DB_HANDLE hConn, const TCHAR *szQuery);
DB_RESULT LIBNXDB_EXPORTABLE DBSelectFormatted(DB_HANDLE hConn, const TCHAR *szQuery, ...);
DB_RESULT LIBNXDB_EXPORTABLE DBSelectEx(DB_HANDLE hConn, const TCHAR *szQuery, TCHAR *errorText);
int LIBNXDB_EXPORTABLE DBGetColumnCount(DB_RESULT hResult);
bool LIBNXDB_EXPORTABLE DBGetColumnName(DB_RESULT hResult, int column, TCHAR *buffer, int bufSize);
bool LIBNXDB_EXPORTABLE DBGetColumnNameA(DB_RESULT hResult, int column, char *buffer, int bufSize);
int LIBNXDB_EXPORTABLE DBGetNumRows(DB_RESULT hResult);
void LIBNXDB_EXPORTABLE DBFreeResult(DB_RESULT hResult);

TCHAR LIBNXDB_EXPORTABLE *DBGetField(DB_RESULT hResult, int row, int col, TCHAR *buffer, size_t bufferSize);
String LIBNXDB_EXPORTABLE DBGetFieldAsString(DB_RESULT hResult, int row, int col);
SharedString LIBNXDB_EXPORTABLE DBGetFieldAsSharedString(DB_RESULT hResult, int row, int col);
TCHAR LIBNXDB_EXPORTABLE *DBGetFieldForXML(DB_RESULT hResult, int row, int col);
char LIBNXDB_EXPORTABLE *DBGetFieldA(DB_RESULT hResult, int row, int column, char *buffer, size_t bufferSize);
char LIBNXDB_EXPORTABLE *DBGetFieldUTF8(DB_RESULT hResult, int row, int column, char *buffer, size_t bufferSize);
int16_t LIBNXDB_EXPORTABLE DBGetFieldInt16(DB_RESULT hResult, int row, int column);
uint16_t LIBNXDB_EXPORTABLE DBGetFieldUInt16(DB_RESULT hResult, int row, int column);
int32_t LIBNXDB_EXPORTABLE DBGetFieldInt32(DB_RESULT hResult, int row, int column);
uint32_t LIBNXDB_EXPORTABLE DBGetFieldUInt32(DB_RESULT hResult, int row, int column);
int64_t LIBNXDB_EXPORTABLE DBGetFieldInt64(DB_RESULT hResult, int row, int column);
uint64_t LIBNXDB_EXPORTABLE DBGetFieldUInt64(DB_RESULT hResult, int row, int column);
double LIBNXDB_EXPORTABLE DBGetFieldDouble(DB_RESULT hResult, int row, int column);
uint32_t LIBNXDB_EXPORTABLE DBGetFieldIPAddr(DB_RESULT hResult, int row, int column);
InetAddress LIBNXDB_EXPORTABLE DBGetFieldInetAddr(DB_RESULT hResult, int row, int column);
MacAddress LIBNXDB_EXPORTABLE DBGetFieldMacAddr(DB_RESULT hResult, int row, int column);
bool LIBNXDB_EXPORTABLE DBGetFieldByteArray(DB_RESULT hResult, int iRow, int iColumn, int *pnArray, size_t size, int defaultValue);
bool LIBNXDB_EXPORTABLE DBGetFieldByteArray2(DB_RESULT hResult, int iRow, int iColumn, BYTE *data, size_t size, BYTE defaultValue);
uuid LIBNXDB_EXPORTABLE DBGetFieldGUID(DB_RESULT hResult, int row, int column);

static inline time_t DBGetFieldTime(DB_RESULT hResult, int row, int column)
{
   return static_cast<time_t>(DBGetFieldInt64(hResult, row, column));
}

static inline Timestamp DBGetFieldTimestamp(DB_RESULT hResult, int row, int column)
{
   return Timestamp::fromMilliseconds(DBGetFieldInt64(hResult, row, column));
}

DB_UNBUFFERED_RESULT LIBNXDB_EXPORTABLE DBSelectUnbuffered(DB_HANDLE hConn, const TCHAR *szQuery);
DB_UNBUFFERED_RESULT LIBNXDB_EXPORTABLE DBSelectUnbufferedEx(DB_HANDLE hConn, const TCHAR *szQuery, TCHAR *errorText);
bool LIBNXDB_EXPORTABLE DBFetch(DB_UNBUFFERED_RESULT hResult);
int LIBNXDB_EXPORTABLE DBGetColumnCount(DB_UNBUFFERED_RESULT hResult);
bool LIBNXDB_EXPORTABLE DBGetColumnName(DB_UNBUFFERED_RESULT hResult, int column, TCHAR *buffer, int bufSize);
bool LIBNXDB_EXPORTABLE DBGetColumnNameA(DB_UNBUFFERED_RESULT hResult, int column, char *buffer, int bufSize);
void LIBNXDB_EXPORTABLE DBFreeResult(DB_UNBUFFERED_RESULT hResult);
void LIBNXDB_EXPORTABLE DBResultToTable(DB_RESULT hResult, Table *table);

TCHAR LIBNXDB_EXPORTABLE *DBGetField(DB_UNBUFFERED_RESULT hResult, int iColumn, TCHAR *buffer, size_t bufSize);
char LIBNXDB_EXPORTABLE *DBGetFieldUTF8(DB_UNBUFFERED_RESULT hResult, int iColumn, char *buffer, size_t bufSize);
int16_t LIBNXDB_EXPORTABLE DBGetFieldInt16(DB_UNBUFFERED_RESULT hResult, int column);
uint16_t LIBNXDB_EXPORTABLE DBGetFieldUInt16(DB_UNBUFFERED_RESULT hResult, int column);
int32_t LIBNXDB_EXPORTABLE DBGetFieldInt32(DB_UNBUFFERED_RESULT hResult, int column);
uint32_t LIBNXDB_EXPORTABLE DBGetFieldUInt32(DB_UNBUFFERED_RESULT hResult, int column);
int64_t LIBNXDB_EXPORTABLE DBGetFieldInt64(DB_UNBUFFERED_RESULT hResult, int column);
uint64_t LIBNXDB_EXPORTABLE DBGetFieldUInt64(DB_UNBUFFERED_RESULT hResult, int column);
double LIBNXDB_EXPORTABLE DBGetFieldDouble(DB_UNBUFFERED_RESULT hResult, int iColumn);
uint32_t LIBNXDB_EXPORTABLE DBGetFieldIPAddr(DB_UNBUFFERED_RESULT hResult, int column);
InetAddress LIBNXDB_EXPORTABLE DBGetFieldInetAddr(DB_UNBUFFERED_RESULT hResult, int column);
uuid LIBNXDB_EXPORTABLE DBGetFieldGUID(DB_UNBUFFERED_RESULT hResult, int column);

static inline time_t DBGetFieldTime(DB_UNBUFFERED_RESULT hResult, int column)
{
   return static_cast<time_t>(DBGetFieldInt64(hResult, column));
}

static inline Timestamp DBGetFieldTimestamp(DB_UNBUFFERED_RESULT hResult, int column)
{
   return Timestamp::fromMilliseconds(DBGetFieldInt64(hResult, column));
}

bool LIBNXDB_EXPORTABLE DBBegin(DB_HANDLE hConn);
bool LIBNXDB_EXPORTABLE DBCommit(DB_HANDLE hConn);
bool LIBNXDB_EXPORTABLE DBRollback(DB_HANDLE hConn);

StringList LIBNXDB_EXPORTABLE *DBGetTableList(DB_HANDLE hdb);
int LIBNXDB_EXPORTABLE DBIsTableExist(DB_HANDLE conn, const TCHAR *table);

bool LIBNXDB_EXPORTABLE DBGetSchemaVersion(DB_HANDLE conn, int32_t *major, int32_t *minor);
int LIBNXDB_EXPORTABLE DBGetSyntax(DB_HANDLE conn, const TCHAR *fallback = nullptr);
void LIBNXDB_EXPORTABLE DBSetSyntaxReader(bool (*reader)(DB_HANDLE, TCHAR *));

StringBuffer LIBNXDB_EXPORTABLE DBPrepareString(DB_HANDLE conn, const TCHAR *str, size_t maxSize = 0);
StringBuffer LIBNXDB_EXPORTABLE DBPrepareString(DB_DRIVER drv, const TCHAR *str, size_t maxSize = 0);
#ifdef UNICODE
StringBuffer LIBNXDB_EXPORTABLE DBPrepareStringA(DB_HANDLE conn, const char *str, size_t maxSize = 0);
StringBuffer LIBNXDB_EXPORTABLE DBPrepareStringA(DB_DRIVER drv, const char *str, size_t maxSize = 0);
#else
#define DBPrepareStringA DBPrepareString
#endif
StringBuffer LIBNXDB_EXPORTABLE DBPrepareStringUTF8(DB_HANDLE conn, const char *str, size_t maxSize = 0);
StringBuffer LIBNXDB_EXPORTABLE DBPrepareStringUTF8(DB_DRIVER drv, const char *str, size_t maxSize = 0);

bool LIBNXDB_EXPORTABLE DBConnectionPoolStartup(DB_DRIVER driver, const TCHAR *server, const TCHAR *dbName,
      const TCHAR *login, const TCHAR *password, const TCHAR *schema, int basePoolSize, int maxPoolSize,
      int cooldownTime, int connTTL);
void LIBNXDB_EXPORTABLE DBConnectionPoolShutdown();
void LIBNXDB_EXPORTABLE DBConnectionPoolReset();
DB_HANDLE LIBNXDB_EXPORTABLE __DBConnectionPoolAcquireConnection(const char *srcFile, int srcLine);
#define DBConnectionPoolAcquireConnection() __DBConnectionPoolAcquireConnection(__FILE__, __LINE__)
void LIBNXDB_EXPORTABLE DBConnectionPoolReleaseConnection(DB_HANDLE connection);
int LIBNXDB_EXPORTABLE DBConnectionPoolGetSize();
int LIBNXDB_EXPORTABLE DBConnectionPoolGetAcquiredCount();

void LIBNXDB_EXPORTABLE DBSetLongRunningThreshold(uint32_t threshold);
void LIBNXDB_EXPORTABLE DBSetLongRunningThreshold(DB_HANDLE conn, uint32_t threshold);
ObjectArray<PoolConnectionInfo> LIBNXDB_EXPORTABLE *DBConnectionPoolGetConnectionList();
void LIBNXDB_EXPORTABLE DBGetPerfCounters(LIBNXDB_PERF_COUNTERS *counters);

bool LIBNXDB_EXPORTABLE IsDatabaseRecordExist(DB_HANDLE hdb, const TCHAR *table, const TCHAR *idColumn, uint32_t id);
bool LIBNXDB_EXPORTABLE IsDatabaseRecordExist(DB_HANDLE hdb, const TCHAR *table, const TCHAR *idColumn, uint64_t id);
bool LIBNXDB_EXPORTABLE IsDatabaseRecordExist(DB_HANDLE hdb, const TCHAR *table, const TCHAR *idColumn, const uuid& id);
bool LIBNXDB_EXPORTABLE IsDatabaseRecordExist(DB_HANDLE hdb, const TCHAR *table, const TCHAR *idColumn, const TCHAR *id);
bool LIBNXDB_EXPORTABLE IsDatabaseRecordExist(DB_HANDLE hdb, const TCHAR *table, const TCHAR *idColumn, void *id, int cType, int sqlType, int allocType);

void LIBNXDB_EXPORTABLE DBSetUtilityQueryTracer(void (*queryTracer)(const TCHAR *, bool, const TCHAR *));

bool LIBNXDB_EXPORTABLE DBRenameTable(DB_HANDLE hdb, const TCHAR *oldName, const TCHAR *newName);
bool LIBNXDB_EXPORTABLE DBDropPrimaryKey(DB_HANDLE hdb, const TCHAR *table);
bool LIBNXDB_EXPORTABLE DBAddPrimaryKey(DB_HANDLE hdb, const TCHAR *table, const TCHAR *columns);
bool LIBNXDB_EXPORTABLE DBRemoveNotNullConstraint(DB_HANDLE hdb, const TCHAR *table, const TCHAR *column);
bool LIBNXDB_EXPORTABLE DBSetNotNullConstraint(DB_HANDLE hdb, const TCHAR *table, const TCHAR *column);
bool LIBNXDB_EXPORTABLE DBResizeColumn(DB_HANDLE hdb, const TCHAR *table, const TCHAR *column, int newSize, bool nullable);
bool LIBNXDB_EXPORTABLE DBDropColumn(DB_HANDLE hdb, const TCHAR *table, const TCHAR *column);
bool LIBNXDB_EXPORTABLE DBRenameColumn(DB_HANDLE hdb, const TCHAR *tableName, const TCHAR *oldName, const TCHAR *newName);
bool LIBNXDB_EXPORTABLE DBGetColumnDataType(DB_HANDLE hdb, const TCHAR *table, const TCHAR *column, TCHAR *definition, size_t len);
bool LIBNXDB_EXPORTABLE DBDropIndex(DB_HANDLE hdb, const TCHAR *table, const TCHAR *index);

DB_HANDLE LIBNXDB_EXPORTABLE DBOpenInMemoryDatabase();
void LIBNXDB_EXPORTABLE DBCloseInMemoryDatabase(DB_HANDLE hdb);
bool LIBNXDB_EXPORTABLE DBCacheTable(DB_HANDLE cacheDB, DB_HANDLE sourceDB, const TCHAR *table, const TCHAR *indexColumn, const TCHAR *columns, const TCHAR * const *intColumns = NULL);

// Compatibility defines
#define DBGetFieldLong DBGetFieldInt32
#define DBGetFieldULong DBGetFieldUInt32

#endif   /* _nxsrvapi_h_ */
