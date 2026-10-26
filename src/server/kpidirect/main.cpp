/*
** NetXMS - Network Management System
** Direct KPI computation provider
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
** File: main.cpp
**
**/

#include "kpidirect.h"
#include <nms_core.h>
#include <nxmodule.h>
#include <netxms-version.h>

/**
 * Module metadata
 */
DEFINE_MODULE_METADATA("KPIDIRECT", "Raden Solutions", NETXMS_VERSION_STRING_A, NETXMS_BUILD_TAG_A)

/**
 * Direct computation provider: arithmetic only, never estimates
 */
class KpiDirectProvider : public ComputationProvider
{
public:
   const wchar_t *id() const override { return KPIDIRECT_METHOD_ID; }
   const wchar_t *version() const override { return KPIDIRECT_METHOD_VERSION; }
   const wchar_t *displayName() const override { return KPIDIRECT_DISPLAY_NAME; }
   bool isCommercial() const override { return false; }
   double coverageLevel() const override { return 0; }
   int32_t inputMarginSeconds() const override { return KPIDIRECT_INPUT_MARGIN; }
   void compute(const ComputationInput& input, ComputationOutput *output) override { ComposeComponents(input, output); }
};

/**
 * Initialize module
 */
static bool InitModule(Config *config)
{
   return RegisterComputationProvider(make_shared<KpiDirectProvider>());
}

/**
 * Module entry point
 */
extern "C" bool __EXPORT NXM_Register(NXMODULE *module, Config *config)
{
   module->dwSize = sizeof(NXMODULE);
   wcscpy(module->name, L"KPIDIRECT");
   module->pfInitialize = InitModule;
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
