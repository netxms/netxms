/* 
** NetXMS - Network Management System
** Notification channel driver for Telegram messenger
** Copyright (C) 2014-2019 Raden Solutions
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
** File: slack.cpp
**
**/

#include <ncdrv.h>
#include <nms_util.h>
#include <curl/curl.h>
#include <jansson.h>

#ifndef CURL_MAX_HTTP_HEADER
// workaround for older cURL versions
#define CURL_MAX_HTTP_HEADER CURL_MAX_WRITE_SIZE
#endif

#define DEBUG_TAG _T("ncd.telegram")

/**
 * Request data for cURL call
 */
struct ResponseData
{
   size_t size;
   size_t allocated;
   char *data;
};

/**
 * Telegram driver class
 */
class TelegramDriver : public NCDriver
{
private:
   char m_authToken[64];

public:
   TelegramDriver(Config *config);

   virtual bool send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body) override;
};

/**
 * Callback for processing data received from cURL
 */
static size_t OnCurlDataReceived(char *ptr, size_t size, size_t nmemb, void *context)
{
   ResponseData *response = static_cast<ResponseData*>(context);
   if ((response->allocated - response->size) < (size * nmemb))
   {
      char *newData = MemRealloc(response->data, response->allocated + CURL_MAX_HTTP_HEADER);
      if (newData == NULL)
      {
         return 0;
      }
      response->data = newData;
      response->allocated += CURL_MAX_HTTP_HEADER;
   }

   memcpy(response->data + response->size, ptr, size * nmemb);
   response->size += size * nmemb;

   return size * nmemb;
}

/**
 * Send request to Telegram API
 */
static json_t *SendTelegramRequest(const char *token, const char *method, json_t *data)
{
   CURL *curl = curl_easy_init();
   if (curl == NULL)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_init() failed"));
      return NULL;
   }

#if HAVE_DECL_CURLOPT_NOSIGNAL
   curl_easy_setopt(curl, CURLOPT_NOSIGNAL, (long)1);
#endif

   curl_easy_setopt(curl, CURLOPT_HEADER, (long)0); // do not include header in data
   curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &OnCurlDataReceived);
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);

   ResponseData *responseData = MemAllocStruct<ResponseData>();
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, responseData);

   char *json;
   if (data != NULL)
   {
      char *json = json_dumps(data, 0);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);
   }
   else
   {
      json = NULL;
   }

   json_t *response = NULL;

   char url[256];
   snprintf(url, 256, "https://api.telegram.org/bot%s/%s", token, method);
   if (curl_easy_setopt(curl, CURLOPT_URL, url) == CURLE_OK)
   {
      if (curl_easy_perform(curl) == CURLE_OK)
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("Got %d bytes"), responseData->size);
         if (responseData->allocated > 0)
         {
            responseData->data[responseData->size] = 0;
            json_error_t error;
            response = json_loads(responseData->data, 0, &error);
            if (response == NULL)
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot parse API response (%hs)"), error.text);
            }
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_perform() failed"));
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_setopt(CURLOPT_URL) failed"));
   }
   MemFree(responseData->data);
   MemFree(responseData);
   curl_easy_cleanup(curl);
   return response;
}

/**
 * Init driver
 */
TelegramDriver::TelegramDriver(Config *config)
{
   m_authToken[0] = 0;

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Driver loaded"));
   nxlog_debug_tag(DEBUG_TAG, 3, _T("cURL version: %hs"), curl_version());
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
   nxlog_debug_tag(DEBUG_TAG, 3, _T("cURL supported protocols: %hs"), protocols);
#endif

   NX_CFG_TEMPLATE configTemplate[] = 
	{
		{ _T("AuthToken"), CT_MB_STRING, 0, 0, sizeof(m_authToken), 0, m_authToken },
		{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
	};

	config->parseTemplate(_T("Telegram"), configTemplate);

	json_t *info = SendTelegramRequest(m_authToken, "getMe", NULL);
	if (info != NULL)
	{
	   json_t *ok = json_object_get(info, "ok");
	   if (json_is_true(ok))
	   {
	      nxlog_debug_tag(DEBUG_TAG, 1, _T("Received valid API response"));
	      json_t *result = json_object_get(info, "result");
	      if (json_is_object(result))
	      {
	         json_t *name = json_object_get(result, "first_name");
	         if (json_is_string(name))
	         {
	            nxlog_debug_tag(DEBUG_TAG, 1, _T("Bot name: %hs"), json_string_value(name));
	         }
	      }
	   }
	   else
	   {
	      json_t *d = json_object_get(info, "description");
	      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Telegram API call failed (%hs), driver configuration could be incorrect"),
	               json_is_string(d) ? json_string_value(d) : "Unknown reason");
	   }
	   json_decref(info);
	}
	else
	{
	   nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Telegram API call failed, driver configuration could be incorrect"));
	}
}

/**
 * Send SMS
 */
bool TelegramDriver::send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body)
{
   bool success = false;

   nxlog_debug_tag(DEBUG_TAG, 4, _T("recipient=\"%s\", text=\"%s\""), recipient, body);

   json_t *request = json_object();
   json_object_set_new(request, "chat_id", json_string_t(recipient));
   json_object_set_new(request, "text", json_string_t(body));

   json_t *response = SendTelegramRequest(m_authToken, "sendMessage", request);
   json_decref(request);

   if (json_is_object(response))
   {

   }

   return success;
}

/**
 * Driver entry point
 */
DECLARE_NCD_ENTRY_POINT(Telegram, NULL)
{
   if (!InitializeLibCURL())
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("cURL initialization failed"));
      return NULL;
   }
   return new TelegramDriver(config);
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
