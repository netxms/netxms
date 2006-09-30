/* $Id: db.cpp,v 1.19 2006-09-30 22:41:10 victor Exp $ */
/*
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006 Victor Kirhenshtein
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

#include "libnxsrv.h"


//
// Global variables
//

TCHAR LIBNXSRV_EXPORTABLE g_szDbDriver[MAX_PATH] = _T("");
TCHAR LIBNXSRV_EXPORTABLE g_szDbDrvParams[MAX_PATH] = _T("");
TCHAR LIBNXSRV_EXPORTABLE g_szDbServer[MAX_PATH] = _T("127.0.0.1");
TCHAR LIBNXSRV_EXPORTABLE g_szDbLogin[MAX_DB_LOGIN] = _T("netxms");
TCHAR LIBNXSRV_EXPORTABLE g_szDbPassword[MAX_DB_PASSWORD] = _T("");
TCHAR LIBNXSRV_EXPORTABLE g_szDbName[MAX_DB_NAME] = _T("netxms_db");


//
// Static data
//

static BOOL m_bWriteLog = FALSE;
static BOOL m_bLogSQLErrors = FALSE;
static BOOL m_bDumpSQL = FALSE;
static int m_nReconnect = 0;
static MUTEX m_mutexReconnect = INVALID_MUTEX_HANDLE;
static HMODULE m_hDriver = NULL;
static DB_CONNECTION (* m_fpDrvConnect)(char *, char *, char *, char *) = NULL;
static void (* m_fpDrvDisconnect)(DB_CONNECTION) = NULL;
static DWORD (* m_fpDrvQuery)(DB_CONNECTION, WCHAR *) = NULL;
static DB_RESULT (* m_fpDrvSelect)(DB_CONNECTION, WCHAR *, DWORD *) = NULL;
static DB_ASYNC_RESULT (* m_fpDrvAsyncSelect)(DB_CONNECTION, WCHAR *, DWORD *) = NULL;
static BOOL (* m_fpDrvFetch)(DB_ASYNC_RESULT) = NULL;
static WCHAR* (* m_fpDrvGetField)(DB_RESULT, int, int, WCHAR *, int) = NULL;
static WCHAR* (* m_fpDrvGetFieldAsync)(DB_ASYNC_RESULT, int, WCHAR *, int) = NULL;
static int (* m_fpDrvGetNumRows)(DB_RESULT) = NULL;
static void (* m_fpDrvFreeResult)(DB_RESULT) = NULL;
static void (* m_fpDrvFreeAsyncResult)(DB_ASYNC_RESULT) = NULL;
static DWORD (* m_fpDrvBegin)(DB_CONNECTION) = NULL;
static DWORD (* m_fpDrvCommit)(DB_CONNECTION) = NULL;
static DWORD (* m_fpDrvRollback)(DB_CONNECTION) = NULL;
static void (* m_fpDrvUnload)(void) = NULL;
static void (* m_fpEventHandler)(DWORD, TCHAR *);


//
// Get symbol address and log errors
//

static void *DLGetSymbolAddrEx(HMODULE hModule, TCHAR *pszSymbol)
{
   void *pFunc;
   char szErrorText[256];

   pFunc = DLGetSymbolAddr(hModule, pszSymbol, szErrorText);
   if ((pFunc == NULL) && m_bWriteLog)
      WriteLog(MSG_DLSYM_FAILED, EVENTLOG_WARNING_TYPE, _T("ss"), pszSymbol, szErrorText);
   return pFunc;
}


//
// Load and initialize database driver
//

BOOL LIBNXSRV_EXPORTABLE DBInit(BOOL bWriteLog, BOOL bLogErrors, BOOL bDumpSQL,
                                void (* fpEventHandler)(DWORD, TCHAR *))
{
   BOOL (* fpDrvInit)(char *);
   DWORD *pdwAPIVersion;
   char szErrorText[256];
   static DWORD dwVersionZero = 0;

   m_bWriteLog = bWriteLog;
   m_bLogSQLErrors = bLogErrors && bWriteLog;
   m_bDumpSQL = bDumpSQL;
   m_fpEventHandler = fpEventHandler;
   m_nReconnect = 0;
   m_mutexReconnect = MutexCreate();

   // Load driver's module
   m_hDriver = DLOpen(g_szDbDriver, szErrorText);
   if (m_hDriver == NULL)
   {
      if (m_bWriteLog)
         WriteLog(MSG_DLOPEN_FAILED, EVENTLOG_ERROR_TYPE, "ss", g_szDbDriver, szErrorText);
      return FALSE;
   }

   // Check API version supported by driver
   pdwAPIVersion = (DWORD *)DLGetSymbolAddr(m_hDriver, "drvAPIVersion", NULL);
   if (pdwAPIVersion == NULL)
      pdwAPIVersion = &dwVersionZero;
   if (*pdwAPIVersion != DBDRV_API_VERSION)
   {
      if (m_bWriteLog)
         WriteLog(MSG_DBDRV_API_VERSION_MISMATCH, EVENTLOG_ERROR_TYPE, "sdd",
                  g_szDbDriver, DBDRV_API_VERSION, *pdwAPIVersion);
      DLClose(m_hDriver);
      m_hDriver = NULL;
      return FALSE;
   }

   // Import symbols
   fpDrvInit = (BOOL (*)(char *))DLGetSymbolAddrEx(m_hDriver, "DrvInit");
   m_fpDrvConnect = (DB_CONNECTION (*)(char *, char *, char *, char *))DLGetSymbolAddrEx(m_hDriver, "DrvConnect");
   m_fpDrvDisconnect = (void (*)(DB_CONNECTION))DLGetSymbolAddrEx(m_hDriver, "DrvDisconnect");
   m_fpDrvQuery = (DWORD (*)(DB_CONNECTION, WCHAR *))DLGetSymbolAddrEx(m_hDriver, "DrvQuery");
   m_fpDrvSelect = (DB_RESULT (*)(DB_CONNECTION, WCHAR *, DWORD *))DLGetSymbolAddrEx(m_hDriver, "DrvSelect");
   m_fpDrvAsyncSelect = (DB_ASYNC_RESULT (*)(DB_CONNECTION, WCHAR *, DWORD *))DLGetSymbolAddrEx(m_hDriver, "DrvAsyncSelect");
   m_fpDrvFetch = (BOOL (*)(DB_ASYNC_RESULT))DLGetSymbolAddrEx(m_hDriver, "DrvFetch");
   m_fpDrvGetField = (WCHAR* (*)(DB_RESULT, int, int, WCHAR *, int))DLGetSymbolAddrEx(m_hDriver, "DrvGetField");
   m_fpDrvGetFieldAsync = (WCHAR* (*)(DB_ASYNC_RESULT, int, WCHAR *, int))DLGetSymbolAddrEx(m_hDriver, "DrvGetFieldAsync");
   m_fpDrvGetNumRows = (int (*)(DB_RESULT))DLGetSymbolAddrEx(m_hDriver, "DrvGetNumRows");
   m_fpDrvFreeResult = (void (*)(DB_RESULT))DLGetSymbolAddrEx(m_hDriver, "DrvFreeResult");
   m_fpDrvFreeAsyncResult = (void (*)(DB_ASYNC_RESULT))DLGetSymbolAddrEx(m_hDriver, "DrvFreeAsyncResult");
   m_fpDrvBegin = (DWORD (*)(DB_CONNECTION))DLGetSymbolAddrEx(m_hDriver, "DrvBegin");
   m_fpDrvCommit = (DWORD (*)(DB_CONNECTION))DLGetSymbolAddrEx(m_hDriver, "DrvCommit");
   m_fpDrvRollback = (DWORD (*)(DB_CONNECTION))DLGetSymbolAddrEx(m_hDriver, "DrvRollback");
   m_fpDrvUnload = (void (*)(void))DLGetSymbolAddrEx(m_hDriver, "DrvUnload");
   if ((fpDrvInit == NULL) || (m_fpDrvConnect == NULL) || (m_fpDrvDisconnect == NULL) ||
       (m_fpDrvQuery == NULL) || (m_fpDrvSelect == NULL) || (m_fpDrvGetField == NULL) ||
       (m_fpDrvGetNumRows == NULL) || (m_fpDrvFreeResult == NULL) || 
       (m_fpDrvUnload == NULL) || (m_fpDrvAsyncSelect == NULL) || (m_fpDrvFetch == NULL) ||
       (m_fpDrvFreeAsyncResult == NULL) || (m_fpDrvGetFieldAsync == NULL) ||
       (m_fpDrvBegin == NULL) || (m_fpDrvCommit == NULL) || (m_fpDrvRollback == NULL))
   {
      if (m_bWriteLog)
         WriteLog(MSG_DBDRV_NO_ENTRY_POINTS, EVENTLOG_ERROR_TYPE, "s", g_szDbDriver);
      DLClose(m_hDriver);
      m_hDriver = NULL;
      return FALSE;
   }

   // Initialize driver
   if (!fpDrvInit(g_szDbDrvParams))
   {
      if (m_bWriteLog)
         WriteLog(MSG_DBDRV_INIT_FAILED, EVENTLOG_ERROR_TYPE, "s", g_szDbDriver);
      DLClose(m_hDriver);
      m_hDriver = NULL;
      return FALSE;
   }

   // Success
   if (m_bWriteLog)
      WriteLog(MSG_DBDRV_LOADED, EVENTLOG_INFORMATION_TYPE, "s", g_szDbDriver);
   return TRUE;
}


//
// Notify driver of unload
//

void LIBNXSRV_EXPORTABLE DBUnloadDriver(void)
{
   m_fpDrvUnload();
   DLClose(m_hDriver);
   MutexDestroy(m_mutexReconnect);
}


//
// Connect to database
//

DB_HANDLE LIBNXSRV_EXPORTABLE DBConnect(void)
{
   return DBConnectEx(g_szDbServer, g_szDbName, g_szDbLogin, g_szDbPassword);
}


//
// Connect to database using provided parameters
//

DB_HANDLE LIBNXSRV_EXPORTABLE DBConnectEx(TCHAR *pszServer, TCHAR *pszDBName,
                                          TCHAR *pszLogin, TCHAR *pszPassword)
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

void LIBNXSRV_EXPORTABLE DBDisconnect(DB_HANDLE hConn)
{
   if (hConn == NULL)
      return;

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
            m_fpEventHandler(DBEVENT_CONNECTION_LOST, NULL);
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
         m_fpEventHandler(DBEVENT_CONNECTION_RESTORED, NULL);
      MutexUnlock(m_mutexReconnect);
   }
}


//
// Perform a non-SELECT SQL query
//

BOOL LIBNXSRV_EXPORTABLE DBQuery(DB_HANDLE hConn, TCHAR *szQuery)
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

   dwResult = m_fpDrvQuery(hConn->hConn, pwszQuery);
   if (dwResult == DBERR_CONNECTION_LOST)
   {
      DBReconnect(hConn);
      dwResult = m_fpDrvQuery(hConn->hConn, pwszQuery);
   }

#ifndef UNICODE
   free(pwszQuery);
#endif
   
   if (m_bDumpSQL)
   {
      ms = GetCurrentTimeMs() - ms;
      _tprintf(_T("%s sync query: \"%s\" [%d ms]\n"), (dwResult == DBERR_SUCCESS) ? _T("Successful") : _T("Failed"), szQuery, ms);
   }
   
   MutexUnlock(hConn->mutexTransLock);
   if ((dwResult != DBERR_SUCCESS) && m_bLogSQLErrors)
      WriteLog(MSG_SQL_ERROR, EVENTLOG_ERROR_TYPE, _T("s"), szQuery);
   return dwResult == DBERR_SUCCESS;
#undef pwszQuery
}


//
// Perform SELECT query
//

DB_RESULT LIBNXSRV_EXPORTABLE DBSelect(DB_HANDLE hConn, TCHAR *szQuery)
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
   hResult = m_fpDrvSelect(hConn->hConn, pwszQuery, &dwError);
   if ((hResult == NULL) && (dwError == DBERR_CONNECTION_LOST))
   {
      DBReconnect(hConn);
      hResult = m_fpDrvSelect(hConn->hConn, pwszQuery, &dwError);
   }

#ifndef UNICODE
   free(pwszQuery);
#endif
   
   if (m_bDumpSQL)
   {
      ms = GetCurrentTimeMs() - ms;
      _tprintf(_T("%s sync query: \"%s\" [%d ms]\n"), (hResult != NULL) ? _T("Successful") : _T("Failed"), szQuery, (DWORD)ms);
   }
   MutexUnlock(hConn->mutexTransLock);
   if ((hResult == NULL) && m_bLogSQLErrors)
      WriteLog(MSG_SQL_ERROR, EVENTLOG_ERROR_TYPE, _T("s"), szQuery);
   return hResult;
}


//
// Get field's value
//

TCHAR LIBNXSRV_EXPORTABLE *DBGetField(DB_RESULT hResult, int iRow, int iColumn,
                                      TCHAR *pszBuffer, int nBufLen)
{
#ifdef UNICODE
   if (pszBuffer != NULL)
   {
      return m_fpDrvGetField(hResult, iRow, iColumn, pszBuffer, nBufLen);
   }
   else
   {
      return wcsdup(m_fpDrvGetField(hResult, iRow, iColumn, alloca(65536), 65536);
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
      pwszBuffer = (WCHAR *)malloc(65536 * sizeof(WCHAR));
      pwszData = m_fpDrvGetField(hResult, iRow, iColumn, pwszBuffer, 65536);
      if (pwszData != NULL)
      {
         nLen = wcslen(pwszData) + 1;
         pszRet = (char *)malloc(nLen);
         WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                             pwszData, -1, pszRet, nLen, NULL, NULL);
         pszRet = pszBuffer;
      }
      else
      {
         pszRet = NULL;
      }
      free(pwszBuffer);
   }
   return pszRet;
#endif
}


//
// Get field's value as unsigned long
//

DWORD LIBNXSRV_EXPORTABLE DBGetFieldULong(DB_RESULT hResult, int iRow, int iColumn)
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

QWORD LIBNXSRV_EXPORTABLE DBGetFieldUInt64(DB_RESULT hResult, int iRow, int iColumn)
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

LONG LIBNXSRV_EXPORTABLE DBGetFieldLong(DB_RESULT hResult, int iRow, int iColumn)
{
   TCHAR *pszVal, szBuffer[256];

   pszVal = DBGetField(hResult, iRow, iColumn, szBuffer, 256);
   return pszVal == NULL ? 0 : _tcstol(pszVal, NULL, 10);
}


//
// Get field's value as signed 64-bit int
//

INT64 LIBNXSRV_EXPORTABLE DBGetFieldInt64(DB_RESULT hResult, int iRow, int iColumn)
{
   TCHAR *pszVal, szBuffer[256];

   pszVal = DBGetField(hResult, iRow, iColumn, szBuffer, 256);
   return pszVal == NULL ? 0 : _tcstoll(pszVal, NULL, 10);
}


//
// Get field's value as double
//

double LIBNXSRV_EXPORTABLE DBGetFieldDouble(DB_RESULT hResult, int iRow, int iColumn)
{
   TCHAR *pszVal, szBuffer[256];

   pszVal = DBGetField(hResult, iRow, iColumn, szBuffer, 256);
   return pszVal == NULL ? 0 : _tcstod(pszVal, NULL);
}


//
// Get field's value as IP address
//

DWORD LIBNXSRV_EXPORTABLE DBGetFieldIPAddr(DB_RESULT hResult, int iRow, int iColumn)
{
   TCHAR *pszVal, szBuffer[256];

   pszVal = DBGetField(hResult, iRow, iColumn, szBuffer, 256);
   return pszVal == NULL ? INADDR_NONE : ntohl(_t_inet_addr(pszVal));
}


//
// Get field's value as integer array from byte array encoded in hex
//

BOOL LIBNXSRV_EXPORTABLE DBGetFieldByteArray(DB_RESULT hResult, int iRow, int iColumn,
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

BOOL LIBNXSRV_EXPORTABLE DBGetFieldGUID(DB_RESULT hResult, int iRow,
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

int LIBNXSRV_EXPORTABLE DBGetNumRows(DB_RESULT hResult)
{
   if (hResult == NULL)
      return 0;
   return m_fpDrvGetNumRows(hResult);
}


//
// Free result
//

void LIBNXSRV_EXPORTABLE DBFreeResult(DB_RESULT hResult)
{
   if (hResult != NULL)
      m_fpDrvFreeResult(hResult);
}


//
// Asyncronous SELECT query
//

DB_ASYNC_RESULT LIBNXSRV_EXPORTABLE DBAsyncSelect(DB_HANDLE hConn, TCHAR *szQuery)
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
   hResult = m_fpDrvAsyncSelect(hConn->hConn, pwszQuery, &dwError);
   if ((hResult == NULL) && (dwError == DBERR_CONNECTION_LOST))
   {
      DBReconnect(hConn);
      hResult = m_fpDrvAsyncSelect(hConn->hConn, pwszQuery, &dwError);
   }

#ifndef UNICODE
   free(pwszQuery);
#endif
   
   if (m_bDumpSQL)
   {
      ms = GetCurrentTimeMs() - ms;
      _tprintf(_T("%s async query: \"%s\" [%d ms]\n"), (hResult != NULL) ? _T("Successful") : _T("Failed"), szQuery, (DWORD)ms);
   }
   if (hResult == NULL)
   {
      MutexUnlock(hConn->mutexTransLock);
      if (m_bLogSQLErrors)
        WriteLog(MSG_SQL_ERROR, EVENTLOG_ERROR_TYPE, _T("s"), szQuery);
   }
   return hResult;
}


//
// Fetch next row from asynchronous SELECT result
//

BOOL LIBNXSRV_EXPORTABLE DBFetch(DB_ASYNC_RESULT hResult)
{
   return m_fpDrvFetch(hResult);
}


//
// Get field's value from asynchronous SELECT result
//

TCHAR LIBNXSRV_EXPORTABLE *DBGetFieldAsync(DB_ASYNC_RESULT hResult, int iColumn, TCHAR *pBuffer, int iBufSize)
{
#ifdef UNICODE
   return m_fpDrvGetFieldAsync(hResult, iColumn, pBuffer, iBufSize);
#else
   WCHAR *pwszBuffer;
   char *pszRet;

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
   return pszRet;
#endif
}


//
// Get field's value as unsigned long from asynchronous SELECT result
//

DWORD LIBNXSRV_EXPORTABLE DBGetFieldAsyncULong(DB_ASYNC_RESULT hResult, int iColumn)
{
   LONG iVal;
   DWORD dwVal;
   char szBuffer[64];

   if (DBGetFieldAsync(hResult, iColumn, szBuffer, 64) == NULL)
      return 0;
   iVal = strtol(szBuffer, NULL, 10);
   memcpy(&dwVal, &iVal, sizeof(LONG));   // To prevent possible conversion
   return dwVal;
}


//
// Get field's value as unsigned 64-bit int from asynchronous SELECT result
//

QWORD LIBNXSRV_EXPORTABLE DBGetFieldAsyncUInt64(DB_ASYNC_RESULT hResult, int iColumn)
{
   INT64 iVal;
   QWORD qwVal;
   char szBuffer[64];

   if (DBGetFieldAsync(hResult, iColumn, szBuffer, 64) == NULL)
      return 0;
   iVal = strtoll(szBuffer, NULL, 10);
   memcpy(&qwVal, &iVal, sizeof(INT64));   // To prevent possible conversion
   return qwVal;
}


//
// Get field's value as signed long from asynchronous SELECT result
//

LONG LIBNXSRV_EXPORTABLE DBGetFieldAsyncLong(DB_RESULT hResult, int iColumn)
{
   char szBuffer[64];
   
   return DBGetFieldAsync(hResult, iColumn, szBuffer, 64) == NULL ? 0 : strtol(szBuffer, NULL, 10);
}


//
// Get field's value as signed 64-bit int from asynchronous SELECT result
//

INT64 LIBNXSRV_EXPORTABLE DBGetFieldAsyncInt64(DB_RESULT hResult, int iColumn)
{
   char szBuffer[64];
   
   return DBGetFieldAsync(hResult, iColumn, szBuffer, 64) == NULL ? 0 : strtoll(szBuffer, NULL, 10);
}


//
// Get field's value as signed long from asynchronous SELECT result
//

double LIBNXSRV_EXPORTABLE DBGetFieldAsyncDouble(DB_RESULT hResult, int iColumn)
{
   char szBuffer[64];
   
   return DBGetFieldAsync(hResult, iColumn, szBuffer, 64) == NULL ? 0 : strtod(szBuffer, NULL);
}


//
// Get field's value as IP address from asynchronous SELECT result
//

DWORD LIBNXSRV_EXPORTABLE DBGetFieldAsyncIPAddr(DB_RESULT hResult, int iColumn)
{
   char szBuffer[64];
   
   return DBGetFieldAsync(hResult, iColumn, szBuffer, 64) == NULL ? INADDR_NONE : 
      ntohl(inet_addr(szBuffer));
}


//
// Free asynchronous SELECT result
//

void LIBNXSRV_EXPORTABLE DBFreeAsyncResult(DB_HANDLE hConn, DB_ASYNC_RESULT hResult)
{
   m_fpDrvFreeAsyncResult(hResult);
   MutexUnlock(hConn->mutexTransLock);
}


//
// Begin transaction
//

BOOL LIBNXSRV_EXPORTABLE DBBegin(DB_HANDLE hConn)
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
      }
      else
      {
         MutexUnlock(hConn->mutexTransLock);
      }
   }
   else
   {
      hConn->nTransactionLevel++;
      bRet = TRUE;
   }
   return bRet;
}


//
// Commit transaction
//

BOOL LIBNXSRV_EXPORTABLE DBCommit(DB_HANDLE hConn)
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
      MutexUnlock(hConn->mutexTransLock);
   }
   MutexUnlock(hConn->mutexTransLock);
   return bRet;
}


//
// Begin transaction
//

BOOL LIBNXSRV_EXPORTABLE DBRollback(DB_HANDLE hConn)
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
      MutexUnlock(hConn->mutexTransLock);
   }
   MutexUnlock(hConn->mutexTransLock);
   return bRet;
}


//
// Characters to be escaped before writing to SQL
//

static char m_szSpecialChars[] = _T("\x01\x02\x03\x04\x05\x06\x07\x08")
                                 _T("\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10")
                                 _T("\x11\x12\x13\x14\x15\x16\x17\x18")
                                 _T("\x19\x1A\x1B\x1C\x1D\x1E\x1F")
                                 _T("#%\"\\'\x7F");


//
// Escape some special characters in string for writing into database
//

TCHAR LIBNXSRV_EXPORTABLE *EncodeSQLString(const TCHAR *pszIn)
{
   char *pszOut;
   int iPosIn, iPosOut, iStrSize;

   if (*pszIn != 0)
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

void LIBNXSRV_EXPORTABLE DecodeSQLString(TCHAR *pszStr)
{
   int iPosIn, iPosOut;

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
