/* $Id: pgsql.cpp,v 1.4 2005-06-08 08:25:33 victor Exp $ */
/* 
** PostgreSQL Database Driver
** Copyright (C) 2003, 2005 Victor Kirhenshtein and Alex Kirhenshtein
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
** $module: pgsql.cpp
**
**/

#include "pgsqldrv.h"


//
// Static data
//

static MUTEX m_hQueryLock;
static PGresult *m_FetchBuffer = NULL;


//
// Initialize driver
//

extern "C" BOOL EXPORT DrvInit(char *szCmdLine)
{
	m_hQueryLock = MutexCreate();

	return TRUE;
}

//
// Unload handler
//

extern "C" void EXPORT DrvUnload(void)
{
	MutexDestroy(m_hQueryLock);
}


//
// Connect to database
//

extern "C" DB_HANDLE EXPORT DrvConnect(char *szHost, char *szLogin, char *szPassword, char *szDatabase)
{
	PGconn *pConn;
   
	// should be replaced with PQconnectdb();
   pConn = PQsetdbLogin(szHost, NULL, NULL, NULL, 
                        (szDatabase != NULL) ? szDatabase : "template1",
                        szLogin, szPassword);

	if (PQstatus(pConn) == CONNECTION_BAD)
      return 0;

   return (DB_HANDLE)pConn;
}


//
// Disconnect from database
//

extern "C" void EXPORT DrvDisconnect(DB_HANDLE hConn)
{
   PQfinish((PGconn *)hConn);
}


//
// Perform non-SELECT query
//

static BOOL UnsafeDrvQuery(DB_HANDLE hConn, char *szQuery)
{
	PGresult	*pResult;

	pResult = PQexec((PGconn *)hConn, szQuery);

	if (pResult == NULL)
	{
		return FALSE;
	}

	if (PQresultStatus(pResult) != PGRES_COMMAND_OK)
	{
		PQclear(pResult);
		return FALSE;
	}

	PQclear(pResult);
   return TRUE;
}

extern "C" BOOL EXPORT DrvQuery(DB_HANDLE hConn, char *szQuery)
{
	BOOL bRet;

	MutexLock(m_hQueryLock, INFINITE);
	bRet = UnsafeDrvQuery(hConn, szQuery);
	MutexUnlock(m_hQueryLock);

	return bRet;
}


//
// Perform SELECT query
//

static DB_RESULT UnsafeDrvSelect(DB_HANDLE hConn, char *szQuery)
{
	PGresult	*pResult;

	pResult = PQexec((PGconn *)hConn, szQuery);

	if (pResult == NULL)
	{
		return NULL;
	}

	if ((PQresultStatus(pResult) != PGRES_COMMAND_OK) &&
	    (PQresultStatus(pResult) != PGRES_TUPLES_OK))
	{
		PQclear(pResult);
		return NULL;
	}

   return (DB_RESULT)pResult;
}

extern "C" DB_RESULT EXPORT DrvSelect(DB_HANDLE hConn, char *szQuery)
{
	DB_RESULT pResult;

	MutexLock(m_hQueryLock, INFINITE);
	pResult = UnsafeDrvSelect(hConn, szQuery);
	MutexUnlock(m_hQueryLock);

   return pResult;
}


//
// Get field value from result
//

extern "C" char EXPORT *DrvGetField(DB_RESULT hResult, int iRow, int nColumn)
{
   return PQgetvalue((PGresult *)hResult, iRow, nColumn);
}


//
// Get number of rows in result
//

extern "C" int EXPORT DrvGetNumRows(DB_RESULT hResult)
{
   return PQntuples((PGresult *)hResult);
}


//
// Free SELECT results
//

extern "C" void EXPORT DrvFreeResult(DB_RESULT hResult)
{
   PQclear((PGresult *)hResult);
}


//
// Perform asynchronous SELECT query
//

extern "C" DB_ASYNC_RESULT EXPORT DrvAsyncSelect(DB_HANDLE hConn, char *szQuery)
{
	BOOL bStatus = FALSE;

	MutexLock(m_hQueryLock, INFINITE);

	if (UnsafeDrvQuery(hConn, "BEGIN"))
	{
		char *szReq = (char *)malloc(strlen(szQuery) + strlen("DECLARE cur1 CURSOR FOR ") + 1);
		if (szReq != NULL)
		{
			strcpy(szReq, "DECLARE cur1 CURSOR FOR ");
			strcat(szReq, szQuery);
			if (UnsafeDrvQuery(hConn, szReq))
			{
				bStatus = TRUE;
			}
			free(szReq);
		}
	}

	if (bStatus != TRUE)
	{
		UnsafeDrvQuery(hConn, "ROLLBACK");
		MutexUnlock(m_hQueryLock);
		return NULL;
	}

	return (DB_ASYNC_RESULT)hConn;
}


//
// Fetch next result line from asynchronous SELECT results
//

extern "C" BOOL EXPORT DrvFetch(DB_ASYNC_RESULT hResult)
{
   BOOL bResult = TRUE;

	m_FetchBuffer = NULL;
   
   if (hResult == NULL)
   {
      bResult = FALSE;
   }
   else
   {
		m_FetchBuffer = (PGresult *)UnsafeDrvSelect((DB_HANDLE)hResult, "FETCH cur1");
		if (m_FetchBuffer == NULL || DrvGetNumRows(m_FetchBuffer) <= 0)
		{
			bResult = FALSE;
		}
   }
   return bResult;
}


//
// Get field from current row in async query result
//

extern "C" char EXPORT *DrvGetFieldAsync(DB_ASYNC_RESULT hResult, int nColumn, char *pBuffer, int nBufSize)
{
   int nLen;
	char *szResult;

   // Check if we have valid result handle
   if (m_FetchBuffer == NULL)
      return NULL;

	// validate column index
	if (nColumn > PQnfields(m_FetchBuffer))
	{
		return NULL;
	}

	// FIXME: correct processing of binaries fields
	if (PQfformat(m_FetchBuffer, nColumn) != 0)
	{
		fprintf(stderr, "db:postgres:binary fields not supported\n");
		fflush(stderr);
		// abort();
		return NULL;
	}

	szResult = PQgetvalue(m_FetchBuffer, 0, nColumn);
	if (szResult == NULL)
	{
		return NULL;
	}

   // Now get column data
   nLen = min((int)strlen(szResult), nBufSize - 1);
   if (nLen > 0)
      memcpy(pBuffer, szResult, nLen);
   pBuffer[nLen] = 0;
   
   return pBuffer;
}


//
// Destroy result of async query
//

extern "C" void EXPORT DrvFreeAsyncResult(DB_ASYNC_RESULT hResult)
{
   if (m_FetchBuffer != NULL)
   {
		PQclear(m_FetchBuffer);
		UnsafeDrvQuery((DB_HANDLE)hResult, "CLOSE cur1");
		UnsafeDrvQuery((DB_HANDLE)hResult, "ROLLBACK");
   }
	MutexUnlock(m_hQueryLock);
}


//
// DLL Entry point
//

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   return TRUE;
}

#endif
