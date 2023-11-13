/*
** NetXMS - Network Management System
** Copyright (C) 2023 Raden Solutions
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
** File: objects.cpp
**
**/

#include "webapi.h"
#include <unordered_set>

/**
 * Handler for /v1/objects/search
 */
int H_ObjectSearch(Context *context)
{
   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("H_ObjectSearch: empty request"));
      return 400;
   }

   uint32_t parentId = json_object_get_uint32(request, "parent");
   int32_t zoneUIN = json_object_get_int32(request, "zoneUIN");

   TCHAR name[256];
   utf8_to_tchar(json_object_get_string_utf8(request, "name", ""), -1, name, 256);

   InetAddress ipAddressFilter;
   const char *ipAddressText = json_object_get_string_utf8(request, "ipAddress", nullptr);
   if (ipAddressText != nullptr)
   {
      ipAddressFilter = InetAddress::parse(ipAddressText);
      if (!ipAddressFilter.isValid())
      {
         context->setErrorResponse("Invalid IP address");
         return 400;
      }
   }

   std::unordered_set<int> classFilter;
   json_t *classes = json_object_get(request, "class");
   if (json_is_array(classes))
   {
      int i;
      json_t *c;
      json_array_foreach(classes, i, c)
      {
         int n = NetObj::getObjectClassByNameA(json_string_value(c));
         if (n == OBJECT_GENERIC)
         {
            context->setErrorResponse("Invalid object class");
            return 400;
         }
         classFilter.insert(n);
      }
   }

   if ((parentId == 0) && (zoneUIN == 0) && (name[0] == 0) && classFilter.empty() && !ipAddressFilter.isValid())
   {
      context->setErrorResponse("At least one search criteria should be set");
      return 400;
   }

   unique_ptr<SharedObjectArray<NetObj>> objects = g_idxObjectById.getObjects(
      [context, parentId, zoneUIN, name, &classFilter, ipAddressFilter] (NetObj *object) -> bool
      {
         if (object->isHidden() || object->isSystem() || object->isDeleted() || !object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ))
            return false;
         if ((zoneUIN != 0) && (object->getZoneUIN() != zoneUIN))
            return false;
         if (!classFilter.empty() && (classFilter.count(object->getObjectClass()) == 0))
            return false;
         if ((name[0] != 0) && (_tcsistr(object->getName(), name) == nullptr) && (_tcsistr(object->getAlias(), name) == nullptr))
            return false;
         if (ipAddressFilter.isValid() && !object->getPrimaryIpAddress().equals(ipAddressFilter))
            return false;
         return (parentId != 0) ? object->isParent(parentId) : true;
      });

   json_t *output = json_array();
   for(int i = 0; i < objects->size(); i++)
   {
      NetObj *object = objects->get(i);
      json_array_append_new(output, CreateObjectSummary(object));
   }

   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for /v1/objects/query
 */
int H_ObjectQuery(Context *context)
{
   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("H_ObjectQuery: empty request"));
      return 400;
   }

   TCHAR *query = json_object_get_string_t(request, "query", nullptr);
   if (query == nullptr)
   {
      context->setErrorResponse("Query not provided");
      return 400;
   }

   StringList fields(json_object_get(request, "fields"));
   StringList orderBy(json_object_get(request, "orderBy"));
   StringMap inputFields(json_object_get(request, "inputFields"));

   TCHAR errorMessage[1024];
   unique_ptr<ObjectArray<ObjectQueryResult>> objects = QueryObjects(query, context->getUserId(), errorMessage, 1024,
      json_object_get_boolean(request, "readAllFields"), &fields, &orderBy, &inputFields, json_object_get_int32(request, "limit"));
   MemFree(query);

   json_t *output = json_array();
   for(int i = 0; i < objects->size(); i++)
   {
      ObjectQueryResult *r = objects->get(i);
      json_t *e = json_object();
      json_object_set_new(e, "object", CreateObjectSummary(r->object.get()));
      json_object_set_new(e, "fields", r->values->toJson());
      json_array_append_new(output, e);
   }

   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for /v1/objects
 */
int H_Objects(Context *context)
{
   uint32_t parentId = context->getQueryParameterAsUInt32("parent");

   TCHAR filter[256];
   utf8_to_tchar(CHECK_NULL_EX_A(context->getQueryParameter("filter")), -1, filter, 256);

   unique_ptr<SharedObjectArray<NetObj>> objects = g_idxObjectById.getObjects(
      [context, parentId, filter] (NetObj *object) -> bool
      {
         if (object->isHidden() || object->isSystem() || object->isDeleted() || !object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ))
            return false;
         if ((filter[0] != 0) && (_tcsistr(object->getName(), filter) == nullptr) && (_tcsistr(object->getAlias(), filter) == nullptr))
            return false;
         return (parentId != 0) ? object->isDirectParent(parentId) : !object->hasAccessibleParents(context->getUserId());
      });

   json_t *output = json_array();
   for(int i = 0; i < objects->size(); i++)
   {
      NetObj *object = objects->get(i);
      json_array_append_new(output, CreateObjectSummary(object));
   }

   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for /v1/objects/:object-id
 */
