/* $Id: netxmsd.cpp,v 1.25 2008-01-18 17:00:35 victor Exp $ */
/* 
** NetXMS - Network Management System
** Server startup module
** Copyright (C) 2003, 2004, 2005, 2006, 2007 NetXMS Team
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


//
// Global data
//

BOOL g_bCheckDB = FALSE;


//
// Help text
//

static char help_text[]="NetXMS Server Version " NETXMS_VERSION_STRING "\n"
                        "Copyright (c) 2003, 2004, 2005, 2006, 2007, 2008 NetXMS Team\n\n"
                        "Usage: netxmsd [<options>]\n\n"
                        "Valid options are:\n"
                        "   -e          : Run database check on startup\n"
                        "   -c <file>   : Set non-default configuration file\n"
                        "               : Default is " DEFAULT_CONFIG_FILE "\n"
                        "   -d          : Run as daemon/service\n"
                        "   -D <level>  : Set debug level (valid levels are 0..9)\n"
                        "   -h          : Display help and exit\n"
#ifdef _WIN32
                        "   -I          : Install Windows service\n"
                        "   -L <user>   : Login name for service account.\n"
                        "   -P <passwd> : Password for service account.\n"
#else
                        "   -p <file>   : Specify pid file.\n"
#endif
#ifdef _WIN32
                        "   -R          : Remove Windows service\n"
                        "   -s          : Start Windows service\n"
                        "   -S          : Stop Windows service\n"
#endif
                        "   -v          : Display version and exit\n"
                        "\n";


//
// Windows exception handling
// ****************************************************
//

#ifdef _WIN32


//
// Static data
//

static FILE *m_pExInfoFile = NULL;


//
// Writer for SEHShowCallStack()
//

static void ExceptionDataWriter(char *pszText)
{
	if (m_pExInfoFile != NULL)
		fputs(pszText, m_pExInfoFile);
}


//
// Exception handler
//

static BOOL ExceptionHandler(EXCEPTION_POINTERS *pInfo)
{
	char szBuffer[MAX_PATH], szInfoFile[MAX_PATH], szDumpFile[MAX_PATH];
	HANDLE hFile;
	time_t t;
	MINIDUMP_EXCEPTION_INFORMATION mei;
	OSVERSIONINFO ver;
   SYSTEM_INFO sysInfo;

	t = time(NULL);

	// Create info file
	_snprintf(szInfoFile, MAX_PATH, "%s\\netxmsd-%d-%u.info",
	          g_szDumpDir, GetCurrentProcessId(), (DWORD)t);
	m_pExInfoFile = fopen(szInfoFile, "w");
	if (m_pExInfoFile != NULL)
	{
		fprintf(m_pExInfoFile, "NETXMSD CRASH DUMP\n%s\n", ctime(&t));
		fprintf(m_pExInfoFile, "EXCEPTION: %08X (%s) at %08X\n",
		        pInfo->ExceptionRecord->ExceptionCode,
		        SEHExceptionName(pInfo->ExceptionRecord->ExceptionCode),
		        pInfo->ExceptionRecord->ExceptionAddress);

		// NetXMS and OS version
		ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx(&ver);
		if (ver.dwMajorVersion == 5)
		{
			switch(ver.dwMinorVersion)
			{
				case 0:
					strcpy(szBuffer, "2000");
					break;
				case 1:
					strcpy(szBuffer, "XP");
					break;
				case 2:
					strcpy(szBuffer, "Server 2003");
					break;
				default:
					sprintf(szBuffer, "NT %d.%d", ver.dwMajorVersion, ver.dwMinorVersion);
					break;
			}
		}
		else
		{
			sprintf(szBuffer, "NT %d.%d", ver.dwMajorVersion, ver.dwMinorVersion);
		}
		fprintf(m_pExInfoFile, "\nNetXMS Server Version: " NETXMS_VERSION_STRING "\n"
		                       "OS Version: Windows %s Build %d %s\n",
		        szBuffer, ver.dwBuildNumber, ver.szCSDVersion);

		// Processor architecture
		fprintf(m_pExInfoFile, "Processor architecture: ");
		GetSystemInfo(&sysInfo);
		switch(sysInfo.wProcessorArchitecture)
		{
			case PROCESSOR_ARCHITECTURE_INTEL:
				fprintf(m_pExInfoFile, "Intel x86\n");
				break;
			case PROCESSOR_ARCHITECTURE_MIPS:
				fprintf(m_pExInfoFile, "MIPS\n");
				break;
			case PROCESSOR_ARCHITECTURE_ALPHA:
				fprintf(m_pExInfoFile, "ALPHA\n");
				break;
			case PROCESSOR_ARCHITECTURE_PPC:
				fprintf(m_pExInfoFile, "PowerPC\n");
				break;
			case PROCESSOR_ARCHITECTURE_IA64:
				fprintf(m_pExInfoFile, "Intel IA-64\n");
				break;
			case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
				fprintf(m_pExInfoFile, "Intel x86 on Win64\n");
				break;
			case PROCESSOR_ARCHITECTURE_AMD64:
				fprintf(m_pExInfoFile, "AMD64 (Intel EM64T)\n");
				break;
			default:
				fprintf(m_pExInfoFile, "UNKNOWN\n");
				break;
		}

#ifdef _X86_
		fprintf(m_pExInfoFile, "\nRegister information:\n"
		        "  eax=%08X  ebx=%08X  ecx=%08X  edx=%08X\n"
		        "  esi=%08X  edi=%08X  ebp=%08X  esp=%08X\n"
		        "  cs=%04X  ds=%04X  es=%04X  ss=%04X  fs=%04X  gs=%04X  flags=%08X\n",
		        pInfo->ContextRecord->Eax, pInfo->ContextRecord->Ebx,
		        pInfo->ContextRecord->Ecx, pInfo->ContextRecord->Edx,
		        pInfo->ContextRecord->Esi, pInfo->ContextRecord->Edi,
		        pInfo->ContextRecord->Ebp, pInfo->ContextRecord->Esp,
		        pInfo->ContextRecord->SegCs, pInfo->ContextRecord->SegDs,
		        pInfo->ContextRecord->SegEs, pInfo->ContextRecord->SegSs,
		        pInfo->ContextRecord->SegFs, pInfo->ContextRecord->SegGs,
		        pInfo->ContextRecord->EFlags);
#endif

		fprintf(m_pExInfoFile, "\nCall stack:\n");
		SEHShowCallStack(pInfo->ContextRecord);

		fclose(m_pExInfoFile);
	}

	// Create minidump
	_snprintf(szDumpFile, MAX_PATH, "%s\\netxmsd-%d-%u.mdmp",
	          g_szDumpDir, GetCurrentProcessId(), (DWORD)t);
   hFile = CreateFile(szDumpFile, GENERIC_WRITE, 0, NULL,
                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
   if (hFile != INVALID_HANDLE_VALUE)
   {
		mei.ThreadId = GetCurrentThreadId();
		mei.ExceptionPointers = pInfo;
		mei.ClientPointers = FALSE;
      MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile,
                        MiniDumpNormal, &mei, NULL, NULL);
      CloseHandle(hFile);
   }

	// Write event log
	WriteLog(MSG_EXCEPTION, EVENTLOG_ERROR_TYPE, "xsxss",
            pInfo->ExceptionRecord->ExceptionCode,
            SEHExceptionName(pInfo->ExceptionRecord->ExceptionCode),
            pInfo->ExceptionRecord->ExceptionAddress,
				szInfoFile, szDumpFile);

	if (IsStandalone())
	{
		printf("\n\n*************************************************************\n"
		       "EXCEPTION: %08X (%s) at %08X\nPROCESS TERMINATED",
             pInfo->ExceptionRecord->ExceptionCode,
             SEHExceptionName(pInfo->ExceptionRecord->ExceptionCode),
             pInfo->ExceptionRecord->ExceptionAddress);
	}

	return TRUE;	// Terminate process
}


#endif	/* _WIN32 */


