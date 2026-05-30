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

/**
 * Parse the request body as a JSON array of object identifiers into the given
 * IntegerArray. On failure writes a 400 error response and returns false.
 */
static bool ParseObjectIdArray(json_t *request, IntegerArray<uint32_t> *out, Context *context)
{
   if (!json_is_array(request))
   {
      context->setErrorResponse("Request body must be a JSON array of object identifiers");
      return false;
   }
   size_t i;
   json_t *element;
   json_array_foreach(request, i, element)
   {
      if (!json_is_integer(element))
      {
         context->setErrorResponse("Object identifier list must contain only integers");
         return false;
      }
      out->add(static_cast<uint32_t>(json_integer_value(element)));
   }
   return true;
}

/**
 * Handler for PUT /v1/objects/:object-id/dashboards
 * Replaces the full list of dashboards/network maps associated with the object.
 * Body is a JSON array of object IDs.
 */
int H_ObjectDashboards(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> object = LoadObjectForModify(context, OBJECT_ACCESS_MODIFY, &httpCode);
   if (object == nullptr)
      return httpCode;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      context->setErrorResponse("Request body must be a JSON array");
      return 400;
   }

   IntegerArray<uint32_t> dashboards;
   if (!ParseObjectIdArray(request, &dashboards, context))
      return 400;

   json_t *oldSnapshot = object->toJson(false);
   object->setDashboards(dashboards);

   json_t *newSnapshot = object->toJson(false);
   context->writeAuditLogWithValues(AUDIT_OBJECTS, true, object->getId(), oldSnapshot, newSnapshot,
      L"Associated dashboards of object %s [%u] changed", object->getName(), object->getId());
   json_decref(oldSnapshot);

   context->setResponseData(newSnapshot);
   json_decref(newSnapshot);
   return 200;
}

/**
 * Handler for PUT /v1/objects/:object-id/trusted-objects
 * Replaces the full list of trusted object IDs. Body is a JSON array of object
 * IDs. Rejects the object's own ID and references to non-existent objects.
 */
int H_ObjectTrustedObjects(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> object = LoadObjectForModify(context, OBJECT_ACCESS_MODIFY, &httpCode);
   if (object == nullptr)
      return httpCode;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      context->setErrorResponse("Request body must be a JSON array");
      return 400;
   }

   IntegerArray<uint32_t> trustedObjects;
   if (!ParseObjectIdArray(request, &trustedObjects, context))
      return 400;

   for(int i = 0; i < trustedObjects.size(); i++)
   {
      uint32_t id = trustedObjects.get(i);
      if (id == object->getId())
      {
         context->setErrorResponse("Cannot trust object itself");
         return 400;
      }
      if (FindObjectById(id) == nullptr)
      {
         wchar_t message[64];
         _sntprintf(message, 64, L"Trusted object %u not found", id);
         context->setErrorResponse(message);
         return 400;
      }
   }

   json_t *oldSnapshot = object->toJson(false);
   object->setTrustedObjects(trustedObjects);

   json_t *newSnapshot = object->toJson(false);
   context->writeAuditLogWithValues(AUDIT_OBJECTS, true, object->getId(), oldSnapshot, newSnapshot,
      L"Trusted objects of object %s [%u] changed", object->getName(), object->getId());
   json_decref(oldSnapshot);

   context->setResponseData(newSnapshot);
   json_decref(newSnapshot);
   return 200;
}

/**
 * Maximum length of a URL or its description (matches object_urls varchar(2000)).
 */
#define MAX_OBJECT_URL_LEN  2000

/**
 * Read and validate the { url, description } body for a URL create/update. On
 * failure writes a 400 error response and returns false. "url" is required and
 * non-empty; "description" is optional (absent/null treated as empty string).
 */
