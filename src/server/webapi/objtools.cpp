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
** File: objects.cpp
**
**/

#include "webapi.h"
#include <nxtools.h>

#define DEBUG_TAG _T("webapi.objtools")

/**
 * Token validity period in seconds for tool output WebSocket sessions
 */
static const int TOOL_OUTPUT_TOKEN_VALIDITY = 30;

/**
 * Tool output WebSocket session. Shared between tool execution thread and WebSocket reader thread.
 * Tool is executed only after client connects, so output is never buffered.
 */
class ToolOutputWebSocketSession
{
private:
   uuid m_token;
   MHD_UpgradeResponseHandle *m_responseHandle;
   SOCKET m_socket;
   bool m_closeFrameSent;
   bool m_disconnected;
   Mutex m_socketMutex;
   Condition m_connected;

   void sendMessage(json_t *msg);
   void finish(json_t *msg);

public:
   ToolOutputWebSocketSession(const uuid& token);

   bool waitForConnection();
   void connect(MHD_UpgradeResponseHandle *responseHandle, SOCKET s);
   void readFrames();

   void sendOutput(const wchar_t *text);
   void sendOutputUtf8(const char *text, size_t length);
   void sendResult(const wchar_t *result);
   void sendCompleted();
   void sendError(const char *message);
};

/**
 * Tool output sessions waiting for WebSocket connection, indexed by connection token
 */
static SharedHashMap<uuid, ToolOutputWebSocketSession> s_pendingToolOutputSessions;
static Mutex s_pendingToolOutputSessionsLock(MutexType::FAST);

/**
 * Session constructor
 */
ToolOutputWebSocketSession::ToolOutputWebSocketSession(const uuid& token) : m_token(token), m_socketMutex(MutexType::FAST), m_connected(true)
{
   m_responseHandle = nullptr;
   m_socket = INVALID_SOCKET;
   m_closeFrameSent = false;
   m_disconnected = false;
}

/**
 * Wait for WebSocket client connection (called from execution thread). Returns false if client
 * did not connect within token validity period - in that case token is invalidated and tool should not be executed.
 */
bool ToolOutputWebSocketSession::waitForConnection()
{
   if (m_connected.wait(TOOL_OUTPUT_TOKEN_VALIDITY * 1000))
      return true;

   // Token may be consumed by connection handler right at timeout, in that case connection will be established shortly
   s_pendingToolOutputSessionsLock.lock();
   bool expired = s_pendingToolOutputSessions.contains(m_token);
   if (expired)
      s_pendingToolOutputSessions.remove(m_token);
   s_pendingToolOutputSessionsLock.unlock();

   if (expired)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Tool execution cancelled: client did not connect within %d seconds", TOOL_OUTPUT_TOKEN_VALIDITY);
      return false;
   }

   m_connected.wait(INFINITE);
   return true;
}

/**
 * Attach WebSocket connection to session
 */
void ToolOutputWebSocketSession::connect(MHD_UpgradeResponseHandle *responseHandle, SOCKET s)
{
   m_socketMutex.lock();
   m_responseHandle = responseHandle;
   m_socket = s;
   m_socketMutex.unlock();
   m_connected.set();
   nxlog_debug_tag(DEBUG_TAG, 5, L"Tool output WebSocket session started");
}

/**
 * Send JSON message to WebSocket client
 */
void ToolOutputWebSocketSession::sendMessage(json_t *msg)
{
   char *encoded = json_dumps(msg, 0);
   m_socketMutex.lock();
   if ((m_socket != INVALID_SOCKET) && !m_closeFrameSent && !m_disconnected)
      SendWebsocketFrame(m_socket, encoded, strlen(encoded));
   m_socketMutex.unlock();
   MemFree(encoded);
}

/**
 * Send final message and close frame, then shut down socket so that reader thread will finish
 */
void ToolOutputWebSocketSession::finish(json_t *msg)
{
   sendMessage(msg);
   m_socketMutex.lock();
   if ((m_socket != INVALID_SOCKET) && !m_closeFrameSent && !m_disconnected)
   {
      SendWebsocketCloseFrame(m_socket, WS_CLOSE_NORMAL);
      shutdown(m_socket, SHUT_RDWR);
   }
   m_closeFrameSent = true;
   m_socketMutex.unlock();
}

