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
** File: snmp_traps.cpp
**
**/

#include "webapi.h"

/**
 * Maximum lengths of trap mapping fields (match database column sizes)
 */
#define MAX_TRAP_OID_LEN            255
#define MAX_TRAP_DESCRIPTION_LEN    255
#define MAX_TRAP_EVENT_TAG_LEN      63

/**
 * Check optional string field in given JSON object. Field is valid if absent, null, or string not longer than maxLen
 * characters. On failure writes error response to the context and returns false.
 */
static bool CheckStringField(Context *context, json_t *json, const char *name, size_t maxLen)
{
   json_t *field = json_object_get(json, name);
   if ((field == nullptr) || json_is_null(field))
      return true;

   char message[128];
   if (!json_is_string(field))
   {
      snprintf(message, sizeof(message), "Field \"%s\" must be a string", name);
      context->setErrorResponse(message);
      return false;
   }
   // utf8_wcharlen counts the terminating null
   if ((maxLen > 0) && (utf8_wcharlen(json_string_value(field), -1) > maxLen + 1))
   {
      snprintf(message, sizeof(message), "Field \"%s\" is too long (maximum length is %u characters)", name, static_cast<unsigned int>(maxLen));
      context->setErrorResponse(message);
      return false;
   }
   return true;
}

/**
 * Check optional integer field in given JSON object. On failure writes error response to the context and returns false.
 */
static bool CheckIntegerField(Context *context, json_t *json, const char *name)
{
   json_t *field = json_object_get(json, name);
   if ((field == nullptr) || json_is_integer(field))
      return true;

   char message[128];
   snprintf(message, sizeof(message), "Field \"%s\" must be an integer", name);
   context->setErrorResponse(message);
   return false;
}

/**
 * Validate trap mapping create/update request. OID validity is checked by server core.
 * On failure writes error response to the context and returns false.
 */
static bool ValidateTrapMappingRequest(Context *context, json_t *request, bool create)
{
   if (!json_is_object(request))
   {
      context->setErrorResponse("Request body must be a JSON object");
      return false;
   }

   if (create && !json_is_string(json_object_get(request, "oid")))
   {
      context->setErrorResponse("Field \"oid\" is required and must be a string");
      return false;
   }

   if (!CheckStringField(context, request, "oid", MAX_TRAP_OID_LEN) ||
       !CheckStringField(context, request, "description", MAX_TRAP_DESCRIPTION_LEN) ||
       !CheckStringField(context, request, "eventTag", MAX_TRAP_EVENT_TAG_LEN) ||
       !CheckStringField(context, request, "transformationScript", 0) ||
       !CheckIntegerField(context, request, "eventCode"))
      return false;

   json_t *parameters = json_object_get(request, "parameters");
   if (parameters == nullptr)
      return true;
   if (!json_is_array(parameters))
   {
      context->setErrorResponse("Field \"parameters\" must be an array");
      return false;
   }

   size_t index;
   json_t *parameter;
   json_array_foreach(parameters, index, parameter)
   {
      if (!json_is_object(parameter))
      {
         context->setErrorResponse("Parameter mapping must be a JSON object");
         return false;
      }
      if (!CheckStringField(context, parameter, "oid", MAX_TRAP_OID_LEN) ||
          !CheckStringField(context, parameter, "description", MAX_TRAP_DESCRIPTION_LEN) ||
          !CheckIntegerField(context, parameter, "position") ||
          !CheckIntegerField(context, parameter, "flags"))
         return false;
      if ((json_integer_value(json_object_get(parameter, "position")) <= 0) && !json_is_string(json_object_get(parameter, "oid")))
      {
         context->setErrorResponse("Parameter mapping must have either \"oid\" or positive \"position\"");
         return false;
      }
   }
   return true;
}

/**
 * Convert trap mapping create/update RCC into HTTP response code
 */
