/* 
** NetXMS - Network Management System
** Server startup module
** Copyright (C) 2003-2025 Raden Solutions
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
** File: netxmsd.cpp
**
**/

#include "netxmsd.h"
#include <netxms_getopt.h>
#include <netxms-version.h>

#ifdef _WIN32
#include <dbghelp.h>
#include <client/windows/handler/exception_handler.h>
#if !_M_ARM64
#include <openssl/applink.c>
#endif
#endif

NETXMS_EXECUTABLE_HEADER(netxmsd)

/**
 * Global data
 */
bool g_checkDatabase = false;

/**
 * Debug level
 */
static int s_debugLevel = NXCONFIG_UNINITIALIZED_VALUE;
static TCHAR *s_debugTags = nullptr;

/**
 * Configuration entries
 */
static StringList *s_configEntries = nullptr;
static bool s_generateConfig = false;

/**
 * Help text
 */
static wchar_t s_helpText[] =
         L"NetXMS Server Version " NETXMS_VERSION_STRING L" Build " NETXMS_BUILD_TAG L"\n"
         L"Copyright (c) 2003-2025 Raden Solutions\n\n"
         L"Usage: netxmsd [<options>]\n\n"
         L"Valid options are:\n"
         L"   -A <entry>       : Add configuration file entry\n"
         L"   -e               : Run database check on startup\n"
         L"   -c <file>        : Set non-default configuration file\n"
         L"   -C               : Check configuration and exit\n"
         L"   -d               : Run as daemon/service\n"
         L"   -D <level>       : Set debug level (valid levels are 0..9)\n"
         L"   -G               : Generate configuration file and exit\n"
         L"   -h               : Display help and exit\n"
#ifdef _WIN32
         L"   -I               : Install Windows service\n"
         L"   -L <user>        : Login name for service account\n"
#endif
         L"   -l               : Show log file location\n"
#ifdef _WIN32
         L"   -m               : Ignore service start command if service is configured for manual start\n"
         L"   -M               : Create service with manual start\n"
         L"   -P <passwd>      : Password for service account\n"
#else
         L"   -p <file>        : Path to pid file (default: /var/run/netxmsd.pid)\n"
#endif
         L"   -q               : Disable interactive console\n"
#ifdef _WIN32
         L"   -R               : Remove Windows service\n"
         L"   -s               : Start Windows service\n"
         L"   -S               : Stop Windows service\n"
#else
#if WITH_SYSTEMD
         L"   -S               : Run as systemd daemon\n"
#endif
#endif
         L"   -t <tag>:<level> : Set debug level for specific tag\n"
         L"   -T               : Enable SQL query trace\n"
         L"   -v               : Display version and exit\n"
         L"\n";

#ifdef _WIN32

/**
* Create minidump of given process
*/
static void CreateMiniDump(DWORD pid)
{
   HANDLE hFile, hProcess;
	TCHAR error[256];

	_tprintf(_T("INFO: Starting minidump for process %d\n"), pid);
	hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (hProcess != NULL)
	{
      TCHAR fname[MAX_PATH];
      _sntprintf(fname, MAX_PATH, _T("%s\\netxmsd-%u-%I64u.mdmp"), g_szDumpDir, pid, static_cast<UINT64>(time(NULL)));
		hFile = CreateFile(fname, GENERIC_WRITE, 0, NULL,
								 CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
         static const char *comments = "Version=" NETXMS_VERSION_STRING_A "; BuildTag=" NETXMS_BUILD_TAG_A;
         MINIDUMP_USER_STREAM us;
         us.Type = CommentStreamA;
         us.Buffer = (void*)comments;
         us.BufferSize = static_cast<ULONG>(strlen(comments) + 1);

         MINIDUMP_USER_STREAM_INFORMATION usi;
         usi.UserStreamCount = 1;
         usi.UserStreamArray = &us;

			if (MiniDumpWriteDump(hProcess, pid, hFile, 
                   static_cast<MINIDUMP_TYPE>(MiniDumpWithFullMemory | MiniDumpWithHandleData | MiniDumpWithProcessThreadData),
                   NULL, &usi, NULL))
         {
   			CloseHandle(hFile);
            if (DeflateFile(fname))
               DeleteFile(fname);
   			_tprintf(_T("INFO: Minidump created successfully\n"));
         }
         else
         {
   			CloseHandle(hFile);
            DeleteFile(fname);
   			_tprintf(_T("INFO: Minidump creation failed\n"));
         }
		}
		else
		{
			_tprintf(_T("ERROR: cannot create file for minidump (%s)\n"), GetSystemErrorText(GetLastError(), error, 256));
		}
	}
	else
	{
		_tprintf(_T("ERROR: cannot open process %d (%s)\n"), pid, GetSystemErrorText(GetLastError(), error, 256));
	}
}

