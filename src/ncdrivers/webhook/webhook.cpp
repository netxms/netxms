/*
** NetXMS - Network Management System
** Generic webhook notification channel driver
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
** File: webhook.cpp
**
** Generic templated HTTP notification channel driver. Sends a notification
** as a customizable HTTP request (POST/PUT/PATCH) with a user-supplied text
** (typically JSON) body template. Covers the majority of REST-style
** notification gateways without writing a vendor-specific driver.
**
** Non-goals (v1) - intentionally out of scope:
**   - Header value placeholder substitution (headers carry literal config values)
**   - Per-recipient URL aliasing (no [Channels] map)
**   - mtime/stat-based template-reload short-circuit (template re-read every send)
**   - Multi-endpoint per channel via [servers/serverN]
**   - GET/DELETE methods
**   - HMAC/signing helpers
**   - Response body capture beyond debug-level tracing
**
** Note: jansson (json_t API, used from Task 5 onward) is pulled in via
** nms_util.h, which applies the correct USE_INTERNAL_JANSSON guard. Do not
** include <jansson.h> directly - that breaks internal-jansson builds. This
** matches the convention used by every other jansson+libcurl NC driver
** (nexmo, slack, telegram, twilio).
**
**/

#include <ncdrv.h>
#include <nms_util.h>
#include <netxms-version.h>
#include <nxlibcurl.h>
#include "webhook_helpers.h"

#define DEBUG_TAG _T("ncd.webhook")

/**
 * Generic webhook driver class
 */
class WebhookDriver : public NCDriver
{
private:
   TCHAR m_url[1024];
   TCHAR m_method[8];
   TCHAR m_contentType[256];
   TCHAR m_templateFile[MAX_PATH];
   StringMap m_headers;
   uint32_t m_timeout;
   bool m_verifyPeer;
   IntegerArray<int> m_successHttpCodes;
   IntegerArray<int> m_retryHttpCodes;
   TCHAR m_successJsonPath[1024];
   TCHAR m_successValue[1024];

   WebhookDriver()
   {
      m_url[0] = 0;
      _tcscpy(m_method, _T("POST"));
      _tcscpy(m_contentType, _T("application/json"));
      m_templateFile[0] = 0;
      m_timeout = 10;
      m_verifyPeer = true;
      m_successJsonPath[0] = 0;
      m_successValue[0] = 0;
   }

public:
   virtual int send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body) override;

   static WebhookDriver *createInstance(Config *config);
};

/**
 * Apply HTTP code list from a config key, falling back to the provided
 * defaults when the key is missing, empty, or malformed.
 *
 * ParseIntList() lives in webhook_helpers.cpp (libcurl-free, unit-tested).
 */
static void ConfigureHttpCodeList(Config *config, const TCHAR *key, IntegerArray<int> *out, const int *defaults, size_t defaultCount)
{
   const TCHAR *value = config->getValue(key, nullptr);
   if ((value != nullptr) && (*value != 0) && ParseIntList(value, out))
      return;
   out->clear();
   for(size_t i = 0; i < defaultCount; i++)
      out->add(defaults[i]);
}

/**
 * Create driver instance.
 *
 * Reads and validates configuration, parses success/retry HTTP code lists,
 * loads the [Headers] map, and verifies that the configured template file is
 * readable. Template content is NOT cached here - it is reloaded on every
 * send() so edits take effect without restarting the channel.
 */
