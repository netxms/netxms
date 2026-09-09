/*
** NetXMS ntopng traffic connector module
** Copyright (C) 2026 Victor Kirhenshtein
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
** File: ntopng.cpp
**/

#include "ntopng.h"
#include <netxms-version.h>

/**
 * Module metadata
 */
DEFINE_MODULE_METADATA("NTOPNG", "Raden Solutions", NETXMS_VERSION_STRING_A, NETXMS_BUILD_TAG_A)

/**
 * Initialize connector
 */
static TrafficConnectorStatus NtopngInitialize()
{
   if (!InitializeLibCURL())
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"cURL initialization failed");
      return TrafficConnectorStatus::CONNECTOR_UNAVAILABLE;
   }
   return TrafficConnectorStatus::SUCCESS;
}

/**
 * Module shutdown handler
 */
static void NtopngShutdown()
{
   NtopngClearCache();
}

/**
 * Credential fields read from the observer's credentials JSON
 */
static const TrafficCredentialField s_credentialFields[] =
{
   { "url", L"URL", L"Base URL of the ntopng instance (for example https://ntopng.example.com:3000)", TrafficCredentialFieldType::STRING, true, nullptr },
   { "token", L"API token", L"ntopng REST API token (admin token required for host alias synchronization)", TrafficCredentialFieldType::PASSWORD, true, nullptr },
   { "timeout", L"Request timeout (seconds)", nullptr, TrafficCredentialFieldType::INTEGER, false, L"30" },
   { "verifyTls", L"Verify TLS certificate", nullptr, TrafficCredentialFieldType::BOOLEAN, false, L"true" },
   { "cacheTtl", L"Cache TTL (seconds)", L"Connector-side cache lifetime for active host lists, host details, and interface counters", TrafficCredentialFieldType::INTEGER, false, L"30" },
   { "pageSize", L"Host list page size", nullptr, TrafficCredentialFieldType::INTEGER, false, L"1000" },
   { "maxHosts", L"Maximum active hosts per observation point", nullptr, TrafficCredentialFieldType::INTEGER, false, L"100000" }
};

/**
 * Traffic connector interface
 */
static TrafficConnectorInterface s_ntopngConnector =
{
   NtopngInitialize,
   NtopngTestConnection,
   NtopngGetCapabilities,
   NtopngDiscoverPoints,
   NtopngGetObserverMetric,
   NtopngGetPointMetric,
   NtopngGetPointTable,
   NtopngGetActiveHosts,
   NtopngGetHostMetric,
   NtopngGetHostTable,
   NtopngGetMetricDefinitions,
   NtopngSyncHostAliases,
   s_credentialFields,
   sizeof(s_credentialFields) / sizeof(s_credentialFields[0])
};

/**
 * Module initialization
 */
static bool NtopngInitializeModule(Config *config)
{
   nxlog_debug_tag(DEBUG_TAG, 2, L"ntopng traffic connector module version " NETXMS_VERSION_STRING L" loaded");
   return true;
}

/**
 * Module entry point
 */
extern "C" bool __EXPORT NXM_Register(NXMODULE *module)
{
   module->dwSize = sizeof(NXMODULE);
   wcscpy(module->name, L"NTOPNG");
   module->pfInitialize = NtopngInitializeModule;
   module->pfShutdown = NtopngShutdown;
   module->trafficConnector = &s_ntopngConnector;
   return true;
}

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
