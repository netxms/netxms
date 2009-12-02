/* 
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2009 Victor Kirhenshtein
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


//
// Global variables
//

DB_HANDLE g_hCoreDB;
BOOL g_bIgnoreErrors = FALSE;
BOOL g_bTrace = FALSE;
bool g_isGuiMode = false;
int g_iSyntax;
const TCHAR *g_pszTableSuffix = _T("");
const TCHAR *g_pszSqlType[5][2] = 
{
   { _T("text"), _T("bigint") },       // MySQL
   { _T("varchar"), _T("bigint") },    // PostgreSQL
   { _T("text"), _T("bigint") },       // Microsoft SQL
   { _T("clob"), _T("number(20)") },   // Oracle
   { _T("varchar"), _T("number(20)") } // SQLite
};


//
// Static data
//

static TCHAR m_szCodePage[MAX_PATH] = ICONV_DEFAULT_CODEPAGE;
static TCHAR s_encryptedDbPassword[MAX_DB_STRING] = _T("");
static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
   { "CodePage", CT_STRING, 0, 0, MAX_PATH, 0, m_szCodePage },
   { "CreateCrashDumps", CT_IGNORE, 0, 0, 0, 0, NULL },
   { "DBDriver", CT_STRING, 0, 0, MAX_PATH, 0, g_szDbDriver },
   { "DBDrvParams", CT_STRING, 0, 0, MAX_PATH, 0, g_szDbDrvParams },
   { "DBEncryptedPassword", CT_STRING, 0, 0, MAX_DB_STRING, 0, s_encryptedDbPassword },
   { "DBLogin", CT_STRING, 0, 0, MAX_DB_LOGIN, 0, g_szDbLogin },
   { "DBName", CT_STRING, 0, 0, MAX_DB_NAME, 0, g_szDbName },
   { "DBPassword", CT_STRING, 0, 0, MAX_DB_PASSWORD, 0, g_szDbPassword },
   { "DBServer", CT_STRING, 0, 0, MAX_PATH, 0, g_szDbServer },
   { "DataDirectory", CT_IGNORE, 0, 0, 0, 0, NULL },
   { "DumpDirectory", CT_IGNORE, 0, 0, 0, 0, NULL },
   { "LogFailedSQLQueries", CT_IGNORE, 0, 0, 0, 0, NULL },
   { "LogFile", CT_IGNORE, 0, 0, 0, 0, NULL },
   { "Module", CT_IGNORE, 0, 0, 0, 0, NULL },
   { "", CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};
static BOOL m_bForce = FALSE;


//
// Show query if trace mode is ON
//

void ShowQuery(const TCHAR *pszQuery)
{
#ifdef _WIN32
      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x0F);
      _tprintf(_T(">>> "));
      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x0A);
      puts(pszQuery);
      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x07);
#else
      _tprintf(_T(">>> %s\n"), pszQuery);
#endif
}


//
// Get Yes or No answer from keyboard
//

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
		printf(" (Y/N) ");

		if (m_bForce)
		{
			printf("Y\n");
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
					printf("Y\n");
					return true;
				}
				if ((ch == 'n') || (ch == 'N'))
				{
					printf("N\n");
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


//
// Execute SQL SELECT query and print error message on screen if query failed
//

DB_RESULT SQLSelect(const TCHAR *pszQuery)
{
   DB_RESULT hResult;
	TCHAR errorText[DBDRV_MAX_ERROR_TEXT];

   if (g_bTrace)
      ShowQuery(pszQuery);

   hResult = DBSelectEx(g_hCoreDB, pszQuery, errorText);
   if (hResult == NULL)
   {
#ifdef _WIN32
      _tprintf(_T("SQL query failed (%s):\n"), errorText);
      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x0E);
      _tprintf(_T("%s\n"), pszQuery);
      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x07);
#else
      _tprintf(_T("SQL query failed (%s):\n%s\n"), errorText, pszQuery);
#endif
   }
   return hResult;
}


//
// Execute SQL SELECT query via DBAsyncSelect and print error message on screen if query failed
//

DB_ASYNC_RESULT SQLAsyncSelect(const TCHAR *pszQuery)
{
   DB_ASYNC_RESULT hResult;
	TCHAR errorText[DBDRV_MAX_ERROR_TEXT];

   if (g_bTrace)
      ShowQuery(pszQuery);

   hResult = DBAsyncSelectEx(g_hCoreDB, pszQuery, errorText);
   if (hResult == NULL)
   {
#ifdef _WIN32
      _tprintf(_T("SQL query failed (%s):\n"), errorText);
      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x0E);
      _tprintf(_T("%s\n"), pszQuery);
      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x07);
#else
      _tprintf(_T("SQL query failed (%s):\n%s\n"), errorText, pszQuery);
#endif
   }
   return hResult;
}


//
// Execute SQL query and print error message on screen if query failed
//

BOOL SQLQuery(const TCHAR *pszQuery)
{
   BOOL bResult;
	TCHAR errorText[DBDRV_MAX_ERROR_TEXT];

	if (*pszQuery == 0)
		return TRUE;

   if (g_bTrace)
      ShowQuery(pszQuery);

   bResult = DBQueryEx(g_hCoreDB, pszQuery, errorText);
   if (!bResult)
   {
#ifdef _WIN32
      _tprintf(_T("SQL query failed (%s):\n"), errorText);
      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x0E);
      _tprintf(_T("%s\n"), pszQuery);
      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x07);
#else
      _tprintf(_T("SQL query failed (%s):\n%s\n"), errorText, pszQuery);
#endif
   }
   return bResult;
}


//
// Execute SQL batch
//

BOOL SQLBatch(const TCHAR *pszBatch)
{
   TCHAR *pszBuffer, *pszQuery, *ptr;
	TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
   BOOL bRet = TRUE;

   pszBuffer = _tcsdup(pszBatch);
   TranslateStr(pszBuffer, _T("$SQL:TEXT"), g_pszSqlType[g_iSyntax][SQL_TYPE_TEXT]);
   TranslateStr(pszBuffer, _T("$SQL:INT64"), g_pszSqlType[g_iSyntax][SQL_TYPE_INT64]);

   pszQuery = pszBuffer;
   while(1)
   {
      ptr = _tcschr(pszQuery, _T('\n'));
      if (ptr != NULL)
         *ptr = 0;
      if (!_tcscmp(pszQuery, _T("<END>")))
         break;

      if (g_bTrace)
         ShowQuery(pszQuery);

      if (!DBQueryEx(g_hCoreDB, pszQuery, errorText))
      {
#ifdef _WIN32
         _tprintf(_T("SQL query failed (%s):\n"), errorText);
         SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x0E);
         _tprintf(_T("%s\n"), pszQuery);
         SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x07);
#else
         _tprintf(_T("SQL query failed (%s):\n%s\n"), errorText, pszQuery);
#endif
         if (!g_bIgnoreErrors)
         {
            bRet = FALSE;
            break;
         }
      }
      ptr++;
      pszQuery = ptr;
   }
   free(pszBuffer);
   return bRet;
}


//
// Read string value from metadata table
//

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


//
// Read string value from configuration table
//

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
      DecodeSQLString(pszBuffer);
      bSuccess = TRUE;
   }

   DBFreeResult(hResult);
   return bSuccess;
}


//
// Read integer value from configuration table
//

int ConfigReadInt(const TCHAR *pszVar, int iDefault)
{
   TCHAR szBuffer[64];

   if (ConfigReadStr(pszVar, szBuffer, 64, _T("")))
      return _tcstol(szBuffer, NULL, 0);
   else
      return iDefault;
}


//
// Read unsigned long value from configuration table
//

DWORD ConfigReadULong(const TCHAR *pszVar, DWORD dwDefault)
{
   TCHAR szBuffer[64];

   if (ConfigReadStr(pszVar, szBuffer, 64, _T("")))
      return _tcstoul(szBuffer, NULL, 0);
   else
      return dwDefault;
}


//
// Check that database has correct schema version and is not locked
//

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
		_tprintf(_T("Your database has format version %d, this tool is compiled for version %d.\n"
					 "You need to upgrade your server before using this database.\n"),
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
         DecodeSQLString(szLockStatus);
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
               DecodeSQLString(szLockInfo);
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


//
// Startup
//

int main(int argc, char *argv[])
{
   BOOL bStart = TRUE, bForce = FALSE;
   int ch;
   TCHAR szConfigFile[MAX_PATH] = DEFAULT_CONFIG_FILE;
#ifdef _WIN32
   HKEY hKey;
   DWORD dwSize;
#else
   char *pszEnv;
#endif

   InitThreadLibrary();

   printf("NetXMS Database Manager Version " NETXMS_VERSION_STRING "\n\n");

   // Check for alternate config file location
#ifdef _WIN32
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\NetXMS\\Server"), 0,
                    KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
   {
      dwSize = MAX_PATH * sizeof(TCHAR);
      RegQueryValueEx(hKey, _T("ConfigFile"), NULL, NULL, (BYTE *)szConfigFile, &dwSize);
      RegCloseKey(hKey);
   }
#else
   pszEnv = getenv("NETXMSD_CONFIG");
   if (pszEnv != NULL)
      nx_strncpy(szConfigFile, pszEnv, MAX_PATH);
#endif

   // Parse command line
   opterr = 1;
   while((ch = getopt(argc, argv, "c:fGhIMtvX")) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            printf("Usage: nxdbmgr [<options>] <command>\n"
                   "Valid commands are:\n"
						 "   batch <file>  : Run SQL batch file\n"
                   "   check         : Check database for errors\n"
                   "   export <file> : Export database to file\n"
                   "   import <file> : Import database from file\n"
                   "   init <file>   : Initialize database\n"
				       "   reindex       : Reindex database\n"
                   "   unlock        : Forced database unlock\n"
                   "   upgrade       : Upgrade database to new version\n"
                   "Valid options are:\n"
                   "   -c <config> : Use alternate configuration file. Default is " DEFAULT_CONFIG_FILE "\n"
                   "   -f          : Force repair - do not ask for confirmation.\n"
#ifdef _WIN32
				       "   -G          : GUI mode.\n"
#endif
                   "   -h          : Display help and exit.\n"
                   "   -I          : MySQL only - specify TYPE=InnoDB for new tables.\n"
                   "   -M          : MySQL only - specify TYPE=MyISAM for new tables.\n"
                   "   -t          : Enable trace mode (show executed SQL queries).\n"
                   "   -v          : Display version and exit.\n"
                   "   -X          : Ignore SQL errors when upgrading (USE WITH CARE!!!)\n"
                   "\n");
            bStart = FALSE;
            break;
         case 'v':   // Print version and exit
            bStart = FALSE;
            break;
         case 'c':
            nx_strncpy(szConfigFile, optarg, MAX_PATH);
            break;
         case 'f':
            m_bForce = TRUE;
            break;
			case 'G':
				g_isGuiMode = true;
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

   // Check parameter correctness
   if (argc - optind == 0)
   {
      _tprintf(_T("Command missing. Type nxdbmgr -h for command line syntax.\n"));
      return 1;
   }
   if (strcmp(argv[optind], "batch") && 
       strcmp(argv[optind], "check") && 
       strcmp(argv[optind], "export") && 
       strcmp(argv[optind], "import") && 
       strcmp(argv[optind], "reindex") &&
       strcmp(argv[optind], "upgrade") &&
       strcmp(argv[optind], "unlock") &&
       strcmp(argv[optind], "init"))
   {
      _tprintf(_T("Invalid command \"%s\". Type nxdbmgr -h for command line syntax.\n"), argv[optind]);
      return 1;
   }
   if ((!strcmp(argv[optind], "init") || !strcmp(argv[optind], "batch") || !strcmp(argv[optind], "export") || !strcmp(argv[optind], "import")) && (argc - optind < 2))
   {
      _tprintf("Required command argument missing\n");
      return 1;
   }

   // Read configuration file
#if !defined(_WIN32) && !defined(_NETWARE)
	if (!_tcscmp(szConfigFile, _T("{search}")))
	{
		if (access(PREFIX "/etc/netxmsd.conf", 4) == 0)
		{
			_tcscpy(szConfigFile, PREFIX "/etc/netxmsd.conf");
		}
		else if (access("/usr/etc/netxmsd.conf", 4) == 0)
		{
			_tcscpy(szConfigFile, "/usr/etc/netxmsd.conf");
		}
		else
		{
			_tcscpy(szConfigFile, "/etc/netxmsd.conf");
		}
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
		DecryptPassword(g_szDbLogin, s_encryptedDbPassword, g_szDbPassword);
	}

#ifndef _WIN32
	SetDefaultCodepage(m_szCodePage);
#endif

   // Connect to database
   if (!DBInit(0, 0, FALSE, NULL))
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

   if (!strcmp(argv[optind], "init"))
   {
      InitDatabase(argv[optind + 1]);
   }
   else
   {
      // Get database syntax
		g_iSyntax = DBGetSyntax(g_hCoreDB);
		if (g_iSyntax == DB_SYNTAX_UNKNOWN)
		{
         _tprintf(_T("Unable to determine database syntax\n"));
         DBDisconnect(g_hCoreDB);
         DBUnloadDriver();
         return 5;
      }

      // Do requested operation
      if (!strcmp(argv[optind], "batch"))
         ExecSQLBatch(argv[optind + 1]);
      else if (!strcmp(argv[optind], "check"))
         CheckDatabase();
      else if (!strcmp(argv[optind], "upgrade"))
         UpgradeDatabase();
      else if (!strcmp(argv[optind], "unlock"))
         UnlockDatabase();
      else if (!strcmp(argv[optind], "reindex"))
         ReindexDatabase();
      else if (!strcmp(argv[optind], "export"))
         ExportDatabase(argv[optind + 1]);
      else if (!strcmp(argv[optind], "import"))
         ImportDatabase(argv[optind + 1]);
   }

   // Shutdown
   DBDisconnect(g_hCoreDB);
   DBUnloadDriver();
   return 0;
}
