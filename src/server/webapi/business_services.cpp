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
** File: business_services.cpp
**
**/

#include "webapi.h"

/**
 * Upper bound for period boundaries (2038-01-19T03:14:07Z). Downtime and ticket timestamps are
 * stored as 32 bit values, so anything above this cannot be represented in the database.
 */
#define MAX_PERIOD_TIMESTAMP  _LL(0x7FFFFFFF)

/**
 * Handler for GET /v1/objects/:object-id/availability
 *
 * Time period defaults to epoch .. now.
 */
int H_BusinessServiceAvailability(Context *context)
{
   uint32_t objectId = context->getPlaceholderValueAsUInt32(L"object-id");
   if (objectId == 0)
      return 400;

   shared_ptr<NetObj> object = FindObjectById(objectId, OBJECT_BUSINESSSERVICE);
   if (object == nullptr)
      return 404;

   if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ))
   {
      context->writeAuditLog(AUDIT_OBJECTS, false, objectId, L"Access denied on reading business service uptime");
      return 403;
   }

   // Parsing error is reported as -1 so that it cannot be confused with epoch, which is a valid
   // input and also the default for "from". Boundaries outside of [epoch, MAX_PERIOD_TIMESTAMP]
   // are rejected as well - such timestamps cannot be stored in the database and would break
   // uptime calculation (a millisecond timestamp passed as "to" is the typical case).
   time_t from = 0;
   const char *fromText = context->getQueryParameter("from");
   if (fromText != nullptr)
   {
      from = ParseTimestamp(fromText, -1);
      if ((from < 0) || (static_cast<int64_t>(from) > MAX_PERIOD_TIMESTAMP))
      {
         context->setErrorResponse("Invalid \"from\" parameter");
         return 400;
      }
   }

   time_t to = time(nullptr);
   const char *toText = context->getQueryParameter("to");
   if (toText != nullptr)
   {
      to = ParseTimestamp(toText, -1);
      if ((to < 0) || (static_cast<int64_t>(to) > MAX_PERIOD_TIMESTAMP))
      {
         context->setErrorResponse("Invalid \"to\" parameter");
         return 400;
      }
   }

   if (to < from)
   {
      context->setErrorResponse("\"to\" must not be earlier than \"from\"");
      return 400;
   }

   double uptime = GetServiceUptime(objectId, from, to);
   if (uptime < 0)
      return 500;

   json_t *tickets = nullptr;
   if (context->getQueryParameterAsBoolean("includeTickets", false))
   {
      tickets = GetServiceTicketsAsJson(objectId, from, to);
      if (tickets == nullptr)
         return 500;
   }

   json_t *output = json_object();
   json_object_set_new(output, "uptime", json_real(uptime));
   json_object_set_new(output, "from", json_string(FormatISO8601Timestamp(from).c_str()));
   json_object_set_new(output, "to", json_string(FormatISO8601Timestamp(to).c_str()));
   if (tickets != nullptr)
      json_object_set_new(output, "tickets", tickets);
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Find business service or business service prototype addressed by request and check user's access to it.
 * Returns nullptr and sets HTTP status code on failure.
 */
static shared_ptr<BaseBusinessService> LoadBusinessService(Context *context, uint32_t requiredAccess, int *httpCode)
{
   uint32_t objectId = context->getPlaceholderValueAsUInt32(L"object-id");
   if (objectId == 0)
   {
      *httpCode = 400;
      return shared_ptr<BaseBusinessService>();
   }

   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object == nullptr)
   {
      *httpCode = 404;
      return shared_ptr<BaseBusinessService>();
   }

   if ((object->getObjectClass() != OBJECT_BUSINESSSERVICE) && (object->getObjectClass() != OBJECT_BUSINESSSERVICEPROTO))
   {
      context->setErrorResponse("Object is not a business service or business service prototype");
      *httpCode = 400;
      return shared_ptr<BaseBusinessService>();
   }

   if (!object->checkAccessRights(context->getUserId(), requiredAccess))
   {
      context->writeAuditLog(AUDIT_OBJECTS, false, objectId, (requiredAccess == OBJECT_ACCESS_READ) ?
            L"Access denied on reading business service checks of %s" : L"Access denied on modifying business service checks of %s", object->getName());
      *httpCode = 403;
      return shared_ptr<BaseBusinessService>();
   }

   return static_pointer_cast<BaseBusinessService>(object);
}

/**
 * Map RCC from business service check management methods to HTTP status code
 */
static int MapCheckRCC(Context *context, uint32_t rcc, const MutableString& errorText)
{
   switch(rcc)
   {
      case RCC_INVALID_BUSINESS_CHECK_ID:
         context->setErrorResponse("Business service check not found");
         return 404;
      case RCC_AUTO_CREATED_CHECK:
         context->setErrorResponse("Check was created automatically (from prototype or by auto-binding) and cannot be modified");
         return 409;
      case RCC_INVALID_ARGUMENT:
         context->setErrorResponse(errorText.cstr());
         return 400;
      case RCC_NXSL_COMPILATION_ERROR:
         context->setErrorResponse(StringBuffer(L"Script compilation error: ").append(errorText).cstr());
         return 400;
      default:
         context->setErrorResponse("Internal server error");
         return 500;
   }
}

