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
** File: extension.h
**
**/

#ifndef _nxagent_extension_h_
#define _nxagent_extension_h_

#define DEBUG_TAG_EXTENSION   _T("extension")

#define EXTENSION_PROTOCOL_VERSION   1

struct json_t;
class ExtensionProcess;
class ByteStream;
class ConfigEntry;

/**
 * Extension lifecycle mode
 */
enum class ExtensionMode
{
   SPAWN,      // agent fork/execs the extension and supervises it
   CONNECT     // agent connects to an externally-managed listener
};

/**
 * Extension runtime state
 */
enum class ExtensionState
{
   IDLE,                // configured but not started
   STARTING,            // child process being launched (spawn mode)
   CONNECTING,          // TCP connection in progress
   HANDSHAKING,         // hello / list_capabilities exchange in progress
   READY,               // handshake completed, serving requests
   RECONNECT_BACKOFF,   // waiting before retry
   DEAD                 // permanently failed (config error)
};

/**
 * Per-extension static configuration parsed from agent config
 */
struct ExtensionConfig
{
   TCHAR name[MAX_SUBAGENT_NAME];
   ExtensionMode mode;

   // spawn mode
   MutableString command;
   StringMap environment;
   MutableString runAs;
   uint32_t restartDelayMs;
   uint32_t restartDelayMaxMs;
   uint32_t shutdownTimeoutMs;

   // connect mode
   InetAddress endpointAddr;
   uint16_t endpointPort;
   uint32_t reconnectDelayMs;
   uint32_t reconnectDelayMaxMs;
   bool allowRemote;

   // both
   MutableString token;
   uint32_t requestTimeoutMs;
   uint32_t actionTimeoutMs;
   MutableString debugTag;

   ExtensionConfig() : endpointPort(0)
   {
      name[0] = 0;
      mode = ExtensionMode::SPAWN;
      restartDelayMs = 5000;
      restartDelayMaxMs = 300000;
      shutdownTimeoutMs = 10000;
      reconnectDelayMs = 5000;
      reconnectDelayMaxMs = 300000;
      allowRemote = false;
      requestTimeoutMs = 5000;
      actionTimeoutMs = 60000;
   }
};

/**
 * Pending JSON-RPC request awaiting response
 */
struct PendingExtRequest
{
   uint32_t id;
   Condition completion;
   json_t *result;       // owned; nullptr until completion
   json_t *error;        // owned; nullptr unless error response
   bool completed;

   // optional streaming output sink for execute_action
   AbstractCommSession *outputSession;
   uint32_t outputSessionRequestId;

   PendingExtRequest(uint32_t _id) : completion(false)
   {
      id = _id;
      result = nullptr;
      error = nullptr;
      completed = false;
      outputSession = nullptr;
      outputSessionRequestId = 0;
   }

   ~PendingExtRequest();
};

/**
 * Generic agent extension speaking JSON-RPC 2.0 over a TCP loopback connection.
 * Parallel to ExternalSubagent (named pipe + NXCP) but language-agnostic.
 */
class AgentExtension
{
private:
   ExtensionConfig m_config;
   ExtensionState m_state;
   Mutex m_stateLock;

   SOCKET m_socket;
   ExtensionProcess *m_process;     // spawn mode only
   uint16_t m_assignedPort;         // spawn mode only — port handed to child
   MutableString m_runtimeToken;    // spawn mode: generated; connect: copy of config token
   THREAD m_supervisorThread;
   Condition m_stopCondition;       // signalled when stop() is called — wakes backoff sleep

   // request/response correlation
   Mutex m_pendingLock;
   HashMap<uint32_t, PendingExtRequest> m_pending;
   uint32_t m_nextRequestId;
   Mutex m_sendLock;

   // capabilities resolved from list_capabilities
   StructArray<NETXMS_SUBAGENT_PARAM> m_parameters;
   StructArray<NETXMS_SUBAGENT_LIST> m_lists;
   StructArray<NETXMS_SUBAGENT_TABLE> m_tables;
   StructArray<ACTION> m_actions;
   Mutex m_capabilitiesLock;

   // bookkeeping
   time_t m_handshakeTime;
   uint32_t m_restartCount;
   uint32_t m_currentBackoffMs;
   MutableString m_lastError;

   // I/O loop control
   volatile bool m_stopRequested;
   bool m_capabilitiesDirty;        // capabilities_changed received → re-fetch
   int64_t m_lastPingSentMs;
   int64_t m_lastPingReplyMs;
   uint32_t m_pingStrikes;
   ByteStream *m_inbuffer;          // accumulated inbound bytes (line-delimited)

   // supervisor thread (defined in extension_lifecycle.cpp)
   void supervisorLoop();
   bool setupConnection();
   void tearDownConnection();
   bool spawnAndConnect();
   bool connectOnly();
   void applyBackoff();

