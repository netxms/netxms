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
** File: script_execution.cpp
**
**/

#include "webapi.h"

#define DEBUG_TAG L"webapi.script"

/**
 * Token validity period in seconds for script output WebSocket connection
 */
static const int SCRIPT_OUTPUT_TOKEN_VALIDITY = 30;

/**
 * Convert script result to JSON. If resultAsMap is true, result is always presented as JSON object.
 */
static json_t *ScriptResultToJson(NXSL_Value *result, bool resultAsMap)
{
   if (!resultAsMap || result->isHashMap())
      return result->toJson();

   json_t *map = json_object();
   if (result->isArray())
   {
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
   }
   else
   {
      json_object_set_new(map, "element1", json_string_t(result->getValueAsCString()));
   }
   return map;
}

/**
 * Ad-hoc script execution with output streamed to WebSocket client
 */
class ScriptExecution : public enable_shared_from_this<ScriptExecution>
{
private:
   uint32_t m_id;
   uuid m_token;
   uint32_t m_userId;
   bool m_resultAsMap;
   NXSL_VM *m_vm;
   ObjectRefArray<NXSL_Value> m_args;
   bool m_stopRequested;
   Mutex m_vmLock;
   Condition m_connected;
   MHD_UpgradeResponseHandle *m_responseHandle;
   SOCKET m_socket;
   bool m_closeFrameSent;
   bool m_disconnected;
   Mutex m_socketLock;

   void run();
   void sendMessage(json_t *msg);
   void sendCloseFrame();

public:
   ScriptExecution(uint32_t userId, bool resultAsMap);
   ~ScriptExecution();

   uint32_t getId() const { return m_id; }
   uint32_t getUserId() const { return m_userId; }

   uuid start(NXSL_VM *vm, const ObjectRefArray<NXSL_Value>& args);
   void stop();
   void sendOutput(const wchar_t *text);

   void connect(MHD_UpgradeResponseHandle *responseHandle, SOCKET s);
   void readFrames();
};

/**
 * Running script executions
 */
static SynchronizedSharedHashMap<uint32_t, ScriptExecution> s_executions;

/**
 * Script executions waiting for WebSocket connection, indexed by connection token
 */
static SharedHashMap<uuid, ScriptExecution> s_pendingConnections;
static Mutex s_pendingConnectionsLock(MutexType::FAST);

/**
 * Last used execution ID
 */
static VolatileCounter s_executionId = 0;

/**
 * NXSL environment that streams script output to WebSocket client
 */
class NXSL_ScriptExecutionEnv : public NXSL_ServerEnv
{
private:
   ScriptExecution *m_execution;

public:
   NXSL_ScriptExecutionEnv(ScriptExecution *execution) : NXSL_ServerEnv(), m_execution(execution) {}

   virtual void print(const wchar_t *text) override
   {
      m_execution->sendOutput(text);
   }

   virtual void trace(int level, const wchar_t *text) override
   {
      StringBuffer line(text);
      line.append(L'\n');
      m_execution->sendOutput(line);
      NXSL_ServerEnv::trace(level, text);
   }
};

/**
 * Script execution constructor
 */
ScriptExecution::ScriptExecution(uint32_t userId, bool resultAsMap) : m_vmLock(MutexType::FAST), m_connected(true), m_socketLock(MutexType::FAST)
{
   m_id = InterlockedIncrement(&s_executionId);
   m_userId = userId;
   m_resultAsMap = resultAsMap;
   m_vm = nullptr;
   m_stopRequested = false;
   m_responseHandle = nullptr;
   m_socket = INVALID_SOCKET;
   m_closeFrameSent = false;
   m_disconnected = false;
}

/**
 * Script execution destructor
 */
ScriptExecution::~ScriptExecution()
{
   delete m_vm;
}

/**
 * Start execution with given VM and arguments. Script will run as soon as WebSocket client connects.
 * Returns token for WebSocket connection.
 */
uuid ScriptExecution::start(NXSL_VM *vm, const ObjectRefArray<NXSL_Value>& args)
{
   m_vm = vm;
   for(int i = 0; i < args.size(); i++)
      m_args.add(args.get(i));
   m_token = uuid::generate();

   shared_ptr<ScriptExecution> self = shared_from_this();
   s_executions.set(m_id, self);
   s_pendingConnectionsLock.lock();
   s_pendingConnections.set(m_token, self);
   s_pendingConnectionsLock.unlock();

   ThreadCreate([self] () -> void { self->run(); });
   return m_token;
}

