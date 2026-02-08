/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: pds.cpp
**
**/

#include "nxcore.h"
#include <pdsdrv.h>
#include <netxms-version.h>

#define MAX_PDS_DRIVERS		8

#define DEBUG_TAG L"pdsdrv"

/**
 * Server configuration
 */
extern Config g_serverConfig;

/**
 * Drivers to load
 */
wchar_t *g_pdsLoadList = nullptr;

/**
 * List of loaded drivers
 */
static int s_numDrivers = 0;
static PerfDataStorageDriver *s_drivers[MAX_PDS_DRIVERS];

/**
 * Driver base class constructor
 */
PerfDataStorageDriver::PerfDataStorageDriver()
{
}

/**
 * Driver base class destructor
 */
PerfDataStorageDriver::~PerfDataStorageDriver()
{
}

/**
 * Get driver's name
 */
const wchar_t *PerfDataStorageDriver::getName()
{
   return L"GENERIC";
}

/**
 * Get driver's version
 */
const wchar_t *PerfDataStorageDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Initialize driver. Default implementation always returns true.
 */
bool PerfDataStorageDriver::init(Config *config)
{
   return true;
}

/**
 * Shutdown driver
 */
void PerfDataStorageDriver::shutdown()
{
}

/**
 * Save DCI value
 */
bool PerfDataStorageDriver::saveDCItemValue(DCItem *dcObject, Timestamp timestamp, const wchar_t *value)
{
   return false;
}

/**
 * Save table value
 */
bool PerfDataStorageDriver::saveDCTableValue(DCTable *dcObject, Timestamp timestamp, Table *value)
{
   return false;
}

/**
 * Get internal metric
 */
DataCollectionError PerfDataStorageDriver::getInternalMetric(const TCHAR *metric, wchar_t *value)
{
   return DCE_NOT_SUPPORTED;
}

/**
 * Storage request
 */
void PerfDataStorageRequest(DCItem *dci, Timestamp timestamp, const wchar_t *value)
{
   for(int i = 0; i < s_numDrivers; i++)
      s_drivers[i]->saveDCItemValue(dci, timestamp, value);
}

/**
 * Storage request
 */
void PerfDataStorageRequest(DCTable *dci, Timestamp timestamp, Table *value)
{
   for(int i = 0; i < s_numDrivers; i++)
      s_drivers[i]->saveDCTableValue(dci, timestamp, value);
}

/**
 * Load perf data storage driver
 *
 * @param file Driver's file name
 */
static void LoadDriver(const wchar_t *file)
{
   wchar_t errorText[256], fullName[MAX_PATH];
#ifdef _WIN32
   bool resetDllPath = false;
	if (wcschr(file, L'\\') == nullptr)
	{
	   wchar_t path[MAX_PATH];
	   wcscpy(path, g_netxmsdLibDir);
	   wcscat(path, LDIR_PDSDRV);
   	SetDllDirectory(path);
      resetDllPath = true;
   }

   size_t len = wcslen(fullName);
   if ((len < 5) || (wcsicmp(&fullName[len - 5], L".pdsd") && wcsicmp(&fullName[len - 4], L".dll")))
      wcslcat(fullName, L".pdsd", MAX_PATH);

	HMODULE hModule = DLOpen(fullName, errorText);

   if (resetDllPath)
   {
      SetDllDirectory(nullptr);
   }
#else
	if (wcschr(file, L'/') == nullptr)
	{
	   wchar_t libdir[MAX_PATH];
	   GetNetXMSDirectory(nxDirLib, libdir);
      _sntprintf(fullName, MAX_PATH, L"%s/pdsdrv/%s", libdir, file);
	}
	else
	{
		wcslcpy(fullName, file, MAX_PATH);
	}

   size_t len = wcslen(fullName);
   if ((len < 5) || (wcsicmp(&fullName[len - 5], L".pdsd") && wcsicmp(&fullName[len - wcslen(SHLIB_SUFFIX)], SHLIB_SUFFIX)))
      wcslcat(fullName, L".pdsd", MAX_PATH);

   HMODULE hModule = DLOpen(fullName, errorText);
#endif

   if (hModule != nullptr)
   {
		int *apiVersion = (int *)DLGetSymbolAddr(hModule, "pdsdrvAPIVersion", errorText);
      PerfDataStorageDriver *(*CreateInstance)() = (PerfDataStorageDriver *(*)())DLGetSymbolAddr(hModule, "pdsdrvCreateInstance", errorText);

      if ((apiVersion != nullptr) && (CreateInstance != nullptr))
      {
         if (*apiVersion == PDSDRV_API_VERSION)
         {
            PerfDataStorageDriver *driver = CreateInstance();
            if ((driver != nullptr) && driver->init(&g_serverConfig))
            {
               s_drivers[s_numDrivers++] = driver;
               nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, L"Performance data storage driver %s loaded successfully", driver->getName());
            }
            else
            {
               delete driver;
               nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Initialization of performance data storage driver \"%s\" failed", file);
               DLClose(hModule);
            }
         }
         else
         {
            nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Performance data storage driver \"%s\" cannot be loaded because of API version mismatch (driver: %d; server: %d)",
                     file, *apiVersion, PDSDRV_API_VERSION);
            DLClose(hModule);
         }
      }
      else
      {
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Unable to find entry point in performance data storage driver \"%s\"", file);
         DLClose(hModule);
      }
   }
   else
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Unable to load performance data storage driver \"%s\" (%s)", file, errorText);
   }
}

/**
 * Load all available performance data storage drivers
 */
void LoadPerfDataStorageDrivers()
{
   memset(s_drivers, 0, sizeof(PerfDataStorageDriver *) * MAX_PDS_DRIVERS);

   nxlog_debug_tag(DEBUG_TAG, 1, L"Loading performance data storage drivers");
   for(wchar_t *curr = g_pdsLoadList, *next = nullptr; curr != nullptr; curr = next)
   {
      next = _tcschr(curr, _T('\n'));
      if (next != nullptr)
      {
         *next = 0;
         next++;
      }
      Trim(curr);
      if (*curr == 0)
         continue;

      LoadDriver(curr);
      if (s_numDrivers == MAX_PDS_DRIVERS)
         break;	// Too many drivers already loaded
   }
   if (s_numDrivers > 0)
      g_flags |= AF_PERFDATA_STORAGE_DRIVER_LOADED;
   nxlog_debug_tag(DEBUG_TAG, 1, L"%d performance data storage drivers loaded", s_numDrivers);
}

/**
 * Shutdown performance data storage drivers
 */
void ShutdownPerfDataStorageDrivers()
{
   for(int i = 0; i < s_numDrivers; i++)
   {
      nxlog_debug_tag(DEBUG_TAG, 2, L"Executing shutdown handler for driver %s", s_drivers[i]->getName());
      s_drivers[i]->shutdown();
      delete s_drivers[i];
   }
   nxlog_debug_tag(DEBUG_TAG, 1, L"All performance data storage drivers unloaded");
}

/**
 * Get internal metric from performance data storage driver
 */
DataCollectionError GetPerfDataStorageDriverMetric(const wchar_t *driver, const wchar_t *metric, wchar_t *value)
{
   DataCollectionError rc = DCE_NO_SUCH_INSTANCE;
   for(int i = 0; i < s_numDrivers; i++)
   {
      if (!wcsicmp(s_drivers[i]->getName(), driver))
      {
         rc = s_drivers[i]->getInternalMetric(metric, value);
         break;
      }
   }
   return rc;
}
