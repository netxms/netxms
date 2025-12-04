/*
** NetXMS - Network Management System
** Database Abstraction Library
** Copyright (C) 2003-2025 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: session.cpp
**
**/

#include "libnxdb.h"

/**
 * Check if statement handle is valid
 */
#define IS_VALID_STATEMENT_HANDLE(s) ((s != nullptr) && (s->m_connection != nullptr) && (s->m_statement != nullptr))

/**
 * Performance counters
 */
static VolatileCounter64 s_perfSelectQueries = 0;
static VolatileCounter64 s_perfNonSelectQueries = 0;
static VolatileCounter64 s_perfTotalQueries = 0;
static VolatileCounter64 s_perfLongRunningQueries = 0;
static VolatileCounter64 s_perfFailedQueries = 0;

/**
 * Query trace flag
 */
static bool s_queryTrace = false;

/**
 * Session init callback
 */
static void (*s_sessionInitCb)(DB_HANDLE session) = nullptr;

/**
 * Invalidate all prepared statements on connection
 */
static void InvalidatePreparedStatements(DB_HANDLE hConn)
{
   hConn->m_preparedStatementsLock.lock();
   for(int i = 0; i < hConn->m_preparedStatements.size(); i++)
   {
      db_statement_t *stmt = hConn->m_preparedStatements.get(i);
      hConn->m_driver->m_callTable.FreeStatement(stmt->m_statement);
      stmt->m_statement = nullptr;
      stmt->m_connection = nullptr;
   }
   hConn->m_preparedStatements.clear();
   hConn->m_preparedStatementsLock.unlock();
}

/**
 * Enable or disable SQL query trace
 */
void LIBNXDB_EXPORTABLE DBEnableQueryTrace(bool enabled)
{
   s_queryTrace = enabled;
}

/**
 * Get SQL query trace state
 */
bool LIBNXDB_EXPORTABLE DBIsQueryTraceEnabled()
{
   return s_queryTrace;
}

/**
 * Connect to database
 */
DB_HANDLE LIBNXDB_EXPORTABLE DBConnect(DB_DRIVER driver, const TCHAR *server, const TCHAR *dbName,
         const TCHAR *login, const TCHAR *password, const TCHAR *schema, TCHAR *errorText)
{
   DB_HANDLE hConn = nullptr;

	nxlog_debug_tag(DEBUG_TAG_CONNECTION, 8, _T("DBConnect: server=%s db=%s login=%s schema=%s"), CHECK_NULL(server), CHECK_NULL(dbName), CHECK_NULL(login), CHECK_NULL(schema));
	char *utfServer = UTF8StringFromTString(server);
	char *utfDatabase = UTF8StringFromTString(dbName);
	char *utfLogin = UTF8StringFromTString(login);
	char *utfPassword = UTF8StringFromTString(password);
	char *utfSchema = UTF8StringFromTString(schema);
#ifdef UNICODE
	errorText[0] = 0;
	DBDRV_CONNECTION hDrvConn = driver->m_callTable.Connect(utfServer, utfLogin, utfPassword, utfDatabase, utfSchema, errorText);
#else
	WCHAR wcErrorText[DBDRV_MAX_ERROR_TEXT] = L"";
	DBDRV_CONNECTION hDrvConn = driver->m_callTable.Connect(utfServer, utfLogin, utfPassword, utfDatabase, utfSchema, wcErrorText);
	wchar_to_mb(wcErrorText, -1, errorText, DBDRV_MAX_ERROR_TEXT);
	errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
#endif
   if (hDrvConn != nullptr)
   {
      hConn = new db_handle_t(driver, hDrvConn, utfDatabase, utfLogin, utfPassword, utfServer, utfSchema);
      if (driver->m_callTable.SetPrefetchLimit != nullptr)
         driver->m_callTable.SetPrefetchLimit(hDrvConn, driver->m_defaultPrefetchLimit);
      nxlog_debug_tag(DEBUG_TAG_CONNECTION, 4, _T("New DB connection opened: handle=%p"), hConn);
      if (s_sessionInitCb != nullptr)
         s_sessionInitCb(hConn);
   }
	if (hConn == nullptr)
	{
		MemFree(utfServer);
		MemFree(utfDatabase);
		MemFree(utfLogin);
		MemFree(utfPassword);
		MemFree(utfSchema);
	}
   return hConn;
}

/**
 * Disconnect from database
 */
void LIBNXDB_EXPORTABLE DBDisconnect(DB_HANDLE hConn)
{
   if (hConn == nullptr)
      return;

   nxlog_debug_tag(DEBUG_TAG_CONNECTION, 4, _T("DB connection %p closed"), hConn);

   InvalidatePreparedStatements(hConn);

	hConn->m_driver->m_callTable.Disconnect(hConn->m_connection);
   delete hConn;
}

/**
 * Enable/disable reconnect
 */
void LIBNXDB_EXPORTABLE DBEnableReconnect(DB_HANDLE hConn, bool enabled)
{
	if (hConn != nullptr)
		hConn->m_reconnectEnabled = enabled;
}

/**
 * Reconnect to database
 */
static void DBReconnect(DB_HANDLE hConn)
{
   int nCount;
	WCHAR errorText[DBDRV_MAX_ERROR_TEXT];

   nxlog_debug_tag(DEBUG_TAG_CONNECTION, 4, _T("DB reconnect: handle=%p"), hConn);

   InvalidatePreparedStatements(hConn);
	hConn->m_driver->m_callTable.Disconnect(hConn->m_connection);
   for(nCount = 0; ; nCount++)
   {
		hConn->m_connection = hConn->m_driver->m_callTable.Connect(hConn->m_server, hConn->m_login,
		         hConn->m_password, hConn->m_dbName, hConn->m_schema, errorText);
      if (hConn->m_connection != nullptr)
      {
         if (hConn->m_driver->m_callTable.SetPrefetchLimit != nullptr)
            hConn->m_driver->m_callTable.SetPrefetchLimit(hConn->m_connection, hConn->m_driver->m_defaultPrefetchLimit);
         if (s_sessionInitCb != nullptr)
            s_sessionInitCb(hConn);
         break;
      }
      if (nCount == 0)
      {
         hConn->m_driver->m_mutexReconnect->lock();
         if ((hConn->m_driver->m_reconnect == 0) && (hConn->m_driver->m_fpEventHandler != nullptr))
				hConn->m_driver->m_fpEventHandler(DBEVENT_CONNECTION_LOST, NULL, NULL, true, hConn->m_driver->m_context);
         hConn->m_driver->m_reconnect++;
         hConn->m_driver->m_mutexReconnect->unlock();
      }
      ThreadSleepMs(1000);
   }
   if (nCount > 0)
   {
      hConn->m_driver->m_mutexReconnect->lock();
      hConn->m_driver->m_reconnect--;
      if ((hConn->m_driver->m_reconnect == 0) && (hConn->m_driver->m_fpEventHandler != nullptr))
			hConn->m_driver->m_fpEventHandler(DBEVENT_CONNECTION_RESTORED, NULL, NULL, false, hConn->m_driver->m_context);
      hConn->m_driver->m_mutexReconnect->unlock();
   }
}

/**
 * Get connection's driver
 */
DB_DRIVER LIBNXDB_EXPORTABLE DBGetDriver(DB_HANDLE hConn)
{
   return hConn->m_driver;
}

/**
 * Set default prefetch limit
 */
void LIBNXDB_EXPORTABLE DBSetDefaultPrefetchLimit(DB_DRIVER driver, int limit)
{
   driver->m_defaultPrefetchLimit = limit;
}

/**
 * Set prefetch limit
 */
bool LIBNXDB_EXPORTABLE DBSetPrefetchLimit(DB_HANDLE hConn, int limit)
{
   if (hConn->m_driver->m_callTable.SetPrefetchLimit == nullptr)
      return false;  // Not supported by driver
   return hConn->m_driver->m_callTable.SetPrefetchLimit(hConn->m_connection, limit);
}

/**
 * Set session initialization callback
 */
void LIBNXDB_EXPORTABLE DBSetSessionInitCallback(void (*cb)(DB_HANDLE))
{
   s_sessionInitCb = cb;
}

/**
 * Perform a non-SELECT SQL query
 */
