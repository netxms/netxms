/* 
** Microsoft SQL Server Database Driver
** Copyright (C) 2004 Victor Kirhenshtein
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
** $module: mssqldrv.h
**
**/

#ifndef _mssqldrv_h_
#define _mssqldrv_h_

#ifdef _WIN32
#include <windows.h>
#define EXPORT __declspec(dllexport)
#define DBNTWIN32
#else
#define EXPORT
#endif   /* _WIN32 */

#include <stdio.h>
#include <dbdrv.h>
#include <nms_threads.h>
#include <sqlfront.h>
#include <sqldb.h>


//
// Query results structure
//

typedef struct
{
   int iNumRows;
   int iNumCols;
   char **pValues;
} MSDB_QUERY_RESULT;


//
// Asynchronous query results structure
//

typedef struct
{
   PDBPROCESS hProcess;
   BOOL bNoMoreRows;
   int iNumCols;
   int *piColTypes;
} MSDB_ASYNC_QUERY_RESULT;

#endif   /* _mssqldrv_h_ */
