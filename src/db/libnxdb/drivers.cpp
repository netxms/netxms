/*
** NetXMS - Network Management System
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
** File: drivers.cpp
**
**/

#include "libnxdb.h"

/**
 * Log message codes
 */
UINT32 g_logMsgCode = 0;
UINT32 g_sqlErrorMsgCode = 0;

/**
 * Long-running query threshold
 */
UINT32 g_sqlQueryExecTimeThreshold = 0xFFFFFFFF;

/**
 * Loaded drivers
 */
static DB_DRIVER s_drivers[MAX_DB_DRIVERS];
static MUTEX s_driverListLock;

/**
 * Options
 */
static bool s_writeLog = false;
static bool s_logSqlErrors = false;

/**
 * Get symbol address and log errors
 */
static void *DLGetSymbolAddrEx(HMODULE hModule, const char *pszSymbol, bool mandatory = true)
{
   void *pFunc;
   TCHAR szErrorText[256];

   pFunc = DLGetSymbolAddr(hModule, pszSymbol, szErrorText);
   if ((pFunc == NULL) && mandatory && s_writeLog)
      __DBWriteLog(EVENTLOG_WARNING_TYPE, _T("Unable to resolve symbol \"%hs\": %s"), pszSymbol, szErrorText);
   return pFunc;
}

/**
 * Init DB library
 */
bool LIBNXDB_EXPORTABLE DBInit(DWORD logMsgCode, DWORD sqlErrorMsgCode)
{
	memset(s_drivers, 0, sizeof(s_drivers));
	s_driverListLock = MutexCreate();

	g_logMsgCode = logMsgCode;
   s_writeLog = (logMsgCode > 0);
	g_sqlErrorMsgCode = sqlErrorMsgCode;
   s_logSqlErrors = (sqlErrorMsgCode > 0) && s_writeLog;

	return true;
}

/**
 * Load and initialize database driver
 * If logMsgCode == 0, logging will be disabled
 * If sqlErrorMsgCode == 0, failed SQL queries will not be logged
 *
 * @return driver handle on success, NULL on failure
 */
