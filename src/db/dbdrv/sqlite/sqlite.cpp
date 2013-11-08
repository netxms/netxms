/* 
** SQLite Database Driver
** Copyright (C) 2005-2013 Victor Kirhenshtein
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
** File: sqlite.cpp
**
**/

#include "sqlitedrv.h"

DECLARE_DRIVER_HEADER("SQLITE")

/**
 * Get error message from connection
 */
static void GetErrorMessage(sqlite3 *hdb, WCHAR *errorText)
{
	if (errorText == NULL)
		return;

#if UNICODE_UCS2
	wcsncpy(errorText, (const WCHAR *)sqlite3_errmsg16(hdb), DBDRV_MAX_ERROR_TEXT);
#else
	MultiByteToWideChar(CP_UTF8, 0, sqlite3_errmsg(hdb), -1, errorText, DBDRV_MAX_ERROR_TEXT);
#endif
	errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
	RemoveTrailingCRLFW(errorText);
}

/**
 * Prepare string for using in SQL query - enclose in quotes and escape as needed
 */
extern "C" char EXPORT *DrvPrepareStringA(const char *str)
{
	int len = (int)strlen(str) + 3;   // + two quotes and \0 at the end
	int bufferSize = len + 128;
	char *out = (char *)malloc(bufferSize);
	out[0] = '\'';

	const char *src = str;
	int outPos;
	for(outPos = 1; *src != 0; src++)
	{
		if (*src == '\'')
		{
			len++;
			if (len >= bufferSize)
			{
				bufferSize += 128;
				out = (char *)realloc(out, bufferSize);
			}
			out[outPos++] = '\'';
			out[outPos++] = '\'';
		}
		else
		{
			out[outPos++] = *src;
		}
	}
	out[outPos++] = '\'';
	out[outPos++] = 0;

	return out;
}

/**
 * Prepare string for using in SQL query - enclose in quotes and escape as needed
 */
extern "C" WCHAR EXPORT *DrvPrepareStringW(const WCHAR *str)
{
	int len = (int)wcslen(str) + 3;   // + two quotes and \0 at the end
	int bufferSize = len + 128;
	WCHAR *out = (WCHAR *)malloc(bufferSize * sizeof(WCHAR));
	out[0] = L'\'';

	const WCHAR *src = str;
	int outPos;
	for(outPos = 1; *src != 0; src++)
	{
		if (*src == L'\'')
		{
			len++;
			if (len >= bufferSize)
			{
				bufferSize += 128;
				out = (WCHAR *)realloc(out, bufferSize * sizeof(WCHAR));
			}
			out[outPos++] = L'\'';
			out[outPos++] = L'\'';
		}
		else
		{
			out[outPos++] = *src;
		}
	}
	out[outPos++] = L'\'';
	out[outPos++] = 0;

	return out;
}

/**
 * Initialize driver
 */
extern "C" BOOL EXPORT DrvInit(const char *cmdLine)
{
   return sqlite3_threadsafe() &&	// Fail if SQLite compiled without threading support
		    (sqlite3_initialize() == SQLITE_OK);
}

/**
 * Unload handler
 */
extern "C" void EXPORT DrvUnload()
{
	sqlite3_shutdown();
}

/**
 * Connect to database
 */
extern "C" DBDRV_CONNECTION EXPORT DrvConnect(const char *host, const char *login,
                                              const char *password, const char *database, const char *schema,  WCHAR *errorText)
{
   SQLITE_CONN *pConn;
	sqlite3 *hdb;

   if (sqlite3_open_v2(database, &hdb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL) == SQLITE_OK)
   {
      sqlite3_busy_timeout(hdb, 30000);  // 30 sec. busy timeout

		// Create new handle
		pConn = (SQLITE_CONN *)malloc(sizeof(SQLITE_CONN));
		memset(pConn, 0, sizeof(SQLITE_CONN));
		pConn->pdb = hdb;
		pConn->mutexQueryLock = MutexCreate();

      sqlite3_exec(hdb, "PRAGMA foreign_keys = ON", NULL, NULL, NULL);
   }
	else
	{
		GetErrorMessage(hdb, errorText);
		pConn = NULL;
		sqlite3_close(hdb);
	}
   return (DBDRV_CONNECTION)pConn;
}

/**
 * Disconnect from database
 */
