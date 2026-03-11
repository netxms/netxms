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
#include <nms_objects.h>
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

int H_OxidizedNodes(Context *context);

/**
 * Global configuration variables
 */
char g_oxidizedBaseURL[256] = "http://127.0.0.1:8888";
char g_oxidizedLogin[128] = "";
char g_oxidizedPassword[128] = "";
char g_oxidizedDefaultModel[64] = "";

/**
 * NDD driver name to Oxidized model mapping (case-insensitive, checked first)
 */
static StringMap s_driverModelMap;

/**
 * sysDescr substring to Oxidized model mapping (case-insensitive, checked after driver)
 */
struct SysDescrMapping
{
   TCHAR pattern[128];
   TCHAR model[64];
};
static ObjectArray<SysDescrMapping> s_sysDescrMappings(16, 16, Ownership::True);

/**
 * Vendor to Oxidized model mapping (case-insensitive, used as fallback)
 */
static StringMap s_vendorModelMap;

/**
 * Initialize default driver-to-model mappings
 */
static void InitializeDriverModelMap()
{
   // Cisco
   s_driverModelMap.set(L"CISCO-GENERIC", L"ios");
   s_driverModelMap.set(L"CATALYST-GENERIC", L"ios");
   s_driverModelMap.set(L"CATALYST-2900XL", L"ios");
   s_driverModelMap.set(L"CISCO-NEXUS", L"nxos");
   s_driverModelMap.set(L"CISCO-SB", L"ciscosmb");
   s_driverModelMap.set(L"CISCO-ESW", L"ios");
   s_driverModelMap.set(L"CISCO-WLC", L"aireos");
   // Juniper
   s_driverModelMap.set(L"JUNIPER", L"junos");
   s_driverModelMap.set(L"NETSCREEN", L"screenos");
   // MikroTik
   s_driverModelMap.set(L"MIKROTIK", L"routeros");
   // Huawei
   s_driverModelMap.set(L"HUAWEI-SW", L"vrp");
   s_driverModelMap.set(L"OPTIX", L"vrp");
   // Fortinet
   s_driverModelMap.set(L"FORTIGATE", L"fortios");
   // HPE / Aruba
   s_driverModelMap.set(L"ARUBA-SW", L"aoscx");
   s_driverModelMap.set(L"PROCURVE", L"procurve");
   s_driverModelMap.set(L"H3C", L"comware");
   s_driverModelMap.set(L"HPSW", L"procurve");
   // Extreme
   s_driverModelMap.set(L"EXTREME", L"xos");
   // Dell
   s_driverModelMap.set(L"DELL-PWC", L"powerconnect");
   // Ubiquiti
   s_driverModelMap.set(L"UBNT-AIRMAX", L"airos");
   s_driverModelMap.set(L"UBNT-EDGESW", L"edgeswitch");
   // Hirschmann
   s_driverModelMap.set(L"HIRSCHMANN-HIOS", L"hirschmann");
   s_driverModelMap.set(L"HIRSCHMANN-CLASSIC", L"hirschmann");
   // D-Link
   s_driverModelMap.set(L"DLINK", L"dlink");
   // TP-Link
   s_driverModelMap.set(L"TPLINK", L"tplink");
   // Eltex
   s_driverModelMap.set(L"ELTEX", L"eltex");
   // Edgecore
   s_driverModelMap.set(L"EDGECORE-ESW", L"edgecos");
   // Teltonika
   s_driverModelMap.set(L"TELTONIKA", L"openwrt");
}

/**
 * Add sysDescr mapping entry
 */
static void AddSysDescrMapping(const TCHAR *pattern, const TCHAR *model)
{
   auto *m = new SysDescrMapping();
   _tcslcpy(m->pattern, pattern, 128);
   _tcslcpy(m->model, model, 64);
   s_sysDescrMappings.add(m);
}

/**
 * Initialize default sysDescr-to-model mappings
 */
static void InitializeSysDescrModelMap()
{
   // Cisco OS variants
   AddSysDescrMapping(L"Adaptive Security Appliance", L"asa");
   AddSysDescrMapping(L"IOS-XR", L"iosxr");
   AddSysDescrMapping(L"IOS XR", L"iosxr");
   AddSysDescrMapping(L"NX-OS", L"nxos");
   // Dell OS variants
   AddSysDescrMapping(L"Dell EMC Networking OS10", L"os10");
   AddSysDescrMapping(L"OS10 Enterprise", L"os10");
   AddSysDescrMapping(L"Force10", L"ftos");
   AddSysDescrMapping(L"FTOS", L"ftos");
}

/**
 * Initialize default vendor-to-model mappings (fallback for devices using GENERIC driver)
 */
