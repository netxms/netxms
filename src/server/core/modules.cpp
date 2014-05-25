/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
 * List of loaded modules
 */
TCHAR *g_pszModLoadList = NULL;
UINT32 g_dwNumModules = 0;
NXMODULE *g_pModuleList = NULL;

/**
 * Load module
 */
static bool LoadNetXMSModule(const TCHAR *name)
{
	bool success = false;
   TCHAR szErrorText[256];

#if !defined(_WIN32) && !defined(_NETWARE)
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
      nx_strncpy(fullName, name, MAX_PATH);
   }
   HMODULE hModule = DLOpen(fullName, szErrorText);
#else
   HMODULE hModule = DLOpen(name, szErrorText);
#endif

   if (hModule != NULL)
   {
      BOOL (* ModuleInit)(NXMODULE *);

      ModuleInit = (BOOL (*)(NXMODULE *))DLGetSymbolAddr(hModule, "NetXMSModuleInit", szErrorText);
      if (ModuleInit != NULL)
      {
         NXMODULE module;
         memset(&module, 0, sizeof(NXMODULE));
         if (ModuleInit(&module))
         {
            if (module.dwSize == sizeof(NXMODULE))
            {
               // Add module to module's list
               g_pModuleList = (NXMODULE *)realloc(g_pModuleList, 
                                                   sizeof(NXMODULE) * (g_dwNumModules + 1));
               memcpy(&g_pModuleList[g_dwNumModules], &module, sizeof(NXMODULE));
               g_pModuleList[g_dwNumModules].hModule = hModule;

               nxlog_write(MSG_MODULE_LOADED, EVENTLOG_INFORMATION_TYPE, "s", g_pModuleList[g_dwNumModules].szName);
               g_dwNumModules++;

               success = true;
            }
            else
            {
               nxlog_write(MSG_MODULE_BAD_MAGIC, EVENTLOG_ERROR_TYPE, "s", name);
               DLClose(hModule);
            }
         }
         else
         {
            nxlog_write(MSG_MODULE_INIT_FAILED, EVENTLOG_ERROR_TYPE, "s", name);
            DLClose(hModule);
         }
      }
      else
      {
         nxlog_write(MSG_NO_MODULE_ENTRY_POINT, EVENTLOG_ERROR_TYPE, "s", name);
         DLClose(hModule);
      }
   }
   else
   {
      nxlog_write(MSG_DLOPEN_FAILED, EVENTLOG_ERROR_TYPE, "ss", name, szErrorText);
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

	for(curr = g_pszModLoadList; curr != NULL; curr = next)
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
