/*
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
#include <nxnet.h>
#include <nms_users.h>

/**
 * Externals
 */
extern char g_codePage[];
extern StringList g_moduleLoadList;
extern wchar_t *g_pdsLoadList;
extern InetAddressList g_peerNodeAddrList;
extern StringSet g_trustedCertificates;
extern StringSet g_crlList;
extern TCHAR g_serverCertificatePath[];
extern TCHAR g_serverCertificateKeyPath[];
extern char g_serverCertificatePassword[];
extern TCHAR g_internalCACertificatePath[];
extern TCHAR g_internalCACertificateKeyPath[];
extern char g_internalCACertificatePassword[];
extern char g_auditLogKey[];
extern int32_t g_maxClientSessions;
extern uint64_t g_maxClientMessageSize;
extern uint32_t g_clientFirstPacketTimeout;

TCHAR s_serverCertificatePath[MAX_PATH] = _T("");
TCHAR s_serverCertificateKeyPath[MAX_PATH] = _T("");
char s_serverCertificatePassword[MAX_PASSWORD] = "";
TCHAR s_tunnelCertificatePath[MAX_PATH] = _T("");
TCHAR s_tunnelCertificateKeyPath[MAX_PATH] = _T("");
char s_tunnelCertificatePassword[MAX_PASSWORD] = "";
TCHAR s_internalCACertificatePath[MAX_PATH] = _T("");
TCHAR s_internalCACertificateKeyPath[MAX_PATH] = _T("");
char s_internalCACertificatePassword[MAX_PASSWORD] = "";

void UpdateAlarmExpirationTimes();
void WakeupActiveDiscoveryThread();

void OnSNMPAgentConfigurationChange(const wchar_t *name, const wchar_t *value);

/**
 * Database connection parameters
 */
wchar_t g_szDbDriver[MAX_PATH] = L"";
wchar_t g_szDbDrvParams[MAX_PATH] = L"";
wchar_t g_szDbServer[MAX_PATH] = L"127.0.0.1";
wchar_t g_szDbLogin[MAX_DB_LOGIN] = L"netxms";
wchar_t g_szDbPassword[MAX_PASSWORD] = L"";
wchar_t g_szDbName[MAX_DB_NAME] = L"netxms_db";
wchar_t g_szDbSchema[MAX_DB_NAME] = L"";

/**
 * Debug level from config
 */
static uint32_t s_debugLevel = (uint32_t)NXCONFIG_UNINITIALIZED_VALUE;

/**
 * Debug tags from config
 */
static TCHAR *s_debugTags = nullptr;

/**
 * Peer node information
 */
static TCHAR s_peerNode[MAX_DB_STRING];

/**
 * Default thared stack size
 */
static uint64_t s_defaultThreadStackSize = 1024 * 1024;  // 1MB by default

/**
 * Config file template
 */
