/* $Id: pgsql.cpp,v 1.14 2006-08-21 15:11:57 victor Exp $ */
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
// Initialize driver
//

extern "C" BOOL EXPORT DrvInit(char *szCmdLine)
{
	return TRUE;
}

//
// Unload handler
//

extern "C" void EXPORT DrvUnload(void)
{
}


//
// Connect to database
//

extern "C" DB_CONNECTION EXPORT DrvConnect(
		char *szHost,
		char *szLogin,
		char *szPassword,
		char *szDatabase)
{
	PG_CONN *pConn;

	if (szDatabase == NULL || *szDatabase == 0)
	{
		return NULL;
	}

	pConn = (PG_CONN *)malloc(sizeof(PG_CONN));
	if (pConn != NULL)
	{
		// should be replaced with PQconnectdb();
		pConn->pHandle = PQsetdbLogin(szHost, NULL, NULL, NULL, 
				szDatabase, szLogin, szPassword);

		if (PQstatus(pConn->pHandle) == CONNECTION_BAD)
		{
			free(pConn);
			pConn = NULL;
		}
		else
		{
   		pConn->mutexQueryLock = MutexCreate();
         pConn->pFetchBuffer = NULL;
		}
	}

   return (DB_CONNECTION)pConn;
}


//
// Disconnect from database
//

extern "C" void EXPORT DrvDisconnect(DB_CONNECTION pConn)
{
	if (pConn != NULL)
	{
   	PQfinish(((PG_CONN *)pConn)->pHandle);
     	MutexDestroy(((PG_CONN *)pConn)->mutexQueryLock);
      free(pConn);
	}
}


//
// Perform non-SELECT query
//

