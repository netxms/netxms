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


//
// Load DLL/shared library
//

HMODULE LIBNETXMS_EXPORTABLE DLOpen(char *szLibName, char *pszErrorText)
{
   HMODULE hModule;

#ifdef _WIN32
   hModule = LoadLibrary(szLibName);
   if (hModule == NULL)
      GetSystemErrorText(GetLastError(), pszErrorText, 255);
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
#ifdef _WIN32
   FreeLibrary(hModule);
#else    /* _WIN32 */
   dlclose(hModule);
#endif
}


//
// Get symbol address from library
//

void LIBNETXMS_EXPORTABLE *DLGetSymbolAddr(HMODULE hModule, char *szSymbol, char *pszErrorText)
{
   void *pAddr;

#ifdef _WIN32
   pAddr = GetProcAddress(hModule, szSymbol);
   if (pAddr == NULL)
      GetSystemErrorText(GetLastError(), pszErrorText, 255);
#else    /* _WIN32 */
   pAddr = dlsym(hModule, szSymbol);
   if (pAddr == NULL)
      strncpy(pszErrorText, dlerror(), 255);
#endif
   return pAddr;
}
