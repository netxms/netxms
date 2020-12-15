/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
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

#define DEBUG_TAG _T("pdsdrv")

/**
 * Server configuration
 */
extern Config g_serverConfig;

/**
 * Drivers to load
 */
TCHAR *g_pdsLoadList = NULL;

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
const TCHAR *PerfDataStorageDriver::getName()
{
   return _T("GENERIC");
}

/**
 * Get driver's version
 */
const TCHAR *PerfDataStorageDriver::getVersion()
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
bool PerfDataStorageDriver::saveDCItemValue(DCItem *dcObject, time_t timestamp, const TCHAR *value)
{
   return false;
}

/**
 * Save table value
 */
bool PerfDataStorageDriver::saveDCTableValue(DCTable *dcObject, time_t timestamp, Table *value)
{
   return false;
}

/**
 * Storage request
 */
void PerfDataStorageRequest(DCItem *dci, time_t timestamp, const TCHAR *value)
{
   for(int i = 0; i < s_numDrivers; i++)
      s_drivers[i]->saveDCItemValue(dci, timestamp, value);
}

/**
 * Storage request
 */
void PerfDataStorageRequest(DCTable *dci, time_t timestamp, Table *value)
{
   for(int i = 0; i < s_numDrivers; i++)
      s_drivers[i]->saveDCTableValue(dci, timestamp, value);
}

/**
 * Load perf data storage driver
 *
 * @param file Driver's file name
 */
static void LoadDriver(const TCHAR *file)
{
   TCHAR errorText[256];
#ifdef _WIN32
   bool resetDllPath = false;
	if (_tcschr(file, _T('\\')) == nullptr)
	{
	   TCHAR path[MAX_PATH];
	   _tcscpy(path, g_netxmsdLibDir);
	   _tcscat(path, LDIR_PDSDRV);
   	SetDllDirectory(path);
      resetDllPath = true;
   }
   HMODULE hModule = DLOpen(file, errorText);
   if (resetDllPath)
   {
      SetDllDirectory(nullptr);
   }
#else
	TCHAR fullName[MAX_PATH];
	if (_tcschr(file, _T('/')) == nullptr)
	{
	   TCHAR libdir[MAX_PATH];
	   GetNetXMSDirectory(nxDirLib, libdir);
      _sntprintf(fullName, MAX_PATH, _T("%s/pdsdrv/%s"), libdir, file);
	}
	else
	{
		_tcslcpy(fullName, file, MAX_PATH);
	}
   HMODULE hModule = DLOpen(fullName, errorText);
#endif

   if (hModule != nullptr)
   {
		int *apiVersion = (int *)DLGetSymbolAddr(hModule, "pdsdrvAPIVersion", errorText);
      PerfDataStorageDriver *(* CreateInstance)() = (PerfDataStorageDriver *(*)())DLGetSymbolAddr(hModule, "pdsdrvCreateInstance", errorText);

      if ((apiVersion != nullptr) && (CreateInstance != nullptr))
      {
         if (*apiVersion == PDSDRV_API_VERSION)
         {
            PerfDataStorageDriver *driver = CreateInstance();
            if ((driver != NULL) && driver->init(&g_serverConfig))
            {
               s_drivers[s_numDrivers++] = driver;
               nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Performance data storage driver %s loaded successfully"), driver->getName());
            }
            else
            {
               delete driver;
               nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Initialization of performance data storage driver \"%s\" failed"), file);
               DLClose(hModule);
            }
         }
         else
         {
            nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Performance data storage driver \"%s\" cannot be loaded because of API version mismatch (driver: %d; server: %d)"),
                     file, *apiVersion, PDSDRV_API_VERSION);
            DLClose(hModule);
         }
      }
      else
      {
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unable to find entry point in performance data storage driver \"%s\""), file);
         DLClose(hModule);
      }
   }
   else
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unable to load module \"%s\" (%s)"), file, errorText);
   }
}

/**
 * Load all available performance data storage drivers
 */
void LoadPerfDataStorageDrivers()
{
   memset(s_drivers, 0, sizeof(PerfDataStorageDriver *) * MAX_PDS_DRIVERS);

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Loading performance data storage drivers"));
   for(TCHAR *curr = g_pdsLoadList, *next = nullptr; curr != nullptr; curr = next)
   {
      next = _tcschr(curr, _T('\n'));
      if (next != nullptr)
      {
         *next = 0;
         next++;
      }
      StrStrip(curr);
      if (*curr == 0)
         continue;

      LoadDriver(curr);
      if (s_numDrivers == MAX_PDS_DRIVERS)
         break;	// Too many drivers already loaded
   }
   if (s_numDrivers > 0)
      g_flags |= AF_PERFDATA_STORAGE_DRIVER_LOADED;
   nxlog_debug_tag(DEBUG_TAG, 1, _T("%d performance data storage drivers loaded"), s_numDrivers);
}

/**
 * Shutdown performance data storage drivers
 */
void ShutdownPerfDataStorageDrivers()
{
   for(int i = 0; i < s_numDrivers; i++)
   {
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Executing shutdown handler for driver %s"), s_drivers[i]->getName());
      s_drivers[i]->shutdown();
      delete s_drivers[i];
   }
   nxlog_debug_tag(DEBUG_TAG, 1, _T("All performance data storage drivers unloaded"));
}
