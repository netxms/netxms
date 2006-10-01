/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: modules.cpp
**
**/

#include "nxcore.h"


//
// List of loaded modules
//

DWORD g_dwNumModules = 0;
NXMODULE *g_pModuleList = NULL;


//
// Starter for module's main thread
//

static THREAD_RESULT THREAD_CALL ModuleThreadStarter(void *pArg)
{
   if (((NXMODULE *)pArg)->pfMain != NULL)
      ((NXMODULE *)pArg)->pfMain();
   return THREAD_OK;
}


//
// Load all registered modules
//

void LoadNetXMSModules(void)
{
   DB_RESULT hResult;
   DWORD i, dwNumRows, dwFlags;
   TCHAR szBuffer[MAX_DB_STRING];
   NXMODULE module;

   hResult = DBSelect(g_hCoreDB, _T("SELECT module_name,exec_name,module_flags FROM modules"));
   if (hResult != NULL)
   {
      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         memset(&module, 0, sizeof(NXMODULE));
         dwFlags = DBGetFieldULong(hResult, i, 2);
         if (!(dwFlags & MODFLAG_DISABLED))
         {
            char szErrorText[256];
            HMODULE hModule;

            hModule = DLOpen(DBGetField(hResult, i, 1, szBuffer, MAX_DB_STRING), szErrorText);
            if (hModule != NULL)
            {
               BOOL (* ModuleInit)(NXMODULE *);

               ModuleInit = (BOOL (*)(NXMODULE *))DLGetSymbolAddr(hModule, _T("NetXMSModuleInit"), szErrorText);
               if (ModuleInit != NULL)
               {
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
                        g_pModuleList[g_dwNumModules].dwFlags = dwFlags;
                        DBGetField(hResult, i, 0, g_pModuleList[g_dwNumModules].szName, MAX_OBJECT_NAME);

                        // Start module's main thread
                        ThreadCreate(ModuleThreadStarter, 0, &g_pModuleList[g_dwNumModules]);

                        WriteLog(MSG_MODULE_LOADED, EVENTLOG_INFORMATION_TYPE,
                                 "s", g_pModuleList[g_dwNumModules].szName);
                        g_dwNumModules++;
                     }
                     else
                     {
                        WriteLog(MSG_MODULE_BAD_MAGIC, EVENTLOG_ERROR_TYPE,
                                 _T("s"), DBGetField(hResult, i, 0, szBuffer, MAX_DB_STRING));
                        DLClose(hModule);
                     }
                  }
                  else
                  {
                     WriteLog(MSG_MODULE_INIT_FAILED, EVENTLOG_ERROR_TYPE,
                              _T("s"), DBGetField(hResult, i, 0, szBuffer, MAX_DB_STRING));
                     DLClose(hModule);
                  }
               }
               else
               {
                  WriteLog(MSG_NO_MODULE_ENTRY_POINT, EVENTLOG_ERROR_TYPE,
                           _T("s"), DBGetField(hResult, i, 0, szBuffer, MAX_DB_STRING));
                  DLClose(hModule);
               }
            }
            else
            {
               WriteLog(MSG_DLOPEN_FAILED, EVENTLOG_ERROR_TYPE, 
                        _T("ss"), DBGetField(hResult, i, 0, szBuffer, MAX_DB_STRING), szErrorText);
            }
         }
      }
      DBFreeResult(hResult);
   }
}
