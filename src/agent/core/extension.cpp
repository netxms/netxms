/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: extension.cpp
**
**/

#include "nxagentd.h"
#include "extension.h"
#include <netxms-version.h>

#define DEBUG_TAG   DEBUG_TAG_EXTENSION

#define MAX_LINE_LENGTH       (1024 * 1024)
#define READ_CHUNK_SIZE       16384
#define PING_INTERVAL_MS      30000
#define PING_TIMEOUT_MS       10000
#define PING_MAX_STRIKES      3

/**
 * JSON-RPC error codes — both standard and protocol-specific
 */
#define JSONRPC_ERR_PARSE_ERROR       (-32700)
#define JSONRPC_ERR_INVALID_REQUEST   (-32600)
#define JSONRPC_ERR_METHOD_NOT_FOUND  (-32601)
#define JSONRPC_ERR_INVALID_PARAMS    (-32602)
#define JSONRPC_ERR_INTERNAL          (-32603)
#define JSONRPC_ERR_UNAUTHORIZED      (-32001)
#define JSONRPC_ERR_NO_SUCH_INSTANCE  (-32010)
#define JSONRPC_ERR_METRIC_ERROR      (-32011)

/**
 * Map a JSON-RPC error code to an agent metric return code.
 */
static uint32_t MapJsonRpcError(int code)
{
   switch (code)
   {
      case JSONRPC_ERR_METHOD_NOT_FOUND:
      case JSONRPC_ERR_INVALID_PARAMS:
         return ERR_UNKNOWN_METRIC;
      case JSONRPC_ERR_NO_SUCH_INSTANCE:
         return ERR_NO_SUCH_INSTANCE;
      case JSONRPC_ERR_METRIC_ERROR:
      case JSONRPC_ERR_INTERNAL:
      default:
         return ERR_INTERNAL_ERROR;
   }
}

/**
 * Parse a "type" string from list_capabilities into a DCI_DT_* constant.
 */
static int ParseDataType(const char *type)
{
   if (type == nullptr)
      return DCI_DT_STRING;
   if (!strcmp(type, "int") || !strcmp(type, "int32"))
      return DCI_DT_INT;
   if (!strcmp(type, "uint") || !strcmp(type, "uint32"))
      return DCI_DT_UINT;
   if (!strcmp(type, "int64"))
      return DCI_DT_INT64;
   if (!strcmp(type, "uint64"))
      return DCI_DT_UINT64;
   if (!strcmp(type, "float") || !strcmp(type, "double"))
      return DCI_DT_FLOAT;
   if (!strcmp(type, "counter32"))
      return DCI_DT_COUNTER32;
   if (!strcmp(type, "counter64"))
      return DCI_DT_COUNTER64;
   return DCI_DT_STRING;
}

/**
 * Copy a UTF-8 JSON string field into a caller-supplied TCHAR buffer.
 * Returns false on missing/non-string field.
 */
static bool JsonStringToBuffer(json_t *obj, const char *key, TCHAR *buffer, size_t bufferSize)
{
   const char *s = json_object_get_string_utf8(obj, key, nullptr);
   if (s == nullptr)
      return false;
#ifdef UNICODE
   utf8_to_wchar(s, -1, buffer, bufferSize);
#else
   utf8_to_mb(s, -1, buffer, bufferSize);
#endif
   buffer[bufferSize - 1] = 0;
   return true;
}

/**
 * PendingExtRequest destructor — release any held JSON
 */
PendingExtRequest::~PendingExtRequest()
{
   if (result != nullptr)
      json_decref(result);
   if (error != nullptr)
      json_decref(error);
}

/**
 * Constructor
 */
AgentExtension::AgentExtension(const ExtensionConfig& cfg) :
      m_config(cfg),
      m_stopCondition(true),
      m_pending(Ownership::False),
      m_parameters(0, 16),
      m_lists(0, 8),
      m_tables(0, 8),
      m_actions(0, 8)
{
   m_state = ExtensionState::IDLE;
   m_socket = INVALID_SOCKET;
   m_process = nullptr;
   m_assignedPort = 0;
   m_supervisorThread = INVALID_THREAD_HANDLE;
   m_nextRequestId = 1;
   m_handshakeTime = 0;
   m_restartCount = 0;
   m_currentBackoffMs = 0;
   m_stopRequested = false;
   m_capabilitiesDirty = false;
   m_lastPingSentMs = 0;
   m_lastPingReplyMs = 0;
   m_pingStrikes = 0;
   m_inbuffer = new ByteStream(READ_CHUNK_SIZE * 2);
}

/**
 * Destructor
 */
AgentExtension::~AgentExtension()
{
   stop(false);
   delete m_inbuffer;
}

/**
 * Update state under lock
 */
void AgentExtension::setState(ExtensionState newState)
{
   LockGuard guard(m_stateLock);
   m_state = newState;
}

/**
 * Record last error message for observability
 */
void AgentExtension::setLastError(const TCHAR *fmt, ...)
{
   TCHAR buffer[1024];
   va_list args;
   va_start(args, fmt);
   _vsntprintf(buffer, 1024, fmt, args);
   va_end(args);
   buffer[1023] = 0;
   m_lastError = buffer;
   nxlog_debug_tag(getDebugTag(), 3, _T("Extension(%s): %s"), m_config.name, buffer);
}

/**
 * Start the extension — launches the supervisor thread.
 */
void AgentExtension::start()
{
   nxlog_debug_tag(getDebugTag(), 2, _T("Extension(%s): start requested (mode=%s)"),
            m_config.name,
            (m_config.mode == ExtensionMode::SPAWN) ? _T("spawn") : _T("connect"));
   m_stopRequested = false;
   m_stopCondition.reset();
   setState(ExtensionState::IDLE);
   m_supervisorThread = ThreadCreateEx(this, &AgentExtension::supervisorLoop);
}

