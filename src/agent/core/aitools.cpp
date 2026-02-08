/*
** NetXMS multiplatform core agent
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
** File: aitools.cpp
**
**/

#include "nxagentd.h"
#include <jansson/jansson.h>
#include <netxms-version.h>

#define DEBUG_TAG _T("ai.tools")

bool GetPlatformName(TCHAR *value);

/**
 * AI tool registry entry
 */
struct AIToolEntry
{
   const AIToolDefinition *definition;
   const TCHAR *subagentName;
};

/**
 * Tool registry
 */
static ObjectArray<AIToolEntry> s_registry(16, 16, Ownership::True);
static Mutex s_lock(MutexType::FAST);

/**
 * Register AI tools from subagent info (called during subagent load)
 */
void RegisterAIToolsFromSubagent(const NETXMS_SUBAGENT_INFO *info)
{
   if (info->numAITools == 0 || info->aiTools == nullptr)
      return;

   LockGuard lockGuard(s_lock);
   for (size_t i = 0; i < info->numAITools; i++)
   {
      AIToolEntry *entry = new AIToolEntry();
      entry->definition = &info->aiTools[i];
      entry->subagentName = info->name;
      s_registry.add(entry);
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Registered AI tool '%hs' from subagent %s"), info->aiTools[i].name, info->name);
   }
}

/**
 * Find tool by name
 */
static const AIToolEntry *FindTool(const char *name)
{
   for (int i = 0; i < s_registry.size(); i++)
   {
      if (!strcmp(s_registry.get(i)->definition->name, name))
         return s_registry.get(i);
   }
   return nullptr;
}

/**
 * Generate JSON schema for all registered tools
 */
char *GenerateAIToolsSchema()
{
   json_t *root = json_object();
   json_object_set_new(root, "schema_version", json_string("1.0"));
   json_object_set_new(root, "agent_version", json_string(NETXMS_VERSION_STRING_A));

   TCHAR buffer[MAX_RESULT_LENGTH];
   if (GetPlatformName(buffer))
   {
      json_object_set_new(root, "platform", json_string_t(buffer));
   }

   json_t *tools = json_array();

   s_lock.lock();
   for (int i = 0; i < s_registry.size(); i++)
   {
      const AIToolDefinition *def = s_registry.get(i)->definition;
      json_t *tool = json_object();
      json_object_set_new(tool, "name", json_string(def->name));
      json_object_set_new(tool, "category", json_string(def->category));
      json_object_set_new(tool, "description", json_string(def->description));

      // Build parameters schema
      json_t *params = json_object();
      json_object_set_new(params, "type", json_string("object"));

      json_t *properties = json_object();
      json_t *required = json_array();

      for (size_t j = 0; j < def->numParameters; j++)
      {
         const AIToolParameter *p = &def->parameters[j];
         json_t *prop = json_object();
         json_object_set_new(prop, "type", json_string(p->type));
         json_object_set_new(prop, "description", json_string(p->description));

         if (p->defaultValue != nullptr)
         {
            json_error_t error;
            json_t *defaultVal = json_loads(p->defaultValue, 0, &error);
            if (defaultVal != nullptr)
               json_object_set_new(prop, "default", defaultVal);
         }

         if (p->constraints != nullptr)
         {
            json_error_t error;
            json_t *c = json_loads(p->constraints, 0, &error);
            if (c != nullptr)
            {
               json_object_update(prop, c);
               json_decref(c);
            }
         }

         json_object_set_new(properties, p->name, prop);

         if (p->required)
            json_array_append_new(required, json_string(p->name));
      }

      json_object_set_new(params, "properties", properties);
      json_object_set_new(params, "required", required);
      json_object_set_new(tool, "parameters", params);

      json_array_append_new(tools, tool);
   }
   s_lock.unlock();

   json_object_set_new(root, "tools", tools);

   char *result = json_dumps(root, JSON_COMPACT);
   json_decref(root);
   return result;
}

/**
 * Execute an AI tool
 */
uint32_t ExecuteAITool(const char *toolName, const char *jsonParams, char **jsonResult, AbstractCommSession *session)
{
   *jsonResult = nullptr;

   s_lock.lock();
   const AIToolEntry *entry = FindTool(toolName);
   s_lock.unlock();

   if (entry == nullptr)
   {
      *jsonResult = MemCopyStringA("{\"error\":{\"code\":\"TOOL_NOT_FOUND\",\"message\":\"Tool not found\"}}");
      return ERR_UNKNOWN_COMMAND;
   }

   nxlog_debug_tag(DEBUG_TAG, 5, _T("Executing AI tool '%hs' from subagent %s"), toolName, entry->subagentName);

   // Parse input JSON
   json_error_t parseError;
   json_t *params = json_loads(jsonParams, 0, &parseError);
   if (params == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("ExecuteAITool(%hs): JSON parse error at line %d: %hs"),
         toolName, parseError.line, parseError.text);
      *jsonResult = MemCopyStringA("{\"error\":{\"code\":\"INVALID_JSON\",\"message\":\"Failed to parse input JSON\"}}");
      return ERR_MALFORMED_COMMAND;
   }

   // Call handler
   json_t *result = nullptr;
   uint32_t rcc = entry->definition->handler(params, &result, session);
   json_decref(params);

   // Serialize result
   if (result != nullptr)
   {
      *jsonResult = json_dumps(result, JSON_COMPACT);
      json_decref(result);
   }

   return rcc;
}

/**
 * Get number of registered AI tools
 */
int GetAIToolCount()
{
   LockGuard lockGuard(s_lock);
   return s_registry.size();
}
