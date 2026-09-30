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

/**
 * Handler for POST /v1/events
 * Request body identifies event by "eventCode" or "eventName" and may contain source object
 * ("objectId"), origin timestamp ("originTimestamp"), tags ("tags") and parameters ("parameters",
 * array of objects with optional "name" and mandatory "value"). If source object is not provided,
 * node with client's IP address (or management node for loopback connections) is used.
 */
int H_EventPost(Context *context)
{
   json_t *request = context->getRequestDocument();
   if (!json_is_object(request))
   {
      context->setErrorResponse("Invalid request document");
      return 400;
   }

   // Find event's source object
   shared_ptr<NetObj> object;
   json_t *objectId = json_object_get(request, "objectId");
   if (objectId != nullptr)
   {
      json_int_t id = json_integer_value_ex(objectId, 0);
      if ((id <= 0) || (id > UINT32_MAX))
      {
         context->setErrorResponse("Invalid object identifier");
         return 400;
      }
      object = FindObjectById(static_cast<uint32_t>(id));
      if (object == nullptr)
      {
         context->setErrorResponse("Source object not found");
         return 404;
      }
   }
   else
   {
      InetAddress clientAddress = context->getClientAddress();
      if (clientAddress.isLoopback())
         object = FindObjectById(g_dwMgmtNode);
      else
         object = FindNodeByIP(0, clientAddress);
      if (object == nullptr)
      {
         context->setErrorResponse("Cannot find source object by client address");
         return 400;
      }
   }

   if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_SEND_EVENTS))
      return 403;

   uint32_t eventCode;
   if (json_object_get(request, "eventCode") != nullptr)
   {
      json_int_t code = json_integer_value_ex(json_object_get(request, "eventCode"), 0);
      eventCode = ((code > 0) && (code <= UINT32_MAX)) ? static_cast<uint32_t>(code) : 0;
   }
   else
   {
      wchar_t eventName[MAX_EVENT_NAME] = L"";
      if (!json_object_update_string(request, "eventName", eventName, MAX_EVENT_NAME))
      {
         context->setErrorResponse("Invalid event name");
         return 400;
      }
      eventCode = (eventName[0] != 0) ? EventCodeFromName(eventName, 0) : 0;
   }
   if (eventCode == 0)
   {
      context->setErrorResponse("Invalid event code or name");
      return 400;
   }

   time_t originTimestamp = 0;
   json_t *timestamp = json_object_get(request, "originTimestamp");
   if ((timestamp != nullptr) && !json_is_null(timestamp))
   {
      if (json_is_integer(timestamp))
         originTimestamp = static_cast<time_t>(json_integer_value(timestamp));
      else if (json_is_string(timestamp))
         originTimestamp = ParseTimestamp(json_string_value(timestamp), -1);
      else
         originTimestamp = -1;

      // Timestamps are stored in database as 32 bit values
      if ((originTimestamp <= 0) || (originTimestamp > 0x7FFFFFFF))
      {
         context->setErrorResponse("Invalid origin timestamp");
         return 400;
      }
   }

   uint64_t eventId = 0;
   EventBuilder builder(eventCode, *object);
   builder.origin(EventOrigin::CLIENT).originTimestamp(originTimestamp).storeEventId(&eventId);

   json_t *tags = json_object_get(request, "tags");
   if ((tags != nullptr) && !json_is_null(tags))
   {
      if (!json_is_array(tags))
      {
         context->setErrorResponse("Tags should be provided as array of strings");
         return 400;
      }
      size_t i;
      json_t *tag;
      json_array_foreach(tags, i, tag)
      {
         if (!json_is_string(tag))
         {
            context->setErrorResponse("Tags should be provided as array of strings");
            return 400;
         }
         StringBuffer value(String(json_string_value(tag), "utf8"));
         value.trim();
         builder.tag(value);
      }
   }

   json_t *parameters = json_object_get(request, "parameters");
   if ((parameters != nullptr) && !json_is_null(parameters))
   {
      if (!json_is_array(parameters))
      {
         context->setErrorResponse("Parameters should be provided as array of objects");
         return 400;
      }
      size_t i;
      json_t *p;
      json_array_foreach(parameters, i, p)
      {
         json_t *name = json_object_get(p, "name");
         json_t *value = json_object_get(p, "value");
         if (!json_is_object(p) || ((name != nullptr) && !json_is_string(name)) || (value == nullptr))
         {
            context->setErrorResponse("Invalid event parameter");
            return 400;
         }

         wchar_t defaultName[32];
         if (name == nullptr)
            nx_swprintf(defaultName, 32, L"parameter%u", static_cast<uint32_t>(i + 1));
         String pname = (name != nullptr) ? String(json_string_value(name), "utf8") : String(defaultName);

         if (json_is_string(value))
            builder.paramUtf8String(pname, json_string_value(value));
         else if (json_is_integer(value))
            builder.param(pname, static_cast<int64_t>(json_integer_value(value)));
         else if (json_is_real(value))
            builder.param(pname, json_real_value(value));
         else
         {
            context->setErrorResponse("Event parameter value should be string or number");
            return 400;
         }
      }
   }

   if (!builder.post())
   {
      context->setErrorResponse("Invalid event code");
      return 400;
   }

   json_t *output = json_object();
   json_object_set_new(output, "eventId", json_integer(eventId));
   context->setResponseData(output);
   json_decref(output);
   return 201;
}
