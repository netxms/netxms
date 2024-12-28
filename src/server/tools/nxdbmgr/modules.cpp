/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2024 Victor Kirhenshtein
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

#include "nxdbmgr.h"

#define MODULE_NXDBMGR_EXTENSION
#include <nxmodule.h>

/**
 * Module information structure
 */
struct Module
{
   HMODULE handle;
   TCHAR name[MAX_PATH];
   bool (*CheckDB)();
   bool (*CheckDBVersion)();
   bool (*UpgradeDB)();
   const TCHAR* const * (*GetTables)();
   const TCHAR* (*GetSchemaPrefix)();
};

/**
 * Loaded modules
 */
static ObjectArray<Module> s_modules(8, 8, Ownership::True);

/**
 * Load module
 */
static bool LoadServerModule(const wchar_t *name, bool mandatory, bool quiet)
{
   bool success = false;

   wchar_t fullName[MAX_PATH];
#ifdef _WIN32
   wcslcpy(fullName, name, MAX_PATH);
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
      wcslcpy(fullName, name, MAX_PATH);
   }

   size_t len = _tcslen(fullName);
   if ((len < 4) || (_tcsicmp(&fullName[len - 4], _T(".nxm")) && _tcsicmp(&fullName[len - _tcslen(SHLIB_SUFFIX)], SHLIB_SUFFIX)))
      _tcslcat(fullName, _T(".nxm"), MAX_PATH);
#endif

   wchar_t errorText[256];
   HMODULE hModule = DLOpen(fullName, errorText);
   if (hModule != nullptr)
   {
      auto m = new Module();
      m->handle = hModule;
      auto metadata = static_cast<NXMODULE_METADATA*>(DLGetSymbolAddr(hModule, "NXM_metadata", errorText));
      if (metadata != nullptr)
      {
         mb_to_wchar(metadata->name, -1, m->name, MAX_PATH);
      }
      else
      {
         wcslcpy(m->name, name, MAX_PATH);
      }
      m->CheckDB = (bool (*)())DLGetSymbolAddr(hModule, "NXM_CheckDB", errorText);
      m->CheckDBVersion = (bool (*)())DLGetSymbolAddr(hModule, "NXM_CheckDBVersion", errorText);
      m->UpgradeDB = (bool (*)())DLGetSymbolAddr(hModule, "NXM_UpgradeDB", errorText);
      m->GetTables = (const TCHAR* const * (*)())DLGetSymbolAddr(hModule, "NXM_GetTables", errorText);
      m->GetSchemaPrefix = (const TCHAR* (*)())DLGetSymbolAddr(hModule, "NXM_GetSchemaPrefix", errorText);
      if ((m->CheckDB != nullptr) || (m->UpgradeDB != nullptr) || (m->GetTables != nullptr) || (m->GetSchemaPrefix != nullptr))
      {
         if (!quiet)
            WriteToTerminalEx(_T("\x1b[32;1mINFO:\x1b[0m Server module \x1b[1m%s\x1b[0m loaded\n"), m->name);
         s_modules.add(m);
      }
      else
      {
         if (!quiet)
            WriteToTerminalEx(_T("\x1b[32;1mINFO:\x1b[0m Server module \x1b[1m%s\x1b[0m skipped\n"), m->name);
         DLClose(m->handle);
         delete m;
      }
      success = true;
   }
   else
   {
      if (mandatory || !quiet)
      {
         WriteToTerminalEx(_T("\x1b[%s:\x1b[0m Cannot load server module \x1b[1m%s\x1b[0m (%s)\n"),
                  mandatory ? _T("31;1mERROR") : _T("33;1mWARNING"), name, errorText);
      }
   }
   return success;
}

/**
 * Load all registered modules
 */
bool LoadServerModules(wchar_t *moduleLoadList, bool quiet)
{
   wchar_t *curr, *next, *ptr;
   bool success = true;

   for(curr = moduleLoadList; curr != nullptr; curr = next)
   {
      next = _tcschr(curr, _T('\n'));
      if (next != nullptr)
      {
         *next = 0;
         next++;
      }
      Trim(curr);
      if (*curr == 0)
         continue;

      bool mandatory = true;

      // Check for "mandatory" option
      ptr = _tcschr(curr, _T(','));
      if (ptr != nullptr)
      {
         *ptr = 0;
         ptr++;
         Trim(curr);
         Trim(ptr);
         mandatory = (*ptr == L'1') || (*ptr == L'Y') || (*ptr == L'y');
      }

      if (!LoadServerModule(curr, mandatory, quiet))
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
 * Enumerate all tables in all modules
 */
bool EnumerateModuleTables(bool (*handler)(const wchar_t*, void*), void *userData)
{
   for(int i = 0; i < s_modules.size(); i++)
   {
      Module *m = s_modules.get(i);
      if (m->GetTables == nullptr)
         continue;
      const wchar_t * const *tables = m->GetTables();
      if (tables == nullptr)
         continue;
      for(int j = 0; tables[j] != nullptr; j++)
      {
         if (!handler(tables[j], userData))
            return false;
      }
   }
   return true;
}

/**
 * Enumerate all module schemas
 */
bool EnumerateModuleSchemas(bool (*handler)(const wchar_t*, void*), void *userData)
{
   for(int i = 0; i < s_modules.size(); i++)
   {
      Module *m = s_modules.get(i);
      if (m->GetSchemaPrefix == nullptr)
         continue;
      const wchar_t *schema = m->GetSchemaPrefix();
      if (schema == nullptr)
         continue;
      if (!handler(schema, userData))
         return false;
   }
   return true;
}

/**
 * Upgrade all module schemas
 */
bool UpgradeModuleSchemas()
{
   for(int i = 0; i < s_modules.size(); i++)
   {
      Module *m = s_modules.get(i);
      if (m->UpgradeDB == nullptr)
         continue;
      if (!m->UpgradeDB())
         return false;
   }
   return true;
}

/**
 * Check all module schemas
 */
bool CheckModuleSchemas()
{
   for(int i = 0; i < s_modules.size(); i++)
   {
      Module *m = s_modules.get(i);
      if (m->CheckDB == nullptr)
         continue;
      if (!m->CheckDB())
         return false;
   }
   return true;
}

/**
 * Check versions of all module schemas
 */
bool CheckModuleSchemaVersions()
{
   for(int i = 0; i < s_modules.size(); i++)
   {
      Module *m = s_modules.get(i);
      if (m->CheckDBVersion == nullptr)
         continue;
      if (!m->CheckDBVersion())
         return false;
   }
   return true;
}
