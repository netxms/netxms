/** 
** NetXMS - Network Management System
** Utility Library
** Copyright (C) 2003-2017 Victor Kirhenshtein
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

#if defined(_NETWARE)
#undef SEVERITY_CRITICAL
#include <netware.h>
#elif !defined(_WIN32)
#ifdef _HPUX
// dlfcn.h on HP-UX defines it's own UINT64 which conflicts with our definition
#define UINT64 __UINT64
#include <dlfcn.h>
#undef UINT64
#else
#include <dlfcn.h>
#endif
#ifndef RTLD_LOCAL
#define RTLD_LOCAL	0
#endif
#endif

#ifdef _NETWARE

/**
 * Error texts for NetWare LoadModule()
 */
static char *m_pszErrorText[] =
{
   "No error",
   "File not found",
   "Error reading file",
   "File not in NLM format",
   "Wrong NLM file version",
   "Reentrant initialization failure",
   "Cannot load multiple copies",
   "Operation already in progress",
   "Not enough memory",
   "Initialization failure",
   "Inconsistent file format",
   "Cannot load module at startup",
   "Unresolved dependencies",
   "Unresolved external symbols",
   "Public symbol already defined",
   "XDC data error",
   "Module must be loaded in the kernel",
   "Module is NIOS compatible only and cannot be loaded on NetWare",
   "Cannot create specified address space",
   "Module initialization failed"
};

#endif

/**
 * Load DLL/shared library
 */
HMODULE LIBNETXMS_EXPORTABLE DLOpen(const TCHAR *pszLibName, TCHAR *pszErrorText)
{
   HMODULE hModule;

#if defined(_WIN32)
   hModule = LoadLibrary(pszLibName);
   if ((hModule == NULL) && (pszErrorText != NULL))
      GetSystemErrorText(GetLastError(), pszErrorText, 255);
#elif defined(_NETWARE)
   TCHAR szBuffer[MAX_PATH + 4];
   int nError;

   _tcslcpy(&szBuffer[4], pszLibName, MAX_PATH);
   nError = LoadModule(getnetwarelogger(), &szBuffer[4], LO_RETURN_HANDLE);
   if (nError == 0)
   {
      hModule = *((HMODULE *)szBuffer);
		if (pszErrorText != NULL)
      	*pszErrorText = 0;
   }
   else
   {
      hModule = NULL;
		if (pszErrorText != NULL)
      	_tcslcpy(pszErrorText, (nError <= 19) ? m_pszErrorText[nError] : "Unknown error code", 255);
   }
#else    /* not _WIN32 and not _NETWARE */
#ifdef UNICODE
	char *mbbuffer = MBStringFromWideString(pszLibName);
   hModule = dlopen(mbbuffer, RTLD_NOW | RTLD_LOCAL);
   if ((hModule == NULL) && (pszErrorText != NULL))
   {
   	WCHAR *wbuffer = WideStringFromMBString(dlerror());
      _tcslcpy(pszErrorText, wbuffer, 255);
      free(wbuffer);
   }
   free(mbbuffer);
#else
   hModule = dlopen(pszLibName, RTLD_NOW | RTLD_LOCAL);
   if ((hModule == NULL) && (pszErrorText != NULL))
      _tcslcpy(pszErrorText, dlerror(), 255);
#endif
#endif
   return hModule;
}

/**
 * Unload DLL/shared library
 */
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

/**
 * Get symbol address from library
 */
void LIBNETXMS_EXPORTABLE *DLGetSymbolAddr(HMODULE hModule, const char *pszSymbol, TCHAR *pszErrorText)
{
   void *pAddr;

#if defined(_WIN32)
   pAddr = (void *)GetProcAddress(hModule, pszSymbol);
   if ((pAddr == NULL) && (pszErrorText != NULL))
      GetSystemErrorText(GetLastError(), pszErrorText, 255);
#elif defined(_NETWARE)
   pAddr = ImportPublicObject(hModule, pszSymbol);
   if (pszErrorText != NULL)
      *pszErrorText = 0;
#else    /* _WIN32 */
   pAddr = dlsym(hModule, pszSymbol);
   if ((pAddr == NULL) && (pszErrorText != NULL))
	{
#ifdef UNICODE
   	WCHAR *wbuffer = WideStringFromMBString(dlerror());
      _tcslcpy(pszErrorText, wbuffer, 255);
      free(wbuffer);
#else
      _tcslcpy(pszErrorText, dlerror(), 255);
#endif
	}
#endif
   return pAddr;
}
