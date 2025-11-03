/*
** NetXMS - Network Management System
** Copyright (C) 2023-2025 Raden Solutions
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
 * Create object summary JSON document
 */
json_t *CreateObjectSummary(const NetObj& object)
{
   json_t *jsonObject = json_object();
   json_object_set_new(jsonObject, "id", json_integer(object.getId()));
   json_object_set_new(jsonObject, "guid", object.getGuid().toJson());
   json_object_set_new(jsonObject, "class", json_string(object.getObjectClassNameA()));
   json_object_set_new(jsonObject, "name", json_string_t(object.getName()));
   json_object_set_new(jsonObject, "alias", json_string_t(object.getAlias()));
   json_object_set_new(jsonObject, "category", json_integer(object.getCategoryId()));
   json_object_set_new(jsonObject, "timestamp", json_time_string(object.getTimeStamp()));
   json_object_set_new(jsonObject, "status", json_integer(object.getStatus()));
   return jsonObject;
}

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

   wchar_t name[256];
   utf8_to_wchar(json_object_get_string_utf8(request, "name", ""), -1, name, 256);

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
      size_t i;
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
         if (object->isHidden() || object->isDeleted() || !object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ))
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
      json_array_append_new(output, CreateObjectSummary(*objects->get(i)));
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

   wchar_t *query = json_object_get_string_w(request, "query", nullptr);
   if (query == nullptr)
   {
      context->setErrorResponse("Query not provided");
      return 400;
   }

   StringList fields(json_object_get(request, "fields"));
   StringList orderBy(json_object_get(request, "orderBy"));
   StringMap inputFields(json_object_get(request, "inputFields"));

   wchar_t errorMessage[1024];
   unique_ptr<ObjectArray<ObjectQueryResult>> objects = QueryObjects(query, json_object_get_uint32(request, "rootObjectId", 0),
      context->getUserId(), errorMessage, 1024, nullptr, json_object_get_boolean(request, "readAllFields"), &fields, &orderBy, &inputFields, json_object_get_int32(request, "limit"));
   MemFree(query);

   if (objects == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("H_ObjectQuery: query failed (%s)"), errorMessage);
      context->setErrorResponse(errorMessage);
      return 400;
   }

   json_t *output = json_array();
   for(int i = 0; i < objects->size(); i++)
   {
      ObjectQueryResult *r = objects->get(i);
      json_t *e = json_object();
      json_object_set_new(e, "object", CreateObjectSummary(*r->object));
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
         if (object->isHidden() || object->isDeleted() || !object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ))
            return false;
         if ((filter[0] != 0) && (_tcsistr(object->getName(), filter) == nullptr) && (_tcsistr(object->getAlias(), filter) == nullptr))
            return false;
         return (parentId != 0) ? object->isDirectParent(parentId) : !object->hasAccessibleParents(context->getUserId());
      });

   json_t *output = json_array();
   for(int i = 0; i < objects->size(); i++)
   {
      json_array_append_new(output, CreateObjectSummary(*objects->get(i)));
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
 * Check if an object has any descendants matching the class filter
 */
static bool HasDescendantsMatchingClassFilter(const shared_ptr<NetObj>& object, uint32_t userId, const std::unordered_set<int> &classFilter, std::unordered_set<uint32_t> &visited, int maxDepth, int currentDepth)
{
   uint32_t objectId = object->getId();
   
   // Prevent infinite recursion and respect maximum depth
   if (visited.count(objectId) > 0 || currentDepth >= maxDepth)
      return false;

   visited.insert(objectId);

   unique_ptr<SharedObjectArray<NetObj>> children = object->getChildren();
   bool hasMatchingDescendants = false;
   for(int i = 0; i < children->size() && !hasMatchingDescendants; i++)
   {
      NetObj *child = children->get(i);

      if (child->isHidden() || child->isDeleted() || !child->checkAccessRights(userId, OBJECT_ACCESS_READ))
         continue;

      // Check if this child matches the class filter
      if (classFilter.count(child->getObjectClass()) > 0)
      {
         hasMatchingDescendants = true;
      }
      else
      {
         // Check descendants recursively
         hasMatchingDescendants = HasDescendantsMatchingClassFilter(children->getShared(i), userId, classFilter, visited, maxDepth, currentDepth + 1);
      }
   }

   visited.erase(objectId);
   return hasMatchingDescendants;
}

/**
 * Recursively build nested object tree structure
 */
static json_t *BuildNestedObjectTree(const shared_ptr<NetObj>& object, uint32_t userId, const std::unordered_set<int> &classFilter, std::unordered_set<uint32_t> &visited, int maxDepth, int currentDepth)
{
   uint32_t objectId = object->getId();
   
   if (visited.count(objectId) > 0 || currentDepth >= maxDepth)
      return json_array();

   visited.insert(objectId);

   unique_ptr<SharedObjectArray<NetObj>> children = object->getChildren();
   json_t *output = json_array();
   for(int i = 0; i < children->size(); i++)
   {
      NetObj *child = children->get(i);

      if (child->isHidden() || child->isDeleted() || !child->checkAccessRights(userId, OBJECT_ACCESS_READ))
         continue;

      // Apply class filter if specified
      if (!classFilter.empty())
      {
         if (classFilter.count(child->getObjectClass()) == 0)
         {
            // Check if this child has descendants matching the class filter
            std::unordered_set<uint32_t> tempVisited;
            if (!HasDescendantsMatchingClassFilter(children->getShared(i), userId, classFilter, tempVisited, maxDepth, currentDepth + 1))
               continue;
         }
      }

      json_t *childObject = CreateObjectSummary(*child);

      json_t *nestedChildren = BuildNestedObjectTree(children->getShared(i), userId, classFilter, visited, maxDepth, currentDepth + 1);
      if (json_array_size(nestedChildren) > 0)
      {
         json_object_set_new(childObject, "children", nestedChildren);
      }
      else
      {
         json_decref(nestedChildren);
      }

      json_array_append_new(output, childObject);
   }

   visited.erase(objectId);
   return output;
}

/**
 * Handler for /v1/objects/:object-id/sub-tree
 */
int H_ObjectSubTree(Context *context)
{
   uint32_t objectId = context->getPlaceholderValueAsUInt32(_T("object-id"));
   if (objectId == 0)
      return 400;

   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object == nullptr)
      return 404;

   if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ))
      return 403;

   std::unordered_set<int> classFilter;
   const char *classFilterParam = context->getQueryParameter("class");
   if (classFilterParam != nullptr)
   {
      String(classFilterParam, "UTF8").split(L",", true,
         [&classFilter] (const String& className) ->void
         {
            int n = NetObj::getObjectClassByName(className);
            if (n != OBJECT_GENERIC)
               classFilter.insert(n);
         });
   }

   // Get maximum depth parameter (default 10)
   int maxDepth = context->getQueryParameterAsInt32("maxDepth", 10);
   if (maxDepth > 100)  // Cap at 100 levels to prevent abuse
      maxDepth = 100;

   // Always use recursive implementation
   std::unordered_set<uint32_t> visited;
   json_t *output = BuildNestedObjectTree(object, context->getUserId(), classFilter, visited, maxDepth, 0);

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

   String commandLine(json_object_get_string_utf8(request, "command", nullptr), "utf8");
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

   StringList args = SplitCommandLine(object->expandText(commandLine, alarm, nullptr, shared_ptr<DCObjectInfo>(), context->getLoginName(), nullptr, nullptr, &inputFields, nullptr));
   wchar_t actionName[MAX_PARAM_NAME];
   wcslcpy(actionName, args.get(0), MAX_PARAM_NAME);
   args.remove(0);

   uint32_t rcc = pConn->executeCommand(actionName, args);
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

   delete alarm;
   return responseCode;
}