//
// Execute command and wait
//

static BOOL ExecAndWait(TCHAR *pszCommand)
{
   BOOL bSuccess = TRUE;

#ifdef _WIN32
   STARTUPINFO si;
   PROCESS_INFORMATION pi;

   // Fill in process startup info structure
   memset(&si, 0, sizeof(STARTUPINFO));
   si.cb = sizeof(STARTUPINFO);
   si.dwFlags = 0;

   // Create new process
   if (!CreateProcess(NULL, pszCommand, NULL, NULL, FALSE,
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
   TCHAR *eptr;
#ifdef _WIN32
	char login[256] = "", password[256] = "", exePath[MAX_PATH], dllPath[MAX_PATH], *ptr;
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
		{ "install", 0, NULL, 'I' },
		{ "login", 1, NULL, 'L' },
		{ "password", 1, NULL, 'P' },
		{ "remove", 0, NULL, 'R' },
		{ "start", 0, NULL, 's' },
		{ "stop", 0, NULL, 'S' },
#else
		{ "pid-file", 1, NULL, 'p' },
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
	         printf(help_text);
	         return FALSE;
			case 'v':
				printf("NetXMS Server Version " NETXMS_VERSION_STRING " Build of " __DATE__ "\n");
				return FALSE;
			case 'c':
				nx_strncpy(g_szConfigFile, optarg, MAX_PATH);
				break;
			case 'C':	// Check config
				g_dwFlags &= ~AF_DAEMON;
				printf("Checking configuration file (%s):\n\n", g_szConfigFile);
				LoadConfig();
				return FALSE;
			case 'd':
				g_dwFlags |= AF_DAEMON;
				break;
			case 'D':	// Debug level
				g_nDebugLevel = strtol(optarg, &eptr, 0);
				if ((*eptr != 0) || (g_nDebugLevel < 0) || (g_nDebugLevel > 9))
				{
					printf("Invalid debug level \"%s\" - should be in range 0..9\n", optarg);
					g_nDebugLevel = 0;
				}
				break;
			case 'e':
				g_bCheckDB = TRUE;
				break;
#ifdef _WIN32
			case 'L':
				nx_strncpy(login, optarg, 256);
				useLogin = TRUE;
				break;
			case 'P':
				nx_strncpy(password, optarg, 256);
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

				InstallService(exePath, dllPath, useLogin ? login : NULL, useLogin ? password : NULL);
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
   pszEnv = getenv("NETXMSD_CONFIG");
   if (pszEnv != NULL)
      nx_strncpy(g_szConfigFile, pszEnv, MAX_PATH);
#endif

   if (!ParseCommandLine(argc, argv))
      return 1;

   if (!LoadConfig())
   {
      if (IsStandalone())
         printf("Error loading configuration file\n");
      return 1;
   }

	// Set exception handler
#ifdef _WIN32
	if (g_dwFlags & AF_CATCH_EXCEPTIONS)
		SetExceptionHandler(ExceptionHandler, ExceptionDataWriter);
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
      sprintf(pszSep, "nxdbmgr -c \"%s\" -f check", g_szConfigFile);
      if (!ExecAndWait(szCmd))
         if (IsStandalone())
            printf("ERROR: Failed to execute command \"%s\"\n", szCmd);
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
         printf("NetXMS Core initialization failed\n");

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
   fp = fopen(g_szPIDFile, "w");
   if (fp != NULL)
   {
      fprintf(fp, "%d", getpid());
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