/**
 * Send output chunk to WebSocket client
 */
void ToolOutputWebSocketSession::sendOutput(const wchar_t *text)
{
   json_t *msg = json_object();
   json_object_set_new(msg, "type", json_string("output"));
   json_object_set_new(msg, "data", json_string_w(text));
   sendMessage(msg);
   json_decref(msg);
}

/**
 * Send output chunk from UTF-8 source
 */
void ToolOutputWebSocketSession::sendOutputUtf8(const char *text, size_t length)
{
   json_t *msg = json_object();
   json_object_set_new(msg, "type", json_string("output"));
   json_object_set_new(msg, "data", json_stringn(text, length));
   sendMessage(msg);
   json_decref(msg);
}

/**
 * Send script result
 */
void ToolOutputWebSocketSession::sendResult(const wchar_t *result)
{
   json_t *msg = json_object();
   json_object_set_new(msg, "type", json_string("result"));
   json_object_set_new(msg, "data", json_string_w(result));
   sendMessage(msg);
   json_decref(msg);
}

/**
 * Send completion message and close connection
 */
void ToolOutputWebSocketSession::sendCompleted()
{
   json_t *msg = json_object();
   json_object_set_new(msg, "type", json_string("completed"));
   finish(msg);
   json_decref(msg);
}

/**
 * Send error message and close connection
 */
void ToolOutputWebSocketSession::sendError(const char *message)
{
   json_t *msg = json_object();
   json_object_set_new(msg, "type", json_string("error"));
   json_object_set_new(msg, "message", json_string(message));
   finish(msg);
   json_decref(msg);
}

/**
 * Read frames from WebSocket client until connection is closed by either side
 */
void ToolOutputWebSocketSession::readFrames()
{
   while(true)
   {
      ByteStream buffer;
      BYTE frameType;
      if (!ReadWebsocketFrame(m_socket, &buffer, &frameType))
      {
         nxlog_debug_tag(DEBUG_TAG, 5, L"Tool output WebSocket: read error or connection closed");
         break;
      }

      if (frameType == 0x08)  // Close frame
      {
         nxlog_debug_tag(DEBUG_TAG, 5, L"Tool output WebSocket: received close frame");
         break;
      }
      else if (frameType == 0x09)  // Ping frame
      {
         m_socketMutex.lock();
         if (!m_closeFrameSent)
         {
            BYTE pong[2] = { 0x8A, 0x00 };  // FIN + pong opcode, no payload
            SendEx(m_socket, pong, 2, 0, nullptr);
         }
         m_socketMutex.unlock();
      }
      // Ignore other frame types
   }

   m_socketMutex.lock();
   if (!m_closeFrameSent)
      SendWebsocketCloseFrame(m_socket, WS_CLOSE_NORMAL);
   m_disconnected = true;
   m_socketMutex.unlock();

   MHD_upgrade_action(m_responseHandle, MHD_UPGRADE_ACTION_CLOSE);
   nxlog_debug_tag(DEBUG_TAG, 5, L"Tool output WebSocket session closed");
}

/**
 * Callback for agent action output capture (WebAPI) - synchronous mode
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
 * Callback for agent action output capture (WebAPI) - streaming mode
 */
static void StreamingActionOutputCallback(ActionCallbackEvent e, const void *text, void *arg)
{
   ToolOutputWebSocketSession *session = static_cast<ToolOutputWebSocketSession*>(arg);
   if (e == ACE_DATA)
   {
      const char *utf8Text = static_cast<const char*>(text);
      session->sendOutputUtf8(utf8Text, strlen(utf8Text));
   }
}

/**
 * Process executor that streams output to a WebSocket session
 */
class WebAPIStreamingProcessExecutor : public ProcessExecutor
{
private:
   shared_ptr<ToolOutputWebSocketSession> m_session;

protected:
   virtual void onOutput(const char *text, size_t length) override
   {
      m_session->sendOutputUtf8(text, length);
   }

   virtual void endOfOutput() override
   {
      m_session->sendCompleted();
   }

public:
   WebAPIStreamingProcessExecutor(const TCHAR *command, const shared_ptr<ToolOutputWebSocketSession>& session)
      : ProcessExecutor(command, true), m_session(session)
   {
      m_sendOutput = true;
      m_replaceNullCharacters = true;
   }
};

