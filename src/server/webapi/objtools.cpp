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
 * Tool output WebSocket session
 */
class ToolOutputWebSocketSession
{
private:
   MHD_UpgradeResponseHandle *m_responseHandle;
   SOCKET m_socket;
   Mutex m_socketMutex;
   bool m_started;
   bool m_closed;
   Mutex m_closeMutex;
   time_t m_creationTime;
   StringBuffer m_pendingOutput;
   bool m_executionComplete;
   Condition m_websocketReady;

   void sendTextFrame(const char *text)
   {
      m_socketMutex.lock();
      if (m_started && !m_closed)
         SendWebsocketFrame(m_socket, text, strlen(text));
      m_socketMutex.unlock();
   }

public:
   ToolOutputWebSocketSession() : m_socketMutex(MutexType::FAST), m_closeMutex(MutexType::FAST), m_websocketReady(true)
   {
      m_responseHandle = nullptr;
      m_socket = INVALID_SOCKET;
      m_started = false;
      m_closed = false;
      m_executionComplete = false;
      m_creationTime = time(nullptr);
   }

   time_t getCreationTime() const { return m_creationTime; }
   bool isClosed() const { return m_closed; }

   /**
    * Wait for WebSocket connection to be established (called from execution thread)
    */
   bool waitForWebSocket(uint32_t timeout)
   {
      return m_websocketReady.wait(timeout);
   }

   /**
    * Start WebSocket session after upgrade
    */
   void start(MHD_UpgradeResponseHandle *responseHandle, SOCKET s)
   {
      m_socketMutex.lock();
      m_responseHandle = responseHandle;
      m_socket = s;
      m_started = true;

      // Flush any output that arrived before WebSocket was connected
      if (!m_pendingOutput.isEmpty())
      {
         json_t *msg = json_object();
         json_object_set_new(msg, "type", json_string("output"));
         json_object_set_new(msg, "data", json_string_t(m_pendingOutput.cstr()));
         char *encoded = json_dumps(msg, 0);
         SendWebsocketFrame(m_socket, encoded, strlen(encoded));
         MemFree(encoded);
         json_decref(msg);
         m_pendingOutput.clear();
      }

      m_socketMutex.unlock();
      m_websocketReady.set();
      nxlog_debug_tag(DEBUG_TAG, 5, L"Tool output WebSocket session started");
   }

   /**
    * Send output chunk to WebSocket client
    */
   void sendOutput(const TCHAR *text)
   {
      if (m_closed)
         return;

      m_socketMutex.lock();
      if (m_started)
      {
         json_t *msg = json_object();
         json_object_set_new(msg, "type", json_string("output"));
         json_object_set_new(msg, "data", json_string_t(text));
         char *encoded = json_dumps(msg, 0);
         SendWebsocketFrame(m_socket, encoded, strlen(encoded));
         MemFree(encoded);
         json_decref(msg);
      }
      else
      {
         m_pendingOutput.append(text);
      }
      m_socketMutex.unlock();
   }

   /**
    * Send output chunk from UTF-8 source
    */
   void sendOutputUtf8(const char *text, size_t length)
   {
      if (m_closed)
         return;

      m_socketMutex.lock();
      if (m_started)
      {
         // Build JSON with raw UTF-8 text
         json_t *msg = json_object();
         json_object_set_new(msg, "type", json_string("output"));
         json_object_set_new(msg, "data", json_stringn(text, length));
         char *encoded = json_dumps(msg, 0);
         SendWebsocketFrame(m_socket, encoded, strlen(encoded));
         MemFree(encoded);
         json_decref(msg);
      }
      else
      {
#ifdef UNICODE
         m_pendingOutput.appendUtf8String(text, length);
#else
         m_pendingOutput.append(text, length);
#endif
      }
      m_socketMutex.unlock();
   }

   /**
    * Send completion message
    */
   void sendCompleted()
   {
      sendTextFrame("{\"type\":\"completed\"}");
      close();
   }