bool LIBNXDB_EXPORTABLE DBQueryEx(DB_HANDLE hConn, const TCHAR *query, TCHAR *errorText)
{
#ifdef UNICODE
   auto wcQuery = query;
   auto wcErrorText = errorText;
#else
   WCHAR *wcQuery = WideStringFromMBString(query);
	WCHAR wcErrorText[DBDRV_MAX_ERROR_TEXT] = L"";
#endif

   hConn->m_mutexTransLock.lock();
   int64_t ms = GetMonotonicClockTime();

   uint32_t rc = hConn->m_driver->m_callTable.Query(hConn->m_connection, wcQuery, wcErrorText);
   if ((rc == DBERR_CONNECTION_LOST) && hConn->m_reconnectEnabled)
   {
      DBReconnect(hConn);
      rc = hConn->m_driver->m_callTable.Query(hConn->m_connection, wcQuery, wcErrorText);
   }

   s_perfNonSelectQueries++;
   s_perfTotalQueries++;

   ms = GetMonotonicClockTime() - ms;
   if (s_queryTrace)
   {
      nxlog_debug_tag(DEBUG_TAG_QUERY, 9, _T("%s sync query: \"%s\" [%d ms]"), (rc == DBERR_SUCCESS) ? _T("Successful") : _T("Failed"), query, ms);
   }
   if ((rc == DBERR_SUCCESS) && (static_cast<uint32_t>(ms) > hConn->getQueryExecTimeThreshold()))
   {
      nxlog_debug_tag(DEBUG_TAG_QUERY, 3, _T("Long running query: \"%s\" [%d ms]"), query, (int)ms);
      s_perfLongRunningQueries++;
   }
   
   hConn->m_mutexTransLock.unlock();

#ifndef UNICODE
	wchar_to_mb(wcErrorText, -1, errorText, DBDRV_MAX_ERROR_TEXT);
	errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
#endif

   if (rc != DBERR_SUCCESS)
	{	
      s_perfFailedQueries++;
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_DRIVER, _T("SQL query failed (Query = \"%s\"): %s"), query, errorText);
		if (hConn->m_driver->m_fpEventHandler != NULL)
			hConn->m_driver->m_fpEventHandler(DBEVENT_QUERY_FAILED, wcQuery, wcErrorText, rc == DBERR_CONNECTION_LOST, hConn->m_driver->m_context);
	}

#ifndef UNICODE
   MemFree(wcQuery);
#endif

   return rc == DBERR_SUCCESS;
}

bool LIBNXDB_EXPORTABLE DBQuery(DB_HANDLE hConn, const TCHAR *query)
{
   TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
	return DBQueryEx(hConn, query, errorText);
}

/**
 * Perform SELECT query
 */
DB_RESULT LIBNXDB_EXPORTABLE DBSelectEx(DB_HANDLE hConn, const TCHAR *query, TCHAR *errorText)
{
#ifdef UNICODE
   auto wcQuery = query;
   auto wcErrorText = errorText;
#else
   WCHAR *wcQuery = WideStringFromMBString(query);
	WCHAR wcErrorText[DBDRV_MAX_ERROR_TEXT] = L"";
#endif

   hConn->m_mutexTransLock.lock();
   int64_t ms = GetMonotonicClockTime();

   InterlockedIncrement64(&s_perfSelectQueries);
   InterlockedIncrement64(&s_perfTotalQueries);

   uint32_t errorCode = DBERR_OTHER_ERROR;
   DBDRV_RESULT hResult = hConn->m_driver->m_callTable.Select(hConn->m_connection, wcQuery, &errorCode, wcErrorText);
   if ((hResult == nullptr) && (errorCode == DBERR_CONNECTION_LOST) && hConn->m_reconnectEnabled)
   {
      DBReconnect(hConn);
      hResult = hConn->m_driver->m_callTable.Select(hConn->m_connection, wcQuery, &errorCode, wcErrorText);
   }

   ms = GetMonotonicClockTime() - ms;
   if (s_queryTrace)
   {
      nxlog_debug_tag(DEBUG_TAG_QUERY, 9, _T("%s sync query: \"%s\" [%d ms]"), (hResult != nullptr) ? _T("Successful") : _T("Failed"), query, (int)ms);
   }
   if ((hResult != nullptr) && (static_cast<uint32_t>(ms) > hConn->getQueryExecTimeThreshold()))
   {
      nxlog_debug_tag(DEBUG_TAG_QUERY, 3, _T("Long running query: \"%s\" [%d ms]"), query, (int)ms);
      InterlockedIncrement64(&s_perfLongRunningQueries);
   }
   hConn->m_mutexTransLock.unlock();

#ifndef UNICODE
	wchar_to_mb(wcErrorText, -1, errorText, DBDRV_MAX_ERROR_TEXT);
	errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
#endif

	if (hResult == nullptr)
	{
	   InterlockedIncrement64(&s_perfFailedQueries);
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_DRIVER, _T("SQL query failed (Query = \"%s\"): %s"), query, errorText);
		if (hConn->m_driver->m_fpEventHandler != NULL)
			hConn->m_driver->m_fpEventHandler(DBEVENT_QUERY_FAILED, wcQuery, wcErrorText, errorCode == DBERR_CONNECTION_LOST, hConn->m_driver->m_context);
	}

#ifndef UNICODE
   MemFree(wcQuery);
#endif
   
	if (hResult == nullptr)
	   return nullptr;

   DB_RESULT result = MemAllocStruct<db_result_t>();
   result->m_driver = hConn->m_driver;
   result->m_connection = hConn;
   result->m_data = hResult;
   return result;
}

/**
 * Perform SELECT query
 */
DB_RESULT LIBNXDB_EXPORTABLE DBSelect(DB_HANDLE hConn, const TCHAR *query)
{
   TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
	return DBSelectEx(hConn, query, errorText);
}

/**
 * Perform SELECT query built from formatted string
 */
DB_RESULT LIBNXDB_EXPORTABLE DBSelectFormatted(DB_HANDLE hConn, const TCHAR *query, ...)
{
   TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
   TCHAR formattedQuery[4096];
   va_list args;
   va_start(args, query);
   _vsntprintf(formattedQuery, 4096, query, args);
   va_end(args);
   return DBSelectEx(hConn, formattedQuery, errorText);
}

/**
 * Get number of columns
 */
int LIBNXDB_EXPORTABLE DBGetColumnCount(DB_RESULT hResult)
{
	return hResult->m_driver->m_callTable.GetColumnCount(hResult->m_data);
}

/**
 * Get column name
 */
bool LIBNXDB_EXPORTABLE DBGetColumnName(DB_RESULT hResult, int column, TCHAR *buffer, int bufSize)
{
	const char *name = hResult->m_driver->m_callTable.GetColumnName(hResult->m_data, column);
	if (name != nullptr)
	{
#ifdef UNICODE
		mb_to_wchar(name, -1, buffer, bufSize);
		buffer[bufSize - 1] = 0;
#else
		strlcpy(buffer, name, bufSize);
#endif
	}
	return name != nullptr;
}

/**
 * Get column name as multibyte string
 */
bool LIBNXDB_EXPORTABLE DBGetColumnNameA(DB_RESULT hResult, int column, char *buffer, int bufSize)
{
   const char *name = hResult->m_driver->m_callTable.GetColumnName(hResult->m_data, column);
   if (name != nullptr)
   {
      strlcpy(buffer, name, bufSize);
   }
   return name != nullptr;
}

/**
 * Get column count for unbuffered result set
 */
int LIBNXDB_EXPORTABLE DBGetColumnCount(DB_UNBUFFERED_RESULT hResult)
{
	return hResult->m_driver->m_callTable.GetColumnCountUnbuffered(hResult->m_data);
}

/**
 * Get column name for unbuffered result set
 */
bool LIBNXDB_EXPORTABLE DBGetColumnName(DB_UNBUFFERED_RESULT hResult, int column, TCHAR *buffer, int bufSize)
{
	const char *name = hResult->m_driver->m_callTable.GetColumnNameUnbuffered(hResult->m_data, column);
	if (name != nullptr)
	{
#ifdef UNICODE
		mb_to_wchar(name, -1, buffer, bufSize);
		buffer[bufSize - 1] = 0;
#else
		strlcpy(buffer, name, bufSize);
#endif
	}
	return name != nullptr;
}

/**
 * Get column name for unbuffered result set as multibyte string
 */
bool LIBNXDB_EXPORTABLE DBGetColumnNameA(DB_UNBUFFERED_RESULT hResult, int column, char *buffer, int bufSize)
{
   const char *name = hResult->m_driver->m_callTable.GetColumnNameUnbuffered(hResult->m_data, column);
   if (name != nullptr)
   {
      strlcpy(buffer, name, bufSize);
   }
   return name != nullptr;
}

/**
 * Get field's value. If buffer is NULL, dynamically allocated string will be returned.
 * Caller is responsible for destroying it by calling MemFree().
 */
TCHAR LIBNXDB_EXPORTABLE *DBGetField(DB_RESULT hResult, int iRow, int iColumn, TCHAR *pszBuffer, size_t nBufLen)
{
#ifdef UNICODE
   if (pszBuffer != nullptr)
   {
      *pszBuffer = 0;
      return hResult->m_driver->m_callTable.GetField(hResult->m_data, iRow, iColumn, pszBuffer, (int)nBufLen);
   }
   else
   {
      WCHAR *pszTemp;
      int32_t nLen = hResult->m_driver->m_callTable.GetFieldLength(hResult->m_data, iRow, iColumn);
      if (nLen == -1)
      {
         pszTemp = nullptr;
      }
      else
      {
         nLen++;
         pszTemp = MemAllocStringW(nLen);
         hResult->m_driver->m_callTable.GetField(hResult->m_data, iRow, iColumn, pszTemp, nLen);
      }
      return pszTemp;
   }
#else
	return DBGetFieldA(hResult, iRow, iColumn, pszBuffer, nBufLen);
#endif
}

