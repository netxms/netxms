/* 
** NetXMS - Network Management System
** SMS driver for Nexmo gateway
** Copyright (C) 2014-2016 Raden Solutions
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
#include <jansson.h>

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
static char s_apiKey[128] = "key";
static char s_apiSecret[128] = "secret";
static char s_from[128] = "NetXMS";

/**
 * Init driver
 */
extern "C" bool EXPORT SMSDriverInit(const TCHAR *initArgs, Config *config)
{
   if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK)
   {
      nxlog_debug(1, _T("Nexmo: cURL initialization failed"));
      return false;
   }

   nxlog_debug(1, _T("Nexmo: driver loaded"));
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

   ExtractNamedOptionValueA(realInitArgs, "apiKey", s_apiKey, 128);
   ExtractNamedOptionValueA(realInitArgs, "apiSecret", s_apiSecret, 128);
   ExtractNamedOptionValueA(realInitArgs, "from", s_from, 128);

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
 * Parse Nexmo server response
 */
static bool ParseResponse(const char *response)
{
   json_error_t error;
   json_t *root = json_loads(response, 0, &error);
   if (root == NULL)
   {
      nxlog_debug(4, _T("Nexmo: cannot parse response JSON"));
      return false;
   }

   bool success = false;
   json_t *messages = json_object_get(root, "messages");
   if (messages != NULL)
   {
      size_t count = json_array_size(messages);
      if (count > 0)
      {
         success = true;
         for(size_t i = 0; i < count; i++)
         {
            json_t *m = json_array_get(messages, i);
            if ((m == NULL) || !json_is_object(m))
            {
               nxlog_debug(4, _T("Nexmo: invalid response (\"messages\"[%d] is invalid)"), (int)i);
               success = false;
               break;
            }
            json_t *status = json_object_get(m, "status");
            if ((status == NULL) || !json_is_string(status))
            {
               nxlog_debug(4, _T("Nexmo: invalid response (\"status\" is missing or invalid for \"messages\"[%d])"), (int)i);
               success = false;
               break;
            }
            char *eptr;
            int statusCode = (int)strtol(json_string_value(status), &eptr, 10);
            if (*eptr != 0)
            {
               nxlog_debug(4, _T("Nexmo: invalid response (\"status\" is not an integer for \"messages\"[%d])"), (int)i);
               success = false;
               break;
            }
            if (statusCode != 0)
            {
               nxlog_debug(4, _T("Nexmo: message sending error (status code %d for part %d)"), statusCode, (int)i);
               success = false;
               break;
            }
         }
      }
      else
      {
         nxlog_debug(4, _T("Nexmo: invalid response (\"messages\" is empty or not an array)"));
      }
   }
   else
   {
      nxlog_debug(4, _T("Nexmo: invalid response (missing \"messages\" part)"));
   }

   json_decref(root);
   return success;
}

/**
 * Send SMS
 */
extern "C" bool EXPORT SMSDriverSend(const TCHAR *phoneNumber, const TCHAR *text)
{
   bool success = false;

   nxlog_debug(4, _T("Nexmo: phone=\"%s\", text=\"%s\""), phoneNumber, text);

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

#ifdef UNICODE
      char *mbphone = MBStringFromWideString(phoneNumber);
      char *mbmsg = MBStringFromWideString(text);
      char *phone = curl_easy_escape(curl, mbphone, 0);
      char *from = curl_easy_escape(curl, s_from, 0);
      char *msg = curl_easy_escape(curl, mbmsg, 0);
      free(mbphone);
      free(mbmsg);
#else
      char *phone = curl_easy_escape(curl, phoneNumber, 0);
      char *from = curl_easy_escape(curl, s_from, 0);
      char *msg = curl_easy_escape(curl, text, 0);
#endif

      char url[4096];
      snprintf(url, 4096, "https://rest.nexmo.com/sms/json?api_key=%s&api_secret=%s&to=%s&from=%s&text=%s",
               s_apiKey, s_apiSecret, phone, from, msg);
      nxlog_debug(7, _T("Nexmo: URL set to \"%hs\""), url);

      curl_free(phone);
      curl_free(from);
      curl_free(msg);

      if (curl_easy_setopt(curl, CURLOPT_URL, url) == CURLE_OK)
      {
         if (curl_easy_perform(curl) == CURLE_OK)
         {
            nxlog_debug(4, _T("Nexmo: %d bytes received"), data->size);
            if (data->allocated > 0)
               data->data[data->size] = 0;

            long response = 500;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response);
            nxlog_debug(4, _T("Nexmo: response code %03d"), (int)response);
            if (response == 200)
            {
               success = ParseResponse(data->data);
            }
         }
         else
         {
         	nxlog_debug(4, _T("Nexmo: call to curl_easy_perform() failed"));
         }
      }
      else
      {
      	nxlog_debug(4, _T("Nexmo: call to curl_easy_setopt(CURLOPT_URL) failed"));
      }
      safe_free(data->data);
      free(data);
      curl_easy_cleanup(curl);
   }
   else
   {
   	nxlog_debug(4, _T("Nexmo: call to curl_easy_init() failed"));
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
