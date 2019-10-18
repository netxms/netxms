/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Victor Kirhenshtein
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
** File: modules.cpp
**
**/

#include "nxcore.h"

/**
 * Server config
 */
extern Config g_serverConfig;

/**
 * List of loaded modules
 */
TCHAR *g_moduleLoadList = NULL;
UINT32 g_dwNumModules = 0;
NXMODULE *g_pModuleList = NULL;

/**
 * Load module
 */
static bool LoadNetXMSModule(const TCHAR *name)
{
	bool success = false;
   TCHAR szErrorText[256];

#ifdef _WIN32
   HMODULE hModule = DLOpen(name, szErrorText);
#else
   TCHAR fullName[MAX_PATH];

   if (_tcschr(name, _T('/')) == NULL)
   {
      // Assume that module name without path given
      // Try to load it from pkglibdir
      const TCHAR *homeDir = _tgetenv(_T("NETXMS_HOME"));
      if (homeDir != NULL)
      {
         _sntprintf(fullName, MAX_PATH, _T("%s/lib/netxms/%s"), homeDir, name);
      }
      else
      {
         _sntprintf(fullName, MAX_PATH, _T("%s/%s"), PKGLIBDIR, name);
      }
   }
   else
   {
      _tcslcpy(fullName, name, MAX_PATH);
   }
   HMODULE hModule = DLOpen(fullName, szErrorText);
#endif

   if (hModule != NULL)
   {
      bool (* RegisterModule)(NXMODULE *) = (bool (*)(NXMODULE *))DLGetSymbolAddr(hModule, "NXM_Register", szErrorText);
      if (RegisterModule != NULL)
      {
         NXMODULE module;
         memset(&module, 0, sizeof(NXMODULE));
         if (RegisterModule(&module))
         {
            if (module.dwSize == sizeof(NXMODULE))
            {
               if (module.pfInitialize(&g_serverConfig))
               {
                  // Add module to module's list
                  g_pModuleList = (NXMODULE *)realloc(g_pModuleList, sizeof(NXMODULE) * (g_dwNumModules + 1));
                  memcpy(&g_pModuleList[g_dwNumModules], &module, sizeof(NXMODULE));
                  g_pModuleList[g_dwNumModules].hModule = hModule;

                  nxlog_write(NXLOG_INFO, _T("Server module %s loaded successfully"), g_pModuleList[g_dwNumModules].szName);
                  g_dwNumModules++;

                  success = true;
               }
               else
               {
                  nxlog_write(NXLOG_ERROR, _T("Initialization of server module \"%s\" failed"), name);
                  DLClose(hModule);
               }
            }
            else
            {
               nxlog_write(NXLOG_ERROR, _T("Module \"%s\" has invalid magic number - probably it was compiled for different NetXMS server version"), name);
               DLClose(hModule);
            }
         }
         else
         {
            nxlog_write(NXLOG_ERROR, _T("Registartion of server module \"%s\" failed"), name);
            DLClose(hModule);
         }
      }
      else
      {
         nxlog_write(NXLOG_ERROR, _T("Unable to find entry point in server module \"%s\""), name);
         DLClose(hModule);
      }
   }
   else
   {
      nxlog_write(NXLOG_ERROR, _T("Unable to load module \"%s\" (%s)"), name, szErrorText);
   }
   return success;
}

/**
 * Load all registered modules
 */
bool LoadNetXMSModules()
{
   TCHAR *curr, *next, *ptr;
   bool success = true;

	for(curr = g_moduleLoadList; curr != NULL; curr = next)
   {
		next = _tcschr(curr, _T('\n'));
		if (next != NULL)
		{
			*next = 0;
			next++;
		}
		StrStrip(curr);
		if (*curr == 0)
			continue;

		bool mandatory = false;

		// Check for "mandatory" option
		ptr = _tcschr(curr, _T(','));
		if (ptr != NULL)
		{
			*ptr = 0;
			ptr++;
			StrStrip(curr);
			StrStrip(ptr);
			mandatory = (*ptr == _T('1')) || (*ptr == _T('Y')) || (*ptr == _T('y'));
		}

      if (!LoadNetXMSModule(curr))
      {
			if (mandatory)
			{
				success = false;
				break;
			}
      }
   }
	return success;
}

/**
 * Module object data constructor
 */
ModuleData::ModuleData()
{
}

/**
 * Module object data destructor
 */
ModuleData::~ModuleData()
{
}

/**
 * Fill NXCP message with module data
 */
void ModuleData::fillMessage(NXCPMessage *msg, UINT32 baseId)
{
}

/**
 * Save module data to database.
 */
bool ModuleData::saveToDatabase(DB_HANDLE hdb, UINT32 objectId)
{
   return true;
}

/**
 * Save runtime data to database (called only on server shutdown).
 */
bool ModuleData::saveRuntimeData(DB_HANDLE hdb, UINT32 objectId)
{
   return true;
}

/**
 * Delete module data from database.
 */
bool ModuleData::deleteFromDatabase(DB_HANDLE hdb, UINT32 objectId)
{
   return true;
}
