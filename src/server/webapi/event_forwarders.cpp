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
** File: event_forwarders.cpp
**
**/

#include "webapi.h"
#include <nms_actions.h>

/**
 * Handler for GET /v1/event-forwarders
 */
int H_EventForwarders(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SERVER_CONFIG))
      return 403;

   json_t *output = GetEventForwarders(false, true);
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/event-forwarders/:forwarder-name
 */
int H_EventForwarderDetails(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SERVER_CONFIG))
      return 403;

   const wchar_t *forwarderName = context->getPlaceholderValue(L"forwarder-name");
   if (forwarderName == nullptr)
      return 400;

   json_t *output = GetEventForwarderByName(forwarderName, true);
   if (output == nullptr)
      return 404;

   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for POST /v1/event-forwarders
 */
int H_EventForwarderCreate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SERVER_CONFIG))
      return 403;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   json_t *jsonName = json_object_get(request, "name");
   json_t *jsonDriverName = json_object_get(request, "driverName");
   if (!json_is_string(jsonName) || !json_is_string(jsonDriverName))
   {
      context->setErrorResponse("Missing or invalid required fields (name, driverName)");
      return 400;
   }

   wchar_t name[MAX_OBJECT_NAME], driverName[MAX_OBJECT_NAME], description[MAX_NC_DESCRIPTION];
   utf8_to_wchar(json_string_value(jsonName), -1, name, MAX_OBJECT_NAME);
   utf8_to_wchar(json_string_value(jsonDriverName), -1, driverName, MAX_OBJECT_NAME);

   if (name[0] == 0)
   {
      context->setErrorResponse("Event forwarder name cannot be empty");
      return 400;
   }

   if (driverName[0] == 0)
   {
      context->setErrorResponse("Driver name cannot be empty");
      return 400;
   }

   if (IsEventForwarderExists(name))
   {
      context->setErrorResponse("Event forwarder with this name already exists");
      return 409;
   }

   json_t *jsonDescription = json_object_get(request, "description");
   if (json_is_string(jsonDescription))
      utf8_to_wchar(json_string_value(jsonDescription), -1, description, MAX_NC_DESCRIPTION);
   else
      description[0] = 0;

   json_t *jsonConfig = json_object_get(request, "configuration");
   json_t *configuration = json_is_object(jsonConfig) ? json_incref(jsonConfig) : json_object();

   CreateEventForwarder(name, description, driverName, configuration);
   NotifyClientSessions(NX_NOTIFY_EVENT_FORWARDER_CHANGED, 0);
   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Event forwarder \"%s\" created via REST API", name);

   json_t *output = GetEventForwarderByName(name, true);
   context->setResponseData(output);
   json_decref(output);
   return 201;
}

/**
 * Handler for PUT /v1/event-forwarders/:forwarder-name
 */
int H_EventForwarderUpdate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SERVER_CONFIG))
      return 403;

   const wchar_t *forwarderName = context->getPlaceholderValue(L"forwarder-name");
   if (forwarderName == nullptr)
      return 400;

   if (!IsEventForwarderExists(forwarderName))
      return 404;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   // Get current forwarder data to use as defaults for unspecified fields
   json_t *current = GetEventForwarderByName(forwarderName, true);
   if (current == nullptr)
      return 404;

   json_t *jsonDriverName = json_object_get(request, "driverName");
   const char *driverNameUtf8 = json_is_string(jsonDriverName) ? json_string_value(jsonDriverName) : json_string_value(json_object_get(current, "driverName"));

   json_t *jsonDescription = json_object_get(request, "description");
   const char *descriptionUtf8 = json_is_string(jsonDescription) ? json_string_value(jsonDescription) : json_string_value(json_object_get(current, "description"));

   wchar_t driverName[MAX_OBJECT_NAME], description[MAX_NC_DESCRIPTION];
   utf8_to_wchar(driverNameUtf8, -1, driverName, MAX_OBJECT_NAME);
   utf8_to_wchar(descriptionUtf8, -1, description, MAX_NC_DESCRIPTION);

   json_t *jsonConfig = json_object_get(request, "configuration");
   json_t *configuration = json_is_object(jsonConfig) ? json_incref(jsonConfig) : json_incref(json_object_get(current, "configuration"));

   json_decref(current);

   UpdateEventForwarder(forwarderName, description, driverName, configuration);
   NotifyClientSessions(NX_NOTIFY_EVENT_FORWARDER_CHANGED, 0);
   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Event forwarder \"%s\" updated via REST API", forwarderName);

   json_t *output = GetEventForwarderByName(forwarderName, true);
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for DELETE /v1/event-forwarders/:forwarder-name
 */
int H_EventForwarderDelete(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SERVER_CONFIG))
      return 403;

   const wchar_t *forwarderName = context->getPlaceholderValue(L"forwarder-name");
   if (forwarderName == nullptr)
      return 400;

   wchar_t name[MAX_OBJECT_NAME];
   wcslcpy(name, forwarderName, MAX_OBJECT_NAME);

   if (CheckForwarderIsUsedInAction(name))
   {
      context->setErrorResponse("Event forwarder is used in server actions");
      return 409;
   }

   if (!DeleteEventForwarder(name))
      return 404;

   NotifyClientSessions(NX_NOTIFY_EVENT_FORWARDER_CHANGED, 0);
   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Event forwarder \"%s\" deleted via REST API", name);
   return 204;
}

/**
 * Handler for POST /v1/event-forwarders/:forwarder-name/rename
 */
int H_EventForwarderRename(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SERVER_CONFIG))
      return 403;

   const wchar_t *forwarderName = context->getPlaceholderValue(L"forwarder-name");
   if (forwarderName == nullptr)
      return 400;

   if (!IsEventForwarderExists(forwarderName))
      return 404;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   json_t *jsonNewName = json_object_get(request, "newName");
   if (!json_is_string(jsonNewName))
   {
      context->setErrorResponse("Missing or invalid required field (newName)");
      return 400;
   }

   wchar_t newName[MAX_OBJECT_NAME];
   utf8_to_wchar(json_string_value(jsonNewName), -1, newName, MAX_OBJECT_NAME);
   if (newName[0] == 0)
   {
      context->setErrorResponse("New event forwarder name cannot be empty");
      return 400;
   }

   if (IsEventForwarderExists(newName))
   {
      context->setErrorResponse("Event forwarder with this name already exists");
      return 409;
   }

   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Event forwarder \"%s\" renamed to \"%s\" via REST API", forwarderName, newName);
   RenameEventForwarder(MemCopyString(forwarderName), MemCopyString(newName));
   NotifyClientSessions(NX_NOTIFY_EVENT_FORWARDER_CHANGED, 0);
   return 200;
}

/**
 * Handler for GET /v1/event-forwarder-drivers
 */
int H_EventForwarderDrivers(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SERVER_CONFIG))
      return 403;

   json_t *output = GetEventForwarderDriversAsJson();
   context->setResponseData(output);
   json_decref(output);
   return 200;
}
