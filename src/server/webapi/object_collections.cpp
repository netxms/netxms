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
** File: object_collections.cpp
**
** Handlers for item-level CRUD on object property collections:
** custom-attributes and responsible-users. Unlike the merge-patch property
** groups (object_properties.cpp), each collection element is addressed by its
** own URL and manipulated independently.
**
**/

#include "object_helpers.h"
#include <nms_users.h>

/**
 * Maximum length of a custom attribute name (matches attr_name varchar(127)).
 */
#define MAX_CUSTOM_ATTRIBUTE_NAME_LEN  127

/**
 * Validate custom attribute name taken from URL placeholder. On failure writes
 * an error response to the context and returns false. Allowed: spaces, unicode,
 * dots, dashes, colons. Rejected: empty, longer than 127 chars, containing '/'
 * or any control character (< 0x20).
 */
static bool ValidateCustomAttributeName(const wchar_t *name, Context *context)
{
   if ((name == nullptr) || (name[0] == 0))
   {
      context->setErrorResponse("Custom attribute name cannot be empty");
      return false;
   }
   if (wcslen(name) > MAX_CUSTOM_ATTRIBUTE_NAME_LEN)
   {
      context->setErrorResponse("Custom attribute name is too long");
      return false;
   }
   for(const wchar_t *p = name; *p != 0; p++)
   {
      if ((*p == L'/') || (*p < 0x20))
      {
         context->setErrorResponse("Custom attribute name contains invalid characters");
         return false;
      }
   }
   return true;
}

/**
 * Callback for serializing object's own custom attributes as a JSON map
 * name -> { value, inheritable }. Inherited attributes that are not redefined
 * on this object are skipped (mirrors NetObj::toJson filtering).
 */
static EnumerationCallbackResult CustomAttributeToMap(const wchar_t *name, const CustomAttribute *attr, json_t *map)
{
   if ((attr->sourceObject != 0) && !attr->isRedefined())
      return _CONTINUE;

   json_t *entry = json_object();
   json_object_set_new(entry, "value", json_string_t(attr->value));
   json_object_set_new(entry, "inheritable", json_boolean(attr->isInheritable()));

   char utf8name[512];
   wchar_to_utf8(name, -1, utf8name, sizeof(utf8name));
   utf8name[sizeof(utf8name) - 1] = 0;
   json_object_set_new(map, utf8name, entry);
   return _CONTINUE;
}

/**
 * Handler for GET /v1/objects/:object-id/custom-attributes
 * Returns a map of the object's own custom attributes: name -> { value, inheritable }.
 */