static bool ParseUrlBody(Context *context, SharedString *url, SharedString *description)
{
   json_t *request = context->getRequestDocument();
   if ((request == nullptr) || !json_is_object(request))
   {
      context->setErrorResponse("Request body must be a JSON object");
      return false;
   }

   json_t *jsonUrl = json_object_get(request, "url");
   if (!json_is_string(jsonUrl))
   {
      context->setErrorResponse("Field \"url\" is required and must be a string");
      return false;
   }
   json_t *jsonDescription = json_object_get(request, "description");
   if ((jsonDescription != nullptr) && !json_is_string(jsonDescription) && !json_is_null(jsonDescription))
   {
      context->setErrorResponse("Field \"description\" must be a string");
      return false;
   }

   *url = SharedString(json_string_value(jsonUrl), "utf8");
   if (url->isEmpty())
   {
      context->setErrorResponse("Field \"url\" cannot be empty");
      return false;
   }
   if (url->length() > MAX_OBJECT_URL_LEN)
   {
      context->setErrorResponse("Field \"url\" is too long");
      return false;
   }

   *description = json_is_string(jsonDescription) ? SharedString(json_string_value(jsonDescription), "utf8") : SharedString(L"");
   if (description->length() > MAX_OBJECT_URL_LEN)
   {
      context->setErrorResponse("Field \"description\" is too long");
      return false;
   }
   return true;
}

/**
 * Build the JSON representation of a single URL: { id, url, description }.
 */
static json_t *UrlToJson(uint32_t urlId, const SharedString& url, const SharedString& description)
{
   json_t *entry = json_object();
   json_object_set_new(entry, "id", json_integer(urlId));
   json_object_set_new(entry, "url", json_string_t(url.cstr()));
   json_object_set_new(entry, "description", json_string_t(description.cstr()));
   return entry;
}

/**
 * Handler for GET /v1/objects/:object-id/urls
 * Returns the object's associated URLs as an array of { id, url, description }.
 */
