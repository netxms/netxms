/* 
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2013 Victor Kirhenshtein
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

/**
 * Global variables
 */
DB_HANDLE g_hCoreDB;
BOOL g_bIgnoreErrors = FALSE;
BOOL g_bTrace = FALSE;
bool g_isGuiMode = false;
bool g_checkData = false;
bool g_checkDataTablesOnly = false;
bool g_dataOnlyMigration = false;
bool g_skipDataMigration = false;
int g_dbSyntax;
const TCHAR *g_pszTableSuffix = _T("");
const TCHAR *g_pszSqlType[6][3] = 
{
   { _T("text"), _T("text"), _T("bigint") },                 // MySQL
   { _T("text"), _T("varchar(4000)"), _T("bigint") },        // PostgreSQL
   { _T("text"), _T("varchar(4000)"), _T("bigint") },        // Microsoft SQL
   { _T("clob"), _T("varchar(4000)"), _T("number(20)") },    // Oracle
   { _T("varchar"), _T("varchar(4000)"), _T("number(20)") }, // SQLite
   { _T("long varchar"), _T("varchar(4000)"), _T("bigint") } // DB/2
};

/**
 * Static data
 */
static char m_szCodePage[MAX_PATH] = ICONV_DEFAULT_CODEPAGE;
static TCHAR s_encryptedDbPassword[MAX_DB_STRING] = _T("");
static TCHAR s_dbDriver[MAX_PATH] = _T("");
static TCHAR s_dbDrvParams[MAX_PATH] = _T("");
static TCHAR s_dbServer[MAX_PATH] = _T("127.0.0.1");
static TCHAR s_dbLogin[MAX_DB_LOGIN] = _T("netxms");
static TCHAR s_dbPassword[MAX_DB_PASSWORD] = _T("");
static TCHAR s_dbName[MAX_DB_NAME] = _T("netxms_db");
static TCHAR s_dbSchema[MAX_DB_NAME] = _T("");
static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
   { _T("CodePage"), CT_MB_STRING, 0, 0, MAX_PATH, 0, m_szCodePage },
   { _T("DBDriver"), CT_STRING, 0, 0, MAX_PATH, 0, s_dbDriver },
   { _T("DBDrvParams"), CT_STRING, 0, 0, MAX_PATH, 0, s_dbDrvParams },
   { _T("DBEncryptedPassword"), CT_STRING, 0, 0, MAX_DB_STRING, 0, s_encryptedDbPassword },
   { _T("DBLogin"), CT_STRING, 0, 0, MAX_DB_LOGIN, 0, s_dbLogin },
   { _T("DBName"), CT_STRING, 0, 0, MAX_DB_NAME, 0, s_dbName },
   { _T("DBPassword"), CT_STRING, 0, 0, MAX_DB_PASSWORD, 0, s_dbPassword },
   { _T("DBSchema"), CT_STRING, 0, 0, MAX_DB_NAME, 0, s_dbSchema },
   { _T("DBServer"), CT_STRING, 0, 0, MAX_PATH, 0, s_dbServer },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};
static BOOL m_bForce = FALSE;

/**
 * Show query if trace mode is ON
 */
void ShowQuery(const TCHAR *pszQuery)
{
	WriteToTerminalEx(_T("\x1b[1m>>> \x1b[32;1m%s\x1b[0m\n"), pszQuery);
}

/**
 * Get Yes or No answer from keyboard
 */
bool GetYesNo(const TCHAR *format, ...)
{
	va_list args;

	if (g_isGuiMode)
	{
		if (m_bForce)
			return true;

		TCHAR message[4096];
		va_start(args, format);
		_vsntprintf(message, 4096, format, args);
		va_end(args);

#ifdef _WIN32
		return MessageBox(NULL, message, _T("NetXMS Database Manager"), MB_YESNO | MB_ICONQUESTION) == IDYES;
#else
		return false;
#endif
	}
	else
	{
		va_start(args, format);
		_vtprintf(format, args);
		va_end(args);
		_tprintf(_T(" (Y/N) "));

		if (m_bForce)
		{
			_tprintf(_T("Y\n"));
			return true;
		}
		else
		{
#ifdef _WIN32
			int ch;

			while(1)
			{
				ch = _getch();
				if ((ch == 'y') || (ch == 'Y'))
				{
					_tprintf(_T("Y\n"));
					return true;
				}
				if ((ch == 'n') || (ch == 'N'))
				{
					_tprintf(_T("N\n"));
					return false;
				}
			}
#else
			TCHAR szBuffer[16];

			fflush(stdout);
			_fgetts(szBuffer, 16, stdin);
			StrStrip(szBuffer);
			return ((szBuffer[0] == 'y') || (szBuffer[0] == 'Y'));
#endif
	   }
	}
}

