/* 
** Informix Database Driver
** Copyright (C) 2010, 2011 Raden Solutions
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

#define _CRT_SECURE_NO_WARNINGS

#include <winsock2.h>
#include <windows.h>

#define EXPORT __declspec(dllexport)

#else

#define EXPORT

#endif   /* _WIN32 */

#include <stdio.h>
#include <string.h>
#include <dbdrv.h>
#include <nms_util.h>

#ifndef _WIN32

#if HAVE_WCHAR_H
#define NETXMS_WCHAR		wchar_t
#else		/* HAVE_WCHAR_T */
#define NETXMS_WCHAR		WCHAR
#endif	/* HAVE_WCHAR_T */
#undef WCHAR

#else		/* _WIN32 */
#define NETXMS_WCHAR		WCHAR
#endif

//#define __BOOL // disable BOOL typedef
#include <infxcli.h>


//
// Driver connection handle structure
//

typedef struct
{
   MUTEX mutexQuery;
   SQLHENV sqlEnv;
   SQLHDBC sqlConn;
   SQLHSTMT sqlStatement;
} INFORMIX_CONN;


//
// Prepared statement structure
//

typedef struct
{
	SQLHSTMT handle;
	Array *buffers;
	INFORMIX_CONN *connection;
} INFORMIX_STATEMENT;


//
// Result buffer structure
//

typedef struct
{
   long iNumRows;
   long iNumCols;
   NETXMS_WCHAR **pValues;
	char **columnNames;
} INFORMIX_QUERY_RESULT;


//
// Async result buffer structure
//

typedef struct
{
   long iNumCols;
   INFORMIX_CONN *pConn;
   BOOL bNoMoreRows;
	char **columnNames;
} INFORMIX_ASYNC_QUERY_RESULT;


#endif   /* _informixdrv_h_ */
