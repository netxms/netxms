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
** File: extension_registry.cpp
**
** Global registry and dispatch surface for AgentExtension instances.
** Mirrors the pattern in extagent.cpp's global helpers.
**
**/

#include "nxagentd.h"
#include "extension.h"
#include <nxconfig.h>

#define DEBUG_TAG   DEBUG_TAG_EXTENSION

/**
 * Registered extensions
 */
static ObjectArray<AgentExtension> s_extensions(0, 8, Ownership::True);
static Mutex s_registryLock;

/**
 * Look up an extension by name.  Caller must hold s_registryLock.
 */
static AgentExtension *FindExtension(const TCHAR *name)
{
   for (int i = 0; i < s_extensions.size(); i++)
   {
      if (!_tcscmp(s_extensions.get(i)->getName(), name))
         return s_extensions.get(i);
   }
   return nullptr;
}

/**
 * Register a fully-formed extension config.  Returns false on duplicate name
 * or other validation failure.
 */
bool AddExtension(const ExtensionConfig& config)
{
   LockGuard guard(s_registryLock);
   if (FindExtension(config.name) != nullptr)
   {
      nxlog_write(NXLOG_ERROR, _T("Duplicate agent extension name \"%s\""), config.name);
      return false;
   }
   s_extensions.add(new AgentExtension(config));
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Extension \"%s\" registered (mode=%s)"),
            config.name,
            (config.mode == ExtensionMode::SPAWN) ? _T("spawn") : _T("connect"));
   return true;
}

/**
 * Resolve a "Token" spec.  "file:<path>" reads the (whitespace-trimmed) file
 * contents; anything else is taken literally.  Returns false on read failure.
 */
static bool ResolveToken(const TCHAR *spec, MutableString *out, const TCHAR *extName)
{
   if ((spec == nullptr) || (*spec == 0))
   {
      *out = _T("");
      return true;
   }
   if (!_tcsncmp(spec, _T("file:"), 5))
   {
      const TCHAR *path = spec + 5;
      size_t size = 0;
      BYTE *data = LoadFile(path, &size);
      if (data == nullptr)
      {
         nxlog_write(NXLOG_ERROR, _T("Agent extension \"%s\": cannot read token file \"%s\""), extName, path);
         return false;
      }
      const char *s = reinterpret_cast<const char*>(data);
      size_t len = size;
      while ((len > 0) && isspace(static_cast<unsigned char>(s[len - 1]))) len--;
      size_t start = 0;
      while ((start < len) && isspace(static_cast<unsigned char>(s[start]))) start++;
      StringBuffer sb;
      sb.appendUtf8String(s + start, static_cast<ssize_t>(len - start));
      MemFree(data);
      if (sb.isEmpty())
      {
         nxlog_write(NXLOG_ERROR, _T("Agent extension \"%s\": token file \"%s\" is empty"), extName, path);
         return false;
      }
      *out = sb;
      return true;
   }
   *out = spec;
   return true;
}

/**
 * Parse "[tcp://]host:port" into address + port.
 */
static bool ParseEndpoint(const TCHAR *endpoint, InetAddress *addr, uint16_t *port)
{
   const TCHAR *p = endpoint;
   if (!_tcsnicmp(p, _T("tcp://"), 6))
      p += 6;

   const TCHAR *lastColon = _tcsrchr(p, _T(':'));
   if (lastColon == nullptr)
      return false;

   TCHAR host[256];
   size_t hostLen = std::min(static_cast<size_t>(lastColon - p), sizeof(host) / sizeof(TCHAR) - 1);
   memcpy(host, p, hostLen * sizeof(TCHAR));
   host[hostLen] = 0;
   if ((hostLen >= 2) && (host[0] == _T('[')) && (host[hostLen - 1] == _T(']')))
   {
      memmove(host, host + 1, (hostLen - 2) * sizeof(TCHAR));
      host[hostLen - 2] = 0;
   }

   long portNum = _tcstol(lastColon + 1, nullptr, 10);
   if ((portNum <= 0) || (portNum > 65535))
      return false;
   *port = static_cast<uint16_t>(portNum);

   *addr = InetAddress::resolveHostName(host);
   return addr->isValid();
}

/**
 * Apply common (mode-independent) settings and a default debug tag.
 */
static void ApplyDefaultDebugTag(ExtensionConfig *cfg)
{
   StringBuffer tag(_T("extension."));
   tag.append(cfg->name);
   cfg->debugTag = tag;
}

