/*
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
** File: config.cpp
**
**/

#include "nxcore.h"
#include <nxconfig.h>

/**
 * Externals
 */
extern char g_szCodePage[];
extern TCHAR *g_pszModLoadList;

/**
 * database connection parameters
 */
TCHAR g_szDbDriver[MAX_PATH] = _T("");
TCHAR g_szDbDrvParams[MAX_PATH] = _T("");
TCHAR g_szDbServer[MAX_PATH] = _T("127.0.0.1");
TCHAR g_szDbLogin[MAX_DB_LOGIN] = _T("netxms");
TCHAR g_szDbPassword[MAX_DB_PASSWORD] = _T("");
TCHAR g_szDbName[MAX_DB_NAME] = _T("netxms_db");
TCHAR g_szDbSchema[MAX_DB_NAME] = _T("");

/**
 * Config file template
 */
static TCHAR s_encryptedDbPassword[MAX_DB_STRING] = _T("");
static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
   { _T("CodePage"), CT_MB_STRING, 0, 0, 256, 0, g_szCodePage, NULL },
   { _T("CreateCrashDumps"), CT_BOOLEAN, 0, 0, AF_CATCH_EXCEPTIONS, 0, &g_dwFlags, NULL },
   { _T("DailyLogFileSuffix"), CT_STRING, 0, 0, MAX_PATH, 0, g_szDailyLogFileSuffix, NULL },
   { _T("DataDirectory"), CT_STRING, 0, 0, MAX_PATH, 0, g_szDataDir, NULL },
   { _T("DBDriver"), CT_STRING, 0, 0, MAX_PATH, 0, g_szDbDriver, NULL },
   { _T("DBDrvParams"), CT_STRING, 0, 0, MAX_PATH, 0, g_szDbDrvParams, NULL },
   { _T("DBEncryptedPassword"), CT_STRING, 0, 0, MAX_DB_STRING, 0, s_encryptedDbPassword, NULL },
   { _T("DBLogin"), CT_STRING, 0, 0, MAX_DB_LOGIN, 0, g_szDbLogin, NULL },
   { _T("DBName"), CT_STRING, 0, 0, MAX_DB_NAME, 0, g_szDbName, NULL },
   { _T("DBPassword"), CT_STRING, 0, 0, MAX_DB_PASSWORD, 0, g_szDbPassword, NULL },
   { _T("DBSchema"), CT_STRING, 0, 0, MAX_PATH, 0, g_szDbSchema, NULL },
   { _T("DBServer"), CT_STRING, 0, 0, MAX_PATH, 0, g_szDbServer, NULL },
   { _T("DebugLevel"), CT_LONG, 0, 0, 0, 0, &g_debugLevel, &g_debugLevel },
   { _T("DumpDirectory"), CT_STRING, 0, 0, MAX_PATH, 0, g_szDumpDir, NULL },
   { _T("FullCrashDumps"), CT_BOOLEAN, 0, 0, AF_WRITE_FULL_DUMP, 0, &g_dwFlags, NULL },
   { _T("JavaLibraryDirectory"), CT_STRING, 0, 0, MAX_PATH, 0, g_szJavaLibDir, NULL },
   { _T("JavaPath"), CT_STRING, 0, 0, MAX_PATH, 0, g_szJavaPath, NULL },
   { _T("LibraryDirectory"), CT_STRING, 0, 0, MAX_PATH, 0, g_szLibDir, NULL },
   { _T("ListenAddress"), CT_STRING, 0, 0, MAX_PATH, 0, g_szListenAddress, NULL },
   { _T("LogFailedSQLQueries"), CT_BOOLEAN, 0, 0, AF_LOG_SQL_ERRORS, 0, &g_dwFlags, NULL },
   { _T("LogFile"), CT_STRING, 0, 0, MAX_PATH, 0, g_szLogFile, NULL },
   { _T("LogHistorySize"), CT_LONG, 0, 0, 0, 0, &g_dwLogHistorySize, NULL },
   { _T("LogRotationMode"), CT_LONG, 0, 0, 0, 0, &g_dwLogRotationMode, NULL },
   { _T("MaxLogSize"), CT_LONG, 0, 0, 0, 0, &g_dwMaxLogSize, NULL },
   { _T("Module"), CT_STRING_LIST, '\n', 0, 0, 0, &g_pszModLoadList, NULL },
   { _T("ProcessAffinityMask"), CT_LONG, 0, 0, 0, 0, &g_processAffinityMask, NULL },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL, NULL }
};

/**
 * Load and parse configuration file
 * Returns TRUE on success and FALSE on failure
 */
