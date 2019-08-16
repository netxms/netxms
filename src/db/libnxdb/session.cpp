/*
** NetXMS - Network Management System
** Database Abstraction Library
** Copyright (C) 2003-2019 Victor Kirhenshtein
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
#define IS_VALID_STATEMENT_HANDLE(s) ((s != NULL) && (s->m_connection != NULL))

/**
 * Performance counters
 */
static UINT64 s_perfSelectQueries = 0;
static UINT64 s_perfNonSelectQueries = 0;
static UINT64 s_perfTotalQueries = 0;
static UINT64 s_perfLongRunningQueries = 0;
static UINT64 s_perfFailedQueries = 0;

/**
 * Session init callback
 */
static void (*s_sessionInitCb)(DB_HANDLE session) = NULL;

/**
 * Invalidate all prepared statements on connection
 */
static void InvalidatePreparedStatements(DB_HANDLE hConn)
{
   for(int i = 0; i < hConn->m_preparedStatements->size(); i++)
   {
      db_statement_t *stmt = hConn->m_preparedStatements->get(i);
      hConn->m_driver->m_fpDrvFreeStatement(stmt->m_statement);
      stmt->m_statement = NULL;
      stmt->m_connection = NULL;
   }
   hConn->m_preparedStatements->clear();
}

/**
 * Connect to database
 */
DB_HANDLE LIBNXDB_EXPORTABLE DBConnect(DB_DRIVER driver, const TCHAR *server, const TCHAR *dbName,
                                       const TCHAR *login, const TCHAR *password, const TCHAR *schema, TCHAR *errorText)
{
   DBDRV_CONNECTION hDrvConn;
   DB_HANDLE hConn = NULL;

	nxlog_debug_tag(DEBUG_TAG_CONNECTION, 8, _T("DBConnect: server=%s db=%s login=%s schema=%s"), CHECK_NULL(server), CHECK_NULL(dbName), CHECK_NULL(login), CHECK_NULL(schema));
#ifdef UNICODE
	char *mbServer = (server == NULL) ? NULL : MBStringFromWideString(server);
	char *mbDatabase = (dbName == NULL) ? NULL : MBStringFromWideString(dbName);
	char *mbLogin = (login == NULL) ? NULL : MBStringFromWideString(login);
	char *mbPassword = (password == NULL) ? NULL : MBStringFromWideString(password);
	char *mbSchema = (schema == NULL) ? NULL : MBStringFromWideString(schema);
	errorText[0] = 0;
   hDrvConn = driver->m_fpDrvConnect(mbServer, mbLogin, mbPassword, mbDatabase, mbSchema, errorText);
#else
	WCHAR wcErrorText[DBDRV_MAX_ERROR_TEXT] = L"";
   hDrvConn = driver->m_fpDrvConnect(server, login, password, dbName, schema, wcErrorText);
	WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, wcErrorText, -1, errorText, DBDRV_MAX_ERROR_TEXT, NULL, NULL);
	errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
#endif
   if (hDrvConn != NULL)
   {
      hConn = (DB_HANDLE)malloc(sizeof(struct db_handle_t));
      if (hConn != NULL)
      {
			hConn->m_driver = driver;
			hConn->m_dumpSql = driver->m_dumpSql;
			hConn->m_reconnectEnabled = true;
			hConn->m_connection = hDrvConn;
         hConn->m_mutexTransLock = MutexCreateRecursive();
         hConn->m_transactionLevel = 0;
         hConn->m_preparedStatements = new ObjectArray<db_statement_t>(4, 4, false);
#ifdef UNICODE
         hConn->m_dbName = mbDatabase;
         hConn->m_login = mbLogin;
         hConn->m_password = mbPassword;
         hConn->m_server = mbServer;
         hConn->m_schema = mbSchema;
#else
         hConn->m_dbName = (dbName == NULL) ? NULL : _tcsdup(dbName);
         hConn->m_login = (login == NULL) ? NULL : _tcsdup(login);
         hConn->m_password = (password == NULL) ? NULL : _tcsdup(password);
         hConn->m_server = (server == NULL) ? NULL : _tcsdup(server);
         hConn->m_schema = (schema == NULL) ? NULL : _tcsdup(schema);
#endif
         if (driver->m_fpDrvSetPrefetchLimit != NULL)
            driver->m_fpDrvSetPrefetchLimit(hDrvConn, driver->m_defaultPrefetchLimit);
		   nxlog_debug_tag(DEBUG_TAG_CONNECTION, 4, _T("New DB connection opened: handle=%p"), hConn);
         if (s_sessionInitCb != NULL)
            s_sessionInitCb(hConn);
      }
      else
      {
         driver->m_fpDrvDisconnect(hDrvConn);
      }
   }
#ifdef UNICODE
	if (hConn == NULL)
	{
		MemFree(mbServer);
		MemFree(mbDatabase);
		MemFree(mbLogin);
		MemFree(mbPassword);
		MemFree(mbSchema);
	}
#endif
   return hConn;
}

/**
 * Disconnect from database
 */
void LIBNXDB_EXPORTABLE DBDisconnect(DB_HANDLE hConn)
{
   if (hConn == NULL)
      return;

   nxlog_debug_tag(DEBUG_TAG_CONNECTION, 4, _T("DB connection %p closed"), hConn);
   
   InvalidatePreparedStatements(hConn);

	hConn->m_driver->m_fpDrvDisconnect(hConn->m_connection);
   MutexDestroy(hConn->m_mutexTransLock);
   MemFree(hConn->m_dbName);
   MemFree(hConn->m_login);
   MemFree(hConn->m_password);
   MemFree(hConn->m_server);
   MemFree(hConn->m_schema);
   delete hConn->m_preparedStatements;
   MemFree(hConn);
}

/**
 * Enable/disable reconnect
 */
