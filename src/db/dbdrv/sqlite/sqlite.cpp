/* 
** SQLite Database Driver
** Copyright (C) 2005-2016 Victor Kirhenshtein
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
extern "C" bool EXPORT DrvInit(const char *cmdLine)
{
   if (!sqlite3_threadsafe() ||	// Fail if SQLite compiled without threading support
		 (sqlite3_initialize() != SQLITE_OK))
      return false;
   sqlite3_enable_shared_cache(1);
   nxlog_debug(1, _T("SQLite version %hs"), sqlite3_libversion());
   return true;
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

retry:
   int rc = sqlite3_prepare_v2(hConn->pdb, pszQueryUTF8, -1, &stmt, NULL);
   if ((rc == SQLITE_LOCKED) || (rc == SQLITE_LOCKED_SHAREDCACHE))
   {
      // database locked by another thread, retry in 10 milliseconds
      ThreadSleepMs(10);
      goto retry;
   }
	else if (rc != SQLITE_OK)
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
      case DB_CTYPE_UTF8_STRING:
         sqlite3_bind_text(stmt, pos, (char *)buffer, (int)strlen((char *)buffer),
            (allocType == DB_BIND_STATIC) ? SQLITE_STATIC : ((allocType == DB_BIND_DYNAMIC) ? free : SQLITE_TRANSIENT));
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
retry:
	int rc = sqlite3_step(stmt);
	if ((rc == SQLITE_DONE) || (rc == SQLITE_ROW))
	{
	   if (sqlite3_reset(stmt) == SQLITE_OK)
      {
   		result = DBERR_SUCCESS;
      }
      else
	   {
		   GetErrorMessage(hConn->pdb, errorText);
		   result = DBERR_OTHER_ERROR;
	   }
	}
   else if ((rc == SQLITE_LOCKED) || (rc == SQLITE_LOCKED_SHAREDCACHE))
   {
      // database locked by another thread, retry in 10 milliseconds
      ThreadSleepMs(10);
      sqlite3_reset(stmt);
      goto retry;
   }
	else
	{
		GetErrorMessage(hConn->pdb, errorText);
		result = DBERR_OTHER_ERROR;

      sqlite3_reset(stmt);
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
retry:
   int rc = sqlite3_exec(pConn->pdb, pszQuery, NULL, NULL, NULL);
   if (rc == SQLITE_OK)
	{
		result = DBERR_SUCCESS;
	}
   else if ((rc == SQLITE_LOCKED) || (rc == SQLITE_LOCKED_SHAREDCACHE))
   {
      // database locked by another thread, retry in 10 milliseconds
      ThreadSleepMs(10);
      goto retry;
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
static int SelectCallback(void *arg, int nCols, char **ppszData, char **ppszNames)
{
   int i, nPos, nMaxCol;

   if (static_cast<SQLITE_RESULT*>(arg)->nCols == 0)
   {
      static_cast<SQLITE_RESULT*>(arg)->nCols = nCols;
      nMaxCol = nCols;
   }
   else
   {
      nMaxCol = std::min(static_cast<SQLITE_RESULT*>(arg)->nCols, nCols);
   }

	// Store column names
	if ((static_cast<SQLITE_RESULT*>(arg)->ppszNames == NULL) && (nCols > 0) && (ppszNames != NULL))
	{
		static_cast<SQLITE_RESULT*>(arg)->ppszNames = (char **)malloc(sizeof(char *) * nCols);
		for(i = 0; i < nCols; i++)
			static_cast<SQLITE_RESULT*>(arg)->ppszNames[i] = strdup(ppszNames[i]);
	}

   nPos = static_cast<SQLITE_RESULT*>(arg)->nRows * static_cast<SQLITE_RESULT*>(arg)->nCols;
   static_cast<SQLITE_RESULT*>(arg)->nRows++;
   static_cast<SQLITE_RESULT*>(arg)->ppszData = (char **)realloc(static_cast<SQLITE_RESULT*>(arg)->ppszData,
         sizeof(char *) * static_cast<SQLITE_RESULT*>(arg)->nCols * static_cast<SQLITE_RESULT*>(arg)->nRows);

   for(i = 0; i < nMaxCol; i++, nPos++)
      static_cast<SQLITE_RESULT*>(arg)->ppszData[nPos] = strdup(CHECK_NULL_EX_A(ppszData[i]));
   for(; i < static_cast<SQLITE_RESULT*>(arg)->nCols; i++, nPos++)
      static_cast<SQLITE_RESULT*>(arg)->ppszData[nPos] = strdup("");
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
retry:
   int rc = sqlite3_exec(hConn->pdb, pszQueryUTF8, SelectCallback, result, NULL);
   if ((rc == SQLITE_LOCKED) || (rc == SQLITE_LOCKED_SHAREDCACHE))
   {
      // database locked by another thread, retry in 10 milliseconds
      ThreadSleepMs(10);
      goto retry;
   }
   else if (rc != SQLITE_OK)
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

	int nCols = sqlite3_column_count(stmt);
	char **cnames = (char **)malloc(sizeof(char *) * nCols * 2);	// column names + values
	char **values = &cnames[nCols];
	bool firstRow = true;
	while(1)
	{
		int rc = sqlite3_step(stmt);

      if (firstRow && ((rc == SQLITE_LOCKED) || (rc == SQLITE_LOCKED_SHAREDCACHE)))
      {
         // database locked by another thread, retry in 10 milliseconds
         ThreadSleepMs(10);
         sqlite3_reset(stmt);
         continue;
      }

		if (((rc == SQLITE_DONE) || (rc == SQLITE_ROW)) && firstRow)
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
   safe_free(cnames);

	if (*pdwError == DBERR_SUCCESS)
   {
	   if (sqlite3_reset(stmt) != SQLITE_OK)
	   {
		   GetErrorMessage(hConn->pdb, errorText);
		   *pdwError = DBERR_OTHER_ERROR;
	   }
   }
   else
   {
      sqlite3_reset(stmt);
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
extern "C" WCHAR EXPORT *DrvGetField(DBDRV_RESULT hResult, int iRow, int iColumn, WCHAR *pwszBuffer, int nBufLen)
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
 * Get field value from result as UTF8 string
 */
