/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2019 Victor Kirhenshtein
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
      _tprintf(_T("Unable to connect to database %s@%s as %s (%s)\n"), s_dbName, s_dbServer, s_dbLogin, errorText);
   }
   return hdb;
}

/**
 * Database type descriptor
 */
struct DB_TYPE
{
   const char *name;
   const TCHAR *displayName;
};

/**
 * Database types for ODBC driver
 */
static const DB_TYPE s_dbTypesODBC[] =
{
   { "db2", _T("DB/2") },
   { "mssql", _T("Microsoft SQL") },
   { "mysql", _T("MySQL / MariaDB") },
   { "oracle", _T("Oracle") },
   { "pgsql", _T("PostgreSQL") },
   { "tsdb", _T("TimeScaleDB") },
   { NULL, NULL }
};

/**
 * Database types for PostgreSQL driver
 */
static const DB_TYPE s_dbTypesPostgreSQL[] =
{
   { "pgsql", _T("PostgreSQL") },
   { "tsdb", _T("TimeScaleDB") },
   { NULL, NULL }
};

/**
 * Select database type
 */
static const char *SelectDatabaseType(const char *driver)
{
   _tprintf(_T("Selected database driver supports multiple database types.\nPlease select actual database type:\n"));

   const DB_TYPE *types = !strcmp(driver, "odbc") ? s_dbTypesODBC : s_dbTypesPostgreSQL;
   int index = 0;
   for(; types[index].name != NULL; index++)
   {
      _tprintf(_T("   %d. %s (%hs)\n"), index + 1, types[index].displayName, types[index].name);
   }

   int selection;
   while(true)
   {
      _tprintf(_T("Enter database type [1..%d] or 0 to abort: "), index);
      fflush(stdout);

      TCHAR buffer[256];
      if (_fgetts(buffer, 256, stdin) == NULL)
         return NULL;

      TCHAR *p = _tcschr(buffer, _T('\n'));
      if (p != NULL)
         *p = 0;
      Trim(buffer);

      TCHAR *eptr;
      selection = _tcstol(buffer, &eptr, 10);
      if ((*eptr == 0) && (selection >= 0) && (selection <= index))
         break;
   }
   return (selection > 0) ? types[selection - 1].name : NULL;
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
   bool showOutput = false;
	TCHAR fallbackSyntax[32] = _T("");
	StringList excludedTables;
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
   while((ch = getopt(argc, argv, "Ac:dDe:EfF:GhILMNoqRsStT:vXY")) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
			   _tprintf(_T("NetXMS Database Manager Version ") NETXMS_VERSION_STRING _T(" Build ") NETXMS_BUILD_TAG IS_UNICODE_BUILD_STRING _T("\n\n"));
            _tprintf(_T("Usage: nxdbmgr [<options>] <command> [<options>]\n")
                     _T("Valid commands are:\n")
                     _T("   background-upgrade   : Run pending background upgrade procedures\n")
						   _T("   batch <file>         : Run SQL batch file\n")
                     _T("   check                : Check database for errors\n")
                     _T("   check-data-tables    : Check database for missing data tables\n")
                     _T("   export <file>        : Export database to file\n")
                     _T("   get <name>           : Get value of server configuration variable\n")
                     _T("   import <file>        : Import database from file\n")
                     _T("   init [<file>]        : Initialize database. If schema file is not specified,\n")
                     _T("                          it's loaded from $NETXMS_HOME/share/netxms/sql/dbinit_DBTYPE.sql\n")
				         _T("   migrate <source>     : Migrate database from given source\n")
                     _T("   reset-system-account : Unlock user \"system\" and reset it's password to default\n")
                     _T("   set <name> <value>   : Set value of server configuration variable\n")
                     _T("   unlock               : Forced database unlock\n")
                     _T("   upgrade              : Upgrade database to new version\n")
                     _T("Valid options are:\n")
                     _T("   -A          : Skip export of audit log\n")
                     _T("   -c <config> : Use alternate configuration file. Default is %s\n")
                     _T("   -d          : Check collected data (may take very long time).\n")
                     _T("   -D          : Migrate only collected data.\n")
                     _T("   -e <table>  : Exclude specific table from export, import, or migration.\n")
                     _T("   -E          : Skip export or migration of event log\n")
                     _T("   -f          : Force repair - do not ask for confirmation.\n")
                     _T("   -F <syntax> : Fallback database syntax to use if not set in metadata.\n")
#ifdef _WIN32
				         _T("   -G          : GUI mode.\n")
#endif
                     _T("   -h          : Display help and exit.\n")
                     _T("   -I          : MySQL only - specify TYPE=InnoDB for new tables.\n")
                     _T("   -L          : Skip export or migration of alarm log.\n")
                     _T("   -M          : MySQL only - specify TYPE=MyISAM for new tables.\n")
                     _T("   -N          : Do not replace existing configuration value (\"set\" command only).\n")
                     _T("   -o          : Show output from SELECT statements in a batch.\n")
                     _T("   -q          : Quiet mode (don't show startup banner).\n")
                     _T("   -R          : Skip export or migration of SNMP trap log.\n")
                     _T("   -s          : Skip collected data during migration on export.\n")
                     _T("   -S          : Skip collected data during migration on export and do not clear or create data tables.\n")
                     _T("   -t          : Enable trace mode (show executed SQL queries).\n")
                     _T("   -T <recs>   : Transaction size for migration.\n")
                     _T("   -v          : Display version and exit.\n")
                     _T("   -X          : Ignore SQL errors when upgrading (USE WITH CAUTION!!!)\n")
                     _T("   -Y          : Skip export or migration of collected syslog records.\n")
                     _T("\n"), configFile);
            bStart = FALSE;
            break;
         case 'v':   // Print version and exit
			   _tprintf(_T("NetXMS Database Manager Version ") NETXMS_VERSION_STRING _T(" Build ") NETXMS_BUILD_TAG IS_UNICODE_BUILD_STRING _T("\n\n"));
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
            strlcpy(configFile, optarg, MAX_PATH);
#endif
            break;
			case 'd':
				g_checkData = true;
				break;
         case 'D':
            g_dataOnlyMigration = true;
            break;
         case 'e':
            excludedTables.addMBString(optarg);
            break;
         case 'E':
            skipEvent = true;
            break;
         case 'f':
            SetDBMgrForcedConfirmationMode(true);
            break;
         case 'F':
#ifdef UNICODE
	         MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, fallbackSyntax, 32);
				fallbackSyntax[31] = 0;
#else
            strlcpy(fallbackSyntax, optarg, 32);
#endif
            break;
			case 'G':
			   SetDBMgrGUIMode(true);
				break;
         case 'N':
            replaceValue = false;
            break;
         case 'o':
            showOutput = true;
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
		_tprintf(_T("NetXMS Database Manager Version ") NETXMS_VERSION_STRING _T(" Build ") NETXMS_BUILD_TAG IS_UNICODE_BUILD_STRING _T("\n\n"));

   // Check parameter correctness
   if (argc - optind == 0)
   {
      _tprintf(_T("Command missing. Type nxdbmgr -h for command line syntax.\n"));
      return 1;
   }
   if (strcmp(argv[optind], "background-upgrade") &&
       strcmp(argv[optind], "batch") &&
       strcmp(argv[optind], "check") &&
       strcmp(argv[optind], "check-data-tables") &&
       strcmp(argv[optind], "export") &&
       strcmp(argv[optind], "get") &&
       strcmp(argv[optind], "import") &&
       strcmp(argv[optind], "init") &&
       strcmp(argv[optind], "migrate") &&
       strcmp(argv[optind], "online-upgrade") &&   // synonym for "background-upgrade" for compatibility
       strcmp(argv[optind], "reset-system-account") &&
       strcmp(argv[optind], "set") &&
       strcmp(argv[optind], "unlock") &&
       strcmp(argv[optind], "upgrade"))
   {
      _tprintf(_T("Invalid command \"%hs\". Type nxdbmgr -h for command line syntax.\n"), argv[optind]);
      return 1;
   }
   if (((!strcmp(argv[optind], "batch") || !strcmp(argv[optind], "export") || !strcmp(argv[optind], "import") || !strcmp(argv[optind], "get") || !strcmp(argv[optind], "migrate")) && (argc - optind < 2)) ||
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
   if (!DBInit())
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
      if (argc - optind < 2) 
      {
         TCHAR shareDir[MAX_PATH];
         GetNetXMSDirectory(nxDirShare, shareDir);

         char driver[64];
         strcpy(driver, DBGetDriverName(s_driver));
         strlwr(driver);

         if (!strcmp(driver, "odbc") || !strcmp(driver, "pgsql"))
         {
            if (!_isatty(_fileno(stdin)))
            {
               _tprintf(_T("Cannot determine database type from driver name\n"));
               return 7;
            }
            const char *dbType = SelectDatabaseType(driver);
            if (dbType == NULL)
            {
               _tprintf(_T("Database initialization aborted\n"));
               return 8;
            }
            strcpy(driver, dbType);
         }
         else if (!strcmp(driver, "mariadb"))
         {
            strcpy(driver, "mysql");
         }

         String initFile = shareDir;
         initFile.append(FS_PATH_SEPARATOR _T("sql") FS_PATH_SEPARATOR _T("dbinit_"));
         initFile.appendMBString(driver, strlen(driver), CP_ACP);
         initFile.append(_T(".sql"));

         char *initFileUtf8 = initFile.getUTF8String();
         InitDatabase(initFileUtf8);
         MemFree(initFileUtf8);
      }
      else if (strchr(argv[optind + 1], FS_PATH_SEPARATOR_CHAR_A) == NULL)
      {
         TCHAR shareDir[MAX_PATH];
         GetNetXMSDirectory(nxDirShare, shareDir);

         String initFile = shareDir;
         initFile.append(FS_PATH_SEPARATOR _T("sql") FS_PATH_SEPARATOR _T("dbinit_"));
         initFile.appendMBString(argv[optind + 1], strlen(argv[optind + 1]), CP_ACP);
         initFile.append(_T(".sql"));

         char *initFileUtf8 = initFile.getUTF8String();
         InitDatabase(initFileUtf8);
         MemFree(initFileUtf8);
      }
      else
      {
         InitDatabase(argv[optind + 1]);
      }
   }
   else
   {
      // Get database syntax
      g_dbSyntax = DBGetSyntax(g_dbHandle, fallbackSyntax);
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
         ExecSQLBatch(argv[optind + 1], showOutput);
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
      else if (!strcmp(argv[optind], "background-upgrade") || !strcmp(argv[optind], "online-upgrade"))
      {
         RunPendingOnlineUpgrades();
      }
      else if (!strcmp(argv[optind], "unlock"))
      {
         UnlockDatabase();
      }
      else if (!strcmp(argv[optind], "export"))
      {
         ExportDatabase(argv[optind + 1], skipAudit, skipAlarms, skipEvent, skipSysLog, skipTrapLog, excludedTables);
      }
      else if (!strcmp(argv[optind], "import"))
      {
         ImportDatabase(argv[optind + 1], excludedTables);
      }
      else if (!strcmp(argv[optind], "migrate"))
		{
#ifdef UNICODE
			WCHAR *sourceConfig = WideStringFromMBString(argv[optind + 1]);
#else
			char *sourceConfig = argv[optind + 1];
#endif
			TCHAR destConfFields[2048];
			_sntprintf(destConfFields, 2048, _T("\tDriver: %s\n\tDB Name: %s\n\tDB Server: %s\n\tDB Login: %s"), s_dbDriver, s_dbName, s_dbServer, s_dbLogin);
         MigrateDatabase(sourceConfig, destConfFields, skipAudit, skipAlarms, skipEvent, skipSysLog, skipTrapLog, excludedTables);
#ifdef UNICODE
         MemFree(sourceConfig);
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
			MemFree(var);
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
			MemFree(var);
			MemFree(value);
#endif
		}
      else if (!strcmp(argv[optind], "reset-system-account"))
      {
         ResetSystemAccount();
      }

      if (IsOnlineUpgradePending())
         WriteToTerminal(_T("\n\x1b[31;1mWARNING:\x1b[0m Background upgrades pending. Please run \x1b[1mnxdbmgr background-upgrade\x1b[0m when possible.\n"));
   }

   // Shutdown
   DBDisconnect(g_dbHandle);
   DBUnloadDriver(s_driver);
   return 0;
}