int H_ObjectCustomAttributes(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> object = LoadObjectForModify(context, OBJECT_ACCESS_READ, &httpCode);
   if (object == nullptr)
      return httpCode;

   json_t *output = json_object();
   object->forEachCustomAttribute(CustomAttributeToMap, output);
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for PUT /v1/objects/:object-id/custom-attributes/:name
 * Upserts a single custom attribute. Body: { "value": "...", "inheritable": bool }.
 */
int H_ObjectCustomAttributeUpdate(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> object = LoadObjectForModify(context, OBJECT_ACCESS_MODIFY, &httpCode);
   if (object == nullptr)
      return httpCode;

   const wchar_t *name = context->getPlaceholderValue(L"name");
   if (!ValidateCustomAttributeName(name, context))
      return 400;

   json_t *request = context->getRequestDocument();
   if ((request == nullptr) || !json_is_object(request))
   {
      context->setErrorResponse("Request body must be a JSON object");
      return 400;
   }

   json_t *jsonValue = json_object_get(request, "value");
   if ((jsonValue != nullptr) && !json_is_string(jsonValue) && !json_is_null(jsonValue))
   {
      context->setErrorResponse("Custom attribute value must be a string");
      return 400;
   }
   json_t *jsonInheritable = json_object_get(request, "inheritable");
   if ((jsonInheritable != nullptr) && !json_is_boolean(jsonInheritable))
   {
      context->setErrorResponse("Field \"inheritable\" must be a boolean");
      return 400;
   }

   String value = json_object_get_string(request, "value", L"");
   bool inheritable = json_object_get_boolean(request, "inheritable", false);

   SharedString oldValue = object->getCustomAttribute(name);
   object->setCustomAttribute(name, value, inheritable ? StateChange::SET : StateChange::CLEAR);

   context->writeAuditLogWithValues(AUDIT_OBJECTS, true, object->getId(), oldValue.cstr(), value.cstr(), 'T',
      L"Custom attribute \"%s\" of object %s [%u] changed", name, object->getName(), object->getId());

   json_t *output = json_object();
   json_object_set_new(output, "name", json_string_w(name));
   json_object_set_new(output, "value", json_string_t(value));
   json_object_set_new(output, "inheritable", json_boolean(inheritable));
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for DELETE /v1/objects/:object-id/custom-attributes/:name
 * Removes a single custom attribute. Idempotent (no error if attribute absent).
 */
int H_ObjectCustomAttributeDelete(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> object = LoadObjectForModify(context, OBJECT_ACCESS_MODIFY, &httpCode);
   if (object == nullptr)
      return httpCode;

   const wchar_t *name = context->getPlaceholderValue(L"name");
   if (!ValidateCustomAttributeName(name, context))
      return 400;

   object->deleteCustomAttribute(name);
   context->writeAuditLog(AUDIT_OBJECTS, true, object->getId(),
      L"Custom attribute \"%s\" of object %s [%u] deleted", name, object->getName(), object->getId());
   return 204;
}

/**
 * Handler for GET /v1/objects/:object-id/responsible-users
 * Returns array of the object's own responsible users: [ { userId, tag } ].
 */
int H_ObjectResponsibleUsers(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> object = LoadObjectForModify(context, OBJECT_ACCESS_READ, &httpCode);
   if (object == nullptr)
      return httpCode;

   unique_ptr<StructArray<ResponsibleUser>> users = object->getResponsibleUsers();
   json_t *output = json_array();
   for(int i = 0; i < users->size(); i++)
   {
      const ResponsibleUser *r = users->get(i);
      json_t *entry = json_object();
      json_object_set_new(entry, "userId", json_integer(r->userId));
      json_object_set_new(entry, "tag", json_string_t(r->tag));
      json_array_append_new(output, entry);
   }
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for PUT /v1/objects/:object-id/responsible-users/:user-id
 * Upserts the tag for a single responsible user. Body: { "tag": "..." }.
 */
int H_ObjectResponsibleUserUpdate(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> object = LoadObjectForModify(context, OBJECT_ACCESS_MODIFY, &httpCode);
   if (object == nullptr)
      return httpCode;

   uint32_t userId = context->getPlaceholderValueAsUInt32(L"user-id");
   wchar_t userName[MAX_USER_NAME];
   if ((userId == 0) || (ResolveUserId(userId, userName, false) == nullptr))
   {
      wchar_t message[64];
      _sntprintf(message, 64, L"User or group %u not found", userId);
      context->setErrorResponse(message);
      return 400;
   }

   json_t *request = context->getRequestDocument();
   if ((request != nullptr) && !json_is_object(request))
   {
      context->setErrorResponse("Request body must be a JSON object");
      return 400;
   }

   json_t *jsonTag = (request != nullptr) ? json_object_get(request, "tag") : nullptr;
   if ((jsonTag != nullptr) && !json_is_string(jsonTag) && !json_is_null(jsonTag))
   {
      context->setErrorResponse("Field \"tag\" must be a string");
      return 400;
   }

   String tag = (request != nullptr) ? json_object_get_string(request, "tag", L"") : String(L"");
   if (tag.length() >= MAX_RESPONSIBLE_USER_TAG_LEN)
   {
      context->setErrorResponse("Responsible user tag is too long");
      return 400;
   }

   object->updateResponsibleUser(userId, tag, context);
   context->writeAuditLog(AUDIT_OBJECTS, true, object->getId(),
      L"Responsible user %s [%u] tag set to \"%s\" on object %s [%u]", userName, userId, tag.cstr(), object->getName(), object->getId());

   json_t *output = json_object();
   json_object_set_new(output, "userId", json_integer(userId));
   json_object_set_new(output, "tag", json_string_t(tag));
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for DELETE /v1/objects/:object-id/responsible-users/:user-id
 * Removes a single responsible user. Idempotent (no error if user not responsible).
 */
int H_ObjectResponsibleUserDelete(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> object = LoadObjectForModify(context, OBJECT_ACCESS_MODIFY, &httpCode);
   if (object == nullptr)
      return httpCode;

   uint32_t userId = context->getPlaceholderValueAsUInt32(L"user-id");
   if (userId == 0)
      return 400;

   if (object->deleteResponsibleUser(userId, context))
   {
      context->writeAuditLog(AUDIT_OBJECTS, true, object->getId(),
         L"Responsible user [%u] removed from object %s [%u]", userId, object->getName(), object->getId());
   }
   return 204;
}
