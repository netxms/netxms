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
** File: extension_lifecycle.cpp
**
** Spawn-mode supervision and connect-mode reconnection.  Owns the per-extension
** supervisor thread that drives spawn → connect → ioLoop → backoff → retry.
**
**/

#include "nxagentd.h"
#include "extension.h"
#include <nxproc.h>
#ifndef _WIN32
#include <signal.h>
#endif

#define DEBUG_TAG   DEBUG_TAG_EXTENSION

#define CONNECT_TIMEOUT_MS         2000
#define POSTSPAWN_CONNECT_TRIES    50
#define POSTSPAWN_CONNECT_INTERVAL 100   // ms between attempts (50 * 100ms = 5s window)

/**
 * Process-wide lock serialising spawn-time env-var manipulation.
 *
 * We inject NXAGENT_EXTENSION_PORT / NXAGENT_EXTENSION_TOKEN (and optional
 * user-supplied vars) into our own environment, fork+exec, then unset.  Doing
 * this without a lock would race with concurrent spawns; serialising spawns
 * is acceptable since they only happen at startup or after backoff.
 */
static Mutex s_spawnLock;

/**
 * Reserve a free ephemeral port on loopback by binding+closing.
 * Caller passes the port to the spawned extension, which re-binds it.
 */
static bool ReservePort(uint16_t *port)
{
   SOCKET s = CreateSocket(AF_INET, SOCK_STREAM, 0);
   if (s == INVALID_SOCKET)
      return false;

   struct sockaddr_in sa;
   memset(&sa, 0, sizeof(sa));
   sa.sin_family = AF_INET;
   sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
   sa.sin_port = 0;

   if (bind(s, reinterpret_cast<struct sockaddr*>(&sa), sizeof(sa)) != 0)
   {
      closesocket(s);
      return false;
   }

   socklen_t len = sizeof(sa);
   if (getsockname(s, reinterpret_cast<struct sockaddr*>(&sa), &len) != 0)
   {
      closesocket(s);
      return false;
   }

   *port = ntohs(sa.sin_port);
   closesocket(s);
   return true;
}

/**
 * Generate a hex-encoded random token of the requested byte length.
 */
static String GenerateExtensionToken(size_t byteLength = 32)
{
   BYTE *bytes = MemAllocArray<BYTE>(byteLength);
   GenerateRandomBytes(bytes, byteLength);
   StringBuffer hex;
   hex.appendAsHexString(bytes, byteLength);
   MemFree(bytes);
   return hex;
}

/**
 * Spawned extension child process.  Inherits ProcessExecutor's output thread
 * and pid bookkeeping; we override onOutput to forward stderr line-by-line
 * into the agent log under the extension's debug tag.
 */
class ExtensionProcess : public ProcessExecutor
{
private:
   String m_extensionName;
   String m_logTag;
   StringBuffer m_lineBuffer;

   void emitLine()
   {
      if (!m_lineBuffer.isEmpty() && m_lineBuffer.charAt(m_lineBuffer.length() - 1) == _T('\r'))
         m_lineBuffer.shrink(1);
      nxlog_debug_tag(m_logTag.cstr(), 5, _T("Extension(%s): %s"),
               m_extensionName.cstr(), m_lineBuffer.cstr());
      m_lineBuffer.clear();
   }

protected:
   virtual void onOutput(const char *text, size_t length) override
   {
      const char *start = text;
      const char *end = text + length;
      for (const char *p = start; p < end; p++)
      {
         if (*p == '\n')
         {
            m_lineBuffer.appendUtf8String(start, static_cast<ssize_t>(p - start));
            emitLine();
            start = p + 1;
         }
      }
      if (start < end)
         m_lineBuffer.appendUtf8String(start, static_cast<ssize_t>(end - start));
   }

   virtual void endOfOutput() override
   {
      if (!m_lineBuffer.isEmpty())
         emitLine();
   }

public:
   ExtensionProcess(const TCHAR *cmd, const TCHAR *extensionName, const TCHAR *logTag)
         : ProcessExecutor(cmd, false /* shellExec */, false /* selfDestruct */),
           m_extensionName(extensionName), m_logTag(logTag)
   {
      m_sendOutput = true;
   }

