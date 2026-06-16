/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
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
** File: asset_management.cpp
**
**/

#include "webapi.h"
#include <asset_management.h>

/**
 * Handler for GET /v1/asset-management-schema
 */
int H_AssetManagementSchema(Context *context)
{
   json_t *attributes = GetAssetManagementSchemaAsJson();
   json_t *output = json_object();
   json_object_set_new(output, "attributes", attributes);
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/asset-management-schema/:attribute-name
 */
int H_AssetAttributeDetails(Context *context)
{
   const wchar_t *name = context->getPlaceholderValue(L"attribute-name");
   if ((name == nullptr) || (*name == 0))
      return 400;

   json_t *output = GetAssetAttributeAsJson(name);
   if (output == nullptr)
      return 404;

   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for POST /v1/asset-management-schema
 */
int H_AssetAttributeCreate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_AM_SCHEMA))
      return 403;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   wchar_t *name = json_object_get_string_t(request, "name", L"");
   if (*name == 0)
   {
      MemFree(name);
      context->setErrorResponse("Missing or invalid name field");
      return 400;
   }

   uint32_t rcc = CreateAssetAttributeFromJSON(request);
   if (rcc == RCC_INVALID_OBJECT_NAME)
   {
      MemFree(name);
      context->setErrorResponse("Invalid attribute name");
      return 400;
   }
   if (rcc == RCC_ATTRIBUTE_ALREADY_EXISTS)
   {
      MemFree(name);
      context->setErrorResponse("Asset attribute with this name already exists");
      return 409;
   }
   if (rcc != RCC_SUCCESS)
   {
      MemFree(name);
      context->setErrorResponse("Database failure");
      return 500;
   }

   json_t *output = GetAssetAttributeAsJson(name);
   context->writeAuditLogWithValues(AUDIT_SYSCFG, true, 0, nullptr, output, L"Asset management attribute \"%s\" created via REST API", name);
   MemFree(name);

   if (output != nullptr)
   {
      context->setResponseData(output);
      json_decref(output);
   }
   return 201;
}

/**
 * Handler for PUT /v1/asset-management-schema/:attribute-name
 */
int H_AssetAttributeUpdate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_AM_SCHEMA))
      return 403;

   const wchar_t *name = context->getPlaceholderValue(L"attribute-name");
   if ((name == nullptr) || (*name == 0))
      return 400;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   json_t *oldValue = GetAssetAttributeAsJson(name);
   if (oldValue == nullptr)
      return 404;

   uint32_t rcc = UpdateAssetAttributeFromJSON(name, request);
   if (rcc == RCC_UNKNOWN_ATTRIBUTE)
   {
      json_decref(oldValue);
      return 404;
   }
   if (rcc != RCC_SUCCESS)
   {
      json_decref(oldValue);
      context->setErrorResponse("Database failure");
      return 500;
   }

   json_t *output = GetAssetAttributeAsJson(name);
   context->writeAuditLogWithValues(AUDIT_SYSCFG, true, 0, oldValue, output, L"Asset management attribute \"%s\" updated via REST API", name);
   json_decref(oldValue);

   if (output != nullptr)
   {
      context->setResponseData(output);
      json_decref(output);
   }
   return 200;
}

/**
 * Handler for DELETE /v1/asset-management-schema/:attribute-name
 */
int H_AssetAttributeDelete(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_AM_SCHEMA))
      return 403;

   const wchar_t *name = context->getPlaceholderValue(L"attribute-name");
   if ((name == nullptr) || (*name == 0))
      return 400;

   json_t *oldValue = GetAssetAttributeAsJson(name);
   if (oldValue == nullptr)
      return 404;

   uint32_t rcc = DeleteAssetAttribute(name);
   if (rcc == RCC_UNKNOWN_ATTRIBUTE)
   {
      json_decref(oldValue);
      return 404;
   }
   if (rcc != RCC_SUCCESS)
   {
      json_decref(oldValue);
      context->setErrorResponse("Database failure");
      return 500;
   }

   context->writeAuditLogWithValues(AUDIT_SYSCFG, true, 0, oldValue, nullptr, L"Asset management attribute \"%s\" deleted via REST API", name);
   json_decref(oldValue);
   return 204;
}
