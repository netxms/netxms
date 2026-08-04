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
** File: incidents.cpp
**
**/

#include "webapi.h"
#include <nms_incident.h>
#include <nms_users.h>

/**
 * Get incident referenced by :incident-id placeholder. Sets response code and returns nullptr
 * if incident cannot be accessed.
 */
static shared_ptr<Incident> IncidentFromRequest(Context *context, uint32_t objectAccess, int *responseCode)
{
   uint32_t incidentId = context->getPlaceholderValueAsUInt32(L"incident-id");
   if (incidentId == 0)
   {
      *responseCode = 400;
      return shared_ptr<Incident>();
   }

   shared_ptr<Incident> incident = LoadIncidentById(incidentId);
   if (incident == nullptr)
   {
      *responseCode = 404;
      return shared_ptr<Incident>();
   }

   shared_ptr<NetObj> object = FindObjectById(incident->getSourceObjectId());
   if ((object == nullptr) || !object->checkAccessRights(context->getUserId(), objectAccess))
   {
      context->writeAuditLog(AUDIT_OBJECTS, false, incident->getSourceObjectId(),
         L"Access denied on incident [%u] via REST API", incidentId);
      *responseCode = 403;
      return shared_ptr<Incident>();
   }

   return incident;
}

/**
 * Check that incident can still be modified. Closed incidents are terminal - every mutation
 * on them must be refused. Core functions cannot report this reliably, because closed incidents
 * are dropped from in-memory list on server restart and would be reported as non-existent.
 */
static inline bool CheckIncidentNotClosed(Context *context, const Incident& incident, int *responseCode)
{
   if (incident.getState() != INCIDENT_STATE_CLOSED)
      return true;
   context->setErrorResponse("Incident is closed");
   *responseCode = 409;
   return false;
}

/**
 * Convert return code of incident operation to HTTP response code
 */
static int IncidentResponseCode(Context *context, uint32_t rcc, int successCode)
{
   switch(rcc)
   {
      case RCC_SUCCESS:
         return successCode;
      case RCC_ACCESS_DENIED:
         return 403;
      case RCC_INVALID_INCIDENT_ID:
         return 404;
      case RCC_INCIDENT_CLOSED:
         context->setErrorResponse("Incident is closed");
         return 409;
      case RCC_ALARM_ALREADY_IN_INCIDENT:
         context->setErrorResponse("Alarm is already linked to another incident");
         return 409;
      case RCC_INVALID_INCIDENT_STATE:
         context->setErrorResponse("Invalid incident state");
         return 400;
      case RCC_COMMENT_REQUIRED:
         context->setErrorResponse("Comment is required for this state change");
         return 400;
      default:
         context->setErrorResponse("Database failure");
         return 500;
   }
}

/**
 * Set incident details as response data. All mutations answer with updated incident, so that
 * client can refresh from response instead of issuing additional GET.
 */
static int SetIncidentResponse(Context *context, const Incident& incident, int responseCode)
{
   json_t *output = incident.toJson(true);
   context->setResponseData(output);
   json_decref(output);
   return responseCode;
}

/**
 * Check that alarm exists and is visible to current user. Sets response code and returns false
 * otherwise. Attaching an alarm to an incident makes incident lifecycle resolve and terminate
 * that alarm, so alarms user cannot see must not be attachable. Uses same read access check as
 * alarm endpoints.
 */
static bool CheckAlarmAccess(Context *context, uint32_t alarmId, int *responseCode)
{
   Alarm *alarm = FindAlarmById(alarmId);
   if (alarm == nullptr)
   {
      context->setErrorResponse("Alarm not found");
      *responseCode = 404;
      return false;
   }

   uint32_t sourceObjectId = alarm->getSourceObject();
   shared_ptr<NetObj> object = FindObjectById(sourceObjectId);
   bool accessible = (object != nullptr) &&
       object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ_ALARMS) &&
       alarm->checkCategoryAccess(context->getUserId(), context->getSystemAccessRights());
   delete alarm;

   if (!accessible)
   {
      context->writeAuditLog(AUDIT_OBJECTS, false, sourceObjectId,
         L"Access denied on attaching alarm [%u] to incident via REST API", alarmId);
      *responseCode = 403;
      return false;
   }

   return true;
}

