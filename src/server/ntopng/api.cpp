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
 * Execute request against ntopng REST API v2. GET when postFields is null,
 * otherwise POST with given JSON body.
 */
static json_t *NtopngApiRequest(json_t *credentials, const char *path, const char *postFields, TrafficConnectorStatus *status)
{
   if (IsShutdownInProgress())
   {
      SetStatus(status, TrafficConnectorStatus::CONNECTOR_UNAVAILABLE);
      return nullptr;
   }

   const char *baseUrl = json_object_get_string_utf8(credentials, "url", nullptr);
   const char *token = json_object_get_string_utf8(credentials, "token", nullptr);
   if ((baseUrl == nullptr) || (*baseUrl == 0) || (token == nullptr) || (*token == 0))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"NtopngApiRequest: missing \"url\" or \"token\" in credentials");
      SetStatus(status, TrafficConnectorStatus::AUTH_ERROR);
      return nullptr;
   }

   char url[1024];
   size_t baseLen = strlen(baseUrl);
   while ((baseLen > 0) && (baseUrl[baseLen - 1] == '/'))
      baseLen--;
   snprintf(url, sizeof(url), "%.*s%s", static_cast<int>(baseLen), baseUrl, path);

   CURL *curl = curl_easy_init();
   if (curl == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"NtopngApiRequest: curl_easy_init() failed");
      SetStatus(status, TrafficConnectorStatus::CONNECTOR_UNAVAILABLE);
      return nullptr;
   }

#if HAVE_DECL_CURLOPT_NOSIGNAL
   curl_easy_setopt(curl, CURLOPT_NOSIGNAL, (long)1);
#endif
   curl_easy_setopt(curl, CURLOPT_HEADER, (long)0);
   curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)json_object_get_uint32(credentials, "timeout", 30));
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

   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);
   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

   char errorBuffer[CURL_ERROR_SIZE];
   errorBuffer[0] = 0;
   curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
   curl_easy_setopt(curl, CURLOPT_URL, url);

   CURLcode rc = curl_easy_perform(curl);
   if (rc != CURLE_OK)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"NtopngApiRequest(%hs): call failed (%hs)", url, errorBuffer);
      curl_slist_free_all(headers);
      curl_easy_cleanup(curl);
      SetStatus(status, TrafficConnectorStatus::API_ERROR);
      return nullptr;
   }

   long httpCode = 0;
   curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
   curl_slist_free_all(headers);
   curl_easy_cleanup(curl);

   if ((httpCode == 302) || (httpCode == 301) || (httpCode == 401) || (httpCode == 403))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"NtopngApiRequest(%hs): authentication failure (HTTP %d)", url, static_cast<int>(httpCode));
      SetStatus(status, TrafficConnectorStatus::AUTH_ERROR);
      return nullptr;
   }

   if ((httpCode < 200) || (httpCode > 299))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"NtopngApiRequest(%hs): HTTP error %d", url, static_cast<int>(httpCode));
      SetStatus(status, TrafficConnectorStatus::API_ERROR);
      return nullptr;
   }

   if (responseData.size() == 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"NtopngApiRequest(%hs): empty response", url);
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
      nxlog_debug_tag(DEBUG_TAG, 5, L"NtopngApiRequest(%hs): failed to parse JSON on line %d (%hs)", url, error.line, error.text);
      SetStatus(status, TrafficConnectorStatus::API_ERROR);
      return nullptr;
   }

   if (json_object_get_int32(response, "rc", -1) != 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"NtopngApiRequest(%hs): API error (rc=%d, %hs)", url,
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