   /**
    * Send error message
    */
   void sendError(const char *message)
   {
      json_t *msg = json_object();
      json_object_set_new(msg, "type", json_string("error"));
      json_object_set_new(msg, "message", json_string(message));
      char *encoded = json_dumps(msg, 0);
      sendTextFrame(encoded);
      MemFree(encoded);
      json_decref(msg);
      close();
   }

   /**
    * Send script result
    */
   void sendResult(const TCHAR *result)
   {
      if (m_closed)
         return;

      m_socketMutex.lock();
      if (m_started)
      {
         json_t *msg = json_object();
         json_object_set_new(msg, "type", json_string("result"));
         json_object_set_new(msg, "data", json_string_t(result));
         char *encoded = json_dumps(msg, 0);
         SendWebsocketFrame(m_socket, encoded, strlen(encoded));
         MemFree(encoded);
         json_decref(msg);
      }
      m_socketMutex.unlock();
   }

   /**
    * Main loop - read incoming WebSocket frames
    */
   void run()
   {
      while (!m_closed && !IsShutdownInProgress())
      {
         ByteStream buffer;
         BYTE frameType;

         if (!ReadWebsocketFrame(static_cast<int>(m_socket), &buffer, &frameType))
         {
            nxlog_debug_tag(DEBUG_TAG, 5, L"Tool output WebSocket: read error");
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
            BYTE pong[2] = { 0x8A, 0x00 };
            SendEx(m_socket, pong, 2, 0, nullptr);
            m_socketMutex.unlock();
         }
         // Ignore other frame types for now (cancel support can be added later)
      }
      close();
   }

   /**
    * Close the session
    */
   void close()
   {
      m_closeMutex.lock();
      if (m_closed)
      {
         m_closeMutex.unlock();
         return;
      }
      m_closed = true;
      m_closeMutex.unlock();

      m_socketMutex.lock();
      if (m_socket != INVALID_SOCKET)
         SendWebsocketCloseFrame(m_socket, WS_CLOSE_NORMAL);
      m_socketMutex.unlock();

      if (m_responseHandle != nullptr)
         MHD_upgrade_action(m_responseHandle, MHD_UPGRADE_ACTION_CLOSE);

      nxlog_debug_tag(DEBUG_TAG, 5, L"Tool output WebSocket session closed");
   }

   /**
    * Mark execution as complete (used by cleanup to know if session can be removed)
    */
   void setExecutionComplete() { m_executionComplete = true; }
   bool isExecutionComplete() const { return m_executionComplete; }
};

/**
 * Pending tool output sessions
 */
static HashMap<uuid, ToolOutputWebSocketSession> s_pendingToolOutputSessions(Ownership::True);
static Mutex s_pendingToolOutputSessionsLock;

/**
 * Cleanup expired tool output sessions
 */
void CleanupExpiredToolOutputSessions()
{
   time_t now = time(nullptr);

   s_pendingToolOutputSessionsLock.lock();

   StructArray<uuid> expired;
   s_pendingToolOutputSessions.forEach(
      [now, &expired](const uuid& token, ToolOutputWebSocketSession *session) -> EnumerationCallbackResult
      {
         if (now - session->getCreationTime() > TOOL_OUTPUT_TOKEN_VALIDITY * 2)
         {
            nxlog_debug_tag(DEBUG_TAG, 5, L"Cleaning up expired tool output session token");
            session->close();
            expired.add(token);
         }
         return _CONTINUE;
      });

   for (int i = 0; i < expired.size(); i++)
      s_pendingToolOutputSessions.remove(*expired.get(i));

   s_pendingToolOutputSessionsLock.unlock();

   ThreadPoolScheduleRelative(g_mainThreadPool, 300000, CleanupExpiredToolOutputSessions);
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
   ToolOutputWebSocketSession *m_session;

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
   WebAPIStreamingProcessExecutor(const TCHAR *command, ToolOutputWebSocketSession *session)
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
   ToolOutputWebSocketSession *m_session;

public:
   NXSL_WebAPIStreamingEnv(ToolOutputWebSocketSession *session) : NXSL_ServerEnv(), m_session(session) {}
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

   String inputFieldsLog = BuildAuditInputFieldsString(*inputFields, maskedFields);
   context->writeAuditLog(AUDIT_OBJECTS, true, object->getId(), _T("Executed agent action \"%s\" on object %s [%u]%s"),
         actionName, object->getName(), object->getId(), inputFieldsLog.cstr());
   return 200;
}

