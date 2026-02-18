/*
** NetXMS Oxidized integration module
** Copyright (C) 2026 Raden Solutions
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

#include "oxidized.h"
#include <nxlibcurl.h>
#include <netxms-version.h>

/**
 * Node list cache
 */
static Mutex s_cacheMutex;
static time_t s_cacheTimestamp = 0;
static json_t *s_cachedNodeList = nullptr;

/**
 * Internal curl request - returns raw response in ByteStream
 */
static CURLcode ExecuteCurlRequest(const char *url, ByteStream *responseData, long *httpCode)
{
   CURL *curl = curl_easy_init();
   if (curl == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Call to curl_easy_init() failed");
      return CURLE_FAILED_INIT;
   }

#if HAVE_DECL_CURLOPT_NOSIGNAL
   curl_easy_setopt(curl, CURLOPT_NOSIGNAL, (long)1);
#endif

   curl_easy_setopt(curl, CURLOPT_HEADER, (long)0);
   curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60);
   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ByteStream::curlWriteFunction);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, responseData);
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, (long)0);
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, (long)0);
   curl_easy_setopt(curl, CURLOPT_USERAGENT, "NetXMS Oxidized Integration/" NETXMS_VERSION_STRING_A);

   // Set HTTP Basic Auth if credentials are configured
   if (g_oxidizedLogin[0] != 0)
   {
      curl_easy_setopt(curl, CURLOPT_HTTPAUTH, (long)CURLAUTH_BASIC);
      curl_easy_setopt(curl, CURLOPT_USERNAME, g_oxidizedLogin);
      curl_easy_setopt(curl, CURLOPT_PASSWORD, g_oxidizedPassword);
   }

   char errorBuffer[CURL_ERROR_SIZE];
   curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
   curl_easy_setopt(curl, CURLOPT_URL, url);

   CURLcode rc = curl_easy_perform(curl);
   if (rc != CURLE_OK)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"Call to curl_easy_perform(%hs) failed (%d: %hs)", url, rc, errorBuffer);
   }
   else
   {
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, httpCode);
      nxlog_debug_tag(DEBUG_TAG, 7, L"Got %d bytes from %hs (HTTP %d)", static_cast<int>(responseData->size()), url, static_cast<int>(*httpCode));
   }

   curl_easy_cleanup(curl);
   return rc;
}

/**
 * Do GET request to Oxidized REST API, returns parsed JSON response
 */
json_t *OxidizedApiRequest(const char *endpoint)
{
   if (IsShutdownInProgress())
      return nullptr;

   char url[512];
   snprintf(url, sizeof(url), "%s/%s", g_oxidizedBaseURL, endpoint);

   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);
   long httpCode = 0;

   CURLcode rc = ExecuteCurlRequest(url, &responseData, &httpCode);
   if (rc != CURLE_OK)
      return nullptr;

   if ((httpCode < 200) || (httpCode > 299))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"Error response from Oxidized server: HTTP %d", static_cast<int>(httpCode));
      return nullptr;
   }

   if (responseData.size() <= 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"Empty response from Oxidized server");
      return nullptr;
   }

   responseData.write('\0');
   const char *data = reinterpret_cast<const char*>(responseData.buffer());
   json_error_t error;
   json_t *json = json_loads(data, 0, &error);
   if (json == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Failed to parse JSON on line %d: %hs", error.line, error.text);
      return nullptr;
   }

   nxlog_debug_tag(DEBUG_TAG, 7, L"Successful JSON response from Oxidized server: %hs", data);
   return json;
}

/**
 * Do GET request to Oxidized REST API, returns raw text response
 */
char *OxidizedApiRequestText(const char *endpoint, size_t *responseSize)
{
   if (IsShutdownInProgress())
      return nullptr;

   char url[512];
   snprintf(url, sizeof(url), "%s/%s", g_oxidizedBaseURL, endpoint);

   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);
   long httpCode = 0;

   CURLcode rc = ExecuteCurlRequest(url, &responseData, &httpCode);
   if (rc != CURLE_OK)
      return nullptr;

   if ((httpCode < 200) || (httpCode > 399))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"Error response from Oxidized server: HTTP %d", static_cast<int>(httpCode));
      return nullptr;
   }

   size_t size = responseData.size();

   if (responseSize != nullptr)
      *responseSize = size;

   char *result = MemAllocArrayNoInit<char>(size + 1);
   if (size > 0)
      memcpy(result, responseData.buffer(), size);
   result[size] = 0;
   return result;
}

/**
 * Get cached node list from Oxidized
 */
json_t *GetCachedNodeList()
{
   s_cacheMutex.lock();

   time_t now = time(nullptr);
   if ((s_cachedNodeList != nullptr) && (now - s_cacheTimestamp < 60))
   {
      json_incref(s_cachedNodeList);
      s_cacheMutex.unlock();
      return s_cachedNodeList;
   }

   s_cacheMutex.unlock();

   // Fetch fresh data outside of lock
   json_t *response = OxidizedApiRequest("nodes.json");

   s_cacheMutex.lock();
   if (response != nullptr)
   {
      if (s_cachedNodeList != nullptr)
         json_decref(s_cachedNodeList);
      s_cachedNodeList = response;
      json_incref(s_cachedNodeList);  // Keep reference for cache
      s_cacheTimestamp = now;
      s_cacheMutex.unlock();
      return response;  // Caller owns this reference
   }

   // Return stale cache if available
   if (s_cachedNodeList != nullptr)
   {
      json_incref(s_cachedNodeList);
      s_cacheMutex.unlock();
      return s_cachedNodeList;
   }

   s_cacheMutex.unlock();
   return nullptr;
}

/**
 * Invalidate node list cache
 */
void InvalidateNodeCache()
{
   LockGuard lg(s_cacheMutex);
   s_cacheTimestamp = 0;
}

/**
 * Find node in Oxidized node list by name (NetXMS node ID)
 */
json_t *FindOxidizedNodeByName(json_t *nodeList, const char *nodeName)
{
   if (!json_is_array(nodeList))
      return nullptr;

   size_t index;
   json_t *node;
   json_array_foreach(nodeList, index, node)
   {
      const char *name = json_object_get_string_utf8(node, "name", nullptr);
      if ((name != nullptr) && !strcmp(name, nodeName))
         return node;
   }

   return nullptr;
}
