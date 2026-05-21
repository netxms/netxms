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
** Note: jansson (json_t API) is pulled in via nms_util.h, which applies the
** correct USE_INTERNAL_JANSSON guard. Do not include <jansson.h> directly -
** that breaks internal-jansson builds. This matches the convention used by
** every other jansson+libcurl NC driver (nexmo, slack, telegram, twilio).
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
 *
 * Config strings consumed by libcurl are stored as UTF-8 (char[]) so the
 * per-send hot path avoids repeated TCHAR->UTF-8 conversions. The request
 * header list (Content-Type + [Headers] section) is built once at init and
 * reused on every send. The body template path stays in TCHAR because it is
 * passed to _taccess() / LoadFileAsUTF8String() which take filesystem chars.
 */
class WebhookDriver : public NCDriver
{
private:
   char m_url[1024];
   char m_method[8];
   TCHAR m_templateFile[MAX_PATH];
   curl_slist *m_headers;
   uint32_t m_timeout;
   bool m_verifyPeer;
   IntegerArray<int> m_successHttpCodes;
   IntegerArray<int> m_retryHttpCodes;
   char m_successJsonPath[1024];
   char m_successValue[1024];

   WebhookDriver()
   {
      m_url[0] = 0;
      strcpy(m_method, "POST");
      m_templateFile[0] = 0;
      m_headers = nullptr;
      m_timeout = 10;
      m_verifyPeer = true;
      m_successJsonPath[0] = 0;
      m_successValue[0] = 0;
   }

public:
   virtual ~WebhookDriver()
   {
      if (m_headers != nullptr)
         curl_slist_free_all(m_headers);
   }

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
 * Append a header line ("Name: Value") built from TCHAR config values to the
 * pre-built curl_slist. Marshals through UTF-8 (curl expects UTF-8 / ASCII).
 */
static curl_slist *AppendHeader(curl_slist *list, const TCHAR *name, const TCHAR *value)
{
   StringBuffer line(name);
   line.append(_T(": "));
   line.append(value);
   char *utf8 = line.getUTF8String();
   curl_slist *result = curl_slist_append(list, utf8);
   MemFree(utf8);
   return result;
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

   TCHAR url[1024] = { 0 };
   TCHAR method[8] = _T("POST");
   TCHAR contentType[256] = _T("application/json");
   TCHAR templateFile[MAX_PATH] = { 0 };
   TCHAR successJsonPath[1024] = { 0 };
   TCHAR successValue[1024] = { 0 };
   uint32_t timeout = 10;
   bool verifyPeer = true;

   NX_CFG_TEMPLATE configTemplate[] =
   {
      { _T("ContentType"), CT_STRING, 0, 0, sizeof(contentType) / sizeof(TCHAR), 0, contentType },
      { _T("Method"), CT_STRING, 0, 0, sizeof(method) / sizeof(TCHAR), 0, method },
      { _T("SuccessJsonPath"), CT_STRING, 0, 0, sizeof(successJsonPath) / sizeof(TCHAR), 0, successJsonPath },
      { _T("SuccessValue"), CT_STRING, 0, 0, sizeof(successValue) / sizeof(TCHAR), 0, successValue },
      { _T("TemplateFile"), CT_STRING, 0, 0, sizeof(templateFile) / sizeof(TCHAR), 0, templateFile },
      { _T("Timeout"), CT_LONG, 0, 0, 0, 0, &timeout },
      { _T("URL"), CT_STRING, 0, 0, sizeof(url) / sizeof(TCHAR), 0, url },
      { _T("VerifyPeer"), CT_BOOLEAN, 0, 0, 1, 0, &verifyPeer },
      { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr }
   };

   if (!config->parseTemplate(_T("Webhook"), configTemplate))
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Error parsing driver configuration"));
      return nullptr;
   }

