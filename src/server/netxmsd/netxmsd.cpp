/* 
** NetXMS - Network Management System
** Server startup module
** Copyright (C) 2003-2017 Raden Solutions
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

#if defined(_WITH_ENCRYPTION) && defined(_WIN32)
#include <openssl/applink.c>
#endif

/**
 * Global data
 */
BOOL g_bCheckDB = FALSE;

/**
 * Debug level
 */
static int s_debugLevel = NXCONFIG_UNINITIALIZED_VALUE;
static TCHAR *s_debugTags = NULL;

/**
 * Help text
 */
static TCHAR help_text[] = _T("NetXMS Server Version ") NETXMS_VERSION_STRING _T(" Build ") NETXMS_VERSION_BUILD_STRING _T(" (") NETXMS_BUILD_TAG _T(")") IS_UNICODE_BUILD_STRING _T("\n")
                           _T("Copyright (c) 2003-2015 Raden Solutions\n\n")
                           _T("Usage: netxmsd [<options>]\n\n")
                           _T("Valid options are:\n")
                           _T("   -e          : Run database check on startup\n")
                           _T("   -c <file>   : Set non-default configuration file\n")
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
                           _T("   -q          : Disable interactive console\n")
#ifdef _WIN32
                           _T("   -R          : Remove Windows service\n")
                           _T("   -s          : Start Windows service\n")
                           _T("   -S          : Stop Windows service\n")
#endif
                           _T("   -v          : Display version and exit\n")
                           _T("\n");

/**
 * Execute command and wait
 */
static BOOL ExecAndWait(char *pszCommand)
{
   BOOL bSuccess = TRUE;

#ifdef _WIN32
   STARTUPINFOA si;
   PROCESS_INFORMATION pi;

   // Fill in process startup info structure
   memset(&si, 0, sizeof(STARTUPINFOA));
   si.cb = sizeof(STARTUPINFOA);
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
#define VALID_OPTIONS   "c:CdD:ehIL:P:qRsSv"
#else
#define VALID_OPTIONS   "c:CdD:ehp:qv"
#endif

/**
 * Parse command line
 * Returns TRUE on success and FALSE on failure
 */
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
		{ (char *)"check-db", 0, NULL, 'e' },
		{ (char *)"config", 1, NULL, 'c' },
		{ (char *)"daemon", 0, NULL, 'd' },
		{ (char *)"debug", 1, NULL, 'D' },
		{ (char *)"help", 0, NULL, 'h' },
		{ (char *)"quiet", 1, NULL, 'q' },
#ifdef _WIN32
		{ (char *)"check-service", 0, NULL, '!' },
		{ (char *)"dump", 1, NULL, '~' },
		{ (char *)"dump-dir", 1, NULL, '@' },
		{ (char *)"install", 0, NULL, 'I' },
		{ (char *)"login", 1, NULL, 'L' },
		{ (char *)"password", 1, NULL, 'P' },
		{ (char *)"remove", 0, NULL, 'R' },
		{ (char *)"start", 0, NULL, 's' },
		{ (char *)"stop", 0, NULL, 'S' },
#else
		{ (char *)"pid-file", 1, NULL, 'p' },
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
            {
				   _tprintf(_T("NetXMS Server Version ") NETXMS_VERSION_STRING _T(" Build ") NETXMS_VERSION_BUILD_STRING _T(" (") NETXMS_BUILD_TAG _T(")") IS_UNICODE_BUILD_STRING _T("\n"));
               String ciphers = NXCPGetSupportedCiphersAsText();
               _tprintf(_T("NXCP: %d.%d.%d.%d (%s)\n"), 
                  NXCP_VERSION, CLIENT_PROTOCOL_VERSION_BASE, CLIENT_PROTOCOL_VERSION_MOBILE, CLIENT_PROTOCOL_VERSION_FULL,
                  ciphers.isEmpty() ? _T("NO ENCRYPTION") : ciphers.getBuffer());
               _tprintf(_T("Built with: %hs\n"), CPP_COMPILER_VERSION);
            }
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
				g_flags &= ~AF_DAEMON;
				_tprintf(_T("Checking configuration file (%s):\n\n"), g_szConfigFile);
				LoadConfig(&s_debugLevel);
				return FALSE;
			case 'd':
				g_flags |= AF_DAEMON;
				break;
			case 'D':	// Debug level
			   s_debugLevel = strtoul(optarg, &eptr, 0);
				if ((*eptr != 0) || (s_debugLevel > 9))
				{
					_tprintf(_T("Invalid debug level \"%hs\" - should be in range 0..9\n"), optarg);
					s_debugLevel = 0;
				}
				break;
			case 'q': // disable interactive console
				g_flags |= AF_DEBUG_CONSOLE_DISABLED;
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
#ifndef _WIN32
			case 'p':   // PID file
#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, g_szPIDFile, MAX_PATH);
				g_szPIDFile[MAX_PATH - 1] = 0;
#else
				nx_strncpy(g_szPIDFile, optarg, MAX_PATH);
#endif
				break;
#endif
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
         case '@':
#ifdef UNICODE
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, g_szDumpDir, MAX_PATH);
#else
            strlcpy(g_szDumpDir, optarg, MAX_PATH);
#endif
            if (g_szDumpDir[_tcslen(g_szDumpDir) - 1] == _T('\\'))
            {
               g_szDumpDir[_tcslen(g_szDumpDir) - 1] = 0;
            }
            break;
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

/**
 * Startup code
 */
int main(int argc, char* argv[])
{
   InitNetXMSProcess(false);

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
   const TCHAR *configEnv = _tgetenv(_T("NETXMSD_CONFIG"));
   if ((configEnv != NULL) && (*configEnv != 0))
   {
      nx_strncpy(g_szConfigFile, configEnv, MAX_PATH);
   }
#endif

   if (!ParseCommandLine(argc, argv))
      return 1;

   if (!LoadConfig(&s_debugLevel))
   {
      if (IsStandalone())
         _tprintf(_T("Error loading configuration file\n"));
      return 1;
   }

   if (s_debugLevel == NXCONFIG_UNINITIALIZED_VALUE)
      s_debugLevel = 0;
   nxlog_set_debug_level(s_debugLevel);
   free(s_debugTags);

	// Set exception handler
#ifdef _WIN32
	if (g_flags & AF_CATCH_EXCEPTIONS)
		SetExceptionHandler(SEHServiceExceptionHandler, SEHServiceExceptionDataWriter,
                          g_szDumpDir, _T("netxmsd"), MSG_EXCEPTION, (g_flags & AF_WRITE_FULL_DUMP) ? true : false, IsStandalone());
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
         if (g_flags & AF_DB_LOCKED)
         {
            UnlockDB();
            ShutdownDB();
         }
         nxlog_close();
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
   FILE *fp = _tfopen(g_szPIDFile, _T("w"));
   if (fp != NULL)
   {
      _ftprintf(fp, _T("%d"), getpid());
      fclose(fp);
   }

   // Initialize server
   if (!Initialize())
   {
      // Remove database lock
      if (g_flags & AF_DB_LOCKED)
      {
         UnlockDB();
         ShutdownDB();
      }
      nxlog_close();
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
