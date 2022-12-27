/*
 ** NetXMS Network Service check subagent
 ** Copyright (C) 2013-2022 Alex Kirhenshtein
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
 **/

#include "netsvc.h"

/**
 * Sub-handler for HTTP/HTTPS service check
 */
LONG NetworkServiceStatus_HTTP(CURL *curl, const OptionList &options, char *url, PCRE *compiledPattern, int *result)
{
   *result = PC_ERR_BAD_PARAMS;

   curl_easy_setopt(curl, CURLOPT_HEADER, static_cast<long>(1)); // include header in data
   curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 6.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/41.0.2228.0 Safari/537.36");

   // Receiving buffer
   ByteStream data(32768);
   data.setAllocationStep(32768);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ByteStream::curlWriteFunction);

   char *requestURL = url;

retry:
   if (curl_easy_setopt(curl, CURLOPT_URL, requestURL) == CURLE_OK)
   {
      CURLcode rc = curl_easy_perform(curl);
      if (rc == CURLE_OK)
      {
         long responseCode = 0;
         curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
         nxlog_debug_tag(DEBUG_TAG, 6, _T("NetworkServiceStatus_HTTP(%hs): got reply (%u bytes, response code %03ld)"), url, static_cast<uint32_t>(data.size()), responseCode);

         if ((responseCode >= 300) && (responseCode <= 399) && options.getAsBoolean(_T("follow-location")))
         {
            char *redirectURL = nullptr;
            curl_easy_getinfo(curl, CURLINFO_REDIRECT_URL, &redirectURL);
            if (redirectURL != nullptr)
            {
               requestURL = redirectURL;
               nxlog_debug_tag(DEBUG_TAG, 6, _T("NetworkServiceStatus_HTTP(%hs): follow redirect to %hs"), url, redirectURL);
               data.clear();
               goto retry;
            }
         }

         if (data.size() > 0)
         {
            data.write('\0');
            size_t size;
            int pmatch[30];
#ifdef UNICODE
            WCHAR *text = WideStringFromUTF8String((char*)data.buffer(&size));
            if (_pcre_exec_t(compiledPattern, nullptr, reinterpret_cast<const PCRE_WCHAR*>(text), static_cast<int>(wcslen(text)), 0, 0, pmatch, 30) >= 0)
#else
            char *text = MBStringFromUTF8String((char*)data.buffer(&size));
            if (pcre_exec(compiledPattern, nullptr, text, static_cast<int>(size), 0, 0, pmatch, 30) >= 0)
#endif
            {
               nxlog_debug_tag(DEBUG_TAG, 5, _T("NetworkServiceStatus_HTTP(%hs): matched"), url);
               *result = PC_ERR_NONE;
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 5, _T("NetworkServiceStatus_HTTP(%hs): not matched"), url);
               *result = PC_ERR_NOMATCH;
            }
            MemFree(text);
         }
         else
         {
            // zero size reply
            *result = PC_ERR_NOMATCH;
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("NetworkServiceStatus_HTTP(%hs): call to curl_easy_perform failed"), url);
         *result = CURLCodeToCheckResult(rc);
      }
   }

   return SYSINFO_RC_SUCCESS;
}

/**
 * Check HTTP/HTTPS service - entry point for command handler and legacy metrics
 */
int CheckHTTP(const char *hostname, const InetAddress& addr, uint16_t port, bool useTLS, const char *uri, const char *hostHeader, const char *match, uint32_t timeout)
{
   CURL *curl = curl_easy_init();
   if (curl == nullptr)
      return PC_ERR_INTERNAL;

   OptionList options(_T(""));
   char url[4096], ipAddrText[64];
   snprintf(url, sizeof(url), "%s://%s:%u/%s", useTLS ? "https" : "http", (hostname != nullptr) ? hostname : addr.toStringA(ipAddrText), static_cast<uint32_t>(port), (uri[0] == '/') ? &uri[1] : uri);
   CurlCommonSetup(curl, url, options, timeout);

   int result;
   const char *eptr;
   int eoffset;
#ifdef UNICODE
   WCHAR wmatch[1024];
   utf8_to_wchar(match, -1, wmatch, 1024);
   PCRE *compiledPattern = _pcre_compile_w(reinterpret_cast<const PCRE_WCHAR*>(wmatch), PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, nullptr);
#else
   char mbmatch[1024];
   utf8_to_mb(match, -1, mbmatch, 1024);
   PCRE *compiledPattern = pcre_compile(mbmatch, PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, nullptr);
#endif
   if (compiledPattern != nullptr)
   {
      NetworkServiceStatus_HTTP(curl, options, url, compiledPattern, &result);
      _pcre_free_t(compiledPattern);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("CheckHTTP(%hs): Cannot compile pattern \"%hs\""), url, match);
      result = PC_ERR_BAD_PARAMS;
   }
   return result;
}

/**
 * Check HTTP/HTTPS service - parameter handler
 */
LONG H_CheckHTTP(const TCHAR* param, const TCHAR* arg, TCHAR* value, AbstractCommSession* session)
{
   char hostname[1024], portText[32], uri[1024], hostHeader[256], match[1024];
   AgentGetParameterArgA(param, 1, hostname, sizeof(hostname));
   AgentGetParameterArgA(param, 2, portText, sizeof(portText));
   AgentGetParameterArgA(param, 3, uri, sizeof(uri));
   AgentGetParameterArgA(param, 4, hostHeader, sizeof(hostHeader));
   AgentGetParameterArgA(param, 5, match, sizeof(match));

   if (hostname[0] == 0 || uri[0] == 0)
      return SYSINFO_RC_ERROR;

   uint16_t port = 0;
   if (portText[0] == 0)
   {
      port = (arg[1] == 'S') ? 443 : 80;
   }
   else
   {
      port = static_cast<uint16_t>(strtoul(portText, nullptr, 10));
      if (port == 0)
         return SYSINFO_RC_UNSUPPORTED;
   }

   uint32_t timeout = GetTimeoutFromArgs(param, 6);
   int64_t start = GetCurrentTimeMs();

   LONG ret = SYSINFO_RC_SUCCESS;
   int result = CheckHTTP(hostname, InetAddress::resolveHostName(hostname), port, arg[1] == 'S', uri, hostHeader[0] != 0 ? hostHeader : hostname, match, timeout);
   if (*arg == 'R')
   {
      if (result == PC_ERR_NONE)
         ret_int64(value, GetCurrentTimeMs() - start);
      else if (g_netsvcFlags & NETSVC_AF_NEGATIVE_TIME_ON_ERROR)
         ret_int(value, -(GetCurrentTimeMs() - start));
      else
         ret = SYSINFO_RC_ERROR;
   }
   else
   {
      ret_int(value, result);
   }
   return ret;
}