/**
 * Wait for client connection and execute script
 */
void ScriptExecution::run()
{
   if (!m_connected.wait(SCRIPT_OUTPUT_TOKEN_VALIDITY * 1000))
   {
      // Token may be consumed by connection handler right at timeout, in that case connection will be established shortly
      s_pendingConnectionsLock.lock();
      bool expired = s_pendingConnections.contains(m_token);
      if (expired)
         s_pendingConnections.remove(m_token);
      s_pendingConnectionsLock.unlock();

      if (expired)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, L"Script execution %u cancelled: client did not connect within %d seconds", m_id, SCRIPT_OUTPUT_TOKEN_VALIDITY);
         s_executions.remove(m_id);
         return;
      }
      m_connected.wait(INFINITE);
   }

   nxlog_debug_tag(DEBUG_TAG, 5, L"Script execution %u started", m_id);

   // Stop request received before VM started would be lost because VM resets stop flag on start
   m_vmLock.lock();
   bool stopRequested = m_stopRequested;
   m_vmLock.unlock();

   bool success = !stopRequested && m_vm->run(m_args);

   json_t *msg = json_object();
   if (success)
   {
      json_object_set_new(msg, "type", json_string("result"));
      json_object_set_new(msg, "data", ScriptResultToJson(m_vm->getResult(), m_resultAsMap));
      sendMessage(msg);
      json_decref(msg);

      msg = json_object();
      json_object_set_new(msg, "type", json_string("completed"));
   }
   else
   {
      m_vmLock.lock();
      stopRequested = m_stopRequested;
      m_vmLock.unlock();

      if (stopRequested)
      {
         json_object_set_new(msg, "type", json_string("stopped"));
      }
      else
      {
         json_object_set_new(msg, "type", json_string("error"));
         json_object_set_new(msg, "message", json_string("Script execution failed"));
         json_object_set_new(msg, "diagnostic", m_vm->getErrorJson());
      }
   }
   sendMessage(msg);
   json_decref(msg);

   nxlog_debug_tag(DEBUG_TAG, 5, L"Script execution %u %s", m_id, success ? L"completed" : (stopRequested ? L"stopped" : L"failed"));

   m_vmLock.lock();
   delete_and_null(m_vm);
   m_vmLock.unlock();

   sendCloseFrame();
   s_executions.remove(m_id);
}

/**
 * Stop script execution
 */
void ScriptExecution::stop()
{
   LockGuard lockGuard(m_vmLock);
   m_stopRequested = true;
   if (m_vm != nullptr)
      m_vm->stop();
}

/**
 * Send JSON message to WebSocket client
 */
void ScriptExecution::sendMessage(json_t *msg)
{
   char *encoded = json_dumps(msg, 0);
   m_socketLock.lock();
   if ((m_socket != INVALID_SOCKET) && !m_closeFrameSent && !m_disconnected)
      SendWebsocketFrame(m_socket, encoded, strlen(encoded));
   m_socketLock.unlock();
   MemFree(encoded);
}

/**
 * Send script output to WebSocket client
 */
void ScriptExecution::sendOutput(const wchar_t *text)
{
   json_t *msg = json_object();
   json_object_set_new(msg, "type", json_string("output"));
   json_object_set_new(msg, "data", json_string_w(text));
   sendMessage(msg);
   json_decref(msg);
}

/**
 * Send close frame after execution completion and shut down socket so that reader thread will finish
 */
void ScriptExecution::sendCloseFrame()
{
   m_socketLock.lock();
   if (!m_closeFrameSent && !m_disconnected)
   {
      SendWebsocketCloseFrame(m_socket, WS_CLOSE_NORMAL);
      shutdown(m_socket, SHUT_RDWR);
   }
   m_closeFrameSent = true;
   m_socketLock.unlock();
}

/**
 * Attach WebSocket connection to execution
 */
void ScriptExecution::connect(MHD_UpgradeResponseHandle *responseHandle, SOCKET s)
{
   m_socketLock.lock();
   m_responseHandle = responseHandle;
   m_socket = s;
   m_socketLock.unlock();
   m_connected.set();
}

