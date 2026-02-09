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
** File: ai_provider.cpp
**
**/

#include "nxcore.h"
#include <nxlibcurl.h>
#include <netxms-version.h>
#include <ai_provider.h>

#define DEBUG_TAG _T("ai.provider")

/**
 * LLMProvider constructor
 */
LLMProvider::LLMProvider(const LLMProviderConfig& config) : m_config(config)
{
}

/**
 * LLMProvider destructor
 */
LLMProvider::~LLMProvider()
{
}

/**
 * Build base request with model and common options
 */
json_t *LLMProvider::buildBaseRequest()
{
   json_t *request = json_object();
   json_object_set_new(request, "model", json_string(m_config.model));
   json_object_set_new(request, "stream", json_boolean(false));

   if (m_config.temperature >= 0)
      json_object_set_new(request, "temperature", json_real(m_config.temperature));

   if (m_config.topP >= 0)
      json_object_set_new(request, "top_p", json_real((m_config.topP > 1) ? 1.0 : m_config.topP));

   return request;
}

/**
 * Build HTTP headers for the request. Default implementation adds Content-Type and Bearer auth.
 */
struct curl_slist *LLMProvider::buildHttpHeaders()
{
   struct curl_slist *headers = curl_slist_append(nullptr, "Content-Type: application/json");
   if (m_config.authToken[0] != 0)
   {
      char authHeader[384];
      snprintf(authHeader, sizeof(authHeader), "Authorization: Bearer %s", m_config.authToken);
      headers = curl_slist_append(headers, authHeader);
   }
   return headers;
}

/**
 * Send HTTP request to LLM provider
 */
json_t *LLMProvider::doHttpRequest(json_t *requestData)
{
   if (IsShutdownInProgress())
      return nullptr;

   CURL *curl = curl_easy_init();
   if (curl == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_init() failed"));
      return nullptr;
   }

#if HAVE_DECL_CURLOPT_NOSIGNAL
   curl_easy_setopt(curl, CURLOPT_NOSIGNAL, (long)1);
#endif

   curl_easy_setopt(curl, CURLOPT_HEADER, (long)0);
   curl_easy_setopt(curl, CURLOPT_TIMEOUT, m_config.timeout);
   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ByteStream::curlWriteFunction);
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, (long)0);
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, (long)0);
   curl_easy_setopt(curl, CURLOPT_USERAGENT, "NetXMS Server/" NETXMS_VERSION_STRING_A);

   char *postData = (requestData != nullptr) ? json_dumps(requestData, 0) : MemCopyStringA("{}");
   curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);
   curl_easy_setopt(curl, CURLOPT_POST, (long)1);

   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

   char errorBuffer[CURL_ERROR_SIZE];
   curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);

   struct curl_slist *headers = buildHttpHeaders();
   curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

   bool success = true;

   if (curl_easy_setopt(curl, CURLOPT_URL, m_config.url) != CURLE_OK)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_setopt(CURLOPT_URL) failed"));
      success = false;
   }

   if (success)
   {
      nxlog_debug_tag(DEBUG_TAG, 8, _T("Sending request to %hs: %hs"), m_config.url, postData);
      CURLcode rc = curl_easy_perform(curl);
      if (rc != CURLE_OK)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Call to curl_easy_perform(%hs) failed (%d: %hs)"), m_config.url, rc, errorBuffer);
         success = false;
      }
   }

   if (success)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Got %d bytes from %hs"), static_cast<int>(responseData.size()), m_config.url);
      long httpStatusCode = 0;
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatusCode);
      if ((httpStatusCode < 200) || (httpStatusCode > 299))
      {
         char errorMessage[256] = "Error message not provided";
         if (responseData.size() > 0)
         {
            responseData.write('\0');
            const char *data = reinterpret_cast<const char*>(responseData.buffer());
            json_error_t error;
            json_t *json = json_loads(data, 0, &error);
            if (json != nullptr)
            {
               const char *msg = json_object_get_string_utf8(json, "message", nullptr);
               if ((msg != nullptr) && (*msg != 0))
               {
                  strlcpy(errorMessage, msg, sizeof(errorMessage));
               }
               else
               {
                  json_t *errorObj = json_object_get(json, "error");
                  if (json_is_object(errorObj))
                  {
                     msg = json_object_get_string_utf8(errorObj, "message", nullptr);
                     if ((msg != nullptr) && (*msg != 0))
                     {
                        strlcpy(errorMessage, msg, sizeof(errorMessage));
                     }
                  }
               }
               json_decref(json);
            }
            nxlog_debug_tag(DEBUG_TAG, 7, _T("Error response document: %hs"), data);
         }
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Error response from LLM provider %s: %d (%hs)"),
            m_config.name.cstr(), static_cast<int>(httpStatusCode), errorMessage);
         success = false;
      }
   }

   if (success && responseData.size() <= 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Empty response from LLM provider %s"), m_config.name.cstr());
      success = false;
   }

   json_t *response = nullptr;
   if (success)
   {
      responseData.write('\0');
      const char *data = reinterpret_cast<const char*>(responseData.buffer());
      json_error_t error;
      response = json_loads(data, 0, &error);
      if (response != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 8, _T("Successful response from LLM provider %s: %hs"), m_config.name.cstr(), data);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to parse JSON on line %d: %hs"), error.line, error.text);
         success = false;
      }
   }

   curl_slist_free_all(headers);
   curl_easy_cleanup(curl);
   MemFree(postData);
   return response;
}

/**
 * Create provider from configuration
 */
shared_ptr<LLMProvider> CreateLLMProvider(const LLMProviderConfig& config)
{
   switch(config.type)
   {
      case LLMProviderType::OLLAMA:
         return make_shared<OllamaProvider>(config);
      case LLMProviderType::OPENAI:
         return make_shared<OpenAIProvider>(config);
      case LLMProviderType::ANTHROPIC:
         return make_shared<AnthropicProvider>(config);
      default:
         nxlog_debug_tag(DEBUG_TAG, 3, _T("Unknown provider type %d, defaulting to Ollama"), static_cast<int>(config.type));
         return make_shared<OllamaProvider>(config);
   }
}
