/* 
** NetXMS - Network Management System
** Notification channel driver for Microsoft Teams
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
** File: msteams.cpp
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

#define DEBUG_TAG _T("ncd.msteams")

static const NCConfigurationTemplate s_config(true, true); 

/**
 * Flags
 */
#define MST_USE_CARDS   0x0001

/**
 * Request data for cURL call
 */
struct ResponseData
{
   size_t size;
   size_t allocated;
   char *data;
};

/**
 * Microsoft Teams driver class
 */
class MicrosoftTeamsDriver : public NCDriver
{
private:
   UINT32 m_flags;
   StringMap m_channels;
   TCHAR m_themeColor[8];

   MicrosoftTeamsDriver(UINT32 flags, const TCHAR *themeColor) : NCDriver()
   {
      m_flags = flags;
      _tcslcpy(m_themeColor, themeColor, 8);
   }

public:
   virtual bool send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body) override;

   static MicrosoftTeamsDriver *createInstance(Config *config);
};

/**
 * Callback for processing data received from cURL
 */
static size_t OnCurlDataReceived(char *ptr, size_t size, size_t nmemb, void *context)
{
   ResponseData *response = static_cast<ResponseData*>(context);
   if ((response->allocated - response->size) < (size * nmemb))
   {
      char *newData = MemRealloc(response->data, response->allocated + CURL_MAX_HTTP_HEADER);
      if (newData == NULL)
      {
         return 0;
      }
      response->data = newData;
      response->allocated += CURL_MAX_HTTP_HEADER;
   }

   memcpy(response->data + response->size, ptr, size * nmemb);
   response->size += size * nmemb;

   return size * nmemb;
}

/**
 * Create driver instance
 */
MicrosoftTeamsDriver *MicrosoftTeamsDriver::createInstance(Config *config)
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Creating new driver instance"));

   UINT32 flags = 0;
   TCHAR themeColor[8] = _T("FF6A00");
   NX_CFG_TEMPLATE configTemplate[] = 
	{
		{ _T("ThemeColor"), CT_STRING, 0, 0, sizeof(themeColor), 0, themeColor },
      { _T("UseMessageCards"), CT_BOOLEAN, 0, 0, MST_USE_CARDS, 0, &flags },
		{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
	};

	if (!config->parseTemplate(_T("MicrosoftTeams"), configTemplate))
	{
	   nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Error parsing driver configuration"));
	   return NULL;
	}

   MicrosoftTeamsDriver *driver = new MicrosoftTeamsDriver(flags, themeColor);
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Microsoft Teams driver instantiated"));

	ObjectArray<ConfigEntry> *channels = config->getSubEntries(_T("/Channels"), _T("*"));
	if (channels != NULL)
	{
	   for(int i = 0; i < channels->size(); i++)
	   {
	      ConfigEntry *channel = channels->get(i);
	      driver->m_channels.set(channel->getName(), channel->getValue());
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Added channel mapping %s = %s"), channel->getName(), channel->getValue());
	   }
	   delete channels;
	}

	return driver;
}

/**
 * Send notification
 */
bool MicrosoftTeamsDriver::send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body)
{
   String jsubject = EscapeStringForJSON(subject);
   String jbody = EscapeStringForJSON(body);

   StringBuffer request(_T("{ "));
   if (m_flags & MST_USE_CARDS)
   {
      request.append(_T("\"@type\":\"MessageCard\", \"@context\":\"https://schema.org/extensions\", \"themeColor\":\""));
      request.append(m_themeColor);
      request.append(_T("\", \"summary\":\""));
      if (jsubject.isEmpty())
      {
         request.append(jbody);
      }
      else
      {
         request.append(jsubject);
         request.append(_T("\", \"title\":\""));
         request.append(jsubject);
      }
      request.append(_T("\", \"text\":\""));
      request.append(jbody);
      request.append(_T("\""));
   }
   else
   {
      request.append(_T("\"text\":\""));
      request.append(jsubject);
      if (!jsubject.isEmpty() && !jbody.isEmpty())
         request.append(_T("\\n\\n"));
      request.append(jbody);
      request.append(_T("\""));
   }
   request.append(_T(" }"));
   nxlog_debug_tag(DEBUG_TAG, 7, _T("Prepared request: %s"), request.cstr());

   // Attempt to lookup URL alias
   const TCHAR *url = m_channels.get(recipient);
   if (url == NULL)
      url = recipient;

   CURL *curl = curl_easy_init();
   if (curl == NULL)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_init() failed"));
      return false;
   }

#if HAVE_DECL_CURLOPT_NOSIGNAL
   curl_easy_setopt(curl, CURLOPT_NOSIGNAL, (long)1);
#endif

   curl_easy_setopt(curl, CURLOPT_HEADER, (long)0); // do not include header in data
   curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &OnCurlDataReceived);
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
   curl_easy_setopt(curl, CURLOPT_USERAGENT, "NetXMS Microsoft Teams Driver/" NETXMS_VERSION_STRING_A);

   ResponseData *responseData = MemAllocStruct<ResponseData>();
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, responseData);

   struct curl_slist *headers = NULL;
   char *json = request.getUTF8String();
   curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);
   headers = curl_slist_append(headers, "Content-Type: application/json");
   curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

   bool success = false;

   char utf8url[256];
#ifdef UNICODE
   WideCharToMultiByte(CP_UTF8, 0, url, -1, utf8url, 256, NULL, NULL);
#else
   mb_to_utf8(url, -1, utf8url, 256);
#endif
   if (curl_easy_setopt(curl, CURLOPT_URL, utf8url) == CURLE_OK)
   {
      if (curl_easy_perform(curl) == CURLE_OK)
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Got %d bytes"), responseData->size);
         if (responseData->allocated > 0)
         {
            responseData->data[responseData->size] = 0;
            if (!strcmp(responseData->data, "1"))
            {
               success = true;
               nxlog_debug_tag(DEBUG_TAG, 6, _T("Webhook responded with success"));
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("Error response from webhook: %hs"), responseData->data);
            }
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("Empty response from webhook"));
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
   MemFree(responseData->data);
   MemFree(responseData);
   curl_slist_free_all(headers);
   curl_easy_cleanup(curl);
   MemFree(json);
   return success;
}

/**
 * Driver entry point
 */
DECLARE_NCD_ENTRY_POINT(MicrosoftTeams, &s_config)
{
   if (!InitializeLibCURL())
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("cURL initialization failed"));
      return NULL;
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
