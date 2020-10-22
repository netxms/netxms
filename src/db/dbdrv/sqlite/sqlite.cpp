/* 
** SQLite Database Driver
** Copyright (C) 2005-2020 Victor Kirhenshtein
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
	if (errorText == nullptr)
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
extern "C" char __EXPORT *DrvPrepareStringA(const char *str)
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
extern "C" WCHAR __EXPORT *DrvPrepareStringW(const WCHAR *str)
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
extern "C" bool __EXPORT DrvInit(const char *cmdLine)
{
   if (!sqlite3_threadsafe() ||	// Fail if SQLite compiled without threading support
		 (sqlite3_initialize() != SQLITE_OK))
      return false;
   sqlite3_config(SQLITE_CONFIG_MEMSTATUS, 0);
   sqlite3_enable_shared_cache(1);
   nxlog_debug(1, _T("SQLite version %hs"), sqlite3_libversion());
   return true;
}

/**
 * Unload handler
 */
extern "C" void __EXPORT DrvUnload()
{
	sqlite3_shutdown();
}

/**
 * Connect to database
 */
extern "C" DBDRV_CONNECTION __EXPORT DrvConnect(const char *host, const char *login,
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
extern "C" void __EXPORT DrvDisconnect(SQLITE_CONN *hConn)
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
extern "C" DBDRV_STATEMENT __EXPORT DrvPrepare(SQLITE_CONN *hConn, WCHAR *pwszQuery, bool optimizeForReuse, DWORD *pdwError, WCHAR *errorText)
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
extern "C" void __EXPORT DrvBind(sqlite3_stmt *stmt, int pos, int sqlType, int cType, void *buffer, int allocType)
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
					MemFree(buffer);
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
				MemFree(buffer);
			break;
		case DB_CTYPE_INT64:
		case DB_CTYPE_UINT64:
			sqlite3_bind_int64(stmt, pos, *((sqlite3_int64 *)buffer));
			if (allocType == DB_BIND_DYNAMIC)
				MemFree(buffer);
			break;
		case DB_CTYPE_DOUBLE:
			sqlite3_bind_double(stmt, pos, *((double *)buffer));
			if (allocType == DB_BIND_DYNAMIC)
				MemFree(buffer);
			break;
		default:
			if (allocType == DB_BIND_DYNAMIC)
				MemFree(buffer);
			break;
	}
}

/**
 * Execute prepared statement
 */
extern "C" DWORD __EXPORT DrvExecute(SQLITE_CONN *hConn, sqlite3_stmt *stmt, WCHAR *errorText)
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
extern "C" void __EXPORT DrvFreeStatement(sqlite3_stmt *stmt)
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
extern "C" DWORD __EXPORT DrvQuery(SQLITE_CONN *conn, WCHAR *query, WCHAR *errorText)
{
   char *queryUTF8 = UTF8StringFromWideString(query);
   DWORD rc = DrvQueryInternal(conn, queryUTF8, errorText);
   MemFree(queryUTF8);
   return rc;
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
	if ((static_cast<SQLITE_RESULT*>(arg)->ppszNames == nullptr) && (nCols > 0) && (ppszNames != nullptr))
	{
		static_cast<SQLITE_RESULT*>(arg)->ppszNames = (char **)malloc(sizeof(char *) * nCols);
		for(i = 0; i < nCols; i++)
			static_cast<SQLITE_RESULT*>(arg)->ppszNames[i] = MemCopyStringA(ppszNames[i]);
	}

   nPos = static_cast<SQLITE_RESULT*>(arg)->nRows * static_cast<SQLITE_RESULT*>(arg)->nCols;
   static_cast<SQLITE_RESULT*>(arg)->nRows++;
   static_cast<SQLITE_RESULT*>(arg)->ppszData = (char **)realloc(static_cast<SQLITE_RESULT*>(arg)->ppszData,
         sizeof(char *) * static_cast<SQLITE_RESULT*>(arg)->nCols * static_cast<SQLITE_RESULT*>(arg)->nRows);

   for(i = 0; i < nMaxCol; i++, nPos++)
      static_cast<SQLITE_RESULT*>(arg)->ppszData[nPos] = MemCopyStringA(CHECK_NULL_EX_A(ppszData[i]));
   for(; i < static_cast<SQLITE_RESULT*>(arg)->nCols; i++, nPos++)
      static_cast<SQLITE_RESULT*>(arg)->ppszData[nPos] = MemCopyStringA("");
   return 0;
}