void LIBNXDB_EXPORTABLE DBEnableReconnect(DB_HANDLE hConn, bool enabled)
{
	if (hConn != NULL)
	{
		hConn->m_reconnectEnabled = enabled;
	}
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
	hConn->m_driver->m_fpDrvDisconnect(hConn->m_connection);
   for(nCount = 0; ; nCount++)
   {
		hConn->m_connection = hConn->m_driver->m_fpDrvConnect(hConn->m_server, hConn->m_login,
                                                            hConn->m_password, hConn->m_dbName, hConn->m_schema, errorText);
      if (hConn->m_connection != NULL)
      {
         if (hConn->m_driver->m_fpDrvSetPrefetchLimit != NULL)
            hConn->m_driver->m_fpDrvSetPrefetchLimit(hConn->m_connection, hConn->m_driver->m_defaultPrefetchLimit);
         if (s_sessionInitCb != NULL)
            s_sessionInitCb(hConn);
         break;
      }
      if (nCount == 0)
      {
			MutexLock(hConn->m_driver->m_mutexReconnect);
         if ((hConn->m_driver->m_reconnect == 0) && (hConn->m_driver->m_fpEventHandler != NULL))
				hConn->m_driver->m_fpEventHandler(DBEVENT_CONNECTION_LOST, NULL, NULL, true, hConn->m_driver->m_userArg);
         hConn->m_driver->m_reconnect++;
         MutexUnlock(hConn->m_driver->m_mutexReconnect);
      }
      ThreadSleepMs(1000);
   }
   if (nCount > 0)
   {
      MutexLock(hConn->m_driver->m_mutexReconnect);
      hConn->m_driver->m_reconnect--;
      if ((hConn->m_driver->m_reconnect == 0) && (hConn->m_driver->m_fpEventHandler != NULL))
			hConn->m_driver->m_fpEventHandler(DBEVENT_CONNECTION_RESTORED, NULL, NULL, false, hConn->m_driver->m_userArg);
      MutexUnlock(hConn->m_driver->m_mutexReconnect);
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
   if (hConn->m_driver->m_fpDrvSetPrefetchLimit == NULL)
      return false;  // Not supported by driver
   return hConn->m_driver->m_fpDrvSetPrefetchLimit(hConn->m_connection, limit);
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
bool LIBNXDB_EXPORTABLE DBQueryEx(DB_HANDLE hConn, const TCHAR *szQuery, TCHAR *errorText)
{
   DWORD dwResult;
#ifdef UNICODE
#define pwszQuery szQuery
#define wcErrorText errorText
#else
   WCHAR *pwszQuery = WideStringFromMBString(szQuery);
	WCHAR wcErrorText[DBDRV_MAX_ERROR_TEXT] = L"";
#endif

   MutexLock(hConn->m_mutexTransLock);
   INT64 ms = GetCurrentTimeMs();

   dwResult = hConn->m_driver->m_fpDrvQuery(hConn->m_connection, pwszQuery, wcErrorText);
   if ((dwResult == DBERR_CONNECTION_LOST) && hConn->m_reconnectEnabled)
   {
      DBReconnect(hConn);
      dwResult = hConn->m_driver->m_fpDrvQuery(hConn->m_connection, pwszQuery, wcErrorText);
   }

   s_perfNonSelectQueries++;
   s_perfTotalQueries++;

   ms = GetCurrentTimeMs() - ms;
   if (hConn->m_driver->m_dumpSql)
   {
      nxlog_debug_tag(DEBUG_TAG_QUERY, 9, _T("%s sync query: \"%s\" [%d ms]"), (dwResult == DBERR_SUCCESS) ? _T("Successful") : _T("Failed"), szQuery, ms);
   }
   if ((dwResult == DBERR_SUCCESS) && ((UINT32)ms > g_sqlQueryExecTimeThreshold))
   {
      nxlog_debug_tag(DEBUG_TAG_QUERY, 3, _T("Long running query: \"%s\" [%d ms]"), szQuery, (int)ms);
      s_perfLongRunningQueries++;
   }
   
   MutexUnlock(hConn->m_mutexTransLock);

#ifndef UNICODE
	WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, wcErrorText, -1, errorText, DBDRV_MAX_ERROR_TEXT, NULL, NULL);
	errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
#endif

   if (dwResult != DBERR_SUCCESS)
	{	
      s_perfFailedQueries++;
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_DRIVER, _T("SQL query failed (Query = \"%s\"): %s"), szQuery, errorText);
		if (hConn->m_driver->m_fpEventHandler != NULL)
			hConn->m_driver->m_fpEventHandler(DBEVENT_QUERY_FAILED, pwszQuery, wcErrorText, dwResult == DBERR_CONNECTION_LOST, hConn->m_driver->m_userArg);
	}

#ifndef UNICODE
   free(pwszQuery);
#endif
   
   return dwResult == DBERR_SUCCESS;
#undef pwszQuery
#undef wcErrorText
}

bool LIBNXDB_EXPORTABLE DBQuery(DB_HANDLE hConn, const TCHAR *query)
{
   TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
	return DBQueryEx(hConn, query, errorText);
}

/**
 * Perform SELECT query
 */
DB_RESULT LIBNXDB_EXPORTABLE DBSelectEx(DB_HANDLE hConn, const TCHAR *szQuery, TCHAR *errorText)
{
   DBDRV_RESULT hResult;
	DB_RESULT result = NULL;
   DWORD dwError = DBERR_OTHER_ERROR;
#ifdef UNICODE
#define pwszQuery szQuery
#define wcErrorText errorText
#else
   WCHAR *pwszQuery = WideStringFromMBString(szQuery);
	WCHAR wcErrorText[DBDRV_MAX_ERROR_TEXT] = L"";
#endif
   
   MutexLock(hConn->m_mutexTransLock);
   INT64 ms = GetCurrentTimeMs();

   s_perfSelectQueries++;
   s_perfTotalQueries++;

   hResult = hConn->m_driver->m_fpDrvSelect(hConn->m_connection, pwszQuery, &dwError, wcErrorText);
   if ((hResult == NULL) && (dwError == DBERR_CONNECTION_LOST) && hConn->m_reconnectEnabled)
   {
      DBReconnect(hConn);
      hResult = hConn->m_driver->m_fpDrvSelect(hConn->m_connection, pwszQuery, &dwError, wcErrorText);
   }

   ms = GetCurrentTimeMs() - ms;
   if (hConn->m_driver->m_dumpSql)
   {
      nxlog_debug_tag(DEBUG_TAG_QUERY, 9, _T("%s sync query: \"%s\" [%d ms]"), (hResult != NULL) ? _T("Successful") : _T("Failed"), szQuery, (int)ms);
   }
   if ((hResult != NULL) && ((UINT32)ms > g_sqlQueryExecTimeThreshold))
   {
      nxlog_debug_tag(DEBUG_TAG_QUERY, 3, _T("Long running query: \"%s\" [%d ms]"), szQuery, (int)ms);
      s_perfLongRunningQueries++;
   }
   MutexUnlock(hConn->m_mutexTransLock);

#ifndef UNICODE
	WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, wcErrorText, -1, errorText, DBDRV_MAX_ERROR_TEXT, NULL, NULL);
	errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
#endif

	if (hResult == NULL)
	{
	   s_perfFailedQueries++;
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_DRIVER, _T("SQL query failed (Query = \"%s\"): %s"), szQuery, errorText);
		if (hConn->m_driver->m_fpEventHandler != NULL)
			hConn->m_driver->m_fpEventHandler(DBEVENT_QUERY_FAILED, pwszQuery, wcErrorText, dwError == DBERR_CONNECTION_LOST, hConn->m_driver->m_userArg);
	}

#ifndef UNICODE
   free(pwszQuery);
#endif
   
	if (hResult != NULL)
	{
		result = (DB_RESULT)malloc(sizeof(db_result_t));
		result->m_driver = hConn->m_driver;
		result->m_connection = hConn;
		result->m_data = hResult;
	}

   return result;
#undef pwszQuery
#undef wcErrorText
}

DB_RESULT LIBNXDB_EXPORTABLE DBSelect(DB_HANDLE hConn, const TCHAR *query)
{
   TCHAR errorText[DBDRV_MAX_ERROR_TEXT];

	return DBSelectEx(hConn, query, errorText);
}

/**
 * Get number of columns
 */
int LIBNXDB_EXPORTABLE DBGetColumnCount(DB_RESULT hResult)
{
	return hResult->m_driver->m_fpDrvGetColumnCount(hResult->m_data);
}

/**
 * Get column name
 */
bool LIBNXDB_EXPORTABLE DBGetColumnName(DB_RESULT hResult, int column, TCHAR *buffer, int bufSize)
{
	const char *name = hResult->m_driver->m_fpDrvGetColumnName(hResult->m_data, column);
	if (name != NULL)
	{
#ifdef UNICODE
		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, name, -1, buffer, bufSize);
		buffer[bufSize - 1] = 0;
#else
		strlcpy(buffer, name, bufSize);
#endif
	}
	return name != NULL;
}

/**
 * Get column name as multibyte string
 */
bool LIBNXDB_EXPORTABLE DBGetColumnNameA(DB_RESULT hResult, int column, char *buffer, int bufSize)
{
   const char *name = hResult->m_driver->m_fpDrvGetColumnName(hResult->m_data, column);
   if (name != NULL)
   {
      strlcpy(buffer, name, bufSize);
   }
   return name != NULL;
}

/**
 * Get column count for unbuffered result set
 */
int LIBNXDB_EXPORTABLE DBGetColumnCount(DB_UNBUFFERED_RESULT hResult)
{
	return hResult->m_driver->m_fpDrvGetColumnCountUnbuffered(hResult->m_data);
}

/**
 * Get column name for unbuffered result set
 */
