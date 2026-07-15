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
** File: object_dashboard.cpp
**
** Handlers for GET/PUT /v1/objects/:object-id/dashboard - the content
** (column count, display priority, and element layout) of dashboard and
** dashboard template objects. Delegates to DashboardBase/Dashboard
** modifyFromJSON for the actual mutation.
**
**/

#include "object_helpers.h"

/**
 * Content keys forwarded between the dashboard sub-resource and the object
 * modify path. displayPriority/forcedContextObjectId apply to plain dashboards
 * only; they are silently ignored for dashboard templates.
 */
static const char *s_dashboardContentKeys[] = { "numColumns", "displayPriority", "forcedContextObjectId", "elements", nullptr };

/**
 * Load dashboard (or dashboard template) object by URL placeholder "object-id"
 * and check the given access rights. On failure returns nullptr and writes the
 * matching HTTP status code to *httpCode (400 if the object is not a dashboard).
 */
static shared_ptr<NetObj> LoadDashboardForModify(Context *context, uint32_t requiredRights, int *httpCode)
{
   shared_ptr<NetObj> object = LoadObjectForModify(context, requiredRights, httpCode);
   if (object == nullptr)
      return shared_ptr<NetObj>();

   int objectClass = object->getObjectClass();
   if ((objectClass != OBJECT_DASHBOARD) && (objectClass != OBJECT_DASHBOARDTEMPLATE))
   {
      wchar_t message[256];
      nx_swprintf(message, 256, L"Property group dashboard does not apply to object class %s", object->getObjectClassName());
      context->setErrorResponse(message);
      *httpCode = 400;
      return shared_ptr<NetObj>();
   }

   return object;
}

/**
 * Handler for GET /v1/objects/:object-id/dashboard - dashboard content view
 * (column count, display priority, and element layout). Requires read access.
 */
int H_ObjectDashboard(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> object = LoadDashboardForModify(context, OBJECT_ACCESS_READ, &httpCode);
   if (object == nullptr)
      return httpCode;

   json_t *full = object->toJson(false);
   json_t *view = json_object();
   for(int i = 0; s_dashboardContentKeys[i] != nullptr; i++)
   {
      json_t *value = json_object_get(full, s_dashboardContentKeys[i]);
      if (value != nullptr)
         json_object_set(view, s_dashboardContentKeys[i], value);
   }
   json_decref(full);

   context->setResponseData(view);
   json_decref(view);
   return 200;
}

/**
 * Handler for PUT /v1/objects/:object-id/dashboard - replace dashboard content.
 * The "elements" array, when present, fully replaces the element layout. Only
 * dashboard content keys are applied; other object properties are left untouched.
 */
int H_ObjectDashboardUpdate(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> object = LoadDashboardForModify(context, OBJECT_ACCESS_MODIFY, &httpCode);
   if (object == nullptr)
      return httpCode;

   json_t *request = context->getRequestDocument();
   if ((request == nullptr) || !json_is_object(request))
   {
      context->setErrorResponse("Request body must be a JSON object");
      return 400;
   }

   json_t *patch = json_object();
   for(int i = 0; s_dashboardContentKeys[i] != nullptr; i++)
   {
      json_t *value = json_object_get(request, s_dashboardContentKeys[i]);
      if (value != nullptr)
         json_object_set(patch, s_dashboardContentKeys[i], value);
   }

   json_t *oldSnapshot = object->toJson(false);
   uint32_t rcc = object->modifyFromJSON(patch, context);
   json_decref(patch);
   if (rcc != RCC_SUCCESS)
   {
      json_decref(oldSnapshot);
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, L"H_ObjectDashboardUpdate: modifyFromJSON failed for object %s [%u] with RCC %u",
         object->getName(), object->getId(), rcc);
      context->setErrorResponse("Invalid dashboard content in request");
      return 400;
   }

   json_t *newSnapshot = object->toJson(false);
   context->writeAuditLogWithValues(AUDIT_OBJECTS, true, object->getId(), oldSnapshot, newSnapshot,
      L"Modified dashboard content of object %s [%u]", object->getName(), object->getId());
   json_decref(oldSnapshot);

   context->setResponseData(newSnapshot);
   json_decref(newSnapshot);
   return 200;
}
