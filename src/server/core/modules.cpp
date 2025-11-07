/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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

#define DEBUG_TAG _T("modules")

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
static bool LoadNetXMSModule(const wchar_t *name)
{
	bool success = false;

	wchar_t fullName[MAX_PATH];
#ifdef _WIN32
   wcslcpy(fullName, name, MAX_PATH);
   size_t len = wcslen(fullName);
   if ((len < 4) || (wcsicmp(&fullName[len - 4], L".nxm") && wcsicmp(&fullName[len - 4], L".dll")))
      wcslcat(fullName, L".nxm", MAX_PATH);
#else
   if (wcschr(name, L'/') == nullptr)
   {
      // Assume that module name without path given
      // Try to load it from pkglibdir
      wchar_t libdir[MAX_PATH];
      GetNetXMSDirectory(nxDirLib, libdir);
      _sntprintf(fullName, MAX_PATH, L"%s/%s", libdir, name);
   }
   else
   {
      wcslcpy(fullName, name, MAX_PATH);
   }

   size_t len = wcslen(fullName);
   if ((len < 4) || (wcsicmp(&fullName[len - 4], L".nxm") && wcsicmp(&fullName[len - wcslen(SHLIB_SUFFIX)], SHLIB_SUFFIX)))
      wcslcat(fullName, L".nxm", MAX_PATH);
#endif

   wchar_t errorText[256];
   HMODULE hModule = DLOpen(fullName, errorText);
   if (hModule != nullptr)
   {
      auto RegisterModule = DLGetFunctionAddr<bool (*)(NXMODULE*)>(hModule, "NXM_Register", errorText);
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
                  module.metadata = static_cast<NXMODULE_METADATA*>(DLGetSymbolAddr(hModule, "NXM_metadata", errorText));
                  g_moduleList.add(module);
                  nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, L"Server module %s loaded successfully", module.name);
                  success = true;
               }
               else
               {
                  nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Initialization of server module \"%s\" failed", name);
                  DLClose(hModule);
               }
            }
            else
            {
               nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Module \"%s\" has invalid magic number - probably it was compiled for different NetXMS server version", name);
               DLClose(hModule);
            }
         }
         else
         {
            nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Registration of server module \"%s\" failed", name);
            DLClose(hModule);
         }
      }
      else
      {
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Unable to find entry point in server module \"%s\"", name);
         DLClose(hModule);
      }
   }
   else
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unable to load module \"%s\" (%s)"), name, errorText);
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
      wchar_t *curr = MemCopyStringW(g_moduleLoadList.get(i));
      TrimW(curr);
      if (*curr == 0)
		{
		   MemFree(curr);
			continue;
		}

		bool mandatory = true;

		// Check for "mandatory" option
		wchar_t *options = wcschr(curr, L',');
		if (options != nullptr)
		{
			*options = 0;
			options++;
			TrimW(curr);
			TrimW(options);
			if ((*options == L'0') || (*options == L'N') || (*options == L'n'))
			   mandatory = false;
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
void ModuleData::fillMessage(NXCPMessage *msg, uint32_t baseId) const
{
}

/**
 * Save module data to database.
 */
bool ModuleData::saveToDatabase(DB_HANDLE hdb, uint32_t objectId)
{
   return true;
}

/**
 * Save runtime data to database (called only on server shutdown).
 */
bool ModuleData::saveRuntimeData(DB_HANDLE hdb, uint32_t objectId)
{
   return true;
}

/**
 * Delete module data from database.
 */
bool ModuleData::deleteFromDatabase(DB_HANDLE hdb, uint32_t objectId)
{
   return true;
}

/**
 * Create JSON representation
 */
json_t *ModuleData::toJson() const
{
   return json_object();
}
