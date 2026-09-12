/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: nxsl_http.cpp
**
**/

#include "nxcore.h"
#include <nxlibcurl.h>
#include <netxms-version.h>

#define DEBUG_TAG L"nxsl.http"

/**
 * Guardrails for requests issued from scripts
 */
#define HTTP_MAX_RESPONSE_SIZE      (16 * 1024 * 1024)
#define HTTP_MAX_REDIRECTS          10

/**
 * Configuration (server configuration variables NXSL.HttpRequests.*)
 */
static bool s_enabled = true;
static int s_defaultTimeout = 30;
static int s_maxTimeout = 300;

/**
 * Load configuration. Called at server startup and on change of any NXSL.HttpRequests.* configuration variable.
 */
void LoadNXSLHttpRequestsConfiguration()
{
   s_enabled = ConfigReadBoolean(L"NXSL.HttpRequests.Enabled", true);
   int maxTimeout = ConfigReadInt(L"NXSL.HttpRequests.MaxTimeout", 300);
   if (maxTimeout < 1)
      maxTimeout = 1;
   int defaultTimeout = ConfigReadInt(L"NXSL.HttpRequests.DefaultTimeout", 30);
   if (defaultTimeout < 1)
      defaultTimeout = 1;
   else if (defaultTimeout > maxTimeout)
      defaultTimeout = maxTimeout;
   s_maxTimeout = maxTimeout;
   s_defaultTimeout = defaultTimeout;
   nxlog_debug_tag(DEBUG_TAG, 3, L"HTTP requests from NXSL scripts %s (default timeout %d seconds, maximum timeout %d seconds)",
      s_enabled ? L"enabled" : L"disabled", s_defaultTimeout, s_maxTimeout);
}

/**
 * Authentication types
 */
enum class HttpAuthType
{
   UNSET = -1,
   NONE = 0,
   BASIC = 1,
   DIGEST = 2,
   NTLM = 3,
   BEARER = 4
};

/**
 * HTTP client settings. Used both as session-wide defaults and as per-request overrides;
 * negative integers, UNSET auth type, and empty strings mean "not set".
 */
struct HttpClientSettings
{
   int timeout;               // seconds
   int verifyPeer;
   int verifyHost;
   int followRedirects;
   int64_t maxResponseSize;   // bytes
   HttpAuthType authType;
   SharedString login;
   SharedString password;     // password or bearer token
   SharedString userAgent;
   SharedString proxy;
   StringMap headers;

   HttpClientSettings() : timeout(-1), verifyPeer(-1), verifyHost(-1), followRedirects(-1), maxResponseSize(-1), authType(HttpAuthType::UNSET) { }
};

/**
 * HTTP request
 */
struct HttpRequestData
{
   HttpClientSettings settings;
   SharedString method;    // always uppercase
   SharedString url;
   char *body;       // UTF-8, nullptr if no body
   size_t bodySize;

   HttpRequestData(const wchar_t *m, const wchar_t *u) : method(m), url(u)
   {
      body = nullptr;
      bodySize = 0;
   }

   ~HttpRequestData()
   {
      MemFree(body);
   }

   void setBody(const char *utf8Text)
   {
      MemFree(body);
      body = MemCopyStringA(utf8Text);
      bodySize = (body != nullptr) ? strlen(body) : 0;
   }
};

/**
 * HTTP response
 */
struct HttpResponseData
{
   int statusCode;         // 0 if request failed before receiving response
   StringMap headers;      // header names are converted to lower case
   ByteStream body;
   int32_t responseTime;   // milliseconds
   SharedString errorMessage;    // empty on success
   SharedString effectiveUrl;
   json_t *json;           // cached parsed body
   bool jsonParsed;

   HttpResponseData() : body(32768)
   {
      statusCode = 0;
      responseTime = 0;
      json = nullptr;
      jsonParsed = false;
      body.setAllocationStep(32768);
   }

   ~HttpResponseData()
   {
      if (json != nullptr)
         json_decref(json);
   }

   bool isSuccess() const
   {
      return errorMessage.isEmpty() && (statusCode >= 200) && (statusCode < 300);
   }
};

/**
 * HTTP session (persistent curl handle providing cookie jar and connection reuse)
 */
struct HttpSessionData
{
   CURL *curl;
   HttpClientSettings settings;

   HttpSessionData()
   {
      curl = nullptr;
   }

   ~HttpSessionData()
   {
      if (curl != nullptr)
         curl_easy_cleanup(curl);
   }
};

/**
 * Context for response body write callback
 */
struct HttpWriteContext
{
   ByteStream *data;
   size_t limit;
   bool overflow;
};

/**
 * Response body write callback
 */
static size_t WriteCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
   size_t bytes = size * nmemb;
   auto context = static_cast<HttpWriteContext*>(userdata);
   if (context->data->size() + bytes > context->limit)
   {
      context->overflow = true;
      return 0;   // Abort transfer
   }
   context->data->write(ptr, bytes);
   return bytes;
}

/**
 * Response header callback. Header names are stored in lower case; repeated headers are joined with comma.
 */
