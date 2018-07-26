/*
** NetXMS - Network Management System
** Copyright (C) 2003-2018 Victor Kirhenshtein
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
#include <nxjava.h>

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
extern TCHAR g_jvmPath[];
extern TCHAR *g_jvmOptions;
extern TCHAR *g_userClasspath;

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
   { _T("ClassPath"), CT_STRING_LIST, JAVA_CLASSPATH_SEPARATOR, 0, 0, 0, &g_userClasspath },
   { _T("CodePage"), CT_MB_STRING, 0, 0, 256, 0, g_szCodePage, NULL },
   { _T("CreateCrashDumps"), CT_BOOLEAN64, 0, 0, AF_CATCH_EXCEPTIONS, 0, &g_flags, NULL },
   { _T("DailyLogFileSuffix"), CT_STRING, 0, 0, 64, 0, g_szDailyLogFileSuffix, NULL },
   { _T("DataDirectory"), CT_STRING, 0, 0, MAX_PATH, 0, g_netxmsdDataDir, NULL },
   { _T("DBCacheConfigurationTables"), CT_BOOLEAN64, 0, 0, AF_CACHE_DB_ON_STARTUP, 0, &g_flags, NULL },
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
   { _T("JVM"), CT_STRING, 0, 0, MAX_PATH, 0,  g_jvmPath },
   { _T("JVMOptions"), CT_STRING_LIST, _T('\n'), 0, 0, 0, &g_jvmOptions },
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

   // Try to set default JVM before parsing config
   FindJavaRuntime(g_jvmPath, MAX_PATH);

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

   _tcslcpy(buffer, defaultValue, bufSize);
   if (_tcslen(name) > 127)
      return false;

   RWLockReadLock(s_metadataCacheLock, INFINITE);
   const TCHAR *value = s_metadataCache.get(name);
   if (value != NULL)
   {
      _tcslcpy(buffer, value, bufSize);
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
INT32 NXCORE_EXPORTABLE MetaDataReadInt32(const TCHAR *variable, INT32 defaultValue)
{
   TCHAR buffer[256];
   if (MetaDataReadStr(variable, buffer, 256, _T("")))
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
bool NXCORE_EXPORTABLE MetaDataWriteStr(const TCHAR *variable, const TCHAR *value)
{
   if (_tcslen(variable) > 63)
      return false;

   RWLockWriteLock(s_metadataCacheLock, INFINITE);
   s_metadataCache.set(variable, value);
   RWLockUnlock(s_metadataCacheLock);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Check for variable existence
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT var_value FROM metadata WHERE var_name=?"));
	if (hStmt == NULL)
   {
      DBConnectionPoolReleaseConnection(hdb);
		return false;
   }
	DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, variable, DB_BIND_STATIC);
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
		DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, variable, DB_BIND_STATIC);
	}
   else
	{
		hStmt = DBPrepare(hdb, _T("INSERT INTO metadata (var_name,var_value) VALUES (?,?)"));
		if (hStmt == NULL)
      {
         DBConnectionPoolReleaseConnection(hdb);
			return false;
      }
		DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, variable, DB_BIND_STATIC);
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
bool NXCORE_EXPORTABLE MetaDataWriteInt32(const TCHAR *variable, INT32 value)
{
   TCHAR buffer[32];
   _sntprintf(buffer, 32, _T("%d"), value);
   return MetaDataWriteStr(variable, buffer);
}

/**
 * Config cache
 */
static StringMap s_configCache;
static RWLOCK s_configCacheLock = RWLockCreate();

/**
 * Pre-load configuration
 */
