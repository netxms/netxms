/*
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Victor Kirhenshtein
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
extern TCHAR *g_moduleLoadList;
extern TCHAR *g_pdsLoadList;
extern InetAddressList g_peerNodeAddrList;
extern TCHAR *g_serverCACertificatesPath;
extern TCHAR g_serverCertificatePath[];
extern TCHAR g_serverCertificateKeyPath[];
extern char g_serverCertificatePassword[];

/**
 * Database connection parameters
 */
TCHAR g_szDbDriver[MAX_PATH] = _T("");
TCHAR g_szDbDrvParams[MAX_PATH] = _T("");
TCHAR g_szDbServer[MAX_PATH] = _T("127.0.0.1");
TCHAR g_szDbLogin[MAX_DB_LOGIN] = _T("netxms");
TCHAR g_szDbPassword[MAX_PASSWORD] = _T("");
TCHAR g_szDbName[MAX_DB_NAME] = _T("netxms_db");
TCHAR g_szDbSchema[MAX_DB_NAME] = _T("");

/**
 * Debug level from config
 */
static UINT32 s_debugLevel = (UINT32)NXCONFIG_UNINITIALIZED_VALUE;

/**
 * Debug tags from config
 */
static TCHAR *s_debugTags = NULL;

/**
 * Peer node information
 */
static TCHAR s_peerNode[MAX_DB_STRING];

/**
 * Config file template
 */
static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
   { _T("BackgroundLogWriter"), CT_BOOLEAN64, 0, 0, AF_BACKGROUND_LOG_WRITER, 0, &g_flags, NULL },
   { _T("CodePage"), CT_MB_STRING, 0, 0, 256, 0, g_szCodePage, NULL },
   { _T("CreateCrashDumps"), CT_BOOLEAN64, 0, 0, AF_CATCH_EXCEPTIONS, 0, &g_flags, NULL },
   { _T("DailyLogFileSuffix"), CT_STRING, 0, 0, 64, 0, g_szDailyLogFileSuffix, NULL },
   { _T("DataDirectory"), CT_STRING, 0, 0, MAX_PATH, 0, g_netxmsdDataDir, NULL },
   { _T("DBDriver"), CT_STRING, 0, 0, MAX_PATH, 0, g_szDbDriver, NULL },
   { _T("DBDrvParams"), CT_STRING, 0, 0, MAX_PATH, 0, g_szDbDrvParams, NULL },
   { _T("DBLogin"), CT_STRING, 0, 0, MAX_DB_LOGIN, 0, g_szDbLogin, NULL },
   { _T("DBName"), CT_STRING, 0, 0, MAX_DB_NAME, 0, g_szDbName, NULL },
   { _T("DBPassword"), CT_STRING, 0, 0, MAX_PASSWORD, 0, g_szDbPassword, NULL },
   { _T("DBEncryptedPassword"), CT_STRING, 0, 0, MAX_PASSWORD, 0, g_szDbPassword, NULL },
   { _T("DBSchema"), CT_STRING, 0, 0, MAX_DB_NAME, 0, g_szDbSchema, NULL },
   { _T("DBServer"), CT_STRING, 0, 0, MAX_PATH, 0, g_szDbServer, NULL },
   { _T("DebugLevel"), CT_LONG, 0, 0, 0, 0, &s_debugLevel, &s_debugLevel },
   { _T("DebugTags"), CT_STRING_LIST, ',', 0, 0, 0, &s_debugTags, NULL },
   { _T("DumpDirectory"), CT_STRING, 0, 0, MAX_PATH, 0, g_szDumpDir, NULL },
   { _T("EnableServerConsole"), CT_BOOLEAN64, 0, 0, AF_ENABLE_LOCAL_CONSOLE, 0, &g_flags, NULL },
   { _T("FullCrashDumps"), CT_BOOLEAN64, 0, 0, AF_WRITE_FULL_DUMP, 0, &g_flags, NULL },
   { _T("LibraryDirectory"), CT_STRING, 0, 0, MAX_PATH, 0, g_netxmsdLibDir, NULL },
   { _T("ListenAddress"), CT_STRING, 0, 0, MAX_PATH, 0, g_szListenAddress, NULL },
   { _T("LogFailedSQLQueries"), CT_BOOLEAN64, 0, 0, AF_LOG_SQL_ERRORS, 0, &g_flags, NULL },
   { _T("LogFile"), CT_STRING, 0, 0, MAX_PATH, 0, g_szLogFile, NULL },
   { _T("LogHistorySize"), CT_LONG, 0, 0, 0, 0, &g_logHistorySize, NULL },
   { _T("LogRotationMode"), CT_LONG, 0, 0, 0, 0, &g_logRotationMode, NULL },
   { _T("MaxLogSize"), CT_SIZE_BYTES, 0, 0, 0, 0, &g_maxLogSize, NULL },
   { _T("Module"), CT_STRING_LIST, '\n', 0, 0, 0, &g_moduleLoadList, NULL },
   { _T("PeerNode"), CT_STRING, 0, 0, MAX_DB_STRING, 0, s_peerNode, NULL },
   { _T("PerfDataStorageDriver"), CT_STRING_LIST, '\n', 0, 0, 0, &g_pdsLoadList, NULL },
   { _T("ProcessAffinityMask"), CT_LONG, 0, 0, 0, 0, &g_processAffinityMask, NULL },
   { _T("ServerCACertificate"), CT_STRING_LIST, '\n', 0, 0, 0, &g_serverCACertificatesPath, NULL },
   { _T("ServerCertificate"), CT_STRING, 0, 0, MAX_PATH, 0, g_serverCertificatePath, NULL },
   { _T("ServerCertificateKey"), CT_STRING, 0, 0, MAX_PATH, 0, g_serverCertificateKeyPath, NULL },
   { _T("ServerCertificatePassword"), CT_MB_STRING, 0, 0, MAX_PASSWORD, 0, g_serverCertificatePassword, NULL },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL, NULL }
};