static size_t HeaderCallback(char *buffer, size_t size, size_t nitems, void *userdata)
{
   size_t bytes = size * nitems;
   auto headers = static_cast<StringMap*>(userdata);

   if ((bytes >= 5) && !memcmp(buffer, "HTTP/", 5))
   {
      // Status line starts new response (redirect or "100 Continue"), discard headers from previous one
      headers->clear();
      return bytes;
   }

   const char *separator = static_cast<const char*>(memchr(buffer, ':', bytes));
   if (separator == nullptr)
      return bytes;   // Empty line terminating header block

   size_t nameLen = separator - buffer;
   if (nameLen >= 256)
      return bytes;

   wchar_t name[256];
   for(size_t i = 0; i < nameLen; i++)
      name[i] = static_cast<wchar_t>(tolower(static_cast<unsigned char>(buffer[i])));
   name[nameLen] = 0;

   const char *value = separator + 1;
   size_t valueLen = bytes - nameLen - 1;
   while((valueLen > 0) && isspace(static_cast<unsigned char>(*value)))
   {
      value++;
      valueLen--;
   }
   while((valueLen > 0) && isspace(static_cast<unsigned char>(value[valueLen - 1])))
      valueLen--;

   StringBuffer text;
   const wchar_t *existing = headers->get(name);
   if (existing != nullptr)
   {
      text.append(existing);
      text.append(L", ");
   }
   text.appendUtf8String(value, valueLen);
   headers->set(name, text);
   return bytes;
}

/**
 * Select effective integer setting: request value, then session value, then default
 */
static inline int EffectiveSetting(int requestValue, int sessionValue, int defaultValue)
{
   return (requestValue >= 0) ? requestValue : ((sessionValue >= 0) ? sessionValue : defaultValue);
}

/**
 * Select effective string setting: request value, then session value (may be empty)
 */
static inline const SharedString& EffectiveSetting(const SharedString& requestValue, const SharedString& sessionValue)
{
   return requestValue.isEmpty() ? sessionValue : requestValue;
}

/**
 * Convert auth type to curl auth mask
 */
static long CurlAuthMask(HttpAuthType authType)
{
   switch(authType)
   {
      case HttpAuthType::BASIC:
         return CURLAUTH_BASIC;
      case HttpAuthType::DIGEST:
         return CURLAUTH_DIGEST;
      case HttpAuthType::NTLM:
         return CURLAUTH_NTLM;
      default:
         return CURLAUTH_NONE;
   }
}

/**
 * Append header to curl header list
 */
static struct curl_slist *AppendHeader(struct curl_slist *list, const wchar_t *name, const wchar_t *value)
{
   StringBuffer line(name);
   line.append(L": ");
   line.append(value);
   char *utf8 = UTF8StringFromWideString(line);
   list = curl_slist_append(list, utf8);
   MemFree(utf8);
   return list;
}

/**
 * Execute HTTP request using given curl handle. Session settings may be null for standalone requests.
 */
static void ExecuteHttpRequest(CURL *curl, const HttpRequestData& request, const HttpClientSettings *sessionSettings, HttpResponseData *response)
{
   static const HttpClientSettings defaultSettings;
   if (sessionSettings == nullptr)
      sessionSettings = &defaultSettings;

   int timeout = EffectiveSetting(request.settings.timeout, sessionSettings->timeout, s_defaultTimeout);
   if (timeout < 1)
      timeout = 1;
   else if (timeout > s_maxTimeout)
      timeout = s_maxTimeout;

   int64_t maxResponseSize = (request.settings.maxResponseSize >= 0) ? request.settings.maxResponseSize :
         ((sessionSettings->maxResponseSize >= 0) ? sessionSettings->maxResponseSize : HTTP_MAX_RESPONSE_SIZE);
   if ((maxResponseSize < 1) || (maxResponseSize > HTTP_MAX_RESPONSE_SIZE))
      maxResponseSize = HTTP_MAX_RESPONSE_SIZE;

   bool verifyPeer = EffectiveSetting(request.settings.verifyPeer, sessionSettings->verifyPeer, 1) != 0;
   bool verifyHost = EffectiveSetting(request.settings.verifyHost, sessionSettings->verifyHost, 1) != 0;
   bool followRedirects = EffectiveSetting(request.settings.followRedirects, sessionSettings->followRedirects, 1) != 0;

   curl_easy_reset(curl);

   char errorBuffer[CURL_ERROR_SIZE];
   errorBuffer[0] = 0;
   curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
#if HAVE_DECL_CURLOPT_NOSIGNAL
   curl_easy_setopt(curl, CURLOPT_NOSIGNAL, static_cast<long>(1));
#endif
#if HAVE_DECL_CURLOPT_PROTOCOLS_STR
   curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "http,https");
   curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR, "http,https");
#else
   curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
   curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
