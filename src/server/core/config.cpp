/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 Victor Kirhenshtein
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


//
// Externals
//

extern TCHAR g_szCodePage[];
extern TCHAR *g_pszModLoadList;


//
// database connection parameters
//

TCHAR g_szDbDriver[MAX_PATH] = _T("");
TCHAR g_szDbDrvParams[MAX_PATH] = _T("");
TCHAR g_szDbServer[MAX_PATH] = _T("127.0.0.1");
TCHAR g_szDbLogin[MAX_DB_LOGIN] = _T("netxms");
TCHAR g_szDbPassword[MAX_DB_PASSWORD] = _T("");
TCHAR g_szDbName[MAX_DB_NAME] = _T("netxms_db");
TCHAR g_szDbSchema[MAX_DB_NAME] = _T("");
TCHAR g_szJavaPath[MAX_DB_NAME] = _T("java");


//
// Load and parse configuration file
// Returns TRUE on success and FALSE on failure
//

static TCHAR s_encryptedDbPassword[MAX_DB_STRING] = _T("");
static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
   { _T("CodePage"), CT_STRING, 0, 0, 256, 0, g_szCodePage },
   { _T("CreateCrashDumps"), CT_BOOLEAN, 0, 0, AF_CATCH_EXCEPTIONS, 0, &g_dwFlags },
   { _T("DataDirectory"), CT_STRING, 0, 0, MAX_PATH, 0, g_szDataDir },
   { _T("DBDriver"), CT_STRING, 0, 0, MAX_PATH, 0, g_szDbDriver },
   { _T("DBDrvParams"), CT_STRING, 0, 0, MAX_PATH, 0, g_szDbDrvParams },
   { _T("DBEncryptedPassword"), CT_STRING, 0, 0, MAX_DB_STRING, 0, s_encryptedDbPassword },
   { _T("DBLogin"), CT_STRING, 0, 0, MAX_DB_LOGIN, 0, g_szDbLogin },
   { _T("DBName"), CT_STRING, 0, 0, MAX_DB_NAME, 0, g_szDbName },
   { _T("DBPassword"), CT_STRING, 0, 0, MAX_DB_PASSWORD, 0, g_szDbPassword },
   { _T("DBSchema"), CT_STRING, 0, 0, MAX_PATH, 0, g_szDbSchema },
   { _T("DBServer"), CT_STRING, 0, 0, MAX_PATH, 0, g_szDbServer },
   { _T("DumpDirectory"), CT_STRING, 0, 0, MAX_PATH, 0, g_szDumpDir },
   { _T("FullCrashDumps"), CT_BOOLEAN, 0, 0, AF_WRITE_FULL_DUMP, 0, &g_dwFlags },
   { _T("JavaPath"), CT_STRING, 0, 0, MAX_PATH, 0, g_szJavaPath },
   { _T("LibraryDirectory"), CT_STRING, 0, 0, MAX_PATH, 0, g_szLibDir },
   { _T("ListenAddress"), CT_STRING, 0, 0, MAX_PATH, 0, g_szListenAddress },
   { _T("LogFailedSQLQueries"), CT_BOOLEAN, 0, 0, AF_LOG_SQL_ERRORS, 0, &g_dwFlags },
   { _T("LogFile"), CT_STRING, 0, 0, MAX_PATH, 0, g_szLogFile },
   { _T("Module"), CT_STRING_LIST, '\n', 0, 0, 0, &g_pszModLoadList },
   { _T("ProcessAffinityMask"), CT_LONG, 0, 0, 0, 0, &g_processAffinityMask },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};

BOOL NXCORE_EXPORTABLE LoadConfig()
{
   BOOL bSuccess = FALSE;
	Config *config;

#if !defined(_WIN32) && !defined(_NETWARE)
	if (!_tcscmp(g_szConfigFile, _T("{search}")))
	{
		if (access(PREFIX _T("/etc/netxmsd.conf"), 4) == 0)
		{
			_tcscpy(g_szConfigFile, PREFIX _T("/etc/netxmsd.conf"));
		}
		else if (access(_T("/usr/etc/netxmsd.conf"), 4) == 0)
		{
			_tcscpy(g_szConfigFile, _T("/usr/etc/netxmsd.conf"));
		}
		else
		{
			_tcscpy(g_szConfigFile, _T("/etc/netxmsd.conf"));
		}
	}
#endif

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


//
// Read string value from metadata table
//

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


//
// Callback for configuration variables change
//

static void OnConfigVariableChange(BOOL isCLOB, const TCHAR *name, const TCHAR *value)
{
	// Restart syslog parser if configuration was changed
	if (isCLOB && !_tcscmp(name, _T("SyslogParser")))
	{
		ReinitializeSyslogParser();
	}
}


//
// Read string value from configuration table
//

BOOL NXCORE_EXPORTABLE ConfigReadStr(const TCHAR *szVar, TCHAR *szBuffer, int iBufSize, const TCHAR *szDefault)
{
   DB_RESULT hResult;
   BOOL bSuccess = FALSE;

   nx_strncpy(szBuffer, szDefault, iBufSize);
   if (_tcslen(szVar) > 127)
      return FALSE;

	DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, _T("SELECT var_value FROM config WHERE var_name=?"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, szVar, DB_BIND_STATIC);
		hResult = DBSelectPrepared(hStmt);
	   if (hResult != NULL)
		{
			if (DBGetNumRows(hResult) > 0)
			{
				DBGetField(hResult, 0, 0, szBuffer, iBufSize);
				DecodeSQLString(szBuffer);
				DbgPrintf(8, _T("ConfigReadStr: name=%s value=\"%s\""), szVar, szBuffer);
				bSuccess = TRUE;
			}
		   DBFreeResult(hResult);
		}
		DBFreeStatement(hStmt);
	}
   return bSuccess;
}