bool LIBNXDB_EXPORTABLE DBGetColumnName(DB_UNBUFFERED_RESULT hResult, int column, TCHAR *buffer, int bufSize)
{
	const char *name = hResult->m_driver->m_fpDrvGetColumnNameUnbuffered(hResult->m_data, column);
	if (name != NULL)
	{
#ifdef UNICODE
		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, name, -1, buffer, bufSize);
		buffer[bufSize - 1] = 0;
#else
		strlcpy(buffer, name, bufSize);
#endif
	}
	return name != NULL;
}

/**
 * Get column name for unbuffered result set as multibyte string
 */
bool LIBNXDB_EXPORTABLE DBGetColumnNameA(DB_UNBUFFERED_RESULT hResult, int column, char *buffer, int bufSize)
{
   const char *name = hResult->m_driver->m_fpDrvGetColumnNameUnbuffered(hResult->m_data, column);
   if (name != NULL)
   {
      strlcpy(buffer, name, bufSize);
   }
   return name != NULL;
}

/**
 * Get field's value. If buffer is NULL, dynamically allocated string will be returned.
 * Caller is responsible for destroying it by calling free().
 */
TCHAR LIBNXDB_EXPORTABLE *DBGetField(DB_RESULT hResult, int iRow, int iColumn, TCHAR *pszBuffer, size_t nBufLen)
{
#ifdef UNICODE
   if (pszBuffer != NULL)
   {
      *pszBuffer = 0;
      return hResult->m_driver->m_fpDrvGetField(hResult->m_data, iRow, iColumn, pszBuffer, (int)nBufLen);
   }
   else
   {
      WCHAR *pszTemp;
      LONG nLen = hResult->m_driver->m_fpDrvGetFieldLength(hResult->m_data, iRow, iColumn);
      if (nLen == -1)
      {
         pszTemp = NULL;
      }
      else
      {
         nLen++;
         pszTemp = MemAllocStringW(nLen);
         hResult->m_driver->m_fpDrvGetField(hResult->m_data, iRow, iColumn, pszTemp, nLen);
      }
      return pszTemp;
   }
#else
	return DBGetFieldA(hResult, iRow, iColumn, pszBuffer, nBufLen);
#endif
}

/**
 * Get field's value as UTF8 string. If buffer is NULL, dynamically allocated string will be returned.
 * Caller is responsible for destroying it by calling free().
 */
char LIBNXDB_EXPORTABLE *DBGetFieldUTF8(DB_RESULT hResult, int iRow, int iColumn, char *pszBuffer, size_t nBufLen)
{
   if (hResult->m_driver->m_fpDrvGetFieldUTF8 != NULL)
   {
      if (pszBuffer != NULL)
      {
         *pszBuffer = 0;
         return hResult->m_driver->m_fpDrvGetFieldUTF8(hResult->m_data, iRow, iColumn, pszBuffer, (int)nBufLen);
      }
      else
      {
         char *pszTemp;
         LONG nLen = hResult->m_driver->m_fpDrvGetFieldLength(hResult->m_data, iRow, iColumn);
         if (nLen == -1)
         {
            pszTemp = NULL;
         }
         else
         {
            nLen = nLen * 2 + 1;  // increase buffer size because driver may return field length in characters
            pszTemp = (char *)malloc(nLen);
            hResult->m_driver->m_fpDrvGetFieldUTF8(hResult->m_data, iRow, iColumn, pszTemp, nLen);
         }
         return pszTemp;
      }
   }
   else
   {
      LONG nLen = hResult->m_driver->m_fpDrvGetFieldLength(hResult->m_data, iRow, iColumn);
      if (nLen == -1)
         return NULL;
      nLen = nLen * 2 + 1;  // increase buffer size because driver may return field length in characters

      WCHAR *wtemp = MemAllocStringW(nLen);
      hResult->m_driver->m_fpDrvGetField(hResult->m_data, iRow, iColumn, wtemp, nLen);
      char *value = (pszBuffer != NULL) ? pszBuffer : (char *)malloc(nLen);
      WideCharToMultiByte(CP_UTF8, 0, wtemp, -1, value, (pszBuffer != NULL) ? (int)nBufLen : nLen, NULL, NULL);
      MemFree(wtemp);
      return value;
   }
}

/**
 * Get field's value as multibyte string. If buffer is NULL, dynamically allocated string will be returned.
 * Caller is responsible for destroying it by calling free().
 */
char LIBNXDB_EXPORTABLE *DBGetFieldA(DB_RESULT hResult, int iRow, int iColumn, char *pszBuffer, size_t nBufLen)
{
   WCHAR *pwszData, *pwszBuffer;
   char *pszRet;
   int nLen;

   if (pszBuffer != NULL)
   {
      *pszBuffer = 0;
      pwszBuffer = MemAllocStringW(nBufLen);
      pwszData = hResult->m_driver->m_fpDrvGetField(hResult->m_data, iRow, iColumn, pwszBuffer, (int)nBufLen);
      if (pwszData != NULL)
      {
         WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, pwszData, -1, pszBuffer, (int)nBufLen, NULL, NULL);
         pszRet = pszBuffer;
      }
      else
      {
         pszRet = NULL;
      }
      MemFree(pwszBuffer);
   }
   else
   {
      nLen = hResult->m_driver->m_fpDrvGetFieldLength(hResult->m_data, iRow, iColumn);
      if (nLen == -1)
      {
         pszRet = NULL;
      }
      else
      {
         nLen++;
         pwszBuffer = MemAllocStringW(nLen);
         pwszData = hResult->m_driver->m_fpDrvGetField(hResult->m_data, iRow, iColumn, pwszBuffer, nLen);
         if (pwszData != NULL)
         {
            nLen = (int)wcslen(pwszData) + 1;
            pszRet = (char *)malloc(nLen);
            WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, pwszData, -1, pszRet, nLen, NULL, NULL);
         }
         else
         {
            pszRet = NULL;
         }
         MemFree(pwszBuffer);
      }
   }
   return pszRet;
}

/**
 * Get text field and escape it for XML document. Returned string
 * always dynamically allocated and must be destroyed by caller.
 */
TCHAR LIBNXDB_EXPORTABLE *DBGetFieldForXML(DB_RESULT hResult, int row, int col)
{
   TCHAR *value = DBGetField(hResult, row, col, NULL, 0);
   TCHAR *xmlString = EscapeStringForXML(value, -1);
   MemFree(value);
   return xmlString;
}

/**
 * Get field's value as unsigned long
 */
UINT32 LIBNXDB_EXPORTABLE DBGetFieldULong(DB_RESULT hResult, int iRow, int iColumn)
{
   INT32 iVal;
   UINT32 dwVal;
   TCHAR *pszVal, szBuffer[256];

   pszVal = DBGetField(hResult, iRow, iColumn, szBuffer, 256);
   if (pszVal == NULL)
      return 0;
	StrStrip(pszVal);
	if (*pszVal == _T('-'))
	{
		iVal = _tcstol(pszVal, NULL, 10);
		memcpy(&dwVal, &iVal, sizeof(INT32));   // To prevent possible conversion
	}
	else
	{
		dwVal = _tcstoul(pszVal, NULL, 10);
	}
   return dwVal;
}

/**
 * Get field's value as unsigned 64-bit int
 */
UINT64 LIBNXDB_EXPORTABLE DBGetFieldUInt64(DB_RESULT hResult, int iRow, int iColumn)
{
   INT64 iVal;
   UINT64 qwVal;
   TCHAR *pszVal, szBuffer[256];

   pszVal = DBGetField(hResult, iRow, iColumn, szBuffer, 256);
   if (pszVal == NULL)
      return 0;
	StrStrip(pszVal);
	if (*pszVal == _T('-'))
	{
		iVal = _tcstoll(pszVal, NULL, 10);
		memcpy(&qwVal, &iVal, sizeof(INT64));   // To prevent possible conversion
	}
	else
	{
		qwVal = _tcstoull(pszVal, NULL, 10);
	}
   return qwVal;
}

/**
 * Get field's value as signed long
 */