static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
   { _T("AuditLogKey"), CT_MB_STRING, 0, 0, 128, 0, g_auditLogKey, nullptr },
   { _T("BackgroundLogWriter"), CT_BOOLEAN_FLAG_64, 0, 0, AF_BACKGROUND_LOG_WRITER, 0, (void*)&g_flags, nullptr },
   { _T("CodePage"), CT_MB_STRING, 0, 0, 256, 0, g_codePage, nullptr },
   { _T("CreateCrashDumps"), CT_BOOLEAN_FLAG_64, 0, 0, AF_CATCH_EXCEPTIONS, 0, (void*)&g_flags, nullptr },
   { _T("CRL"), CT_STRING_SET, 0, 0, 0, 0, &g_crlList, nullptr },
   { _T("DailyLogFileSuffix"), CT_STRING, 0, 0, 64, 0, g_szDailyLogFileSuffix, nullptr },
   { _T("DataDirectory"), CT_STRING, 0, 0, MAX_PATH, 0, g_netxmsdDataDir, nullptr },
   { _T("DBCacheConfigurationTables"), CT_BOOLEAN_FLAG_64, 0, 0, AF_CACHE_DB_ON_STARTUP, 0, (void*)&g_flags, nullptr },
   { _T("DBDriver"), CT_STRING, 0, 0, MAX_PATH, 0, g_szDbDriver, nullptr },
   { _T("DBDriverOptions"), CT_STRING, 0, 0, MAX_PATH, 0, g_szDbDrvParams, nullptr },
   { _T("DBLogin"), CT_STRING, 0, 0, MAX_DB_LOGIN, 0, g_szDbLogin, nullptr },
   { _T("DBName"), CT_STRING, 0, 0, MAX_DB_NAME, 0, g_szDbName, nullptr },
   { _T("DBPassword"), CT_STRING, 0, 0, MAX_PASSWORD, 0, g_szDbPassword, nullptr },
   { _T("DBEncryptedPassword"), CT_STRING, 0, 0, MAX_PASSWORD, 0, g_szDbPassword, nullptr },
   { _T("DBSchema"), CT_STRING, 0, 0, MAX_DB_NAME, 0, g_szDbSchema, nullptr },
   { _T("DBServer"), CT_STRING, 0, 0, MAX_PATH, 0, g_szDbServer, nullptr },
   { _T("DBSessionSetupSQLScript"), CT_STRING, 0, 0, MAX_PATH, 0, g_dbSessionSetupSqlScriptPath, nullptr },
   { _T("DebugLevel"), CT_LONG, 0, 0, 0, 0, &s_debugLevel, &s_debugLevel },
   { _T("DebugTags"), CT_STRING_CONCAT, ',', 0, 0, 0, &s_debugTags, nullptr },
   { _T("DefaultThreadStackSize"), CT_SIZE_BYTES, 0, 0, 0, 0, &s_defaultThreadStackSize, nullptr },
   { _T("DumpDirectory"), CT_STRING, 0, 0, MAX_PATH, 0, g_szDumpDir, nullptr },
   { _T("FullCrashDumps"), CT_BOOLEAN_FLAG_64, 0, 0, AF_WRITE_FULL_DUMP, 0, (void*)&g_flags, nullptr },
   { _T("InternalCACertificate"), CT_STRING, 0, 0, MAX_PATH, 0, s_internalCACertificatePath, nullptr },
   { _T("InternalCACertificateKey"), CT_STRING, 0, 0, MAX_PATH, 0, s_internalCACertificateKeyPath, nullptr },
   { _T("InternalCACertificatePassword"), CT_MB_STRING, 0, 0, MAX_PASSWORD, 0, s_internalCACertificatePassword, nullptr },
   { _T("LibraryDirectory"), CT_STRING, 0, 0, MAX_PATH, 0, g_netxmsdLibDir, nullptr },
   { _T("ListenAddress"), CT_STRING, 0, 0, MAX_PATH, 0, g_szListenAddress, nullptr },
   { _T("LogFile"), CT_STRING, 0, 0, MAX_PATH, 0, g_szLogFile, nullptr },
   { _T("LogHistorySize"), CT_LONG, 0, 0, 0, 0, &g_logHistorySize, nullptr },
   { _T("LogRotationMode"), CT_LONG, 0, 0, 0, 0, &g_logRotationMode, nullptr },
   { _T("MaxClientMessageSize"), CT_SIZE_BYTES, 0, 0, 0, 0, &g_maxClientMessageSize, nullptr },
   { _T("MaxClientSessions"), CT_LONG, 0, 0, 0, 0, &g_maxClientSessions, nullptr },
   { _T("MaxLogSize"), CT_SIZE_BYTES, 0, 0, 0, 0, &g_maxLogSize, nullptr },
   { _T("Module"), CT_STRING_LIST, 0, 0, 0, 0, &g_moduleLoadList, nullptr },
   { _T("PeerNode"), CT_STRING, 0, 0, MAX_DB_STRING, 0, s_peerNode, nullptr },
   { _T("PerfDataStorageDriver"), CT_STRING_CONCAT, '\n', 0, 0, 0, &g_pdsLoadList, nullptr },
   { _T("ProcessAffinityMask"), CT_LONG, 0, 0, 0, 0, &g_processAffinityMask, nullptr },
   { _T("ServerCertificate"), CT_STRING, 0, 0, MAX_PATH, 0, s_serverCertificatePath, nullptr },
   { _T("ServerCertificateKey"), CT_STRING, 0, 0, MAX_PATH, 0, s_serverCertificateKeyPath, nullptr },
   { _T("ServerCertificatePassword"), CT_MB_STRING, 0, 0, MAX_PASSWORD, 0, s_serverCertificatePassword, nullptr },
   { _T("StartupSQLScript"), CT_STRING, 0, 0, MAX_PATH, 0, g_startupSqlScriptPath, nullptr },
   { _T("TrustedCertificate"), CT_STRING_SET, 0, 0, 0, 0, &g_trustedCertificates, nullptr },
   { _T("TunnelCertificate"), CT_STRING, 0, 0, MAX_PATH, 0, s_tunnelCertificatePath, nullptr },
   { _T("TunnelCertificateKey"), CT_STRING, 0, 0, MAX_PATH, 0, s_tunnelCertificateKeyPath, nullptr },
   { _T("TunnelCertificatePassword"), CT_MB_STRING, 0, 0, MAX_PASSWORD, 0, s_tunnelCertificatePassword, nullptr },
   { _T("WriteLogAsJson"), CT_BOOLEAN_FLAG_64, 0, 0, AF_LOG_IN_JSON_FORMAT, 0, (void*)&g_flags, nullptr },
   /* deprecated parameters */
   { _T("DBDrvParams"), CT_STRING, 0, 0, MAX_PATH, 0, g_szDbDrvParams, nullptr }, 
   { _T("ServerCACertificate"), CT_STRING_SET, 0, 0, 0, 0, &g_trustedCertificates, nullptr },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr, nullptr }
};

/**
 * Loaded server config
 */
Config g_serverConfig;

/**
 * Find configuration file if set to "{search}"
 */
void NXCORE_EXPORTABLE FindConfigFile()
{
   if (_tcscmp(g_szConfigFile, _T("{search}")))
      return;

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
   String homeDir = GetEnvironmentVariableEx(_T("NETXMS_HOME"));
   if (!homeDir.isEmpty())
   {
      TCHAR config[MAX_PATH];
      _sntprintf(config, MAX_PATH, _T("%s/etc/netxmsd.conf"), homeDir.cstr());
      if (_taccess(config, 4) == 0)
      {
         _tcscpy(g_szConfigFile, config);
         goto stop_search;
      }
   }
   if (_taccess(SYSCONFDIR _T("/netxmsd.conf"), 4) == 0)
   {
      _tcscpy(g_szConfigFile, SYSCONFDIR _T("/netxmsd.conf"));
   }
   else
   {
      _tcscpy(g_szConfigFile, _T("/etc/netxmsd.conf"));
   }
stop_search:
   ;
#endif
}