/**
 * Handler for /v1/objects/:object-id/execute-script
 */
int H_ObjectExecuteScript(Context *context)
{
   uint32_t objectId = context->getPlaceholderValueAsUInt32(_T("object-id"));
   if (objectId == 0)
      return 400;

   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object == nullptr)
      return 404;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("H_ObjectExecuteScript: empty request"));
      return 400;
   }

   unique_cstring_ptr script(json_object_get_string_t(request, "script", nullptr));
   if (script == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("H_ObjectExecuteScript: missing script source code"));
      return 400;
   }

   if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_MODIFY) &&
       !(!ConfigReadBoolean(_T("Objects.ScriptExecution.RequireWriteAccess"), true) && object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ)))
   {
      context->writeAuditLogWithValues(AUDIT_OBJECTS, false, object->getId(), nullptr, script.get(), 'T', _T("Access denied on ad-hoc script execution for object %s [%u]"), object->getName(), object->getId());
      return 403;
   }

   NXSL_CompilationDiagnostic diag;
   NXSL_VM *vm = NXSLCompileAndCreateVM(script.get(), new NXSL_ServerEnv(), &diag);
   if (vm == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("H_ObjectExecuteScript: script compilation error (%s)"), diag.errorText.cstr());
      json_t *response = json_object();
      json_object_set_new(response, "reason", json_string("Script compilation failed"));
      json_object_set_new(response, "diagnostic", diag.toJson());
      context->setResponseData(response);
      json_decref(response);
      return 400;
   }

   SetupServerScriptVM(vm, object, shared_ptr<DCObjectInfo>());
   context->writeAuditLogWithValues(AUDIT_OBJECTS, true, object->getId(), nullptr, script.get(), 'T', _T("Executed ad-hoc script for object %s [%u]"), object->getName(), object->getId());

   ObjectRefArray<NXSL_Value> sargs(0, 8);
   json_t *parameters = json_object_get(request, "parameters");
   if (json_is_array(parameters))
   {
      size_t i;
      json_t *e;
      json_array_foreach(parameters, i, e)
      {
         if (json_is_string(e))
            sargs.add(vm->createValue(json_string_value(e)));
         else if (json_is_integer(e))
            sargs.add(vm->createValue(static_cast<int64_t>(json_integer_value(e))));
         else if (json_is_number(e))
            sargs.add(vm->createValue(json_number_value(e)));
         else
            sargs.add(vm->createValue());
      }
   }

   int responseCode;
   if (vm->run(sargs))
   {
      responseCode = 200;
      json_t *response = json_object();

      if (json_object_get_boolean(request, "resultAsMap", false))
      {
         NXSL_Value *result = vm->getResult();
         if (result->isHashMap())
         {
            json_object_set_new(response, "result", vm->getResult()->toJson());
         }
         else if (result->isArray())
         {
            json_t *map = json_object();
            NXSL_Array *a = result->getValueAsArray();
            for(int i = 0; i < a->size(); i++)
            {
               NXSL_Value *e = a->getByPosition(i);
               char key[16];
               snprintf(key, 16, "element%d", i + 1);
               if (e->isHashMap())
               {
                  json_object_set_new(map, key, e->toJson());
               }
               else
               {
                  json_object_set_new(map, key, json_string_t(e->getValueAsCString()));
               }
            }
            json_object_set_new(response, "result", map);
         }
         else
         {
            json_t *map = json_object();
            json_object_set_new(map, "element1", json_string_t(result->getValueAsCString()));
            json_object_set_new(response, "result", map);
         }
      }
      else
      {
         json_object_set_new(response, "result", vm->getResult()->toJson());
      }

      context->setResponseData(response);
      json_decref(response);
   }
   else
   {
      responseCode = 500;
      json_t *response = json_object();
      json_object_set_new(response, "reason", json_string("Script execution failed"));
      json_object_set_new(response, "diagnostic", vm->getErrorJson());
      context->setResponseData(response);
      json_decref(response);
   }

   delete vm;
   return responseCode;
}

