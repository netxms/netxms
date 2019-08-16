/* 
** NetXMS - Network Management System
** SMS driver for Text2Reach.com service
** Copyright (C) 2014-2019 Raden Solutions
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

#include <ncdrv.h>
#include <nms_util.h>
#include <curl/curl.h>

#ifndef CURL_MAX_HTTP_HEADER
// workaround for older cURL versions
#define CURL_MAX_HTTP_HEADER CURL_MAX_WRITE_SIZE
#endif

#define DEBUG_TAG _T("ncd.text2reach")

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
 * Text2Reach driver class
 */
class Text2ReachDriver : public NCDriver
{
private:
   char m_apikey[128];
   char m_from[128];
   bool m_unicode; // if set to false, characters not in GSM 7bit alphabet will be converted (ā => a) or removed
   bool m_blacklist; // Checks if the number is in blacklist before sending. Returns error -503 if found.

public:
   Text2ReachDriver(Config *config);
   virtual bool send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body) override;
};

/**
 * Configuration
 */

/**
 * Init driver
 */
Text2ReachDriver::Text2ReachDriver(Config *config)
{
   strcpy(m_apikey, "apikey");
   strcpy(m_from, "from");

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

   int flags = 1; //default values m_unicode = true, m_blacklist = false
   NX_CFG_TEMPLATE configTemplate[] = 
	{
		{ _T("apikey"), CT_MB_STRING, 0, 0, sizeof(m_apikey), 0, m_apikey },	
		{ _T("from"), CT_MB_STRING, 0, 0, sizeof(m_from), 0, m_from },	
		{ _T("unicode"), CT_BOOLEAN, 0, 0, 1, 0, &flags },	
		{ _T("blacklist"), CT_BOOLEAN, 0, 0, 2, 0, &flags },	
		{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
	};

	config->parseTemplate(_T("Text2Reach"), configTemplate);
   m_unicode = (flags & 1) > 0;
   m_blacklist = (flags & 2) > 0;
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
bool Text2ReachDriver::send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body)
{
   bool success = false;

   nxlog_debug_tag(DEBUG_TAG, 4, _T("phone=\"%s\", text=\"%s\""), recipient, body);
	
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
      char *mbphone = MBStringFromWideString(recipient);
      char *mbmsg = UTF8StringFromWideString(body);
      char *phone = curl_easy_escape(curl, mbphone, 0);
      char *msg = curl_easy_escape(curl, mbmsg, 0);
      free(mbphone);
      free(mbmsg);
#else
      char *phone = curl_easy_escape(curl, recipient, 0);
      char *mbmsg = UTF8StringFromMBString(body);
      char *msg = curl_easy_escape(curl, mbmsg, 0);
      free(mbmsg);
#endif

      char url[4096];
      snprintf(url, 4096, "http://www.text2reach.com/api/1.1/sms/bulk/?api_key=%s&phone=%s&from=%s&message=%s&unicode=%s&blacklist=%s",
               m_apikey, phone, m_from, msg, (m_unicode ? "true" : "false"), (m_blacklist ? "true" : "false"));
      nxlog_debug_tag(DEBUG_TAG, 7, _T("URL set to \"%hs\""), url);

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
                     nxlog_debug_tag(DEBUG_TAG, 4, _T("SMS successfully sent"));
                     success = true;
                  }
                  else if (messageId == 0)
                  {
                     nxlog_debug_tag(DEBUG_TAG, 4, _T("Malformed response %hs"), data->data);
                  }
                  else
                  {
                     nxlog_debug_tag(DEBUG_TAG, 4, _T("Sending error %d"), (int)messageId);
                  }
               }
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("Request cannot be fulfilled, HTTP response code %03d"), response);
            }
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_perform() failed"));
         }
      }
      else
      {
      	nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_setopt(CURLOPT_URL) failed: %s"), errorBuffer);
      }
      MemFree(data->data);
      free(data);
      curl_easy_cleanup(curl);
   }
   else
   {
   	nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_init() failed"));
   }

   return success;
}

/**
 * Driver entry point
 */
DECLARE_NCD_ENTRY_POINT(Text2Reach, NULL)
{
   if (!InitializeLibCURL())
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("cURL initialization failed"));
      return NULL;
   }
   return new Text2ReachDriver(config);
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