void ConfigPreLoad()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT var_name,var_value FROM config"));
   if (hResult != NULL)
   {
      RWLockWriteLock(s_configCacheLock, INFINITE);
      s_configCache.clear();
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         s_configCache.setPreallocated(DBGetField(hResult, i, 0, NULL, 0), DBGetField(hResult, i, 1, NULL, 0));
      }
      RWLockUnlock(s_configCacheLock);
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

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
      if (ConfigReadBoolean(_T("EnableAlarmSummaryEmails"), false))
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
         DeleteScheduledTaskByHandlerId(ALARM_SUMMARY_EMAIL_TASK_ID);
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
bool NXCORE_EXPORTABLE ConfigReadStrEx(DB_HANDLE dbHandle, const TCHAR *variable, TCHAR *buffer, size_t size, const TCHAR *defaultValue)
{
   if (defaultValue != NULL)
      _tcslcpy(buffer, defaultValue, size);
   if (_tcslen(variable) > 127)
      return false;

   RWLockReadLock(s_configCacheLock, INFINITE);
   const TCHAR *value = s_configCache.get(variable);
   RWLockUnlock(s_configCacheLock);
   if (value != NULL)
   {
      _tcslcpy(buffer, value, size);
		DbgPrintf(8, _T("ConfigReadStr: (cached) name=%s value=\"%s\""), variable, buffer);
      return true;
   }

   bool success = false;
   DB_HANDLE hdb = (dbHandle == NULL) ? DBConnectionPoolAcquireConnection() : dbHandle;
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT var_value FROM config WHERE var_name=?"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, variable, DB_BIND_STATIC);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
	   if (hResult != NULL)
		{
			if (DBGetNumRows(hResult) > 0)
			{
				DBGetField(hResult, 0, 0, buffer, size);
				DbgPrintf(8, _T("ConfigReadStr: name=%s value=\"%s\""), variable, buffer);
				success = true;
			}
		   DBFreeResult(hResult);
		}
		DBFreeStatement(hStmt);
	}
   if (dbHandle == NULL)
      DBConnectionPoolReleaseConnection(hdb);

   if (success)
   {
      RWLockWriteLock(s_configCacheLock, INFINITE);
      s_configCache.set(variable, buffer);
      RWLockUnlock(s_configCacheLock);
   }

   return success;
}

/**
 * Read string value from configuration table
 */
bool NXCORE_EXPORTABLE ConfigReadStr(const TCHAR *variable, TCHAR *buffer, size_t size, const TCHAR *defaultValue)
{
   return ConfigReadStrEx(NULL, variable, buffer, size, defaultValue);
}

/**
 * Read multibyte string from configuration table
 */
#ifdef UNICODE

bool NXCORE_EXPORTABLE ConfigReadStrA(const WCHAR *variable, char *buffer, size_t size, const char *defaultValue)
{
	WCHAR wcBuffer[MAX_DB_STRING];
   bool rc = ConfigReadStr(variable, wcBuffer, MAX_DB_STRING, NULL);
	if (rc)
	{
		WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, wcBuffer, -1, buffer, size, NULL, NULL);
	}
	else
	{
	   if (defaultValue != NULL)
	      strncpy(buffer, defaultValue, size);
	}
	buffer[size - 1] = 0;
	return rc;
}

#endif

/**
 * Read string value from configuration table as UTF8 string
 */
bool NXCORE_EXPORTABLE ConfigReadStrUTF8(const TCHAR *variable, char *buffer, size_t size, const char *defaultValue)
{
   if (defaultValue != NULL)
      strncpy(buffer, defaultValue, size);
   if (_tcslen(variable) > 127)
      return false;

   bool success = false;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT var_value FROM config WHERE var_name=?"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, variable, DB_BIND_STATIC);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
	   if (hResult != NULL)
		{
			if (DBGetNumRows(hResult) > 0)
			{
				DBGetFieldUTF8(hResult, 0, 0, buffer, size);
				success = true;
			}
		   DBFreeResult(hResult);
		}
		DBFreeStatement(hStmt);
	}
   DBConnectionPoolReleaseConnection(hdb);

   return success;
}

/**
 * Read boolean value from configuration table
 */
bool NXCORE_EXPORTABLE ConfigReadBoolean(const TCHAR *variable, bool defaultValue)
{
   TCHAR buffer[64];
   if (!ConfigReadStr(variable, buffer, 64, NULL))
      return defaultValue;
   if (!_tcsicmp(buffer, _T("true")))
      return true;
   return _tcstol(buffer, NULL, 0) != 0;
}

