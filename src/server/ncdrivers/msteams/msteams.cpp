/*
** NetXMS - Network Management System
** Notification channel driver for Microsoft Teams
** Copyright (C) 2014-2026 Raden Solutions
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
** Sends notifications to Microsoft Teams Workflows (Power Automate) webhook URLs
** as Adaptive Cards wrapped in a Bot Framework message envelope. Legacy Office 365
** connector webhooks are not supported (retired by Microsoft).
**
**/

#include <ncdrv.h>
#include <nms_core.h>
#include <netxms-version.h>
#include <nxlibcurl.h>

#define DEBUG_TAG _T("ncd.msteams")

/**
 * Microsoft Teams driver class
 */
class MicrosoftTeamsDriver : public NCDriver
{
private:
   bool m_severityMapping;
   StringMap m_channels;

   MicrosoftTeamsDriver(bool severityMapping) : NCDriver()
   {
      m_severityMapping = severityMapping;
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

   bool severityMapping = false;
   NX_CFG_TEMPLATE configTemplate[] =
	{
      { _T("EventSeverityMapping"), CT_BOOLEAN, 0, 0, 1, 0, &severityMapping },
		{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr }
	};

	if (!config->parseTemplate(_T("MicrosoftTeams"), configTemplate))
	{
	   nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Error parsing driver configuration"));
	   return nullptr;
	}

   if ((config->getValue(_T("/MicrosoftTeams/UseMessageCards")) != nullptr) || (config->getValue(_T("/MicrosoftTeams/ThemeColor")) != nullptr))
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Configuration parameters UseMessageCards and ThemeColor are ignored (legacy Office 365 connector support was removed)"));

   MicrosoftTeamsDriver *driver = new MicrosoftTeamsDriver(severityMapping);
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
 * Get adaptive card container style for given event severity
 */
static const char *StyleFromSeverity(uint32_t severity)
{
   switch(severity)
   {
      case EVENT_SEVERITY_NORMAL:
         return "good";
      case EVENT_SEVERITY_WARNING:
      case EVENT_SEVERITY_MINOR:
         return "warning";
      case EVENT_SEVERITY_MAJOR:
      case EVENT_SEVERITY_CRITICAL:
         return "attention";
      default:
         return "default";
   }
}

/**
 * Build message payload (adaptive card in Bot Framework message envelope)
 */
static char *BuildMessagePayload(const NotificationContext& context, bool severityMapping)
{
   json_t *items = json_array();
   if ((context.subject != nullptr) && (context.subject[0] != 0))
   {
      json_t *title = json_object();
      json_object_set_new(title, "type", json_string("TextBlock"));
      json_object_set_new(title, "size", json_string("Large"));
      json_object_set_new(title, "weight", json_string("Bolder"));
      json_object_set_new(title, "wrap", json_true());
      json_object_set_new(title, "text", json_string(context.subject));
      json_array_append_new(items, title);
   }

   json_t *text = json_object();
   json_object_set_new(text, "type", json_string("TextBlock"));
   json_object_set_new(text, "wrap", json_true());
   json_object_set_new(text, "text", json_string((context.markdownBody != nullptr) ? context.markdownBody : CHECK_NULL_EX_A(context.body)));
   json_array_append_new(items, text);

   json_t *cardBody;
   if (severityMapping && (context.event != nullptr))
   {
      json_t *container = json_object();
      json_object_set_new(container, "type", json_string("Container"));
      json_object_set_new(container, "style", json_string(StyleFromSeverity(context.event->getSeverity())));
      json_object_set_new(container, "bleed", json_true());
      json_object_set_new(container, "items", items);
      cardBody = json_array();
      json_array_append_new(cardBody, container);
   }
   else
   {
      cardBody = items;
   }

   json_t *card = json_object();
   json_object_set_new(card, "$schema", json_string("http://adaptivecards.io/schemas/adaptive-card.json"));
   json_object_set_new(card, "type", json_string("AdaptiveCard"));
   json_object_set_new(card, "version", json_string("1.4"));
   json_t *msteams = json_object();
   json_object_set_new(msteams, "width", json_string("Full"));
   json_object_set_new(card, "msteams", msteams);
   json_object_set_new(card, "body", cardBody);

   json_t *attachment = json_object();
   json_object_set_new(attachment, "contentType", json_string("application/vnd.microsoft.card.adaptive"));
   json_object_set_new(attachment, "contentUrl", json_null());
   json_object_set_new(attachment, "content", card);

   json_t *root = json_object();
   json_object_set_new(root, "type", json_string("message"));
   json_t *attachments = json_array();
   json_array_append_new(attachments, attachment);
   json_object_set_new(root, "attachments", attachments);

   char *payload = json_dumps(root, JSON_COMPACT);
   json_decref(root);
   return payload;
}

/**
 * Send notification
 */
int MicrosoftTeamsDriver::send(const NotificationContext& context)
{
   char *request = BuildMessagePayload(context, m_severityMapping);
   if (request == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot serialize message payload"));
      return -1;
   }
   nxlog_debug_tag(DEBUG_TAG, 7, _T("Prepared request: %hs"), request);

   // Attempt to lookup URL alias (channel mappings are stored as wide strings)
   TCHAR *key = TStringFromUTF8String(context.recipient);
   const TCHAR *alias = m_channels.get(key);
   char *url = (alias != nullptr) ? UTF8StringFromTString(alias) : MemCopyStringA(context.recipient);
   MemFree(key);

   CURL *curl = curl_easy_init();
   if (curl == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_init() failed"));
      MemFree(url);
      MemFree(request);
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

   curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request);

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
      // Workflow trigger normally responds with 202 Accepted and empty body
      long httpCode = 0;
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
      if ((httpCode >= 200) && (httpCode <= 299))
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("Message successfully sent (HTTP response code %03d)"), static_cast<int>(httpCode));
      }
      else
      {
         responseData.write('\0');
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Error response from webhook: HTTP response code is %03d (%hs)"),
            static_cast<int>(httpCode), reinterpret_cast<const char*>(responseData.buffer()));
         if (httpCode == 429 || httpCode == 502 || httpCode == 503 || httpCode == 504)
            result = 10;
         else
            result = -1;
      }
   }

   curl_slist_free_all(headers);
   curl_easy_cleanup(curl);
   MemFree(url);
   MemFree(request);
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