#endif
   curl_easy_setopt(curl, CURLOPT_HEADER, static_cast<long>(0));
   curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(timeout));
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, verifyPeer ? static_cast<long>(1) : static_cast<long>(0));
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, verifyHost ? static_cast<long>(2) : static_cast<long>(0));
   curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, followRedirects ? static_cast<long>(1) : static_cast<long>(0));
   curl_easy_setopt(curl, CURLOPT_MAXREDIRS, static_cast<long>(HTTP_MAX_REDIRECTS));
   curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
   curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "");   // Enable cookie engine (cookies survive curl_easy_reset)
   EnableLibCURLUnexpectedEOFWorkaround(curl);

   const SharedString& userAgent = EffectiveSetting(request.settings.userAgent, sessionSettings->userAgent);
   if (!userAgent.isEmpty())
   {
      char *utf8 = UTF8StringFromWideString(userAgent);
      curl_easy_setopt(curl, CURLOPT_USERAGENT, utf8);
      MemFree(utf8);
   }
   else
   {
      curl_easy_setopt(curl, CURLOPT_USERAGENT, "NetXMS Server/" NETXMS_VERSION_STRING_A);
   }

   const SharedString& proxy = EffectiveSetting(request.settings.proxy, sessionSettings->proxy);
   if (!proxy.isEmpty())
   {
      char *utf8 = UTF8StringFromWideString(proxy);
      curl_easy_setopt(curl, CURLOPT_PROXY, utf8);
      MemFree(utf8);
   }

   // Method and body
   bool allowBody;
   if (!wcscmp(request.method, L"GET"))
   {
      curl_easy_setopt(curl, CURLOPT_HTTPGET, static_cast<long>(1));
      allowBody = false;
   }
   else if (!wcscmp(request.method, L"HEAD"))
   {
      curl_easy_setopt(curl, CURLOPT_NOBODY, static_cast<long>(1));
      allowBody = false;
   }
   else
   {
      char method[16];
      wchar_to_utf8(request.method, -1, method, sizeof(method));
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
      allowBody = true;
   }
   if (allowBody && (request.body != nullptr))
   {
      curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(request.bodySize));
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.body);
   }
   else if (!wcscmp(request.method, L"POST"))
   {
      curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(0));
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
   }

   // Authentication
   HttpAuthType authType = (request.settings.authType != HttpAuthType::UNSET) ? request.settings.authType : sessionSettings->authType;
   const HttpClientSettings& authSource = (request.settings.authType != HttpAuthType::UNSET) ? request.settings : *sessionSettings;
   struct curl_slist *headers = nullptr;
   switch(authType)
   {
      case HttpAuthType::BASIC:
      case HttpAuthType::DIGEST:
      case HttpAuthType::NTLM:
      {
         char *login = UTF8StringFromWideString(authSource.login);
         char *password = UTF8StringFromWideString(authSource.password);
         curl_easy_setopt(curl, CURLOPT_USERNAME, login);
         curl_easy_setopt(curl, CURLOPT_PASSWORD, password);
         curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CurlAuthMask(authType));
         MemFree(login);
         MemFree(password);
         break;
      }
      case HttpAuthType::BEARER:
      {
         char *token = UTF8StringFromWideString(authSource.password);
#if HAVE_DECL_CURLOPT_XOAUTH2_BEARER && defined(CURLAUTH_BEARER)
         // Credentials set this way are not sent to a different host after redirect (unlike custom headers)
         curl_easy_setopt(curl, CURLOPT_XOAUTH2_BEARER, token);
         curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BEARER);
#else
         StringBuffer value(L"Bearer ");
         value.appendUtf8String(token);
         headers = AppendHeader(headers, L"Authorization", value);
#endif
         MemFree(token);
         break;
      }
      default:
         curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_NONE);
         break;
   }

   // Headers: session defaults overridden by request headers
   StringMap effectiveHeaders(sessionSettings->headers);
   effectiveHeaders.addAll(&request.settings.headers);
   effectiveHeaders.forEach(
      [&headers] (const wchar_t *name, const void *value) -> EnumerationCallbackResult
      {
         headers = AppendHeader(headers, name, static_cast<const wchar_t*>(value));
         return _CONTINUE;
      });
   if (headers != nullptr)
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

   // Response receivers
   HttpWriteContext writeContext;
   writeContext.data = &response->body;
   writeContext.limit = static_cast<size_t>(maxResponseSize);
   writeContext.overflow = false;
   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, &writeContext);
   curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
   curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response->headers);

   char *url = UTF8StringFromWideString(request.url);
   CURLcode rc = curl_easy_setopt(curl, CURLOPT_URL, url);
   MemFree(url);

   if (rc == CURLE_OK)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, L"Executing %s %s (timeout=%d, verifyPeer=%s, verifyHost=%s, followRedirects=%s)",
         request.method.cstr(), request.url.cstr(), timeout, BooleanToString(verifyPeer), BooleanToString(verifyHost), BooleanToString(followRedirects));
      rc = curl_easy_perform(curl);

      double totalTime = 0;
      curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &totalTime);
      response->responseTime = static_cast<int32_t>(totalTime * 1000);

      long statusCode = 0;
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);
      response->statusCode = static_cast<int>(statusCode);

      char *effectiveUrl = nullptr;
      curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &effectiveUrl);
      if (effectiveUrl != nullptr)
         response->effectiveUrl = SharedString(effectiveUrl, "UTF-8");
   }

   if (rc != CURLE_OK)
   {
      if (writeContext.overflow)
      {
         response->errorMessage = L"Response size limit exceeded";
      }
      else
      {
         response->errorMessage = SharedString((errorBuffer[0] != 0) ? errorBuffer : curl_easy_strerror(rc), "UTF-8");
      }
      nxlog_debug_tag(DEBUG_TAG, 6, L"%s %s failed: %s", request.method.cstr(), request.url.cstr(), response->errorMessage.cstr());
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 6, L"%s %s: status %d, %u bytes received in %d ms",
         request.method.cstr(), request.url.cstr(), response->statusCode, static_cast<unsigned int>(response->body.size()), response->responseTime);
   }

   curl_slist_free_all(headers);
}

