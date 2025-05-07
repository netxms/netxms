/* 
** NetXMS - Network Management System
** Notification channel driver for Mattermost
** Copyright (C) 2024-2025 Raden Solutions
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
** File: mattermost.cpp
**
**/

#include <ncdrv.h>
#include <nms_util.h>
#include <netxms-version.h>
#include <nxlibcurl.h>

#define DEBUG_TAG _T("ncd.mattermost")

#define MM_USE_ATTACHMENTS 0x01

/**
 * Mattermost driver class
 */
class MattermostDriver : public NCDriver
{
private:
   char *m_postUrl;
   char *m_statusUrl;
   char m_token[64];
   TCHAR *m_footer;
   TCHAR m_color[16];
   uint32_t m_flags;
   StringMap m_channels;

   MattermostDriver(uint32_t flags, const char *serverUrl, const char *token, const TCHAR *color, const TCHAR *footer) : NCDriver()
   {
      m_flags = flags;

      size_t l = strlen(serverUrl);
      m_postUrl = MemAllocStringA(l + 16);
      strcpy(m_postUrl, serverUrl);
      strcat(m_postUrl, "api/v4/posts");

      m_statusUrl = MemAllocStringA(l + 16);
      strcpy(m_statusUrl, serverUrl);
      strcat(m_statusUrl, "api/v4/users/me");

      strlcpy(m_token, token, 64);
      m_footer = MemCopyString(footer);
      _tcslcpy(m_color, color, 16);
   }

public:
   virtual ~MattermostDriver()
   {
      MemFree(m_postUrl);
      MemFree(m_statusUrl);
      MemFree(m_footer);
   }

   virtual int send(const TCHAR* recipient, const TCHAR* subject, const TCHAR* body) override;

   virtual bool checkHealth() override;

   static MattermostDriver *createInstance(Config *config);
};

/**
 * Create driver instance
 */
MattermostDriver *MattermostDriver::createInstance(Config *config)
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Creating new Mattermost driver instance"));

   uint32_t flags = MM_USE_ATTACHMENTS;
   TCHAR colorDefinition[16] = _T("");
   TCHAR footer[1024] = _T("");
   char url[1024] = "";
   char token[64] = "";
   NX_CFG_TEMPLATE configTemplate[] = 
	{
      { _T("AuthToken"), CT_MB_STRING, 0, 0, sizeof(token), 0, token },
		{ _T("Color"), CT_STRING, 0, 0, sizeof(colorDefinition) / sizeof(TCHAR), 0, colorDefinition },
      { _T("Footer"), CT_STRING, 0, 0, sizeof(footer) / sizeof(TCHAR), 0, footer },
      { _T("ServerURL"), CT_MB_STRING, 0, 0, sizeof(url), 0, url },
      { _T("UseAttachments"), CT_BOOLEAN_FLAG_32, 0, 0, MM_USE_ATTACHMENTS, 0, &flags },
		{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr }
	};

	if (!config->parseTemplate(_T("Mattermost"), configTemplate))
	{
	   nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Error parsing driver configuration"));
	   return nullptr;
	}

	if (url[0] == 0)
	{
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Mattermost server URL is mandatory but not provided"));
      return nullptr;
	}
	if (url[strlen(url) - 1] != '/')
	   strlcat(url, "/", 1024);

   if (token[0] == 0)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Mattermost authentication token is mandatory but not provided"));
      return nullptr;
   }

   if (colorDefinition[0] != 0)
   {
      Color color = Color::parseCSS(colorDefinition);
      _tcscpy(colorDefinition, color.toCSS(true));
   }

   MattermostDriver *driver = new MattermostDriver(flags, url, token, colorDefinition, footer);
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Mattermost driver instantiated"));

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
 * Send notification
 */