/**
 * NXSL environment that captures print output for WebAPI (synchronous mode)
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
 * NXSL environment that streams print output to WebSocket (streaming mode)
 */
class NXSL_WebAPIStreamingEnv : public NXSL_ServerEnv
{
private:
   shared_ptr<ToolOutputWebSocketSession> m_session;

public:
   NXSL_WebAPIStreamingEnv(const shared_ptr<ToolOutputWebSocketSession>& session) : NXSL_ServerEnv(), m_session(session) {}
   virtual void print(const TCHAR *text) override { m_session->sendOutput(text); }
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
 * Handler for /v1/objects/:object-id/object-tools
 */
int H_ObjectToolsForObject(Context *context)
{
   uint32_t objectId = context->getPlaceholderValueAsUInt32(_T("object-id"));
   if (objectId == 0)
      return 400;

   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object == nullptr)
      return 404;

   if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ))
      return 403;

   const char *typesFilter = context->getQueryParameter("types");
   json_t *tools = GetObjectToolsIntoJSON(context->getUserId(),
      context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_TOOLS), typesFilter, objectId);
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
 * Translate RCC from object-tool save into HTTP response code, populating the error body when
 * applicable. Returns the chosen HTTP status code.
 */
static int SetObjectToolSaveErrorResponse(Context *context, uint32_t rcc)
{
   switch(rcc)
   {
      case RCC_INVALID_OBJECT_NAME:
         context->setErrorResponse("Missing or invalid tool name");
         return 400;
      case RCC_INVALID_REQUEST:
         context->setErrorResponse("Missing or invalid tool type");
         return 400;
      case RCC_INVALID_TOOL_ID:
         return 404;
      default:
         context->setErrorResponse("Database failure");
         return 500;
   }
}

/**
 * Handler for POST /v1/object-tools
 */
int H_ObjectToolCreate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_TOOLS))
      return 403;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      context->setErrorResponse("Missing request body");
      return 400;
   }

   uint32_t toolId = 0;
   uint32_t rcc = CreateObjectToolFromJson(request, &toolId);
   if (rcc != RCC_SUCCESS)
      return SetObjectToolSaveErrorResponse(context, rcc);

   json_t *tool = GetObjectToolIntoJSON(toolId, context->getUserId(), true);
   if (tool == nullptr)
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Object tool [%u] created", toolId);

   context->setResponseData(tool);
   json_decref(tool);
   return 201;
}

/**
 * Handler for PUT /v1/object-tools/:tool-id
 */
int H_ObjectToolUpdate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_TOOLS))
      return 403;

   uint32_t toolId = context->getPlaceholderValueAsUInt32(L"tool-id");
   if (toolId == 0)
      return 400;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      context->setErrorResponse("Missing request body");
      return 400;
   }

   uint32_t rcc = UpdateObjectToolFromJson(toolId, request);
   if (rcc != RCC_SUCCESS)
      return SetObjectToolSaveErrorResponse(context, rcc);

   json_t *tool = GetObjectToolIntoJSON(toolId, context->getUserId(), true);
   if (tool == nullptr)
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Object tool [%u] updated", toolId);

   context->setResponseData(tool);
   json_decref(tool);
   return 200;
}

/**
 * Handler for DELETE /v1/object-tools/:tool-id
 */
int H_ObjectToolDelete(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_TOOLS))
      return 403;

   uint32_t toolId = context->getPlaceholderValueAsUInt32(L"tool-id");
   if (toolId == 0)
      return 400;

   uint32_t rcc = DeleteObjectToolFromDB(toolId);
   if (rcc == RCC_SUCCESS)
   {
      context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Object tool [%u] deleted", toolId);
      return 204;
   }

   context->setErrorResponse("Database failure");
   return 500;
}

/**
 * Common implementation for enable/disable handlers
 */
static int ChangeObjectToolEnabledState(Context *context, bool enabled)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_TOOLS))
      return 403;

   uint32_t toolId = context->getPlaceholderValueAsUInt32(L"tool-id");
   if (toolId == 0)
      return 400;

   uint32_t rcc = ChangeObjectToolStatus(toolId, enabled);
   if (rcc == RCC_SUCCESS)
   {
      context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Object tool [%u] %s", toolId, enabled ? L"enabled" : L"disabled");
      return 204;
   }

   context->setErrorResponse("Database failure");
   return 500;
}