   // I/O loop (defined in extension.cpp)
   void ioLoop();

   // protocol primitives — implemented in phase 2
   bool sendJsonRpc(json_t *msg);
   json_t *call(const char *method, json_t *params, uint32_t timeoutMs);
   json_t *callSync(const char *method, json_t *params, uint32_t timeoutMs);
   void notify(const char *method, json_t *params);
   void handleIncomingLine(const char *line, size_t length);
   void handleNotification(const char *method, json_t *params);
   void handleResponse(uint32_t id, json_t *result, json_t *error);
   bool performHandshake();
   bool fetchCapabilities();
   void clearCapabilities();

   void onDisconnect(const TCHAR *reason);
   void scheduleReconnect();
   void setState(ExtensionState newState);
   void setLastError(const TCHAR *fmt, ...);

   // sub-helpers used by handleNotification / handleResponse
   void handlePushMetric(json_t *params);
   void handlePushMetricsBatch(json_t *params);
   void handleSendEvent(json_t *params);
   void handleLog(json_t *params);
   void handleActionOutput(json_t *params);
   void maybeSendPing(int64_t now);
   bool readAndDispatch(uint32_t timeoutMs);

public:
   AgentExtension(const ExtensionConfig& cfg);
   ~AgentExtension();

   void start();
   void stop(bool restart);

   bool isConnected() const { return m_state == ExtensionState::READY; }
   const TCHAR *getName() const { return m_config.name; }
   const TCHAR *getDebugTag() const { return m_config.debugTag.cstr(); }
   ExtensionMode getMode() const { return m_config.mode; }
   ExtensionState getState() const { return m_state; }
   uint32_t getRestartCount() const { return m_restartCount; }
   time_t getHandshakeTime() const { return m_handshakeTime; }
   const TCHAR *getLastError() const { return m_lastError.cstr(); }
   uint16_t getAssignedPort() const { return m_assignedPort; }
   uint32_t getProcessPid() const;    // defined in extension_lifecycle.cpp (ExtensionProcess is opaque here)

   // dispatch surface — mirrors ExternalSubagent
   uint32_t getParameter(const TCHAR *name, TCHAR *buffer);
   uint32_t getList(const TCHAR *name, StringList *value);
   uint32_t getTable(const TCHAR *name, Table *value);
   uint32_t executeAction(const TCHAR *name, const StringList& args,
            AbstractCommSession *session, uint32_t requestId, bool sendOutput);

   void listParameters(NXCPMessage *msg, uint32_t *baseId, uint32_t *count);
   void listParameters(StringList *list);
   void listLists(NXCPMessage *msg, uint32_t *baseId, uint32_t *count);
   void listLists(StringList *list);
   void listTables(NXCPMessage *msg, uint32_t *baseId, uint32_t *count);
   void listTables(StringList *list);
   void listActions(NXCPMessage *msg, uint32_t *baseId, uint32_t *count);
   void listActions(StringList *list);
};

/**
 * Global registry / dispatch entry points (defined in extension_registry.cpp)
 */
bool AddExtension(const ExtensionConfig& config);
bool AddExtensionFromConfig(const ConfigEntry *config);
bool AddExtensionFromShorthand(const TCHAR *shorthand);
void StartExtensions();
void StopExtensions(bool restart);

uint32_t GetParameterValueFromExtension(const TCHAR *name, TCHAR *buffer);
uint32_t GetListValueFromExtension(const TCHAR *name, StringList *value);
uint32_t GetTableValueFromExtension(const TCHAR *name, Table *value);
uint32_t ExecuteActionByExtension(const TCHAR *name, const StringList& args,
         const shared_ptr<AbstractCommSession>& session, uint32_t requestId, bool sendOutput);

void ListParametersFromExtensions(NXCPMessage *msg, uint32_t *baseId, uint32_t *count);
void ListParametersFromExtensions(StringList *list);
void ListListsFromExtensions(NXCPMessage *msg, uint32_t *baseId, uint32_t *count);
void ListListsFromExtensions(StringList *list);
void ListTablesFromExtensions(NXCPMessage *msg, uint32_t *baseId, uint32_t *count);
void ListTablesFromExtensions(StringList *list);
void ListActionsFromExtensions(NXCPMessage *msg, uint32_t *baseId, uint32_t *count);
void ListActionsFromExtensions(StringList *list);

bool IsExtensionConnected(const TCHAR *name);
time_t GetExtensionUptime(const TCHAR *name);

const TCHAR *ExtensionStateName(ExtensionState state);

// Metric / table handlers for Agent.Extension.* (defined in extension_registry.cpp)
LONG H_ExtensionIsConnected(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_ExtensionUptime(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_ExtensionsTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session);

#endif   /* _nxagent_extension_h_ */