INT32 LIBNXDB_EXPORTABLE DBGetFieldLong(DB_RESULT hResult, int iRow, int iColumn)
{
   TCHAR *pszVal, szBuffer[256];

   pszVal = DBGetField(hResult, iRow, iColumn, szBuffer, 256);
   return pszVal == NULL ? 0 : _tcstol(pszVal, NULL, 10);
}

/**
 * Get field's value as signed 64-bit int
 */
INT64 LIBNXDB_EXPORTABLE DBGetFieldInt64(DB_RESULT hResult, int iRow, int iColumn)
{
   TCHAR *pszVal, szBuffer[256];

   pszVal = DBGetField(hResult, iRow, iColumn, szBuffer, 256);
   return pszVal == NULL ? 0 : _tcstoll(pszVal, NULL, 10);
}

/**
 * Get field's value as double
 */
double LIBNXDB_EXPORTABLE DBGetFieldDouble(DB_RESULT hResult, int iRow, int iColumn)
{
   TCHAR *pszVal, szBuffer[256];

   pszVal = DBGetField(hResult, iRow, iColumn, szBuffer, 256);
   return pszVal == NULL ? 0 : _tcstod(pszVal, NULL);
}

/**
 * Get field's value as IPv4 address
 */
UINT32 LIBNXDB_EXPORTABLE DBGetFieldIPAddr(DB_RESULT hResult, int iRow, int iColumn)
{
   TCHAR *pszVal, szBuffer[256];

   pszVal = DBGetField(hResult, iRow, iColumn, szBuffer, 256);
   return pszVal == NULL ? 0 : ntohl(_t_inet_addr(pszVal));
}

/**
 * Get field's value as IP address
 */
InetAddress LIBNXDB_EXPORTABLE DBGetFieldInetAddr(DB_RESULT hResult, int iRow, int iColumn)
{
   TCHAR *pszVal, szBuffer[256];

   pszVal = DBGetField(hResult, iRow, iColumn, szBuffer, 256);
   return pszVal == NULL ? InetAddress() : InetAddress::parse(pszVal);
}

/**
 * Get field`s value as MAC address
 */
MacAddress LIBNXDB_EXPORTABLE DBGetFieldMacAddr(DB_RESULT hResult, int iRow, int iColumn)
{
   TCHAR *pszVal, buffer[36];
   pszVal = DBGetField(hResult, iRow, iColumn, buffer, 36);

   return pszVal == NULL ? MacAddress(MacAddress::ZERO) : MacAddress::parse(pszVal);
}

/**
 * Get field's value as integer array from byte array encoded in hex
 */
bool LIBNXDB_EXPORTABLE DBGetFieldByteArray(DB_RESULT hResult, int iRow, int iColumn,
                                            int *pnArray, int nSize, int nDefault)
{
   char pbBytes[128];
   bool bResult;
   int i, nLen;
   TCHAR *pszVal, szBuffer[256];

   pszVal = DBGetField(hResult, iRow, iColumn, szBuffer, 256);
   if (pszVal != NULL)
   {
      StrToBin(pszVal, (BYTE *)pbBytes, 128);
      nLen = (int)_tcslen(pszVal) / 2;
      for(i = 0; (i < nSize) && (i < nLen); i++)
         pnArray[i] = pbBytes[i];
      for(; i < nSize; i++)
         pnArray[i] = nDefault;
      bResult = true;
   }
   else
   {
      for(i = 0; i < nSize; i++)
         pnArray[i] = nDefault;
      bResult = false;
   }
   return bResult;
}

/**
 * Get field's value as integer array from byte array encoded in hex
 */
bool LIBNXDB_EXPORTABLE DBGetFieldByteArray2(DB_RESULT hResult, int iRow, int iColumn,
                                             BYTE *data, int nSize, int nDefault)
{
   bool bResult;
   TCHAR *pszVal, szBuffer[256];

   pszVal = DBGetField(hResult, iRow, iColumn, szBuffer, 256);
   if (pszVal != NULL)
   {
      int bytes = (int)StrToBin(pszVal, data, nSize);
		if (bytes < nSize)
			memset(&data[bytes], 0, nSize - bytes);
      bResult = true;
   }
   else
   {
		memset(data, nDefault, nSize);
      bResult = false;
   }
   return bResult;
}

/**
 * Get field's value as GUID
 */
uuid LIBNXDB_EXPORTABLE DBGetFieldGUID(DB_RESULT hResult, int iRow, int iColumn)
{
   TCHAR buffer[256];
   TCHAR *value = DBGetField(hResult, iRow, iColumn, buffer, 256);
   return (value == NULL) ? uuid::NULL_UUID : uuid::parse(value);
}

/**
 * Get number of rows in result
 */
int LIBNXDB_EXPORTABLE DBGetNumRows(DB_RESULT hResult)
{
   if (hResult == NULL)
      return 0;
   return hResult->m_driver->m_fpDrvGetNumRows(hResult->m_data);
}

/**
 * Free result
 */
void LIBNXDB_EXPORTABLE DBFreeResult(DB_RESULT hResult)
{
   if (hResult != NULL)
	{
      hResult->m_driver->m_fpDrvFreeResult(hResult->m_data);
      MemFree(hResult);
	}
}

/**
 * Unbuffered SELECT query
 */
DB_UNBUFFERED_RESULT LIBNXDB_EXPORTABLE DBSelectUnbufferedEx(DB_HANDLE hConn, const TCHAR *szQuery, TCHAR *errorText)
{
   DBDRV_UNBUFFERED_RESULT hResult;
	DB_UNBUFFERED_RESULT result = NULL;
   DWORD dwError = DBERR_OTHER_ERROR;
#ifdef UNICODE
#define pwszQuery szQuery
#define wcErrorText errorText
#else
   WCHAR *pwszQuery = WideStringFromMBString(szQuery);
	WCHAR wcErrorText[DBDRV_MAX_ERROR_TEXT] = L"";
#endif
   
   MutexLock(hConn->m_mutexTransLock);
   INT64 ms = GetCurrentTimeMs();

   s_perfSelectQueries++;
   s_perfTotalQueries++;

   hResult = hConn->m_driver->m_fpDrvSelectUnbuffered(hConn->m_connection, pwszQuery, &dwError, wcErrorText);
   if ((hResult == NULL) && (dwError == DBERR_CONNECTION_LOST) && hConn->m_reconnectEnabled)
   {
      DBReconnect(hConn);
      hResult = hConn->m_driver->m_fpDrvSelectUnbuffered(hConn->m_connection, pwszQuery, &dwError, wcErrorText);
   }

   ms = GetCurrentTimeMs() - ms;
   if (hConn->m_driver->m_dumpSql)
   {
      nxlog_debug_tag(DEBUG_TAG_QUERY, 9, _T("%s unbuffered query: \"%s\" [%d ms]"), (hResult != NULL) ? _T("Successful") : _T("Failed"), szQuery, (int)ms);
   }
   if ((hResult != NULL) && ((UINT32)ms > g_sqlQueryExecTimeThreshold))
   {
      nxlog_debug_tag(DEBUG_TAG_QUERY, 3, _T("Long running query: \"%s\" [%d ms]"), szQuery, (int)ms);
      s_perfLongRunningQueries++;
   }
   if (hResult == NULL)
   {
      s_perfFailedQueries++;
      MutexUnlock(hConn->m_mutexTransLock);

#ifndef UNICODE
		WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, wcErrorText, -1, errorText, DBDRV_MAX_ERROR_TEXT, NULL, NULL);
		errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
#endif

      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_DRIVER, _T("SQL query failed (Query = \"%s\"): %s"), szQuery, errorText);
		if (hConn->m_driver->m_fpEventHandler != NULL)
			hConn->m_driver->m_fpEventHandler(DBEVENT_QUERY_FAILED, pwszQuery, wcErrorText, dwError == DBERR_CONNECTION_LOST, hConn->m_driver->m_userArg);
   }

