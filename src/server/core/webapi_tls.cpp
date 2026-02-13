/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
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
** File: webapi_tls.cpp
**
**/

#include "nxcore.h"

#define DEBUG_TAG_WEBAPI_TLS   L"webapi.tls"

/**
 * Loaded server config
 */
extern Config g_serverConfig;

/**
 * Configuration
 */
static bool s_tlsEnabled = false;
static uint16_t s_tlsPort = 8443;
static InetAddress s_tlsAddress = InetAddress::NONE;
static TCHAR s_tlsCertificate[MAX_PATH] = L"";
static TCHAR s_tlsCertificateKey[MAX_PATH] = L"";
static TCHAR s_reproxyPath[MAX_PATH] = L"reproxy";

/**
 * Monitor thread state
 */
static THREAD s_monitorThread = INVALID_THREAD_HANDLE;
static Condition s_shutdownCondition(true);
static bool s_stopping = false;
static Mutex s_executorLock;
static ProcessExecutor *s_executor = nullptr;

/**
 * Process executor subclass that captures reproxy output and logs it
 */
class ReproxyExecutor : public ProcessExecutor
{
private:
   char m_lineBuffer[4096];
   size_t m_linePos;

   void processLine()
   {
      m_lineBuffer[m_linePos] = 0;
      if (m_linePos > 0)
         nxlog_write_tag(NXLOG_INFO, L"webapi.reproxy", L"%hs", m_lineBuffer);
      m_linePos = 0;
   }

protected:
   virtual void onOutput(const char *text, size_t length) override
   {
      for (size_t i = 0; i < length; i++)
      {
         if (text[i] == '\n')
         {
            processLine();
         }
         else if (text[i] != '\r')
         {
            if (m_linePos < sizeof(m_lineBuffer) - 1)
               m_lineBuffer[m_linePos++] = text[i];
         }
      }
   }

   virtual void endOfOutput() override
   {
      if (m_linePos > 0)
         processLine();
   }

public:
   ReproxyExecutor(const wchar_t *cmd) : ProcessExecutor(cmd, false)
   {
      m_sendOutput = true;
      m_linePos = 0;
   }
};

/**
 * Build reproxy command line
 */
static StringBuffer BuildReproxyCommand(uint16_t webApiPort, const InetAddress& webApiAddress)
{
   StringBuffer cmd(L"['");
   cmd.append(s_reproxyPath);
   cmd.append(L"', '--listen=");
   cmd.append(s_tlsAddress.toString());
   cmd.append(L":");
   cmd.append(s_tlsPort);
   cmd.append(L"', '--ssl.type=static', '--ssl.cert=");
   cmd.append(s_tlsCertificate);
   cmd.append(L"', '--ssl.key=");
   cmd.append(s_tlsCertificateKey);
   cmd.append(L"', '--static.enabled', '--static.rule=*,^/(.*),http://");
   if (webApiAddress.isLoopback() || webApiAddress.isAnyLocal())
      cmd.append(L"127.0.0.1");
   else
      cmd.append(webApiAddress.toString());
   cmd.append(L":");
   cmd.append(webApiPort);
   cmd.append(L"/$1']");
   return cmd;
}

/**
 * Reproxy monitor thread
 */
static void ReproxyMonitorThread(uint16_t webApiPort, InetAddress webApiAddress)
{
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_WEBAPI_TLS, L"Reproxy monitor thread started");

   uint32_t restartDelay = 0;
   const uint32_t maxDelay = 300000;

   while(true)
   {
      if (restartDelay > 0)
      {
         nxlog_debug_tag(DEBUG_TAG_WEBAPI_TLS, 3, L"Waiting %u seconds before restarting reproxy", restartDelay / 1000);
         if (s_shutdownCondition.wait(restartDelay))
            break;
      }

      if (s_stopping)
         break;

      StringBuffer cmd = BuildReproxyCommand(webApiPort, webApiAddress);
      nxlog_debug_tag(DEBUG_TAG_WEBAPI_TLS, 5, L"Starting reproxy: %s", cmd.cstr());

      ReproxyExecutor *executor = new ReproxyExecutor(cmd);

      s_executorLock.lock();
      s_executor = executor;
      s_executorLock.unlock();

      if (!executor->execute())
      {
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_WEBAPI_TLS, L"Failed to start reproxy process");

         s_executorLock.lock();
         s_executor = nullptr;
         s_executorLock.unlock();
         delete executor;

         restartDelay = (restartDelay == 0) ? 5000 : std::min(restartDelay * 2, maxDelay);
         continue;
      }

      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_WEBAPI_TLS, L"Reproxy process started (PID %d)", executor->getProcessId());

      time_t startTime = time(nullptr);
      executor->waitForCompletion(INFINITE);

      if (s_stopping)
      {
         s_executorLock.lock();
         s_executor = nullptr;
         s_executorLock.unlock();
         delete executor;
         break;
      }

      uint32_t exitCode = executor->getExitCode();
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_WEBAPI_TLS, L"Reproxy process exited with code %u", exitCode);

      s_executorLock.lock();
      s_executor = nullptr;
      s_executorLock.unlock();
      delete executor;

      if (time(nullptr) - startTime > 60)
         restartDelay = 5000;
      else
         restartDelay = (restartDelay == 0) ? 5000 : std::min(restartDelay * 2, maxDelay);
   }

   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_WEBAPI_TLS, L"Reproxy monitor thread stopped");
}