/**
 * Loaded server config
 */
Config g_serverConfig;

/**
 * Load and parse configuration file
 * Returns TRUE on success and FALSE on failure
 */
bool NXCORE_EXPORTABLE LoadConfig(int *debugLevel)
{
   bool bSuccess = false;

	if (!_tcscmp(g_szConfigFile, _T("{search}")))
	{
#ifdef _WIN32
      TCHAR path[MAX_PATH];
      GetNetXMSDirectory(nxDirEtc, path);
      _tcscat(path, _T("\\netxmsd.conf"));
      if (_taccess(path, 4) == 0)
      {
		   _tcscpy(g_szConfigFile, path);
      }
      else
      {
         _tcscpy(g_szConfigFile, _T("C:\\netxmsd.conf"));
      }
#else
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
#endif
	}

   if (IsStandalone())
      _tprintf(_T("Using configuration file \"%s\"\n"), g_szConfigFile);

	if (g_serverConfig.loadConfig(g_szConfigFile, _T("server")) && g_serverConfig.parseTemplate(_T("server"), m_cfgTemplate))
   {
      if ((!_tcsicmp(g_szLogFile, _T("{EventLog}"))) ||
          (!_tcsicmp(g_szLogFile, _T("{syslog}"))))
      {
         g_flags |= AF_USE_SYSLOG;
      }
      else
      {
         g_flags &= ~AF_USE_SYSLOG;
      }
      bSuccess = true;
   }

	if (*debugLevel == NXCONFIG_UNINITIALIZED_VALUE)
	   *debugLevel = (int)s_debugLevel;

   if (s_debugTags != NULL)
   {
      int count;
      TCHAR **tagList = SplitString(s_debugTags, _T(','), &count);
      if (tagList != NULL)
      {
         for(int i = 0; i < count; i++)
         {
            TCHAR *level = _tcschr(tagList[i], _T(':'));
            if (level != NULL)
            {
               *level = 0;
               level++;
               Trim(tagList[i]);
               nxlog_set_debug_level_tag(tagList[i], _tcstol(level, NULL, 0));
            }
            free(tagList[i]);
         }
         free(tagList);
      }
      free(s_debugTags);
   }

	// Decrypt password
   DecryptPassword(g_szDbLogin, g_szDbPassword, g_szDbPassword, MAX_PASSWORD);

   // Parse peer node information
   if (s_peerNode[0] != 0)
   {
      int count = 0;
      TCHAR **list = SplitString(s_peerNode, _T(','), &count);
      for(int i = 0; i < count; i++)
      {
         InetAddress a = InetAddress::resolveHostName(list[i]);
         if (a.isValidUnicast())
            g_peerNodeAddrList.add(a);
         free(list[i]);
      }
      free(list);
   }
   return bSuccess;
}