/**
 * Handler for GET /v1/objects/:object-id/business-service-checks
 */
int H_BusinessServiceChecks(Context *context)
{
   int httpCode;
   shared_ptr<BaseBusinessService> service = LoadBusinessService(context, OBJECT_ACCESS_READ, &httpCode);
   if (service == nullptr)
      return httpCode;

   json_t *output = json_array();
   unique_ptr<SharedObjectArray<BusinessServiceCheck>> checks = service->getChecks();
   for (const shared_ptr<BusinessServiceCheck>& check : *checks)
      json_array_append_new(output, check->toJson());
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for POST /v1/objects/:object-id/business-service-checks
 */
int H_BusinessServiceCheckCreate(Context *context)
{
   int httpCode;
   shared_ptr<BaseBusinessService> service = LoadBusinessService(context, OBJECT_ACCESS_MODIFY, &httpCode);
   if (service == nullptr)
      return httpCode;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      context->setErrorResponse("Missing or invalid JSON body");
      return 400;
   }

   shared_ptr<BusinessServiceCheck> check;
   MutableString errorText;
   uint32_t rcc = service->createCheckFromJSON(request, &check, &errorText);
   if (rcc != RCC_SUCCESS)
      return MapCheckRCC(context, rcc, errorText);

   json_t *output = check->toJson();
   context->writeAuditLogWithValues(AUDIT_OBJECTS, true, service->getId(), nullptr, output,
         L"Business service check [%u] created in %s", check->getId(), service->getName());
   context->setResponseData(output);
   json_decref(output);
   return 201;
}

/**
 * Handler for GET /v1/objects/:object-id/business-service-checks/:check-id
 */
int H_BusinessServiceCheckDetails(Context *context)
{
   int httpCode;
   shared_ptr<BaseBusinessService> service = LoadBusinessService(context, OBJECT_ACCESS_READ, &httpCode);
   if (service == nullptr)
      return httpCode;

   shared_ptr<BusinessServiceCheck> check = service->getCheck(context->getPlaceholderValueAsUInt32(L"check-id"));
   if (check == nullptr)
   {
      context->setErrorResponse("Business service check not found");
      return 404;
   }

   json_t *output = check->toJson();
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for PATCH /v1/objects/:object-id/business-service-checks/:check-id
 */
int H_BusinessServiceCheckUpdate(Context *context)
{
   int httpCode;
   shared_ptr<BaseBusinessService> service = LoadBusinessService(context, OBJECT_ACCESS_MODIFY, &httpCode);
   if (service == nullptr)
      return httpCode;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      context->setErrorResponse("Missing or invalid JSON body");
      return 400;
   }

   uint32_t checkId = context->getPlaceholderValueAsUInt32(L"check-id");
   shared_ptr<BusinessServiceCheck> check = service->getCheck(checkId);
   if (check == nullptr)
   {
      context->setErrorResponse("Business service check not found");
      return 404;
   }

   json_t *oldValue = check->toJson();
   MutableString errorText;
   uint32_t rcc = service->modifyCheckFromJSON(checkId, request, &errorText);
   if (rcc != RCC_SUCCESS)
   {
      json_decref(oldValue);
      return MapCheckRCC(context, rcc, errorText);
   }

   json_t *output = check->toJson();
   context->writeAuditLogWithValues(AUDIT_OBJECTS, true, service->getId(), oldValue, output,
         L"Business service check [%u] modified in %s", checkId, service->getName());
   json_decref(oldValue);
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for DELETE /v1/objects/:object-id/business-service-checks/:check-id
 */
int H_BusinessServiceCheckDelete(Context *context)
{
   int httpCode;
   shared_ptr<BaseBusinessService> service = LoadBusinessService(context, OBJECT_ACCESS_MODIFY, &httpCode);
   if (service == nullptr)
      return httpCode;

   uint32_t checkId = context->getPlaceholderValueAsUInt32(L"check-id");
   shared_ptr<BusinessServiceCheck> check = service->getCheck(checkId);
   if (check == nullptr)
   {
      context->setErrorResponse("Business service check not found");
      return 404;
   }

   json_t *oldValue = check->toJson();
   uint32_t rcc = service->deleteCheck(checkId);
   if (rcc != RCC_SUCCESS)
   {
      json_decref(oldValue);
      return MapCheckRCC(context, rcc, MutableString());
   }

   context->writeAuditLogWithValues(AUDIT_OBJECTS, true, service->getId(), oldValue, nullptr,
         L"Business service check [%u] deleted from %s", checkId, service->getName());
   json_decref(oldValue);
   return 204;
}
