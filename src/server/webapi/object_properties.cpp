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
   shared_ptr<NetObj> object = LoadObjectForModify(context, OBJECT_ACCESS_MODIFY, &httpCode);
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
   shared_ptr<NetObj> object = LoadObjectForModify(context, OBJECT_ACCESS_MODIFY, &httpCode);
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
   shared_ptr<NetObj> object = LoadObjectForModify(context, OBJECT_ACCESS_MODIFY, &httpCode);
   if (object == nullptr)
      return httpCode;
   return ApplyJsonPatch(context, object.get(), "statusCalculation", L"Modified status calculation of object %s [%u]");
}

/**
 * Handler for GET /v1/objects/:object-id/access-rights - object's access control
 * list and inherited-rights flag. Requires read access on the object (the same
 * ACL is already exposed by the full object representation).
 */
int H_ObjectAccessRights(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> object = LoadObjectForModify(context, OBJECT_ACCESS_READ, &httpCode);
   if (object == nullptr)
      return httpCode;

   json_t *output = object->accessListToJson();
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for PUT /v1/objects/:object-id/access-rights - full replacement of the
 * object's access control list plus the inherited-rights flag. Requires both
 * modify and access-control rights (mirrors the NXCP object-modification path).
 */
int H_ObjectAccessRightsUpdate(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> object = LoadObjectForModify(context, OBJECT_ACCESS_MODIFY | OBJECT_ACCESS_ACL, &httpCode);
   if (object == nullptr)
      return httpCode;

   json_t *request = context->getRequestDocument();
   if ((request == nullptr) || !json_is_object(request))
   {
      context->setErrorResponse("Request body must be a JSON object");
      return 400;
   }

   json_t *oldSnapshot = object->accessListToJson();
   uint32_t rcc = object->updateAccessListFromJson(request);
   if (rcc != RCC_SUCCESS)
   {
      json_decref(oldSnapshot);
      context->setErrorResponse("Invalid access list in request");
      return 400;
   }

   json_t *newSnapshot = object->accessListToJson();
   context->writeAuditLogWithValues(AUDIT_OBJECTS, true, object->getId(), oldSnapshot, newSnapshot,
      L"Modified access rights of object %s [%u]", object->getName(), object->getId());
   json_decref(oldSnapshot);

   context->setResponseData(newSnapshot);
   json_decref(newSnapshot);
   return 200;
}