/**
 * Metadata cache
 */
static StringMap s_metadataCache;
static RWLOCK s_metadataCacheLock = RWLockCreate();

/**
 * Pre-load metadata
 */
void MetaDataPreLoad()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT var_name,var_value FROM metadata"));
   if (hResult != NULL)
   {
      RWLockWriteLock(s_metadataCacheLock, INFINITE);
      s_metadataCache.clear();
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         s_metadataCache.setPreallocated(DBGetField(hResult, i, 0, NULL, 0), DBGetField(hResult, i, 1, NULL, 0));
      }
      RWLockUnlock(s_metadataCacheLock);
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Read string value from metadata table
 */
bool NXCORE_EXPORTABLE MetaDataReadStr(const TCHAR *name, TCHAR *buffer, int bufSize, const TCHAR *defaultValue)
{
   bool bSuccess = false;

   nx_strncpy(buffer, defaultValue, bufSize);
   if (_tcslen(name) > 127)
      return false;

   RWLockReadLock(s_metadataCacheLock, INFINITE);
   const TCHAR *value = s_metadataCache.get(name);
   if (value != NULL)
   {
      nx_strncpy(buffer, value, bufSize);
      bSuccess = true;
   }
   RWLockUnlock(s_metadataCacheLock);

   if (!bSuccess)
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT var_value FROM metadata WHERE var_name=?"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
         DB_RESULT hResult = DBSelectPrepared(hStmt);
         if (hResult != NULL)
         {
            if (DBGetNumRows(hResult) > 0)
            {
               DBGetField(hResult, 0, 0, buffer, bufSize);
               RWLockWriteLock(s_metadataCacheLock, INFINITE);
               s_metadataCache.setPreallocated(_tcsdup(name), DBGetField(hResult, 0, 0, NULL, 0));
               RWLockUnlock(s_metadataCacheLock);
               bSuccess = true;
            }
            DBFreeResult(hResult);
         }
         DBFreeStatement(hStmt);
      }
      DBConnectionPoolReleaseConnection(hdb);
   }
   return bSuccess;
}

/**
 * Read integer value from metadata table
 */
INT32 NXCORE_EXPORTABLE MetaDataReadInt32(const TCHAR *var, INT32 defaultValue)
{
   TCHAR buffer[256];
   if (MetaDataReadStr(var, buffer, 256, _T("")))
   {
      TCHAR *eptr;
      INT32 value = _tcstol(buffer, &eptr, 0);
      return (*eptr == 0) ? value : defaultValue;
   }
   else
   {
      return defaultValue;
   }
}

/**
 * Write string value to metadata table
 */