BOOL NXCORE_EXPORTABLE LoadConfig()
{
   BOOL bSuccess = FALSE;
	Config *config;

#if !defined(_WIN32) && !defined(_NETWARE)
	if (!_tcscmp(g_szConfigFile, _T("{search}")))
	{
      const TCHAR *homeDir = _tgetenv(_T("NETXMS_HOME"));
      if ((homeDir != NULL) && (*homeDir != 0))
      {
         TCHAR config[MAX_PATH];
         _sntprintf(config, MAX_PATH, _T("%s/etc/netxmsd.conf"), homeDir);
		   if (_taccess(config, 4) == 0)
		   {
			   _tcscpy(g_szConfigFile, config);
            goto stop_search;
		   }
      }
		if (_taccess(PREFIX _T("/etc/netxmsd.conf"), 4) == 0)
		{
			_tcscpy(g_szConfigFile, PREFIX _T("/etc/netxmsd.conf"));
		}
		else if (_taccess(_T("/usr/etc/netxmsd.conf"), 4) == 0)
		{
			_tcscpy(g_szConfigFile, _T("/usr/etc/netxmsd.conf"));
		}
		else
		{
			_tcscpy(g_szConfigFile, _T("/etc/netxmsd.conf"));
		}
stop_search:
      ;
	}
#endif

   // Read default values from enviroment
   const TCHAR *homeDir = _tgetenv(_T("NETXMS_HOME"));
   if ((homeDir != NULL) && (*homeDir != 0))
   {
#ifdef _WIN32
      _sntprintf(g_szLibDir, MAX_PATH, _T("%s%slib"), homeDir,
         (homeDir[_tcslen(homeDir) - 1] != FS_PATH_SEPARATOR_CHAR) ? FS_PATH_SEPARATOR : _T(""));
#else
      _sntprintf(g_szLibDir, MAX_PATH, _T("%s/lib/netxms"), homeDir);
#endif
   }

   if (IsStandalone())
      _tprintf(_T("Using configuration file \"%s\"\n"), g_szConfigFile);

	config = new Config();
	if (config->loadConfig(g_szConfigFile, _T("server")) && config->parseTemplate(_T("server"), m_cfgTemplate))
   {
      if ((!_tcsicmp(g_szLogFile, _T("{EventLog}"))) ||
          (!_tcsicmp(g_szLogFile, _T("{syslog}"))))
      {
         g_dwFlags |= AF_USE_SYSLOG;
      }
      else
      {
         g_dwFlags &= ~AF_USE_SYSLOG;
      }
      bSuccess = TRUE;
   }
	delete config;

	// Decrypt password
	if (s_encryptedDbPassword[0] != 0)
	{
		DecryptPassword(g_szDbLogin, s_encryptedDbPassword, g_szDbPassword);
	}

   return bSuccess;
}

/**
 * Read string value from metadata table
 */
BOOL NXCORE_EXPORTABLE MetaDataReadStr(const TCHAR *szVar, TCHAR *szBuffer, int iBufSize, const TCHAR *szDefault)
{
   DB_RESULT hResult;
   BOOL bSuccess = FALSE;

   nx_strncpy(szBuffer, szDefault, iBufSize);
   if (_tcslen(szVar) > 127)
      return FALSE;

	DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, _T("SELECT var_value FROM metadata WHERE var_name=?"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, szVar, DB_BIND_STATIC);
		hResult = DBSelectPrepared(hStmt);
		if (hResult != NULL)
		{
			if (DBGetNumRows(hResult) > 0)
			{
				DBGetField(hResult, 0, 0, szBuffer, iBufSize);
				bSuccess = TRUE;
			}
			DBFreeResult(hResult);
		}
		DBFreeStatement(hStmt);
	}
   return bSuccess;
}

/**
 * Callback for configuration variables change
 */
static void OnConfigVariableChange(BOOL isCLOB, const TCHAR *name, const TCHAR *value)
{
	// Restart syslog parser if configuration was changed
	if (isCLOB && !_tcscmp(name, _T("SyslogParser")))
	{
		ReinitializeSyslogParser();
	}
   else if (!_tcsncmp(name, _T("CAS"), 3))
   {
      CASReadSettings();
   }
   else if (!_tcscmp(name, _T("StrictAlarmStatusFlow")))
   {
      NotifyClientSessions(NX_NOTIFY_ALARM_STATUS_FLOW_CHANGED, _tcstol(value, NULL, 0));
   }
}

/**
 * Read string value from configuration table
 */
