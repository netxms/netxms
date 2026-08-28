/*
** NetXMS - Network Management System
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
** File: endpoint.cpp
**
**/

#include "mcp.h"
#include <netxms-version.h>

/**
 * JSON-RPC 2.0 error codes
 */
#define JSON_RPC_PARSE_ERROR        (-32700)
#define JSON_RPC_INVALID_REQUEST    (-32600)
#define JSON_RPC_METHOD_NOT_FOUND   (-32601)
#define JSON_RPC_INVALID_PARAMS     (-32602)

/**
 * Create JSON-RPC response envelope with given request ID (nullptr if request ID is not known)
 */
static json_t *CreateResponseEnvelope(json_t *id)
{
   json_t *response = json_object();
   json_object_set_new(response, "jsonrpc", json_string("2.0"));
   if (id != nullptr)
      json_object_set(response, "id", id);
   else
      json_object_set_new(response, "id", json_null());
   return response;
}

/**
 * Send JSON-RPC result (consumes result object)
 */
static int SendResult(Context *context, json_t *id, json_t *result)
{
   json_t *response = CreateResponseEnvelope(id);
   json_object_set_new(response, "result", result);
   context->setResponseData(response);
   json_decref(response);
   return 200;
}

/**
 * Send JSON-RPC error. Protocol-level failures that render the HTTP request itself
 * invalid (unparseable body, malformed JSON-RPC) use non-200 HTTP codes.
 */
static int SendError(Context *context, json_t *id, int code, const char *message, int httpCode = 200)
{
   json_t *response = CreateResponseEnvelope(id);
   json_t *error = json_object();
   json_object_set_new(error, "code", json_integer(code));
   json_object_set_new(error, "message", json_string(message));
   json_object_set_new(response, "error", error);
   context->setResponseData(response);
   json_decref(response);
   return httpCode;
}

/**
 * Handler for "initialize" method
 */
static int MethodInitialize(Context *context, json_t *id)
{
   json_t *result = json_object();
   json_object_set_new(result, "protocolVersion", json_string(MCP_PROTOCOL_VERSION));

   json_t *capabilities = json_object();
   json_object_set_new(capabilities, "tools", json_object());
   json_object_set_new(capabilities, "prompts", json_object());
   json_object_set_new(result, "capabilities", capabilities);

   json_t *serverInfo = json_object();
   json_object_set_new(serverInfo, "name", json_string("NetXMS"));
   json_object_set_new(serverInfo, "title", json_string("NetXMS server"));
   json_object_set_new(serverInfo, "version", json_string(NETXMS_VERSION_STRING_A));
   json_object_set_new(result, "serverInfo", serverInfo);

   return SendResult(context, id, result);
}

/**
 * Handler for "tools/list" method
 */
static int MethodToolsList(Context *context, json_t *id)
{
   json_t *result = json_object();
   json_object_set_new(result, "tools", GetMCPToolsAsJson());
   return SendResult(context, id, result);
}

/**
 * Handler for "tools/call" method
 */
static int MethodToolsCall(Context *context, json_t *id, json_t *params)
{
   const char *name = json_object_get_string_utf8(params, "name", "");
   if (*name == 0)
      return SendError(context, id, JSON_RPC_INVALID_PARAMS, "Tool name is required");

   json_t *arguments = json_object_get(params, "arguments");
   if ((arguments != nullptr) && !json_is_object(arguments))
      return SendError(context, id, JSON_RPC_INVALID_PARAMS, "Tool arguments must be an object");

   bool found;
   std::string output = CallMCPTool(name, arguments, context->getUserId(), &found);
   if (!found)
   {
      nxlog_debug_tag(DEBUG_TAG_MCP, 5, L"Call to unknown tool \"%hs\"", name);
      return SendError(context, id, JSON_RPC_INVALID_PARAMS, "Unknown tool");
   }

   bool isError = (output.compare(0, 6, "Error:") == 0);
   context->writeAuditLog(AUDIT_AI, !isError, 0, L"AI assistant function \"%hs\" called via MCP", name);
   nxlog_debug_tag(DEBUG_TAG_MCP, 5, L"Tool \"%hs\" executed (%s)", name, isError ? L"error" : L"success");

   json_t *result = json_object();
   json_t *content = json_array();
   json_t *item = json_object();
   json_object_set_new(item, "type", json_string("text"));
   json_object_set_new(item, "text", json_string(output.c_str()));
   json_array_append_new(content, item);
   json_object_set_new(result, "content", content);
   json_object_set_new(result, "isError", json_boolean(isError));
   return SendResult(context, id, result);
}