/**
 * Load and parse configuration file
 * Returns TRUE on success and FALSE on failure
 */
bool NXCORE_EXPORTABLE LoadConfig(int *debugLevel)
{
   bool bSuccess = false;

   FindConfigFile();
   if (IsStandalone())
      WriteToTerminalEx(_T("Using configuration file \"%s\"\n"), g_szConfigFile);

	if (g_serverConfig.loadConfig(g_szConfigFile, _T("server")) && g_serverConfig.parseTemplate(_T("server"), m_cfgTemplate))
   {
      if ((!_tcsicmp(g_szLogFile, _T("{EventLog}"))) ||
          (!_tcsicmp(g_szLogFile, _T("{syslog}"))))
      {
         g_flags |= AF_USE_SYSLOG;
         g_flags &= ~(AF_USE_SYSTEMD_JOURNAL | AF_LOG_TO_STDOUT);
      }
      else if (!_tcsicmp(g_szLogFile, _T("{systemd}")))
      {
         g_flags |= AF_USE_SYSTEMD_JOURNAL;
         g_flags &= ~(AF_USE_SYSLOG | AF_LOG_TO_STDOUT);
      }
      else if (!_tcsicmp(g_szLogFile, _T("{stdout}")))
      {
         g_flags |= AF_LOG_TO_STDOUT;
         g_flags &= ~(AF_USE_SYSLOG | AF_USE_SYSTEMD_JOURNAL);
      }
      else
      {
         g_flags &= ~(AF_USE_SYSLOG | AF_USE_SYSTEMD_JOURNAL | AF_LOG_TO_STDOUT);
      }

      if (s_internalCACertificatePath[0] != 0)
      {
         _tcsncpy(g_internalCACertificatePath, s_internalCACertificatePath, MAX_PATH);
         _tcsncpy(g_internalCACertificateKeyPath, s_internalCACertificateKeyPath, MAX_PATH);
         strlcpy(g_internalCACertificatePassword, s_internalCACertificatePassword, MAX_PASSWORD);
      }
      else if (s_serverCertificatePath[0] != 0)
      {
         _tcsncpy(g_internalCACertificatePath, s_serverCertificatePath, MAX_PATH);
         _tcsncpy(g_internalCACertificateKeyPath, s_serverCertificateKeyPath, MAX_PATH);
         strlcpy(g_internalCACertificatePassword, s_serverCertificatePassword, MAX_PASSWORD);
      }

      if (s_tunnelCertificatePath[0] != 0)
      {
         _tcsncpy(g_serverCertificatePath, s_tunnelCertificatePath, MAX_PATH);
         _tcsncpy(g_serverCertificateKeyPath, s_tunnelCertificateKeyPath, MAX_PATH);
         strlcpy(g_serverCertificatePassword, s_tunnelCertificatePassword, MAX_PASSWORD);
      }
      else if (s_serverCertificatePath[0] != 0)
      {
         _tcsncpy(g_serverCertificatePath, s_serverCertificatePath, MAX_PATH);
         _tcsncpy(g_serverCertificateKeyPath, s_serverCertificateKeyPath, MAX_PATH);
         strlcpy(g_serverCertificatePassword, s_serverCertificatePassword, MAX_PASSWORD);
      }

      bSuccess = true;
   }

   if (*debugLevel == NXCONFIG_UNINITIALIZED_VALUE)
      *debugLevel = (int)s_debugLevel;

   if (s_debugTags != nullptr)
   {
      int count;
      TCHAR **tagList = SplitString(s_debugTags, _T(','), &count);
      if (tagList != nullptr)
      {
         for(int i = 0; i < count; i++)
         {
            TCHAR *level = _tcschr(tagList[i], _T(':'));
            if (level != nullptr)
            {
               *level = 0;
               level++;
               Trim(tagList[i]);
               nxlog_set_debug_level_tag(tagList[i], _tcstol(level, nullptr, 0));
            }
            MemFree(tagList[i]);
         }
         MemFree(tagList);
      }
      MemFree(s_debugTags);
   }

	// Decrypt password
   DecryptPassword(g_szDbLogin, g_szDbPassword, g_szDbPassword, MAX_PASSWORD);
   DecryptPasswordA("netxms", g_auditLogKey, g_auditLogKey, 128);

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
         MemFree(list[i]);
      }
      MemFree(list);
   }

   // Update environment from config
   unique_ptr<ObjectArray<ConfigEntry>> environment = g_serverConfig.getSubEntries(_T("/ENV"), _T("*"));
   if (environment != nullptr)
   {
      for (int i = 0; i < environment->size(); i++)
      {
         ConfigEntry *e = environment->get(i);
         SetEnvironmentVariable(e->getName(), e->getValue());
      }
   }

   if (bSuccess)
      ThreadSetDefaultStackSize(static_cast<int>(s_defaultThreadStackSize));

   return bSuccess;
}

/**
 * Metadata cache
 */
static StringMap s_metadataCache;
static RWLock s_metadataCacheLock;
static bool s_metadataCacheLoaded = false;

/**
 * Pre-load metadata
 */
void MetaDataPreLoad()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT var_name,var_value FROM metadata"));
   if (hResult != nullptr)
   {
      s_metadataCacheLock.writeLock();
      s_metadataCache.clear();
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         s_metadataCache.setPreallocated(DBGetField(hResult, i, 0, nullptr, 0), DBGetField(hResult, i, 1, nullptr, 0));
      }
      s_metadataCacheLoaded = true;
      s_metadataCacheLock.unlock();
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Read string value from metadata table
 */
