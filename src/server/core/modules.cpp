/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
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
StringList g_moduleLoadList;
StructArray<NXMODULE> g_moduleList(0, 8);

/**
 * Load module
 */
static bool LoadNetXMSModule(const TCHAR *name)
{
	bool success = false;
   TCHAR errorText[256];

#ifdef _WIN32
   HMODULE hModule = DLOpen(name, errorText);
#else
   TCHAR fullName[MAX_PATH];

   if (_tcschr(name, _T('/')) == nullptr)
   {
      // Assume that module name without path given
      // Try to load it from pkglibdir
      TCHAR libdir[MAX_PATH];
      GetNetXMSDirectory(nxDirLib, libdir);
      _sntprintf(fullName, MAX_PATH, _T("%s/%s"), libdir, name);
   }
   else
   {
      _tcslcpy(fullName, name, MAX_PATH);
   }
   HMODULE hModule = DLOpen(fullName, errorText);
#endif

   if (hModule != nullptr)
   {
      bool (*RegisterModule)(NXMODULE *) = (bool (*)(NXMODULE *))DLGetSymbolAddr(hModule, "NXM_Register", errorText);
      if (RegisterModule != nullptr)
      {
         NXMODULE module;
         memset(&module, 0, sizeof(NXMODULE));
         if (RegisterModule(&module))
         {
            if (module.dwSize == sizeof(NXMODULE))
            {
               if ((module.pfInitialize == nullptr) || module.pfInitialize(&g_serverConfig))
               {
                  // Add module to module's list
                  module.hModule = hModule;
                  module.metadata = (NXMODULE_METADATA*)DLGetSymbolAddr(hModule, "NXM_metadata", errorText);
                  g_moduleList.add(module);
                  nxlog_write(NXLOG_INFO, _T("Server module %s loaded successfully"), module.szName);
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
      nxlog_write(NXLOG_ERROR, _T("Unable to load module \"%s\" (%s)"), name, errorText);
   }
   return success;
}

/**
 * Load all registered modules
 */
bool LoadNetXMSModules()
{
   bool success = true;
	for(int i = 0; i < g_moduleLoadList.size(); i++)
   {
	   TCHAR *curr = MemCopyString(g_moduleLoadList.get(i));
		StrStrip(curr);
		if (*curr == 0)
		{
		   MemFree(curr);
			continue;
		}

		bool mandatory = false;

		// Check for "mandatory" option
		TCHAR *options = _tcschr(curr, _T(','));
		if (options != nullptr)
		{
			*options = 0;
			options++;
			StrStrip(curr);
			StrStrip(options);
			mandatory = (*options == _T('1')) || (*options == _T('Y')) || (*options == _T('y'));
		}

      if (!LoadNetXMSModule(curr))
      {
			if (mandatory)
			{
            MemFree(curr);
				success = false;
				break;
			}
      }
      MemFree(curr);
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
