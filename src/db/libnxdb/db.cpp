/*
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Victor Kirhenshtein
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
** File: db.cpp
**
**/

#include "libnxdb.h"


//
// Global variables
//

TCHAR LIBNXDB_EXPORTABLE g_szDbDriver[MAX_PATH] = _T("");
TCHAR LIBNXDB_EXPORTABLE g_szDbDrvParams[MAX_PATH] = _T("");
TCHAR LIBNXDB_EXPORTABLE g_szDbServer[MAX_PATH] = _T("127.0.0.1");
TCHAR LIBNXDB_EXPORTABLE g_szDbLogin[MAX_DB_LOGIN] = _T("netxms");
TCHAR LIBNXDB_EXPORTABLE g_szDbPassword[MAX_DB_PASSWORD] = _T("");
TCHAR LIBNXDB_EXPORTABLE g_szDbName[MAX_DB_NAME] = _T("netxms_db");


//
// Static data
//

static BOOL m_bWriteLog = FALSE;
static BOOL m_bLogSQLErrors = FALSE;
static DWORD m_logMsgCode = 0;
static DWORD m_sqlErrorMsgCode = 0;
static BOOL m_bDumpSQL = FALSE;
static int m_nReconnect = 0;
static MUTEX m_mutexReconnect = INVALID_MUTEX_HANDLE;
static HMODULE m_hDriver = NULL;
static DB_CONNECTION (* m_fpDrvConnect)(const char *, const char *, const char *, const char *) = NULL;
static void (* m_fpDrvDisconnect)(DB_CONNECTION) = NULL;
static DWORD (* m_fpDrvQuery)(DB_CONNECTION, WCHAR *, TCHAR *) = NULL;
static DB_RESULT (* m_fpDrvSelect)(DB_CONNECTION, WCHAR *, DWORD *, TCHAR *) = NULL;
static DB_ASYNC_RESULT (* m_fpDrvAsyncSelect)(DB_CONNECTION, WCHAR *, DWORD *, TCHAR *) = NULL;
static BOOL (* m_fpDrvFetch)(DB_ASYNC_RESULT) = NULL;
static LONG (* m_fpDrvGetFieldLength)(DB_RESULT, int, int) = NULL;
static LONG (* m_fpDrvGetFieldLengthAsync)(DB_RESULT, int) = NULL;
static WCHAR* (* m_fpDrvGetField)(DB_RESULT, int, int, WCHAR *, int) = NULL;
static WCHAR* (* m_fpDrvGetFieldAsync)(DB_ASYNC_RESULT, int, WCHAR *, int) = NULL;
static int (* m_fpDrvGetNumRows)(DB_RESULT) = NULL;
static void (* m_fpDrvFreeResult)(DB_RESULT) = NULL;
static void (* m_fpDrvFreeAsyncResult)(DB_ASYNC_RESULT) = NULL;
static DWORD (* m_fpDrvBegin)(DB_CONNECTION) = NULL;
static DWORD (* m_fpDrvCommit)(DB_CONNECTION) = NULL;
static DWORD (* m_fpDrvRollback)(DB_CONNECTION) = NULL;
static void (* m_fpDrvUnload)(void) = NULL;
static void (* m_fpEventHandler)(DWORD, const TCHAR *, const TCHAR *);
static int (* m_fpDrvGetColumnCount)(DB_RESULT);
static const char* (* m_fpDrvGetColumnName)(DB_RESULT, int);
static int (* m_fpDrvGetColumnCountAsync)(DB_ASYNC_RESULT);
static const char* (* m_fpDrvGetColumnNameAsync)(DB_ASYNC_RESULT, int);
static TCHAR* (* m_fpDrvPrepareString)(const TCHAR *) = NULL;


//
// Write log
//

static void WriteLog(WORD level, const TCHAR *format, ...)
{
	TCHAR buffer[4096];
	va_list args;
	
	va_start(args, format);
	_vsntprintf(buffer, 4096, format, args);
	va_end(args);
	nxlog_write(m_logMsgCode, level, "s", buffer);
}


//
// Get symbol address and log errors
//

