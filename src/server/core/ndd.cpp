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
** File: ndd.cpp
**
**/

#include "nxcore.h"

#define DEBUG_TAG             _T("ndd")
#define MAX_DEVICE_DRIVERS		1024

/**
 * List of loaded drivers
 */
static int s_numDrivers = 0;
static NetworkDeviceDriver *s_drivers[MAX_DEVICE_DRIVERS];
static NetworkDeviceDriver s_defaultDriver;

/**
 * Load device driver
 *
 * @param file Driver's file name
 */
static void LoadDriver(const TCHAR *file, const StringList &blacklist)
{
   TCHAR errorText[256];

   HMODULE hModule = DLOpen(file, errorText);
   if (hModule != nullptr)
   {
		int *apiVersion = (int *)DLGetSymbolAddr(hModule, "nddAPIVersion", errorText);
      ObjectArray<NetworkDeviceDriver> *(* CreateInstances)() = (ObjectArray<NetworkDeviceDriver> *(*)())DLGetSymbolAddr(hModule, "nddCreateInstances", errorText);
      if ((apiVersion != NULL) && (CreateInstances != NULL))
      {
         if (*apiVersion == NDDRV_API_VERSION)
         {
            ObjectArray<NetworkDeviceDriver> *drivers = CreateInstances();
				if (drivers != nullptr)
				{
				   int accepted = 0;
				   for(int i = 0; i < drivers->size(); i++)
				   {
				      NetworkDeviceDriver *drv = drivers->get(i);
				      if (!blacklist.containsIgnoreCase(drv->getName()))
				      {
                     s_drivers[s_numDrivers++] = drv;
                     nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Network device driver %s loaded successfully"), drv->getName());
                     accepted++;
				      }
				      else
				      {
                     nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Network device driver %s blacklisted"), drv->getName());
                     delete drv;
				      }
				   }
				   delete drivers;
				   if (accepted == 0)
				   {
	               nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("No drivers accepted from module \"%s\""), file);
	               DLClose(hModule);
				   }
				}
				else
				{
					nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Initialization of network device driver \"%s\" failed"), file);
					DLClose(hModule);
				}
         }
         else
         {
            nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Network device driver \"%s\" cannot be loaded because of API version mismatch (driver: %d; server: %d)"),
                     file, *apiVersion, NDDRV_API_VERSION);
            DLClose(hModule);
         }
      }
      else
      {
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unable to find entry point in network device driver \"%s\""), file);
         DLClose(hModule);
      }
   }
   else
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unable to load module \"%s\" (%s)"), file, errorText);
   }
}

/**
 * Load all available device drivers
 */
void LoadNetworkDeviceDrivers()
{
	memset(s_drivers, 0, sizeof(NetworkDeviceDriver *) * MAX_DEVICE_DRIVERS);

	TCHAR path[MAX_PATH];
	_tcscpy(path, g_netxmsdLibDir);
	_tcscat(path, LDIR_NDD);

	TCHAR buffer[MAX_CONFIG_VALUE_LENGTH];
	ConfigReadStr(_T("NetworkDeviceDrivers.Blacklist"), buffer, MAX_CONFIG_VALUE_LENGTH, _T(""));
   StringList blacklist;
	blacklist.splitAndAdd(buffer, _T(","));

	nxlog_debug_tag(DEBUG_TAG, 1, _T("Loading network device drivers from %s"), path);
#ifdef _WIN32
	SetDllDirectory(path);
#endif
	_TDIR *dir = _topendir(path);
	if (dir != nullptr)
	{
		_tcscat(path, FS_PATH_SEPARATOR);
		int insPos = (int)_tcslen(path);

		struct _tdirent *f;
		while((f = _treaddir(dir)) != NULL)
		{
			if (MatchString(_T("*.ndd"), f->d_name, false))
			{
				_tcscpy(&path[insPos], f->d_name);
				LoadDriver(path, blacklist);
				if (s_numDrivers == MAX_DEVICE_DRIVERS)
					break;	// Too many drivers already loaded
			}
		}
		_tclosedir(dir);
	}
#ifdef _WIN32
	SetDllDirectory(nullptr);
#endif
	nxlog_debug_tag(DEBUG_TAG, 1, _T("%d network device drivers loaded"), s_numDrivers);
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
NetworkDeviceDriver *FindDriverForNode(Node *node, SNMP_Transport *snmpTransport)
{
   TCHAR driverName[64];
   return FindDriverForNode(node->getName(), node->getSNMPObjectId(),
            node->getCustomAttribute(_T("snmp.driver"), driverName, 64), snmpTransport);
}

/**
 * Find appropriate device driver for given network device
 *
 * @param name Node name
 * @param snmpObjectId SNMP object ID
 * @param defaultDriver default driver to use
 * @param snmpTransport transport
 * @returns Pointer to device driver object
 */
NetworkDeviceDriver *FindDriverForNode(const TCHAR *name, const SNMP_ObjectId& snmpObjectId, const TCHAR *defaultDriver, SNMP_Transport *snmpTransport)
{
	struct __selected_driver selection[MAX_DEVICE_DRIVERS];
	int selected = 0;

	for(int i = 0; i < s_numDrivers; i++)
	{
		int pri = s_drivers[i]->isPotentialDevice(snmpObjectId);
		if (pri > 0)
		{
			if (pri > 255)
				pri = 255;
			selection[selected].priority = pri;
			selection[selected].driver = s_drivers[i];
			selected++;
			nxlog_debug_tag(DEBUG_TAG, 6, _T("FindDriverForNode(%s): found potential device driver %s with priority %d"),
			          name, s_drivers[i]->getName(), pri);
		}
	}

	if (selected > 0)
	{
		qsort(selection, selected, sizeof(struct __selected_driver), CompareDrivers);
		for(int i = 0 ; i < selected; i++)
		{
			if (selection[i].driver->isDeviceSupported(snmpTransport, snmpObjectId))
				return selection[i].driver;
		}
	}

   // Manual driver selection
   if (defaultDriver != nullptr)
   {
      NetworkDeviceDriver *driver = FindDriverByName(defaultDriver);
      if (driver != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("FindDriverForNode(%s): device driver %s selected manually"), name, driver->getName());
         return driver;
      }
   }

	return &s_defaultDriver;
}

/**
 * Add all custom test OIDs specified by drivers to OID list
 */
void AddDriverSpecificOids(StringList *list)
{
	for(int i = 0; i < s_numDrivers; i++)
	{
      const TCHAR *oid = s_drivers[i]->getCustomTestOID();
      if (oid != nullptr)
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
	if ((name == nullptr) || (name[0] == 0))
		return &s_defaultDriver;

	for(int i = 0; i < s_numDrivers; i++)
	{
		if (!_tcsicmp(s_drivers[i]->getName(), name))
			return s_drivers[i];
	}

	return &s_defaultDriver;
}

/**
 * Print list of loaded drivers on console
 */
void PrintNetworkDeviceDriverList(ServerConsole *console)
{
   console->print(_T("Loaded network device drivers:\n"));
   for(int i = 0; i < s_numDrivers; i++)
   {
      console->printf(_T("   %-32s %-16s\n"), s_drivers[i]->getName(), s_drivers[i]->getVersion());
   }
   console->printf(_T("%d drivers loaded\n"), s_numDrivers);
}
