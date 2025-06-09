/* 
** SQLite Database Driver
** Copyright (C) 2005-2025 Victor Kirhenshtein
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

#define DEBUG_TAG _T("db.drv.sqlite")

/**
 * Get error message from SQLite handle
 */
static void GetErrorMessage(sqlite3 *hdb, WCHAR *errorText)
{
	if (errorText == nullptr)
		return;

#if UNICODE_UCS2
	wcslcpy(errorText, (const WCHAR *)sqlite3_errmsg16(hdb), DBDRV_MAX_ERROR_TEXT);
#else
	utf8_to_wchar(sqlite3_errmsg(hdb), -1, errorText, DBDRV_MAX_ERROR_TEXT);
#endif
	errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
	RemoveTrailingCRLFW(errorText);
}

/**
 * Get error message from connection
 */
static inline void GetErrorMessage(DBDRV_CONNECTION connection, WCHAR *errorText)
{
   GetErrorMessage(static_cast<SQLITE_CONN*>(connection)->pdb, errorText);
}

/**
 * Prepare string for using in SQL query - enclose in quotes and escape as needed
 */
static StringBuffer PrepareString(const TCHAR *str, size_t maxSize)
{
   StringBuffer out;
   out.append(_T('\''));
   for(const TCHAR *src = str; (*src != 0) && (maxSize > 0); src++, maxSize--)
   {
      if (*src == _T('\''))
         out.append(_T("''"), 2);
      else
         out.append(*src);
   }
   out.append(_T('\''));
   return out;
}

/**
 * Initialize driver
 */
static bool Initialize(const char *options)
{
   if (!sqlite3_threadsafe() ||	// Fail if SQLite compiled without threading support
		 (sqlite3_initialize() != SQLITE_OK))
      return false;
   sqlite3_config(SQLITE_CONFIG_MEMSTATUS, 0);
   sqlite3_enable_shared_cache(1);
   nxlog_debug_tag(DEBUG_TAG, 1, _T("SQLite version %hs"), sqlite3_libversion());
   return true;
}

/**
 * Unload handler
 */
static void Unload()
{
	sqlite3_shutdown();
}

/**
 * Connect to database
 */
static DBDRV_CONNECTION Connect(const char *host, const char *login, const char *password, const char *database, const char *schema,  WCHAR *errorText)
{
   SQLITE_CONN *pConn;
	sqlite3 *hdb;
   if (sqlite3_open_v2(database, &hdb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) == SQLITE_OK)
   {
      sqlite3_busy_timeout(hdb, 30000);  // 30 sec. busy timeout

		// Create new handle
		pConn = new SQLITE_CONN();
		pConn->pdb = hdb;

      sqlite3_exec(hdb, "PRAGMA foreign_keys = ON", nullptr, nullptr, nullptr);
      sqlite3_exec(hdb, "PRAGMA journal_mode = WAL", nullptr, nullptr, nullptr);
   }
	else
	{
		GetErrorMessage(hdb, errorText);
		pConn = nullptr;
		sqlite3_close(hdb);
	}
   return pConn;
}

/**
 * Disconnect from database
 */
static void Disconnect(DBDRV_CONNECTION connection)
{
	sqlite3_close(static_cast<SQLITE_CONN*>(connection)->pdb);
   delete static_cast<SQLITE_CONN*>(connection);
}

/**
 * Prepare statement
 */
static DBDRV_STATEMENT Prepare(DBDRV_CONNECTION connection, const WCHAR *query, bool optimizeForReuse, uint32_t *errorCode, WCHAR *errorText)
{
   char *pszQueryUTF8 = UTF8StringFromWideString(query);
   static_cast<SQLITE_CONN*>(connection)->mutexQueryLock.lock();
	sqlite3_stmt *stmt;

retry:
   int rc = sqlite3_prepare_v2(static_cast<SQLITE_CONN*>(connection)->pdb, pszQueryUTF8, -1, &stmt, nullptr);
   if ((rc == SQLITE_LOCKED) || (rc == SQLITE_LOCKED_SHAREDCACHE))
   {
      // database locked by another thread, retry in 10 milliseconds
      ThreadSleepMs(10);
      goto retry;
   }
	else if (rc != SQLITE_OK)
   {
		GetErrorMessage(connection, errorText);
		stmt = NULL;
		*errorCode = DBERR_OTHER_ERROR;
   }
   static_cast<SQLITE_CONN*>(connection)->mutexQueryLock.unlock();
   MemFree(pszQueryUTF8);
   return stmt;
}

