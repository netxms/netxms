/*
** NetXMS Oxidized integration module
** Copyright (C) 2026 Raden Solutions
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
** File: oxidized.cpp
**/

#include "oxidized.h"
#include <netxms-version.h>

/**
 * Module metadata
 */
DEFINE_MODULE_METADATA("OXIDIZED", "Raden Solutions", NETXMS_VERSION_STRING_A, NETXMS_BUILD_TAG_A)

/**
 * External functions
 */
DeviceBackupApiStatus OxidizedRegisterDevice(Node *node);
DeviceBackupApiStatus OxidizedDeleteDevice(Node *node);
bool OxidizedIsDeviceRegistered(const Node& node);
DeviceBackupApiStatus OxidizedValidateDeviceRegistration(Node *node);

DeviceBackupApiStatus OxidizedStartJob(const Node& node);
std::pair<DeviceBackupApiStatus, DeviceBackupJobInfo> OxidizedGetLastJobStatus(const Node& node);
std::pair<DeviceBackupApiStatus, BackupData> OxidizedGetLatestBackup(const Node& node);
std::pair<DeviceBackupApiStatus, std::vector<BackupData>> OxidizedGetBackupList(const Node& node);
std::pair<DeviceBackupApiStatus, BackupData> OxidizedGetBackupById(const Node& node, int64_t id);

/**
 * Global configuration variables
 */
char g_oxidizedBaseURL[256] = "http://127.0.0.1:8888";
char g_oxidizedLogin[128] = "";
char g_oxidizedPassword[128] = "";
char g_oxidizedDefaultModel[64] = "";

/**
 * Vendor to Oxidized model mapping (case-insensitive vendor match)
 */
static StringMap s_vendorModelMap;

/**
 * Initialize default vendor-to-model mappings
 */
static void InitializeVendorModelMap()
{
   // Cisco
   s_vendorModelMap.set(L"Cisco", L"ios");
   s_vendorModelMap.set(L"Cisco Systems Inc.", L"ios");
   // Juniper
   s_vendorModelMap.set(L"Juniper Networks", L"junos");
   // MikroTik
   s_vendorModelMap.set(L"MikroTik", L"routeros");
   // Huawei
   s_vendorModelMap.set(L"Huawei", L"vrp");
   // Fortinet
   s_vendorModelMap.set(L"Fortinet", L"fortios");
   // HPE / Aruba
   s_vendorModelMap.set(L"HPE Aruba Networking", L"arubaos");
   s_vendorModelMap.set(L"Hewlett Packard Enterprise", L"procurve");
   // Extreme
   s_vendorModelMap.set(L"Extreme Networks", L"xos");
   // D-Link
   s_vendorModelMap.set(L"D-Link", L"dlink");
   // TP-Link
   s_vendorModelMap.set(L"TP-Link", L"tplink");
   // Dell
   s_vendorModelMap.set(L"Dell", L"powerconnect");
   // Ubiquiti
   s_vendorModelMap.set(L"Ubiquiti", L"edgeos");
   s_vendorModelMap.set(L"Ubiquiti, Inc.", L"airos");
   // Eltex
   s_vendorModelMap.set(L"Eltex Ltd.", L"eltex");
   // Moxa
   s_vendorModelMap.set(L"Moxa", L"moxa");
   // Hirschmann
   s_vendorModelMap.set(L"Hirschmann", L"hirschmann");
   // Teltonika
   s_vendorModelMap.set(L"Teltonika", L"routeros");
   // Edgecore
   s_vendorModelMap.set(L"Edgecore", L"edgecos");
}

/**
 * Callback for case-insensitive vendor search
 */
struct VendorSearchData
{
   const TCHAR *vendor;
   const TCHAR *model;
};

static EnumerationCallbackResult VendorSearchCallback(const TCHAR *key, const void *value, void *userData)
{
   VendorSearchData *data = static_cast<VendorSearchData*>(userData);
   if (!wcsicmp(key, data->vendor))
   {
      data->model = static_cast<const TCHAR*>(value);
      return _STOP;
   }
   return _CONTINUE;
}

/**
 * Resolve Oxidized model name from node vendor string.
 * Returns empty string if no mapping found and no default model configured.
 */
std::string ResolveOxidizedModel(const TCHAR *vendor)
{
   if ((vendor != nullptr) && (vendor[0] != 0))
   {
      VendorSearchData data;
      data.vendor = vendor;
      data.model = nullptr;
      s_vendorModelMap.forEach(VendorSearchCallback, &data);

      if (data.model != nullptr)
      {
         char result[64];
         wchar_to_utf8(data.model, -1, result, sizeof(result));
         return std::string(result);
      }
   }

   if (g_oxidizedDefaultModel[0] != 0)
      return std::string(g_oxidizedDefaultModel);

   return std::string();
}