bool NXCORE_EXPORTABLE MetaDataReadStr(const TCHAR *name, TCHAR *buffer, int bufSize, const TCHAR *defaultValue)
{
   bool success = false;

   _tcslcpy(buffer, defaultValue, bufSize);
   if (_tcslen(name) > 127)
      return false;

   s_metadataCacheLock.readLock();
   const TCHAR *value = s_metadataCache.get(name);
   if (value != nullptr)
   {
      _tcslcpy(buffer, value, bufSize);
      success = true;
   }
   s_metadataCacheLock.unlock();

   if (!success && !s_metadataCacheLoaded)
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT var_value FROM metadata WHERE var_name=?"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
         DB_RESULT hResult = DBSelectPrepared(hStmt);
         if (hResult != nullptr)
         {
            if (DBGetNumRows(hResult) > 0)
            {
               DBGetField(hResult, 0, 0, buffer, bufSize);
               s_metadataCacheLock.writeLock();
               s_metadataCache.setPreallocated(_tcsdup(name), DBGetField(hResult, 0, 0, nullptr, 0));
               s_metadataCacheLock.unlock();
               success = true;
            }
            DBFreeResult(hResult);
         }
         DBFreeStatement(hStmt);
      }
      DBConnectionPoolReleaseConnection(hdb);
   }
   return success;
}

/**
 * Read integer value from metadata table
 */