/**
 * Get field as string object
 */
String LIBNXDB_EXPORTABLE DBGetFieldAsString(DB_RESULT hResult, int row, int col)
{
   int32_t len = hResult->m_driver->m_callTable.GetFieldLength(hResult->m_data, row, col);
   if (len <= 0)
      return String();

   len++;
   if (len <= STRING_INTERNAL_BUFFER_SIZE)
   {
      WCHAR value[STRING_INTERNAL_BUFFER_SIZE];
      hResult->m_driver->m_callTable.GetField(hResult->m_data, row, col, value, len);
#ifdef UNICODE
      return String(value);
#else
      char mbvalue[STRING_INTERNAL_BUFFER_SIZE * 4];
      wchar_to_mb(value, -1, mbvalue, STRING_INTERNAL_BUFFER_SIZE * 4);
      return String(mbvalue);
#endif
   }

   WCHAR *value = MemAllocStringW(len);
   hResult->m_driver->m_callTable.GetField(hResult->m_data, row, col, value, len);
#ifdef UNICODE
   return String(value, -1, Ownership::True);
#else
   char *mbvalue = MBStringFromWideString(value);
   MemFree(value);
   return String(mbvalue, -1, Ownership::True);
#endif
}

/**
 * Get field as shared string
 */
SharedString LIBNXDB_EXPORTABLE DBGetFieldAsSharedString(DB_RESULT hResult, int row, int col)
{
   return SharedString(DBGetFieldAsString(hResult, row, col));
}

/**
 * Get field's value as UTF8 string. If buffer is NULL, dynamically allocated string will be returned.
 * Caller is responsible for destroying it by calling free().
 */
char LIBNXDB_EXPORTABLE *DBGetFieldUTF8(DB_RESULT hResult, int row, int column, char *buffer, size_t bufferSize)
{
   if (hResult->m_driver->m_callTable.GetFieldUTF8 != nullptr)
   {
      if (buffer != nullptr)
      {
         *buffer = 0;
         return hResult->m_driver->m_callTable.GetFieldUTF8(hResult->m_data, row, column, buffer, (int)bufferSize);
      }
      else
      {
         char *pszTemp;
         int32_t nLen = hResult->m_driver->m_callTable.GetFieldLength(hResult->m_data, row, column);
         if (nLen == -1)
         {
            pszTemp = nullptr;
         }
         else
         {
            nLen = nLen * 2 + 1;  // increase buffer size because driver may return field length in characters
            pszTemp = MemAllocStringA(nLen);
            hResult->m_driver->m_callTable.GetFieldUTF8(hResult->m_data, row, column, pszTemp, nLen);
         }
         return pszTemp;
      }
   }
   else
   {
      int32_t nLen = hResult->m_driver->m_callTable.GetFieldLength(hResult->m_data, row, column);
      if (nLen == -1)
         return nullptr;
      nLen++;

      Buffer<WCHAR, 1024> wtemp(nLen);
      hResult->m_driver->m_callTable.GetField(hResult->m_data, row, column, wtemp, nLen);
      size_t utf8len = wchar_utf8len(wtemp, -1);
      char *value = (buffer != nullptr) ? buffer : MemAllocStringA(utf8len);
      wchar_to_utf8(wtemp, -1, value, (buffer != nullptr) ? bufferSize : utf8len);
      return value;
   }
}

/**
 * Get field's value as multibyte string. If buffer is NULL, dynamically allocated string will be returned.
 * Caller is responsible for destroying it by calling MemFree().
 */
char LIBNXDB_EXPORTABLE *DBGetFieldA(DB_RESULT hResult, int row, int column, char *buffer, size_t bufferSize)
{
   char *result;
   if (buffer != nullptr)
   {
      *buffer = 0;
      Buffer<WCHAR, 1024> wbuffer(bufferSize);
      WCHAR *value = hResult->m_driver->m_callTable.GetField(hResult->m_data, row, column, wbuffer, (int)bufferSize);
      if (value != nullptr)
      {
         wchar_to_mb(value, -1, buffer, (int)bufferSize);
         result = buffer;
      }
      else
      {
         result = nullptr;
      }
   }
   else
   {
      int nLen = hResult->m_driver->m_callTable.GetFieldLength(hResult->m_data, row, column);
      if (nLen == -1)
      {
         result = nullptr;
      }
      else
      {
         nLen++;
         Buffer<WCHAR, 1024> wbuffer(nLen);
         WCHAR *value = hResult->m_driver->m_callTable.GetField(hResult->m_data, row, column, wbuffer, nLen);
         if (value != nullptr)
         {
            nLen = (int)wcslen(value) + 1;
            result = MemAllocStringA(nLen);
            wchar_to_mb(value, -1, result, nLen);
         }
         else
         {
            result = nullptr;
         }
      }
   }
   return result;
}

/**
 * Get text field and escape it for XML document. Returned string
 * always dynamically allocated and must be destroyed by caller.
 */
TCHAR LIBNXDB_EXPORTABLE *DBGetFieldForXML(DB_RESULT hResult, int row, int col)
{
   TCHAR *value = DBGetField(hResult, row, col, nullptr, 0);
   TCHAR *xmlString = EscapeStringForXML(value, -1);
   MemFree(value);
   return xmlString;
}

/**
 * Get field's value as unsigned long
 */
uint32_t LIBNXDB_EXPORTABLE DBGetFieldUInt32(DB_RESULT hResult, int row, int column)
{
   TCHAR buffer[256];
   TCHAR *value = DBGetField(hResult, row, column, buffer, 256);
   if (value == nullptr)
      return 0;

	Trim(value);

	uint32_t u;
	if (*value == _T('-'))
	{
		int32_t i = _tcstol(value, nullptr, 10);
		memcpy(&u, &i, sizeof(int32_t));   // To prevent possible conversion
	}
	else
	{
		u = _tcstoul(value, nullptr, 10);
	}
   return u;
}

/**
 * Get field's value as unsigned 64-bit int
 */
uint64_t LIBNXDB_EXPORTABLE DBGetFieldUInt64(DB_RESULT hResult, int row, int column)
{
   TCHAR buffer[256];
   TCHAR *value = DBGetField(hResult, row, column, buffer, 256);
   if (value == nullptr)
      return 0;

   Trim(value);

   uint64_t u;
   if (*value == _T('-'))
	{
		int64_t i = _tcstoll(value, nullptr, 10);
		memcpy(&u, &i, sizeof(int64_t));   // To prevent possible conversion
	}
	else
	{
		u = _tcstoull(value, nullptr, 10);
	}
   return u;
}

/**
 * Get field's value as unsigned 16-bit int
 */
uint16_t LIBNXDB_EXPORTABLE DBGetFieldUInt16(DB_RESULT hResult, int row, int column)
{
   return static_cast<uint16_t>(DBGetFieldUInt32(hResult, row, column));
}

/**
 * Get field's value as signed long
 */
int32_t LIBNXDB_EXPORTABLE DBGetFieldInt32(DB_RESULT hResult, int row, int column)
{
   TCHAR buffer[256];
   TCHAR *value = DBGetField(hResult, row, column, buffer, 256);
   return (value != nullptr) ? _tcstol(value, nullptr, 10) : 0;
}

/**
 * Get field's value as signed 64-bit int
 */
int64_t LIBNXDB_EXPORTABLE DBGetFieldInt64(DB_RESULT hResult, int row, int column)
{
   TCHAR buffer[256];
   TCHAR *value = DBGetField(hResult, row, column, buffer, 256);
   return (value != nullptr) ? _tcstoll(value, nullptr, 10) : 0;
}

/**
 * Get field's value as signed 16-bit int
 */
int16_t LIBNXDB_EXPORTABLE DBGetFieldInt16(DB_RESULT hResult, int row, int column)
{
   TCHAR buffer[256];
   TCHAR *value = DBGetField(hResult, row, column, buffer, 256);
   return (value != nullptr) ? static_cast<int16_t>(_tcstol(value, nullptr, 10)) : 0;
}

/**
 * Get field's value as double
 */
double LIBNXDB_EXPORTABLE DBGetFieldDouble(DB_RESULT hResult, int row, int column)
{
   TCHAR buffer[256];
   TCHAR *value = DBGetField(hResult, row, column, buffer, 256);
   return (value != nullptr) ? _tcstod(value, nullptr) : 0;
}

/**
 * Get field's value as IPv4 address
 */
uint32_t LIBNXDB_EXPORTABLE DBGetFieldIPAddr(DB_RESULT hResult, int row, int column)
{
   TCHAR buffer[256];
   TCHAR *value = DBGetField(hResult, row, column, buffer, 256);
   if (value == nullptr)
      return 0;
   InetAddress addr = InetAddress::parse(value);
   return (addr.getFamily() == AF_INET) ? addr.getAddressV4() : 0;
}

/**
 * Get field's value as IP address
 */
InetAddress LIBNXDB_EXPORTABLE DBGetFieldInetAddr(DB_RESULT hResult, int row, int column)
{
   TCHAR buffer[256];
   TCHAR *value = DBGetField(hResult, row, column, buffer, 256);
   return (value != nullptr) ? InetAddress::parse(value) : InetAddress();
}

