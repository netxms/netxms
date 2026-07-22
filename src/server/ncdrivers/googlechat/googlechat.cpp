/*
** NetXMS - Network Management System
** Notification channel driver for Microsoft Teams
** Copyright (C) 2021-2023 Raden Solutions
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
** File: googlechat.cpp
**
**/

#include <ncdrv.h>
#include <nms_util.h>
#include <netxms-version.h>
#include <nxlibcurl.h>

#define DEBUG_TAG _T("ncd.googlechat")

static const NCConfigurationTemplate s_config(true, true); 

/**
 * Google Chat driver class
 */
class GoogleChatDriver : public NCDriver
{
private:
   StringMap m_rooms;
   
   GoogleChatDriver() { }

public:
   virtual int send(const NotificationContext& context) override;

   static GoogleChatDriver *createInstance(Config *config);
};

/**
 * Create driver instance
 */
GoogleChatDriver *GoogleChatDriver::createInstance(Config *config)
{
   GoogleChatDriver *driver = new GoogleChatDriver();

   unique_ptr<ObjectArray<ConfigEntry>> rooms = config->getSubEntries(_T("/Rooms"), nullptr);
   if (rooms != nullptr)
   {
      for (int i = 0; i < rooms->size(); i++)
      {
         ConfigEntry *room = rooms->get(i);
         driver->m_rooms.set(room->getName(), room->getValue());
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Added room mapping %s = %s"), room->getName(), room->getValue());
      }
   }
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Google Chat driver instantiated"));

	return driver;
}

/**
 * Append UTF-8 text to request being built
 */
static inline void AppendText(ByteStream& request, const char *text)
{
   request.write(text, strlen(text));
}

/**
 * Send notification
 */
int GoogleChatDriver::send(const NotificationContext& context)
{
   const char *recipient = context.recipient;
   const char *subject = context.subject;
   const char *body = context.body;
   char *jsubject = EscapeStringForJSONUtf8(subject);
   char *jbody = EscapeStringForJSONUtf8(body);

   ByteStream request(4096);
   AppendText(request, "{ \"text\":\"");
   AppendText(request, jsubject);
   if ((jsubject[0] != 0) && (jbody[0] != 0))
      AppendText(request, "\\n\\n");
   AppendText(request, jbody);
   AppendText(request, "\" }");
   request.write('\0');

   MemFree(jsubject);
   MemFree(jbody);

   const char *json = reinterpret_cast<const char*>(request.buffer());
   nxlog_debug_tag(DEBUG_TAG, 7, _T("Prepared request: %hs"), json);

   CURL *curl = curl_easy_init();
   if (curl == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_init() failed"));
      return -1;
   }

#if HAVE_DECL_CURLOPT_NOSIGNAL
   curl_easy_setopt(curl, CURLOPT_NOSIGNAL, (long)1);
#endif

#if HAVE_DECL_CURLOPT_PROTOCOLS_STR
   curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "https");
#else
   curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTPS);
#endif

   curl_easy_setopt(curl, CURLOPT_HEADER, (long)0); // do not include header in data
   curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ByteStream::curlWriteFunction);
   curl_easy_setopt(curl, CURLOPT_USERAGENT, "NetXMS Google Chat Driver/" NETXMS_VERSION_STRING_A);

   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

   struct curl_slist *headers = nullptr;
   curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);
   headers = curl_slist_append(headers, "Content-Type: application/json");
   curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

   char errorBuffer[CURL_ERROR_SIZE];
   curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);

   int result = -1;

   // Attempt to lookup URL alias (room mappings are stored as wide strings)
   TCHAR *key = TStringFromUTF8String(recipient);
   const TCHAR *alias = m_rooms.get(key);
   char *url = (alias != nullptr) ? UTF8StringFromTString(alias) : MemCopyStringA(recipient);
   MemFree(key);

   if (curl_easy_setopt(curl, CURLOPT_URL, url) == CURLE_OK)
   {
      CURLcode rc = curl_easy_perform(curl);
      if (rc == CURLE_OK)
      {
         long httpCode = 0;
         curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
         if (httpCode == 200)
         {
            result = 0;
            nxlog_debug_tag(DEBUG_TAG, 6, _T("Webhook responded with success"));
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("Error response from webhook: HTTP response code is %d"), httpCode);
            if (httpCode == 429 || httpCode == 502 || httpCode == 504)
               result = 10;
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Call to curl_easy_perform() failed (%d: %hs)"), rc, errorBuffer);
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Call to curl_easy_setopt(CURLOPT_URL) failed"));
   }
   curl_slist_free_all(headers);
   curl_easy_cleanup(curl);
   MemFree(url);
   return result;
}

/**
 * Driver entry point
 */
DECLARE_NCD_ENTRY_POINT(GoogleChat, &s_config)
{
   if (!InitializeLibCURL())
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("cURL initialization failed"));
      return nullptr;
   }
   return GoogleChatDriver::createInstance(config);
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
