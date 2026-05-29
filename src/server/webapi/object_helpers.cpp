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
 * Load object addressed by URL placeholder "object-id" and check that the
 * caller has the given access rights on it. On any failure returns nullptr and
 * writes the matching HTTP status code (400 / 403 / 404) to *httpCode. A denied
 * modify request is recorded in the audit log.
 */
shared_ptr<NetObj> LoadObjectForModify(Context *context, uint32_t requiredRights, int *httpCode)
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

   if (!object->checkAccessRights(context->getUserId(), requiredRights))
   {
      if (requiredRights & OBJECT_ACCESS_MODIFY)
         context->writeAuditLog(AUDIT_OBJECTS, false, object->getId(),
            L"Access denied on modification of object %s [%u]", object->getName(), object->getId());
      *httpCode = 403;
      return shared_ptr<NetObj>();
   }

   return object;
}
