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
** File: epp.cpp
**
**/

#include "webapi.h"
/**
 * Read chain ID from ":chain-id" placeholder. Returns false if it is not a number.
 */
static bool ReadChainId(Context *context, uint32_t *chainId)
{
   const wchar_t *value = context->getPlaceholderValue(L"chain-id");
   if ((value == nullptr) || (*value == 0))
      return false;
   wchar_t *eptr;
   *chainId = wcstoul(value, &eptr, 10);
   return *eptr == 0;
}

/**
 * Read chain access list from JSON array of { "userId": n, "accessRights": n }.
 * Returns false if the array is malformed.
 */
static bool ParseChainAccessList(json_t *array, StructArray<ACL_ELEMENT> *acl)
{
   if (!json_is_array(array))
      return false;

   size_t i;
   json_t *entry;
   json_array_foreach(array, i, entry)
   {
      if (!json_is_object(entry) || !json_is_integer(json_object_get(entry, "userId")))
         return false;
      ACL_ELEMENT e;
      e.userId = json_object_get_uint32(entry, "userId");
      e.accessRights = json_object_get_uint32(entry, "accessRights");
      acl->add(e);
   }
   return true;
}

/**
 * Handler for GET /v1/event-processing-policy/chains
 */
int H_EventProcessingPolicyChains(Context *context)
{
   // A user without the global EPP right gets the chains granted by chain access lists
   if (!GetEventProcessingPolicy()->hasAnyChainAccess(context->getUserId()))
      return 403;

   json_t *output = GetEventProcessingPolicy()->getChainsAsJson(context->getUserId());
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for POST /v1/event-processing-policy/chains
 */
int H_EventProcessingPolicyChainCreate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_EPP))
      return 403;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   String name = json_object_get_string(request, "name", L"");
   if (name.isEmpty())
   {
      context->setErrorResponse("Chain name is required");
      return 400;
   }
   String description = json_object_get_string(request, "description", L"");

   StructArray<ACL_ELEMENT> acl;
   json_t *aclJson = json_object_get(request, "accessList");
   if ((aclJson != nullptr) && !ParseChainAccessList(aclJson, &acl))
   {
      context->setErrorResponse("Invalid access list");
      return 400;
   }

   uint32_t chainId;
   uuid chainGuid;
   uint32_t rcc = GetEventProcessingPolicy()->createChain(name, description, acl, &chainId, &chainGuid);
   if (rcc != RCC_SUCCESS)
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Event processing policy chain \"%s\" [%u] created", name.cstr(), chainId);
   json_t *output = GetEventProcessingPolicy()->getChainAsJson(chainId, context->getUserId(), &rcc);
   context->setResponseData(output);
   json_decref(output);
   return 201;
}

/**
 * Handler for GET /v1/event-processing-policy/chains/:chain-id
 */
int H_EventProcessingPolicyChain(Context *context)
{
   uint32_t chainId;
   if (!ReadChainId(context, &chainId))
      return 400;

   uint32_t rcc;
   json_t *output = GetEventProcessingPolicy()->getChainAsJson(chainId, context->getUserId(), &rcc);
   if (output == nullptr)
      return (rcc == RCC_ACCESS_DENIED) ? 403 : 404;

   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for PUT /v1/event-processing-policy/chains/:chain-id (name, description, access list)
 */
int H_EventProcessingPolicyChainUpdate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_EPP))
      return 403;

   uint32_t chainId;
   if (!ReadChainId(context, &chainId))
      return 400;
   if (chainId == 0)
   {
      context->setErrorResponse("Main chain has fixed name and no access list");
      return 405;
   }

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   uint32_t rcc;
   json_t *current = GetEventProcessingPolicy()->getChainAsJson(chainId, context->getUserId(), &rcc);
   if (current == nullptr)
      return 404;

   // Fields absent from the request keep their current values
   String currentName = json_object_get_string(current, "name", L"");
   String currentDescription = json_object_get_string(current, "description", L"");
   json_decref(current);
   String name = json_object_get_string(request, "name", currentName);
   String description = json_object_get_string(request, "description", currentDescription);
   if (name.isEmpty())
   {
      context->setErrorResponse("Chain name cannot be empty");
      return 400;
   }

   StructArray<ACL_ELEMENT> acl;
   json_t *aclJson = json_object_get(request, "accessList");
   if ((aclJson != nullptr) && !ParseChainAccessList(aclJson, &acl))
   {
      context->setErrorResponse("Invalid access list");
      return 400;
   }

   rcc = GetEventProcessingPolicy()->modifyChain(chainId, name, description, (aclJson != nullptr) ? &acl : nullptr);
   if (rcc == RCC_INVALID_ARGUMENT)
      return 404;
   if (rcc != RCC_SUCCESS)
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Event processing policy chain \"%s\" [%u] modified", name.cstr(), chainId);
   json_t *output = GetEventProcessingPolicy()->getChainAsJson(chainId, context->getUserId(), &rcc);
   if (output == nullptr)
      return 404;
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for DELETE /v1/event-processing-policy/chains/:chain-id
 */