static int TrapMappingRCCToHttpCode(Context *context, uint32_t rcc)
{
   switch(rcc)
   {
      case RCC_SUCCESS:
         return 200;
      case RCC_INVALID_TRAP_ID:
         return 404;
      case RCC_INVALID_EVENT_CODE:
         context->setErrorResponse("Invalid event code");
         return 400;
      case RCC_INVALID_ARGUMENT:
         context->setErrorResponse("Invalid trap OID or parameter OID");
         return 400;
      default:
         context->setErrorResponse("Database failure");
         return 500;
   }
}

/**
 * Handler for GET /v1/snmp-trap-mappings
 */
int H_SnmpTrapMappings(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_CONFIGURE_TRAPS))
      return 403;

   json_t *output = GetTrapMappingsAsJson();
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/snmp-trap-mappings/:trap-id
 */
int H_SnmpTrapMappingDetails(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_CONFIGURE_TRAPS))
      return 403;

   uint32_t trapId = context->getPlaceholderValueAsUInt32(L"trap-id");
   if (trapId == 0)
      return 400;

   json_t *output = GetTrapMappingAsJson(trapId);
   if (output == nullptr)
      return 404;

   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for POST /v1/snmp-trap-mappings
 */
int H_SnmpTrapMappingCreate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_CONFIGURE_TRAPS))
   {
      context->writeAuditLog(AUDIT_SYSCFG, false, 0, L"Access denied on trap configuration update");
      return 403;
   }

   json_t *request = context->getRequestDocument();
   if (!ValidateTrapMappingRequest(context, request, true))
      return 400;

   uint32_t trapId = 0;
   uint32_t rcc = CreateTrapMappingFromJson(request, &trapId);
   if (rcc != RCC_SUCCESS)
      return TrapMappingRCCToHttpCode(context, rcc);

   json_t *output = GetTrapMappingAsJson(trapId);
   context->writeAuditLogWithValues(AUDIT_SYSCFG, true, 0, nullptr, output, L"Trap mapping [%u] created", trapId);
   if (output != nullptr)
   {
      context->setResponseData(output);
      json_decref(output);
   }
   return 201;
}

/**
 * Handler for PUT /v1/snmp-trap-mappings/:trap-id
 */
int H_SnmpTrapMappingUpdate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_CONFIGURE_TRAPS))
   {
      context->writeAuditLog(AUDIT_SYSCFG, false, 0, L"Access denied on trap configuration update");
      return 403;
   }

   uint32_t trapId = context->getPlaceholderValueAsUInt32(L"trap-id");
   if (trapId == 0)
      return 400;

   json_t *request = context->getRequestDocument();
   if (!ValidateTrapMappingRequest(context, request, false))
      return 400;

   json_t *oldValue = nullptr, *newValue = nullptr;
   uint32_t rcc = ModifyTrapMappingFromJson(trapId, request, &oldValue, &newValue);
   if (rcc != RCC_SUCCESS)
      return TrapMappingRCCToHttpCode(context, rcc);

   context->writeAuditLogWithValues(AUDIT_SYSCFG, true, 0, oldValue, newValue, L"Trap mapping [%u] updated", trapId);
   json_decref(oldValue);

   context->setResponseData(newValue);
   json_decref(newValue);
   return 200;
}

/**
 * Handler for DELETE /v1/snmp-trap-mappings/:trap-id
 */
int H_SnmpTrapMappingDelete(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_CONFIGURE_TRAPS))
   {
      context->writeAuditLog(AUDIT_SYSCFG, false, 0, L"Access denied on trap configuration update");
      return 403;
   }

   uint32_t trapId = context->getPlaceholderValueAsUInt32(L"trap-id");
   if (trapId == 0)
      return 400;

   json_t *oldValue = GetTrapMappingAsJson(trapId);
   if (oldValue == nullptr)
      return 404;

   uint32_t rcc = DeleteTrapMapping(trapId);
   if (rcc == RCC_SUCCESS)
      context->writeAuditLogWithValues(AUDIT_SYSCFG, true, 0, oldValue, nullptr, L"Trap mapping [%u] deleted", trapId);
   json_decref(oldValue);

   if (rcc == RCC_SUCCESS)
      return 204;
   return TrapMappingRCCToHttpCode(context, rcc);
}