/**
 * Get non-empty string field from request document. Returns nullptr and sets response code
 * if field is missing or empty.
 */
static wchar_t *StringFieldFromRequest(Context *context, json_t *request, const char *name, const char *errorMessage, int *responseCode)
{
   wchar_t *value = json_object_get_string_w(request, name, nullptr);
   if ((value == nullptr) || (*value == 0))
   {
      MemFree(value);
      context->setErrorResponse(errorMessage);
      *responseCode = 400;
      return nullptr;
   }
   return value;
}

/**
 * Handler for GET /v1/incidents
 */
int H_Incidents(Context *context)
{
   uint32_t objectId = context->getQueryParameterAsUInt32("objectId");

   IntegerArray<int32_t> states;
   const char *stateFilter = context->getQueryParameter("state");
   if ((stateFilter != nullptr) && (*stateFilter != 0))
   {
      for(const char *p = stateFilter; *p != 0;)
      {
         char *eptr;
         int32_t state = strtol(p, &eptr, 10);
         if ((eptr == p) || ((*eptr != 0) && (*eptr != ',')) || ((*eptr == ',') && (eptr[1] == 0)) ||
             (state < INCIDENT_STATE_OPEN) || (state > INCIDENT_STATE_CLOSED))
         {
            context->setErrorResponse("Invalid value for \"state\" parameter");
            return 400;
         }
         states.add(state);
         p = (*eptr == ',') ? eptr + 1 : eptr;
      }
   }

   time_t from = context->getQueryParameterAsTime("from", 0);
   if ((from == 0) && (context->getQueryParameter("from") != nullptr))
   {
      context->setErrorResponse("Invalid value for \"from\" parameter");
      return 400;
   }

   time_t to = context->getQueryParameterAsTime("to", 0);
   if ((to == 0) && (context->getQueryParameter("to") != nullptr))
   {
      context->setErrorResponse("Invalid value for \"to\" parameter");
      return 400;
   }

   int32_t limit = context->getQueryParameterAsInt32("limit", 1000);
   if (limit <= 0)
      limit = 1000;
   else if (limit > 10000)
      limit = 10000;

   json_t *output = GetIncidentSummariesAsJson(context->getUserId(), objectId, &states, from, to, limit);
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for POST /v1/incidents
 */
int H_IncidentCreate(Context *context)
{
   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   uint32_t sourceObjectId = json_object_get_uint32(request, "sourceObjectId", 0);
   if (sourceObjectId == 0)
   {
      context->setErrorResponse("Missing or invalid sourceObjectId field");
      return 400;
   }

   shared_ptr<NetObj> object = FindObjectById(sourceObjectId);
   if (object == nullptr)
   {
      context->setErrorResponse("Source object not found");
      return 404;
   }
   if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_MANAGE_INCIDENTS))
   {
      context->writeAuditLog(AUDIT_OBJECTS, false, sourceObjectId, L"Access denied on creating incident via REST API");
      return 403;
   }

   int responseCode;
   wchar_t *title = StringFieldFromRequest(context, request, "title", "Incident title cannot be empty", &responseCode);
   if (title == nullptr)
      return responseCode;

   uint32_t sourceAlarmId = json_object_get_uint32(request, "sourceAlarmId", 0);
   if ((sourceAlarmId != 0) && !CheckAlarmAccess(context, sourceAlarmId, &responseCode))
   {
      MemFree(title);
      return responseCode;
   }

   wchar_t *initialComment = json_object_get_string_w(request, "initialComment", nullptr);

   uint32_t incidentId = 0;
   uint32_t rcc = CreateIncident(sourceObjectId, title, initialComment, sourceAlarmId, context->getUserId(), &incidentId);
   MemFree(title);
   MemFree(initialComment);

   if (rcc != RCC_SUCCESS)
      return IncidentResponseCode(context, rcc, 201);

   context->writeAuditLog(AUDIT_OBJECTS, true, sourceObjectId, L"Incident [%u] created via REST API", incidentId);

   wchar_t location[256];
   nx_swprintf(location, 256, L"/v1/incidents/%u", incidentId);
   context->setResponseHeader(L"Location", location);

   // Incident was created, so failure to read it back is not a creation failure - answer with
   // 201 and no body instead of an error, client can follow Location header to get details.
   shared_ptr<Incident> incident = LoadIncidentById(incidentId);
   if (incident == nullptr)
      return 201;

   return SetIncidentResponse(context, *incident, 201);
}

