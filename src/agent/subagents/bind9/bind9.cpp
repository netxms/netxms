/*
** NetXMS Bind9 subagent
** Copyright (C) 2021-2025 Raden Solutions
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
** File: bind9.cpp
**
**/

#include <netxms-version.h>
#include <nms_agent.h>
#include <nms_util.h>
#include <curl/curl.h>

#define DEBUG_TAG _T("bind9")

/**
 * JSON response containing bind9 statistics
 */
json_t * s_response = nullptr;

/**
 * Statistic cache retention time
 */
static uint32_t s_cacheRetentionTime = 5;

/**
 * Timestamp of last query
 */
static time_t s_queryTimestamp = 0;

/**
 * Address for cURL call to get stats
 */
static char s_queryUrl[MAX_PATH];

/**
 * Lock for json parameter read/writing
 */
static Mutex s_paramsLock;

/**
 * Flag to prevent error spam in case of bad configuration
 */
static bool s_inErrorState = false;

/**
 * Function to fetch stats json using cURL. Assumed to be called under s_paramsLock mutex
 */
static void FetchStats()
{
   time_t now = time(nullptr);

   // Skip fetch if last query still valid
   if ((now - s_queryTimestamp) < s_cacheRetentionTime)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Skipping fetch due to query time not reached"));
      return;
   }

   json_decref(s_response);
   s_response = nullptr;

   // Do query
   CURL *curl = curl_easy_init();
   if (curl == nullptr)
   {
      if (!s_inErrorState)
      {
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Call to curl_easy_init() failed"));
         s_inErrorState = true;
      }
      return;
   }

   curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ByteStream::curlWriteFunction);
   curl_easy_setopt(curl, CURLOPT_USERAGENT, "NetXMS Bind9 Driver/" NETXMS_VERSION_STRING_A);

   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

   if (curl_easy_setopt(curl, CURLOPT_URL, s_queryUrl) == CURLE_OK)
   {
      if (curl_easy_perform(curl) == CURLE_OK)
      {
         responseData.write('\0');
         json_error_t error;
         s_response = json_loads(reinterpret_cast<const char*>(responseData.buffer()), 0, &error);
         if ((s_response == nullptr) && (!s_inErrorState))
         {
            nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Cannot parse API response (%hs)"), error.text);
            s_inErrorState = true;
         }
      }
      else if (!s_inErrorState)
      {
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Call to curl_easy_perform() failed"));
         s_inErrorState = true;
      }
   }
   else if (!s_inErrorState)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Parameter parsing failed. Check \"QueryURL\""));
      s_inErrorState = true;
   }

   s_queryTimestamp = now;

   curl_easy_cleanup(curl);
}

/**
 * Parser function for bind9 stats request
 */
static LONG H_Bind9Info(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   LONG rc = SYSINFO_RC_SUCCESS;

   s_paramsLock.lock();
   FetchStats();
   if (s_response != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Received valid API response"));
      s_inErrorState = false;

      json_t *property = json_object_get_by_path_a(s_response, reinterpret_cast<const char*>(arg));
      if (property != nullptr)
      {
         if (json_is_integer(property))
         {
            ret_int64(value, json_integer_value(property));
         }
         else if (json_is_string(property))
         {
            ret_utf8string(value, json_string_value(property));
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("Unsupported data type for property \"%hs\""), reinterpret_cast<const char*>(arg));
            rc = SYSINFO_RC_UNSUPPORTED;
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Cannot find property \"%hs\""), reinterpret_cast<const char*>(arg));
         rc = SYSINFO_RC_UNSUPPORTED;
      }
   }
   else
   {
      rc = SYSINFO_RC_ERROR;
   }
   s_paramsLock.unlock();

   return rc;
}

/**
 * Get metric from user-supplied path
 */
static LONG H_Bind9CustomInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char path[256];
   if (!AgentGetParameterArgA(param, 1, path, 256))
      return SYSINFO_RC_UNSUPPORTED;
   return H_Bind9Info(param, reinterpret_cast<TCHAR*>(path), value, session);
}

