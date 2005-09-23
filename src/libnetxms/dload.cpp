/* 
** NetXMS - Network Management System
** Utility Library
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
** $module: dload.cpp
**
**/

#include "libnetxms.h"

#if defined(_NETWARE)
#include <netware.h>
#elif !defined(_WIN32)
#include <dlfcn.h>
#endif


//
// Load DLL/shared library
//

HMODULE LIBNETXMS_EXPORTABLE DLOpen(TCHAR *szLibName, TCHAR *pszErrorText)
{
   HMODULE hModule;

#if defined(_WIN32)
   hModule = LoadLibrary(szLibName);
   if (hModule == NULL)
      GetSystemErrorText(GetLastError(), pszErrorText, 255);
#elif defined(_NETWARE)
   TCHAR szBuffer[MAX_PATH + 4];

   _tcsncpy(&szBuffer[4], szLibName, MAX_PATH);
   if (LoadModule(getscreenhandle(), &szBuffer[4], LO_RETURN_HANDLE) == 0)
   {
      hModule = *((HMODULE *)szBuffer);
   }
   else
   {
      hModule = NULL;
      *pszErrorText = 0;
   }
#else    /* _WIN32 */
   hModule = dlopen(szLibName, RTLD_NOW | RTLD_GLOBAL);
   if (hModule == NULL)
      strncpy(pszErrorText, dlerror(), 255);
#endif
   return hModule;
}


//
// Unload DLL/shared library
//

void LIBNETXMS_EXPORTABLE DLClose(HMODULE hModule)
{
   if (hModule != NULL)
   {
#if defined(_WIN32)
      FreeLibrary(hModule);
#elif defined(_NETWARE)
      char szName[MAX_PATH], szTemp[1024];

      GetNLMNames(hModule, szName, szTemp);
      szName[szName[0] + 1] = 0;
      UnloadModule(getscreenhandle(), &szName[1]);
#else
      dlclose(hModule);
#endif
   }
}


//
// Get symbol address from library
//

void LIBNETXMS_EXPORTABLE *DLGetSymbolAddr(HMODULE hModule,
										   TCHAR *pszSymbol,
										   TCHAR *pszErrorText)
{
   void *pAddr;

#if defined(_WIN32)
#if !defined(UNDER_CE) && defined(UNICODE)
   char szBuffer[256];

   WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                       pszSymbol, -1, szBuffer, 256, NULL, NULL);
   pAddr = GetProcAddress(hModule, szBuffer);
#else
   pAddr = GetProcAddress(hModule, pszSymbol);
#endif
   if (pAddr == NULL)
      GetSystemErrorText(GetLastError(), pszErrorText, 255);
#elif defined(_NETWARE)
   pAddr = ImportPublicObject(hModule, pszSymbol);
   *pszErrorText = 0;
#else    /* _WIN32 */
   pAddr = dlsym(hModule, pszSymbol);
   if (pAddr == NULL)
      _tcsncpy(pszErrorText, dlerror(), 255);
#endif
   return pAddr;
}
