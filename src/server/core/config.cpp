/* 
** Project X - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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
** $module: config.cpp
**
**/

#include "nms_core.h"


//
// Help text
//

static char help_text[]="NMS Version " NETXMS_VERSION_STRING " Server\n"
                        "Copyright (c) 2003 SecurityProjects.org\n\n"
                        "Usage: nms_core [<options>] <command>\n\n"
                        "Valid options are:\n"
                        "   --config <file>     : Set non-default configuration file\n"
                        "                       : Default is " DEFAULT_CONFIG_FILE "\n"
                        "   --debug-all         : Turn on all possible debug output\n"
                        "   --debug-cscp        : Print client-server communication protocol debug\n"
                        "                       : information to console.\n"
                        "   --debug-dc          : Print data collection debug information to console.\n"
                        "   --debug-discovery   : Print network discovery debug information to console.\n"
                        "   --debug-events      : Print events to console.\n"
                        "   --debug-housekeeper : Print debug information for housekeeping thread.\n"
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
// Load and parse configuration file
// Returns TRUE on success and FALSE on failure
//

BOOL LoadConfig(void)
{
   FILE *cfg;
   char *ptr, buffer[4096];
   int sourceLine = 0, errors = 0;

   if (IsStandalone())
      printf("Using configuration file \"%s\"\n", g_szConfigFile);

   cfg = fopen(g_szConfigFile, "r");
   if (cfg == NULL)
   {
      if (IsStandalone())
         printf("Unable to open configuration file: %s\n", strerror(errno));
      return FALSE;
   }

   while(!feof(cfg))
   {
      buffer[0] = 0;
      fgets(buffer, 4095, cfg);
      sourceLine++;
      ptr = strchr(buffer, '\n');
      if (ptr != NULL)
         *ptr = 0;
      ptr = strchr(buffer, '#');
      if (ptr != NULL)
         *ptr = 0;

      StrStrip(buffer);
      if (buffer[0] == 0)
         continue;

      ptr = strchr(buffer, '=');
      if (ptr == NULL)
      {
         errors++;
         if (IsStandalone())
            printf("Syntax error in configuration file, line %d\n", sourceLine);
         continue;
      }
      *ptr = 0;
      ptr++;
      StrStrip(buffer);
      StrStrip(ptr);

      if (!stricmp(buffer,"LogFile"))
      {
#ifdef _WIN32
         if (!stricmp(ptr,"{EventLog}"))
         {
            g_dwFlags |= AF_USE_EVENT_LOG;
         }
         else
#endif  /* _WIN32 */
         {
            g_dwFlags &= ~AF_USE_EVENT_LOG;
            memset(g_szLogFile, 0, MAX_PATH);
            strncpy(g_szLogFile, ptr, MAX_PATH - 1);
         }
      }
      else if (!stricmp(buffer,"DBDriver"))
      {
         memset(g_szDbDriver, 0, MAX_PATH);
         strncpy(g_szDbDriver, ptr, MAX_PATH - 1);
      }
      else if (!stricmp(buffer,"DBDrvParams"))
      {
         memset(g_szDbDrvParams, 0, MAX_PATH);
         strncpy(g_szDbDrvParams, ptr, MAX_PATH - 1);
      }
      else if (!stricmp(buffer,"DBServer"))
      {
         memset(g_szDbServer, 0, MAX_PATH);
         strncpy(g_szDbServer, ptr, MAX_PATH - 1);
      }
      else if (!stricmp(buffer,"DBLogin"))
      {
         memset(g_szDbLogin, 0, MAX_DB_LOGIN);
         strncpy(g_szDbLogin, ptr, MAX_DB_LOGIN - 1);
      }
      else if (!stricmp(buffer,"DBPassword"))
      {
         memset(g_szDbPassword, 0, MAX_DB_PASSWORD);
         strncpy(g_szDbPassword, ptr, MAX_DB_PASSWORD - 1);
      }
      else if (!stricmp(buffer,"DBName"))
      {
         memset(g_szDbName, 0, MAX_DB_NAME);
         strncpy(g_szDbName, ptr, MAX_DB_NAME - 1);
      }
      else if (!stricmp(buffer,"LogFailedSQLQueries"))
      {
         if (!stricmp(ptr, "yes") || !stricmp(ptr, "true") || !stricmp(ptr, "1"))
            g_dwFlags |= AF_LOG_SQL_ERRORS;
         else
            g_dwFlags &= ~AF_LOG_SQL_ERRORS;
      }
      else
      {
         errors++;
         if (g_dwFlags & AF_STANDALONE)
            printf("Error in configuration file, line %d: unknown option \"%s\"\n",sourceLine,buffer);
      }
   }

   if ((IsStandalone()) && (!errors))
      printf("Configuration file OK\n");

   fclose(cfg);
   return TRUE;
}


