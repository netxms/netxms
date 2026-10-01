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
** File: ai_memory.cpp
**
** AI memory store management endpoints
**
**/

#include "webapi.h"
#include <nxai.h>

#define DEBUG_TAG  L"webapi.ai"

/**
 * Map RCC from AI memory functions to HTTP status code
 */
static int MapAIMemoryRCC(Context *context, uint32_t rcc, const std::string& errorText)
{
   switch(rcc)
   {
      case RCC_SUCCESS:
         return 200;
      case RCC_ACCESS_DENIED:
         context->setErrorResponse(errorText.empty() ? "Access denied" : errorText.c_str());
         return 403;
      case RCC_NO_SUCH_RECORD:
         context->setErrorResponse("Memory entry not found");
         return 404;
      case RCC_INVALID_OBJECT_ID:
         context->setErrorResponse("Object not found");
         return 400;
      case RCC_INVALID_USER_ID:
         context->setErrorResponse("User not found");
         return 400;
      case RCC_INVALID_ARGUMENT:
         context->setErrorResponse(errorText.empty() ? "Invalid request" : errorText.c_str());
         return 400;
      case RCC_NAME_ALEARDY_EXISTS:
         context->setErrorResponse(errorText.empty() ? "Entry with this title already exists" : errorText.c_str());
         return 409;
      default:
         context->setErrorResponse("Internal server error");
         return 500;
   }
}

/**
 * Handler for GET /v1/ai/memory - list memory entries readable by the caller.
 * Optional query parameters: scope (environment|user|object) and scopeId (user or object ID; current user if omitted for user scope).
 */
int H_AiMemoryEntries(Context *context)
{
   json_t *output;
   const char *scopeName = context->getQueryParameter("scope");
   if ((scopeName != nullptr) && (scopeName[0] != 0))
   {
      AIMemoryScope scope;
      if (!AIMemoryScopeFromName(scopeName, &scope))
      {
         context->setErrorResponse("Invalid scope");
         return 400;
      }
      uint32_t scopeId = context->getQueryParameterAsUInt32("scopeId");
      if ((scope == AIMemoryScope::USER) && (scopeId == 0))
         scopeId = context->getUserId();
      output = GetAIMemoryEntriesAsJson(context->getUserId(), scope, scopeId);
   }
   else
   {
      output = GetAIMemoryEntriesAsJson(context->getUserId());
   }
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for POST /v1/ai/memory - create memory entry.
 * Request body: { "scope": "environment|user|object", "scopeId": <user or object ID>, "title": "...", "content": "...", "locked": false }
 */
int H_AiMemoryEntryCreate(Context *context)
{
   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      context->setErrorResponse("Request body is required");
      return 400;
   }

   AIMemoryScope scope;
   const char *scopeName = json_object_get_string_utf8(request, "scope", "");
   if (!AIMemoryScopeFromName(scopeName, &scope))
   {
      context->setErrorResponse("Invalid or missing scope");
      return 400;
   }
   uint32_t scopeId = json_object_get_uint32(request, "scopeId", 0);
   if ((scope == AIMemoryScope::USER) && (scopeId == 0))
      scopeId = context->getUserId();

   uint32_t entryId;
   std::string errorText;
   uint32_t rcc = CreateAIMemoryEntry(scope, scopeId, json_object_get_string_utf8(request, "title", ""),
      json_object_get_string_utf8(request, "content", ""), json_object_get_boolean(request, "locked", false), false, context->getUserId(), &entryId, &errorText);
   if (rcc != RCC_SUCCESS)
   {
      if (rcc == RCC_ACCESS_DENIED)
         context->writeAuditLog(AUDIT_AI, false, 0, L"Access denied on creating AI memory entry");
      return MapAIMemoryRCC(context, rcc, errorText);
   }

   context->writeAuditLog(AUDIT_AI, true, 0, L"AI memory entry [%u] created", entryId);

   shared_ptr<AIMemoryEntry> entry = GetAIMemoryEntry(entryId);
   if (entry == nullptr)   // deleted concurrently
   {
      context->setErrorResponse("Memory entry not found");
      return 404;
   }
   json_t *output = entry->toJson();
   context->setResponseData(output);
   json_decref(output);
   return 201;
}

/**
 * Handler for GET /v1/ai/memory/:memory-id - get memory entry
 */
int H_AiMemoryEntryDetails(Context *context)
{
   uint32_t entryId = context->getPlaceholderValueAsUInt32(L"memory-id");
   shared_ptr<AIMemoryEntry> entry = GetAIMemoryEntry(entryId);
   if ((entry == nullptr) || !CanReadAIMemoryEntry(*entry, context->getUserId()))
   {
      context->setErrorResponse("Memory entry not found");
      return 404;
   }
   json_t *output = entry->toJson();
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for PATCH /v1/ai/memory/:memory-id - modify memory entry (title, content, locked)
 */
int H_AiMemoryEntryUpdate(Context *context)
{
   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      context->setErrorResponse("Request body is required");
      return 400;
   }

   uint32_t entryId = context->getPlaceholderValueAsUInt32(L"memory-id");
   std::string errorText;
   uint32_t rcc = ModifyAIMemoryEntry(entryId, request, false, context->getUserId(), &errorText);
   if (rcc != RCC_SUCCESS)
   {
      if (rcc == RCC_ACCESS_DENIED)
         context->writeAuditLog(AUDIT_AI, false, 0, L"Access denied on changing AI memory entry [%u]", entryId);
      return MapAIMemoryRCC(context, rcc, errorText);
   }

   context->writeAuditLog(AUDIT_AI, true, 0, L"AI memory entry [%u] modified", entryId);

   shared_ptr<AIMemoryEntry> entry = GetAIMemoryEntry(entryId);
   if (entry == nullptr)   // deleted concurrently
   {
      context->setErrorResponse("Memory entry not found");
      return 404;
   }
   json_t *output = entry->toJson();
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for DELETE /v1/ai/memory/:memory-id - delete memory entry
 */
int H_AiMemoryEntryDelete(Context *context)
{
   uint32_t entryId = context->getPlaceholderValueAsUInt32(L"memory-id");
   uint32_t rcc = DeleteAIMemoryEntry(entryId, false, context->getUserId());
   if (rcc != RCC_SUCCESS)
   {
      if (rcc == RCC_ACCESS_DENIED)
         context->writeAuditLog(AUDIT_AI, false, 0, L"Access denied on deleting AI memory entry [%u]", entryId);
      return MapAIMemoryRCC(context, rcc, std::string());
   }
   context->writeAuditLog(AUDIT_AI, true, 0, L"AI memory entry [%u] deleted", entryId);
   return 204;
}