#endif

#ifdef _WIN32
#define VALID_OPTIONS   "A:c:CdD:eGhIlL:mMP:qRsSt:Tv"
#else
#define VALID_OPTIONS   "A:c:CdD:eGhlp:qSt:Tv"
#endif

/**
 * Parse command line
 * Returns TRUE on success and FALSE on failure
 */
static bool ParseCommandLine(int argc, char *argv[])
{
   int ch;
   char *eptr;
#ifdef _WIN32
	TCHAR login[256] = _T(""), password[256] = _T("");
	char exePath[MAX_PATH], dllPath[MAX_PATH], *ptr;
	BOOL useLogin = FALSE;
   bool installService = false;
   bool manualStart = false;
   bool startService = false;
   bool ignoreManualStartService = false;
#endif
#if defined(_WIN32) || HAVE_DECL_GETOPT_LONG
	static struct option longOptions[] =
	{
      { (char *)"add-config-entry", 1, NULL, 'A' },
		{ (char *)"check-db", 0, NULL, 'e' },
      { (char *)"config", 1, NULL, 'c' },
      { (char *)"check-config", 0, NULL, 'C' },
		{ (char *)"daemon", 0, NULL, 'd' },
		{ (char *)"debug", 1, NULL, 'D' },
      { (char *)"debug-tag", 1, NULL, 't' },
      { (char *)"generate-config", 0, NULL, 'G' },
		{ (char *)"help", 0, NULL, 'h' },
		{ (char *)"quiet", 1, NULL, 'q' },
      { (char *)"show-log-location", 0, NULL, 'l' },
      { (char *)"trace-sql", 0, NULL, 'T' },
      { (char *)"version", 0, NULL, 'v' },
#ifdef _WIN32
		{ (char *)"check-service", 0, NULL, '!' },
		{ (char *)"dump", 1, NULL, '~' },
		{ (char *)"dump-dir", 1, NULL, '@' },
		{ (char *)"ignore-manual-start", 0, NULL, 'm' },
		{ (char *)"install", 0, NULL, 'I' },
		{ (char *)"login", 1, NULL, 'L' },
      { (char *)"manual-start", 0, NULL, 'M' },
      { (char *)"password", 1, NULL, 'P' },
		{ (char *)"remove", 0, NULL, 'R' },
		{ (char *)"start", 0, NULL, 's' },
		{ (char *)"stop", 0, NULL, 'S' },
#else
		{ (char *)"pid-file", 1, NULL, 'p' },
		{ (char *)"systemd-daemon", 0, NULL, 'S' },
#endif
		{ NULL, 0, 0, 0 }
	};
#endif

#if defined(_WIN32) || HAVE_DECL_GETOPT_LONG
   while((ch = getopt_long(argc, argv, VALID_OPTIONS, longOptions, nullptr)) != -1)
#else
   while((ch = getopt(argc, argv, VALID_OPTIONS)) != -1)
#endif
   {
   	switch(ch)
   	{
			case 'h':
	         WriteToTerminal(s_helpText);
	         return false;
			case 'v':
            {
               WriteToTerminal(_T("NetXMS Server Version ") NETXMS_VERSION_STRING _T(" Build ") NETXMS_BUILD_TAG IS_UNICODE_BUILD_STRING _T("\n"));
               String ciphers = NXCPGetSupportedCiphersAsText();
               WriteToTerminalEx(_T("NXCP: %d.%d.%d.%d (%s)\n"),
                  NXCP_VERSION, CLIENT_PROTOCOL_VERSION_BASE, CLIENT_PROTOCOL_VERSION_MOBILE, CLIENT_PROTOCOL_VERSION_FULL,
                  ciphers.isEmpty() ? _T("NO ENCRYPTION") : ciphers.cstr());
               WriteToTerminalEx(_T("Built with: %hs\n"), CPP_COMPILER_VERSION);
            }
				return false;
			case 'A':
			   if (s_configEntries == nullptr)
			      s_configEntries = new StringList();
			   s_configEntries->addPreallocated(WideStringFromMBStringSysLocale(optarg));
			   break;
			case 'c':
				MultiByteToWideCharSysLocale(optarg, g_szConfigFile, MAX_PATH);
				g_szConfigFile[MAX_PATH - 1] = 0;
				break;
			case 'C':	// Check config
				g_flags &= ~AF_DAEMON;
				WriteToTerminalEx(_T("Checking configuration file (%s):\n\n"), g_szConfigFile);
				LoadConfig(&s_debugLevel);
				return FALSE;
			case 'd':
				g_flags |= AF_DAEMON;
				break;
			case 'D':	// Debug level
			   s_debugLevel = strtoul(optarg, &eptr, 0);
				if ((*eptr != 0) || (s_debugLevel > 9))
				{
					WriteToTerminalEx(_T("Invalid debug level \"%hs\" - should be in range 0..9\n"), optarg);
					s_debugLevel = 0;
				}
				break;
			case 'q': // disable interactive console
				g_flags |= AF_DEBUG_CONSOLE_DISABLED;
				break;
			case 'e':
				g_checkDatabase = true;
				break;
         case 'G':
            s_generateConfig = true;
            break;
#ifdef _WIN32
			case 'L':
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, login, 256);
				login[255] = 0;
				useLogin = true;
				break;
#endif
         case 'l':
            LoadConfig(&s_debugLevel);
            WriteToTerminalEx(_T("%s\n"), g_szLogFile);
            return false;
#ifdef _WIN32
         case 'm':
            ignoreManualStartService = true;
            break;
         case 'M':
            manualStart = true;
            break;
         case 'P':
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, password, 256);
				password[255] = 0;
				break;
			case 'I':	// Install service
            installService = true;
            break;
			case 'R':	// Remove service
				RemoveService();
				return FALSE;
			case 's':	// Start service
            startService = true;
            break;
			case 'S':	// Stop service
				StopCoreService();
				return false;
			case '!':	// Check service configuration (for migration from pre-0.2.20)
				CheckServiceConfig();
				return false;
         case '@':
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, g_szDumpDir, MAX_PATH);
            if (g_szDumpDir[_tcslen(g_szDumpDir) - 1] == _T('\\'))
            {
               g_szDumpDir[_tcslen(g_szDumpDir) - 1] = 0;
            }
            break;
			case '~':
				CreateMiniDump(strtoul(optarg, nullptr, 0));
				return false;
