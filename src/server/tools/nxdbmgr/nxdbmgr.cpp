/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2025 Victor Kirhenshtein
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
#include <netxms_getopt.h>
#include <netxms-version.h>
#include <nxvault.h>

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
bool g_machineReadableOutput = false;
int g_migrationTxnSize = 4096;

/**
 * Static data
 */
static char s_codePage[MAX_PATH] = ICONV_DEFAULT_CODEPAGE;
static wchar_t s_dbDriver[MAX_PATH] = L"";
static wchar_t s_dbDriverOptions[MAX_PATH] = L"";
static wchar_t s_dbServer[MAX_PATH] = L"127.0.0.1";
static wchar_t s_dbLogin[MAX_DB_LOGIN] = L"netxms";
static wchar_t s_dbPassword[MAX_PASSWORD] = L"";
static wchar_t s_dbName[MAX_DB_NAME] = L"netxms_db";
static wchar_t s_dbSchema[MAX_DB_NAME] = L"";
static wchar_t s_dbPasswordCommand[MAX_PATH] = L"";
static wchar_t *s_moduleLoadList = nullptr;

// Vault configuration
static char s_vaultURL[512] = "";
static char s_vaultAppRoleId[256] = "";
static char s_vaultAppRoleSecretId[256] = "";
static char s_vaultDBCredentialPath[512] = "";
static uint32_t s_vaultTimeout = 5000;
static bool s_vaultTLSVerify = true;

static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
   { _T("CodePage"), CT_MB_STRING, 0, 0, MAX_PATH, 0, s_codePage },
   { _T("DBDriver"), CT_STRING, 0, 0, MAX_PATH, 0, s_dbDriver },
   { _T("DBDriverOptions"), CT_STRING, 0, 0, MAX_PATH, 0, s_dbDriverOptions },
   { _T("DBLogin"), CT_STRING, 0, 0, MAX_DB_LOGIN, 0, s_dbLogin },
   { _T("DBName"), CT_STRING, 0, 0, MAX_DB_NAME, 0, s_dbName },
   { _T("DBPassword"), CT_STRING, 0, 0, MAX_PASSWORD, 0, s_dbPassword },
   { _T("DBPasswordCommand"), CT_STRING, 0, 0, MAX_PATH, 0, s_dbPasswordCommand },
   { _T("DBSchema"), CT_STRING, 0, 0, MAX_DB_NAME, 0, s_dbSchema },
   { _T("DBServer"), CT_STRING, 0, 0, MAX_PATH, 0, s_dbServer },
   { _T("Module"), CT_STRING_CONCAT, '\n', 0, 0, 0, &s_moduleLoadList, nullptr },
   /* deprecated parameters */
   { _T("DBDrvParams"), CT_STRING, 0, 0, MAX_PATH, 0, s_dbDriverOptions },
   { _T("DBEncryptedPassword"), CT_STRING, 0, 0, MAX_PASSWORD, 0, s_dbPassword },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr }
};
static DB_DRIVER s_driver = nullptr;

/**
 * Vault configuration template
 */
static NX_CFG_TEMPLATE m_vaultCfgTemplate[] =
{
   { _T("AppRoleId"), CT_MB_STRING, 0, 0, sizeof(s_vaultAppRoleId), 0, s_vaultAppRoleId, nullptr },
   { _T("AppRoleSecretId"), CT_MB_STRING, 0, 0, sizeof(s_vaultAppRoleSecretId), 0, s_vaultAppRoleSecretId, nullptr },
   { _T("DBCredentialPath"), CT_MB_STRING, 0, 0, sizeof(s_vaultDBCredentialPath), 0, s_vaultDBCredentialPath, nullptr },
   { _T("TLSVerify"), CT_BOOLEAN, 0, 0, 0, 0, &s_vaultTLSVerify, nullptr },
   { _T("Timeout"), CT_LONG, 0, 0, 0, 0, &s_vaultTimeout, nullptr },
   { _T("URL"), CT_MB_STRING, 0, 0, sizeof(s_vaultURL), 0, s_vaultURL, nullptr },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr, nullptr }
};

/**
 * Query tracer callback
 */
static void QueryTracerCallback(const wchar_t *query, bool failure, const wchar_t *errorText)
{
   if (failure)
      WriteToTerminalEx(L"SQL query failed (%s):\n\x1b[33;1m%s\x1b[0m\n", errorText, query);
   else if (IsQueryTraceEnabled())
      ShowQuery(query);
}