int32_t NXCORE_EXPORTABLE MetaDataReadInt32(const TCHAR *variable, int32_t defaultValue)
{
   TCHAR buffer[256];
   if (MetaDataReadStr(variable, buffer, 256, _T("")))
   {
      TCHAR *eptr;
      int32_t value = _tcstol(buffer, &eptr, 0);
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

   s_metadataCacheLock.writeLock();
   s_metadataCache.set(variable, value);
   s_metadataCacheLock.unlock();

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Check for variable existence
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT var_value FROM metadata WHERE var_name=?"));
	if (hStmt == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
		return false;
   }
	DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, variable, DB_BIND_STATIC);
	DB_RESULT hResult = DBSelectPrepared(hStmt);
   bool bVarExist = false;
   if (hResult != nullptr)
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
		if (hStmt == nullptr)
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
		if (hStmt == nullptr)
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
bool NXCORE_EXPORTABLE MetaDataWriteInt32(const TCHAR *variable, int32_t value)
{
   TCHAR buffer[32];
   return MetaDataWriteStr(variable, IntegerToString(value, buffer));
}

/**
 * Config cache
 */
static StringMap s_configCache;
static RWLock s_configCacheLock;
static bool s_configCacheLoaded = false;

/**
 * Pre-load configuration
 */
void ConfigPreLoad()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT var_name,var_value FROM config"));
   if (hResult != nullptr)
   {
      s_configCacheLock.writeLock();
      s_configCache.clear();
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         s_configCache.setPreallocated(DBGetField(hResult, i, 0, nullptr, 0), DBGetField(hResult, i, 1, nullptr, 0));
      }
      s_configCacheLoaded = true;
      s_configCacheLock.unlock();
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Convert given text to uint32_t or use default value on conversion failure
 */
static inline uint32_t ConvertToUint32(const TCHAR *value, uint32_t defaultValue)
{
   TCHAR *eptr;
   uint32_t i = _tcstoul(value, &eptr, 0);
   return (*eptr == 0) ? i : defaultValue;
}

/**
 * Update server flag
 */
static inline void UpdateServerFlag(uint64_t flag, const TCHAR *value)
{
   if (_tcstol(value, nullptr, 0))
      InterlockedOr64(&g_flags, flag);
   else
      InterlockedAnd64(&g_flags, ~flag);
}

/**
 * Callback for configuration variables change
 */
static void OnConfigVariableChange(bool isCLOB, const TCHAR *name, const TCHAR *value)
{
   s_configCacheLock.writeLock();
   s_configCache.set(name, value);
   s_configCacheLock.unlock();

	// Restart syslog parser if configuration was changed
	if (isCLOB && !_tcscmp(name, _T("SyslogParser")))
	{
		ReinitializeSyslogParser();
	}
   if (isCLOB && !_tcscmp(name, _T("WindowsEventParser")))
   {
      InitializeWindowsEventParser();
   }
   else if (!_tcscmp(name, _T("Agent.RestartWaitTime")))
   {
      g_agentRestartWaitTime = _tcstol(value, nullptr, 0);
   }
   else if (!_tcscmp(name, _T("Alarms.ResolveExpirationTime")))
   {
      UpdateAlarmExpirationTimes();
   }
   else if (!_tcscmp(name, _T("Alarms.SummaryEmail.Enable")))
   {
      if (_tcstol(value, nullptr, 0))
         EnableAlarmSummaryEmails();
      else
         DeleteScheduledTaskByHandlerId(ALARM_SUMMARY_EMAIL_TASK_ID);
   }
   else if (!_tcscmp(name, _T("Alarms.SummaryEmail.Schedule")))
   {
      if (ConfigReadBoolean(_T("Alarms.SummaryEmail.Enable"), false))
         EnableAlarmSummaryEmails();  // this call will update schedule for existing task
   }
   else if (!_tcsncmp(name, _T("CAS."), 4))
   {
      CASReadSettings();
   }
   else if (!_tcscmp(name, _T("Client.FirstPacketTimeout")))
   {
      g_clientFirstPacketTimeout = ConvertToUint32(value, 2000);
      if (g_clientFirstPacketTimeout < 100)
         g_clientFirstPacketTimeout = 100;
      nxlog_debug_tag(_T("client.session"), 2, _T("Client first packet timeout set to %u milliseconds"), g_clientFirstPacketTimeout);
   }
   else if (!_tcscmp(name, _T("DataCollection.InstanceRetentionTime")))
   {
      g_instanceRetentionTime = _tcstol(value, nullptr, 0);
   }
   else if (!_tcscmp(name, _T("DBWriter.HouseKeeperInterlock")))
   {
      switch(_tcstol(value, nullptr, 0))
      {
         case 0:  // Auto
            if (g_dbSyntax == DB_SYNTAX_MSSQL)
               InterlockedOr64(&g_flags, AF_DBWRITER_HK_INTERLOCK);
            else
               InterlockedAnd64(&g_flags, ~AF_DBWRITER_HK_INTERLOCK);
            break;
         case 1:  // Off
            InterlockedAnd64(&g_flags, ~AF_DBWRITER_HK_INTERLOCK);
            break;
         case 2:  // On
            InterlockedOr64(&g_flags, AF_DBWRITER_HK_INTERLOCK);
            break;
         default:
            break;
      }
      nxlog_write_tag(NXLOG_INFO, _T("db.writer"), _T("DBWriter/Housekeeper interlock is %s"), (g_flags & AF_DBWRITER_HK_INTERLOCK) ? _T("ON") : _T("OFF"));
   }
   else if (!_tcscmp(name, _T("DBWriter.MaxQueueSize")))
   {
      OnDBWriterMaxQueueSizeChange();
   }
   else if (!_tcscmp(name, _T("DataCollection.DefaultDCIPollingInterval")))
   {
      DCObject::m_defaultPollingInterval = _tcstol(value, nullptr, 0);
   }
   else if (!_tcscmp(name, _T("DataCollection.DefaultDCIRetentionTime")))
   {
      DCObject::m_defaultRetentionTime = _tcstol(value, nullptr, 0);
   }
   else if (!_tcscmp(name, _T("ICMP.CollectPollStatistics")))
   {
      UpdateServerFlag(AF_COLLECT_ICMP_STATISTICS, value);
   }
   else if (!_tcscmp(name, _T("ICMP.PingSize")))
   {
      uint32_t size = ConvertToUint32(value, 46);
      if (size < MIN_PING_SIZE)
         g_icmpPingSize = MIN_PING_SIZE;
      else if (size > MAX_PING_SIZE)
         g_icmpPingSize = MAX_PING_SIZE;
      else
         g_icmpPingSize = size;
   }
   else if (!_tcscmp(name, _T("ICMP.PingTimeout")))
   {
      g_icmpPingTimeout = ConvertToUint32(value, 1500);
   }
   else if (!_tcscmp(name, _T("ICMP.PollingInterval")))
   {
      g_icmpPollingInterval = ConvertToUint32(value, 60);
   }
   else if (!_tcscmp(name, _T("ICMP.StatisticPeriod")))
   {
      uint32_t period = ConvertToUint32(value, 60);
      nxlog_debug_tag(_T("poll.icmp"), 3, _T("Updating ICMP statistic period for configured collectors (new period is %u polls)"), period);
      g_idxNodeById.forEach(
         [period] (NetObj *object) -> EnumerationCallbackResult
         {
            static_cast<Node*>(object)->updateIcmpStatisticPeriod(period);
            return _CONTINUE;
         });
   }
   else if (!_tcscmp(name, _T("NetworkDiscovery.ActiveDiscovery.Interval")) ||
            !_tcscmp(name, _T("NetworkDiscovery.ActiveDiscovery.Schedule")))
   {
      WakeupActiveDiscoveryThread();
   }
   else if (!_tcscmp(name, _T("NetworkDiscovery.DisableProtocolProbe.Agent")))
   {
      UpdateServerFlag(AF_DISABLE_AGENT_PROBE, value);
   }
   else if (!_tcscmp(name, _T("NetworkDiscovery.DisableProtocolProbe.EtherNetIP")))
   {
      UpdateServerFlag(AF_DISABLE_ETHERNETIP_PROBE, value);
   }
   else if (!_tcscmp(name, _T("NetworkDiscovery.DisableProtocolProbe.SNMP.V1")))
   {
      UpdateServerFlag(AF_DISABLE_SNMP_V1_PROBE, value);
   }
   else if (!_tcscmp(name, _T("NetworkDiscovery.DisableProtocolProbe.SNMP.V2")))
   {
      UpdateServerFlag(AF_DISABLE_SNMP_V2_PROBE, value);
   }
   else if (!_tcscmp(name, _T("NetworkDiscovery.DisableProtocolProbe.SNMP.V3")))
   {
      UpdateServerFlag(AF_DISABLE_SNMP_V3_PROBE, value);
   }
   else if (!_tcscmp(name, _T("NetworkDiscovery.DisableProtocolProbe.SSH")))
   {
      UpdateServerFlag(AF_DISABLE_SSH_PROBE, value);
   }
   else if (!_tcscmp(name, _T("NetworkDiscovery.EnableParallelProcessing")))
   {
      UpdateServerFlag(AF_PARALLEL_NETWORK_DISCOVERY, value);
   }
   else if (!_tcscmp(name, _T("NetworkDiscovery.MergeDuplicateNodes")))
   {
      UpdateServerFlag(AF_MERGE_DUPLICATE_NODES, value);
   }
   else if (!_tcscmp(name, _T("NetworkDiscovery.PassiveDiscovery.Interval")))
   {
      g_discoveryPollingInterval = ConvertToUint32(value, 900);
   }
   else if (!_tcscmp(name, _T("NXSL.EnableContainerFunctions")))
   {
      UpdateServerFlag(AF_ENABLE_NXSL_CONTAINER_FUNCTIONS, value);
   }
   else if (!_tcscmp(name, _T("Objects.AutobindOnConfigurationPoll")))
   {
      UpdateServerFlag(AF_AUTOBIND_ON_CONF_POLL, value);
   }
   else if (!_tcscmp(name, _T("Objects.AutobindPollingInterval")))
   {
      g_autobindPollingInterval = ConvertToUint32(value, 3600);
   }
   else if (!_tcscmp(name, _T("Objects.ConfigurationPollingInterval")))
   {
      g_configurationPollingInterval = ConvertToUint32(value, 3600);
   }
   else if (!_tcscmp(name, _T("Objects.Interfaces.Enable8021xStatusPoll")))
   {
      UpdateServerFlag(AF_ENABLE_8021X_STATUS_POLL, value);
   }
   else if (!_tcscmp(name, _T("Objects.NetworkMaps.UpdateInterval")))
   {
      g_mapUpdatePollingInterval = ConvertToUint32(value, 60);
   }
   else if (!_tcscmp(name, _T("Objects.Nodes.ResolveDNSToIPOnStatusPoll")))
   {
      switch(ConvertToUint32(value, static_cast<int>(PrimaryIPUpdateMode::NEVER)))
      {
         case static_cast<int>(PrimaryIPUpdateMode::ALWAYS):
            g_primaryIpUpdateMode = PrimaryIPUpdateMode::ALWAYS;
            break;
         case static_cast<int>(PrimaryIPUpdateMode::ON_FAILURE):
            g_primaryIpUpdateMode = PrimaryIPUpdateMode::ON_FAILURE;
            break;
         default:
            g_primaryIpUpdateMode = PrimaryIPUpdateMode::NEVER;
            break;
      }
   }
   else if (!_tcscmp(name, _T("Objects.Nodes.ResolveDNSToIPOnStatusPoll.Interval")))
   {
      g_pollsBetweenPrimaryIpUpdate = ConvertToUint32(value, 1);
   }
   else if (!_tcscmp(name, _T("Objects.Nodes.ResolveNames")))
   {
      UpdateServerFlag(AF_RESOLVE_NODE_NAMES, value);
   }
   else if (!_tcscmp(name, _T("Objects.Security.CheckTrustedObjects")))
   {
      UpdateServerFlag(AF_CHECK_TRUSTED_OBJECTS, value);
   }
   else if (!_tcscmp(name, _T("Objects.StatusPollingInterval")))
   {
      g_statusPollingInterval = ConvertToUint32(value, 60);
   }
   else if (!_tcscmp(name, _T("Objects.Subnets.DeleteEmpty")))
   {
      UpdateServerFlag(AF_DELETE_EMPTY_SUBNETS, value);
   }
   else if (!_tcsncmp(name, _T("SNMP.Agent."), 11))
   {
      OnSNMPAgentConfigurationChange(name, value);
   }
   else if (!_tcscmp(name, _T("SNMP.Codepage")))
   {
      tchar_to_utf8(value, -1, g_snmpCodepage, sizeof(g_snmpCodepage));
   }
   else if (!_tcscmp(name, _T("SNMP.Traps.AllowVarbindsConversion")))
   {
      UpdateServerFlag(AF_ALLOW_TRAP_VARBIND_CONVERSION, value);
   }
   else if (!_tcscmp(name, _T("SNMP.Traps.LogAll")))
   {
      UpdateServerFlag(AF_LOG_ALL_SNMP_TRAPS, value);
   }
   else if (!_tcscmp(name, _T("SNMP.Traps.ProcessUnmanagedNodes")))
   {
      UpdateServerFlag(AF_TRAPS_FROM_UNMANAGED_NODES, value);
   }
   else if (!_tcscmp(name, _T("SNMP.Traps.RateLimit.Threshold")))
   {
      g_snmpTrapStormCountThreshold = ConvertToUint32(value, 0);
   }
   else if (!_tcscmp(name, _T("SNMP.Traps.RateLimit.Duration")))
   {
      g_snmpTrapStormDurationThreshold = ConvertToUint32(value, 15);
   }
   else if (!_tcscmp(name, _T("SNMP.Traps.UnmatchedTrapEvent")))
   {
      UpdateServerFlag(AF_ENABLE_UNMATCHED_TRAP_EVENT, value);
   }
   else if (!_tcscmp(name, _T("Alarms.StrictStatusFlow")))
   {
      NotifyClientSessions(NX_NOTIFY_ALARM_STATUS_FLOW_CHANGED, _tcstol(value, nullptr, 0));
   }
   else if (!_tcsncmp(name, _T("Syslog."), 7))
   {
      OnSyslogConfigurationChange(name, value);
   }
   else if (!_tcsncmp(name, _T("WindowsEvents."), 14))
   {
      OnWindowsEventsConfigurationChange(name, value);
   }
}

/**
 * Read string value from configuration table
 */
bool NXCORE_EXPORTABLE ConfigReadStrEx(DB_HANDLE dbHandle, const TCHAR *variable, TCHAR *buffer, size_t size, const TCHAR *defaultValue)
{
   if (defaultValue != nullptr)
      _tcslcpy(buffer, defaultValue, size);
   if (_tcslen(variable) > 127)
      return false;

   s_configCacheLock.readLock();
   const TCHAR *value = s_configCache.get(variable);
   s_configCacheLock.unlock();
   if (value != nullptr)
   {
      _tcslcpy(buffer, value, size);
		nxlog_debug_tag(_T("config"), 8, _T("ConfigReadStr: (cached) name=%s value=\"%s\""), variable, buffer);
      return true;
   }

   if (s_configCacheLoaded)
      return false;

   if (g_flags & AF_SHUTDOWN) // Database may not be available when shutdown is initiated
      return false;

   bool success = false;
   DB_HANDLE hdb = (dbHandle == nullptr) ? DBConnectionPoolAcquireConnection() : dbHandle;
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT var_value FROM config WHERE var_name=?"));
	if (hStmt != nullptr)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, variable, DB_BIND_STATIC);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
	   if (hResult != nullptr)
		{
			if (DBGetNumRows(hResult) > 0)
			{
				DBGetField(hResult, 0, 0, buffer, size);
				nxlog_debug_tag(_T("config"), 8, _T("ConfigReadStr: name=%s value=\"%s\""), variable, buffer);
				success = true;
			}
		   DBFreeResult(hResult);
		}
		DBFreeStatement(hStmt);
	}
   if (dbHandle == nullptr)
      DBConnectionPoolReleaseConnection(hdb);

   if (success)
   {
      s_configCacheLock.writeLock();
      s_configCache.set(variable, buffer);
      s_configCacheLock.unlock();
   }

   return success;
}

