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
   ANTHROPIC = 2   // Anthropic Messages API: separate system prompt, content blocks array
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
   int contextSize;
   int timeout;           // seconds

   LLMProviderConfig()
   {
      type = LLMProviderType::OLLAMA;
      url[0] = 0;
      model[0] = 0;
      authToken[0] = 0;
      temperature = -1.0;
      topP = -1.0;
      contextSize = 32768;
      timeout = 180;
   }
};

/**
 * LLM provider abstract base class
 */
class NXCORE_EXPORTABLE LLMProvider
{
protected:
   LLMProviderConfig m_config;

   // Shared HTTP handling
   json_t *doHttpRequest(json_t *requestData);

   // Build base request with model and options
   json_t *buildBaseRequest();

   // Build HTTP headers for the request (can be overridden by subclasses)
   virtual struct curl_slist *buildHttpHeaders();

public:
   LLMProvider(const LLMProviderConfig& config);
   virtual ~LLMProvider();

   // Core API - subclasses implement request/response formatting
   virtual json_t *chat(const char *systemPrompt, json_t *messages, json_t *tools) = 0;

   // Capabilities
   virtual bool supportsToolCalling() const { return true; }

   // Accessors
   const TCHAR *getName() const { return m_config.name.cstr(); }
   const char *getModelName() const { return m_config.model; }
   const char *getUrl() const { return m_config.url; }
   int getContextSize() const { return m_config.contextSize; }
   double getTemperature() const { return m_config.temperature; }
   double getTopP() const { return m_config.topP; }
   LLMProviderType getType() const { return m_config.type; }
};

/**
 * Ollama provider implementation
 */
class NXCORE_EXPORTABLE OllamaProvider : public LLMProvider
{
public:
   OllamaProvider(const LLMProviderConfig& config);
   virtual ~OllamaProvider();

   virtual json_t *chat(const char *systemPrompt, json_t *messages, json_t *tools) override;
};

/**
 * OpenAI provider implementation
 */
class NXCORE_EXPORTABLE OpenAIProvider : public LLMProvider
{
public:
   OpenAIProvider(const LLMProviderConfig& config);
   virtual ~OpenAIProvider();

   virtual json_t *chat(const char *systemPrompt, json_t *messages, json_t *tools) override;
};

/**
 * Anthropic provider implementation
 */
class NXCORE_EXPORTABLE AnthropicProvider : public LLMProvider
{
protected:
   virtual struct curl_slist *buildHttpHeaders() override;

public:
   AnthropicProvider(const LLMProviderConfig& config);
   virtual ~AnthropicProvider();

   virtual json_t *chat(const char *systemPrompt, json_t *messages, json_t *tools) override;
};

/**
 * Create provider from configuration
 */
shared_ptr<LLMProvider> CreateLLMProvider(const LLMProviderConfig& config);

#endif
