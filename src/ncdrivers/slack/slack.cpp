/*
** NetXMS - Network Management System
** SMS driver for slack.com service
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
** File: slack.cpp
**
**/

#include <ncdrv.h>
#include <nms_util.h>
#include <nxlibcurl.h>

#define DEBUG_TAG _T("ncd.slack")

/**
 * Slack driver class
 */
class SlackDriver : public NCDriver
{
private:
   char s_username[64];
   char s_url[1024];

public:
   SlackDriver(Config *config);
   virtual int send(const TCHAR* recipient, const TCHAR* subject, const TCHAR* body) override;
};

/**
 * Init driver
 */
SlackDriver::SlackDriver(Config *config)
{
   s_username[0] = 0;
   s_url[0] = 0;

   NX_CFG_TEMPLATE configTemplate[] = 
	{
		{ _T("url"), CT_MB_STRING, 0, 0, sizeof(s_url), 0, s_url },	
		{ _T("username"), CT_MB_STRING, 0, 0, sizeof(s_username), 0, s_username },	
		{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
	};
	config->parseTemplate(_T("Slack"), configTemplate);

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Slack driver instantiated"));
}

/**
 * Send SMS
 */
int SlackDriver::send(const TCHAR* recipient, const TCHAR* subject, const TCHAR* body)
{
   int result = 0;

   nxlog_debug_tag(DEBUG_TAG, 4, _T("channel=\"%s\", text=\"%s\""), recipient, body);

   CURL *curl = curl_easy_init();
   if (curl != nullptr)
   {
#if HAVE_DECL_CURLOPT_NOSIGNAL
      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, (long)1);
#endif

      curl_easy_setopt(curl, CURLOPT_HEADER, (long)0); // do not include header in data
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ByteStream::curlWriteFunction);

      ByteStream responseData(32768);
      responseData.setAllocationStep(32768);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

#ifdef UNICODE
      char *_channel = MBStringFromWideString(recipient);
      char *_text = MBStringFromWideString(body);
#else
      char *_channel = strdup(recipient);
      char *_text = strdup(body);
#endif

      json_t *root = json_object();
      if (_channel[0] != 0)
      {
         json_object_set_new(root, "channel", json_string(_channel));
      }
      if (s_username[0] != 0)
      {
         json_object_set_new(root, "username", json_string(s_username));
      }
      json_object_set_new(root, "text", json_string(_text));
      char *json_body = json_dumps(root, 0);

      char request[4096];
      snprintf(request, 4095, "payload=%s", json_body);
      MemFree(json_body);

      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request);
      json_decref(root);

      char errBuff[CURL_ERROR_SIZE];
      curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errBuff);

      MemFree(_channel);
      MemFree(_text);

      if (curl_easy_setopt(curl, CURLOPT_URL, s_url) != CURLE_OK)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_setopt(CURLOPT_URL) failed"));
         result = -1;
      }

      if (result == 0)
      {
         CURLcode rc = curl_easy_perform(curl);
         if (rc != CURLE_OK)
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("Call to curl_easy_perform() failed: %d: %hs"), rc, errBuff);
            result = -1;
         }
      }

      if (result == 0)
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Got %d bytes"), static_cast<int>(responseData.size()));
         long httpCode = 0;
         curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
         if (httpCode != 200)
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("Error response from webhook: HTTP response code is %d"), httpCode);
            if (httpCode == 429 || httpCode == 502 || httpCode == 504)
               result = 10;
            else
               result = -1;
         }
      }

      if (result == 0 && responseData.size() <= 0)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Empty response from webhook"));
         result = -1;
      }

      if (result == 0)
      {
         responseData.write('\0');
         const char* data = reinterpret_cast<const char*>(responseData.buffer());
         if (!strcmp(data, "ok"))
         {
            nxlog_debug_tag(DEBUG_TAG, 6, _T("Message successfully sent"));
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("Error response from webhook: %hs"), data);
            result = -1;
         }
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
DECLARE_NCD_ENTRY_POINT(Slack, NULL)
{
   if (!InitializeLibCURL())
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("cURL initialization failed"));
      return NULL;
   }
   return new SlackDriver(config);
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