/**
 * Handler for POST /v1/object-tools/:tool-id/enable
 */
int H_ObjectToolEnable(Context *context)
{
   return ChangeObjectToolEnabledState(context, true);
}

/**
 * Handler for POST /v1/object-tools/:tool-id/disable
 */
int H_ObjectToolDisable(Context *context)
{
   return ChangeObjectToolEnabledState(context, false);
}

/**
 * Execute agent action tool
 */
static int ExecuteAgentAction(Context *context, const shared_ptr<NetObj>& object, const TCHAR *toolData, uint32_t toolFlags, Alarm *alarm, const StringMap *inputFields, const StringList *maskedFields, json_t *response)
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
   wchar_t actionName[MAX_PARAM_NAME];
   wcslcpy(actionName, args.get(0), MAX_PARAM_NAME);
   args.remove(0);

   uint32_t rcc;
   if (toolFlags & TF_GENERATES_OUTPUT)
   {
      StringBuffer output;
      rcc = conn->executeCommand(actionName, args, true, ActionOutputCallback, &output, true);
      // Command terminated by agent on execution time limit still has partial output worth returning
      if ((rcc == ERR_SUCCESS) || (rcc == ERR_EXEC_TIMEOUT))
      {
         json_object_set_new(response, "type", json_string("text"));
         json_object_set_new(response, "output", json_string_t(output.cstr()));
         json_object_set_new(response, "status", json_string((rcc == ERR_SUCCESS) ? "completed" : "timeout"));
      }
   }
   else
   {
      rcc = conn->executeCommand(actionName, args);
      if (rcc == ERR_SUCCESS)
      {
         json_object_set_new(response, "type", json_string("none"));
         json_object_set_new(response, "status", json_string("completed"));
      }
   }

   if ((rcc != ERR_SUCCESS) && (rcc != ERR_EXEC_TIMEOUT))
   {
      context->setAgentErrorResponse(rcc);
      return 500;
   }

   String inputFieldsLog = BuildAuditInputFieldsString(*inputFields, maskedFields);
   context->writeAuditLog(AUDIT_OBJECTS, true, object->getId(), _T("Executed agent action \"%s\" on object %s [%u]%s%s"),
         actionName, object->getName(), object->getId(), inputFieldsLog.cstr(),
         (rcc == ERR_EXEC_TIMEOUT) ? _T(" (terminated after exceeding execution time limit)") : _T(""));
   return 200;
}

/**
 * Execute server command tool
 */
static int ExecuteServerCommand(Context *context, const shared_ptr<NetObj>& object, const TCHAR *toolData, uint32_t toolFlags, Alarm *alarm, const StringMap *inputFields, const StringList *maskedFields, json_t *response)
{
   if ((object->getObjectClass() != OBJECT_NODE) && (object->getObjectClass() != OBJECT_CONTAINER) &&
         (object->getObjectClass() != OBJECT_COLLECTOR) && (object->getObjectClass() != OBJECT_SERVICEROOT) &&
         (object->getObjectClass() != OBJECT_FACILITY) && (object->getObjectClass() != OBJECT_POWERDOMAIN) &&
         (object->getObjectClass() != OBJECT_COOLINGZONE) && (object->getObjectClass() != OBJECT_ROOM) &&
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

   String inputFieldsLog = BuildAuditInputFieldsString(*inputFields, maskedFields);
   context->writeAuditLog(AUDIT_OBJECTS, true, object->getId(), L"Executed server command %s%s", expandedCommand.cstr(), inputFieldsLog.cstr());
   return 200;
}

/**
 * Execute server script tool
 */
static int ExecuteServerScript(Context *context, const shared_ptr<NetObj>& object, const TCHAR *toolData, Alarm *alarm, const StringMap *inputFields, const StringList *maskedFields, json_t *response)
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
   vm->setSecurityContext(new NXSL_UserSecurityContext(context));
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

   String inputFieldsLog = BuildAuditInputFieldsString(*inputFields, maskedFields);
   context->writeAuditLog(AUDIT_OBJECTS, true, object->getId(), L"Executed server script tool \"%s\" on object %s [%u]%s",
         scriptArgs->get(0), object->getName(), object->getId(), inputFieldsLog.cstr());

   delete vm;
   delete scriptArgs;
   return 200;
}