bool NXCORE_EXPORTABLE MetaDataWriteStr(const TCHAR *varName, const TCHAR *value)
{
   if (_tcslen(varName) > 63)
      return false;

   RWLockWriteLock(s_metadataCacheLock, INFINITE);
   s_metadataCache.set(varName, value);
   RWLockUnlock(s_metadataCacheLock);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Check for variable existence
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT var_value FROM metadata WHERE var_name=?"));
	if (hStmt == NULL)
   {
      DBConnectionPoolReleaseConnection(hdb);
		return false;
   }
	DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, varName, DB_BIND_STATIC);
	DB_RESULT hResult = DBSelectPrepared(hStmt);
   bool bVarExist = false;
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         bVarExist = true;
      DBFreeResult(hResult);
   }
	DBFreeStatement(hStmt);

   // Create or update variable value
   if (bVarExist)
	{
		hStmt = DBPrepare(hdb, _T("UPDATE metadata SET var_value=? WHERE var_name=?"));
		if (hStmt == NULL)
      {
         DBConnectionPoolReleaseConnection(hdb);
			return false;
      }
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, value, DB_BIND_STATIC);
		DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, varName, DB_BIND_STATIC);
	}
   else
	{
		hStmt = DBPrepare(hdb, _T("INSERT INTO metadata (var_name,var_value) VALUES (?,?)"));
		if (hStmt == NULL)
      {
         DBConnectionPoolReleaseConnection(hdb);
			return false;
      }
		DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, varName, DB_BIND_STATIC);
		DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, value, DB_BIND_STATIC);
	}
   bool success = DBExecute(hStmt);
	DBFreeStatement(hStmt);
   DBConnectionPoolReleaseConnection(hdb);
	return success;
}

/**
 * Write integer value to metadata table
 */
bool NXCORE_EXPORTABLE MetaDataWriteInt32(const TCHAR *name, INT32 value)
{
   TCHAR buffer[32];
   _sntprintf(buffer, 32, _T("%d"), value);
   return MetaDataWriteStr(name, buffer);
}

/**
 * Config cache
 */
static StringMap s_configCache;
static RWLOCK s_configCacheLock = RWLockCreate();

/**
 * Callback for configuration variables change
 */
static void OnConfigVariableChange(bool isCLOB, const TCHAR *name, const TCHAR *value)
{
   RWLockWriteLock(s_configCacheLock, INFINITE);
   s_configCache.set(name, value);
   RWLockUnlock(s_configCacheLock);

	// Restart syslog parser if configuration was changed
	if (isCLOB && !_tcscmp(name, _T("SyslogParser")))
	{
		ReinitializeSyslogParser();
	}
   else if (!_tcscmp(name, _T("AlarmSummaryEmailSchedule")))
   {
      if (ConfigReadInt(_T("EnableAlarmSummaryEmails"), 0))
         EnableAlarmSummaryEmails();  // this call will update schedule for existing task
   }
   else if (!_tcsncmp(name, _T("CAS"), 3))
   {
      CASReadSettings();
   }
   else if (!_tcscmp(name, _T("DefaultDCIPollingInterval")))
   {
      DCObject::m_defaultPollingInterval = _tcstol(value, NULL, 0);
   }
   else if (!_tcscmp(name, _T("DefaultDCIRetentionTime")))
   {
      DCObject::m_defaultRetentionTime = _tcstol(value, NULL, 0);
   }
   else if (!_tcscmp(name, _T("EnableAlarmSummaryEmails")))
   {
      if (_tcstol(value, NULL, 0))
         EnableAlarmSummaryEmails();
      else
         RemoveScheduledTaskByHandlerId(ALARM_SUMMARY_EMAIL_TASK_ID);
   }
   else if (!_tcscmp(name, _T("StrictAlarmStatusFlow")))
   {
      NotifyClientSessions(NX_NOTIFY_ALARM_STATUS_FLOW_CHANGED, _tcstol(value, NULL, 0));
   }
   else if (!_tcsncmp(name, _T("Syslog"), 6))
   {
      OnSyslogConfigurationChange(name, value);
   }
}

/**
 * Read string value from configuration table
 */