/**
 * Stop the extension and wait for the supervisor to exit.
 */
void AgentExtension::stop(bool restart)
{
   nxlog_debug_tag(getDebugTag(), 2, _T("Extension(%s): stop requested (restart=%s)"),
            m_config.name, restart ? _T("yes") : _T("no"));
   m_stopRequested = true;
   m_stopCondition.set();    // wake any backoff sleep
   if (m_socket != INVALID_SOCKET)
      shutdown(m_socket, SHUT_RDWR);
   if (m_supervisorThread != INVALID_THREAD_HANDLE)
   {
      ThreadJoin(m_supervisorThread);
      m_supervisorThread = INVALID_THREAD_HANDLE;
   }
   setState(ExtensionState::DEAD);
}

/**
 * Main I/O loop.
 *
 * Pre-conditions: m_socket is a connected TCP socket, set up by the supervisor
 * (Phase 3).  The loop performs the JSON-RPC handshake, fetches capabilities,
 * then services the bidirectional stream until disconnect or stop.
 */
void AgentExtension::ioLoop()
{
   ThreadSetName("ext-io");

   if (m_socket == INVALID_SOCKET)
   {
      setLastError(_T("ioLoop entered with no socket"));
      onDisconnect(_T("internal error: no socket"));
      return;
   }

   setState(ExtensionState::HANDSHAKING);
   if (!performHandshake())
   {
      onDisconnect(_T("handshake failed"));
      return;
   }
   if (!fetchCapabilities())
   {
      onDisconnect(_T("list_capabilities failed"));
      return;
   }
   setState(ExtensionState::READY);
   m_handshakeTime = time(nullptr);
   m_lastPingSentMs = GetCurrentTimeMs();
   m_lastPingReplyMs = m_lastPingSentMs;
   m_pingStrikes = 0;
   nxlog_debug_tag(getDebugTag(), 2, _T("Extension(%s): handshake complete, %d metrics / %d lists / %d tables / %d actions"),
            m_config.name, m_parameters.size(), m_lists.size(), m_tables.size(), m_actions.size());

   while (!m_stopRequested)
   {
      if (!readAndDispatch(1000))
      {
         onDisconnect(_T("read error or peer closed"));
         return;
      }
      if (m_capabilitiesDirty)
      {
         m_capabilitiesDirty = false;
         nxlog_debug_tag(getDebugTag(), 4, _T("Extension(%s): re-fetching capabilities"), m_config.name);
         clearCapabilities();
         if (!fetchCapabilities())
         {
            onDisconnect(_T("list_capabilities failed after capabilities_changed"));
            return;
         }
      }
      maybeSendPing(GetCurrentTimeMs());
   }

   onDisconnect(_T("stop requested"));
}

/**
 * Send a JSON-RPC envelope (request, response, or notification).  Caller retains
 * ownership of `msg` and is responsible for json_decref.
 */
bool AgentExtension::sendJsonRpc(json_t *msg)
{
   if (m_socket == INVALID_SOCKET)
      return false;

   char *encoded = json_dumps(msg, JSON_COMPACT);
   if (encoded == nullptr)
   {
      setLastError(_T("json_dumps failed"));
      return false;
   }

   size_t len = strlen(encoded);
   if (nxlog_get_debug_level_tag(getDebugTag()) >= 7)
      nxlog_debug_tag(getDebugTag(), 7, _T("Extension(%s): -> %hs"), m_config.name, encoded);

   ssize_t rc = SendEx(m_socket, encoded, len, 0, &m_sendLock, m_config.requestTimeoutMs);
   bool ok = (rc == static_cast<ssize_t>(len));
   if (ok)
   {
      const char nl = '\n';
      rc = SendEx(m_socket, &nl, 1, 0, &m_sendLock, m_config.requestTimeoutMs);
      ok = (rc == 1);
   }
   MemFree(encoded);
   return ok;
}

/**
 * Issue a JSON-RPC request and wait for the response.
 * Takes ownership of `params`; returns owned result (caller must json_decref) or nullptr on error.
 */
json_t *AgentExtension::call(const char *method, json_t *params, uint32_t timeoutMs)
{
   uint32_t id;
   {
      LockGuard guard(m_pendingLock);
      id = m_nextRequestId++;
      if (id == 0)
         id = m_nextRequestId++;
   }

   PendingExtRequest *req = new PendingExtRequest(id);
   {
      LockGuard guard(m_pendingLock);
      m_pending.set(id, req);
   }

   json_t *envelope = json_object();
   json_object_set_new(envelope, "jsonrpc", json_string("2.0"));
   json_object_set_new(envelope, "id", json_integer(id));
   json_object_set_new(envelope, "method", json_string(method));
   if (params != nullptr)
      json_object_set_new(envelope, "params", params);    // takes ownership

   bool sent = sendJsonRpc(envelope);
   json_decref(envelope);

   if (!sent)
   {
      LockGuard guard(m_pendingLock);
      m_pending.unlink(id);
      delete req;
      return nullptr;
   }

   bool signalled = req->completion.wait(timeoutMs);
   {
      LockGuard guard(m_pendingLock);
      m_pending.unlink(id);
   }

   if (!signalled || !req->completed)
   {
      setLastError(_T("call(%hs) timed out after %u ms"), method, timeoutMs);
      delete req;
      return nullptr;
   }

   if (req->error != nullptr)
   {
      // surface the error to caller's log; return nullptr (caller maps via dispatch)
      json_t *codeNode = json_object_get(req->error, "code");
      json_t *msgNode = json_object_get(req->error, "message");
      int code = json_is_integer(codeNode) ? static_cast<int>(json_integer_value(codeNode)) : 0;
      const char *errMsg = json_is_string(msgNode) ? json_string_value(msgNode) : "";
      nxlog_debug_tag(getDebugTag(), 5, _T("Extension(%s): %hs returned error %d (%hs)"),
               m_config.name, method, code, errMsg);
      delete req;
      return nullptr;
   }

   json_t *result = req->result;
   req->result = nullptr;     // detach; caller now owns
   delete req;
   return result;
}

