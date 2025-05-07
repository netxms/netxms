/* 
** NetXMS - Network Management System
** Notification channel driver for Kannel gateway
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
** File: kannel.cpp
**
**/

#include <ncdrv.h>
#include <nms_util.h>
#include <nxlibcurl.h>

#define DEBUG_TAG _T("ncd.kannel")

/**
 * Kannel driver class
 */
class KannelDriver : public NCDriver
{
private:
   char m_hostname[128];
   int m_port;
   char m_login[128];
   char m_password[128];

public:
   KannelDriver(Config *config);
   virtual int send(const TCHAR* recipient, const TCHAR* subject, const TCHAR* body) override;
};

/**
 * Init driver
 */
KannelDriver::KannelDriver(Config *config)
{
   m_port = 13001;
   strcpy(m_hostname,"127.0.0.1");
   strcpy(m_login, "user");
   strcpy(m_password, "password");

   nxlog_debug_tag(DEBUG_TAG, 5, _T("Creating new driver instance"));

   NX_CFG_TEMPLATE configTemplate[] = 
   {
      { _T("Host"), CT_MB_STRING, 0, 0, sizeof(m_hostname), 0, m_hostname },
      { _T("Login"), CT_MB_STRING, 0, 0, sizeof(m_login), 0, m_login },
      { _T("Password"), CT_MB_STRING, 0, 0, sizeof(m_password), 0, m_password },
      { _T("Port"), CT_LONG, 0, 0, 0, 0,	&m_port },
      { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr }
   };

	config->parseTemplate(_T("Kannel"), configTemplate);
}

/**
 * Send SMS
 */
int KannelDriver::send(const TCHAR* recipient, const TCHAR* subject, const TCHAR* body)
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

      ByteStream *data = new ByteStream();
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);

      bool intlPrefix = (recipient[0] == _T('+'));
#ifdef UNICODE
      char *utf8phone = UTF8StringFromWideString(intlPrefix ? &recipient[1] : recipient);
      char *utf8msg = UTF8StringFromWideString(body);
#else
      char *utf8phone = UTF8StringFromMBString(intlPrefix ? &recipient[1] : recipient);
      char *utf8msg = UTF8StringFromMBString(body);
#endif
      char *phone = curl_easy_escape(curl, utf8phone, 0);
      char *msg = curl_easy_escape(curl, utf8msg, 0);
      MemFree(utf8phone);
      MemFree(utf8msg);

      char url[4096];
      snprintf(url, 4096, "http://%s:%d/cgi-bin/sendsms?username=%s&password=%s&to=%s%s&text=%s",
               m_hostname, m_port, m_login, m_password, intlPrefix ? "00" : "", phone, msg);
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
            nxlog_debug_tag(DEBUG_TAG, 4, _T("%d bytes received"), static_cast<int>(data->size()));

            long response = 500;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response);
            nxlog_debug_tag(DEBUG_TAG, 4, _T("Response code %03d"), (int)response);
            if (response == 202)
            {
               result = 0;
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
      delete data;
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
DECLARE_NCD_ENTRY_POINT(Kannel, nullptr)
{
   if (!InitializeLibCURL())
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("cURL initialization failed"));
      return nullptr;
   }
   return new KannelDriver(config);
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