/**
 * Execute table tool (SNMP table, agent table, or agent list).
 */
static int ExecuteTableTool(Context *context, const shared_ptr<NetObj>& object, uint32_t toolId, const StringMap& inputFields, const StringList *maskedFields, json_t *response)
{
   if (object->getObjectClass() != OBJECT_NODE)
   {
      context->setErrorResponse("Object is not a node");
      return 400;
   }

   json_t *tableResult = nullptr;
   uint32_t rcc = ExecuteTableToolToJSON(toolId, static_pointer_cast<Node>(object), &tableResult, &inputFields, maskedFields, context->getLoginName());
   if (rcc != RCC_SUCCESS)
   {
      context->setErrorResponse("Table tool execution failed");
      return 500;
   }

   json_object_set_new(response, "type", json_string("table"));
   json_object_set_new(response, "table", tableResult);
   String inputFieldsLog = BuildAuditInputFieldsString(inputFields, maskedFields);
   context->writeAuditLog(AUDIT_OBJECTS, true, object->getId(),
         L"Executed table tool [%u] on object %s [%u]%s",
         toolId, object->getName(), object->getId(), inputFieldsLog.cstr());
   return 200;
}

/**
 * Execute SSH command tool. The SSH session is opened against the owning node of the target
 * object (which may be a node, interface, sensor, or access point); the source object is used
 * for macro expansion context. Access rights must already have been validated by the caller.
 */
static int ExecuteSSHCommand(Context *context, const shared_ptr<NetObj>& object, const TCHAR *toolData, uint32_t toolFlags, Alarm *alarm, const StringMap *inputFields, const StringList *maskedFields, json_t *response)
{
   shared_ptr<Node> targetNode = GetParentNodeForObjectTool(object);
   if (targetNode == nullptr)
   {
      context->setErrorResponse("SSH command not supported for this object");
      return 400;
   }
   Node& node = *targetNode;

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

   String inputFieldsLog = BuildAuditInputFieldsString(*inputFields, maskedFields);
   if (object->getId() != node.getId())
      context->writeAuditLog(AUDIT_OBJECTS, true, node.getId(), _T("Executed SSH command on node %s [%u] (context: %s [%u])%s"),
            node.getName(), node.getId(), object->getName(), object->getId(), inputFieldsLog.cstr());
   else
      context->writeAuditLog(AUDIT_OBJECTS, true, node.getId(), _T("Executed SSH command on object %s [%u]%s"),
            node.getName(), node.getId(), inputFieldsLog.cstr());
   return 200;
}

/**
 * Data for streaming tool execution on thread pool
 */
struct StreamingToolData
{
   shared_ptr<NetObj> object;
   TCHAR *toolData;
   Alarm *alarm;
   StringMap inputFields;
   shared_ptr<ToolOutputWebSocketSession> session;
   uint32_t userId;
   wchar_t loginName[MAX_USER_NAME];

   StreamingToolData(const shared_ptr<NetObj>& _object, TCHAR *_toolData, Alarm *_alarm, const StringMap& _inputFields,
            const shared_ptr<ToolOutputWebSocketSession>& _session, const Context *context) : object(_object), inputFields(_inputFields), session(_session)
   {
      toolData = _toolData;
      alarm = _alarm;
      userId = context->getUserId();
      wcslcpy(loginName, context->getLoginName(), MAX_USER_NAME);
   }

   ~StreamingToolData()
   {
      delete alarm;
      MemFree(toolData);
   }
};

/**
 * Execute agent action in streaming mode (thread pool callback)
 */
static void StreamingAgentActionThread(StreamingToolData *data)
{
   if (!data->session->waitForConnection())
   {
      delete data;
      return;
   }

   if (data->object->getObjectClass() != OBJECT_NODE)
   {
      data->session->sendError("Object is not a node");
      delete data;
      return;
   }

   shared_ptr<AgentConnectionEx> conn = static_cast<Node&>(*data->object).createAgentConnection();
   if (conn == nullptr)
   {
      data->session->sendError("Cannot connect to agent");
      delete data;
      return;
   }

   StringList args = SplitCommandLine(data->object->expandText(data->toolData, data->alarm, nullptr, shared_ptr<DCObjectInfo>(), data->loginName, nullptr, nullptr, &data->inputFields, nullptr));
   wchar_t actionName[MAX_PARAM_NAME];
   wcslcpy(actionName, args.get(0), MAX_PARAM_NAME);
   args.remove(0);

   uint32_t rcc = conn->executeCommand(actionName, args, true, StreamingActionOutputCallback, data->session.get(), true);
   if (rcc != ERR_SUCCESS)
   {
      char errorMsg[256];
      snprintf(errorMsg, sizeof(errorMsg), "Agent error %u", rcc);
      data->session->sendError(errorMsg);
   }
   else
   {
      data->session->sendCompleted();
   }

   delete data;
}

