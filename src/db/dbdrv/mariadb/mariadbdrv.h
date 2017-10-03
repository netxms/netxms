/* 
** MariaDB Database Driver
** Copyright (C) 2003-2017 Victor Kirhenshtein
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
** File: mariadbdrv.h
**
**/

#ifndef _mariadbdrv_h_
#define _mariadbdrv_h_

#ifdef _WIN32

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0502
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#define _CRT_NONSTDC_NO_WARNINGS

#include <winsock2.h>
#include <windows.h>
#define EXPORT __declspec(dllexport)
#else
#include <string.h>
#define EXPORT
#endif   /* _WIN32 */

#include <dbdrv.h>
#include <nms_util.h>

#undef GROUP_FLAG
#include <mysql.h>
#include <errmsg.h>

/**
 * Structure of DB connection handle
 */
typedef struct
{
   MYSQL *pMySQL;
   MUTEX mutexQueryLock;
} MARIADB_CONN;

/**
 * Structure of prepared statement
 */
typedef struct
{
	MARIADB_CONN *connection;
	MYSQL_STMT *statement;
	MYSQL_BIND *bindings;
	unsigned long *lengthFields;
	Array *buffers;
	int paramCount;
} MARIADB_STATEMENT;

/**
 * Structure of synchronous SELECT result
 */
typedef struct
{
	MARIADB_CONN *connection;
	MYSQL_RES *resultSet;
	bool isPreparedStatement;
	MYSQL_STMT *statement;
	int numColumns;
	int numRows;
	int currentRow;
	MYSQL_BIND *bindings;
	unsigned long *lengthFields;
} MARIADB_RESULT;

/**
 * Structure of asynchronous SELECT result
 */
typedef struct
{
   MARIADB_CONN *connection;
   MYSQL_RES *resultSet;
   MYSQL_ROW pCurrRow;
   bool noMoreRows;
   int numColumns;
   MYSQL_BIND *bindings;
   unsigned long *lengthFields;
   bool isPreparedStatement;
   MYSQL_STMT *statement;
} MARIADB_UNBUFFERED_RESULT;

#endif   /* _mariadbdrv_h_ */