//
// Parse command line
// Returns TRUE on success and FALSE on failure
//

BOOL ParseCommandLine(int argc, char *argv[])
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
      else if (!strcmp(argv[i], "--debug-housekeeper"))
      {
         g_dwFlags |= AF_DEBUG_HOUSEKEEPER;
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
         char path[MAX_PATH], *ptr;

         ptr = strrchr(argv[0], '\\');
         if (ptr != NULL)
            ptr++;
         else
            ptr = argv[0];

         _fullpath(path, ptr, 255);

         if (stricmp(&path[strlen(path)-4], ".exe"))
            strcat(path, ".exe");

         if (!strcmp(argv[i], "install"))
            InstallService(path);
         else
            InstallEventSource(path);
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
// Read string value from configuration table
//

BOOL ConfigReadStr(char *szVar, char *szBuffer, int iBufSize, char *szDefault)
{
   DB_RESULT hResult;
   char szQuery[256];
   BOOL bSuccess = FALSE;

   strncpy(szBuffer, szDefault, iBufSize - 1);
   szBuffer[iBufSize - 1] = 0;
   if (strlen(szVar) > 127)
      return FALSE;

   sprintf(szQuery, "SELECT value FROM config WHERE name='%s'", szVar);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult == 0)
      return FALSE;

   if (DBGetNumRows(hResult) > 0)
   {
      strncpy(szBuffer, DBGetField(hResult, 0, 0), iBufSize - 1);
      bSuccess = TRUE;
   }

   DBFreeResult(hResult);
   return bSuccess;
}


//
// Read integer value from configuration table
//

int ConfigReadInt(char *szVar, int iDefault)
{
   char szBuffer[64];

   if (ConfigReadStr(szVar, szBuffer, 64, ""))
      return strtol(szBuffer, NULL, 0);
   else
      return iDefault;
}


//
// Read unsigned long value from configuration table
//

DWORD ConfigReadULong(char *szVar, DWORD dwDefault)
{
   char szBuffer[64];

   if (ConfigReadStr(szVar, szBuffer, 64, ""))
      return strtoul(szBuffer, NULL, 0);
   else
      return dwDefault;
}


//
// Write string value to configuration table
//

BOOL ConfigWriteStr(char *szVar, char *szValue, BOOL bCreate)
{
   DB_RESULT hResult;
   char szQuery[1024];
   BOOL bVarExist = FALSE;

   if (strlen(szVar) > 127)
      return FALSE;

   // Check for variable existence
   sprintf(szQuery, "SELECT value FROM config WHERE name='%s'", szVar);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != 0)
   {
      if (DBGetNumRows(hResult) > 0)
         bVarExist = TRUE;
      DBFreeResult(hResult);
   }

   // Don't create non-existing variable if creation flag not set
   if (!bCreate && !bVarExist)
      return FALSE;

   // Create or update variable value
   if (bVarExist)
      sprintf(szQuery, "UPDATE config SET value='%s' WHERE name='%s'", szValue, szVar);
   else
      sprintf(szQuery, "INSERT INTO config (name,value) VALUES ('%s','%s')", szVar, szValue);
   return DBQuery(g_hCoreDB, szQuery);
}


//
// Write integer value to configuration table
//

BOOL ConfigWriteInt(char *szVar, int iValue, BOOL bCreate)
{
   char szBuffer[64];

   sprintf(szBuffer, "%ld", iValue);
   return ConfigWriteStr(szVar, szBuffer, bCreate);
}


//
// Write unsigned long value to configuration table
//

BOOL ConfigWriteULong(char *szVar, DWORD dwValue, BOOL bCreate)
{
   char szBuffer[64];

   sprintf(szBuffer, "%lu", dwValue);
   return ConfigWriteStr(szVar, szBuffer, bCreate);
}
