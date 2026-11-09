/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
      ObjectArray<NetworkDeviceDriver> *(* CreateInstances)() = DLGetFunctionAddr<ObjectArray<NetworkDeviceDriver> *(*)()>(hModule, "nddCreateInstances", errorText);
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

	wchar_t path[MAX_PATH];
	wcscpy(path, g_netxmsdLibDir);
	wcscat(path, LDIR_NDD);

	wchar_t buffer[MAX_CONFIG_VALUE_LENGTH];
	ConfigReadStr(L"NetworkDeviceDrivers.Blacklist", buffer, MAX_CONFIG_VALUE_LENGTH, _T(""));
   StringList blacklist;
	blacklist.splitAndAdd(buffer, L",");

	nxlog_debug_tag(DEBUG_TAG, 1, L"Loading network device drivers from %s", path);
#ifdef _WIN32
	SetDllDirectory(path);
#endif
	DIRHANDLEW *dir = OpenDirW(path);
	if (dir != nullptr)
	{
		wcscat(path, FS_PATH_SEPARATOR);
		int insPos = (int)_tcslen(path);

		DIRENTRYW *f;
		while((f = ReadDirW(dir)) != nullptr)
		{
			if (MatchStringW(L"*.ndd", f->d_name, false))
			{
				wcscpy(&path[insPos], f->d_name);
				LoadDriver(path, blacklist);
				if (s_numDrivers == MAX_DEVICE_DRIVERS)
					break;	// Too many drivers already loaded
			}
		}
		CloseDirW(dir);
	}
#ifdef _WIN32
	SetDllDirectory(nullptr);
#endif
	nxlog_debug_tag(DEBUG_TAG, 1, L"%d network device drivers loaded", s_numDrivers);
}

/**
 * Priority assigned to transport-compatible drivers when device's SNMP object ID cannot be used
 */
#define BASELINE_DRIVER_PRIORITY 10

/**
 * Information about selected (potential) driver
 */
struct SelectedDriver
{
   int priority;
   int index;   // Position in driver list, resolves ties by registration order
   NetworkDeviceDriver *driver;
};

/**
 * Compare two drivers by priority (higher priority first, then by registration order)
 */
static int CompareDrivers(const void *e1, const void *e2)
{
   const SelectedDriver *d1 = static_cast<const SelectedDriver*>(e1);
   const SelectedDriver *d2 = static_cast<const SelectedDriver*>(e2);
   if (d1->priority != d2->priority)
      return d2->priority - d1->priority;
   return d1->index - d2->index;
}

/**
 * Check if driver can be used with given set of available transports: all required
 * transports must be available and at least one supported transport must be available.
 */
bool IsDriverUsable(NetworkDeviceDriver *driver, uint32_t availableTransports)
{
   uint32_t required = driver->getRequiredTransports();
   return ((required & availableTransports) == required) && ((driver->getSupportedTransports() & availableTransports) != 0);
}

/**
 * Select device driver using transports available in given device context. Candidates are ranked by
 * isPotentialDevice() when SNMP is available, otherwise every transport-compatible driver gets baseline
 * priority. Candidates are confirmed by isDeviceSupported() with shared device context. If no candidate
 * confirms, manually selected driver is used if it is compatible with available transports. Generic
 * driver is returned as last resort; on device without SNMP it is a placeholder that should not be called.
 */
static NetworkDeviceDriver *SelectDriver(const wchar_t *name, const SNMP_ObjectId& snmpObjectId, const wchar_t *manualDriverName, DeviceContext *context)
{
   uint32_t availableTransports = context->getAvailableTransports();
   bool useObjectId = (availableTransports & NDD_TRANSPORT_SNMP) != 0;
   nxlog_debug_tag(DEBUG_TAG, 6, L"FindDriverForNode(%s): available transports 0x%04X", name, availableTransports);

   SelectedDriver selection[MAX_DEVICE_DRIVERS];
   int selected = 0;
   for(int i = 0; i < s_numDrivers; i++)
   {
      NetworkDeviceDriver *driver = s_drivers[i];
      if (!IsDriverUsable(driver, availableTransports))
         continue;

      int pri = useObjectId ? driver->isPotentialDevice(snmpObjectId) : BASELINE_DRIVER_PRIORITY;
      if (pri <= 0)
         continue;

      if (pri > 255)
         pri = 255;
      selection[selected].priority = pri;
      selection[selected].index = i;
      selection[selected].driver = driver;
      selected++;
      nxlog_debug_tag(DEBUG_TAG, 6, L"FindDriverForNode(%s): found potential device driver %s with priority %d", name, driver->getName(), pri);
   }

   if (selected > 0)
   {
      qsort(selection, selected, sizeof(SelectedDriver), CompareDrivers);
      for(int i = 0; i < selected; i++)
      {
         if (selection[i].driver->isDeviceSupported(context, snmpObjectId))
            return selection[i].driver;
      }
   }

   // Manual driver selection
   if ((manualDriverName != nullptr) && (manualDriverName[0] != 0))
   {
      NetworkDeviceDriver *driver = FindDriverByName(manualDriverName);
      if (IsDriverUsable(driver, availableTransports))
      {
         nxlog_debug_tag(DEBUG_TAG, 6, L"FindDriverForNode(%s): device driver %s selected manually", name, driver->getName());
         return driver;
      }
      nxlog_debug_tag(DEBUG_TAG, 6, L"FindDriverForNode(%s): manually selected device driver %s ignored (incompatible with available transports)", name, driver->getName());
   }

   if (!useObjectId)
      nxlog_debug_tag(DEBUG_TAG, 6, L"FindDriverForNode(%s): no device driver confirmed for device without SNMP", name);
   return &s_defaultDriver;
}

/**
 * Find appropriate device driver for given node, using transports available on that node
 * (SNMP object ID and custom attribute snmp.driver are taken from node object)
 *
 * @param node Node object to test
 * @param context device context for given node, shared by all candidate drivers
 * @returns Pointer to device driver object
 */
NetworkDeviceDriver *FindDriverForNode(Node *node, DeviceContext *context)
{
   wchar_t driverName[64];
   return SelectDriver(node->getName(), node->getSNMPObjectId(), node->getCustomAttribute(L"snmp.driver", driverName, 64), context);
}

/**
 * Find appropriate device driver for network device accessible via SNMP only (used before node object is created)
 *
 * @param name device name for logging
 * @param snmpObjectId SNMP object ID
 * @param snmpTransport SNMP transport
 * @returns Pointer to device driver object
 */
NetworkDeviceDriver *FindDriverForNode(const wchar_t *name, const SNMP_ObjectId& snmpObjectId, SNMP_Transport *snmpTransport)
{
   SnmpDeviceContext context(snmpTransport);
   return SelectDriver(name, snmpObjectId, nullptr, &context);
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
