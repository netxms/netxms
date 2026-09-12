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
** File: test-server.cpp
**
** Test launcher for server core code that needs an initialized server. It takes the
** place of netxmsd: prepares a SQLite database with test data in a scratch work
** directory, runs the regular server initialization, executes the tests inside the
** running server, and shuts the server down.
**
** Options:
**    -D <level>   server debug level (written to the log file in the work directory)
**    -k           keep the work directory after a successful run
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <nxcpapi.h>
#include <nxproc.h>
#include <nms_core.h>
#include <testtools.h>

#ifndef _WIN32
#include <unistd.h>
#endif

void TestAuthenticationTokens();
void TestNXSLHttp();

/**
 * Scratch work directory holding the configuration file, database, data directory and log
 */
static TCHAR s_workDir[MAX_PATH];
static bool s_keepWorkDir = false;

/**
 * Set once all tests have passed and the server is being shut down
 */
static bool s_serverStopping = false;

/**
 * Run database manager against the test database. Output is suppressed; a non-zero exit code is a failure.
 */
static bool RunDatabaseManager(const TCHAR *configFile, const TCHAR *arguments)
{
   TCHAR binDir[MAX_PATH];
   GetNetXMSDirectory(nxDirBin, binDir);

   StringBuffer command;
   command.append(_T('"'));
   command.append(binDir);
   command.append(FS_PATH_SEPARATOR _T("nxdbmgr\" -c \""));
   command.append(configFile);
   command.append(_T("\" -q "));
   command.append(arguments);

   ProcessExecutor executor(command, false);
   if (!executor.execute())
   {
      WriteToTerminalEx(_T("Cannot execute \"%s\"\n"), command.cstr());
      return false;
   }
   executor.waitForCompletion(INFINITE);
   if (executor.getExitCode() != 0)
   {
      WriteToTerminalEx(_T("Command \"%s\" failed with exit code %u\n"), command.cstr(), executor.getExitCode());
      return false;
   }
   return true;
}

/**
 * Write server configuration file. Listeners are bound to loopback only, the local
 * administration interface and the web API are disabled, and everything the server writes
 * stays inside the work directory.
 */
static bool WriteConfigFile(const TCHAR *configFile)
{
   StringBuffer content;
   content.append(_T("DBDriver = sqlite\n"));
   content.append(_T("DBName = "));
   content.append(s_workDir);
   content.append(FS_PATH_SEPARATOR _T("netxms.db\n"));
   content.append(_T("DataDirectory = "));
   content.append(s_workDir);
   content.append(FS_PATH_SEPARATOR _T("data\n"));
   content.append(_T("LogFile = "));
   content.append(s_workDir);
   content.append(FS_PATH_SEPARATOR _T("netxmsd.log\n"));
   content.append(_T("ListenAddress = 127.0.0.1\n"));
   content.append(_T("LocalAdminInterface = no\n"));
   content.append(_T("DBCacheConfigurationTables = no\n"));
   content.append(_T("\n[WEBAPI]\n"));
   content.append(_T("Enable = no\n"));

   char *utf8 = UTF8StringFromTString(content);
   SaveFileStatus status = SaveFile(configFile, utf8, strlen(utf8), false);
   MemFree(utf8);
   if (status != SaveFileStatus::SUCCESS)
   {
      WriteToTerminalEx(_T("Cannot write configuration file \"%s\"\n"), configFile);
      return false;
   }
   return true;
}

/**
 * Create and populate test database. Remaining network listeners are moved away from the
 * default ports so that the launcher can run next to a regular server; the SNMP trap
 * receiver is disabled because its default port is privileged.
 */
static bool PrepareDatabase(const TCHAR *configFile)
{
   static const TCHAR *settings[] =
   {
      _T("set Client.ListenerPort 14701"),
      _T("set AgentTunnels.ListenPort 14703"),
      _T("set MobileDeviceListenerPort 14747"),
      _T("set SNMP.Traps.Enable 0"),
      nullptr
   };

   if (!RunDatabaseManager(configFile, _T("init")))
      return false;

   for(int i = 0; settings[i] != nullptr; i++)
   {
      if (!RunDatabaseManager(configFile, settings[i]))
         return false;
   }
   return true;
}

