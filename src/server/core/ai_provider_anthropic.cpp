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
** File: ai_provider_anthropic.cpp
**
**/

#include "nxcore.h"
#include <nxlibcurl.h>
#include <ai_provider.h>

#define DEBUG_TAG _T("ai.prov.anthropic")

/**
 * AnthropicProvider constructor
 */
AnthropicProvider::AnthropicProvider(const LLMProviderConfig& config) : LLMProvider(config)
{
   nxlog_debug_tag(DEBUG_TAG, 4, _T("Created Anthropic provider \"%s\" (URL=%hs, model=%hs)"), config.name.cstr(), config.url, config.model);
}

/**
 * AnthropicProvider destructor
 */
AnthropicProvider::~AnthropicProvider()
{
}

/**
 * Build HTTP headers for Anthropic API (x-api-key instead of Bearer token)
 */
struct curl_slist *AnthropicProvider::buildHttpHeaders()
{
   struct curl_slist *headers = curl_slist_append(nullptr, "Content-Type: application/json");
   if (m_config.authToken[0] != 0)
   {
      char authHeader[384];
      snprintf(authHeader, sizeof(authHeader), "x-api-key: %s", m_config.authToken);
      headers = curl_slist_append(headers, authHeader);
   }
   headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");
   return headers;
}

/**
 * Convert OpenAI-format tool declarations to Anthropic format.
 * OpenAI: [{"type": "function", "function": {"name": ..., "description": ..., "parameters": ...}}]
 * Anthropic: [{"name": ..., "description": ..., "input_schema": ...}]
 */
static json_t *ConvertToolsToAnthropic(json_t *tools)
{
   if (tools == nullptr || !json_is_array(tools) || json_array_size(tools) == 0)
      return nullptr;

   json_t *anthropicTools = json_array();
   size_t i;
   json_t *tool;
   json_array_foreach(tools, i, tool)
   {
      json_t *function = json_object_get(tool, "function");
      if (!json_is_object(function))
         continue;

      json_t *anthropicTool = json_object();
      json_t *name = json_object_get(function, "name");
      if (json_is_string(name))
         json_object_set(anthropicTool, "name", name);

      json_t *description = json_object_get(function, "description");
      if (json_is_string(description))
         json_object_set(anthropicTool, "description", description);

      json_t *parameters = json_object_get(function, "parameters");
      if (json_is_object(parameters))
         json_object_set(anthropicTool, "input_schema", parameters);

      json_array_append_new(anthropicTools, anthropicTool);
   }
   return anthropicTools;
}

/**
 * Convert messages from OpenAI format to Anthropic format.
 * Key differences:
 * - role "tool" messages become role "user" with tool_result content blocks
 * - assistant messages with tool_calls become content blocks with tool_use entries
 * - simple user/assistant messages keep content as string
 */
static json_t *ConvertMessagesToAnthropic(json_t *messages)
{
   json_t *anthropicMessages = json_array();
   size_t i;
   json_t *msg;
   json_array_foreach(messages, i, msg)
   {
      const char *role = json_object_get_string_utf8(msg, "role", "");

      if (!strcmp(role, "tool"))
      {
         // Convert tool result: role "tool" -> role "user" with tool_result content block
         const char *toolCallId = json_object_get_string_utf8(msg, "tool_call_id", "");
         const char *content = json_object_get_string_utf8(msg, "content", "");

         json_t *toolResult = json_object();
         json_object_set_new(toolResult, "type", json_string("tool_result"));
         json_object_set_new(toolResult, "tool_use_id", json_string(toolCallId));
         json_object_set_new(toolResult, "content", json_string(content));

         // Check if previous message is also a user message with tool_result - merge them
         size_t lastIdx = json_array_size(anthropicMessages);
         if (lastIdx > 0)
         {
            json_t *lastMsg = json_array_get(anthropicMessages, lastIdx - 1);
            const char *lastRole = json_object_get_string_utf8(lastMsg, "role", "");
            if (!strcmp(lastRole, "user"))
            {
               json_t *lastContent = json_object_get(lastMsg, "content");
               if (json_is_array(lastContent))
               {
                  json_array_append_new(lastContent, toolResult);
                  continue;
               }
            }
         }

         json_t *userMsg = json_object();
         json_object_set_new(userMsg, "role", json_string("user"));
         json_t *contentArray = json_array();
         json_array_append_new(contentArray, toolResult);
         json_object_set_new(userMsg, "content", contentArray);
         json_array_append_new(anthropicMessages, userMsg);
      }
      else if (!strcmp(role, "assistant"))
      {
         json_t *toolCalls = json_object_get(msg, "tool_calls");
         if (json_is_array(toolCalls) && json_array_size(toolCalls) > 0)
         {
            // Assistant message with tool calls -> content blocks
            json_t *contentArray = json_array();

            // Add text content if present
            const char *textContent = json_object_get_string_utf8(msg, "content", nullptr);
            if (textContent != nullptr && textContent[0] != 0)
            {
               json_t *textBlock = json_object();
               json_object_set_new(textBlock, "type", json_string("text"));
               json_object_set_new(textBlock, "text", json_string(textContent));
               json_array_append_new(contentArray, textBlock);
            }

            // Add tool_use blocks
            size_t j;
            json_t *toolCall;
            json_array_foreach(toolCalls, j, toolCall)
            {
               json_t *function = json_object_get(toolCall, "function");
               if (!json_is_object(function))
                  continue;

               json_t *toolUse = json_object();
               json_object_set_new(toolUse, "type", json_string("tool_use"));

               json_t *id = json_object_get(toolCall, "id");
               if (json_is_string(id))
                  json_object_set(toolUse, "id", id);

               json_t *name = json_object_get(function, "name");
               if (json_is_string(name))
                  json_object_set(toolUse, "name", name);

               // arguments can be string or object
               json_t *arguments = json_object_get(function, "arguments");
               if (json_is_string(arguments))
               {
                  json_t *parsed = json_loads(json_string_value(arguments), 0, nullptr);
                  if (parsed != nullptr)
                     json_object_set_new(toolUse, "input", parsed);
                  else
                     json_object_set_new(toolUse, "input", json_object());
               }
               else if (json_is_object(arguments))
               {
                  json_object_set(toolUse, "input", arguments);
               }
               else
               {
                  json_object_set_new(toolUse, "input", json_object());
               }

               json_array_append_new(contentArray, toolUse);
            }

            json_t *assistantMsg = json_object();
            json_object_set_new(assistantMsg, "role", json_string("assistant"));
            json_object_set_new(assistantMsg, "content", contentArray);
            json_array_append_new(anthropicMessages, assistantMsg);
         }
         else
         {
            // Simple assistant message
            json_t *assistantMsg = json_object();
            json_object_set_new(assistantMsg, "role", json_string("assistant"));
            json_t *content = json_object_get(msg, "content");
            if (json_is_string(content))
               json_object_set(assistantMsg, "content", content);
            else
               json_object_set_new(assistantMsg, "content", json_string(""));
            json_array_append_new(anthropicMessages, assistantMsg);
         }
      }
      else if (!strcmp(role, "user"))
      {
         // User message - pass through
         json_t *userMsg = json_object();
         json_object_set_new(userMsg, "role", json_string("user"));
         json_t *content = json_object_get(msg, "content");
         if (json_is_string(content))
            json_object_set(userMsg, "content", content);
         else
            json_object_set_new(userMsg, "content", json_string(""));
         json_array_append_new(anthropicMessages, userMsg);
      }
   }
   return anthropicMessages;
}