BOOL NXCORE_EXPORTABLE ConfigReadStr(const TCHAR *szVar, TCHAR *szBuffer, int iBufSize, const TCHAR *szDefault)
{
   DB_RESULT hResult;
   BOOL bSuccess = FALSE;

   nx_strncpy(szBuffer, szDefault, iBufSize);
   if (_tcslen(szVar) > 127)
      return FALSE;

   DB_HANDLE hdb = (g_dwFlags & AF_DB_CONNECTION_POOL_READY) ? DBConnectionPoolAcquireConnection() : g_hCoreDB;
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT var_value FROM config WHERE var_name=?"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, szVar, DB_BIND_STATIC);
		hResult = DBSelectPrepared(hStmt);
	   if (hResult != NULL)
		{
			if (DBGetNumRows(hResult) > 0)
			{
				DBGetField(hResult, 0, 0, szBuffer, iBufSize);
				DbgPrintf(8, _T("ConfigReadStr: name=%s value=\"%s\""), szVar, szBuffer);
				bSuccess = TRUE;
			}
		   DBFreeResult(hResult);
		}
		DBFreeStatement(hStmt);
	}
   if (g_dwFlags & AF_DB_CONNECTION_POOL_READY)
      DBConnectionPoolReleaseConnection(hdb);
   return bSuccess;
}

/**
 * Read multibyte string from configuration table
 */
#ifdef UNICODE

BOOL NXCORE_EXPORTABLE ConfigReadStrA(const WCHAR *szVar, char *szBuffer, int iBufSize, const char *szDefault)
{
	WCHAR *wcBuffer = (WCHAR *)malloc(iBufSize * sizeof(WCHAR));
   BOOL rc = ConfigReadStr(szVar, wcBuffer, iBufSize, _T(""));
	if (rc)
	{
		WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, wcBuffer, -1, szBuffer, iBufSize, NULL, NULL);
	}
	else
	{
		strncpy(szBuffer, szDefault, iBufSize);
	}
	szBuffer[iBufSize - 1] = 0;
	free(wcBuffer);
	return rc;
}

#endif

/**
 * Read integer value from configuration table
 */
int NXCORE_EXPORTABLE ConfigReadInt(const TCHAR *szVar, int iDefault)
{
   TCHAR szBuffer[64];

   if (ConfigReadStr(szVar, szBuffer, 64, _T("")))
      return _tcstol(szBuffer, NULL, 0);
   else
      return iDefault;
}

/**
 * Read unsigned long value from configuration table
 */
UINT32 NXCORE_EXPORTABLE ConfigReadULong(const TCHAR *szVar, UINT32 dwDefault)
{
   TCHAR szBuffer[64];

   if (ConfigReadStr(szVar, szBuffer, 64, _T("")))
      return _tcstoul(szBuffer, NULL, 0);
   else
      return dwDefault;
}

/**
 * Read byte array (in hex form) from configuration table into integer array
 */
BOOL NXCORE_EXPORTABLE ConfigReadByteArray(const TCHAR *pszVar, int *pnArray, int nSize, int nDefault)
{
   TCHAR szBuffer[256];
   char pbBytes[128];
   BOOL bResult;
   int i, nLen;

   if (ConfigReadStr(pszVar, szBuffer, 256, _T("")))
   {
      StrToBin(szBuffer, (BYTE *)pbBytes, 128);
      nLen = (int)_tcslen(szBuffer) / 2;
      for(i = 0; (i < nSize) && (i < nLen); i++)
         pnArray[i] = pbBytes[i];
      for(; i < nSize; i++)
         pnArray[i] = nDefault;
      bResult = TRUE;
   }
   else
   {
      for(i = 0; i < nSize; i++)
         pnArray[i] = nDefault;
      bResult = FALSE;
   }
   return bResult;
}

/**
 * Write string value to configuration table
 */
BOOL NXCORE_EXPORTABLE ConfigWriteStr(const TCHAR *varName, const TCHAR *value, BOOL bCreate, BOOL isVisible, BOOL needRestart)
{
   if (_tcslen(varName) > 63)
      return FALSE;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Check for variable existence
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT var_value FROM config WHERE var_name=?"));
	if (hStmt == NULL)
   {
      DBConnectionPoolReleaseConnection(hdb);
		return FALSE;
   }
	DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, varName, DB_BIND_STATIC);
	DB_RESULT hResult = DBSelectPrepared(hStmt);
   BOOL bVarExist = FALSE;
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         bVarExist = TRUE;
      DBFreeResult(hResult);
   }
	DBFreeStatement(hStmt);

   // Don't create non-existing variable if creation flag not set
   if (!bCreate && !bVarExist)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return FALSE;
   }

   // Create or update variable value
   if (bVarExist)
	{
		hStmt = DBPrepare(hdb, _T("UPDATE config SET var_value=? WHERE var_name=?"));
		if (hStmt == NULL)
			return FALSE;
		DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, value, DB_BIND_STATIC);
		DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, varName, DB_BIND_STATIC);
	}
   else
	{
		hStmt = DBPrepare(hdb, _T("INSERT INTO config (var_name,var_value,is_visible,need_server_restart) VALUES (?,?,?,?)"));
		if (hStmt == NULL)
			return FALSE;
		DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, varName, DB_BIND_STATIC);
		DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, value, DB_BIND_STATIC);
		DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (LONG)(isVisible ? 1 : 0));
		DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (LONG)(needRestart ? 1 : 0));
	}
   BOOL success = DBExecute(hStmt);
	DBFreeStatement(hStmt);
   DBConnectionPoolReleaseConnection(hdb);
	if (success)
		OnConfigVariableChange(FALSE, varName, value);
	return success;
}