/**
 * Validate and normalize HTTP method name. Returns false if method is not supported.
 */
static bool NormalizeHttpMethod(const wchar_t *method, wchar_t *normalized)
{
   static const wchar_t *supportedMethods[] = { L"GET", L"POST", L"PUT", L"DELETE", L"PATCH", L"HEAD", L"OPTIONS", nullptr };

   size_t len = wcslen(method);
   if ((len == 0) || (len > 7))
      return false;

   for(size_t i = 0; i <= len; i++)
      normalized[i] = towupper(method[i]);

   for(int i = 0; supportedMethods[i] != nullptr; i++)
      if (!wcscmp(normalized, supportedMethods[i]))
         return true;
   return false;
}

/**
 * Create response object
 */
static inline NXSL_Value *CreateResponseObject(NXSL_VM *vm, HttpResponseData *response)
{
   return vm->createValue(vm->createObject(&g_nxslHttpResponseClass, response));
}

/**
 * Validate header name (RFC 7230 token) and value (no control characters), so that data from
 * untrusted sources cannot inject additional headers or requests
 */
static bool IsValidHeader(const wchar_t *name, const wchar_t *value)
{
   if (*name == 0)
      return false;
   for(const wchar_t *p = name; *p != 0; p++)
   {
      wchar_t c = *p;
      if ((c > 0x7E) || !(iswalnum(c) || (wcschr(L"!#$%&'*+-.^_`|~", c) != nullptr)))
         return false;
   }
   for(const wchar_t *p = value; *p != 0; p++)
   {
      if ((*p < 0x20) || (*p == 0x7F))
         return false;
   }
   return true;
}

/**
 * Set request body from NXSL value. Strings are sent as is; JSON objects, arrays, and hash maps
 * are serialized to JSON (with Content-Type set to application/json unless already set); null clears body.
 */
static void SetRequestBody(HttpRequestData *request, NXSL_Value *value)
{
   if (value->isNull())
   {
      request->setBody(nullptr);
   }
   else if (value->isObject(L"JsonObject") || value->isObject(L"JsonArray") || value->isArray() || value->isHashMap())
   {
      json_t *json = value->toJson();
      char *text = json_dumps(json, JSON_COMPACT);
      json_decref(json);
      request->setBody(text);
      MemFree(text);
      if (!request->settings.headers.contains(L"Content-Type"))   // header names are matched case-insensitively
         request->settings.headers.set(L"Content-Type", L"application/json");
   }
   else
   {
      char *utf8 = UTF8StringFromWideString(value->getValueAsCString());
      request->setBody(utf8);
      MemFree(utf8);
   }
}

/**
 * Set HTTP client settings attribute (common for HttpRequest and HttpSession)
 */
bool NXSL_HttpClientClass::setSettingsAttr(HttpClientSettings *settings, const NXSL_Identifier& attr, NXSL_Value *value)
{
   if (NXSL_COMPARE_ATTRIBUTE_NAME("timeout"))
   {
      settings->timeout = value->isNull() ? -1 : value->getValueAsInt32();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("verifyPeer"))
   {
      settings->verifyPeer = value->isNull() ? -1 : (value->isTrue() ? 1 : 0);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("verifyHost"))
   {
      settings->verifyHost = value->isNull() ? -1 : (value->isTrue() ? 1 : 0);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("followRedirects"))
   {
      settings->followRedirects = value->isNull() ? -1 : (value->isTrue() ? 1 : 0);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("maxResponseSize"))
   {
      settings->maxResponseSize = value->isNull() ? -1 : value->getValueAsInt64();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("userAgent"))
   {
      settings->userAgent = value->isNull() ? L"" : value->getValueAsCString();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("proxy"))
   {
      settings->proxy = value->isNull() ? L"" : value->getValueAsCString();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("headers"))
   {
      settings->headers.clear();
      if (value->isHashMap())
      {
         NXSL_HashMap *map = value->getValueAsHashMap();
         StringList keys = map->getKeysAsList();
         for(int i = 0; i < keys.size(); i++)
         {
            NXSL_Value *v = map->get(keys.get(i));
            if ((v == nullptr) || v->isNull())
               continue;
            if (IsValidHeader(keys.get(i), v->getValueAsCString()))
               settings->headers.set(keys.get(i), v->getValueAsCString());
            else
               nxlog_debug_tag(DEBUG_TAG, 5, L"Invalid HTTP header \"%s\" ignored", keys.get(i));
         }
      }
   }
   else
   {
      return false;
   }
   return true;
}

/**
 * Get HTTP client settings attribute (common for HttpRequest and HttpSession)
 */