#ifndef UNICODE
   free(pwszQuery);
#endif
   
	if (hResult != NULL)
	{
		result = (DB_UNBUFFERED_RESULT)malloc(sizeof(db_unbuffered_result_t));
		result->m_driver = hConn->m_driver;
		result->m_connection = hConn;
		result->m_data = hResult;
	}

   return result;
#undef pwszQuery
#undef wcErrorText
}

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
	return hResult->m_driver->m_fpDrvFetch(hResult->m_data);
}

/**
 * Get field's value from unbuffered SELECT result
 */
TCHAR LIBNXDB_EXPORTABLE *DBGetField(DB_UNBUFFERED_RESULT hResult, int iColumn, TCHAR *pBuffer, size_t iBufSize)
{
#ifdef UNICODE
   if (pBuffer != NULL)
   {
	   return hResult->m_driver->m_fpDrvGetFieldUnbuffered(hResult->m_data, iColumn, pBuffer, (int)iBufSize);
   }
   else
   {
      INT32 nLen;
      WCHAR *pszTemp;

      nLen = hResult->m_driver->m_fpDrvGetFieldLengthUnbuffered(hResult->m_data, iColumn);
      if (nLen == -1)
      {
         pszTemp = NULL;
      }
      else
      {
         nLen++;
         pszTemp = MemAllocStringW(nLen);
         hResult->m_driver->m_fpDrvGetFieldUnbuffered(hResult->m_data, iColumn, pszTemp, nLen);
      }
      return pszTemp;
   }
#else
   WCHAR *pwszData, *pwszBuffer;
   char *pszRet;
   int nLen;

   if (pBuffer != NULL)
   {
		pwszBuffer = MemAllocStringW(iBufSize);
		if (hResult->m_driver->m_fpDrvGetFieldUnbuffered(hResult->m_data, iColumn, pwszBuffer, (int)iBufSize) != NULL)
		{
			WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
									  pwszBuffer, -1, pBuffer, iBufSize, NULL, NULL);
			pszRet = pBuffer;
		}
		else
		{
			pszRet = NULL;
		}
		MemFree(pwszBuffer);
   }
   else
   {
		nLen = hResult->m_driver->m_fpDrvGetFieldLengthUnbuffered(hResult->m_data, iColumn);
      if (nLen == -1)
      {
         pszRet = NULL;
      }
      else
      {
         nLen++;
         pwszBuffer = MemAllocStringW(nLen);
			pwszData = hResult->m_driver->m_fpDrvGetFieldUnbuffered(hResult->m_data, iColumn, pwszBuffer, nLen);
         if (pwszData != NULL)
         {
            nLen = (int)wcslen(pwszData) + 1;
            pszRet = (char *)malloc(nLen);
            WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                                pwszData, -1, pszRet, nLen, NULL, NULL);
         }
         else
         {
            pszRet = NULL;
         }
         free(pwszBuffer);
      }
   }
   return pszRet;
#endif
}

/**
 * Get field's value from unbuffered SELECT result as UTF-8 string
 */
char LIBNXDB_EXPORTABLE *DBGetFieldUTF8(DB_UNBUFFERED_RESULT hResult, int iColumn, char *buffer, size_t iBufSize)
{
   if (hResult->m_driver->m_fpDrvGetFieldUTF8 != NULL)
   {
      if (buffer != NULL)
      {
         *buffer = 0;
         return hResult->m_driver->m_fpDrvGetFieldUnbufferedUTF8(hResult->m_data, iColumn, buffer, (int)iBufSize);
      }
      else
      {
         char *pszTemp;
         LONG nLen = hResult->m_driver->m_fpDrvGetFieldLengthUnbuffered(hResult->m_data, iColumn);
         if (nLen == -1)
         {
            pszTemp = NULL;
         }
         else
         {
            nLen = nLen * 2 + 1;  // increase buffer size because driver may return field length in characters
            pszTemp = MemAllocStringA(nLen);
            hResult->m_driver->m_fpDrvGetFieldUnbufferedUTF8(hResult->m_data, iColumn, pszTemp, nLen);
         }
         return pszTemp;
      }
   }
   else
   {
      LONG nLen = hResult->m_driver->m_fpDrvGetFieldLengthUnbuffered(hResult->m_data, iColumn);
      if (nLen == -1)
         return NULL;
      nLen = nLen * 2 + 1;  // increase buffer size because driver may return field length in characters

      WCHAR *wtemp = MemAllocStringW(nLen);
      hResult->m_driver->m_fpDrvGetFieldUnbuffered(hResult->m_data, iColumn, wtemp, nLen);
      char *value = (buffer != NULL) ? buffer : (char *)malloc(nLen);
      WideCharToMultiByte(CP_UTF8, 0, wtemp, -1, value, (buffer != NULL) ? (int)iBufSize : nLen, NULL, NULL);
      MemFree(wtemp);
      return value;
   }
}

/**
 * Get field's value as unsigned long from unbuffered SELECT result
 */
UINT32 LIBNXDB_EXPORTABLE DBGetFieldULong(DB_UNBUFFERED_RESULT hResult, int iColumn)
{
   INT32 iVal;
   UINT32 dwVal;
   TCHAR szBuffer[64];

   if (DBGetField(hResult, iColumn, szBuffer, 64) == NULL)
      return 0;
	StrStrip(szBuffer);
	if (szBuffer[0] == _T('-'))
	{
		iVal = _tcstol(szBuffer, NULL, 10);
		memcpy(&dwVal, &iVal, sizeof(INT32));   // To prevent possible conversion
	}
	else
	{
		dwVal = _tcstoul(szBuffer, NULL, 10);
	}
   return dwVal;
}

/**
 * Get field's value as unsigned 64-bit int from unbuffered SELECT result
 */
UINT64 LIBNXDB_EXPORTABLE DBGetFieldUInt64(DB_UNBUFFERED_RESULT hResult, int iColumn)
{
   INT64 iVal;
   UINT64 qwVal;
   TCHAR szBuffer[64];

   if (DBGetField(hResult, iColumn, szBuffer, 64) == NULL)
      return 0;
	StrStrip(szBuffer);
	if (szBuffer[0] == _T('-'))
	{
		iVal = _tcstoll(szBuffer, NULL, 10);
		memcpy(&qwVal, &iVal, sizeof(INT64));   // To prevent possible conversion
	}
	else
	{
		qwVal = _tcstoull(szBuffer, NULL, 10);
	}
   return qwVal;
}

/**
 * Get field's value as signed long from unbuffered SELECT result
 */
INT32 LIBNXDB_EXPORTABLE DBGetFieldLong(DB_UNBUFFERED_RESULT hResult, int iColumn)
{
   TCHAR szBuffer[64];
   return DBGetField(hResult, iColumn, szBuffer, 64) == NULL ? 0 : _tcstol(szBuffer, NULL, 10);
}

/**
 * Get field's value as signed 64-bit int from unbuffered SELECT result
 */
INT64 LIBNXDB_EXPORTABLE DBGetFieldInt64(DB_UNBUFFERED_RESULT hResult, int iColumn)
{
   TCHAR szBuffer[64];
   return DBGetField(hResult, iColumn, szBuffer, 64) == NULL ? 0 : _tcstoll(szBuffer, NULL, 10);
}

/**
 * Get field's value as signed long from unbuffered SELECT result
 */
double LIBNXDB_EXPORTABLE DBGetFieldDouble(DB_UNBUFFERED_RESULT hResult, int iColumn)
{
   TCHAR szBuffer[64];
   return DBGetField(hResult, iColumn, szBuffer, 64) == NULL ? 0 : _tcstod(szBuffer, NULL);
}

/**
 * Get field's value as IPv4 address from unbuffered SELECT result
 */