/**
 * Execute server command tool
 */
static int ExecuteServerCommand(Context *context, const shared_ptr<NetObj>& object, const TCHAR *toolData, uint32_t toolFlags, Alarm *alarm, const StringMap *inputFields, const StringList *maskedFields, json_t *response)
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

   String inputFieldsLog = BuildAuditInputFieldsString(*inputFields, maskedFields);
   context->writeAuditLog(AUDIT_OBJECTS, true, object->getId(), L"Executed server script tool \"%s\" on object %s [%u]%s",
         scriptArgs->get(0), object->getName(), object->getId(), inputFieldsLog.cstr());

   delete vm;
   delete scriptArgs;
   return 200;
}

/**
 * Execute table tool (SNMP table, agent table, or agent list).
 * These tool types do not perform macro expansion, so input fields are not used.
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
 * Data for streaming agent action execution on thread pool
 */
struct StreamingAgentActionData
{
   shared_ptr<NetObj> object;
   TCHAR *toolData;
   Alarm *alarm;
   StringMap inputFields;
   StringList maskedFields;
   ToolOutputWebSocketSession *session;
   uint32_t userId;
   wchar_t loginName[MAX_USER_NAME];
};

/**
 * Execute agent action in streaming mode (thread pool callback)
 */
static void StreamingAgentActionThread(StreamingAgentActionData *data)
{
   data->session->waitForWebSocket(TOOL_OUTPUT_TOKEN_VALIDITY * 1000);

   if (data->session->isClosed())
   {
      delete data->alarm;
      MemFree(data->toolData);
      delete data;
      return;
   }

   if (data->object->getObjectClass() != OBJECT_NODE)
   {
      data->session->sendError("Object is not a node");
      delete data->alarm;
      MemFree(data->toolData);
      delete data;
      return;
   }

   shared_ptr<AgentConnectionEx> conn = static_cast<Node&>(*data->object).createAgentConnection();
   if (conn == nullptr)
   {
      data->session->sendError("Cannot connect to agent");
      delete data->alarm;
      MemFree(data->toolData);
      delete data;
      return;
   }

   StringList args = SplitCommandLine(data->object->expandText(data->toolData, data->alarm, nullptr, shared_ptr<DCObjectInfo>(), data->loginName, nullptr, nullptr, &data->inputFields, nullptr));
   wchar_t actionName[MAX_PARAM_NAME];
   wcslcpy(actionName, args.get(0), MAX_PARAM_NAME);
   args.remove(0);

   uint32_t rcc = conn->executeCommand(actionName, args, true, StreamingActionOutputCallback, data->session, true);
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

   delete data->alarm;
   MemFree(data->toolData);
   delete data;
}

/**
 * Data for streaming server command execution on thread pool
 */
struct StreamingServerCommandData
{
   shared_ptr<NetObj> object;
   TCHAR *toolData;
   Alarm *alarm;
   StringMap inputFields;
   StringList maskedFields;
   ToolOutputWebSocketSession *session;
   wchar_t loginName[MAX_USER_NAME];
};

/**
 * Execute server command in streaming mode (thread pool callback)
 */
