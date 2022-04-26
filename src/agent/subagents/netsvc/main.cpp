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

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>
#include <netxms-regex.h>
#include <netxms-version.h>

#include <curl/curl.h>

#include "netsvc.h"

// workaround for older cURL versions
#ifndef CURL_MAX_HTTP_HEADER
#define CURL_MAX_HTTP_HEADER CURL_MAX_WRITE_SIZE
#endif

uint32_t g_netsvcFlags = NETSVC_AF_VERIFYPEER;
char g_certBundle[1024] = {0};
uint32_t g_netsvcTimeout = 30;

/**
 * Config file definition
 */
static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
   { _T("VerifyPeer"), CT_BOOLEAN_FLAG_32, 0, 0, NETSVC_AF_VERIFYPEER, 0, &g_netsvcFlags },
   { _T("CA"), CT_MB_STRING, 0, 0, 1024, 0, &g_certBundle },
   { _T("Timeout"), CT_WORD, 0, 0, 0, 0, &g_netsvcTimeout },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr }
};

/**
 * Callback for processing data received from cURL
 */
static size_t OnCurlDataReceived(char *ptr, size_t size, size_t nmemb, void *userdata)
{
   ByteStream *data = (ByteStream *)userdata;
   size_t bytes = size * nmemb;
   data->write(ptr, bytes);
   return bytes;
}

/**
 * Handler for Service.Status(url, pattern)
 */
static LONG H_CheckService(const TCHAR *parameters, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char url[2048] = "";
   AgentGetParameterArgA(parameters, 1, url, 2048);
   TrimA(url);
   if (url[0] == 0)
      return SYSINFO_RC_UNSUPPORTED;

   TCHAR pattern[256] = _T("");
   AgentGetParameterArg(parameters, 2, pattern, 256);
   Trim(pattern);
   if (pattern[0] == 0)
   {
      _tcscpy(pattern, _T("^HTTP/(1\\.[01]|2) 200 .*"));
   }

   const char *eptr;
   int eoffset;
   PCRE *compiledPattern = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(pattern), PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, nullptr);
   if (compiledPattern == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("H_CheckService(%hs): Cannot compile pattern \"%s\""), url, pattern);
      return SYSINFO_RC_UNSUPPORTED;
   }

   TCHAR optionsText[256] = _T("");
   AgentGetParameterArg(parameters, 3, optionsText, 256);
   Trim(optionsText);
   _tcslwr(optionsText);
   StringList *options = String(optionsText).split(_T(","));

   nxlog_debug_tag(DEBUG_TAG, 5, _T("H_CheckService(%hs): pattern=\"%s\", options=\"%s\""), url, pattern, optionsText);

   int retCode = PC_ERR_BAD_PARAMS;

   CURL *curl = curl_easy_init();
   if (curl != nullptr)
   {
#if HAVE_DECL_CURLOPT_NOSIGNAL
      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, static_cast<long>(1)); // do not install signal handlers or send signals
#endif
      curl_easy_setopt(curl, CURLOPT_HEADER, static_cast<long>(1)); // include header in data
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, g_netsvcTimeout);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnCurlDataReceived);
      curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 6.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/41.0.2228.0 Safari/537.36");

      // SSL-related stuff
      bool verifyPeer = (((g_netsvcFlags & NETSVC_AF_VERIFYPEER) != 0) || options->contains(_T("verify-peer"))) && !options->contains(_T("no-verify-peer"));
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, static_cast<long>(verifyPeer ? 1 : 0));
      if (g_certBundle[0] != 0)
      {
         curl_easy_setopt(curl, CURLOPT_CAINFO, g_certBundle);
      }
      if (options->contains(_T("no-verify-host")))
         curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, static_cast<long>(0));
      else if (options->contains(_T("verify-host")))
         curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, static_cast<long>(2));

      // Receiving buffer
      ByteStream data(32768);
      data.setAllocationStep(32768);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);

      char *requestURL = url;

retry:
      if (curl_easy_setopt(curl, CURLOPT_URL, requestURL) == CURLE_OK)
      {
         if (curl_easy_perform(curl) == 0)
         {
            long responseCode = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
            nxlog_debug_tag(DEBUG_TAG, 6, _T("H_CheckService(%hs): got reply (%u bytes, response code %03ld)"), url, static_cast<uint32_t>(data.size()), responseCode);

            if ((responseCode >= 300) && (responseCode <= 399) && options->contains(_T("follow-location")))
            {
               char *redirectURL = nullptr;
               curl_easy_getinfo(curl, CURLINFO_REDIRECT_URL, &redirectURL);
               if (redirectURL != nullptr)
               {
                  requestURL = redirectURL;
                  nxlog_debug_tag(DEBUG_TAG, 6, _T("H_CheckService(%hs): follow redirect to %hs"), url, redirectURL);
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
               WCHAR *wtext = WideStringFromUTF8String((char *)data.buffer(&size));
               if (_pcre_exec_t(compiledPattern, NULL, reinterpret_cast<const PCRE_TCHAR*>(wtext), static_cast<int>(wcslen(wtext)), 0, 0, pmatch, 30) >= 0)
#else
               char *text = (char *)data.buffer(&size);
               if (pcre_exec(compiledPattern, NULL, text, static_cast<int>(size), 0, 0, pmatch, 30) >= 0)
#endif
               {
                  nxlog_debug_tag(DEBUG_TAG, 5, _T("H_CheckService(%hs): matched"), url);
                  retCode = PC_ERR_NONE;
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG, 5, _T("H_CheckService(%hs): not matched"), url);
                  retCode = PC_ERR_NOMATCH;
               }
#ifdef UNICODE
               MemFree(wtext);
#endif
            }
            else
            {
               // zero size reply
               retCode = PC_ERR_NOMATCH;
            }
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 6, _T("H_CheckService(%hs): call to curl_easy_perform failed"), url);
            retCode = PC_ERR_CONNECT;
         }
      }
      curl_easy_cleanup(curl);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("H_CheckService(%hs): curl_init failed"), url);
   }

   _pcre_free_t(compiledPattern);
   delete options;

   ret_int(value, retCode);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Subagent initialization
 */
static bool SubagentInit(Config *config)
{
   bool success = config->parseTemplate(_T("netsvc"), m_cfgTemplate);

   if (success)
      success = InitializeLibCURL();

   return success;
}

/**
 * Called by master agent at unload
 */
static void SubagentShutdown()
{
}

/**
 * Provided parameters
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] = 
{
   { _T("NetworkService.Check(*)"), H_CheckService, nullptr, DCI_DT_INT, _T("Service {instance} status") },
   { _T("Service.Check(*)"), H_CheckService, nullptr, DCI_DT_DEPRECATED, _T("Service {instance} status") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO s_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("NETSVC"), NETXMS_VERSION_STRING,
   SubagentInit, SubagentShutdown, nullptr, nullptr,
   sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
   m_parameters,
   0, nullptr, // enums
   0, nullptr,	// tables
   0, nullptr,	// actions
   0, nullptr	// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(NETSVC)
{
   *ppInfo = &s_info;
   return true;
}

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