/**
 * Get field`s value as MAC address
 */
MacAddress LIBNXDB_EXPORTABLE DBGetFieldMacAddr(DB_RESULT hResult, int row, int column)
{
   TCHAR buffer[36];
   TCHAR *v = DBGetField(hResult, row, column, buffer, 36);
   return (v != nullptr) ? MacAddress::parse(v) : MacAddress::ZERO;
}

/**
 * Get field's value as integer array from byte array encoded in hex
 */
bool LIBNXDB_EXPORTABLE DBGetFieldByteArray(DB_RESULT hResult, int iRow, int iColumn, int *pnArray, size_t size, int defaultValue)
{
   char pbBytes[2048];
   bool bResult;
   TCHAR szBuffer[4096];
   TCHAR *pszVal = DBGetField(hResult, iRow, iColumn, szBuffer, 4096);
   if (pszVal != NULL)
   {
      StrToBin(pszVal, (BYTE *)pbBytes, 2048);
      size_t nLen = _tcslen(pszVal) / 2;
      size_t i;
      for(i = 0; (i < size) && (i < nLen); i++)
         pnArray[i] = pbBytes[i];
      for(; i < size; i++)
         pnArray[i] = defaultValue;
      bResult = true;
   }
   else
   {
      for(size_t i = 0; i < size; i++)
         pnArray[i] = defaultValue;
      bResult = false;
   }
   return bResult;
}

/**
 * Get field's value as integer array from byte array encoded in hex
 */
bool LIBNXDB_EXPORTABLE DBGetFieldByteArray2(DB_RESULT hResult, int iRow, int iColumn, BYTE *data, size_t size, BYTE defaultValue)
{
   bool success;
   TCHAR buffer[4096];
   TCHAR *value = DBGetField(hResult, iRow, iColumn, buffer, 4096);
   if (value != nullptr)
   {
      size_t bytes = StrToBin(value, data, size);
		if (bytes < size)
			memset(&data[bytes], defaultValue, size - bytes);
		success = true;
   }
   else
   {
		memset(data, defaultValue, size);
		success = false;
   }
   return success;
}

/**
 * Get field's value as GUID
 */
uuid LIBNXDB_EXPORTABLE DBGetFieldGUID(DB_RESULT hResult, int row, int column)
{
   TCHAR buffer[256];
   TCHAR *value = DBGetField(hResult, row, column, buffer, 256);
   return (value == nullptr) ? uuid::NULL_UUID : uuid::parse(value);
}

/**
 * Get number of rows in result
 */
int LIBNXDB_EXPORTABLE DBGetNumRows(DB_RESULT hResult)
{
   if (hResult == nullptr)
      return 0;
   return hResult->m_driver->m_callTable.GetNumRows(hResult->m_data);
}

/**
 * Free result
 */
void LIBNXDB_EXPORTABLE DBFreeResult(DB_RESULT hResult)
{
   if (hResult == nullptr)
      return;
   hResult->m_driver->m_callTable.FreeResult(hResult->m_data);
   MemFree(hResult);
}

/**
 * Unbuffered SELECT query
 */
DB_UNBUFFERED_RESULT LIBNXDB_EXPORTABLE DBSelectUnbufferedEx(DB_HANDLE hConn, const TCHAR *query, TCHAR *errorText)
{
#ifdef UNICODE
   auto wcQuery = query;
   auto wcErrorText = errorText;
#else
   WCHAR *wcQuery = WideStringFromMBString(query);
	WCHAR wcErrorText[DBDRV_MAX_ERROR_TEXT] = L"";
#endif

   hConn->m_mutexTransLock.lock();
   int64_t ms = GetMonotonicClockTime();

   InterlockedIncrement64(&s_perfSelectQueries);
   InterlockedIncrement64(&s_perfTotalQueries);

   uint32_t errorCode = DBERR_OTHER_ERROR;
   DBDRV_UNBUFFERED_RESULT hResult = hConn->m_driver->m_callTable.SelectUnbuffered(hConn->m_connection, wcQuery, &errorCode, wcErrorText);
   if ((hResult == nullptr) && (errorCode == DBERR_CONNECTION_LOST) && hConn->m_reconnectEnabled)
   {
      DBReconnect(hConn);
      hResult = hConn->m_driver->m_callTable.SelectUnbuffered(hConn->m_connection, wcQuery, &errorCode, wcErrorText);
   }

   ms = GetMonotonicClockTime() - ms;
   if (s_queryTrace)
   {
      nxlog_debug_tag(DEBUG_TAG_QUERY, 9, _T("%s unbuffered query: \"%s\" [%d ms]"), (hResult != nullptr) ? _T("Successful") : _T("Failed"), query, (int)ms);
   }
   if ((hResult != nullptr) && ((uint32_t)ms > hConn->getQueryExecTimeThreshold()))
   {
      nxlog_debug_tag(DEBUG_TAG_QUERY, 3, _T("Long running query: \"%s\" [%d ms]"), query, (int)ms);
      InterlockedIncrement64(&s_perfLongRunningQueries);
   }
   if (hResult == nullptr)
   {
      InterlockedIncrement64(&s_perfFailedQueries);
      hConn->m_mutexTransLock.unlock();

#ifndef UNICODE
		wchar_to_mb(wcErrorText, -1, errorText, DBDRV_MAX_ERROR_TEXT);
		errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
#endif

      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_DRIVER, _T("SQL query failed (Query = \"%s\"): %s"), query, errorText);
		if (hConn->m_driver->m_fpEventHandler != nullptr)
			hConn->m_driver->m_fpEventHandler(DBEVENT_QUERY_FAILED, wcQuery, wcErrorText, errorCode == DBERR_CONNECTION_LOST, hConn->m_driver->m_context);
   }

#ifndef UNICODE
   MemFree(wcQuery);
#endif

	if (hResult == nullptr)
	   return nullptr;

   DB_UNBUFFERED_RESULT result = MemAllocStruct<db_unbuffered_result_t>();
   result->m_driver = hConn->m_driver;
   result->m_connection = hConn;
   result->m_data = hResult;
   return result;
}

/**
 * Unbuffered SELECT query
 */
DB_UNBUFFERED_RESULT LIBNXDB_EXPORTABLE DBSelectUnbuffered(DB_HANDLE hConn, const TCHAR *query)
{
   TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
	return DBSelectUnbufferedEx(hConn, query, errorText);
}

/**
 * Fetch next row from unbuffered SELECT result
 */
bool LIBNXDB_EXPORTABLE DBFetch(DB_UNBUFFERED_RESULT hResult)
{
	return hResult->m_driver->m_callTable.Fetch(hResult->m_data);
}

/**
 * Get field's value from unbuffered SELECT result
 */
TCHAR LIBNXDB_EXPORTABLE *DBGetField(DB_UNBUFFERED_RESULT hResult, int iColumn, TCHAR *pBuffer, size_t iBufSize)
{
#ifdef UNICODE
   if (pBuffer != nullptr)
   {
	   return hResult->m_driver->m_callTable.GetFieldUnbuffered(hResult->m_data, iColumn, pBuffer, (int)iBufSize);
   }
   else
   {
      WCHAR *pszTemp;
      int32_t nLen = hResult->m_driver->m_callTable.GetFieldLengthUnbuffered(hResult->m_data, iColumn);
      if (nLen == -1)
      {
         pszTemp = nullptr;
      }
      else
      {
         nLen++;
         pszTemp = MemAllocStringW(nLen);
         if (hResult->m_driver->m_callTable.GetFieldUnbuffered(hResult->m_data, iColumn, pszTemp, nLen) == nullptr)
         {
            MemFree(pszTemp);
            pszTemp = nullptr;
         }
      }
      return pszTemp;
   }
#else
   WCHAR *pwszData, *pwszBuffer;
   char *pszRet;
   int nLen;

   if (pBuffer != nullptr)
   {
		pwszBuffer = MemAllocStringW(iBufSize);
		if (hResult->m_driver->m_callTable.GetFieldUnbuffered(hResult->m_data, iColumn, pwszBuffer, (int)iBufSize) != NULL)
		{
			wchar_to_mb(pwszBuffer, -1, pBuffer, iBufSize);
			pszRet = pBuffer;
		}
		else
		{
			pszRet = nullptr;
		}
		MemFree(pwszBuffer);
   }
   else
   {
		nLen = hResult->m_driver->m_callTable.GetFieldLengthUnbuffered(hResult->m_data, iColumn);
      if (nLen == -1)
      {
         pszRet = nullptr;
      }
      else
      {
         nLen++;
         pwszBuffer = MemAllocStringW(nLen);
			pwszData = hResult->m_driver->m_callTable.GetFieldUnbuffered(hResult->m_data, iColumn, pwszBuffer, nLen);
         if (pwszData != nullptr)
         {
            nLen = (int)wcslen(pwszData) + 1;
            pszRet = MemAllocStringA(nLen);
            wchar_to_mb(pwszData, -1, pszRet, nLen);
         }
         else
         {
            pszRet = nullptr;
         }
         MemFree(pwszBuffer);
      }
   }
   return pszRet;
#endif
}

