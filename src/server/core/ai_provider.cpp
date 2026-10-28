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
 * Global AI usage counters
 */
extern VolatileCounter64 g_aiTotalRequests;
extern VolatileCounter64 g_aiTotalInputTokens;
extern VolatileCounter64 g_aiTotalOutputTokens;
extern VolatileCounter64 g_aiFailedRequests;

/**
 * Per-thread token usage collector
 */
static thread_local LLMTokenUsage *s_tokenUsageCollector = nullptr;

/**
 * Set LLM token usage collector for calling thread
 */
LLMTokenUsage NXCORE_EXPORTABLE *SetLLMTokenUsageCollector(LLMTokenUsage *collector)
{
   LLMTokenUsage *previous = s_tokenUsageCollector;
   s_tokenUsageCollector = collector;
   return previous;
}

/**
 * LLMProvider constructor
 */
LLMProvider::LLMProvider(const LLMProviderConfig& config) : m_config(config), m_totalRequests(0), m_totalInputTokens(0), m_totalOutputTokens(0), m_failedRequests(0)
{
   m_modelLimitsRequestTime = 0;
   m_modelLimitsRetrieved = false;
   m_modelContextWindow = 0;
   m_modelMaxOutputTokens = 0;
}

/**
 * LLMProvider destructor
 */
LLMProvider::~LLMProvider()
{
}

/**
 * Record token usage from a successful API call
 */
void LLMProvider::recordUsage(int64_t inputTokens, int64_t outputTokens, LLMTokenUsage *callUsage)
{
   InterlockedIncrement64(&m_totalRequests);
   InterlockedAdd64(&m_totalInputTokens, inputTokens);
   InterlockedAdd64(&m_totalOutputTokens, outputTokens);
   InterlockedIncrement64(&g_aiTotalRequests);
   InterlockedAdd64(&g_aiTotalInputTokens, inputTokens);
   InterlockedAdd64(&g_aiTotalOutputTokens, outputTokens);
   if (s_tokenUsageCollector != nullptr)
   {
      s_tokenUsageCollector->inputTokens += inputTokens;
      s_tokenUsageCollector->outputTokens += outputTokens;
   }
   if (callUsage != nullptr)
   {
      callUsage->inputTokens = inputTokens;
      callUsage->outputTokens = outputTokens;
   }
}

/**
 * Record a failed API call
 */
void LLMProvider::recordFailure()
{
   InterlockedIncrement64(&m_totalRequests);
   InterlockedIncrement64(&m_failedRequests);
   InterlockedIncrement64(&g_aiTotalRequests);
   InterlockedIncrement64(&g_aiFailedRequests);
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
 * Send HTTP request to LLM provider. Sends POST request with given JSON document as body, or GET request if document is null.
 */
json_t *LLMProvider::doHttpRequest(const char *url, json_t *requestData)
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
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, m_config.verifySSL ? (long)2 : (long)0);
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, m_config.verifySSL ? (long)1 : (long)0);
   curl_easy_setopt(curl, CURLOPT_USERAGENT, "NetXMS Server/" NETXMS_VERSION_STRING_A);

   char *postData = (requestData != nullptr) ? json_dumps(requestData, 0) : nullptr;
   if (postData != nullptr)
   {
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);
      curl_easy_setopt(curl, CURLOPT_POST, (long)1);
   }

   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

   char errorBuffer[CURL_ERROR_SIZE];
   curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);

   struct curl_slist *headers = buildHttpHeaders();
   curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

   bool success = true;

   if (curl_easy_setopt(curl, CURLOPT_URL, url) != CURLE_OK)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_setopt(CURLOPT_URL) failed"));
      success = false;
   }

   if (success)
   {
      nxlog_debug_tag(DEBUG_TAG, 8, _T("Sending request to %hs: %hs"), url, (postData != nullptr) ? postData : "(GET)");
      CURLcode rc = curl_easy_perform(curl);
      if (rc != CURLE_OK)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Call to curl_easy_perform(%hs) failed (%d: %hs)"), url, rc, errorBuffer);
         success = false;
      }
   }

   if (success)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Got %d bytes from %hs"), static_cast<int>(responseData.size()), url);
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
 * Build URL of another API endpoint by replacing given suffix of configured chat endpoint URL path with new path.
 * Query string of configured URL is preserved. Returns empty string if URL path does not end with given suffix.
 */
