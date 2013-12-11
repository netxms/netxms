/* 
** MS SQL Database Driver
** Copyright (C) 2004-2011 Victor Kirhenshtein
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


//
// Driver connection handle structure
//

typedef struct
{
   MUTEX mutexQuery;
   SQLHENV sqlEnv;
   SQLHDBC sqlConn;
   SQLHSTMT sqlStatement;
} MSSQL_CONN;


//
// Prepared statement structure
//

typedef struct
{
	SQLHSTMT handle;
	Array *buffers;
	MSSQL_CONN *connection;
} MSSQL_STATEMENT;


//
// Result buffer structure
//

typedef struct
{
   long iNumRows;
   long iNumCols;
   WCHAR **pValues;
	char **columnNames;
} MSSQL_QUERY_RESULT;


//
// Async result buffer structure
//

typedef struct
{
   long iNumCols;
   MSSQL_CONN *pConn;
   BOOL bNoMoreRows;
	char **columnNames;
   WCHAR **data;
   BYTE *dataBuffer;
} MSSQL_ASYNC_QUERY_RESULT;


#endif   /* _mssqldrv_h_ */
