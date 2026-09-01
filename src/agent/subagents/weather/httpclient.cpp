/*
** NetXMS weather subagent
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
** File: httpclient.cpp
** HTTP transport for provider APIs, including conditional-request handling.
**
**/

#include "weather.h"
#include <nxlibcurl.h>

/**
 * cURL write callback: accumulate response into a ByteStream.
 */
static size_t CurlWriteFunction(char *ptr, size_t size, size_t nmemb, ByteStream *data)
{
   size_t bytes = size * nmemb;
   data->write(ptr, bytes);
   return bytes;
}

/**
 * Copy a response header value into a buffer, stripping the trailing CRLF and
 * surrounding whitespace.
 */
static void ExtractHeaderValue(const char *value, size_t len, char *buffer, size_t bufferSize)
{
   while((len > 0) && ((*value == ' ') || (*value == '\t')))
   {
      value++;
      len--;
   }
   while((len > 0) && ((value[len - 1] == '\r') || (value[len - 1] == '\n') || (value[len - 1] == ' ') || (value[len - 1] == '\t')))
      len--;
   if (len >= bufferSize)
      len = bufferSize - 1;
   memcpy(buffer, value, len);
   buffer[len] = 0;
}

/**
 * cURL header callback: capture the validators used for the next conditional request.
 */
static size_t CurlHeaderFunction(char *ptr, size_t size, size_t nmemb, HttpCacheState *cache)
{
   size_t bytes = size * nmemb;
   const char *separator = static_cast<const char*>(memchr(ptr, ':', bytes));
   if (separator != nullptr)
   {
      size_t nameLength = separator - ptr;
      if ((nameLength == 13) && !strnicmp(ptr, "Last-Modified", 13))
      {
         ExtractHeaderValue(separator + 1, bytes - nameLength - 1, cache->lastModified, sizeof(cache->lastModified));
      }
      else if ((nameLength == 7) && !strnicmp(ptr, "Expires", 7))
      {
         char value[64];
         ExtractHeaderValue(separator + 1, bytes - nameLength - 1, value, sizeof(value));
         time_t expires = curl_getdate(value, nullptr);
         cache->expires = (expires > 0) ? expires : 0;
      }
   }
   return bytes;
}

/**
 * Constructor
 */
HttpClient::HttpClient(uint32_t timeout, const char *userAgent)
{
   m_timeout = timeout;
   strlcpy(m_userAgent, userAgent, sizeof(m_userAgent));
}

/**
 * Execute an API GET request.
 */
HttpRequestResult HttpClient::get(const char *url, HttpCacheState *cache, ByteStream *response) const
{
   CURL *curl = curl_easy_init();
   if (curl == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("curl_easy_init() failed"));
      return HttpRequestResult::FAILURE;
   }

   char errorText[CURL_ERROR_SIZE];
   errorText[0] = 0;
   curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorText);
#if HAVE_DECL_CURLOPT_NOSIGNAL
   curl_easy_setopt(curl, CURLOPT_NOSIGNAL, static_cast<long>(1));
#endif
#if HAVE_DECL_CURLOPT_PROTOCOLS_STR
   curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "https");
#else
   curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTPS);
#endif
   curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, static_cast<long>(m_timeout));
   curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, static_cast<long>(1));
   curl_easy_setopt(curl, CURLOPT_MAXREDIRS, static_cast<long>(4));
   curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
   curl_easy_setopt(curl, CURLOPT_USERAGENT, m_userAgent);
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, static_cast<long>(1));
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, static_cast<long>(2));
   EnableLibCURLUnexpectedEOFWorkaround(curl);

   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteFunction);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

   // Validators are updated in place only after a successful transfer, so keep
   // the previous ones until then - a failed request must not discard them.
   HttpCacheState newCache;
   curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, CurlHeaderFunction);
   curl_easy_setopt(curl, CURLOPT_HEADERDATA, &newCache);

   struct curl_slist *headers = nullptr;
   if (cache->lastModified[0] != 0)
   {
      char header[96];
      snprintf(header, sizeof(header), "If-Modified-Since: %s", cache->lastModified);
      headers = curl_slist_append(headers, header);
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
   }

   HttpRequestResult result = HttpRequestResult::FAILURE;
   if (curl_easy_setopt(curl, CURLOPT_URL, url) == CURLE_OK)
   {
      CURLcode rc = curl_easy_perform(curl);
      if (rc == CURLE_OK)
      {
         long httpCode = 0;
         curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
         if ((httpCode >= 200) && (httpCode <= 299))
         {
            response->write('\0');
            *cache = newCache;
            result = HttpRequestResult::SUCCESS;
         }
         else if (httpCode == 304)
         {
            // Keep the stored Last-Modified: a 304 does not repeat it, but the
            // refreshed Expires (if any) still moves the next check forward.
            cache->expires = newCache.expires;
            result = HttpRequestResult::NOT_MODIFIED;
         }
         else
         {
            // Log host and path only - the query string may carry an API key
            const char *query = strchr(url, '?');
            int urlLength = (query != nullptr) ? static_cast<int>(query - url) : static_cast<int>(strlen(url));
            nxlog_debug_tag(DEBUG_TAG, 5, _T("HTTP response code %03ld for [%.*hs]"), httpCode, urlLength, url);
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("curl_easy_perform() failed (%hs)"), errorText);
      }
   }
   curl_easy_cleanup(curl);
   curl_slist_free_all(headers);
   return result;
}
