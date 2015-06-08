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

   char url[2048] = {0};
   char pattern[4096] = {0};
   regex_t compiledPattern;

   AgentGetParameterArgA(parameters, 1, url, 2048);
   AgentGetParameterArgA(parameters, 2, pattern, 256);
   StrStripA(url);
   StrStripA(pattern);
   if (url[0] != 0)
   {
      if (pattern[0] == 0)
      {
         strcpy(pattern, "^HTTP/1.[01] 200 .*");
      }

      AgentWriteDebugLog(5, _T("Check service: url=%hs, pattern=%hs"), url, pattern);

      if (tre_regcomp(&compiledPattern, pattern, REG_EXTENDED | REG_ICASE | REG_NOSUB) == 0)
      {
         CURL *curl = curl_easy_init();
         if (curl != NULL)
         {
            ret = SYSINFO_RC_SUCCESS;

#if HAVE_DECL_CURLOPT_NOSIGNAL
            curl_easy_setopt(curl, CURLOPT_NOSIGNAL, (long)1);
#endif

            // curl_easy_setopt(curl, CURLOPT_VERBOSE, (long)1);
            curl_easy_setopt(curl, CURLOPT_HEADER, (long)1); // include header in data
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, g_timeout);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnCurlDataReceived);

            // SSL-related stuff
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, g_flags & NETSVC_AF_VERIFYPEER);
            if (g_certBundle[0] != 0)
            {
               curl_easy_setopt(curl, CURLOPT_CAINFO, g_certBundle);
            }

            ByteStream data(CURL_MAX_HTTP_HEADER * 20);
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
                     if (tre_regexec(&compiledPattern, (char *)data.buffer(&size), 0, NULL, 0) == 0)
                     {
                        AgentWriteDebugLog(5, _T("Check service: matched"));
                        retCode = PC_ERR_NONE;
                     }
                     else
                     {
                        AgentWriteDebugLog(5, _T("Check service: not matched"));
                        retCode = PC_ERR_NOMATCH;
                     }
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
         tre_regfree(&compiledPattern);
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
static BOOL SubagentInit(Config *config) 
{
   bool ret = false;

	config->parseTemplate(_T("netsvc"), m_cfgTemplate);

   ret = curl_global_init(CURL_GLOBAL_ALL) == 0;

   if (ret)
   {
      AgentWriteDebugLog(3, _T("cURL version: %hs"), curl_version());
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
   return ret;
}

/**
 * Called by master agent at unload
 */
static void SubagentShutdown()
{
   curl_global_cleanup();
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
static NETXMS_SUBAGENT_INFO m_info = 
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("NETSVC"), NETXMS_VERSION_STRING,
   SubagentInit, SubagentShutdown, NULL,
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
   *ppInfo = &m_info;
   return TRUE;
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