#else /* _WIN32 */
         case 'p':   // PID file
            MultiByteToWideCharSysLocale(optarg, g_szPIDFile, MAX_PATH);
            g_szPIDFile[MAX_PATH - 1] = 0;
            break;
			case 'S':
				g_flags |= AF_DAEMON | AF_SYSTEMD_DAEMON;
				break;
#endif   /* _WIN32 */
			case 't':
			   nxlog_set_debug_level_tag(optarg);
			   break;
         case 'T':
            DBEnableQueryTrace(true);
            nxlog_set_debug_level_tag(_T("db.query"), 9);
            break;
         default:
   			break;
   	}
   }

#ifdef _WIN32
   if (installService)
   {
      ptr = strrchr(argv[0], '\\');
      if (ptr != nullptr)
         ptr++;
      else
         ptr = argv[0];

      _fullpath(exePath, ptr, 255);

      if (stricmp(&exePath[strlen(exePath) - 4], ".exe"))
         strcat(exePath, ".exe");
      strcpy(dllPath, exePath);
      ptr = strrchr(dllPath, '\\');
      if (ptr != nullptr)  // Shouldn't be NULL
      {
         ptr++;
         strcpy(ptr, "libnxsrv.dll");
      }
      WCHAR wexePath[MAX_PATH], wdllPath[MAX_PATH];
      MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, exePath, -1, wexePath, MAX_PATH);
      wexePath[MAX_PATH - 1] = 0;
      MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, dllPath, -1, wdllPath, MAX_PATH);
      wdllPath[MAX_PATH - 1] = 0;
      InstallService(wexePath, wdllPath, useLogin ? login : NULL, useLogin ? password : NULL, manualStart);
      return false;
   }

   if (startService)
   {
      StartCoreService(ignoreManualStartService);
      return false;
   }
