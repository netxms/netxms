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
** File: rooms.cpp
**
**/

#include "object_helpers.h"

/**
 * Load room object by URL placeholder "object-id" and check the given access
 * rights. Returns the room on success. On failure returns nullptr and writes the
 * corresponding HTTP status code to *httpCode (400 if the object is not a room).
 */
static shared_ptr<Room> LoadRoomForModify(Context *context, uint64_t requiredRights, int *httpCode)
{
   shared_ptr<NetObj> object = LoadObjectForModify(context, requiredRights, httpCode);
   if (object == nullptr)
      return shared_ptr<Room>();
   if (object->getObjectClass() != OBJECT_ROOM)
   {
      context->setErrorResponse("Object is not a room");
      *httpCode = 400;
      return shared_ptr<Room>();
   }
   return static_pointer_cast<Room>(object);
}

/**
 * Handler for /v1/objects/:object-id/floor-plan
 * Returns room geometry, passive elements, and racks located in the room with their
 * footprint and placement in a single response.
 */
int H_RoomFloorPlan(Context *context)
{
   uint32_t objectId = context->getPlaceholderValueAsUInt32(L"object-id");
   if (objectId == 0)
      return 400;

   shared_ptr<NetObj> object = FindObjectById(objectId);
   if ((object == nullptr) || object->isUnpublished() || object->isDeleted())
      return 404;

   if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ))
      return 403;

   if (object->getObjectClass() != OBJECT_ROOM)
   {
      context->setErrorResponse("Object is not a room");
      return 400;
   }

   json_t *output = static_cast<Room&>(*object).getFloorPlan(context->getUserId());
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for POST /v1/objects/:object-id/floor-plan/passive-elements
 * Creates a new passive element in the room. The server assigns the element ID.
 */
int H_RoomPassiveElementCreate(Context *context)
{
   int httpCode = 0;
   shared_ptr<Room> room = LoadRoomForModify(context, OBJECT_ACCESS_MODIFY, &httpCode);
   if (room == nullptr)
      return httpCode;

   json_t *request = context->getRequestDocument();
   if ((request == nullptr) || !json_is_object(request))
   {
      context->setErrorResponse("Request body must be a JSON object");
      return 400;
   }

   json_t *element = nullptr;
   uint32_t rcc = room->createPassiveElementFromJson(request, &element);
   if (rcc != RCC_SUCCESS)
   {
      context->setErrorResponse("Invalid passive element definition");
      return 400;
   }

   uint32_t elementId = json_object_get_int32(element, "id", 0);
   context->writeAuditLog(AUDIT_OBJECTS, true, room->getId(),
      L"Passive element [%u] added to room %s [%u]", elementId, room->getName(), room->getId());

   wchar_t location[256];
   nx_swprintf(location, 256, L"/v1/objects/%u/floor-plan/passive-elements/%u", room->getId(), elementId);
   context->setResponseHeader(L"Location", location);

   context->setResponseData(element);
   json_decref(element);
   return 201;
}

/**
 * Handler for PATCH /v1/objects/:object-id/floor-plan/passive-elements/:element-id
 * Updates an existing passive element using merge-patch semantics.
 */
int H_RoomPassiveElementUpdate(Context *context)
{
   int httpCode = 0;
   shared_ptr<Room> room = LoadRoomForModify(context, OBJECT_ACCESS_MODIFY, &httpCode);
   if (room == nullptr)
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
   uint32_t rcc = room->updatePassiveElementFromJson(elementId, request, &element);
   if (rcc == RCC_INVALID_OBJECT_ID)
      return 404;
   if (rcc != RCC_SUCCESS)
   {
      context->setErrorResponse("Invalid passive element definition");
      return 400;
   }

   context->writeAuditLog(AUDIT_OBJECTS, true, room->getId(),
      L"Passive element [%u] of room %s [%u] changed", elementId, room->getName(), room->getId());

   context->setResponseData(element);
   json_decref(element);
   return 200;
}

/**
 * Handler for DELETE /v1/objects/:object-id/floor-plan/passive-elements/:element-id
 * Removes a passive element from the room.
 */
int H_RoomPassiveElementDelete(Context *context)
{
   int httpCode = 0;
   shared_ptr<Room> room = LoadRoomForModify(context, OBJECT_ACCESS_MODIFY, &httpCode);
   if (room == nullptr)
      return httpCode;

   uint32_t elementId = context->getPlaceholderValueAsUInt32(L"element-id");
   if (elementId == 0)
      return 400;

   uint32_t rcc = room->deletePassiveElement(elementId);
   if (rcc == RCC_INVALID_OBJECT_ID)
      return 404;

   context->writeAuditLog(AUDIT_OBJECTS, true, room->getId(),
      L"Passive element [%u] removed from room %s [%u]", elementId, room->getName(), room->getId());
   return 204;
}
