/* 
** PostgreSQL Database Driver
** Copyright (C) 2003-2015 Victor Kirhenshtein
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
** File: pgsqldrv.h
**
**/

#ifndef _pgsqldrv_h_
#define _pgsqldrv_h_

#ifdef _WIN32

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0502
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <winsock2.h>
#include <windows.h>
#define EXPORT __declspec(dllexport)
#define HAVE_PQFFORMAT 1
#else
#define EXPORT
#endif   /* _WIN32 */

#include <nms_common.h>
#include <nms_util.h>
#include <dbdrv.h>
#include <libpq-fe.h>
#include <string.h>

/**
 * PostgreSQL connection
 */
typedef struct
{
	PGconn *handle;
	MUTEX mutexQueryLock;
} PG_CONN;

/**
 * Prepared statement
 */
typedef struct
{
	PG_CONN *connection;
	char name[64];
	int pcount;		// Number of parameters
	int allocated;	// Allocated buffers
	char **buffers;	
} PG_STATEMENT;

/**
 * Unbuffered query result
 */
typedef struct
{
   PG_CONN *conn;
   PGresult *fetchBuffer;
   bool keepFetchBuffer;
   bool singleRowMode;
   int currRow;
} PG_UNBUFFERED_RESULT;

#endif   /* _pgsqldrv_h_ */
