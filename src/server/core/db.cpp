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

#include "nms_core.h"


//
// Global variables
//

char g_szDbDriver[MAX_PATH] = "";
char g_szDbDrvParams[MAX_PATH] = "";
char g_szDbServer[MAX_PATH] = "127.0.0.1";
char g_szDbLogin[MAX_DB_LOGIN] = "netxms";
char g_szDbPassword[MAX_DB_PASSWORD] = "";
char g_szDbName[MAX_DB_NAME] = "netxms_db";


//
// Static data
//

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
   if (pFunc == NULL)
      WriteLog(MSG_DLSYM_FAILED, EVENTLOG_WARNING_TYPE, "ss", pszSymbol, szErrorText);
   return pFunc;
}


//
// Load and initialize database driver
//

BOOL DBInit(void)
{
   BOOL (* fpDrvInit)(char *);
   char szErrorText[256];

   // Load driver's module
   m_hDriver = DLOpen(g_szDbDriver, szErrorText);
   if (m_hDriver == NULL)
   {
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
      WriteLog(MSG_DBDRV_NO_ENTRY_POINTS, EVENTLOG_ERROR_TYPE, "s", g_szDbDriver);
      return FALSE;
   }

   // Initialize driver
   if (!fpDrvInit(g_szDbDrvParams))
   {
      WriteLog(MSG_DBDRV_INIT_FAILED, EVENTLOG_ERROR_TYPE, "s", g_szDbDriver);
      DLClose(m_hDriver);
      m_hDriver = NULL;
      return FALSE;
   }

   // Success
   WriteLog(MSG_DBDRV_LOADED, EVENTLOG_INFORMATION_TYPE, "s", g_szDbDriver);
   return TRUE;
}


//
// Notify driver of unload
//

void DBUnloadDriver(void)
{
   m_fpDrvUnload();
   DLClose(m_hDriver);
}


//
// Connect to database
//

DB_HANDLE DBConnect(void)
{
   return m_fpDrvConnect(g_szDbServer, g_szDbLogin, g_szDbPassword, g_szDbName);
}


//
// Disconnect from database
//

void DBDisconnect(DB_HANDLE hConn)
{
   m_fpDrvDisconnect(hConn);
}


//
// Perform a non-SELECT SQL query
//

BOOL DBQuery(DB_HANDLE hConn, char *szQuery)
{
   BOOL bResult;

   bResult = m_fpDrvQuery(hConn, szQuery);
   DbgPrintf(AF_DEBUG_SQL, "%s sync query: \"%s\"", bResult ? "Successful" : "Failed", szQuery);
   if ((!bResult) && (g_dwFlags & AF_LOG_SQL_ERRORS))
      WriteLog(MSG_SQL_ERROR, EVENTLOG_ERROR_TYPE, "s", szQuery);
   return bResult;
}


//
// Perform SELECT query
//

DB_RESULT DBSelect(DB_HANDLE hConn, char *szQuery)
{
   DB_RESULT hResult;
   
   hResult = m_fpDrvSelect(hConn, szQuery);
   DbgPrintf(AF_DEBUG_SQL, "%s sync query: \"%s\"", (hResult != NULL) ? "Successful" : "Failed", szQuery);
   if ((!hResult) && (g_dwFlags & AF_LOG_SQL_ERRORS))
      WriteLog(MSG_SQL_ERROR, EVENTLOG_ERROR_TYPE, "s", szQuery);
   return hResult;
}


//
// Get field's value
//

char *DBGetField(DB_RESULT hResult, int iRow, int iColumn)
{
   return m_fpDrvGetField(hResult, iRow, iColumn);
}


//
// Get field's value as unsigned long
//

DWORD DBGetFieldULong(DB_RESULT hResult, int iRow, int iColumn)
{
   long iVal;
   DWORD dwVal;
   char *szVal;

   szVal = DBGetField(hResult, iRow, iColumn);
   if (szVal == NULL)
      return 0;
   iVal = strtol(szVal, NULL, 10);
   memcpy(&dwVal, &iVal, sizeof(long));   // To prevent possible conversion
   return dwVal;
}


//
// Get field's value as unsigned 64-bit int
//

QWORD DBGetFieldUQuad(DB_RESULT hResult, int iRow, int iColumn)
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