   /**
    * Launch the child with NXAGENT_EXTENSION_PORT/TOKEN injected.  Serialised
    * through s_spawnLock so concurrent spawns don't corrupt env state.
    */
   bool launch(uint16_t port, const TCHAR *token, const StringMap& extraEnv, const TCHAR *runAs)
   {
      LockGuard guard(s_spawnLock);

      char portStr[16];
      snprintf(portStr, sizeof(portStr), "%u", port);
#ifdef UNICODE
      char *tokenU = UTF8StringFromWideString(token);
#else
      char *tokenU = const_cast<char*>(token);
#endif

#ifndef _WIN32
      setenv("NXAGENT_EXTENSION_PORT", portStr, 1);
      setenv("NXAGENT_EXTENSION_TOKEN", tokenU, 1);
      StringList setKeys;
      extraEnv.forEach([&setKeys] (const TCHAR *key, const void *value) {
         SetEnvironmentVariable(key, static_cast<const TCHAR*>(value));
         setKeys.add(key);
         return _CONTINUE;
      });

      if (runAs != nullptr && *runAs != 0)
         nxlog_debug_tag(DEBUG_TAG, 3, _T("Extension(%s): RunAs=%s ignored — not yet implemented in v1"), m_extensionName.cstr(), runAs);
#else
      // Windows env injection — TODO in a follow-up; spawn mode degrades to
      // inheriting agent's environment and the extension reading port/token
      // from its own config.  For v1 the primary platform is Linux.
      nxlog_debug_tag(DEBUG_TAG, 1, _T("Extension(%s): spawn-mode env injection not implemented on Windows; child must use connect mode"), m_extensionName.cstr());
#endif

      bool ok = execute();

#ifndef _WIN32
      SetEnvironmentVariable(_T("NXAGENT_EXTENSION_PORT"), nullptr);
      SetEnvironmentVariable(_T("NXAGENT_EXTENSION_TOKEN"), nullptr);
      for (int i = 0; i < setKeys.size(); i++)
      {
         SetEnvironmentVariable(setKeys.get(i), nullptr);
      }
#endif

#ifdef UNICODE
      MemFree(tokenU);
#endif
      return ok;
   }

   /**
    * Send SIGTERM to the process group, wait, then escalate to SIGKILL via stop().
    */
   void terminateGracefully(uint32_t timeoutMs)
   {
#ifndef _WIN32
      pid_t pid = getProcessId();
      if (pid > 0 && isRunning())
      {
         if (kill(-pid, SIGTERM) == 0)
            nxlog_debug_tag(DEBUG_TAG, 5, _T("Extension(%s): SIGTERM sent to pgid %u"), m_extensionName.cstr(), static_cast<unsigned>(pid));
         if (waitForCompletion(timeoutMs))
            return;
      }
#endif
      stop();   // SIGKILL on Unix; TerminateProcess on Windows
   }
};

/**
 * PID of the spawned child (0 if not in spawn mode or not running).
 */
uint32_t AgentExtension::getProcessPid() const
{
   return (m_process != nullptr) ? static_cast<uint32_t>(m_process->getProcessId()) : 0;
}

/**
 * Per-extension supervisor — drives the spawn/connect → ioLoop → backoff → retry cycle.
 */
void AgentExtension::supervisorLoop()
{
   ThreadSetName("ext-sup");
   nxlog_debug_tag(getDebugTag(), 3, _T("Extension(%s): supervisor started"), m_config.name);

   while (!m_stopRequested)
   {
      if (setupConnection())
      {
         m_currentBackoffMs = 0;          // successful setup resets backoff
         ioLoop();                         // returns when disconnected or stop requested
      }
      tearDownConnection();

      if (m_stopRequested)
         break;

      applyBackoff();
   }

   nxlog_debug_tag(getDebugTag(), 3, _T("Extension(%s): supervisor exiting"), m_config.name);
}

/**
 * Establish a connection (spawn child if needed, or connect to operator-managed listener).
 */
bool AgentExtension::setupConnection()
{
   if (m_config.mode == ExtensionMode::SPAWN)
      return spawnAndConnect();
   else
      return connectOnly();
}

/**
 * Spawn the child process and connect to its loopback port.
 */
