/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2009 Victor Kirhenshtein
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
** File: libnxdb.h
**
**/

#ifndef _libnxsrv_h_
#define _libnxsrv_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_threads.h>
#include <nxdbapi.h>


//
// Constants
//

#define MAX_DB_DRIVERS			16


//
// Database driver structure
//

struct db_driver_t
{
	const char *m_name;
	int m_refCount;
	bool m_logSqlErrors;
	bool m_dumpSql;
	int m_reconnect;
	MUTEX m_mutexReconnect;
	HMODULE m_handle;
	void *m_userArg;
	DBDRV_CONNECTION (* m_fpDrvConnect)(const char *, const char *, const char *, const char *);
	void (* m_fpDrvDisconnect)(DBDRV_CONNECTION);
	DWORD (* m_fpDrvQuery)(DBDRV_CONNECTION, WCHAR *, TCHAR *);
	DBDRV_RESULT (* m_fpDrvSelect)(DBDRV_CONNECTION, WCHAR *, DWORD *, TCHAR *);
	DBDRV_ASYNC_RESULT (* m_fpDrvAsyncSelect)(DBDRV_CONNECTION, WCHAR *, DWORD *, TCHAR *);
	BOOL (* m_fpDrvFetch)(DBDRV_ASYNC_RESULT);
	LONG (* m_fpDrvGetFieldLength)(DBDRV_RESULT, int, int);
	LONG (* m_fpDrvGetFieldLengthAsync)(DBDRV_RESULT, int);
	WCHAR* (* m_fpDrvGetField)(DBDRV_RESULT, int, int, WCHAR *, int);
	WCHAR* (* m_fpDrvGetFieldAsync)(DBDRV_ASYNC_RESULT, int, WCHAR *, int);
	int (* m_fpDrvGetNumRows)(DBDRV_RESULT);
	void (* m_fpDrvFreeResult)(DBDRV_RESULT);
	void (* m_fpDrvFreeAsyncResult)(DBDRV_ASYNC_RESULT);
	DWORD (* m_fpDrvBegin)(DBDRV_CONNECTION);
	DWORD (* m_fpDrvCommit)(DBDRV_CONNECTION);
	DWORD (* m_fpDrvRollback)(DBDRV_CONNECTION);
	void (* m_fpDrvUnload)(void);
	void (* m_fpEventHandler)(DWORD, const TCHAR *, const TCHAR *, void *);
	int (* m_fpDrvGetColumnCount)(DBDRV_RESULT);
	const char* (* m_fpDrvGetColumnName)(DBDRV_RESULT, int);
	int (* m_fpDrvGetColumnCountAsync)(DBDRV_ASYNC_RESULT);
	const char* (* m_fpDrvGetColumnNameAsync)(DBDRV_ASYNC_RESULT, int);
	TCHAR* (* m_fpDrvPrepareString)(const TCHAR *);
};


//
// database connection structure
//

struct db_handle_t
{
   DBDRV_CONNECTION m_connection;
	DB_DRIVER m_driver;
	bool m_dumpSql;
   MUTEX m_mutexTransLock;      // Transaction lock
   int m_transactionLevel;
   TCHAR *m_server;
   TCHAR *m_login;
   TCHAR *m_password;
   TCHAR *m_dbName;
};


//
// SELECT query result
//

struct db_result_t
{
	DB_DRIVER m_driver;
	DB_HANDLE m_connection;
	DBDRV_RESULT m_data;
};


//
// Async SELECT query result
//

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

extern DWORD g_logMsgCode;
extern DWORD g_sqlErrorMsgCode;


#endif   /* _libnxsrv_h_ */