bool NXCORE_EXPORTABLE ConfigReadStrEx(DB_HANDLE dbHandle, const TCHAR *szVar, TCHAR *szBuffer, int iBufSize, const TCHAR *szDefault)
{
   nx_strncpy(szBuffer, szDefault, iBufSize);
   if (_tcslen(szVar) > 127)
      return false;

   RWLockReadLock(s_configCacheLock, INFINITE);
   const TCHAR *value = s_configCache.get(szVar);
   RWLockUnlock(s_configCacheLock);
   if (value != NULL)
   {
      nx_strncpy(szBuffer, value, iBufSize);
		DbgPrintf(8, _T("ConfigReadStr: (cached) name=%s value=\"%s\""), szVar, szBuffer);
      return true;
   }

   bool bSuccess = false;
   DB_HANDLE hdb = (dbHandle == NULL) ? DBConnectionPoolAcquireConnection() : dbHandle;
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT var_value FROM config WHERE var_name=?"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, szVar, DB_BIND_STATIC);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
	   if (hResult != NULL)
		{
			if (DBGetNumRows(hResult) > 0)
			{
				DBGetField(hResult, 0, 0, szBuffer, iBufSize);
				DbgPrintf(8, _T("ConfigReadStr: name=%s value=\"%s\""), szVar, szBuffer);
				bSuccess = true;
			}
		   DBFreeResult(hResult);
		}
		DBFreeStatement(hStmt);
	}
   if (dbHandle == NULL)
      DBConnectionPoolReleaseConnection(hdb);

   if (bSuccess)
   {
      RWLockWriteLock(s_configCacheLock, INFINITE);
      s_configCache.set(szVar, szBuffer);
      RWLockUnlock(s_configCacheLock);
   }

   return bSuccess;
}

/**
 * Read string value from configuration table
 */
bool NXCORE_EXPORTABLE ConfigReadStr(const TCHAR *var, TCHAR *buffer, int bufferSize, const TCHAR *defaultValue)
{
   return ConfigReadStrEx(NULL, var, buffer, bufferSize, defaultValue);
}

/**
 * Read multibyte string from configuration table
 */
#ifdef UNICODE

bool NXCORE_EXPORTABLE ConfigReadStrA(const WCHAR *szVar, char *szBuffer, int iBufSize, const char *szDefault)
{
	WCHAR *wcBuffer = (WCHAR *)malloc(iBufSize * sizeof(WCHAR));
   bool rc = ConfigReadStr(szVar, wcBuffer, iBufSize, _T(""));
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
 * Read string value from configuration table as UTF8 string
 */
bool NXCORE_EXPORTABLE ConfigReadStrUTF8(const TCHAR *szVar, char *szBuffer, int iBufSize, const char *szDefault)
{
   DB_RESULT hResult;
   bool bSuccess = false;

   strncpy(szBuffer, szDefault, iBufSize);
   if (_tcslen(szVar) > 127)
      return false;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT var_value FROM config WHERE var_name=?"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, szVar, DB_BIND_STATIC);
		hResult = DBSelectPrepared(hStmt);
	   if (hResult != NULL)
		{
			if (DBGetNumRows(hResult) > 0)
			{
				DBGetFieldUTF8(hResult, 0, 0, szBuffer, iBufSize);
				bSuccess = true;
			}
		   DBFreeResult(hResult);
		}
		DBFreeStatement(hStmt);
	}
   DBConnectionPoolReleaseConnection(hdb);

   return bSuccess;
}

/**
 * Read integer value from configuration table
 */
int NXCORE_EXPORTABLE ConfigReadInt(const TCHAR *szVar, int iDefault)
{
   return ConfigReadIntEx(NULL, szVar, iDefault);
}

/**
 * Read integer value from configuration table
 */