static void *DLGetSymbolAddrEx(HMODULE hModule, const TCHAR *pszSymbol)
{
   void *pFunc;
   char szErrorText[256];

   pFunc = DLGetSymbolAddr(hModule, pszSymbol, szErrorText);
   if ((pFunc == NULL) && m_bWriteLog)
      WriteLog(EVENTLOG_WARNING_TYPE, _T("Unable to resolve symbol \"%s\": %s"), pszSymbol, szErrorText);
   return pFunc;
}


//
// Load and initialize database driver
// If logMsgCode == 0, logging will be disabled
// If sqlErrorMsgCode == 0, failed SQL queries will not be logged
//

BOOL LIBNXDB_EXPORTABLE DBInit(DWORD logMsgCode, DWORD sqlErrorMsgCode, BOOL bDumpSQL,
                                void (* fpEventHandler)(DWORD, const TCHAR *, const TCHAR *))
{
   BOOL (* fpDrvInit)(char *);
   DWORD *pdwAPIVersion;
   char szErrorText[256];
   static DWORD dwVersionZero = 0;

	m_logMsgCode = logMsgCode;
   m_bWriteLog = (logMsgCode > 0);
	m_sqlErrorMsgCode = sqlErrorMsgCode;
   m_bLogSQLErrors = (sqlErrorMsgCode > 0) && m_bWriteLog;
   m_bDumpSQL = bDumpSQL;
   m_fpEventHandler = fpEventHandler;
   m_nReconnect = 0;
   m_mutexReconnect = MutexCreate();

   // Load driver's module
   m_hDriver = DLOpen(g_szDbDriver, szErrorText);
   if (m_hDriver == NULL)
   {
      if (m_bWriteLog)
         WriteLog(EVENTLOG_ERROR_TYPE, _T("Unable to load module \"%s\": %s"), g_szDbDriver, szErrorText);
      return FALSE;
   }

   // Check API version supported by driver
   pdwAPIVersion = (DWORD *)DLGetSymbolAddr(m_hDriver, "drvAPIVersion", NULL);
   if (pdwAPIVersion == NULL)
      pdwAPIVersion = &dwVersionZero;
   if (*pdwAPIVersion != DBDRV_API_VERSION)
   {
      if (m_bWriteLog)
         WriteLog(EVENTLOG_ERROR_TYPE, _T("Database driver \"%s\" cannot be loaded because of API version mismatch (driver: %d; server: %d)"),
                  g_szDbDriver, (int)(DBDRV_API_VERSION), (int)(*pdwAPIVersion));
      DLClose(m_hDriver);
      m_hDriver = NULL;
      return FALSE;
   }

   // Import symbols
   fpDrvInit = (BOOL (*)(char *))DLGetSymbolAddrEx(m_hDriver, "DrvInit");
   m_fpDrvConnect = (DB_CONNECTION (*)(const char *, const char *, const char *, const char *))DLGetSymbolAddrEx(m_hDriver, "DrvConnect");
   m_fpDrvDisconnect = (void (*)(DB_CONNECTION))DLGetSymbolAddrEx(m_hDriver, "DrvDisconnect");
   m_fpDrvQuery = (DWORD (*)(DB_CONNECTION, WCHAR *, TCHAR *))DLGetSymbolAddrEx(m_hDriver, "DrvQuery");
   m_fpDrvSelect = (DB_RESULT (*)(DB_CONNECTION, WCHAR *, DWORD *, TCHAR *))DLGetSymbolAddrEx(m_hDriver, "DrvSelect");
   m_fpDrvAsyncSelect = (DB_ASYNC_RESULT (*)(DB_CONNECTION, WCHAR *, DWORD *, TCHAR *))DLGetSymbolAddrEx(m_hDriver, "DrvAsyncSelect");
   m_fpDrvFetch = (BOOL (*)(DB_ASYNC_RESULT))DLGetSymbolAddrEx(m_hDriver, "DrvFetch");
   m_fpDrvGetFieldLength = (LONG (*)(DB_RESULT, int, int))DLGetSymbolAddrEx(m_hDriver, "DrvGetFieldLength");
   m_fpDrvGetFieldLengthAsync = (LONG (*)(DB_RESULT, int))DLGetSymbolAddrEx(m_hDriver, "DrvGetFieldLengthAsync");
   m_fpDrvGetField = (WCHAR* (*)(DB_RESULT, int, int, WCHAR *, int))DLGetSymbolAddrEx(m_hDriver, "DrvGetField");
   m_fpDrvGetFieldAsync = (WCHAR* (*)(DB_ASYNC_RESULT, int, WCHAR *, int))DLGetSymbolAddrEx(m_hDriver, "DrvGetFieldAsync");
   m_fpDrvGetNumRows = (int (*)(DB_RESULT))DLGetSymbolAddrEx(m_hDriver, "DrvGetNumRows");
   m_fpDrvGetColumnCount = (int (*)(DB_RESULT))DLGetSymbolAddrEx(m_hDriver, "DrvGetColumnCount");
   m_fpDrvGetColumnName = (const char* (*)(DB_RESULT, int))DLGetSymbolAddrEx(m_hDriver, "DrvGetColumnName");
   m_fpDrvGetColumnCountAsync = (int (*)(DB_ASYNC_RESULT))DLGetSymbolAddrEx(m_hDriver, "DrvGetColumnCountAsync");
   m_fpDrvGetColumnNameAsync = (const char* (*)(DB_ASYNC_RESULT, int))DLGetSymbolAddrEx(m_hDriver, "DrvGetColumnNameAsync");
   m_fpDrvFreeResult = (void (*)(DB_RESULT))DLGetSymbolAddrEx(m_hDriver, "DrvFreeResult");
   m_fpDrvFreeAsyncResult = (void (*)(DB_ASYNC_RESULT))DLGetSymbolAddrEx(m_hDriver, "DrvFreeAsyncResult");
   m_fpDrvBegin = (DWORD (*)(DB_CONNECTION))DLGetSymbolAddrEx(m_hDriver, "DrvBegin");
   m_fpDrvCommit = (DWORD (*)(DB_CONNECTION))DLGetSymbolAddrEx(m_hDriver, "DrvCommit");
   m_fpDrvRollback = (DWORD (*)(DB_CONNECTION))DLGetSymbolAddrEx(m_hDriver, "DrvRollback");
   m_fpDrvUnload = (void (*)(void))DLGetSymbolAddrEx(m_hDriver, "DrvUnload");
   m_fpDrvPrepareString = (TCHAR* (*)(const TCHAR *))DLGetSymbolAddrEx(m_hDriver, "DrvPrepareString");
   if ((fpDrvInit == NULL) || (m_fpDrvConnect == NULL) || (m_fpDrvDisconnect == NULL) ||
       (m_fpDrvQuery == NULL) || (m_fpDrvSelect == NULL) || (m_fpDrvGetField == NULL) ||
       (m_fpDrvGetNumRows == NULL) || (m_fpDrvFreeResult == NULL) || 
       (m_fpDrvUnload == NULL) || (m_fpDrvAsyncSelect == NULL) || (m_fpDrvFetch == NULL) ||
       (m_fpDrvFreeAsyncResult == NULL) || (m_fpDrvGetFieldAsync == NULL) ||
       (m_fpDrvBegin == NULL) || (m_fpDrvCommit == NULL) || (m_fpDrvRollback == NULL) ||
		 (m_fpDrvGetColumnCount == NULL) || (m_fpDrvGetColumnName == NULL) ||
		 (m_fpDrvGetColumnCountAsync == NULL) || (m_fpDrvGetColumnNameAsync == NULL) ||
       (m_fpDrvGetFieldLength == NULL) || (m_fpDrvGetFieldLengthAsync == NULL) ||
		 (m_fpDrvPrepareString == NULL))
   {
      if (m_bWriteLog)
         WriteLog(EVENTLOG_ERROR_TYPE, _T("Unable to find all required exportable functions in database driver \"%s\""), g_szDbDriver);
      DLClose(m_hDriver);
      m_hDriver = NULL;
      return FALSE;
   }

   // Initialize driver
   if (!fpDrvInit(g_szDbDrvParams))
   {
      if (m_bWriteLog)
         WriteLog(EVENTLOG_ERROR_TYPE, _T("Database driver \"%s\" initialization failed"), g_szDbDriver);
      DLClose(m_hDriver);
      m_hDriver = NULL;
      return FALSE;
   }

   // Success
   if (m_bWriteLog)
      WriteLog(EVENTLOG_INFORMATION_TYPE, _T("Database driver \"%s\" loaded and initialized successfully"), g_szDbDriver);
   return TRUE;
}


