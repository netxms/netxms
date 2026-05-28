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
** File: object_properties.cpp
**
** Handlers for PATCH /v1/objects/:object-id (common NetObj scalars) and
** related per-property-group sub-resources. JSON merge-patch semantics
** (RFC 7396) are applied by delegating to NetObj::modifyFromJSON.
**
**/

#include "object_helpers.h"

/**
 * Apply a JSON merge-patch document to the object, write an audit log entry,
 * and emit the updated object as the response. If `groupKey` is non-null the
 * request body is wrapped as `{ groupKey: <body> }` before dispatch so a
 * sub-resource handler (e.g. PATCH /location) shares the same modify path as
 * the top-level PATCH.
 */
static int ApplyJsonPatch(Context *context, NetObj *object, const char *groupKey, const wchar_t *auditLabel)
{
   json_t *request = context->getRequestDocument();
   if ((request == nullptr) || !json_is_object(request))
   {
      context->setErrorResponse("Request body must be a JSON object");
      return 400;
   }

   json_t *patch;
   if (groupKey != nullptr)
   {
      patch = json_object();
      json_object_set(patch, groupKey, request);
   }
   else
   {
      patch = json_incref(request);
   }

   json_t *oldSnapshot = object->toJson(false);
   uint32_t rcc = object->modifyFromJSON(patch, context);
   json_decref(patch);
   if (rcc != RCC_SUCCESS)
   {
      json_decref(oldSnapshot);
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, L"ApplyJsonPatch: modifyFromJSON failed for object %s [%u] (group=%hs) with RCC %u",
         object->getName(), object->getId(), (groupKey != nullptr) ? groupKey : "(top-level)", rcc);
      context->setErrorResponse("Invalid property values in request");
      return 400;
   }

   json_t *newSnapshot = object->toJson(false);
   context->writeAuditLogWithValues(AUDIT_OBJECTS, true, object->getId(), oldSnapshot, newSnapshot,
      auditLabel, object->getName(), object->getId());
   json_decref(oldSnapshot);

   context->setResponseData(newSnapshot);
   json_decref(newSnapshot);
   return 200;
}

/**
 * Handler for PATCH /v1/objects/:object-id - common NetObj scalars.
 * Accepts a JSON merge-patch body. Returns the full updated object on success.
 */
int H_ObjectPropertiesUpdate(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> object = LoadObjectForModify(context, &httpCode);
   if (object == nullptr)
      return httpCode;
   return ApplyJsonPatch(context, object.get(), nullptr, L"Modified properties of object %s [%u]");
}

/**
 * Handler for PATCH /v1/objects/:object-id/location - geo location + postal address.
 */
int H_ObjectLocationUpdate(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> object = LoadObjectForModify(context, &httpCode);
   if (object == nullptr)
      return httpCode;
   return ApplyJsonPatch(context, object.get(), "location", L"Modified location of object %s [%u]");
}

/**
 * Handler for PATCH /v1/objects/:object-id/status-calculation -
 * status calculation/propagation algorithms and thresholds.
 */
int H_ObjectStatusCalculationUpdate(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> object = LoadObjectForModify(context, &httpCode);
   if (object == nullptr)
      return httpCode;
   return ApplyJsonPatch(context, object.get(), "statusCalculation", L"Modified status calculation of object %s [%u]");
}
