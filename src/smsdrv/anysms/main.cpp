/* 
** NetXMS - Network Management System
** SMS driver for any-sms.biz service
** Copyright (C) 2014-2017 Raden Solutions
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
static char s_login[128] = "user";
static char s_password[128] = "password";
static char s_sender[64] = "NETXMS";
static char s_gateway[64] = "28";

/**
 * Init driver
 */
extern "C" bool EXPORT SMSDriverInit(const TCHAR *initArgs, Config *config)
{
   if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK)
   {
      nxlog_debug(1, _T("AnySMS: cURL initialization failed"));
      return false;
   }

   nxlog_debug(1, _T("AnySMS: driver loaded"));
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
   WCHAR buffer[128];

   if (ExtractNamedOptionValue(initArgs, _T("login"), buffer, 128))
      WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, buffer, -1, s_login, 128, NULL, NULL);

   if (ExtractNamedOptionValue(initArgs, _T("password"), buffer, 128))
      WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, buffer, -1, s_password, 128, NULL, NULL);

   if (ExtractNamedOptionValue(initArgs, _T("sender"), buffer, 64))
      WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, buffer, -1, s_sender, 64, NULL, NULL);

   if (ExtractNamedOptionValue(initArgs, _T("gateway"), buffer, 64))
      WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, buffer, -1, s_gateway, 64, NULL, NULL);
#else
   ExtractNamedOptionValue(initArgs, _T("login"), s_login, 128);
   ExtractNamedOptionValue(initArgs, _T("password"), s_password, 128);
   ExtractNamedOptionValue(initArgs, _T("sender"), s_sender, 64);
   ExtractNamedOptionValue(initArgs, _T("gateway"), s_gateway, 64);
#endif

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

	nxlog_debug(4, _T("AnySMS: phone=\"%s\", text=\"%s\""), phoneNumber, text);

   CURL *curl = curl_easy_init();
   if (curl != NULL)
   {
#if HAVE_DECL_CURLOPT_NOSIGNAL
      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, (long)1);
#endif

      curl_easy_setopt(curl, CURLOPT_HEADER, (long)0); // do not include header in data
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &OnCurlDataReceived);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);

      RequestData *data = (RequestData *)malloc(sizeof(RequestData));
      memset(data, 0, sizeof(RequestData));
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);

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
      snprintf(url, 4096, "http://gateway.any-sms.biz/send_sms.php?id=%s&pass=%s&text=%s&nummer=%s&gateway=%s&absender=%s",
               s_login, s_password, msg, phone, s_gateway, s_sender);
      nxlog_debug(4, _T("AnySMS: URL set to \"%hs\""), url);

      curl_free(phone);
      curl_free(msg);

      if (curl_easy_setopt(curl, CURLOPT_URL, url) == CURLE_OK)
      {
         if (curl_easy_perform(curl) == CURLE_OK)
         {
            nxlog_debug(4, _T("AnySMS: %d bytes received"), data->size);
            if (data->allocated > 0)
            {
               data->data[data->size] = 0;
               if (!strncmp(data->data, "err:", 4))
               {
                  int code = strtol(data->data + 4, NULL, 10);
                  if (code == 0)
                  {
               	   nxlog_debug(4, _T("AnySMS: success"));
                     success = true;
                  }
                  else
                  {
               	   nxlog_debug(4, _T("AnySMS: error response (%d)"), code);
                  }
               }
               else
               {
               	nxlog_debug(4, _T("AnySMS: unexpected response (%hs)"), data->data);
               }
            }
         }
         else
         {
         	nxlog_debug(4, _T("AnySMS: call to curl_easy_perform() failed"));
         }
      }
      else
      {
      	nxlog_debug(4, _T("AnySMS: call to curl_easy_setopt(CURLOPT_URL) failed"));
      }
      safe_free(data->data);
      free(data);
      curl_easy_cleanup(curl);
   }
   else
   {
   	nxlog_debug(4, _T("AnySMS: call to curl_easy_init() failed"));
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
