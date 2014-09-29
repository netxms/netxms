/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2014 Victor Kirhenshtein
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

#define MAX_PDS_DRIVERS		8

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
 * Init driver. Default implementation always returns true.
 */
bool PerfDataStorageDriver::init()
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
 * Load perf data storage driver
 *
 * @param file Driver's file name
 */
static void LoadDriver(const TCHAR *file)
{
   TCHAR errorText[256];
#ifdef _WIN32
   bool resetDllPath = false;
	if (_tcschr(file, _T('\\')) == NULL)
	{
	   TCHAR path[MAX_PATH];
	   _tcscpy(path, g_szLibDir);
	   _tcscat(path, LDIR_PDSDRV);
   	SetDllDirectory(path);
      resetDllPath = true;
   }
   HMODULE hModule = DLOpen(file, errorText);
   if (resetDllPath)
   {
      SetDllDirectory(NULL);
   }
#else
	TCHAR fullName[MAX_PATH];
	if (_tcschr(file, _T('/')) == NULL)
	{
      const TCHAR *homeDir = _tgetenv(_T("NETXMS_HOME"));
      if ((homeDir != NULL) && (*homeDir != 0))
      {
         _sntprintf(fullName, MAX_PATH, _T("%s/lib/netxms/pdsdrv/%s"), homeDir, file);
      }
      else
      {
		   _sntprintf(fullName, MAX_PATH, _T("%s/pdsdrv/%s"), PKGLIBDIR, file);
      }
	}
	else
	{
		nx_strncpy(fullName, file, MAX_PATH);
	}
   HMODULE hModule = DLOpen(fullName, errorText);
#endif

   if (hModule != NULL)
   {
		int *apiVersion = (int *)DLGetSymbolAddr(hModule, "pdsdrvAPIVersion", errorText);
      PerfDataStorageDriver *(* CreateInstance)() = (PerfDataStorageDriver *(*)())DLGetSymbolAddr(hModule, "pdsdrvCreateInstance", errorText);

      if ((apiVersion != NULL) && (CreateInstance != NULL))
      {
         if (*apiVersion == PDSDRV_API_VERSION)
         {
				PerfDataStorageDriver *driver = CreateInstance();
				if ((driver != NULL) && driver->init())
				{
					s_drivers[s_numDrivers++] = driver;
					nxlog_write(MSG_PDSDRV_LOADED, EVENTLOG_INFORMATION_TYPE, "s", driver->getName());
				}
				else
				{
               delete driver;
					nxlog_write(MSG_PDSDRV_INIT_FAILED, EVENTLOG_ERROR_TYPE, "s", file);
					DLClose(hModule);
				}
         }
         else
         {
            nxlog_write(MSG_PDSDRV_API_VERSION_MISMATCH, EVENTLOG_ERROR_TYPE, "sdd", file, PDSDRV_API_VERSION, *apiVersion);
            DLClose(hModule);
         }
      }
      else
      {
         nxlog_write(MSG_NO_PDSDRV_ENTRY_POINT, EVENTLOG_ERROR_TYPE, "s", file);
         DLClose(hModule);
      }
   }
   else
   {
      nxlog_write(MSG_DLOPEN_FAILED, EVENTLOG_ERROR_TYPE, "ss", file, errorText);
   }
}

/**
 * Load all available device drivers
 */
void LoadPerfDataStorageDrivers()
{
	memset(s_drivers, 0, sizeof(PerfDataStorageDriver *) * MAX_PDS_DRIVERS);

	DbgPrintf(1, _T("Loading performance data storage drivers"));
	for(TCHAR *curr = g_pdsLoadList, *next = NULL; curr != NULL; curr = next)
   {
		next = _tcschr(curr, _T('\n'));
		if (next != NULL)
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
	DbgPrintf(1, _T("%d performance data storage drivers loaded"), s_numDrivers);
}