/**
 * Retrieve database credentials from Vault
 */
static void RetrieveDatabaseCredentialsFromVault(Config *config)
{
   // Parse vault configuration section
   config->parseTemplate(L"VAULT", m_vaultCfgTemplate);

   VaultDatabaseCredentialConfig vaultConfig;
   vaultConfig.url = s_vaultURL;
   vaultConfig.appRoleId = s_vaultAppRoleId;
   vaultConfig.appRoleSecretId = s_vaultAppRoleSecretId;
   vaultConfig.dbCredentialPath = s_vaultDBCredentialPath;
   vaultConfig.timeout = s_vaultTimeout;
   vaultConfig.tlsVerify = s_vaultTLSVerify;

   RetrieveDatabaseCredentialsFromVault(&vaultConfig, s_dbLogin, MAX_DB_LOGIN, s_dbPassword, MAX_PASSWORD, nullptr, true);
}

/**
 * Execute database password command
 */
static void ExecuteDatabasePasswordCommand()
{
   if (s_dbPasswordCommand[0] == 0)
      return;

   _tprintf(_T("Executing database password command: %s\n"), s_dbPasswordCommand);

   OutputCapturingProcessExecutor executor(s_dbPasswordCommand, true);

   if (!executor.execute())
   {
      _tprintf(_T("ERROR: Failed to execute database password command\n"));
      return;
   }

   if (!executor.waitForCompletion(30000))  // 30 second timeout
   {
      _tprintf(_T("ERROR: Database password command timed out\n"));
      return;
   }

   if (executor.getExitCode() == 0)
   {
      const char *output = executor.getOutput();
      if (output != nullptr && *output != 0)
      {
         // Trim trailing whitespace (including newlines)
         size_t len = strlen(output);
         while (len > 0 && (output[len-1] == '\n' || output[len-1] == '\r' ||
                output[len-1] == ' ' || output[len-1] == '\t'))
         {
            len--;
         }

         // Convert to wide char and store as password
         MultiByteToWideCharSysLocale(output, s_dbPassword, len);
         s_dbPassword[len] = 0;

         _tprintf(_T("Database password successfully retrieved from command\n"));
      }
      else
      {
         _tprintf(_T("WARNING: Database password command returned empty output\n"));
      }
   }
   else
   {
      _tprintf(_T("ERROR: Database password command failed with exit code %d\n"), executor.getExitCode());
   }
}

/**
 * Check that database has correct schema version and is not locked
 */
