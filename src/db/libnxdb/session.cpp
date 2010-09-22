/*
** NetXMS - Network Management System
** Database Abstraction Library
** Copyright (C) 2003-2010 Victor Kirhenshtein
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


//
// Connect to database
//

DB_HANDLE LIBNXDB_EXPORTABLE DBConnect(DB_DRIVER driver, const TCHAR *pszServer, const TCHAR *pszDBName,
                                       const TCHAR *pszLogin, const TCHAR *pszPassword)
{
   DBDRV_CONNECTION hDrvConn;
   DB_HANDLE hConn = NULL;

	__DBDbgPrintf(8, _T("DBConnect: server=%s db=%s login=%s"), pszServer, pszDBName, pszLogin);
   hDrvConn = driver->m_fpDrvConnect(pszServer, pszLogin, pszPassword, pszDBName);
   if (hDrvConn != NULL)
   {
      hConn = (DB_HANDLE)malloc(sizeof(struct db_handle_t));
      if (hConn != NULL)
      {
			hConn->m_driver = driver;
			hConn->m_dumpSql = driver->m_dumpSql;
			hConn->m_connection = hDrvConn;
         hConn->m_mutexTransLock = MutexCreateRecursive();
         hConn->m_transactionLevel = 0;
         hConn->m_dbName = (pszDBName == NULL) ? NULL : _tcsdup(pszDBName);
         hConn->m_login = (pszLogin == NULL) ? NULL : _tcsdup(pszLogin);
         hConn->m_password = (pszPassword == NULL) ? NULL : _tcsdup(pszPassword);
         hConn->m_server = (pszServer == NULL) ? NULL : _tcsdup(pszServer);
		   __DBDbgPrintf(4, _T("New DB connection opened: handle=%p"), hConn);
      }
      else
      {
         driver->m_fpDrvDisconnect(hDrvConn);
      }
   }
   return hConn;
}


//
// Disconnect from database
//

void LIBNXDB_EXPORTABLE DBDisconnect(DB_HANDLE hConn)
{
   if (hConn == NULL)
      return;

   __DBDbgPrintf(4, _T("DB connection %p closed"), hConn);
   
	hConn->m_driver->m_fpDrvDisconnect(hConn->m_connection);
   MutexDestroy(hConn->m_mutexTransLock);
   safe_free(hConn->m_dbName);
   safe_free(hConn->m_login);
   safe_free(hConn->m_password);
   safe_free(hConn->m_server);
   free(hConn);
}


//
// Reconnect to database
//

static void DBReconnect(DB_HANDLE hConn)
{
   int nCount;

	hConn->m_driver->m_fpDrvDisconnect(hConn->m_connection);
   for(nCount = 0; ; nCount++)
   {
		hConn->m_connection = hConn->m_driver->m_fpDrvConnect(hConn->m_server, hConn->m_login,
                                                            hConn->m_password, hConn->m_dbName);
      if (hConn->m_connection != NULL)
         break;
      if (nCount == 0)
      {
			MutexLock(hConn->m_driver->m_mutexReconnect, INFINITE);
         if ((hConn->m_driver->m_reconnect == 0) && (hConn->m_driver->m_fpEventHandler != NULL))
				hConn->m_driver->m_fpEventHandler(DBEVENT_CONNECTION_LOST, NULL, NULL, hConn->m_driver->m_userArg);
         hConn->m_driver->m_reconnect++;
         MutexUnlock(hConn->m_driver->m_mutexReconnect);
      }
      ThreadSleepMs(1000);
   }
   if (nCount > 0)
   {
      MutexLock(hConn->m_driver->m_mutexReconnect, INFINITE);
      hConn->m_driver->m_reconnect--;
      if ((hConn->m_driver->m_reconnect == 0) && (hConn->m_driver->m_fpEventHandler != NULL))
			hConn->m_driver->m_fpEventHandler(DBEVENT_CONNECTION_RESTORED, NULL, NULL, hConn->m_driver->m_userArg);
      MutexUnlock(hConn->m_driver->m_mutexReconnect);
   }
}


//
// Perform a non-SELECT SQL query
//

BOOL LIBNXDB_EXPORTABLE DBQueryEx(DB_HANDLE hConn, const TCHAR *szQuery, TCHAR *errorText)
{
   DWORD dwResult;
   INT64 ms;
#ifdef UNICODE
#define pwszQuery szQuery
#else
   WCHAR *pwszQuery = WideStringFromMBString(szQuery);
#endif

   MutexLock(hConn->m_mutexTransLock, INFINITE);
   if (hConn->m_driver->m_dumpSql)
      ms = GetCurrentTimeMs();

   dwResult = hConn->m_driver->m_fpDrvQuery(hConn->m_connection, pwszQuery, errorText);
   if (dwResult == DBERR_CONNECTION_LOST)
   {
      DBReconnect(hConn);
      dwResult = hConn->m_driver->m_fpDrvQuery(hConn->m_connection, pwszQuery, errorText);
   }

#ifndef UNICODE
   free(pwszQuery);
#endif
   
   if (hConn->m_driver->m_dumpSql)
   {
      ms = GetCurrentTimeMs() - ms;
      __DBDbgPrintf(9, _T("%s sync query: \"%s\" [%d ms]"), (dwResult == DBERR_SUCCESS) ? _T("Successful") : _T("Failed"), szQuery, ms);
   }
   
   MutexUnlock(hConn->m_mutexTransLock);
   if (dwResult != DBERR_SUCCESS)
	{	
		if (hConn->m_driver->m_logSqlErrors)
			nxlog_write(g_sqlErrorMsgCode, EVENTLOG_ERROR_TYPE, "ss", szQuery, errorText);
		if (hConn->m_driver->m_fpEventHandler != NULL)
			hConn->m_driver->m_fpEventHandler(DBEVENT_QUERY_FAILED, szQuery, errorText, hConn->m_driver->m_userArg);
	}
   return dwResult == DBERR_SUCCESS;
#undef pwszQuery
}

BOOL LIBNXDB_EXPORTABLE DBQuery(DB_HANDLE hConn, const TCHAR *query)
{
   TCHAR errorText[DBDRV_MAX_ERROR_TEXT];

	return DBQueryEx(hConn, query, errorText);
}


//
// Perform SELECT query
//

DB_RESULT LIBNXDB_EXPORTABLE DBSelectEx(DB_HANDLE hConn, const TCHAR *szQuery, TCHAR *errorText)
{
   DBDRV_RESULT hResult;
	DB_RESULT result = NULL;
   DWORD dwError;
   INT64 ms;
#ifdef UNICODE
#define pwszQuery szQuery
#else
   WCHAR *pwszQuery = WideStringFromMBString(szQuery);
#endif
   
   MutexLock(hConn->m_mutexTransLock, INFINITE);
   if (hConn->m_driver->m_dumpSql)
      ms = GetCurrentTimeMs();
   hResult = hConn->m_driver->m_fpDrvSelect(hConn->m_connection, pwszQuery, &dwError, errorText);
   if ((hResult == NULL) && (dwError == DBERR_CONNECTION_LOST))
   {
      DBReconnect(hConn);
      hResult = hConn->m_driver->m_fpDrvSelect(hConn->m_connection, pwszQuery, &dwError, errorText);
   }

#ifndef UNICODE
   free(pwszQuery);
#endif
   
   if (hConn->m_driver->m_dumpSql)
   {
      ms = GetCurrentTimeMs() - ms;
      __DBDbgPrintf(9, _T("%s sync query: \"%s\" [%d ms]"), (hResult != NULL) ? _T("Successful") : _T("Failed"), szQuery, (DWORD)ms);
   }
   MutexUnlock(hConn->m_mutexTransLock);
   if (hResult == NULL)
	{
		if (hConn->m_driver->m_logSqlErrors)
			nxlog_write(g_sqlErrorMsgCode, EVENTLOG_ERROR_TYPE, "ss", szQuery, errorText);
		if (hConn->m_driver->m_fpEventHandler != NULL)
			hConn->m_driver->m_fpEventHandler(DBEVENT_QUERY_FAILED, szQuery, errorText, hConn->m_driver->m_userArg);
	}

	if (hResult != NULL)
	{
		result = (DB_RESULT)malloc(sizeof(db_result_t));
		result->m_driver = hConn->m_driver;
		result->m_connection = hConn;
		result->m_data = hResult;
	}

   return result;
}

DB_RESULT LIBNXDB_EXPORTABLE DBSelect(DB_HANDLE hConn, const TCHAR *query)
{
   TCHAR errorText[DBDRV_MAX_ERROR_TEXT];

	return DBSelectEx(hConn, query, errorText);
}


//
// Get number of columns
//

int LIBNXDB_EXPORTABLE DBGetColumnCount(DB_RESULT hResult)
{
	return hResult->m_driver->m_fpDrvGetColumnCount(hResult->m_data);
}


//
// Get column name
//

BOOL LIBNXDB_EXPORTABLE DBGetColumnName(DB_RESULT hResult, int column, TCHAR *buffer, int bufSize)
{
	const char *name;

	name = hResult->m_driver->m_fpDrvGetColumnName(hResult->m_data, column);
	if (name != NULL)
	{
#ifdef UNICODE
		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, name, -1, buffer, bufSize);
		buffer[bufSize - 1] = 0;
#else
		nx_strncpy(buffer, name, bufSize);
#endif
	}
	return name != NULL;
}


//
// Get column count for async request
//

int LIBNXDB_EXPORTABLE DBGetColumnCountAsync(DB_ASYNC_RESULT hResult)
{
	return hResult->m_driver->m_fpDrvGetColumnCountAsync(hResult->m_data);
}


//
// Get column name for async request
//

BOOL LIBNXDB_EXPORTABLE DBGetColumnNameAsync(DB_ASYNC_RESULT hResult, int column, TCHAR *buffer, int bufSize)
{
	const char *name;

	name = hResult->m_driver->m_fpDrvGetColumnNameAsync(hResult->m_data, column);
	if (name != NULL)
	{
#ifdef UNICODE
		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, name, -1, buffer, bufSize);
		buffer[bufSize - 1] = 0;
#else
		nx_strncpy(buffer, name, bufSize);
#endif
	}
	return name != NULL;
}


//
// Get field's value
//

TCHAR LIBNXDB_EXPORTABLE *DBGetField(DB_RESULT hResult, int iRow, int iColumn,
                                      TCHAR *pszBuffer, int nBufLen)
{
#ifdef UNICODE
   if (pszBuffer != NULL)
   {
      return hResult->m_driver->m_fpDrvGetField(hResult->m_data, iRow, iColumn, pszBuffer, nBufLen);
   }
   else
   {
      LONG nLen;
      WCHAR *pszTemp;

      nLen = hResult->m_driver->m_fpDrvGetFieldLength(hResult->m_data, iRow, iColumn);
      if (nLen == -1)
      {
         pszTemp = NULL;
      }
      else
      {
         nLen++;
         pszBuffer = (WCHAR *)malloc(nLen * sizeof(WCHAR));
         hResult->m_driver->m_fpDrvGetField(hResult->m_data, iRow, iColumn, pszTemp, nLen);
      }
      return pszTemp;
   }
#else
   WCHAR *pwszData, *pwszBuffer;
   char *pszRet;
   int nLen;

   if (pszBuffer != NULL)
   {
      pwszBuffer = (WCHAR *)malloc(nBufLen * sizeof(WCHAR));
      pwszData = hResult->m_driver->m_fpDrvGetField(hResult->m_data, iRow, iColumn, pwszBuffer, nBufLen);
      if (pwszData != NULL)
      {
         WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                             pwszData, -1, pszBuffer, nBufLen, NULL, NULL);
         pszRet = pszBuffer;
      }
      else
      {
         pszRet = NULL;
      }
      free(pwszBuffer);
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
         pwszBuffer = (WCHAR *)malloc(nLen * sizeof(WCHAR));
         pwszData = hResult->m_driver->m_fpDrvGetField(hResult->m_data, iRow, iColumn, pwszBuffer, nLen);
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


//
// Get field's value as unsigned long
//

DWORD LIBNXDB_EXPORTABLE DBGetFieldULong(DB_RESULT hResult, int iRow, int iColumn)
{
   LONG iVal;
   DWORD dwVal;
   TCHAR *pszVal, szBuffer[256];

   pszVal = DBGetField(hResult, iRow, iColumn, szBuffer, 256);
   if (pszVal == NULL)
      return 0;
   iVal = _tcstol(pszVal, NULL, 10);
   memcpy(&dwVal, &iVal, sizeof(LONG));   // To prevent possible conversion
   return dwVal;
}


//
// Get field's value as unsigned 64-bit int
//

QWORD LIBNXDB_EXPORTABLE DBGetFieldUInt64(DB_RESULT hResult, int iRow, int iColumn)
{
   INT64 iVal;
   QWORD qwVal;
   TCHAR *pszVal, szBuffer[256];

   pszVal = DBGetField(hResult, iRow, iColumn, szBuffer, 256);
   if (pszVal == NULL)
      return 0;
   iVal = _tcstoll(pszVal, NULL, 10);
   memcpy(&qwVal, &iVal, sizeof(INT64));   // To prevent possible conversion
   return qwVal;
}


//
// Get field's value as signed long
//

LONG LIBNXDB_EXPORTABLE DBGetFieldLong(DB_RESULT hResult, int iRow, int iColumn)
{
   TCHAR *pszVal, szBuffer[256];

   pszVal = DBGetField(hResult, iRow, iColumn, szBuffer, 256);
   return pszVal == NULL ? 0 : _tcstol(pszVal, NULL, 10);
}


//
// Get field's value as signed 64-bit int
//

INT64 LIBNXDB_EXPORTABLE DBGetFieldInt64(DB_RESULT hResult, int iRow, int iColumn)
{
   TCHAR *pszVal, szBuffer[256];

   pszVal = DBGetField(hResult, iRow, iColumn, szBuffer, 256);
   return pszVal == NULL ? 0 : _tcstoll(pszVal, NULL, 10);
}


//
// Get field's value as double
//

double LIBNXDB_EXPORTABLE DBGetFieldDouble(DB_RESULT hResult, int iRow, int iColumn)
{
   TCHAR *pszVal, szBuffer[256];

   pszVal = DBGetField(hResult, iRow, iColumn, szBuffer, 256);
   return pszVal == NULL ? 0 : _tcstod(pszVal, NULL);
}


//
// Get field's value as IP address
//

DWORD LIBNXDB_EXPORTABLE DBGetFieldIPAddr(DB_RESULT hResult, int iRow, int iColumn)
{
   TCHAR *pszVal, szBuffer[256];

   pszVal = DBGetField(hResult, iRow, iColumn, szBuffer, 256);
   return pszVal == NULL ? INADDR_NONE : ntohl(_t_inet_addr(pszVal));
}


//
// Get field's value as integer array from byte array encoded in hex
//

BOOL LIBNXDB_EXPORTABLE DBGetFieldByteArray(DB_RESULT hResult, int iRow, int iColumn,
                                             int *pnArray, int nSize, int nDefault)
{
   char pbBytes[128];
   BOOL bResult;
   int i, nLen;
   TCHAR *pszVal, szBuffer[256];

   pszVal = DBGetField(hResult, iRow, iColumn, szBuffer, 256);
   if (pszVal != NULL)
   {
      StrToBin(pszVal, (BYTE *)pbBytes, 128);
      nLen = (int)strlen(pszVal) / 2;
      for(i = 0; (i < nSize) && (i < nLen); i++)
         pnArray[i] = pbBytes[i];
      for(; i < nSize; i++)
         pnArray[i] = nDefault;
      bResult = TRUE;
   }
   else
   {
      for(i = 0; i < nSize; i++)
         pnArray[i] = nDefault;
      bResult = FALSE;
   }
   return bResult;
}


//
// Get field's value as GUID
//

BOOL LIBNXDB_EXPORTABLE DBGetFieldGUID(DB_RESULT hResult, int iRow,
                                        int iColumn, uuid_t guid)
{
   TCHAR *pszVal, szBuffer[256];
   BOOL bResult;

   pszVal = DBGetField(hResult, iRow, iColumn, szBuffer, 256);
   if (pszVal != NULL)
   {
      if (uuid_parse(pszVal, guid) == 0)
      {
         bResult = TRUE;
      }
      else
      {
         uuid_clear(guid);
         bResult = FALSE;
      }
   }
   else
   {
      uuid_clear(guid);
      bResult = FALSE;
   }
   return bResult;
}


//
// Get number of rows in result
//

int LIBNXDB_EXPORTABLE DBGetNumRows(DB_RESULT hResult)
{
   if (hResult == NULL)
      return 0;
   return hResult->m_driver->m_fpDrvGetNumRows(hResult->m_data);
}


//
// Free result
//

void LIBNXDB_EXPORTABLE DBFreeResult(DB_RESULT hResult)
{
   if (hResult != NULL)
	{
      hResult->m_driver->m_fpDrvFreeResult(hResult->m_data);
		free(hResult);
	}
}


//
// Asyncronous SELECT query
//

DB_ASYNC_RESULT LIBNXDB_EXPORTABLE DBAsyncSelectEx(DB_HANDLE hConn, const TCHAR *szQuery, TCHAR *errorText)
{
   DBDRV_ASYNC_RESULT hResult;
	DB_ASYNC_RESULT result = NULL;
   DWORD dwError;
   INT64 ms;
#ifdef UNICODE
#define pwszQuery szQuery
#else
   WCHAR *pwszQuery = WideStringFromMBString(szQuery);
#endif
   
   MutexLock(hConn->m_mutexTransLock, INFINITE);
	if (hConn->m_driver->m_dumpSql)
      ms = GetCurrentTimeMs();
   hResult = hConn->m_driver->m_fpDrvAsyncSelect(hConn->m_connection, pwszQuery, &dwError, errorText);
   if ((hResult == NULL) && (dwError == DBERR_CONNECTION_LOST))
   {
      DBReconnect(hConn);
      hResult = hConn->m_driver->m_fpDrvAsyncSelect(hConn->m_connection, pwszQuery, &dwError, errorText);
   }

#ifndef UNICODE
   free(pwszQuery);
#endif
   
   if (hConn->m_driver->m_dumpSql)
   {
      ms = GetCurrentTimeMs() - ms;
      __DBDbgPrintf(9, _T("%s async query: \"%s\" [%d ms]"), (hResult != NULL) ? _T("Successful") : _T("Failed"), szQuery, (DWORD)ms);
   }
   if (hResult == NULL)
   {
      MutexUnlock(hConn->m_mutexTransLock);
		if (hConn->m_driver->m_logSqlErrors)
        nxlog_write(g_sqlErrorMsgCode, EVENTLOG_ERROR_TYPE, "ss", szQuery, errorText);
		if (hConn->m_driver->m_fpEventHandler != NULL)
			hConn->m_driver->m_fpEventHandler(DBEVENT_QUERY_FAILED, szQuery, errorText, hConn->m_driver->m_userArg);
   }

	if (hResult != NULL)
	{
		result = (DB_ASYNC_RESULT)malloc(sizeof(db_async_result_t));
		result->m_driver = hConn->m_driver;
		result->m_connection = hConn;
		result->m_data = hResult;
	}

   return result;
}

DB_ASYNC_RESULT LIBNXDB_EXPORTABLE DBAsyncSelect(DB_HANDLE hConn, const TCHAR *query)
{
   TCHAR errorText[DBDRV_MAX_ERROR_TEXT];

	return DBAsyncSelectEx(hConn, query, errorText);
}


//
// Fetch next row from asynchronous SELECT result
//

BOOL LIBNXDB_EXPORTABLE DBFetch(DB_ASYNC_RESULT hResult)
{
	return hResult->m_driver->m_fpDrvFetch(hResult->m_data);
}


//
// Get field's value from asynchronous SELECT result
//

TCHAR LIBNXDB_EXPORTABLE *DBGetFieldAsync(DB_ASYNC_RESULT hResult, int iColumn, TCHAR *pBuffer, int iBufSize)
{
#ifdef UNICODE
   if (pBuffer != NULL)
   {
	   return hResult->m_driver->m_fpDrvGetFieldAsync(hResult->m_data, iColumn, pBuffer, iBufSize);
   }
   else
   {
      LONG nLen;
      WCHAR *pszTemp;

      nLen = hResult->m_driver->m_fpDrvGetFieldLengthAsync(hResult->m_data, iColumn);
      if (nLen == -1)
      {
         pszTemp = NULL;
      }
      else
      {
         nLen++;
         pszBuffer = (WCHAR *)malloc(nLen * sizeof(WCHAR));
         hResult->m_driver->m_fpDrvGetFieldAsync(hResult->m_data, iColumn, pszTemp, nLen);
      }
      return pszTemp;
   }
#else
   WCHAR *pwszData, *pwszBuffer;
   char *pszRet;
   int nLen;

   if (pBuffer != NULL)
   {
		pwszBuffer = (WCHAR *)malloc(iBufSize * sizeof(WCHAR));
		if (hResult->m_driver->m_fpDrvGetFieldAsync(hResult->m_data, iColumn, pwszBuffer, iBufSize) != NULL)
		{
			WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
									  pwszBuffer, -1, pBuffer, iBufSize, NULL, NULL);
			pszRet = pBuffer;
		}
		else
		{
			pszRet = NULL;
		}
		free(pwszBuffer);
   }
   else
   {
		nLen = hResult->m_driver->m_fpDrvGetFieldLengthAsync(hResult->m_data, iColumn);
      if (nLen == -1)
      {
         pszRet = NULL;
      }
      else
      {
         nLen++;
         pwszBuffer = (WCHAR *)malloc(nLen * sizeof(WCHAR));
			pwszData = hResult->m_driver->m_fpDrvGetFieldAsync(hResult->m_data, iColumn, pwszBuffer, nLen);
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


//
// Get field's value as unsigned long from asynchronous SELECT result
//

DWORD LIBNXDB_EXPORTABLE DBGetFieldAsyncULong(DB_ASYNC_RESULT hResult, int iColumn)
{
   LONG iVal;
   DWORD dwVal;
   TCHAR szBuffer[64];

   if (DBGetFieldAsync(hResult, iColumn, szBuffer, 64) == NULL)
      return 0;
   iVal = _tcstol(szBuffer, NULL, 10);
   memcpy(&dwVal, &iVal, sizeof(LONG));   // To prevent possible conversion
   return dwVal;
}


//
// Get field's value as unsigned 64-bit int from asynchronous SELECT result
//

QWORD LIBNXDB_EXPORTABLE DBGetFieldAsyncUInt64(DB_ASYNC_RESULT hResult, int iColumn)
{
   INT64 iVal;
   QWORD qwVal;
   TCHAR szBuffer[64];

   if (DBGetFieldAsync(hResult, iColumn, szBuffer, 64) == NULL)
      return 0;
   iVal = _tcstoll(szBuffer, NULL, 10);
   memcpy(&qwVal, &iVal, sizeof(INT64));   // To prevent possible conversion
   return qwVal;
}


//
// Get field's value as signed long from asynchronous SELECT result
//

LONG LIBNXDB_EXPORTABLE DBGetFieldAsyncLong(DB_ASYNC_RESULT hResult, int iColumn)
{
   TCHAR szBuffer[64];
   
   return DBGetFieldAsync(hResult, iColumn, szBuffer, 64) == NULL ? 0 : _tcstol(szBuffer, NULL, 10);
}


//
// Get field's value as signed 64-bit int from asynchronous SELECT result
//

INT64 LIBNXDB_EXPORTABLE DBGetFieldAsyncInt64(DB_ASYNC_RESULT hResult, int iColumn)
{
   TCHAR szBuffer[64];
   
   return DBGetFieldAsync(hResult, iColumn, szBuffer, 64) == NULL ? 0 : _tcstoll(szBuffer, NULL, 10);
}


//
// Get field's value as signed long from asynchronous SELECT result
//

double LIBNXDB_EXPORTABLE DBGetFieldAsyncDouble(DB_ASYNC_RESULT hResult, int iColumn)
{
   TCHAR szBuffer[64];
   
   return DBGetFieldAsync(hResult, iColumn, szBuffer, 64) == NULL ? 0 : _tcstod(szBuffer, NULL);
}


//
// Get field's value as IP address from asynchronous SELECT result
//

DWORD LIBNXDB_EXPORTABLE DBGetFieldAsyncIPAddr(DB_ASYNC_RESULT hResult, int iColumn)
{
   TCHAR szBuffer[64];
   
   return DBGetFieldAsync(hResult, iColumn, szBuffer, 64) == NULL ? INADDR_NONE : 
      ntohl(inet_addr(szBuffer));
}


//
// Free asynchronous SELECT result
//

void LIBNXDB_EXPORTABLE DBFreeAsyncResult(DB_HANDLE hConn, DB_ASYNC_RESULT hResult)
{
	hResult->m_driver->m_fpDrvFreeAsyncResult(hResult->m_data);
	free(hResult);
   MutexUnlock(hConn->m_mutexTransLock);
}


//
// Begin transaction
//

BOOL LIBNXDB_EXPORTABLE DBBegin(DB_HANDLE hConn)
{
   DWORD dwResult;
   BOOL bRet = FALSE;

   MutexLock(hConn->m_mutexTransLock, INFINITE);
   if (hConn->m_transactionLevel == 0)
   {
      dwResult = hConn->m_driver->m_fpDrvBegin(hConn->m_connection);
      if (dwResult == DBERR_CONNECTION_LOST)
      {
         DBReconnect(hConn);
         dwResult = hConn->m_driver->m_fpDrvBegin(hConn->m_connection);
      }
      if (dwResult == DBERR_SUCCESS)
      {
         hConn->m_transactionLevel++;
         bRet = TRUE;
			__DBDbgPrintf(9, _T("BEGIN TRANSACTION successful (level %d)"), hConn->m_transactionLevel);
      }
      else
      {
         MutexUnlock(hConn->m_mutexTransLock);
			__DBDbgPrintf(9, _T("BEGIN TRANSACTION failed"), hConn->m_transactionLevel);
      }
   }
   else
   {
      hConn->m_transactionLevel++;
      bRet = TRUE;
		__DBDbgPrintf(9, _T("BEGIN TRANSACTION successful (level %d)"), hConn->m_transactionLevel);
   }
   return bRet;
}


//
// Commit transaction
//

BOOL LIBNXDB_EXPORTABLE DBCommit(DB_HANDLE hConn)
{
   BOOL bRet = FALSE;

   MutexLock(hConn->m_mutexTransLock, INFINITE);
   if (hConn->m_transactionLevel > 0)
   {
      hConn->m_transactionLevel--;
      if (hConn->m_transactionLevel == 0)
         bRet = (hConn->m_driver->m_fpDrvCommit(hConn->m_connection) == DBERR_SUCCESS);
      else
         bRet = TRUE;
		__DBDbgPrintf(9, _T("COMMIT TRANSACTION %s (level %d)"), bRet ? _T("successful") : _T("failed"), hConn->m_transactionLevel);
      MutexUnlock(hConn->m_mutexTransLock);
   }
   MutexUnlock(hConn->m_mutexTransLock);
   return bRet;
}


//
// Begin transaction
//

BOOL LIBNXDB_EXPORTABLE DBRollback(DB_HANDLE hConn)
{
   BOOL bRet = FALSE;

   MutexLock(hConn->m_mutexTransLock, INFINITE);
   if (hConn->m_transactionLevel > 0)
   {
      hConn->m_transactionLevel--;
      if (hConn->m_transactionLevel == 0)
         bRet = (hConn->m_driver->m_fpDrvRollback(hConn->m_connection) == DBERR_SUCCESS);
      else
         bRet = TRUE;
		__DBDbgPrintf(9, _T("ROLLBACK TRANSACTION %s (level %d)"), bRet ? _T("successful") : _T("failed"), hConn->m_transactionLevel);
      MutexUnlock(hConn->m_mutexTransLock);
   }
   MutexUnlock(hConn->m_mutexTransLock);
   return bRet;
}


//
// Prepare string for using in SQL statement
//

String LIBNXDB_EXPORTABLE DBPrepareString(DB_HANDLE conn, const TCHAR *str, int maxSize)
{
	String out;
	if ((maxSize > 0) && (str != NULL) && (maxSize < (int)_tcslen(str)))
	{
		TCHAR *temp = (TCHAR *)malloc((maxSize + 1) * sizeof(TCHAR));
		nx_strncpy(temp, str, maxSize + 1);
		out.setBuffer(conn->m_driver->m_fpDrvPrepareString(temp));
		free(temp);
	}
	else	
	{
		out.setBuffer(conn->m_driver->m_fpDrvPrepareString(CHECK_NULL_EX(str)));
	}
	return out;
}


//
// Characters to be escaped before writing to SQL
//

static TCHAR m_szSpecialChars[] = _T("\x01\x02\x03\x04\x05\x06\x07\x08")
                                  _T("\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10")
                                  _T("\x11\x12\x13\x14\x15\x16\x17\x18")
                                  _T("\x19\x1A\x1B\x1C\x1D\x1E\x1F")
                                  _T("#%\"\\'\x7F");


//
// Escape some special characters in string for writing into database
//

TCHAR LIBNXDB_EXPORTABLE *EncodeSQLString(const TCHAR *pszIn)
{
   char *pszOut;
   int iPosIn, iPosOut, iStrSize;

   if ((pszIn != NULL) && (*pszIn != 0))
   {
      // Allocate destination buffer
      iStrSize = (int)_tcslen(pszIn) + 1;
      for(iPosIn = 0; pszIn[iPosIn] != 0; iPosIn++)
         if (_tcschr(m_szSpecialChars, pszIn[iPosIn])  != NULL)
            iStrSize += 2;
      pszOut = (char *)malloc(iStrSize);

      // Translate string
      for(iPosIn = 0, iPosOut = 0; pszIn[iPosIn] != 0; iPosIn++)
         if (strchr(m_szSpecialChars, pszIn[iPosIn]) != NULL)
         {
            pszOut[iPosOut++] = _T('#');
            pszOut[iPosOut++] = bin2hex(pszIn[iPosIn] >> 4);
            pszOut[iPosOut++] = bin2hex(pszIn[iPosIn] & 0x0F);
         }
         else
         {
            pszOut[iPosOut++] = pszIn[iPosIn];
         }
      pszOut[iPosOut] = 0;
   }
   else
   {
      // Encode empty strings as #00
      pszOut = (TCHAR *)malloc(4 * sizeof(TCHAR));
      _tcscpy(pszOut, _T("#00"));
   }
   return pszOut;
}


//
// Restore characters encoded by EncodeSQLString()
// Characters are decoded "in place"
//

void LIBNXDB_EXPORTABLE DecodeSQLString(TCHAR *pszStr)
{
   int iPosIn, iPosOut;

   if (pszStr == NULL)
      return;

   for(iPosIn = 0, iPosOut = 0; pszStr[iPosIn] != 0; iPosIn++)
   {
      if (pszStr[iPosIn] == _T('#'))
      {
         iPosIn++;
         pszStr[iPosOut] = hex2bin(pszStr[iPosIn]) << 4;
         iPosIn++;
         pszStr[iPosOut] |= hex2bin(pszStr[iPosIn]);
         iPosOut++;
      }
      else
      {
         pszStr[iPosOut++] = pszStr[iPosIn];
      }
   }
   pszStr[iPosOut] = 0;
}


//
// Get database schema version
// Will return 0 for unknown and -1 in case of SQL errors
//

int LIBNXDB_EXPORTABLE DBGetSchemaVersion(DB_HANDLE conn)
{
	DB_RESULT hResult;
	int version = 0;

	// Read schema version from 'metadata' table, where it should
	// be stored starting from schema version 87
	// We ignore SQL error in this case, because table 'metadata'
	// may not exist in old schema versions
   hResult = DBSelect(conn, _T("SELECT var_value FROM metadata WHERE var_name='SchemaVersion'"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         version = DBGetFieldLong(hResult, 0, 0);
      DBFreeResult(hResult);
   }

	// If database schema version is less than 87, version number
	// will be stored in 'config' table
	if (version == 0)
	{
		hResult = DBSelect(conn, _T("SELECT var_value FROM config WHERE var_name='DBFormatVersion'"));
		if (hResult != NULL)
		{
			if (DBGetNumRows(hResult) > 0)
				version = DBGetFieldLong(hResult, 0, 0);
			DBFreeResult(hResult);
		}
		else
		{
			version = -1;
		}
	}

	return version;
}


//
// Get database syntax
//

int LIBNXDB_EXPORTABLE DBGetSyntax(DB_HANDLE conn)
{
	DB_RESULT hResult;
	TCHAR syntaxId[256];
	BOOL read = FALSE;
	int syntax;

   // Get database syntax
   hResult = DBSelect(conn, _T("SELECT var_value FROM metadata WHERE var_name='Syntax'"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         DBGetField(hResult, 0, 0, syntaxId, sizeof(syntaxId));
			read = TRUE;
      }
      else
      {
         _tcscpy(syntaxId, _T("UNKNOWN"));
      }
      DBFreeResult(hResult);
   }

	// If database schema version is less than 87, syntax
	// will be stored in 'config' table, so try it
	if (!read)
	{
		hResult = DBSelect(conn, _T("SELECT var_value FROM config WHERE var_name='DBSyntax'"));
		if (hResult != NULL)
		{
			if (DBGetNumRows(hResult) > 0)
			{
				DBGetField(hResult, 0, 0, syntaxId, sizeof(syntaxId));
				read = TRUE;
			}
			else
			{
				_tcscpy(syntaxId, _T("UNKNOWN"));
			}
			DBFreeResult(hResult);
		}
	}

   if (!_tcscmp(syntaxId, _T("MYSQL")))
   {
      syntax = DB_SYNTAX_MYSQL;
   }
   else if (!_tcscmp(syntaxId, _T("PGSQL")))
   {
      syntax = DB_SYNTAX_PGSQL;
   }
   else if (!_tcscmp(syntaxId, _T("MSSQL")))
   {
      syntax = DB_SYNTAX_MSSQL;
   }
   else if (!_tcscmp(syntaxId, _T("ORACLE")))
   {
      syntax = DB_SYNTAX_ORACLE;
   }
   else if (!_tcscmp(syntaxId, _T("SQLITE")))
   {
      syntax = DB_SYNTAX_SQLITE;
   }
   else if (!_tcscmp(syntaxId, _T("DB2")))
   {
      syntax = DB_SYNTAX_DB2;
   }
   else
   {
		syntax = DB_SYNTAX_UNKNOWN;
   }

	return syntax;
}
