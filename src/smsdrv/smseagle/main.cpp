/* 
** NetXMS - Network Management System
** SMS driver for SMSEagle Hardware SMS Gateway
** 
** SMSEagle API documentation - https://www.smseagle.eu/api/
**
** Copyright (C) 2014-2016 Raden Solutions
** Copyright (C) 2016 TEMPEST a.s.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: main.cpp
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <nxconfig.h>
#include <curl/curl.h>

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

#ifndef CURL_MAX_HTTP_HEADER
// workaround for older cURL versions
#define CURL_MAX_HTTP_HEADER CURL_MAX_WRITE_SIZE
#endif

/**
 * Request data for cURL call
 */
struct RequestData
{
   size_t size;
   size_t allocated;
   char *data;
};

/**
 * Configuration
 */
static char s_hostname[128] = "127.0.0.1";
static int s_port = 80;
static char s_login[128] = "user";
static char s_password[128] = "password";
static bool s_useHttps = false;

/**
 * Init driver
 */
extern "C" bool EXPORT SMSDriverInit(const TCHAR *initArgs, Config *config)
{
   if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK)
   {
      nxlog_debug(1, _T("SMSEagle: cURL initialization failed"));
      return false;
   }

   nxlog_debug(1, _T("SMSEagle: driver loaded"));
   nxlog_debug(3, _T("cURL version: %hs"), curl_version());
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
   nxlog_debug(3, _T("cURL supported protocols: %hs"), protocols);
#endif

#ifdef UNICODE
   char initArgsA[1024];
   WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, initArgs, -1, initArgsA, 1024, NULL, NULL);
#define realInitArgs initArgsA
#else
#define realInitArgs initArgs
#endif

   ExtractNamedOptionValueA(realInitArgs, "host", s_hostname, 128);
   s_port = (int)ExtractNamedOptionValueAsIntA(realInitArgs, "port", s_port);
   ExtractNamedOptionValueA(realInitArgs, "login", s_login, 128);
   ExtractNamedOptionValueA(realInitArgs, "password", s_password, 128);
   s_useHttps = ExtractNamedOptionValueAsBoolA(realInitArgs, "https", false);

   return true;
}

/**
 * Callback for processing data received from cURL
 */
static size_t OnCurlDataReceived(char *ptr, size_t size, size_t nmemb, void *userdata)
{
   RequestData *data = (RequestData *)userdata;
   if ((data->allocated - data->size) < (size * nmemb))
   {
      char *newData = (char *)realloc(data->data, data->allocated + CURL_MAX_HTTP_HEADER);
      if (newData == NULL)
      {
         return 0;
      }
      data->data = newData;
      data->allocated += CURL_MAX_HTTP_HEADER;
   }

   memcpy(data->data + data->size, ptr, size * nmemb);
   data->size += size * nmemb;

   return size * nmemb;
}

/**
 * Send SMS
 */
extern "C" bool EXPORT SMSDriverSend(const TCHAR *phoneNumber, const TCHAR *text)
{
   bool success = false;

   nxlog_debug(4, _T("SMSEagle: phone/group=\"%s\", text=\"%s\""), phoneNumber, text);

   CURL *curl = curl_easy_init();
   if (curl != NULL)
   {
#if HAVE_DECL_CURLOPT_NOSIGNAL
      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, (long)1);
#endif

      curl_easy_setopt(curl, CURLOPT_HEADER, (long)0); // do not include header in data
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &OnCurlDataReceived);

      RequestData *data = (RequestData *)malloc(sizeof(RequestData));
      memset(data, 0, sizeof(RequestData));
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);

      bool intlPrefix = (phoneNumber[0] == _T('+')); // this will be used to decide whether it is number or group name
#ifdef UNICODE
      char *mbphone = MBStringFromWideString(phoneNumber);
      char *mbmsg = MBStringFromWideString(text);
      char *phone = curl_easy_escape(curl, mbphone, 0);
      char *msg = curl_easy_escape(curl, mbmsg, 0);
      free(mbphone);
      free(mbmsg);
#else
      char *phone = curl_easy_escape(curl, phoneNumber, 0);
      char *msg = curl_easy_escape(curl, text, 0);
#endif

      char url[4096];
      snprintf(url, 4096,
               intlPrefix ? "%s://%s:%d/index.php/http_api/send_sms?login=%s&pass=%s&to=%s&message=%s" : "%s://%s:%d/index.php/http_api/send_togroup?login=%s&pass=%s&groupname=%s&message=%s",
               s_useHttps ? "https" : "http", s_hostname, s_port, s_login, s_password, phone, msg);
      nxlog_debug(4, _T("SMSEagle: URL set to \"%hs\""), url);

      curl_free(phone);
      curl_free(msg);

      if (curl_easy_setopt(curl, CURLOPT_URL, url) == CURLE_OK)
      {
         if (curl_easy_perform(curl) == CURLE_OK)
         {
            nxlog_debug(4, _T("SMSEagle: %d bytes received"), data->size);
            if (data->allocated > 0)
               data->data[data->size] = 0;

            long response = 500;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response);
            nxlog_debug(4, _T("SMSEagle: response code %03d"), (int)response);
            if (response == 200)
            {
               success = true;
            }
         }
         else
         {
         	nxlog_debug(4, _T("SMSEagle: call to curl_easy_perform() failed"));
         }
      }
      else
      {
      	nxlog_debug(4, _T("SMSEagle: call to curl_easy_setopt(CURLOPT_URL) failed"));
      }
      safe_free(data->data);
      free(data);
      curl_easy_cleanup(curl);
   }
   else
   {
   	nxlog_debug(4, _T("SMSEagle: call to curl_easy_init() failed"));
   }

   return success;
}

/**
 * Unload driver
 */
extern "C" void EXPORT SMSDriverUnload()
{
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