/**
 * Build an ExtensionConfig from an <extensions> child block and register it.
 */
bool AddExtensionFromConfig(const ConfigEntry *config)
{
   ExtensionConfig cfg;
   _tcslcpy(cfg.name, config->getName(), MAX_SUBAGENT_NAME);
   ApplyDefaultDebugTag(&cfg);

   const TCHAR *modeStr = config->getSubEntryValue(_T("Mode"));
   if (modeStr == nullptr)
   {
      nxlog_write(NXLOG_ERROR, _T("Agent extension \"%s\": Mode is required"), cfg.name);
      return false;
   }
   if (!_tcsicmp(modeStr, _T("spawn")))
      cfg.mode = ExtensionMode::SPAWN;
   else if (!_tcsicmp(modeStr, _T("connect")))
      cfg.mode = ExtensionMode::CONNECT;
   else
   {
      nxlog_write(NXLOG_ERROR, _T("Agent extension \"%s\": invalid Mode \"%s\" (expected spawn or connect)"), cfg.name, modeStr);
      return false;
   }

   cfg.requestTimeoutMs = config->getSubEntryValueAsUInt(_T("RequestTimeout"), 0, 5) * 1000;
   cfg.actionTimeoutMs = config->getSubEntryValueAsUInt(_T("ActionTimeout"), 0, 60) * 1000;
   const TCHAR *dbgTag = config->getSubEntryValue(_T("DebugTag"));
   if ((dbgTag != nullptr) && (*dbgTag != 0))
      cfg.debugTag = dbgTag;

   const TCHAR *tokenSpec = config->getSubEntryValue(_T("Token"));
   if ((tokenSpec != nullptr) && !ResolveToken(tokenSpec, &cfg.token, cfg.name))
      return false;

   if (cfg.mode == ExtensionMode::SPAWN)
   {
      const TCHAR *cmd = config->getSubEntryValue(_T("Command"));
      if ((cmd == nullptr) || (*cmd == 0))
      {
         nxlog_write(NXLOG_ERROR, _T("Agent extension \"%s\": spawn mode requires Command"), cfg.name);
         return false;
      }
      cfg.command = cmd;

      const TCHAR *runAs = config->getSubEntryValue(_T("RunAs"));
      if ((runAs != nullptr) && (*runAs != 0))
         cfg.runAs = runAs;

      cfg.restartDelayMs = config->getSubEntryValueAsUInt(_T("RestartDelay"), 0, 5) * 1000;
      cfg.restartDelayMaxMs = config->getSubEntryValueAsUInt(_T("RestartDelayMax"), 0, 300) * 1000;
      cfg.shutdownTimeoutMs = config->getSubEntryValueAsUInt(_T("ShutdownTimeout"), 0, 10) * 1000;

      const ConfigEntry *envEntry = config->findEntry(_T("Environment"));
      if (envEntry != nullptr)
      {
         for (int i = 0; i < envEntry->getValueCount(); i++)
         {
            const TCHAR *kv = envEntry->getValue(i);
            const TCHAR *eq = (kv != nullptr) ? _tcschr(kv, _T('=')) : nullptr;
            if (eq != nullptr)
            {
               String key(kv, static_cast<size_t>(eq - kv));
               cfg.environment.set(key, eq + 1);
            }
            else if ((kv != nullptr) && (*kv != 0))
            {
               nxlog_write(NXLOG_WARNING, _T("Agent extension \"%s\": ignoring malformed Environment entry \"%s\""), cfg.name, kv);
            }
         }
      }
   }
   else  // CONNECT
   {
      const TCHAR *endpoint = config->getSubEntryValue(_T("Endpoint"));
      if ((endpoint == nullptr) || (*endpoint == 0))
      {
         nxlog_write(NXLOG_ERROR, _T("Agent extension \"%s\": connect mode requires Endpoint"), cfg.name);
         return false;
      }
      if (!ParseEndpoint(endpoint, &cfg.endpointAddr, &cfg.endpointPort))
      {
         nxlog_write(NXLOG_ERROR, _T("Agent extension \"%s\": cannot parse Endpoint \"%s\""), cfg.name, endpoint);
         return false;
      }
      cfg.allowRemote = config->getSubEntryValueAsBoolean(_T("AllowRemote"), 0, false);
      if (!cfg.endpointAddr.isLoopback() && !cfg.allowRemote)
      {
         nxlog_write(NXLOG_ERROR, _T("Agent extension \"%s\": non-loopback Endpoint requires AllowRemote=yes"), cfg.name);
         return false;
      }
      if (cfg.token.isEmpty())
      {
         nxlog_write(NXLOG_ERROR, _T("Agent extension \"%s\": connect mode requires Token"), cfg.name);
         return false;
      }
      cfg.reconnectDelayMs = config->getSubEntryValueAsUInt(_T("ReconnectDelay"), 0, 5) * 1000;
      cfg.reconnectDelayMaxMs = config->getSubEntryValueAsUInt(_T("ReconnectDelayMax"), 0, 300) * 1000;
   }

   return AddExtension(cfg);
}

