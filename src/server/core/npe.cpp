/*
** NetXMS - Network Management System
** Copyright (C) 2003-2016 Victor Kirhenshtein
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
** File: npe.cpp
**
**/

#include "nxcore.h"

/**
 * Base class constructor
 */
PredictionEngine::PredictionEngine()
{
}

/**
 * Base class destructor
 */
PredictionEngine::~PredictionEngine()
{
}

/**
 * Default initialization method - always returns true
 */
bool PredictionEngine::initialize(TCHAR *errorMessage)
{
   errorMessage[0] = 0;
   return true;
}

/**
 * Prediction engine registry
 */
static StringObjectMap<PredictionEngine> s_engines(true);

/**
 * Register prediction engines on startup
 */
void RegisterPredictionEngines()
{
   ENUMERATE_MODULES(pfGetPredictionEngines)
   {
      ObjectArray<PredictionEngine> *engines = g_pModuleList[__i].pfGetPredictionEngines();
      engines->setOwner(false);
      for(int i = 0; i < engines->size(); i++)
      {
         PredictionEngine *e = engines->get(i);
         TCHAR errorMessage[MAX_NPE_ERROR_MESSAGE_LEN];
         if (e->initialize(errorMessage))
         {
            s_engines.set(e->getName(), e);
            nxlog_write(MSG_NPE_REGISTERED, NXLOG_INFO, "ss", e->getName(), e->getVersion());
         }
         else
         {
            nxlog_write(MSG_NPE_INIT_FAILED, NXLOG_ERROR, "ss", e->getName(), e->getVersion());
            delete e;
         }
      }
      delete engines;
   }
}

/**
 * Callback for ShowPredictionEngines
 */
static EnumerationCallbackResult ShowEngineDetails(const TCHAR *key, const void *value, void *data)
{
   const PredictionEngine *p = (const PredictionEngine *)value;
   ConsolePrintf((CONSOLE_CTX)data, _T("%-16s | %-12s | %s\n"), key, p->getVersion(), p->getVendor());
   return _CONTINUE;
}

/**
 * Show registered prediction engines on console
 */
void ShowPredictionEngines(CONSOLE_CTX console)
{
   if (s_engines.size() == 0)
   {
      ConsolePrintf(console, _T("No prediction engines registered\n"));
      return;
   }

   ConsolePrintf(console, _T("Name             | Version      | Vendor\n"));
   ConsolePrintf(console, _T("-----------------+--------------+------------------------------\n"));
   s_engines.forEach(ShowEngineDetails, console);
}

/**
 * Find prediction engine by name
 */
PredictionEngine NXCORE_EXPORTABLE *FindPredictionEngine(const TCHAR *name)
{
   return s_engines.get(name);
}
