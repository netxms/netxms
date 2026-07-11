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
** File: ai_operators.cpp
**
**/

#include "webapi.h"
#include <nxai.h>

#define DEBUG_TAG  L"webapi.ai"

/**
 * Map RCC from AI operator management functions to HTTP status code
 */
static int MapAIOperatorRCC(Context *context, uint32_t rcc)
{
   switch(rcc)
   {
      case RCC_SUCCESS:
         return 200;
      case RCC_INVALID_TASK_ID:
         context->setErrorResponse("AI operator instance not found");
         return 404;
      case RCC_INVALID_ARGUMENT:
         context->setErrorResponse("Invalid configuration");
         return 400;
      default:
         context->setErrorResponse("Internal server error");
         return 500;
   }
}

/**
 * Handler for GET /v1/ai/operators - list all AI operator instances
 */
int H_AiOperators(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_AI_OPERATORS))
      return 403;

   json_t *output = GetAIOperatorInstancesAsJson();
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for POST /v1/ai/operators - create new AI operator instance
 */
int H_AiOperatorCreate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_AI_OPERATORS))
      return 403;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      context->setErrorResponse("Request body is required");
      return 400;
   }

   uint32_t instanceId;
   uint32_t rcc = CreateAIOperatorInstance(request, context->getUserId(), &instanceId);
   if (rcc != RCC_SUCCESS)
      return MapAIOperatorRCC(context, rcc);

   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"AI operator instance [%u] created", instanceId);

   shared_ptr<AIOperatorInstance> instance = GetAIOperatorInstance(instanceId);
   if (instance == nullptr)   // deleted concurrently
      return 201;
   json_t *output = instance->toJson();
   context->setResponseData(output);
   json_decref(output);
   return 201;
}

/**
 * Handler for GET /v1/ai/operators/:operator-id - get AI operator instance details
 */
int H_AiOperatorDetails(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_AI_OPERATORS))
      return 403;

   shared_ptr<AIOperatorInstance> instance = GetAIOperatorInstance(context->getPlaceholderValueAsUInt32(L"operator-id"));
   if (instance == nullptr)
   {
      context->setErrorResponse("AI operator instance not found");
      return 404;
   }

   json_t *output = instance->toJson();
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for PATCH /v1/ai/operators/:operator-id - modify AI operator instance (partial update)
 */
int H_AiOperatorUpdate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_AI_OPERATORS))
      return 403;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      context->setErrorResponse("Request body is required");
      return 400;
   }

   uint32_t instanceId = context->getPlaceholderValueAsUInt32(L"operator-id");
   uint32_t rcc = ModifyAIOperatorInstance(instanceId, request);
   if (rcc != RCC_SUCCESS)
      return MapAIOperatorRCC(context, rcc);

   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"AI operator instance [%u] modified", instanceId);

   shared_ptr<AIOperatorInstance> instance = GetAIOperatorInstance(instanceId);
   if (instance == nullptr)   // deleted concurrently
   {
      context->setErrorResponse("AI operator instance not found");
      return 404;
   }
   json_t *output = instance->toJson();
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for DELETE /v1/ai/operators/:operator-id - delete AI operator instance
 */
int H_AiOperatorDelete(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_AI_OPERATORS))
      return 403;

   uint32_t instanceId = context->getPlaceholderValueAsUInt32(L"operator-id");
   uint32_t rcc = DeleteAIOperatorInstance(instanceId);
   if (rcc != RCC_SUCCESS)
      return MapAIOperatorRCC(context, rcc);

   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"AI operator instance [%u] deleted", instanceId);
   return 204;
}

/**
 * Handler for POST /v1/ai/operators/:operator-id/reset-memento - reset accumulated state
 */
int H_AiOperatorResetMemento(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_AI_OPERATORS))
      return 403;

   uint32_t instanceId = context->getPlaceholderValueAsUInt32(L"operator-id");
   uint32_t rcc = ResetAIOperatorInstanceMemento(instanceId);
   if (rcc != RCC_SUCCESS)
      return MapAIOperatorRCC(context, rcc);

   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"AI operator instance [%u] accumulated state reset", instanceId);
   return 204;
}

/**
 * Observation state code to symbolic name
 */
static const char *ObservationStateName(int state)
{
   switch(state)
   {
      case 0:
         return "new";
      case 1:
         return "acknowledged";
      case 2:
         return "dismissed";
      default:
         return "unknown";
   }
}

/**
 * Handler for GET /v1/ai/observations - query AI operator observations
 *
 * Query parameters:
 *   instance - limit to observations of given operator instance
 *   object   - limit to observations related to given object
 *   state    - limit to observations in given state (new, acknowledged, dismissed)
 *   since    - only observations recorded at or after given time (UNIX timestamp, ISO 8601, or relative like -1h)
 *   limit    - maximum number of records to return (default 100, most recent first)
 */