std::string LLMProvider::buildEndpointUrl(const char *chatPathSuffix, const char *path) const
{
   const char *query = strchr(m_config.url, '?');
   size_t pathLen = (query != nullptr) ? static_cast<size_t>(query - m_config.url) : strlen(m_config.url);
   size_t suffixLen = strlen(chatPathSuffix);
   if ((pathLen < suffixLen) || strncmp(&m_config.url[pathLen - suffixLen], chatPathSuffix, suffixLen))
      return std::string();

   std::string url(m_config.url, pathLen - suffixLen);
   url.append(path);
   if (query != nullptr)
      url.append(query);
   return url;
}

/**
 * Get context window size from model description object. Different OpenAI-compatible servers use
 * different field names: max_input_tokens (Anthropic, LiteLLM), max_context_length (Mistral, LM Studio),
 * context_length (OpenRouter), max_model_len (vLLM), meta.n_ctx_train (llama.cpp).
 */
int LLMProvider::getContextWindowFromModelInfo(json_t *model)
{
   static const char *fields[] = { "max_input_tokens", "max_context_length", "context_length", "max_model_len", "context_window", nullptr };
   for(int i = 0; fields[i] != nullptr; i++)
   {
      int32_t value = json_object_get_int32(model, fields[i], 0);
      if (value > 0)
         return value;
   }

   json_t *meta = json_object_get(model, "meta");
   if (json_is_object(meta))
   {
      int32_t value = json_object_get_int32(meta, "n_ctx_train", 0);
      if (value > 0)
         return value;
   }
   return 0;
}

/**
 * Retrieve model limits from provider API (default implementation - provider does not report limits)
 */
bool LLMProvider::fetchModelLimits(int *contextWindow, int *maxOutputTokens)
{
   return true;
}

/**
 * Retrieve model limits from provider API if not retrieved yet. Failed retrieval is retried
 * not more often than every 10 minutes.
 */
void LLMProvider::updateModelLimits()
{
   LockGuard lockGuard(m_modelLimitsMutex);
   time_t now = time(nullptr);
   if (m_modelLimitsRetrieved || (now - m_modelLimitsRequestTime < 600))
      return;

   int contextWindow = 0, maxOutputTokens = 0;
   if (fetchModelLimits(&contextWindow, &maxOutputTokens))
   {
      m_modelContextWindow = contextWindow;
      m_modelMaxOutputTokens = maxOutputTokens;
      m_modelLimitsRetrieved = true;
      nxlog_debug_tag(DEBUG_TAG, 4, L"Model limits for provider \"%s\" (model %hs): context window = %d, max output tokens = %d (0 = not reported)",
         m_config.name.cstr(), m_config.model, contextWindow, maxOutputTokens);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Cannot retrieve model limits for provider \"%s\" (model %hs)", m_config.name.cstr(), m_config.model);
   }
   m_modelLimitsRequestTime = now;
}

/**
 * Get effective context window size (0 if unknown)
 */
int LLMProvider::getContextWindow(LLMContextWindowSource *source) const
{
   LLMContextWindowSource s;
   int size;
   if (m_config.contextSize > 0)
   {
      s = LLMContextWindowSource::CONFIGURATION;
      size = m_config.contextSize;
   }
   else if (m_modelContextWindow > 0)
   {
      s = LLMContextWindowSource::PROVIDER_API;
      size = m_modelContextWindow;
   }
   else
   {
      size = getDefaultContextWindow();
      s = (size > 0) ? LLMContextWindowSource::DEFAULT : LLMContextWindowSource::UNKNOWN;
   }
   if (source != nullptr)
      *source = s;
   return size;
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
      case LLMProviderType::MISTRAL:
         return make_shared<MistralProvider>(config);
      default:
         nxlog_debug_tag(DEBUG_TAG, 3, _T("Unknown provider type %d, defaulting to Ollama"), static_cast<int>(config.type));
         return make_shared<OllamaProvider>(config);
   }
}
