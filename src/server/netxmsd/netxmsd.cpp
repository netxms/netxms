/* 
** NetXMS - Network Management System
** Server startup module
** Copyright (C) 2003, 2004 NetXMS Team
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
** $module: main.cpp
**
**/

#include "netxmsd.h"


//
// Help text
//

static char help_text[]="NetXMS Server Version " NETXMS_VERSION_STRING "\n"
                        "Copyright (c) 2003, 2004 NetXMS Team\n\n"
                        "Usage: netxmsd [<options>] <command>\n\n"
                        "Valid options are:\n"
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
                        "   --debug-snmp        : Print SNMP debug information.\n"
                        "   --dump-sql          : Dump all SQL queries to log.\n"
#ifndef _WIN32
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
                        "   version             : Display version and exit\n"
                        "\n"
                        "NOTE: All debug options will work only in standalone mode.\n\n";


//
// Parse command line
// Returns TRUE on success and FALSE on failure
//

static BOOL ParseCommandLine(int argc, char *argv[])
{
   int i;

   for(i = 1; i < argc; i++)
   {
      if (!strcmp(argv[i], "help"))    // Display help and exit
      {
         printf(help_text);
         return FALSE;
      }
      else if (!strcmp(argv[i], "version"))    // Display version and exit
      {
         printf("NMS Version " NETXMS_VERSION_STRING " Build of " __DATE__ "\n");
         return FALSE;
      }
      else if (!strcmp(argv[i], "--config"))  // Config file
      {
         i++;
         strcpy(g_szConfigFile, argv[i]);     // Next word should contain name of the config file
      }
#ifndef _WIN32
      else if (!strcmp(argv[i], "--pid-file"))  // PID file
      {
         i++;
         strcpy(g_szPIDFile, argv[i]);     // Next word should contain name of the PID file
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
            InstallService(exePath, dllPath);
         else
            InstallEventSource(dllPath);
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
#ifndef _WIN32
   int i;
   FILE *fp;
#endif

   if (!ParseCommandLine(argc, argv))
      return 1;

   if (!LoadConfig())
   {
      if (IsStandalone())
         printf("Error loading configuration file\n");
      return 1;
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
         printf("NMS Core initialization failed\n");

         // Remove database lock
         if (g_dwFlags & AF_DB_LOCKED)
         {
            UnlockDB();
            ShutdownDB();
         }
         return 3;
      }
      Main();
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

   // Setup signal handlers
   for(i = 0; i < 32; i++)
      signal(i, SIG_IGN);
   signal(SIGTERM, OnSignal);
   signal(SIGSEGV, OnSignal);

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
   Main();
#endif   /* _WIN32 */
   return 0;
}
