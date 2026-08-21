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
