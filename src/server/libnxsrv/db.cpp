/* 
** Project X - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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
** $module: db.cpp
**
**/

#include "libnxsrv.h"


//
// Global variables
//

char LIBNXSRV_EXPORTABLE g_szDbDriver[MAX_PATH] = "";
char LIBNXSRV_EXPORTABLE g_szDbDrvParams[MAX_PATH] = "";
char LIBNXSRV_EXPORTABLE g_szDbServer[MAX_PATH] = "127.0.0.1";
char LIBNXSRV_EXPORTABLE g_szDbLogin[MAX_DB_LOGIN] = "netxms";
char LIBNXSRV_EXPORTABLE g_szDbPassword[MAX_DB_PASSWORD] = "";
char LIBNXSRV_EXPORTABLE g_szDbName[MAX_DB_NAME] = "netxms_db";


//
// Static data
//

static BOOL m_bWriteLog = FALSE;
static BOOL m_bLogSQLErrors = FALSE;
static BOOL m_bDumpSQL = FALSE;
static HMODULE m_hDriver = NULL;
static DB_HANDLE (* m_fpDrvConnect)(char *, char *, char *, char *) = NULL;
static void (* m_fpDrvDisconnect)(DB_HANDLE) = NULL;
static BOOL (* m_fpDrvQuery)(DB_HANDLE, char *) = NULL;
static DB_RESULT (* m_fpDrvSelect)(DB_HANDLE, char *) = NULL;
static DB_ASYNC_RESULT (* m_fpDrvAsyncSelect)(DB_HANDLE, char *) = NULL;
static BOOL (* m_fpDrvFetch)(DB_ASYNC_RESULT) = NULL;
static char* (* m_fpDrvGetField)(DB_RESULT, int, int) = NULL;
static char* (* m_fpDrvGetFieldAsync)(DB_ASYNC_RESULT, int, char *, int) = NULL;
static int (* m_fpDrvGetNumRows)(DB_RESULT) = NULL;
static void (* m_fpDrvFreeResult)(DB_RESULT) = NULL;
static void (* m_fpDrvFreeAsyncResult)(DB_ASYNC_RESULT) = NULL;
static void (* m_fpDrvUnload)(void) = NULL;


//
// Get symbol address and log errors
//

static void *DLGetSymbolAddrEx(HMODULE hModule, char *pszSymbol)
{
   void *pFunc;
   char szErrorText[256];

   pFunc = DLGetSymbolAddr(hModule, pszSymbol, szErrorText);
   if ((pFunc == NULL) && m_bWriteLog)
      WriteLog(MSG_DLSYM_FAILED, EVENTLOG_WARNING_TYPE, "ss", pszSymbol, szErrorText);
   return pFunc;
}


//
// Load and initialize database driver
//

