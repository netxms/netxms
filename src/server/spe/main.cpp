/*
** NetXMS - Network Management System
** Copyright (C) 2003-2018 Raden Solutions
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

#include "spe.h"
#include <netxms-version.h>

/**
 * Module metadata
 */
DEFINE_MODULE_METADATA("SPE", "Raden Solutions", NETXMS_VERSION_STRING_A, NETXMS_BUILD_TAG_A)

/**
 * Get prediction engines
 */
static ObjectArray<PredictionEngine> *GetPredictionEngines()
{
   ObjectArray<PredictionEngine> *engines = new ObjectArray<PredictionEngine>();
   engines->add(new TimeSeriesRegressionEngine());
   return engines;
}

/**
 * Module entry point
 */
extern "C" bool __EXPORT NXM_Register(NXMODULE *module, Config *config)
{
   module->dwSize = sizeof(NXMODULE);
   _tcscpy(module->name, _T("SPE"));
   module->pfGetPredictionEngines = GetPredictionEngines;
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