/**
 * Bind parameter to statement
 */
static void Bind(DBDRV_STATEMENT hStmt, int pos, int sqlType, int cType, void *buffer, int allocType)
{
   auto stmt = static_cast<sqlite3_stmt*>(hStmt);
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
static uint32_t Execute(DBDRV_CONNECTION connection, DBDRV_STATEMENT hStmt, WCHAR *errorText)
{
	uint32_t result;

	static_cast<SQLITE_CONN*>(connection)->mutexQueryLock.lock();
retry:
	int rc = sqlite3_step(static_cast<sqlite3_stmt*>(hStmt));
	if ((rc == SQLITE_DONE) || (rc == SQLITE_ROW))
	{
	   if (sqlite3_reset(static_cast<sqlite3_stmt*>(hStmt)) == SQLITE_OK)
      {
   		result = DBERR_SUCCESS;
      }
      else
	   {
		   GetErrorMessage(connection, errorText);
		   result = DBERR_OTHER_ERROR;
	   }
	}
   else if ((rc == SQLITE_LOCKED) || (rc == SQLITE_LOCKED_SHAREDCACHE))
   {
      // database locked by another thread, retry in 10 milliseconds
      ThreadSleepMs(10);
      sqlite3_reset(static_cast<sqlite3_stmt*>(hStmt));
      goto retry;
   }
	else
	{
		GetErrorMessage(connection, errorText);
		result = DBERR_OTHER_ERROR;

      sqlite3_reset(static_cast<sqlite3_stmt*>(hStmt));
	}
	static_cast<SQLITE_CONN*>(connection)->mutexQueryLock.unlock();
	return result;
}

/**
 * Destroy prepared statement
 */
static void FreeStatement(DBDRV_STATEMENT hStmt)
{
   sqlite3_finalize(static_cast<sqlite3_stmt*>(hStmt));
}

/**
 * Internal query
 */
static uint32_t QueryInternal(SQLITE_CONN *conn, const char *query, WCHAR *errorText)
{
   uint32_t result;

   conn->mutexQueryLock.lock();
retry:
   int rc = sqlite3_exec(conn->pdb, query, nullptr, nullptr, nullptr);
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
		GetErrorMessage(conn->pdb, errorText);
		result = DBERR_OTHER_ERROR;
	}
   conn->mutexQueryLock.unlock();
   return result;
}

/**
 * Perform non-SELECT query
 */