/**
 * Read string value from configuration table
 */
bool NXCORE_EXPORTABLE ConfigReadStr(const TCHAR *variable, TCHAR *buffer, size_t size, const TCHAR *defaultValue)
{
   return ConfigReadStrEx(nullptr, variable, buffer, size, defaultValue);
}

/**
 * Read string value from configuration table. Returns dynamically allocated string.
 */
TCHAR NXCORE_EXPORTABLE *ConfigReadStr(const TCHAR *variable, const TCHAR *defaultValue)
{
   TCHAR buffer[MAX_CONFIG_VALUE];
   bool success = ConfigReadStrEx(nullptr, variable, buffer, MAX_CONFIG_VALUE, nullptr);
   return success ? MemCopyString(buffer) : MemCopyString(defaultValue);
}

/**
 * Read multibyte string from configuration table
 */
bool NXCORE_EXPORTABLE ConfigReadStrA(const wchar_t *variable, char *buffer, size_t size, const char *defaultValue)
{
   wchar_t wcBuffer[MAX_DB_STRING];
   bool rc = ConfigReadStr(variable, wcBuffer, MAX_DB_STRING, nullptr);
	if (rc)
	{
		wchar_to_mb(wcBuffer, -1, buffer, size);
	}
	else
	{
	   if (defaultValue != nullptr)
	      strlcpy(buffer, defaultValue, size);
	}
	buffer[size - 1] = 0;
	return rc;
}