NXSL_Value *NXSL_HttpClientClass::getSettingsAttr(NXSL_VM *vm, const HttpClientSettings *settings, const NXSL_Identifier& attr)
{
   NXSL_Value *value = nullptr;
   if (NXSL_COMPARE_ATTRIBUTE_NAME("timeout"))
   {
      value = (settings->timeout >= 0) ? vm->createValue(settings->timeout) : vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("verifyPeer"))
   {
      value = (settings->verifyPeer >= 0) ? vm->createValue(settings->verifyPeer != 0) : vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("verifyHost"))
   {
      value = (settings->verifyHost >= 0) ? vm->createValue(settings->verifyHost != 0) : vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("followRedirects"))
   {
      value = (settings->followRedirects >= 0) ? vm->createValue(settings->followRedirects != 0) : vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("maxResponseSize"))
   {
      value = (settings->maxResponseSize >= 0) ? vm->createValue(settings->maxResponseSize) : vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("userAgent"))
   {
      value = !settings->userAgent.isEmpty() ? vm->createValue(settings->userAgent.cstr()) : vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("proxy"))
   {
      value = !settings->proxy.isEmpty() ? vm->createValue(settings->proxy.cstr()) : vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("headers"))
   {
      value = vm->createValue(new NXSL_HashMap(vm, &settings->headers));
   }
   return value;
}

/**
 * Get settings structure from object of class HttpRequest or HttpSession
 */
static inline HttpClientSettings *GetSettings(NXSL_Object *object)
{
   return object->getClass()->instanceOf(L"HttpSession") ?
      &static_cast<HttpSessionData*>(object->getData())->settings :
      &static_cast<HttpRequestData*>(object->getData())->settings;
}

/**
 * setHeader(name, value) method. Null value removes header. Returns false if header name or value is invalid.
 */
NXSL_METHOD_DEFINITION(HttpClient, setHeader)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   HttpClientSettings *settings = GetSettings(object);
   const wchar_t *name = argv[0]->getValueAsCString();
   if (argv[1]->isNull())
   {
      settings->headers.remove(name);
   }
   else if (IsValidHeader(name, argv[1]->getValueAsCString()))
   {
      settings->headers.set(name, argv[1]->getValueAsCString());
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"setHeader: invalid HTTP header \"%s\" rejected", name);
      *result = vm->createValue(false);
      return NXSL_ERR_SUCCESS;
   }
   *result = vm->createValue(true);
   return NXSL_ERR_SUCCESS;
}

/**
 * removeHeader(name) method
 */
NXSL_METHOD_DEFINITION(HttpClient, removeHeader)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   GetSettings(object)->headers.remove(argv[0]->getValueAsCString());
   *result = vm->createValue();
   return NXSL_ERR_SUCCESS;
}

/**
 * setBasicAuth(login, password) method
 */
NXSL_METHOD_DEFINITION(HttpClient, setBasicAuth)
{
   if (!argv[0]->isString() || !argv[1]->isString())
      return NXSL_ERR_NOT_STRING;

   HttpClientSettings *settings = GetSettings(object);
   settings->authType = HttpAuthType::BASIC;
   settings->login = argv[0]->getValueAsCString();
   settings->password = argv[1]->getValueAsCString();
   *result = vm->createValue();
   return NXSL_ERR_SUCCESS;
}

/**
 * setBearerAuth(token) method
 */
NXSL_METHOD_DEFINITION(HttpClient, setBearerAuth)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   HttpClientSettings *settings = GetSettings(object);
   settings->authType = HttpAuthType::BEARER;
   settings->login = L"";
   settings->password = argv[0]->getValueAsCString();
   *result = vm->createValue();
   return NXSL_ERR_SUCCESS;
}

/**
 * setAuth(type, login, password) method
 * Type is one of "none", "basic", "digest", "ntlm", "bearer" (login is ignored for bearer, password holds the token)
 */
NXSL_METHOD_DEFINITION(HttpClient, setAuth)
{
   if ((argc < 1) || (argc > 3))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   for(int i = 0; i < argc; i++)
      if (!argv[i]->isString())
         return NXSL_ERR_NOT_STRING;

   const wchar_t *type = argv[0]->getValueAsCString();
   HttpAuthType authType;
   if (!wcsicmp(type, L"none"))
      authType = HttpAuthType::NONE;
   else if (!wcsicmp(type, L"basic"))
      authType = HttpAuthType::BASIC;
   else if (!wcsicmp(type, L"digest"))
      authType = HttpAuthType::DIGEST;
   else if (!wcsicmp(type, L"ntlm"))
      authType = HttpAuthType::NTLM;
   else if (!wcsicmp(type, L"bearer"))
      authType = HttpAuthType::BEARER;
   else
   {
      *result = vm->createValue(false);
      return NXSL_ERR_SUCCESS;
   }

   HttpClientSettings *settings = GetSettings(object);
   settings->authType = authType;
   settings->login = (argc > 1) ? argv[1]->getValueAsCString() : L"";
   settings->password = (argc > 2) ? argv[2]->getValueAsCString() : L"";
   *result = vm->createValue(true);
   return NXSL_ERR_SUCCESS;
}

/**
 * clearAuth() method - reset authentication to "not set" (session default for requests, no authentication for sessions)
 */
