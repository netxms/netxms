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
