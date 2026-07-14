/*
** NetXMS - Network Management System
** Copyright (C) 2025 Raden Solutions
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
** File: racks.cpp
**
**/

#include "object_helpers.h"

/**
 * Load rack object by URL placeholder "object-id" and check the given access
 * rights. Returns the rack on success. On failure returns nullptr and writes the
 * corresponding HTTP status code to *httpCode (400 if the object is not a rack).
 */
static shared_ptr<Rack> LoadRackForModify(Context *context, uint32_t requiredRights, int *httpCode)
{
   shared_ptr<NetObj> object = LoadObjectForModify(context, requiredRights, httpCode);
   if (object == nullptr)
      return shared_ptr<Rack>();
   if (object->getObjectClass() != OBJECT_RACK)
   {
      context->setErrorResponse("Object is not a rack");
      *httpCode = 400;
      return shared_ptr<Rack>();
   }
   return static_pointer_cast<Rack>(object);
}

/**
 * Handler for /v1/objects/:object-id/rack-layout
 * Returns rack metadata, passive elements, and placed child objects with their
 * placement geometry in a single response.
 */
int H_RackLayout(Context *context)
{
   uint32_t objectId = context->getPlaceholderValueAsUInt32(_T("object-id"));
   if (objectId == 0)
      return 400;

   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object == nullptr)
      return 404;

   if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ))
      return 403;

   if (object->getObjectClass() != OBJECT_RACK)
      return 400;

   json_t *output = static_cast<Rack&>(*object).getRackLayout(context->getUserId());
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for POST /v1/objects/:object-id/rack-layout/passive-elements
 * Creates a new passive element on the rack. The server assigns the element ID.
 */
int H_RackPassiveElementCreate(Context *context)
{
   int httpCode = 0;
   shared_ptr<Rack> rack = LoadRackForModify(context, OBJECT_ACCESS_MODIFY, &httpCode);
   if (rack == nullptr)
      return httpCode;

   json_t *request = context->getRequestDocument();
   if ((request == nullptr) || !json_is_object(request))
   {
      context->setErrorResponse("Request body must be a JSON object");
      return 400;
   }

   json_t *element = nullptr;
   uint32_t rcc = rack->createPassiveElementFromJson(request, &element);
   if (rcc != RCC_SUCCESS)
   {
      context->setErrorResponse("Invalid passive element definition");
      return 400;
   }

   uint32_t elementId = json_object_get_int32(element, "id", 0);
   context->writeAuditLog(AUDIT_OBJECTS, true, rack->getId(),
      L"Passive element [%u] added to rack %s [%u]", elementId, rack->getName(), rack->getId());

   wchar_t location[256];
   _sntprintf(location, 256, L"/v1/objects/%u/rack-layout/passive-elements/%u", rack->getId(), elementId);
   context->setResponseHeader(L"Location", location);

   context->setResponseData(element);
   json_decref(element);
   return 201;
}

/**
 * Handler for PATCH /v1/objects/:object-id/rack-layout/passive-elements/:element-id
 * Updates an existing passive element using merge-patch semantics.
 */
int H_RackPassiveElementUpdate(Context *context)
{
   int httpCode = 0;
   shared_ptr<Rack> rack = LoadRackForModify(context, OBJECT_ACCESS_MODIFY, &httpCode);
   if (rack == nullptr)
      return httpCode;

   uint32_t elementId = context->getPlaceholderValueAsUInt32(L"element-id");
   if (elementId == 0)
      return 400;

   json_t *request = context->getRequestDocument();
   if ((request == nullptr) || !json_is_object(request))
   {
      context->setErrorResponse("Request body must be a JSON object");
      return 400;
   }

   json_t *element = nullptr;
   uint32_t rcc = rack->updatePassiveElementFromJson(elementId, request, &element);
   if (rcc == RCC_INVALID_OBJECT_ID)
      return 404;
   if (rcc != RCC_SUCCESS)
   {
      context->setErrorResponse("Invalid passive element definition");
      return 400;
   }

   context->writeAuditLog(AUDIT_OBJECTS, true, rack->getId(),
      L"Passive element [%u] of rack %s [%u] changed", elementId, rack->getName(), rack->getId());

   context->setResponseData(element);
   json_decref(element);
   return 200;
}

/**
 * Handler for DELETE /v1/objects/:object-id/rack-layout/passive-elements/:element-id
 * Removes a passive element from the rack.
 */
int H_RackPassiveElementDelete(Context *context)
{
   int httpCode = 0;
   shared_ptr<Rack> rack = LoadRackForModify(context, OBJECT_ACCESS_MODIFY, &httpCode);
   if (rack == nullptr)
      return httpCode;

   uint32_t elementId = context->getPlaceholderValueAsUInt32(L"element-id");
   if (elementId == 0)
      return 400;

   uint32_t rcc = rack->deletePassiveElement(elementId);
   if (rcc == RCC_INVALID_OBJECT_ID)
      return 404;

   context->writeAuditLog(AUDIT_OBJECTS, true, rack->getId(),
      L"Passive element [%u] removed from rack %s [%u]", elementId, rack->getName(), rack->getId());
   return 204;
}
