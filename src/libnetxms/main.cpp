/* 
** NetXMS - Network Management System
** Utility Library
** Copyright (C) 2003-2012 Victor Kirhenshtein
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
** File: main.cpp
**
**/

#include "libnetxms.h"

#if HAVE_LIBCURL
#include <curl/curl.h>
#endif

/**
 * libcurl initialization flag
 */
static int s_curlInitialized = 0;

/**
 * Saved cURL version
 */
static const char *s_curlVersion = "uninitialized";

/**
 * Initialize libcurl
 */
bool LIBNETXMS_EXPORTABLE InitializeLibCURL()
{
#if HAVE_LIBCURL
   static VolatileCounter reentryGuarg = 0;

retry:
   if (s_curlInitialized > 0)
      return true;
   if (s_curlInitialized < 0)
      return false;

   if (InterlockedIncrement(&reentryGuarg) > 1)
   {
      InterlockedDecrement(&reentryGuarg);
      goto retry;
   }

   if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK)
   {
      nxlog_debug_tag(_T("init.curl"), 1, _T("cURL initialization failed"));
      s_curlInitialized = -1;
      return false;
   }

   s_curlVersion = curl_version();
   nxlog_debug_tag(_T("init.curl"), 3, _T("cURL initialized (version: %hs)"), s_curlVersion);
   s_curlInitialized = 1;
   return true;
#else
   return false;
#endif
}

/**
 * Global cleanup for libcurl
 */
void LibCURLCleanup()
{
#if HAVE_LIBCURL
   if (s_curlInitialized > 0)
      curl_global_cleanup();
#endif
}

/**
 * Get libcurl version
 */
const char LIBNETXMS_EXPORTABLE *GetLibCURLVersion()
{
   return s_curlVersion;
}

/**
 * Wrappers for byte swap functions for C code linking
 */
#if !(HAVE_DECL_BSWAP_16)
extern "C" UINT16 LIBNETXMS_EXPORTABLE __bswap_16(UINT16 val)
{
   return bswap_16(val);
}
#endif

#if !(HAVE_DECL_BSWAP_32)
extern "C" UINT32 LIBNETXMS_EXPORTABLE __bswap_32(UINT32 val)
{
   return bswap_32(val);
}
#endif

#if !(HAVE_DECL_BSWAP_64)
extern "C" UINT64 LIBNETXMS_EXPORTABLE __bswap_64(UINT64 val)
{
   return bswap_64(val);
}
#endif

/**
 * Swap bytes in INT16 array or UCS-2 string
 * Length -1 causes stop at first 0 value
 */
void LIBNETXMS_EXPORTABLE bswap_array_16(UINT16 *v, int len)
{
   if (len < 0)
   {
      for(UINT16 *p = v; *p != 0; p++)
         *p = bswap_16(*p);
   }
   else
   {
      int count = 0;
      for(UINT16 *p = v; count < len; p++, count++)
         *p = bswap_16(*p);
   }
}

/**
 * Swap bytes in INT32 array or UCS-4 string
 * Length -1 causes stop at first 0 value
 */
void LIBNETXMS_EXPORTABLE bswap_array_32(UINT32 *v, int len)
{
   if (len < 0)
   {
      for(UINT32 *p = v; *p != 0; p++)
         *p = bswap_32(*p);
   }
   else
   {
      int count = 0;
      for(UINT32 *p = v; count < len; p++, count++)
         *p = bswap_32(*p);
   }
}

#if !defined(_WIN32) && !defined(_NETWARE)

/**
 * strupr() implementation for non-windows platforms
 */
void LIBNETXMS_EXPORTABLE __strupr(char *in)
{
	char *p = in;

	if (in == NULL) 
   { 
		return;
	}
	
	for (; *p != 0; p++) 
   {
		// TODO: check/set locale
		*p = toupper(*p);
	}
}

#if defined(UNICODE_UCS2) || defined(UNICODE_UCS4)

/**
 * wcsupr() implementation for non-Windows platforms
 */
void LIBNETXMS_EXPORTABLE __wcsupr(WCHAR *in)
{
	if (in == NULL) 
		return;
	
   WCHAR *p = in;
	for(; *p != 0; p++)
   {
		// TODO: check/set locale
#if HAVE_TOWUPPER
		*p = towupper(*p);
#else
		if (*p < 256)
		{
			*p = (WCHAR)toupper(*p);
		}
#endif
	}
}

#endif

#endif

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
	{
      DisableThreadLibraryCalls(hInstance);
		SEHInit();
	}
   return TRUE;
}

#endif   /* _WIN32 */
