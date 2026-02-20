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
#include <nxtools.h>

/**
 * Callback for agent action output capture (WebAPI)
 */
static void ActionOutputCallback(ActionCallbackEvent e, const void *text, void *arg)
{
   if (e == ACE_DATA)
   {
      StringBuffer *output = static_cast<StringBuffer*>(arg);
#ifdef UNICODE
      output->appendUtf8String(static_cast<const char*>(text));
#else
      output->append(static_cast<const char*>(text));
#endif
   }
}

/**
 * NXSL environment that captures print output for WebAPI
 */
class NXSL_WebAPIEnv : public NXSL_ServerEnv
{
private:
   StringBuffer m_output;

public:
   NXSL_WebAPIEnv() : NXSL_ServerEnv() {}
   virtual void print(const TCHAR *text) override { m_output.append(text); }
   const StringBuffer& getOutput() const { return m_output; }
};

/**
 * Handler for /v1/object-tools
 */
int H_ObjectTools(Context *context)
{
   const char *typesFilter = context->getQueryParameter("types");
   json_t *tools = GetObjectToolsIntoJSON(context->getUserId(), context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_TOOLS), typesFilter);
   if (tools == nullptr)
   {
      context->setErrorResponse("Database failure");
      return 500;
   }
   context->setResponseData(tools);
   json_decref(tools);
   return 200;
}

/**
 * Handler for /v1/object-tools/:tool-id
 */
int H_ObjectToolDetails(Context *context)
{
   uint32_t toolId = context->getPlaceholderValueAsUInt32(_T("tool-id"));
   if (toolId == 0)
      return 400;

   json_t *tool = GetObjectToolIntoJSON(toolId, context->getUserId(), context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_TOOLS));
   if (tool == nullptr)
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   if (json_is_null(tool))
   {
      json_decref(tool);
      return 403;
   }

   context->setResponseData(tool);
   json_decref(tool);
   return 200;
}

/**
 * Execute agent action tool
 */
static int ExecuteAgentAction(Context *context, const shared_ptr<NetObj>& object, const TCHAR *toolData, uint32_t toolFlags, Alarm *alarm, const StringMap *inputFields, json_t *response)
{
   if (object->getObjectClass() != OBJECT_NODE)
   {
      context->setErrorResponse("Object is not a node");
      return 400;
   }

   shared_ptr<AgentConnectionEx> conn = static_cast<Node&>(*object).createAgentConnection();
   if (conn == nullptr)
   {
      context->setErrorResponse("Cannot connect to agent");
      return 500;
   }

   StringList args = SplitCommandLine(object->expandText(toolData, alarm, nullptr, shared_ptr<DCObjectInfo>(), context->getLoginName(), nullptr, nullptr, inputFields, nullptr));
   TCHAR actionName[MAX_PARAM_NAME];
   _tcslcpy(actionName, args.get(0), MAX_PARAM_NAME);
   args.remove(0);

   uint32_t rcc;
   if (toolFlags & TF_GENERATES_OUTPUT)
   {
      StringBuffer output;
      rcc = conn->executeCommand(actionName, args, true, ActionOutputCallback, &output, true);
      if (rcc == ERR_SUCCESS)
      {
         json_object_set_new(response, "type", json_string("text"));
         json_object_set_new(response, "output", json_string_t(output.cstr()));
      }
   }
   else
   {
      rcc = conn->executeCommand(actionName, args);
      if (rcc == ERR_SUCCESS)
      {
         json_object_set_new(response, "type", json_string("none"));
      }
   }

   if (rcc != ERR_SUCCESS)
   {
      context->setAgentErrorResponse(rcc);
      return 500;
   }

   context->writeAuditLog(AUDIT_OBJECTS, true, object->getId(), _T("Executed agent action \"%s\" on object %s [%u]"), actionName, object->getName(), object->getId());
   return 200;
}

/**
 * Execute server command tool
 */
