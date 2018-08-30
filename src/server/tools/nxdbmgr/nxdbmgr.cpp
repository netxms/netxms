/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2018 Victor Kirhenshtein
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
** File: nxdbmgr.cpp
**
**/

#include "nxdbmgr.h"
#include <nxconfig.h>

#ifdef _WIN32
#include <conio.h>
#endif

NETXMS_EXECUTABLE_HEADER(nxdbmgr)

/**
 * Global variables
 */
bool g_checkData = false;
bool g_checkDataTablesOnly = false;
bool g_dataOnlyMigration = false;
bool g_skipDataMigration = false;
bool g_skipDataSchemaMigration = false;
int g_migrationTxnSize = 4096;

/**
 * Static data
 */
static char m_szCodePage[MAX_PATH] = ICONV_DEFAULT_CODEPAGE;
static TCHAR s_dbDriver[MAX_PATH] = _T("");
static TCHAR s_dbDrvParams[MAX_PATH] = _T("");
static TCHAR s_dbServer[MAX_PATH] = _T("127.0.0.1");
static TCHAR s_dbLogin[MAX_DB_LOGIN] = _T("netxms");
static TCHAR s_dbPassword[MAX_PASSWORD] = _T("");
static TCHAR s_dbName[MAX_DB_NAME] = _T("netxms_db");
static TCHAR s_dbSchema[MAX_DB_NAME] = _T("");
static TCHAR *s_moduleLoadList = NULL;
static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
   { _T("CodePage"), CT_MB_STRING, 0, 0, MAX_PATH, 0, m_szCodePage },
   { _T("DBDriver"), CT_STRING, 0, 0, MAX_PATH, 0, s_dbDriver },
   { _T("DBDrvParams"), CT_STRING, 0, 0, MAX_PATH, 0, s_dbDrvParams },
   { _T("DBLogin"), CT_STRING, 0, 0, MAX_DB_LOGIN, 0, s_dbLogin },
   { _T("DBName"), CT_STRING, 0, 0, MAX_DB_NAME, 0, s_dbName },
   { _T("DBPassword"), CT_STRING, 0, 0, MAX_PASSWORD, 0, s_dbPassword },
   { _T("DBEncryptedPassword"), CT_STRING, 0, 0, MAX_PASSWORD, 0, s_dbPassword },
   { _T("DBSchema"), CT_STRING, 0, 0, MAX_DB_NAME, 0, s_dbSchema },
   { _T("DBServer"), CT_STRING, 0, 0, MAX_PATH, 0, s_dbServer },
   { _T("Module"), CT_STRING_LIST, '\n', 0, 0, 0, &s_moduleLoadList, NULL },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};
static DB_DRIVER s_driver = NULL;

/**
 * Query tracer callback
 */
static void QueryTracerCallback(const TCHAR *query, bool failure, const TCHAR *errorText)
{
   if (failure)
      WriteToTerminalEx(_T("SQL query failed (%s):\n\x1b[33;1m%s\x1b[0m\n"), errorText, query);
   else if (IsQueryTraceEnabled())
      ShowQuery(query);
}

/**
 * Check that database has correct schema version and is not locked
 */