/**
 * Execute server command in streaming mode (thread pool callback)
 */
static void StreamingServerCommandThread(StreamingToolData *data)
{
   if (!data->session->waitForConnection())
   {
      delete data;
      return;
   }

   StringBuffer expandedCommand = data->object->expandText(data->toolData, data->alarm, nullptr, shared_ptr<DCObjectInfo>(), data->loginName, nullptr, nullptr, &data->inputFields, nullptr);

   WebAPIStreamingProcessExecutor executor(expandedCommand, data->session);
   if (executor.execute())
   {
      // Wait for process to complete (endOfOutput will send completed message)
      executor.waitForCompletion(INFINITE);
   }
   else
   {
      data->session->sendError("Failed to execute server command");
   }

   delete data;
}

/**
 * Execute server script in streaming mode (thread pool callback)
 */
static void StreamingServerScriptThread(StreamingToolData *data)
{
   if (!data->session->waitForConnection())
   {
      delete data;
      return;
   }

   StringBuffer expandedScript = data->object->expandText(data->toolData, data->alarm, nullptr, shared_ptr<DCObjectInfo>(), data->loginName, nullptr, nullptr, &data->inputFields, nullptr);
   StringList *scriptArgs = ParseCommandLine(expandedScript);

   if (scriptArgs->size() == 0)
   {
      delete scriptArgs;
      data->session->sendError("Empty script name");
      delete data;
      return;
   }

   NXSL_WebAPIStreamingEnv *env = new NXSL_WebAPIStreamingEnv(data->session);
   NXSL_VM *vm = GetServerScriptLibrary()->createVM(scriptArgs->get(0), env);
   if (vm == nullptr)
   {
      delete scriptArgs;
      data->session->sendError("Script not found in library");
      delete data;
      return;
   }

   SetupServerScriptVM(vm, data->object, shared_ptr<DCObjectInfo>());
   vm->setSecurityContext(new NXSL_UserSecurityContext(data->userId));
   vm->setGlobalVariable("$INPUT", vm->createValue(new NXSL_HashMap(vm, &data->inputFields)));

   ObjectRefArray<NXSL_Value> sargs(scriptArgs->size() - 1, 1);
   for(int i = 1; i < scriptArgs->size(); i++)
      sargs.add(vm->createValue(scriptArgs->get(i)));

   if (!vm->run(sargs))
   {
      data->session->sendError("Script execution failed");
   }
   else
   {
      const TCHAR *result = vm->getResult()->getValueAsCString();
      if (result != nullptr)
         data->session->sendResult(result);
      data->session->sendCompleted();
   }

   delete vm;
   delete scriptArgs;
   delete data;
}

/**
 * Execute SSH command in streaming mode (thread pool callback). The SSH session is opened
 * against the owning node of the target object; `data->object` is used for macro expansion
 * context.
 */