int NXCORE_EXPORTABLE ConfigReadIntEx(DB_HANDLE hdb, const TCHAR *szVar, int iDefault)
{
   TCHAR szBuffer[64];

   if (ConfigReadStrEx(hdb, szVar, szBuffer, 64, _T("")))
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
bool NXCORE_EXPORTABLE ConfigReadByteArray(const TCHAR *pszVar, int *pnArray, int nSize, int nDefault)
{
   TCHAR szBuffer[256];
   char pbBytes[128];
   bool bResult;
   int i, nLen;

   if (ConfigReadStr(pszVar, szBuffer, 256, _T("")))
   {
      StrToBin(szBuffer, (BYTE *)pbBytes, 128);
      nLen = (int)_tcslen(szBuffer) / 2;
      for(i = 0; (i < nSize) && (i < nLen); i++)
         pnArray[i] = pbBytes[i];
      for(; i < nSize; i++)
         pnArray[i] = nDefault;
      bResult = true;
   }
   else
   {
      for(i = 0; i < nSize; i++)
         pnArray[i] = nDefault;
      bResult = false;
   }
   return bResult;
}

/**
 * Write string value to configuration table
 */
bool NXCORE_EXPORTABLE ConfigWriteStr(const TCHAR *varName, const TCHAR *value, bool bCreate, bool isVisible, bool needRestart)
{
   if (_tcslen(varName) > 63)
      return false;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Check for variable existence
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT var_value FROM config WHERE var_name=?"));
	if (hStmt == NULL)
   {
      DBConnectionPoolReleaseConnection(hdb);
		return false;
   }
	DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, varName, DB_BIND_STATIC);
	DB_RESULT hResult = DBSelectPrepared(hStmt);
   bool bVarExist = false;
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         bVarExist = true;
      DBFreeResult(hResult);
   }
	DBFreeStatement(hStmt);

   // Don't create non-existing variable if creation flag not set
   if (!bCreate && !bVarExist)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return false;
   }

   // Create or update variable value
   if (bVarExist)
	{
		hStmt = DBPrepare(hdb, _T("UPDATE config SET var_value=? WHERE var_name=?"));
		if (hStmt == NULL)
      {
         DBConnectionPoolReleaseConnection(hdb);
			return false;
      }
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, value, DB_BIND_STATIC);
		DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, varName, DB_BIND_STATIC);
	}
   else
	{
		hStmt = DBPrepare(hdb, _T("INSERT INTO config (var_name,var_value,is_visible,need_server_restart) VALUES (?,?,?,?)"));
		if (hStmt == NULL)
      {
         DBConnectionPoolReleaseConnection(hdb);
			return false;
      }
		DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, varName, DB_BIND_STATIC);
		DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, value, DB_BIND_STATIC);
		DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (LONG)(isVisible ? 1 : 0));
		DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (LONG)(needRestart ? 1 : 0));
	}
   bool success = DBExecute(hStmt);
	DBFreeStatement(hStmt);
   DBConnectionPoolReleaseConnection(hdb);
	if (success)
		OnConfigVariableChange(false, varName, value);
	return success;
}

/**
 * Write integer value to configuration table
 */
bool NXCORE_EXPORTABLE ConfigWriteInt(const TCHAR *szVar, int iValue, bool bCreate, bool isVisible, bool needRestart)
{
   TCHAR szBuffer[64];

   _sntprintf(szBuffer, 64, _T("%d"), iValue);
   return ConfigWriteStr(szVar, szBuffer, bCreate, isVisible, needRestart);
}

/**
 * Write unsigned long value to configuration table
 */
bool NXCORE_EXPORTABLE ConfigWriteULong(const TCHAR *szVar, UINT32 dwValue, bool bCreate, bool isVisible, bool needRestart)
{
   TCHAR szBuffer[64];

   _sntprintf(szBuffer, 64, _T("%u"), dwValue);
   return ConfigWriteStr(szVar, szBuffer, bCreate, isVisible, needRestart);
}

/**
 * Write integer array to configuration table
 */
