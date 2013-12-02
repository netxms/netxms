/* 
** NetXMS - Network Management System
** DB Library
** Copyright (C) 2003-2011 Victor Kirhenshtein
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

#ifdef _WIN32
#ifdef LIBNXDB_EXPORTS
#define LIBNXDB_EXPORTABLE __declspec(dllexport)
#else
#define LIBNXDB_EXPORTABLE __declspec(dllimport)
#endif
#else    /* _WIN32 */
#define LIBNXDB_EXPORTABLE
#endif

#include <dbdrv.h>
#include <uuid.h>


//
// DB-related constants
//

#define MAX_DB_LOGIN          64
#define MAX_DB_PASSWORD       64
#define MAX_DB_NAME           256


//
// DB events
//

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
#define DB_SYNTAX_UNKNOWN	-1


//
// Database connection structures
//

struct db_driver_t;
typedef db_driver_t * DB_DRIVER;

struct db_handle_t;
typedef db_handle_t * DB_HANDLE;

struct db_statement_t;
typedef db_statement_t * DB_STATEMENT;

struct db_result_t;
typedef db_result_t * DB_RESULT;

struct db_async_result_t;
typedef db_async_result_t * DB_ASYNC_RESULT;


//
// Functions
//

bool LIBNXDB_EXPORTABLE DBInit(DWORD logMsgCode, DWORD sqlErrorMsgCode);
DB_DRIVER LIBNXDB_EXPORTABLE DBLoadDriver(const TCHAR *module, const TCHAR *initParameters,
														bool dumpSQL, void (* fpEventHandler)(DWORD, const WCHAR *, const WCHAR *, void *),
														void *userArg);
void LIBNXDB_EXPORTABLE DBUnloadDriver(DB_DRIVER driver);
const char LIBNXDB_EXPORTABLE *DBGetDriverName(DB_DRIVER driver);

DB_HANDLE LIBNXDB_EXPORTABLE DBConnect(DB_DRIVER driver, const TCHAR *server, const TCHAR *dbName,
                                       const TCHAR *login, const TCHAR *password, const TCHAR *schema, TCHAR *errorText);
void LIBNXDB_EXPORTABLE DBDisconnect(DB_HANDLE hConn);
void LIBNXDB_EXPORTABLE DBEnableReconnect(DB_HANDLE hConn, bool enabled);

DB_STATEMENT LIBNXDB_EXPORTABLE DBPrepare(DB_HANDLE hConn, const TCHAR *szQuery);
DB_STATEMENT LIBNXDB_EXPORTABLE DBPrepareEx(DB_HANDLE hConn, const TCHAR *szQuery, TCHAR *errorText);
void LIBNXDB_EXPORTABLE DBFreeStatement(DB_STATEMENT hStmt);
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, int cType, void *buffer, int allocType);
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, const TCHAR *value, int allocType);
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, const TCHAR *value, int allocType, int maxLen);
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, INT32 value);
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, UINT32 value);
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, INT64 value);
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, UINT64 value);
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, double value);
BOOL LIBNXDB_EXPORTABLE DBExecute(DB_STATEMENT hStmt);
BOOL LIBNXDB_EXPORTABLE DBExecuteEx(DB_STATEMENT hStmt, TCHAR *errorText);
DB_RESULT LIBNXDB_EXPORTABLE DBSelectPrepared(DB_STATEMENT hStmt);
DB_RESULT LIBNXDB_EXPORTABLE DBSelectPreparedEx(DB_STATEMENT hStmt, TCHAR *errorText);

BOOL LIBNXDB_EXPORTABLE DBQuery(DB_HANDLE hConn, const TCHAR *szQuery);
BOOL LIBNXDB_EXPORTABLE DBQueryEx(DB_HANDLE hConn, const TCHAR *szQuery, TCHAR *errorText);

DB_RESULT LIBNXDB_EXPORTABLE DBSelect(DB_HANDLE hConn, const TCHAR *szQuery);
DB_RESULT LIBNXDB_EXPORTABLE DBSelectEx(DB_HANDLE hConn, const TCHAR *szQuery, TCHAR *errorText);
int LIBNXDB_EXPORTABLE DBGetColumnCount(DB_RESULT hResult);
BOOL LIBNXDB_EXPORTABLE DBGetColumnName(DB_RESULT hResult, int column, TCHAR *buffer, int bufSize);
int LIBNXDB_EXPORTABLE DBGetNumRows(DB_RESULT hResult);
void LIBNXDB_EXPORTABLE DBFreeResult(DB_RESULT hResult);

