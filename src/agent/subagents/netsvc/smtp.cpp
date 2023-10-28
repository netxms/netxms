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
 * Read callback for curl
 */
static size_t ReadCallback(char *buffer, size_t size, size_t nitems, void *data)
{
   return static_cast<ByteStream*>(data)->read(buffer, size * nitems);
}

/**
 * Encode SMTP header
 */
static char *EncodeHeader(const char *header, const char *data, char *buffer, size_t bufferSize)
{
   bool encode = false;
   for(const char *p = data; *p != 0; p++)
      if (*p & 0x80)
      {
         encode = true;
         break;
      }
   if (encode)
   {
      char *encodedData = nullptr;
      base64_encode_alloc(data, strlen(data), &encodedData);
      if (encodedData != nullptr)
      {
         if (header != nullptr)
            snprintf(buffer, bufferSize, "%s: =?utf8?B?%s?=\r\n", header, encodedData);
         else
            snprintf(buffer, bufferSize, "=?utf8?B?%s?=", encodedData);
         free(encodedData);
      }
      else
      {
         // fallback
         if (header != nullptr)
            snprintf(buffer, bufferSize, "%s: %s\r\n", header, data);
         else
            strlcpy(buffer, data, bufferSize);
      }
   }
   else
   {
      if (header != nullptr)
         snprintf(buffer, bufferSize, "%s: %s\r\n", header, data);
      else
         strlcpy(buffer, data, bufferSize);
   }
   return buffer;
}

/**
 * Prepare mail body
 */
static void PrepareMailBody(ByteStream *data, const char *sender, const char *recipient, const char *subject, const char *body)
{
   char buffer[1204];

   // Mail headers
   snprintf(buffer, sizeof(buffer), "From: <%s>\r\n", sender);
   data->writeString(buffer, strlen(buffer), false, false);

   snprintf(buffer, sizeof(buffer), "To: <%s>\r\n", recipient);
   data->writeString(buffer, strlen(buffer), false, false);

   EncodeHeader("Subject", subject, buffer, sizeof(buffer));
   data->writeString(buffer, strlen(buffer), false, false);

   // date
   time_t currentTime;
   struct tm *pCurrentTM;
   time(&currentTime);
#ifdef HAVE_LOCALTIME_R
   struct tm currentTM;
   localtime_r(&currentTime, &currentTM);
   pCurrentTM = &currentTM;
#else
   pCurrentTM = localtime(&currentTime);
#endif
#ifdef _WIN32
   strftime(buffer, sizeof(buffer), "Date: %a, %d %b %Y %H:%M:%S ", pCurrentTM);

   TIME_ZONE_INFORMATION tzi;
   uint32_t tzType = GetTimeZoneInformation(&tzi);
   LONG effectiveBias;
   switch(tzType)
   {
      case TIME_ZONE_ID_STANDARD:
         effectiveBias = tzi.Bias + tzi.StandardBias;
         break;
      case TIME_ZONE_ID_DAYLIGHT:
         effectiveBias = tzi.Bias + tzi.DaylightBias;
         break;
      case TIME_ZONE_ID_UNKNOWN:
         effectiveBias = tzi.Bias;
         break;
      default:    // error
         effectiveBias = 0;
         nxlog_debug_tag(DEBUG_TAG, 4, _T("GetTimeZoneInformation() call failed"));
         break;
   }
   int offset = abs(effectiveBias);
   sprintf(&buffer[strlen(buffer)], "%c%02d%02d\r\n", effectiveBias <= 0 ? '+' : '-', offset / 60, offset % 60);
#else
   strftime(buffer, sizeof(buffer), "Date: %a, %d %b %Y %H:%M:%S %z\r\n", pCurrentTM);
#endif
   data->writeString(buffer, strlen(buffer), false, false);

   // content-type
   static char contentTypeHeader[] = "Content-Type: text/plain; charset=utf8\r\nContent-Transfer-Encoding: 8bit\r\n\r\n";
   data->writeString(contentTypeHeader, strlen(contentTypeHeader), false, false);

   // Mail body
   data->writeString(body, strlen(body), false, false);
   data->writeString("\r\n", 2, false, false);
}

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

   ByteStream data;
   PrepareMailBody(&data, from, to, "NetXMS Test Message", "Test message");
   data.seek(0);
   curl_easy_setopt(curl, CURLOPT_READFUNCTION, ReadCallback);
   curl_easy_setopt(curl, CURLOPT_READDATA, &data);
   curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

   CURLcode rc = curl_easy_perform(curl);
   if (rc != CURLE_OK)
   {
      TCHAR addrText[64];
      nxlog_debug_tag(DEBUG_TAG, 6, _T("CheckSMTP(%s//%s:%d): call to curl_easy_perform failed (%d: %hs)"), enableTLS ? _T("smtps") : _T("smtp"), addr.toString(addrText), port, rc, errorText);
   }
   int result = CURLCodeToCheckResult(rc);

   curl_slist_free_all(recipients);
   curl_easy_cleanup(curl);
   return result;
}

/**
 * Check SMTP service - metric sub-handler
 */
LONG NetworkServiceStatus_SMTP(CURL *curl, const OptionList& options, const char *url, int *result)
{
   char from[128], to[128], subject[256], text[256];
   tchar_to_utf8(options.get(_T("from"), _T("")), -1, from, 128);
   tchar_to_utf8(options.get(_T("to"), _T("")), -1, to, 128);
   tchar_to_utf8(options.get(_T("subject"), _T("")), -1, subject, 256);
   tchar_to_utf8(options.get(_T("text"), _T("")), -1, text, 256);

   if (to[0] == 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("NetworkServiceStatus_SMTP(%hs): destination address not provided"), url);
      return SYSINFO_RC_UNSUPPORTED;
   }

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

   ByteStream data;
   PrepareMailBody(&data, from, to, subject, text);
   data.seek(0);
   curl_easy_setopt(curl, CURLOPT_READFUNCTION, ReadCallback);
   curl_easy_setopt(curl, CURLOPT_READDATA, &data);
   curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

   CURLcode rc = curl_easy_perform(curl);
   if (rc != CURLE_OK)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("NetworkServiceStatus_SMTP(%hs): call to curl_easy_perform failed (%d: %hs)"), url, rc, errorText);
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