/**
 * Synchronous request used during the handshake phase, before the main read
 * loop is running.  Sends the request, then pumps readAndDispatch() on the I/O
 * thread itself until the matching response arrives (or timeout).  Must only be
 * called from the I/O thread (no other thread touches the socket at that point).
 * Takes ownership of `params`; returns owned result or nullptr on error/timeout.
 */
json_t *AgentExtension::callSync(const char *method, json_t *params, uint32_t timeoutMs)
{
   uint32_t id;
   {
      LockGuard guard(m_pendingLock);
      id = m_nextRequestId++;
      if (id == 0)
         id = m_nextRequestId++;
   }

   PendingExtRequest *req = new PendingExtRequest(id);
   {
      LockGuard guard(m_pendingLock);
      m_pending.set(id, req);
   }

   json_t *envelope = json_object();
   json_object_set_new(envelope, "jsonrpc", json_string("2.0"));
   json_object_set_new(envelope, "id", json_integer(id));
   json_object_set_new(envelope, "method", json_string(method));
   if (params != nullptr)
      json_object_set_new(envelope, "params", params);
   bool sent = sendJsonRpc(envelope);
   json_decref(envelope);

   if (!sent)
   {
      LockGuard guard(m_pendingLock);
      m_pending.unlink(id);
      delete req;
      return nullptr;
   }

   int64_t deadline = GetCurrentTimeMs() + timeoutMs;
   bool ioOk = true;
   while (!req->completed && !m_stopRequested)
   {
      int64_t remaining = deadline - GetCurrentTimeMs();
      if (remaining <= 0)
         break;
      if (!readAndDispatch(static_cast<uint32_t>(std::min<int64_t>(remaining, 500))))
      {
         ioOk = false;
         break;
      }
   }

   {
      LockGuard guard(m_pendingLock);
      m_pending.unlink(id);
   }

   if (!req->completed)
   {
      setLastError(ioOk ? _T("callSync(%hs) timed out after %u ms") : _T("callSync(%hs) failed: connection lost"),
               method, timeoutMs);
      delete req;
      return nullptr;
   }
   if (req->error != nullptr)
   {
      json_t *codeNode = json_object_get(req->error, "code");
      json_t *msgNode = json_object_get(req->error, "message");
      int code = json_is_integer(codeNode) ? static_cast<int>(json_integer_value(codeNode)) : 0;
      const char *errMsg = json_is_string(msgNode) ? json_string_value(msgNode) : "";
      nxlog_debug_tag(getDebugTag(), 5, _T("Extension(%s): %hs returned error %d (%hs)"),
               m_config.name, method, code, errMsg);
      delete req;
      return nullptr;
   }

   json_t *result = req->result;
   req->result = nullptr;
   delete req;
   return result;
}

/**
 * Send a JSON-RPC notification (no response expected).  Takes ownership of `params`.
 */
void AgentExtension::notify(const char *method, json_t *params)
{
   json_t *envelope = json_object();
   json_object_set_new(envelope, "jsonrpc", json_string("2.0"));
   json_object_set_new(envelope, "method", json_string(method));
   if (params != nullptr)
      json_object_set_new(envelope, "params", params);
   sendJsonRpc(envelope);
   json_decref(envelope);
}

/**
 * Read available bytes (up to one timeout slice) and dispatch any complete
 * lines.  Returns false on hard error / EOF.
 */
bool AgentExtension::readAndDispatch(uint32_t timeoutMs)
{
   ByteStream& buffer = *m_inbuffer;

   BYTE chunk[READ_CHUNK_SIZE];
   ssize_t n = RecvEx(m_socket, chunk, sizeof(chunk), 0, timeoutMs);
   if (n == -2)
      return true;       // timeout — nothing to do, keep going
   if (n <= 0)
      return false;      // 0 = peer closed, -1 = socket error
   buffer.write(chunk, static_cast<size_t>(n));

   if (buffer.size() > MAX_LINE_LENGTH)
   {
      setLastError(_T("inbound line buffer exceeded %u bytes"), MAX_LINE_LENGTH);
      buffer.clear();
      return false;
   }

   // scan for complete lines
   while (buffer.size() > 0)
   {
      const BYTE *data = buffer.buffer();
      size_t total = buffer.size();
      const BYTE *nl = static_cast<const BYTE*>(memchr(data, '\n', total));
      if (nl == nullptr)
         break;
      size_t lineLen = static_cast<size_t>(nl - data);
      handleIncomingLine(reinterpret_cast<const char*>(data), lineLen);
      buffer.truncateLeft(lineLen + 1);
   }
   return true;
}

/**
 * Parse a single inbound JSON-RPC line and route to handleResponse / handleNotification.
 */