/**
 * Free SELECT results
 */
static void DrvFreeResultInternal(SQLITE_RESULT *hResult)
{
   if (hResult->ppszData != nullptr)
   {
      int nCount = hResult->nRows * hResult->nCols;
      for(int i = 0; i < nCount; i++)
         MemFree(hResult->ppszData[i]);
      MemFree(hResult->ppszData);

      for(int i = 0; i < hResult->nCols; i++)
         MemFree(hResult->ppszNames[i]);
      MemFree(hResult->ppszNames);
   }
   MemFree(hResult);
}

/**
 * Free SELECT results - public entry point
 */
extern "C" void __EXPORT DrvFreeResult(SQLITE_RESULT *hResult)
{
   if (hResult != nullptr)
      DrvFreeResultInternal(hResult);
}

/**
 * Perform SELECT query - actual implementation
 */
static SQLITE_RESULT *DrvSelectInternal(SQLITE_CONN *conn, WCHAR *query, DWORD *errorCode, WCHAR *errorText)
{
   char *queryUTF8 = UTF8StringFromWideString(query);

   SQLITE_RESULT *result = MemAllocStruct<SQLITE_RESULT>();

	MutexLock(conn->mutexQueryLock);
retry:
   int rc = sqlite3_exec(conn->pdb, queryUTF8, SelectCallback, result, NULL);
   if ((rc == SQLITE_LOCKED) || (rc == SQLITE_LOCKED_SHAREDCACHE))
   {
      // database locked by another thread, retry in 10 milliseconds
      ThreadSleepMs(10);
      goto retry;
   }
   else if (rc != SQLITE_OK)
   {
		GetErrorMessage(conn->pdb, errorText);
		DrvFreeResultInternal(result);
		result = nullptr;
   }
   MutexUnlock(conn->mutexQueryLock);

	MemFree(queryUTF8);
   *errorCode = (result != NULL) ? DBERR_SUCCESS : DBERR_OTHER_ERROR;
   return result;
}

/**
 * Perform SELECT query - public entry point
 */
extern "C" DBDRV_RESULT __EXPORT DrvSelect(SQLITE_CONN *conn, WCHAR *query, DWORD *errorCode, WCHAR *errorText)
{
   return DrvSelectInternal(conn, query, errorCode, errorText);
}

/**
 * Perform SELECT query using prepared statement
 */
extern "C" DBDRV_RESULT __EXPORT DrvSelectPrepared(SQLITE_CONN *hConn, sqlite3_stmt *stmt, DWORD *pdwError, WCHAR *errorText)
{
   SQLITE_RESULT *result = MemAllocStruct<SQLITE_RESULT>();

   MutexLock(hConn->mutexQueryLock);

	int nCols = sqlite3_column_count(stmt);
	char **cnames = (char **)malloc(sizeof(char *) * nCols * 2);	// column names + values
	char **values = &cnames[nCols];
	bool firstRow = true;
	while(true)
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
   MemFree(cnames);

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
	   DrvFreeResultInternal(result);
		result = nullptr;
	}

	return result;
}

/**
 * Get field length from result
 */
extern "C" LONG __EXPORT DrvGetFieldLength(SQLITE_RESULT *hResult, int row, int column)
{
   if ((row < hResult->nRows) && (column < hResult->nCols) && (row >= 0) && (column >= 0))
      return (LONG)strlen(hResult->ppszData[row * hResult->nCols + column]);
   return -1;
}

/**
 * Get field value from result
 */