UINT32 LIBNXDB_EXPORTABLE DBGetFieldIPAddr(DB_UNBUFFERED_RESULT hResult, int iColumn)
{
   TCHAR buffer[64];
   return (DBGetField(hResult, iColumn, buffer, 64) == NULL) ? INADDR_NONE : ntohl(_t_inet_addr(buffer));
}

/**
 * Get field's value as IP address from unbuffered SELECT result
 */
InetAddress LIBNXDB_EXPORTABLE DBGetFieldInetAddr(DB_UNBUFFERED_RESULT hResult, int iColumn)
{
   TCHAR buffer[64];
   return (DBGetField(hResult, iColumn, buffer, 64) == NULL) ? InetAddress() : InetAddress::parse(buffer);
}

/**
 * Get field's value as GUID from unbuffered SELECT result
 */
uuid LIBNXDB_EXPORTABLE DBGetFieldGUID(DB_UNBUFFERED_RESULT hResult, int iColumn)
{
   TCHAR buffer[64];
   return (DBGetField(hResult, iColumn, buffer, 64) == NULL) ? uuid::NULL_UUID : uuid::parse(buffer);
}

/**
 * Free unbuffered SELECT result
 */
void LIBNXDB_EXPORTABLE DBFreeResult(DB_UNBUFFERED_RESULT hResult)
{
	hResult->m_driver->m_fpDrvFreeUnbufferedResult(hResult->m_data);
	MutexUnlock(hResult->m_connection->m_mutexTransLock);
	MemFree(hResult);
}

/**
 * Prepare statement
 */
DB_STATEMENT LIBNXDB_EXPORTABLE DBPrepareEx(DB_HANDLE hConn, const TCHAR *query, bool optimizeForReuse, TCHAR *errorText)
{
	DB_STATEMENT result = NULL;
	INT64 ms;

#ifdef UNICODE
#define pwszQuery query
#define wcErrorText errorText
#else
   WCHAR *pwszQuery = WideStringFromMBString(query);
	WCHAR wcErrorText[DBDRV_MAX_ERROR_TEXT] = L"";
#endif

	MutexLock(hConn->m_mutexTransLock);

	if (hConn->m_driver->m_dumpSql)
      ms = GetCurrentTimeMs();

	DWORD errorCode;
	DBDRV_STATEMENT stmt = hConn->m_driver->m_fpDrvPrepare(hConn->m_connection, pwszQuery, optimizeForReuse, &errorCode, wcErrorText);
   if ((stmt == NULL) && (errorCode == DBERR_CONNECTION_LOST) && hConn->m_reconnectEnabled)
   {
      DBReconnect(hConn);
		stmt = hConn->m_driver->m_fpDrvPrepare(hConn->m_connection, pwszQuery, optimizeForReuse, &errorCode, wcErrorText);
	}
   MutexUnlock(hConn->m_mutexTransLock);

	if (stmt != NULL)
	{
		result = (DB_STATEMENT)malloc(sizeof(db_statement_t));
		result->m_driver = hConn->m_driver;
		result->m_connection = hConn;
		result->m_statement = stmt;
		result->m_query = _tcsdup(query);
	}
	else
	{
#ifndef UNICODE
		WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, wcErrorText, -1, errorText, DBDRV_MAX_ERROR_TEXT, NULL, NULL);
		errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
#endif

      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_DRIVER, _T("SQL query failed (Query = \"%s\"): %s"), query, errorText);
		if (hConn->m_driver->m_fpEventHandler != NULL)
			hConn->m_driver->m_fpEventHandler(DBEVENT_QUERY_FAILED, pwszQuery, wcErrorText, errorCode == DBERR_CONNECTION_LOST, hConn->m_driver->m_userArg);

		s_perfFailedQueries++;
		s_perfTotalQueries++;
	}

   if (hConn->m_driver->m_dumpSql)
   {
      ms = GetCurrentTimeMs() - ms;
		nxlog_debug_tag(DEBUG_TAG_QUERY, 9, _T("{%p} %s prepare: \"%s\" [%d ms]"), result, (result != NULL) ? _T("Successful") : _T("Failed"), query, ms);
	}

#ifndef UNICODE
	free(pwszQuery);
#endif

   if (result != NULL)
   {
      hConn->m_preparedStatements->add(result);
   }

	return result;
#undef pwszQuery
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
   if (hStmt == NULL)
      return;

   if (hStmt->m_connection != NULL)
   {
      hStmt->m_connection->m_preparedStatements->remove(hStmt);
   }
   hStmt->m_driver->m_fpDrvFreeStatement(hStmt->m_statement);
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
   if (!IS_VALID_STATEMENT_HANDLE(hStmt) || (hStmt->m_driver->m_fpDrvOpenBatch == NULL))
      return false;
   return hStmt->m_driver->m_fpDrvOpenBatch(hStmt->m_statement);
}

/**
 * Start next batch row batch
 */
void LIBNXDB_EXPORTABLE DBNextBatchRow(DB_STATEMENT hStmt)
{
   if (IS_VALID_STATEMENT_HANDLE(hStmt) && (hStmt->m_driver->m_fpDrvNextBatchRow != NULL))
      hStmt->m_driver->m_fpDrvNextBatchRow(hStmt->m_statement);
}

/**
 * Bind parameter (generic)
 */
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, int cType, void *buffer, int allocType)
{
	if ((pos <= 0) || !IS_VALID_STATEMENT_HANDLE(hStmt))
		return;

	if (hStmt->m_connection->m_driver->m_dumpSql)
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
					_sntprintf(text, 64, _T("%d"), *((INT32 *)buffer));
					break;
				case DB_CTYPE_UINT32:
					_sntprintf(text, 64, _T("%u"), *((UINT32 *)buffer));
					break;
				case DB_CTYPE_INT64:
					_sntprintf(text, 64, INT64_FMT, *((INT64 *)buffer));
					break;
				case DB_CTYPE_UINT64:
					_sntprintf(text, 64, UINT64_FMT, *((UINT64 *)buffer));
					break;
				case DB_CTYPE_DOUBLE:
					_sntprintf(text, 64, _T("%f"), *((double *)buffer));
					break;
			}
			nxlog_debug_tag(DEBUG_TAG_QUERY, 9, _T("{%p} bind at pos %d: \"%s\""), hStmt, pos, text);
		}
	}

#ifdef UNICODE
#define wBuffer buffer
#define realAllocType allocType
#else
	void *wBuffer;
	int realAllocType = allocType;
	if (cType == DB_CTYPE_STRING)
	{
		wBuffer = (void *)WideStringFromMBString((char *)buffer);
		if (allocType == DB_BIND_DYNAMIC)
			free(buffer);
		realAllocType = DB_BIND_DYNAMIC;
	}
	else
	{
		wBuffer = buffer;
	}
#endif
	hStmt->m_driver->m_fpDrvBind(hStmt->m_statement, pos, sqlType, cType, wBuffer, realAllocType);
#undef wBuffer
#undef realAllocType
}

/**
 * Bind string parameter
 */
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, const TCHAR *value, int allocType)
{
	if (value != NULL)
		DBBind(hStmt, pos, sqlType, DB_CTYPE_STRING, (void *)value, allocType);
	else
		DBBind(hStmt, pos, sqlType, DB_CTYPE_STRING, (void *)_T(""), DB_BIND_STATIC);
}

/**
 * Bind string parameter with length validation
 */
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, const TCHAR *value, int allocType, int maxLen)
{
	if (value != NULL)
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
            TCHAR *temp = (TCHAR *)nx_memdup(value, sizeof(TCHAR) * (maxLen + 1));
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
 * Bind 32 bit integer parameter
 */
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, INT32 value)
{
	DBBind(hStmt, pos, sqlType, DB_CTYPE_INT32, &value, DB_BIND_TRANSIENT);
}

