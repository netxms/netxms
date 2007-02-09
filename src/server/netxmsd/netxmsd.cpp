/* $Id: netxmsd.cpp,v 1.17 2007-02-09 17:31:57 victor Exp $ */
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
#include <dbghelp.h>


//
// Global data
//

BOOL g_bCheckDB = FALSE;


//
// Help text
//

static char help_text[]="NetXMS Server Version " NETXMS_VERSION_STRING "\n"
                        "Copyright (c) 2003, 2004, 2005 NetXMS Team\n\n"
                        "Usage: netxmsd [<options>] <command>\n\n"
                        "Valid options are:\n"
                        "   --check-db          : Run database check on startup\n"
                        "   --config <file>     : Set non-default configuration file\n"
                        "                       : Default is " DEFAULT_CONFIG_FILE "\n"
                        "   --debug-all         : Turn on all possible debug output\n"
                        "   --debug-actions     : Print debug information for event actions.\n"
                        "   --debug-cscp        : Print client-server communication protocol debug\n"
                        "                       : information to console.\n"
                        "   --debug-dc          : Print data collection debug information to console.\n"
                        "   --debug-discovery   : Print network discovery debug information to console.\n"
                        "   --debug-events      : Print events to console.\n"
                        "   --debug-housekeeper : Print debug information for housekeeping thread.\n"
                        "   --debug-locks       : Print debug information about component locking.\n"
                        "   --debug-objects     : Print object manager debug information.\n"
                        "   --debug-snmp        : Print SNMP debug information.\n"
                        "   --dump-sql          : Dump all SQL queries to log.\n"
#ifdef _WIN32
                        "   --login <user>      : Login name for service account.\n"
                        "   --password <passwd> : Password for service account.\n"
#else
                        "   --pid-file <file>   : Specify pid file.\n"
#endif
                        "\n"
                        "Valid commands are:\n"
                        "   check-config        : Check configuration file syntax\n"
#ifdef _WIN32
                        "   install             : Install Win32 service\n"
                        "   install-events      : Install Win32 event source\n"
#endif
                        "   help                : Display help and exit\n"
#ifdef _WIN32
                        "   remove              : Remove Win32 service\n"
                        "   remove-events       : Remove Win32 event source\n"
#endif
                        "   standalone          : Run in standalone mode (not as service)\n"
#ifdef _WIN32
                        "   start               : Start service\n"
                        "   stop                : Stop service\n"
#endif
                        "   version             : Display version and exit\n"
                        "\n"
                        "NOTE: All debug options will work only in standalone mode.\n\n";


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