extern "C" void EXPORT DrvDisconnect(SQLITE_CONN *hConn)
{
   if (hConn == NULL)
		return;

	sqlite3_close(hConn->pdb);
   MutexDestroy(hConn->mutexQueryLock);
   free(hConn);
}

/**
 * Prepare statement
 */
extern "C" DBDRV_STATEMENT EXPORT DrvPrepare(SQLITE_CONN *hConn, WCHAR *pwszQuery, DWORD *pdwError, WCHAR *errorText)
{
   char *pszQueryUTF8 = UTF8StringFromWideString(pwszQuery);
   MutexLock(hConn->mutexQueryLock);
	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(hConn->pdb, pszQueryUTF8, -1, &stmt, NULL) != SQLITE_OK)
   {
		GetErrorMessage(hConn->pdb, errorText);
		stmt = NULL;
		*pdwError = DBERR_OTHER_ERROR;
   }
   MutexUnlock(hConn->mutexQueryLock);
   free(pszQueryUTF8);
   return stmt;
}

/**
 * Bind parameter to statement
 */
extern "C" void EXPORT DrvBind(sqlite3_stmt *stmt, int pos, int sqlType, int cType, void *buffer, int allocType)
{
	switch(cType)
	{
		case DB_CTYPE_STRING:
#if UNICODE_UCS2
			sqlite3_bind_text16(stmt, pos, buffer, (int)wcslen((WCHAR *)buffer) * sizeof(WCHAR), 
				(allocType == DB_BIND_STATIC) ? SQLITE_STATIC : ((allocType == DB_BIND_DYNAMIC) ? free : SQLITE_TRANSIENT));
#else
			{
				char *utf8String = UTF8StringFromWideString((WCHAR *)buffer);
				sqlite3_bind_text(stmt, pos, utf8String, strlen(utf8String), free);
				if (allocType == DB_BIND_DYNAMIC)
					safe_free(buffer);
			}
#endif
			break;
		case DB_CTYPE_INT32:
		case DB_CTYPE_UINT32:
			sqlite3_bind_int(stmt, pos, *((int *)buffer));
			if (allocType == DB_BIND_DYNAMIC)
				safe_free(buffer);
			break;
		case DB_CTYPE_INT64:
		case DB_CTYPE_UINT64:
			sqlite3_bind_int64(stmt, pos, *((sqlite3_int64 *)buffer));
			if (allocType == DB_BIND_DYNAMIC)
				safe_free(buffer);
			break;
		case DB_CTYPE_DOUBLE:
			sqlite3_bind_double(stmt, pos, *((double *)buffer));
			if (allocType == DB_BIND_DYNAMIC)
				safe_free(buffer);
			break;
		default:
			if (allocType == DB_BIND_DYNAMIC)
				safe_free(buffer);
			break;
	}
}

/**
 * Execute prepared statement
 */
extern "C" DWORD EXPORT DrvExecute(SQLITE_CONN *hConn, sqlite3_stmt *stmt, WCHAR *errorText)
{
	DWORD result;

	MutexLock(hConn->mutexQueryLock);
	if (sqlite3_reset(stmt) == SQLITE_OK)
	{
		int rc = sqlite3_step(stmt);
		if ((rc == SQLITE_DONE) || (rc ==SQLITE_ROW))
		{
			result = DBERR_SUCCESS;
		}
		else
		{
			GetErrorMessage(hConn->pdb, errorText);
			result = DBERR_OTHER_ERROR;
		}
	}
	else
	{
		GetErrorMessage(hConn->pdb, errorText);
		result = DBERR_OTHER_ERROR;
	}
	MutexUnlock(hConn->mutexQueryLock);
	return result;
}

/**
 * Destroy prepared statement
 */
extern "C" void EXPORT DrvFreeStatement(sqlite3_stmt *stmt)
{
   if (stmt != NULL)
	   sqlite3_finalize(stmt);
}

/**
 * Internal query
 */
static DWORD DrvQueryInternal(SQLITE_CONN *pConn, const char *pszQuery, WCHAR *errorText)
{
   DWORD result;

   MutexLock(pConn->mutexQueryLock);
   if (sqlite3_exec(pConn->pdb, pszQuery, NULL, NULL, NULL) == SQLITE_OK)
	{
		result = DBERR_SUCCESS;
	}
	else
	{
		GetErrorMessage(pConn->pdb, errorText);
		result = DBERR_OTHER_ERROR;
	}
   MutexUnlock(pConn->mutexQueryLock);
   return result;
}