/**
 * Get field's value from unbuffered SELECT result as UTF-8 string
 */
char LIBNXDB_EXPORTABLE *DBGetFieldUTF8(DB_UNBUFFERED_RESULT hResult, int column, char *buffer, size_t bufferSize)
{
   if (hResult->m_driver->m_callTable.GetFieldUTF8 != nullptr)
   {
      if (buffer != nullptr)
      {
         *buffer = 0;
         return hResult->m_driver->m_callTable.GetFieldUnbufferedUTF8(hResult->m_data, column, buffer, (int)bufferSize);
      }
      else
      {
         char *pszTemp;
         int32_t nLen = hResult->m_driver->m_callTable.GetFieldLengthUnbuffered(hResult->m_data, column);
         if (nLen == -1)
         {
            pszTemp = nullptr;
         }
         else
         {
            nLen = nLen * 2 + 1;  // increase buffer size because driver may return field length in characters
            pszTemp = MemAllocStringA(nLen);
            hResult->m_driver->m_callTable.GetFieldUnbufferedUTF8(hResult->m_data, column, pszTemp, nLen);
         }
         return pszTemp;
      }
   }
   else
   {
      int32_t nLen = hResult->m_driver->m_callTable.GetFieldLengthUnbuffered(hResult->m_data, column);
      if (nLen == -1)
         return nullptr;
      nLen++;

      Buffer<WCHAR, 1024> wtemp(nLen);
      hResult->m_driver->m_callTable.GetFieldUnbuffered(hResult->m_data, column, wtemp, nLen);
      size_t utf8len = wchar_utf8len(wtemp, -1);
      char *value = (buffer != nullptr) ? buffer : MemAllocStringA(utf8len);
      wchar_to_utf8(wtemp, -1, value, (buffer != nullptr) ? bufferSize : utf8len);
      return value;
   }
}

/**
 * Get field's value as unsigned long from unbuffered SELECT result
 */
uint32_t LIBNXDB_EXPORTABLE DBGetFieldUInt32(DB_UNBUFFERED_RESULT hResult, int iColumn)
{
   TCHAR buffer[64];
   if (DBGetField(hResult, iColumn, buffer, 64) == nullptr)
      return 0;

	Trim(buffer);

   uint32_t value;
	if (buffer[0] == _T('-'))
	{
		int32_t intVal = _tcstol(buffer, nullptr, 10);
		memcpy(&value, &intVal, sizeof(int32_t));   // To prevent possible conversion
	}
	else
	{
	   value = _tcstoul(buffer, nullptr, 10);
	}
   return value;
}

/**
 * Get field's value as unsigned 64-bit int from unbuffered SELECT result
 */
uint64_t LIBNXDB_EXPORTABLE DBGetFieldUInt64(DB_UNBUFFERED_RESULT hResult, int iColumn)
{
   TCHAR buffer[64];
   if (DBGetField(hResult, iColumn, buffer, 64) == nullptr)
      return 0;

   Trim(buffer);

   uint64_t value;
	if (buffer[0] == _T('-'))
	{
		int64_t intVal = _tcstoll(buffer, nullptr, 10);
		memcpy(&value, &intVal, sizeof(int64_t));   // To prevent possible conversion
	}
	else
	{
	   value = _tcstoull(buffer, nullptr, 10);
	}
   return value;
}

/**
 * Get field's value as signed long from unbuffered SELECT result
 */
int32_t LIBNXDB_EXPORTABLE DBGetFieldInt32(DB_UNBUFFERED_RESULT hResult, int column)
{
   TCHAR buffer[64];
   return DBGetField(hResult, column, buffer, 64) == nullptr ? 0 : _tcstol(buffer, nullptr, 10);
}

/**
 * Get field's value as signed 64-bit int from unbuffered SELECT result
 */
int64_t LIBNXDB_EXPORTABLE DBGetFieldInt64(DB_UNBUFFERED_RESULT hResult, int column)
{
   TCHAR buffer[64];
   return DBGetField(hResult, column, buffer, 64) == nullptr ? 0 : _tcstoll(buffer, nullptr, 10);
}

/**
 * Get field's value as signed 16-bit int from unbuffered SELECT result
 */
int16_t LIBNXDB_EXPORTABLE DBGetFieldInt16(DB_UNBUFFERED_RESULT hResult, int column)
{
   TCHAR buffer[64];
   return DBGetField(hResult, column, buffer, 64) == nullptr ? 0 : static_cast<int16_t>(_tcstol(buffer, nullptr, 10));
}

/**
 * Get field's value as unsigned 16-bit int from unbuffered SELECT result
 */
uint16_t LIBNXDB_EXPORTABLE DBGetFieldUInt16(DB_UNBUFFERED_RESULT hResult, int column)
{
   return static_cast<uint16_t>(DBGetFieldUInt32(hResult, column));
}

/**
 * Get field's value as signed long from unbuffered SELECT result
 */
double LIBNXDB_EXPORTABLE DBGetFieldDouble(DB_UNBUFFERED_RESULT hResult, int iColumn)
{
   TCHAR buffer[64];
   return DBGetField(hResult, iColumn, buffer, 64) == NULL ? 0 : _tcstod(buffer, nullptr);
}

/**
 * Get field's value as IPv4 address from unbuffered SELECT result
 */
uint32_t LIBNXDB_EXPORTABLE DBGetFieldIPAddr(DB_UNBUFFERED_RESULT hResult, int column)
{
   TCHAR buffer[64];
   TCHAR *value = DBGetField(hResult, column, buffer, 64);
   if (value == nullptr)
      return 0;
   InetAddress addr = InetAddress::parse(value);
   return (addr.getFamily() == AF_INET) ? addr.getAddressV4() : 0;
}

/**
 * Get field's value as IP address from unbuffered SELECT result
 */
InetAddress LIBNXDB_EXPORTABLE DBGetFieldInetAddr(DB_UNBUFFERED_RESULT hResult, int column)
{
   TCHAR buffer[64];
   TCHAR *value = DBGetField(hResult, column, buffer, 64);
   return (value != nullptr) ? InetAddress::parse(buffer) : InetAddress();
}

/**
 * Get field's value as GUID from unbuffered SELECT result
 */
uuid LIBNXDB_EXPORTABLE DBGetFieldGUID(DB_UNBUFFERED_RESULT hResult, int column)
{
   TCHAR buffer[64];
   return (DBGetField(hResult, column, buffer, 64) == nullptr) ? uuid::NULL_UUID : uuid::parse(buffer);
}

/**
 * Free unbuffered SELECT result
 */
void LIBNXDB_EXPORTABLE DBFreeResult(DB_UNBUFFERED_RESULT hResult)
{
   if (hResult == nullptr)
      return;
	hResult->m_driver->m_callTable.FreeUnbufferedResult(hResult->m_data);
	hResult->m_connection->m_mutexTransLock.unlock();
	MemFree(hResult);
}

/**
 * Prepare statement
 */
DB_STATEMENT LIBNXDB_EXPORTABLE DBPrepareEx(DB_HANDLE hConn, const TCHAR *query, bool optimizeForReuse, TCHAR *errorText)
{
	DB_STATEMENT result = nullptr;
	INT64 ms;

#ifdef UNICODE
#define wcQuery query
#define wcErrorText errorText
#else
   WCHAR *wcQuery = WideStringFromMBString(query);
	WCHAR wcErrorText[DBDRV_MAX_ERROR_TEXT] = L"";
#endif

	hConn->m_mutexTransLock.lock();

	if (s_queryTrace)
      ms = GetMonotonicClockTime();

	uint32_t errorCode;
	DBDRV_STATEMENT stmt = hConn->m_driver->m_callTable.Prepare(hConn->m_connection, wcQuery, optimizeForReuse, &errorCode, wcErrorText);
   if ((stmt == nullptr) && (errorCode == DBERR_CONNECTION_LOST) && hConn->m_reconnectEnabled)
   {
      DBReconnect(hConn);
		stmt = hConn->m_driver->m_callTable.Prepare(hConn->m_connection, wcQuery, optimizeForReuse, &errorCode, wcErrorText);
	}
   hConn->m_mutexTransLock.unlock();

	if (stmt != nullptr)
	{
		result = MemAllocStruct<db_statement_t>();
		result->m_driver = hConn->m_driver;
		result->m_connection = hConn;
		result->m_statement = stmt;
		result->m_query = _tcsdup(query);
	}
	else
	{
#ifndef UNICODE
		wchar_to_mb(wcErrorText, -1, errorText, DBDRV_MAX_ERROR_TEXT);
		errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
#endif

      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_DRIVER, _T("SQL query failed (Query = \"%s\"): %s"), query, errorText);
		if (hConn->m_driver->m_fpEventHandler != NULL)
			hConn->m_driver->m_fpEventHandler(DBEVENT_QUERY_FAILED, wcQuery, wcErrorText, errorCode == DBERR_CONNECTION_LOST, hConn->m_driver->m_context);

		InterlockedIncrement64(&s_perfFailedQueries);
		InterlockedIncrement64(&s_perfTotalQueries);
	}

   if (s_queryTrace)
   {
      ms = GetMonotonicClockTime() - ms;
		nxlog_debug_tag(DEBUG_TAG_QUERY, 9, _T("{%p} %s prepare: \"%s\" [%d ms]"), result, (result != NULL) ? _T("Successful") : _T("Failed"), query, ms);
	}

