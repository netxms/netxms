/* 
** NetXMS - Network Management System
** SMS driver for Nexmo gateway
** Copyright (C) 2014-2025 Raden Solutions
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
** File: nexmo.cpp
**
**/

#include <ncdrv.h>
#include <nms_util.h>
#include <nxlibcurl.h>

#define DEBUG_TAG _T("ncd.nexmo")

/**
 * Nexmo driver class
 */
class NexmoDriver : public NCDriver
{
private:
   char m_apiKey[128];
   char m_apiSecret[128];
   char m_from[128];

public:
   NexmoDriver(Config *config);
   virtual int send(const TCHAR* recipient, const TCHAR* subject, const TCHAR* body) override;
};

/**
 * Init driver
 */
NexmoDriver::NexmoDriver(Config *config)
{
   strcpy(m_apiKey, "key");
   strcpy(m_apiSecret, "secret");
   strcpy(m_from, "NetXMS");

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
		{ _T("apiKey"), CT_MB_STRING, 0, 0, sizeof(m_apiKey), 0, m_apiKey },	
		{ _T("apiSecret"), CT_MB_STRING, 0, 0, sizeof(m_apiSecret), 0, m_apiSecret },	
		{ _T("from"), CT_MB_STRING, 0, 0, sizeof(m_from), 0,	m_from },	
		{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
	};

	config->parseTemplate(_T("Nexmo"), configTemplate);
}

/**
 * Parse Nexmo server response
 */
static bool ParseResponse(const char *response)
{
   json_error_t error;
   json_t *root = json_loads(response, 0, &error);
   if (root == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot parse response JSON"));
      return false;
   }

   bool success = false;
   json_t *messages = json_object_get(root, "messages");
   if (messages != nullptr)
   {
      size_t count = json_array_size(messages);
      if (count > 0)
      {
         success = true;
         for(size_t i = 0; i < count; i++)
         {
            json_t *m = json_array_get(messages, i);
            if ((m == nullptr) || !json_is_object(m))
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("Invalid response (\"messages\"[%d] is invalid)"), (int)i);
               success = false;
               break;
            }
            json_t *status = json_object_get(m, "status");
            if ((status == nullptr) || !json_is_string(status))
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("Invalid response (\"status\" is missing or invalid for \"messages\"[%d])"), (int)i);
               success = false;
               break;
            }
            char *eptr;
            int statusCode = (int)strtol(json_string_value(status), &eptr, 10);
            if (*eptr != 0)
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("Invalid response (\"status\" is not an integer for \"messages\"[%d])"), (int)i);
               success = false;
               break;
            }
            if (statusCode != 0)
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("message sending error (status code %d for part %d)"), statusCode, (int)i);
               success = false;
               break;
            }
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Invalid response (\"messages\" is empty or not an array)"));
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Invalid response (missing \"messages\" part)"));
   }

   json_decref(root);
   return success;
}

/**
 * Send SMS
 */
int NexmoDriver::send(const TCHAR* recipient, const TCHAR* subject, const TCHAR* body)
{
   int result = -1;

   nxlog_debug_tag(DEBUG_TAG, 4, _T("phone=\"%s\", text=\"%s\""), recipient, body);

   CURL *curl = curl_easy_init();
   if (curl != nullptr)
   {
#if HAVE_DECL_CURLOPT_NOSIGNAL
      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, (long)1);
#endif

      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, (long)1); // do not install signal handlers or send signals
      curl_easy_setopt(curl, CURLOPT_HEADER, (long)0); // do not include header in data
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ByteStream::curlWriteFunction);

      ByteStream responseData(32768);
      responseData.setAllocationStep(32768);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

#ifdef UNICODE
      char *mbphone = MBStringFromWideString(recipient);
      char *mbmsg = MBStringFromWideString(body);
      char *phone = curl_easy_escape(curl, mbphone, 0);
      char *from = curl_easy_escape(curl, m_from, 0);
      char *msg = curl_easy_escape(curl, mbmsg, 0);
      free(mbphone);
      free(mbmsg);
#else
      char *phone = curl_easy_escape(curl, recipient, 0);
      char *from = curl_easy_escape(curl, m_from, 0);
      char *msg = curl_easy_escape(curl, body, 0);
#endif

      char url[4096];
      snprintf(url, 4096, "https://rest.nexmo.com/sms/json?api_key=%s&api_secret=%s&to=%s&from=%s&text=%s",
               m_apiKey, m_apiSecret, phone, from, msg);
      nxlog_debug_tag(DEBUG_TAG, 7, _T("URL set to \"%hs\""), url);

      curl_free(phone);
      curl_free(from);
      curl_free(msg);

      char errorBuffer[CURL_ERROR_SIZE];
      curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);

      if (curl_easy_setopt(curl, CURLOPT_URL, url) == CURLE_OK)
      {
         CURLcode rc = curl_easy_perform(curl);
         if (rc == CURLE_OK)
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("%d bytes received"), static_cast<int>(responseData.size()));
            if (responseData.size() > 0)
               responseData.write('\0');

            long response = 500;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response);
            nxlog_debug_tag(DEBUG_TAG, 4, _T("Response code %03d"), (int)response);
            if (response == 200)
            {
               result = ParseResponse(reinterpret_cast<const char*>(responseData.buffer())) ? 0 : -1;
            }
         }
         else
         {
         	nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_perform() failed (%d: %hs)"), rc, errorBuffer);
         }
      }
      else
      {
      	nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_setopt(CURLOPT_URL) failed"));
      }
      curl_easy_cleanup(curl);
   }
   else
   {
   	nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_init() failed"));
   }

   return result;
}

/**
 * Driver entry point
 */
DECLARE_NCD_ENTRY_POINT(Nexmo, NULL)
{
   if (!InitializeLibCURL())
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("cURL initialization failed"));
      return NULL;
   }
   return new NexmoDriver(config);
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