/**
 * Execute SQL SELECT query and print error message on screen if query failed
 */
DB_RESULT SQLSelect(const TCHAR *pszQuery)
{
   DB_RESULT hResult;
	TCHAR errorText[DBDRV_MAX_ERROR_TEXT];

   if (g_bTrace)
      ShowQuery(pszQuery);

   hResult = DBSelectEx(g_hCoreDB, pszQuery, errorText);
   if (hResult == NULL)
      WriteToTerminalEx(_T("SQL query failed (%s):\n\x1b[33;1m%s\x1b[0m\n"), errorText, pszQuery);
   return hResult;
}

/**
 * Execute SQL SELECT query via DBAsyncSelect and print error message on screen if query failed
 */
DB_ASYNC_RESULT SQLAsyncSelect(const TCHAR *pszQuery)
{
   DB_ASYNC_RESULT hResult;
	TCHAR errorText[DBDRV_MAX_ERROR_TEXT];

   if (g_bTrace)
      ShowQuery(pszQuery);

   hResult = DBAsyncSelectEx(g_hCoreDB, pszQuery, errorText);
   if (hResult == NULL)
      WriteToTerminalEx(_T("SQL query failed (%s):\n\x1b[33;1m%s\x1b[0m\n"), errorText, pszQuery);
   return hResult;
}

/**
 * Execute prepared statement and print error message on screen if query failed
 */
BOOL SQLExecute(DB_STATEMENT hStmt)
{
	TCHAR errorText[DBDRV_MAX_ERROR_TEXT];

   if (g_bTrace)
      ShowQuery(DBGetStatementSource(hStmt));

   BOOL result = DBExecuteEx(hStmt, errorText);
   if (!result)
      WriteToTerminalEx(_T("SQL query failed (%s):\n\x1b[33;1m%s\x1b[0m\n"), errorText, DBGetStatementSource(hStmt));
   return result;
}

/**
 * Execute SQL query and print error message on screen if query failed
 */
BOOL SQLQuery(const TCHAR *pszQuery)
{
   BOOL bResult;
	TCHAR errorText[DBDRV_MAX_ERROR_TEXT];

	if (*pszQuery == 0)
		return TRUE;

	String query(pszQuery);

   query.replace(_T("$SQL:TEXT"), g_pszSqlType[g_dbSyntax][SQL_TYPE_TEXT]);
   query.replace(_T("$SQL:TXT4K"), g_pszSqlType[g_dbSyntax][SQL_TYPE_TEXT4K]);
   query.replace(_T("$SQL:INT64"), g_pszSqlType[g_dbSyntax][SQL_TYPE_INT64]);

   if (g_bTrace)
      ShowQuery(query);

   bResult = DBQueryEx(g_hCoreDB, (const TCHAR *)query, errorText);
   if (!bResult)
      WriteToTerminalEx(_T("SQL query failed (%s):\n\x1b[33;1m%s\x1b[0m\n"), errorText, (const TCHAR *)query);
   return bResult;
}

/**
 * Execute SQL batch
 */