/**
 * Handler for GET /v1/incidents/:incident-id
 */
int H_IncidentDetails(Context *context)
{
   int responseCode;
   shared_ptr<Incident> incident = IncidentFromRequest(context, OBJECT_ACCESS_READ, &responseCode);
   if (incident == nullptr)
      return responseCode;

   return SetIncidentResponse(context, *incident, 200);
}

/**
 * Handler for PUT /v1/incidents/:incident-id
 */
int H_IncidentUpdate(Context *context)
{
   int responseCode;
   shared_ptr<Incident> incident = IncidentFromRequest(context, OBJECT_ACCESS_MANAGE_INCIDENTS, &responseCode);
   if (incident == nullptr)
      return responseCode;

   if (!CheckIncidentNotClosed(context, *incident, &responseCode))
      return responseCode;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   wchar_t *title = StringFieldFromRequest(context, request, "title", "Incident title cannot be empty", &responseCode);
   if (title == nullptr)
      return responseCode;

   uint32_t rcc = UpdateIncident(incident->getId(), title, context->getUserId());
   MemFree(title);

   if (rcc != RCC_SUCCESS)
      return IncidentResponseCode(context, rcc, 200);

   context->writeAuditLog(AUDIT_OBJECTS, true, incident->getSourceObjectId(),
      L"Incident [%u] updated via REST API", incident->getId());
   return SetIncidentResponse(context, *incident, 200);
}

/**
 * Handler for POST /v1/incidents/:incident-id/state
 */
int H_IncidentChangeState(Context *context)
{
   int responseCode;
   shared_ptr<Incident> incident = IncidentFromRequest(context, OBJECT_ACCESS_MANAGE_INCIDENTS, &responseCode);
   if (incident == nullptr)
      return responseCode;

   if (!CheckIncidentNotClosed(context, *incident, &responseCode))
      return responseCode;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   json_t *jsonState = json_object_get(request, "state");
   if (!json_is_integer(jsonState))
   {
      context->setErrorResponse("Missing or invalid state field");
      return 400;
   }

   int newState = static_cast<int>(json_integer_value(jsonState));
   wchar_t *comment = json_object_get_string_w(request, "comment", nullptr);
   uint32_t rcc = ChangeIncidentState(incident->getId(), newState, context->getUserId(), comment);
   MemFree(comment);

   if (rcc != RCC_SUCCESS)
      return IncidentResponseCode(context, rcc, 200);

   // Requested state is logged instead of current incident state, because concurrent change
   // could already move incident to a different state by the time audit record is written.
   context->writeAuditLog(AUDIT_OBJECTS, true, incident->getSourceObjectId(),
      L"Incident [%u] state changed to \"%s\" via REST API", incident->getId(), GetIncidentStateName(newState));
   return SetIncidentResponse(context, *incident, 200);
}

/**
 * Handler for POST /v1/incidents/:incident-id/assign
 */
int H_IncidentAssign(Context *context)
{
   int responseCode;
   shared_ptr<Incident> incident = IncidentFromRequest(context, OBJECT_ACCESS_MANAGE_INCIDENTS, &responseCode);
   if (incident == nullptr)
      return responseCode;

   if (!CheckIncidentNotClosed(context, *incident, &responseCode))
      return responseCode;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   json_t *jsonUserId = json_object_get(request, "userId");
   if (!json_is_integer(jsonUserId))
   {
      context->setErrorResponse("Missing or invalid userId field");
      return 400;
   }

   // User ID 0 clears assignment, any other ID must reference existing user
   uint32_t assignedUserId = static_cast<uint32_t>(json_integer_value(jsonUserId));
   wchar_t userName[MAX_USER_NAME];
   if ((assignedUserId != 0) && (ResolveUserId(assignedUserId, userName) == nullptr))
   {
      context->setErrorResponse("Invalid user ID");
      return 400;
   }

   uint32_t rcc = AssignIncident(incident->getId(), assignedUserId, context->getUserId());
   if (rcc != RCC_SUCCESS)
      return IncidentResponseCode(context, rcc, 200);

   if (assignedUserId != 0)
      context->writeAuditLog(AUDIT_OBJECTS, true, incident->getSourceObjectId(),
         L"Incident [%u] assigned to user %s [%u] via REST API", incident->getId(), userName, assignedUserId);
   else
      context->writeAuditLog(AUDIT_OBJECTS, true, incident->getSourceObjectId(),
         L"Assignment cleared on incident [%u] via REST API", incident->getId());
   return SetIncidentResponse(context, *incident, 200);
}

