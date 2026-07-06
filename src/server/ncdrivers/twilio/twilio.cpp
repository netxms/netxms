/* 
** NetXMS - Network Management System
** Notification channel driver for Twilio
** Copyright (C) 2022-2025 Raden Solutions
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
** File: twilio.cpp
**
**/

#include <ncdrv.h>
#include <nms_util.h>
#include <netxms-version.h>
#include <nxlibcurl.h>

#define DEBUG_TAG _T("ncd.twilio")

static const NCConfigurationTemplate s_config(false, true);

/**
 * Twilio driver class
 */
class TwilioDriver : public NCDriver
{
private:
   char m_sid[128];
   char m_token[128];
   char m_callerId[128];
   char m_voice[128];
   bool m_useTTS;
   bool m_verifyPeer;

   TwilioDriver()
   {
      memset(m_sid, 0, sizeof(m_sid));
      memset(m_token, 0, sizeof(m_token));
      memset(m_callerId, 0, sizeof(m_callerId));
      memset(m_voice, 0, sizeof(m_voice));
      m_useTTS = false;
      m_verifyPeer = false;
   }

public:
   virtual int send(const NotificationContext& context) override;

   static TwilioDriver *createInstance(Config *config);
};

/**
 * Create driver instance
 */
TwilioDriver *TwilioDriver::createInstance(Config *config)
{
   char sid[128] = "", token[128] = "", callerId[128] = "";
   TCHAR voice[128] = _T("");
   uint32_t flags = 0;
   NX_CFG_TEMPLATE configTemplate[] =
   {
      { _T("CallerId"), CT_UTF8_STRING, 0, 0, sizeof(callerId), 0, callerId },
      { _T("SID"), CT_UTF8_STRING, 0, 0, sizeof(sid), 0, sid },
      { _T("Token"), CT_UTF8_STRING, 0, 0, sizeof(token), 0, token },
      { _T("Voice"), CT_STRING, 0, 0, 128, 0, voice },
      { _T("UseTTS"), CT_BOOLEAN_FLAG_32, 0, 0, 1, 0, &flags },
      { _T("verifyPeer"), CT_BOOLEAN_FLAG_32, 0, 0, 2, 0, &flags },
      { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr }
   };

   if (!config->parseTemplate(_T("Twilio"), configTemplate))
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Error parsing driver configuration"));
      return nullptr;
   }

   if ((sid[0] == 0) || (token[0] == 0) || (callerId[0] == 0))
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Missing mandatory driver configuration parameters"));
      return nullptr;
   }

   TwilioDriver *driver = new TwilioDriver();
   strcpy(driver->m_callerId, callerId);
   strcpy(driver->m_sid, sid);
   strcpy(driver->m_token, token);
   char *utf8voice = UTF8StringFromTString(voice);
   strlcpy(driver->m_voice, utf8voice, 128);
   MemFree(utf8voice);
   driver->m_useTTS = (flags & 1) ? true : false;
   driver->m_verifyPeer = (flags & 2) ? true : false;

   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Twilio driver instantiated"));
	return driver;
}

/**
 * Escape string for XML. Returns dynamically allocated UTF-8 string (empty string for null input).
 */
static char *EscapeStringForXMLUtf8(const char *s)
{
   if (s == nullptr)
      return MemCopyStringA("");
   char *xs = MemAllocStringA(strlen(s) * 6 + 1);
   char *out = xs;
   for(const char *p = s; *p != 0; p++)
   {
      switch(*p)
      {
         case '&':
            memcpy(out, "&amp;", 5);
            out += 5;
            break;
         case '<':
            memcpy(out, "&lt;", 4);
            out += 4;
            break;
         case '>':
            memcpy(out, "&gt;", 4);
            out += 4;
            break;
         case '"':
            memcpy(out, "&quot;", 6);
            out += 6;
            break;
         case '\'':
            memcpy(out, "&apos;", 6);
            out += 6;
            break;
         default:
            if (static_cast<unsigned char>(*p) < 32)
               out += snprintf(out, 7, "&#x%02X;", *p);
            else
               *out++ = *p;
            break;
      }
   }
   *out = 0;
   return xs;
}

/**
 * Send notification
 */
