/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
static bool LoadNetXMSModule(const TCHAR *name)
{
	bool success = false;

   TCHAR fullName[MAX_PATH];
#ifdef _WIN32
   _tcslcpy(fullName, name, MAX_PATH);
   size_t len = _tcslen(fullName);
   if ((len < 4) || (_tcsicmp(&fullName[len - 4], _T(".nxm")) && _tcsicmp(&fullName[len - 4], _T(".dll"))))
      _tcslcat(fullName, _T(".nxm"), MAX_PATH);
#else
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

   size_t len = _tcslen(fullName);
   if ((len < 4) || (_tcsicmp(&fullName[len - 4], _T(".nxm")) && _tcsicmp(&fullName[len - _tcslen(SHLIB_SUFFIX)], SHLIB_SUFFIX)))
      _tcslcat(fullName, _T(".nxm"), MAX_PATH);
#endif

   TCHAR errorText[256];
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
                  nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Server module %s loaded successfully"), module.szName);
                  success = true;
               }
               else
               {
                  nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Initialization of server module \"%s\" failed"), name);
                  DLClose(hModule);
               }
            }
            else
            {
               nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Module \"%s\" has invalid magic number - probably it was compiled for different NetXMS server version"), name);
               DLClose(hModule);
            }
         }
         else
         {
            nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Registartion of server module \"%s\" failed"), name);
            DLClose(hModule);
         }
      }
      else
      {
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unable to find entry point in server module \"%s\""), name);
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
	   TCHAR *curr = MemCopyString(g_moduleLoadList.get(i));
		Trim(curr);
		if (*curr == 0)
		{
		   MemFree(curr);
			continue;
		}

		bool mandatory = true;

		// Check for "mandatory" option
		TCHAR *options = _tcschr(curr, _T(','));
		if (options != nullptr)
		{
			*options = 0;
			options++;
			Trim(curr);
			Trim(options);
			if ((*options == _T('0')) || (*options == _T('N')) || (*options == _T('n')))
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
