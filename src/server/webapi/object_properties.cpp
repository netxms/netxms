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
 * Handler for PATCH /v1/objects/:object-id - common NetObj scalars.
 * Accepts a JSON merge-patch body. Returns the full updated object on success.
 */
int H_ObjectPropertiesUpdate(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> object = LoadObjectForModify(context, &httpCode);
   if (object == nullptr)
      return httpCode;

   json_t *request = context->getRequestDocument();
   if ((request == nullptr) || !json_is_object(request))
   {
      context->setErrorResponse("Request body must be a JSON object");
      return 400;
   }

   json_t *oldSnapshot = object->toJson(false);
   uint32_t rcc = object->modifyFromJSON(request, context);
   if (rcc != RCC_SUCCESS)
   {
      json_decref(oldSnapshot);
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, L"H_ObjectPropertiesUpdate: modifyFromJSON failed for object %s [%u] with RCC %u",
         object->getName(), object->getId(), rcc);
      context->setErrorResponse("Invalid property values in request");
      return 400;
   }

   json_t *newSnapshot = object->toJson(false);
   context->writeAuditLogWithValues(AUDIT_OBJECTS, true, object->getId(), oldSnapshot, newSnapshot,
      L"Modified properties of object %s [%u]", object->getName(), object->getId());
   json_decref(oldSnapshot);

   context->setResponseData(newSnapshot);
   json_decref(newSnapshot);
   return 200;
}