int H_ObjectDetails(Context *context)
{
   uint32_t objectId = context->getPlaceholderValueAsUInt32(_T("object-id"));
   if (objectId == 0)
      return 400;

   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object == nullptr)
      return 404;

   if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ))
      return 403;

   json_t *output = object->toJson();
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for /v1/objects/:object-id/execute-agent-command
 */
int H_ObjectExecuteAgentCommand(Context *context)
{
   uint32_t objectId = context->getPlaceholderValueAsUInt32(_T("object-id"));
   if (objectId == 0)
      return 400;

   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object == nullptr)
      return 404;

   if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_CONTROL))
      return 403;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("H_ObjectExecuteAgentCommand: empty request"));
      return 400;
   }

   String commandLine(json_object_get_string_t(request, "command", nullptr), -1, Ownership::True);
   if (commandLine.isEmpty())
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("H_ObjectExecuteAgentCommand: missing command"));
      context->setErrorResponse("Command is missing");
      return 400;
   }

   uint32_t alarmId = json_object_get_uint32(request, "alarmId", 0);
   Alarm *alarm = (alarmId != 0) ? FindAlarmById(alarmId) : 0;
   if ((alarm != nullptr) && (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ_ALARMS) || !alarm->checkCategoryAccess(context->getUserId(), context->getSystemAccessRights())))
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("H_ObjectExecuteAgentCommand: alarm ID is provided but user has no access to that alarm"));
      context->setErrorResponse("Alarm ID is provided but user has no access to that alarm");
      delete alarm;
      return 403;
   }

   shared_ptr<AgentConnectionEx> pConn = static_cast<Node&>(*object).createAgentConnection();
   if (pConn == nullptr)
   {
      context->setErrorResponse("Cannot connect to agent");
      delete alarm;
      return 500;
   }

   StringMap inputFields(json_object_get(request, "inputFields"));

   StringList *list = SplitCommandLine(object->expandText(commandLine, alarm, nullptr, shared_ptr<DCObjectInfo>(), context->getLoginName(), nullptr, nullptr, &inputFields, nullptr));
   TCHAR actionName[MAX_PARAM_NAME];
   _tcslcpy(actionName, list->get(0), MAX_PARAM_NAME);
   list->remove(0);

   uint32_t rcc = pConn->executeCommand(actionName, *list);
   nxlog_debug_tag(DEBUG_TAG_WEBAPI, 4, _T("H_ObjectExecuteAgentCommand: execution completed, RCC=%u"), rcc);

   int responseCode;
   if (rcc ==  ERR_SUCCESS)
   {
      responseCode = 201;
      // TODO: audit
   }
   else
   {
      responseCode = 500;
      context->setAgentErrorResponse(rcc);
   }

   delete list;
   delete alarm;
   return responseCode;
}

/**
 * Handler for /v1/objects/:object-id/take-screenshot
 */
int H_TakeScreenshot(Context *context)
{
   uint32_t objectId = context->getPlaceholderValueAsUInt32(_T("object-id"));
   if (objectId == 0)
      return 400;

   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object == nullptr)
      return 404;

   if (object->getObjectClass() != OBJECT_NODE)
      return 400;

   if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_SCREENSHOT))
      return 403;

   shared_ptr<AgentConnectionEx> conn = static_cast<Node&>(*object).createAgentConnection();
   if (conn == nullptr)
   {
      context->setErrorResponse("No connection to agent");
      return 500;
   }

   TCHAR sessionName[256] = _T("");
   const char *s = context->getQueryParameter("sessionName");
   if ((s != nullptr) && (*s != 0))
   {
      utf8_to_tchar(s, -1, sessionName, 256);
   }
   else
   {
      _tcscpy(sessionName, _T("Console"));
   }

   // Screenshot transfer can take significant amount of time on slow links
   conn->setCommandTimeout(60000);

   BYTE *data = nullptr;
   size_t size;
   uint32_t rcc = conn->takeScreenshot(sessionName, &data, &size);
   if (rcc != ERR_SUCCESS)
   {
      context->setAgentErrorResponse(rcc);
      return 500;
   }

   context->setResponseData(data, size, "image/png");
   MemFree(data);

   context->writeAuditLog(AUDIT_OBJECTS, true, objectId, _T("Screenshot taken for session \"%s\""), sessionName);
   return 200;
}