static uint32_t Query(DBDRV_CONNECTION connection, const WCHAR *query, WCHAR *errorText)
{
   char *queryUTF8 = UTF8StringFromWideString(query);
   uint32_t rc = QueryInternal(static_cast<SQLITE_CONN*>(connection), queryUTF8, errorText);
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
static void FreeResult(DBDRV_RESULT hResult)
{
   auto result = static_cast<SQLITE_RESULT*>(hResult);
   if (result->ppszData != nullptr)
   {
      int count = result->nRows * result->nCols;
      for(int i = 0; i < count; i++)
         MemFree(result->ppszData[i]);
      MemFree(result->ppszData);

      for(int i = 0; i < result->nCols; i++)
         MemFree(result->ppszNames[i]);
      MemFree(result->ppszNames);
   }
   MemFree(result);
}

/**
 * Perform SELECT query
 */
static DBDRV_RESULT Select(DBDRV_CONNECTION connection, const WCHAR *query, uint32_t *errorCode, WCHAR *errorText)
{
   char *queryUTF8 = UTF8StringFromWideString(query);

   SQLITE_RESULT *result = MemAllocStruct<SQLITE_RESULT>();

   static_cast<SQLITE_CONN*>(connection)->mutexQueryLock.lock();
retry:
   int rc = sqlite3_exec(static_cast<SQLITE_CONN*>(connection)->pdb, queryUTF8, SelectCallback, result, nullptr);
   if ((rc == SQLITE_LOCKED) || (rc == SQLITE_LOCKED_SHAREDCACHE))
   {
      // database locked by another thread, retry in 10 milliseconds
      ThreadSleepMs(10);
      goto retry;
   }
   else if (rc != SQLITE_OK)
   {
		GetErrorMessage(connection, errorText);
		FreeResult(result);
		result = nullptr;
   }
   static_cast<SQLITE_CONN*>(connection)->mutexQueryLock.unlock();

	MemFree(queryUTF8);
   *errorCode = (result != nullptr) ? DBERR_SUCCESS : DBERR_OTHER_ERROR;
   return result;
}

/**
 * Perform SELECT query using prepared statement
 */
static DBDRV_RESULT SelectPrepared(DBDRV_CONNECTION connection, DBDRV_STATEMENT hStmt, uint32_t *errorCode, WCHAR *errorText)
{
   SQLITE_RESULT *result = MemAllocStruct<SQLITE_RESULT>();

   static_cast<SQLITE_CONN*>(connection)->mutexQueryLock.lock();

   auto stmt = static_cast<sqlite3_stmt*>(hStmt);
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
			*errorCode = DBERR_SUCCESS;
			break;
		}
		else
		{
			GetErrorMessage(connection, errorText);
			*errorCode = DBERR_OTHER_ERROR;
			break;
		}
	}
   MemFree(cnames);

	if (*errorCode == DBERR_SUCCESS)
   {
	   if (sqlite3_reset(stmt) != SQLITE_OK)
	   {
		   GetErrorMessage(connection, errorText);
		   *errorCode = DBERR_OTHER_ERROR;
	   }
   }
   else
   {
      sqlite3_reset(stmt);
   }

	static_cast<SQLITE_CONN*>(connection)->mutexQueryLock.unlock();

	if (*errorCode != DBERR_SUCCESS)
	{
	   FreeResult(result);
		result = nullptr;
	}

	return result;
}

/**
 * Get field length from result
 */
static int32_t GetFieldLength(DBDRV_RESULT hResult, int row, int column)
{
   auto result = static_cast<SQLITE_RESULT*>(hResult);
   if ((row < result->nRows) && (column < result->nCols) && (row >= 0) && (column >= 0))
      return static_cast<int32_t>(strlen(result->ppszData[row * result->nCols + column]));
   return -1;
}

/**
 * Get field value from result
 */
static WCHAR *GetField(DBDRV_RESULT hResult, int row, int column, WCHAR *buffer, int nBufLen)
{
   auto result = static_cast<SQLITE_RESULT*>(hResult);
   if ((row < result->nRows) && (column < result->nCols) && (row >= 0) && (column >= 0))
   {
      utf8_to_wchar(result->ppszData[row * result->nCols + column], -1, buffer, nBufLen);
      buffer[nBufLen - 1] = 0;
      return buffer;
   }
   return nullptr;
}

/**
 * Get field value from result as UTF8 string
 */
static char *GetFieldUTF8(DBDRV_RESULT hResult, int row, int column, char *buffer, int nBufLen)
{
   auto result = static_cast<SQLITE_RESULT*>(hResult);
   if ((row < result->nRows) && (column < result->nCols) && (row >= 0) && (column >= 0))
   {
      strlcpy(buffer, result->ppszData[row * result->nCols + column], nBufLen);
      return buffer;
   }
   return nullptr;
}

/**
 * Get number of rows in result
 */
static int GetNumRows(DBDRV_RESULT hResult)
{
   return static_cast<SQLITE_RESULT*>(hResult)->nRows;
}

/**
 * Get column count in query result
 */
static int GetColumnCount(DBDRV_RESULT hResult)
{
	return static_cast<SQLITE_RESULT*>(hResult)->nCols;
}

/**
 * Get column name in query result
 */
static const char *GetColumnName(DBDRV_RESULT hResult, int column)
{
   return ((column >= 0) && (column < static_cast<SQLITE_RESULT*>(hResult)->nCols)) ? static_cast<SQLITE_RESULT*>(hResult)->ppszNames[column] : nullptr;
}

