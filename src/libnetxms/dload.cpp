/** 
** NetXMS - Network Management System
** Utility Library
** Copyright (C) 2003-2018 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: dload.cpp
**
**/

#include "libnetxms.h"

#ifndef _WIN32
#ifdef _HPUX
// dlfcn.h on HP-UX defines it's own UINT64 which conflicts with our definition
#define UINT64 __UINT64
#include <dlfcn.h>
#undef UINT64
#else /* _HPUX */
#include <dlfcn.h>
#endif /* _HPUX */
#ifndef RTLD_LOCAL
#define RTLD_LOCAL	0
#endif
#endif /* !_WIN32 */

#define DEBUG_TAG _T("dload")

HMODULE LIBNETXMS_EXPORTABLE DLOpenEx(const TCHAR *libName, bool global, TCHAR *errorText)
{
#ifdef _WIN32
   HMODULE hModule = LoadLibrary(libName);
   if ((hModule == NULL) && (errorText != NULL))
      GetSystemErrorText(GetLastError(), errorText, 255);
#else  /* not _WIN32 */
#ifdef UNICODE
   char *mbbuffer = MBStringFromWideString(libName);
   HMODULE hModule = dlopen(mbbuffer, RTLD_NOW | (global ? RTLD_GLOBAL : RTLD_LOCAL));
   if ((hModule == NULL) && (errorText != NULL))
   {
      WCHAR *wbuffer = WideStringFromMBString(dlerror());
      _tcslcpy(errorText, wbuffer, 255);
      MemFree(wbuffer);
   }
   MemFree(mbbuffer);
#else
   HMODULE hModule = dlopen(libName, RTLD_NOW | (global ? RTLD_GLOBAL : RTLD_LOCAL));
   if ((hModule == NULL) && (errorText != NULL))
      _tcslcpy(errorText, dlerror(), 255);
#endif
#endif
   nxlog_debug_tag(DEBUG_TAG, 7, _T("DLOpen: file=\"%s\", module=%p"), libName, hModule);
   return hModule;
}

/**
 * Load DLL/shared library
 */
HMODULE LIBNETXMS_EXPORTABLE DLOpen(const TCHAR *libName, TCHAR *errorText)
{
   return DLOpenEx(libName, false, errorText);
}

/**
 * Unload DLL/shared library
 */
void LIBNETXMS_EXPORTABLE DLClose(HMODULE hModule)
{
   if (hModule != NULL)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("DLClose: module=%p"), hModule);
#ifdef _WIN32
      FreeLibrary(hModule);
#else
      dlclose(hModule);
#endif
   }
}

/**
 * Get symbol address from library
 */
void LIBNETXMS_EXPORTABLE *DLGetSymbolAddr(HMODULE hModule, const char *symbol, TCHAR *errorText)
{
#ifdef _WIN32
   void *addr = (void *)GetProcAddress(hModule, symbol);
   if ((addr == nullptr) && (errorText != nullptr))
      GetSystemErrorText(GetLastError(), errorText, 255);
#else    /* _WIN32 */
   void *addr = dlsym(hModule, symbol);
   if ((addr == nullptr) && (errorText != nullptr))
	{
#ifdef UNICODE
   	WCHAR *wbuffer = WideStringFromMBString(dlerror());
      _tcslcpy(errorText, wbuffer, 255);
      MemFree(wbuffer);
#else
      _tcslcpy(errorText, dlerror(), 255);
#endif
	}
#endif
   nxlog_debug_tag(DEBUG_TAG, 7, _T("DLGetSymbolAddr: module=%p, symbol=%hs, address=%p"), hModule, symbol, addr);
   return addr;
}