#ifndef UNICODE
	MemFree(wcQuery);
#endif

   if (result != nullptr)
   {
      hConn->m_preparedStatementsLock.lock();
      hConn->m_preparedStatements.add(result);
      hConn->m_preparedStatementsLock.unlock();
   }

	return result;
#undef wcQuery
#undef wcErrorText
}

/**
 * Prepare statement
 */
DB_STATEMENT LIBNXDB_EXPORTABLE DBPrepare(DB_HANDLE hConn, const TCHAR *query, bool optimizeForReuse)
{
	TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
	return DBPrepareEx(hConn, query, optimizeForReuse, errorText);
}

/**
 * Destroy prepared statement
 */
void LIBNXDB_EXPORTABLE DBFreeStatement(DB_STATEMENT hStmt)
{
   if (hStmt == nullptr)
      return;

   if (hStmt->m_connection != nullptr)
   {
      hStmt->m_connection->m_preparedStatementsLock.lock();
      hStmt->m_connection->m_preparedStatements.remove(hStmt);
      hStmt->m_connection->m_preparedStatementsLock.unlock();
   }
   if (hStmt->m_statement != nullptr)
   {
      hStmt->m_driver->m_callTable.FreeStatement(hStmt->m_statement);
   }
   MemFree(hStmt->m_query);
   MemFree(hStmt);
}

/**
 * Get source query for prepared statement
 */
const TCHAR LIBNXDB_EXPORTABLE *DBGetStatementSource(DB_STATEMENT hStmt)
{
   return hStmt->m_query;
}

/**
 * Open batch
 */
bool LIBNXDB_EXPORTABLE DBOpenBatch(DB_STATEMENT hStmt)
{
   if (!IS_VALID_STATEMENT_HANDLE(hStmt) || (hStmt->m_driver->m_callTable.OpenBatch == nullptr))
      return false;
   return hStmt->m_driver->m_callTable.OpenBatch(hStmt->m_statement);
}

/**
 * Start next batch row batch
 */
void LIBNXDB_EXPORTABLE DBNextBatchRow(DB_STATEMENT hStmt)
{
   if (IS_VALID_STATEMENT_HANDLE(hStmt) && (hStmt->m_driver->m_callTable.NextBatchRow != nullptr))
      hStmt->m_driver->m_callTable.NextBatchRow(hStmt->m_statement);
}

/**
 * Bind parameter (generic)
 */
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, int cType, const void *buffer, int allocType)
{
	if ((pos <= 0) || !IS_VALID_STATEMENT_HANDLE(hStmt))
		return;

	if (s_queryTrace)
   {
		if (cType == DB_CTYPE_STRING)
		{
			nxlog_debug_tag(DEBUG_TAG_QUERY, 9, _T("{%p} bind at pos %d: \"%s\""), hStmt, pos, buffer);
		}
		else if (cType == DB_CTYPE_UTF8_STRING)
      {
         nxlog_debug_tag(DEBUG_TAG_QUERY, 9, _T("{%p} bind at pos %d (UTF-8): \"%hs\""), hStmt, pos, buffer);
      }
		else
		{
			TCHAR text[64];
			switch(cType)
			{
				case DB_CTYPE_INT32:
				   IntegerToString(*static_cast<const int32_t*>(buffer), text);
					break;
				case DB_CTYPE_UINT32:
               IntegerToString(*static_cast<const uint32_t*>(buffer), text);
					break;
				case DB_CTYPE_INT64:
               IntegerToString(*static_cast<const int64_t*>(buffer), text);
					break;
				case DB_CTYPE_UINT64:
               IntegerToString(*static_cast<const uint64_t*>(buffer), text);
					break;
				case DB_CTYPE_DOUBLE:
					_sntprintf(text, 64, _T("%f"), *static_cast<const double*>(buffer));
					break;
			}
			nxlog_debug_tag(DEBUG_TAG_QUERY, 9, _T("{%p} bind at pos %d: \"%s\""), hStmt, pos, text);
		}
	}

#ifdef UNICODE
#define wBuffer const_cast<void*>(buffer)
#define realAllocType allocType
#else
	void *wBuffer;
	int realAllocType = allocType;
	if (cType == DB_CTYPE_STRING)
	{
		wBuffer = WideStringFromMBString(static_cast<const char*>(buffer));
		if (allocType == DB_BIND_DYNAMIC)
			MemFree(const_cast<void*>(buffer));
		realAllocType = DB_BIND_DYNAMIC;
	}
	else
	{
		wBuffer = const_cast<void*>(buffer);
	}
#endif
	hStmt->m_driver->m_callTable.Bind(hStmt->m_statement, pos, sqlType, cType, wBuffer, realAllocType);
#undef wBuffer
#undef realAllocType
}

/**
 * Bind string parameter
 */
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, const TCHAR *value, int allocType)
{
	if (value != nullptr)
		DBBind(hStmt, pos, sqlType, DB_CTYPE_STRING, (void *)value, allocType);
	else
		DBBind(hStmt, pos, sqlType, DB_CTYPE_STRING, (void *)_T(""), DB_BIND_STATIC);
}

/**
 * Bind string parameter with length validation
 */
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, const TCHAR *value, int allocType, int maxLen)
{
	if (value != nullptr)
   {
      if ((int)_tcslen(value) <= maxLen)
      {
		   DBBind(hStmt, pos, sqlType, DB_CTYPE_STRING, (void *)value, allocType);
      }
      else
      {
         if (allocType == DB_BIND_DYNAMIC)
         {
            ((TCHAR *)value)[maxLen] = 0;
   		   DBBind(hStmt, pos, sqlType, DB_CTYPE_STRING, (void *)value, DB_BIND_DYNAMIC);
         }
         else
         {
            TCHAR *temp = MemCopyBlock(value, sizeof(TCHAR) * (maxLen + 1));
            temp[maxLen] = 0;
   		   DBBind(hStmt, pos, sqlType, DB_CTYPE_STRING, temp, DB_BIND_DYNAMIC);
         }
      }
   }
	else
   {
		DBBind(hStmt, pos, sqlType, DB_CTYPE_STRING, (void *)_T(""), DB_BIND_STATIC);
   }
}

/**
 * Bind 16 bit integer parameter
 */
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, int16_t value)
{
   int32_t v = value;
   DBBind(hStmt, pos, sqlType, DB_CTYPE_INT32, &v, DB_BIND_TRANSIENT);
}

/**
 * Bind 16 bit unsigned integer parameter
 */
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, uint16_t value)
{
   int32_t v = value;
   DBBind(hStmt, pos, sqlType, DB_CTYPE_INT32, &v, DB_BIND_TRANSIENT);
}

/**
 * Bind 32 bit integer parameter
 */
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, int32_t value)
{
	DBBind(hStmt, pos, sqlType, DB_CTYPE_INT32, &value, DB_BIND_TRANSIENT);
}

/**
 * Bind 32 bit unsigned integer parameter
 */
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, uint32_t value)
{
	// C type intentionally set to INT32 - unsigned numbers will be written as negatives
	// and correctly parsed on read by DBGetFieldULong
	// Setting it to UINT32 may cause INSERT/UPDATE failures on some databases
	DBBind(hStmt, pos, sqlType, DB_CTYPE_INT32, &value, DB_BIND_TRANSIENT);
}

/**
 * Bind 64 bit integer parameter
 */
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, int64_t value)
{
	DBBind(hStmt, pos, sqlType, DB_CTYPE_INT64, &value, DB_BIND_TRANSIENT);
}

/**
 * Bind 64 bit unsigned integer parameter
 */
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, uint64_t value)
{
	DBBind(hStmt, pos, sqlType, DB_CTYPE_UINT64, &value, DB_BIND_TRANSIENT);
}

/**
 * Bind floating point parameter
 */
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, double value)
{
	DBBind(hStmt, pos, sqlType, DB_CTYPE_DOUBLE, &value, DB_BIND_TRANSIENT);
}

/**
 * Bind UUID parameter
 */
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, const uuid& value)
{
   TCHAR buffer[64];
   DBBind(hStmt, pos, sqlType, DB_CTYPE_STRING, value.toString(buffer), DB_BIND_TRANSIENT);
}

/**
 * Bind MAC address
 */
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, const MacAddress& value)
{
   TCHAR buffer[36];
   DBBind(hStmt, pos, sqlType, DB_CTYPE_STRING, value.toString(buffer, MacAddressNotation::FLAT_STRING), DB_BIND_TRANSIENT);
}

/**
 * Bind IP address
 */
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, const InetAddress& value)
{
   TCHAR buffer[64];
   DBBind(hStmt, pos, sqlType, DB_CTYPE_STRING, value.toString(buffer), DB_BIND_TRANSIENT);
}