/**
 * Perform unbuffered SELECT query
 */
static DBDRV_UNBUFFERED_RESULT SelectUnbuffered(DBDRV_CONNECTION connection, const WCHAR *query, uint32_t *errorCode, WCHAR *errorText)
{
   SQLITE_UNBUFFERED_RESULT *result;
   sqlite3_stmt *stmt;

   char *pszQueryUTF8 = UTF8StringFromWideString(query);
   static_cast<SQLITE_CONN*>(connection)->mutexQueryLock.lock();
retry:
   int rc = sqlite3_prepare(static_cast<SQLITE_CONN*>(connection)->pdb, pszQueryUTF8, -1, &stmt, nullptr);
	if (rc == SQLITE_OK)
   {
      result = MemAllocStruct<SQLITE_UNBUFFERED_RESULT>();
      result->connection = static_cast<SQLITE_CONN*>(connection);
      result->stmt = stmt;
      result->prepared = false;
      result->numColumns = -1;
		*errorCode = DBERR_SUCCESS;
   }
   else if ((rc == SQLITE_LOCKED) || (rc == SQLITE_LOCKED_SHAREDCACHE))
   {
      // database locked by another thread, retry in 10 milliseconds
      ThreadSleepMs(10);
      goto retry;
   }
   else
   {
		GetErrorMessage(connection, errorText);
		static_cast<SQLITE_CONN*>(connection)->mutexQueryLock.unlock();
      result = nullptr;
		*errorCode = DBERR_OTHER_ERROR;
   }
   MemFree(pszQueryUTF8);
   return result;
}

/**
 * Perform unbuffered SELECT query using prepared statement
 */
static DBDRV_UNBUFFERED_RESULT SelectPreparedUnbuffered(DBDRV_CONNECTION connection, DBDRV_STATEMENT hStmt, uint32_t *errorCode, WCHAR *errorText)
{
   SQLITE_UNBUFFERED_RESULT *result = MemAllocStruct<SQLITE_UNBUFFERED_RESULT>();
   result->connection = static_cast<SQLITE_CONN*>(connection);
   result->stmt = static_cast<sqlite3_stmt*>(hStmt);
   result->prepared = true;
   result->numColumns = -1;
   *errorCode = DBERR_SUCCESS;
   return result;
}

/**
 * Fetch next result line from asynchronous SELECT results
 */
