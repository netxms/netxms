/* 
** MS SQL Database Driver
** Copyright (C) 2004-2025 Victor Kirhenshtein
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

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <nms_util.h>
#include <dbdrv.h>

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#define DEBUG_TAG _T("db.drv.mssql")

/**
 * Driver connection handle structure
 */
struct MSSQL_CONN
{
   Mutex *mutexQuery;
   SQLHENV sqlEnv;
   SQLHDBC sqlConn;
};

/**
 * Prepared statement structure
 */
struct MSSQL_STATEMENT
{
	SQLHSTMT handle;
	Array *buffers;
	MSSQL_CONN *connection;
};

/**
 * Result buffer structure
 */
struct MSSQL_QUERY_RESULT
{
   int numRows;
   int numColumns;
   WCHAR **pValues;
	char **columnNames;
};

/**
 * Unbuffered result buffer structure
 */
struct MSSQL_UNBUFFERED_QUERY_RESULT
{
   SQLHSTMT sqlStatement;
   bool isPrepared;
   int numColumns;
   MSSQL_CONN *pConn;
   bool noMoreRows;
	char **columnNames;
   WCHAR **data;
};

#endif   /* _mssqldrv_h_ */