/**
 * Bind timestamp
 */
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, Timestamp value)
{
   DBBind(hStmt, pos, sqlType, value.asMilliseconds());
}

/**
 * Bind JSON object
 */
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, json_t *value, int allocType)
{
   if (value != nullptr)
   {
      char *jsonText = json_dumps(value, JSON_COMPACT);
      DBBind(hStmt, pos, sqlType, DB_CTYPE_UTF8_STRING, jsonText, DB_BIND_TRANSIENT);
      MemFree(jsonText);
      if (allocType == DB_BIND_DYNAMIC)
         json_decref(value);
   }
   else
   {
      DBBind(hStmt, pos, sqlType, DB_CTYPE_STRING, (void *)_T(""), DB_BIND_STATIC);
   }
}

/**
 * Execute prepared statement (non-SELECT)
 */
bool LIBNXDB_EXPORTABLE DBExecuteEx(DB_STATEMENT hStmt, TCHAR *errorText)
{
   if (!IS_VALID_STATEMENT_HANDLE(hStmt))
   {
      _tcscpy(errorText, _T("Invalid statement handle"));
      return false;
   }

#ifdef UNICODE
   auto wcErrorText = errorText;
#else
	WCHAR wcErrorText[DBDRV_MAX_ERROR_TEXT] = L"";
#endif

	DB_HANDLE hConn = hStmt->m_connection;
	hConn->m_mutexTransLock.lock();
   uint64_t ms = GetMonotonicClockTime();

   InterlockedIncrement64(&s_perfNonSelectQueries);
   InterlockedIncrement64(&s_perfTotalQueries);

	uint32_t rc = hConn->m_driver->m_callTable.Execute(hConn->m_connection, hStmt->m_statement, wcErrorText);
   ms = GetMonotonicClockTime() - ms;
   if (s_queryTrace)
   {
      nxlog_debug_tag(DEBUG_TAG_QUERY, 9, _T("%s prepared sync query: \"%s\" [%d ms]"), (rc == DBERR_SUCCESS) ? _T("Successful") : _T("Failed"), hStmt->m_query, (int)ms);
   }
   if ((rc == DBERR_SUCCESS) && (static_cast<uint32_t>(ms) > hConn->getQueryExecTimeThreshold()))
   {
      nxlog_debug_tag(DEBUG_TAG_QUERY, 3, _T("Long running query: \"%s\" [%d ms]"), hStmt->m_query, (int)ms);
      InterlockedIncrement64(&s_perfLongRunningQueries);
   }

   // Do reconnect if needed, but don't retry statement execution
   // because it will fail anyway
   if ((rc == DBERR_CONNECTION_LOST) && hConn->m_reconnectEnabled)
   {
      DBReconnect(hConn);
   }

   hConn->m_mutexTransLock.unlock();

#ifndef UNICODE
	wchar_to_mb(wcErrorText, -1, errorText, DBDRV_MAX_ERROR_TEXT);
	errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
#endif

   if (rc != DBERR_SUCCESS)
	{	
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_DRIVER, _T("SQL query failed (Query = \"%s\"): %s"), hStmt->m_query, errorText);
		if (hConn->m_driver->m_fpEventHandler != nullptr)
		{
#ifdef UNICODE
			hConn->m_driver->m_fpEventHandler(DBEVENT_QUERY_FAILED, hStmt->m_query, wcErrorText, rc == DBERR_CONNECTION_LOST, hConn->m_driver->m_context);
#else
			WCHAR *query = WideStringFromMBString(hStmt->m_query);
			hConn->m_driver->m_fpEventHandler(DBEVENT_QUERY_FAILED, query, wcErrorText, rc == DBERR_CONNECTION_LOST, hConn->m_driver->m_context);
			MemFree(query);
#endif
		}
		InterlockedIncrement64(&s_perfFailedQueries);
	}

   return rc == DBERR_SUCCESS;
}

/**
 * Execute prepared statement (non-SELECT)
 */
bool LIBNXDB_EXPORTABLE DBExecute(DB_STATEMENT hStmt)
{
	TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
	return DBExecuteEx(hStmt, errorText);
}

/**
 * Execute prepared SELECT statement
 */
DB_RESULT LIBNXDB_EXPORTABLE DBSelectPreparedEx(DB_STATEMENT hStmt, TCHAR *errorText)
{
   if (!IS_VALID_STATEMENT_HANDLE(hStmt))
   {
      _tcscpy(errorText, _T("Invalid statement handle"));
      return nullptr;
   }

#ifdef UNICODE
   auto wcErrorText = errorText;
#else
	WCHAR wcErrorText[DBDRV_MAX_ERROR_TEXT] = L"";
#endif

	DB_HANDLE hConn = hStmt->m_connection;
   hConn->m_mutexTransLock.lock();

   InterlockedIncrement64(&s_perfSelectQueries);
   InterlockedIncrement64(&s_perfTotalQueries);

   int64_t ms = GetMonotonicClockTime();
   uint32_t errorCode = DBERR_OTHER_ERROR;
	DBDRV_RESULT hResult = hConn->m_driver->m_callTable.SelectPrepared(hConn->m_connection, hStmt->m_statement, &errorCode, wcErrorText);

   ms = GetMonotonicClockTime() - ms;
   if (s_queryTrace)
   {
      nxlog_debug_tag(DEBUG_TAG_QUERY, 9, _T("%s prepared sync query: \"%s\" [%d ms]"),
		              (hResult != nullptr) ? _T("Successful") : _T("Failed"), hStmt->m_query, (int)ms);
   }
   if ((hResult != nullptr) && (static_cast<uint32_t>(ms) > hConn->getQueryExecTimeThreshold()))
   {
      nxlog_debug_tag(DEBUG_TAG_QUERY, 3, _T("Long running query: \"%s\" [%d ms]"), hStmt->m_query, (int)ms);
      InterlockedIncrement64(&s_perfLongRunningQueries);
   }

   // Do reconnect if needed, but don't retry statement execution
   // because it will fail anyway
   if ((hResult == nullptr) && (errorCode == DBERR_CONNECTION_LOST) && hConn->m_reconnectEnabled)
   {
      DBReconnect(hConn);
   }

   hConn->m_mutexTransLock.unlock();

#ifndef UNICODE
	wchar_to_mb(wcErrorText, -1, errorText, DBDRV_MAX_ERROR_TEXT);
	errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
#endif

	if (hResult == nullptr)
	{
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_DRIVER, _T("SQL query failed (Query = \"%s\"): %s"), hStmt->m_query, errorText);
		if (hConn->m_driver->m_fpEventHandler != nullptr)
		{
#ifdef UNICODE
			hConn->m_driver->m_fpEventHandler(DBEVENT_QUERY_FAILED, hStmt->m_query, wcErrorText, errorCode == DBERR_CONNECTION_LOST, hConn->m_driver->m_context);
#else
			WCHAR *query = WideStringFromMBString(hStmt->m_query);
			hConn->m_driver->m_fpEventHandler(DBEVENT_QUERY_FAILED, query, wcErrorText, errorCode == DBERR_CONNECTION_LOST, hConn->m_driver->m_context);
			MemFree(query);
#endif
		}
		InterlockedIncrement64(&s_perfFailedQueries);
      return nullptr;
	}

   DB_RESULT result = MemAllocStruct<db_result_t>();
   result->m_driver = hConn->m_driver;
   result->m_connection = hConn;
   result->m_data = hResult;
   return result;
}

/**
 * Execute prepared SELECT statement
 */
DB_RESULT LIBNXDB_EXPORTABLE DBSelectPrepared(DB_STATEMENT hStmt)
{
	TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
	return DBSelectPreparedEx(hStmt, errorText);
}

/**
 * Execute prepared SELECT statement without caching results
 */