/**
 * Read frames from WebSocket client until connection is closed. Closing connection by client stops script.
 */
void ScriptExecution::readFrames()
{
   while(true)
   {
      ByteStream buffer;
      BYTE frameType;
      if (!ReadWebsocketFrame(m_socket, &buffer, &frameType))
      {
         nxlog_debug_tag(DEBUG_TAG, 5, L"Script execution %u: WebSocket read error or connection closed", m_id);
         break;
      }

      if (frameType == 0x08)  // Close frame
      {
         nxlog_debug_tag(DEBUG_TAG, 5, L"Script execution %u: received close frame", m_id);
         break;
      }
      else if (frameType == 0x09)  // Ping frame
      {
         m_socketLock.lock();
         if (!m_closeFrameSent)
         {
            BYTE pong[2] = { 0x8A, 0x00 };  // FIN + pong opcode, no payload
            SendEx(m_socket, pong, 2, 0, nullptr);
         }
         m_socketLock.unlock();
      }
      else if (frameType == 0x01)  // Text frame
      {
         json_error_t error;
         json_t *msg = json_loadb(reinterpret_cast<const char*>(buffer.buffer()), buffer.size(), 0, &error);
         const char *type = json_object_get_string_utf8(msg, "type", "");
         if (!strcmp(type, "stop"))
         {
            nxlog_debug_tag(DEBUG_TAG, 5, L"Script execution %u: stop requested by client", m_id);
            stop();
         }
         else
         {
            json_t *response = json_object();
            json_object_set_new(response, "type", json_string("error"));
            json_object_set_new(response, "message", json_string("Invalid message"));
            sendMessage(response);
            json_decref(response);
         }
         json_decref(msg);
      }
      // Ignore other frame types
   }

   stop();  // Nobody will receive output after client disconnect

   m_socketLock.lock();
   if (!m_closeFrameSent)
      SendWebsocketCloseFrame(m_socket, WS_CLOSE_NORMAL);
   m_disconnected = true;
   m_socketLock.unlock();

   MHD_upgrade_action(m_responseHandle, MHD_UPGRADE_ACTION_CLOSE);
   nxlog_debug_tag(DEBUG_TAG, 5, L"Script execution %u: WebSocket connection closed", m_id);
}

/**
 * Handler for /v1/objects/:object-id/execute-script
 */
