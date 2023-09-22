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

/**
 * Handler for /v1/objects
 */
int H_Objects(Context *context)
{
   uint32_t parentId = context->getQueryParameterAsUInt32("parent");

   unique_ptr<SharedObjectArray<NetObj>> objects = g_idxObjectById.getObjects(
      [context, parentId] (NetObj *object) -> bool
      {
         if (object->isHidden() || object->isSystem() || object->isDeleted() || !object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ))
            return false;
         return (parentId != 0) ? object->isDirectParent(parentId) : !object->hasAccessibleParents(context->getUserId());
      });

   json_t *output = json_array();
   for(int i = 0; i < objects->size(); i++)
   {
      NetObj *object = objects->get(i);
      json_t *jsonObject = json_object();
      json_object_set_new(jsonObject, "id", json_integer(object->getId()));
      json_object_set_new(jsonObject, "guid", object->getGuid().toJson());
      json_object_set_new(jsonObject, "class", json_string(object->getObjectClassNameA()));
      json_object_set_new(jsonObject, "name", json_string_t(object->getName()));
      json_object_set_new(jsonObject, "alias", json_string_t(object->getAlias()));
      json_object_set_new(jsonObject, "category", json_integer(object->getCategoryId()));
      json_object_set_new(jsonObject, "timestamp", json_time_string(object->getTimeStamp()));
      json_object_set_new(jsonObject, "status", json_integer(object->getStatus()));
      json_array_append_new(output, jsonObject);
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

   uint32_t alarmId = static_cast<uint32_t>(json_object_get_integer(request, "alarmId", 0));
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

   StringList *list = SplitCommandLine(object->expandText(commandLine, alarm, nullptr, shared_ptr<DCObjectInfo>(), context->getUserName(), nullptr, nullptr, &inputFields, nullptr));
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
      json_t *response = json_object();
      json_object_set_new(response, "reason", json_string("Agent error"));
      json_object_set_new(response, "agentErrorCode", json_integer(rcc));
      json_object_set_new(response, "agentErrorText", json_string_t(AgentErrorCodeToText(rcc)));
      context->setResponseData(response);
      json_decref(response);
   }

   delete list;
   delete alarm;
   return responseCode;
}