bool NXCORE_EXPORTABLE ConfigWriteByteArray(const TCHAR *pszVar, int *pnArray, int nSize, bool bCreate, bool isVisible, bool needRestart)
{
   TCHAR szBuffer[256];
   int i, j;

   for(i = 0, j = 0; (i < nSize) && (i < 127); i++, j += 2)
      _sntprintf(&szBuffer[j], 256 - j, _T("%02X"), (char)((pnArray[i] > 127) ? 127 : ((pnArray[i] < -127) ? -127 : pnArray[i])));
   return ConfigWriteStr(pszVar, szBuffer, bCreate, isVisible, needRestart);
}

/**
 * Delete configuratrion variable
 */
bool NXCORE_EXPORTABLE ConfigDelete(const TCHAR *name)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   TCHAR query[1024];
   _sntprintf(query, 1024, _T("DELETE FROM config WHERE var_name=%s"), (const TCHAR *)DBPrepareString(hdb, name));
   bool success = DBQuery(hdb, query) ? true : false;

   DBConnectionPoolReleaseConnection(hdb);

   if (success)
   {
      RWLockWriteLock(s_configCacheLock, INFINITE);
      s_configCache.remove(name);
      RWLockUnlock(s_configCacheLock);
   }

   return success;
}

/**
 * Read large string (clob) value from configuration table
 */
TCHAR NXCORE_EXPORTABLE *ConfigReadCLOB(const TCHAR *var, const TCHAR *defValue)
{
   TCHAR *result = NULL;

   if (_tcslen(var) <= 63)
	{
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT var_value FROM config_clob WHERE var_name=?"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, var, DB_BIND_STATIC);
         DB_RESULT hResult = DBSelectPrepared(hStmt);
		   if (hResult != NULL)
		   {
			   if (DBGetNumRows(hResult) > 0)
			   {
				   result = DBGetField(hResult, 0, 0, NULL, 0);
			   }
			   DBFreeResult(hResult);
		   }
         DBFreeStatement(hStmt);
      }
      DBConnectionPoolReleaseConnection(hdb);
	}

	// Return default value in case of error
	if ((result == NULL) && (defValue != NULL))
		result = _tcsdup(defValue);

	return result;
}

/**
 * Write large string (clob) value to configuration table
 */
bool NXCORE_EXPORTABLE ConfigWriteCLOB(const TCHAR *var, const TCHAR *value, bool bCreate)
{
   if (_tcslen(var) > 63)
      return false;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Check for variable existence
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT var_value FROM config_clob WHERE var_name=?"));
	if (hStmt == NULL)
   {
      DBConnectionPoolReleaseConnection(hdb);
		return false;
   }
	DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, var, DB_BIND_STATIC);
	DB_RESULT hResult = DBSelectPrepared(hStmt);
   bool bVarExist = false;
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         bVarExist = true;
      DBFreeResult(hResult);
   }
	DBFreeStatement(hStmt);

   // Don't create non-existing variable if creation flag not set
   if (!bCreate && !bVarExist)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return false;
   }

   // Create or update variable value
   if (bVarExist)
	{
		hStmt = DBPrepare(hdb, _T("UPDATE config_clob SET var_value=? WHERE var_name=?"));
		if (hStmt == NULL)
      {
         DBConnectionPoolReleaseConnection(hdb);
			return false;
      }
		DBBind(hStmt, 1, DB_SQLTYPE_TEXT, value, DB_BIND_STATIC);
		DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, var, DB_BIND_STATIC);
	}
   else
	{
		hStmt = DBPrepare(hdb, _T("INSERT INTO config_clob (var_name,var_value) VALUES (?,?)"));
		if (hStmt == NULL)
      {
         DBConnectionPoolReleaseConnection(hdb);
			return false;
      }
		DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, var, DB_BIND_STATIC);
		DBBind(hStmt, 2, DB_SQLTYPE_TEXT, value, DB_BIND_STATIC);
	}
   bool success = DBExecute(hStmt) ? true : false;
	DBFreeStatement(hStmt);
   DBConnectionPoolReleaseConnection(hdb);
	if (success)
		OnConfigVariableChange(true, var, value);
	return success;
}