/**
 * Read integer value from configuration table
 */
int NXCORE_EXPORTABLE ConfigReadInt(const TCHAR *variable, int defaultValue)
{
   return ConfigReadIntEx(NULL, variable, defaultValue);
}

/**
 * Read integer value from configuration table
 */
int NXCORE_EXPORTABLE ConfigReadIntEx(DB_HANDLE hdb, const TCHAR *variable, int defaultValue)
{
   TCHAR buffer[64];
   if (ConfigReadStrEx(hdb, variable, buffer, 64, NULL))
      return _tcstol(buffer, NULL, 0);
   else
      return defaultValue;
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
bool NXCORE_EXPORTABLE ConfigReadByteArray(const TCHAR *variable, int *buffer, size_t size, int defaultElementValue)
{
   bool success;
   TCHAR text[256];
   if (ConfigReadStr(variable, text, 256, _T("")))
   {
      char pbBytes[128];
      StrToBin(text, (BYTE *)pbBytes, 128);
      size_t textLen = _tcslen(text) / 2;
      size_t i;
      for(i = 0; (i < size) && (i < textLen); i++)
         buffer[i] = pbBytes[i];
      for(; i < size; i++)
         buffer[i] = defaultElementValue;
      success = true;
   }
   else
   {
      for(size_t i = 0; i < size; i++)
         buffer[i] = defaultElementValue;
      success = false;
   }
   return success;
}

/**
 * Write string value to configuration table
 */
bool NXCORE_EXPORTABLE ConfigWriteStr(const TCHAR *variable, const TCHAR *value, bool create, bool isVisible, bool needRestart)
{
   if (_tcslen(variable) > 63)
      return false;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Check for variable existence
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT var_value FROM config WHERE var_name=?"));
	if (hStmt == NULL)
   {
      DBConnectionPoolReleaseConnection(hdb);
		return false;
   }
	DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, variable, DB_BIND_STATIC);
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
   if (!create && !bVarExist)
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
		DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, variable, DB_BIND_STATIC);
	}
   else
	{
		hStmt = DBPrepare(hdb, _T("INSERT INTO config (var_name,var_value,is_visible,need_server_restart) VALUES (?,?,?,?)"));
		if (hStmt == NULL)
      {
         DBConnectionPoolReleaseConnection(hdb);
			return false;
      }
		DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, variable, DB_BIND_STATIC);
		DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, value, DB_BIND_STATIC);
		DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (LONG)(isVisible ? 1 : 0));
		DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (LONG)(needRestart ? 1 : 0));
	}
   bool success = DBExecute(hStmt);
	DBFreeStatement(hStmt);
   DBConnectionPoolReleaseConnection(hdb);
	if (success)
		OnConfigVariableChange(false, variable, value);
	return success;
}

/**
 * Write integer value to configuration table
 */
bool NXCORE_EXPORTABLE ConfigWriteInt(const TCHAR *variable, int value, bool create, bool isVisible, bool needRestart)
{
   TCHAR szBuffer[64];

   _sntprintf(szBuffer, 64, _T("%d"), value);
   return ConfigWriteStr(variable, szBuffer, create, isVisible, needRestart);
}

/**
 * Write unsigned long value to configuration table
 */
bool NXCORE_EXPORTABLE ConfigWriteULong(const TCHAR *variable, UINT32 value, bool create, bool isVisible, bool needRestart)
{
   TCHAR szBuffer[64];

   _sntprintf(szBuffer, 64, _T("%u"), value);
   return ConfigWriteStr(variable, szBuffer, create, isVisible, needRestart);
}

/**
 * Write integer array to configuration table
 */
bool NXCORE_EXPORTABLE ConfigWriteByteArray(const TCHAR *variable, int *pnArray, size_t size, bool create, bool isVisible, bool needRestart)
{
   TCHAR szBuffer[256];
   for(size_t i = 0, j = 0; (i < size) && (i < 127); i++, j += 2)
      _sntprintf(&szBuffer[j], 256 - j, _T("%02X"), (char)((pnArray[i] > 127) ? 127 : ((pnArray[i] < -127) ? -127 : pnArray[i])));
   return ConfigWriteStr(variable, szBuffer, create, isVisible, needRestart);
}