/**
 * Write integer value to configuration table
 */
BOOL NXCORE_EXPORTABLE ConfigWriteInt(const TCHAR *szVar, int iValue, BOOL bCreate, BOOL isVisible, BOOL needRestart)
{
   TCHAR szBuffer[64];

   _sntprintf(szBuffer, 64, _T("%d"), iValue);
   return ConfigWriteStr(szVar, szBuffer, bCreate, isVisible, needRestart);
}

/**
 * Write unsigned long value to configuration table
 */
BOOL NXCORE_EXPORTABLE ConfigWriteULong(const TCHAR *szVar, UINT32 dwValue, BOOL bCreate, BOOL isVisible, BOOL needRestart)
{
   TCHAR szBuffer[64];

   _sntprintf(szBuffer, 64, _T("%u"), dwValue);
   return ConfigWriteStr(szVar, szBuffer, bCreate, isVisible, needRestart);
}

/**
 * Write integer array to configuration table
 */
BOOL NXCORE_EXPORTABLE ConfigWriteByteArray(const TCHAR *pszVar, int *pnArray, int nSize, BOOL bCreate, BOOL isVisible, BOOL needRestart)
{
   TCHAR szBuffer[256];
   int i, j;

   for(i = 0, j = 0; (i < nSize) && (i < 127); i++, j += 2)
      _sntprintf(&szBuffer[j], 256 - j, _T("%02X"), (char)((pnArray[i] > 127) ? 127 : ((pnArray[i] < -127) ? -127 : pnArray[i])));
   return ConfigWriteStr(pszVar, szBuffer, bCreate, isVisible, needRestart);
}

/**
 * Read large string (clob) value from configuration table
 */
TCHAR NXCORE_EXPORTABLE *ConfigReadCLOB(const TCHAR *var, const TCHAR *defValue)
{
   DB_RESULT hResult;
   TCHAR query[256], *result = NULL;
   BOOL bSuccess = FALSE;

   if (_tcslen(var) <= 127)
	{
		_sntprintf(query, 256, _T("SELECT var_value FROM config_clob WHERE var_name='%s'"), var);
		hResult = DBSelect(g_hCoreDB, query);
		if (hResult != NULL)
		{
			if (DBGetNumRows(hResult) > 0)
			{
				result = DBGetField(hResult, 0, 0, NULL, 0);
				if (result != NULL)
					DecodeSQLString(result);
			}
			DBFreeResult(hResult);
		}
	}

	// Return default value in case of error
	if ((result == NULL) && (defValue != NULL))
		result = _tcsdup(defValue);

	return result;
}

/**
 * Write large string (clob) value to configuration table
 */
BOOL NXCORE_EXPORTABLE ConfigWriteCLOB(const TCHAR *var, const TCHAR *value, BOOL bCreate)
{
   DB_RESULT hResult;
   TCHAR *escValue, *query;
	size_t len;
   BOOL bVarExist = FALSE, success = FALSE;

   if (_tcslen(var) > 127)
      return FALSE;

   escValue = EncodeSQLString(CHECK_NULL_EX(value));
	len = _tcslen(escValue) + 256;
	query = (TCHAR *)malloc(len * sizeof(TCHAR));

   // Check for variable existence
   _sntprintf(query, len, _T("SELECT var_value FROM config_clob WHERE var_name='%s'"), var);
   hResult = DBSelect(g_hCoreDB, query);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         bVarExist = TRUE;
      DBFreeResult(hResult);
   }

   // Don't create non-existing variable if creation flag not set
   if (bCreate || bVarExist)
	{
		// Create or update variable value
		if (bVarExist)
			_sntprintf(query, len, _T("UPDATE config_clob SET var_value='%s' WHERE var_name='%s'"),
					     escValue, var);
		else
			_sntprintf(query, len, _T("INSERT INTO config_clob (var_name,var_value) VALUES ('%s','%s')"),
						  var, escValue);
		success = DBQuery(g_hCoreDB, query);
	}

	free(query);
	free(escValue);

	if (success)
		OnConfigVariableChange(TRUE, var, value);
	return success;
}