/**
 * Convert Anthropic response to OpenAI-normalized format.
 * Anthropic response: {"content": [{"type": "text", "text": "..."}, {"type": "tool_use", ...}], "role": "assistant", ...}
 * OpenAI format: {"role": "assistant", "content": "...", "tool_calls": [...]}
 */
static json_t *ConvertResponseFromAnthropic(json_t *response)
{
   json_t *contentArray = json_object_get(response, "content");
   if (!json_is_array(contentArray))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Property \"content\" is missing or not an array in Anthropic response"));
      return nullptr;
   }

   json_t *result = json_object();
   json_object_set_new(result, "role", json_string("assistant"));

   std::string textContent;
   json_t *toolCalls = nullptr;

   size_t i;
   json_t *block;
   json_array_foreach(contentArray, i, block)
   {
      const char *type = json_object_get_string_utf8(block, "type", "");

      if (!strcmp(type, "text"))
      {
         const char *text = json_object_get_string_utf8(block, "text", "");
         if (text[0] != 0)
         {
            if (!textContent.empty())
               textContent.append("\n");
            textContent.append(text);
         }
      }
      else if (!strcmp(type, "tool_use"))
      {
         if (toolCalls == nullptr)
            toolCalls = json_array();

         json_t *toolCall = json_object();
         json_t *id = json_object_get(block, "id");
         if (json_is_string(id))
            json_object_set(toolCall, "id", id);
         json_object_set_new(toolCall, "type", json_string("function"));

         json_t *function = json_object();
         json_t *name = json_object_get(block, "name");
         if (json_is_string(name))
            json_object_set(function, "name", name);

         json_t *input = json_object_get(block, "input");
         if (json_is_object(input))
            json_object_set(function, "arguments", input);
         else
            json_object_set_new(function, "arguments", json_object());

         json_object_set_new(toolCall, "function", function);
         json_array_append_new(toolCalls, toolCall);
      }
   }

   if (!textContent.empty())
      json_object_set_new(result, "content", json_string(textContent.c_str()));
   else
      json_object_set_new(result, "content", json_null());

   if (toolCalls != nullptr)
      json_object_set_new(result, "tool_calls", toolCalls);

   return result;
}

/**
 * Send chat request to Anthropic Messages API
 */
json_t *AnthropicProvider::chat(const char *systemPrompt, json_t *messages, json_t *tools)
{
   json_t *request = buildBaseRequest();

   // Anthropic requires max_tokens
   json_object_set_new(request, "max_tokens", json_integer(m_config.contextSize > 0 ? m_config.contextSize : 8192));

   // System prompt is a top-level parameter in Anthropic API (not a message)
   if (systemPrompt != nullptr && systemPrompt[0] != 0)
   {
      json_object_set_new(request, "system", json_string(systemPrompt));
   }

   // Convert and add messages
   json_t *anthropicMessages = ConvertMessagesToAnthropic(messages);
   json_object_set_new(request, "messages", anthropicMessages);

   // Convert and add tools
   json_t *anthropicTools = ConvertToolsToAnthropic(tools);
   if (anthropicTools != nullptr && json_array_size(anthropicTools) > 0)
   {
      json_object_set_new(request, "tools", anthropicTools);
   }
   else
   {
      json_decref(anthropicTools);
   }

   json_t *response = doHttpRequest(request);
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
      int64_t inputTokens = json_integer_value(json_object_get(usage, "input_tokens"));
      int64_t outputTokens = json_integer_value(json_object_get(usage, "output_tokens"));
      recordUsage(inputTokens, outputTokens);
   }
   else
   {
      recordUsage(0, 0);
   }

   // Convert Anthropic response to OpenAI-normalized format
   json_t *result = ConvertResponseFromAnthropic(response);
   json_decref(response);
   return result;
}