static void StreamingSSHCommandThread(StreamingToolData *data)
{
   if (!data->session->waitForConnection())
   {
      delete data;
      return;
   }

   shared_ptr<Node> targetNode = GetParentNodeForObjectTool(data->object);
   if (targetNode == nullptr)
   {
      data->session->sendError("SSH command not supported for this object");
      delete data;
      return;
   }
   Node& node = *targetNode;

   StringBuffer command = data->object->expandText(data->toolData, data->alarm, nullptr, shared_ptr<DCObjectInfo>(), data->loginName, nullptr, nullptr, &data->inputFields, nullptr);

   uint32_t proxyId = node.getEffectiveSshProxy();
   shared_ptr<Node> proxy = static_pointer_cast<Node>(FindObjectById(proxyId, OBJECT_NODE));
   if (proxy == nullptr)
   {
      data->session->sendError("SSH proxy not available");
      delete data;
      return;
   }

   shared_ptr<AgentConnectionEx> conn = proxy->createAgentConnection();
   if (conn == nullptr)
   {
      data->session->sendError("Cannot connect to SSH proxy agent");
      delete data;
      return;
   }

   StringList sshArgs;
   TCHAR ipAddr[64];
   sshArgs.add(node.getIpAddress().toString(ipAddr));
   sshArgs.add(node.getSshPort());
   sshArgs.add(node.getSshLogin());
   sshArgs.add(node.getSshPassword());
   sshArgs.add(command);
   sshArgs.add(node.getSshKeyId());

   uint32_t rcc = conn->executeCommand(_T("SSH.Command"), sshArgs, true, StreamingActionOutputCallback, data->session.get(), true);
   if (rcc != ERR_SUCCESS)
   {
      char errorMsg[256];
      snprintf(errorMsg, sizeof(errorMsg), "Agent error %u", rcc);
      data->session->sendError(errorMsg);
   }
   else
   {
      data->session->sendCompleted();
   }

   delete data;
}

/**
 * Check if tool type supports streaming
 */