/**
 * Read string value from configuration table as UTF8 string
 */
bool NXCORE_EXPORTABLE ConfigReadStrUTF8(const wchar_t *variable, char *buffer, size_t size, const char *defaultValue)
{
   if (defaultValue != nullptr)
      strlcpy(buffer, defaultValue, size);
   if (_tcslen(variable) > 127)
      return false;

   s_configCacheLock.readLock();
   const TCHAR *value = s_configCache.get(variable);
   s_configCacheLock.unlock();
   if (value != nullptr)
   {
      tchar_to_utf8(value, -1, buffer, size);
      return true;
   }

   if (s_configCacheLoaded)
      return false;

   bool success = false;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT var_value FROM config WHERE var_name=?"));
	if (hStmt != nullptr)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, variable, DB_BIND_STATIC);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
	   if (hResult != nullptr)
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
 * Read string value from configuration table as UTF8 string. Returns dynamically allocated string.
 */
char NXCORE_EXPORTABLE *ConfigReadStrUTF8(const wchar_t *variable, const char *defaultValue)
{
   char buffer[MAX_CONFIG_VALUE * 3];
   bool success = ConfigReadStrUTF8(variable, buffer, sizeof(buffer), nullptr);
   return success ? MemCopyStringA(buffer) : MemCopyStringA(defaultValue);
}

/**
 * Read boolean value from configuration table
 */
bool NXCORE_EXPORTABLE ConfigReadBoolean(const wchar_t *variable, bool defaultValue)
{
   wchar_t buffer[64];
   if (!ConfigReadStr(variable, buffer, 64, nullptr))
      return defaultValue;
   if (!_tcsicmp(buffer, _T("true")))
      return true;
   return _tcstol(buffer, nullptr, 0) != 0;
}

/**
 * Read integer value from configuration table
 */
int32_t NXCORE_EXPORTABLE ConfigReadInt(const TCHAR *variable, int32_t defaultValue)
{
   return ConfigReadIntEx(nullptr, variable, defaultValue);
}

