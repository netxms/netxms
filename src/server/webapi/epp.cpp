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
 * Handler for GET /v1/event-processing-policy
 */
int H_EventProcessingPolicy(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_EPP))
      return 403;

   json_t *output = GetEventProcessingPolicy()->toJson();
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/event-processing-policy/rules/:rule-guid
 */
int H_EventProcessingPolicyRule(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_EPP))
      return 403;

   const wchar_t *guidText = context->getPlaceholderValue(L"rule-guid");
   if (guidText == nullptr)
      return 400;

   uuid guid = uuid::parse(guidText);
   if (guid.isNull())
   {
      context->setErrorResponse("Invalid rule GUID");
      return 400;
   }

   json_t *output = GetEventProcessingPolicy()->getRuleAsJson(guid);
   if (output == nullptr)
      return 404;

   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for PUT /v1/event-processing-policy
 */
int H_EventProcessingPolicyUpdate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_EPP))
      return 403;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   json_t *response = nullptr;
   uint32_t rcc = UpdateEventProcessingPolicyFromJson(request, context->getUserId(), &response);
   switch(rcc)
   {
      case RCC_SUCCESS:
         context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Event processing policy updated (%u rules, version %u)",
            json_object_get_uint32(response, "ruleCount", 0), json_object_get_uint32(response, "version", 0));
         context->setResponseData(response);
         json_decref(response);
         return 200;
      case RCC_EPP_CONFLICT:
         context->writeAuditLog(AUDIT_SYSCFG, false, 0, L"Event processing policy update rejected due to version conflict");
         json_object_set_new(response, "error", json_string("Event processing policy was modified by another client; re-read it and retry"));
         context->setResponseData(response);
         json_decref(response);
         return 409;
      case RCC_INVALID_ARGUMENT:
         context->setErrorResponse("Invalid event processing policy data");
         return 400;
      default:
         context->setErrorResponse("Database failure");
         return 500;
   }
}