static void InitializeVendorModelMap()
{
   s_vendorModelMap.set(L"Cisco", L"ios");
   s_vendorModelMap.set(L"Cisco Systems Inc.", L"ios");
   s_vendorModelMap.set(L"Juniper Networks", L"junos");
   s_vendorModelMap.set(L"MikroTik", L"routeros");
   s_vendorModelMap.set(L"Huawei", L"vrp");
   s_vendorModelMap.set(L"Fortinet", L"fortios");
   s_vendorModelMap.set(L"HPE Aruba Networking", L"arubaos");
   s_vendorModelMap.set(L"Hewlett Packard Enterprise", L"procurve");
   s_vendorModelMap.set(L"Extreme Networks", L"xos");
   s_vendorModelMap.set(L"D-Link", L"dlink");
   s_vendorModelMap.set(L"TP-Link", L"tplink");
   s_vendorModelMap.set(L"Dell", L"powerconnect");
   s_vendorModelMap.set(L"Ubiquiti", L"edgeos");
   s_vendorModelMap.set(L"Ubiquiti, Inc.", L"airos");
   s_vendorModelMap.set(L"Eltex Ltd.", L"eltex");
   s_vendorModelMap.set(L"Moxa", L"moxa");
   s_vendorModelMap.set(L"Hirschmann", L"hirschmann");
   s_vendorModelMap.set(L"Teltonika", L"openwrt");
   s_vendorModelMap.set(L"Edgecore", L"edgecos");
}

/**
 * Callback for case-insensitive map search
 */
struct MapSearchData
{
   const TCHAR *key;
   const TCHAR *value;
};

static EnumerationCallbackResult CaseInsensitiveSearchCallback(const TCHAR *key, const void *value, void *userData)
{
   MapSearchData *data = static_cast<MapSearchData*>(userData);
   if (!wcsicmp(key, data->key))
   {
      data->value = static_cast<const TCHAR*>(value);
      return _STOP;
   }
   return _CONTINUE;
}

/**
 * Case-insensitive lookup in a StringMap, returns nullptr if not found
 */
static const TCHAR *CaseInsensitiveLookup(StringMap& map, const TCHAR *key)
{
   if ((key == nullptr) || (key[0] == 0))
      return nullptr;

   MapSearchData data;
   data.key = key;
   data.value = nullptr;
   map.forEach(CaseInsensitiveSearchCallback, &data);
   return data.value;
}

/**
 * Resolve Oxidized model name from node properties.
 * Priority: user override (oxidized.model custom attribute) > NDD driver > sysDescr > vendor > default.
 * Returns empty string if no mapping found and no default model configured.
 */
std::string ResolveOxidizedModel(const Node *node)
{
   // Tier 0: user-set custom attribute override
   SharedString userModel = node->getCustomAttribute(L"oxidized.model");
   if (!userModel.isEmpty())
   {
      char result[64];
      wchar_to_utf8(userModel.cstr(), -1, result, sizeof(result));
      nxlog_debug_tag(DEBUG_TAG, 5, L"ResolveOxidizedModel(%s [%u]): using user-set model \"%hs\" from custom attribute",
         node->getName(), node->getId(), result);
      return std::string(result);
   }

   // Tier 1: NDD driver name (skip GENERIC - it matches everything and is not useful for mapping)
   const TCHAR *driverName = node->getDriverName();
   if ((driverName != nullptr) && wcsicmp(driverName, L"GENERIC"))
   {
      const TCHAR *model = CaseInsensitiveLookup(s_driverModelMap, driverName);
      if (model != nullptr)
      {
         char result[64];
         wchar_to_utf8(model, -1, result, sizeof(result));
         nxlog_debug_tag(DEBUG_TAG, 5, L"ResolveOxidizedModel(%s [%u]): resolved model \"%hs\" via driver \"%s\"",
            node->getName(), node->getId(), result, driverName);
         return std::string(result);
      }
   }

   // Tier 2: sysDescr substring match
   const TCHAR *sysDescr = node->getSysDescription();
   if ((sysDescr != nullptr) && (sysDescr[0] != 0))
   {
      for (int i = 0; i < s_sysDescrMappings.size(); i++)
      {
         SysDescrMapping *m = s_sysDescrMappings.get(i);
         if (wcsistr(sysDescr, m->pattern) != nullptr)
         {
            char result[64];
            wchar_to_utf8(m->model, -1, result, sizeof(result));
            nxlog_debug_tag(DEBUG_TAG, 5, L"ResolveOxidizedModel(%s [%u]): resolved model \"%hs\" via sysDescr pattern \"%s\"",
               node->getName(), node->getId(), result, m->pattern);
            return std::string(result);
         }
      }
   }

   // Tier 3: vendor name
   SharedString vendor = node->getVendor();
   const TCHAR *model = CaseInsensitiveLookup(s_vendorModelMap, vendor.cstr());
   if (model != nullptr)
   {
      char result[64];
      wchar_to_utf8(model, -1, result, sizeof(result));
      nxlog_debug_tag(DEBUG_TAG, 5, L"ResolveOxidizedModel(%s [%u]): resolved model \"%hs\" via vendor \"%s\"",
         node->getName(), node->getId(), result, vendor.cstr());
      return std::string(result);
   }

   // Tier 4: default model
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

   // Initialize built-in mappings
   InitializeDriverModelMap();
   InitializeSysDescrModelMap();
   InitializeVendorModelMap();

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

   RouteBuilder("oxidized/nodes")
      .GET(H_OxidizedNodes)
      .scope("oxydized")
      .build();

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