/**
 * Read integer value from configuration table
 */
int32_t NXCORE_EXPORTABLE ConfigReadIntEx(DB_HANDLE hdb, const TCHAR *variable, int32_t defaultValue)
{
   TCHAR buffer[64];
   if (ConfigReadStrEx(hdb, variable, buffer, 64, nullptr))
      return _tcstol(buffer, nullptr, 0);
   return defaultValue;
}

/**
 * Read unsigned long value from configuration table
 */
uint32_t NXCORE_EXPORTABLE ConfigReadULong(const TCHAR *szVar, uint32_t defaultValue)
{
   TCHAR buffer[64];
   if (ConfigReadStr(szVar, buffer, 64, _T("")))
      return _tcstoul(buffer, nullptr, 0);
   return defaultValue;
}

/**
 * Read signed long long value from configuration table
 */
int64_t NXCORE_EXPORTABLE ConfigReadInt64(const TCHAR *variable, int64_t defaultValue)
{
   TCHAR buffer[64];
   if (ConfigReadStr(variable, buffer, 64, _T("")))
      return _tcstoll(buffer, nullptr, 10);
   return defaultValue;
}

/**
 * Read unsigned long long value from configuration table
 */
uint64_t NXCORE_EXPORTABLE ConfigReadUInt64(const TCHAR *variable, uint64_t defaultValue)
{
   TCHAR buffer[64];
   if (ConfigReadStr(variable, buffer, 64, _T("")))
      return _tcstoull(buffer, nullptr, 10);
   return defaultValue;
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
	if (hStmt == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
		return false;
   }
	DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, variable, DB_BIND_STATIC);
	DB_RESULT hResult = DBSelectPrepared(hStmt);
   bool bVarExist = false;
   if (hResult != nullptr)
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
		if (hStmt == nullptr)
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
		if (hStmt == nullptr)
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
bool NXCORE_EXPORTABLE ConfigWriteInt(const TCHAR *variable, int32_t value, bool create, bool isVisible, bool needRestart)
{
   TCHAR buffer[64];
   return ConfigWriteStr(variable, IntegerToString(value, buffer), create, isVisible, needRestart);
}

/**
 * Write unsigned long value to configuration table
 */
bool NXCORE_EXPORTABLE ConfigWriteULong(const TCHAR *variable, uint32_t value, bool create, bool isVisible, bool needRestart)
{
   TCHAR buffer[64];
   return ConfigWriteStr(variable, IntegerToString(value, buffer), create, isVisible, needRestart);
}

/**
 * Write signed long long value to configuration table
 */
bool NXCORE_EXPORTABLE ConfigWriteInt64(const TCHAR *variable, int64_t value, bool create, bool isVisible, bool needRestart)
{
   TCHAR buffer[64];
   return ConfigWriteStr(variable, IntegerToString(value, buffer), create, isVisible, needRestart);
}

/**
 * Write unsigned long long value to configuration table
 */
bool NXCORE_EXPORTABLE ConfigWriteUInt64(const TCHAR *variable, uint64_t value, bool create, bool isVisible, bool needRestart)
{
   TCHAR buffer[64];
   return ConfigWriteStr(variable, IntegerToString(value, buffer), create, isVisible, needRestart);
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
      s_configCacheLock.writeLock();
      s_configCache.remove(variable);
      s_configCacheLock.unlock();
   }

   return success;
}

/**
 * Read large string (clob) value from configuration table
 */
TCHAR NXCORE_EXPORTABLE *ConfigReadCLOB(const TCHAR *variable, const TCHAR *defaultValue)
{
   TCHAR *result = nullptr;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT var_value FROM config_clob WHERE var_name=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, variable, DB_BIND_STATIC);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            result = DBGetField(hResult, 0, 0, nullptr, 0);
         }
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
	return (result != nullptr) ? result : MemCopyString(defaultValue);
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
	if (hStmt == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
		return false;
   }
	DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, variable, DB_BIND_STATIC);
	DB_RESULT hResult = DBSelectPrepared(hStmt);
   bool bVarExist = false;
   if (hResult != nullptr)
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
		if (hStmt == nullptr)
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
		if (hStmt == nullptr)
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
   uint32_t count;
   uint32_t fieldId;
   NXCPMessage *msg;
};

/**
 * Enumeration callback for GetClientConfigurationHints
 */
static EnumerationCallbackResult GetClientConfigurationHints_Callback(const TCHAR *key, const TCHAR *value, GetClientConfigurationHints_CallbackData *context)
{
   if (_tcsncmp(key, _T("Client."), 7))
      return _CONTINUE;

   context->msg->setField(context->fieldId++, &key[7]);
   context->msg->setField(context->fieldId++, value);
   context->count++;
   return _CONTINUE;
}

/**
 * Get client configuration hints
 */
void GetClientConfigurationHints(NXCPMessage *msg, uint32_t userId)
{
   GetClientConfigurationHints_CallbackData data;
   data.count = 0;
   data.fieldId = VID_CONFIG_HINT_LIST_BASE;
   data.msg = msg;

   s_configCacheLock.readLock();
   s_configCache.forEach(GetClientConfigurationHints_Callback, &data);
   s_configCacheLock.unlock();

   EnumerateUserDbObjectAttributes(userId, GetClientConfigurationHints_Callback, &data);

   msg->setField(VID_CONFIG_HINT_COUNT, data.count);
}