void AgentExtension::handleIncomingLine(const char *line, size_t length)
{
   if (length == 0)
      return;

   if (nxlog_get_debug_level_tag(getDebugTag()) >= 7)
   {
      char snippet[256];
      size_t n = std::min(length, sizeof(snippet) - 1);
      memcpy(snippet, line, n);
      snippet[n] = 0;
      nxlog_debug_tag(getDebugTag(), 7, _T("Extension(%s): <- %hs%s"),
               m_config.name, snippet, (length > n) ? "..." : "");
   }

   json_error_t err;
   json_t *root = json_loadb(line, length, 0, &err);
   if (root == nullptr)
   {
      nxlog_debug_tag(getDebugTag(), 4, _T("Extension(%s): JSON parse error at col %d: %hs"),
               m_config.name, err.column, err.text);
      return;
   }

   json_t *idNode = json_object_get(root, "id");
   json_t *methodNode = json_object_get(root, "method");

   if (json_is_integer(idNode) && (json_object_get(root, "result") != nullptr ||
                                   json_object_get(root, "error") != nullptr))
   {
      // response
      uint32_t id = static_cast<uint32_t>(json_integer_value(idNode));
      json_t *result = json_object_get(root, "result");
      json_t *error = json_object_get(root, "error");
      if (result != nullptr)
         json_incref(result);
      if (error != nullptr)
         json_incref(error);
      handleResponse(id, result, error);
   }
   else if (json_is_string(methodNode))
   {
      // request from extension (rare — e.g. 'ping' from the other side) or notification
      const char *method = json_string_value(methodNode);
      json_t *params = json_object_get(root, "params");
      if (params != nullptr)
         json_incref(params);
      if (idNode != nullptr && !json_is_null(idNode))
      {
         // Inbound *request* — only "ping" is supported in v1.
         if (!strcmp(method, "ping"))
         {
            json_t *resp = json_object();
            json_object_set_new(resp, "jsonrpc", json_string("2.0"));
            json_object_set(resp, "id", idNode);
            json_object_set_new(resp, "result", json_object());
            sendJsonRpc(resp);
            json_decref(resp);
         }
         else
         {
            json_t *resp = json_object();
            json_object_set_new(resp, "jsonrpc", json_string("2.0"));
            json_object_set(resp, "id", idNode);
            json_t *errObj = json_object();
            json_object_set_new(errObj, "code", json_integer(JSONRPC_ERR_METHOD_NOT_FOUND));
            json_object_set_new(errObj, "message", json_string("Unknown method"));
            json_object_set_new(resp, "error", errObj);
            sendJsonRpc(resp);
            json_decref(resp);
         }
         if (params != nullptr)
            json_decref(params);
      }
      else
      {
         handleNotification(method, params);    // takes ownership of params
      }
   }
   else
   {
      nxlog_debug_tag(getDebugTag(), 4, _T("Extension(%s): malformed JSON-RPC envelope"), m_config.name);
   }

   json_decref(root);
}

/**
 * Match an inbound response to a pending request and unblock the waiter.
 * Takes ownership of result / error.
 */
void AgentExtension::handleResponse(uint32_t id, json_t *result, json_t *error)
{
   PendingExtRequest *req;
   {
      LockGuard guard(m_pendingLock);
      req = m_pending.get(id);
   }

   if (req == nullptr)
   {
      // Not a tracked request — common case: ping responses we sent with a sentinel id.
      // Phase 2 simply discards.  (Phase 3 may track ping ids more strictly.)
      nxlog_debug_tag(getDebugTag(), 6, _T("Extension(%s): response for unknown id %u"), m_config.name, id);
      if (result != nullptr) json_decref(result);
      if (error != nullptr) json_decref(error);
      m_lastPingReplyMs = GetCurrentTimeMs();
      m_pingStrikes = 0;
      return;
   }

   req->result = result;
   req->error = error;
   req->completed = true;
   req->completion.set();
}

/**
 * Run the JSON-RPC `hello` handshake.
 */
bool AgentExtension::performHandshake()
{
   const TCHAR *token = m_runtimeToken.isEmpty() ? m_config.token.cstr() : m_runtimeToken.cstr();

   json_t *params = json_object();
   json_object_set_new(params, "protocol", json_integer(EXTENSION_PROTOCOL_VERSION));
   json_object_set_new(params, "agent", json_string("nxagentd/" NETXMS_VERSION_STRING_A));
#ifdef UNICODE
   char *tokenU = UTF8StringFromWideString(token);
   json_object_set_new(params, "token", json_string(tokenU));
   MemFree(tokenU);
#else
   json_object_set_new(params, "token", json_string(token));
#endif

   json_t *result = callSync("hello", params, m_config.requestTimeoutMs);
   if (result == nullptr)
   {
      setLastError(_T("hello call failed"));
      return false;
   }

   json_t *protoNode = json_object_get(result, "protocol");
   int proto = json_is_integer(protoNode) ? static_cast<int>(json_integer_value(protoNode)) : 0;
   if (proto < EXTENSION_PROTOCOL_VERSION)
   {
      setLastError(_T("extension reported protocol version %d, agent requires %d"),
               proto, EXTENSION_PROTOCOL_VERSION);
      json_decref(result);
      return false;
   }
   json_decref(result);
   return true;
}

/**
 * Issue list_capabilities and populate the descriptor arrays.
 */