TCHAR LIBNXDB_EXPORTABLE *DBGetField(DB_RESULT hResult, int iRow, int iColumn, TCHAR *pszBuffer, int nBufLen);
char LIBNXDB_EXPORTABLE *DBGetFieldA(DB_RESULT hResult, int iRow, int iColumn, char *pszBuffer, int nBufLen);
INT32 LIBNXDB_EXPORTABLE DBGetFieldLong(DB_RESULT hResult, int iRow, int iColumn);
UINT32 LIBNXDB_EXPORTABLE DBGetFieldULong(DB_RESULT hResult, int iRow, int iColumn);
INT64 LIBNXDB_EXPORTABLE DBGetFieldInt64(DB_RESULT hResult, int iRow, int iColumn);
UINT64 LIBNXDB_EXPORTABLE DBGetFieldUInt64(DB_RESULT hResult, int iRow, int iColumn);
double LIBNXDB_EXPORTABLE DBGetFieldDouble(DB_RESULT hResult, int iRow, int iColumn);
UINT32 LIBNXDB_EXPORTABLE DBGetFieldIPAddr(DB_RESULT hResult, int iRow, int iColumn);
BOOL LIBNXDB_EXPORTABLE DBGetFieldByteArray(DB_RESULT hResult, int iRow, int iColumn,
                                             int *pnArray, int nSize, int nDefault);
BOOL LIBNXDB_EXPORTABLE DBGetFieldByteArray2(DB_RESULT hResult, int iRow, int iColumn,
                                             BYTE *data, int nSize, int nDefault);
BOOL LIBNXDB_EXPORTABLE DBGetFieldGUID(DB_RESULT hResult, int iRow,
                                        int iColumn, uuid_t guid);

DB_ASYNC_RESULT LIBNXDB_EXPORTABLE DBAsyncSelect(DB_HANDLE hConn, const TCHAR *szQuery);
DB_ASYNC_RESULT LIBNXDB_EXPORTABLE DBAsyncSelectEx(DB_HANDLE hConn, const TCHAR *szQuery, TCHAR *errorText);
BOOL LIBNXDB_EXPORTABLE DBFetch(DB_ASYNC_RESULT hResult);
int LIBNXDB_EXPORTABLE DBGetColumnCountAsync(DB_ASYNC_RESULT hResult);
BOOL LIBNXDB_EXPORTABLE DBGetColumnNameAsync(DB_ASYNC_RESULT hResult, int column, TCHAR *buffer, int bufSize);
void LIBNXDB_EXPORTABLE DBFreeAsyncResult(DB_ASYNC_RESULT hResult);

TCHAR LIBNXDB_EXPORTABLE *DBGetFieldAsync(DB_ASYNC_RESULT hResult, int iColumn, TCHAR *pBuffer, int iBufSize);
INT32 LIBNXDB_EXPORTABLE DBGetFieldAsyncLong(DB_ASYNC_RESULT hResult, int iColumn);
UINT32 LIBNXDB_EXPORTABLE DBGetFieldAsyncULong(DB_ASYNC_RESULT hResult, int iColumn);
INT64 LIBNXDB_EXPORTABLE DBGetFieldAsyncInt64(DB_ASYNC_RESULT hResult, int iColumn);
UINT64 LIBNXDB_EXPORTABLE DBGetFieldAsyncUInt64(DB_ASYNC_RESULT hResult, int iColumn);
double LIBNXDB_EXPORTABLE DBGetFieldAsyncDouble(DB_ASYNC_RESULT hResult, int iColumn);
UINT32 LIBNXDB_EXPORTABLE DBGetFieldAsyncIPAddr(DB_ASYNC_RESULT hResult, int iColumn);

BOOL LIBNXDB_EXPORTABLE DBBegin(DB_HANDLE hConn);
BOOL LIBNXDB_EXPORTABLE DBCommit(DB_HANDLE hConn);
BOOL LIBNXDB_EXPORTABLE DBRollback(DB_HANDLE hConn);

int LIBNXDB_EXPORTABLE DBGetSchemaVersion(DB_HANDLE conn);
int LIBNXDB_EXPORTABLE DBGetSyntax(DB_HANDLE conn);

String LIBNXDB_EXPORTABLE DBPrepareString(DB_HANDLE conn, const TCHAR *str, int maxSize = -1);
#ifdef UNICODE
String LIBNXDB_EXPORTABLE DBPrepareStringA(DB_HANDLE conn, const char *str, int maxSize = -1);
#else
#define DBPrepareStringA DBPrepareString
#endif
TCHAR LIBNXDB_EXPORTABLE *EncodeSQLString(const TCHAR *pszIn);
void LIBNXDB_EXPORTABLE DecodeSQLString(TCHAR *pszStr);

bool LIBNXDB_EXPORTABLE DBConnectionPoolStartup(DB_DRIVER driver, const TCHAR *server, const TCHAR *dbName,
																const TCHAR *login, const TCHAR *password, const TCHAR *schema,
																int basePoolSize, int maxPoolSize, int cooldownTime,
																int connTTL, DB_HANDLE fallback);
void LIBNXDB_EXPORTABLE DBConnectionPoolShutdown();
DB_HANDLE LIBNXDB_EXPORTABLE DBConnectionPoolAcquireConnection();
void LIBNXDB_EXPORTABLE DBConnectionPoolReleaseConnection(DB_HANDLE connection);
int LIBNXDB_EXPORTABLE DBConnectionPoolGetSize();
int LIBNXDB_EXPORTABLE DBConnectionPoolGetAcquiredCount();

void LIBNXDB_EXPORTABLE DBSetDebugPrintCallback(void (*cb)(int, const TCHAR *, va_list));

#endif   /* _nxsrvapi_h_ */
