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
** File: grafana.cpp
**
**/

#include "webapi.h"
#include <nms_users.h>

static const char *s_severityNames[] =
{
   "Normal",
   "Warning",
   "Minor",
   "Major",
   "Critical",
   "Unknown",
   "Unmanaged",
   "Disabled",
   "Testing"
};

static const char *s_stateNames[] =
{
   "Outstanding",
   "Acknowledged",
   "Resolved",
   "Terminated"
};

/**
 * Format object name for Grafana
 */
static inline String FormatObjectNameForGrafana(const NetObj *object)
{
   StringBuffer buffer;
   SharedString alias = object->getAlias();
   if (!alias.isBlank())
   {
      buffer.append(object->getName());
      buffer.append(_T(" ("));
      buffer.append(alias.cstr());
      buffer.append(_T(")"));
   }
   else
   {
      buffer.append(object->getName());
   }
   return buffer;
}

/**
 * Handler for /v1/grafana/infinity/alarms
 */
int H_GrafanaGetAlarms(Context *context)
{
   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("H_GrafanaGetAlarms: empty request"));
      return 400;
   }

   uint32_t rootId = json_object_get_int32(request, "rootObjectId", 0);
   if (rootId != 0)
   {
      shared_ptr<NetObj> object = FindObjectById(rootId);
      if (object == nullptr)
         return 404;
      if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ_ALARMS))
         return 403;
   }

   json_t *alarmArray = json_array();
   ObjectArray<Alarm> *alarms = GetAlarms(rootId, true);
   for(int i = 0; i < alarms->size(); i++)
   {
      Alarm *alarm = alarms->get(i);
      shared_ptr<NetObj> object = FindObjectById(alarm->getSourceObject());
      if ((object != nullptr) &&
          object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ_ALARMS) &&
          alarm->checkCategoryAccess(context->getUserId(), context->getSystemAccessRights()))
      {
         json_t *json = json_object();
         json_object_set_new(json, "Id", json_integer(alarm->getAlarmId()));
         json_object_set_new(json, "Severity", json_string(s_severityNames[alarm->getCurrentSeverity()]));
         json_object_set_new(json, "State", json_string(s_stateNames[alarm->getState() & ALARM_STATE_MASK]));
         json_object_set_new(json, "Source", json_string_t(FormatObjectNameForGrafana(object.get())));
         json_object_set_new(json, "Message", json_string_t(alarm->getMessage()));
         json_object_set_new(json, "Count", json_integer(alarm->getRepeatCount()));

         if ((alarm->getState() & ALARM_STATE_MASK) == ALARM_STATE_OUTSTANDING)
         {
            json_object_set_new(json, "Ack/Resolve by", json_string(""));

         }
         else
         {
            uint32_t userId;
            switch (alarm->getState() & ALARM_STATE_MASK)
            {
               case ALARM_STATE_ACKNOWLEDGED:
                  userId = alarm->getAckByUser();
                  break;
               case ALARM_STATE_RESOLVED:
                  userId = alarm->getResolvedByUser();
                  break;
               case ALARM_STATE_TERMINATED:
                  userId = alarm->getTermByUser();
                  break;
            }
            TCHAR buffer[MAX_USER_NAME];
            ResolveUserId(userId, buffer, true);
            json_object_set_new(json, "Ack/Resolve by", json_string_t(buffer));
         }
         json_object_set_new(json, "Created", json_time_string(alarm->getCreationTime()));
         json_object_set_new(json, "Last Change", json_time_string(alarm->getLastChangeTime()));
         json_array_append_new(alarmArray, json);
      }
   }
   delete alarms;

   context->setResponseData(alarmArray);
   json_decref(alarmArray);
   return 200;
}

/**
 * Common implementation for query execution
 */
