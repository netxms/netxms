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
** File: events.cpp
**
**/

#include "webapi.h"

/**
 * Check if user has event DB view access
 */
static bool CheckEventViewAccess(Context *context)
{
   return context->checkSystemAccessRights(SYSTEM_ACCESS_VIEW_EVENT_DB) ||
          context->checkSystemAccessRights(SYSTEM_ACCESS_EDIT_EVENT_DB) ||
          context->checkSystemAccessRights(SYSTEM_ACCESS_EPP);
}

/**
 * Handler for GET /v1/event-templates
 */
int H_EventTemplates(Context *context)
{
   if (!CheckEventViewAccess(context))
      return 403;

   json_t *output = GetEventTemplatesAsJson();
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/event-templates/:event-code
 */
int H_EventTemplateDetails(Context *context)
{
   if (!CheckEventViewAccess(context))
      return 403;

   uint32_t eventCode = context->getPlaceholderValueAsUInt32(L"event-code");
   if (eventCode == 0)
   {
      context->setErrorResponse("Invalid event code");
      return 400;
   }

   shared_ptr<EventTemplate> e = FindEventTemplateByCode(eventCode);
   if (e == nullptr)
      return 404;

   json_t *output = e->toJson();
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for POST /v1/event-templates
 */
int H_EventTemplateCreate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_EDIT_EVENT_DB))
      return 403;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   json_t *newValue = nullptr;
   uint32_t rcc = CreateEventTemplateFromJson(request, &newValue);
   if (rcc == RCC_SUCCESS)
   {
      uint32_t eventCode = json_object_get_uint32(newValue, "code");
      context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Event template %u created", eventCode);

      context->setResponseData(newValue);
      json_decref(newValue);
      return 201;
   }

   if (newValue != nullptr)
      json_decref(newValue);

   if (rcc == RCC_INVALID_OBJECT_NAME)
   {
      context->setErrorResponse("Invalid event name");
      return 400;
   }
   if (rcc == RCC_NAME_ALEARDY_EXISTS)
   {
      context->setErrorResponse("Event with this name already exists");
      return 409;
   }

   context->setErrorResponse("Database failure");
   return 500;
}

/**
 * Handler for PUT /v1/event-templates/:event-code
 */
int H_EventTemplateUpdate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_EDIT_EVENT_DB))
      return 403;

   uint32_t eventCode = context->getPlaceholderValueAsUInt32(L"event-code");
   if (eventCode == 0)
   {
      context->setErrorResponse("Invalid event code");
      return 400;
   }

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   json_t *oldValue = nullptr, *newValue = nullptr;
   uint32_t rcc = ModifyEventTemplateFromJson(eventCode, request, &oldValue, &newValue);
   if (rcc == RCC_SUCCESS)
   {
      context->writeAuditLogWithValues(AUDIT_SYSCFG, true, 0, oldValue, newValue, L"Event template %u updated", eventCode);
      json_decref(oldValue);

      context->setResponseData(newValue);
      json_decref(newValue);
      return 200;
   }

   if (oldValue != nullptr)
      json_decref(oldValue);
   if (newValue != nullptr)
      json_decref(newValue);

   if (rcc == RCC_INVALID_EVENT_CODE)
      return 404;
   if (rcc == RCC_INVALID_OBJECT_NAME)
   {
      context->setErrorResponse("Invalid event name");
      return 400;
   }
   if (rcc == RCC_NAME_ALEARDY_EXISTS)
   {
      context->setErrorResponse("Event with this name already exists");
      return 409;
   }

   context->setErrorResponse("Database failure");
   return 500;
}

/**
 * Handler for DELETE /v1/event-templates/:event-code
 */
int H_EventTemplateDelete(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_EDIT_EVENT_DB))
      return 403;

   uint32_t eventCode = context->getPlaceholderValueAsUInt32(L"event-code");
   if (eventCode == 0)
   {
      context->setErrorResponse("Invalid event code");
      return 400;
   }

   if (eventCode < FIRST_USER_EVENT_ID)
   {
      context->setErrorResponse("Cannot delete system event template");
      return 400;
   }

   uint32_t rcc = DeleteEventTemplate(eventCode);
   if (rcc == RCC_SUCCESS)
   {
      context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Event template %u deleted", eventCode);
      return 204;
   }

   if (rcc == RCC_INVALID_EVENT_CODE)
      return 404;

   context->setErrorResponse("Database failure");
   return 500;
}
