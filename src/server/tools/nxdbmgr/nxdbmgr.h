/* 
** nxdbmgr - NetXMS database manager
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
** $module: nxdbmgr.h
**
**/

#ifndef _nxdbmgr_h_
#define _nxdbmgr_h_

#include <nms_common.h>
#include <nms_util.h>
#include <uuid.h>
#include <nxsrvapi.h>
#include <netxmsdb.h>


//
// Database syntax codes
//

#define DB_SYNTAX_MYSQL    0
#define DB_SYNTAX_PGSQL    1
#define DB_SYNTAX_MSSQL    2
#define DB_SYNTAX_ORACLE   3
#define DB_SYNTAX_SQLITE   4


//
// Non-standard data type codes
//

#define SQL_TYPE_TEXT      0
#define SQL_TYPE_INT64     1


//
// Functions
//

void CheckDatabase(void);
void InitDatabase(TCHAR *pszInitFile);
void ReindexDatabase(void);
void UpgradeDatabase(void);
void UnlockDatabase(void);
DB_RESULT SQLSelect(TCHAR *pszQuery);
BOOL SQLQuery(TCHAR *pszQuery);
BOOL SQLBatch(TCHAR *pszBatch);
BOOL GetYesNo(void);
void ShowQuery(TCHAR *pszQuery);

BOOL ConfigReadStr(TCHAR *pszVar, TCHAR *pszBuffer, int iBufSize, const TCHAR *pszDefault);
int ConfigReadInt(TCHAR *pszVar, int iDefault);
DWORD ConfigReadULong(TCHAR *pszVar, DWORD dwDefault);


//
// Global variables
//

extern DB_HANDLE g_hCoreDB;
extern BOOL g_bIgnoreErrors;
extern BOOL g_bTrace;
extern int g_iSyntax;
extern TCHAR *g_pszTableSuffix;
extern TCHAR *g_pszSqlType[][2];


#endif
