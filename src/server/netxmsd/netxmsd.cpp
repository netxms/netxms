/* 
** NetXMS - Network Management System
** Server startup module
** Copyright (C) 2003-2011 NetXMS Team
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

#if HAVE_GETOPT_H
#include <getopt.h>
#endif

#ifdef _WIN32
#include <dbghelp.h>
#endif

#ifdef __sun
#include <signal.h>
#endif


//
// Global data
//

BOOL g_bCheckDB = FALSE;


//
// Help text
//

static TCHAR help_text[] = _T("NetXMS Server Version ") NETXMS_VERSION_STRING _T("\n")
                           _T("Copyright (c) 2003, 2004, 2005, 2006, 2007, 2008 NetXMS Team\n\n")
                           _T("Usage: netxmsd [<options>]\n\n")
                           _T("Valid options are:\n")
                           _T("   -e          : Run database check on startup\n")
                           _T("   -c <file>   : Set non-default configuration file\n")
                           _T("               : Default is ") DEFAULT_CONFIG_FILE _T("\n")
                           _T("   -d          : Run as daemon/service\n")
                           _T("   -D <level>  : Set debug level (valid levels are 0..9)\n")
                           _T("   -h          : Display help and exit\n")
#ifdef _WIN32
                           _T("   -I          : Install Windows service\n")
                           _T("   -L <user>   : Login name for service account.\n")
                           _T("   -P <passwd> : Password for service account.\n")
#else
                           _T("   -p <file>   : Specify pid file.\n")
#endif
#ifdef _WIN32
                           _T("   -R          : Remove Windows service\n")
                           _T("   -s          : Start Windows service\n")
                           _T("   -S          : Stop Windows service\n")
#endif
                           _T("   -v          : Display version and exit\n")
                           _T("\n");


//
// Execute command and wait
//

static BOOL ExecAndWait(char *pszCommand)
{
   BOOL bSuccess = TRUE;

#ifdef _WIN32
   STARTUPINFOA si;
   PROCESS_INFORMATION pi;

   // Fill in process startup info structure
   memset(&si, 0, sizeof(STARTUPINFO));
   si.cb = sizeof(STARTUPINFO);
   si.dwFlags = 0;

   // Create new process
   if (!CreateProcessA(NULL, pszCommand, NULL, NULL, FALSE,
                       IsStandalone() ? 0 : CREATE_NO_WINDOW | DETACHED_PROCESS,
                       NULL, NULL, &si, &pi))
   {
      bSuccess = FALSE;
   }
   else
   {
      WaitForSingleObject(pi.hProcess, INFINITE);

      // Close all handles
      CloseHandle(pi.hThread);
      CloseHandle(pi.hProcess);
   }
#else
   bSuccess = (system(pszCommand) != -1);
#endif

   return bSuccess;
}


//
// Create minidump of given process
//

#ifdef _WIN32

static void CreateMiniDump(DWORD pid)
{
   HANDLE hFile, hProcess;
	TCHAR error[256];

	_tprintf(_T("INFO: Starting minidump for process %d\n"), pid);
	hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (hProcess != NULL)
	{
		hFile = CreateFile(_T("C:\\netxmsd.mdmp"), GENERIC_WRITE, 0, NULL,
								 CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			MiniDumpWriteDump(hProcess, pid, hFile, MiniDumpNormal, NULL, NULL, NULL);
			CloseHandle(hFile);
			_tprintf(_T("INFO: Minidump created successfully\n"));
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


//
// Parse command line
// Returns TRUE on success and FALSE on failure
//

#ifdef _WIN32
#define VALID_OPTIONS   "c:CdD:ehIL:P:RsSv"
#else
#define VALID_OPTIONS   "c:CdD:ehp:v"
#endif

static BOOL ParseCommandLine(int argc, char *argv[])
{
   int ch;
   char *eptr;
#ifdef _WIN32
	TCHAR login[256] = _T(""), password[256] = _T("");
	char exePath[MAX_PATH], dllPath[MAX_PATH], *ptr;
	BOOL useLogin = FALSE;
#endif
#if defined(_WIN32) || HAVE_DECL_GETOPT_LONG
	static struct option longOptions[] =
	{
		{ "check-db", 0, NULL, 'e' },
		{ "config", 1, NULL, 'c' },
		{ "daemon", 0, NULL, 'd' },
		{ "debug", 1, NULL, 'D' },
		{ "help", 0, NULL, 'h' },
#ifdef _WIN32
		{ "check-service", 0, NULL, '!' },
		{ "dump", 1, NULL, '~' },
		{ "install", 0, NULL, 'I' },
		{ "login", 1, NULL, 'L' },
		{ "password", 1, NULL, 'P' },
		{ "remove", 0, NULL, 'R' },
		{ "start", 0, NULL, 's' },
		{ "stop", 0, NULL, 'S' },
#else
		{ _T("pid-file"), 1, NULL, 'p' },
#endif
		{ NULL, 0, 0, 0 }
	};
#endif
   
#if defined(_WIN32) || HAVE_DECL_GETOPT_LONG
   while((ch = getopt_long(argc, argv, VALID_OPTIONS, longOptions, NULL)) != -1)
#else
   while((ch = getopt(argc, argv, VALID_OPTIONS)) != -1)
#endif
   {
   	switch(ch)
   	{
			case 'h':
	         _tprintf(help_text);
	         return FALSE;
			case 'v':
				_tprintf(_T("NetXMS Server Version ") NETXMS_VERSION_STRING _T(" Build of %hs\n"), __DATE__);
				return FALSE;
			case 'c':
#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, g_szConfigFile, MAX_PATH);
				g_szConfigFile[MAX_PATH - 1] = 0;
#else
				nx_strncpy(g_szConfigFile, optarg, MAX_PATH);
#endif
				break;
			case 'C':	// Check config
				g_dwFlags &= ~AF_DAEMON;
				_tprintf(_T("Checking configuration file (%s):\n\n"), g_szConfigFile);
				LoadConfig();
				return FALSE;
			case 'd':
				g_dwFlags |= AF_DAEMON;
				break;
			case 'D':	// Debug level
				g_nDebugLevel = strtol(optarg, &eptr, 0);
				if ((*eptr != 0) || (g_nDebugLevel < 0) || (g_nDebugLevel > 9))
				{
					_tprintf(_T("Invalid debug level \"%hs\" - should be in range 0..9\n"), optarg);
					g_nDebugLevel = 0;
				}
				break;
			case 'e':
				g_bCheckDB = TRUE;
				break;
#ifdef _WIN32
			case 'L':
#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, login, 256);
				login[255] = 0;
#else
				nx_strncpy(login, optarg, 256);
#endif
				useLogin = TRUE;
				break;
			case 'P':
#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, password, 256);
				password[255] = 0;
#else
				nx_strncpy(password, optarg, 256);
#endif
				break;
			case 'I':	// Install service
				ptr = strrchr(argv[0], '\\');
				if (ptr != NULL)
					ptr++;
				else
					ptr = argv[0];

				_fullpath(exePath, ptr, 255);

				if (stricmp(&exePath[strlen(exePath)-4], ".exe"))
					strcat(exePath, ".exe");
				strcpy(dllPath, exePath);
				ptr = strrchr(dllPath, '\\');
				if (ptr != NULL)  // Shouldn't be NULL
				{
					ptr++;
					strcpy(ptr, "libnxsrv.dll");
				}
#ifdef UNICODE
				WCHAR wexePath[MAX_PATH], wdllPath[MAX_PATH];
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, exePath, -1, wexePath, MAX_PATH);
				wexePath[MAX_PATH - 1] = 0;
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, dllPath, -1, wdllPath, MAX_PATH);
				wdllPath[MAX_PATH - 1] = 0;
				InstallService(wexePath, wdllPath, useLogin ? login : NULL, useLogin ? password : NULL);
#else
				InstallService(exePath, dllPath, useLogin ? login : NULL, useLogin ? password : NULL);
#endif
				return FALSE;
			case 'R':	// Remove service
				RemoveService();
				return FALSE;
			case 's':	// Start service
				StartCoreService();
				return FALSE;
			case 'S':	// Stop service
				StopCoreService();
				return FALSE;
			case '!':	// Check service configuration (for migration from pre-0.2.20)
				CheckServiceConfig();
				return FALSE;
			case '~':
				CreateMiniDump(strtoul(optarg, NULL, 0));
				return FALSE;
#endif
   		default:
   			break;
   	}
   }
   return TRUE;
}


//
// Startup code
//

int main(int argc, char *argv[])
{
#ifdef _WIN32
   HKEY hKey;
   DWORD dwSize;
#else
   int i;
   FILE *fp;
   char *pszEnv;
#endif

#ifdef __sun
   signal(SIGPIPE, SIG_IGN);
   signal(SIGHUP, SIG_IGN);
   signal(SIGINT, SIG_IGN);
   signal(SIGQUIT, SIG_IGN);
   signal(SIGUSR1, SIG_IGN);
   signal(SIGUSR2, SIG_IGN);
#endif

   InitThreadLibrary();

#ifdef NETXMS_MEMORY_DEBUG
	InitMemoryDebugger();
#endif

   // Check for alternate config file location
#ifdef _WIN32
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\NetXMS\\Server"), 0,
                    KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
   {
      dwSize = MAX_PATH * sizeof(TCHAR);
      RegQueryValueEx(hKey, _T("ConfigFile"), NULL, NULL, (BYTE *)g_szConfigFile, &dwSize);
      RegCloseKey(hKey);
   }
#else
   pszEnv = _tgetenv(_T("NETXMSD_CONFIG"));
   if (pszEnv != NULL)
      nx_strncpy(g_szConfigFile, pszEnv, MAX_PATH);
#endif

   if (!ParseCommandLine(argc, argv))
      return 1;

   if (!LoadConfig())
   {
      if (IsStandalone())
         _tprintf(_T("Error loading configuration file\n"));
      return 1;
   }

	// Set exception handler
#ifdef _WIN32
	if (g_dwFlags & AF_CATCH_EXCEPTIONS)
		SetExceptionHandler(SEHServiceExceptionHandler, SEHServiceExceptionDataWriter,
		                    g_szDumpDir, _T("netxmsd"), MSG_EXCEPTION, g_dwFlags & AF_WRITE_FULL_DUMP, IsStandalone());
	__try {
#endif

   // Check database before start if requested
   if (g_bCheckDB)
   {
      char szCmd[MAX_PATH + 128], *pszSep;

      strncpy(szCmd, argv[0], MAX_PATH);
      pszSep = strrchr(szCmd, FS_PATH_SEPARATOR[0]);
      if (pszSep != NULL)
         pszSep++;
      else
         pszSep = szCmd;
#ifdef UNICODE
      snprintf(pszSep, 128, "nxdbmgr -c \"%S\" -f check", g_szConfigFile);
#else
      snprintf(pszSep, 128, "nxdbmgr -c \"%s\" -f check", g_szConfigFile);
#endif
      if (!ExecAndWait(szCmd))
         if (IsStandalone())
            _tprintf(_T("ERROR: Failed to execute command \"%hs\"\n"), szCmd);
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
         _tprintf(_T("NetXMS Core initialization failed\n"));

         // Remove database lock
         if (g_dwFlags & AF_DB_LOCKED)
         {
            UnlockDB();
            ShutdownDB();
         }
         return 3;
      }
      Main(NULL);
   }
#else    /* not _WIN32 */
   if (!IsStandalone())
   {
      if (daemon(0, 0) == -1)
      {
         perror("Call to daemon() failed");
         return 2;
      }
   }

   // Write PID file
   fp = _tfopen(g_szPIDFile, _T("w"));
   if (fp != NULL)
   {
      _ftprintf(fp, _T("%d"), getpid());
      fclose(fp);
   }

   // Initialize server
   if (!Initialize())
   {
      // Remove database lock
      if (g_dwFlags & AF_DB_LOCKED)
      {
         UnlockDB();
         ShutdownDB();
      }
      return 3;
   }

   // Everything is OK, start common main loop
   StartMainLoop(SignalHandler, Main);
#endif   /* _WIN32 */

#ifdef _WIN32
	LIBNETXMS_EXCEPTION_HANDLER
#endif
   return 0;
}