int H_EventProcessingPolicyChainDelete(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_EPP))
      return 403;

   uint32_t chainId;
   if (!ReadChainId(context, &chainId))
      return 400;
   if (chainId == 0)
   {
      context->setErrorResponse("Main chain cannot be deleted");
      return 405;
   }

   StructArray<EPPChainVersion> updatedChains;
   uint32_t rcc = GetEventProcessingPolicy()->deleteChain(chainId, &updatedChains);
   if (rcc == RCC_INVALID_ARGUMENT)
      return 404;
   if (rcc != RCC_SUCCESS)
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Event processing policy chain [%u] deleted", chainId);

   // Chains whose rules lost a call to the deleted chain have new versions
   json_t *output = json_object();
   json_t *versions = json_array();
   for(int i = 0; i < updatedChains.size(); i++)
   {
      json_t *v = json_object();
      json_object_set_new(v, "chainId", json_integer(updatedChains.get(i)->chainId));
      json_object_set_new(v, "version", json_integer(updatedChains.get(i)->version));
      json_array_append_new(versions, v);
   }
   json_object_set_new(output, "updatedChains", versions);
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for PUT /v1/event-processing-policy/chains/:chain-id/rules
 */
int H_EventProcessingPolicyChainRulesUpdate(Context *context)
{
   // Access is checked per chain: global EPP right, or edit right from the chain access list
   uint32_t chainId;
   if (!ReadChainId(context, &chainId))
      return 400;
   if (!GetEventProcessingPolicy()->chainExists(chainId))
      return 404;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   json_t *response = nullptr;
   uint32_t rcc = UpdateEventProcessingPolicyFromJson(request, chainId, context->getUserId(), &response);
   switch(rcc)
   {
      case RCC_SUCCESS:
         context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Event processing policy chain [%u] rules updated (%u rules, version %u)",
            chainId, json_object_get_uint32(response, "ruleCount", 0), json_object_get_uint32(response, "version", 0));
         context->setResponseData(response);
         json_decref(response);
         return 200;
      case RCC_EPP_CONFLICT:
         context->writeAuditLog(AUDIT_SYSCFG, false, 0, L"Event processing policy chain [%u] update rejected due to version conflict", chainId);
         json_object_set_new(response, "error", json_string("Event processing policy chain was modified by another client; re-read it and retry"));
         context->setResponseData(response);
         json_decref(response);
         return 409;
      case RCC_INVALID_ARGUMENT:
         context->setErrorResponse("Invalid event processing policy data");
         return 400;
      case RCC_ACCESS_DENIED:
         context->writeAuditLog(AUDIT_SYSCFG, false, 0, L"Access denied on event processing policy chain [%u] update", chainId);
         return 403;
      default:
         context->setErrorResponse("Database failure");
         return 500;
   }
}

/**
 * Handler for GET /v1/event-processing-policy/chains/:chain-id/callers
 */
int H_EventProcessingPolicyChainCallers(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_EPP))
      return 403;

   uint32_t chainId;
   if (!ReadChainId(context, &chainId))
      return 400;

   json_t *output = GetEventProcessingPolicy()->getChainCallersAsJson(chainId);
   if (output == nullptr)
      return 404;

   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/event-processing-policy/rules/:rule-guid
 */
int H_EventProcessingPolicyRule(Context *context)
{
   const wchar_t *guidText = context->getPlaceholderValue(L"rule-guid");
   if (guidText == nullptr)
      return 400;

   uuid guid = uuid::parse(guidText);
   if (guid.isNull())
   {
      context->setErrorResponse("Invalid rule GUID");
      return 400;
   }

   uint32_t rcc;
   json_t *output = GetEventProcessingPolicy()->getRuleAsJson(guid, context->getUserId(), &rcc);
   if (output == nullptr)
      return (rcc == RCC_ACCESS_DENIED) ? 403 : 404;

   context->setResponseData(output);
   json_decref(output);
   return 200;
}
