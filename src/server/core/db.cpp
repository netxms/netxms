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
char g_szDbLogin[MAX_DB_LOGIN] = "root";
char g_szDbPassword[MAX_DB_PASSWORD] = "";
char g_szDbName[MAX_DB_NAME] = "nms";


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
// Load and initialize database driver
//

BOOL DBInit(void)
{
   BOOL (* fpDrvInit)(char *);

   // Load driver's module
   m_hDriver = DLOpen(g_szDbDriver);
   if (m_hDriver == NULL)
      return FALSE;

   // Import symbols
   fpDrvInit = (BOOL (*)(char *))DLGetSymbolAddr(m_hDriver, "DrvInit");
   m_fpDrvConnect = (DB_HANDLE (*)(char *, char *, char *, char *))DLGetSymbolAddr(m_hDriver, "DrvConnect");
   m_fpDrvDisconnect = (void (*)(DB_HANDLE))DLGetSymbolAddr(m_hDriver, "DrvDisconnect");
   m_fpDrvQuery = (BOOL (*)(DB_HANDLE, char *))DLGetSymbolAddr(m_hDriver, "DrvQuery");
   m_fpDrvSelect = (DB_RESULT (*)(DB_HANDLE, char *))DLGetSymbolAddr(m_hDriver, "DrvSelect");
   m_fpDrvAsyncSelect = (DB_ASYNC_RESULT (*)(DB_HANDLE, char *))DLGetSymbolAddr(m_hDriver, "DrvAsyncSelect");
   m_fpDrvFetch = (BOOL (*)(DB_ASYNC_RESULT))DLGetSymbolAddr(m_hDriver, "DrvFetch");
   m_fpDrvGetField = (char* (*)(DB_RESULT, int, int))DLGetSymbolAddr(m_hDriver, "DrvGetField");
   m_fpDrvGetFieldAsync = (char* (*)(DB_ASYNC_RESULT, int, char *, int))DLGetSymbolAddr(m_hDriver, "DrvGetFieldAsync");
   m_fpDrvGetNumRows = (int (*)(DB_RESULT))DLGetSymbolAddr(m_hDriver, "DrvGetNumRows");
   m_fpDrvFreeResult = (void (*)(DB_RESULT))DLGetSymbolAddr(m_hDriver, "DrvFreeResult");
   m_fpDrvFreeAsyncResult = (void (*)(DB_ASYNC_RESULT))DLGetSymbolAddr(m_hDriver, "DrvFreeAsyncResult");
   m_fpDrvUnload = (void (*)(void))DLGetSymbolAddr(m_hDriver, "DrvUnload");
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
// Get field's value as signed long
//

long DBGetFieldLong(DB_RESULT hResult, int iRow, int iColumn)
{
   char *szVal;

   szVal = DBGetField(hResult, iRow, iColumn);
   return szVal == NULL ? 0 : strtol(szVal, NULL, 10);
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
// Get field's value as signed long from asynchronous SELECT result
//

long DBGetFieldAsyncLong(DB_RESULT hResult, int iColumn)
{
   char szBuffer[64];
   
   return DBGetFieldAsync(hResult, iColumn, szBuffer, 64) == NULL ? 0 : strtol(szBuffer, NULL, 10);
}


//
// Free asynchronous SELECT result
//

void DBFreeAsyncResult(DB_ASYNC_RESULT hResult)
{
   m_fpDrvFreeAsyncResult(hResult);
}