long DBGetFieldLong(DB_RESULT hResult, int iRow, int iColumn)
{
   char *szVal;

   szVal = DBGetField(hResult, iRow, iColumn);
   return szVal == NULL ? 0 : strtol(szVal, NULL, 10);
}


//
// Get field's value as signed 64-bit int
//

INT64 DBGetFieldQuad(DB_RESULT hResult, int iRow, int iColumn)
{
   char *szVal;

   szVal = DBGetField(hResult, iRow, iColumn);
   return szVal == NULL ? 0 : strtoll(szVal, NULL, 10);
}


//
// Get field's value as double
//

double DBGetFieldDouble(DB_RESULT hResult, int iRow, int iColumn)
{
   char *szVal;

   szVal = DBGetField(hResult, iRow, iColumn);
   return szVal == NULL ? 0 : strtod(szVal, NULL);
}


//
// Get number of rows in result
//

int DBGetNumRows(DB_RESULT hResult)
{
   return m_fpDrvGetNumRows(hResult);
}


//
// Free result
//

void DBFreeResult(DB_RESULT hResult)
{
   m_fpDrvFreeResult(hResult);
}


//
// Asyncronous SELECT query
//

DB_ASYNC_RESULT DBAsyncSelect(DB_HANDLE hConn, char *szQuery)
{
   DB_RESULT hResult;
   
   hResult = m_fpDrvAsyncSelect(hConn, szQuery);
   DbgPrintf(AF_DEBUG_SQL, "%s async query: \"%s\"", (hResult != NULL) ? "Successful" : "Failed", szQuery);
   if ((!hResult) && (g_dwFlags & AF_LOG_SQL_ERRORS))
      WriteLog(MSG_SQL_ERROR, EVENTLOG_ERROR_TYPE, "s", szQuery);
   return hResult;
}


//
// Fetch next row from asynchronous SELECT result
//

BOOL DBFetch(DB_ASYNC_RESULT hResult)
{
   return m_fpDrvFetch(hResult);
}


//
// Get field's value from asynchronous SELECT result
//

char *DBGetFieldAsync(DB_ASYNC_RESULT hResult, int iColumn, char *pBuffer, int iBufSize)
{
   return m_fpDrvGetFieldAsync(hResult, iColumn, pBuffer, iBufSize);
}


//
// Get field's value as unsigned long from asynchronous SELECT result
//

DWORD DBGetFieldAsyncULong(DB_ASYNC_RESULT hResult, int iColumn)
{
   long iVal;
   DWORD dwVal;
   char szBuffer[64];

   if (DBGetFieldAsync(hResult, iColumn, szBuffer, 64) == NULL)
      return 0;
   iVal = strtol(szBuffer, NULL, 10);
   memcpy(&dwVal, &iVal, sizeof(long));   // To prevent possible conversion
   return dwVal;
}


//
// Get field's value as unsigned 64-bit int from asynchronous SELECT result
//

QWORD DBGetFieldAsyncUQuad(DB_ASYNC_RESULT hResult, int iColumn)
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

long DBGetFieldAsyncLong(DB_RESULT hResult, int iColumn)
{
   char szBuffer[64];
   
   return DBGetFieldAsync(hResult, iColumn, szBuffer, 64) == NULL ? 0 : strtol(szBuffer, NULL, 10);
}


//
// Get field's value as signed 64-bit int from asynchronous SELECT result
//

INT64 DBGetFieldAsyncQuad(DB_RESULT hResult, int iColumn)
{
   char szBuffer[64];
   
   return DBGetFieldAsync(hResult, iColumn, szBuffer, 64) == NULL ? 0 : strtoll(szBuffer, NULL, 10);
}


//
// Get field's value as signed long from asynchronous SELECT result
//

double DBGetFieldAsyncDouble(DB_RESULT hResult, int iColumn)
{
   char szBuffer[64];
   
   return DBGetFieldAsync(hResult, iColumn, szBuffer, 64) == NULL ? 0 : strtod(szBuffer, NULL);
}


//
// Free asynchronous SELECT result
//

void DBFreeAsyncResult(DB_ASYNC_RESULT hResult)
{
   m_fpDrvFreeAsyncResult(hResult);
}