BOOL SQLBatch(const TCHAR *pszBatch)
{
   String batch(pszBatch);
   TCHAR *pszBuffer, *pszQuery, *ptr;
	TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
   BOOL bRet = TRUE;
	TCHAR table[128], column[128];

   batch.replace(_T("$SQL:TEXT"), g_pszSqlType[g_dbSyntax][SQL_TYPE_TEXT]);
   batch.replace(_T("$SQL:TXT4K"), g_pszSqlType[g_dbSyntax][SQL_TYPE_TEXT4K]);
   batch.replace(_T("$SQL:INT64"), g_pszSqlType[g_dbSyntax][SQL_TYPE_INT64]);

   pszQuery = pszBuffer = batch.getBuffer();
   while(1)
   {
      ptr = _tcschr(pszQuery, _T('\n'));
      if (ptr != NULL)
         *ptr = 0;
      if (!_tcscmp(pszQuery, _T("<END>")))
         break;

		if (_stscanf(pszQuery, _T("ALTER TABLE %128s DROP COLUMN %128s"), table, column) == 2)
		{
			if (!SQLDropColumn(table, column))
			{
				WriteToTerminalEx(_T("Cannot drop column \x1b[37;1m%s.%s\x1b[0m\n"), table, column);
				if (!g_bIgnoreErrors)
				{
					bRet = FALSE;
					break;
				}
			}
		}
		else
		{
			if (g_bTrace)
				ShowQuery(pszQuery);

			if (!DBQueryEx(g_hCoreDB, pszQuery, errorText))
			{
				WriteToTerminalEx(_T("SQL query failed (%s):\n\x1b[33;1m%s\x1b[0m\n"), errorText, pszQuery);
				if (!g_bIgnoreErrors)
				{
					bRet = FALSE;
					break;
				}
			}
		}

      ptr++;
      pszQuery = ptr;
   }
   return bRet;
}

/**
 * Drop column from the table
 */
BOOL SQLDropColumn(const TCHAR *table, const TCHAR *column)
{
	TCHAR query[1024];
	DB_RESULT hResult;
	BOOL success = FALSE;

	if (g_dbSyntax != DB_SYNTAX_SQLITE)
	{
		_sntprintf(query, 1024, _T("ALTER TABLE %s DROP COLUMN %s"), table, column);
		success = SQLQuery(query);
	}
	else
	{
		_sntprintf(query, 1024, _T("PRAGMA TABLE_INFO('%s')"), table);
		hResult = SQLSelect(query);
		if (hResult != NULL)
		{
			int rows = DBGetNumRows(hResult);
			const int blen = 2048;
			TCHAR buffer[blen];
			// Intermediate buffers for SQLs
			TCHAR columnList[1024], createList[1024]; 
			// TABLE_INFO() columns
			TCHAR tabColName[128], tabColType[64], tabColNull[10], tabColDefault[128];
			columnList[0] = createList[0] = _T('\0');
			for (int i = 0; i < rows; i++)
			{
				DBGetField(hResult, i, 1, tabColName, 128);
				DBGetField(hResult, i, 2, tabColType, 64);
				DBGetField(hResult, i, 3, tabColNull, 10);
				DBGetField(hResult, i, 4, tabColDefault, 128);
				if (_tcsnicmp(tabColName, column, 128))
				{
					_tcscat(columnList, tabColName);
					if (columnList[0] != _T('\0'))
						_tcscat(columnList, _T(","));
					_tcscat(createList, tabColName);
					_tcscat(createList, tabColType);
					if (tabColDefault[0] != _T('\0'))
					{
						_tcscat(createList, _T("DEFAULT "));
						_tcscat(createList, tabColDefault);
					}
					if (tabColNull[0] == _T('1'))
						_tcscat(createList, _T(" NOT NULL"));
					_tcscat(createList, _T(","));
				}
			}
			DBFreeResult(hResult);
			if (rows > 0)
			{
				int cllen = (int)_tcslen(columnList);
				if (cllen > 0 && columnList[cllen - 1] == _T(','))
					columnList[cllen - 1] = _T('\0');
				// TODO: figure out if SQLite transactions will work here
				_sntprintf(buffer, blen, _T("CREATE TABLE %s__backup__ (%s)"), table, columnList);
				CHK_EXEC(SQLQuery(buffer));
				_sntprintf(buffer, blen, _T("INSERT INTO %s__backup__  (%s) SELECT %s FROM %s"), 
					table, columnList, columnList, table);
				CHK_EXEC(SQLQuery(buffer));
				_sntprintf(buffer, blen, _T("DROP TABLE %s"), table);
				CHK_EXEC(SQLQuery(buffer));
				_sntprintf(buffer, blen, _T("ALTER TABLE %s__backup__ RENAME to %s"), table, table);
				CHK_EXEC(SQLQuery(buffer));
				success = TRUE;
			}
		}
	}

	// TODO: preserve indices and constraints??

	return success;
}