extern "C" WCHAR __EXPORT *DrvGetField(SQLITE_RESULT *hResult, int row, int column, WCHAR *buffer, int nBufLen)
{
   if ((row < hResult->nRows) && (column < hResult->nCols) && (row >= 0) && (column >= 0))
   {
      utf8_to_wchar(hResult->ppszData[row * hResult->nCols + column], -1, buffer, nBufLen);
      buffer[nBufLen - 1] = 0;
      return buffer;
   }
   return nullptr;
}

/**
 * Get field value from result as UTF8 string
 */
extern "C" char __EXPORT *DrvGetFieldUTF8(SQLITE_RESULT *hResult, int row, int column, char *buffer, int nBufLen)
{
   if ((row < hResult->nRows) && (column < hResult->nCols) && (row >= 0) && (column >= 0))
   {
      strlcpy(buffer, hResult->ppszData[row * hResult->nCols + column], nBufLen);
      return buffer;
   }
   return nullptr;
}

/**
 * Get number of rows in result
 */
extern "C" int __EXPORT DrvGetNumRows(SQLITE_RESULT *hResult)
{
   return hResult->nRows;
}

/**
 * Get column count in query result
 */
extern "C" int __EXPORT DrvGetColumnCount(SQLITE_RESULT *hResult)
{
	return (hResult != NULL) ? hResult->nCols : 0;
}

/**
 * Get column name in query result
 */
extern "C" const char __EXPORT *DrvGetColumnName(SQLITE_RESULT *hResult, int column)
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
extern "C" DBDRV_UNBUFFERED_RESULT __EXPORT DrvSelectUnbuffered(SQLITE_CONN *hConn, WCHAR *pwszQuery, DWORD *pdwError, WCHAR *errorText)
{
   SQLITE_UNBUFFERED_RESULT *result;
   sqlite3_stmt *stmt;

   char *pszQueryUTF8 = UTF8StringFromWideString(pwszQuery);
   MutexLock(hConn->mutexQueryLock);
retry:
   int rc = sqlite3_prepare(hConn->pdb, pszQueryUTF8, -1, &stmt, nullptr);
	if (rc == SQLITE_OK)
   {
      result = MemAllocStruct<SQLITE_UNBUFFERED_RESULT>();
      result->connection = hConn;
      result->stmt = stmt;
      result->prepared = false;
      result->numColumns = -1;
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
      result = nullptr;
		*pdwError = DBERR_OTHER_ERROR;
   }
   MemFree(pszQueryUTF8);
   return result;
}

/**
 * Perform unbuffered SELECT query using prepared statement
 */
extern "C" DBDRV_UNBUFFERED_RESULT __EXPORT DrvSelectPreparedUnbuffered(SQLITE_CONN *hConn, sqlite3_stmt *stmt, DWORD *pdwError, WCHAR *errorText)
{
   if ((hConn == nullptr) || (stmt == nullptr))
      return nullptr;

   SQLITE_UNBUFFERED_RESULT *result = MemAllocStruct<SQLITE_UNBUFFERED_RESULT>();
   result->connection = hConn;
   result->stmt = stmt;
   result->prepared = true;
   result->numColumns = -1;
   *pdwError = DBERR_SUCCESS;
   return result;
}

/**
 * Fetch next result line from asynchronous SELECT results
 */