/**
 * Module initialization - parse configuration
 */
static bool OxidizedInitializeModule(Config *config)
{
   NX_CFG_TEMPLATE configTemplate[] =
   {
      { L"BaseURL", CT_MB_STRING, 0, 0, sizeof(g_oxidizedBaseURL), 0, g_oxidizedBaseURL },
      { L"Login", CT_MB_STRING, 0, 0, sizeof(g_oxidizedLogin), 0, g_oxidizedLogin },
      { L"Password", CT_MB_STRING, 0, 0, sizeof(g_oxidizedPassword), 0, g_oxidizedPassword },
      { L"DefaultModel", CT_MB_STRING, 0, 0, sizeof(g_oxidizedDefaultModel), 0, g_oxidizedDefaultModel },
      { L"", CT_END_OF_LIST, 0, 0, 0, 0, nullptr }
   };
   if (!config->parseTemplate(L"OXIDIZED", configTemplate))
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Oxidized integration module initialization failed (cannot parse module configuration)");
      return false;
   }

   // Initialize default vendor-to-model mappings, then override with config
   InitializeVendorModelMap();
   ConfigEntry *vendorMapEntry = config->getEntry(L"/OXIDIZED/VendorModelMap");
   if (vendorMapEntry != nullptr)
   {
      unique_ptr<ObjectArray<ConfigEntry>> entries(vendorMapEntry->getSubEntries(L"*"));
      if (entries != nullptr)
      {
         for (int i = 0; i < entries->size(); i++)
         {
            ConfigEntry *e = entries->get(i);
            s_vendorModelMap.set(e->getName(), e->getValue());
            nxlog_debug_tag(DEBUG_TAG, 5, L"Vendor model mapping: \"%s\" -> \"%s\"", e->getName(), e->getValue());
         }
      }
   }

   if (strncmp(g_oxidizedBaseURL, "https://", 8) && strncmp(g_oxidizedBaseURL, "http://", 7))
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Oxidized integration module initialization failed (invalid base URL)");
      return false;
   }

   // Strip trailing slash from base URL
   size_t l = strlen(g_oxidizedBaseURL);
   if ((l > 0) && (g_oxidizedBaseURL[l - 1] == '/'))
      g_oxidizedBaseURL[l - 1] = 0;

   nxlog_debug_tag(DEBUG_TAG, 5, L"Oxidized base URL set to \"%hs\"", g_oxidizedBaseURL);
   if (g_oxidizedLogin[0] != 0)
      nxlog_debug_tag(DEBUG_TAG, 5, L"Oxidized HTTP Basic Auth enabled (user: %hs)", g_oxidizedLogin);
   if (g_oxidizedDefaultModel[0] != 0)
      nxlog_debug_tag(DEBUG_TAG, 5, L"Oxidized default model set to \"%hs\"", g_oxidizedDefaultModel);

   extern int H_OxidizedNodes(Context *context);
   RouteBuilder("oxidized/nodes").GET(H_OxidizedNodes).build();

   nxlog_debug_tag(DEBUG_TAG, 2, L"Oxidized integration module version " NETXMS_VERSION_STRING L" initialized");
   return true;
}

/**
 * Interface initialization - validate connectivity to Oxidized
 */
static DeviceBackupApiStatus OxidizedInitializeInterface()
{
   json_t *response = OxidizedApiRequest("nodes.json");
   if (response != nullptr)
   {
      size_t nodeCount = json_array_size(response);
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, L"Oxidized server responded successfully (%zu nodes)", nodeCount);
      json_decref(response);
   }
   else
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"Oxidized server is not reachable, will retry on demand");
   }

   return DeviceBackupApiStatus::SUCCESS;
}

/**
 * Network device backup interface
 */
static DeviceBackupInterface s_interface =
{
   .Initialize = OxidizedInitializeInterface,
   .RegisterDevice = OxidizedRegisterDevice,
   .DeleteDevice = OxidizedDeleteDevice,
   .IsDeviceRegistered = OxidizedIsDeviceRegistered,
   .ValidateDeviceRegistration = OxidizedValidateDeviceRegistration,
   .StartJob = OxidizedStartJob,
   .GetLastJobStatus = OxidizedGetLastJobStatus,
   .GetLatestBackup = OxidizedGetLatestBackup,
   .GetBackupList = OxidizedGetBackupList,
   .GetBackupById = OxidizedGetBackupById
};

/**
 * Module registration
 */
extern "C" bool __EXPORT NXM_Register(NXMODULE *module)
{
   module->dwSize = sizeof(NXMODULE);
   wcscpy(module->name, L"OXIDIZED");
   module->pfInitialize = OxidizedInitializeModule;
   module->deviceBackupInterface = &s_interface;
   return true;
}

#ifdef _WIN32

/**
 * DLL Entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