/**
 * Perform non-SELECT query
 */
extern "C" DWORD EXPORT DrvQuery(SQLITE_CONN *pConn, WCHAR *pwszQuery, WCHAR *errorText)
{
   DWORD dwResult;
   char *pszQueryUTF8;

   pszQueryUTF8 = UTF8StringFromWideString(pwszQuery);
   dwResult = DrvQueryInternal(pConn, pszQueryUTF8, errorText);
   free(pszQueryUTF8);
   return dwResult;
}

/**
 * SELECT callback
 */
static int SelectCallback(void *pArg, int nCols, char **ppszData, char **ppszNames)
{
   int i, nPos, nMaxCol;

   if (((SQLITE_RESULT *)pArg)->nCols == 0)
   {
      ((SQLITE_RESULT *)pArg)->nCols = nCols;
      nMaxCol = nCols;
   }
   else
   {
      nMaxCol = min(((SQLITE_RESULT *)pArg)->nCols, nCols);
   }

	// Store column names
	if ((((SQLITE_RESULT *)pArg)->ppszNames == NULL) && (nCols > 0) && (ppszNames != NULL))
	{
		((SQLITE_RESULT *)pArg)->ppszNames = (char **)malloc(sizeof(char *) * nCols);
		for(i = 0; i < nCols; i++)
			((SQLITE_RESULT *)pArg)->ppszNames[i] = strdup(ppszNames[i]);
	}

   nPos = ((SQLITE_RESULT *)pArg)->nRows * ((SQLITE_RESULT *)pArg)->nCols;
   ((SQLITE_RESULT *)pArg)->nRows++;
   ((SQLITE_RESULT *)pArg)->ppszData = (char **)realloc(((SQLITE_RESULT *)pArg)->ppszData,
         sizeof(char *) * ((SQLITE_RESULT *)pArg)->nCols * ((SQLITE_RESULT *)pArg)->nRows);

   for(i = 0; i < nMaxCol; i++, nPos++)
      ((SQLITE_RESULT *)pArg)->ppszData[nPos] = strdup(CHECK_NULL_EX_A(ppszData[i]));
   for(; i < ((SQLITE_RESULT *)pArg)->nCols; i++, nPos++)
      ((SQLITE_RESULT *)pArg)->ppszData[nPos] = strdup("");
   return 0;
}

/**
 * Free SELECT results
 */
extern "C" void EXPORT DrvFreeResult(SQLITE_RESULT *hResult)
{
   int i, nCount;

   if (hResult != NULL)
   {
      if (hResult->ppszData != NULL)
      {
         nCount = hResult->nRows * hResult->nCols;
         for(i = 0; i < nCount; i++)
            safe_free(hResult->ppszData[i]);
         free(hResult->ppszData);

         for(i = 0; i < hResult->nCols; i++)
            safe_free(hResult->ppszNames[i]);
         free(hResult->ppszNames);
      }
      free(hResult);
   }
}

/**
 * Perform SELECT query
 */
extern "C" DBDRV_RESULT EXPORT DrvSelect(SQLITE_CONN *hConn, WCHAR *pwszQuery, DWORD *pdwError, WCHAR *errorText)
{
   char *pszQueryUTF8 = UTF8StringFromWideString(pwszQuery);

	SQLITE_RESULT *result = (SQLITE_RESULT *)malloc(sizeof(SQLITE_RESULT));
   memset(result, 0, sizeof(SQLITE_RESULT));

	MutexLock(hConn->mutexQueryLock);
   if (sqlite3_exec(hConn->pdb, pszQueryUTF8, SelectCallback, result, NULL) != SQLITE_OK)
   {
		GetErrorMessage(hConn->pdb, errorText);
		DrvFreeResult(result);
		result = NULL;
   }
   MutexUnlock(hConn->mutexQueryLock);

	free(pszQueryUTF8);
   *pdwError = (result != NULL) ? DBERR_SUCCESS : DBERR_OTHER_ERROR;
   return result;
}

/**
 * Perform SELECT query using prepared statement
 */