NXSL_METHOD_DEFINITION(HttpClient, clearAuth)
{
   HttpClientSettings *settings = GetSettings(object);
   settings->authType = HttpAuthType::UNSET;
   settings->login = L"";
   settings->password = L"";
   *result = vm->createValue();
   return NXSL_ERR_SUCCESS;
}

/**
 * Base class for HttpRequest and HttpSession (registers common settings methods)
 */
NXSL_HttpClientClass::NXSL_HttpClientClass() : NXSL_Class()
{
   NXSL_REGISTER_METHOD(HttpClient, clearAuth, 0);
   NXSL_REGISTER_METHOD(HttpClient, removeHeader, 1);
   NXSL_REGISTER_METHOD(HttpClient, setAuth, -1);
   NXSL_REGISTER_METHOD(HttpClient, setBasicAuth, 2);
   NXSL_REGISTER_METHOD(HttpClient, setBearerAuth, 1);
   NXSL_REGISTER_METHOD(HttpClient, setHeader, 2);
}

/**
 * HttpRequest::execute() method - execute request without session
 */
NXSL_METHOD_DEFINITION(HttpRequest, execute)
{
   CURL *curl = curl_easy_init();
   if (curl == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"HttpRequest::execute: curl_easy_init failed");
      *result = vm->createValue();
      return NXSL_ERR_SUCCESS;
   }

   auto response = new HttpResponseData();
   ExecuteHttpRequest(curl, *static_cast<HttpRequestData*>(object->getData()), nullptr, response);
   curl_easy_cleanup(curl);
   *result = CreateResponseObject(vm, response);
   return NXSL_ERR_SUCCESS;
}

/**
 * NXSL class HttpRequest: constructor
 */
NXSL_HttpRequestClass::NXSL_HttpRequestClass() : NXSL_HttpClientClass()
{
   setName(L"HttpRequest");
   NXSL_REGISTER_METHOD(HttpRequest, execute, 0);
}

/**
 * NXSL class HttpRequest: get attribute
 */
NXSL_Value *NXSL_HttpRequestClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto request = static_cast<HttpRequestData*>(object->getData());
   if (request == nullptr)
      return nullptr;   // Attribute scan

   value = getSettingsAttr(vm, &request->settings, attr);
   if (value != nullptr)
      return value;

   if (NXSL_COMPARE_ATTRIBUTE_NAME("body"))
   {
      value = (request->body != nullptr) ? vm->createValue(request->body) : vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("method"))
   {
      value = vm->createValue(request->method.cstr());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("url"))
   {
      value = vm->createValue(request->url.cstr());
   }
   return value;
}

/**
 * NXSL class HttpRequest: set attribute
 */
bool NXSL_HttpRequestClass::setAttr(NXSL_Object *object, const NXSL_Identifier& attr, NXSL_Value *value)
{
   auto request = static_cast<HttpRequestData*>(object->getData());
   if (setSettingsAttr(&request->settings, attr, value))
      return true;

   if (NXSL_COMPARE_ATTRIBUTE_NAME("body"))
   {
      SetRequestBody(request, value);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("method"))
   {
      wchar_t method[8];
      if (!value->isString() || !NormalizeHttpMethod(value->getValueAsCString(), method))
         return false;
      request->method = method;
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("url"))
   {
      if (!value->isString())
         return false;
      request->url = value->getValueAsCString();
   }
   else
   {
      return false;
   }
   return true;
}

/**
 * NXSL class HttpRequest: object destruction handler
 */
void NXSL_HttpRequestClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<HttpRequestData*>(object->getData());
}

/**
 * HttpResponse::getHeader(name) method - case-insensitive header lookup
 */
NXSL_METHOD_DEFINITION(HttpResponse, getHeader)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   StringBuffer name(argv[0]->getValueAsCString());
   name.toLowercase();
   const wchar_t *value = static_cast<HttpResponseData*>(object->getData())->headers.get(name);
   *result = (value != nullptr) ? vm->createValue(value) : vm->createValue();
   return NXSL_ERR_SUCCESS;
}

/**
 * NXSL class HttpResponse: constructor
 */
NXSL_HttpResponseClass::NXSL_HttpResponseClass() : NXSL_Class()
{
   setName(L"HttpResponse");
   NXSL_REGISTER_METHOD(HttpResponse, getHeader, 1);
}

/**
 * NXSL class HttpResponse: get attribute
 */