WebhookDriver *WebhookDriver::createInstance(Config *config)
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Creating new webhook driver instance"));

   WebhookDriver *driver = new WebhookDriver();

   NX_CFG_TEMPLATE configTemplate[] =
   {
      { _T("ContentType"), CT_STRING, 0, 0, sizeof(driver->m_contentType) / sizeof(TCHAR), 0, driver->m_contentType },
      { _T("Method"), CT_STRING, 0, 0, sizeof(driver->m_method) / sizeof(TCHAR), 0, driver->m_method },
      { _T("SuccessJsonPath"), CT_STRING, 0, 0, sizeof(driver->m_successJsonPath) / sizeof(TCHAR), 0, driver->m_successJsonPath },
      { _T("SuccessValue"), CT_STRING, 0, 0, sizeof(driver->m_successValue) / sizeof(TCHAR), 0, driver->m_successValue },
      { _T("TemplateFile"), CT_STRING, 0, 0, sizeof(driver->m_templateFile) / sizeof(TCHAR), 0, driver->m_templateFile },
      { _T("Timeout"), CT_LONG, 0, 0, 0, 0, &driver->m_timeout },
      { _T("URL"), CT_STRING, 0, 0, sizeof(driver->m_url) / sizeof(TCHAR), 0, driver->m_url },
      { _T("VerifyPeer"), CT_BOOLEAN, 0, 0, 1, 0, &driver->m_verifyPeer },
      { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr }
   };

   if (!config->parseTemplate(_T("Webhook"), configTemplate))
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Error parsing driver configuration"));
      delete driver;
      return nullptr;
   }

   if (driver->m_url[0] == 0)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Required configuration parameter URL is missing"));
      delete driver;
      return nullptr;
   }

   if (driver->m_templateFile[0] == 0)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Required configuration parameter TemplateFile is missing"));
      delete driver;
      return nullptr;
   }

   _tcsupr(driver->m_method);
   if (_tcscmp(driver->m_method, _T("POST")) && _tcscmp(driver->m_method, _T("PUT")) && _tcscmp(driver->m_method, _T("PATCH")))
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unsupported HTTP method \"%s\" (must be POST, PUT, or PATCH)"), driver->m_method);
      delete driver;
      return nullptr;
   }

   if (_taccess(driver->m_templateFile, R_OK) != 0)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Template file \"%s\" is not readable"), driver->m_templateFile);
      delete driver;
      return nullptr;
   }

   // Guard against a misconfigured timeout: CURLOPT_TIMEOUT of 0 means "no
   // timeout" in libcurl (a single unreachable endpoint would block a
   // notification worker forever), and a negative configured value wraps to a
   // huge unsigned value. Fall back to the default outside a sane range.
   if ((driver->m_timeout == 0) || (driver->m_timeout > 300))
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Configured timeout %u is out of range (1-300s), using default of 10s"), driver->m_timeout);
      driver->m_timeout = 10;
   }

   static const int defaultSuccessCodes[] = { 200, 201, 202, 204 };
   static const int defaultRetryCodes[] = { 429, 502, 503, 504 };
   ConfigureHttpCodeList(config, _T("/Webhook/SuccessHttpCodes"), &driver->m_successHttpCodes,
            defaultSuccessCodes, sizeof(defaultSuccessCodes) / sizeof(int));
   ConfigureHttpCodeList(config, _T("/Webhook/RetryHttpCodes"), &driver->m_retryHttpCodes,
            defaultRetryCodes, sizeof(defaultRetryCodes) / sizeof(int));

   unique_ptr<ObjectArray<ConfigEntry>> headers = config->getSubEntries(_T("/Headers"), _T("*"));
   if (headers != nullptr)
   {
      for(int i = 0; i < headers->size(); i++)
      {
         ConfigEntry *header = headers->get(i);
         driver->m_headers.set(header->getName(), header->getValue());
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Added header %s"), header->getName());
      }
   }

   nxlog_debug_tag(DEBUG_TAG, 5, _T("Configuration: method=%s, contentType=%s, timeout=%u, verifyPeer=%s, successCodes=%d, retryCodes=%d, jsonPath=%s"),
            driver->m_method, driver->m_contentType, driver->m_timeout, driver->m_verifyPeer ? _T("yes") : _T("no"),
            driver->m_successHttpCodes.size(), driver->m_retryHttpCodes.size(),
            (driver->m_successJsonPath[0] != 0) ? driver->m_successJsonPath : _T("(none)"));
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Webhook driver instantiated"));
   return driver;
}

/**
 * Send notification.
 *
 * Reloads the template, builds the effective URL and request body, executes
 * the HTTP request with libcurl, and releases all resources on every exit
 * path. The HTTP status code is matched against the configured success/retry
 * code lists; when a SuccessJsonPath is configured the response body is
 * parsed and the value at that JSON Pointer is compared to SuccessValue.
 *
 * Returns 0 on success, 10 to request a retry (retryable HTTP code, or
 * success code with a JSON-path value mismatch), and -1 on hard failure.
 */