bool ValidateDatabase(bool allowLock)
{
   // Get database format version
   int32_t major, minor;
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

   if (allowLock)
      return true;

   // Check if database is locked
   bool locked = false;
   TCHAR lockStatus[MAX_DB_STRING], lockInfo[MAX_DB_STRING];
   DB_RESULT hResult = DBSelect(g_dbHandle, _T("SELECT var_value FROM config WHERE var_name='DBLockStatus'"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         DBGetField(hResult, 0, 0, lockStatus, MAX_DB_STRING);
         locked = _tcscmp(lockStatus, _T("UNLOCKED")) != 0;
      }
      DBFreeResult(hResult);

      if (locked)
      {
         lockInfo[0] = 0;
         hResult = DBSelect(g_dbHandle, _T("SELECT var_value FROM config WHERE var_name='DBLockInfo'"));
         if (hResult != nullptr)
         {
            if (DBGetNumRows(hResult) > 0)
            {
               DBGetField(hResult, 0, 0, lockInfo, MAX_DB_STRING);
            }
            DBFreeResult(hResult);
         }
      }
   }

   if (locked)
   {
      _tprintf(_T("Database is locked by server %s [%s]\n"), lockStatus, lockInfo);
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
   if (hdb == nullptr)
   {
      _tprintf(_T("Unable to connect to database %s@%s as %s (%s)\n"), s_dbName, s_dbServer, s_dbLogin, errorText);
   }
   return hdb;
}

/**
 * Open database connection
 */
static DB_HANDLE ConnectToDatabaseAsDBA(const TCHAR *login, const TCHAR *password)
{
   TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
   DB_HANDLE hdb = DBConnect(s_driver, s_dbServer, nullptr, login, password, nullptr, errorText);
   if (hdb == nullptr)
   {
      _tprintf(_T("Unable to connect to database server %s as %s (%s)\n"), s_dbServer, login, errorText);
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
   { nullptr, nullptr }
};

/**
 * Database types for PostgreSQL driver
 */
static const DB_TYPE s_dbTypesPostgreSQL[] =
{
   { "pgsql", _T("PostgreSQL") },
   { "tsdb", _T("TimeScaleDB") },
   { nullptr, nullptr }
};

/**
 * Select database type
 */
static const char *SelectDatabaseType(const char *driver)
{
   _tprintf(_T("Selected database driver supports multiple database types.\nPlease select actual database type:\n"));

   const DB_TYPE *types = !strcmp(driver, "odbc") ? s_dbTypesODBC : s_dbTypesPostgreSQL;
   int index = 0;
   for(; types[index].name != nullptr; index++)
   {
      _tprintf(_T("   %d. %s (%hs)\n"), index + 1, types[index].displayName, types[index].name);
   }

   int selection;
   while(true)
   {
      _tprintf(_T("Enter database type [1..%d] or 0 to abort: "), index);
      fflush(stdout);

      TCHAR buffer[256];
      if (_fgetts(buffer, 256, stdin) == nullptr)
         return NULL;

      TCHAR *p = _tcschr(buffer, _T('\n'));
      if (p != nullptr)
         *p = 0;
      Trim(buffer);

      TCHAR *eptr;
      selection = _tcstol(buffer, &eptr, 10);
      if ((*eptr == 0) && (selection >= 0) && (selection <= index))
         break;
   }
   return (selection > 0) ? types[selection - 1].name : nullptr;
}

/**
 * Print configuration variable(s)
 */
static void PrintConfig(const TCHAR *pattern)
{
   StringMap *variables = DBMgrGetConfigurationVariables(pattern);
   if (variables == nullptr)
   {
      _tprintf(_T("Error reading configuration variables from database\n"));
      return;
   }

   if (g_machineReadableOutput)
   {
      auto it = variables->begin();
      while(it.hasNext())
      {
         auto v = it.next();
         _tprintf(_T("%s=%s\n"), v->key, v->value);
      }
   }
   else
   {
      int flen = 0;
      auto it = variables->begin();
      while(it.hasNext())
         flen = std::max(flen, static_cast<int>(_tcslen(it.next()->key)));

      it = variables->begin();
      while(it.hasNext())
      {
         auto v = it.next();
         _tprintf(_T("%*s = %s\n"), -flen, v->key, v->value);
      }
   }

   delete variables;
}

/**
 * Add log table(s) to table list
 */
static void AddLogToList(const char *logname, StringList *tableList)
{
   if (!stricmp(logname, "action"))
   {
      tableList->add(_T("server_action_execution_log"));
   }
   else if (!stricmp(logname, "all"))
   {
      tableList->add(_T("alarms"));
      tableList->add(_T("alarm_events"));
      tableList->add(_T("alarm_notes"));
      tableList->add(_T("alarm_state_changes"));
      tableList->add(_T("asset_change_log"));
      tableList->add(_T("audit_log"));
      tableList->add(_T("certificate_action_log"));
      tableList->add(_T("event_log"));
      tableList->add(_T("maintenance_journal"));
      tableList->add(_T("notification_log"));
      tableList->add(_T("server_action_execution_log"));
      tableList->add(_T("snmp_trap_log"));
      tableList->add(_T("syslog"));
      tableList->add(_T("win_event_log"));
   }
   else if (!stricmp(logname, "alarm"))
   {
      tableList->add(_T("alarms"));
      tableList->add(_T("alarm_events"));
      tableList->add(_T("alarm_notes"));
      tableList->add(_T("alarm_state_changes"));
   }
   else if (!stricmp(logname, "asset"))
   {
      tableList->add(_T("asset_change_log"));
   }
   else if (!stricmp(logname, "audit"))
   {
      tableList->add(_T("audit_log"));
   }
   else if (!stricmp(logname, "certificate"))
   {
      tableList->add(_T("certificate_action_log"));
   }
   else if (!stricmp(logname, "event"))
   {
      tableList->add(_T("event_log"));
   }
   else if (!stricmp(logname, "maintenance"))
   {
      tableList->add(_T("maintenance_journal"));
   }
   else if (!stricmp(logname, "notification"))
   {
      tableList->add(_T("notification_log"));
   }
   else if (!stricmp(logname, "snmptrap"))
   {
      tableList->add(_T("snmp_trap_log"));
   }
   else if (!stricmp(logname, "syslog"))
   {
      tableList->add(_T("syslog"));
   }
   else if (!stricmp(logname, "winevent"))
   {
      tableList->add(_T("win_event_log"));
   }
}

#ifdef _WIN32
#define PAUSE do { if (pauseAfterError) { _tprintf(_T("\n***** PRESS ANY KEY TO CONTINUE *****\n")); _getch(); } } while(0)
#else
#define PAUSE do { if (pauseAfterError) { _tprintf(_T("\n***** PRESS ENTER TO CONTINUE *****\n")); char s[1024]; fgets(s, 1024, stdin); } } while(0)
#endif

/**
 * Startup
 */
int main(int argc, char *argv[])
{
   bool bStart = true, bQuiet = false;
   bool replaceValue = true;
   bool showOutput = false;
   bool pauseAfterError = false;
   bool ignoreDataMigrationErrors = false;
	TCHAR fallbackSyntax[32] = _T("");
	TCHAR *dbaLogin = nullptr, *dbaPassword = nullptr;
	StringList includedTables, excludedTables;
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
   String envConfig = GetEnvironmentVariableEx(_T("NETXMSD_CONFIG"));
   if (!envConfig.isEmpty())
      _tcslcpy(configFile, envConfig, MAX_PATH);
#endif

   // Search for config
   if (configFile[0] == 0)
   {
#ifdef _WIN32
      TCHAR path[MAX_PATH];
      GetNetXMSDirectory(nxDirEtc, path);
      _tcscat(path, _T("\\netxmsd.conf"));
      if (_taccess(path, R_OK) == 0)
      {
		   _tcscpy(configFile, path);
      }
      else
      {
         _tcscpy(configFile, _T("C:\\netxmsd.conf"));
      }
#else
      String homeDir = GetEnvironmentVariableEx(_T("NETXMS_HOME"));
      if (!homeDir.isEmpty())
      {
         TCHAR config[MAX_PATH];
         _sntprintf(config, MAX_PATH, _T("%s/etc/netxmsd.conf"), homeDir.cstr());
		   if (_taccess(config, R_OK) == 0)
		   {
			   _tcscpy(configFile, config);
            goto stop_search;
		   }
      }
		if (_taccess(SYSCONFDIR _T("/netxmsd.conf"), R_OK) == 0)
		{
			_tcscpy(configFile, SYSCONFDIR _T("/netxmsd.conf"));
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
   while((ch = getopt(argc, argv, "c:C:dDe:EfF:GhIL:mMNoPqsStT:vxXY:Z:")) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
			   _tprintf(_T("NetXMS Database Manager Version ") NETXMS_VERSION_STRING _T(" Build ") NETXMS_BUILD_TAG _T("\n\n"));
            _tprintf(_T("Usage: nxdbmgr [<options>] <command> [<options>]\n")
                     _T("Valid commands are:\n")
                     _T("   background-convert   : Convert collected data to TimescaleDB format in background\n")
                     _T("   background-upgrade   : Run pending background upgrade procedures\n")
                     _T("   batch <file>         : Run SQL batch file\n")
                     _T("   check                : Check database for errors\n")
                     _T("   check-data-tables    : Check database for missing data tables\n")
                     _T("   convert              : Convert standard PostgreSQL schema to TimescaleDB schema\n")
                     _T("   export <file>        : Export database to file\n")
                     _T("   get <pattern>        : Get value of server configuration variable(s) matching given pattern\n")
                     _T("   import <file>        : Import database from file\n")
                     _T("   init [<type>]        : Initialize database. If type is not provided it will be deduced from driver name.\n")
                     _T("   migrate <source>     : Migrate database from given source\n")
                     _T("   reset-system-account : Unlock user \"system\" and reset it's password to default\n")
                     _T("   set <name> <value>   : Set value of server configuration variable\n")
                     _T("   unlock               : Forced database unlock\n")
                     _T("   upgrade              : Upgrade database to new version\n")
                     _T("Valid options are:\n")
                     _T("   -c <config> : Use alternate configuration file. Default is %s\n")
                     _T("   -C <dba>    : Create database and user before initialization using provided DBA credentials\n")
                     _T("   -d          : Check collected data (may take very long time).\n")
                     _T("   -D          : Migrate only collected data.\n")
                     _T("   -e <table>  : Exclude specific table from export, import, or migration.\n")
                     _T("   -E          : Fail check if fix required\n")
                     _T("   -f          : Force repair - do not ask for confirmation.\n")
                     _T("   -F <syntax> : Fallback database syntax to use if not set in metadata.\n")
#ifdef _WIN32
				         _T("   -G          : GUI mode.\n")
#endif
                     _T("   -h          : Display help and exit.\n")
                     _T("   -I          : MySQL only - specify TYPE=InnoDB for new tables.\n")
                     _T("   -L <log>    : Migrate only specific log.\n")
                     _T("   -m          : Improved machine readability of output.\n")
                     _T("   -M          : MySQL only - specify TYPE=MyISAM for new tables.\n")
                     _T("   -N          : Do not replace existing configuration value (\"set\" command only).\n")
                     _T("   -o          : Show output from SELECT statements in a batch.\n")
                     _T("   -P          : Pause after error.\n")
                     _T("   -q          : Quiet mode (don't show startup banner).\n")
                     _T("   -s          : Skip collected data during export, import, conversion, or migration.\n")
                     _T("   -S          : Skip collected data during export, import, or migration and do not clear or create data tables.\n")
                     _T("   -t          : Enable trace mode (show executed SQL queries).\n")
                     _T("   -T <recs>   : Transaction size for migration.\n")
                     _T("   -v          : Display version and exit.\n")
                     _T("   -x          : Ignore collected data import/migration errors\n")
                     _T("   -X          : Ignore SQL errors when upgrading (USE WITH CAUTION!!!)\n")
                     _T("   -Y <table>  : Migrate only given table.\n")
                     _T("   -Z <log>    : Exclude specific log from export, import, or migration.\n")
                     _T("Valid database types:\n")
                     _T("   db2 mssql mysql oracle pgsql sqlite tsdb\n")
                     _T("Notes:\n")
                     _T("   * -e, -L, -Y, and -Z options can be specified more than once for different tables\n")
                     _T("   * -L and -Y options automatically exclude all other (not explicitly listed) tables\n")
                     _T("   * DBA credentials should be provided in form login/password\n")
                     _T("   * Configuration variable name pattern can include character %% to match any number of characters\n")
                     _T("   * Valid log names for -L and -Z options:\n")
                     _T("        action, alarm, asset, audit, certificate, event, maintenance, notification, snmptrap, syslog, winevent\n")
                     _T("   * Use -Z all to exclude all logs\n")
                     _T("\n"), configFile);
            bStart = false;
            break;
         case 'v':   // Print version and exit
			   WriteToTerminal(L"NetXMS Database Manager Version " NETXMS_VERSION_STRING L" Build " NETXMS_BUILD_TAG L"\n\n");
            bStart = false;
            break;
         case 'c':
	         MultiByteToWideCharSysLocale(optarg, configFile, MAX_PATH);
				configFile[MAX_PATH - 1] = 0;
            break;
         case 'C':
            dbaLogin = WideStringFromMBStringSysLocale(optarg);
            dbaPassword = _tcschr(dbaLogin, _T('/'));
            if (dbaPassword != nullptr)
            {
               *dbaPassword = 0;
               dbaPassword++;
            }
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
            SetDBMgrFailOnFixMode(true);
            break;
         case 'f':
            SetDBMgrForcedConfirmationMode(true);
            break;
         case 'F':
            MultiByteToWideCharSysLocale(optarg, fallbackSyntax, 32);
				fallbackSyntax[31] = 0;
            break;
			case 'G':
			   SetDBMgrGUIMode(true);
				break;
         case 'I':
            SetTableSuffix(_T(" TYPE=InnoDB"));
            break;
         case 'L':
            AddLogToList(optarg, &includedTables);
            break;
         case 'm':
            g_machineReadableOutput = true;
            break;
         case 'M':
            SetTableSuffix(_T(" TYPE=MyISAM"));
            break;
         case 'N':
            replaceValue = false;
            break;
         case 'o':
            showOutput = true;
            break;
         case 'P':
            pauseAfterError = true;
            break;
         case 'q':
            bQuiet = true;
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
            g_migrationTxnSize = strtol(optarg, nullptr, 0);
            if ((g_migrationTxnSize < 1) || (g_migrationTxnSize > 100000))
            {
               _tprintf(_T("WARNING: invalid transaction size, reset to default"));
               g_migrationTxnSize = 4096;
            }
            break;
         case 'X':
            g_ignoreErrors = true;
            break;
         case 'Y':
            includedTables.addMBString(optarg);
            break;
         case 'Z':
            AddLogToList(optarg, &excludedTables);
            break;
         case '?':
            bStart = false;
            break;
         default:
            break;
      }
   }

   if (!bStart)
      return 1;

	if (!bQuiet)
		WriteToTerminal(L"NetXMS Database Manager Version " NETXMS_VERSION_STRING L" Build " NETXMS_BUILD_TAG L"\n\n");

   // Check parameter correctness
   if (argc - optind == 0)
   {
      WriteToTerminal(L"Command missing. Type nxdbmgr -h for command line syntax.\n");
      PAUSE;
      return 1;
   }
   if (strcmp(argv[optind], "background-convert") &&
       strcmp(argv[optind], "background-upgrade") &&
       strcmp(argv[optind], "batch") &&
       strcmp(argv[optind], "check") &&
       strcmp(argv[optind], "check-data-tables") &&
       strcmp(argv[optind], "convert") &&
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
      PAUSE;
      return 1;
   }
   if (((!strcmp(argv[optind], "batch") || !strcmp(argv[optind], "export") || !strcmp(argv[optind], "import") || !strcmp(argv[optind], "get") || !strcmp(argv[optind], "migrate")) && (argc - optind < 2)) ||
       (!strcmp(argv[optind], "set") && (argc - optind < 3)))
   {
      _tprintf(_T("Required command argument(s) missing\n"));
      PAUSE;
      return 1;
   }

   // Read configuration file
	Config *config = new Config();
	if (!config->loadIniConfig(configFile, _T("server")) || !config->parseTemplate(_T("server"), m_cfgTemplate))
   {
      _tprintf(_T("Error loading configuration file\n"));
      PAUSE;
      return 2;
   }

	// Read and decrypt password
	if (!_tcscmp(s_dbPassword, _T("?")))
   {
	   if (!ReadPassword(L"Database password: ", s_dbPassword, MAX_PASSWORD))
	   {
	      _tprintf(_T("Cannot read password from terminal\n"));
         PAUSE;
         return 3;
	   }
   }
   DecryptPassword(s_dbLogin, s_dbPassword, s_dbPassword, MAX_PASSWORD);

   // Execute DBPasswordCommand if specified
   ExecuteDatabasePasswordCommand();

   // Retrieve database credentials from Vault if configured
   RetrieveDatabaseCredentialsFromVault(config);

	delete config;

#ifndef _WIN32
	SetDefaultCodepage(s_codePage);
#endif

   // Connect to database
   if (!DBInit())
   {
      _tprintf(_T("Unable to initialize database library\n"));
      PAUSE;
      return 3;
   }

	s_driver = DBLoadDriver(s_dbDriver, s_dbDriverOptions, nullptr, nullptr);
	if (s_driver == nullptr)
   {
      _tprintf(_T("Unable to load and initialize database driver \"%s\"\n"), s_dbDriver);
      PAUSE;
      return 3;
   }

   if (!strcmp(argv[optind], "init") && (dbaLogin != nullptr))
   {
      g_dbHandle = ConnectToDatabaseAsDBA(dbaLogin, CHECK_NULL_EX(dbaPassword));
      if (g_dbHandle == nullptr)
      {
         DBUnloadDriver(s_driver);
         PAUSE;
         return 4;
      }
      bool success = CreateDatabase(DBGetDriverName(s_driver), s_dbName, s_dbLogin, s_dbPassword);
      DBDisconnect(g_dbHandle);
      if (!success)
      {
         _tprintf(_T("Unable to create database or user\n"));
         DBUnloadDriver(s_driver);
         PAUSE;
         return 9;
      }
      _tprintf(_T("Database created successfully\n"));
   }

   g_dbHandle = ConnectToDatabase();
   if (g_dbHandle == nullptr)
   {
      DBUnloadDriver(s_driver);
      PAUSE;
      return 4;
   }

   if (!LoadServerModules(s_moduleLoadList, bQuiet))
   {
      DBDisconnect(g_dbHandle);
      DBUnloadDriver(s_driver);
      PAUSE;
      return 6;
   }

   int exitCode = 0;
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
            if (fallbackSyntax[0] == 0)
            {
               if (!_isatty(_fileno(stdin)))
               {
                  _tprintf(_T("Cannot determine database type from driver name\n"));
                  return 7;
               }
               const char *dbType = SelectDatabaseType(driver);
               if (dbType == nullptr)
               {
                  _tprintf(_T("Database initialization aborted\n"));
                  return 8;
               }
               strcpy(driver, dbType);
            }
            else
            {
               wchar_to_ASCII(fallbackSyntax, -1, driver, sizeof(driver));
            }
         }
         else if (!strcmp(driver, "mariadb"))
         {
            strcpy(driver, "mysql");
         }

         StringBuffer initFile = shareDir;
         initFile.append(FS_PATH_SEPARATOR _T("sql") FS_PATH_SEPARATOR _T("dbinit_"));
         initFile.appendMBString(driver);
         initFile.append(_T(".sql"));

         char *initFileMB = MBStringFromWideStringSysLocale(initFile);
         exitCode = InitDatabase(initFileMB);
         MemFree(initFileMB);
      }
      else if (strchr(argv[optind + 1], FS_PATH_SEPARATOR_CHAR_A) == nullptr)
      {
         wchar_t shareDir[MAX_PATH];
         GetNetXMSDirectory(nxDirShare, shareDir);

         StringBuffer initFile = shareDir;
         initFile.append(FS_PATH_SEPARATOR _T("sql") FS_PATH_SEPARATOR _T("dbinit_"));
         initFile.appendMBString(argv[optind + 1]);
         initFile.append(_T(".sql"));

         char *initFileMB = MBStringFromWideStringSysLocale(initFile);
         exitCode = InitDatabase(initFileMB);
         MemFree(initFileMB);
      }
      else
      {
         if (_isatty(_fileno(stdin)))
         {
            WriteToTerminal(_T("\x1b[1mWARNING:\x1b[0m Under normal circumstances you should not specify full path to database initialization script.\n"));
            if (!GetYesNo(_T("Do you really want to continue")))
            {
               _tprintf(_T("Database initialization aborted\n"));
               PAUSE;
               return 8;
            }
         }
         exitCode = InitDatabase(argv[optind + 1]);
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
         PAUSE;
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
      else if (!strcmp(argv[optind], "convert"))
      {
         if (!ConvertDatabase())
         {
            WriteToTerminal(_T("Database conversion \x1b[1;31mFAILED\x1b[0m\n"));
            exitCode = 1;
         }
      }
      else if (!strcmp(argv[optind], "background-convert"))
      {
         if (!ConvertDataTables())
         {
            WriteToTerminal(_T("Background data tables conversion \x1b[1;31mFAILED\x1b[0m\n"));
            exitCode = 1;
         }
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
         ExportDatabase(argv[optind + 1], excludedTables, includedTables);
      }
      else if (!strcmp(argv[optind], "import"))
      {
         ImportDatabase(argv[optind + 1], excludedTables, includedTables, ignoreDataMigrationErrors);
      }
      else if (!strcmp(argv[optind], "migrate"))
		{
         wchar_t *sourceConfig = WideStringFromMBStringSysLocale(argv[optind + 1]);
         wchar_t destConfFields[2048];
			_sntprintf(destConfFields, 2048, _T("\tDriver: %s\n\tDB Name: %s\n\tDB Server: %s\n\tDB Login: %s"), s_dbDriver, s_dbName, s_dbServer, s_dbLogin);
         MigrateDatabase(sourceConfig, destConfFields, excludedTables, includedTables, ignoreDataMigrationErrors);
         MemFree(sourceConfig);
		}
      else if (!strcmp(argv[optind], "get"))
		{
         wchar_t *var = WideStringFromMBStringSysLocale(argv[optind + 1]);
			PrintConfig(var);
			MemFree(var);
		}
      else if (!strcmp(argv[optind], "set"))
		{
         wchar_t *var = WideStringFromMBStringSysLocale(argv[optind + 1]);
         wchar_t *value = WideStringFromMBStringSysLocale(argv[optind + 2]);
			CreateConfigParam(var, value, true, false, replaceValue);
			MemFree(var);
			MemFree(value);
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

   if (exitCode != 0)
      PAUSE;
   return exitCode;
}