static void StreamingServerCommandThread(StreamingServerCommandData *data)
{
   data->session->waitForWebSocket(TOOL_OUTPUT_TOKEN_VALIDITY * 1000);

   if (data->session->isClosed())
   {
      delete data->alarm;
      MemFree(data->toolData);
      delete data;
      return;
   }

   StringBuffer expandedCommand = data->object->expandText(data->toolData, data->alarm, nullptr, shared_ptr<DCObjectInfo>(), data->loginName, nullptr, nullptr, &data->inputFields, nullptr);

   WebAPIStreamingProcessExecutor executor(expandedCommand, data->session);
   if (!executor.execute())
   {
      data->session->sendError("Failed to execute server command");
      delete data->alarm;
      MemFree(data->toolData);
      delete data;
      return;
   }

   // Wait for process to complete (endOfOutput will send completed/error)
   executor.waitForCompletion(INFINITE);

   delete data->alarm;
   MemFree(data->toolData);
   delete data;
}

/**
 * Data for streaming server script execution on thread pool
 */
struct StreamingServerScriptData
{
   shared_ptr<NetObj> object;
   TCHAR *toolData;
   Alarm *alarm;
   StringMap inputFields;
   StringList maskedFields;
   ToolOutputWebSocketSession *session;
   uint32_t userId;
   wchar_t loginName[MAX_USER_NAME];
};

/**
 * Execute server script in streaming mode (thread pool callback)
 */