static bool Fetch(DBDRV_UNBUFFERED_RESULT hResult)
{
   auto result = static_cast<SQLITE_UNBUFFERED_RESULT*>(hResult);
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
static int32_t GetFieldLengthUnbuffered(DBDRV_UNBUFFERED_RESULT hResult, int column)
{
   auto result = static_cast<SQLITE_UNBUFFERED_RESULT*>(hResult);
   if ((column >= 0) && (column < result->numColumns))
   {
      auto v = reinterpret_cast<const char*>(sqlite3_column_text(result->stmt, column));
      return static_cast<int32_t>((v != nullptr) ? strlen(v) : 0);
   }
   return 0;
}

/**
 * Get field from current row in unbuffered query result
 */
static WCHAR *GetFieldUnbuffered(DBDRV_UNBUFFERED_RESULT hResult, int column, WCHAR *buffer, int bufferSize)
{
   auto result = static_cast<SQLITE_UNBUFFERED_RESULT*>(hResult);
   WCHAR *value = nullptr;
   if ((column >= 0) && (column < result->numColumns))
   {
      char *data = (char *)sqlite3_column_text(result->stmt, column);
      if (data != nullptr)
      {
         utf8_to_wchar(data, -1, buffer, bufferSize);
      }
      buffer[bufferSize - 1] = 0;
      value = buffer;
   }
   return value;
}

/**
 * Get field from current row in unbuffered query result as UTF-8 string
 */
static char *GetFieldUnbufferedUTF8(DBDRV_UNBUFFERED_RESULT hResult, int column, char *buffer, int bufferSize)
{
   auto result = static_cast<SQLITE_UNBUFFERED_RESULT*>(hResult);
   char *value = nullptr;
   if ((column >= 0) && (column < result->numColumns))
   {
      char *data = (char *)sqlite3_column_text(result->stmt, column);
      if (data != nullptr)
      {
         strlcpy(buffer, data, bufferSize);
         value = buffer;
      }
   }
   return value;
}

/**
 * Get column count in async query result
 */
static int GetColumnCountUnbuffered(DBDRV_UNBUFFERED_RESULT hResult)
{
   auto result = static_cast<SQLITE_UNBUFFERED_RESULT*>(hResult);
   if (result->numColumns == -1)
      result->numColumns = sqlite3_column_count(result->stmt);
   return result->numColumns;
}

/**
 * Get column name in async query result
 */
static const char *GetColumnNameUnbuffered(DBDRV_UNBUFFERED_RESULT hResult, int column)
{
   auto result = static_cast<SQLITE_UNBUFFERED_RESULT*>(hResult);
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
static void FreeUnbufferedResult(DBDRV_UNBUFFERED_RESULT hResult)
{
   auto result = static_cast<SQLITE_UNBUFFERED_RESULT*>(hResult);
   if (result->prepared)
      sqlite3_reset(result->stmt);
   else
      sqlite3_finalize(result->stmt);
   result->connection->mutexQueryLock.unlock();
   MemFree(result);
}

/**
 * Begin transaction
 */
static uint32_t Begin(DBDRV_CONNECTION connection)
{
   return QueryInternal(static_cast<SQLITE_CONN*>(connection), "BEGIN IMMEDIATE", nullptr);
}

/**
 * Commit transaction
 */
static uint32_t Commit(DBDRV_CONNECTION connection)
{
   return QueryInternal(static_cast<SQLITE_CONN*>(connection), "COMMIT", nullptr);
}

/**
 * Rollback transaction
 */
static uint32_t Rollback(DBDRV_CONNECTION connection)
{
   return QueryInternal(static_cast<SQLITE_CONN*>(connection), "ROLLBACK", nullptr);
}

/**
 * Check if table exist
 */
static int IsTableExist(DBDRV_CONNECTION connection, const WCHAR *name)
{
   WCHAR query[256];
#if HAVE_SWPRINTF
   swprintf(query, 256, L"SELECT count(*) FROM sqlite_master WHERE type='table' AND upper(name)=upper('%ls')", name);
#else
   wcscpy(query, L"SELECT count(*) FROM sqlite_master WHERE type='table' AND upper(name)=upper('");
   wcscat(query, name);
   wcscat(query, L"')");
#endif
   uint32_t error;
   int rc = DBIsTableExist_Failure;
   SQLITE_RESULT *result = static_cast<SQLITE_RESULT*>(Select(connection, query, &error, nullptr));
   if (result != nullptr)
   {
      if ((result->nRows > 0) && (result->nCols > 0))
      {
         rc = (strtol(result->ppszData[0], nullptr, 10) > 0) ? DBIsTableExist_Found : DBIsTableExist_NotFound;
      }
      else
      {
         rc = DBIsTableExist_NotFound;
      }
      FreeResult(result);
   }
   return rc;
}

/**
 * Driver call table
 */
static DBDriverCallTable s_callTable =
{
   Initialize,
   Connect,
   Disconnect,
   nullptr, // SetPrefetchLimit
   Prepare,
   FreeStatement,
   nullptr, // OpenBatch
   nullptr, // NextBatchRow
   Bind,
   Execute,
   Query,
   Select,
   SelectUnbuffered,
   SelectPrepared,
   SelectPreparedUnbuffered,
   Fetch,
   GetFieldLength,
   GetFieldLengthUnbuffered,
   GetField,
   GetFieldUTF8,
   GetFieldUnbuffered,
   GetFieldUnbufferedUTF8,
   GetNumRows,
   FreeResult,
   FreeUnbufferedResult,
   Begin,
   Commit,
   Rollback,
   Unload,
   GetColumnCount,
   GetColumnName,
   GetColumnCountUnbuffered,
   GetColumnNameUnbuffered,
   PrepareString,
   IsTableExist
};

DB_DRIVER_ENTRY_POINT("SQLITE", s_callTable)

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