/**
 * Subagent initialization
 */
static bool SubAgentInit(Config *config)
{
   // Parse configuration
   s_cacheRetentionTime = config->getValueAsInt(_T("/Bind9/CacheRetentionTime"), s_cacheRetentionTime);
   const TCHAR *ip = config->getValue(_T("/Bind9/Address"), _T("127.0.0.1"));
   int port = config->getValueAsInt(_T("/Bind9/Port"), 80);

   if ((port <= 0) || (port >= 65535))
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Invalid Address/port configuration. Check \"Address\" and \"Port\" parameters"));
      return false;
   }

#ifdef UNICODE
   char *utfIp = UTF8StringFromWideString(ip);
   snprintf(s_queryUrl, MAX_PATH, "http://%s:%d/json/v1/server", utfIp, port);
   MemFree(utfIp);
#else
   snprintf(s_queryUrl, MAX_PATH, "http://%s:%d/json/v1/server", ip, port);
#endif

   nxlog_debug_tag(DEBUG_TAG, 2, _T("Query URL: %hs"), s_queryUrl);
   return true;
}

/**
 * Called by master agent at unload
 */
static void SubAgentShutdown()
{
   json_decref(s_response);
}

/**
 * Subagent parameters
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
   { _T("Bind9.Answers.Auth"), H_Bind9Info, reinterpret_cast<const TCHAR*>("nsstats/QryAuthAns"), DCI_DT_UINT, _T("Bind9: queries resulted in an authoritative answer") },
   { _T("Bind9.Answers.FORMERR"), H_Bind9Info, reinterpret_cast<const TCHAR*>("nsstats/QryFORMERR"), DCI_DT_UINT, _T("Bind9: queries resulted in a FORMERR answer") },
   { _T("Bind9.Answers.NoData"), H_Bind9Info, reinterpret_cast<const TCHAR*>("nsstats/QryNxrrset"), DCI_DT_UINT, _T("Bind9: queries resulted in NOERROR responses with no data") },
   { _T("Bind9.Answers.NonAuth"), H_Bind9Info, reinterpret_cast<const TCHAR*>("nsstats/QryNoauthAns"), DCI_DT_UINT, _T("Bind9: queries resulted in a non-authoritative answer") },
   { _T("Bind9.Answers.NXDOMAIN"), H_Bind9Info, reinterpret_cast<const TCHAR*>("nsstats/QryNXDOMAIN"), DCI_DT_UINT, _T("Bind9: queries resulted in a NXDOMAIN answer") },
   { _T("Bind9.Answers.Recursive"), H_Bind9Info, reinterpret_cast<const TCHAR*>("nsstats/QryRecursion"), DCI_DT_UINT, _T("Bind9: queries that caused recursion") },
   { _T("Bind9.Answers.Referral"), H_Bind9Info, reinterpret_cast<const TCHAR*>("nsstats/QryReferral"), DCI_DT_UINT, _T("Bind9: queries resulted in referral answer") },
   { _T("Bind9.Answers.SERVFAIL"), H_Bind9Info, reinterpret_cast<const TCHAR*>("nsstats/QrySERVFAIL"), DCI_DT_UINT, _T("Bind9: queries resulted in a SERVFAIL answer") },
   { _T("Bind9.BootTime"), H_Bind9Info, reinterpret_cast<const TCHAR*>("boot-time"), DCI_DT_STRING, _T("Bind9: boot time") },
   { _T("Bind9.ConfigTime"), H_Bind9Info, reinterpret_cast<const TCHAR*>("config-time"), DCI_DT_STRING, _T("Bind9: boot time") },
   { _T("Bind9.Notifies.Received.IPv4"), H_Bind9Info, reinterpret_cast<const TCHAR*>("zonestats/NotifyInv4"), DCI_DT_UINT, _T("Bind9: received IPv4 notifies") },
   { _T("Bind9.Notifies.Received.IPv6"), H_Bind9Info, reinterpret_cast<const TCHAR*>("zonestats/NotifyInv6"), DCI_DT_UINT, _T("Bind9: received IPv6 notifies") },
   { _T("Bind9.Notifies.Sent.IPv4"), H_Bind9Info, reinterpret_cast<const TCHAR*>("zonestats/NotifyOutv4"), DCI_DT_UINT, _T("Bind9: sent IPv4 notifies") },
   { _T("Bind9.Notifies.Sent.IPv6"), H_Bind9Info, reinterpret_cast<const TCHAR*>("zonestats/NotifyOutv6"), DCI_DT_UINT, _T("Bind9: sent IPv6 notifies") },
   { _T("Bind9.Other(*)"), H_Bind9CustomInfo, nullptr, DCI_DT_UINT, _T("Bind9: value of metric {instance}") },
   { _T("Bind9.Requests.Completed.Updates"), H_Bind9Info, reinterpret_cast<const TCHAR*>("nsstats/UpdateDone"), DCI_DT_UINT, _T("Bind9: completed updates") },
   { _T("Bind9.Requests.Completed.ZoneTransfers"), H_Bind9Info, reinterpret_cast<const TCHAR*>("nsstats/XfrReqDone"), DCI_DT_UINT, _T("Bind9: completed zone transfers") },
   { _T("Bind9.Requests.Failed.Dropped"), H_Bind9Info, reinterpret_cast<const TCHAR*>("nsstats/QryDropped"), DCI_DT_UINT, _T("Bind9: dropped queries") },
   { _T("Bind9.Requests.Failed.Other"), H_Bind9Info, reinterpret_cast<const TCHAR*>("nsstats/QryFailure"), DCI_DT_UINT, _T("Bind9: other query failures") },
   { _T("Bind9.Requests.Received.IPv4"), H_Bind9Info, reinterpret_cast<const TCHAR*>("nsstats/Requestv4"), DCI_DT_UINT, _T("Bind9: received IPv4 requests") },
   { _T("Bind9.Requests.Received.IPv6"), H_Bind9Info, reinterpret_cast<const TCHAR*>("nsstats/Requestv6"), DCI_DT_UINT, _T("Bind9: received IPv6 requests") },
   { _T("Bind9.Requests.Received.TCP"), H_Bind9Info, reinterpret_cast<const TCHAR*>("nsstats/ReqTCP"), DCI_DT_UINT, _T("Bind9: received TCP requests") },
   { _T("Bind9.Requests.Rejected.Authoritative"), H_Bind9Info, reinterpret_cast<const TCHAR*>("nsstats/AuthQryRej"), DCI_DT_UINT, _T("Bind9: rejected authoritative queries") },
   { _T("Bind9.Requests.Rejected.Recursive"), H_Bind9Info, reinterpret_cast<const TCHAR*>("nsstats/RecQryRej"), DCI_DT_UINT, _T("Bind9: rejected recursive queries") },
   { _T("Bind9.Requests.Rejected.Updates"), H_Bind9Info, reinterpret_cast<const TCHAR*>("nsstats/UpdateRej"), DCI_DT_UINT, _T("Bind9: rejected updates") },
   { _T("Bind9.Requests.Rejected.ZoneTransfers"), H_Bind9Info, reinterpret_cast<const TCHAR*>("nsstats/XfrRej"), DCI_DT_UINT, _T("Bind9: rejected zone transfers") },
   { _T("Bind9.Version"), H_Bind9Info, reinterpret_cast<const TCHAR*>("version"), DCI_DT_STRING, _T("Bind9: version") },
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO s_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("Bind9"), NETXMS_VERSION_STRING,
   SubAgentInit, SubAgentShutdown, nullptr, nullptr, nullptr,
   sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
   m_parameters,
   0, nullptr, // lists
   0, nullptr, // tables
   0, nullptr, // actions
   0, nullptr  // push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(Bind9)
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
