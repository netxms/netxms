/* 
** SQLite Database Driver
** Copyright (C) 2005-2009 Victor Kirhenshtein
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
** File: sqlitedrv.h
**
**/

#ifndef _sqlitedrv_h_
#define _sqlitedrv_h_

#ifdef _WIN32

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0502
#endif

#include <winsock2.h>
#include <windows.h>
#define EXPORT __declspec(dllexport)
#else
#include <string.h>
#define EXPORT
#endif   /* _WIN32 */

#include <dbdrv.h>
#include <nms_util.h>
#include <sqlite3.h>

/**
 * Structure of synchronous SELECT result
 */
typedef struct
{
   int nRows;
   int nCols;
   char **ppszData;
	char **ppszNames;
} SQLITE_RESULT;

/**
 * Structure of DB connection handle
 */
typedef struct
{
   sqlite3 *pdb;
   MUTEX mutexQueryLock;
   sqlite3_stmt *pvm;
   int nNumCols;              // Number of columns in async result
} SQLITE_CONN;

#endif   /* _sqlitedrv_h_ */
