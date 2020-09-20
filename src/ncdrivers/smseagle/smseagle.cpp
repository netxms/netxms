/* 
** NetXMS - Network Management System
** SMS driver for SMSEagle Hardware SMS Gateway
** 
** SMSEagle API documentation - https://www.smseagle.eu/api/
**
** Copyright (C) 2014-2019 Raden Solutions
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
** File: smseagle.cpp
**
**/

#include <ncdrv.h>
#include <nms_util.h>
#include <curl/curl.h>

#ifndef CURL_MAX_HTTP_HEADER
// workaround for older cURL versions
#define CURL_MAX_HTTP_HEADER CURL_MAX_WRITE_SIZE
#endif

#define DEBUG_TAG _T("ncd.smseagle")

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
 * SMSEagle driver class
 */
class SMSEagleDriver : public NCDriver
{
private:
   char m_hostname[128];
   int m_port;
   char m_login[128];
   char m_password[128];
   bool m_useHttps;

public:
   SMSEagleDriver(Config *config);
   virtual bool send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body) override;
};

/**
 * Init driver
 */
SMSEagleDriver::SMSEagleDriver(Config *config)
{
   strcpy(m_hostname, "127.0.0.1");
   strcpy(m_login, "user");
   strcpy(m_password, "password");
   m_port = 80;

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

   int flag;
   NX_CFG_TEMPLATE configTemplate[] = 
	{
		{ _T("host"), CT_MB_STRING, 0, 0, sizeof(m_hostname), 0, m_hostname },	
		{ _T("port"), CT_LONG, 0, 0, 0, 0, &m_port },	
		{ _T("login"), CT_MB_STRING, 0, 0, sizeof(m_login), 0, m_login },	
		{ _T("password"), CT_MB_STRING, 0, 0, sizeof(m_password), 0, m_password },	
		{ _T("https"), CT_BOOLEAN_FLAG_32, 0, 0, 1, 0, &flag },
		{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
	};

	config->parseTemplate(_T("SMSEagle"), configTemplate);
   m_useHttps = (flag == 1);
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
bool SMSEagleDriver::send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body)
{
   bool success = false;

   nxlog_debug_tag(DEBUG_TAG, 4, _T("phone/group=\"%s\", body=\"%s\""), recipient, body);

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

      bool intlPrefix = (recipient[0] == _T('+')); // this will be used to decide whether it is number or group name
#ifdef UNICODE
      char *mbphone = MBStringFromWideString(recipient);
      char *mbmsg = MBStringFromWideString(body);
      char *phone = curl_easy_escape(curl, mbphone, 0);
      char *msg = curl_easy_escape(curl, mbmsg, 0);
      free(mbphone);
      free(mbmsg);
#else
      char *phone = curl_easy_escape(curl, recipient, 0);
      char *msg = curl_easy_escape(curl, body, 0);
#endif

      char url[4096];
      snprintf(url, 4096,
               intlPrefix ? "%s://%s:%d/index.php/http_api/send_sms?login=%s&pass=%s&to=%s&message=%s" : "%s://%s:%d/index.php/http_api/send_togroup?login=%s&pass=%s&groupname=%s&message=%s",
               m_useHttps ? "https" : "http", m_hostname, m_port, m_login, m_password, phone, msg);
      nxlog_debug_tag(DEBUG_TAG, 4, _T("URL set to \"%hs\""), url);

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
               success = true;
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
DECLARE_NCD_ENTRY_POINT(SMSEagle, NULL)
{
   if (!InitializeLibCURL())
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("cURL initialization failed"));
      return NULL;
   }
   return new SMSEagleDriver(config);
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
