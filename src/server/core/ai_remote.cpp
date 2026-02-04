/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
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
** File: ai_remote.cpp
**
**/

#include "nxcore.h"
#include <nxai.h>

/**
 * Get AI tools available on a node's agent
 */
std::string F_GetNodeAITools(json_t *arguments, uint32_t userId)
{
   const char *nodeRef = json_object_get_string_utf8(arguments, "node", nullptr);
   if ((nodeRef == nullptr) || (nodeRef[0] == 0))
      return R"({"error": "node parameter is required"})";

   shared_ptr<NetObj> obj = FindObjectByNameOrId(nodeRef, OBJECT_NODE);
   if (obj == nullptr)
      return R"({"error": "Node not found"})";

   shared_ptr<Node> node = static_pointer_cast<Node>(obj);
   if (!node->checkAccessRights(userId, OBJECT_ACCESS_READ))
      return R"({"error": "Access denied"})";

   const char *schema = node->getAgentAIToolsSchema();
   if (schema == nullptr)
      return R"({"tools": [], "message": "No AI tools available on this agent"})";

   // Parse the schema and wrap it with guidance for the LLM
   json_error_t error;
   json_t *schemaJson = json_loads(schema, 0, &error);
   if (schemaJson == nullptr)
      return R"({"error": "Failed to parse tool schema from agent"})";

   // Create response with explicit instructions
   json_t *response = json_object();
   json_object_set_new(response, "status", json_string("success"));
   json_object_set_new(response, "node", json_string(nodeRef));
   json_object_set_new(response, "instructions", json_string(
      "Tool discovery complete. To use any of these tools, call the execute-agent-tool function with: "
      "node (same node name), tool_name (from the tools list below), and parameters (JSON object matching the tool's parameter schema). "
      "Do NOT call get-node-ai-tools again - proceed directly to execute-agent-tool."));

   // Merge schema fields into response
   const char *key;
   json_t *value;
   json_object_foreach(schemaJson, key, value)
   {
      json_object_set(response, key, value);
   }
   json_decref(schemaJson);

   return JsonToString(response);
}

/**
 * Execute an AI tool on a node's agent
 */
std::string F_ExecuteAgentTool(json_t *arguments, uint32_t userId)
{
   const char *nodeRef = json_object_get_string_utf8(arguments, "node", nullptr);
   const char *toolName = json_object_get_string_utf8(arguments, "tool_name", nullptr);
   json_t *params = json_object_get(arguments, "parameters");

   if ((nodeRef == nullptr) || (nodeRef[0] == 0) || (toolName == nullptr) || (toolName[0] == 0))
      return R"({"error": "node and tool_name are required"})";

   // Find node
   shared_ptr<NetObj> obj = FindObjectByNameOrId(nodeRef, OBJECT_NODE);
   if (obj == nullptr)
      return R"({"error": "Node not found"})";

   shared_ptr<Node> node = static_pointer_cast<Node>(obj);
   if (!node->checkAccessRights(userId, OBJECT_ACCESS_READ))
      return R"({"error": "Access denied"})";

   // Check agent reachability
   if (node->getState() & DCSF_UNREACHABLE)
      return R"({"error": "Agent is currently unreachable"})";

   // Get agent connection
   shared_ptr<AgentConnectionEx> conn = node->createAgentConnection();
   if (conn == nullptr)
      return R"({"error": "Cannot connect to agent"})";

   // Serialize parameters
   char *paramsStr = (params != nullptr) ? json_dumps(params, JSON_COMPACT) : MemCopyStringA("{}");

   // Execute tool
   char *result = nullptr;
   uint32_t execTime = 0;
   uint32_t rcc = conn->executeAITool(toolName, paramsStr, &result, &execTime);
   MemFree(paramsStr);

   if (rcc != ERR_SUCCESS)
   {
      json_t *error = json_object();
      json_object_set_new(error, "error", json_string("Tool execution failed"));
      json_object_set_new(error, "error_code", json_integer(rcc));
      MemFree(result);
      return JsonToString(error);
   }

   std::string response = (result != nullptr) ? result : "{}";
   MemFree(result);
   return response;
}