bool AgentExtension::spawnAndConnect()
{
   setState(ExtensionState::STARTING);

   uint16_t port;
   if (!ReservePort(&port))
   {
      setLastError(_T("could not reserve loopback port"));
      return false;
   }
   m_assignedPort = port;
   m_runtimeToken = GenerateExtensionToken();

   if (m_config.command.isEmpty())
   {
      setLastError(_T("spawn mode requires Command"));
      return false;
   }

   m_process = new ExtensionProcess(m_config.command.cstr(), m_config.name, getDebugTag());
   if (!m_process->launch(port, m_runtimeToken.cstr(), m_config.environment, m_config.runAs.cstr()))
   {
      setLastError(_T("failed to spawn extension process"));
      delete m_process;
      m_process = nullptr;
      return false;
   }
   nxlog_debug_tag(getDebugTag(), 3, _T("Extension(%s): spawned PID=%u, port=%u"),
            m_config.name, static_cast<unsigned>(m_process->getProcessId()), port);

   // Connect with retries — child needs a moment to bind.
   InetAddress loopback = InetAddress::LOOPBACK;
   setState(ExtensionState::CONNECTING);
   for (int i = 0; i < POSTSPAWN_CONNECT_TRIES; i++)
   {
      if (m_stopRequested)
         return false;
      if (!m_process->isRunning())
      {
         setLastError(_T("child exited before accepting connection"));
         return false;
      }
      m_socket = ConnectToHost(loopback, port, CONNECT_TIMEOUT_MS);
      if (m_socket != INVALID_SOCKET)
         return true;
      ThreadSleepMs(POSTSPAWN_CONNECT_INTERVAL);
   }
   setLastError(_T("could not connect to spawned extension on port %u"), port);
   return false;
}

/**
 * Connect to an operator-managed extension listener.
 */
bool AgentExtension::connectOnly()
{
   setState(ExtensionState::CONNECTING);

   if (!m_config.endpointAddr.isValid() || m_config.endpointPort == 0)
   {
      setLastError(_T("invalid endpoint address/port"));
      return false;
   }
   if (!m_config.endpointAddr.isLoopback() && !m_config.allowRemote)
   {
      setLastError(_T("non-loopback endpoint requires AllowRemote=yes"));
      return false;
   }

   m_runtimeToken = m_config.token;

   m_socket = ConnectToHost(m_config.endpointAddr, m_config.endpointPort, CONNECT_TIMEOUT_MS);
   if (m_socket == INVALID_SOCKET)
   {
      setLastError(_T("connect to %s:%u failed"),
               static_cast<const TCHAR*>(m_config.endpointAddr.toString()),
               m_config.endpointPort);
      return false;
   }
   return true;
}

/**
 * Close socket, terminate child, clear capabilities.
 */
void AgentExtension::tearDownConnection()
{
   clearCapabilities();

   // Best-effort graceful shutdown signal while the socket is still up
   // (only relevant when stop() is called on a live connection).
   if (m_socket != INVALID_SOCKET && m_process != nullptr)
      notify("shutdown", nullptr);

   if (m_socket != INVALID_SOCKET)
   {
      closesocket(m_socket);
      m_socket = INVALID_SOCKET;
   }

   if (m_process != nullptr)
   {
      if (m_process->isRunning())
         m_process->terminateGracefully(m_config.shutdownTimeoutMs);
      delete m_process;
      m_process = nullptr;
   }
}

/**
 * Sleep with exponential backoff — wakeable on stop.
 */
void AgentExtension::applyBackoff()
{
   if (m_currentBackoffMs == 0)
   {
      m_currentBackoffMs = (m_config.mode == ExtensionMode::SPAWN) ? m_config.restartDelayMs : m_config.reconnectDelayMs;
   }
   else
   {
      uint32_t maxMs = (m_config.mode == ExtensionMode::SPAWN) ? m_config.restartDelayMaxMs : m_config.reconnectDelayMaxMs;
      m_currentBackoffMs *= 2;
      if (m_currentBackoffMs > maxMs)
         m_currentBackoffMs = maxMs;
   }
   m_restartCount++;
   setState(ExtensionState::RECONNECT_BACKOFF);
   nxlog_debug_tag(getDebugTag(), 3, _T("Extension(%s): backing off for %u ms (restart #%u)"), m_config.name, m_currentBackoffMs, m_restartCount);
   m_stopCondition.wait(m_currentBackoffMs);
}