static BOOL UnsafeDrvQuery(PG_CONN *pConn, char *szQuery)
{
	PGresult	*pResult;

	pResult = PQexec(pConn->pHandle, szQuery);

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

extern "C" DWORD EXPORT DrvQuery(PG_CONN *pConn, char *szQuery)
{
	DWORD dwRet = DBERR_INVALID_HANDLE;

	if (pConn != NULL && szQuery != NULL)
	{
		MutexLock(pConn->mutexQueryLock, INFINITE);
		if (UnsafeDrvQuery(pConn, szQuery))
      {
         dwRet = DBERR_SUCCESS;
      }
      else
      {
         dwRet = (PQstatus(pConn->pHandle) == CONNECTION_BAD) ? DBERR_CONNECTION_LOST : DBERR_OTHER_ERROR;
      }
		MutexUnlock(pConn->mutexQueryLock);
	}

	return dwRet;
}


//
// Perform SELECT query
//

static DB_RESULT UnsafeDrvSelect(PG_CONN *pConn, char *szQuery)
{
	PGresult	*pResult;

	pResult = PQexec(((PG_CONN *)pConn)->pHandle, szQuery);

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

extern "C" DB_RESULT EXPORT DrvSelect(PG_CONN *pConn, char *szQuery, DWORD *pdwError)
{
	DB_RESULT pResult;

   if (pConn == NULL)
   {
      *pdwError = DBERR_INVALID_HANDLE;
      return NULL;
   }

	MutexLock(pConn->mutexQueryLock, INFINITE);
	pResult = UnsafeDrvSelect(pConn, szQuery);
   if (pResult != NULL)
   {
      *pdwError = DBERR_SUCCESS;
   }
   else
   {
      *pdwError = (PQstatus(pConn->pHandle) == CONNECTION_BAD) ? DBERR_CONNECTION_LOST : DBERR_OTHER_ERROR;
   }
	MutexUnlock(pConn->mutexQueryLock);

   return pResult;
}


//
// Get field value from result
//

extern "C" char EXPORT *DrvGetField(DB_RESULT pResult, int nRow, int nColumn)
{
	if (pResult != NULL)
	{
   	return PQgetvalue((PGresult *)pResult, nRow, nColumn);
	}

	return NULL;
}


//
// Get number of rows in result
//

extern "C" int EXPORT DrvGetNumRows(DB_RESULT pResult)
{
	if (pResult != NULL)
	{
   	return PQntuples((PGresult *)pResult);
	}
	
	return 0;
}


//
// Free SELECT results
//

extern "C" void EXPORT DrvFreeResult(DB_RESULT pResult)
{
	if (pResult != NULL)
	{
   	PQclear((PGresult *)pResult);
	}
}


//
// Perform asynchronous SELECT query
//

extern "C" DB_ASYNC_RESULT EXPORT DrvAsyncSelect(PG_CONN *pConn, char *szQuery, DWORD *pdwError)
{
	BOOL bSuccess = FALSE;
   char *pszReq;
   static char szDeclareCursor[] = "DECLARE cur1 CURSOR FOR ";

	if (pConn == NULL)
		return NULL;

	MutexLock(pConn->mutexQueryLock, INFINITE);

	if (UnsafeDrvQuery(pConn, "BEGIN"))
   {
   	pszReq = (char *)malloc(strlen(szQuery) + sizeof(szDeclareCursor));
	   if (pszReq != NULL)
	   {
		   strcpy(pszReq, szDeclareCursor);
		   strcat(pszReq, szQuery);
		   if (UnsafeDrvQuery(pConn, pszReq))
		   {
			   bSuccess = TRUE;
		   }
		   free(pszReq);
	   }

      if (!bSuccess)
         UnsafeDrvQuery(pConn, "ROLLBACK");
   }

	if (bSuccess)
   {
      *pdwError = DBERR_SUCCESS;
   }
   else
	{
      *pdwError = (PQstatus(pConn->pHandle) == CONNECTION_BAD) ? DBERR_CONNECTION_LOST : DBERR_OTHER_ERROR;
		MutexUnlock(pConn->mutexQueryLock);
		return NULL;
	}

	return (DB_ASYNC_RESULT)pConn;
}


//
// Fetch next result line from asynchronous SELECT results
//

extern "C" BOOL EXPORT DrvFetch(DB_ASYNC_RESULT pConn)
{
   BOOL bResult = TRUE;
   
   if (pConn == NULL)
   {
      bResult = FALSE;
   }
   else
   {
      if (((PG_CONN *)pConn)->pFetchBuffer != NULL)
         PQclear(((PG_CONN *)pConn)->pFetchBuffer);
		((PG_CONN *)pConn)->pFetchBuffer =
			(PGresult *)UnsafeDrvSelect((PG_CONN *)pConn, "FETCH cur1");
		if (((PG_CONN *)pConn)->pFetchBuffer != NULL)
      {
         if (DrvGetNumRows(((PG_CONN *)pConn)->pFetchBuffer) <= 0)
		   {
            PQclear(((PG_CONN *)pConn)->pFetchBuffer);
            ((PG_CONN *)pConn)->pFetchBuffer = NULL;
			   bResult = FALSE;
		   }
      }
      else
      {
         bResult = FALSE;
      }
   }
   return bResult;
}


//
// Get field from current row in async query result
//

extern "C" char EXPORT *DrvGetFieldAsync(
		PG_CONN *pConn,
		int nColumn,
		char *pBuffer,
		int nBufSize)
{
	int nLen;
	char *pszResult;

	if ((pConn == NULL) || (pConn->pFetchBuffer == NULL))
	{
		return NULL;
	}

	// validate column index
	if (nColumn >= PQnfields(pConn->pFetchBuffer))
	{
		return NULL;
	}

	// FIXME: correct processing of binary fields
	if (PQfformat(pConn->pFetchBuffer, nColumn) != 0)
	{
		//fprintf(stderr, "db:postgres:binary fields not supported\n");
		//fflush(stderr);
		// abort();
		return NULL;
	}

	pszResult = PQgetvalue(pConn->pFetchBuffer, 0, nColumn);
	if (pszResult == NULL)
	{
		return NULL;
	}

	// Now get column data
	nLen = min((int)strlen(pszResult), nBufSize - 1);
	if (nLen > 0)
		memcpy(pBuffer, pszResult, nLen);
	pBuffer[nLen] = 0;

	return pBuffer;
}


//
// Destroy result of async query
//

extern "C" void EXPORT DrvFreeAsyncResult(PG_CONN *pConn)
{
   if (pConn != NULL)
   {
      if (pConn->pFetchBuffer != NULL)
      {
		   PQclear(pConn->pFetchBuffer);
         pConn->pFetchBuffer = NULL;
      }
		UnsafeDrvQuery(pConn, "CLOSE cur1");
		UnsafeDrvQuery(pConn, "COMMIT");
   }
	MutexUnlock(pConn->mutexQueryLock);
}


//
// Begin transaction
//

extern "C" DWORD EXPORT DrvBegin(PG_CONN *pConn)
{
   DWORD dwResult;

	if (pConn == NULL)
      return DBERR_INVALID_HANDLE;

	MutexLock(pConn->mutexQueryLock, INFINITE);
	if (UnsafeDrvQuery(pConn, "BEGIN"))
   {
      dwResult = DBERR_SUCCESS;
   }
   else
   {
      dwResult = (PQstatus(pConn->pHandle) == CONNECTION_BAD) ? DBERR_CONNECTION_LOST : DBERR_OTHER_ERROR;
   }
	MutexUnlock(pConn->mutexQueryLock);
   return dwResult;
}


//
// Commit transaction
//

extern "C" DWORD EXPORT DrvCommit(PG_CONN *pConn)
{
   BOOL bRet;

	if (pConn == NULL)
      return DBERR_INVALID_HANDLE;

	MutexLock(pConn->mutexQueryLock, INFINITE);
	bRet = UnsafeDrvQuery(pConn, "COMMIT");
	MutexUnlock(pConn->mutexQueryLock);
   return bRet ? DBERR_SUCCESS : DBERR_OTHER_ERROR;
}


//
// Rollback transaction
//

extern "C" DWORD EXPORT DrvRollback(PG_CONN *pConn)
{
   BOOL bRet;

	if (pConn == NULL)
      return DBERR_INVALID_HANDLE;

	MutexLock(pConn->mutexQueryLock, INFINITE);
	bRet = UnsafeDrvQuery(pConn, "ROLLBACK");
	MutexUnlock(pConn->mutexQueryLock);
   return bRet ? DBERR_SUCCESS : DBERR_OTHER_ERROR;
}


//
// DLL Entry point
//

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
