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
** File: object_helpers.cpp
**
**/

#include "object_helpers.h"

/**
 * Load object addressed by URL placeholder "object-id" and check that the caller
 * has either all rights from `requiredRights` or all rights from `alternativeRights`
 * on it. On any failure returns nullptr and writes the matching HTTP status code
 * (400 / 403 / 404) to *httpCode. A denied modify request is recorded in the audit log.
 */
shared_ptr<NetObj> LoadObjectForModify(Context *context, uint64_t requiredRights, uint64_t alternativeRights, int *httpCode)
{
   uint32_t objectId = context->getPlaceholderValueAsUInt32(L"object-id");
   if (objectId == 0)
   {
      *httpCode = 400;
      return shared_ptr<NetObj>();
   }

   shared_ptr<NetObj> object = FindObjectById(objectId);
   if ((object == nullptr) || object->isUnpublished() || object->isDeleted())
   {
      *httpCode = 404;
      return shared_ptr<NetObj>();
   }

   if (!object->checkAccessRights(context->getUserId(), requiredRights) &&
       !object->checkAccessRights(context->getUserId(), alternativeRights))
   {
      if (requiredRights & OBJECT_ACCESS_MODIFY)
         context->writeAuditLog(AUDIT_OBJECTS, false, object->getId(),
            L"Access denied on modification of object %s [%u]", object->getName(), object->getId());
      *httpCode = 403;
      return shared_ptr<NetObj>();
   }

   return object;
}

/**
 * Apply a JSON merge-patch document to the object, write an audit log entry, and
 * emit the updated object as the response. If `groupKey` is non-null the request
 * body is wrapped as `{ groupKey: <body> }` before dispatch so a sub-resource
 * handler (e.g. PATCH /location) shares the same modify path as the top-level PATCH.
 */
int ApplyJsonPatch(Context *context, NetObj *object, const char *groupKey, const wchar_t *auditLabel)
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

   json_t *oldSnapshot = object->toJson(OBJECT_JSON_SERVER_SIDE_ATTRIBUTES);
   uint32_t rcc = object->modifyFromJSON(patch, context);
   json_decref(patch);
   if (rcc != RCC_SUCCESS)
   {
      json_decref(oldSnapshot);
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, L"ApplyJsonPatch: modifyFromJSON failed for object %s [%u] (group=%hs) with RCC %u",
         object->getName(), object->getId(), (groupKey != nullptr) ? groupKey : "(top-level)", rcc);
      switch(rcc)
      {
         case RCC_ACCESS_DENIED:
            context->setErrorResponse("Access denied");
            return 403;
         case RCC_IP_ADDRESS_CONFLICT:
            context->setErrorResponse("IP address is already used by another node or subnet");
            return 409;
         case RCC_SUBNET_OVERLAP:
            context->setErrorResponse("Subnet overlaps with existing one");
            return 409;
         case RCC_INVALID_OBJECT_ID:
            context->setErrorResponse("Request references an object that does not exist or is not of the required class");
            return 400;
         case RCC_OBJECT_LOOP:
            context->setErrorResponse("Requested change would create a loop in the object tree");
            return 409;
         case RCC_INVALID_TIME_ZONE:
            context->setErrorResponse("Invalid or unresolvable time zone");
            return 400;
         default:
            context->setErrorResponse("Invalid property values in request");
            return 400;
      }
   }

   json_t *newSnapshot = object->toJson(OBJECT_JSON_SERVER_SIDE_ATTRIBUTES);
   context->writeAuditLogWithValues(AUDIT_OBJECTS, true, object->getId(), oldSnapshot, newSnapshot,
      auditLabel, object->getName(), object->getId());
   json_decref(oldSnapshot);
   json_decref(newSnapshot);

   json_t *output = object->toJson();
   AddEffectiveRights(output, *object, context->getUserId());
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Load node addressed by URL placeholder "object-id" for a node-scoped table request and check read access.
 * On any failure returns nullptr and writes the matching HTTP status code (400 / 403 / 404) to *httpCode.
 */
shared_ptr<Node> LoadNodeForTableRequest(Context *context, int *httpCode)
{
   uint32_t objectId = context->getPlaceholderValueAsUInt32(L"object-id");
   if (objectId == 0)
   {
      *httpCode = 400;
      return shared_ptr<Node>();
   }

   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object == nullptr)
   {
      *httpCode = 404;
      return shared_ptr<Node>();
   }

   if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ))
   {
      *httpCode = 403;
      return shared_ptr<Node>();
   }

   if (object->getObjectClass() != OBJECT_NODE)
   {
      context->setErrorResponse("Object is not a node");
      *httpCode = 400;
      return shared_ptr<Node>();
   }

   return static_pointer_cast<Node>(object);
}

/**
 * Resolve interface name by index. Returns JSON null if there is no matching interface object.
 * When preferParent is set, an interface that is a member of an Ethernet or LAG parent interface
 * is reported under the parent's name (same rule as ForwardingDatabase::interfaceIndexToName).
 */
json_t *InterfaceNameToJson(const Node& node, uint32_t ifIndex, bool preferParent)
{
   shared_ptr<Interface> iface = node.findInterfaceByIndex(ifIndex);
   if (iface == nullptr)
      return json_null();

   if (preferParent && (iface->getParentInterfaceId() != 0))
   {
      shared_ptr<Interface> parent = static_pointer_cast<Interface>(FindObjectById(iface->getParentInterfaceId(), OBJECT_INTERFACE));
      if ((parent != nullptr) &&
          ((parent->getIfType() == IFTYPE_ETHERNET_CSMACD) || (parent->getIfType() == IFTYPE_IEEE8023ADLAG)))
         return json_string_t(parent->getName());
   }

   return json_string_t(iface->getName());
}
