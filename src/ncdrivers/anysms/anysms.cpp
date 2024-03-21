/* 
** NetXMS - Network Management System
** SMS driver for any-sms.biz service
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
** File: anysms.cpp
**
**/

#include <ncdrv.h>
#include <nms_util.h>
#include <nxlibcurl.h>

#ifndef CURL_MAX_HTTP_HEADER
// workaround for older cURL versions
#define CURL_MAX_HTTP_HEADER CURL_MAX_WRITE_SIZE
#endif

#define DEBUG_TAG _T("ncd.anysms")

/**
 * Any SMS driver class
 */
class AnySMSDriver : public NCDriver
{
private:
   char m_login[128];
   char m_password[128];
   char m_sender[64];
   char m_gateway[64];
   bool m_verifyPeer;

public:
   AnySMSDriver(Config *config);
   virtual int send(const TCHAR* recipient, const TCHAR* subject, const TCHAR* body) override;
};

/**
 * Init driver
 */
AnySMSDriver::AnySMSDriver(Config *config)
{
   strcpy(m_login, "user");
   strcpy(m_password, "password");
   strcpy(m_sender, "NETXMS");
   strcpy(m_gateway, "28");
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
		{ _T("sender"), CT_MB_STRING, 0, 0, sizeof(m_sender), 0,	m_sender },	
		{ _T("gateway"), CT_MB_STRING, 0, 0, sizeof(m_gateway), 0,	m_gateway },
		{ _T("verifyPeer"), CT_BOOLEAN, 0, 0, 1, 0, &m_verifyPeer },
		{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
	};

	config->parseTemplate(_T("AnySMS"), configTemplate);
}

/**
 * Callback for processing data received from cURL
 */
static size_t OnCurlDataReceived(char *ptr, size_t size, size_t nmemb, void *context)
{
   size_t bytes = size * nmemb;
   static_cast<ByteStream*>(context)->write(ptr, bytes);
   return bytes;
}

/**
 * Send SMS
 */
int AnySMSDriver::send(const TCHAR* recipient, const TCHAR* subject, const TCHAR* body)
{
   int result = -1;

	nxlog_debug_tag(DEBUG_TAG, 4, _T("phone=\"%s\", text=\"%s\""), recipient, body);

   CURL *curl = curl_easy_init();
   if (curl != NULL)
   {
#if HAVE_DECL_CURLOPT_NOSIGNAL
      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, (long)1);
#endif

      curl_easy_setopt(curl, CURLOPT_HEADER, (long)0); // do not include header in data
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &OnCurlDataReceived);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, static_cast<long>(m_verifyPeer ? 1 : 0));

      ByteStream responseData(32768);
      responseData.setAllocationStep(32768);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

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
      snprintf(url, 4096, "http://gateway.any-sms.biz/send_sms.php?id=%s&pass=%s&text=%s&nummer=%s&gateway=%s&absender=%s",
               m_login, m_password, msg, phone, m_gateway, m_sender);
      nxlog_debug_tag(DEBUG_TAG, 4, _T("URL set to \"%hs\""), url);

      curl_free(phone);
      curl_free(msg);

      if (curl_easy_setopt(curl, CURLOPT_URL, url) == CURLE_OK)
      {
         if (curl_easy_perform(curl) == CURLE_OK)
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("%d bytes received"), static_cast<int>(responseData.size()));
            if (responseData.size() > 0)
            {
               responseData.write('\0');
               const char* data = reinterpret_cast<const char*>(responseData.buffer());
               if (!strncmp(data, "Err:", 4))
               {
                  int code = strtol(data + 4, NULL, 10);
                  if (code == 0)
                  {
               	   nxlog_debug_tag(DEBUG_TAG, 4, _T("Success"));
                     result = 0;
                  }
                  else
                  {
               	   nxlog_debug_tag(DEBUG_TAG, 4, _T("Error response (%d)"), code);
                  }
               }
               else
               {
               	nxlog_debug_tag(DEBUG_TAG, 4, _T("Unexpected response (%hs)"), data);
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
DECLARE_NCD_ENTRY_POINT(AnySMS, NULL)
{
   if (!InitializeLibCURL())
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("cURL initialization failed"));
      return NULL;
   }
   return new AnySMSDriver(config);
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
