/* 
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004 Victor Kirhenshtein
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
** $module: nxdbmgr.cpp
**
**/

#include "nxdbmgr.h"


//
// Global variables
//

DB_HANDLE g_hCoreDB;
BOOL g_bIgnoreErrors = FALSE;


//
// Static data
//

static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
   { "DBDriver", CT_STRING, 0, 0, MAX_PATH, 0, g_szDbDriver },
   { "DBDrvParams", CT_STRING, 0, 0, MAX_PATH, 0, g_szDbDrvParams },
   { "DBLogin", CT_STRING, 0, 0, MAX_DB_LOGIN, 0, g_szDbLogin },
   { "DBName", CT_STRING, 0, 0, MAX_DB_NAME, 0, g_szDbName },
   { "DBPassword", CT_STRING, 0, 0, MAX_DB_PASSWORD, 0, g_szDbPassword },
   { "DBServer", CT_STRING, 0, 0, MAX_PATH, 0, g_szDbServer },
   { "LogFailedSQLQueries", CT_IGNORE, 0, 0, 0, 0, NULL },
   { "LogFile", CT_IGNORE, 0, 0, 0, 0, NULL },
   { "", CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};


//
// Execute SQL query and print error message on screen if query failed
//

BOOL SQLQuery(TCHAR *pszQuery)
{
   BOOL bResult;

   bResult = DBQuery(g_hCoreDB, pszQuery);
   if (!bResult)
   {
#ifdef _WIN32
      _tprintf(_T("SQL query failed:\n"));
      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x0E);
      _tprintf(_T("%s\n"), pszQuery);
      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x07);
#else
      _tprintf(_T("SQL query failed:\n%s\n"), pszQuery);
#endif
   }
   return bResult;
}


//
// Execute SQL batch
//

BOOL SQLBatch(TCHAR *pszBatch)
{
   TCHAR *pszQuery, *ptr;

   pszQuery = pszBatch;
   while(1)
   {
      ptr = _tcschr(pszQuery, _T('\n'));
      if (ptr != NULL)
         *ptr = 0;
      if (!_tcscmp(pszQuery, _T("<END>")))
         break;
      if (!DBQuery(g_hCoreDB, pszQuery))
      {
#ifdef _WIN32
         _tprintf(_T("SQL query failed:\n"));
         SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x0E);
         _tprintf(_T("%s\n"), pszQuery);
         SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x07);
#else
         _tprintf(_T("SQL query failed:\n%s\n"), pszQuery);
#endif
         if (!g_bIgnoreErrors)
            return FALSE;
      }
      ptr++;
      pszQuery = ptr;
   }
   return TRUE;
}


//
// Startup
//

int main(int argc, char *argv[])
{
   BOOL bStart = TRUE, bForce = FALSE;
   int ch;
   TCHAR szConfigFile[MAX_PATH] = DEFAULT_CONFIG_FILE;

   printf("NetXMS Database Manager Version " NETXMS_VERSION_STRING "\n\n");

   // Parse command line
   opterr = 1;
   while((ch = getopt(argc, argv, "c:fhvX")) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            _tprintf(_T("Usage: nxdbmgr [<options>] <command>\n"
                        "Valid commands are:\n"
                        "   check        : Check database for errors\n"
                        "   upgrade      : Upgrade database to new version\n"
                        "Valid options are:\n"
                        "   -c <config>  : Use alternate configuration file. Default is" DEFAULT_CONFIG_FILE "\n"
                        "   -f           : Force repair - do not ask for confirmation.\n"
                        "   -h           : Display help and exit.\n"
                        "   -v           : Display version and exit.\n"
                        "   -X           : Ignore SQL errors when upgrading (USE WITH CARE!!!)\n"
                        "\n"));
            bStart = FALSE;
            break;
         case 'v':   // Print version and exit
            bStart = FALSE;
            break;
         case 'c':
            _tcsncpy(szConfigFile, optarg, MAX_PATH);
            break;
         case 'f':
            bForce = TRUE;
            break;
         case 'X':
            g_bIgnoreErrors = TRUE;
            break;
         case '?':
            bStart = FALSE;
            break;
         default:
            break;
      }
   }

   if (!bStart)
      return 1;

   // Check parameter correctness
   if (argc - optind == 0)
   {
      _tprintf(_T("Command missing. Type nxdbmgr -h for command line syntax.\n"));
      return 1;
   }
   if (strcmp(argv[optind], "check") && strcmp(argv[optind], "upgrade"))
   {
      _tprintf(_T("Invalid command \"%s\". Type nxdbmgr -h for command line syntax.\n"), argv[optind]);
      return 1;
   }

   // Read configuration file
   if (NxLoadConfig(szConfigFile, _T(""), m_cfgTemplate, TRUE) != NXCFG_ERR_OK)
   {
      _tprintf(_T("Error loading configuration file\n"));
      return 2;
   }

   // Connect to database
   if (!DBInit(FALSE, FALSE))
   {
      _tprintf(_T("Unable to load and initialize database driver \"%s\"\n"), g_szDbDriver);
      return 3;
   }

   g_hCoreDB = DBConnect();
   if (g_hCoreDB == NULL)
   {
      _tprintf(_T("Unable to connect to database %s@%s as %s\n"), g_szDbName, 
               g_szDbServer, g_szDbLogin);
      DBUnloadDriver();
      return 4;
   }

   // Do requested operation
   if (!strcmp(argv[optind], "check"))
      CheckDatabase(bForce);
   else if (!strcmp(argv[optind], "upgrade"))
      UpgradeDatabase();

   // Shutdown
   DBDisconnect(g_hCoreDB);
   DBUnloadDriver();
   return 0;
}