/**
 * Read string value from metadata table
 */
BOOL MetaDataReadStr(const TCHAR *pszVar, TCHAR *pszBuffer, int iBufSize, const TCHAR *pszDefault)
{
   DB_RESULT hResult;
   TCHAR szQuery[256];
   BOOL bSuccess = FALSE;

   nx_strncpy(pszBuffer, pszDefault, iBufSize);
   if (_tcslen(pszVar) > 127)
      return FALSE;

   _sntprintf(szQuery, 256, _T("SELECT var_value FROM metadata WHERE var_name='%s'"), pszVar);
   hResult = SQLSelect(szQuery);
   if (hResult == NULL)
      return FALSE;

   if (DBGetNumRows(hResult) > 0)
   {
      DBGetField(hResult, 0, 0, pszBuffer, iBufSize);
      bSuccess = TRUE;
   }

   DBFreeResult(hResult);
   return bSuccess;
}

/**
 * Read integer value from configuration table
 */
int MetaDataReadInt(const TCHAR *pszVar, int iDefault)
{
   TCHAR szBuffer[64];

   if (MetaDataReadStr(pszVar, szBuffer, 64, _T("")))
      return _tcstol(szBuffer, NULL, 0);
   else
      return iDefault;
}

/**
 * Read string value from configuration table
 */
BOOL ConfigReadStr(const TCHAR *pszVar, TCHAR *pszBuffer, int iBufSize, const TCHAR *pszDefault)
{
   DB_RESULT hResult;
   TCHAR szQuery[256];
   BOOL bSuccess = FALSE;

   nx_strncpy(pszBuffer, pszDefault, iBufSize);
   if (_tcslen(pszVar) > 127)
      return FALSE;

   _sntprintf(szQuery, 256, _T("SELECT var_value FROM config WHERE var_name='%s'"), pszVar);
   hResult = SQLSelect(szQuery);
   if (hResult == NULL)
      return FALSE;

   if (DBGetNumRows(hResult) > 0)
   {
      DBGetField(hResult, 0, 0, pszBuffer, iBufSize);
      bSuccess = TRUE;
   }

   DBFreeResult(hResult);
   return bSuccess;
}

/**
 * Read integer value from configuration table
 */
int ConfigReadInt(const TCHAR *pszVar, int iDefault)
{
   TCHAR szBuffer[64];

   if (ConfigReadStr(pszVar, szBuffer, 64, _T("")))
      return _tcstol(szBuffer, NULL, 0);
   else
      return iDefault;
}

/**
 * Read unsigned long value from configuration table
 */
DWORD ConfigReadULong(const TCHAR *pszVar, DWORD dwDefault)
{
   TCHAR szBuffer[64];

   if (ConfigReadStr(pszVar, szBuffer, 64, _T("")))
      return _tcstoul(szBuffer, NULL, 0);
   else
      return dwDefault;
}

/**
 * Check that database has correct schema version and is not locked
 */
BOOL ValidateDatabase()
{
	DB_RESULT hResult;
	LONG nVersion = 0;
	BOOL bLocked = FALSE;
   TCHAR szLockStatus[MAX_DB_STRING], szLockInfo[MAX_DB_STRING];

   // Get database format version
   nVersion = DBGetSchemaVersion(g_hCoreDB);
   if (nVersion < DB_FORMAT_VERSION)
   {
      _tprintf(_T("Your database has format version %d, this tool is compiled for version %d.\nUse \"upgrade\" command to upgrade your database first.\n"),
               nVersion, DB_FORMAT_VERSION);
		return FALSE;
   }
   else if (nVersion > DB_FORMAT_VERSION)
   {
		_tprintf(_T("Your database has format version %d, this tool is compiled for version %d.\n")
		         _T("You need to upgrade your server before using this database.\n"),
				   nVersion, DB_FORMAT_VERSION);
		return FALSE;
   }

   // Check if database is locked
   hResult = DBSelect(g_hCoreDB, _T("SELECT var_value FROM config WHERE var_name='DBLockStatus'"));
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
         hResult = DBSelect(g_hCoreDB, _T("SELECT var_value FROM config WHERE var_name='DBLockInfo'"));
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
		return FALSE;
   }

	return TRUE;
}

/**
 * Startup
 */