#endif

   return true;
}

/**
 * Debug writer used during process startup
 */
static void StartupDebugWriter(const TCHAR *tag, const TCHAR *format, va_list args)
{
   if (tag != nullptr)
      _tprintf(_T("[%s] "), tag);
   _vtprintf(format, args);
   _tprintf(_T("\n"));
}

/**
 * Startup code
 */
int main(int argc, char* argv[])
{
   InitNetXMSProcess(false);

#if HAVE_LIBEDIT && HAVE_FWIDE
   // Try to switch stdout to byte oriented mode
   fwide(stdout, -1);
#endif

   // Check for alternate config file location
#ifdef _WIN32
   HKEY hKey;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\NetXMS\\Server"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
   {
      DWORD dwSize = MAX_PATH * sizeof(TCHAR);
      RegQueryValueEx(hKey, _T("ConfigFile"), NULL, NULL, (BYTE *)g_szConfigFile, &dwSize);
      RegCloseKey(hKey);
   }
#else
   String configEnv = GetEnvironmentVariableEx(_T("NETXMSD_CONFIG"));
   if (!configEnv.isEmpty())
   {
      _tcslcpy(g_szConfigFile, configEnv, MAX_PATH);
   }
#endif

   if (!ParseCommandLine(argc, argv))
      return 1;

   if (IsStandalone())
      nxlog_set_debug_writer(StartupDebugWriter);

   if (s_generateConfig)
   {
      FindConfigFile();
      WriteToTerminalEx(_T("Using configuration file \"%s\"\n"), g_szConfigFile);

      FILE *fp = _wfopen(g_szConfigFile, L"w");
      if (fp == nullptr)
      {
         WriteToTerminalEx(_T("Cannot create file \"%s\" (%s)"), g_szConfigFile, _tcserror(errno));
         return 1;
      }

      time_t now = time(nullptr);
      fprintf(fp, "#\n# Configuration file generated on %s#\n\n", ctime(&now));
      if (s_configEntries != nullptr)
      {
         for(int i = 0; i < s_configEntries->size(); i++)
         {
            char *line = UTF8StringFromTString(s_configEntries->get(i));
            fprintf(fp, "%s\n", line);
            MemFree(line);
         }
      }

      fclose(fp);
      return 0;
   }

   if (!LoadConfig(&s_debugLevel))
   {
      if (IsStandalone())
         WriteToTerminal(_T("Error loading configuration file\n"));
      return 1;
   }
   delete s_configEntries;

   if (s_debugLevel == NXCONFIG_UNINITIALIZED_VALUE)
      s_debugLevel = 0;
   nxlog_set_debug_level(s_debugLevel);
   MemFree(s_debugTags);

	// Set exception handler
#ifdef _WIN32
   ProcessExecutor *crashServer = nullptr;
   google_breakpad::ExceptionHandler *exceptionHandler = nullptr;

   if (g_flags & AF_CATCH_EXCEPTIONS)
   {
      TCHAR pipeName[64];
      _sntprintf(pipeName, 64, _T("\\\\.\\pipe\\netxmsd-crashsrv-%u"), GetCurrentProcessId());

      TCHAR crashServerCmdLine[256];
      _sntprintf(crashServerCmdLine, 256, _T("nxcrashsrv.exe netxmsd-crashsrv-%u \"%s\""), GetCurrentProcessId(), g_szDumpDir);
      crashServer = new ProcessExecutor(crashServerCmdLine, false);
      if (crashServer->execute())
      {
         // Wait for server's named pipe to appear
         bool success = false;
         uint32_t timeout = 2000;
         while (timeout > 0)
         {
            if (WaitNamedPipe(pipeName, timeout))
            {
               success = true;
               break;   // Success
            }
            if (GetLastError() != ERROR_FILE_NOT_FOUND)
               break;   // Unrecoverable error
            Sleep(200);
            timeout -= 200;
         }
         if (success)
         {
            static google_breakpad::CustomInfoEntry clientInfoEntries[] = { { L"ProcessName", L"netxmsd" } };
            static google_breakpad::CustomClientInfo clientInfo = { clientInfoEntries, 1 };
            exceptionHandler = new google_breakpad::ExceptionHandler(g_szDumpDir, nullptr, nullptr, nullptr, google_breakpad::ExceptionHandler::HANDLER_EXCEPTION | google_breakpad::ExceptionHandler::HANDLER_PURECALL,
               static_cast<MINIDUMP_TYPE>(((g_flags & AF_WRITE_FULL_DUMP) ? MiniDumpWithFullMemory : MiniDumpNormal) | MiniDumpWithHandleData | MiniDumpWithProcessThreadData),
               pipeName, &clientInfo);
         }
         else
         {
            delete_and_null(crashServer);
         }
      }
      else
      {
         delete_and_null(crashServer);
      }
   }
#endif

   // Check database before start if requested
   if (g_checkDatabase)
   {
      char command[MAX_PATH + 128];
      strlcpy(command, argv[0], MAX_PATH);
      char *pathSeparator = strrchr(command, FS_PATH_SEPARATOR[0]);
      if (pathSeparator != nullptr)
         pathSeparator++;
      else
         pathSeparator = command;
      snprintf(pathSeparator, 128, "nxdbmgr -c \"%S\" -f check", g_szConfigFile);
      wchar_t *wcmd = WideStringFromMBStringSysLocale(command);
      ProcessExecutor executor(wcmd, false);
      MemFree(wcmd);
      if (executor.execute())
      {
         executor.waitForCompletion(INFINITE);
      }
      else
      {
         if (IsStandalone())
            WriteToTerminalEx(L"ERROR: Failed to execute command \"%hs\"\n", command);
      }
   }

#ifdef _WIN32
   if (!IsStandalone())
   {
      InitService();
   }
   else
   {
      if (!Initialize())
      {
         WriteToTerminal(_T("NetXMS Core initialization failed\n"));

         InitiateProcessShutdown();

         // Remove database lock
         if (g_flags & AF_DB_LOCKED)
            UnlockDatabase();
         ShutdownDatabase();
         nxlog_close();
         return 3;
      }
      Main(nullptr);
   }
#else    /* not _WIN32 */
   if (!IsStandalone() && !(g_flags & AF_SYSTEMD_DAEMON))
   {
      if (daemon(0, 0) == -1)
      {
         perror("Call to daemon() failed");
         return 2;
      }
   }

   // Write PID file
   FILE *fp = _wfopen(g_szPIDFile, L"w");
   if (fp != nullptr)
   {
      _ftprintf(fp, _T("%u"), getpid());
      fclose(fp);
   }

   // Initialize server
   if (!Initialize())
   {
      InitiateProcessShutdown();

      // Remove database lock
      if (g_flags & AF_DB_LOCKED)
         UnlockDatabase();

      ShutdownDatabase();
      nxlog_close();
      return 3;
   }

   // Everything is OK, start common main loop
   StartMainLoop(SignalHandler, Main);
#endif   /* _WIN32 */

#ifdef _WIN32
   delete exceptionHandler;
   delete crashServer;
#endif
   return 0;
}
