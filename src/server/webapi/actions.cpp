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
** File: actions.cpp
**
**/

#include "webapi.h"
#include <nms_actions.h>

/**
 * Check if user has action read access
 */
static bool CheckActionReadAccess(Context *context)
{
   return context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_ACTIONS) ||
          context->checkSystemAccessRights(SYSTEM_ACCESS_EPP);
}

/**
 * Handler for GET /v1/server-actions
 */
int H_ServerActions(Context *context)
{
   if (!CheckActionReadAccess(context))
      return 403;

   json_t *output = GetActions();
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/server-actions/:action-id
 */
int H_ServerActionDetails(Context *context)
{
   if (!CheckActionReadAccess(context))
      return 403;

   uint32_t actionId = context->getPlaceholderValueAsUInt32(L"action-id");
   if (actionId == 0)
      return 400;

   json_t *output = GetActionById(actionId);
   if (output == nullptr)
      return 404;

   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for POST /v1/server-actions
 */
int H_ServerActionCreate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_ACTIONS))
      return 403;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   json_t *jsonName = json_object_get(request, "name");
   if (!json_is_string(jsonName))
   {
      context->setErrorResponse("Missing or invalid name field");
      return 400;
   }

   wchar_t name[MAX_OBJECT_NAME];
   utf8_to_wchar(json_string_value(jsonName), -1, name, MAX_OBJECT_NAME);

   uint32_t actionId = 0;
   uint32_t rcc = CreateAction(name, &actionId);
   if (rcc == RCC_OBJECT_ALREADY_EXISTS)
   {
      context->setErrorResponse("Action with this name already exists");
      return 409;
   }
   if (rcc != RCC_SUCCESS)
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   // If additional fields provided, update the action
   if (json_object_get(request, "type") != nullptr ||
       json_object_get(request, "isDisabled") != nullptr ||
       json_object_get(request, "data") != nullptr ||
       json_object_get(request, "recipientAddress") != nullptr ||
       json_object_get(request, "emailSubject") != nullptr ||
       json_object_get(request, "notificationChannelName") != nullptr)
   {
      json_t *oldValue = nullptr, *newValue = nullptr;
      rcc = ModifyActionFromJson(actionId, request, &oldValue, &newValue);
      if (oldValue != nullptr)
         json_decref(oldValue);
      if (newValue != nullptr)
         json_decref(newValue);
   }

   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Server action %u created", actionId);

   json_t *output = GetActionById(actionId);
   context->setResponseData(output);
   json_decref(output);
   return 201;
}

/**
 * Handler for PUT /v1/server-actions/:action-id
 */
int H_ServerActionUpdate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_ACTIONS))
      return 403;

   uint32_t actionId = context->getPlaceholderValueAsUInt32(L"action-id");
   if (actionId == 0)
      return 400;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   json_t *oldValue = nullptr, *newValue = nullptr;
   uint32_t rcc = ModifyActionFromJson(actionId, request, &oldValue, &newValue);
   if (rcc == RCC_SUCCESS)
   {
      context->writeAuditLogWithValues(AUDIT_SYSCFG, true, 0, oldValue, newValue, L"Server action %u updated", actionId);
      json_decref(oldValue);

      context->setResponseData(newValue);
      json_decref(newValue);
      return 200;
   }

   if (oldValue != nullptr)
      json_decref(oldValue);
   if (newValue != nullptr)
      json_decref(newValue);

   if (rcc == RCC_INVALID_ACTION_ID)
      return 404;
   if (rcc == RCC_OBJECT_ALREADY_EXISTS)
   {
      context->setErrorResponse("Action with this name already exists");
      return 409;
   }

   context->setErrorResponse("Database failure");
   return 500;
}

/**
 * Handler for DELETE /v1/server-actions/:action-id
 */
int H_ServerActionDelete(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_ACTIONS))
      return 403;

   uint32_t actionId = context->getPlaceholderValueAsUInt32(L"action-id");
   if (actionId == 0)
      return 400;

   if (GetEventProcessingPolicy()->isActionInUse(actionId))
   {
      context->setErrorResponse("Action is used in event processing policy");
      return 409;
   }

   uint32_t rcc = DeleteAction(actionId);
   if (rcc == RCC_SUCCESS)
   {
      context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Server action %u deleted", actionId);
      return 204;
   }

   if (rcc == RCC_INVALID_ACTION_ID)
      return 404;

   context->setErrorResponse("Database failure");
   return 500;
}
