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
 * Check POP3/POP3S service
 */
int CheckPOP3(const InetAddress& addr, uint16_t port, bool enableTLS, const char *username, const char *password, uint32_t timeout)
{
   CURL *curl = PrepareCurlHandle(addr, port, enableTLS ? "pop3s" : "pop3", timeout);
   if (curl == nullptr)
      return PC_ERR_BAD_PARAMS;

   curl_easy_setopt(curl, CURLOPT_USERNAME, username);
   curl_easy_setopt(curl, CURLOPT_PASSWORD, password);

   char errorText[CURL_ERROR_SIZE] = "";
   curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorText);

   CURLcode rc = curl_easy_perform(curl);
   if (rc != CURLE_OK)
   {
      TCHAR addrText[64];
      nxlog_debug_tag(DEBUG_TAG, 6, _T("CheckPOP3(%s//%s:%d): call to curl_easy_perform failed (%d: %hs)"), enableTLS ? _T("pop3s") : _T("pop3"), addr.toString(addrText), port, rc, errorText);
   }
   curl_easy_cleanup(curl);
   return CURLCodeToCheckResult(rc);
}

/**
 * Check POP3/POP3S service - legacy metric handler
 */
LONG H_CheckPOP3(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char hostname[256];
   char username[256];
   char password[256];
   char portText[256];

   if (!AgentGetParameterArgA(param, 1, hostname, sizeof(hostname)) ||
       !AgentGetParameterArgA(param, 2, username, sizeof(username)) ||
       !AgentGetParameterArgA(param, 3, password, sizeof(password)) ||
       !AgentGetParameterArgA(param, 5, portText, sizeof(portText)))
      return SYSINFO_RC_UNSUPPORTED;

   if (hostname[0] == 0 || username[0] == 0 || password[0] == 0)
      return SYSINFO_RC_UNSUPPORTED;

   uint16_t port;
   if (portText[0] == 0)
   {
      port = (arg[1] == 'S') ? 995 : 110;
   }
   else
   {
      port = static_cast<uint16_t>(strtoul(portText, nullptr, 10));
      if (port == 0)
         return SYSINFO_RC_UNSUPPORTED;
   }

   int64_t start = GetCurrentTimeMs();

   uint32_t timeout = GetTimeoutFromArgs(param, 4);

   LONG ret = SYSINFO_RC_SUCCESS;
   int result = CheckPOP3(InetAddress::resolveHostName(hostname), port, arg[1] == 'S', username, password, timeout);
   if (*arg == 'R')
   {
      if (result == PC_ERR_NONE)
         ret_int64(value, GetCurrentTimeMs() - start);
      else if (g_netsvcFlags & NETSVC_AF_NEGATIVE_TIME_ON_ERROR)
         ret_int64(value, -(GetCurrentTimeMs() - start));
      else
         ret = SYSINFO_RC_ERROR;
   }
   else
   {
      ret_int(value, result);
   }
   return ret;
}
