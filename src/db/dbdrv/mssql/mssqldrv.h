/* 
** MS SQL Database Driver
** Copyright (C) 2004-2016 Victor Kirhenshtein
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
** File: mssqldrv.h
**
**/

#ifndef _mssqldrv_h_
#define _mssqldrv_h_

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0502
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <dbdrv.h>
#include <nms_util.h>

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#define _SQLNCLI_ODBC_
#include <sqlncli.h>

/**
 * Driver connection handle structure
 */
typedef struct
{
   MUTEX mutexQuery;
   SQLHENV sqlEnv;
   SQLHDBC sqlConn;
} MSSQL_CONN;

/**
 * Prepared statement structure
 */
typedef struct
{
	SQLHSTMT handle;
	Array *buffers;
	MSSQL_CONN *connection;
} MSSQL_STATEMENT;

/**
 * Result buffer structure
 */
typedef struct
{
   int numRows;
   int numColumns;
   WCHAR **pValues;
	char **columnNames;
} MSSQL_QUERY_RESULT;

/**
 * Unbuffered result buffer structure
 */
typedef struct
{
   SQLHSTMT sqlStatement;
   bool isPrepared;
   int numColumns;
   MSSQL_CONN *pConn;
   bool noMoreRows;
	char **columnNames;
   WCHAR **data;
} MSSQL_UNBUFFERED_QUERY_RESULT;

#endif   /* _mssqldrv_h_ */