/**
 * Handler for POST /v1/incidents/:incident-id/comments
 */
int H_IncidentAddComment(Context *context)
{
   int responseCode;
   shared_ptr<Incident> incident = IncidentFromRequest(context, OBJECT_ACCESS_MANAGE_INCIDENTS, &responseCode);
   if (incident == nullptr)
      return responseCode;

   if (!CheckIncidentNotClosed(context, *incident, &responseCode))
      return responseCode;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   wchar_t *text = StringFieldFromRequest(context, request, "text", "Comment text cannot be empty", &responseCode);
   if (text == nullptr)
      return responseCode;

   uint32_t rcc = AddIncidentComment(incident->getId(), text, context->getUserId(), nullptr);
   MemFree(text);

   if (rcc != RCC_SUCCESS)
      return IncidentResponseCode(context, rcc, 201);

   context->writeAuditLog(AUDIT_OBJECTS, true, incident->getSourceObjectId(),
      L"Comment added to incident [%u] via REST API", incident->getId());
   return SetIncidentResponse(context, *incident, 201);
}

/**
 * Handler for POST /v1/incidents/:incident-id/alarms
 */
int H_IncidentLinkAlarm(Context *context)
{
   int responseCode;
   shared_ptr<Incident> incident = IncidentFromRequest(context, OBJECT_ACCESS_MANAGE_INCIDENTS, &responseCode);
   if (incident == nullptr)
      return responseCode;

   if (!CheckIncidentNotClosed(context, *incident, &responseCode))
      return responseCode;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   uint32_t alarmId = json_object_get_uint32(request, "alarmId", 0);
   if (alarmId == 0)
   {
      context->setErrorResponse("Missing or invalid alarmId field");
      return 400;
   }

   if (!CheckAlarmAccess(context, alarmId, &responseCode))
      return responseCode;

   uint32_t rcc = LinkAlarmToIncident(incident->getId(), alarmId, context->getUserId());
   if (rcc != RCC_SUCCESS)
      return IncidentResponseCode(context, rcc, 200);

   context->writeAuditLog(AUDIT_OBJECTS, true, incident->getSourceObjectId(),
      L"Alarm [%u] linked to incident [%u] via REST API", alarmId, incident->getId());
   return SetIncidentResponse(context, *incident, 200);
}

/**
 * Handler for DELETE /v1/incidents/:incident-id/alarms/:alarm-id
 *
 * Answers with updated incident instead of 204, so that all incident mutations behave
 * consistently and client never needs additional GET.
 */
int H_IncidentUnlinkAlarm(Context *context)
{
   int responseCode;
   shared_ptr<Incident> incident = IncidentFromRequest(context, OBJECT_ACCESS_MANAGE_INCIDENTS, &responseCode);
   if (incident == nullptr)
      return responseCode;

   if (!CheckIncidentNotClosed(context, *incident, &responseCode))
      return responseCode;

   uint32_t alarmId = context->getPlaceholderValueAsUInt32(L"alarm-id");
   if (alarmId == 0)
      return 400;

   uint32_t rcc = UnlinkAlarmFromIncident(incident->getId(), alarmId, context->getUserId());
   if (rcc != RCC_SUCCESS)
      return IncidentResponseCode(context, rcc, 200);

   context->writeAuditLog(AUDIT_OBJECTS, true, incident->getSourceObjectId(),
      L"Alarm [%u] unlinked from incident [%u] via REST API", alarmId, incident->getId());
   return SetIncidentResponse(context, *incident, 200);
}

/**
 * Handler for GET /v1/incidents/:incident-id/activity
 */
int H_IncidentActivity(Context *context)
{
   int responseCode;
   shared_ptr<Incident> incident = IncidentFromRequest(context, OBJECT_ACCESS_READ, &responseCode);
   if (incident == nullptr)
      return responseCode;

   json_t *output = GetIncidentActivityAsJson(incident->getId());
   context->setResponseData(output);
   json_decref(output);
   return 200;
}