bool AgentExtension::fetchCapabilities()
{
   json_t *result = callSync("list_capabilities", nullptr, m_config.requestTimeoutMs);
   if (result == nullptr)
   {
      setLastError(_T("list_capabilities call failed"));
      return false;
   }

   LockGuard guard(m_capabilitiesLock);

   json_t *metrics = json_object_get(result, "metrics");
   if (json_is_array(metrics))
   {
      size_t n = json_array_size(metrics);
      for (size_t i = 0; i < n; i++)
      {
         json_t *entry = json_array_get(metrics, i);
         if (!json_is_object(entry)) continue;
         NETXMS_SUBAGENT_PARAM p;
         memset(&p, 0, sizeof(p));
         JsonStringToBuffer(entry, "name", p.name, MAX_PARAM_NAME);
         json_t *typeNode = json_object_get(entry, "type");
         p.dataType = ParseDataType(json_is_string(typeNode) ? json_string_value(typeNode) : nullptr);
         JsonStringToBuffer(entry, "description", p.description, MAX_DB_STRING);
         m_parameters.add(p);
      }
   }

   json_t *lists = json_object_get(result, "lists");
   if (json_is_array(lists))
   {
      size_t n = json_array_size(lists);
      for (size_t i = 0; i < n; i++)
      {
         json_t *entry = json_array_get(lists, i);
         if (!json_is_object(entry)) continue;
         NETXMS_SUBAGENT_LIST l;
         memset(&l, 0, sizeof(l));
         JsonStringToBuffer(entry, "name", l.name, MAX_PARAM_NAME);
         JsonStringToBuffer(entry, "description", l.description, MAX_DB_STRING);
         m_lists.add(l);
      }
   }

   json_t *tables = json_object_get(result, "tables");
   if (json_is_array(tables))
   {
      size_t n = json_array_size(tables);
      for (size_t i = 0; i < n; i++)
      {
         json_t *entry = json_array_get(tables, i);
         if (!json_is_object(entry)) continue;
         NETXMS_SUBAGENT_TABLE t;
         memset(&t, 0, sizeof(t));
         JsonStringToBuffer(entry, "name", t.name, MAX_PARAM_NAME);
         JsonStringToBuffer(entry, "description", t.description, MAX_DB_STRING);
         // Build comma-separated list of instance columns (matches subagent ABI).
         json_t *cols = json_object_get(entry, "columns");
         if (json_is_array(cols))
         {
            StringBuffer instanceCols;
            size_t cn = json_array_size(cols);
            for (size_t j = 0; j < cn; j++)
            {
               json_t *c = json_array_get(cols, j);
               if (json_is_object(c) && json_object_get_boolean(c, "instance", false))
               {
                  TCHAR *colName = json_object_get_string_t(c, "name", nullptr);
                  if (colName != nullptr)
                  {
                     if (!instanceCols.isEmpty()) instanceCols.append(_T(","));
                     instanceCols.append(colName);
                     MemFree(colName);
                  }
               }
            }
            _tcslcpy(t.instanceColumns, instanceCols.cstr(), MAX_COLUMN_NAME * 4);
         }
         m_tables.add(t);
      }
   }

   json_t *actions = json_object_get(result, "actions");
   if (json_is_array(actions))
   {
      size_t n = json_array_size(actions);
      for (size_t i = 0; i < n; i++)
      {
         json_t *entry = json_array_get(actions, i);
         if (!json_is_object(entry)) continue;
         ACTION a;
         memset(&a, 0, sizeof(a));
         JsonStringToBuffer(entry, "name", a.name, MAX_PARAM_NAME);
         JsonStringToBuffer(entry, "description", a.description, MAX_DB_STRING);
         a.isExternal = false;
         m_actions.add(a);
      }
   }

   json_decref(result);
   return true;
}

/**
 * Drop all advertised capabilities (e.g. on disconnect or re-discovery)
 */
void AgentExtension::clearCapabilities()
{
   LockGuard guard(m_capabilitiesLock);
   m_parameters.clear();
   m_lists.clear();
   m_tables.clear();
   m_actions.clear();
}

/**
 * Connection lost — clean up and (Phase 3) trigger reconnect.
 */
void AgentExtension::onDisconnect(const TCHAR *reason)
{
   nxlog_debug_tag(getDebugTag(), 3, _T("Extension(%s): disconnected (%s)"), m_config.name, reason);
   clearCapabilities();

   // wake any callers blocked on pending requests
   {
      LockGuard guard(m_pendingLock);
      auto it = m_pending.begin();
      auto end = m_pending.end();
      while (it != end)
      {
         PendingExtRequest *req = *it;
         req->completed = true;
         req->completion.set();
         ++it;
      }
   }

   if (m_socket != INVALID_SOCKET)
   {
      closesocket(m_socket);
      m_socket = INVALID_SOCKET;
   }
   setState(ExtensionState::RECONNECT_BACKOFF);
   // TODO(phase 3): scheduleReconnect()
}

/**
 * Schedule a reconnect with exponential backoff. Phase 3.
 */
void AgentExtension::scheduleReconnect()
{
   // TODO(phase 3)
}

/**
 * Periodic ping management — called from the I/O loop on each tick.
 */
void AgentExtension::maybeSendPing(int64_t now)
{
   // Unanswered ping window — count strikes and disconnect if exhausted.
   if (m_lastPingSentMs > m_lastPingReplyMs && now - m_lastPingSentMs > PING_TIMEOUT_MS)
   {
      m_pingStrikes++;
      m_lastPingReplyMs = now;     // suppress repeated counts in the same window
      nxlog_debug_tag(getDebugTag(), 4, _T("Extension(%s): ping strike %u/%u"),
               m_config.name, m_pingStrikes, PING_MAX_STRIKES);
      if (m_pingStrikes >= PING_MAX_STRIKES)
      {
         m_stopRequested = true;
         return;
      }
   }

   if (now - m_lastPingSentMs < PING_INTERVAL_MS)
      return;

   uint32_t id;
   {
      LockGuard guard(m_pendingLock);
      id = m_nextRequestId++;
   }
   json_t *req = json_object();
   json_object_set_new(req, "jsonrpc", json_string("2.0"));
   json_object_set_new(req, "id", json_integer(id));
   json_object_set_new(req, "method", json_string("ping"));
   sendJsonRpc(req);
   json_decref(req);
   m_lastPingSentMs = now;
}