/**
 * Handler for /v1/objects/:object-id/expand-text
 */
int H_ObjectExpandText(Context *context)
{
   uint32_t objectId = context->getPlaceholderValueAsUInt32(_T("object-id"));
   if (objectId == 0)
      return 400;

   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object == nullptr)
      return 404;

   if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ))
      return 403;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("H_ObjectExpandText: empty request"));
      return 400;
   }

   TCHAR *text = json_object_get_string_t(request, "text", nullptr);
   if (text == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("H_ObjectExpandText: missing text parameter"));
      context->setErrorResponse("Text parameter is missing");
      return 400;
   }

   uint32_t alarmId = json_object_get_uint32(request, "alarmId", 0);
   Alarm *alarm = (alarmId != 0) ? FindAlarmById(alarmId) : nullptr;
   if ((alarm != nullptr) && (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ_ALARMS) || !alarm->checkCategoryAccess(context->getUserId(), context->getSystemAccessRights())))
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("H_ObjectExpandText: alarm ID is provided but user has no access to that alarm"));
      context->setErrorResponse("Alarm ID is provided but user has no access to that alarm");
      delete alarm;
      MemFree(text);
      return 403;
   }

   StringMap inputFields(json_object_get(request, "inputFields"));

   StringBuffer expandedText = object->expandText(text, alarm, nullptr, shared_ptr<DCObjectInfo>(), context->getLoginName(), nullptr, nullptr, &inputFields, nullptr);

   json_t *response = json_object();
   json_object_set_new(response, "expandedText", json_string_t(expandedText));
   context->setResponseData(response);
   json_decref(response);

   delete alarm;
   MemFree(text);
   return 200;
}