static void ExceptionHandler(EXCEPTION_POINTERS *pInfo)
{
	char szBuffer[MAX_PATH];
	HANDLE hFile;
	time_t t;
	MINIDUMP_EXCEPTION_INFORMATION mei;
	OSVERSIONINFO ver;
   SYSTEM_INFO sysInfo;

	t = time(NULL);

	// Create info file
	_snprintf(szBuffer, MAX_PATH, "%s\\netxmsd-%d-%u.info",
	          g_szDumpDir, GetCurrentProcessId(), (DWORD)t);
	m_pExInfoFile = fopen(szBuffer, "w");
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
	_snprintf(szBuffer, MAX_PATH, "%s\\netxmsd-%d-%u.mdmp",
	          g_szDumpDir, GetCurrentProcessId(), (DWORD)t);
   hFile = CreateFile(szBuffer, GENERIC_WRITE, 0, NULL,
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

	if (IsStandalone())
	{
		printf("\n\n*************************************************************\n"
		       "EXCEPTION: %08X (%s) at %08X\nPROCESS TERMINATED",
             pInfo->ExceptionRecord->ExceptionCode,
             SEHExceptionName(pInfo->ExceptionRecord->ExceptionCode),
             pInfo->ExceptionRecord->ExceptionAddress);
	}
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

static BOOL ParseCommandLine(int argc, char *argv[])
{
   int i;
   TCHAR *pszLogin = NULL, *pszPassword = NULL;

   for(i = 1; i < argc; i++)
   {
      if (!strcmp(argv[i], "help"))    // Display help and exit
      {
         printf(help_text);
         return FALSE;
      }
      else if (!strcmp(argv[i], "version"))    // Display version and exit
      {
         printf("NetXMS Server Version " NETXMS_VERSION_STRING " Build of " __DATE__ "\n");
         return FALSE;
      }
      else if (!strcmp(argv[i], "--config"))  // Config file
      {
         i++;
         nx_strncpy(g_szConfigFile, argv[i], MAX_PATH);     // Next word should contain name of the config file
      }
      else if (!strcmp(argv[i], "--check-db"))
      {
         g_bCheckDB = TRUE;
      }
#ifdef _WIN32
      else if (!strcmp(argv[i], "--login"))
      {
         i++;
         pszLogin = argv[i];
      }
      else if (!strcmp(argv[i], "--password"))
      {
         i++;
         pszPassword = argv[i];
      }
#else
      else if (!strcmp(argv[i], "--pid-file"))  // PID file
      {
         i++;
         nx_strncpy(g_szPIDFile, argv[i], MAX_PATH);     // Next word should contain name of the PID file
      }
#endif
      else if (!strcmp(argv[i], "--debug-all"))
      {
         g_dwFlags |= AF_DEBUG_ALL;
      }
      else if (!strcmp(argv[i], "--debug-events"))
      {
         g_dwFlags |= AF_DEBUG_EVENTS;
      }
      else if (!strcmp(argv[i], "--debug-cscp"))
      {
         g_dwFlags |= AF_DEBUG_CSCP;
      }
      else if (!strcmp(argv[i], "--debug-discovery"))
      {
         g_dwFlags |= AF_DEBUG_DISCOVERY;
      }
      else if (!strcmp(argv[i], "--debug-dc"))
      {
         g_dwFlags |= AF_DEBUG_DC;
      }
      else if (!strcmp(argv[i], "--debug-locks"))
      {
         g_dwFlags |= AF_DEBUG_LOCKS;
      }
      else if (!strcmp(argv[i], "--debug-objects"))
      {
         g_dwFlags |= AF_DEBUG_OBJECTS;
      }
      else if (!strcmp(argv[i], "--debug-housekeeper"))
      {
         g_dwFlags |= AF_DEBUG_HOUSEKEEPER;
      }
      else if (!strcmp(argv[i], "--debug-actions"))
      {
         g_dwFlags |= AF_DEBUG_ACTIONS;
      }
      else if (!strcmp(argv[i], "--debug-snmp"))
      {
         g_dwFlags |= AF_DEBUG_SNMP;
      }
      else if (!strcmp(argv[i], "--dump-sql"))
      {
         g_dwFlags |= AF_DEBUG_SQL;
      }
      else if (!strcmp(argv[i], "check-config"))
      {
         g_dwFlags |= AF_STANDALONE;
         printf("Checking configuration file (%s):\n\n", g_szConfigFile);
         LoadConfig();
         return FALSE;
      }
      else if (!strcmp(argv[i], "standalone"))  // Run in standalone mode
      {
         g_dwFlags |= AF_STANDALONE;
         return TRUE;
      }
#ifdef _WIN32
      else if ((!strcmp(argv[i], "install"))||
               (!strcmp(argv[i], "install-events")))
      {
         char exePath[MAX_PATH], dllPath[MAX_PATH], *ptr;

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

         if (!strcmp(argv[i], "install"))
         {
            if ((pszLogin != NULL) && (pszPassword == NULL))
               pszPassword = _T("");
            if ((pszLogin == NULL) && (pszPassword != NULL))
               pszPassword = NULL;
            InstallService(exePath, dllPath, pszLogin, pszPassword);
         }
         else
         {
            InstallEventSource(dllPath);
         }
         return FALSE;
      }
      else if (!strcmp(argv[i], "remove"))
      {
         RemoveService();
         return FALSE;
      }
      else if (!strcmp(argv[i], "remove-events"))
      {
         RemoveEventSource();
         return FALSE;
      }
      else if (!strcmp(argv[i], "start"))
      {
         StartCoreService();
         return FALSE;
      }
      else if (!strcmp(argv[i], "stop"))
      {
         StopCoreService();
         return FALSE;
      }
#endif   /* _WIN32 */
      else
      {
         printf("ERROR: Invalid command line argument\n\n");
         return FALSE;
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
   return 0;
}