/**
 * Dispatch surface — Phase 2 wires through call().
 */
uint32_t AgentExtension::getMetric(const TCHAR *name, TCHAR *buffer)
{
   if (!isConnected())
      return ERR_UNKNOWN_METRIC;

   // Quick reject: must be one of our advertised metrics (match by base name before '(').
   {
      LockGuard guard(m_capabilitiesLock);
      bool found = false;
      for (int i = 0; i < m_parameters.size(); i++)
      {
         if (MatchString(m_parameters.get(i)->name, name, false))
         {
            found = true;
            break;
         }
      }
      if (!found)
         return ERR_UNKNOWN_METRIC;
   }

#ifdef UNICODE
   char *nameU = UTF8StringFromWideString(name);
#else
   char *nameU = MemCopyStringA(name);
#endif
   json_t *params = json_object();
   json_object_set_new(params, "name", json_string(nameU));
   MemFree(nameU);

   json_t *result = call("get_metric", params, m_config.requestTimeoutMs);
   if (result == nullptr)
      return ERR_INTERNAL_ERROR;

   if (!JsonStringToBuffer(result, "value", buffer, MAX_RESULT_LENGTH))
   {
      json_decref(result);
      return ERR_INTERNAL_ERROR;
   }
   json_decref(result);
   return ERR_SUCCESS;
}

/**
 * Get list value
 */
uint32_t AgentExtension::getList(const TCHAR *name, StringList *value)
{
   if (!isConnected())
      return ERR_UNKNOWN_METRIC;

   {
      LockGuard guard(m_capabilitiesLock);
      bool found = false;
      for (int i = 0; i < m_lists.size(); i++)
      {
         if (MatchString(m_lists.get(i)->name, name, false))
         {
            found = true;
            break;
         }
      }
      if (!found)
         return ERR_UNKNOWN_METRIC;
   }

#ifdef UNICODE
   char *nameU = UTF8StringFromWideString(name);
#else
   char *nameU = MemCopyStringA(name);
#endif
   json_t *params = json_object();
   json_object_set_new(params, "name", json_string(nameU));
   MemFree(nameU);

   json_t *result = call("get_list", params, m_config.requestTimeoutMs);
   if (result == nullptr)
      return ERR_INTERNAL_ERROR;

   json_t *vals = json_object_get(result, "values");
   if (!json_is_array(vals))
   {
      json_decref(result);
      return ERR_INTERNAL_ERROR;
   }
   size_t n = json_array_size(vals);
   for (size_t i = 0; i < n; i++)
   {
      json_t *e = json_array_get(vals, i);
      if (json_is_string(e))
      {
         const char *u = json_string_value(e);
#ifdef UNICODE
         value->addPreallocated(WideStringFromUTF8String(u));
#else
         value->add(u);
#endif
      }
   }
   json_decref(result);
   return ERR_SUCCESS;
}

/**
 * Get table value
 */
uint32_t AgentExtension::getTable(const TCHAR *name, Table *value)
{
   if (!isConnected())
      return ERR_UNKNOWN_METRIC;

   {
      LockGuard guard(m_capabilitiesLock);
      bool found = false;
      for (int i = 0; i < m_tables.size(); i++)
      {
         if (MatchString(m_tables.get(i)->name, name, false))
         {
            found = true;
            break;
         }
      }
      if (!found)
         return ERR_UNKNOWN_METRIC;
   }

#ifdef UNICODE
   char *nameU = UTF8StringFromWideString(name);
#else
   char *nameU = MemCopyStringA(name);
#endif
   json_t *params = json_object();
   json_object_set_new(params, "name", json_string(nameU));
   MemFree(nameU);

   json_t *result = call("get_table", params, m_config.requestTimeoutMs);
   if (result == nullptr)
      return ERR_INTERNAL_ERROR;

   json_t *cols = json_object_get(result, "columns");
   json_t *rows = json_object_get(result, "rows");
   if (!json_is_array(cols) || !json_is_array(rows))
   {
      json_decref(result);
      return ERR_INTERNAL_ERROR;
   }

   size_t cn = json_array_size(cols);
   for (size_t i = 0; i < cn; i++)
   {
      json_t *c = json_array_get(cols, i);
      if (!json_is_object(c)) continue;
      TCHAR *colName = json_object_get_string_t(c, "name", nullptr);
      int colType = ParseDataType(json_object_get_string_utf8(c, "type", nullptr));
      bool isInstance = json_object_get_boolean(c, "instance", false);
      value->addColumn(CHECK_NULL_EX(colName), colType, nullptr, isInstance);
      MemFree(colName);
   }

   size_t rn = json_array_size(rows);
   for (size_t i = 0; i < rn; i++)
   {
      json_t *row = json_array_get(rows, i);
      if (!json_is_array(row)) continue;
      value->addRow();
      size_t fieldCount = json_array_size(row);
      for (size_t j = 0; j < fieldCount && j < cn; j++)
      {
         json_t *f = json_array_get(row, j);
         if (json_is_string(f))
         {
            TCHAR *s;
#ifdef UNICODE
            s = WideStringFromUTF8String(json_string_value(f));
#else
            s = MBStringFromUTF8String(json_string_value(f));
#endif
            value->set(static_cast<int>(j), s);
            MemFree(s);
         }
         else if (json_is_integer(f))
         {
            value->set(static_cast<int>(j), static_cast<int64_t>(json_integer_value(f)));
         }
         else if (json_is_real(f))
         {
            value->set(static_cast<int>(j), json_real_value(f));
         }
      }
   }
   json_decref(result);
   return ERR_SUCCESS;
}

/**
 * Execute action
 */