int TwilioDriver::send(const NotificationContext& context)
{
   const char *recipient = context.recipient;
   const char *body = context.body;
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
   curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "http,https");
#else
   curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
#endif

   curl_easy_setopt(curl, CURLOPT_HEADER, (long)0); // do not include header in data
   curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ByteStream::curlWriteFunction);
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, static_cast<long>(m_verifyPeer ? 1 : 0));
   curl_easy_setopt(curl, CURLOPT_USERAGENT, "NetXMS Twilio Driver/" NETXMS_VERSION_STRING_A);
   curl_easy_setopt(curl, CURLOPT_USERNAME, m_sid);
   curl_easy_setopt(curl, CURLOPT_PASSWORD, m_token);

   char *request = MemAllocStringA(8192);
   strcpy(request, "From=");
   char *s = curl_easy_escape(curl, m_callerId, 0);
   strlcat(request, s, 8192);
   curl_free(s);
   strlcat(request, "&To=", 8192);
   s = curl_easy_escape(curl, recipient, 0);
   strlcat(request, s, 8192);
   curl_free(s);
   if (m_useTTS)
   {
      strlcat(request, "&Twiml=", 8192);
      char *xbody = EscapeStringForXMLUtf8(body);
      char *payload = MemAllocStringA(strlen(xbody) + strlen(m_voice) + 64);
      if (m_voice[0] != 0)
         sprintf(payload, "<Response><Say voice=\"%s\">%s</Say></Response>", m_voice, xbody);
      else
         sprintf(payload, "<Response><Say>%s</Say></Response>", xbody);
      MemFree(xbody);
      s = curl_easy_escape(curl, payload, 0);
      MemFree(payload);
      strlcat(request, s, 8192);
      curl_free(s);
   }
   else
   {
      strlcat(request, "&Body=", 8192);
      s = curl_easy_escape(curl, body, 0);
      strlcat(request, s, 8192);
      curl_free(s);
   }
   curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request);

   char errorBuffer[CURL_ERROR_SIZE];
   curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);

   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

   char url[1024] = "https://api.twilio.com/2010-04-01/Accounts/";
   strcat(url, m_sid);
   strcat(url, m_useTTS ? "/Calls.json" : "/Messages.json");

   int result = -1;

   if (curl_easy_setopt(curl, CURLOPT_URL, url) == CURLE_OK)
   {
      CURLcode rc = curl_easy_perform(curl);
      if (rc == CURLE_OK)
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Got %d bytes"), static_cast<int>(responseData.size()));
         if (responseData.size() > 0)
         {
            responseData.write('\0');
            const char* data = reinterpret_cast<const char*>(responseData.buffer());
            nxlog_debug_tag(DEBUG_TAG, 8, _T("Full API server response: %hs"), data);

            json_error_t error;
            json_t *response = json_loads(data, 0, &error);
            if (response != nullptr)
            {
               char status[64] = "";
               json_t *jstatus = json_object_get(response, "status");
               if (jstatus != nullptr)
               {
                  if (json_is_string(jstatus))
                     strlcpy(status, json_string_value(jstatus), 64);
                  else if (json_is_integer(jstatus))
                     snprintf(status, 64, "%" JSON_INTEGER_FORMAT, json_integer_value(jstatus));
               }
               if (!strcmp(status, "sent") || !strcmp(status, "queued"))
               {
                  nxlog_debug_tag(DEBUG_TAG, 6, _T("API server responded with success"));
                  result = 0;
               }
               else
               {
                  const char *message = json_string_value(json_object_get(response, "message"));
                  if (message == nullptr)
                     message = json_string_value(json_object_get(response, "error_message"));
                  nxlog_debug_tag(DEBUG_TAG, 4, _T("API server responsed with error (status=%hs message=\"%hs\")"), status, (message != nullptr) ? message : "(null)");
               }
               json_decref(response);
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot parse API server response"));
            }
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("Empty response from API server"));
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
   MemFree(request);
   return result;
}

/**
 * Driver entry point
 */
DECLARE_NCD_ENTRY_POINT(Twilio, &s_config)
{
   if (!InitializeLibCURL())
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("cURL initialization failed"));
      return nullptr;
   }
   return TwilioDriver::createInstance(config);
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