/**
 * Parse the flat shorthand form and register the resulting extension.
 * Format: "name: [spawn] <command line...>".  Connect mode is not supported in
 * the shorthand (it needs a Token) — use an <extensions> block instead.
 */
bool AddExtensionFromShorthand(const TCHAR *shorthand)
{
   const TCHAR *colon = _tcschr(shorthand, _T(':'));
   if ((colon == nullptr) || (colon == shorthand))
   {
      nxlog_write(NXLOG_ERROR, _T("Malformed Extension shorthand: \"%s\""), shorthand);
      return false;
   }

   ExtensionConfig cfg;
   size_t nameLen = std::min(static_cast<size_t>(colon - shorthand), static_cast<size_t>(MAX_SUBAGENT_NAME - 1));
   memcpy(cfg.name, shorthand, nameLen * sizeof(TCHAR));
   cfg.name[nameLen] = 0;
   Trim(cfg.name);
   if (cfg.name[0] == 0)
   {
      nxlog_write(NXLOG_ERROR, _T("Malformed Extension shorthand (empty name): \"%s\""), shorthand);
      return false;
   }
   ApplyDefaultDebugTag(&cfg);
   cfg.requestTimeoutMs = 5000;
   cfg.actionTimeoutMs = 60000;
   cfg.mode = ExtensionMode::SPAWN;

   const TCHAR *rest = colon + 1;
   while (*rest == _T(' ') || *rest == _T('\t'))
      rest++;
   if (!_tcsnicmp(rest, _T("connect"), 7) && (rest[7] == 0 || rest[7] == _T(' ') || rest[7] == _T('\t')))
   {
      nxlog_write(NXLOG_ERROR, _T("Agent extension \"%s\": connect mode is not supported in shorthand form; use an <extensions> block"), cfg.name);
      return false;
   }
   if (!_tcsnicmp(rest, _T("spawn"), 5) && (rest[5] == _T(' ') || rest[5] == _T('\t')))
   {
      rest += 6;
      while (*rest == _T(' ') || *rest == _T('\t'))
         rest++;
   }
   if (*rest == 0)
   {
      nxlog_write(NXLOG_ERROR, _T("Agent extension \"%s\": missing command in shorthand"), cfg.name);
      return false;
   }
   cfg.command = rest;

   return AddExtension(cfg);
}

/**
 * Start all registered extensions.
 */
void StartExtensions()
{
   LockGuard guard(s_registryLock);
   for (int i = 0; i < s_extensions.size(); i++)
      s_extensions.get(i)->start();
}

/**
 * Stop all registered extensions.
 */
void StopExtensions(bool restart)
{
   LockGuard guard(s_registryLock);
   for (int i = 0; i < s_extensions.size(); i++)
      s_extensions.get(i)->stop(restart);
}

/**
 * Dispatch helper: iterate connected extensions and forward metric read request
 */
uint32_t GetMetricValueFromExtension(const TCHAR *name, TCHAR *buffer)
{
   LockGuard guard(s_registryLock);
   uint32_t rc = ERR_UNKNOWN_METRIC;
   for (int i = 0; i < s_extensions.size(); i++)
   {
      AgentExtension *ext = s_extensions.get(i);
      if (ext->isConnected())
      {
         rc = ext->getMetric(name, buffer);
         if (rc != ERR_UNKNOWN_METRIC)
            break;
      }
   }
   return rc;
}

/**
 * Dispatch helper: iterate connected extensions and forward list read request
 */
uint32_t GetListValueFromExtension(const TCHAR *name, StringList *value)
{
   LockGuard guard(s_registryLock);
   uint32_t rc = ERR_UNKNOWN_METRIC;
   for (int i = 0; i < s_extensions.size(); i++)
   {
      AgentExtension *ext = s_extensions.get(i);
      if (ext->isConnected())
      {
         rc = ext->getList(name, value);
         if (rc != ERR_UNKNOWN_METRIC)
            break;
      }
   }
   return rc;
}

