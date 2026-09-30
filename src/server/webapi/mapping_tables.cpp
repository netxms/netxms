/*
** NetXMS - Network Management System
** Copyright (C) 2023-2026 Raden Solutions
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
** File: mapping_tables.cpp
**
**/

#include "webapi.h"
#include <netxms_mt.h>

/**
 * Maximum lengths of mapping table fields (match database column sizes)
 */
#define MAX_MT_NAME_LEN          63
#define MAX_MT_KEY_LEN           63
#define MAX_MT_TEXT_LEN          4000

/**
 * Check optional string field in given JSON object. Field is valid if absent, null, or string not longer than maxLen
 * characters. If notEmpty is true, field (if present) cannot be null or empty string. On failure writes error
 * response to the context and returns false.
 */
static bool CheckStringField(Context *context, json_t *json, const char *name, size_t maxLen, bool notEmpty)
{
   json_t *field = json_object_get(json, name);
   if ((field == nullptr) || (json_is_null(field) && !notEmpty))
      return true;

   char message[128];
   if (!json_is_string(field))
   {
      snprintf(message, sizeof(message), "Field \"%s\" must be a string", name);
      context->setErrorResponse(message);
      return false;
   }
   if (notEmpty && (*json_string_value(field) == 0))
   {
      snprintf(message, sizeof(message), "Field \"%s\" cannot be empty", name);
      context->setErrorResponse(message);
      return false;
   }
   // utf8_wcharlen counts the terminating null
   if (utf8_wcharlen(json_string_value(field), -1) > maxLen + 1)
   {
      snprintf(message, sizeof(message), "Field \"%s\" is too long (maximum length is %u characters)", name, static_cast<unsigned int>(maxLen));
      context->setErrorResponse(message);
      return false;
   }
   return true;
}

/**
 * Validate mapping table create/update request. On failure writes error response to the context and returns false.
 */
static bool ValidateMappingTableRequest(Context *context, json_t *request, bool create)
{
   if (!json_is_object(request))
   {
      context->setErrorResponse("Request body must be a JSON object");
      return false;
   }

   if (create && (json_object_get(request, "name") == nullptr))
   {
      context->setErrorResponse("Field \"name\" is required");
      return false;
   }
   if (!CheckStringField(context, request, "name", MAX_MT_NAME_LEN, true) ||
       !CheckStringField(context, request, "description", MAX_MT_TEXT_LEN, false))
      return false;

   json_t *flags = json_object_get(request, "flags");
   if ((flags != nullptr) && !json_is_integer(flags))
   {
      context->setErrorResponse("Field \"flags\" must be an integer");
      return false;
   }

   json_t *elements = json_object_get(request, "elements");
   if (elements == nullptr)
      return true;
   if (!json_is_array(elements))
   {
      context->setErrorResponse("Field \"elements\" must be an array");
      return false;
   }

   size_t index;
   json_t *element;
   json_array_foreach(elements, index, element)
   {
      if (!json_is_object(element))
      {
         context->setErrorResponse("Mapping table element must be a JSON object");
         return false;
      }
      if (json_object_get(element, "key") == nullptr)
      {
         context->setErrorResponse("Field \"key\" is required for mapping table element");
         return false;
      }
      if (!CheckStringField(context, element, "key", MAX_MT_KEY_LEN, true) ||
          !CheckStringField(context, element, "value", MAX_MT_TEXT_LEN, false) ||
          !CheckStringField(context, element, "description", MAX_MT_TEXT_LEN, false))
         return false;
   }
   return true;
}

/**
 * Handler for GET /v1/mapping-tables
 */
int H_MappingTables(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_MAPPING_TBLS))
      return 403;

   json_t *output = ListMappingTablesAsJson();
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/mapping-tables/:table-id
 * Mapping table content is needed for displaying DCI values, so (same as in NXCP) no system access right is required.
 */
int H_MappingTableDetails(Context *context)
{
   uint32_t tableId = context->getPlaceholderValueAsUInt32(L"table-id");
   if (tableId == 0)
      return 400;

   json_t *output = GetMappingTableAsJson(tableId);
   if (output == nullptr)
      return 404;

   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Convert mapping table create/update RCC into HTTP response code
 */
static int MappingTableRCCToHttpCode(Context *context, uint32_t rcc)
{
   switch(rcc)
   {
      case RCC_SUCCESS:
         return 200;
      case RCC_INVALID_MAPPING_TABLE_ID:
         return 404;
      case RCC_OBJECT_ALREADY_EXISTS:
         context->setErrorResponse("Mapping table with same name or GUID already exists");
         return 409;
      default:
         context->setErrorResponse("Database failure");
         return 500;
   }
}

/**
 * Handler for POST /v1/mapping-tables
 */
int H_MappingTableCreate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_MAPPING_TBLS))
   {
      context->writeAuditLog(AUDIT_SYSCFG, false, 0, L"Access denied on creating mapping table");
      return 403;
   }

   json_t *request = context->getRequestDocument();
   if (!ValidateMappingTableRequest(context, request, true))
      return 400;

   uint32_t tableId = 0;
   uint32_t rcc = CreateMappingTableFromJson(request, &tableId, context);
   if (rcc != RCC_SUCCESS)
      return MappingTableRCCToHttpCode(context, rcc);

   json_t *output = GetMappingTableAsJson(tableId);
   if (output != nullptr)
   {
      context->setResponseData(output);
      json_decref(output);
   }
   return 201;
}

/**
 * Handler for PUT /v1/mapping-tables/:table-id
 */
int H_MappingTableUpdate(Context *context)
{
   uint32_t tableId = context->getPlaceholderValueAsUInt32(L"table-id");
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_MAPPING_TBLS))
   {
      context->writeAuditLog(AUDIT_SYSCFG, false, 0, L"Access denied on updating mapping table [%u]", tableId);
      return 403;
   }

   if (tableId == 0)
      return 400;

   json_t *request = context->getRequestDocument();
   if (!ValidateMappingTableRequest(context, request, false))
      return 400;

   uint32_t rcc = ModifyMappingTableFromJson(tableId, request, context);
   if (rcc != RCC_SUCCESS)
      return MappingTableRCCToHttpCode(context, rcc);

   json_t *output = GetMappingTableAsJson(tableId);
   if (output != nullptr)
   {
      context->setResponseData(output);
      json_decref(output);
   }
   return 200;
}

/**
 * Handler for DELETE /v1/mapping-tables/:table-id
 */
int H_MappingTableDelete(Context *context)
{
   uint32_t tableId = context->getPlaceholderValueAsUInt32(L"table-id");
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_MAPPING_TBLS))
   {
      context->writeAuditLog(AUDIT_SYSCFG, false, 0, L"Access denied on deleting mapping table [%u]", tableId);
      return 403;
   }

   if (tableId == 0)
      return 400;

   uint32_t rcc = DeleteMappingTable(tableId, context);
   if (rcc == RCC_SUCCESS)
      return 204;
   return MappingTableRCCToHttpCode(context, rcc);
}