static int ExecuteSummaryTableQuery(Context *context, uint32_t tableId, uint32_t rootId)
{
   uint32_t rcc;
   Table *result = QuerySummaryTable(tableId, nullptr, rootId, context->getUserId(), &rcc);
   if (result == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("ExecuteSummaryTableQuery(id=%u): call to QuerySummaryTable failed (RCC = %u)"), tableId, rcc);
      json_t *response = json_object();
      json_object_set_new(response, "reason", json_string("Query error"));
      json_object_set_new(response, "errorCode", json_integer(rcc));
      context->setResponseData(response);
      json_decref(response);
      return (rcc == RCC_ACCESS_DENIED) ? 403: 500;
   }

   nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("ExecuteSummaryTableQuery(id=%u): %d rows in resulting table"), tableId, result->getNumRows());
   json_t *response = result->toGrafanaJson();
   context->setResponseData(response);
   json_decref(response);
   delete result;
   return 200;
}

/**
 * Handler for /v1/grafana/infinity/summary-table
 */
int H_GrafanaGetSummaryTable(Context *context)
{
   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("H_GrafanaGetSummaryTable: empty request"));
      return 400;
   }

   uint32_t rootId = json_object_get_int32(request, "rootObjectId", 0);
   uint32_t tableId = json_object_get_int32(request, "tableId", 0);
   if (tableId == 0)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("H_GrafanaGetSummaryTable: invalid summary table ID"));
      context->setErrorResponse("Invalid summary table ID");
      return 400;
   }
   if (rootId == 0)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("H_GrafanaGetSummaryTable: invalid root object ID"));
      context->setErrorResponse("Invalid root object ID");
      return 400;
   }

   return ExecuteSummaryTableQuery(context, tableId, rootId);
}

/**
 * Handler for /v1/grafana/infinity/object-query
 */
int H_GrafanaGetObjectQuery(Context *context)
{
   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("H_GrafanaGetObjectQuery: empty request"));
      return 400;
   }

   uint32_t rootId = json_object_get_int32(request, "rootObjectId", 0);
   uint32_t queryId = json_object_get_int32(request, "queryId", 0);
   StringMap inputFields(json_object_get(request, "inputFields"));

   TCHAR errorMessage[1024] = L"";
   unique_ptr<ObjectArray<ObjectQueryResult>> objects = FindAndExecuteObjectQueries(queryId, rootId,
         context->getUserId(), errorMessage, 1024, nullptr, true,
         nullptr, nullptr, &inputFields, json_object_get_int32(request, "limit"));
   if (objects == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("H_GrafanaGetObjectQuery: %s"), errorMessage);
      char *utf8Error = UTF8StringFromWideString(errorMessage);
      context->setErrorResponse(utf8Error);
      MemFree(utf8Error);
      return 400;
   }

   json_t *output = json_array();
   for(int i = 0; i < objects->size(); i++)
   {
      ObjectQueryResult *r = objects->get(i);
      json_array_append_new(output, r->values->toJson());
   }

   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for /v1/grafana/objects-status
 */
int H_GrafanaGetObjectsStatus(Context *context)
{
   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("H_GrafanaGetObjectsStatus: empty request"));
      return 400;
   }

   uint32_t rootId = json_object_get_int32(request, "rootObjectId", 0);
   if (rootId == 0)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("H_GrafanaGetObjectsStatus: invalid root object ID"));
      context->setErrorResponse("Invalid root object ID");
      return 400;
   }

   shared_ptr<NetObj> rootObject = FindObjectById(rootId);
   if (rootObject == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("H_GrafanaGetObjectsStatus: no objects found for root ID %u"), rootId);
      return 404;
   }
   if (!rootObject->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ_ALARMS))
      return 403;

   unique_ptr<SharedObjectArray<NetObj>> objects = rootObject->getAllChildren(false);
   json_t *response = json_array();
   for(int i = 0; i < objects->size(); i++)
   {
      NetObj *object = objects->get(i);
      if (object->isContainerObject() || !object->isEventSource())
         continue; // Skip containers

      if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ))
         continue;

      json_t *objJson = json_object();
      json_object_set_new(objJson, "Name",  json_string_t(FormatObjectNameForGrafana(object)));
      json_object_set_new(objJson, "Status", json_integer(object->getStatus()));

      json_array_append_new(response, objJson);
   }

   context->setResponseData(response);
   json_decref(response);
   return 200;
}

/*******************************************
 * Element lists for Grafana
 *******************************************/

/**
 * Handler for /v1/grafana/objects/:object-id/dci-list
 */