extern "C" char EXPORT *DrvGetFieldUTF8(DBDRV_RESULT hResult, int iRow, int iColumn, char *buffer, int nBufLen)
{
   if ((iRow < ((SQLITE_RESULT *)hResult)->nRows) &&
       (iColumn < ((SQLITE_RESULT *)hResult)->nCols) &&
       (iRow >= 0) && (iColumn >= 0))
   {
      strncpy(buffer, ((SQLITE_RESULT *)hResult)->ppszData[iRow * ((SQLITE_RESULT *)hResult)->nCols + iColumn], nBufLen);
      buffer[nBufLen - 1] = 0;
      return buffer;
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
 * Perform unbuffered SELECT query
 */
extern "C" DBDRV_UNBUFFERED_RESULT EXPORT DrvSelectUnbuffered(SQLITE_CONN *hConn, WCHAR *pwszQuery, DWORD *pdwError, WCHAR *errorText)
{
   SQLITE_UNBUFFERED_RESULT *result;
   char *pszQueryUTF8;
   sqlite3_stmt *stmt;

   pszQueryUTF8 = UTF8StringFromWideString(pwszQuery);
   MutexLock(hConn->mutexQueryLock);
retry:
   int rc = sqlite3_prepare(hConn->pdb, pszQueryUTF8, -1, &stmt, NULL);
	if (rc == SQLITE_OK)
   {
      result = (SQLITE_UNBUFFERED_RESULT *)malloc(sizeof(SQLITE_UNBUFFERED_RESULT));
      result->connection = hConn;
      result->stmt = stmt;
      result->prepared = false;
		*pdwError = DBERR_SUCCESS;
   }
   else if ((rc == SQLITE_LOCKED) || (rc == SQLITE_LOCKED_SHAREDCACHE))
   {
      // database locked by another thread, retry in 10 milliseconds
      ThreadSleepMs(10);
      goto retry;
   }
   else
   {
		GetErrorMessage(hConn->pdb, errorText);
      MutexUnlock(hConn->mutexQueryLock);
      result = NULL;
		*pdwError = DBERR_OTHER_ERROR;
   }
   free(pszQueryUTF8);
   return result;
}

/**
 * Perform unbuffered SELECT query using prepared statement
 */
extern "C" DBDRV_UNBUFFERED_RESULT EXPORT DrvSelectPreparedUnbuffered(SQLITE_CONN *hConn, sqlite3_stmt *stmt, DWORD *pdwError, WCHAR *errorText)
{
   if ((hConn == NULL) || (stmt == NULL))
      return NULL;

   SQLITE_UNBUFFERED_RESULT *result = (SQLITE_UNBUFFERED_RESULT *)malloc(sizeof(SQLITE_UNBUFFERED_RESULT));
   result->connection = hConn;
   result->stmt = stmt;
   result->prepared = true;
   *pdwError = DBERR_SUCCESS;
   return result;
}

/**
 * Fetch next result line from asynchronous SELECT results
 */
extern "C" bool EXPORT DrvFetch(SQLITE_UNBUFFERED_RESULT *result)
{
	if (result == NULL)
		return false;

retry:
   int rc = sqlite3_step(result->stmt);
	if (rc == SQLITE_ROW)
	{
		result->numColumns = sqlite3_column_count(result->stmt);
		return true;
	}
   else if ((rc == SQLITE_LOCKED) || (rc == SQLITE_LOCKED_SHAREDCACHE))
   {
      // database locked by another thread, retry in 10 milliseconds
      ThreadSleepMs(10);
      sqlite3_reset(result->stmt);
      goto retry;
   }
	return false;
}

/**
 * Get field length from unbuffered query result
 */
extern "C" LONG EXPORT DrvGetFieldLengthUnbuffered(SQLITE_UNBUFFERED_RESULT *result, int iColumn)
{
   if ((iColumn >= 0) && (iColumn < result->numColumns))
      return (LONG)strlen((char *)sqlite3_column_text(result->stmt, iColumn));
   return 0;
}

/**
 * Get field from current row in unbuffered query result
 */
extern "C" WCHAR EXPORT *DrvGetFieldUnbuffered(SQLITE_UNBUFFERED_RESULT *result, int iColumn, WCHAR *pBuffer, int iBufSize)
{
   char *pszData;
   WCHAR *pwszRet = NULL;

   if ((iColumn >= 0) && (iColumn < result->numColumns))
   {
      pszData = (char *)sqlite3_column_text(result->stmt, iColumn);
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
 * Get field from current row in unbuffered query result as UTF-8 string
 */
extern "C" char EXPORT *DrvGetFieldUnbufferedUTF8(SQLITE_UNBUFFERED_RESULT *result, int iColumn, char *pBuffer, int iBufSize)
{
   char *pszData;
   char *value = NULL;

   if ((iColumn >= 0) && (iColumn < result->numColumns))
   {
      pszData = (char *)sqlite3_column_text(result->stmt, iColumn);
      if (pszData != NULL)
      {
         strncpy(pBuffer, pszData, iBufSize);
         pBuffer[iBufSize - 1] = 0;
         value = pBuffer;
      }
   }
   return value;
}

/**
 * Get column count in async query result
 */
extern "C" int EXPORT DrvGetColumnCountUnbuffered(SQLITE_UNBUFFERED_RESULT *result)
{
	return (result != NULL) ? result->numColumns : 0;
}

/**
 * Get column name in async query result
 */
extern "C" const char EXPORT *DrvGetColumnNameUnbuffered(SQLITE_UNBUFFERED_RESULT *result, int column)
{
   const char *pszRet = NULL;

   if ((column >= 0) && (column < result->numColumns))
   {
      pszRet = sqlite3_column_name(result->stmt, column);
   }
   return pszRet;
}

/**
 * Destroy result of async query
 */
extern "C" void EXPORT DrvFreeUnbufferedResult(SQLITE_UNBUFFERED_RESULT *result)
{
   if (result != NULL)
   {
      if (result->prepared)
         sqlite3_reset(result->stmt);
      else
         sqlite3_finalize(result->stmt);
      MutexUnlock(result->connection->mutexQueryLock);
      free(result);
   }
}

/**
 * Begin transaction
 */
extern "C" DWORD EXPORT DrvBegin(SQLITE_CONN *pConn)
{
   return DrvQueryInternal(pConn, "BEGIN IMMEDIATE", NULL);
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

/**
 * Check if table exist
 */
extern "C" int EXPORT DrvIsTableExist(SQLITE_CONN *pConn, const WCHAR *name)
{
   WCHAR query[256];
#if HAVE_SWPRINTF
   swprintf(query, 256, L"SELECT count(*) FROM sqlite_master WHERE type='table' AND upper(name)=upper('%ls')", name);
#else
   wcscpy(query, L"SELECT count(*) FROM sqlite_master WHERE type='table' AND upper(name)=upper('");
   wcscat(query, name);
   wcscat(query, L"')");
#endif
   DWORD error;
   WCHAR errorText[DBDRV_MAX_ERROR_TEXT];
   int rc = DBIsTableExist_Failure;
   SQLITE_RESULT *hResult = (SQLITE_RESULT *)DrvSelect(pConn, query, &error, errorText);
   if (hResult != NULL)
   {
      WCHAR buffer[64] = L"";
      DrvGetField(hResult, 0, 0, buffer, 64);
      rc = (wcstol(buffer, NULL, 10) > 0) ? DBIsTableExist_Found : DBIsTableExist_NotFound;
      DrvFreeResult(hResult);
   }
   return rc;
}

#ifdef _WIN32

/**
 * DLL Entry point
 */
bool WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return true;
}

#endif