/**
 * Handler for /v1/objects/:object-id/set-maintenance
 */
int H_ObjectSetMaintenance(Context *context)
{
   uint32_t objectId = context->getPlaceholderValueAsUInt32(_T("object-id"));
   if (objectId == 0)
      return 400;

   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object == nullptr)
      return 404;

   if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_MODIFY)) //TODO: check if there should be control maintenance?
      return 403;

   if (!object->isMaintenanceApplicable())
   {
      context->setErrorResponse("Incompatible operation");
      return 400;
   }

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("H_ObjectSetMaintenance: empty request"));
      return 400;
   }

   bool maintenance = json_object_get_boolean(request, "maintenance", false);
   if (maintenance)
   {
      TCHAR *comments = json_object_get_string_t(request, "comments", _T(""));
      object->enterMaintenanceMode(context->getUserId(), comments);
      MemFree(comments);
   }
   else
   {
      object->leaveMaintenanceMode(context->getUserId());
   }
   context->writeAuditLog(AUDIT_OBJECTS, true, object->getId(), _T("Object %s %s to maintenance state"), object->getName(), maintenance ? _T("entered") : _T("left"));
   return 204;
}

/**
 * Handler for /v1/objects/:object-id/set-managed
 */
int H_ObjectSetManaged(Context *context)
{
   uint32_t objectId = context->getPlaceholderValueAsUInt32(_T("object-id"));
   if (objectId == 0)
      return 400;

   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object == nullptr)
      return 404;

   if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_MODIFY))
      return 403;

   if ((object->getObjectClass() == OBJECT_TEMPLATE) ||
       (object->getObjectClass() == OBJECT_TEMPLATEGROUP) ||
       (object->getObjectClass() == OBJECT_TEMPLATEROOT))
   {
      context->setErrorResponse("Incompatible operation");
      return 400;
   }

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("H_ObjectSetManaged: empty request"));
      return 400;
   }

   bool managed = json_object_get_boolean(request, "managed", false);
   object->setMgmtStatus(managed);
   context->writeAuditLog(AUDIT_OBJECTS, true, object->getId(), _T("Object %s set to %s state"), object->getName(), managed ? _T("managed") : _T("unmanaged"));
   return 204;
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