int H_GrafanaDciList(Context *context)
{
   uint32_t objectId = context->getPlaceholderValueAsUInt32(_T("object-id"));
   if (objectId == 0)
      return 400;

   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object == nullptr)
      return 404;

   if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ))
      return 403;

   if (!object->isDataCollectionTarget())
   {
      context->setErrorResponse("Object is not data collection target");
      return 400;
   }
   unique_ptr<SharedObjectArray<DCObject>> dciObjects = static_cast<DataCollectionTarget&>(*object).getAllDCObjects(context->getUserId());

   json_t *array = json_array();
   for(int i = 0; i < dciObjects->size(); i++)
   {
      json_t *json = json_object();
      auto object = dciObjects->get(i);
      json_object_set_new(json, "name", json_string_t(object->getDescription()));
      json_object_set_new(json, "id", json_integer(object->getId()));
      json_array_append_new(array, json);
   }

   json_t *result = json_object();
   json_object_set_new(result, "objects", array);
   context->setResponseData(result);
   json_decref(result);
   return 200;
}

/**
 * Handler for /v1/grafana/query-list
 */
int H_GrafanaObjectQueryList(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_OBJECT_QUERIES))
      return 403;

   json_t *result = json_object();
   json_object_set_new(result, "objects", GetObjectQueriesList());
   context->setResponseData(result);
   json_decref(result);
   return 200;
}

/**
 * Handler for /v1/grafana/summary-table-list
 */
int H_GrafanaSummaryTablesList(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_SUMMARY_TBLS))
      return 403;

   json_t *result = json_object();
   json_object_set_new(result, "objects", GetSummaryTablesList());
   context->setResponseData(result);
   json_decref(result);
   return 200;
}

/**
 * Object list filter types
 */
enum class ObjectFilterTypes
{
   DataCollectionTarget,
   Alarm,
   Query,
   Summary,
   Unknown
};

/**
 * String to ObjectFilterTypes conversion
 */
ObjectFilterTypes StringToObjectFilterType(const char *type)
{
   if (type == nullptr)
      return ObjectFilterTypes::Unknown;
   if (!strcmp(type, "dci"))
      return ObjectFilterTypes::DataCollectionTarget;
   else if (!strcmp(type, "alarm"))
      return ObjectFilterTypes::Alarm;
   else if (!strcmp(type, "query"))
      return ObjectFilterTypes::Query;
   else if (!strcmp(type, "summary"))
      return ObjectFilterTypes::Summary;
   else
      return ObjectFilterTypes::Unknown;
}

/**
 * Handler for /v1/grafana/object-list
 */
int H_GrafanaObjectList(Context *context)
{
   ObjectFilterTypes type = StringToObjectFilterType(context->getQueryParameter("filter"));

   unique_ptr<SharedObjectArray<NetObj>> objects = g_idxObjectById.getObjects(
      [context, type] (NetObj *object) -> bool
      {
         if (object->isHidden() || object->isDeleted() || !object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ))
            return false;
         if (type == ObjectFilterTypes::DataCollectionTarget && !object->isDataCollectionTarget())
            return false;
         if (type == ObjectFilterTypes::Summary && !object->isContainerObject())
            return false;
         if ((type == ObjectFilterTypes::Alarm || type == ObjectFilterTypes::Summary) && (!object->isEventSource() && !object->isContainerObject()))
            return false;

         return true;
      });

   json_t *array = json_array();
   for(int i = 0; i < objects->size(); i++)
   {
      json_t *json = json_object();
      auto object = objects->get(i);
      SharedString alias = object->getAlias();
      if(!alias.isBlank())
      {
         StringBuffer buffer(object->getName());
         buffer.append(_T(" ("));
         buffer.append(alias.cstr());
         buffer.append(_T(")"));
         json_object_set_new(json, "name", json_string_t(buffer));
      }
      else
      {
         json_object_set_new(json, "name", json_string_t(object->getName()));
      }
      json_object_set_new(json, "id", json_integer(object->getId()));
      json_array_append_new(array, json);
   }

   json_t *result = json_object();
   json_object_set_new(result, "objects", array);
   context->setResponseData(result);
   json_decref(result);
   return 200;
}