int WebhookDriver::send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body)
{
   // Reload template from disk on every send so edits take effect without
   // restarting the channel (sends are infrequent; the read is cheap).
   char *tpl = LoadFileAsUTF8String(m_templateFile);
   if (tpl == nullptr)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Cannot read template file \"%s\""), m_templateFile);
      return -1;
   }

   String effectiveUrl = SubstitutePlaceholdersURL(m_url, recipient, subject, body);
   char *utf8url = effectiveUrl.getUTF8String();
   char *requestBody = SubstitutePlaceholdersJSONUtf8(tpl, recipient, subject, body);

   nxlog_debug_tag(DEBUG_TAG, 7, _T("Effective URL: %s"), effectiveUrl.cstr());
   nxlog_debug_tag(DEBUG_TAG, 7, _T("Request body: %hs"), requestBody);

   struct curl_slist *headers = nullptr;
   StringBuffer contentTypeHeader(_T("Content-Type: "));
   contentTypeHeader.append(m_contentType);
   char *contentTypeHeaderUtf8 = contentTypeHeader.getUTF8String();
   headers = curl_slist_append(headers, contentTypeHeaderUtf8);
   MemFree(contentTypeHeaderUtf8);

   // Extra headers carry literal config values - no placeholder substitution
   // (out of scope for v1). Header names/values are intentionally not logged.
   for(KeyValuePair<const TCHAR> *h : m_headers)
   {
      StringBuffer headerLine(h->key);
      headerLine.append(_T(": "));
      headerLine.append(h->value);
      char *headerLineUtf8 = headerLine.getUTF8String();
      headers = curl_slist_append(headers, headerLineUtf8);
      MemFree(headerLineUtf8);
   }

   int result = 0;

   CURL *curl = curl_easy_init();
   if (curl == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_init() failed"));
      curl_slist_free_all(headers);
      MemFree(utf8url);
      MemFree(requestBody);
      MemFree(tpl);
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
   curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(m_timeout));
   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ByteStream::curlWriteFunction);
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, static_cast<long>(m_verifyPeer ? 1 : 0));
   curl_easy_setopt(curl, CURLOPT_USERAGENT, "NetXMS Webhook Driver/" NETXMS_VERSION_STRING_A);

   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

   char errorBuffer[CURL_ERROR_SIZE];
   curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);

   // Method is already uppercased and validated to be POST/PUT/PATCH in
   // createInstance(). POST uses CURLOPT_POST; PUT/PATCH use a custom request
   // verb. All three carry the substituted body.
   if (!_tcscmp(m_method, _T("POST")))
      curl_easy_setopt(curl, CURLOPT_POST, (long)1);
   else
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, !_tcscmp(m_method, _T("PUT")) ? "PUT" : "PATCH");
   curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(strlen(requestBody)));
   curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody);

   curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

   if (curl_easy_setopt(curl, CURLOPT_URL, utf8url) != CURLE_OK)
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
      else
      {
         long httpCode = 0;
         curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
         nxlog_debug_tag(DEBUG_TAG, 6, _T("HTTP response %03d, %d bytes"),
                  static_cast<int>(httpCode), static_cast<int>(responseData.size()));

         if (m_successHttpCodes.contains(static_cast<int>(httpCode)))
         {
            if (m_successJsonPath[0] == 0)
            {
               result = 0;
            }
            else
            {
               responseData.write('\0');   // null-terminate for json_loads()
               json_error_t error;
               json_t *responseJson = json_loads(reinterpret_cast<const char*>(responseData.buffer()), 0, &error);
               if (responseJson == nullptr)
               {
                  nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot parse response JSON (%hs)"), error.text);
                  result = -1;
               }
               else
               {
                  String value = JsonPathLookup(responseJson, m_successJsonPath);
                  nxlog_debug_tag(DEBUG_TAG, 6, _T("JSON path %s resolved to \"%s\" (expected \"%s\")"),
                           m_successJsonPath, value.cstr(), m_successValue);
                  result = !_tcscmp(value, m_successValue) ? 0 : 10;   // value mismatch is retryable
                  json_decref(responseJson);
               }
            }
         }
         else if (m_retryHttpCodes.contains(static_cast<int>(httpCode)))
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("HTTP response %03d treated as retryable"), static_cast<int>(httpCode));
            result = 10;
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("HTTP response %03d treated as failure"), static_cast<int>(httpCode));
            result = -1;
         }
      }
   }

   curl_slist_free_all(headers);
   curl_easy_cleanup(curl);
   MemFree(utf8url);
   MemFree(requestBody);
   MemFree(tpl);
   return result;
}

/**
 * Configuration template
 */
static const NCConfigurationTemplate s_config(true, true);

/**
 * Driver entry point
 */
DECLARE_NCD_ENTRY_POINT(Webhook, &s_config)
{
   if (!InitializeLibCURL())
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("cURL initialization failed"));
      return nullptr;
   }
   return WebhookDriver::createInstance(config);
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
