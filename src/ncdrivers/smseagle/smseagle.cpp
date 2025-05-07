/* 
** NetXMS - Network Management System
** SMS driver for SMSEagle Hardware SMS Gateway
** 
** SMSEagle API documentation - https://www.smseagle.eu/api/
**
** Copyright (C) 2014-2025 Raden Solutions
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
#include <nxlibcurl.h>

#define DEBUG_TAG _T("ncd.smseagle")

#define OPTION_USE_HTTPS            0x01
#define OPTION_VERIFY_HOST          0x02
#define OPTION_VERIFY_CERTIFICATE   0x04

/**
 * SMSEagle driver class
 */
class SMSEagleDriver : public NCDriver
{
private:
   uint32_t m_flags;
   char m_baseURL[256];
   char m_login[128];
   char m_password[128];

public:
   SMSEagleDriver(Config *config);

   virtual int send(const TCHAR* recipient, const TCHAR* subject, const TCHAR* body) override;
};

/**
 * Initialize driver
 */
SMSEagleDriver::SMSEagleDriver(Config *config)
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Creating new SMSEagle driver instance"));

   m_flags = OPTION_USE_HTTPS | OPTION_VERIFY_HOST | OPTION_VERIFY_CERTIFICATE;
   m_baseURL[0] = 0;
   strcpy(m_login, "user");
   strcpy(m_password, "password");

   char hostname[128] = "127.0.0.1";
   uint16_t port = 0;
   NX_CFG_TEMPLATE configTemplate[] = 
	{
      { _T("BaseURL"), CT_MB_STRING, 0, 0, sizeof(m_baseURL), 0, m_baseURL },
		{ _T("Host"), CT_MB_STRING, 0, 0, sizeof(hostname), 0, hostname },
      { _T("HTTPS"), CT_BOOLEAN_FLAG_32, 0, 0, OPTION_USE_HTTPS, 0, &m_flags },   // For compatibility
		{ _T("Login"), CT_MB_STRING, 0, 0, sizeof(m_login), 0, m_login },
		{ _T("Password"), CT_MB_STRING, 0, 0, sizeof(m_password), 0, m_password },
      { _T("Port"), CT_WORD, 0, 0, 0, 0, &port },
      { _T("UseEncryption"), CT_BOOLEAN_FLAG_32, 0, 0, OPTION_USE_HTTPS, 0, &m_flags },
      { _T("VerifyCertificate"), CT_BOOLEAN_FLAG_32, 0, 0, OPTION_VERIFY_CERTIFICATE, 0, &m_flags },
      { _T("VerifyHost"), CT_BOOLEAN_FLAG_32, 0, 0, OPTION_VERIFY_HOST, 0, &m_flags },
		{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr }
	};
	config->parseTemplate(_T("SMSEagle"), configTemplate);

	if (m_baseURL[0] == 0)
	{
	   // Construct base URL from individual elements
	   bool https = (m_flags & OPTION_USE_HTTPS) != 0;
	   snprintf(m_baseURL, sizeof(m_baseURL), "%s://%s:%d/index.php/http_api", https ? "https" : "http", hostname, (port != 0) ? port : (https ? 443 : 80));
	}
	else
	{
	   // Make sure base URL does not end in /
	   size_t index = strlen(m_baseURL) - 1;
	   if (m_baseURL[index] == '/')
	      m_baseURL[index] = 0;
	}
   nxlog_debug_tag(DEBUG_TAG, 4, _T("Base URL set to \"%hs\""), m_baseURL);

   DecryptPasswordA(m_login, m_password, m_password, 128);

   CURL *curl = curl_easy_init();
   if (curl != nullptr)
   {
      char *s = curl_easy_escape(curl, m_login, 0);
      strlcpy(m_login, s, sizeof(m_login));
      curl_free(s);

      s = curl_easy_escape(curl, m_password, 0);
      strlcpy(m_password, s, sizeof(m_password));
      curl_free(s);

      curl_easy_cleanup(curl);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_init() failed"));
   }
}

/**
 * Send SMS
 */
int SMSEagleDriver::send(const TCHAR* recipient, const TCHAR* subject, const TCHAR* body)
{
   nxlog_debug_tag(DEBUG_TAG, 4, _T("phone/group=\"%s\", body=\"%s\""), recipient, body);

   CURL *curl = curl_easy_init();
   if (curl == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_init() failed"));
      return -1;
   }

#if HAVE_DECL_CURLOPT_NOSIGNAL
   curl_easy_setopt(curl, CURLOPT_NOSIGNAL, (long)1);
#endif

   curl_easy_setopt(curl, CURLOPT_HEADER, (long)0); // do not include header in data
   curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ByteStream::curlWriteFunction);

   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, (m_flags & OPTION_VERIFY_CERTIFICATE) ? 1 : 0);
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, (m_flags & OPTION_VERIFY_HOST) ? 2 : 0);
   EnableLibCURLUnexpectedEOFWorkaround(curl);

   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

   char errbuf[CURL_ERROR_SIZE];
   curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);

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
            intlPrefix ? "%s/send_sms?login=%s&pass=%s&to=%s&message=%s" : "%s/send_togroup?login=%s&pass=%s&groupname=%s&message=%s",
            m_baseURL, m_login, m_password, phone, msg);
   nxlog_debug_tag(DEBUG_TAG, 4, _T("URL set to \"%hs\""), url);

   curl_free(phone);
   curl_free(msg);

   int result = -1;
   if (curl_easy_setopt(curl, CURLOPT_URL, url) == CURLE_OK)
   {
      CURLcode rc = curl_easy_perform(curl);
      if (rc == CURLE_OK)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("%d bytes received"), static_cast<int>(responseData.size()));
         long response = 500;
         curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response);
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Response code %03d"), (int)response);
         if (response == 200)
         {
            result = 0;
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_perform() failed (%d: %hs)"), rc, errbuf);
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_setopt(CURLOPT_URL) failed"));
   }
   curl_easy_cleanup(curl);

   return result;
}

/**
 * Driver entry point
 */
DECLARE_NCD_ENTRY_POINT(SMSEagle, nullptr)
{
   if (!InitializeLibCURL())
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("cURL initialization failed"));
      return nullptr;
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