uint32_t AgentExtension::executeAction(const TCHAR *name, const StringList& args, AbstractCommSession *session, uint32_t requestId, bool sendOutput)
{
   if (!isConnected())
      return ERR_UNKNOWN_METRIC;

   {
      LockGuard guard(m_capabilitiesLock);
      bool found = false;
      for (int i = 0; i < m_actions.size(); i++)
      {
         if (!_tcsicmp(m_actions.get(i)->name, name))
         {
            found = true;
            break;
         }
      }
      if (!found)
         return ERR_UNKNOWN_METRIC;
   }

   uint32_t id;
   {
      LockGuard guard(m_pendingLock);
      id = m_nextRequestId++;
      if (id == 0)
         id = m_nextRequestId++;
   }

   PendingExtRequest *req = new PendingExtRequest(id);
   if (sendOutput && session != nullptr)
   {
      req->outputSession = session;
      req->outputSessionRequestId = requestId;
   }
   {
      LockGuard guard(m_pendingLock);
      m_pending.set(id, req);
   }

#ifdef UNICODE
   char *nameU = UTF8StringFromWideString(name);
#else
   char *nameU = MemCopyStringA(name);
#endif
   json_t *params = json_object();
   json_object_set_new(params, "name", json_string(nameU));
   MemFree(nameU);

   json_t *argArr = json_array();
   for (int i = 0; i < args.size(); i++)
   {
#ifdef UNICODE
      char *a = UTF8StringFromWideString(args.get(i));
      json_array_append_new(argArr, json_string(a));
      MemFree(a);
#else
      json_array_append_new(argArr, json_string(args.get(i)));
#endif
   }
   json_object_set_new(params, "args", argArr);

   json_t *envelope = json_object();
   json_object_set_new(envelope, "jsonrpc", json_string("2.0"));
   json_object_set_new(envelope, "id", json_integer(id));
   json_object_set_new(envelope, "method", json_string("execute_action"));
   json_object_set_new(envelope, "params", params);
   bool sent = sendJsonRpc(envelope);
   json_decref(envelope);

   if (!sent)
   {
      LockGuard guard(m_pendingLock);
      m_pending.unlink(id);
      delete req;
      return ERR_INTERNAL_ERROR;
   }

   bool signalled = req->completion.wait(m_config.actionTimeoutMs);
   {
      LockGuard guard(m_pendingLock);
      m_pending.unlink(id);
   }

   uint32_t rc;
   if (!signalled || !req->completed)
   {
      rc = ERR_REQUEST_TIMEOUT;
   }
   else if (req->error != nullptr)
   {
      json_t *codeNode = json_object_get(req->error, "code");
      int code = json_is_integer(codeNode) ? static_cast<int>(json_integer_value(codeNode)) : 0;
      rc = MapJsonRpcError(code);
   }
   else
   {
      rc = ERR_SUCCESS;
   }

   delete req;
   return rc;
}

/**
 * Inbound notification dispatch.  Takes ownership of `params`.
 */
void AgentExtension::handleNotification(const char *method, json_t *params)
{
   if (!strcmp(method, "push_metric"))
   {
      handlePushMetric(params);
   }
   else if (!strcmp(method, "push_metrics"))
   {
      handlePushMetricsBatch(params);
   }
   else if (!strcmp(method, "send_event"))
   {
      handleSendEvent(params);
   }
   else if (!strcmp(method, "log"))
   {
      handleLog(params);
   }
   else if (!strcmp(method, "action_output"))
   {
      handleActionOutput(params);
   }
   else if (!strcmp(method, "capabilities_changed"))
   {
      m_capabilitiesDirty = true;
   }
   else if (!strcmp(method, "shutdown_notice"))
   {
      nxlog_debug_tag(getDebugTag(), 3, _T("Extension(%s): received shutdown_notice"), m_config.name);
      m_stopRequested = true;
   }
   else
   {
      nxlog_debug_tag(getDebugTag(), 5, _T("Extension(%s): unknown notification %hs"),
               m_config.name, method);
   }

   if (params != nullptr)
      json_decref(params);
}

/**
 * push_metric notification handler — forward to PushData() for routing.
 */
void AgentExtension::handlePushMetric(json_t *params)
{
   if (!json_is_object(params))
      return;
   TCHAR *name = json_object_get_string_t(params, "name", nullptr);
   TCHAR *value = json_object_get_string_t(params, "value", nullptr);
   if (name != nullptr && value != nullptr)
   {
      time_t ts = json_object_get_time(params, "timestamp", time(nullptr));
      PushData(name, value, 0, Timestamp::fromTime(ts));
   }
   MemFree(name);
   MemFree(value);
}

/**
 * push_metrics batch notification handler.
 */
void AgentExtension::handlePushMetricsBatch(json_t *params)
{
   if (!json_is_object(params))
      return;
   json_t *items = json_object_get(params, "items");
   if (!json_is_array(items))
      return;
   size_t n = json_array_size(items);
   for (size_t i = 0; i < n; i++)
   {
      json_t *entry = json_array_get(items, i);
      handlePushMetric(entry);    // entry is borrowed; handler doesn't decref
   }
}

/**
 * send_event notification handler.  Forwards to ForwardEvent() / PostEvent().
 */
