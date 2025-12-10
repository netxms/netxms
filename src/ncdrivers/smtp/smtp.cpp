/*
** NetXMS - Network Management System
** Notification driver for SMTP protocol
** Copyright (C) 2019-2024 Raden Solutions
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
** File: smtp.cpp
**
**/

#include <ncdrv.h>
#include <nms_util.h>
#include <nxcldefs.h>
#include <nxlibcurl.h>

#define DEBUG_TAG _T("ncd.smtp")

/**
 * TLS usage mode
 */
enum class TLSMode
{
   NONE,    // No TLS
   TLS,     // Enforced TLS
   STARTTLS // Opportunistic TLS
};

/**
 * SMTP driver class
 */
class SmtpDriver : public NCDriver
{
private:
   char m_server[MAX_DNS_NAME];
   uint16_t m_port;
   char m_login[128];
   char m_password[128];
   char m_fromName[256];
   char m_fromAddr[256];
   bool m_isHtml;
   TLSMode m_tlsMode;

   SmtpDriver();

   void prepareMailBody(ByteStream *data, const char *recipient, const TCHAR *subject, const TCHAR *body);

public:
   virtual int send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body) override;

   static SmtpDriver *createInstance(Config *config);
};

/**
 * Create driver instance
 */
SmtpDriver *SmtpDriver::createInstance(Config *config)
{
   SmtpDriver *driver = new SmtpDriver();

   TCHAR tlsModeBuff[9] = _T("NONE");

   NX_CFG_TEMPLATE configTemplate[] = {
      { _T("FromAddr"), CT_MB_STRING, 0, 0, sizeof(driver->m_fromAddr), 0, driver->m_fromAddr },
      { _T("FromName"), CT_MB_STRING, 0, 0, sizeof(driver->m_fromName), 0, driver->m_fromName },
      { _T("IsHTML"), CT_BOOLEAN, 0, 0, 1, 0, &driver->m_isHtml },
      { _T("Login"), CT_MB_STRING, 0, 0, sizeof(driver->m_login), 0, driver->m_login },
      { _T("Password"), CT_MB_STRING, 0, 0, sizeof(driver->m_password), 0, driver->m_password },
      { _T("Port"), CT_WORD, 0, 0, 0, 0, &driver->m_port },
      { _T("Server"), CT_MB_STRING, 0, 0, sizeof(driver->m_server), 0, driver->m_server },
      { _T("TLSMode"), CT_STRING, 0, 0, 9, 0, tlsModeBuff },
      { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr }
   };

   if (!config->parseTemplate(_T("SMTP"), configTemplate))
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Error parsing driver configuration"));
      delete driver;
      return nullptr;
   }

   if (!_tcsicmp(tlsModeBuff, _T("TLS")))
   {
      driver->m_tlsMode = TLSMode::TLS;
   }
   else if (!_tcsicmp(tlsModeBuff, _T("STARTTLS")))
   {
      driver->m_tlsMode = TLSMode::STARTTLS;
   }
   else if (_tcsicmp(tlsModeBuff, _T("NONE")))
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Invalid TLS mode"));
      delete driver;
      return nullptr;
   }

   if (driver->m_port == 0)
   {
      driver->m_port = driver->m_tlsMode == TLSMode::TLS ? 465 : 25;
   }

   if ((driver->m_login[0] != 0) && (driver->m_password[0] != 0))
      DecryptPasswordA(driver->m_login, driver->m_password, driver->m_password, sizeof(driver->m_password));

   return driver;
}

/**
 * Constructor for SMTP diver
 */