int MattermostDriver::send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body)
{
   // Attempt to lookup channel alias
   const TCHAR *channelId = m_channels.get(recipient);
   if (channelId == nullptr)
      channelId = recipient;

   String jsubject = EscapeStringForJSON(subject);
   String jbody = EscapeStringForJSON(body);

   StringBuffer request(_T("{ \"channel_id\":\""));
   request.append(channelId);
   request.append(_T("\", "));
   if (m_flags & MM_USE_ATTACHMENTS)
   {
      request.append(_T("\"message\":\"\", \"props\":{ \"attachments\": [{ \"fallback\":\""));
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
      if (m_color[0] != 0)
      {
         request.append(_T(", \"color\":\""));
         request.append(m_color);
         request.append(_T("\""));
      }
      if (m_footer[0] != 0)
      {
         request.append(_T(", \"footer\":\""));
         request.append(EscapeStringForJSON(m_footer));
         request.append(_T("\""));
      }
      request.append(_T("}]}"));
   }
   else
   {
      request.append(_T("\"message\":\""));
      request.append(jsubject);
      if (!jsubject.isEmpty() && !jbody.isEmpty())
         request.append(_T("\\n\\n"));
      request.append(jbody);
      request.append(_T("\""));
   }
   request.append(_T(" }"));
   nxlog_debug_tag(DEBUG_TAG, 7, _T("Prepared request: %s"), request.cstr());

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
   curl_easy_setopt(curl, CURLOPT_USERAGENT, "NetXMS Mattermost Driver/" NETXMS_VERSION_STRING_A);

   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

   char *json = request.getUTF8String();
   curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);

   struct curl_slist *headers = nullptr;
   headers = curl_slist_append(headers, "Content-Type: application/json");
   char authHeader[128] = "Authorization: Bearer ";
   strlcat(authHeader, m_token, 128);
   headers = curl_slist_append(headers, authHeader);
   curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

   char errorBuffer[CURL_ERROR_SIZE];
   curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);

   int result = 0;

   if (curl_easy_setopt(curl, CURLOPT_URL, m_postUrl) != CURLE_OK)
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
      if (httpCode == 201)
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("Message successfully sent"));
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Error response from server: HTTP response code is %d"), httpCode);
         result = -1;
      }
   }

   curl_slist_free_all(headers);
   curl_easy_cleanup(curl);
   MemFree(json);
   return result;
}

/**
 * Check driver health
 */
bool MattermostDriver::checkHealth()
{
   CURL *curl = curl_easy_init();
   if (curl == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_init() failed"));
      return false;
   }

#if HAVE_DECL_CURLOPT_NOSIGNAL
   curl_easy_setopt(curl, CURLOPT_NOSIGNAL, (long)1);
#endif

   curl_easy_setopt(curl, CURLOPT_HEADER, (long)0); // do not include header in data
   curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ByteStream::curlWriteFunction);
   curl_easy_setopt(curl, CURLOPT_USERAGENT, "NetXMS Mattermost Driver/" NETXMS_VERSION_STRING_A);

   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

   struct curl_slist *headers = nullptr;
   headers = curl_slist_append(headers, "Content-Type: application/json");
   char authHeader[128] = "Authorization: Bearer ";
   strlcat(authHeader, m_token, 128);
   headers = curl_slist_append(headers, authHeader);
   curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

   char errorBuffer[CURL_ERROR_SIZE];
   curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);

   bool success = true;

   if (curl_easy_setopt(curl, CURLOPT_URL, m_statusUrl) != CURLE_OK)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_setopt(CURLOPT_URL) failed"));
      success = false;
   }

   if (success)
   {
      CURLcode rc = curl_easy_perform(curl);
      if (rc != CURLE_OK)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Call to curl_easy_perform() failed (%d: %hs)"), rc, errorBuffer);
         success = false;
      }
   }

   if (success)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Got %d bytes"), static_cast<int>(responseData.size()));
      long httpCode = 0;
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
      if (httpCode == 200)
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("Status check successful"));
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Error response from server: HTTP response code is %d"), httpCode);
         success = false;
      }
   }

   curl_slist_free_all(headers);
   curl_easy_cleanup(curl);

   return success;
}

/**
 * Configuration template
 */
static const NCConfigurationTemplate s_config(true, true);

/**
 * Driver entry point
 */
DECLARE_NCD_ENTRY_POINT(Mattermost, &s_config)
{
   if (!InitializeLibCURL())
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("cURL initialization failed"));
      return nullptr;
   }
   return MattermostDriver::createInstance(config);
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