extern "C" DBDRV_RESULT EXPORT DrvSelectPrepared(SQLITE_CONN *hConn, sqlite3_stmt *stmt, DWORD *pdwError, WCHAR *errorText)
{
   SQLITE_RESULT *result = (SQLITE_RESULT *)malloc(sizeof(SQLITE_RESULT));
   memset(result, 0, sizeof(SQLITE_RESULT));

   MutexLock(hConn->mutexQueryLock);
	if (sqlite3_reset(stmt) == SQLITE_OK)
	{
		int nCols = sqlite3_column_count(stmt);
		char **cnames = (char **)malloc(sizeof(char *) * nCols * 2);	// column names + values
		char **values = &cnames[nCols];
		bool firstRow = true;
		while(1)
		{
			int rc = sqlite3_step(stmt);
			if (((rc == SQLITE_DONE) || (rc ==SQLITE_ROW)) && firstRow)
			{
				firstRow = false;
				for(int i = 0; i < nCols; i++)
					cnames[i] = (char *)sqlite3_column_name(stmt, i);
			}

			if (rc == SQLITE_ROW)
			{
				for(int i = 0; i < nCols; i++)
					values[i] = (char *)sqlite3_column_text(stmt, i);
				SelectCallback(result, nCols, values, cnames);
			}
			else if (rc == SQLITE_DONE)
			{
				*pdwError = DBERR_SUCCESS;
				break;
			}
			else
			{
				GetErrorMessage(hConn->pdb, errorText);
				*pdwError = DBERR_OTHER_ERROR;
				break;
			}
		}
	}
	else
	{
		GetErrorMessage(hConn->pdb, errorText);
		*pdwError = DBERR_OTHER_ERROR;
	}
	MutexUnlock(hConn->mutexQueryLock);

	if (*pdwError != DBERR_SUCCESS)
	{
		DrvFreeResult(result);
		result = NULL;
	}

	return result;
}

/**
 * Get field length from result
 */
extern "C" LONG EXPORT DrvGetFieldLength(DBDRV_RESULT hResult, int iRow, int iColumn)
{
   if ((iRow < ((SQLITE_RESULT *)hResult)->nRows) &&
       (iColumn < ((SQLITE_RESULT *)hResult)->nCols) &&
       (iRow >= 0) && (iColumn >= 0))
      return (LONG)strlen(((SQLITE_RESULT *)hResult)->ppszData[iRow * ((SQLITE_RESULT *)hResult)->nCols + iColumn]);
   return -1;
}

/**
 * Get field value from result
 */
extern "C" WCHAR EXPORT *DrvGetField(DBDRV_RESULT hResult, int iRow, int iColumn,
                                     WCHAR *pwszBuffer, int nBufLen)
{
   if ((iRow < ((SQLITE_RESULT *)hResult)->nRows) &&
       (iColumn < ((SQLITE_RESULT *)hResult)->nCols) &&
       (iRow >= 0) && (iColumn >= 0))
   {
      MultiByteToWideChar(CP_UTF8, 0, ((SQLITE_RESULT *)hResult)->ppszData[iRow * ((SQLITE_RESULT *)hResult)->nCols + iColumn],
                          -1, pwszBuffer, nBufLen);
      pwszBuffer[nBufLen - 1] = 0;
      return pwszBuffer;
   }
   return NULL;
}

/**
 * Get number of rows in result
 */
extern "C" int EXPORT DrvGetNumRows(SQLITE_RESULT *hResult)
{
   return hResult->nRows;
}

/**
 * Get column count in query result
 */
extern "C" int EXPORT DrvGetColumnCount(SQLITE_RESULT *hResult)
{
	return (hResult != NULL) ? hResult->nCols : 0;
}

/**
 * Get column name in query result
 */
extern "C" const char EXPORT *DrvGetColumnName(SQLITE_RESULT *hResult, int column)
{
   char *pszRet = NULL;

   if ((column >= 0) && (column < hResult->nCols))
   {	
		pszRet = hResult->ppszNames[column];
   }
   return pszRet;
}

/**
 * Perform asynchronous SELECT query
 */