bool ValidateDatabase()
{
	DB_RESULT hResult;
	LONG nVersion = 0;
	BOOL bLocked = FALSE;
   TCHAR szLockStatus[MAX_DB_STRING], szLockInfo[MAX_DB_STRING];

   // Get database format version
   INT32 major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
   {
      _tprintf(_T("Unable to determine database schema version\n"));
      return false;
   }
   if ((major > DB_SCHEMA_VERSION_MAJOR) || ((major == DB_SCHEMA_VERSION_MAJOR) && (minor > DB_SCHEMA_VERSION_MINOR)))
   {
       _tprintf(_T("Your database has format version %d.%d, this tool is compiled for version %d.%d.\n")
                   _T("You need to upgrade your server before using this database.\n"),
                major, minor, DB_SCHEMA_VERSION_MAJOR, DB_SCHEMA_VERSION_MINOR);
       return false;
   }
   if ((major < DB_SCHEMA_VERSION_MAJOR) || ((major == DB_SCHEMA_VERSION_MAJOR) && (minor < DB_SCHEMA_VERSION_MINOR)))
   {
      _tprintf(_T("Your database has format version %d.%d, this tool is compiled for version %d.%d.\nUse \"upgrade\" command to upgrade your database first.\n"),
               major, minor, DB_SCHEMA_VERSION_MAJOR, DB_SCHEMA_VERSION_MINOR);
		return false;
   }

   // Check if database is locked
   hResult = DBSelect(g_dbHandle, _T("SELECT var_value FROM config WHERE var_name='DBLockStatus'"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         DBGetField(hResult, 0, 0, szLockStatus, MAX_DB_STRING);
         bLocked = _tcscmp(szLockStatus, _T("UNLOCKED"));
      }
      DBFreeResult(hResult);

      if (bLocked)
      {
         hResult = DBSelect(g_dbHandle, _T("SELECT var_value FROM config WHERE var_name='DBLockInfo'"));
         if (hResult != NULL)
         {
            if (DBGetNumRows(hResult) > 0)
            {
               DBGetField(hResult, 0, 0, szLockInfo, MAX_DB_STRING);
            }
            DBFreeResult(hResult);
         }
      }
   }

   if (bLocked)
   {
      _tprintf(_T("Database is locked by server %s [%s]\n"), szLockStatus, szLockInfo);
		return false;
   }

	return true;
}

/**
 * Open database connection
 */
DB_HANDLE ConnectToDatabase()
{
   TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
   DB_HANDLE hdb = DBConnect(s_driver, s_dbServer, s_dbName, s_dbLogin, s_dbPassword, s_dbSchema, errorText);
   if (hdb == NULL)
   {
      _tprintf(_T("Unable to connect to database %s@%s as %s: %s\n"), s_dbName, s_dbServer, s_dbLogin, errorText);
   }
   return hdb;
}

/**
 * Startup
 */