/**
 * Bind 32 bit unsigned integer parameter
 */
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, UINT32 value)
{
	// C type intentionally set to INT32 - unsigned numbers will be written as negatives
	// and correctly parsed on read by DBGetFieldULong
	// Setting it to UINT32 may cause INSERT/UPDATE failures on some databases
	DBBind(hStmt, pos, sqlType, DB_CTYPE_INT32, &value, DB_BIND_TRANSIENT);
}

/**
 * Bind 64 bit integer parameter
 */
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, INT64 value)
{
	DBBind(hStmt, pos, sqlType, DB_CTYPE_INT64, &value, DB_BIND_TRANSIENT);
}

/**
 * Bind 64 bit unsigned integer parameter
 */
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, UINT64 value)
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
 * Bind JSON object
 */
void LIBNXDB_EXPORTABLE DBBind(DB_STATEMENT hStmt, int pos, int sqlType, json_t *value, int allocType)
{
   if (value != NULL)
   {
      DBBind(hStmt, pos, sqlType, DB_CTYPE_UTF8_STRING, json_dumps(value, JSON_INDENT(3) | JSON_EMBED), DB_BIND_DYNAMIC);
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
#define wcErrorText errorText
#else
	WCHAR wcErrorText[DBDRV_MAX_ERROR_TEXT] = L"";
#endif

	DB_HANDLE hConn = hStmt->m_connection;
	MutexLock(hConn->m_mutexTransLock);
   UINT64 ms = GetCurrentTimeMs();

   s_perfNonSelectQueries++;
   s_perfTotalQueries++;

	DWORD dwResult = hConn->m_driver->m_fpDrvExecute(hConn->m_connection, hStmt->m_statement, wcErrorText);
   ms = GetCurrentTimeMs() - ms;
   if (hConn->m_driver->m_dumpSql)
   {
      nxlog_debug_tag(DEBUG_TAG_QUERY, 9, _T("%s prepared sync query: \"%s\" [%d ms]"), (dwResult == DBERR_SUCCESS) ? _T("Successful") : _T("Failed"), hStmt->m_query, (int)ms);
   }
   if ((dwResult == DBERR_SUCCESS) && ((UINT32)ms > g_sqlQueryExecTimeThreshold))
   {
      nxlog_debug_tag(DEBUG_TAG_QUERY, 3, _T("Long running query: \"%s\" [%d ms]"), hStmt->m_query, (int)ms);
      s_perfLongRunningQueries++;
   }

   // Do reconnect if needed, but don't retry statement execution
   // because it will fail anyway
   if ((dwResult == DBERR_CONNECTION_LOST) && hConn->m_reconnectEnabled)
   {
      DBReconnect(hConn);
   }
   
   MutexUnlock(hConn->m_mutexTransLock);

#ifndef UNICODE
	WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, wcErrorText, -1, errorText, DBDRV_MAX_ERROR_TEXT, NULL, NULL);
	errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
#endif

   if (dwResult != DBERR_SUCCESS)
	{	
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_DRIVER, _T("SQL query failed (Query = \"%s\"): %s"), hStmt->m_query, errorText);
		if (hConn->m_driver->m_fpEventHandler != NULL)
		{
#ifdef UNICODE
			hConn->m_driver->m_fpEventHandler(DBEVENT_QUERY_FAILED, hStmt->m_query, wcErrorText, dwResult == DBERR_CONNECTION_LOST, hConn->m_driver->m_userArg);
#else
			WCHAR *query = WideStringFromMBString(hStmt->m_query);
			hConn->m_driver->m_fpEventHandler(DBEVENT_QUERY_FAILED, query, wcErrorText, dwResult == DBERR_CONNECTION_LOST, hConn->m_driver->m_userArg);
			free(query);
#endif
		}
		s_perfFailedQueries++;
	}

   return dwResult == DBERR_SUCCESS;
#undef wcErrorText
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
      return NULL;
   }

	DB_RESULT result = NULL;
#ifdef UNICODE
#define wcErrorText errorText
#else
	WCHAR wcErrorText[DBDRV_MAX_ERROR_TEXT] = L"";
#endif

	DB_HANDLE hConn = hStmt->m_connection;
   MutexLock(hConn->m_mutexTransLock);

   s_perfSelectQueries++;
   s_perfTotalQueries++;

   INT64 ms = GetCurrentTimeMs();
   DWORD dwError = DBERR_OTHER_ERROR;
	DBDRV_RESULT hResult = hConn->m_driver->m_fpDrvSelectPrepared(hConn->m_connection, hStmt->m_statement, &dwError, wcErrorText);

   ms = GetCurrentTimeMs() - ms;
   if (hConn->m_driver->m_dumpSql)
   {
      nxlog_debug_tag(DEBUG_TAG_QUERY, 9, _T("%s prepared sync query: \"%s\" [%d ms]"),
		              (hResult != NULL) ? _T("Successful") : _T("Failed"), hStmt->m_query, (int)ms);
   }
   if ((hResult != NULL) && ((UINT32)ms > g_sqlQueryExecTimeThreshold))
   {
      nxlog_debug_tag(DEBUG_TAG_QUERY, 3, _T("Long running query: \"%s\" [%d ms]"), hStmt->m_query, (int)ms);
      s_perfLongRunningQueries++;
   }

   // Do reconnect if needed, but don't retry statement execution
   // because it will fail anyway
   if ((hResult == NULL) && (dwError == DBERR_CONNECTION_LOST) && hConn->m_reconnectEnabled)
   {
      DBReconnect(hConn);
   }

   MutexUnlock(hConn->m_mutexTransLock);

#ifndef UNICODE
	WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, wcErrorText, -1, errorText, DBDRV_MAX_ERROR_TEXT, NULL, NULL);
	errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
#endif

	if (hResult == NULL)
	{
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_DRIVER, _T("SQL query failed (Query = \"%s\"): %s"), hStmt->m_query, errorText);
		if (hConn->m_driver->m_fpEventHandler != NULL)
		{
#ifdef UNICODE
			hConn->m_driver->m_fpEventHandler(DBEVENT_QUERY_FAILED, hStmt->m_query, wcErrorText, dwError == DBERR_CONNECTION_LOST, hConn->m_driver->m_userArg);
#else
			WCHAR *query = WideStringFromMBString(hStmt->m_query);
			hConn->m_driver->m_fpEventHandler(DBEVENT_QUERY_FAILED, query, wcErrorText, dwError == DBERR_CONNECTION_LOST, hConn->m_driver->m_userArg);
			free(query);
#endif
		}
		s_perfFailedQueries++;
	}

	if (hResult != NULL)
	{
		result = (DB_RESULT)malloc(sizeof(db_result_t));
		result->m_driver = hConn->m_driver;
		result->m_connection = hConn;
		result->m_data = hResult;
	}

   return result;
#undef wcErrorText
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
      return NULL;
   }

   DB_UNBUFFERED_RESULT result = NULL;
#ifdef UNICODE
#define wcErrorText errorText
#else
   WCHAR wcErrorText[DBDRV_MAX_ERROR_TEXT] = L"";
#endif

   DB_HANDLE hConn = hStmt->m_connection;
   MutexLock(hConn->m_mutexTransLock);

   s_perfSelectQueries++;
   s_perfTotalQueries++;

   INT64 ms = GetCurrentTimeMs();
   DWORD dwError = DBERR_OTHER_ERROR;
   DBDRV_UNBUFFERED_RESULT hResult = hConn->m_driver->m_fpDrvSelectPreparedUnbuffered(hConn->m_connection, hStmt->m_statement, &dwError, wcErrorText);

   ms = GetCurrentTimeMs() - ms;
   if (hConn->m_driver->m_dumpSql)
   {
      nxlog_debug_tag(DEBUG_TAG_QUERY, 9, _T("%s prepared sync query: \"%s\" [%d ms]"),
                    (hResult != NULL) ? _T("Successful") : _T("Failed"), hStmt->m_query, (int)ms);
   }
   if ((hResult != NULL) && ((UINT32)ms > g_sqlQueryExecTimeThreshold))
   {
      nxlog_debug_tag(DEBUG_TAG_QUERY, 3, _T("Long running query: \"%s\" [%d ms]"), hStmt->m_query, (int)ms);
      s_perfLongRunningQueries++;
   }

   // Do reconnect if needed, but don't retry statement execution
   // because it will fail anyway
   if ((hResult == NULL) && (dwError == DBERR_CONNECTION_LOST) && hConn->m_reconnectEnabled)
   {
      DBReconnect(hConn);
   }