//
// Notify driver of unload
//

void LIBNXDB_EXPORTABLE DBUnloadDriver(void)
{
   m_fpDrvUnload();
   DLClose(m_hDriver);
   MutexDestroy(m_mutexReconnect);
}


//
// Connect to database
//

DB_HANDLE LIBNXDB_EXPORTABLE DBConnect(void)
{
   return DBConnectEx(g_szDbServer, g_szDbName, g_szDbLogin, g_szDbPassword);
}


//
// Connect to database using provided parameters
//

DB_HANDLE LIBNXDB_EXPORTABLE DBConnectEx(const TCHAR *pszServer, const TCHAR *pszDBName,
                                          const TCHAR *pszLogin, const TCHAR *pszPassword)
{
   DB_CONNECTION hDrvConn;
   DB_HANDLE hConn = NULL;

   hDrvConn = m_fpDrvConnect(pszServer, pszLogin, pszPassword, pszDBName);
   if (hDrvConn != NULL)
   {
      hConn = (DB_HANDLE)malloc(sizeof(struct db_handle_t));
      if (hConn != NULL)
      {
         hConn->hConn = hDrvConn;
         hConn->mutexTransLock = MutexCreateRecursive();
         hConn->nTransactionLevel = 0;
         hConn->pszDBName = (pszDBName == NULL) ? NULL : _tcsdup(pszDBName);
         hConn->pszLogin = (pszLogin == NULL) ? NULL : _tcsdup(pszLogin);
         hConn->pszPassword = (pszPassword == NULL) ? NULL : _tcsdup(pszPassword);
         hConn->pszServer = (pszServer == NULL) ? NULL : _tcsdup(pszServer);
		   __DbgPrintf(4, _T("New DB connection opened: handle=%p"), hConn);
      }
      else
      {
         m_fpDrvDisconnect(hDrvConn);
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

   __DbgPrintf(4, _T("DB connection %p closed"), hConn);
   
   m_fpDrvDisconnect(hConn->hConn);
   MutexDestroy(hConn->mutexTransLock);
   safe_free(hConn->pszDBName);
   safe_free(hConn->pszLogin);
   safe_free(hConn->pszPassword);
   safe_free(hConn->pszServer);
   free(hConn);
}


//
// Reconnect to database
//

static void DBReconnect(DB_HANDLE hConn)
{
   int nCount;

   m_fpDrvDisconnect(hConn->hConn);
   for(nCount = 0; ; nCount++)
   {
      hConn->hConn = m_fpDrvConnect(hConn->pszServer, hConn->pszLogin,
                                    hConn->pszPassword, hConn->pszDBName);
      if (hConn->hConn != NULL)
         break;
      if (nCount == 0)
      {
         MutexLock(m_mutexReconnect, INFINITE);
         if ((m_nReconnect == 0) && (m_fpEventHandler != NULL))
            m_fpEventHandler(DBEVENT_CONNECTION_LOST, NULL, NULL);
         m_nReconnect++;
         MutexUnlock(m_mutexReconnect);
      }
      ThreadSleepMs(1000);
   }
   if (nCount > 0)
   {
      MutexLock(m_mutexReconnect, INFINITE);
      m_nReconnect--;
      if ((m_nReconnect == 0) && (m_fpEventHandler != NULL))
         m_fpEventHandler(DBEVENT_CONNECTION_RESTORED, NULL, NULL);
      MutexUnlock(m_mutexReconnect);
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

   MutexLock(hConn->mutexTransLock, INFINITE);
   if (m_bDumpSQL)
      ms = GetCurrentTimeMs();

   dwResult = m_fpDrvQuery(hConn->hConn, pwszQuery, errorText);
   if (dwResult == DBERR_CONNECTION_LOST)
   {
      DBReconnect(hConn);
      dwResult = m_fpDrvQuery(hConn->hConn, pwszQuery, errorText);
   }

#ifndef UNICODE
   free(pwszQuery);
#endif
   
   if (m_bDumpSQL)
   {
      ms = GetCurrentTimeMs() - ms;
      __DbgPrintf(9, _T("%s sync query: \"%s\" [%d ms]"), (dwResult == DBERR_SUCCESS) ? _T("Successful") : _T("Failed"), szQuery, ms);
   }
   
   MutexUnlock(hConn->mutexTransLock);
   if (dwResult != DBERR_SUCCESS)
	{	
		if (m_bLogSQLErrors)
			nxlog_write(m_sqlErrorMsgCode, EVENTLOG_ERROR_TYPE, "ss", szQuery, errorText);
		if (m_fpEventHandler != NULL)
			m_fpEventHandler(DBEVENT_QUERY_FAILED, szQuery, errorText);
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
   DB_RESULT hResult;
   DWORD dwError;
   INT64 ms;
#ifdef UNICODE
#define pwszQuery szQuery
#else
   WCHAR *pwszQuery = WideStringFromMBString(szQuery);
#endif
   
   MutexLock(hConn->mutexTransLock, INFINITE);
   if (m_bDumpSQL)
      ms = GetCurrentTimeMs();
   hResult = m_fpDrvSelect(hConn->hConn, pwszQuery, &dwError, errorText);
   if ((hResult == NULL) && (dwError == DBERR_CONNECTION_LOST))
   {
      DBReconnect(hConn);
      hResult = m_fpDrvSelect(hConn->hConn, pwszQuery, &dwError, errorText);
   }

#ifndef UNICODE
   free(pwszQuery);
#endif
   
   if (m_bDumpSQL)
   {
      ms = GetCurrentTimeMs() - ms;
      __DbgPrintf(9, _T("%s sync query: \"%s\" [%d ms]"), (hResult != NULL) ? _T("Successful") : _T("Failed"), szQuery, (DWORD)ms);
   }
   MutexUnlock(hConn->mutexTransLock);
   if (hResult == NULL)
	{
		if (m_bLogSQLErrors)
			nxlog_write(m_sqlErrorMsgCode, EVENTLOG_ERROR_TYPE, "ss", szQuery, errorText);
		if (m_fpEventHandler != NULL)
			m_fpEventHandler(DBEVENT_QUERY_FAILED, szQuery, errorText);
	}
   return hResult;
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
	return m_fpDrvGetColumnCount(hResult);
}


//
// Get column name
//

BOOL LIBNXDB_EXPORTABLE DBGetColumnName(DB_RESULT hResult, int column, TCHAR *buffer, int bufSize)
{
	const char *name;

	name = m_fpDrvGetColumnName(hResult, column);
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
	return m_fpDrvGetColumnCountAsync(hResult);
}


//
// Get column name for async request
//

BOOL LIBNXDB_EXPORTABLE DBGetColumnNameAsync(DB_ASYNC_RESULT hResult, int column, TCHAR *buffer, int bufSize)
{
	const char *name;

	name = m_fpDrvGetColumnNameAsync(hResult, column);
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
      return m_fpDrvGetField(hResult, iRow, iColumn, pszBuffer, nBufLen);
   }
   else
   {
      LONG nLen;
      WCHAR *pszTemp;

      nLen = m_fpDrvGetFieldLength(hResult, iRow, iColumn);
      if (nLen == -1)
      {
         pszTemp = NULL;
      }
      else
      {
         nLen++;
         pszBuffer = (WCHAR *)malloc(nLen * sizeof(WCHAR));
         m_fpDrvGetField(hResult, iRow, iColumn, pszTemp, nLen);
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
      pwszData = m_fpDrvGetField(hResult, iRow, iColumn, pwszBuffer, nBufLen);
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
      nLen = m_fpDrvGetFieldLength(hResult, iRow, iColumn);
      if (nLen == -1)
      {
         pszRet = NULL;
      }
      else
      {
         nLen++;
         pwszBuffer = (WCHAR *)malloc(nLen * sizeof(WCHAR));
         pwszData = m_fpDrvGetField(hResult, iRow, iColumn, pwszBuffer, nLen);
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
   return m_fpDrvGetNumRows(hResult);
}


//
// Free result
//

void LIBNXDB_EXPORTABLE DBFreeResult(DB_RESULT hResult)
{
   if (hResult != NULL)
      m_fpDrvFreeResult(hResult);
}


//
// Asyncronous SELECT query
//

DB_ASYNC_RESULT LIBNXDB_EXPORTABLE DBAsyncSelectEx(DB_HANDLE hConn, const TCHAR *szQuery, TCHAR *errorText)
{
   DB_RESULT hResult;
   DWORD dwError;
   INT64 ms;
#ifdef UNICODE
#define pwszQuery szQuery
#else
   WCHAR *pwszQuery = WideStringFromMBString(szQuery);
#endif
   
   MutexLock(hConn->mutexTransLock, INFINITE);
   if (m_bDumpSQL)
      ms = GetCurrentTimeMs();
   hResult = m_fpDrvAsyncSelect(hConn->hConn, pwszQuery, &dwError, errorText);
   if ((hResult == NULL) && (dwError == DBERR_CONNECTION_LOST))
   {
      DBReconnect(hConn);
      hResult = m_fpDrvAsyncSelect(hConn->hConn, pwszQuery, &dwError, errorText);
   }

#ifndef UNICODE
   free(pwszQuery);
#endif
   
   if (m_bDumpSQL)
   {
      ms = GetCurrentTimeMs() - ms;
      __DbgPrintf(9, _T("%s async query: \"%s\" [%d ms]"), (hResult != NULL) ? _T("Successful") : _T("Failed"), szQuery, (DWORD)ms);
   }
   if (hResult == NULL)
   {
      MutexUnlock(hConn->mutexTransLock);
      if (m_bLogSQLErrors)
        nxlog_write(m_sqlErrorMsgCode, EVENTLOG_ERROR_TYPE, "ss", szQuery, errorText);
		if (m_fpEventHandler != NULL)
			m_fpEventHandler(DBEVENT_QUERY_FAILED, szQuery, errorText);
   }
   return hResult;
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
   return m_fpDrvFetch(hResult);
}


//
// Get field's value from asynchronous SELECT result
//

TCHAR LIBNXDB_EXPORTABLE *DBGetFieldAsync(DB_ASYNC_RESULT hResult, int iColumn, TCHAR *pBuffer, int iBufSize)
{
#ifdef UNICODE
   if (pBuffer != NULL)
   {
	   return m_fpDrvGetFieldAsync(hResult, iColumn, pBuffer, iBufSize);
   }
   else
   {
      LONG nLen;
      WCHAR *pszTemp;

      nLen = m_fpDrvGetFieldLengthAsync(hResult, iColumn);
      if (nLen == -1)
      {
         pszTemp = NULL;
      }
      else
      {
         nLen++;
         pszBuffer = (WCHAR *)malloc(nLen * sizeof(WCHAR));
         m_fpDrvGetFieldAsync(hResult, iColumn, pszTemp, nLen);
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
		if (m_fpDrvGetFieldAsync(hResult, iColumn, pwszBuffer, iBufSize) != NULL)
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
      nLen = m_fpDrvGetFieldLengthAsync(hResult, iColumn);
      if (nLen == -1)
      {
         pszRet = NULL;
      }
      else
      {
         nLen++;
         pwszBuffer = (WCHAR *)malloc(nLen * sizeof(WCHAR));
         pwszData = m_fpDrvGetFieldAsync(hResult, iColumn, pwszBuffer, nLen);
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

LONG LIBNXDB_EXPORTABLE DBGetFieldAsyncLong(DB_RESULT hResult, int iColumn)
{
   TCHAR szBuffer[64];
   
   return DBGetFieldAsync(hResult, iColumn, szBuffer, 64) == NULL ? 0 : _tcstol(szBuffer, NULL, 10);
}


//
// Get field's value as signed 64-bit int from asynchronous SELECT result
//

INT64 LIBNXDB_EXPORTABLE DBGetFieldAsyncInt64(DB_RESULT hResult, int iColumn)
{
   TCHAR szBuffer[64];
   
   return DBGetFieldAsync(hResult, iColumn, szBuffer, 64) == NULL ? 0 : _tcstoll(szBuffer, NULL, 10);
}


//
// Get field's value as signed long from asynchronous SELECT result
//

double LIBNXDB_EXPORTABLE DBGetFieldAsyncDouble(DB_RESULT hResult, int iColumn)
{
   TCHAR szBuffer[64];
   
   return DBGetFieldAsync(hResult, iColumn, szBuffer, 64) == NULL ? 0 : _tcstod(szBuffer, NULL);
}


//
// Get field's value as IP address from asynchronous SELECT result
//

DWORD LIBNXDB_EXPORTABLE DBGetFieldAsyncIPAddr(DB_RESULT hResult, int iColumn)
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
   m_fpDrvFreeAsyncResult(hResult);
   MutexUnlock(hConn->mutexTransLock);
}


//
// Begin transaction
//

BOOL LIBNXDB_EXPORTABLE DBBegin(DB_HANDLE hConn)
{
   DWORD dwResult;
   BOOL bRet = FALSE;

   MutexLock(hConn->mutexTransLock, INFINITE);
   if (hConn->nTransactionLevel == 0)
   {
      dwResult = m_fpDrvBegin(hConn->hConn);
      if (dwResult == DBERR_CONNECTION_LOST)
      {
         DBReconnect(hConn);
         dwResult = m_fpDrvBegin(hConn->hConn);
      }
      if (dwResult == DBERR_SUCCESS)
      {
         hConn->nTransactionLevel++;
         bRet = TRUE;
			__DbgPrintf(9, _T("BEGIN TRANSACTION successful (level %d)"), hConn->nTransactionLevel);
      }
      else
      {
         MutexUnlock(hConn->mutexTransLock);
			__DbgPrintf(9, _T("BEGIN TRANSACTION failed"), hConn->nTransactionLevel);
      }
   }
   else
   {
      hConn->nTransactionLevel++;
      bRet = TRUE;
		__DbgPrintf(9, _T("BEGIN TRANSACTION successful (level %d)"), hConn->nTransactionLevel);
   }
   return bRet;
}


//
// Commit transaction
//

BOOL LIBNXDB_EXPORTABLE DBCommit(DB_HANDLE hConn)
{
   BOOL bRet = FALSE;

   MutexLock(hConn->mutexTransLock, INFINITE);
   if (hConn->nTransactionLevel > 0)
   {
      hConn->nTransactionLevel--;
      if (hConn->nTransactionLevel == 0)
         bRet = (m_fpDrvCommit(hConn->hConn) == DBERR_SUCCESS);
      else
         bRet = TRUE;
		__DbgPrintf(9, _T("COMMIT TRANSACTION %s (level %d)"), bRet ? _T("successful") : _T("failed"), hConn->nTransactionLevel);
      MutexUnlock(hConn->mutexTransLock);
   }
   MutexUnlock(hConn->mutexTransLock);
   return bRet;
}


//
// Begin transaction
//

BOOL LIBNXDB_EXPORTABLE DBRollback(DB_HANDLE hConn)
{
   BOOL bRet = FALSE;

   MutexLock(hConn->mutexTransLock, INFINITE);
   if (hConn->nTransactionLevel > 0)
   {
      hConn->nTransactionLevel--;
      if (hConn->nTransactionLevel == 0)
         bRet = (m_fpDrvRollback(hConn->hConn) == DBERR_SUCCESS);
      else
         bRet = TRUE;
		__DbgPrintf(9, _T("ROLLBACK TRANSACTION %s (level %d)"), bRet ? _T("successful") : _T("failed"), hConn->nTransactionLevel);
      MutexUnlock(hConn->mutexTransLock);
   }
   MutexUnlock(hConn->mutexTransLock);
   return bRet;
}


//
// Prepare string for using in SQL statement
//

String LIBNXDB_EXPORTABLE DBPrepareString(const TCHAR *str, int maxSize)
{
	String out;
	if ((maxSize > 0) && (str != NULL) && (maxSize < (int)_tcslen(str)))
	{
		TCHAR *temp = (TCHAR *)malloc((maxSize + 1) * sizeof(TCHAR));
		nx_strncpy(temp, str, maxSize + 1);
		out.setBuffer(m_fpDrvPrepareString(temp));
		free(temp);
	}
	else	
	{
		out.setBuffer(m_fpDrvPrepareString(CHECK_NULL_EX(str)));
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
   else
   {
		syntax = DB_SYNTAX_UNKNOWN;
   }

	return syntax;
}
