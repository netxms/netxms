/* 
** NetXMS - Network Management System
** SMS driver for slack.com service
** Copyright (C) 2014 Raden Solutions
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
#include <nxsrvapi.h>
#include <nms_util.h>
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
static char s_username[64] = "";
static char s_url[1024] = "";

/**
 * Init driver
 */
extern "C" BOOL EXPORT SMSDriverInit(const TCHAR *initArgs)
{
   if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK)
   {
      DbgPrintf(1, _T("Slack: cURL initialization failed"));
      return FALSE;
   }

   DbgPrintf(1, _T("Slack: driver loaded"));
   DbgPrintf(3, _T("cURL version: %hs"), curl_version());
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
   DbgPrintf(3, _T("cURL supported protocols: %hs"), protocols);
#endif

#ifdef UNICODE
   WCHAR buffer[1024];

   ExtractNamedOptionValue(initArgs, _T("url"), buffer, 1024);
   WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, buffer, -1, s_url, 1024, NULL, NULL);

   ExtractNamedOptionValue(initArgs, _T("username"), buffer, 64);
   WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, buffer, -1, s_username, 64, NULL, NULL);
#else
   ExtractNamedOptionValue(initArgs, _T("url"), s_url, 1024);
   ExtractNamedOptionValue(initArgs, _T("username"), s_username, 64);
#endif

	return TRUE;
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
extern "C" BOOL EXPORT SMSDriverSend(const TCHAR *channel, const TCHAR *text)
{
   BOOL success = FALSE;

	DbgPrintf(4, _T("Slack: channel=\"%s\", text=\"%s\""), channel, text);

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
      char *_channel = MBStringFromWideString(channel);
      char *_text = MBStringFromWideString(text);
#else
      char *_channel = strdup(channel);
      char *_text = strdup(_text);
#endif

      json_t *root = json_object();
      if (_channel[0] != 0)
      {
         json_object_set_new(root, "channel", json_string(_channel));
      }
      if (s_username[0] != 0)
      {
         json_object_set_new(root, "username", json_string(s_username));
      }
      json_object_set_new(root, "text", json_string(_text));
      char *json_body = json_dumps(root, 0);

      char request[4096];
      snprintf(request, 4095, "payload=%s", json_body);

      printf(">>>%s<<<\n", request);

      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request);
      json_decref(root);

      free(_channel);
      free(_text);

      if (curl_easy_setopt(curl, CURLOPT_URL, s_url) == CURLE_OK)
      {
         if (curl_easy_perform(curl) == CURLE_OK)
         {
            DbgPrintf(4, _T("Slack: got %d bytes"), data->size);
            if (data->allocated > 0)
            {
               data->data[data->size] = 0;

               if (!strcmp(data->data, "ok"))
               {
                  DbgPrintf(4, _T("Slack: message successfully sent"));
                  success = TRUE;
               }
               else
               {
                  DbgPrintf(4, _T("Slack: got error: %s"), data->data);
               }
            }
         }
         else
         {
            DbgPrintf(4, _T("Slack: call to curl_easy_perform() failed"));
         }
      }
      else
      {
         DbgPrintf(4, _T("Slack: call to curl_easy_setopt(CURLOPT_URL) failed"));
      }
      safe_free(data->data);
      free(data);
      curl_easy_cleanup(curl);
   }
   else
   {
   	DbgPrintf(4, _T("Slack: call to curl_easy_init() failed"));
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