SmtpDriver::SmtpDriver()
{
   strcpy(m_server, "localhost");
   m_port = 0;
   m_login[0] = 0;
   m_password[0] = 0;
   strcpy(m_fromName, "NetXMS Server");
   strcpy(m_fromAddr, "netxms@localhost");
   m_isHtml = false;
   m_tlsMode = TLSMode::NONE;
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
            snprintf(buffer, bufferSize, "%s: =?utf-8?B?%s?=\r\n", header, encodedData);
         else
            snprintf(buffer, bufferSize, "=?utf-8?B?%s?=", encodedData);
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
void SmtpDriver::prepareMailBody(ByteStream *data, const char *recipient, const TCHAR *subject, const TCHAR *body)
{
   char buffer[1204];

   // Mail headers
   char from[512];
   snprintf(buffer, sizeof(buffer), "From: \"%s\" <%s>\r\n", EncodeHeader(nullptr, m_fromName, from, 512), m_fromAddr);
   data->writeString(buffer, strlen(buffer), false, false);

   snprintf(buffer, sizeof(buffer), "To: <%s>\r\n", recipient);
   data->writeString(buffer, strlen(buffer), false, false);

   char utf8subject[1024];
#ifdef UNICODE
   wchar_to_utf8(subject, -1, utf8subject, sizeof(utf8subject));
#else
   mb_to_utf8(subject, -1, utf8subject, sizeof(utf8subject));
#endif
   EncodeHeader("Subject", utf8subject, buffer, sizeof(buffer));
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
   snprintf(buffer, sizeof(buffer),
         "Content-Type: text/%s; charset=utf-8\r\n"
         "Content-Transfer-Encoding: 8bit\r\n\r\n", m_isHtml ? "html" : "plain");
   data->writeString(buffer, strlen(buffer), false, false);

   // Mail body
   char *utf8body = UTF8StringFromTString(body);
   data->writeString(utf8body, strlen(utf8body), false, false);
   MemFree(utf8body);
   data->writeString("\r\n", 2, false, false);
}

/**
 * Read callback for curl
 */
static size_t ReadCallback(char *buffer, size_t size, size_t nitems, void *data)
{
   return static_cast<ByteStream*>(data)->read(buffer, size * nitems);
}

/**
 * Driver send method
 */
int SmtpDriver::send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body)
{
#ifdef CURLPROTO_SMTP
   CURL *curl = curl_easy_init();
   if (curl == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_init() failed"));
      return -1;
   }

   char rcptTo[MAX_RCPT_ADDR_LEN];
#ifdef UNICODE
   wchar_to_utf8(recipient, -1, rcptTo, MAX_RCPT_ADDR_LEN);
#else
   mb_to_utf8(recipient, -1, rcptTo, MAX_RCPT_ADDR_LEN);
#endif
   struct curl_slist *recipients = curl_slist_append(nullptr, rcptTo);
   curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

   curl_easy_setopt(curl, CURLOPT_MAIL_FROM, m_fromAddr);
   curl_easy_setopt(curl, CURLOPT_USE_SSL, (m_tlsMode == TLSMode::NONE) ? (long)CURLUSESSL_NONE : (long)CURLUSESSL_ALL);

   if (m_tlsMode != TLSMode::NONE)
   {
      EnableLibCURLUnexpectedEOFWorkaround(curl);
   }

   if ((m_login[0] != 0) && (m_password[0] != 0))
   {
      curl_easy_setopt(curl, CURLOPT_USERNAME, m_login);
      curl_easy_setopt(curl, CURLOPT_PASSWORD, m_password);
   }

   ByteStream mailBody;
   prepareMailBody(&mailBody, rcptTo, subject, body);
   mailBody.seek(0, SEEK_SET);

   curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
   curl_easy_setopt(curl, CURLOPT_READFUNCTION, ReadCallback);
   curl_easy_setopt(curl, CURLOPT_READDATA, &mailBody);

   char errorBuffer[CURL_ERROR_SIZE];
   curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);

   int result = 0;

   char url[1024];
   snprintf(url, 1024, "%s://%s:%d", (m_tlsMode == TLSMode::TLS) ? "smtps" : "smtp", m_server, m_port);
   if (curl_easy_setopt(curl, CURLOPT_URL, url) != CURLE_OK)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_setopt(CURLOPT_URL, \"%hs\") failed"), url);
      result = -1;
   }

   if (result == 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Sending mail with url=\"%hs\", to=\"%s\", subject=\"%s\", login=\"%hs\""), url, recipient, subject, m_login);
      CURLcode rc = curl_easy_perform(curl);
      if (rc != CURLE_OK)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_perform(\"%hs\") failed (%d: %hs)"), url, rc, (errorBuffer[0] != 0) ? errorBuffer : curl_easy_strerror(rc));
         if ((rc == CURLE_COULDNT_CONNECT) || (rc == CURLE_OPERATION_TIMEDOUT) || (rc == CURLE_SEND_ERROR) || (rc == CURLE_RECV_ERROR))
            result = 30;   // Retry in 30 seconds
         else
            result = -1;
      }
   }

   curl_slist_free_all(recipients);
   curl_easy_cleanup(curl);

   return result;
#else
   return -1;
#endif
}

/**
 * Configuration template
 */
static const NCConfigurationTemplate s_config(true, true);

/**
 * Driver entry point
 */
DECLARE_NCD_ENTRY_POINT(SMTP, &s_config)
{
#ifndef CURLPROTO_SMTP
   nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Server was built with libcurl version that does not support SMTP protocol"));
   return nullptr;
#endif

   if (!InitializeLibCURL())
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("cURL initialization failed"));
      return nullptr;
   }

   return SmtpDriver::createInstance(config);
}

#ifdef _WIN32

/**
 * DLL Entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