/**
 * Dispatch helper: iterate connected extensions and forward table read request
 */
uint32_t GetTableValueFromExtension(const TCHAR *name, Table *value)
{
   LockGuard guard(s_registryLock);
   uint32_t rc = ERR_UNKNOWN_METRIC;
   for (int i = 0; i < s_extensions.size(); i++)
   {
      AgentExtension *ext = s_extensions.get(i);
      if (ext->isConnected())
      {
         rc = ext->getTable(name, value);
         if (rc != ERR_UNKNOWN_METRIC)
            break;
      }
   }
   return rc;
}

/**
 * Dispatch helper: iterate connected extensions and forward action execution request
 */
uint32_t ExecuteActionByExtension(const TCHAR *name, const StringList& args, const shared_ptr<AbstractCommSession>& session, uint32_t requestId, bool sendOutput)
{
   LockGuard guard(s_registryLock);
   uint32_t rc = ERR_UNKNOWN_METRIC;
   for (int i = 0; i < s_extensions.size(); i++)
   {
      AgentExtension *ext = s_extensions.get(i);
      if (ext->isConnected())
      {
         rc = ext->executeAction(name, args, session.get(), requestId, sendOutput);
         if (rc != ERR_UNKNOWN_METRIC)
            break;
      }
   }
   return rc;
}

/**
 * List all metrics provided by all extensions
 */
void ListMetricsFromExtensions(NXCPMessage *msg, uint32_t *baseId, uint32_t *count)
{
   LockGuard guard(s_registryLock);
   for (int i = 0; i < s_extensions.size(); i++)
      if (s_extensions.get(i)->isConnected())
         s_extensions.get(i)->listMetrics(msg, baseId, count);
}

/**
 * List all metrics provided by all extensions
 */
void ListMetricsFromExtensions(StringList *list)
{
   LockGuard guard(s_registryLock);
   for (int i = 0; i < s_extensions.size(); i++)
      if (s_extensions.get(i)->isConnected())
         s_extensions.get(i)->listMetrics(list);
}

/**
 * List all lists provided by all extensions
 */
void ListListsFromExtensions(NXCPMessage *msg, uint32_t *baseId, uint32_t *count)
{
   LockGuard guard(s_registryLock);
   for (int i = 0; i < s_extensions.size(); i++)
      if (s_extensions.get(i)->isConnected())
         s_extensions.get(i)->listLists(msg, baseId, count);
}

/**
 * List all lists provided by all extensions
 */
void ListListsFromExtensions(StringList *list)
{
   LockGuard guard(s_registryLock);
   for (int i = 0; i < s_extensions.size(); i++)
      if (s_extensions.get(i)->isConnected())
         s_extensions.get(i)->listLists(list);
}

/**
 * List all tables provided by all extensions
 */
void ListTablesFromExtensions(NXCPMessage *msg, uint32_t *baseId, uint32_t *count)
{
   LockGuard guard(s_registryLock);
   for (int i = 0; i < s_extensions.size(); i++)
      if (s_extensions.get(i)->isConnected())
         s_extensions.get(i)->listTables(msg, baseId, count);
}

/**
 * List all tables provided by all extensions
 */
void ListTablesFromExtensions(StringList *list)
{
   LockGuard guard(s_registryLock);
   for (int i = 0; i < s_extensions.size(); i++)
      if (s_extensions.get(i)->isConnected())
         s_extensions.get(i)->listTables(list);
}

/**
 * List all actions provided by all extensions
 */
void ListActionsFromExtensions(NXCPMessage *msg, uint32_t *baseId, uint32_t *count)
{
   LockGuard guard(s_registryLock);
   for (int i = 0; i < s_extensions.size(); i++)
      if (s_extensions.get(i)->isConnected())
         s_extensions.get(i)->listActions(msg, baseId, count);
}

/**
 * List all actions provided by all extensions
 */
void ListActionsFromExtensions(StringList *list)
{
   LockGuard guard(s_registryLock);
   for (int i = 0; i < s_extensions.size(); i++)
      if (s_extensions.get(i)->isConnected())
         s_extensions.get(i)->listActions(list);
}

/**
 * Check if extension is connected
 */