int H_AiObservations(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_VIEW_EVENT_LOG))
      return 403;

   int stateFilter = -1;
   const char *state = context->getQueryParameter("state");
   if (state != nullptr)
   {
      if (!strcmp(state, "new"))
         stateFilter = 0;
      else if (!strcmp(state, "acknowledged"))
         stateFilter = 1;
      else if (!strcmp(state, "dismissed"))
         stateFilter = 2;
      else
      {
         context->setErrorResponse("Invalid \"state\" parameter (must be \"new\", \"acknowledged\", or \"dismissed\")");
         return 400;
      }
   }

   time_t since = 0;
   const char *sinceText = context->getQueryParameter("since");
   if (sinceText != nullptr)
   {
      since = ParseTimestamp(sinceText);
      if (since == 0)
      {
         context->setErrorResponse("Invalid \"since\" parameter");
         return 400;
      }
   }

   uint32_t instanceId = context->getQueryParameterAsUInt32("instance");
   uint32_t objectId = context->getQueryParameterAsUInt32("object");
   uint32_t limit = context->getQueryParameterAsUInt32("limit", 100);

   StringBuffer query((g_dbSyntax == DB_SYNTAX_TSDB) ?
      L"SELECT id,date_part('epoch',observation_timestamp)::int,instance_id,severity,title,body,object_id,refs,state FROM ai_operator_observations WHERE 1=1" :
      L"SELECT id,observation_timestamp,instance_id,severity,title,body,object_id,refs,state FROM ai_operator_observations WHERE 1=1");
   if (instanceId != 0)
      query.append(L" AND instance_id=?");
   if (objectId != 0)
      query.append(L" AND object_id=?");
   if (stateFilter != -1)
      query.append(L" AND state=?");
   if (since != 0)
      query.append((g_dbSyntax == DB_SYNTAX_TSDB) ? L" AND observation_timestamp>=to_timestamp(?)" : L" AND observation_timestamp>=?");
   query.append(L" ORDER BY id DESC");

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, query);
   if (hStmt == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return 500;
   }

   int bindIndex = 1;
   if (instanceId != 0)
      DBBind(hStmt, bindIndex++, DB_SQLTYPE_INTEGER, instanceId);
   if (objectId != 0)
      DBBind(hStmt, bindIndex++, DB_SQLTYPE_INTEGER, objectId);
   if (stateFilter != -1)
   {
      wchar_t stateText[2] = { static_cast<wchar_t>('0' + stateFilter), 0 };
      DBBind(hStmt, bindIndex++, DB_SQLTYPE_VARCHAR, stateText, DB_BIND_TRANSIENT);
   }
   if (since != 0)
      DBBind(hStmt, bindIndex++, DB_SQLTYPE_INTEGER, static_cast<int64_t>(since));

   DB_UNBUFFERED_RESULT hResult = DBSelectPreparedUnbuffered(hStmt);
   if (hResult == nullptr)
   {
      DBFreeStatement(hStmt);
      DBConnectionPoolReleaseConnection(hdb);
      return 500;
   }

   json_t *output = json_array();
   uint32_t count = 0;
   while (DBFetch(hResult) && ((limit == 0) || (count < limit)))
   {
      json_t *observation = json_object();
      json_object_set_new(observation, "id", json_integer(DBGetFieldInt64(hResult, 0)));
      json_object_set_new(observation, "timestamp", json_time_string(DBGetFieldInt64(hResult, 1)));
      json_object_set_new(observation, "instanceId", json_integer(DBGetFieldUInt32(hResult, 2)));
      json_object_set_new(observation, "severity", json_integer(DBGetFieldInt32(hResult, 3)));

      char *title = DBGetFieldUTF8(hResult, 4, nullptr, 0);
      json_object_set_new(observation, "title", (title != nullptr) ? json_string(title) : json_null());
      MemFree(title);

      char *body = DBGetFieldUTF8(hResult, 5, nullptr, 0);
      json_object_set_new(observation, "body", (body != nullptr) ? json_string(body) : json_null());
      MemFree(body);

      json_object_set_new(observation, "objectId", json_integer(DBGetFieldUInt32(hResult, 6)));

      char *refs = DBGetFieldUTF8(hResult, 7, nullptr, 0);
      json_t *refsJson = ((refs != nullptr) && (*refs != 0)) ? json_loads(refs, 0, nullptr) : nullptr;
      json_object_set_new(observation, "references", (refsJson != nullptr) ? refsJson : json_null());
      MemFree(refs);

      wchar_t stateText[2] = L"";
      DBGetField(hResult, 8, stateText, 2);
      json_object_set_new(observation, "state", json_string(ObservationStateName(stateText[0] - '0')));

      json_array_append_new(output, observation);
      count++;
   }

   DBFreeResult(hResult);
   DBFreeStatement(hStmt);
   DBConnectionPoolReleaseConnection(hdb);

   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for PUT /v1/ai/observations/:observation-id/state - acknowledge or dismiss observation
 */
int H_AiObservationStateUpdate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_VIEW_EVENT_LOG))
      return 403;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      context->setErrorResponse("Request body is required");
      return 400;
   }

   const char *state = json_object_get_string_utf8(request, "state", nullptr);
   AIObservationState newState;
   if ((state != nullptr) && !strcmp(state, "new"))
      newState = AIObservationState::NEW;
   else if ((state != nullptr) && !strcmp(state, "acknowledged"))
      newState = AIObservationState::ACKNOWLEDGED;
   else if ((state != nullptr) && !strcmp(state, "dismissed"))
      newState = AIObservationState::DISMISSED;
   else
   {
      context->setErrorResponse("Invalid or missing \"state\" field (must be \"new\", \"acknowledged\", or \"dismissed\")");
      return 400;
   }

   const wchar_t *idText = context->getPlaceholderValue(L"observation-id");
   int64_t observationId = (idText != nullptr) ? wcstoll(idText, nullptr, 10) : 0;
   if (observationId == 0)
   {
      context->setErrorResponse("Invalid observation ID");
      return 400;
   }

   uint32_t rcc = UpdateAIOperatorObservationState(observationId, newState);
   if (rcc != RCC_SUCCESS)
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   return 204;
}