NXSL_Value *NXSL_HttpResponseClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto response = static_cast<HttpResponseData*>(object->getData());

   if (NXSL_COMPARE_ATTRIBUTE_NAME("body"))
   {
      if (response->body.size() > 0)
      {
         response->body.write('\0');
         value = vm->createValue(reinterpret_cast<const char*>(response->body.buffer()));
         response->body.truncate(response->body.size() - 1);
      }
      else
      {
         value = vm->createValue(L"");
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("bodySize"))
   {
      value = vm->createValue(static_cast<uint64_t>(response->body.size()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("contentType"))
   {
      const wchar_t *contentType = response->headers.get(L"content-type");
      value = (contentType != nullptr) ? vm->createValue(contentType) : vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("errorMessage"))
   {
      value = response->errorMessage.isEmpty() ? vm->createValue() : vm->createValue(response->errorMessage.cstr());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("headers"))
   {
      value = vm->createValue(new NXSL_HashMap(vm, &response->headers));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("json"))
   {
      if (!response->jsonParsed)
      {
         response->jsonParsed = true;
         if (response->body.size() > 0)
         {
            json_error_t error;
            response->json = json_loadb(reinterpret_cast<const char*>(response->body.buffer()), response->body.size(), 0, &error);
            if (response->json == nullptr)
               nxlog_debug_tag(DEBUG_TAG, 6, L"Cannot parse response from %s as JSON: %hs (line %d, column %d)", response->effectiveUrl.cstr(), error.text, error.line, error.column);
         }
      }
      if (response->json != nullptr)
      {
         json_incref(response->json);
         value = vm->createValue(vm->createObject(json_is_array(response->json) ? static_cast<NXSL_Class*>(&g_nxslJsonArrayClass) : static_cast<NXSL_Class*>(&g_nxslJsonObjectClass), response->json));
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("responseTime"))
   {
      value = vm->createValue(response->responseTime);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("statusCode"))
   {
      value = vm->createValue(response->statusCode);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("success"))
   {
      value = vm->createValue(response->isSuccess());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("url"))
   {
      value = vm->createValue(response->effectiveUrl.cstr());
   }
   return value;
}

/**
 * NXSL class HttpResponse: object destruction handler
 */
void NXSL_HttpResponseClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<HttpResponseData*>(object->getData());
}

/**
 * Execute request within session
 */
static int ExecuteSessionRequest(HttpSessionData *session, const HttpRequestData& request, NXSL_Value **result, NXSL_VM *vm)
{
   if (session->curl == nullptr)
   {
      session->curl = curl_easy_init();
      if (session->curl == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, L"HttpSession: curl_easy_init failed");
         *result = vm->createValue();
         return NXSL_ERR_SUCCESS;
      }
   }

   auto response = new HttpResponseData();
   ExecuteHttpRequest(session->curl, request, &session->settings, response);
   *result = CreateResponseObject(vm, response);
   return NXSL_ERR_SUCCESS;
}

/**
 * Execute simple request within session: method(url) or method(url, body)
 */
static int SimpleSessionRequest(NXSL_Object *object, const wchar_t *method, int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm, bool withBody)
{
   int expectedArgCount = withBody ? 2 : 1;
   if (argc != expectedArgCount)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   HttpRequestData request(method, argv[0]->getValueAsCString());
   if (withBody)
      SetRequestBody(&request, argv[1]);
   return ExecuteSessionRequest(static_cast<HttpSessionData*>(object->getData()), request, result, vm);
}

/**
 * HttpSession::execute(request) method
 */
NXSL_METHOD_DEFINITION(HttpSession, execute)
{
   if (!argv[0]->isObject(L"HttpRequest"))
      return NXSL_ERR_NOT_OBJECT;

   return ExecuteSessionRequest(static_cast<HttpSessionData*>(object->getData()),
      *static_cast<HttpRequestData*>(argv[0]->getValueAsObject()->getData()), result, vm);
}

/**
 * HttpSession::get(url) method
 */
NXSL_METHOD_DEFINITION(HttpSession, get)
{
   return SimpleSessionRequest(object, L"GET", argc, argv, result, vm, false);
}

/**
 * HttpSession::head(url) method
 */
NXSL_METHOD_DEFINITION(HttpSession, head)
{
   return SimpleSessionRequest(object, L"HEAD", argc, argv, result, vm, false);
}

/**
 * HttpSession::delete(url) method
 */
NXSL_METHOD_DEFINITION(HttpSession, delete)
{
   return SimpleSessionRequest(object, L"DELETE", argc, argv, result, vm, false);
}

/**
 * HttpSession::post(url, body) method
 */
NXSL_METHOD_DEFINITION(HttpSession, post)
{
   return SimpleSessionRequest(object, L"POST", argc, argv, result, vm, true);
}

/**
 * HttpSession::put(url, body) method
 */
NXSL_METHOD_DEFINITION(HttpSession, put)
{
   return SimpleSessionRequest(object, L"PUT", argc, argv, result, vm, true);
}

/**
 * HttpSession::patch(url, body) method
 */
NXSL_METHOD_DEFINITION(HttpSession, patch)
{
   return SimpleSessionRequest(object, L"PATCH", argc, argv, result, vm, true);
}

/**
 * HttpSession::clearCookies() method
 */
NXSL_METHOD_DEFINITION(HttpSession, clearCookies)
{
   auto session = static_cast<HttpSessionData*>(object->getData());
   if (session->curl != nullptr)
      curl_easy_setopt(session->curl, CURLOPT_COOKIELIST, "ALL");
   *result = vm->createValue();
   return NXSL_ERR_SUCCESS;
}

/**
 * HttpSession::close() method - release connections and cookies (session can be reused afterwards as a new one)
 */
NXSL_METHOD_DEFINITION(HttpSession, close)
{
   auto session = static_cast<HttpSessionData*>(object->getData());
   if (session->curl != nullptr)
   {
      curl_easy_cleanup(session->curl);
      session->curl = nullptr;
   }
   *result = vm->createValue();
   return NXSL_ERR_SUCCESS;
}

/**
 * NXSL class HttpSession: constructor
 */
NXSL_HttpSessionClass::NXSL_HttpSessionClass() : NXSL_HttpClientClass()
{
   setName(L"HttpSession");
   NXSL_REGISTER_METHOD(HttpSession, clearCookies, 0);
   NXSL_REGISTER_METHOD(HttpSession, close, 0);
   NXSL_REGISTER_METHOD(HttpSession, delete, -1);
   NXSL_REGISTER_METHOD(HttpSession, execute, 1);
   NXSL_REGISTER_METHOD(HttpSession, get, -1);
   NXSL_REGISTER_METHOD(HttpSession, head, -1);
   NXSL_REGISTER_METHOD(HttpSession, patch, -1);
   NXSL_REGISTER_METHOD(HttpSession, post, -1);
   NXSL_REGISTER_METHOD(HttpSession, put, -1);
}

/**
 * NXSL class HttpSession: get attribute
 */
NXSL_Value *NXSL_HttpSessionClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto session = static_cast<HttpSessionData*>(object->getData());
   if (session == nullptr)
      return nullptr;   // Attribute scan

   value = getSettingsAttr(vm, &session->settings, attr);
   if (value != nullptr)
      return value;

   if (NXSL_COMPARE_ATTRIBUTE_NAME("cookies"))
   {
      // Cookie list in Netscape format: domain, include subdomains flag, path, secure flag, expiration, name, value
      auto cookies = new NXSL_HashMap(vm);
      struct curl_slist *list = nullptr;
      if ((session->curl != nullptr) && (curl_easy_getinfo(session->curl, CURLINFO_COOKIELIST, &list) == CURLE_OK))
      {
         for(struct curl_slist *entry = list; entry != nullptr; entry = entry->next)
         {
            const char *fields[7];
            int count = 0;
            const char *p = entry->data;
            fields[count++] = p;
            while((count < 7) && ((p = strchr(p, '\t')) != nullptr))
               fields[count++] = ++p;
            if (count != 7)
               continue;

            char *name = MemAllocStringA(fields[6] - fields[5]);
            memcpy(name, fields[5], fields[6] - fields[5] - 1);
            name[fields[6] - fields[5] - 1] = 0;
            wchar_t *wname = WideStringFromUTF8String(name);
            MemFree(name);
            cookies->set(wname, vm->createValue(fields[6]));
            MemFree(wname);
         }
         curl_slist_free_all(list);
      }
      value = vm->createValue(cookies);
   }
   return value;
}