int main(int argc, char *argv[])
{
   BOOL bStart = TRUE, bQuiet = FALSE;
   bool replaceValue = true;
   bool skipAudit = false;
   bool skipAlarms = false;
   bool skipEvent = false;
   bool skipSysLog = false;
   bool skipTrapLog = false;
   int ch;

   InitNetXMSProcess(true);

   TCHAR configFile[MAX_PATH] = _T("");

   // Try to read config location
#ifdef _WIN32
   HKEY hKey;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\NetXMS\\Server"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
   {
      DWORD dwSize = MAX_PATH * sizeof(TCHAR);
      RegQueryValueEx(hKey, _T("ConfigFile"), NULL, NULL, (BYTE *)configFile, &dwSize);
      RegCloseKey(hKey);
   }
#else
   const TCHAR *env = _tgetenv(_T("NETXMSD_CONFIG"));
   if ((env != NULL) && (*env != 0))
      nx_strncpy(configFile, env, MAX_PATH);
#endif

   // Search for config
   if (configFile[0] == 0)
   {
#ifdef _WIN32
      TCHAR path[MAX_PATH];
      GetNetXMSDirectory(nxDirEtc, path);
      _tcscat(path, _T("\\netxmsd.conf"));
      if (_taccess(path, 4) == 0)
      {
		   _tcscpy(configFile, path);
      }
      else
      {
         _tcscpy(configFile, _T("C:\\netxmsd.conf"));
      }
#else
      const TCHAR *homeDir = _tgetenv(_T("NETXMS_HOME"));
      if ((homeDir != NULL) && (*homeDir != 0))
      {
         TCHAR config[MAX_PATH];
         _sntprintf(config, MAX_PATH, _T("%s/etc/netxmsd.conf"), homeDir);
		   if (_taccess(config, 4) == 0)
		   {
			   _tcscpy(configFile, config);
            goto stop_search;
		   }
      }
		if (_taccess(PREFIX _T("/etc/netxmsd.conf"), 4) == 0)
		{
			_tcscpy(configFile, PREFIX _T("/etc/netxmsd.conf"));
		}
		else if (_taccess(_T("/usr/etc/netxmsd.conf"), 4) == 0)
		{
			_tcscpy(configFile, _T("/usr/etc/netxmsd.conf"));
		}
		else
		{
			_tcscpy(configFile, _T("/etc/netxmsd.conf"));
		}
stop_search:
      ;
#endif
   }

   // Parse command line
   opterr = 1;
   while((ch = getopt(argc, argv, "Ac:dDEfGhILMNqRsStT:vXY")) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
			   _tprintf(_T("NetXMS Database Manager Version ") NETXMS_VERSION_STRING _T(" Build ") NETXMS_VERSION_BUILD_STRING _T(" (") NETXMS_BUILD_TAG _T(")") IS_UNICODE_BUILD_STRING _T("\n\n"));
            _tprintf(_T("Usage: nxdbmgr [<options>] <command> [<options>]\n")
                     _T("Valid commands are:\n")
						   _T("   batch <file>         : Run SQL batch file\n")
                     _T("   check                : Check database for errors\n")
                     _T("   check-data-tables    : Check database for missing data tables\n")
                     _T("   export <file>        : Export database to file\n")
                     _T("   get <name>           : Get value of server configuration variable\n")
                     _T("   import <file>        : Import database from file\n")
                     _T("   init <file>          : Initialize database\n")
				         _T("   migrate <source>     : Migrate database from given source\n")
                     _T("   online-upgrade       : Run pending online upgrade procedures\n")
                     _T("   reset-system-account : Unlock user \"system\" and reset it's password to default\n")
                     _T("   set <name> <value>   : Set value of server configuration variable\n")
                     _T("   unlock               : Forced database unlock\n")
                     _T("   upgrade              : Upgrade database to new version\n")
                     _T("Valid options are:\n")
                     _T("   -A          : Skip export of audit log\n")
                     _T("   -c <config> : Use alternate configuration file. Default is %s\n")
                     _T("   -d          : Check collected data (may take very long time).\n")
                     _T("   -D          : Migrate only collected data.\n")
                     _T("   -E          : Skip export of event log\n")
                     _T("   -f          : Force repair - do not ask for confirmation.\n")
#ifdef _WIN32
				         _T("   -G          : GUI mode.\n")
#endif
                     _T("   -h          : Display help and exit.\n")
                     _T("   -I          : MySQL only - specify TYPE=InnoDB for new tables.\n")
                     _T("   -L          : Skip export of alarms.\n")
                     _T("   -M          : MySQL only - specify TYPE=MyISAM for new tables.\n")
                     _T("   -N          : Do not replace existing configuration value (\"set\" command only).\n")
                     _T("   -q          : Quiet mode (don't show startup banner).\n")
                     _T("   -R          : Skip export of SNMP trap log.\n")
                     _T("   -s          : Skip collected data during migration on export.\n")
                     _T("   -S          : Skip collected data during migration on export and do not clear or create data tables.\n")
                     _T("   -t          : Enable trace mode (show executed SQL queries).\n")
                     _T("   -T <recs>   : Transaction size for migration.\n")
                     _T("   -v          : Display version and exit.\n")
                     _T("   -X          : Ignore SQL errors when upgrading (USE WITH CAUTION!!!)\n")
                     _T("   -Y          : Skip export of collected syslog records.\n")
                     _T("\n"), configFile);
            bStart = FALSE;
            break;
         case 'v':   // Print version and exit
			   _tprintf(_T("NetXMS Database Manager Version ") NETXMS_VERSION_STRING _T(" Build ") NETXMS_VERSION_BUILD_STRING _T(" (") NETXMS_BUILD_TAG _T(")") IS_UNICODE_BUILD_STRING _T("\n\n"));
            bStart = FALSE;
            break;
         case 'A':
            skipAudit = true;
            break;
         case 'c':
#ifdef UNICODE
	         MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, configFile, MAX_PATH);
				configFile[MAX_PATH - 1] = 0;
#else
            nx_strncpy(configFile, optarg, MAX_PATH);
#endif
            break;
			case 'd':
				g_checkData = true;
				break;
         case 'D':
            g_dataOnlyMigration = true;
            break;
         case 'E':
            skipEvent = true;
            break;
         case 'f':
            SetDBMgrForcedConfirmationMode(true);
            break;
			case 'G':
			   SetDBMgrGUIMode(true);
				break;
         case 'N':
            replaceValue = false;
            break;
         case 'q':
            bQuiet = TRUE;
            break;
         case 'R':
            skipTrapLog = true;
            break;
         case 's':
            g_skipDataMigration = true;
            break;
         case 'S':
            g_skipDataMigration = true;
            g_skipDataSchemaMigration = true;
            break;
         case 't':
            SetQueryTraceMode(true);
            break;
         case 'T':
            g_migrationTxnSize = strtol(optarg, NULL, 0);
            if ((g_migrationTxnSize < 1) || (g_migrationTxnSize > 100000))
            {
               _tprintf(_T("WARNING: invalid transaction size, reset to default"));
               g_migrationTxnSize = 4096;
            }
            break;
         case 'I':
            SetTableSuffix(_T(" TYPE=InnoDB"));
            break;
         case 'L':
            skipAlarms = true;
            break;
         case 'M':
            SetTableSuffix(_T(" TYPE=MyISAM"));
            break;
         case 'X':
            g_ignoreErrors = true;
            break;
         case 'Y':
            skipSysLog = true;
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

	if (!bQuiet)
		_tprintf(_T("NetXMS Database Manager Version ") NETXMS_VERSION_STRING _T(" Build ") NETXMS_VERSION_BUILD_STRING _T(" (") NETXMS_BUILD_TAG _T(")") IS_UNICODE_BUILD_STRING _T("\n\n"));

   // Check parameter correctness
   if (argc - optind == 0)
   {
      _tprintf(_T("Command missing. Type nxdbmgr -h for command line syntax.\n"));
      return 1;
   }
   if (strcmp(argv[optind], "batch") &&
       strcmp(argv[optind], "check") &&
       strcmp(argv[optind], "check-data-tables") &&
       strcmp(argv[optind], "export") &&
       strcmp(argv[optind], "get") &&
       strcmp(argv[optind], "import") &&
       strcmp(argv[optind], "init") &&
       strcmp(argv[optind], "migrate") &&
       strcmp(argv[optind], "online-upgrade") &&
       strcmp(argv[optind], "reset-system-account") &&
       strcmp(argv[optind], "set") &&
       strcmp(argv[optind], "unlock") &&
       strcmp(argv[optind], "upgrade"))
   {
      _tprintf(_T("Invalid command \"%hs\". Type nxdbmgr -h for command line syntax.\n"), argv[optind]);
      return 1;
   }
   if (((!strcmp(argv[optind], "init") || !strcmp(argv[optind], "batch") || !strcmp(argv[optind], "export") || !strcmp(argv[optind], "import") || !strcmp(argv[optind], "get") || !strcmp(argv[optind], "migrate")) && (argc - optind < 2)) ||
       (!strcmp(argv[optind], "set") && (argc - optind < 3)))
   {
      _tprintf(_T("Required command argument(s) missing\n"));
      return 1;
   }

   // Read configuration file
	Config *config = new Config();
	if (!config->loadIniConfig(configFile, _T("server")) || !config->parseTemplate(_T("server"), m_cfgTemplate))
   {
      _tprintf(_T("Error loading configuration file\n"));
      return 2;
   }
	delete config;

	// Read and decrypt password
	if (!_tcscmp(s_dbPassword, _T("?")))
   {
	   if (!ReadPassword(_T("Database password: "), s_dbPassword, MAX_PASSWORD))
	   {
	      _tprintf(_T("Cannot read password from terminal\n"));
	      return 3;
	   }
   }
   DecryptPassword(s_dbLogin, s_dbPassword, s_dbPassword, MAX_PASSWORD);

#ifndef _WIN32
	SetDefaultCodepage(m_szCodePage);
#endif

   // Connect to database
   if (!DBInit(0, 0))
   {
      _tprintf(_T("Unable to initialize database library\n"));
      return 3;
   }

	s_driver = DBLoadDriver(s_dbDriver, s_dbDrvParams, false, NULL, NULL);
	if (s_driver == NULL)
   {
      _tprintf(_T("Unable to load and initialize database driver \"%s\"\n"), s_dbDriver);
      return 3;
   }

   g_dbHandle = ConnectToDatabase();
   if (g_dbHandle == NULL)
   {
      DBUnloadDriver(s_driver);
      return 4;
   }

   if (!LoadServerModules(s_moduleLoadList))
   {
      DBDisconnect(g_dbHandle);
      DBUnloadDriver(s_driver);
      return 6;
   }

   if (!strcmp(argv[optind], "init"))
   {
      InitDatabase(argv[optind + 1]);
   }
   else
   {
      // Get database syntax
		g_dbSyntax = DBGetSyntax(g_dbHandle);
		if (g_dbSyntax == DB_SYNTAX_UNKNOWN)
		{
         _tprintf(_T("Unable to determine database syntax\n"));
         DBDisconnect(g_dbHandle);
         DBUnloadDriver(s_driver);
         return 5;
      }

		DBSetUtilityQueryTracer(QueryTracerCallback);

      // Do requested operation
      if (!strcmp(argv[optind], "batch"))
      {
         ExecSQLBatch(argv[optind + 1]);
      }
      else if (!strcmp(argv[optind], "check"))
      {
         CheckDatabase();
      }
      else if (!strcmp(argv[optind], "check-data-tables"))
      {
         g_checkDataTablesOnly = true;
         CheckDatabase();
      }
      else if (!strcmp(argv[optind], "upgrade"))
      {
         UpgradeDatabase();
      }
      else if (!strcmp(argv[optind], "online-upgrade"))
      {
         RunPendingOnlineUpgrades();
      }
      else if (!strcmp(argv[optind], "unlock"))
      {
         UnlockDatabase();
      }
      else if (!strcmp(argv[optind], "export"))
      {
         ExportDatabase(argv[optind + 1], skipAudit, skipAlarms, skipEvent, skipSysLog, skipTrapLog);
      }
      else if (!strcmp(argv[optind], "import"))
      {
         ImportDatabase(argv[optind + 1]);
      }
      else if (!strcmp(argv[optind], "migrate"))
		{
#ifdef UNICODE
			WCHAR *sourceConfig = WideStringFromMBString(argv[optind + 1]);
#else
			char *sourceConfig = argv[optind + 1];
#endif
			TCHAR destConfFields[2048];
			_sntprintf(destConfFields, 2048, _T("\tDB Name: %s\n\tDB Server: %s\n\tDB Login: %s"), s_dbName, s_dbServer, s_dbLogin);
         MigrateDatabase(sourceConfig, destConfFields);
#ifdef UNICODE
			free(sourceConfig);
#endif
		}
      else if (!strcmp(argv[optind], "get"))
		{
#ifdef UNICODE
			WCHAR *var = WideStringFromMBString(argv[optind + 1]);
#else
			char *var = argv[optind + 1];
#endif
			TCHAR buffer[MAX_CONFIG_VALUE];
			DBMgrConfigReadStr(var, buffer, MAX_CONFIG_VALUE, _T(""));
			_tprintf(_T("%s\n"), buffer);
#ifdef UNICODE
			free(var);
#endif
		}
      else if (!strcmp(argv[optind], "set"))
		{
#ifdef UNICODE
			WCHAR *var = WideStringFromMBString(argv[optind + 1]);
			WCHAR *value = WideStringFromMBString(argv[optind + 2]);
#else
			char *var = argv[optind + 1];
			char *value = argv[optind + 2];
#endif
			CreateConfigParam(var, value, true, false, replaceValue);
#ifdef UNICODE
			free(var);
			free(value);
#endif
		}
      else if (!strcmp(argv[optind], "reset-system-account"))
      {
         ResetSystemAccount();
      }

      if (IsOnlineUpgradePending())
         WriteToTerminal(_T("\n\x1b[31;1mWARNING:\x1b[0m Online upgrades pending. Please run \x1b[1mnxdbmgr online-upgrade\x1b[0m when possible.\n"));
   }

   // Shutdown
   DBDisconnect(g_dbHandle);
   DBUnloadDriver(s_driver);
   return 0;
}