   if (url[0] == 0)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Required configuration parameter URL is missing"));
      return nullptr;
   }

   if (templateFile[0] == 0)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Required configuration parameter TemplateFile is missing"));
      return nullptr;
   }

   _tcsupr(method);
   if (_tcscmp(method, _T("POST")) && _tcscmp(method, _T("PUT")) && _tcscmp(method, _T("PATCH")))
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unsupported HTTP method \"%s\" (must be POST, PUT, or PATCH)"), method);
      return nullptr;
   }

   if (_taccess(templateFile, R_OK) != 0)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Template file \"%s\" is not readable"), templateFile);
      return nullptr;
   }

   // Guard against a misconfigured timeout: CURLOPT_TIMEOUT of 0 means "no
   // timeout" in libcurl (a single unreachable endpoint would block a
   // notification worker forever), and a negative configured value wraps to a
   // huge unsigned value. Fall back to the default outside a sane range.
   if ((timeout == 0) || (timeout > 300))
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Configured timeout %u is out of range (1-300s), using default of 10s"), timeout);
      timeout = 10;
   }

   WebhookDriver *driver = new WebhookDriver();
   tchar_to_utf8(url, -1, driver->m_url, sizeof(driver->m_url));
   tchar_to_utf8(method, -1, driver->m_method, sizeof(driver->m_method));
   tchar_to_utf8(successJsonPath, -1, driver->m_successJsonPath, sizeof(driver->m_successJsonPath));
   tchar_to_utf8(successValue, -1, driver->m_successValue, sizeof(driver->m_successValue));
   _tcscpy(driver->m_templateFile, templateFile);
   driver->m_timeout = timeout;
   driver->m_verifyPeer = verifyPeer;

   static const int defaultSuccessCodes[] = { 200, 201, 202, 204 };
   static const int defaultRetryCodes[] = { 429, 502, 503, 504 };
   ConfigureHttpCodeList(config, _T("/Webhook/SuccessHttpCodes"), &driver->m_successHttpCodes,
            defaultSuccessCodes, sizeof(defaultSuccessCodes) / sizeof(int));
   ConfigureHttpCodeList(config, _T("/Webhook/RetryHttpCodes"), &driver->m_retryHttpCodes,
            defaultRetryCodes, sizeof(defaultRetryCodes) / sizeof(int));

   // Pre-build the request header list once - it does not change across sends.
   driver->m_headers = AppendHeader(nullptr, _T("Content-Type"), contentType);

   // Extra headers carry literal config values - no placeholder substitution
   // (out of scope for v1). Header names/values are intentionally not logged.
   unique_ptr<ObjectArray<ConfigEntry>> headers = config->getSubEntries(_T("/Headers"), _T("*"));
   if (headers != nullptr)
   {
      for(int i = 0; i < headers->size(); i++)
      {
         ConfigEntry *header = headers->get(i);
         driver->m_headers = AppendHeader(driver->m_headers, header->getName(), header->getValue());
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Added header %s"), header->getName());
      }
   }

   nxlog_debug_tag(DEBUG_TAG, 5, _T("Configuration: method=%hs, contentType=%s, timeout=%u, verifyPeer=%s, successCodes=%d, retryCodes=%d, jsonPath=%hs"),
            driver->m_method, contentType, driver->m_timeout, driver->m_verifyPeer ? _T("yes") : _T("no"),
            driver->m_successHttpCodes.size(), driver->m_retryHttpCodes.size(),
            (driver->m_successJsonPath[0] != 0) ? driver->m_successJsonPath : "(none)");
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Webhook driver instantiated"));
   return driver;
}

/**
 * Substitute ${recipient}, ${subject}, ${body} placeholders in a UTF-8 URL
 * template, replacing each with its percent-encoded (curl_easy_escape) value.
 * Unknown tokens are left literal so user typos stay visible.
 *
 * Returns a heap-allocated, null-terminated UTF-8 buffer (caller MemFree).
 */
static char *SubstituteUrlPlaceholders(CURL *curl, const char *urlTemplate, const TCHAR *recipient, const TCHAR *subject, const TCHAR *body)
{
   char *utf8r = UTF8StringFromTString(recipient);
   char *utf8s = UTF8StringFromTString(subject);
   char *utf8b = UTF8StringFromTString(body);
   char *recipientRepl = curl_easy_escape(curl, utf8r, 0);
   char *subjectRepl = curl_easy_escape(curl, utf8s, 0);
   char *bodyRepl = curl_easy_escape(curl, utf8b, 0);
   MemFree(utf8r);
   MemFree(utf8s);
   MemFree(utf8b);

   ByteStream out(strlen(urlTemplate) + 64);
   const char *p = urlTemplate;
   while(*p != 0)
   {
      if ((p[0] == '$') && (p[1] == '{'))
      {
         const char *end = strchr(p + 2, '}');
         if (end == nullptr)
         {
            out.write(p, strlen(p));   // unterminated placeholder - copy rest literally
            break;
         }

         size_t tokenLen = static_cast<size_t>(end - (p + 2));
         const char *repl = nullptr;
         if ((tokenLen == 9) && !memcmp(p + 2, "recipient", 9))
            repl = recipientRepl;
         else if ((tokenLen == 7) && !memcmp(p + 2, "subject", 7))
            repl = subjectRepl;
         else if ((tokenLen == 4) && !memcmp(p + 2, "body", 4))
            repl = bodyRepl;

         if (repl != nullptr)
            out.write(repl, strlen(repl));
         else
            out.write(p, static_cast<size_t>(end + 1 - p));   // unknown token - literal

         p = end + 1;
      }
      else
      {
         out.write(*p);
         p++;
      }
   }

   curl_free(recipientRepl);
   curl_free(subjectRepl);
   curl_free(bodyRepl);

   size_t len = out.size();
   char *result = MemAllocArray<char>(len + 1);
   if (len > 0)
      memcpy(result, out.buffer(), len);
   result[len] = 0;
   return result;
}