/**
 * Read TLS configuration. Should be called during InitWebAPI().
 */
bool ReadTLSConfiguration()
{
   s_tlsEnabled = g_serverConfig.getValueAsBoolean(L"/WEBAPI/TLSEnable", false);
   if (!s_tlsEnabled)
      return true;

   s_tlsPort = static_cast<uint16_t>(g_serverConfig.getValueAsInt(L"/WEBAPI/TLSPort", 8443));

   const wchar_t *addr = g_serverConfig.getValue(L"/WEBAPI/TLSAddress", L"0.0.0.0");
   InetAddress a = InetAddress::parse(addr);
   if (a.isValid())
   {
      s_tlsAddress = a;
   }
   else
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_WEBAPI_TLS, L"Invalid TLS listener address \"%s\"", addr);
      s_tlsEnabled = false;
      return false;
   }

   const wchar_t *cert = g_serverConfig.getValue(L"/WEBAPI/TLSCertificate", L"");
   const wchar_t *key = g_serverConfig.getValue(L"/WEBAPI/TLSCertificateKey", L"");

   if (cert[0] == 0 || key[0] == 0)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_WEBAPI_TLS, L"TLS is enabled but TLSCertificate and/or TLSCertificateKey are not configured");
      s_tlsEnabled = false;
      return false;
   }

   wcslcpy(s_tlsCertificate, cert, MAX_PATH);
   wcslcpy(s_tlsCertificateKey, key, MAX_PATH);

   const wchar_t *reproxyPath = g_serverConfig.getValue(L"/WEBAPI/ReproxyPath");
   if (reproxyPath != nullptr)
   {
      wcslcpy(s_reproxyPath, reproxyPath, MAX_PATH);
   }
   else
   {
#ifdef _WIN32
      GetNetXMSDirectory(nxDirBin, s_reproxyPath);
      wcslcat(s_reproxyPath, L"\\reproxy.exe", MAX_PATH);
#else
      wcscpy(s_reproxyPath, L"/usr/bin/reproxy");
#endif
   }

   nxlog_debug_tag(DEBUG_TAG_WEBAPI_TLS, 1, L"TLS enabled on %s:%u (certificate: %s, key: %s, reproxy: %s)",
      addr, s_tlsPort, s_tlsCertificate, s_tlsCertificateKey, s_reproxyPath);

   return true;
}

/**
 * Start reproxy process for TLS termination. Should be called from StartWebAPI().
 */
void StartReproxy(uint16_t webApiPort, const InetAddress& webApiAddress)
{
   if (!s_tlsEnabled)
      return;

   if (!webApiAddress.isLoopback())
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_WEBAPI_TLS,
         L"TLS is enabled but WebAPI HTTP listener is bound to a non-loopback address - unencrypted HTTP traffic may be exposed");

   s_monitorThread = ThreadCreateEx(ReproxyMonitorThread, webApiPort, webApiAddress);
}

/**
 * Stop reproxy process. Should be called from ShutdownWebAPI().
 */
void StopReproxy()
{
   if (!s_tlsEnabled)
      return;

   nxlog_debug_tag(DEBUG_TAG_WEBAPI_TLS, 1, L"Stopping reproxy");

   s_stopping = true;
   s_shutdownCondition.set();

   s_executorLock.lock();
   if (s_executor != nullptr)
      s_executor->stop();
   s_executorLock.unlock();

   ThreadJoin(s_monitorThread);
   s_monitorThread = INVALID_THREAD_HANDLE;

   nxlog_debug_tag(DEBUG_TAG_WEBAPI_TLS, 1, L"Reproxy stopped");
}
