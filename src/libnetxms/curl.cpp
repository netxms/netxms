/* 
** NetXMS - Network Management System
** Utility Library
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
#include <gauge_helpers.h>
#include <nxlibcurl.h>

/**
 * libcurl initialization flag
 */
static int s_curlInitialized = 0;

/**
 * Saved cURL version
 */
static const char *s_curlVersion = "uninitialized";

/**
 * libcurl OpenSSL 3 backaend indicator
 */
LIBNETXMS_EXPORTABLE_VAR(bool g_curlOpenSSL3Backend) = false;

/**
 * Initialize libcurl
 */
bool LIBNETXMS_EXPORTABLE InitializeLibCURL()
{
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

#ifndef _WIN32
   g_curlOpenSSL3Backend = (strstr(s_curlVersion, "OpenSSL/3.") != nullptr);
   if (g_curlOpenSSL3Backend)
      nxlog_debug_tag(_T("init.curl"), 3, _T("OpenSSL 3 is used as cURL SSL backend"));
#endif

#if defined(_WIN32) || HAVE_DECL_CURL_VERSION_INFO
   curl_version_info_data *version = curl_version_info(CURLVERSION_NOW);
   char protocols[1024] = "";
   const char * const *p = version->protocols;
   while (*p != nullptr)
   {
      if (protocols[0] != 0)
         strlcat(protocols, " ", sizeof(protocols));
      strlcat(protocols, *p, sizeof(protocols));
      p++;
   }
   nxlog_debug_tag(_T("init.curl"), 3, _T("cURL supported protocols: %hs"), protocols);
#endif

   s_curlInitialized = 1;
   return true;
}

/**
 * Global cleanup for libcurl
 */
void LibCURLCleanup()
{
   if (s_curlInitialized > 0)
      curl_global_cleanup();
}

/**
 * Get libcurl version
 */
const char LIBNETXMS_EXPORTABLE *GetLibCURLVersion()
{
   return s_curlVersion;
}