#ifndef UNICODE
   WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, wcErrorText, -1, errorText, DBDRV_MAX_ERROR_TEXT, NULL, NULL);
   errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
#endif

   if (hResult == NULL)
   {
      MutexUnlock(hConn->m_mutexTransLock);

      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_DRIVER, _T("SQL query failed (Query = \"%s\"): %s"), hStmt->m_query, errorText);
      if (hConn->m_driver->m_fpEventHandler != NULL)
      {
#ifdef UNICODE
         hConn->m_driver->m_fpEventHandler(DBEVENT_QUERY_FAILED, hStmt->m_query, wcErrorText, dwError == DBERR_CONNECTION_LOST, hConn->m_driver->m_userArg);
#else
         WCHAR *query = WideStringFromMBString(hStmt->m_query);
         hConn->m_driver->m_fpEventHandler(DBEVENT_QUERY_FAILED, query, wcErrorText, dwError == DBERR_CONNECTION_LOST, hConn->m_driver->m_userArg);
         free(query);
#endif
      }
      s_perfFailedQueries++;
   }

   if (hResult != NULL)
   {
      result = (DB_UNBUFFERED_RESULT)malloc(sizeof(db_unbuffered_result_t));
      result->m_driver = hConn->m_driver;
      result->m_connection = hConn;
      result->m_data = hResult;
   }

   return result;
#undef wcErrorText
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
   DWORD dwResult;
   bool bRet = false;

   MutexLock(hConn->m_mutexTransLock);
   if (hConn->m_transactionLevel == 0)
   {
      dwResult = hConn->m_driver->m_fpDrvBegin(hConn->m_connection);
      if ((dwResult == DBERR_CONNECTION_LOST) && hConn->m_reconnectEnabled)
      {
         DBReconnect(hConn);
         dwResult = hConn->m_driver->m_fpDrvBegin(hConn->m_connection);
      }
      if (dwResult == DBERR_SUCCESS)
      {
         hConn->m_transactionLevel++;
         bRet = true;
			nxlog_debug_tag(DEBUG_TAG_QUERY, 9, _T("BEGIN TRANSACTION successful (level %d)"), hConn->m_transactionLevel);
      }
      else
      {
         MutexUnlock(hConn->m_mutexTransLock);
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

   MutexLock(hConn->m_mutexTransLock);
   if (hConn->m_transactionLevel > 0)
   {
      hConn->m_transactionLevel--;
      if (hConn->m_transactionLevel == 0)
         bRet = (hConn->m_driver->m_fpDrvCommit(hConn->m_connection) == DBERR_SUCCESS);
      else
         bRet = true;
		nxlog_debug_tag(DEBUG_TAG_QUERY, 9, _T("COMMIT TRANSACTION %s (level %d)"), bRet ? _T("successful") : _T("failed"), hConn->m_transactionLevel);
      MutexUnlock(hConn->m_mutexTransLock);
   }
   MutexUnlock(hConn->m_mutexTransLock);
   return bRet;
}

/**
 * Rollback transaction
 */
bool LIBNXDB_EXPORTABLE DBRollback(DB_HANDLE hConn)
{
   bool bRet = false;

   MutexLock(hConn->m_mutexTransLock);
   if (hConn->m_transactionLevel > 0)
   {
      hConn->m_transactionLevel--;
      if (hConn->m_transactionLevel == 0)
         bRet = (hConn->m_driver->m_fpDrvRollback(hConn->m_connection) == DBERR_SUCCESS);
      else
         bRet = true;
		nxlog_debug_tag(DEBUG_TAG_QUERY, 9, _T("ROLLBACK TRANSACTION %s (level %d)"), bRet ? _T("successful") : _T("failed"), hConn->m_transactionLevel);
      MutexUnlock(hConn->m_mutexTransLock);
   }
   MutexUnlock(hConn->m_mutexTransLock);
   return bRet;
}

/**
 * Prepare string for using in SQL statement
 */
String LIBNXDB_EXPORTABLE DBPrepareString(DB_HANDLE conn, const TCHAR *str, int maxSize)
{
   return DBPrepareString(conn->m_driver, str, maxSize);
}

/**
 * Prepare string for using in SQL statement
 */
String LIBNXDB_EXPORTABLE DBPrepareString(DB_DRIVER drv, const TCHAR *str, int maxSize)
{
	String out;
	if ((maxSize > 0) && (str != NULL) && (maxSize < (int)_tcslen(str)))
	{
		TCHAR *temp = (TCHAR *)malloc((maxSize + 1) * sizeof(TCHAR));
		nx_strncpy(temp, str, maxSize + 1);
#ifdef UNICODE
		out.setBuffer(drv->m_fpDrvPrepareStringW(temp));
#else
		out.setBuffer(drv->m_fpDrvPrepareStringA(temp));
#endif
		MemFree(temp);
	}
	else	
	{
#ifdef UNICODE
		out.setBuffer(drv->m_fpDrvPrepareStringW(CHECK_NULL_EX(str)));
#else
		out.setBuffer(drv->m_fpDrvPrepareStringA(CHECK_NULL_EX(str)));
#endif
	}
	return out;
}

/**
 * Prepare string for using in SQL statement (multi-byte string version)
 */
#ifdef UNICODE

String LIBNXDB_EXPORTABLE DBPrepareStringA(DB_HANDLE conn, const char *str, int maxSize)
{
	WCHAR *wcs = WideStringFromMBString(str);
	String s = DBPrepareString(conn, wcs, maxSize);
	MemFree(wcs);
	return s;
}

String LIBNXDB_EXPORTABLE DBPrepareStringA(DB_DRIVER drv, const char *str, int maxSize)
{
   WCHAR *wcs = WideStringFromMBString(str);
   String s = DBPrepareString(drv, wcs, maxSize);
   MemFree(wcs);
   return s;
}

#endif

/**
 * Prepare string for using in SQL statement (UTF8 string version)
 */
String LIBNXDB_EXPORTABLE DBPrepareStringUTF8(DB_HANDLE conn, const char *str, int maxSize)
{
   return DBPrepareStringUTF8(conn->m_driver, str, maxSize);
}

/**
 * Prepare string for using in SQL statement (UTF8 string version)
 */
String LIBNXDB_EXPORTABLE DBPrepareStringUTF8(DB_DRIVER drv, const char *str, int maxSize)
{
#ifdef UNICODE
   WCHAR *wcs = WideStringFromUTF8String(str);
   String s = DBPrepareString(drv, wcs, maxSize);
   MemFree(wcs);
#else
   char *mbcs = MBStringFromUTF8String(str);
   String s = DBPrepareString(drv, mbcs, maxSize);
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
   return conn->m_driver->m_fpDrvIsTableExist(conn->m_connection, table);
#else
   WCHAR wname[256];
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, table, -1, wname, 256);
   return conn->m_driver->m_fpDrvIsTableExist(conn->m_connection, wname);
#endif
}

/**
 * Get performance counters
 */
void LIBNXDB_EXPORTABLE DBGetPerfCounters(LIBNXDB_PERF_COUNTERS *counters)
{
   counters->failedQueries = s_perfFailedQueries;
   counters->longRunningQueries = s_perfLongRunningQueries;
   counters->nonSelectQueries = s_perfNonSelectQueries;
   counters->selectQueries = s_perfSelectQueries;
   counters->totalQueries = s_perfTotalQueries;
}