/**
 * Delete configuratrion variable
 */
bool NXCORE_EXPORTABLE ConfigDelete(const TCHAR *variable)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   TCHAR query[1024];
   _sntprintf(query, 1024, _T("DELETE FROM config WHERE var_name=%s"), (const TCHAR *)DBPrepareString(hdb, variable));
   bool success = DBQuery(hdb, query);

   DBConnectionPoolReleaseConnection(hdb);

   if (success)
   {
      RWLockWriteLock(s_configCacheLock, INFINITE);
      s_configCache.remove(variable);
      RWLockUnlock(s_configCacheLock);
   }

   return success;
}

/**
 * Read large string (clob) value from configuration table
 */
TCHAR NXCORE_EXPORTABLE *ConfigReadCLOB(const TCHAR *variable, const TCHAR *defaultValue)
{
   TCHAR *result = NULL;

   if (_tcslen(variable) <= 63)
	{
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT var_value FROM config_clob WHERE var_name=?"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, variable, DB_BIND_STATIC);
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
	if ((result == NULL) && (defaultValue != NULL))
		result = _tcsdup(defaultValue);

	return result;
}

/**
 * Write large string (clob) value to configuration table
 */
bool NXCORE_EXPORTABLE ConfigWriteCLOB(const TCHAR *variable, const TCHAR *value, bool create)
{
   if (_tcslen(variable) > 63)
      return false;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Check for variable existence
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT var_value FROM config_clob WHERE var_name=?"));
	if (hStmt == NULL)
   {
      DBConnectionPoolReleaseConnection(hdb);
		return false;
   }
	DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, variable, DB_BIND_STATIC);
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
   if (!create && !bVarExist)
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
		DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, variable, DB_BIND_STATIC);
	}
   else
	{
		hStmt = DBPrepare(hdb, _T("INSERT INTO config_clob (var_name,var_value) VALUES (?,?)"));
		if (hStmt == NULL)
      {
         DBConnectionPoolReleaseConnection(hdb);
			return false;
      }
		DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, variable, DB_BIND_STATIC);
		DBBind(hStmt, 2, DB_SQLTYPE_TEXT, value, DB_BIND_STATIC);
	}
   bool success = DBExecute(hStmt) ? true : false;
	DBFreeStatement(hStmt);
   DBConnectionPoolReleaseConnection(hdb);
	if (success)
		OnConfigVariableChange(true, variable, value);
	return success;
}

/**
 * Enumeration callback data for GetClientConfigurationHints
 */
struct GetClientConfigurationHints_CallbackData
{
   UINT32 count;
   UINT32 fieldId;
   NXCPMessage *msg;
};

/**
 * Enumeration callback for GetClientConfigurationHints
 */
static EnumerationCallbackResult GetClientConfigurationHints_Callback(const TCHAR *key, const void *value, void *userArg)
{
   if (_tcsncmp(key, _T("Client."), 7))
      return _CONTINUE;

   GetClientConfigurationHints_CallbackData *data = static_cast<GetClientConfigurationHints_CallbackData*>(userArg);
   data->msg->setField(data->fieldId++, &key[7]);
   data->msg->setField(data->fieldId++, static_cast<const TCHAR*>(value));
   data->count++;
   return _CONTINUE;
}

/**
 * Get client configuration hints
 */
void GetClientConfigurationHints(NXCPMessage *msg)
{
   GetClientConfigurationHints_CallbackData data;
   data.count = 0;
   data.fieldId = VID_CONFIG_HINT_LIST_BASE;
   data.msg = msg;

   RWLockReadLock(s_configCacheLock, INFINITE);
   s_configCache.forEach(GetClientConfigurationHints_Callback, &data);
   RWLockUnlock(s_configCacheLock);

   msg->setField(VID_CONFIG_HINT_COUNT, data.count);
}
