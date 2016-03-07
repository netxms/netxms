/* 
** NetXMS - Network Management System
** SMS driver for Text2Reach.com service
** Copyright (C) 2014-2016 Raden Solutions
**
** Text2Reach API documentation - http://www.text2reach.com/files/text2reach_bulkapi_v1.14.pdf
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
static char s_apikey[128] = "apikey";
static char s_from[128] = "from";
static char s_unicode = true; // if set to false, characters not in GSM 7bit alphabet will be converted (ā => a) or removed
static char s_blacklist = false; // Checks if the number is in blacklist before sending. Returns error -503 if found.

/**
 * Init driver
 */
extern "C" bool EXPORT SMSDriverInit(const TCHAR *initArgs, Config *config)
{
   if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK)
   {
      nxlog_debug(1, _T("Text2Reach: cURL initialization failed"));
      return false;
   }

   nxlog_debug(1, _T("Text2Reach: driver loaded"));
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

   ExtractNamedOptionValue(initArgs, _T("apikey"), buffer, 128);
   WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, buffer, -1, s_apikey, 128, NULL, NULL);

   ExtractNamedOptionValue(initArgs, _T("from"), buffer, 128);
   WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, buffer, -1, s_from, 128, NULL, NULL);
#else
   ExtractNamedOptionValue(initArgs, _T("apikey"), s_apikey, 128);
   ExtractNamedOptionValue(initArgs, _T("from"), s_from, 128);
#endif

   s_unicode = ExtractNamedOptionValueAsBool(initArgs, _T("unicode"), true);
   s_blacklist = ExtractNamedOptionValueAsBool(initArgs, _T("blacklist"), false);

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

   nxlog_debug(4, _T("Text2Reach: phone=\"%s\", text=\"%s\""), phoneNumber, text);
	
   char errorBuffer[CURL_ERROR_SIZE];
   CURL *curl = curl_easy_init();
   if (curl != NULL)
   {
      curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
      curl_easy_setopt(curl, CURLOPT_HEADER, (long)0); // do not include header in data
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &OnCurlDataReceived);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, (long)0);

      RequestData *data = (RequestData *)malloc(sizeof(RequestData));
      memset(data, 0, sizeof(RequestData));
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);

#ifdef UNICODE
      char *mbphone = MBStringFromWideString(phoneNumber);
      char *mbmsg = UTF8StringFromWideString(text);
      char *phone = curl_easy_escape(curl, mbphone, 0);
      char *msg = curl_easy_escape(curl, mbmsg, 0);
      free(mbphone);
      free(mbmsg);
#else
      char *phone = curl_easy_escape(curl, phoneNumber, 0);
      char *mbmsg = UTF8StringFromMBString(text);
      char *msg = curl_easy_escape(curl, mbmsg, 0);
      free(mbmsg);
#endif

      char url[4096];
      snprintf(url, 4096, "http://www.text2reach.com/api/1.1/sms/bulk/?api_key=%s&phone=%s&from=%s&message=%s&unicode=%s&blacklist=%s",
               s_apikey, phone, s_from, msg, (s_unicode ? "true" : "false"), (s_blacklist ? "true" : "false"));
      nxlog_debug(7, _T("Text2Reach: URL set to \"%hs\""), url);

      curl_free(phone);
      curl_free(msg);

      if (curl_easy_setopt(curl, CURLOPT_URL, url) == CURLE_OK)
      {
         if (curl_easy_perform(curl) == CURLE_OK)
         {
            long response = 500;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response);
            if (response == 200)
            {
               if (data->allocated > 0)
               {
                  data->data[data->size] = 0;
                  long messageId = strtol(data->data, NULL, 0);
                  if (messageId > 0)
                  {
                     nxlog_debug(4, _T("Text2Reach: SMS successfully sent"));
                     success = true;
                  }
                  else if (messageId == 0)
                  {
                     nxlog_debug(4, _T("Text2Reach: malformed response %hs"), data->data);
                  }
                  else
                  {
                     nxlog_debug(4, _T("Text2Reach: sending error %d"), (int)messageId);
                  }
               }
            }
            else
            {
               nxlog_debug(4, _T("Text2Reach: request cannot be fulfilled, HTTP response code %03d"), response);
            }
         }
         else
         {
            nxlog_debug(4, _T("Text2Reach: call to curl_easy_perform() failed"));
         }
      }
      else
      {
      	nxlog_debug(4, _T("Text2Reach: call to curl_easy_setopt(CURLOPT_URL) failed: %s"), errorBuffer);
      }
      safe_free(data->data);
      free(data);
      curl_easy_cleanup(curl);
   }
   else
   {
   	nxlog_debug(4, _T("Text2Reach: call to curl_easy_init() failed"));
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