/**
 * NXSL class HttpSession: set attribute
 */
bool NXSL_HttpSessionClass::setAttr(NXSL_Object *object, const NXSL_Identifier& attr, NXSL_Value *value)
{
   return setSettingsAttr(&static_cast<HttpSessionData*>(object->getData())->settings, attr, value);
}

/**
 * NXSL class HttpSession: object destruction handler
 */
void NXSL_HttpSessionClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<HttpSessionData*>(object->getData());
}

/**
 * HttpRequest constructor: new HttpRequest(url) or new HttpRequest(method, url)
 */
int F_HttpRequest(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if ((argc < 1) || (argc > 2))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   for(int i = 0; i < argc; i++)
      if (!argv[i]->isString())
         return NXSL_ERR_NOT_STRING;

   if (!s_enabled || !vm->validateAccess(NXSL_AC_SYSTEM, SYSTEM_ACCESS_HTTP_REQUESTS))
   {
      if (!s_enabled)
         nxlog_debug_tag(DEBUG_TAG, 5, L"HttpRequest: HTTP requests from scripts are disabled by server configuration");
      *result = vm->createValue();
      return NXSL_ERR_SUCCESS;
   }

   wchar_t method[8] = L"GET";
   if (argc == 2)
   {
      if (!NormalizeHttpMethod(argv[0]->getValueAsCString(), method))
      {
         nxlog_debug_tag(DEBUG_TAG, 5, L"HttpRequest: unsupported HTTP method \"%s\"", argv[0]->getValueAsCString());
         *result = vm->createValue();
         return NXSL_ERR_SUCCESS;
      }
   }

   *result = vm->createValue(vm->createObject(&g_nxslHttpRequestClass, new HttpRequestData(method, argv[argc - 1]->getValueAsCString())));
   return NXSL_ERR_SUCCESS;
}

/**
 * HttpSession constructor: new HttpSession()
 */
int F_HttpSession(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!s_enabled || !vm->validateAccess(NXSL_AC_SYSTEM, SYSTEM_ACCESS_HTTP_REQUESTS))
   {
      if (!s_enabled)
         nxlog_debug_tag(DEBUG_TAG, 5, L"HttpSession: HTTP requests from scripts are disabled by server configuration");
      *result = vm->createValue();
      return NXSL_ERR_SUCCESS;
   }

   *result = vm->createValue(vm->createObject(&g_nxslHttpSessionClass, new HttpSessionData()));
   return NXSL_ERR_SUCCESS;
}

/**
 * Class instances
 */
NXSL_HttpRequestClass g_nxslHttpRequestClass;
NXSL_HttpResponseClass g_nxslHttpResponseClass;
NXSL_HttpSessionClass g_nxslHttpSessionClass;
