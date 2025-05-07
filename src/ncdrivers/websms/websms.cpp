/* 
** NetXMS - Network Management System
** SMS driver for websms.ru service
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
** File: websms.cpp
**
**/

#include <ncdrv.h>
#include <nms_util.h>
#include <nxlibcurl.h>

#define DEBUG_TAG _T("ncd.websms")

/**
 * WebSMS driver class
 */
class WebSMSDriver : public NCDriver
{
private:
   char m_login[128];
   char m_password[128];
   char m_fromPhone[64];
   bool m_verifyPeer;

public:
   WebSMSDriver(Config *config);
   virtual int send(const TCHAR* recipient, const TCHAR* subject, const TCHAR* body) override;
};

/**
 * Init driver
 */
WebSMSDriver::WebSMSDriver(Config *config)
{
   strcpy(m_login, "user");
   strcpy(m_password, "password");
   m_fromPhone[0] = 0;
   m_verifyPeer = true;

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
		{ _T("login"), CT_MB_STRING, 0, 0, sizeof(m_login), 0, m_login },	
		{ _T("password"), CT_MB_STRING, 0, 0, sizeof(m_password), 0, m_password },	
		{ _T("fromPhone"), CT_MB_STRING, 0, 0, sizeof(m_fromPhone), 0, m_fromPhone },	
		{ _T("verifyPeer"), CT_BOOLEAN, 0, 0, 1, 0, &m_verifyPeer },
		{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
	};

	config->parseTemplate(_T("WebSMS"), configTemplate);
}

/**
 * Send SMS
 */
int WebSMSDriver::send(const TCHAR* recipient, const TCHAR* subject, const TCHAR* body)
{
   int result = -1;

   nxlog_debug_tag(DEBUG_TAG, 4, _T("phone=\"%s\", text=\"%s\""), recipient, body);

   CURL *curl = curl_easy_init();
   if (curl != NULL)
   {
#if HAVE_DECL_CURLOPT_NOSIGNAL
      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, (long)1);
#endif

      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, (long)1); // do not install signal handlers or send signals
      curl_easy_setopt(curl, CURLOPT_HEADER, (long)0); // do not include header in data
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ByteStream::curlWriteFunction);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);

      ByteStream responseData(32768);
      responseData.setAllocationStep(32768);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

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
      snprintf(url, 4096, "https://cab.websms.ru/http_in5.asp?http_username=%s&http_password=%s&phone_list=%s%s%s&format=xml&message=%s",
               m_login, m_password, phone, (m_fromPhone[0] != 0) ? "&fromPhone=" : "", (m_fromPhone[0] != 0) ? m_fromPhone : "", msg);
      nxlog_debug_tag(DEBUG_TAG, 4, _T("URL set to \"%hs\""), url);

      curl_free(phone);
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
            {
               responseData.write('\0');
               const char* data = reinterpret_cast<const char*>(responseData.buffer());
               Config *response = new Config;
               response->loadXmlConfigFromMemory(data, (int)strlen(data), _T("WEBSMS"), "XML");
               ConfigEntry *e = response->getEntry(_T("/httpIn"));
               if (e != NULL)
               {
                  int status = e->getAttributeAsInt(_T("error_num"), -1);
                  if (status == 0)
                  {
                     nxlog_debug_tag(DEBUG_TAG, 4, _T("SMS successfully sent"));
                     result = 0;
                  }
                  else
                  {
                     nxlog_debug_tag(DEBUG_TAG, 4, _T("Send error %d"), status);
                  }
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG, 4, _T("Malformed response\n%hs"), data);
               }
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
DECLARE_NCD_ENTRY_POINT(WebSMS, NULL)
{
   if (!InitializeLibCURL())
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("cURL initialization failed"));
      return NULL;
   }
   return new WebSMSDriver(config);
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