/**
 * Stringify a JSON scalar node into a heap-allocated UTF-8 buffer for
 * comparison against SuccessValue. Objects, arrays and null nodes yield an
 * empty string. Caller frees with MemFree.
 *
 * Real values are serialized through jansson rather than snprintf("%f"): "%f"
 * forces six decimals (1.5 -> "1.500000") and honours LC_NUMERIC (decimal
 * separator could be ','), both of which would spuriously fail the equality
 * check. jansson's serializer is locale-independent (always '.') and emits a
 * compact %g-style form (1.5 -> "1.5").
 */
static char *JsonValueToUtf8(json_t *node)
{
   if (json_is_string(node))
      return MemCopyStringA(json_string_value(node));
   if (json_is_integer(node))
   {
      char buffer[32];
      snprintf(buffer, sizeof(buffer), INT64_FMTA, static_cast<int64_t>(json_integer_value(node)));
      return MemCopyStringA(buffer);
   }
   if (json_is_real(node))
   {
      char *s = json_dumps(node, JSON_ENCODE_ANY | JSON_REAL_PRECISION(15));
      return (s != nullptr) ? s : MemCopyStringA("");
   }
   if (json_is_true(node))
      return MemCopyStringA("true");
   if (json_is_false(node))
      return MemCopyStringA("false");
   if (json_is_null(node))
      return MemCopyStringA("null");
   return MemCopyStringA("");
}

/**
 * Send notification.
 *
 * Reloads the template, builds the effective URL and request body, executes
 * the HTTP request with libcurl, and releases all resources on every exit
 * path. The HTTP status code is matched against the configured success/retry
 * code lists; when a SuccessJsonPath is configured the response body is
 * parsed and the value at that path is compared to SuccessValue.
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

   CURL *curl = curl_easy_init();
   if (curl == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_init() failed"));
      MemFree(tpl);
      return -1;
   }

   char *effectiveUrl = SubstituteUrlPlaceholders(curl, m_url, recipient, subject, body);
   char *requestBody = SubstitutePlaceholdersJSONUtf8(tpl, recipient, subject, body);

   nxlog_debug_tag(DEBUG_TAG, 7, _T("Effective URL: %hs"), effectiveUrl);
   nxlog_debug_tag(DEBUG_TAG, 7, _T("Request body: %hs"), requestBody);

   int result = 0;

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
   if (!strcmp(m_method, "POST"))
      curl_easy_setopt(curl, CURLOPT_POST, (long)1);
   else
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, !strcmp(m_method, "PUT") ? "PUT" : "PATCH");
   curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(strlen(requestBody)));
   curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody);

   curl_easy_setopt(curl, CURLOPT_HTTPHEADER, m_headers);

   if (curl_easy_setopt(curl, CURLOPT_URL, effectiveUrl) != CURLE_OK)
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
                  json_t *node = json_object_get_by_path_a(responseJson, m_successJsonPath);
                  char *value = JsonValueToUtf8(node);
                  nxlog_debug_tag(DEBUG_TAG, 6, _T("JSON path %hs resolved to \"%hs\" (expected \"%hs\")"),
                           m_successJsonPath, value, m_successValue);
                  result = !strcmp(value, m_successValue) ? 0 : 10;   // value mismatch is retryable
                  MemFree(value);
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

   curl_easy_cleanup(curl);
   MemFree(effectiveUrl);
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
