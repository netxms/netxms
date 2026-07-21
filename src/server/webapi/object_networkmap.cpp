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
** File: object_networkmap.cpp
**
** Handlers for GET/PUT /v1/objects/:object-id/map - the content (layout
** settings, background, element and link layout) of network map objects.
** Delegates to NetworkMap::modifyFromJSON for the actual mutation.
**
**/

#include "object_helpers.h"

/**
 * Network map content keys forwarded between the map sub-resource and the object
 * modify path. Restricting the forwarded set keeps the sub-resource focused on
 * map content (it cannot rename the object or change unrelated properties).
 */
static const char *s_mapContentKeys[] =
{
   "mapType", "layout", "flags", "seedObjects", "discoveryRadius",
   "defaultLinkColor", "defaultLinkColorSource", "defaultLinkRouting", "defaultLinkWidth", "defaultLinkStyle",
   "objectDisplayMode", "backgroundColor", "background", "backgroundLatitude", "backgroundLongitude", "backgroundZoom",
   "filter", "linkScript", "width", "height", "canvasType", "initialViewMode", "displayPriority",
   "elements", "links",
   nullptr
};

/**
 * Load network map object by URL placeholder "object-id" and check the given
 * access rights. On failure returns nullptr and writes the matching HTTP status
 * code to *httpCode (400 if the object is not a network map).
 */
static shared_ptr<NetObj> LoadNetworkMapForModify(Context *context, uint32_t requiredRights, int *httpCode)
{
   shared_ptr<NetObj> object = LoadObjectForModify(context, requiredRights, httpCode);
   if (object == nullptr)
      return shared_ptr<NetObj>();

   if (object->getObjectClass() != OBJECT_NETWORKMAP)
   {
      wchar_t message[256];
      nx_swprintf(message, 256, L"Property group map does not apply to object class %s", object->getObjectClassName());
      context->setErrorResponse(message);
      *httpCode = 400;
      return shared_ptr<NetObj>();
   }

   return object;
}

/**
 * Handler for GET /v1/objects/:object-id/map - network map content view (layout
 * settings, background, elements, and links). Requires read access.
 */
int H_ObjectNetworkMap(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> object = LoadNetworkMapForModify(context, OBJECT_ACCESS_READ, &httpCode);
   if (object == nullptr)
      return httpCode;

   json_t *full = object->toJson(false);
   json_t *view = json_object();
   for(int i = 0; s_mapContentKeys[i] != nullptr; i++)
   {
      json_t *value = json_object_get(full, s_mapContentKeys[i]);
      if (value != nullptr)
         json_object_set(view, s_mapContentKeys[i], value);
   }
   json_decref(full);

   context->setResponseData(view);
   json_decref(view);
   return 200;
}

/**
 * Handler for PUT /v1/objects/:object-id/map - replace network map content. The
 * "elements"/"links" arrays, when present, fully replace the map layout. Only map
 * content keys are applied; other object properties are left untouched.
 */
int H_ObjectNetworkMapUpdate(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> object = LoadNetworkMapForModify(context, OBJECT_ACCESS_MODIFY, &httpCode);
   if (object == nullptr)
      return httpCode;

   json_t *request = context->getRequestDocument();
   if ((request == nullptr) || !json_is_object(request))
   {
      context->setErrorResponse("Request body must be a JSON object");
      return 400;
   }

   json_t *patch = json_object();
   for(int i = 0; s_mapContentKeys[i] != nullptr; i++)
   {
      json_t *value = json_object_get(request, s_mapContentKeys[i]);
      if (value != nullptr)
         json_object_set(patch, s_mapContentKeys[i], value);
   }

   json_t *oldSnapshot = object->toJson(false);
   uint32_t rcc = object->modifyFromJSON(patch, context);
   json_decref(patch);
   if (rcc != RCC_SUCCESS)
   {
      json_decref(oldSnapshot);
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, L"H_ObjectNetworkMapUpdate: modifyFromJSON failed for object %s [%u] with RCC %u",
         object->getName(), object->getId(), rcc);
      context->setErrorResponse("Invalid network map content in request");
      return 400;
   }

   json_t *newSnapshot = object->toJson(false);
   context->writeAuditLogWithValues(AUDIT_OBJECTS, true, object->getId(), oldSnapshot, newSnapshot,
      L"Modified network map content of object %s [%u]", object->getName(), object->getId());
   json_decref(oldSnapshot);

   context->setResponseData(newSnapshot);
   json_decref(newSnapshot);
   return 200;
}
