/*
** NetXMS - Network Management System
** SMS driver for MyMobile API gateways
** Copyright (C) 2014-2020 Raden Solutions
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
** File: mymobile.cpp
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

#define DEBUG_TAG _T("ncd.mymobile")

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
 * Any SMS driver class
 */
class MyMobileDriver : public NCDriver
{
private:
   char m_username[128];
   char m_password[128];

public:
   MyMobileDriver(Config *config);
   virtual bool send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body) override;
};

/**
 * Init driver
 */
MyMobileDriver::MyMobileDriver(Config *config)
{
   m_username[0] = 0;
   m_password[0] = 0;

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
		{ _T("Username"), CT_MB_STRING, 0, 0, sizeof(m_username), 0, m_username },
		{ _T("Password"), CT_MB_STRING, 0, 0, sizeof(m_password), 0, m_password },
		{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
	};

	config->parseTemplate(_T("MyMobile"), configTemplate);
}

/**
 * Callback for processing data received from cURL
 */
static size_t OnCurlDataReceived(char *ptr, size_t size, size_t nmemb, void *userdata)
{
   RequestData *data = static_cast<RequestData*>(userdata);
   if ((data->allocated - data->size) < (size * nmemb))
   {
      char *newData = MemRealloc(data->data, data->allocated + CURL_MAX_HTTP_HEADER);
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
 * Parse mymobile server response
 */
static bool ParseResponse(const char *xml)
{
   Config *response = new Config();
   if (!response->loadXmlConfigFromMemory(xml, (int)strlen(xml), NULL, "api_result", false))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot parse response XML"));
      return false;
   }

   bool success = false;
   const TCHAR *result = response->getValue(_T("/call_result/result"), _T("false"));
   if (!_tcsicmp(result, _T("true")))
   {
      success = true;
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Sending result: %s"), result);
      const TCHAR *error = response->getValue(_T("/call_result/error"));
      if (error != NULL)
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Sending error details: %s"), error);
   }
   return success;
}

/**
 * Send SMS
 */
bool MyMobileDriver::send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body)
{
   bool success = false;

   nxlog_debug_tag(DEBUG_TAG, 4, _T("phone=\"%s\", body=\"%s\""), recipient, body);

   CURL *curl = curl_easy_init();
   if (curl != nullptr)
   {
#if HAVE_DECL_CURLOPT_NOSIGNAL
      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, (long)1);
#endif

      curl_easy_setopt(curl, CURLOPT_HEADER, (long)0); // do not include header in data
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &OnCurlDataReceived);

      RequestData *data = MemAllocStruct<RequestData>();
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);

#ifdef UNICODE
      char *mbphone = MBStringFromWideString(recipient);
      char *mbmsg = MBStringFromWideString(body);
      char *phone = curl_easy_escape(curl, mbphone, 0);
      char *msg = curl_easy_escape(curl, mbmsg, 0);
      MemFree(mbphone);
      MemFree(mbmsg);
#else
      char *phone = curl_easy_escape(curl, recipient, 0);
      char *msg = curl_easy_escape(curl, body, 0);
#endif

      char url[4096];
      snprintf(url, 4096, "http://www.mymobileapi.com/api5/http5.aspx?Type=sendparam&username=%s&password=%s&numto=%s&data1=%s",
               m_username, m_password, phone, msg);
      nxlog_debug_tag(DEBUG_TAG, 7, _T("URL set to \"%hs\""), url);

      curl_free(phone);
      curl_free(msg);

      if (curl_easy_setopt(curl, CURLOPT_URL, url) == CURLE_OK)
      {
         if (curl_easy_perform(curl) == CURLE_OK)
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("%d bytes received"), data->size);
            if (data->allocated > 0)
               data->data[data->size] = 0;

            long response = 500;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response);
            nxlog_debug_tag(DEBUG_TAG, 4, _T("Response code %03d"), (int)response);
            if (response == 200)
            {
               success = ParseResponse(data->data);
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
      MemFree(data->data);
      MemFree(data);
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
DECLARE_NCD_ENTRY_POINT(MyMobile, nullptr)
{
   if (!InitializeLibCURL())
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("cURL initialization failed"));
      return nullptr;
   }
   return new MyMobileDriver(config);
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
