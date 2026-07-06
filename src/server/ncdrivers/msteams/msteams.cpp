/* 
** NetXMS - Network Management System
** Notification channel driver for Microsoft Teams
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
** File: msteams.cpp
**
**/

#include <ncdrv.h>
#include <nms_util.h>
#include <netxms-version.h>
#include <nxlibcurl.h>

#define DEBUG_TAG _T("ncd.msteams")

/**
 * Flags
 */
#define MST_USE_CARDS   0x0001

/**
 * Microsoft Teams driver class
 */
class MicrosoftTeamsDriver : public NCDriver
{
private:
   uint32_t m_flags;
   StringMap m_channels;
   char m_themeColor[8];

   MicrosoftTeamsDriver(uint32_t flags, const TCHAR *themeColor) : NCDriver()
   {
      m_flags = flags;
      char *color8 = UTF8StringFromTString(themeColor);
      strlcpy(m_themeColor, color8, 8);
      MemFree(color8);
   }

public:
   virtual int send(const NotificationContext& context) override;

   static MicrosoftTeamsDriver *createInstance(Config *config);
};

/**
 * Create driver instance
 */
MicrosoftTeamsDriver *MicrosoftTeamsDriver::createInstance(Config *config)
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Creating new MS Teams driver instance"));

   uint32_t flags = 0;
   TCHAR themeColor[8] = _T("FF6A00");
   NX_CFG_TEMPLATE configTemplate[] =
	{
		{ _T("ThemeColor"), CT_STRING, 0, 0, sizeof(themeColor) / sizeof(TCHAR), 0, themeColor },
      { _T("UseMessageCards"), CT_BOOLEAN_FLAG_32, 0, 0, MST_USE_CARDS, 0, &flags },
		{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr }
	};

	if (!config->parseTemplate(_T("MicrosoftTeams"), configTemplate))
	{
	   nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Error parsing driver configuration"));
	   return nullptr;
	}

   MicrosoftTeamsDriver *driver = new MicrosoftTeamsDriver(flags, themeColor);
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Microsoft Teams driver instantiated"));

   unique_ptr<ObjectArray<ConfigEntry>> channels = config->getSubEntries(_T("/Channels"), _T("*"));
   if (channels != nullptr)
   {
	   for(int i = 0; i < channels->size(); i++)
	   {
	      ConfigEntry *channel = channels->get(i);
	      driver->m_channels.set(channel->getName(), channel->getValue());
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Added channel mapping %s = %s"), channel->getName(), channel->getValue());
      }
   }
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
int MicrosoftTeamsDriver::send(const NotificationContext& context)
{
   const char *recipient = context.recipient;
   const char *subject = context.subject;
   const char *body = context.body;
   char *jsubject = EscapeStringForJSONUtf8(subject);
   char *jbody = EscapeStringForJSONUtf8(body);

   ByteStream request(4096);
   AppendText(request, "{ ");
   if (m_flags & MST_USE_CARDS)
   {
      AppendText(request, "\"@type\":\"MessageCard\", \"@context\":\"https://schema.org/extensions\", \"themeColor\":\"");
      AppendText(request, m_themeColor);
      AppendText(request, "\", \"summary\":\"");
      if (jsubject[0] == 0)
      {
         AppendText(request, jbody);
      }
      else
      {
         AppendText(request, jsubject);
         AppendText(request, "\", \"title\":\"");
         AppendText(request, jsubject);
      }
      AppendText(request, "\", \"text\":\"");
      AppendText(request, jbody);
      AppendText(request, "\"");
   }
   else
   {
      AppendText(request, "\"text\":\"");
      AppendText(request, jsubject);
      if ((jsubject[0] != 0) && (jbody[0] != 0))
         AppendText(request, "\\n\\n");
      AppendText(request, jbody);
      AppendText(request, "\"");
   }
   AppendText(request, " }");
   request.write('\0');

   MemFree(jsubject);
   MemFree(jbody);

   const char *json = reinterpret_cast<const char*>(request.buffer());
   nxlog_debug_tag(DEBUG_TAG, 7, _T("Prepared request: %hs"), json);

   // Attempt to lookup URL alias (channel mappings are stored as wide strings)
   TCHAR *key = TStringFromUTF8String(recipient);
   const TCHAR *alias = m_channels.get(key);
   char *url = (alias != nullptr) ? UTF8StringFromTString(alias) : MemCopyStringA(recipient);
   MemFree(key);

   CURL *curl = curl_easy_init();
   if (curl == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_init() failed"));
      MemFree(url);
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
   curl_easy_setopt(curl, CURLOPT_USERAGENT, "NetXMS Microsoft Teams Driver/" NETXMS_VERSION_STRING_A);

   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

   curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);

   struct curl_slist *headers = nullptr;
   headers = curl_slist_append(headers, "Content-Type: application/json");
   curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

   char errorBuffer[CURL_ERROR_SIZE];
   curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);

   int result = 0;

   if (curl_easy_setopt(curl, CURLOPT_URL, url) != CURLE_OK)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_setopt(CURLOPT_URL) failed"));
      result = -1;
   }

   if (result == 0)
   {
      CURLcode rc = curl_easy_perform(curl);
      if (rc != CURLE_OK)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Call to curl_easy_perform() failed (%d: %hs)"), rc, errorBuffer);
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
         if (httpCode == 412 || httpCode == 429 || httpCode == 502 || httpCode == 504)
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
      if (!strcmp(data, "1"))
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("Message successfully sent"));
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Error response from webhook: %hs"), data);
         result = -1;
      }
   }

   curl_slist_free_all(headers);
   curl_easy_cleanup(curl);
   MemFree(url);
   return result;
}

/**
 * Configuration template
 */
static const NCConfigurationTemplate s_config(true, true);

/**
 * Driver entry point
 */
DECLARE_NCD_ENTRY_POINT(MicrosoftTeams, &s_config)
{
   if (!InitializeLibCURL())
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("cURL initialization failed"));
      return nullptr;
   }
   return MicrosoftTeamsDriver::createInstance(config);
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
