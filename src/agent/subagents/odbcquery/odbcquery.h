/*
** NetXMS ODBCQUERY subagent
** Copyright (C) 2006 Alex Kalimulin
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
** File: odbcparm.h
**
**/

#ifndef __ODBCQUERY_H__
#define __ODBCQUERY_H__

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>


//
// Constants
//

#define MAX_POLLS_PER_MINUTE     60
#define MAX_SQL_QUERY_LEN			4096


//
// Target information structure
//

struct ODBC_QUERY
{
   TCHAR szName[MAX_DB_STRING];	// query name
   TCHAR szOdbcSrc[MAX_DB_STRING];	// ODBC source name
	TCHAR szSqlQuery[MAX_SQL_QUERY_LEN];
   DWORD dwPollInterval;
	TCHAR	szQueryResult[MAX_DB_STRING];
	DWORD dwCompletionCode;
   THREAD hThread;
	void*	pSqlCtx;
};


//
// ODBC functions
//

extern void* OdbcCtxAlloc(void);
extern void OdbcCtxFree(void* pvSqlCtx);
extern int OdbcConnect(void* pvSqlCtx, const TCHAR* pszSrc);
extern int OdbcDisconnect(void* pvSqlCtx);
extern int OdbcQuerySelect(void* pvSqlCtx, const TCHAR* pszQuery, TCHAR* pszResult, size_t nResSize);
extern TCHAR* OdbcGetInfo(void* pvSqlCtx);
extern TCHAR* OdbcGetSqlError(void* pvSqlCtx);
extern int OdbcGetSqlErrorNumber(void* pvSqlCtx);

#endif
