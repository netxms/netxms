/*
** NetXMS - Network Management System
** Copyright (C) 2024-2026 Raden Solutions
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
** File: main.cpp
**
**/

#include "otlp.h"
#include <netxms-version.h>

/**
 * Module metadata
 */
DEFINE_MODULE_METADATA("OTLP", "Raden Solutions", NETXMS_VERSION_STRING_A, NETXMS_BUILD_TAG_A)

/**
 * Initialize module
 */
static bool InitModule(Config *config)
{
   InitMatchingEngine();
   InitInstanceDiscovery();

   RouteBuilder("otlp-backend/v1/metrics")
      .POST(H_OtlpMetrics)
      .acceptProtobuf()
      .build();

   nxlog_debug_tag(DEBUG_TAG_OTLP, 1, L"OTLP receiver module initialized");
   return true;
}

/**
 * Shutdown module
 */
static void ShutdownModule()
{
   ShutdownCounterState();
   ShutdownInstanceDiscovery();
   ShutdownMatchingEngine();
   nxlog_debug_tag(DEBUG_TAG_OTLP, 1, L"OTLP receiver module shut down");
}

/**
 * Module entry point
 */
extern "C" bool __EXPORT NXM_Register(NXMODULE *module, Config *config)
{
   module->dwSize = sizeof(NXMODULE);
   wcscpy(module->name, L"OTLP");
   module->pfInitialize = InitModule;
   module->pfShutdown = ShutdownModule;
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
