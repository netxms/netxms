/* 
** ODBC Database Driver
** Copyright (C) 2004-2022 Victor Kirhenshtein
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
** File: odbcdrv.h
**
**/

#ifndef _odbcdrv_h_
#define _odbcdrv_h_

#ifdef _WIN32

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <winsock2.h>
#include <windows.h>

#endif   /* _WIN32 */

#include <stdio.h>
#include <string.h>
#include <dbdrv.h>
#include <nms_util.h>

#ifndef _WIN32

#if HAVE_WCHAR_T

#define NETXMS_WCHAR		wchar_t

#ifdef UNICODE
#define NETXMS_TCHAR		wchar_t
#else
#define NETXMS_TCHAR		char
#endif

#else		/* HAVE_WCHAR_T */

#define NETXMS_WCHAR		WCHAR
#define NETXMS_TCHAR		TCHAR

#endif	/* HAVE_WCHAR_T */

#undef TCHAR
#undef WCHAR

#else		/* _WIN32 */

#define NETXMS_WCHAR		WCHAR
#define NETXMS_TCHAR		TCHAR

#endif

#ifndef _WIN32
#define DWORD __DWORD
#endif

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#ifndef _WIN32
#undef DWORD
#endif

/**
 * Driver connection handle structure
 */
struct ODBCDRV_CONN
{
   Mutex *mutexQuery;
   SQLHENV sqlEnv;
   SQLHDBC sqlConn;
};

/**
 * Prepared statement structure
 */
struct ODBCDRV_STATEMENT
{
	SQLHSTMT handle;
	Array *buffers;
	ODBCDRV_CONN *connection;
};

/**
 * Result buffer structure
 */
struct ODBCDRV_QUERY_RESULT
{
   long numRows;
   long numColumns;
   NETXMS_WCHAR **pValues;
	char **columnNames;
};

/**
 * Unbuffered result buffer structure
 */
struct ODBCDRV_UNBUFFERED_QUERY_RESULT
{
   SQLHSTMT sqlStatement;
   bool isPrepared;
   int numColumns;
   ODBCDRV_CONN *pConn;
   bool noMoreRows;
	char **columnNames;
   NETXMS_WCHAR **values;
};

#endif   /* _odbcdrv_h_ */