BOOL LIBNXSRV_EXPORTABLE DBInit(BOOL bWriteLog, BOOL bLogErrors, BOOL bDumpSQL)
{
   BOOL (* fpDrvInit)(char *);
   char szErrorText[256];

   m_bWriteLog = bWriteLog;
   m_bLogSQLErrors = bLogErrors && bWriteLog;
   m_bDumpSQL = bDumpSQL;

   // Load driver's module
   m_hDriver = DLOpen(g_szDbDriver, szErrorText);
   if (m_hDriver == NULL)
   {
      if (m_bWriteLog)
         WriteLog(MSG_DLOPEN_FAILED, EVENTLOG_ERROR_TYPE, "ss", g_szDbDriver, szErrorText);
      return FALSE;
   }

   // Import symbols
   fpDrvInit = (BOOL (*)(char *))DLGetSymbolAddrEx(m_hDriver, "DrvInit");
   m_fpDrvConnect = (DB_HANDLE (*)(char *, char *, char *, char *))DLGetSymbolAddrEx(m_hDriver, "DrvConnect");
   m_fpDrvDisconnect = (void (*)(DB_HANDLE))DLGetSymbolAddrEx(m_hDriver, "DrvDisconnect");
   m_fpDrvQuery = (BOOL (*)(DB_HANDLE, char *))DLGetSymbolAddrEx(m_hDriver, "DrvQuery");
   m_fpDrvSelect = (DB_RESULT (*)(DB_HANDLE, char *))DLGetSymbolAddrEx(m_hDriver, "DrvSelect");
   m_fpDrvAsyncSelect = (DB_ASYNC_RESULT (*)(DB_HANDLE, char *))DLGetSymbolAddrEx(m_hDriver, "DrvAsyncSelect");
   m_fpDrvFetch = (BOOL (*)(DB_ASYNC_RESULT))DLGetSymbolAddrEx(m_hDriver, "DrvFetch");
   m_fpDrvGetField = (char* (*)(DB_RESULT, int, int))DLGetSymbolAddrEx(m_hDriver, "DrvGetField");
   m_fpDrvGetFieldAsync = (char* (*)(DB_ASYNC_RESULT, int, char *, int))DLGetSymbolAddrEx(m_hDriver, "DrvGetFieldAsync");
   m_fpDrvGetNumRows = (int (*)(DB_RESULT))DLGetSymbolAddrEx(m_hDriver, "DrvGetNumRows");
   m_fpDrvFreeResult = (void (*)(DB_RESULT))DLGetSymbolAddrEx(m_hDriver, "DrvFreeResult");
   m_fpDrvFreeAsyncResult = (void (*)(DB_ASYNC_RESULT))DLGetSymbolAddrEx(m_hDriver, "DrvFreeAsyncResult");
   m_fpDrvUnload = (void (*)(void))DLGetSymbolAddrEx(m_hDriver, "DrvUnload");
   if ((fpDrvInit == NULL) || (m_fpDrvConnect == NULL) || (m_fpDrvDisconnect == NULL) ||
       (m_fpDrvQuery == NULL) || (m_fpDrvSelect == NULL) || (m_fpDrvGetField == NULL) ||
       (m_fpDrvGetNumRows == NULL) || (m_fpDrvFreeResult == NULL) || 
       (m_fpDrvUnload == NULL) || (m_fpDrvAsyncSelect == NULL) || (m_fpDrvFetch == NULL) ||
       (m_fpDrvFreeAsyncResult == NULL) || (m_fpDrvGetFieldAsync == NULL))
   {
      if (m_bWriteLog)
         WriteLog(MSG_DBDRV_NO_ENTRY_POINTS, EVENTLOG_ERROR_TYPE, "s", g_szDbDriver);
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
}


//
// Connect to database
//

DB_HANDLE LIBNXSRV_EXPORTABLE DBConnect(void)
{
   return m_fpDrvConnect(g_szDbServer, g_szDbLogin, g_szDbPassword, g_szDbName);
}


//
// Connect to database using provided parameters
//

DB_HANDLE LIBNXSRV_EXPORTABLE DBConnectEx(TCHAR *pszServer, TCHAR *pszDBName,
                                          TCHAR *pszLogin, TCHAR *pszPassword)
{
   return m_fpDrvConnect(pszServer, pszLogin, pszPassword, pszDBName);
}


//
// Disconnect from database
//

void LIBNXSRV_EXPORTABLE DBDisconnect(DB_HANDLE hConn)
{
   m_fpDrvDisconnect(hConn);
}


//
// Perform a non-SELECT SQL query
//

BOOL LIBNXSRV_EXPORTABLE DBQuery(DB_HANDLE hConn, char *szQuery)
{
   BOOL bResult;
   INT64 ms;
   
   if (m_bDumpSQL)
      ms = GetCurrentTimeMs();
   bResult = m_fpDrvQuery(hConn, szQuery);
   if (m_bDumpSQL)
   {
      ms = GetCurrentTimeMs() - ms;
      printf("%s sync query: \"%s\" [%d ms]\n", bResult ? "Successful" : "Failed", szQuery, ms);
   }
   if ((!bResult) && m_bLogSQLErrors)
      WriteLog(MSG_SQL_ERROR, EVENTLOG_ERROR_TYPE, "s", szQuery);
   return bResult;
}


//
// Perform SELECT query
//

DB_RESULT LIBNXSRV_EXPORTABLE DBSelect(DB_HANDLE hConn, char *szQuery)
{
   DB_RESULT hResult;
   INT64 ms;
   
   if (m_bDumpSQL)
      ms = GetCurrentTimeMs();
   hResult = m_fpDrvSelect(hConn, szQuery);
   if (m_bDumpSQL)
   {
      ms = GetCurrentTimeMs() - ms;
      printf("%s sync query: \"%s\" [%d ms]\n", (hResult != NULL) ? "Successful" : "Failed", szQuery, (DWORD)ms);
   }
   if ((!hResult) && m_bLogSQLErrors)
      WriteLog(MSG_SQL_ERROR, EVENTLOG_ERROR_TYPE, "s", szQuery);
   return hResult;
}


//
// Get field's value
//

char LIBNXSRV_EXPORTABLE *DBGetField(DB_RESULT hResult, int iRow, int iColumn)
{
   return m_fpDrvGetField(hResult, iRow, iColumn);
}


//
// Get field's value as unsigned long
//

DWORD LIBNXSRV_EXPORTABLE DBGetFieldULong(DB_RESULT hResult, int iRow, int iColumn)
{
   LONG iVal;
   DWORD dwVal;
   char *szVal;

   szVal = DBGetField(hResult, iRow, iColumn);
   if (szVal == NULL)
      return 0;
   iVal = strtol(szVal, NULL, 10);
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
   char *szVal;

   szVal = DBGetField(hResult, iRow, iColumn);
   if (szVal == NULL)
      return 0;
   iVal = strtoll(szVal, NULL, 10);
   memcpy(&qwVal, &iVal, sizeof(INT64));   // To prevent possible conversion
   return qwVal;
}


//
// Get field's value as signed long
//

LONG LIBNXSRV_EXPORTABLE DBGetFieldLong(DB_RESULT hResult, int iRow, int iColumn)
{
   char *szVal;

   szVal = DBGetField(hResult, iRow, iColumn);
   return szVal == NULL ? 0 : strtol(szVal, NULL, 10);
}


//
// Get field's value as signed 64-bit int
//

INT64 LIBNXSRV_EXPORTABLE DBGetFieldInt64(DB_RESULT hResult, int iRow, int iColumn)
{
   char *szVal;

   szVal = DBGetField(hResult, iRow, iColumn);
   return szVal == NULL ? 0 : strtoll(szVal, NULL, 10);
}


//
// Get field's value as double
//

double LIBNXSRV_EXPORTABLE DBGetFieldDouble(DB_RESULT hResult, int iRow, int iColumn)
{
   char *szVal;

   szVal = DBGetField(hResult, iRow, iColumn);
   return szVal == NULL ? 0 : strtod(szVal, NULL);
}


//
// Get field's value as IP address
//

DWORD LIBNXSRV_EXPORTABLE DBGetFieldIPAddr(DB_RESULT hResult, int iRow, int iColumn)
{
   char *szVal;

   szVal = DBGetField(hResult, iRow, iColumn);
   return szVal == NULL ? INADDR_NONE : ntohl(inet_addr(szVal));
}


//
// Get field's value as integer array from byte array encoded in hex
//

BOOL LIBNXSRV_EXPORTABLE DBGetFieldByteArray(DB_RESULT hResult, int iRow, int iColumn,
                                             int *pnArray, int nSize, int nDefault)
{
   char *pszVal, pbBytes[128];
   BOOL bResult;
   int i, nLen;

   pszVal = DBGetField(hResult, iRow, iColumn);
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

DB_ASYNC_RESULT LIBNXSRV_EXPORTABLE DBAsyncSelect(DB_HANDLE hConn, char *szQuery)
{
   DB_RESULT hResult;
   INT64 ms;
   
   if (m_bDumpSQL)
      ms = GetCurrentTimeMs();
   hResult = m_fpDrvAsyncSelect(hConn, szQuery);
   if (m_bDumpSQL)
   {
      ms = GetCurrentTimeMs() - ms;
      printf("%s async query: \"%s\" [%d ms]\n", (hResult != NULL) ? "Successful" : "Failed", szQuery, (DWORD)ms);
   }
   if ((!hResult) && m_bLogSQLErrors)
      WriteLog(MSG_SQL_ERROR, EVENTLOG_ERROR_TYPE, "s", szQuery);
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

char LIBNXSRV_EXPORTABLE *DBGetFieldAsync(DB_ASYNC_RESULT hResult, int iColumn, char *pBuffer, int iBufSize)
{
   return m_fpDrvGetFieldAsync(hResult, iColumn, pBuffer, iBufSize);
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

void LIBNXSRV_EXPORTABLE DBFreeAsyncResult(DB_ASYNC_RESULT hResult)
{
   m_fpDrvFreeAsyncResult(hResult);
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