void AgentExtension::handleSendEvent(json_t *params)
{
   if (!json_is_object(params))
      return;
   uint32_t code = json_object_get_uint32(params, "code", 0);
   TCHAR *eventName = json_object_get_string_t(params, "event", nullptr);

   StringMap argsMap;
   json_t *args = json_object_get(params, "args");
   if (json_is_array(args))
   {
      size_t n = json_array_size(args);
      for (size_t i = 0; i < n; i++)
      {
         json_t *a = json_array_get(args, i);
         if (json_is_string(a))
         {
            TCHAR key[16];
            _sntprintf(key, 16, _T("%u"), static_cast<unsigned>(i + 1));
#ifdef UNICODE
            WCHAR *w = WideStringFromUTF8String(json_string_value(a));
            argsMap.set(key, w);
            MemFree(w);
#else
            argsMap.set(key, json_string_value(a));
#endif
         }
      }
   }

   PostEvent(code, eventName, time(nullptr), argsMap);
   MemFree(eventName);
}

/**
 * log notification handler.
 */
void AgentExtension::handleLog(json_t *params)
{
   if (!json_is_object(params))
      return;
   const char *level = json_object_get_string_utf8(params, "level", "info");
   TCHAR *message = json_object_get_string_t(params, "message", nullptr);
   int lvl = 5;
   if (!stricmp(level, "error")) lvl = 1;
   else if (!stricmp(level, "warning")) lvl = 2;
   else if (!stricmp(level, "info")) lvl = 4;
   else if (!stricmp(level, "debug")) lvl = 6;
   if (message != nullptr)
      nxlog_debug_tag(getDebugTag(), lvl, _T("Extension(%s): %s"), m_config.name, message);
   MemFree(message);
}

/**
 * action_output notification — forward chunk to the originating session.
 */
void AgentExtension::handleActionOutput(json_t *params)
{
   if (!json_is_object(params))
      return;
   uint32_t reqId = json_object_get_uint32(params, "request_id", 0);
   if (reqId == 0)
      return;

   PendingExtRequest *req;
   {
      LockGuard guard(m_pendingLock);
      req = m_pending.get(reqId);
   }
   if (req == nullptr || req->outputSession == nullptr)
      return;

   TCHAR *chunk = json_object_get_string_t(params, "chunk", nullptr);
   if (chunk == nullptr)
      return;

   NXCPMessage msg(CMD_COMMAND_OUTPUT, req->outputSessionRequestId, req->outputSession->getProtocolVersion());
   msg.setField(VID_MESSAGE, chunk);
   req->outputSession->sendMessage(&msg);
   MemFree(chunk);
}

/**
 * List all metrics provided by extension
 */
void AgentExtension::listMetrics(NXCPMessage *msg, uint32_t *baseId, uint32_t *count)
{
   LockGuard guard(m_capabilitiesLock);
   uint32_t fieldId = *baseId;
   for (int i = 0; i < m_parameters.size(); i++)
   {
      const NETXMS_SUBAGENT_PARAM *p = m_parameters.get(i);
      msg->setField(fieldId++, p->name);
      msg->setField(fieldId++, p->description);
      msg->setField(fieldId++, static_cast<uint16_t>(p->dataType));
   }
   *count += m_parameters.size();
   *baseId = fieldId;
}

/**
 * List all metrics provided by extension
 */
void AgentExtension::listMetrics(StringList *list)
{
   LockGuard guard(m_capabilitiesLock);
   for (int i = 0; i < m_parameters.size(); i++)
      list->add(m_parameters.get(i)->name);
}

/**
 * List all lists provided by extension
 */
void AgentExtension::listLists(NXCPMessage *msg, uint32_t *baseId, uint32_t *count)
{
   LockGuard guard(m_capabilitiesLock);
   uint32_t fieldId = *baseId;
   for (int i = 0; i < m_lists.size(); i++)
   {
      msg->setField(fieldId++, m_lists.get(i)->name);
      msg->setField(fieldId++, m_lists.get(i)->description);
      fieldId += 8;
   }
   *count += m_lists.size();
   *baseId = fieldId;
}

/**
 * List all lists provided by extension
 */
void AgentExtension::listLists(StringList *list)
{
   LockGuard guard(m_capabilitiesLock);
   for (int i = 0; i < m_lists.size(); i++)
      list->add(m_lists.get(i)->name);
}

/**
 * List all tables provided by extension
 */
void AgentExtension::listTables(NXCPMessage *msg, uint32_t *baseId, uint32_t *count)
{
   LockGuard guard(m_capabilitiesLock);
   uint32_t fieldId = *baseId;
   for (int i = 0; i < m_tables.size(); i++)
   {
      const NETXMS_SUBAGENT_TABLE *t = m_tables.get(i);
      msg->setField(fieldId++, t->name);
      msg->setField(fieldId++, t->instanceColumns);
      msg->setField(fieldId++, t->description);
   }
   *count += m_tables.size();
   *baseId = fieldId;
}

/**
 * List all tables provided by extension
 */
void AgentExtension::listTables(StringList *list)
{
   LockGuard guard(m_capabilitiesLock);
   for (int i = 0; i < m_tables.size(); i++)
      list->add(m_tables.get(i)->name);
}

/**
 * List all actions provided by extension
 */
void AgentExtension::listActions(NXCPMessage *msg, uint32_t *baseId, uint32_t *count)
{
   LockGuard guard(m_capabilitiesLock);
   uint32_t fieldId = *baseId;
   for (int i = 0; i < m_actions.size(); i++)
   {
      const ACTION *a = m_actions.get(i);
      msg->setField(fieldId++, a->name);
      msg->setField(fieldId++, _T(""));    // command line — empty for extension actions
      msg->setField(fieldId++, m_config.name);
      msg->setField(fieldId++, a->description);
   }
   *count += m_actions.size();
   *baseId = fieldId;
}

/**
 * List all actions provided by extension
 */
void AgentExtension::listActions(StringList *list)
{
   LockGuard guard(m_capabilitiesLock);
   for (int i = 0; i < m_actions.size(); i++)
      list->add(m_actions.get(i)->name);
}
