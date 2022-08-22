/* 
** PostgreSQL Database Driver
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
** File: pgsqldrv.h
**
**/

#ifndef _pgsqldrv_h_
#define _pgsqldrv_h_

#ifdef _WIN32

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <winsock2.h>
#include <windows.h>
#define HAVE_PQFFORMAT 1

#endif   /* _WIN32 */

#include <nms_common.h>
#include <nms_util.h>
#include <dbdrv.h>
#include <libpq-fe.h>
#include <string.h>
#include <vector>

/**
 * PostgreSQL connection
 */
struct PG_CONN
{
	PGconn *handle;
	Mutex mutexQueryLock;

	PG_CONN(PGconn *_handle)
	{
	   handle = _handle;
	}
};

/**
 * Prepared statement
 */
struct PG_STATEMENT
{
	PG_CONN *connection;
	char name[64];
   char *query;
	std::vector<Buffer<char>> buffers;

	PG_STATEMENT(PG_CONN *c)
	{
	   connection = c;
	   query = nullptr;
	}

	~PG_STATEMENT()
	{
	   MemFree(query);
	}
};

/**
 * Unbuffered query result
 */
struct PG_UNBUFFERED_RESULT
{
   PG_CONN *conn;
   PGresult *fetchBuffer;
   bool keepFetchBuffer;
   bool singleRowMode;
   int currRow;
};

#endif   /* _pgsqldrv_h_ */