static int ExecuteServerCommand(Context *context, const shared_ptr<NetObj>& object, const TCHAR *toolData, uint32_t toolFlags, Alarm *alarm, const StringMap *inputFields, json_t *response)
{
   if ((object->getObjectClass() != OBJECT_NODE) && (object->getObjectClass() != OBJECT_CONTAINER) &&
         (object->getObjectClass() != OBJECT_COLLECTOR) && (object->getObjectClass() != OBJECT_SERVICEROOT) &&
         (object->getObjectClass() != OBJECT_SUBNET) && (object->getObjectClass() != OBJECT_CLUSTER) &&
         (object->getObjectClass() != OBJECT_ZONE))
   {
      context->setErrorResponse("Incompatible object class for server command");
      return 400;
   }

   StringBuffer expandedCommand = object->expandText(toolData, alarm, nullptr, shared_ptr<DCObjectInfo>(), context->getLoginName(), nullptr, nullptr, inputFields, nullptr);

   OutputCapturingProcessExecutor executor(expandedCommand);
   if (!executor.execute())
   {
      context->setErrorResponse("Failed to execute server command");
      return 500;
   }

   if (!executor.waitForCompletion(60000))
   {
      context->setErrorResponse("Server command execution timed out");
      return 504;
   }

   if (toolFlags & TF_GENERATES_OUTPUT)
   {
      json_object_set_new(response, "type", json_string("text"));
      const char *output = executor.getOutput();
      json_object_set_new(response, "output", (output != nullptr) ? json_string(output) : json_string(""));
   }
   else
   {
      json_object_set_new(response, "type", json_string("none"));
   }

   context->writeAuditLog(AUDIT_OBJECTS, true, object->getId(), L"Executed server command %s", expandedCommand.cstr());
   return 200;
}

/**
 * Execute server script tool
 */
static int ExecuteServerScript(Context *context, const shared_ptr<NetObj>& object, const TCHAR *toolData, Alarm *alarm, const StringMap *inputFields, json_t *response)
{
   StringBuffer expandedScript = object->expandText(toolData, alarm, nullptr, shared_ptr<DCObjectInfo>(), context->getLoginName(), nullptr, nullptr, inputFields, nullptr);
   StringList *scriptArgs = ParseCommandLine(expandedScript);

   if (scriptArgs->size() == 0)
   {
      delete scriptArgs;
      context->setErrorResponse("Empty script name");
      return 400;
   }

   NXSL_WebAPIEnv *env = new NXSL_WebAPIEnv();
   NXSL_VM *vm = GetServerScriptLibrary()->createVM(scriptArgs->get(0), env);
   if (vm == nullptr)
   {
      delete scriptArgs;
      context->setErrorResponse("Script not found in library");
      return 404;
   }

   SetupServerScriptVM(vm, object, shared_ptr<DCObjectInfo>());
   vm->setSecurityContext(new NXSL_UserSecurityContext(context->getUserId()));
   vm->setGlobalVariable("$INPUT", vm->createValue(new NXSL_HashMap(vm, inputFields)));

   ObjectRefArray<NXSL_Value> sargs(scriptArgs->size() - 1, 1);
   for(int i = 1; i < scriptArgs->size(); i++)
      sargs.add(vm->createValue(scriptArgs->get(i)));

   if (!vm->run(sargs))
   {
      json_t *errorResponse = json_object();
      json_object_set_new(errorResponse, "reason", json_string("Script execution failed"));
      json_object_set_new(errorResponse, "diagnostic", vm->getErrorJson());
      context->setResponseData(errorResponse);
      json_decref(errorResponse);
      delete vm;
      delete scriptArgs;
      return 500;
   }

   json_object_set_new(response, "type", json_string("text"));
   StringBuffer output;
   if (!env->getOutput().isEmpty())
   {
      output.append(env->getOutput());
      output.append(_T("\n"));
   }
   const TCHAR *result = vm->getResult()->getValueAsCString();
   if (result != nullptr)
   {
      output.append(_T("Result: "));
      output.append(result);
   }
   json_object_set_new(response, "output", json_string_t(output.cstr()));

   context->writeAuditLog(AUDIT_OBJECTS, true, object->getId(), L"Executed server script tool \"%s\" on object %s [%u]", scriptArgs->get(0), object->getName(), object->getId());

   delete vm;
   delete scriptArgs;
   return 200;
}