bool IsExtensionConnected(const TCHAR *name)
{
   LockGuard guard(s_registryLock);
   AgentExtension *ext = FindExtension(name);
   return (ext != nullptr) && ext->isConnected();
}

/**
 * Get uptime for given extension
 */
time_t GetExtensionUptime(const TCHAR *name)
{
   LockGuard guard(s_registryLock);
   AgentExtension *ext = FindExtension(name);
   if (ext == nullptr || !ext->isConnected())
      return 0;
   time_t now = time(nullptr);
   time_t hs = ext->getHandshakeTime();
   return (hs > 0 && now > hs) ? (now - hs) : 0;
}

/**
 * Human-readable extension state name (for the Agent.Extensions table).
 */
const TCHAR *ExtensionStateName(ExtensionState state)
{
   switch (state)
   {
      case ExtensionState::IDLE:               return _T("idle");
      case ExtensionState::STARTING:           return _T("starting");
      case ExtensionState::CONNECTING:         return _T("connecting");
      case ExtensionState::HANDSHAKING:        return _T("handshaking");
      case ExtensionState::READY:              return _T("ready");
      case ExtensionState::RECONNECT_BACKOFF:  return _T("backoff");
      case ExtensionState::DEAD:               return _T("dead");
      default:                                 return _T("unknown");
   }
}

/**
 * Handler for Agent.Extension.IsConnected(*)
 */
LONG H_ExtensionIsConnected(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR name[MAX_SUBAGENT_NAME];
   if (!AgentGetParameterArg(cmd, 1, name, MAX_SUBAGENT_NAME))
      return SYSINFO_RC_UNSUPPORTED;

   LockGuard guard(s_registryLock);
   AgentExtension *ext = FindExtension(name);
   if (ext == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;
   ret_int(value, ext->isConnected() ? 1 : 0);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Agent.Extension.Uptime(*)
 */
LONG H_ExtensionUptime(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR name[MAX_SUBAGENT_NAME];
   if (!AgentGetParameterArg(cmd, 1, name, MAX_SUBAGENT_NAME))
      return SYSINFO_RC_UNSUPPORTED;

   LockGuard guard(s_registryLock);
   AgentExtension *ext = FindExtension(name);
   if (ext == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   time_t uptime = 0;
   if (ext->isConnected())
   {
      time_t now = time(nullptr);
      time_t hs = ext->getHandshakeTime();
      if ((hs > 0) && (now > hs))
         uptime = now - hs;
   }
   ret_uint64(value, static_cast<uint64_t>(uptime));
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Agent.Extension.RestartCount(*)
 */
LONG H_ExtensionRestartCount(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR name[MAX_SUBAGENT_NAME];
   if (!AgentGetParameterArg(cmd, 1, name, MAX_SUBAGENT_NAME))
      return SYSINFO_RC_UNSUPPORTED;

   LockGuard guard(s_registryLock);
   AgentExtension *ext = FindExtension(name);
   if (ext == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   ret_uint(value, ext->getRestartCount());
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Agent.Extensions table
 */
LONG H_ExtensionsTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"), true);
   value->addColumn(_T("MODE"), DCI_DT_STRING, _T("Mode"));
   value->addColumn(_T("STATE"), DCI_DT_STRING, _T("State"));
   value->addColumn(_T("PID"), DCI_DT_UINT, _T("PID"));
   value->addColumn(_T("PORT"), DCI_DT_UINT, _T("Port"));
   value->addColumn(_T("RESTART_COUNT"), DCI_DT_UINT, _T("Restart count"));
   value->addColumn(_T("CONNECTED_SINCE"), DCI_DT_UINT64, _T("Connected since"));
   value->addColumn(_T("LAST_ERROR"), DCI_DT_STRING, _T("Last error"));

   LockGuard guard(s_registryLock);
   for (int i = 0; i < s_extensions.size(); i++)
   {
      AgentExtension *ext = s_extensions.get(i);
      value->addRow();
      value->set(0, ext->getName());
      value->set(1, (ext->getMode() == ExtensionMode::SPAWN) ? _T("spawn") : _T("connect"));
      value->set(2, ExtensionStateName(ext->getState()));
      value->set(3, ext->getProcessPid());
      value->set(4, static_cast<uint32_t>(ext->getAssignedPort()));
      value->set(5, ext->getRestartCount());
      value->set(6, static_cast<int64_t>(ext->getHandshakeTime()));
      value->set(7, ext->getLastError());
   }
   return SYSINFO_RC_SUCCESS;
}