DB_UNBUFFERED_RESULT LIBNXDB_EXPORTABLE DBSelectPreparedUnbufferedEx(DB_STATEMENT hStmt, TCHAR *errorText)
{
   if (!IS_VALID_STATEMENT_HANDLE(hStmt))
   {
      _tcscpy(errorText, _T("Invalid statement handle"));
      return nullptr;
   }

#ifdef UNICODE
   auto wcErrorText = errorText;
#else
   WCHAR wcErrorText[DBDRV_MAX_ERROR_TEXT] = L"";
#endif

   DB_HANDLE hConn = hStmt->m_connection;
   hConn->m_mutexTransLock.lock();

   InterlockedIncrement64(&s_perfSelectQueries);
   InterlockedIncrement64(&s_perfTotalQueries);

   int64_t ms = GetMonotonicClockTime();
   uint32_t errorCode = DBERR_OTHER_ERROR;
   DBDRV_UNBUFFERED_RESULT hResult = hConn->m_driver->m_callTable.SelectPreparedUnbuffered(hConn->m_connection, hStmt->m_statement, &errorCode, wcErrorText);

   ms = GetMonotonicClockTime() - ms;
   if (s_queryTrace)
   {
      nxlog_debug_tag(DEBUG_TAG_QUERY, 9, _T("%s prepared sync query: \"%s\" [%d ms]"),
                    (hResult != nullptr) ? _T("Successful") : _T("Failed"), hStmt->m_query, (int)ms);
   }
   if ((hResult != nullptr) && (static_cast<uint32_t>(ms) > hConn->getQueryExecTimeThreshold()))
   {
      nxlog_debug_tag(DEBUG_TAG_QUERY, 3, _T("Long running query: \"%s\" [%d ms]"), hStmt->m_query, (int)ms);
      InterlockedIncrement64(&s_perfLongRunningQueries);
   }

   // Do reconnect if needed, but don't retry statement execution
   // because it will fail anyway
   if ((hResult == nullptr) && (errorCode == DBERR_CONNECTION_LOST) && hConn->m_reconnectEnabled)
   {
      DBReconnect(hConn);
   }

#ifndef UNICODE
   wchar_to_mb(wcErrorText, -1, errorText, DBDRV_MAX_ERROR_TEXT);
   errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
#endif

   if (hResult == nullptr)
   {
      hConn->m_mutexTransLock.unlock();

      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_DRIVER, _T("SQL query failed (Query = \"%s\"): %s"), hStmt->m_query, errorText);
      if (hConn->m_driver->m_fpEventHandler != nullptr)
      {
#ifdef UNICODE
         hConn->m_driver->m_fpEventHandler(DBEVENT_QUERY_FAILED, hStmt->m_query, wcErrorText, errorCode == DBERR_CONNECTION_LOST, hConn->m_driver->m_context);
#else
         WCHAR *query = WideStringFromMBString(hStmt->m_query);
         hConn->m_driver->m_fpEventHandler(DBEVENT_QUERY_FAILED, query, wcErrorText, errorCode == DBERR_CONNECTION_LOST, hConn->m_driver->m_context);
         MemFree(query);
#endif
      }
      InterlockedIncrement64(&s_perfFailedQueries);
      return nullptr;
   }

   DB_UNBUFFERED_RESULT result = MemAllocStruct<db_unbuffered_result_t>();
   result->m_driver = hConn->m_driver;
   result->m_connection = hConn;
   result->m_data = hResult;
   return result;
}

/**
 * Execute prepared SELECT statement
 */
DB_UNBUFFERED_RESULT LIBNXDB_EXPORTABLE DBSelectPreparedUnbuffered(DB_STATEMENT hStmt)
{
   TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
   return DBSelectPreparedUnbufferedEx(hStmt, errorText);
}

/**
 * Begin transaction
 */
bool LIBNXDB_EXPORTABLE DBBegin(DB_HANDLE hConn)
{
   uint32_t dwResult;
   bool bRet = false;

   hConn->m_mutexTransLock.lock();
   if (hConn->m_transactionLevel == 0)
   {
      dwResult = hConn->m_driver->m_callTable.Begin(hConn->m_connection);
      if ((dwResult == DBERR_CONNECTION_LOST) && hConn->m_reconnectEnabled)
      {
         DBReconnect(hConn);
         dwResult = hConn->m_driver->m_callTable.Begin(hConn->m_connection);
      }
      if (dwResult == DBERR_SUCCESS)
      {
         hConn->m_transactionLevel++;
         bRet = true;
			nxlog_debug_tag(DEBUG_TAG_QUERY, 9, _T("BEGIN TRANSACTION successful (level %d)"), hConn->m_transactionLevel);
      }
      else
      {
         hConn->m_mutexTransLock.unlock();
			nxlog_debug_tag(DEBUG_TAG_QUERY, 9, _T("BEGIN TRANSACTION failed"), hConn->m_transactionLevel);
      }
   }
   else
   {
      hConn->m_transactionLevel++;
      bRet = true;
		nxlog_debug_tag(DEBUG_TAG_QUERY, 9, _T("BEGIN TRANSACTION successful (level %d)"), hConn->m_transactionLevel);
   }
   return bRet;
}

/**
 * Commit transaction
 */
bool LIBNXDB_EXPORTABLE DBCommit(DB_HANDLE hConn)
{
   bool bRet = false;

   hConn->m_mutexTransLock.lock();
   if (hConn->m_transactionLevel > 0)
   {
      hConn->m_transactionLevel--;
      if (hConn->m_transactionLevel == 0)
         bRet = (hConn->m_driver->m_callTable.Commit(hConn->m_connection) == DBERR_SUCCESS);
      else
         bRet = true;
		nxlog_debug_tag(DEBUG_TAG_QUERY, 9, _T("COMMIT TRANSACTION %s (level %d)"), bRet ? _T("successful") : _T("failed"), hConn->m_transactionLevel);
      hConn->m_mutexTransLock.unlock();
   }
   hConn->m_mutexTransLock.unlock();
   return bRet;
}

/**
 * Rollback transaction
 */
bool LIBNXDB_EXPORTABLE DBRollback(DB_HANDLE hConn)
{
   bool bRet = false;

   hConn->m_mutexTransLock.lock();
   if (hConn->m_transactionLevel > 0)
   {
      hConn->m_transactionLevel--;
      if (hConn->m_transactionLevel == 0)
         bRet = (hConn->m_driver->m_callTable.Rollback(hConn->m_connection) == DBERR_SUCCESS);
      else
         bRet = true;
		nxlog_debug_tag(DEBUG_TAG_QUERY, 9, _T("ROLLBACK TRANSACTION %s (level %d)"), bRet ? _T("successful") : _T("failed"), hConn->m_transactionLevel);
      hConn->m_mutexTransLock.unlock();
   }
   hConn->m_mutexTransLock.unlock();
   return bRet;
}

/**
 * Prepare string for using in SQL statement
 */
StringBuffer LIBNXDB_EXPORTABLE DBPrepareString(DB_HANDLE conn, const TCHAR *str, size_t maxSize)
{
   return conn->m_driver->m_callTable.PrepareString(CHECK_NULL_EX(str), (maxSize > 0) ? maxSize : INT_MAX);
}

/**
 * Prepare string for using in SQL statement
 */
StringBuffer LIBNXDB_EXPORTABLE DBPrepareString(DB_DRIVER drv, const TCHAR *str, size_t maxSize)
{
   return drv->m_callTable.PrepareString(CHECK_NULL_EX(str), (maxSize > 0) ? maxSize : INT_MAX);
}

/**
 * Prepare string for using in SQL statement (multi-byte string version)
 */
#ifdef UNICODE

StringBuffer LIBNXDB_EXPORTABLE DBPrepareStringA(DB_HANDLE conn, const char *str, size_t maxSize)
{
	WCHAR *wcs = WideStringFromMBString(str);
	StringBuffer s = DBPrepareString(conn, wcs, maxSize);
	MemFree(wcs);
	return s;
}

StringBuffer LIBNXDB_EXPORTABLE DBPrepareStringA(DB_DRIVER drv, const char *str, size_t maxSize)
{
   WCHAR *wcs = WideStringFromMBString(str);
   StringBuffer s = DBPrepareString(drv, wcs, maxSize);
   MemFree(wcs);
   return s;
}

#endif

/**
 * Prepare string for using in SQL statement (UTF8 string version)
 */
StringBuffer LIBNXDB_EXPORTABLE DBPrepareStringUTF8(DB_HANDLE conn, const char *str, size_t maxSize)
{
   return DBPrepareStringUTF8(conn->m_driver, str, maxSize);
}

/**
 * Prepare string for using in SQL statement (UTF8 string version)
 */
StringBuffer LIBNXDB_EXPORTABLE DBPrepareStringUTF8(DB_DRIVER drv, const char *str, size_t maxSize)
{
#ifdef UNICODE
   WCHAR *wcs = WideStringFromUTF8String(str);
   StringBuffer s = DBPrepareString(drv, wcs, maxSize);
   MemFree(wcs);
#else
   char *mbcs = MBStringFromUTF8String(str);
   StringBuffer s = DBPrepareString(drv, mbcs, maxSize);
   MemFree(mbcs);
#endif
   return s;
}

/**
 * Check if given table exist
 */
int LIBNXDB_EXPORTABLE DBIsTableExist(DB_HANDLE conn, const TCHAR *table)
{
#ifdef UNICODE
   return conn->m_driver->m_callTable.IsTableExist(conn->m_connection, table);
#else
   WCHAR wname[256];
   mb_to_wchar(table, -1, wname, 256);
   return conn->m_driver->m_callTable.IsTableExist(conn->m_connection, wname);
#endif
}

/**
 * Get performance counters
 */
void LIBNXDB_EXPORTABLE DBGetPerfCounters(LIBNXDB_PERF_COUNTERS *counters)
{
   counters->failedQueries = static_cast<uint64_t>(s_perfFailedQueries);
   counters->longRunningQueries = static_cast<uint64_t>(s_perfLongRunningQueries);
   counters->nonSelectQueries = static_cast<uint64_t>(s_perfNonSelectQueries);
   counters->selectQueries = static_cast<uint64_t>(s_perfSelectQueries);
   counters->totalQueries = static_cast<uint64_t>(s_perfTotalQueries);
}
