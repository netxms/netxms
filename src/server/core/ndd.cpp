/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
** File: ndd.cpp
**
**/

#include "nxcore.h"

#define MAX_DEVICE_DRIVERS		1024

/**
 * List of loaded drivers
 */
static int s_numDrivers = 0;
static NetworkDeviceDriver *s_drivers[MAX_DEVICE_DRIVERS];
static NetworkDeviceDriver *s_defaultDriver = new NetworkDeviceDriver();

/**
 * Load device driver
 *
 * @param file Driver's file name
 */
static void LoadDriver(const TCHAR *file)
{
   TCHAR errorText[256];

   HMODULE hModule = DLOpen(file, errorText);
   if (hModule != NULL)
   {
		int *apiVersion = (int *)DLGetSymbolAddr(hModule, "nddAPIVersion", errorText);
      NetworkDeviceDriver *(* CreateInstance)() = (NetworkDeviceDriver *(*)())DLGetSymbolAddr(hModule, "nddCreateInstance", errorText);

      if ((apiVersion != NULL) && (CreateInstance != NULL))
      {
         if (*apiVersion == NDDRV_API_VERSION)
         {
				NetworkDeviceDriver *driver = CreateInstance();
				if (driver != NULL)
				{
					s_drivers[s_numDrivers++] = driver;
					nxlog_write(MSG_NDD_LOADED, EVENTLOG_INFORMATION_TYPE, "s", driver->getName());
				}
				else
				{
					nxlog_write(MSG_NDD_INIT_FAILED, EVENTLOG_ERROR_TYPE, "s", file);
					DLClose(hModule);
				}
         }
         else
         {
            nxlog_write(MSG_NDD_API_VERSION_MISMATCH, EVENTLOG_ERROR_TYPE, "sdd", file, NDDRV_API_VERSION, *apiVersion);
            DLClose(hModule);
         }
      }
      else
      {
         nxlog_write(MSG_NO_NDD_ENTRY_POINT, EVENTLOG_ERROR_TYPE, "s", file);
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
void LoadNetworkDeviceDrivers()
{
	memset(s_drivers, 0, sizeof(NetworkDeviceDriver *) * MAX_DEVICE_DRIVERS);

	TCHAR path[MAX_PATH];
	_tcscpy(path, g_szLibDir);
	_tcscat(path, LDIR_NDD);

	DbgPrintf(1, _T("Loading network device drivers from %s"), path);
#ifdef _WIN32
	SetDllDirectory(path);
#endif
	_TDIR *dir = _topendir(path);
	if (dir != NULL)
	{
		_tcscat(path, FS_PATH_SEPARATOR);
		int insPos = (int)_tcslen(path);

		struct _tdirent *f;
		while((f = _treaddir(dir)) != NULL)
		{
			if (MatchString(_T("*.ndd"), f->d_name, FALSE))
			{
				_tcscpy(&path[insPos], f->d_name);
				LoadDriver(path);
				if (s_numDrivers == MAX_DEVICE_DRIVERS)
					break;	// Too many drivers already loaded
			}
		}
		_tclosedir(dir);
	}
#ifdef _WIN32
	SetDllDirectory(NULL);
#endif
	DbgPrintf(1, _T("%d network device drivers loaded"), s_numDrivers);
}

/**
 * Information about selected (potential) driver
 */
struct __selected_driver
{
	int priority;
	NetworkDeviceDriver *driver;
};

/**
 * Compare two drivers by priority
 */
static int CompareDrivers(const void *e1, const void *e2)
{
	return -(((struct __selected_driver *)e1)->priority - ((struct __selected_driver *)e2)->priority);
}

/**
 * Find appropriate device driver for given node
 *
 * @param node Node object to test
 * @returns Pointer to device driver object
 */
NetworkDeviceDriver *FindDriverForNode(Node *node, SNMP_Transport *pTransport)
{
	struct __selected_driver selection[MAX_DEVICE_DRIVERS];
	int selected = 0;

	for(int i = 0; i < s_numDrivers; i++)
	{
		int pri = s_drivers[i]->isPotentialDevice(node->getObjectId());
		if (pri > 0)
		{
			if (pri > 255)
				pri = 255;
			selection[selected].priority = pri;
			selection[selected].driver = s_drivers[i];
			selected++;
			DbgPrintf(6, _T("FindDriverForNode(%s): found potential device driver %s with priority %d"),
			          node->Name(), s_drivers[i]->getName(), pri);
		}
	}

	if (selected > 0)
	{
		qsort(selection, selected, sizeof(struct __selected_driver), CompareDrivers);
		for(int i = 0 ; i < selected; i++)
		{
			if (selection[i].driver->isDeviceSupported(pTransport, node->getObjectId()))
				return selection[i].driver;
		}
	}

	return s_defaultDriver;
}

/**
 * Add all custom test OIDs specified by drivers to OID list
 */
void AddDriverSpecificOids(StringList *list)
{
	for(int i = 0; i < s_numDrivers; i++)
	{
      const TCHAR *oid = s_drivers[i]->getCustomTestOID();
      if (oid != NULL)
         list->add(oid);
	}
}

/**
 * Find device driver by name. Will return default driver if name is empty
 * or driver with given name does not exist.
 *
 * @param name driver's name
 * @returns pointer to device driver object
 */
NetworkDeviceDriver *FindDriverByName(const TCHAR *name)
{
	if ((name == NULL) || (name[0] == 0))
		return s_defaultDriver;

	for(int i = 0; i < s_numDrivers; i++)
	{
		if (!_tcsicmp(s_drivers[i]->getName(), name))
			return s_drivers[i];
	}

	return s_defaultDriver;
}