int H_ObjectExecuteScript(Context *context)
{
   uint32_t objectId = context->getPlaceholderValueAsUInt32(L"object-id");
   if (objectId == 0)
      return 400;

   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object == nullptr)
      return 404;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, L"H_ObjectExecuteScript: empty request");
      return 400;
   }

   unique_cstring_ptr script(json_object_get_string_t(request, "script", nullptr));
   if (script == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, L"H_ObjectExecuteScript: missing script source code");
      return 400;
   }

   if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_EXECUTE_SCRIPT))
   {
      context->writeAuditLogWithValues(AUDIT_OBJECTS, false, object->getId(), nullptr, script.get(), 'T', L"Access denied on ad-hoc script execution for object %s [%u]", object->getName(), object->getId());
      return 403;
   }

   bool resultAsMap = json_object_get_boolean(request, "resultAsMap", false);
   shared_ptr<ScriptExecution> execution = json_object_get_boolean(request, "stream", false) ?
            make_shared<ScriptExecution>(context->getUserId(), resultAsMap) : shared_ptr<ScriptExecution>();

   NXSL_CompilationDiagnostic diag;
   NXSL_VM *vm = NXSLCompileAndCreateVM(script.get(), (execution != nullptr) ? new NXSL_ScriptExecutionEnv(execution.get()) : new NXSL_ServerEnv(), &diag);
   if (vm == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, L"H_ObjectExecuteScript: script compilation error (%s)", diag.errorText.cstr());
      json_t *response = json_object();
      json_object_set_new(response, "reason", json_string("Script compilation failed"));
      json_object_set_new(response, "diagnostic", diag.toJson());
      context->setResponseData(response);
      json_decref(response);
      return 400;
   }

   SetupServerScriptVM(vm, object, shared_ptr<DCObjectInfo>());
   // Streaming execution outlives request context
   vm->setSecurityContext((execution != nullptr) ? new NXSL_UserSecurityContext(context->getUserId()) : new NXSL_UserSecurityContext(context));
   context->writeAuditLogWithValues(AUDIT_OBJECTS, true, object->getId(), nullptr, script.get(), 'T', L"Executed ad-hoc script for object %s [%u]", object->getName(), object->getId());

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

   if (execution != nullptr)
   {
      uuid token = execution->start(vm, sargs);
      nxlog_debug_tag(DEBUG_TAG, 4, L"Script execution %u for object %s [%u] created by user %s [%u] (WebSocket pending)",
            execution->getId(), object->getName(), object->getId(), context->getLoginName(), context->getUserId());

      json_t *response = json_object();
      json_object_set_new(response, "executionId", json_integer(execution->getId()));
      char tokenStr[64];
      token.toStringA(tokenStr);
      json_object_set_new(response, "token", json_string(tokenStr));
      json_object_set_new(response, "expiresIn", json_integer(SCRIPT_OUTPUT_TOKEN_VALIDITY));
      char wsUrl[128];
      snprintf(wsUrl, sizeof(wsUrl), "/v1/script-executions/output/%s", tokenStr);
      json_object_set_new(response, "wsUrl", json_string(wsUrl));
      context->setResponseData(response);
      json_decref(response);
      return 202;
   }

   int responseCode;
   if (vm->run(sargs))
   {
      responseCode = 200;
      json_t *response = json_object();
      json_object_set_new(response, "result", ScriptResultToJson(vm->getResult(), resultAsMap));
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
 * Handler for POST /v1/script-executions/:execution-id/stop
 */
int H_ScriptExecutionStop(Context *context)
{
   uint32_t executionId = context->getPlaceholderValueAsUInt32(L"execution-id");
   if (executionId == 0)
      return 400;

   shared_ptr<ScriptExecution> execution = s_executions.getShared(executionId);
   if (execution == nullptr)
      return 404;

   if (execution->getUserId() != context->getUserId())
      return 403;

   nxlog_debug_tag(DEBUG_TAG, 4, L"Script execution %u: stop requested by user %s [%u]", executionId, context->getLoginName(), context->getUserId());
   execution->stop();
   return 204;
}

/**
 * Reject WebSocket connection
 */
static void RejectConnection(MHD_socket sock, MHD_UpgradeResponseHandle *responseHandle, const wchar_t *reason)
{
   nxlog_debug_tag(DEBUG_TAG, 4, L"Script output WebSocket connection rejected: %s", reason);
   SendWebsocketCloseFrame(static_cast<SOCKET>(sock), WS_CLOSE_POLICY_VIOLATION);
   MHD_upgrade_action(responseHandle, MHD_UPGRADE_ACTION_CLOSE);
}

/**
 * WebSocket upgrade handler for script output streaming
 */
void WS_ScriptOutputConnect(void *cls, MHD_Connection *connection, void *con_cls,
                            const char *extra_in, size_t extra_in_size, MHD_socket sock,
                            MHD_UpgradeResponseHandle *responseHandle)
{
   Context *context = static_cast<Context*>(cls);

   const wchar_t *tokenStr = context->getPlaceholderValue(L"token");
   if (tokenStr == nullptr)
   {
      RejectConnection(sock, responseHandle, L"no token provided");
      return;
   }

   uuid token = uuid::parse(tokenStr);
   if (token.isNull())
   {
      RejectConnection(sock, responseHandle, L"invalid token format");
      return;
   }

   // Token is single-use; it is removed by execution thread when not used within validity period
   s_pendingConnectionsLock.lock();
   shared_ptr<ScriptExecution> execution = s_pendingConnections.getShared(token);
   if (execution != nullptr)
      s_pendingConnections.remove(token);
   s_pendingConnectionsLock.unlock();

   if (execution == nullptr)
   {
      RejectConnection(sock, responseHandle, L"token not found");
      return;
   }

   execution->connect(responseHandle, static_cast<SOCKET>(sock));
   nxlog_debug_tag(DEBUG_TAG, 4, L"Script output WebSocket connection established for execution %u", execution->getId());

   ThreadCreate([execution] () -> void { execution->readFrames(); });
}