/**
 * Execute table tool (SNMP table, agent table, or agent list)
 */
static int ExecuteTableTool(Context *context, const shared_ptr<NetObj>& object, uint32_t toolId, json_t *response)
{
   if (object->getObjectClass() != OBJECT_NODE)
   {
      context->setErrorResponse("Object is not a node");
      return 400;
   }

   json_t *tableResult = nullptr;
   uint32_t rcc = ExecuteTableToolToJSON(toolId, static_pointer_cast<Node>(object), &tableResult);
   if (rcc != RCC_SUCCESS)
   {
      context->setErrorResponse("Table tool execution failed");
      return 500;
   }

   json_object_set_new(response, "type", json_string("table"));
   json_object_set_new(response, "table", tableResult);
   context->writeAuditLog(AUDIT_OBJECTS, true, object->getId(), L"Executed table tool [%u] on object %s [%u]", toolId, object->getName(), object->getId());
   return 200;
}

/**
 * Execute SSH command tool
 */
static int ExecuteSSHCommand(Context *context, const shared_ptr<NetObj>& object, const TCHAR *toolData, uint32_t toolFlags, Alarm *alarm, const StringMap *inputFields, json_t *response)
{
   if (object->getObjectClass() != OBJECT_NODE)
   {
      context->setErrorResponse("Object is not a node");
      return 400;
   }
   Node& node = static_cast<Node&>(*object);

   StringBuffer command = object->expandText(toolData, alarm, nullptr, shared_ptr<DCObjectInfo>(), context->getLoginName(), nullptr, nullptr, inputFields, nullptr);

   uint32_t proxyId = node.getEffectiveSshProxy();
   shared_ptr<Node> proxy = static_pointer_cast<Node>(FindObjectById(proxyId, OBJECT_NODE));
   if (proxy == nullptr)
   {
      context->setErrorResponse("SSH proxy not available");
      return 500;
   }

   shared_ptr<AgentConnectionEx> conn = proxy->createAgentConnection();
   if (conn == nullptr)
   {
      context->setErrorResponse("Cannot connect to SSH proxy agent");
      return 500;
   }

   StringList sshArgs;
   TCHAR ipAddr[64];
   sshArgs.add(node.getIpAddress().toString(ipAddr));
   sshArgs.add(node.getSshPort());
   sshArgs.add(node.getSshLogin());
   sshArgs.add(node.getSshPassword());
   sshArgs.add(command);
   sshArgs.add(node.getSshKeyId());

   uint32_t rcc;
   if (toolFlags & TF_GENERATES_OUTPUT)
   {
      StringBuffer output;
      rcc = conn->executeCommand(_T("SSH.Command"), sshArgs, true, ActionOutputCallback, &output, true);
      if (rcc == ERR_SUCCESS)
      {
         json_object_set_new(response, "type", json_string("text"));
         json_object_set_new(response, "output", json_string_t(output.cstr()));
      }
   }
   else
   {
      rcc = conn->executeCommand(_T("SSH.Command"), sshArgs);
      if (rcc == ERR_SUCCESS)
      {
         json_object_set_new(response, "type", json_string("none"));
      }
   }

   if (rcc != ERR_SUCCESS)
   {
      context->setAgentErrorResponse(rcc);
      return 500;
   }

   context->writeAuditLog(AUDIT_OBJECTS, true, object->getId(), _T("Executed SSH command on object %s [%u]"), object->getName(), object->getId());
   return 200;
}

/**
 * Expand URL tool
 */
static int ExpandURL(Context *context, const shared_ptr<NetObj>& object, const TCHAR *toolData, Alarm *alarm, const StringMap *inputFields, json_t *response)
{
   StringBuffer expandedUrl = object->expandText(toolData, alarm, nullptr, shared_ptr<DCObjectInfo>(), context->getLoginName(), nullptr, nullptr, inputFields, nullptr);
   json_object_set_new(response, "type", json_string("url"));
   json_object_set_new(response, "url", json_string_t(expandedUrl));
   return 200;
}

