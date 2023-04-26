/*
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Raden Solutions
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

#ifdef CURLPROTO_SMTP

/**
 * Check SMTP service (used by command handler and legacy metric handler)
 */
int CheckSMTP(const InetAddress& addr, uint16_t port, bool enableTLS, const char* to, uint32_t timeout)
{
   CURL *curl = PrepareCurlHandle(addr, port, enableTLS ? "smtps" : "smtp", timeout);
   if (curl == nullptr)
      return PC_ERR_BAD_PARAMS;

   char from[128];
   strcpy(from, "noreply@");
   strlcat(from, g_netsvcDomainName, 128);
   curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from);

   struct curl_slist *recipients = curl_slist_append(nullptr, to);
   curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

   char errorText[CURL_ERROR_SIZE] = "";
   curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorText);

   CURLcode rc = curl_easy_perform(curl);
   if (rc != CURLE_OK)
   {
      TCHAR addrText[64];
      nxlog_debug_tag(DEBUG_TAG, 6, _T("CheckSMTP(%s//%s:%d): call to curl_easy_perform failed (%hs)"), enableTLS ? _T("smtps") : _T("smtp"), addr.toString(addrText), port, errorText);
   }
   int result = CURLCodeToCheckResult(rc);

   curl_slist_free_all(recipients);
   return result;
}

/**
 * Check SMTP service - metric sub-handler
 */
LONG NetworkServiceStatus_SMTP(CURL *curl, const OptionList& options, const char *url, int *result)
{
   char from[128], to[128];
   tchar_to_utf8(options.get(_T("from"), _T("")), -1, from, 128);
   tchar_to_utf8(options.get(_T("to"), _T("")), -1, to, 128);

   if (to[0] == 0)
      return SYSINFO_RC_UNSUPPORTED;

   if (from[0] == 0)
   {
      strcpy(from, "noreply@");
      strlcat(from, g_netsvcDomainName, 128);
   }
   curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from);

   struct curl_slist *recipients = curl_slist_append(nullptr, to);
   curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

   char errorText[CURL_ERROR_SIZE] = "";
   curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorText);

   CURLcode rc = curl_easy_perform(curl);
   if (rc != CURLE_OK)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("NetworkServiceStatus_SMTP(%hs): call to curl_easy_perform failed (%hs)"), url, errorText);
   }
   *result = CURLCodeToCheckResult(rc);

   curl_slist_free_all(recipients);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Check SMTP/SMTPS service - legacy metric handler
 */
LONG H_CheckSMTP(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char host[256], recipient[256], portAsChar[256];

   if (!AgentGetParameterArgA(param, 1, host, sizeof(host)) ||
       !AgentGetParameterArgA(param, 2, recipient, sizeof(recipient)) ||
       !AgentGetParameterArgA(param, 4, portAsChar, sizeof(portAsChar)))
      return SYSINFO_RC_UNSUPPORTED;

   if ((host[0] == 0) || (recipient[0] == 0))
      return SYSINFO_RC_UNSUPPORTED;

   uint16_t port;
   if (portAsChar[0] == 0)
   {
      port = (arg[1] == 'S') ? 465 : 25;
   }
   else
   {
      port = static_cast<uint16_t>(strtoul(portAsChar, nullptr, 10));
      if (port == 0)
         return SYSINFO_RC_UNSUPPORTED;
   }

   uint32_t timeout = GetTimeoutFromArgs(param, 3);

   LONG rc = SYSINFO_RC_SUCCESS;
   int64_t start = GetCurrentTimeMs();
   int result = CheckSMTP(InetAddress::resolveHostName(host), port, arg[1] == 'S', recipient, timeout);
   if (*arg == 'R')
   {
      if (result == PC_ERR_NONE)
         ret_int64(value, GetCurrentTimeMs() - start);
      else if (g_netsvcFlags & NETSVC_AF_NEGATIVE_TIME_ON_ERROR)
         ret_int64(value, -(GetCurrentTimeMs() - start));
      else
         rc = SYSINFO_RC_ERROR;
   }
   else
   {
      ret_int(value, result);
   }
   return rc;
}

#else /* No SMTP support in libcurl */

/**
 * Check SMTP service (used by command handler and legacy metric handler)
 */
int CheckSMTP(const InetAddress& addr, uint16_t port, bool enableTLS, const char* to, uint32_t timeout)
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("CheckSMTP: agent was built with libcurl version that does not support SMTP protocol"));
   return PC_ERR_INTERNAL;
}

/**
 * Check SMTP service - metric sub-handler
 */
LONG NetworkServiceStatus_SMTP(CURL *curl, const OptionList& options, int *result)
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("NetworkServiceStatus_SMTP: agent was built with libcurl version that does not support SMTP protocol"));
   return SYSINFO_RC_UNSUPPORTED;
}

/**
 * Check SMTP/SMTPS service - legacy metric handler
 */
LONG H_CheckSMTP(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("H_CheckSMTP: agent was built with libcurl version that does not support SMTP protocol"));
   return SYSINFO_RC_UNSUPPORTED;
}

#endif
