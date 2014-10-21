/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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
** File: libnxdb.h
**
**/

#ifndef _libnxsrv_h_
#define _libnxsrv_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_threads.h>
#include <nxdbapi.h>

/**
 * Max number of loaded database drivers
 */
#define MAX_DB_DRIVERS			16

/**
 * Database driver structure
 */
struct db_driver_t
{
	const char *m_name;
	int m_refCount;
	bool m_logSqlErrors;
	bool m_dumpSql;
	int m_reconnect;
   int m_defaultPrefetchLimit;
	MUTEX m_mutexReconnect;
	HMODULE m_handle;
	void *m_userArg;
	DBDRV_CONNECTION (* m_fpDrvConnect)(const char *, const char *, const char *, const char *, const char *, WCHAR *);
	void (* m_fpDrvDisconnect)(DBDRV_CONNECTION);
	bool (* m_fpDrvSetPrefetchLimit)(DBDRV_CONNECTION, int);
	DBDRV_STATEMENT (* m_fpDrvPrepare)(DBDRV_CONNECTION, const WCHAR *, DWORD *, WCHAR *);
	void (* m_fpDrvFreeStatement)(DBDRV_STATEMENT);
	bool (* m_fpDrvOpenBatch)(DBDRV_STATEMENT);
	void (* m_fpDrvNextBatchRow)(DBDRV_STATEMENT);
	void (* m_fpDrvBind)(DBDRV_STATEMENT, int, int, int, void *, int);
	DWORD (* m_fpDrvExecute)(DBDRV_CONNECTION, DBDRV_STATEMENT, WCHAR *);
	DWORD (* m_fpDrvQuery)(DBDRV_CONNECTION, const WCHAR *, WCHAR *);
	DBDRV_RESULT (* m_fpDrvSelect)(DBDRV_CONNECTION, const WCHAR *, DWORD *, WCHAR *);
	DBDRV_ASYNC_RESULT (* m_fpDrvAsyncSelect)(DBDRV_CONNECTION, const WCHAR *, DWORD *, WCHAR *);
	DBDRV_RESULT (* m_fpDrvSelectPrepared)(DBDRV_CONNECTION, DBDRV_STATEMENT, DWORD *, WCHAR *);
	bool (* m_fpDrvFetch)(DBDRV_ASYNC_RESULT);
	LONG (* m_fpDrvGetFieldLength)(DBDRV_RESULT, int, int);
	LONG (* m_fpDrvGetFieldLengthAsync)(DBDRV_RESULT, int);
	WCHAR* (* m_fpDrvGetField)(DBDRV_RESULT, int, int, WCHAR *, int);
	char* (* m_fpDrvGetFieldUTF8)(DBDRV_RESULT, int, int, char *, int);
	WCHAR* (* m_fpDrvGetFieldAsync)(DBDRV_ASYNC_RESULT, int, WCHAR *, int);
	int (* m_fpDrvGetNumRows)(DBDRV_RESULT);
	void (* m_fpDrvFreeResult)(DBDRV_RESULT);
	void (* m_fpDrvFreeAsyncResult)(DBDRV_ASYNC_RESULT);
	DWORD (* m_fpDrvBegin)(DBDRV_CONNECTION);
	DWORD (* m_fpDrvCommit)(DBDRV_CONNECTION);
	DWORD (* m_fpDrvRollback)(DBDRV_CONNECTION);
	void (* m_fpDrvUnload)();
	void (* m_fpEventHandler)(DWORD, const WCHAR *, const WCHAR *, void *);
	int (* m_fpDrvGetColumnCount)(DBDRV_RESULT);
	const char* (* m_fpDrvGetColumnName)(DBDRV_RESULT, int);
	int (* m_fpDrvGetColumnCountAsync)(DBDRV_ASYNC_RESULT);
	const char* (* m_fpDrvGetColumnNameAsync)(DBDRV_ASYNC_RESULT, int);
	WCHAR* (* m_fpDrvPrepareStringW)(const WCHAR *);
	char* (* m_fpDrvPrepareStringA)(const char *);
	int (* m_fpDrvIsTableExist)(DBDRV_CONNECTION, const WCHAR *);
};

/**
 * Prepared statement
 */
struct db_statement_t
{
	DB_DRIVER m_driver;
	DB_HANDLE m_connection;
	DBDRV_STATEMENT m_statement;
	TCHAR *m_query;
};

/**
 * Database connection structure
 */
struct db_handle_t
{
   DBDRV_CONNECTION m_connection;
	DB_DRIVER m_driver;
	bool m_dumpSql;
	bool m_reconnectEnabled;
   MUTEX m_mutexTransLock;      // Transaction lock
   int m_transactionLevel;
   char *m_server;
   char *m_login;
   char *m_password;
   char *m_dbName;
   char *m_schema;
   ObjectArray<db_statement_t> *m_preparedStatements;
};

/**
 * SELECT query result
 */
struct db_result_t
{
	DB_DRIVER m_driver;
	DB_HANDLE m_connection;
	DBDRV_RESULT m_data;
};

/**
 * Async SELECT query result
 */
struct db_async_result_t
{
	DB_DRIVER m_driver;
	DB_HANDLE m_connection;
	DBDRV_ASYNC_RESULT m_data;
};


//
// Internal functions
//

void __DBWriteLog(WORD level, const TCHAR *format, ...);
void __DBDbgPrintf(int level, const TCHAR *format, ...);


//
// Global variables
//

extern UINT32 g_logMsgCode;
extern UINT32 g_sqlErrorMsgCode;
extern UINT32 g_sqlQueryExecTimeThreshold;


#endif   /* _libnxsrv_h_ */