static bool IsStreamableToolType(int toolType)
{
   return (toolType == TOOL_TYPE_ACTION) ||
          (toolType == TOOL_TYPE_SERVER_COMMAND) ||
          (toolType == TOOL_TYPE_SERVER_SCRIPT) ||
          (toolType == TOOL_TYPE_SSH_COMMAND);
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
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_TOOLS) && !CheckObjectToolAccess(toolId, context->getUserId()))
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

   // Reject client-only tool types. Note that a tool of the removed "internal" type (0) can still
   // exist in a database not yet upgraded - it falls through to the execution dispatch below and is
   // rejected there as unsupported.
   switch(toolType)
   {
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

   if (toolType == TOOL_TYPE_SSH_COMMAND)
   {
      // SSH tool requires CONTROL on the node the session will connect to, plus READ on the
      // source object used for macro expansion context. Resolution of the owning node covers
      // the node-on-node case as an identity and the interface/sensor/AP-on-node cases.
      shared_ptr<Node> sshTarget = GetParentNodeForObjectTool(object);
      if (sshTarget == nullptr)
      {
         MemFree(toolData);
         context->setErrorResponse("SSH command not supported for this object");
         return 400;
      }
      if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ) ||
          !sshTarget->checkAccessRights(context->getUserId(), OBJECT_ACCESS_CONTROL))
      {
         MemFree(toolData);
         return 403;
      }
   }
   else if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_CONTROL))
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

   // Names of input fields whose values must be masked in audit log (typically password-type fields).
   // Optional; if omitted no masking is applied.
   StringList maskedFields;
   json_t *maskedFieldsJson = json_object_get(request, "maskedFields");
   if (json_is_array(maskedFieldsJson))
   {
      size_t index;
      json_t *value;
      json_array_foreach(maskedFieldsJson, index, value)
      {
         if (json_is_string(value))
            maskedFields.addUTF8String(json_string_value(value));
      }
   }

   // Check if streaming mode is requested
   bool streamRequested = json_object_get_boolean(request, "stream", false);
   if (streamRequested && IsStreamableToolType(toolType) && (toolFlags & TF_GENERATES_OUTPUT))
   {
      // Streaming mode - start async execution, return token for WebSocket connection
      uuid token = uuid::generate();
      auto session = make_shared<ToolOutputWebSocketSession>(token);
      s_pendingToolOutputSessionsLock.lock();
      s_pendingToolOutputSessions.set(token, session);
      s_pendingToolOutputSessionsLock.unlock();

      // Start tool execution on thread pool
      auto data = new StreamingToolData(object, toolData, alarm, inputFields, session, context);
      switch(toolType)
      {
         case TOOL_TYPE_ACTION:
            ThreadPoolExecute(g_mainThreadPool, StreamingAgentActionThread, data);
            break;
         case TOOL_TYPE_SERVER_COMMAND:
            ThreadPoolExecute(g_mainThreadPool, StreamingServerCommandThread, data);
            break;
         case TOOL_TYPE_SERVER_SCRIPT:
            ThreadPoolExecute(g_mainThreadPool, StreamingServerScriptThread, data);
            break;
         case TOOL_TYPE_SSH_COMMAND:
            ThreadPoolExecute(g_mainThreadPool, StreamingSSHCommandThread, data);
            break;
      }

      // toolData and alarm ownership transferred to the thread pool callback
      // Return token to client
      json_t *response = json_object();
      char tokenStr[64];
      token.toStringA(tokenStr);
      json_object_set_new(response, "token", json_string(tokenStr));

      char wsUrl[128];
      snprintf(wsUrl, sizeof(wsUrl), "/v1/object-tools/output/%s", tokenStr);
      json_object_set_new(response, "wsUrl", json_string(wsUrl));

      String inputFieldsLog = BuildAuditInputFieldsString(inputFields, &maskedFields);
      context->writeAuditLog(AUDIT_OBJECTS, true, object->getId(), L"Started streaming tool execution on object %s [%u]%s",
            object->getName(), object->getId(), inputFieldsLog.cstr());
      context->setResponseData(response);
      json_decref(response);
      return 202;
   }

   // Synchronous mode (original behavior)
   json_t *response = json_object();

   int httpCode;
   switch(toolType)
   {
      case TOOL_TYPE_ACTION:
         httpCode = ExecuteAgentAction(context, object, toolData, toolFlags, alarm, &inputFields, &maskedFields, response);
         break;
      case TOOL_TYPE_SERVER_COMMAND:
         httpCode = ExecuteServerCommand(context, object, toolData, toolFlags, alarm, &inputFields, &maskedFields, response);
         break;
      case TOOL_TYPE_SERVER_SCRIPT:
         httpCode = ExecuteServerScript(context, object, toolData, alarm, &inputFields, &maskedFields, response);
         break;
      case TOOL_TYPE_SNMP_TABLE:
      case TOOL_TYPE_AGENT_TABLE:
      case TOOL_TYPE_AGENT_LIST:
         httpCode = ExecuteTableTool(context, object, toolId, inputFields, &maskedFields, response);
         break;
      case TOOL_TYPE_SSH_COMMAND:
         httpCode = ExecuteSSHCommand(context, object, toolData, toolFlags, alarm, &inputFields, &maskedFields, response);
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

/**
 * WebSocket upgrade handler for tool output streaming
 */
void WS_ToolOutputConnect(void *cls, MHD_Connection *connection, void *con_cls,
                          const char *extra_in, size_t extra_in_size, MHD_socket sock,
                          MHD_UpgradeResponseHandle *responseHandle)
{
   Context *context = static_cast<Context*>(cls);

   const TCHAR *tokenStr = context->getPlaceholderValue(_T("token"));
   if (tokenStr == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Tool output WebSocket connection rejected: no token provided");
      SendWebsocketCloseFrame(static_cast<SOCKET>(sock), WS_CLOSE_POLICY_VIOLATION);
      MHD_upgrade_action(responseHandle, MHD_UPGRADE_ACTION_CLOSE);
      return;
   }

   uuid token = uuid::parse(tokenStr);
   if (token.isNull())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Tool output WebSocket connection rejected: invalid token format");
      SendWebsocketCloseFrame(static_cast<SOCKET>(sock), WS_CLOSE_POLICY_VIOLATION);
      MHD_upgrade_action(responseHandle, MHD_UPGRADE_ACTION_CLOSE);
      return;
   }

   // Token is single-use; it is removed by execution thread when not used within validity period
   s_pendingToolOutputSessionsLock.lock();
   shared_ptr<ToolOutputWebSocketSession> session = s_pendingToolOutputSessions.getShared(token);
   if (session != nullptr)
      s_pendingToolOutputSessions.remove(token);
   s_pendingToolOutputSessionsLock.unlock();

   if (session == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Tool output WebSocket connection rejected: token not found");
      SendWebsocketCloseFrame(static_cast<SOCKET>(sock), WS_CLOSE_POLICY_VIOLATION);
      MHD_upgrade_action(responseHandle, MHD_UPGRADE_ACTION_CLOSE);
      return;
   }

   session->connect(responseHandle, static_cast<SOCKET>(sock));
   nxlog_debug_tag(DEBUG_TAG, 4, L"Tool output WebSocket connection established");

   ThreadCreate([session]() -> void { session->readFrames(); });
}
