/*
** NetXMS ntopng traffic connector module
** Copyright (C) 2026 Victor Kirhenshtein
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
** File: api.cpp
**/

#include "ntopng.h"
#include <netxms-version.h>

/**
 * Maximum accepted response body size - a misbehaving backend streaming an
 * oversized or endless body must not exhaust server memory
 */
#define MAX_RESPONSE_SIZE  (64 * 1024 * 1024)

/**
 * Set status output if provided
 */
static inline void SetStatus(TrafficConnectorStatus *status, TrafficConnectorStatus value)
{
   if (status != nullptr)
      *status = value;
}

/**
 * cURL write callback with response size ceiling (returning less than the
 * chunk size makes libcurl abort the transfer)
 */
static size_t WriteCallback(char *ptr, size_t size, size_t nmemb, void *context)
{
   ByteStream *data = static_cast<ByteStream*>(context);
   size_t bytes = size * nmemb;
   if (data->size() + bytes > MAX_RESPONSE_SIZE)
      return 0;
   data->write(ptr, bytes);
   return bytes;
}

/**
 * Execute HTTP request against ntopng instance. GET when postFields is null,
 * otherwise POST with given JSON body. Returns false (with status set) when no
 * HTTP response was received; otherwise HTTP status code and response body are
 * returned whatever the status code is.
 */
static bool NtopngHttpRequest(json_t *credentials, const char *path, const char *postFields, ByteStream *responseData, long *httpCode, TrafficConnectorStatus *status)
{
   if (IsShutdownInProgress())
   {
      SetStatus(status, TrafficConnectorStatus::CONNECTOR_UNAVAILABLE);
      return false;
   }

   const char *baseUrl = json_object_get_string_utf8(credentials, "url", nullptr);
   const char *token = json_object_get_string_utf8(credentials, "token", nullptr);
   if ((baseUrl == nullptr) || (*baseUrl == 0) || (token == nullptr) || (*token == 0))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"NtopngHttpRequest: missing \"url\" or \"token\" in credentials");
      SetStatus(status, TrafficConnectorStatus::API_ERROR);   // configuration problem, not a rejected token
      return false;
   }

   char url[1024];
   size_t baseLen = strlen(baseUrl);
   while ((baseLen > 0) && (baseUrl[baseLen - 1] == '/'))
      baseLen--;
   snprintf(url, sizeof(url), "%.*s%s", static_cast<int>(baseLen), baseUrl, path);

   CURL *curl = curl_easy_init();
   if (curl == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"NtopngHttpRequest: curl_easy_init() failed");
      SetStatus(status, TrafficConnectorStatus::CONNECTOR_UNAVAILABLE);
      return false;
   }

#if HAVE_DECL_CURLOPT_NOSIGNAL
   curl_easy_setopt(curl, CURLOPT_NOSIGNAL, (long)1);
#endif
   curl_easy_setopt(curl, CURLOPT_HEADER, (long)0);
   uint32_t timeout = json_object_get_uint32(credentials, "timeout", 30);
   curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)((timeout > 0) ? timeout : 1));   // 0 would disable the timeout
   curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");   // let libcurl negotiate compression (host list pages are large)
   curl_easy_setopt(curl, CURLOPT_USERAGENT, "NetXMS ntopng Connector/" NETXMS_VERSION_STRING_A);
   curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, (long)0);   // 302 = auth failure, never follow

   if (!json_object_get_boolean(credentials, "verifyTls", true))
   {
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, (long)0);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, (long)0);
   }

   char authHeader[512];
   snprintf(authHeader, sizeof(authHeader), "Authorization: Token %s", token);
   struct curl_slist *headers = curl_slist_append(nullptr, authHeader);
   if (postFields != nullptr)
   {
      headers = curl_slist_append(headers, "Content-Type: application/json");
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields);
   }
   curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, responseData);

   char errorBuffer[CURL_ERROR_SIZE];
   errorBuffer[0] = 0;
   curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
   curl_easy_setopt(curl, CURLOPT_URL, url);

   CURLcode rc = curl_easy_perform(curl);
   if (rc != CURLE_OK)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"NtopngHttpRequest(%hs): call failed (%hs)", url, errorBuffer);
      curl_slist_free_all(headers);
      curl_easy_cleanup(curl);
      SetStatus(status, TrafficConnectorStatus::API_ERROR);
      return false;
   }

   *httpCode = 0;
   curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, httpCode);
   curl_slist_free_all(headers);
   curl_easy_cleanup(curl);
   return true;
}

/**
 * Execute request against ntopng REST API v2. GET when postFields is null,
 * otherwise POST with given JSON body.
 */