static void StreamingServerScriptThread(StreamingServerScriptData *data)
{
   data->session->waitForWebSocket(TOOL_OUTPUT_TOKEN_VALIDITY * 1000);

   if (data->session->isClosed())
   {
      delete data->alarm;
      MemFree(data->toolData);
      delete data;
      return;
   }

   StringBuffer expandedScript = data->object->expandText(data->toolData, data->alarm, nullptr, shared_ptr<DCObjectInfo>(), data->loginName, nullptr, nullptr, &data->inputFields, nullptr);
   StringList *scriptArgs = ParseCommandLine(expandedScript);

   if (scriptArgs->size() == 0)
   {
      delete scriptArgs;
      data->session->sendError("Empty script name");
      delete data->alarm;
      MemFree(data->toolData);
      delete data;
      return;
   }

   NXSL_WebAPIStreamingEnv *env = new NXSL_WebAPIStreamingEnv(data->session);
   NXSL_VM *vm = GetServerScriptLibrary()->createVM(scriptArgs->get(0), env);
   if (vm == nullptr)
   {
      delete scriptArgs;
      data->session->sendError("Script not found in library");
      delete data->alarm;
      MemFree(data->toolData);
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
      char errorMsg[1024];
      snprintf(errorMsg, sizeof(errorMsg), "Script execution failed");
      data->session->sendError(errorMsg);
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
   delete data->alarm;
   MemFree(data->toolData);
   delete data;
}

/**
 * Data for streaming SSH command execution on thread pool
 */
struct StreamingSSHCommandData
{
   shared_ptr<NetObj> object;
   TCHAR *toolData;
   Alarm *alarm;
   StringMap inputFields;
   StringList maskedFields;
   ToolOutputWebSocketSession *session;
   wchar_t loginName[MAX_USER_NAME];
};

/**
 * Execute SSH command in streaming mode (thread pool callback). The SSH session is opened
 * against the owning node of the target object; `data->object` is used for macro expansion
 * context.
 */
static void StreamingSSHCommandThread(StreamingSSHCommandData *data)
{
   data->session->waitForWebSocket(TOOL_OUTPUT_TOKEN_VALIDITY * 1000);

   if (data->session->isClosed())
   {
      delete data->alarm;
      MemFree(data->toolData);
      delete data;
      return;
   }

   shared_ptr<Node> targetNode = GetParentNodeForObjectTool(data->object);
   if (targetNode == nullptr)
   {
      data->session->sendError("SSH command not supported for this object");
      delete data->alarm;
      MemFree(data->toolData);
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
      delete data->alarm;
      MemFree(data->toolData);
      delete data;
      return;
   }

   shared_ptr<AgentConnectionEx> conn = proxy->createAgentConnection();
   if (conn == nullptr)
   {
      data->session->sendError("Cannot connect to SSH proxy agent");
      delete data->alarm;
      MemFree(data->toolData);
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

   uint32_t rcc = conn->executeCommand(_T("SSH.Command"), sshArgs, true, StreamingActionOutputCallback, data->session, true);
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

   delete data->alarm;
   MemFree(data->toolData);
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
      auto *session = new ToolOutputWebSocketSession();

      uuid token = uuid::generate();
      s_pendingToolOutputSessionsLock.lock();
      s_pendingToolOutputSessions.set(token, session);
      s_pendingToolOutputSessionsLock.unlock();

      // Start tool execution on thread pool
      switch(toolType)
      {
         case TOOL_TYPE_ACTION:
         {
            auto *data = new StreamingAgentActionData();
            data->object = object;
            data->toolData = toolData;
            data->alarm = alarm;
            data->inputFields = inputFields;
            data->maskedFields = maskedFields;
            data->session = session;
            data->userId = context->getUserId();
            wcslcpy(data->loginName, context->getLoginName(), MAX_USER_NAME);
            ThreadPoolExecute(g_mainThreadPool, StreamingAgentActionThread, data);
            break;
         }
         case TOOL_TYPE_SERVER_COMMAND:
         {
            auto *data = new StreamingServerCommandData();
            data->object = object;
            data->toolData = toolData;
            data->alarm = alarm;
            data->inputFields = inputFields;
            data->maskedFields = maskedFields;
            data->session = session;
            wcslcpy(data->loginName, context->getLoginName(), MAX_USER_NAME);
            ThreadPoolExecute(g_mainThreadPool, StreamingServerCommandThread, data);
            break;
         }
         case TOOL_TYPE_SERVER_SCRIPT:
         {
            auto *data = new StreamingServerScriptData();
            data->object = object;
            data->toolData = toolData;
            data->alarm = alarm;
            data->inputFields = inputFields;
            data->maskedFields = maskedFields;
            data->session = session;
            data->userId = context->getUserId();
            wcslcpy(data->loginName, context->getLoginName(), MAX_USER_NAME);
            ThreadPoolExecute(g_mainThreadPool, StreamingServerScriptThread, data);
            break;
         }
         case TOOL_TYPE_SSH_COMMAND:
         {
            auto *data = new StreamingSSHCommandData();
            data->object = object;
            data->toolData = toolData;
            data->alarm = alarm;
            data->inputFields = inputFields;
            data->maskedFields = maskedFields;
            data->session = session;
            wcslcpy(data->loginName, context->getLoginName(), MAX_USER_NAME);
            ThreadPoolExecute(g_mainThreadPool, StreamingSSHCommandThread, data);
            break;
         }
         default:
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
         httpCode = ExecuteTableTool(context, object, toolId, response);
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

   // Look up and remove pending session (token is single-use)
   s_pendingToolOutputSessionsLock.lock();
   ToolOutputWebSocketSession *session = s_pendingToolOutputSessions.get(token);
   if (session != nullptr)
   {
      s_pendingToolOutputSessions.unlink(token);
   }
   s_pendingToolOutputSessionsLock.unlock();

   if (session == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Tool output WebSocket connection rejected: token not found");
      SendWebsocketCloseFrame(static_cast<SOCKET>(sock), WS_CLOSE_POLICY_VIOLATION);
      MHD_upgrade_action(responseHandle, MHD_UPGRADE_ACTION_CLOSE);
      return;
   }

   // Check expiration
   if (time(nullptr) - session->getCreationTime() > TOOL_OUTPUT_TOKEN_VALIDITY)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Tool output WebSocket connection rejected: token expired");
      session->close();
      delete session;
      SendWebsocketCloseFrame(static_cast<SOCKET>(sock), WS_CLOSE_POLICY_VIOLATION);
      MHD_upgrade_action(responseHandle, MHD_UPGRADE_ACTION_CLOSE);
      return;
   }

   session->start(responseHandle, static_cast<SOCKET>(sock));
   nxlog_debug_tag(DEBUG_TAG, 4, L"Tool output WebSocket connection established");

   ThreadCreate(
      [session]() -> void
      {
         session->run();
         delete session;
      });
}
