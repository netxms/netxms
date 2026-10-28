/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
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
** File: ai_provider.h
**
**/

#ifndef _ai_provider_h_
#define _ai_provider_h_

#include <nms_common.h>
#include <nms_util.h>

/**
 * LLM provider type
 */
enum class LLMProviderType
{
   OLLAMA = 0,     // Ollama API: response has "message" at root level
   OPENAI = 1,     // OpenAI API: response has "choices[0].message" - works for all compatible APIs
   ANTHROPIC = 2,  // Anthropic Messages API: separate system prompt, content blocks array
   MISTRAL = 3     // Mistral API: OpenAI-compatible (choices[0].message, Bearer auth)
};

/**
 * LLM provider configuration
 */
struct NXCORE_EXPORTABLE LLMProviderConfig
{
   MutableString name;
   LLMProviderType type;
   char url[256];
   char model[64];
   char authToken[256];
   double temperature;    // -1 = not set
   double topP;           // -1 = not set
   int contextSize;       // context window size in tokens (0 = detect from provider API)
   int maxOutputTokens;   // maximum number of tokens in response for providers that require explicit limit
   int timeout;           // seconds
   bool verifySSL;        // verify peer certificate for HTTPS connections

   LLMProviderConfig()
   {
      type = LLMProviderType::OLLAMA;
      url[0] = 0;
      model[0] = 0;
      authToken[0] = 0;
      temperature = -1.0;
      topP = -1.0;
      contextSize = 0;
      maxOutputTokens = 32768;
      timeout = 180;
      verifySSL = true;
   }
};

/**
 * Source of LLM provider context window size
 */
enum class LLMContextWindowSource
{
   UNKNOWN = 0,         // context window size is not known
   CONFIGURATION = 1,   // set explicitly in provider configuration
   PROVIDER_API = 2,    // reported by provider API for configured model
   DEFAULT = 3          // provider-specific default applied by NetXMS
};

/**
 * LLM token usage accumulator
 */
struct LLMTokenUsage
{
   int64_t inputTokens;
   int64_t outputTokens;

   LLMTokenUsage()
   {
      inputTokens = 0;
      outputTokens = 0;
   }
};

/**
 * Set LLM token usage collector for calling thread. All token usage recorded by providers on this
 * thread will be accumulated into given collector until it is replaced or removed (set to nullptr).
 * Returns previously set collector so that nested scopes can restore it.
 */
LLMTokenUsage NXCORE_EXPORTABLE *SetLLMTokenUsageCollector(LLMTokenUsage *collector);

/**
 * LLM provider abstract base class
 */
class NXCORE_EXPORTABLE LLMProvider
{
protected:
   LLMProviderConfig m_config;

   // Usage counters
   VolatileCounter64 m_totalRequests;
   VolatileCounter64 m_totalInputTokens;
   VolatileCounter64 m_totalOutputTokens;
   VolatileCounter64 m_failedRequests;

   // Model limits reported by provider API
   Mutex m_modelLimitsMutex;
   time_t m_modelLimitsRequestTime;   // time of last retrieval attempt (0 = never attempted)
   bool m_modelLimitsRetrieved;
   int m_modelContextWindow;          // 0 if not reported
   int m_modelMaxOutputTokens;        // 0 if not reported

   // Shared HTTP handling - sends POST request with given JSON document or GET request if document is null
   json_t *doHttpRequest(const char *url, json_t *requestData);

   // Build URL of another API endpoint by replacing given suffix of configured chat endpoint URL
   std::string buildEndpointUrl(const char *chatPathSuffix, const char *path) const;

   // Retrieve model limits from provider API. Returns false if API request failed (will be retried later).
   // Output values should be left as 0 if not reported by provider.
   virtual bool fetchModelLimits(int *contextWindow, int *maxOutputTokens);

   // Context window size used when it is neither configured nor reported by provider API (0 = unknown)
   virtual int getDefaultContextWindow() const { return 0; }

   // Retrieve model limits from provider API if not retrieved yet
   void updateModelLimits();