extern "C" bool __EXPORT DrvFetch(SQLITE_UNBUFFERED_RESULT *result)
{
	if (result == nullptr)
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
extern "C" LONG __EXPORT DrvGetFieldLengthUnbuffered(SQLITE_UNBUFFERED_RESULT *result, int column)
{
   if ((column >= 0) && (column < result->numColumns))
   {
      auto v = reinterpret_cast<const char*>(sqlite3_column_text(result->stmt, column));
      return static_cast<LONG>((v != nullptr) ? strlen(v) : 0);
   }
   return 0;
}

/**
 * Get field from current row in unbuffered query result
 */
extern "C" WCHAR __EXPORT *DrvGetFieldUnbuffered(SQLITE_UNBUFFERED_RESULT *result, int iColumn, WCHAR *pBuffer, int iBufSize)
{
   WCHAR *value = nullptr;
   if ((iColumn >= 0) && (iColumn < result->numColumns))
   {
      char *data = (char *)sqlite3_column_text(result->stmt, iColumn);
      if (data != nullptr)
      {
         utf8_to_wchar(data, -1, pBuffer, iBufSize);
         pBuffer[iBufSize - 1] = 0;
         value = pBuffer;
      }
   }
   return value;
}

/**
 * Get field from current row in unbuffered query result as UTF-8 string
 */
extern "C" char __EXPORT *DrvGetFieldUnbufferedUTF8(SQLITE_UNBUFFERED_RESULT *result, int iColumn, char *pBuffer, int iBufSize)
{
   char *value = nullptr;
   if ((iColumn >= 0) && (iColumn < result->numColumns))
   {
      char *data = (char *)sqlite3_column_text(result->stmt, iColumn);
      if (data != nullptr)
      {
         strlcpy(pBuffer, data, iBufSize);
         value = pBuffer;
      }
   }
   return value;
}

/**
 * Get column count in async query result
 */
extern "C" int __EXPORT DrvGetColumnCountUnbuffered(SQLITE_UNBUFFERED_RESULT *result)
{
   if (result == nullptr)
      return 0;
   if (result->numColumns == -1)
      result->numColumns = sqlite3_column_count(result->stmt);
   return result->numColumns;
}

/**
 * Get column name in async query result
 */
extern "C" const char __EXPORT *DrvGetColumnNameUnbuffered(SQLITE_UNBUFFERED_RESULT *result, int column)
{
   if (result == nullptr)
      return nullptr;
   if (result->numColumns == -1)
      result->numColumns = sqlite3_column_count(result->stmt);
   const char *name = nullptr;
   if ((column >= 0) && (column < result->numColumns))
   {
      name = sqlite3_column_name(result->stmt, column);
   }
   return name;
}

/**
 * Destroy result of async query
 */
extern "C" void __EXPORT DrvFreeUnbufferedResult(SQLITE_UNBUFFERED_RESULT *result)
{
   if (result != nullptr)
   {
      if (result->prepared)
         sqlite3_reset(result->stmt);
      else
         sqlite3_finalize(result->stmt);
      MutexUnlock(result->connection->mutexQueryLock);
      MemFree(result);
   }
}

/**
 * Begin transaction
 */
extern "C" DWORD __EXPORT DrvBegin(SQLITE_CONN *conn)
{
   return DrvQueryInternal(conn, "BEGIN IMMEDIATE", nullptr);
}

/**
 * Commit transaction
 */
extern "C" DWORD __EXPORT DrvCommit(SQLITE_CONN *conn)
{
   return DrvQueryInternal(conn, "COMMIT", nullptr);
}

/**
 * Rollback transaction
 */
extern "C" DWORD __EXPORT DrvRollback(SQLITE_CONN *conn)
{
   return DrvQueryInternal(conn, "ROLLBACK", nullptr);
}

/**
 * Check if table exist
 */
extern "C" int __EXPORT DrvIsTableExist(SQLITE_CONN *conn, const WCHAR *name)
{
   if (conn == nullptr)
      return DBIsTableExist_Failure;

   WCHAR query[256];
#if HAVE_SWPRINTF
   swprintf(query, 256, L"SELECT count(*) FROM sqlite_master WHERE type='table' AND upper(name)=upper('%ls')", name);
#else
   wcscpy(query, L"SELECT count(*) FROM sqlite_master WHERE type='table' AND upper(name)=upper('");
   wcscat(query, name);
   wcscat(query, L"')");
#endif
   DWORD error;
   int rc = DBIsTableExist_Failure;
   SQLITE_RESULT *hResult = DrvSelectInternal(conn, query, &error, nullptr);
   if (hResult != nullptr)
   {
      if ((hResult->nRows > 0) && (hResult->nCols > 0))
      {
         rc = (strtol(hResult->ppszData[0], nullptr, 10) > 0) ? DBIsTableExist_Found : DBIsTableExist_NotFound;
      }
      else
      {
         rc = DBIsTableExist_NotFound;
      }
      DrvFreeResultInternal(hResult);
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
