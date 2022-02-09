/*
** NetXMS - Network Management System
** Copyright (C) 2003-2022 Victor Kirhenshtein
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
 * Long-running query threshold
 */
uint32_t g_sqlQueryExecTimeThreshold = 0xFFFFFFFF;

/**
 * Loaded drivers
 */
static DB_DRIVER s_drivers[MAX_DB_DRIVERS];
static Mutex s_driverListLock;

/**
 * Init DB library
 */
bool LIBNXDB_EXPORTABLE DBInit()
{
	memset(s_drivers, 0, sizeof(s_drivers));
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
         void (*eventHandler)(uint32_t, const WCHAR *, const WCHAR *, bool, void *), void *context)
{
   static uint32_t versionZero = 0;
   uint32_t *apiVersion = nullptr;
   const char **driverName = nullptr;
   DBDriverCallTable **callTable = nullptr;
   DB_DRIVER driver;
   bool alreadyLoaded = false;
   int position = -1;
   TCHAR szErrorText[256];
   int i; // have to define it here because otherwise HP aCC complains at goto statements

   s_driverListLock.lock();

   driver = MemAllocStruct<db_driver_t>();
   driver->m_fpEventHandler = eventHandler;
   driver->m_context = context;

   // Load driver's module
   TCHAR fullName[MAX_PATH];
#ifdef _WIN32
   _tcslcpy(fullName, module, MAX_PATH);
   size_t len = _tcslen(fullName);
   if ((len < 4) || (_tcsicmp(&fullName[len - 4], _T(".ddr")) && _tcsicmp(&fullName[len - 4], _T(".dll"))))
      _tcslcat(fullName, _T(".ddr"), MAX_PATH);
   driver->m_handle = DLOpen(fullName, szErrorText);
#else
	if (_tcscmp(module, _T(":self:")) && (_tcschr(module, _T('/')) == nullptr))
	{
	   TCHAR libdir[MAX_PATH];
	   GetNetXMSDirectory(nxDirLib, libdir);
      _sntprintf(fullName, MAX_PATH, _T("%s/dbdrv/%s"), libdir, module);
	}
	else
	{
		_tcslcpy(fullName, module, MAX_PATH);
	}

	if (_tcscmp(module, _T(":self:")))
	{
      size_t len = _tcslen(fullName);
      if ((len < 4) || (_tcsicmp(&fullName[len - 4], _T(".ddr")) && _tcsicmp(&fullName[len - _tcslen(SHLIB_SUFFIX)], SHLIB_SUFFIX)))
         _tcslcat(fullName, _T(".ddr"), MAX_PATH);
	}

   driver->m_handle = DLOpen(!_tcscmp(fullName, _T(":self:")) ? nullptr : fullName, szErrorText);
#endif
   if (driver->m_handle == nullptr)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_DRIVER, _T("Unable to load database driver module \"%s\": %s"), module, szErrorText);
		goto failure;
   }

   // Check API version supported by driver
   apiVersion = static_cast<uint32_t*>(DLGetSymbolAddr(driver->m_handle, "drvAPIVersion", nullptr));
   if (apiVersion == nullptr)
      apiVersion = &versionZero;
   if (*apiVersion != DBDRV_API_VERSION)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_DRIVER, _T("Database driver \"%s\" cannot be loaded because of API version mismatch (server: %u; driver: %u)"), module, DBDRV_API_VERSION, *apiVersion);
		goto failure;
   }

	// Check name
   driverName = static_cast<const char**>(DLGetSymbolAddr(driver->m_handle, "drvName", nullptr));
	if ((driverName == nullptr) || (*driverName == nullptr))
	{
	   nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_DRIVER, _T("Unable to find all required entry points in database driver \"%s\""), module);
		goto failure;
	}

	for(i = 0; i < MAX_DB_DRIVERS; i++)
	{
		if ((s_drivers[i] != nullptr) && (!stricmp(s_drivers[i]->m_name, *driverName)))
		{
			alreadyLoaded = true;
			position = i;
			break;
		}
		if (s_drivers[i] == nullptr)
			position = i;
	}

	if (alreadyLoaded)
	{
	   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_DRIVER, _T("Reusing already loaded database driver \"%hs\""), s_drivers[position]->m_name);
		goto reuse_driver;
	}

	if (position == -1)
	{
	   nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_DRIVER, _T("Unable to load database driver \"%s\": too many drivers already loaded"), module);
		goto failure;
	}

   // Import symbols
	callTable = (DBDriverCallTable**)DLGetSymbolAddr(driver->m_handle, "drvCallTable", nullptr);
   if ((callTable == nullptr) || (*callTable == nullptr))
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_DRIVER, _T("Unable to find all required entry points in database driver \"%s\""), module);
      goto failure;
   }
   memcpy(&driver->m_callTable, *callTable, sizeof(DBDriverCallTable));

   // Initialize driver
#ifdef UNICODE
   char mbInitParameters[1024];
   if (initParameters != nullptr)
   {
      wchar_to_mb(initParameters, -1, mbInitParameters, 1024);
      mbInitParameters[1023] = 0;
   }
   else
   {
      mbInitParameters[0] = 0;
   }
   if (!driver->m_callTable.Initialize(mbInitParameters))
#else
   if (!driver->m_callTable.Initialize(CHECK_NULL_EX(initParameters)))
#endif
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_DRIVER, _T("Database driver \"%s\" initialization failed"), module);
		goto failure;
   }

   // Success
   driver->m_mutexReconnect = new Mutex();
   driver->m_name = *driverName;
   driver->m_refCount = 1;
   driver->m_defaultPrefetchLimit = 10;
   s_drivers[position] = driver;
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_DRIVER, _T("Database driver \"%s\" loaded and initialized successfully"), module);
   s_driverListLock.unlock();
   return driver;

failure:
   if (driver->m_handle != nullptr)
      DLClose(driver->m_handle);
   MemFree(driver);
   s_driverListLock.unlock();
   return nullptr;

reuse_driver:
   if (driver->m_handle != nullptr)
      DLClose(driver->m_handle);
   MemFree(driver);
   s_drivers[position]->m_refCount++;
   s_driverListLock.unlock();
   return s_drivers[position];
}

/**
 * Unload driver
 */
void LIBNXDB_EXPORTABLE DBUnloadDriver(DB_DRIVER driver)
{
   if (driver == nullptr)
      return;

   s_driverListLock.lock();

	for(int i = 0; i < MAX_DB_DRIVERS; i++)
	{
		if (s_drivers[i] == driver)
		{
			driver->m_refCount--;
			if (driver->m_refCount <= 0)
			{
				driver->m_callTable.Unload();
				DLClose(driver->m_handle);
				delete driver->m_mutexReconnect;
				MemFree(driver);
				s_drivers[i] = nullptr;
			}
			break;
		}
	}

   s_driverListLock.unlock();
}

/**
 * Get name of loaded driver
 */
const char LIBNXDB_EXPORTABLE *DBGetDriverName(DB_DRIVER driver)
{
	return driver->m_name;
}
