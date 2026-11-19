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
class ScriptExecution : public WebSocketOutputSession, public enable_shared_from_this<ScriptExecution>
{
private:
   uint32_t m_id;
   uint32_t m_userId;
   bool m_resultAsMap;
   NXSL_VM *m_vm;
   ObjectRefArray<NXSL_Value> m_args;
   bool m_stopRequested;
   Mutex m_vmLock;

   void run();

protected:
   virtual void onTextFrame(json_t *msg) override;
   virtual void onDisconnect() override;

public:
   ScriptExecution(uint32_t userId, bool resultAsMap);
   virtual ~ScriptExecution();

   uint32_t getId() const { return m_id; }
   uint32_t getUserId() const { return m_userId; }

   uuid start(NXSL_VM *vm, const ObjectRefArray<NXSL_Value>& args);
   void stop();
};

/**
 * Running script executions
 */
static SynchronizedSharedHashMap<uint32_t, ScriptExecution> s_executions;

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
ScriptExecution::ScriptExecution(uint32_t userId, bool resultAsMap) : m_vmLock(MutexType::FAST)
{
   m_id = InterlockedIncrement(&s_executionId);
   m_userId = userId;
   m_resultAsMap = resultAsMap;
   m_vm = nullptr;
   m_stopRequested = false;
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

   shared_ptr<ScriptExecution> self = shared_from_this();
   s_executions.set(m_id, self);
   uuid token = RegisterWebSocketOutputSession(self);

   ThreadCreate([self] () -> void { self->run(); });
   return token;
}

/**
 * Wait for client connection and execute script
 */
void ScriptExecution::run()
{
   if (!waitForConnection())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Script execution %u cancelled: client did not connect within %d seconds", m_id, WS_OUTPUT_TOKEN_VALIDITY);
      s_executions.remove(m_id);
      return;
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

   nxlog_debug_tag(DEBUG_TAG, 5, L"Script execution %u %s", m_id, success ? L"completed" : (stopRequested ? L"stopped" : L"failed"));

   m_vmLock.lock();
   delete_and_null(m_vm);
   m_vmLock.unlock();

   finish(msg);
   json_decref(msg);
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
 * Handle message from WebSocket client
 */
void ScriptExecution::onTextFrame(json_t *msg)
{
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
}

/**
 * Handle client disconnect. Closing connection by client stops script, because nobody will receive output.
 */
void ScriptExecution::onDisconnect()
{
   nxlog_debug_tag(DEBUG_TAG, 5, L"Script execution %u: WebSocket connection closed", m_id);
   stop();
}

/**
 * Execute ad-hoc script from request. Object is optional and used as script context if provided.
 * Caller is responsible for access rights check.
 */
static int ExecuteScript(Context *context, json_t *request, const wchar_t *script, const shared_ptr<NetObj>& object)
{
   bool resultAsMap = json_object_get_boolean(request, "resultAsMap", false);
   shared_ptr<ScriptExecution> execution = json_object_get_boolean(request, "stream", false) ?
            make_shared<ScriptExecution>(context->getUserId(), resultAsMap) : shared_ptr<ScriptExecution>();

   NXSL_CompilationDiagnostic diag;
   NXSL_VM *vm = NXSLCompileAndCreateVM(script, (execution != nullptr) ? new NXSL_ScriptExecutionEnv(execution.get()) : new NXSL_ServerEnv(), &diag);
   if (vm == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, L"ExecuteScript: script compilation error (%s)", diag.errorText.cstr());
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
   if (object != nullptr)
      context->writeAuditLogWithValues(AUDIT_OBJECTS, true, object->getId(), nullptr, script, 'T', L"Executed ad-hoc script for object %s [%u]", object->getName(), object->getId());
   else
      context->writeAuditLogWithValues(AUDIT_SYSCFG, true, 0, nullptr, script, 'T', L"Executed ad-hoc script without object context");

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
            execution->getId(), (object != nullptr) ? object->getName() : L"(none)", (object != nullptr) ? object->getId() : 0, context->getLoginName(), context->getUserId());

      json_t *response = CreateWebSocketOutputResponse(token, "/v1/script-executions/output");
      json_object_set_new(response, "executionId", json_integer(execution->getId()));
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

   return ExecuteScript(context, request, script.get(), object);
}

/**
 * Handler for /v1/execute-script. Script is executed without object context. Requires script management
 * rights - users with those rights can already run arbitrary code with system privileges via hook scripts.
 */
int H_ExecuteScript(Context *context)
{
   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, L"H_ExecuteScript: empty request");
      return 400;
   }

   unique_cstring_ptr script(json_object_get_string_t(request, "script", nullptr));
   if (script == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, L"H_ExecuteScript: missing script source code");
      return 400;
   }

   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_SCRIPTS))
   {
      context->writeAuditLogWithValues(AUDIT_SYSCFG, false, 0, nullptr, script.get(), 'T', L"Access denied on ad-hoc script execution without object context");
      return 403;
   }

   return ExecuteScript(context, request, script.get(), shared_ptr<NetObj>());
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
 * WebSocket upgrade handler for script output streaming
 */
void WS_ScriptOutputConnect(void *cls, MHD_Connection *connection, void *con_cls,
                            const char *extra_in, size_t extra_in_size, MHD_socket sock,
                            MHD_UpgradeResponseHandle *responseHandle)
{
   ConnectWebSocketOutputSession(static_cast<Context*>(cls), sock, responseHandle);
}