int main(int argc, char *argv[])
{
   BOOL bStart = TRUE, bForce = FALSE, bQuiet = FALSE, bReplaceValue = TRUE;
   int ch;
   TCHAR szConfigFile[MAX_PATH] = DEFAULT_CONFIG_FILE;

   InitThreadLibrary();

   // Check for alternate config file location
#ifdef _WIN32
   HKEY hKey;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\NetXMS\\Server"), 0,
                    KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
   {
      DWORD dwSize = MAX_PATH * sizeof(TCHAR);
      RegQueryValueEx(hKey, _T("ConfigFile"), NULL, NULL, (BYTE *)szConfigFile, &dwSize);
      RegCloseKey(hKey);
   }
#else
   const TCHAR *env = _tgetenv(_T("NETXMSD_CONFIG"));
   if ((env != NULL) && (*env != 0))
      nx_strncpy(szConfigFile, env, MAX_PATH);
#endif

   // Parse command line
   opterr = 1;
   while((ch = getopt(argc, argv, "c:dDfGhIMNqstvX")) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
			   _tprintf(_T("NetXMS Database Manager Version ") NETXMS_VERSION_STRING _T("\n\n"));
            _tprintf(_T("Usage: nxdbmgr [<options>] <command>\n")
                     _T("Valid commands are:\n")
						   _T("   batch <file>       : Run SQL batch file\n")
                     _T("   check              : Check database for errors\n")
                     _T("   check-data-tables  : Check database for missing data tables\n")
                     _T("   export <file>      : Export database to file\n")
                     _T("   get <name>         : Get value of server configuration variable\n")
                     _T("   import <file>      : Import database from file\n")
                     _T("   init <file>        : Initialize database\n")
				         _T("   migrate <source>   : Migrate database from given source\n")
                     _T("   resetadmin         : Unlock user \"admin\" and reset password to default (\"netxms\")\n")
                     _T("   set <name> <value> : Set value of server configuration variable\n")
                     _T("   unlock             : Forced database unlock\n")
                     _T("   upgrade            : Upgrade database to new version\n")
                     _T("Valid options are:\n")
                     _T("   -c <config> : Use alternate configuration file. Default is ") DEFAULT_CONFIG_FILE _T("\n")
                     _T("   -d          : Check collected data (may take very long time).\n")
                     _T("   -D          : Migrate only collected data.\n")
                     _T("   -f          : Force repair - do not ask for confirmation.\n")
#ifdef _WIN32
				         _T("   -G          : GUI mode.\n")
#endif
                     _T("   -h          : Display help and exit.\n")
                     _T("   -I          : MySQL only - specify TYPE=InnoDB for new tables.\n")
                     _T("   -M          : MySQL only - specify TYPE=MyISAM for new tables.\n")
                     _T("   -N          : Do not replace existing configuration value (\"set\" command only).\n")
                     _T("   -q          : Quiet mode (don't show startup banner).\n")
                     _T("   -s          : Skip collected data during migration.\n")
                     _T("   -t          : Enable trace mode (show executed SQL queries).\n")
                     _T("   -v          : Display version and exit.\n")
                     _T("   -X          : Ignore SQL errors when upgrading (USE WITH CAUTION!!!)\n")
                     _T("\n"));
            bStart = FALSE;
            break;
         case 'v':   // Print version and exit
			   _tprintf(_T("NetXMS Database Manager Version ") NETXMS_VERSION_STRING _T("\n\n"));
            bStart = FALSE;
            break;
         case 'c':
#ifdef UNICODE
	         MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, szConfigFile, MAX_PATH);
				szConfigFile[MAX_PATH - 1] = 0;
#else
            nx_strncpy(szConfigFile, optarg, MAX_PATH);
#endif
            break;
			case 'd':
				g_checkData = true;
				break;
         case 'D':
            g_dataOnlyMigration = true;
            break;
         case 'f':
            m_bForce = TRUE;
            break;
			case 'G':
				g_isGuiMode = true;
				break;
         case 'N':
            bReplaceValue = FALSE;
            break;
         case 'q':
            bQuiet = TRUE;
            break;
         case 's':
            g_skipDataMigration = true;
            break;
         case 't':
            g_bTrace = TRUE;
            break;
         case 'I':
            g_pszTableSuffix = _T(" TYPE=InnoDB");
            break;
         case 'M':
            g_pszTableSuffix = _T(" TYPE=MyISAM");
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

	if (!bQuiet)
		_tprintf(_T("NetXMS Database Manager Version ") NETXMS_VERSION_STRING _T(" Build ") NETXMS_VERSION_BUILD_STRING _T("\n\n"));

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
       strcmp(argv[optind], "resetadmin") &&
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
#if !defined(_WIN32) && !defined(_NETWARE)
	if (!_tcscmp(szConfigFile, _T("{search}")))
	{
      const TCHAR *homeDir = _tgetenv(_T("NETXMS_HOME"));
      if ((homeDir != NULL) && (*homeDir != 0))
      {
         TCHAR config[MAX_PATH];
         _sntprintf(config, MAX_PATH, _T("%s/etc/netxmsd.conf"), homeDir);
		   if (_taccess(config, 4) == 0)
		   {
			   _tcscpy(szConfigFile, config);
            goto stop_search;
		   }
      }
		if (_taccess(PREFIX _T("/etc/netxmsd.conf"), 4) == 0)
		{
			_tcscpy(szConfigFile, PREFIX _T("/etc/netxmsd.conf"));
		}
		else if (_taccess(_T("/usr/etc/netxmsd.conf"), 4) == 0)
		{
			_tcscpy(szConfigFile, _T("/usr/etc/netxmsd.conf"));
		}
		else
		{
			_tcscpy(szConfigFile, _T("/etc/netxmsd.conf"));
		}
stop_search:
      ;
	}
#endif

	Config *config = new Config();
	if (!config->loadIniConfig(szConfigFile, _T("server")) || !config->parseTemplate(_T("server"), m_cfgTemplate))
   {
      _tprintf(_T("Error loading configuration file\n"));
      return 2;
   }
	delete config;

	// Decrypt password
	if (s_encryptedDbPassword[0] != 0)
	{
		DecryptPassword(s_dbLogin, s_encryptedDbPassword, s_dbPassword);
	}

#ifndef _WIN32
	SetDefaultCodepage(m_szCodePage);
#endif

   // Connect to database
   if (!DBInit(0, 0))
   {
      _tprintf(_T("Unable to initialize database library\n"));
      return 3;
   }

	DB_DRIVER driver = DBLoadDriver(s_dbDriver, s_dbDrvParams, false, NULL, NULL);
	if (driver == NULL)
   {
      _tprintf(_T("Unable to load and initialize database driver \"%s\"\n"), s_dbDriver);
      return 3;
   }

	TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
   g_hCoreDB = DBConnect(driver, s_dbServer, s_dbName, s_dbLogin, s_dbPassword, s_dbSchema, errorText);
   if (g_hCoreDB == NULL)
   {
		_tprintf(_T("Unable to connect to database %s@%s as %s: %s\n"), s_dbName, s_dbServer, s_dbLogin, errorText);
      DBUnloadDriver(driver);
      return 4;
   }

   if (!strcmp(argv[optind], "init"))
   {
      InitDatabase(argv[optind + 1]);
   }
   else
   {
      // Get database syntax
		g_dbSyntax = DBGetSyntax(g_hCoreDB);
		if (g_dbSyntax == DB_SYNTAX_UNKNOWN)
		{
         _tprintf(_T("Unable to determine database syntax\n"));
         DBDisconnect(g_hCoreDB);
         DBUnloadDriver(driver);
         return 5;
      }

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
      else if (!strcmp(argv[optind], "unlock"))
      {
         UnlockDatabase();
      }
      else if (!strcmp(argv[optind], "export"))
      {
         ExportDatabase(argv[optind + 1]);
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
         MigrateDatabase(sourceConfig);
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
			TCHAR buffer[MAX_DB_STRING];
			ConfigReadStr(var, buffer, MAX_DB_STRING, _T(""));
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
			CreateConfigParam(var, value, 1, 0, bReplaceValue);
#ifdef UNICODE
			free(var);
			free(value);
#endif
		}
      else if (!strcmp(argv[optind], "resetadmin"))
      {
         ResetAdmin();
      }
   }

   // Shutdown
   DBDisconnect(g_hCoreDB);
   DBUnloadDriver(driver);
   return 0;
}
