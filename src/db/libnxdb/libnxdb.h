/* 
** NetXMS - Network Management System
** Server Library
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: libnxsrv.h
**
**/

#ifndef _libnxsrv_h_
#define _libnxsrv_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_threads.h>
#include <nxdbapi.h>

void __DbgPrintf(int level, const TCHAR *format, ...);


//
// Database driver structure
//

struct _DB_DRIVER
{
	BOOL m_bWriteLog;
	BOOL m_bLogSQLErrors;
	DWORD m_logMsgCode;
	DWORD m_sqlErrorMsgCode;
	BOOL m_bDumpSQL;
	int m_nReconnect;
	MUTEX m_mutexReconnect;
	HMODULE m_hDriver;
	DB_CONNECTION (* m_fpDrvConnect)(const char *, const char *, const char *, const char *);
	void (* m_fpDrvDisconnect)(DB_CONNECTION);
	DWORD (* m_fpDrvQuery)(DB_CONNECTION, WCHAR *, TCHAR *);
	DB_RESULT (* m_fpDrvSelect)(DB_CONNECTION, WCHAR *, DWORD *, TCHAR *);
	DB_ASYNC_RESULT (* m_fpDrvAsyncSelect)(DB_CONNECTION, WCHAR *, DWORD *, TCHAR *);
	BOOL (* m_fpDrvFetch)(DB_ASYNC_RESULT);
	LONG (* m_fpDrvGetFieldLength)(DB_RESULT, int, int);
	LONG (* m_fpDrvGetFieldLengthAsync)(DB_RESULT, int);
	WCHAR* (* m_fpDrvGetField)(DB_RESULT, int, int, WCHAR *, int);
	WCHAR* (* m_fpDrvGetFieldAsync)(DB_ASYNC_RESULT, int, WCHAR *, int);
	int (* m_fpDrvGetNumRows)(DB_RESULT);
	void (* m_fpDrvFreeResult)(DB_RESULT);
	void (* m_fpDrvFreeAsyncResult)(DB_ASYNC_RESULT);
	DWORD (* m_fpDrvBegin)(DB_CONNECTION);
	DWORD (* m_fpDrvCommit)(DB_CONNECTION);
	DWORD (* m_fpDrvRollback)(DB_CONNECTION);
	void (* m_fpDrvUnload)(void);
	void (* m_fpEventHandler)(DWORD, const TCHAR *, const TCHAR *);
	int (* m_fpDrvGetColumnCount)(DB_RESULT);
	const char* (* m_fpDrvGetColumnName)(DB_RESULT, int);
	int (* m_fpDrvGetColumnCountAsync)(DB_ASYNC_RESULT);
	const char* (* m_fpDrvGetColumnNameAsync)(DB_ASYNC_RESULT, int);
	TCHAR* (* m_fpDrvPrepareString)(const TCHAR *);
};

#endif   /* _libnxsrv_h_ */
