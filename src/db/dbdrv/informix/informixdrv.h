/* 
** Informix Database Driver
** Copyright (C) 2010-2021 Raden Solutions
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
** File: informixdrv.h
**
**/

#ifndef _informixdrv_h_
#define _informixdrv_h_

#ifdef _WIN32

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#define _CRT_SECURE_NO_WARNINGS

#include <winsock2.h>
#include <windows.h>

#endif   /* _WIN32 */

#include <stdio.h>
#include <string.h>
#include <dbdrv.h>
#include <nms_util.h>

#ifndef _WIN32

#undef WCHAR
#undef TCHAR
#define WCHAR _I_hate_Informix_WCHAR
#define TCHAR _I_hate_Informix_TCHAR

#define SQL_NOUNICODEMAP

#endif

#include <infxcli.h>

#ifndef _WIN32
#undef WCHAR
#undef TCHAR
#define WCHAR wchar_t
#define TCHAR char
#endif

#ifdef UNICODE
#define NETXMS_TCHAR wchar_t
#else
#define NETXMS_TCHAR char
#endif

/**
 * Driver connection handle structure
 */
struct INFORMIX_CONN
{
   Mutex *mutexQuery;
   SQLHENV sqlEnv;
   SQLHDBC sqlConn;
};

/**
 * Prepared statement structure
 */
struct INFORMIX_STATEMENT
{
	SQLHSTMT handle;
	Array *buffers;
	INFORMIX_CONN *connection;
};

/**
 * Result buffer structure
 */
struct INFORMIX_QUERY_RESULT
{
   int numRows;
   int numColumns;
   WCHAR **values;
	char **columnNames;
};

/**
 * Async result buffer structure
 */
struct INFORMIX_UNBUFFERED_QUERY_RESULT
{
   SQLHSTMT sqlStatement;
   bool isPrepared;
   long numColumns;
   INFORMIX_CONN *pConn;
   bool noMoreRows;
	char **columnNames;
};

#endif   /* _informixdrv_h_ */