DB_DRIVER LIBNXDB_EXPORTABLE DBLoadDriver(const TCHAR *module, const TCHAR *initParameters,
														bool dumpSQL, void (* fpEventHandler)(DWORD, const WCHAR *, const WCHAR *, void *),
														void *userArg)
{
   static DWORD dwVersionZero = 0;
   bool (* fpDrvInit)(const char *);
   DWORD *pdwAPIVersion;
   TCHAR szErrorText[256];
	const char *driverName;
	DB_DRIVER driver;
	bool alreadyLoaded = false;
	int position = -1;
   int i; // have to define it here because otherwise HP aCC complains at goto statements

	MutexLock(s_driverListLock);

	driver = (DB_DRIVER)malloc(sizeof(db_driver_t));
	memset(driver, 0, sizeof(db_driver_t));

	driver->m_logSqlErrors = s_logSqlErrors;
   driver->m_dumpSql = dumpSQL;
   driver->m_fpEventHandler = fpEventHandler;
	driver->m_userArg = userArg;

   // Load driver's module
#ifdef _WIN32
   driver->m_handle = DLOpen(module, szErrorText);
#else
	TCHAR fullName[MAX_PATH];
	if (_tcschr(module, _T('/')) == NULL)
	{
      const TCHAR *homeDir = _tgetenv(_T("NETXMS_HOME"));
      if ((homeDir != NULL) && (*homeDir != 0))
      {
         _sntprintf(fullName, MAX_PATH, _T("%s/lib/netxms/dbdrv/%s"), homeDir, module);
      }
      else
      {
		   _sntprintf(fullName, MAX_PATH, _T("%s/dbdrv/%s"), PKGLIBDIR, module);
      }
	}
	else
	{
		nx_strncpy(fullName, module, MAX_PATH);
	}
   driver->m_handle = DLOpen(fullName, szErrorText);
#endif
   if (driver->m_handle == NULL)
   {
      if (s_writeLog)
         __DBWriteLog(EVENTLOG_ERROR_TYPE, _T("Unable to load database driver module \"%s\": %s"), module, szErrorText);
		goto failure;
   }

   // Check API version supported by driver
   pdwAPIVersion = (DWORD *)DLGetSymbolAddr(driver->m_handle, "drvAPIVersion", NULL);
   if (pdwAPIVersion == NULL)
      pdwAPIVersion = &dwVersionZero;
   if (*pdwAPIVersion != DBDRV_API_VERSION)
   {
      if (s_writeLog)
         __DBWriteLog(EVENTLOG_ERROR_TYPE, _T("Database driver \"%s\" cannot be loaded because of API version mismatch (driver: %d; server: %d)"),
                  module, (int)(DBDRV_API_VERSION), (int)(*pdwAPIVersion));
		goto failure;
   }

	// Check name
	driverName = *((const char **)DLGetSymbolAddr(driver->m_handle, "drvName", NULL));
	if (driverName == NULL)
	{
		if (s_writeLog)
			__DBWriteLog(EVENTLOG_ERROR_TYPE, _T("Unable to find all required entry points in database driver \"%s\""), module);
		goto failure;
	}

	for(i = 0; i < MAX_DB_DRIVERS; i++)
	{
		if ((s_drivers[i] != NULL) && (!stricmp(s_drivers[i]->m_name, driverName)))
		{
			alreadyLoaded = true;
			position = i;
			break;
		}
		if (s_drivers[i] == NULL)
			position = i;
	}

	if (alreadyLoaded)
	{
      if (s_writeLog)
			__DBWriteLog(EVENTLOG_INFORMATION_TYPE, _T("Reusing already loaded database driver \"%s\""), s_drivers[position]->m_name);
		goto reuse_driver;
	}

	if (position == -1)
	{
      if (s_writeLog)
			__DBWriteLog(EVENTLOG_ERROR_TYPE, _T("Unable to load database driver \"%s\": too many drivers already loaded"), module);
		goto failure;
	}

   // Import symbols
   fpDrvInit = (bool (*)(const char *))DLGetSymbolAddrEx(driver->m_handle, "DrvInit");
   driver->m_fpDrvConnect = (DBDRV_CONNECTION (*)(const char *, const char *, const char *, const char *, const char *, WCHAR *))DLGetSymbolAddrEx(driver->m_handle, "DrvConnect");
   driver->m_fpDrvDisconnect = (void (*)(DBDRV_CONNECTION))DLGetSymbolAddrEx(driver->m_handle, "DrvDisconnect");
	driver->m_fpDrvPrepare = (DBDRV_STATEMENT (*)(DBDRV_CONNECTION, const WCHAR *, DWORD *, WCHAR *))DLGetSymbolAddrEx(driver->m_handle, "DrvPrepare");
	driver->m_fpDrvFreeStatement = (void (*)(DBDRV_STATEMENT))DLGetSymbolAddrEx(driver->m_handle, "DrvFreeStatement");
	driver->m_fpDrvBind = (void (*)(DBDRV_STATEMENT, int, int, int, void *, int))DLGetSymbolAddrEx(driver->m_handle, "DrvBind");
	driver->m_fpDrvExecute = (DWORD (*)(DBDRV_CONNECTION, DBDRV_STATEMENT, WCHAR *))DLGetSymbolAddrEx(driver->m_handle, "DrvExecute");
   driver->m_fpDrvQuery = (DWORD (*)(DBDRV_CONNECTION, const WCHAR *, WCHAR *))DLGetSymbolAddrEx(driver->m_handle, "DrvQuery");
   driver->m_fpDrvSelect = (DBDRV_RESULT (*)(DBDRV_CONNECTION, const WCHAR *, DWORD *, WCHAR *))DLGetSymbolAddrEx(driver->m_handle, "DrvSelect");
   driver->m_fpDrvAsyncSelect = (DBDRV_ASYNC_RESULT (*)(DBDRV_CONNECTION, const WCHAR *, DWORD *, WCHAR *))DLGetSymbolAddrEx(driver->m_handle, "DrvAsyncSelect");
	driver->m_fpDrvSelectPrepared = (DBDRV_RESULT (*)(DBDRV_CONNECTION, DBDRV_STATEMENT, DWORD *, WCHAR *))DLGetSymbolAddrEx(driver->m_handle, "DrvSelectPrepared");
   driver->m_fpDrvFetch = (bool (*)(DBDRV_ASYNC_RESULT))DLGetSymbolAddrEx(driver->m_handle, "DrvFetch");
   driver->m_fpDrvGetFieldLength = (LONG (*)(DBDRV_RESULT, int, int))DLGetSymbolAddrEx(driver->m_handle, "DrvGetFieldLength");
   driver->m_fpDrvGetFieldLengthAsync = (LONG (*)(DBDRV_RESULT, int))DLGetSymbolAddrEx(driver->m_handle, "DrvGetFieldLengthAsync");
   driver->m_fpDrvGetField = (WCHAR* (*)(DBDRV_RESULT, int, int, WCHAR *, int))DLGetSymbolAddrEx(driver->m_handle, "DrvGetField");
   driver->m_fpDrvGetFieldUTF8 = (char* (*)(DBDRV_RESULT, int, int, char *, int))DLGetSymbolAddrEx(driver->m_handle, "DrvGetFieldUTF8", false); // optional entry point
   driver->m_fpDrvGetFieldAsync = (WCHAR* (*)(DBDRV_ASYNC_RESULT, int, WCHAR *, int))DLGetSymbolAddrEx(driver->m_handle, "DrvGetFieldAsync");
   driver->m_fpDrvGetNumRows = (int (*)(DBDRV_RESULT))DLGetSymbolAddrEx(driver->m_handle, "DrvGetNumRows");
   driver->m_fpDrvGetColumnCount = (int (*)(DBDRV_RESULT))DLGetSymbolAddrEx(driver->m_handle, "DrvGetColumnCount");
   driver->m_fpDrvGetColumnName = (const char* (*)(DBDRV_RESULT, int))DLGetSymbolAddrEx(driver->m_handle, "DrvGetColumnName");
   driver->m_fpDrvGetColumnCountAsync = (int (*)(DBDRV_ASYNC_RESULT))DLGetSymbolAddrEx(driver->m_handle, "DrvGetColumnCountAsync");
   driver->m_fpDrvGetColumnNameAsync = (const char* (*)(DBDRV_ASYNC_RESULT, int))DLGetSymbolAddrEx(driver->m_handle, "DrvGetColumnNameAsync");
   driver->m_fpDrvFreeResult = (void (*)(DBDRV_RESULT))DLGetSymbolAddrEx(driver->m_handle, "DrvFreeResult");
   driver->m_fpDrvFreeAsyncResult = (void (*)(DBDRV_ASYNC_RESULT))DLGetSymbolAddrEx(driver->m_handle, "DrvFreeAsyncResult");
   driver->m_fpDrvBegin = (DWORD (*)(DBDRV_CONNECTION))DLGetSymbolAddrEx(driver->m_handle, "DrvBegin");
   driver->m_fpDrvCommit = (DWORD (*)(DBDRV_CONNECTION))DLGetSymbolAddrEx(driver->m_handle, "DrvCommit");
   driver->m_fpDrvRollback = (DWORD (*)(DBDRV_CONNECTION))DLGetSymbolAddrEx(driver->m_handle, "DrvRollback");
   driver->m_fpDrvUnload = (void (*)(void))DLGetSymbolAddrEx(driver->m_handle, "DrvUnload");
   driver->m_fpDrvPrepareStringA = (char* (*)(const char *))DLGetSymbolAddrEx(driver->m_handle, "DrvPrepareStringA");
   driver->m_fpDrvPrepareStringW = (WCHAR* (*)(const WCHAR *))DLGetSymbolAddrEx(driver->m_handle, "DrvPrepareStringW");
   driver->m_fpDrvIsTableExist = (int (*)(DBDRV_CONNECTION, const WCHAR *))DLGetSymbolAddrEx(driver->m_handle, "DrvIsTableExist");
   if ((fpDrvInit == NULL) || (driver->m_fpDrvConnect == NULL) || (driver->m_fpDrvDisconnect == NULL) ||
	    (driver->m_fpDrvPrepare == NULL) || (driver->m_fpDrvBind == NULL) || (driver->m_fpDrvFreeStatement == NULL) ||
       (driver->m_fpDrvQuery == NULL) || (driver->m_fpDrvSelect == NULL) || (driver->m_fpDrvGetField == NULL) ||
       (driver->m_fpDrvGetNumRows == NULL) || (driver->m_fpDrvFreeResult == NULL) || (driver->m_fpDrvSelectPrepared == NULL) ||
       (driver->m_fpDrvUnload == NULL) || (driver->m_fpDrvAsyncSelect == NULL) || (driver->m_fpDrvFetch == NULL) ||
       (driver->m_fpDrvFreeAsyncResult == NULL) || (driver->m_fpDrvGetFieldAsync == NULL) ||
       (driver->m_fpDrvBegin == NULL) || (driver->m_fpDrvCommit == NULL) || (driver->m_fpDrvRollback == NULL) ||
		 (driver->m_fpDrvGetColumnCount == NULL) || (driver->m_fpDrvGetColumnName == NULL) ||
		 (driver->m_fpDrvGetColumnCountAsync == NULL) || (driver->m_fpDrvGetColumnNameAsync == NULL) ||
       (driver->m_fpDrvGetFieldLength == NULL) || (driver->m_fpDrvGetFieldLengthAsync == NULL) ||
		 (driver->m_fpDrvPrepareStringA == NULL) || (driver->m_fpDrvPrepareStringW == NULL) || (driver->m_fpDrvIsTableExist == NULL))
   {
      if (s_writeLog)
         __DBWriteLog(EVENTLOG_ERROR_TYPE, _T("Unable to find all required entry points in database driver \"%s\""), module);
		goto failure;
   }

   // Initialize driver
#ifdef UNICODE
   char mbInitParameters[1024];
   if (initParameters != NULL)
   {
      WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, initParameters, -1, mbInitParameters, 1024, NULL, NULL);
      mbInitParameters[1023] = 0;
   }
   else
   {
      mbInitParameters[0] = 0;
   }
   if (!fpDrvInit(mbInitParameters))
