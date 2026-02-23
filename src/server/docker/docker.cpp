/*
** NetXMS Docker cloud connector module
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
** File: docker.cpp
**/

#include "docker.h"
#include <netxms-version.h>

/**
 * Module metadata
 */
DEFINE_MODULE_METADATA("DOCKER", "Raden Solutions", NETXMS_VERSION_STRING_A, NETXMS_BUILD_TAG_A)

/**
 * Initialize cloud connector
 */
static CloudConnectorStatus DockerInitialize()
{
   nxlog_debug_tag(DEBUG_TAG, 2, L"Docker cloud connector initialized");
   return CloudConnectorStatus::SUCCESS;
}

/**
 * Shutdown cloud connector
 */
static void DockerShutdown()
{
   nxlog_debug_tag(DEBUG_TAG, 2, L"Docker cloud connector shut down");
}

/**
 * Cloud connector interface
 */
static CloudConnectorInterface s_dockerConnector =
{
   DockerInitialize,
   DockerShutdown,
   DockerDiscover,
   DockerGetMetricDefinitions,
   DockerCollect,
   DockerCollectTable,
   DockerQueryState,
   nullptr  // BatchCollect
};

/**
 * Module initialization
 */
static bool DockerInitializeModule(Config *config)
{
   nxlog_debug_tag(DEBUG_TAG, 2, L"Docker cloud connector module version " NETXMS_VERSION_STRING L" loaded");
   return true;
}

/**
 * Module registration
 */
extern "C" bool __EXPORT NXM_Register(NXMODULE *module)
{
   module->dwSize = sizeof(NXMODULE);
   wcscpy(module->name, L"DOCKER");
   module->pfInitialize = DockerInitializeModule;
   module->cloudConnector = &s_dockerConnector;
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
