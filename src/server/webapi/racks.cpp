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

#include "webapi.h"

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