/**
 * Handler for POST /v1/object-tools/:tool-id/execute
 */
int H_ObjectToolExecute(Context *context)
{
   uint32_t toolId = context->getPlaceholderValueAsUInt32(_T("tool-id"));
   if (toolId == 0)
      return 400;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      context->setErrorResponse("Missing request body");
      return 400;
   }

   uint32_t objectId = json_object_get_uint32(request, "objectId", 0);
   if (objectId == 0)
   {
      context->setErrorResponse("Missing or invalid objectId");
      return 400;
   }

   // Load tool metadata
   int toolType;
   TCHAR *toolData = nullptr;
   uint32_t toolFlags;
   uint32_t rcc = GetObjectToolType(toolId, &toolType, &toolData, &toolFlags);
   if (rcc == RCC_INVALID_TOOL_ID)
   {
      context->setErrorResponse("Object tool not found");
      return 404;
   }
   if (rcc != RCC_SUCCESS)
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   // Check tool ACL
   if (!CheckObjectToolAccess(toolId, context->getUserId()))
   {
      MemFree(toolData);
      return 403;
   }

   // Check if tool is disabled
   if (toolFlags & TF_DISABLED)
   {
      MemFree(toolData);
      context->setErrorResponse("Object tool is disabled");
      return 400;
   }

   // Reject client-only tool types
   switch(toolType)
   {
      case TOOL_TYPE_INTERNAL:
      case TOOL_TYPE_COMMAND:
      case TOOL_TYPE_FILE_DOWNLOAD:
         MemFree(toolData);
         context->setErrorResponse("This tool type can only be executed by the client");
         return 400;
   }

   // Find target object
   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object == nullptr)
   {
      MemFree(toolData);
      context->setErrorResponse("Object not found");
      return 404;
   }

   if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_CONTROL))
   {
      MemFree(toolData);
      return 403;
   }

   // Handle alarm context and input fields for macro expansion
   uint32_t alarmId = json_object_get_uint32(request, "alarmId", 0);
   Alarm *alarm = (alarmId != 0) ? FindAlarmById(alarmId) : nullptr;
   if ((alarm != nullptr) && (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ_ALARMS) || !alarm->checkCategoryAccess(context->getUserId(), context->getSystemAccessRights())))
   {
      delete alarm;
      MemFree(toolData);
      context->setErrorResponse("Access denied to alarm");
      return 403;
   }

   StringMap inputFields(json_object_get(request, "inputFields"));

   json_t *response = json_object();

   int httpCode;
   switch(toolType)
   {
      case TOOL_TYPE_ACTION:
         httpCode = ExecuteAgentAction(context, object, toolData, toolFlags, alarm, &inputFields, response);
         break;
      case TOOL_TYPE_SERVER_COMMAND:
         httpCode = ExecuteServerCommand(context, object, toolData, toolFlags, alarm, &inputFields, response);
         break;
      case TOOL_TYPE_SERVER_SCRIPT:
         httpCode = ExecuteServerScript(context, object, toolData, alarm, &inputFields, response);
         break;
      case TOOL_TYPE_SNMP_TABLE:
      case TOOL_TYPE_AGENT_TABLE:
      case TOOL_TYPE_AGENT_LIST:
         httpCode = ExecuteTableTool(context, object, toolId, response);
         break;
      case TOOL_TYPE_SSH_COMMAND:
         httpCode = ExecuteSSHCommand(context, object, toolData, toolFlags, alarm, &inputFields, response);
         break;
      case TOOL_TYPE_URL:
         httpCode = ExpandURL(context, object, toolData, alarm, &inputFields, response);
         break;
      default:
         context->setErrorResponse("Unsupported tool type");
         httpCode = 400;
         break;
   }

   delete alarm;
   MemFree(toolData);
   if (httpCode == 200)
   {
      context->setResponseData(response);
   }
   json_decref(response);
   return httpCode;
}
