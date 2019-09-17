/*
** NetXMS Network Service check subagent
** Copyright (C) 2013 Alex Kirhenshtein
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

#include <curl/curl.h>

#include "netsvc.h"

// workaround for older cURL versions
#ifndef CURL_MAX_HTTP_HEADER
#define CURL_MAX_HTTP_HEADER CURL_MAX_WRITE_SIZE
#endif

UINT32 g_flags = NETSVC_AF_VERIFYPEER;
char g_certBundle[1024] = {0};
UINT32 g_timeout = 30;

/**
 * Config file definition
 */
static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
   { _T("VerifyPeer"), CT_BOOLEAN, 0, 0, NETSVC_AF_VERIFYPEER, 0, &g_flags },
   { _T("CA"), CT_MB_STRING, 0, 0, 1024, 0, &g_certBundle },
   { _T("Timeout"), CT_WORD, 0, 0, 0, 0, &g_timeout },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
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
 * 
 * TODO: Unicode support!
 */
static LONG H_CheckService(const TCHAR *parameters, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   int ret = SYSINFO_RC_ERROR;
   int retCode = PC_ERR_BAD_PARAMS;

   char url[2048] = "";
   TCHAR pattern[4096] = _T("");

   AgentGetParameterArgA(parameters, 1, url, 2048);
   AgentGetParameterArg(parameters, 2, pattern, 256);
   StrStripA(url);
   StrStrip(pattern);
   if (url[0] != 0)
   {
      if (pattern[0] == 0)
      {
         _tcscpy(pattern, _T("^HTTP/(1\\.[01]|2) 200 .*"));
      }

      AgentWriteDebugLog(5, _T("Check service: url=%hs, pattern=%s"), url, pattern);

      const char *eptr;
      int eoffset;
      PCRE *compiledPattern = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(pattern), PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, NULL);
      if (compiledPattern != NULL)
      {
         CURL *curl = curl_easy_init();
         if (curl != NULL)
         {
            ret = SYSINFO_RC_SUCCESS;

#if HAVE_DECL_CURLOPT_NOSIGNAL
            curl_easy_setopt(curl, CURLOPT_NOSIGNAL, (long)1);
#endif

            curl_easy_setopt(curl, CURLOPT_NOSIGNAL, (long)1); // do not install signal handlers or send signals
            curl_easy_setopt(curl, CURLOPT_HEADER, (long)1); // include header in data
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, g_timeout);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnCurlDataReceived);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 6.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/41.0.2228.0 Safari/537.36");

            // SSL-related stuff
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, g_flags & NETSVC_AF_VERIFYPEER);
            if (g_certBundle[0] != 0)
            {
               curl_easy_setopt(curl, CURLOPT_CAINFO, g_certBundle);
            }

            // Receiving buffer
            ByteStream data(32768);
            data.setAllocationStep(32768);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);

            if (curl_easy_setopt(curl, CURLOPT_URL, url) == CURLE_OK)
            {
               AgentWriteDebugLog(5, _T("Check service: all prepared"));
               if (curl_easy_perform(curl) == 0)
               {
                  AgentWriteDebugLog(6, _T("Check service: got reply: %lu bytes"), (unsigned long)data.size());
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
                        AgentWriteDebugLog(5, _T("Check service: matched"));
                        retCode = PC_ERR_NONE;
                     }
                     else
                     {
                        AgentWriteDebugLog(5, _T("Check service: not matched"));
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
                  retCode = PC_ERR_CONNECT;
               }
            }
            curl_easy_cleanup(curl);
         }
         else
         {
            AgentWriteLog(3, _T("Check service: curl_init failed"));
         }
         _pcre_free_t(compiledPattern);
      }
      else
      {
         AgentWriteLog(3, _T("Check service: Can't compile pattern '%hs'"), pattern);
      }
   }

   if (ret == SYSINFO_RC_SUCCESS)
   {
      ret_int(value, retCode);
   }

   return ret;
}

/**
 * Subagent initialization
 */
static bool SubagentInit(Config *config)
{
   bool success = config->parseTemplate(_T("netsvc"), m_cfgTemplate);
   if (success)
   {
      success = InitializeLibCURL();
   }
   if (success)
   {
      AgentWriteDebugLog(3, _T("Using cURL version: %hs"), GetLibCURLVersion());
#if defined(_WIN32) || HAVE_DECL_CURL_VERSION_INFO
      curl_version_info_data *version = curl_version_info(CURLVERSION_NOW);
      char protocols[1024] = {0};
      const char * const *p = version->protocols;
      while (*p != NULL)
      {
         strncat(protocols, *p, strlen(protocols) - 1);
         strncat(protocols, " ", strlen(protocols) - 1);
         p++;
      }
      AgentWriteDebugLog(3, _T("Supported protocols: %hs"), protocols);
#endif
   }
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
   { _T("Service.Check(*)"), H_CheckService, NULL, DCI_DT_INT, _T("Service {instance} status") },
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO s_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("NETSVC"), NETXMS_BUILD_TAG,
   SubagentInit, SubagentShutdown, NULL, NULL,
   sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
   m_parameters,
   0, NULL, // enums
   0, NULL,	// tables
   0, NULL,	// actions
   0, NULL	// push parameters
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