/**
 * Failed assertion inside the running server: bring the server down before the test
 * process exits, so that the database is unlocked and the log is flushed. The work
 * directory is kept for inspection.
 */
static void TestFailureHandler()
{
   WriteToTerminalEx(_T("Server work directory \"%s\" is kept for inspection\n"), s_workDir);
   FastShutdown(ShutdownReason::OTHER);
}

/**
 * Process exit handler. Shutdown() terminates the process itself, so completion of the
 * shutdown step and removal of the work directory happen here. On the failure path
 * (assertion failure or initialization error) nothing is done and the work directory
 * is left in place.
 */
static void ExitHandler()
{
   if (!s_serverStopping)
      return;
   EndTest();
   if (!s_keepWorkDir)
      DeleteDirectoryTree(s_workDir);
}

/**
 * Entry point
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(false);

   int debugLevel = 0;
   for(int i = 1; i < argc; i++)
   {
      if (!strcmp(argv[i], "-D") && (i + 1 < argc))
      {
         debugLevel = strtol(argv[++i], nullptr, 10);
      }
      else if (!strcmp(argv[i], "-k"))
      {
         s_keepWorkDir = true;
      }
      else
      {
         WriteToTerminal(_T("Usage: test-server [-D <debug level>] [-k]\n"));
         return 1;
      }
   }

#ifdef _WIN32
   String envTempDir = GetEnvironmentVariableEx(_T("TEMP"));
   const TCHAR *tempDir = envTempDir.cstr();
   uint32_t pid = GetCurrentProcessId();
#else
   String envTempDir = GetEnvironmentVariableEx(_T("TMPDIR"));
   const TCHAR *tempDir = envTempDir.isEmpty() ? _T("/tmp") : envTempDir.cstr();
   uint32_t pid = static_cast<uint32_t>(getpid());
#endif
   _sntprintf(s_workDir, MAX_PATH, _T("%s") FS_PATH_SEPARATOR _T("netxms-test-server-%u"), tempDir, pid);
   if (!CreateDirectoryTree(s_workDir))
   {
      WriteToTerminalEx(_T("Cannot create work directory \"%s\"\n"), s_workDir);
      return 1;
   }

   atexit(ExitHandler);

   TCHAR configFile[MAX_PATH];
   _sntprintf(configFile, MAX_PATH, _T("%s") FS_PATH_SEPARATOR _T("netxmsd.conf"), s_workDir);

   StartTest(_T("Test database preparation"));
   if (!WriteConfigFile(configFile) || !PrepareDatabase(configFile))
      return 2;
   EndTest();

   // Run as a daemon (without actually forking): the server then keeps its log out of the
   // test output instead of mirroring it to stdout, and Shutdown() returns on Windows
   g_flags |= AF_DAEMON;

   _tcslcpy(g_szConfigFile, configFile, MAX_PATH);
   int configDebugLevel;
   if (!LoadConfig(&configDebugLevel))
   {
      WriteToTerminal(_T("Error loading configuration file\n"));
      return 2;
   }
   nxlog_set_debug_level(debugLevel);

   StartTest(_T("Server initialization"));
   if (!Initialize())
   {
      WriteToTerminalEx(_T("\x1b[31;1mFAIL\x1b[0m\n   Server initialization failed, see log file in \"%s\"\n"), s_workDir);
      InitiateProcessShutdown();
      if (g_flags & AF_DB_LOCKED)
         UnlockDatabase();
      ShutdownDatabase();
      nxlog_close();
      return 3;
   }
   EndTest();

   SetTestFailureHook(TestFailureHandler);

   TestAuthenticationTokens();
   TestNXSLHttp();

   SetTestFailureHook(nullptr);

   s_serverStopping = true;
   StartTest(_T("Server shutdown"));
   Shutdown();   // Terminates the process on UNIX; ExitHandler() completes the run
   return 0;
}