//
// Read multibyte string from configuration table
//

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


//
// Read integer value from configuration table
//

int NXCORE_EXPORTABLE ConfigReadInt(const TCHAR *szVar, int iDefault)
{
   TCHAR szBuffer[64];

   if (ConfigReadStr(szVar, szBuffer, 64, _T("")))
      return _tcstol(szBuffer, NULL, 0);
   else
      return iDefault;
}


//
// Read unsigned long value from configuration table
//

DWORD NXCORE_EXPORTABLE ConfigReadULong(const TCHAR *szVar, DWORD dwDefault)
{
   TCHAR szBuffer[64];

   if (ConfigReadStr(szVar, szBuffer, 64, _T("")))
      return _tcstoul(szBuffer, NULL, 0);
   else
      return dwDefault;
}


//
// Read byte array (in hex form) from configuration table into integer array
//

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


//
// Write string value to configuration table
//

BOOL NXCORE_EXPORTABLE ConfigWriteStr(const TCHAR *szVar, const TCHAR *szValue, BOOL bCreate,
												  BOOL isVisible, BOOL needRestart)
{
   DB_RESULT hResult;
   TCHAR *pszEscValue, szQuery[1024];
   BOOL bVarExist = FALSE, success;

   if (_tcslen(szVar) > 127)
      return FALSE;

   // Check for variable existence
   _sntprintf(szQuery, 1024, _T("SELECT var_value FROM config WHERE var_name='%s'"), szVar);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         bVarExist = TRUE;
      DBFreeResult(hResult);
   }

   // Don't create non-existing variable if creation flag not set
   if (!bCreate && !bVarExist)
      return FALSE;

   // Create or update variable value
   pszEscValue = EncodeSQLString(szValue);
   if (bVarExist)
      _sntprintf(szQuery, 1024, _T("UPDATE config SET var_value='%s' WHERE var_name='%s'"),
              pszEscValue, szVar);
   else
      _sntprintf(szQuery, 1024, _T("INSERT INTO config (var_name,var_value,is_visible,")
                                _T("need_server_restart) VALUES ('%s','%s',%d,%d)"),
                 szVar, pszEscValue, isVisible, needRestart);
   free(pszEscValue);
   success = DBQuery(g_hCoreDB, szQuery);
	if (success)
		OnConfigVariableChange(FALSE, szVar, szValue);
	return success;
}


//
// Write integer value to configuration table
//

BOOL NXCORE_EXPORTABLE ConfigWriteInt(const TCHAR *szVar, int iValue, BOOL bCreate, BOOL isVisible, BOOL needRestart)
{
   TCHAR szBuffer[64];

   _sntprintf(szBuffer, 64, _T("%d"), iValue);
   return ConfigWriteStr(szVar, szBuffer, bCreate, isVisible, needRestart);
}


//
// Write unsigned long value to configuration table
//

BOOL NXCORE_EXPORTABLE ConfigWriteULong(const TCHAR *szVar, DWORD dwValue, BOOL bCreate, BOOL isVisible, BOOL needRestart)
{
   TCHAR szBuffer[64];

   _sntprintf(szBuffer, 64, _T("%u"), dwValue);
   return ConfigWriteStr(szVar, szBuffer, bCreate, isVisible, needRestart);
}


//
// Write integer array to configuration table
//

BOOL NXCORE_EXPORTABLE ConfigWriteByteArray(const TCHAR *pszVar, int *pnArray, int nSize, BOOL bCreate, BOOL isVisible, BOOL needRestart)
{
   TCHAR szBuffer[256];
   int i, j;

   for(i = 0, j = 0; (i < nSize) && (i < 127); i++, j += 2)
      _sntprintf(&szBuffer[j], 256 - j, _T("%02X"), (char)((pnArray[i] > 127) ? 127 : ((pnArray[i] < -127) ? -127 : pnArray[i])));
   return ConfigWriteStr(pszVar, szBuffer, bCreate, isVisible, needRestart);
}


//
// Read large string (clob) value from configuration table
//

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


//
// Write large string (clob) value to configuration table
//

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
