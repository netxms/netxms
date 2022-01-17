/* 
** MariaDB Database Driver
** Copyright (C) 2003-2022 Victor Kirhenshtein
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
#define _WIN32_WINNT 0x0601
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#define _CRT_NONSTDC_NO_WARNINGS

#include <winsock2.h>
#include <windows.h>
#else
#include <string.h>
#endif   /* _WIN32 */

#include <dbdrv.h>
#include <nms_util.h>

#undef GROUP_FLAG
#ifdef _WIN32
// Include our own wrapper header on Windows to avoid conflict with MySQL client library
#include <mariadb.h>
#else
#include <mysql.h>
#include <errmsg.h>
#endif

/**
 * Structure of DB connection handle
 */
struct MARIADB_CONN
{
   MYSQL *mysql;
   Mutex mutexQueryLock;
   bool fixForCONC281;

   MARIADB_CONN(MYSQL *_mysql)
   {
      mysql = _mysql;
      fixForCONC281 = false;
   }
};

/**
 * Structure of prepared statement
 */
struct MARIADB_STATEMENT
{
	MARIADB_CONN *connection;
	MYSQL_STMT *statement;
	MYSQL_BIND *bindings;
	unsigned long *lengthFields;
	Array *buffers;
	int paramCount;
};

/**
 * Structure of synchronous SELECT result
 */
struct MARIADB_RESULT
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
	MYSQL_ROW *rows;
};

/**
 * Structure of asynchronous SELECT result
 */
struct MARIADB_UNBUFFERED_RESULT
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
};

#endif   /* _mariadbdrv_h_ */
