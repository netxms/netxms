/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 Victor Kirhenshtein
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
** File: dbwrite.cpp
**
**/

#include "nxcore.h"


//
// Constants
//

#define MAX_DB_WRITERS     16


//
// Global variables
//

Queue *g_pLazyRequestQueue = NULL;


//
// Static data
//

static int m_iNumWriters = 1;
static THREAD m_hWriteThreadList[MAX_DB_WRITERS];


//
// Put SQL request into queue for later execution
//

void NXCORE_EXPORTABLE QueueSQLRequest(const TCHAR *query)
{
	DELAYED_SQL_REQUEST *rq = (DELAYED_SQL_REQUEST *)malloc(sizeof(DELAYED_SQL_REQUEST) + (_tcslen(query) + 1) * sizeof(TCHAR));
	rq->query = (TCHAR *)&rq->bindings[0];
	_tcscpy(rq->query, query);
	rq->bindCount = 0;
   g_pLazyRequestQueue->Put(rq);
	DbgPrintf(8, _T("SQL request queued: %s"), query);
}


//
// Put parameterized SQL request into queue for later execution
//

void NXCORE_EXPORTABLE QueueSQLRequest(const TCHAR *query, int bindCount, int *sqlTypes, const TCHAR **values)
{
	int size = sizeof(DELAYED_SQL_REQUEST) + (_tcslen(query) + 1) * sizeof(TCHAR) + bindCount * sizeof(TCHAR *) + bindCount;
	for(int i = 0; i < bindCount; i++)
		size += (_tcslen(values[i]) + 1) * sizeof(TCHAR) + sizeof(TCHAR *);
	DELAYED_SQL_REQUEST *rq = (DELAYED_SQL_REQUEST *)malloc(size);

	BYTE *base = (BYTE *)&rq->bindings[bindCount];
	int pos = 0;
	int align = sizeof(TCHAR *);

	rq->query = (TCHAR *)base;
	_tcscpy(rq->query, query);
	rq->bindCount = bindCount;
	pos += (_tcslen(query) + 1) * sizeof(TCHAR);

	rq->sqlTypes = &base[pos];
	pos += bindCount;
	if (pos % align != 0)
		pos += align - pos % align;

	for(int i = 0; i < bindCount; i++)
	{
		rq->sqlTypes[i] = (BYTE)sqlTypes[i];
		rq->bindings[i] = (TCHAR *)&base[pos];
		_tcscpy(rq->bindings[i], values[i]);
		pos += (_tcslen(values[i]) + 1) * sizeof(TCHAR);
		if (pos % align != 0)
			pos += align - pos % align;
	}

   g_pLazyRequestQueue->Put(rq);
	DbgPrintf(8, _T("SQL request queued: %s"), query);
}


//
// Database "lazy" write thread
//

static THREAD_RESULT THREAD_CALL DBWriteThread(void *arg)
{
   DB_HANDLE hdb;

   if (g_dwFlags & AF_ENABLE_MULTIPLE_DB_CONN)
   {
		TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
      hdb = DBConnect(g_dbDriver, g_szDbServer, g_szDbName, g_szDbLogin, g_szDbPassword, g_szDbSchema, errorText);
      if (hdb == NULL)
      {
         nxlog_write(MSG_DB_CONNFAIL, EVENTLOG_ERROR_TYPE, "s", errorText);
         return THREAD_OK;
      }
   }
   else
   {
      hdb = g_hCoreDB;
   }

   while(1)
   {
      DELAYED_SQL_REQUEST *rq = (DELAYED_SQL_REQUEST *)g_pLazyRequestQueue->GetOrBlock();
      if (rq == INVALID_POINTER_VALUE)   // End-of-job indicator
         break;

		if (rq->bindCount == 0)
		{
			DBQuery(hdb, rq->query);
		}
		else
		{
			DB_STATEMENT hStmt = DBPrepare(hdb, rq->query);
			if (hStmt != NULL)
			{
				for(int i = 0; i < rq->bindCount; i++)
				{
					DBBind(hStmt, i + 1, (int)rq->sqlTypes[i], rq->bindings[i], DB_BIND_STATIC);
				}
				DBExecute(hStmt);
				DBFreeStatement(hStmt);
			}
		}
      free(rq);
   }

   if (g_dwFlags & AF_ENABLE_MULTIPLE_DB_CONN)
   {
      DBDisconnect(hdb);
   }
   return THREAD_OK;
}


//
// Start writer thread
//

void StartDBWriter()
{
   int i;

   if (g_dwFlags & AF_ENABLE_MULTIPLE_DB_CONN)
   {
      m_iNumWriters = ConfigReadInt(_T("NumberOfDatabaseWriters"), 1);
      if (m_iNumWriters < 1)
         m_iNumWriters = 1;
      if (m_iNumWriters > MAX_DB_WRITERS)
         m_iNumWriters = MAX_DB_WRITERS;
   }

   for(i = 0; i < m_iNumWriters; i++)
      m_hWriteThreadList[i] = ThreadCreateEx(DBWriteThread, 0, NULL);
}


//
// Stop writer thread and wait while all queries will be executed
//

void StopDBWriter()
{
   int i;

   for(i = 0; i < m_iNumWriters; i++)
      g_pLazyRequestQueue->Put(INVALID_POINTER_VALUE);
   for(i = 0; i < m_iNumWriters; i++)
      ThreadJoin(m_hWriteThreadList[i]);
}