#else
   if (!fpDrvInit(CHECK_NULL_EX(initParameters)))
#endif
   {
      if (s_writeLog)
         __DBWriteLog(EVENTLOG_ERROR_TYPE, _T("Database driver \"%s\" initialization failed"), module);
		goto failure;
   }

   // Success
   driver->m_mutexReconnect = MutexCreate();
	driver->m_name = driverName;
	driver->m_refCount = 1;
	s_drivers[position] = driver;
   if (s_writeLog)
      __DBWriteLog(EVENTLOG_INFORMATION_TYPE, _T("Database driver \"%s\" loaded and initialized successfully"), module);
	MutexUnlock(s_driverListLock);
   return driver;

failure:
	if (driver->m_handle != NULL)
		DLClose(driver->m_handle);
	free(driver);
	MutexUnlock(s_driverListLock);
	return NULL;

reuse_driver:
	if (driver->m_handle != NULL)
		DLClose(driver->m_handle);
	free(driver);
	s_drivers[position]->m_refCount++;
	MutexUnlock(s_driverListLock);
	return s_drivers[position];
}

/**
 * Unload driver
 */
void LIBNXDB_EXPORTABLE DBUnloadDriver(DB_DRIVER driver)
{
	MutexLock(s_driverListLock);

	for(int i = 0; i < MAX_DB_DRIVERS; i++)
	{
		if (s_drivers[i] == driver)
		{
			driver->m_refCount--;
			if (driver->m_refCount <= 0)
			{
				driver->m_fpDrvUnload();
				DLClose(driver->m_handle);
				MutexDestroy(driver->m_mutexReconnect);
				free(driver);
				s_drivers[i] = NULL;
			}
			break;
		}
	}

	MutexUnlock(s_driverListLock);
}

/**
 * Get name of loaded driver
 */
const char LIBNXDB_EXPORTABLE *DBGetDriverName(DB_DRIVER driver)
{
	return driver->m_name;
}
