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
** File: object_dc_container_config.cpp
**
** Handlers for class-specific configuration property groups of data center
** spatial model objects:
**   GET/PATCH /v1/objects/:object-id/facility       (Facility settings)
**   GET/PATCH /v1/objects/:object-id/power-domain   (PowerDomain settings)
**   GET/PATCH /v1/objects/:object-id/cooling-zone   (CoolingZone settings)
**
**/

#include "object_helpers.h"

/**
 * Load the object addressed by URL placeholder "object-id" for a configuration
 * sub-resource read, checking READ access and object class. On failure returns nullptr
 * and writes the HTTP status code (400 / 403 / 404) to *httpCode.
 */
static shared_ptr<NetObj> LoadObjectForConfigRead(Context *context, int objectClass, const wchar_t *group, int *httpCode)
{
   uint32_t objectId = context->getPlaceholderValueAsUInt32(L"object-id");
   shared_ptr<NetObj> object = (objectId != 0) ? FindObjectById(objectId) : shared_ptr<NetObj>();
   if ((object == nullptr) || object->isUnpublished() || object->isDeleted())
   {
      *httpCode = 404;
      return shared_ptr<NetObj>();
   }

   if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ))
   {
      *httpCode = 403;
      return shared_ptr<NetObj>();
   }

   if (object->getObjectClass() != objectClass)
   {
      wchar_t message[256];
      nx_swprintf(message, 256, L"Property group %s does not apply to object class %s", group, object->getObjectClassName());
      context->setErrorResponse(message);
      *httpCode = 400;
      return shared_ptr<NetObj>();
   }

   return object;
}

/**
 * Load the object addressed by URL placeholder "object-id" for a configuration
 * sub-resource PATCH, checking MODIFY access and object class. On failure returns nullptr
 * and writes the HTTP status code.
 */
static shared_ptr<NetObj> LoadObjectForConfigModify(Context *context, int objectClass, const wchar_t *group, int *httpCode)
{
   shared_ptr<NetObj> object = LoadObjectForModify(context, OBJECT_ACCESS_MODIFY, httpCode);
   if (object == nullptr)
      return object;

   if (object->getObjectClass() != objectClass)
   {
      wchar_t message[256];
      nx_swprintf(message, 256, L"Property group %s does not apply to object class %s", group, object->getObjectClassName());
      context->setErrorResponse(message);
      *httpCode = 400;
      return shared_ptr<NetObj>();
   }
   return object;
}

/**
 * Handler for GET /v1/objects/:object-id/facility.
 * Returns the facility property group (settlement lag, provider selection). Class-blind URL:
 * returns 400 if the target object is not a facility.
 */
int H_ObjectFacilityGet(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> object = LoadObjectForConfigRead(context, OBJECT_FACILITY, L"facility", &httpCode);
   if (object == nullptr)
      return httpCode;

   json_t *response = static_cast<Facility&>(*object).facilityConfigToJson();
   context->setResponseData(response);
   json_decref(response);
   return 200;
}

/**
 * Handler for PATCH /v1/objects/:object-id/facility.
 */
int H_ObjectFacilityUpdate(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> object = LoadObjectForConfigModify(context, OBJECT_FACILITY, L"facility", &httpCode);
   if (object == nullptr)
      return httpCode;

   return ApplyJsonPatch(context, object.get(), "facility", L"Modified facility configuration of object %s [%u]");
}

/**
 * Handler for GET /v1/objects/:object-id/power-domain.
 * Returns the power domain property group (domain type, feed tag, rated power). Class-blind
 * URL: returns 400 if the target object is not a power domain.
 */
int H_ObjectPowerDomainGet(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> object = LoadObjectForConfigRead(context, OBJECT_POWERDOMAIN, L"power-domain", &httpCode);
   if (object == nullptr)
      return httpCode;

   json_t *response = static_cast<PowerDomain&>(*object).powerDomainConfigToJson();
   context->setResponseData(response);
   json_decref(response);
   return 200;
}

/**
 * Handler for PATCH /v1/objects/:object-id/power-domain.
 */
int H_ObjectPowerDomainUpdate(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> object = LoadObjectForConfigModify(context, OBJECT_POWERDOMAIN, L"power-domain", &httpCode);
   if (object == nullptr)
      return httpCode;

   return ApplyJsonPatch(context, object.get(), "powerDomain", L"Modified power domain configuration of object %s [%u]");
}

/**
 * Handler for GET /v1/objects/:object-id/cooling-zone.
 * Returns the cooling zone property group (zone type, rated capacity). Class-blind URL:
 * returns 400 if the target object is not a cooling zone.
 */
int H_ObjectCoolingZoneGet(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> object = LoadObjectForConfigRead(context, OBJECT_COOLINGZONE, L"cooling-zone", &httpCode);
   if (object == nullptr)
      return httpCode;

   json_t *response = static_cast<CoolingZone&>(*object).coolingZoneConfigToJson();
   context->setResponseData(response);
   json_decref(response);
   return 200;
}

/**
 * Handler for PATCH /v1/objects/:object-id/cooling-zone.
 */
int H_ObjectCoolingZoneUpdate(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> object = LoadObjectForConfigModify(context, OBJECT_COOLINGZONE, L"cooling-zone", &httpCode);
   if (object == nullptr)
      return httpCode;

   return ApplyJsonPatch(context, object.get(), "coolingZone", L"Modified cooling zone configuration of object %s [%u]");
}