int H_ObjectUrls(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> object = LoadObjectForModify(context, OBJECT_ACCESS_READ, &httpCode);
   if (object == nullptr)
      return httpCode;

   json_t *output = object->getUrlsAsJson();
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for POST /v1/objects/:object-id/urls
 * Creates a new associated URL. The server assigns the URL ID.
 */
int H_ObjectUrlCreate(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> object = LoadObjectForModify(context, OBJECT_ACCESS_MODIFY, &httpCode);
   if (object == nullptr)
      return httpCode;

   SharedString url, description;
   if (!ParseUrlBody(context, &url, &description))
      return 400;

   uint32_t urlId = object->addUrl(url, description);
   context->writeAuditLog(AUDIT_OBJECTS, true, object->getId(),
      L"URL [%u] \"%s\" added to object %s [%u]", urlId, url.cstr(), object->getName(), object->getId());

   wchar_t location[256];
   _sntprintf(location, 256, L"/v1/objects/%u/urls/%u", object->getId(), urlId);
   context->setResponseHeader(L"Location", location);

   json_t *output = UrlToJson(urlId, url, description);
   context->setResponseData(output);
   json_decref(output);
   return 201;
}

/**
 * Handler for PUT /v1/objects/:object-id/urls/:url-id
 * Updates an existing associated URL.
 */
int H_ObjectUrlUpdate(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> object = LoadObjectForModify(context, OBJECT_ACCESS_MODIFY, &httpCode);
   if (object == nullptr)
      return httpCode;

   uint32_t urlId = context->getPlaceholderValueAsUInt32(L"url-id");
   if (urlId == 0)
      return 400;

   SharedString url, description;
   if (!ParseUrlBody(context, &url, &description))
      return 400;

   if (!object->updateUrl(urlId, url, description))
      return 404;

   context->writeAuditLog(AUDIT_OBJECTS, true, object->getId(),
      L"URL [%u] of object %s [%u] changed to \"%s\"", urlId, object->getName(), object->getId(), url.cstr());

   json_t *output = UrlToJson(urlId, url, description);
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for DELETE /v1/objects/:object-id/urls/:url-id
 * Removes an associated URL.
 */
int H_ObjectUrlDelete(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> object = LoadObjectForModify(context, OBJECT_ACCESS_MODIFY, &httpCode);
   if (object == nullptr)
      return httpCode;

   uint32_t urlId = context->getPlaceholderValueAsUInt32(L"url-id");
   if (urlId == 0)
      return 400;

   if (!object->deleteUrl(urlId))
      return 404;

   context->writeAuditLog(AUDIT_OBJECTS, true, object->getId(),
      L"URL [%u] removed from object %s [%u]", urlId, object->getName(), object->getId());
   return 204;
}

/**
 * Map a binding result code returned by ChangeObjectBinding() to an HTTP status
 * code and write the matching error response to the context. Must only be called
 * for failure codes (rcc != RCC_SUCCESS).
 */
static int BindingErrorResponse(Context *context, uint32_t rcc, const NetObj& parent, const NetObj& child, const SharedString& conflictingTemplateName)
{
   wchar_t message[512];
   switch(rcc)
   {
      case RCC_ACCESS_DENIED:
         context->setErrorResponse("Access denied");
         return 403;
      case RCC_INCOMPATIBLE_OPERATION:
         _sntprintf(message, 512, L"Object of class %s cannot be bound as a child of object of class %s",
            child.getObjectClassName(), parent.getObjectClassName());
         context->setErrorResponse(message);
         return 400;
      case RCC_OBJECT_LOOP:
         context->setErrorResponse("Binding would create a loop in the object hierarchy");
         return 409;
      case RCC_TEMPLATE_EXCLUSION_CONFLICT:
         _sntprintf(message, 512, L"Template conflicts with already applied template \"%s\" from the same exclusion group",
            conflictingTemplateName.cstr());
         context->setErrorResponse(message);
         return 409;
      case RCC_DCI_COPY_ERRORS:
         context->setErrorResponse("Errors occurred while copying data collection configuration from template");
         return 500;
      case RCC_INVALID_ARGUMENT:
         _sntprintf(message, 512, L"Object %s [%u] is not a direct child of object %s [%u]",
            child.getName(), child.getId(), parent.getName(), parent.getId());
         context->setErrorResponse(message);
         return 400;
      default:
         context->setErrorResponse("Internal error");
         return 500;
   }
}

/**
 * Load the child object addressed by URL placeholder "child-id" and check that
 * the caller has modify access on it. On failure returns nullptr and writes the
 * matching HTTP status code (400 / 403 / 404) to *httpCode.
 */
static shared_ptr<NetObj> LoadChildForBinding(Context *context, int *httpCode)
{
   uint32_t childId = context->getPlaceholderValueAsUInt32(L"child-id");
   if (childId == 0)
   {
      *httpCode = 400;
      return shared_ptr<NetObj>();
   }

   shared_ptr<NetObj> child = FindObjectById(childId);
   if ((child == nullptr) || child->isUnpublished() || child->isDeleted())
   {
      *httpCode = 404;
      return shared_ptr<NetObj>();
   }

   if (!child->checkAccessRights(context->getUserId(), OBJECT_ACCESS_MODIFY))
   {
      context->writeAuditLog(AUDIT_OBJECTS, false, child->getId(),
         L"Access denied on modification of object %s [%u]", child->getName(), child->getId());
      *httpCode = 403;
      return shared_ptr<NetObj>();
   }

   return child;
}

/**
 * Handler for PUT /v1/objects/:object-id/children/:child-id
 * Binds the child object to the object addressed by the URL as its parent. Query
 * parameter "force=true" overrides a template exclusion group conflict. Requires
 * modify access on both the parent and the child object.
 */
int H_ObjectBindChild(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> parent = LoadObjectForModify(context, OBJECT_ACCESS_MODIFY, &httpCode);
   if (parent == nullptr)
      return httpCode;

   shared_ptr<NetObj> child = LoadChildForBinding(context, &httpCode);
   if (child == nullptr)
      return httpCode;

   SharedString conflictingTemplateName;
   uint32_t rcc = ChangeObjectBinding(parent, child, true, context->getQueryParameterAsBoolean("force", false), false, context, &conflictingTemplateName);
   if (rcc != RCC_SUCCESS)
      return BindingErrorResponse(context, rcc, *parent, *child, conflictingTemplateName);

   json_t *output = child->toJson(true);
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for DELETE /v1/objects/:object-id/children/:child-id
 * Unbinds the child object from the object addressed by the URL. Query parameter
 * "remove-dci=true" removes data collection items created from a template when a
 * template is unbound from a data collection target. Requires modify access on
 * both the parent and the child object.
 */
int H_ObjectUnbindChild(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> parent = LoadObjectForModify(context, OBJECT_ACCESS_MODIFY, &httpCode);
   if (parent == nullptr)
      return httpCode;

   shared_ptr<NetObj> child = LoadChildForBinding(context, &httpCode);
   if (child == nullptr)
      return httpCode;

   SharedString conflictingTemplateName;
   uint32_t rcc = ChangeObjectBinding(parent, child, false, false, context->getQueryParameterAsBoolean("remove-dci", false), context, &conflictingTemplateName);
   if (rcc != RCC_SUCCESS)
      return BindingErrorResponse(context, rcc, *parent, *child, conflictingTemplateName);

   return 204;
}