extern "C" DBDRV_ASYNC_RESULT EXPORT DrvAsyncSelect(SQLITE_CONN *hConn, WCHAR *pwszQuery, DWORD *pdwError, WCHAR *errorText)
{
   DBDRV_ASYNC_RESULT hResult;
   char *pszQueryUTF8;

   pszQueryUTF8 = UTF8StringFromWideString(pwszQuery);
   MutexLock(hConn->mutexQueryLock);
	if (sqlite3_prepare(hConn->pdb, pszQueryUTF8, -1, &hConn->pvm, NULL) == SQLITE_OK)
   {
      hResult = hConn;
		*pdwError = DBERR_SUCCESS;
   }
   else
   {
		GetErrorMessage(hConn->pdb, errorText);
      MutexUnlock(hConn->mutexQueryLock);
      hResult = NULL;
		*pdwError = DBERR_OTHER_ERROR;
   }
   free(pszQueryUTF8);
   return hResult;
}

/**
 * Fetch next result line from asynchronous SELECT results
 */
extern "C" BOOL EXPORT DrvFetch(DBDRV_ASYNC_RESULT hResult)
{
	if (hResult == NULL)
		return FALSE;

	if (sqlite3_step(((SQLITE_CONN *)hResult)->pvm) == SQLITE_ROW)
	{
		((SQLITE_CONN *)hResult)->nNumCols = sqlite3_column_count(((SQLITE_CONN *)hResult)->pvm);
		return TRUE;
	}
	return FALSE;
}

/**
 * Get field length from async query result
 */
extern "C" LONG EXPORT DrvGetFieldLengthAsync(DBDRV_RESULT hResult, int iColumn)
{
   if ((iColumn >= 0) && (iColumn < ((SQLITE_CONN *)hResult)->nNumCols))
      return (LONG)strlen((char *)sqlite3_column_text(((SQLITE_CONN *)hResult)->pvm, iColumn));
   return 0;
}

/**
 * Get field from current row in async query result
 */
extern "C" WCHAR EXPORT *DrvGetFieldAsync(DBDRV_ASYNC_RESULT hResult, int iColumn, WCHAR *pBuffer, int iBufSize)
{
   char *pszData;
   WCHAR *pwszRet = NULL;

   if ((iColumn >= 0) && (iColumn < ((SQLITE_CONN *)hResult)->nNumCols))
   {
      pszData = (char *)sqlite3_column_text(((SQLITE_CONN *)hResult)->pvm, iColumn);
      if (pszData != NULL)
      {
         MultiByteToWideChar(CP_UTF8, 0, pszData, -1, pBuffer, iBufSize);
         pBuffer[iBufSize - 1] = 0;
         pwszRet = pBuffer;
      }
   }
   return pwszRet;
}

/**
 * Get column count in async query result
 */
extern "C" int EXPORT DrvGetColumnCountAsync(DBDRV_ASYNC_RESULT hResult)
{
	return (hResult != NULL) ? ((SQLITE_CONN *)hResult)->nNumCols : 0;
}

/**
 * Get column name in async query result
 */
extern "C" const char EXPORT *DrvGetColumnNameAsync(DBDRV_ASYNC_RESULT hResult, int column)
{
   const char *pszRet = NULL;

   if ((column >= 0) && (column < ((SQLITE_CONN *)hResult)->nNumCols))
   {
      pszRet = sqlite3_column_name(((SQLITE_CONN *)hResult)->pvm, column);
   }
   return pszRet;
}

/**
 * Destroy result of async query
 */
extern "C" void EXPORT DrvFreeAsyncResult(DBDRV_ASYNC_RESULT hResult)
{
   if (hResult != NULL)
   {
		sqlite3_finalize(((SQLITE_CONN *)hResult)->pvm);
      MutexUnlock(((SQLITE_CONN *)hResult)->mutexQueryLock);
   }
}

/**
 * Begin transaction
 */
extern "C" DWORD EXPORT DrvBegin(SQLITE_CONN *pConn)
{
   return DrvQueryInternal(pConn, "BEGIN", NULL);
}

/**
 * Commit transaction
 */
extern "C" DWORD EXPORT DrvCommit(SQLITE_CONN *pConn)
{
   return DrvQueryInternal(pConn, "COMMIT", NULL);
}

/**
 * Rollback transaction
 */
extern "C" DWORD EXPORT DrvRollback(SQLITE_CONN *pConn)
{
   return DrvQueryInternal(pConn, "ROLLBACK", NULL);
}

#ifdef _WIN32

/**
 * DLL Entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