/**
 * Handler for "prompts/list" method
 */
static int MethodPromptsList(Context *context, json_t *id)
{
   json_t *result = json_object();
   json_object_set_new(result, "prompts", GetMCPPromptsAsJson());
   return SendResult(context, id, result);
}

/**
 * Handler for "prompts/get" method
 */
static int MethodPromptsGet(Context *context, json_t *id, json_t *params)
{
   const char *name = json_object_get_string_utf8(params, "name", "");
   if (*name == 0)
      return SendError(context, id, JSON_RPC_INVALID_PARAMS, "Prompt name is required");

   json_t *prompt = GetMCPPromptAsJson(name);
   if (prompt == nullptr)
      return SendError(context, id, JSON_RPC_INVALID_PARAMS, "Unknown prompt");

   json_t *result = json_object();
   json_object_set(result, "description", json_object_get(prompt, "description"));

   json_t *content = json_object();
   json_object_set_new(content, "type", json_string("text"));
   json_object_set(content, "text", json_object_get(prompt, "prompt"));
   json_t *message = json_object();
   json_object_set_new(message, "role", json_string("user"));
   json_object_set_new(message, "content", content);
   json_t *messages = json_array();
   json_array_append_new(messages, message);
   json_object_set_new(result, "messages", messages);

   json_decref(prompt);
   return SendResult(context, id, result);
}

/**
 * Handler for MCP requests (POST /mcp). Implements stateless streamable HTTP transport:
 * one JSON-RPC 2.0 message per POST, single application/json response.
 */
int H_MCPRequest(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_USE_AI_ASSISTANT))
      return 403;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return SendError(context, nullptr, JSON_RPC_PARSE_ERROR, "Parse error", 400);

   if (json_is_array(request))
      return SendError(context, nullptr, JSON_RPC_INVALID_REQUEST, "JSON-RPC batch requests are not supported", 400);

   if (!json_is_object(request) || strcmp(json_object_get_string_utf8(request, "jsonrpc", ""), "2.0"))
      return SendError(context, nullptr, JSON_RPC_INVALID_REQUEST, "Invalid request", 400);

   const char *method = json_object_get_string_utf8(request, "method", "");
   if (*method == 0)
   {
      // Message without a method can only be a client-sent JSON-RPC response; accept and ignore it
      if ((json_object_get(request, "result") != nullptr) || (json_object_get(request, "error") != nullptr))
         return 202;
      return SendError(context, nullptr, JSON_RPC_INVALID_REQUEST, "Invalid request", 400);
   }

   nxlog_debug_tag(DEBUG_TAG_MCP, 6, L"Request method \"%hs\" (user ID %u)", method, context->getUserId());

   json_t *id = json_object_get(request, "id");
   if ((id == nullptr) || !strncmp(method, "notifications/", 14))
      return 202;    // Notification - accepted, no response body

   json_t *params = json_object_get(request, "params");

   if (!strcmp(method, "initialize"))
      return MethodInitialize(context, id);
   if (!strcmp(method, "ping"))
      return SendResult(context, id, json_object());
   if (!strcmp(method, "tools/list"))
      return MethodToolsList(context, id);
   if (!strcmp(method, "tools/call"))
      return MethodToolsCall(context, id, params);
   if (!strcmp(method, "prompts/list"))
      return MethodPromptsList(context, id);
   if (!strcmp(method, "prompts/get"))
      return MethodPromptsGet(context, id, params);

   return SendError(context, id, JSON_RPC_METHOD_NOT_FOUND, "Method not found");
}
