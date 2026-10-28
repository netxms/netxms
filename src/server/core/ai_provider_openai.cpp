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
** File: ai_provider_openai.cpp
**
**/

#include "nxcore.h"
#include <ai_provider.h>

#define DEBUG_TAG _T("ai.prov.openai")

/**
 * OpenAIProvider constructor
 */
OpenAIProvider::OpenAIProvider(const LLMProviderConfig& config) : LLMProvider(config)
{
   nxlog_debug_tag(DEBUG_TAG, 4, _T("Created OpenAI provider \"%s\" (URL=%hs, model=%hs)"), config.name.cstr(), config.url, config.model);
}

/**
 * OpenAIProvider destructor
 */
OpenAIProvider::~OpenAIProvider()
{
}

/**
 * Send chat request to OpenAI-compatible API
 * OpenAI API returns: { "choices": [{ "message": { "role": ..., "content": ... } }] }
 */
json_t *OpenAIProvider::chat(const char *systemPrompt, json_t *messages, json_t *tools, LLMTokenUsage *callUsage)
{
   updateModelLimits();

   json_t *request = buildBaseRequest();

   // Build messages array with system prompt prepended
   json_t *fullMessages = json_array();
   if (systemPrompt != nullptr && systemPrompt[0] != 0)
   {
      json_t *sysMsg = json_object();
      json_object_set_new(sysMsg, "role", json_string("system"));
      json_object_set_new(sysMsg, "content", json_string(systemPrompt));
      json_array_append_new(fullMessages, sysMsg);
   }
   size_t i;
   json_t *msg;
   json_array_foreach(messages, i, msg)
   {
      json_array_append(fullMessages, msg);
   }

   json_object_set_new(request, "messages", fullMessages);
   if (tools != nullptr && json_array_size(tools) > 0)
   {
      json_object_set(request, "tools", tools);
   }

   json_t *response = doHttpRequest(m_config.url, request);
   json_decref(request);

   if (response == nullptr)
   {
      recordFailure();
      return nullptr;
   }

   // Extract token usage
   json_t *usage = json_object_get(response, "usage");
   if (json_is_object(usage))
   {
      int64_t inputTokens = json_integer_value(json_object_get(usage, "prompt_tokens"));
      int64_t outputTokens = json_integer_value(json_object_get(usage, "completion_tokens"));
      recordUsage(inputTokens, outputTokens, callUsage);
   }
   else
   {
      recordUsage(0, 0, callUsage);
   }

   // OpenAI format: choices[0].message
   json_t *choices = json_object_get(response, "choices");
   if (json_is_array(choices) && json_array_size(choices) > 0)
   {
      json_t *choice = json_array_get(choices, 0);
      if (json_is_object(choice))
      {
         json_t *message = json_object_get(choice, "message");
         if (json_is_object(message))
         {
            // Keep reference and return
            json_incref(message);
            json_decref(response);
            return message;
         }
      }
   }

   nxlog_debug_tag(DEBUG_TAG, 5, _T("Property \"choices[0].message\" is missing in OpenAI response"));
   json_decref(response);
   return nullptr;
}

/**
 * Retrieve model limits from model list (/models endpoint). OpenAI API itself does not report
 * context window size, but many OpenAI-compatible servers (vLLM, llama.cpp, OpenRouter, LM Studio) do.
 * Model list is used instead of /models/{model} because not all compatible servers implement the latter.
 */
bool OpenAIProvider::fetchModelLimits(int *contextWindow, int *maxOutputTokens)
{
   std::string url = buildEndpointUrl("/chat/completions", "/models");
   if (url.empty())
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"Cannot derive model list URL from chat URL %hs", m_config.url);
      return true;
   }

   json_t *response = doHttpRequest(url.c_str(), nullptr);
   if (response == nullptr)
      return false;

   json_t *models = json_object_get(response, "data");
   size_t i;
   json_t *model;
   json_array_foreach(models, i, model)
   {
      const char *id = json_object_get_string_utf8(model, "id", "");
      if (!strcmp(id, m_config.model))
      {
         *contextWindow = getContextWindowFromModelInfo(model);
         break;
      }
   }

   json_decref(response);
   return true;
}
