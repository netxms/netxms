/* 
** NetXMS - Network Management System
** Utility Library
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: curl.cpp
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
 * Name of active TLS backend (first token of curl_version_info()->ssl_version)
 */
static char s_curlSSLBackend[64] = "unknown";

/**
 * libcurl OpenSSL 3 backaend indicator
 */
LIBNETXMS_EXPORTABLE bool g_curlOpenSSL3Backend = false;

/**
 * Select TLS backend in multi-backend libcurl build. Must be called before curl_global_init().
 */
static void SelectSSLBackend(const char *sslBackend)
{
#if HAVE_DECL_CURL_GLOBAL_SSLSET
   const curl_ssl_backend **available = nullptr;
   switch(curl_global_sslset(CURLSSLBACKEND_NONE, sslBackend, &available))
   {
      case CURLSSLSET_OK:
         break;
      case CURLSSLSET_UNKNOWN_BACKEND:
         {
            char list[256] = "";
            if (available != nullptr)
            {
               for(int i = 0; available[i] != nullptr; i++)
               {
                  if (list[0] != 0)
                     strlcat(list, ", ", sizeof(list));
                  strlcat(list, available[i]->name, sizeof(list));
               }
            }
            nxlog_write_tag(NXLOG_ERROR, _T("init.curl"), _T("Requested libcurl TLS backend \"%hs\" is not available (available backends: %hs)"), sslBackend, list);
         }
         break;
      case CURLSSLSET_NO_BACKENDS:
         nxlog_write_tag(NXLOG_WARNING, _T("init.curl"), _T("libcurl is built with single TLS backend, requested TLS backend \"%hs\" ignored"), sslBackend);
         break;
      case CURLSSLSET_TOO_LATE:
         nxlog_write_tag(NXLOG_ERROR, _T("init.curl"), _T("libcurl is already initialized, requested TLS backend \"%hs\" ignored"), sslBackend);
         break;
      default:
         nxlog_write_tag(NXLOG_ERROR, _T("init.curl"), _T("Cannot select libcurl TLS backend \"%hs\""), sslBackend);
         break;
   }
#else
   nxlog_write_tag(NXLOG_WARNING, _T("init.curl"), _T("TLS backend selection is not supported by this libcurl version, requested TLS backend \"%hs\" ignored"), sslBackend);
#endif
}

/**
 * Initialize libcurl. TLS backend (if given) is selected only by the first call that performs
 * actual initialization, so daemons should call this function with configured backend name
 * right after reading configuration and before any other libcurl use.
 */
bool LIBNETXMS_EXPORTABLE InitializeLibCURL(const char *sslBackend)
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

   if ((sslBackend != nullptr) && (*sslBackend != 0))
      SelectSSLBackend(sslBackend);

   if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK)
   {
      nxlog_write_tag(NXLOG_ERROR, _T("init.curl"), _T("libcurl initialization failed"));
      s_curlInitialized = -1;
      return false;
   }

   s_curlVersion = curl_version();

#if defined(_WIN32) || HAVE_DECL_CURL_VERSION_INFO
   curl_version_info_data *version = curl_version_info(CURLVERSION_NOW);

   // In multi-backend builds ssl_version lists all backends in build order, with inactive ones in parentheses
   // (for example "(OpenSSL/3.5.9) Schannel")
   const char *backend = version->ssl_version;
   while ((backend != nullptr) && (*backend == '('))
   {
      backend = strchr(backend, ' ');
      if (backend != nullptr)
         backend++;
   }
   if ((backend != nullptr) && (*backend != 0))
   {
      strlcpy(s_curlSSLBackend, backend, sizeof(s_curlSSLBackend));
      s_curlSSLBackend[strcspn(s_curlSSLBackend, " ")] = 0;
   }
   else
   {
      strcpy(s_curlSSLBackend, "none");
   }
   g_curlOpenSSL3Backend = (strncmp(s_curlSSLBackend, "OpenSSL/3.", 10) == 0);
   nxlog_write_tag(NXLOG_INFO, _T("init.curl"), _T("libcurl initialized (version: %hs, TLS backend: %hs)"), version->version, s_curlSSLBackend);

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
#else
   g_curlOpenSSL3Backend = (strstr(s_curlVersion, "OpenSSL/3.") != nullptr);
   nxlog_write_tag(NXLOG_INFO, _T("init.curl"), _T("libcurl initialized (version: %hs)"), s_curlVersion);
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

/**
 * Get active libcurl TLS backend name with version (for example "OpenSSL/3.5.9" or "Schannel"),
 * "none" if libcurl is built without TLS support, or "unknown" if libcurl is not initialized
 */
const char LIBNETXMS_EXPORTABLE *GetLibCURLSSLBackend()
{
   return s_curlSSLBackend;
}

/**
 * Setup peer certificate trust for curl handle. If CA bundle file is given, it is used as the only
 * trust source. Otherwise, on Windows the system certificate store is used: natively by Schannel
 * backend (with best-effort revocation checks, so unreachable CRL distribution points do not fail
 * verification), and via CURLSSLOPT_NATIVE_CA by OpenSSL backend, which has no default trust store
 * on Windows.
 */
void LIBNETXMS_EXPORTABLE SetupLibCURLTrust(CURL *curl, const char *caBundle)
{
   bool haveBundle = (caBundle != nullptr) && (*caBundle != 0);
   if (haveBundle)
      curl_easy_setopt(curl, CURLOPT_CAINFO, caBundle);

#ifdef _WIN32
   long options = 0;
#ifdef CURLSSLOPT_REVOKE_BEST_EFFORT
   if (!strncmp(s_curlSSLBackend, "Schannel", 8))
      options |= CURLSSLOPT_REVOKE_BEST_EFFORT;
#endif
#ifdef CURLSSLOPT_NATIVE_CA
   if (!haveBundle && !strncmp(s_curlSSLBackend, "OpenSSL", 7))
      options |= CURLSSLOPT_NATIVE_CA;
#endif
   if (options != 0)
      curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, options);
#endif
}
