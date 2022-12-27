/*
** NetXMS - Network Management System
** Copyright (C) 2003-2022 Raden Solutions
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

   int result = CURLCodeToCheckResult(curl_easy_perform(curl));

   curl_slist_free_all(recipients);
   return result;
}

/**
 * Check SMTP service - metric sub-handler
 */
LONG NetworkServiceStatus_SMTP(CURL *curl, const OptionList& options, int *result)
{
   char from[128], to[128];
   wchar_to_utf8(options.get(_T("from"), _T("")), -1, from, 128);
   wchar_to_utf8(options.get(_T("to"), _T("")), -1, to, 128);

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

   *result = CURLCodeToCheckResult(curl_easy_perform(curl));

   curl_slist_free_all(recipients);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Check SMTP/SMTPS service - legacy metric handler
 */
LONG H_CheckSMTP(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char host[256], recipient[256], portAsChar[256];
   uint16_t port;

   AgentGetParameterArgA(param, 1, host, sizeof(host));
   AgentGetParameterArgA(param, 2, recipient, sizeof(recipient));
   uint32_t timeout = GetTimeoutFromArgs(param, 3);
   AgentGetParameterArgA(param, 4, portAsChar, sizeof(portAsChar));

   if ((host[0] == 0) || (recipient[0] == 0))
      return SYSINFO_RC_UNSUPPORTED;

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
