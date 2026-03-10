/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: websvc.cpp
**
**/

#include "webapi.h"
#include <nxcore_websvc.h>

/**
 * Handler for GET /v1/web-service-definitions
 */
int H_WebServiceDefinitions(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_WEB_SERVICE_DEFINITIONS))
      return 403;

   json_t *output = GetWebServiceDefinitionsAsJson();
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/web-service-definitions/:definition-id
 */
int H_WebServiceDefinitionDetails(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_WEB_SERVICE_DEFINITIONS))
      return 403;

   uint32_t definitionId = context->getPlaceholderValueAsUInt32(L"definition-id");
   if (definitionId == 0)
      return 400;

   json_t *output = GetWebServiceDefinitionAsJson(definitionId);
   if (output == nullptr)
      return 404;

   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for POST /v1/web-service-definitions
 */
int H_WebServiceDefinitionCreate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_WEB_SERVICE_DEFINITIONS))
      return 403;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   json_t *jsonName = json_object_get(request, "name");
   if (!json_is_string(jsonName) || (json_string_value(jsonName)[0] == 0))
   {
      context->setErrorResponse("Missing or invalid name field");
      return 400;
   }

   json_t *jsonUrl = json_object_get(request, "url");
   if (!json_is_string(jsonUrl) || (json_string_value(jsonUrl)[0] == 0))
   {
      context->setErrorResponse("Missing or invalid url field");
      return 400;
   }

   // Check for duplicate name
   TCHAR *name = TStringFromUTF8String(json_string_value(jsonName));
   shared_ptr<WebServiceDefinition> existing = FindWebServiceDefinition(name);
   MemFree(name);
   if (existing != nullptr)
   {
      context->setErrorResponse("Web service definition with this name already exists");
      return 409;
   }

   auto definition = make_shared<WebServiceDefinition>(request, 0);
   uint32_t rcc = ModifyWebServiceDefinition(definition);
   if (rcc != RCC_SUCCESS)
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Web service definition \"%s\" [%u] created", definition->getName(), definition->getId());

   json_t *output = GetWebServiceDefinitionAsJson(definition->getId());
   if (output != nullptr)
   {
      context->setResponseData(output);
      json_decref(output);
   }
   return 201;
}

/**
 * Handler for PUT /v1/web-service-definitions/:definition-id
 */
int H_WebServiceDefinitionUpdate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_WEB_SERVICE_DEFINITIONS))
      return 403;

   uint32_t definitionId = context->getPlaceholderValueAsUInt32(L"definition-id");
   if (definitionId == 0)
      return 400;

   // Verify definition exists
   shared_ptr<WebServiceDefinition> current = FindWebServiceDefinition(definitionId);
   if (current == nullptr)
      return 404;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   // Check for duplicate name (excluding self)
   json_t *jsonName = json_object_get(request, "name");
   if (json_is_string(jsonName) && (json_string_value(jsonName)[0] != 0))
   {
      TCHAR *name = TStringFromUTF8String(json_string_value(jsonName));
      shared_ptr<WebServiceDefinition> existing = FindWebServiceDefinition(name);
      MemFree(name);
      if ((existing != nullptr) && (existing->getId() != definitionId))
      {
         context->setErrorResponse("Web service definition with this name already exists");
         return 409;
      }
   }

   auto definition = make_shared<WebServiceDefinition>(request, definitionId);
   uint32_t rcc = ModifyWebServiceDefinition(definition);
   if (rcc != RCC_SUCCESS)
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Web service definition \"%s\" [%u] updated", definition->getName(), definition->getId());

   json_t *output = GetWebServiceDefinitionAsJson(definitionId);
   if (output != nullptr)
   {
      context->setResponseData(output);
      json_decref(output);
   }
   return 200;
}

/**
 * Handler for DELETE /v1/web-service-definitions/:definition-id
 */
int H_WebServiceDefinitionDelete(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_WEB_SERVICE_DEFINITIONS))
      return 403;

   uint32_t definitionId = context->getPlaceholderValueAsUInt32(L"definition-id");
   if (definitionId == 0)
      return 400;

   uint32_t rcc = DeleteWebServiceDefinition(definitionId);
   if (rcc == RCC_INVALID_WEB_SERVICE_ID)
      return 404;

   if (rcc != RCC_SUCCESS)
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Web service definition [%u] deleted", definitionId);
   return 204;
}