   // Get context window size from model description object (checks fields used by different OpenAI-compatible servers)
   static int getContextWindowFromModelInfo(json_t *model);

   // Build base request with model and options
   json_t *buildBaseRequest();

   // Build HTTP headers for the request (can be overridden by subclasses)
   virtual struct curl_slist *buildHttpHeaders();

   // Record token usage from a successful API call (also stored into callUsage if it is not null)
   void recordUsage(int64_t inputTokens, int64_t outputTokens, LLMTokenUsage *callUsage);

   // Record a failed API call
   void recordFailure();

public:
   LLMProvider(const LLMProviderConfig& config);
   virtual ~LLMProvider();

   // Core API - subclasses implement request/response formatting. Token usage of this call is stored into usage if it is not null.
   virtual json_t *chat(const char *systemPrompt, json_t *messages, json_t *tools, LLMTokenUsage *usage) = 0;

   // Capabilities
   virtual bool supportsToolCalling() const { return true; }

   // Accessors
   const TCHAR *getName() const { return m_config.name.cstr(); }
   const char *getModelName() const { return m_config.model; }
   const char *getUrl() const { return m_config.url; }
   int getContextWindow(LLMContextWindowSource *source = nullptr) const;
   int getModelMaxOutputTokens() const { return m_modelMaxOutputTokens; }
   bool isModelLimitsRequested() const { return m_modelLimitsRequestTime != 0; }
   double getTemperature() const { return m_config.temperature; }
   double getTopP() const { return m_config.topP; }
   LLMProviderType getType() const { return m_config.type; }
   bool isSSLVerificationEnabled() const { return m_config.verifySSL; }
   int64_t getTotalRequests() const { return m_totalRequests; }
   int64_t getTotalInputTokens() const { return m_totalInputTokens; }
   int64_t getTotalOutputTokens() const { return m_totalOutputTokens; }
   int64_t getFailedRequests() const { return m_failedRequests; }
};

/**
 * Ollama provider implementation
 */
class NXCORE_EXPORTABLE OllamaProvider : public LLMProvider
{
protected:
   virtual bool fetchModelLimits(int *contextWindow, int *maxOutputTokens) override;
   virtual int getDefaultContextWindow() const override { return 32768; }

public:
   OllamaProvider(const LLMProviderConfig& config);
   virtual ~OllamaProvider();

   virtual json_t *chat(const char *systemPrompt, json_t *messages, json_t *tools, LLMTokenUsage *usage) override;
};

/**
 * OpenAI provider implementation
 */
class NXCORE_EXPORTABLE OpenAIProvider : public LLMProvider
{
protected:
   virtual bool fetchModelLimits(int *contextWindow, int *maxOutputTokens) override;

public:
   OpenAIProvider(const LLMProviderConfig& config);
   virtual ~OpenAIProvider();

   virtual json_t *chat(const char *systemPrompt, json_t *messages, json_t *tools, LLMTokenUsage *usage) override;
};

/**
 * Mistral provider implementation (OpenAI-compatible API)
 */
class NXCORE_EXPORTABLE MistralProvider : public LLMProvider
{
protected:
   virtual bool fetchModelLimits(int *contextWindow, int *maxOutputTokens) override;

public:
   MistralProvider(const LLMProviderConfig& config);
   virtual ~MistralProvider();

   virtual json_t *chat(const char *systemPrompt, json_t *messages, json_t *tools, LLMTokenUsage *usage) override;
};

/**
 * Anthropic provider implementation
 */
class NXCORE_EXPORTABLE AnthropicProvider : public LLMProvider
{
protected:
   virtual struct curl_slist *buildHttpHeaders() override;
   virtual bool fetchModelLimits(int *contextWindow, int *maxOutputTokens) override;

public:
   AnthropicProvider(const LLMProviderConfig& config);
   virtual ~AnthropicProvider();

   virtual json_t *chat(const char *systemPrompt, json_t *messages, json_t *tools, LLMTokenUsage *usage) override;
};

/**
 * Create provider from configuration
 */
shared_ptr<LLMProvider> CreateLLMProvider(const LLMProviderConfig& config);

#endif