static json_t *NtopngApiRequest(json_t *credentials, const char *path, const char *postFields, TrafficConnectorStatus *status)
{
   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);
   long httpCode;
   if (!NtopngHttpRequest(credentials, path, postFields, &responseData, &httpCode, status))
      return nullptr;

   // ntopng answers a bad token (and unknown endpoints) with a 302 redirect to the login page
   if ((httpCode == 302) || (httpCode == 401) || (httpCode == 403))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"NtopngApiRequest(%hs): authentication failure (HTTP %d)", path, static_cast<int>(httpCode));
      SetStatus(status, TrafficConnectorStatus::AUTH_ERROR);
      return nullptr;
   }

   if ((httpCode == 301) || (httpCode == 307) || (httpCode == 308))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"NtopngApiRequest(%hs): unexpected redirect (HTTP %d) - check \"url\" (scheme/port) in credentials", path, static_cast<int>(httpCode));
      SetStatus(status, TrafficConnectorStatus::API_ERROR);
      return nullptr;
   }

   if ((httpCode < 200) || (httpCode > 299))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"NtopngApiRequest(%hs): HTTP error %d", path, static_cast<int>(httpCode));
      SetStatus(status, TrafficConnectorStatus::API_ERROR);
      return nullptr;
   }

   if (responseData.size() == 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"NtopngApiRequest(%hs): empty response", path);
      SetStatus(status, TrafficConnectorStatus::API_ERROR);
      return nullptr;
   }

   responseData.write('\0');
   const char *data = reinterpret_cast<const char*>(responseData.buffer());
   json_error_t error;
   json_t *response = json_loads(data, 0, &error);
   if (response == nullptr)
   {
      // ntopng can return plain text Lua error messages instead of the JSON envelope
      nxlog_debug_tag(DEBUG_TAG, 5, L"NtopngApiRequest(%hs): failed to parse JSON on line %d (%hs)", path, error.line, error.text);
      SetStatus(status, TrafficConnectorStatus::API_ERROR);
      return nullptr;
   }

   if (json_object_get_int32(response, "rc", -1) != 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"NtopngApiRequest(%hs): API error (rc=%d, %hs)", path,
         json_object_get_int32(response, "rc", -1), json_object_get_string_utf8(response, "rc_str", "UNKNOWN"));
      json_decref(response);
      SetStatus(status, TrafficConnectorStatus::API_ERROR);
      return nullptr;
   }

   SetStatus(status, TrafficConnectorStatus::SUCCESS);
   return response;
}

/**
 * Execute GET request against ntopng REST API v2
 */
json_t *NtopngApiGet(json_t *credentials, const char *path, TrafficConnectorStatus *status)
{
   return NtopngApiRequest(credentials, path, nullptr, status);
}

/**
 * Execute POST request with JSON body against ntopng REST API v2
 */
json_t *NtopngApiPost(json_t *credentials, const char *path, json_t *body, TrafficConnectorStatus *status)
{
   char *postFields = json_dumps(body, JSON_COMPACT);
   json_t *response = NtopngApiRequest(credentials, path, postFields, status);
   free(postFields);
   return response;
}

/**
 * Probe ntopng REST API v2 endpoint. GET when body is null, otherwise POST with
 * given JSON body. Returns true when the endpoint answered with the REST envelope,
 * whatever the HTTP status - an error answer to a request without arguments still
 * shows that the endpoint exists and passed its access check. Envelope return code
 * is stored in rc. Unknown endpoints and access refusals of endpoints that render
 * an HTML error page give no envelope.
 */
bool NtopngApiProbe(json_t *credentials, const char *path, json_t *body, int32_t *rc)
{
   char *postFields = (body != nullptr) ? json_dumps(body, JSON_COMPACT) : nullptr;
   ByteStream responseData(1024);
   long httpCode;
   bool success = NtopngHttpRequest(credentials, path, postFields, &responseData, &httpCode, nullptr);
   free(postFields);
   if (!success)
      return false;

   responseData.write('\0');
   json_t *response = json_loads(reinterpret_cast<const char*>(responseData.buffer()), 0, nullptr);
   json_t *rcValue = json_object_get(response, "rc");
   success = json_is_integer(rcValue);
   if (success)
      *rc = static_cast<int32_t>(json_integer_value(rcValue));
   nxlog_debug_tag(DEBUG_TAG, 6, L"NtopngApiProbe(%hs): HTTP %d, %hs", path, static_cast<int>(httpCode), success ? "REST envelope" : "no REST envelope");
   json_decref(response);
   return success;
}
