/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
#include <nxvault.h>

/**
 * Externals
 */
extern char g_codePage[];
extern StringList g_moduleLoadList;
extern wchar_t *g_pdsLoadList;
extern InetAddressList g_peerNodeAddrList;
extern StringSet g_trustedCertificates;
extern StringSet g_crlList;
extern wchar_t g_serverCertificatePath[];
extern wchar_t g_serverCertificateKeyPath[];
extern char g_serverCertificatePassword[];
extern wchar_t g_internalCACertificatePath[];
extern wchar_t g_internalCACertificateKeyPath[];
extern char g_internalCACertificatePassword[];
extern char g_auditLogKey[];
extern int32_t g_maxClientSessions;
extern uint64_t g_maxClientMessageSize;
extern uint32_t g_clientFirstPacketTimeout;

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
static wchar_t *s_debugTags = nullptr;

/**
 * Peer node information
 */
static wchar_t s_peerNode[MAX_DB_STRING];

/**
 * Default thared stack size
 */
static uint64_t s_defaultThreadStackSize = 1024 * 1024;  // 1MB by default

/**
 * Certificate locations
 */
static wchar_t s_serverCertificatePath[MAX_PATH] = L"";
static wchar_t s_serverCertificateKeyPath[MAX_PATH] = L"";
static char s_serverCertificatePassword[MAX_PASSWORD] = "";
static wchar_t s_tunnelCertificatePath[MAX_PATH] = L"";
static wchar_t s_tunnelCertificateKeyPath[MAX_PATH] = L"";
static char s_tunnelCertificatePassword[MAX_PASSWORD] = "";
static wchar_t s_internalCACertificatePath[MAX_PATH] = L"";
static wchar_t s_internalCACertificateKeyPath[MAX_PATH] = L"";
static char s_internalCACertificatePassword[MAX_PASSWORD] = "";

/**
 * Database password command
 */
static wchar_t s_dbPasswordCommand[MAX_PATH] = L"";

/**
 * Vault configuration
 */
static char s_vaultURL[512] = "";
static char s_vaultAppRoleId[256] = "";
static char s_vaultAppRoleSecretId[256] = "";
static char s_vaultDBCredentialPath[256] = "";
static uint32_t s_vaultTimeout = 5000;
static bool s_vaultTLSVerify = true;

/**
 * Vault configuration template
 */
static NX_CFG_TEMPLATE m_vaultCfgTemplate[] =
{
   { L"AppRoleId", CT_MB_STRING, 0, 0, sizeof(s_vaultAppRoleId), 0, s_vaultAppRoleId, nullptr },
   { L"AppRoleSecretId", CT_MB_STRING, 0, 0, sizeof(s_vaultAppRoleSecretId), 0, s_vaultAppRoleSecretId, nullptr },
   { L"DBCredentialPath", CT_MB_STRING, 0, 0, sizeof(s_vaultDBCredentialPath), 0, s_vaultDBCredentialPath, nullptr },
   { L"TLSVerify", CT_BOOLEAN, 0, 0, 0, 0, &s_vaultTLSVerify, nullptr },
   { L"Timeout", CT_LONG, 0, 0, 0, 0, &s_vaultTimeout, nullptr },
   { L"URL", CT_MB_STRING, 0, 0, sizeof(s_vaultURL), 0, s_vaultURL, nullptr },
   { L"", CT_END_OF_LIST, 0, 0, 0, 0, nullptr, nullptr }
};

/**
 * Config file template
 */
static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
   { L"AuditLogKey", CT_MB_STRING, 0, 0, 128, 0, g_auditLogKey, nullptr },
   { L"BackgroundLogWriter", CT_BOOLEAN_FLAG_64, 0, 0, AF_BACKGROUND_LOG_WRITER, 0, (void*)&g_flags, nullptr },
   { L"CodePage", CT_MB_STRING, 0, 0, 256, 0, g_codePage, nullptr },
   { L"CreateCrashDumps", CT_BOOLEAN_FLAG_64, 0, 0, AF_CATCH_EXCEPTIONS, 0, (void*)&g_flags, nullptr },
   { L"CRL", CT_STRING_SET, 0, 0, 0, 0, &g_crlList, nullptr },
   { L"DailyLogFileSuffix", CT_STRING, 0, 0, 64, 0, g_szDailyLogFileSuffix, nullptr },
   { L"DataDirectory", CT_STRING, 0, 0, MAX_PATH, 0, g_netxmsdDataDir, nullptr },
   { L"DBCacheConfigurationTables", CT_BOOLEAN_FLAG_64, 0, 0, AF_CACHE_DB_ON_STARTUP, 0, (void*)&g_flags, nullptr },
   { L"DBDriver", CT_STRING, 0, 0, MAX_PATH, 0, g_szDbDriver, nullptr },
   { L"DBDriverOptions", CT_STRING, 0, 0, MAX_PATH, 0, g_szDbDrvParams, nullptr },
   { L"DBLogin", CT_STRING, 0, 0, MAX_DB_LOGIN, 0, g_szDbLogin, nullptr },
   { L"DBName", CT_STRING, 0, 0, MAX_DB_NAME, 0, g_szDbName, nullptr },
   { L"DBPassword", CT_STRING, 0, 0, MAX_PASSWORD, 0, g_szDbPassword, nullptr },
   { L"DBEncryptedPassword", CT_STRING, 0, 0, MAX_PASSWORD, 0, g_szDbPassword, nullptr },
   { L"DBPasswordCommand", CT_STRING, 0, 0, MAX_PATH, 0, s_dbPasswordCommand, nullptr },
   { L"DBSchema", CT_STRING, 0, 0, MAX_DB_NAME, 0, g_szDbSchema, nullptr },
   { L"DBServer", CT_STRING, 0, 0, MAX_PATH, 0, g_szDbServer, nullptr },
   { L"DBSessionSetupSQLScript", CT_STRING, 0, 0, MAX_PATH, 0, g_dbSessionSetupSqlScriptPath, nullptr },
   { L"DebugLevel", CT_LONG, 0, 0, 0, 0, &s_debugLevel, &s_debugLevel },
   { L"DebugTags", CT_STRING_CONCAT, ',', 0, 0, 0, &s_debugTags, nullptr },
   { L"DefaultThreadStackSize", CT_SIZE_BYTES, 0, 0, 0, 0, &s_defaultThreadStackSize, nullptr },
   { L"DumpDirectory", CT_STRING, 0, 0, MAX_PATH, 0, g_szDumpDir, nullptr },
   { L"FullCrashDumps", CT_BOOLEAN_FLAG_64, 0, 0, AF_WRITE_FULL_DUMP, 0, (void*)&g_flags, nullptr },
   { L"InternalCACertificate", CT_STRING, 0, 0, MAX_PATH, 0, s_internalCACertificatePath, nullptr },
   { L"InternalCACertificateKey", CT_STRING, 0, 0, MAX_PATH, 0, s_internalCACertificateKeyPath, nullptr },
   { L"InternalCACertificatePassword", CT_MB_STRING, 0, 0, MAX_PASSWORD, 0, s_internalCACertificatePassword, nullptr },
   { L"LibraryDirectory", CT_STRING, 0, 0, MAX_PATH, 0, g_netxmsdLibDir, nullptr },
   { L"ListenAddress", CT_STRING, 0, 0, MAX_PATH, 0, g_szListenAddress, nullptr },
   { L"LogFile", CT_STRING, 0, 0, MAX_PATH, 0, g_szLogFile, nullptr },
   { L"LogHistorySize", CT_LONG, 0, 0, 0, 0, &g_logHistorySize, nullptr },
   { L"LogRotationMode", CT_LONG, 0, 0, 0, 0, &g_logRotationMode, nullptr },
   { L"ManagementAgentAddress", CT_STRING, 0, 0, 128, 0, g_mgmtAgentAddress, nullptr },
   { L"MaxClientMessageSize", CT_SIZE_BYTES, 0, 0, 0, 0, &g_maxClientMessageSize, nullptr },
   { L"MaxClientSessions", CT_LONG, 0, 0, 0, 0, &g_maxClientSessions, nullptr },
   { L"MaxLogSize", CT_SIZE_BYTES, 0, 0, 0, 0, &g_maxLogSize, nullptr },
   { L"Module", CT_STRING_LIST, 0, 0, 0, 0, &g_moduleLoadList, nullptr },
   { L"PeerNode", CT_STRING, 0, 0, MAX_DB_STRING, 0, s_peerNode, nullptr },
   { L"PerfDataStorageDriver", CT_STRING_CONCAT, '\n', 0, 0, 0, &g_pdsLoadList, nullptr },
   { L"ProcessAffinityMask", CT_LONG, 0, 0, 0, 0, &g_processAffinityMask, nullptr },
   { L"ServerCertificate", CT_STRING, 0, 0, MAX_PATH, 0, s_serverCertificatePath, nullptr },
   { L"ServerCertificateKey", CT_STRING, 0, 0, MAX_PATH, 0, s_serverCertificateKeyPath, nullptr },
   { L"ServerCertificatePassword", CT_MB_STRING, 0, 0, MAX_PASSWORD, 0, s_serverCertificatePassword, nullptr },
   { L"StartupSQLScript", CT_STRING, 0, 0, MAX_PATH, 0, g_startupSqlScriptPath, nullptr },
   { L"TrustedCertificate", CT_STRING_SET, 0, 0, 0, 0, &g_trustedCertificates, nullptr },
   { L"TunnelCertificate", CT_STRING, 0, 0, MAX_PATH, 0, s_tunnelCertificatePath, nullptr },
   { L"TunnelCertificateKey", CT_STRING, 0, 0, MAX_PATH, 0, s_tunnelCertificateKeyPath, nullptr },
   { L"TunnelCertificatePassword", CT_MB_STRING, 0, 0, MAX_PASSWORD, 0, s_tunnelCertificatePassword, nullptr },
   { L"WriteLogAsJson", CT_BOOLEAN_FLAG_64, 0, 0, AF_LOG_IN_JSON_FORMAT, 0, (void*)&g_flags, nullptr },
   /* deprecated parameters */
   { L"DBDrvParams", CT_STRING, 0, 0, MAX_PATH, 0, g_szDbDrvParams, nullptr }, 
   { L"ServerCACertificate", CT_STRING_SET, 0, 0, 0, 0, &g_trustedCertificates, nullptr },
   { L"", CT_END_OF_LIST, 0, 0, 0, 0, nullptr, nullptr }
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
   if (wcscmp(g_szConfigFile, L"{search}"))
      return;

#ifdef _WIN32
   TCHAR path[MAX_PATH];
   GetNetXMSDirectory(nxDirEtc, path);
   wcscat(path, L"\\netxmsd.conf");
   if (_taccess(path, 4) == 0)
   {
      wcscpy(g_szConfigFile, path);
   }
   else
   {
      wcscpy(g_szConfigFile, L"C:\\netxmsd.conf");
   }
#else
   String homeDir = GetEnvironmentVariableEx(L"NETXMS_HOME");
   if (!homeDir.isEmpty())
   {
      wchar_t config[MAX_PATH];
      _sntprintf(config, MAX_PATH, L"%s/etc/netxmsd.conf", homeDir.cstr());
      if (_waccess(config, 4) == 0)
      {
         _tcscpy(g_szConfigFile, config);
         goto stop_search;
      }
   }
   if (_waccess(SYSCONFDIR L"/netxmsd.conf", 4) == 0)
   {
      wcscpy(g_szConfigFile, SYSCONFDIR L"/netxmsd.conf");
   }
   else
   {
      wcscpy(g_szConfigFile, L"/etc/netxmsd.conf");
   }
stop_search:
   ;
#endif
}

/**
 * Retrieve database credentials from Vault
 */
void RetrieveDatabaseCredentialsFromVault()
{
   // Parse vault configuration section
   g_serverConfig.parseTemplate(L"VAULT", m_vaultCfgTemplate);

   VaultDatabaseCredentialConfig config;
   config.url = s_vaultURL;
   config.appRoleId = s_vaultAppRoleId;
   config.appRoleSecretId = s_vaultAppRoleSecretId;
   config.dbCredentialPath = s_vaultDBCredentialPath;
   config.timeout = s_vaultTimeout;
   config.tlsVerify = s_vaultTLSVerify;

   RetrieveDatabaseCredentialsFromVault(&config, g_szDbLogin, MAX_DB_LOGIN, g_szDbPassword, MAX_PASSWORD, L"config", false);
}

/**
 * Execute database password command if configured
 */
void ExecuteDatabasePasswordCommand()
{
   if (s_dbPasswordCommand[0] == 0)
      return;

   nxlog_write_tag(NXLOG_INFO, L"config", L"Executing database password command: %s", s_dbPasswordCommand);

   OutputCapturingProcessExecutor executor(s_dbPasswordCommand, true);
   if (!executor.execute())
   {
      nxlog_write_tag(NXLOG_ERROR, L"config", L"Failed to execute database password command: %s", s_dbPasswordCommand);
      return;
   }

   if (!executor.waitForCompletion(30000))  // 30 second timeout
   {
      executor.stop();
      nxlog_write_tag(NXLOG_ERROR, L"config", L"Database password command timed out");
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
         MultiByteToWideCharSysLocale(output, g_szDbPassword, len);
         g_szDbPassword[len] = 0;

         nxlog_write_tag(NXLOG_INFO, L"config", L"Database password successfully retrieved from command");
      }
      else
      {
         nxlog_write_tag(NXLOG_WARNING, L"config", L"Database password command returned empty output");
      }
   }
   else
   {
      nxlog_write_tag(NXLOG_ERROR, L"config", L"Database password command failed with exit code %d", executor.getExitCode());
   }
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
      WriteToTerminalEx(L"Using configuration file \"%s\"\n", g_szConfigFile);

	if (g_serverConfig.loadConfig(g_szConfigFile, L"server") && g_serverConfig.parseTemplate(L"server", m_cfgTemplate))
   {
      if ((!_tcsicmp(g_szLogFile, L"{EventLog}")) ||
          (!_tcsicmp(g_szLogFile, L"{syslog}")))
      {
         g_flags |= AF_USE_SYSLOG;
         g_flags &= ~(AF_USE_SYSTEMD_JOURNAL | AF_LOG_TO_STDOUT);
      }
      else if (!_tcsicmp(g_szLogFile, L"{systemd}"))
      {
         g_flags |= AF_USE_SYSTEMD_JOURNAL;
         g_flags &= ~(AF_USE_SYSLOG | AF_LOG_TO_STDOUT);
      }
      else if (!_tcsicmp(g_szLogFile, L"{stdout}"))
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
      wchar_t **tagList = SplitString(s_debugTags, L',', &count);
      if (tagList != nullptr)
      {
         for(int i = 0; i < count; i++)
         {
            wchar_t *level = wcschr(tagList[i], L':');
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
   DecryptPasswordW(g_szDbLogin, g_szDbPassword, g_szDbPassword, MAX_PASSWORD);
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
   unique_ptr<ObjectArray<ConfigEntry>> environment = g_serverConfig.getSubEntries(L"/ENV", L"*");
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
   DB_RESULT hResult = DBSelect(hdb, L"SELECT var_name,var_value FROM metadata");
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
bool NXCORE_EXPORTABLE MetaDataReadStr(const wchar_t *name, wchar_t *buffer, int bufSize, const wchar_t *defaultValue)
{
   bool success = false;

   wcslcpy(buffer, defaultValue, bufSize);
   if (wcslen(name) > 127)
      return false;

   s_metadataCacheLock.readLock();
   const wchar_t *value = s_metadataCache.get(name);
   if (value != nullptr)
   {
      wcslcpy(buffer, value, bufSize);
      success = true;
   }
   s_metadataCacheLock.unlock();

   if (!success && !s_metadataCacheLoaded)
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      DB_STATEMENT hStmt = DBPrepare(hdb, L"SELECT var_value FROM metadata WHERE var_name=?");
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
               s_metadataCache.setPreallocated(MemCopyStringW(name), DBGetField(hResult, 0, 0, nullptr, 0));
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
int32_t NXCORE_EXPORTABLE MetaDataReadInt32(const wchar_t *variable, int32_t defaultValue)
{
   wchar_t buffer[256];
   if (MetaDataReadStr(variable, buffer, 256, L""))
   {
      wchar_t *eptr;
      int32_t value = wcstol(buffer, &eptr, 0);
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
bool NXCORE_EXPORTABLE MetaDataWriteStr(const wchar_t *variable, const wchar_t *value)
{
   if (_tcslen(variable) > 63)
      return false;

   s_metadataCacheLock.writeLock();
   s_metadataCache.set(variable, value);
   s_metadataCacheLock.unlock();

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Check for variable existence
	DB_STATEMENT hStmt = DBPrepare(hdb, L"SELECT var_value FROM metadata WHERE var_name=?");
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
		hStmt = DBPrepare(hdb, L"UPDATE metadata SET var_value=? WHERE var_name=?");
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
		hStmt = DBPrepare(hdb, L"INSERT INTO metadata (var_name,var_value) VALUES (?,?)");
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
bool NXCORE_EXPORTABLE MetaDataWriteInt32(const wchar_t *variable, int32_t value)
{
   wchar_t buffer[32];
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
   DB_RESULT hResult = DBSelect(hdb, L"SELECT var_name,var_value FROM config");
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
static inline uint32_t ConvertToUint32(const wchar_t *value, uint32_t defaultValue)
{
   wchar_t *eptr;
   uint32_t i = wcstoul(value, &eptr, 0);
   return (*eptr == 0) ? i : defaultValue;
}

/**
 * Convert given text to uint32_t or use default value on conversion failure
 */
static inline int32_t ConvertToInt32(const wchar_t *value, int32_t defaultValue)
{
   wchar_t *eptr;
   int32_t i = wcstoul(value, &eptr, 0);
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
	if (isCLOB && !wcscmp(name, L"SyslogParser"))
	{
		ReinitializeSyslogParser();
	}
   if (isCLOB && !wcscmp(name, L"WindowsEventParser"))
   {
      InitializeWindowsEventParser();
   }
   else if (!wcscmp(name, L"Agent.RestartWaitTime"))
   {
      g_agentRestartWaitTime = ConvertToUint32(value, 0);
   }
   else if (!wcscmp(name, L"Alarms.ResolveExpirationTime"))
   {
      UpdateAlarmExpirationTimes();
   }
   else if (!wcscmp(name, L"Alarms.StrictStatusFlow"))
   {
      NotifyClientSessions(NX_NOTIFY_ALARM_STATUS_FLOW_CHANGED, _tcstol(value, nullptr, 0));
   }
   else if (!wcscmp(name, L"Alarms.SummaryEmail.Enable"))
   {
      if (wcstol(value, nullptr, 0))
         EnableAlarmSummaryEmails();
      else
         DeleteScheduledTaskByHandlerId(ALARM_SUMMARY_EMAIL_TASK_ID);
   }
   else if (!wcscmp(name, L"Alarms.SummaryEmail.Schedule"))
   {
      if (ConfigReadBoolean(L"Alarms.SummaryEmail.Enable", false))
         EnableAlarmSummaryEmails();  // this call will update schedule for existing task
   }
   else if (!_tcsncmp(name, L"CAS.", 4))
   {
      CASReadSettings();
   }
   else if (!wcscmp(name, L"Client.FirstPacketTimeout"))
   {
      g_clientFirstPacketTimeout = ConvertToUint32(value, 2000);
      if (g_clientFirstPacketTimeout < 100)
         g_clientFirstPacketTimeout = 100;
      nxlog_debug_tag(L"client.session", 2, L"Client first packet timeout set to %u milliseconds", g_clientFirstPacketTimeout);
   }
   else if (!wcscmp(name, L"DataCollection.DefaultDCIPollingInterval"))
   {
      DCObject::m_defaultPollingInterval = ConvertToInt32(value, 60);
   }
   else if (!wcscmp(name, L"DataCollection.DefaultDCIRetentionTime"))
   {
      DCObject::m_defaultRetentionTime = ConvertToInt32(value, 30);
   }
   else if (!wcscmp(name, L"DataCollection.InstancePollingInterval"))
   {
      g_instancePollingInterval = ConvertToUint32(value, 600);
   }
   else if (!wcscmp(name, L"DataCollection.InstanceRetentionTime"))
   {
      g_instanceRetentionTime = ConvertToUint32(value, 7);
   }
   else if (!wcscmp(name, L"DataCollection.Scheduler.RequireConnectivity"))
   {
      UpdateServerFlag(AF_DC_SCHEDULER_REQUIRES_CONNECTIVITY, value);
   }
   else if (!wcscmp(name, L"DBWriter.HouseKeeperInterlock"))
   {
      switch(wcstol(value, nullptr, 0))
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
      nxlog_write_tag(NXLOG_INFO, L"db.writer", L"DBWriter/Housekeeper interlock is %s", (g_flags & AF_DBWRITER_HK_INTERLOCK) ? L"ON" : L"OFF");
   }
   else if (!wcscmp(name, L"DBWriter.MaxQueueSize"))
   {
      OnDBWriterMaxQueueSizeChange();
   }
   else if (!wcscmp(name, L"ICMP.CollectPollStatistics"))
   {
      UpdateServerFlag(AF_COLLECT_ICMP_STATISTICS, value);
   }
   else if (!wcscmp(name, L"ICMP.PingSize"))
   {
      uint32_t size = ConvertToUint32(value, 46);
      if (size < MIN_PING_SIZE)
         g_icmpPingSize = MIN_PING_SIZE;
      else if (size > MAX_PING_SIZE)
         g_icmpPingSize = MAX_PING_SIZE;
      else
         g_icmpPingSize = size;
   }
   else if (!wcscmp(name, L"ICMP.PingTimeout"))
   {
      g_icmpPingTimeout = ConvertToUint32(value, 1500);
   }
   else if (!wcscmp(name, L"ICMP.PollingInterval"))
   {
      g_icmpPollingInterval = ConvertToUint32(value, 60);
   }
   else if (!wcscmp(name, L"ICMP.StatisticPeriod"))
   {
      uint32_t period = ConvertToUint32(value, 60);
      nxlog_debug_tag(L"poll.icmp", 3, L"Updating ICMP statistic period for configured collectors (new period is %u polls)", period);
      g_idxNodeById.forEach(
         [period] (NetObj *object) -> EnumerationCallbackResult
         {
            static_cast<Node*>(object)->updateIcmpStatisticPeriod(period);
            return _CONTINUE;
         });
   }
   else if (!wcscmp(name, L"NetworkDiscovery.ActiveDiscovery.Interval") ||
            !wcscmp(name, L"NetworkDiscovery.ActiveDiscovery.Schedule"))
   {
      WakeupActiveDiscoveryThread();
   }
   else if (!wcscmp(name, L"NetworkDiscovery.DisableProtocolProbe.Agent"))
   {
      UpdateServerFlag(AF_DISABLE_AGENT_PROBE, value);
   }
   else if (!wcscmp(name, L"NetworkDiscovery.DisableProtocolProbe.EtherNetIP"))
   {
      UpdateServerFlag(AF_DISABLE_ETHERNETIP_PROBE, value);
   }
   else if (!wcscmp(name, L"NetworkDiscovery.DisableProtocolProbe.SNMP.V1"))
   {
      UpdateServerFlag(AF_DISABLE_SNMP_V1_PROBE, value);
   }
   else if (!wcscmp(name, L"NetworkDiscovery.DisableProtocolProbe.SNMP.V2"))
   {
      UpdateServerFlag(AF_DISABLE_SNMP_V2_PROBE, value);
   }
   else if (!wcscmp(name, L"NetworkDiscovery.DisableProtocolProbe.SNMP.V3"))
   {
      UpdateServerFlag(AF_DISABLE_SNMP_V3_PROBE, value);
   }
   else if (!wcscmp(name, L"NetworkDiscovery.DisableProtocolProbe.SSH"))
   {
      UpdateServerFlag(AF_DISABLE_SSH_PROBE, value);
   }
   else if (!wcscmp(name, L"NetworkDiscovery.EnableParallelProcessing"))
   {
      UpdateServerFlag(AF_PARALLEL_NETWORK_DISCOVERY, value);
   }
   else if (!wcscmp(name, L"NetworkDiscovery.MergeDuplicateNodes"))
   {
      UpdateServerFlag(AF_MERGE_DUPLICATE_NODES, value);
   }
   else if (!wcscmp(name, L"NetworkDiscovery.PassiveDiscovery.Interval"))
   {
      g_discoveryPollingInterval = ConvertToUint32(value, 900);
   }
   else if (!wcscmp(name, L"NXSL.EnableContainerFunctions"))
   {
      UpdateServerFlag(AF_ENABLE_NXSL_CONTAINER_FUNCTIONS, value);
   }
   else if (!wcscmp(name, L"Objects.AutobindOnConfigurationPoll"))
   {
      UpdateServerFlag(AF_AUTOBIND_ON_CONF_POLL, value);
   }
   else if (!wcscmp(name, L"Objects.AutobindPollingInterval"))
   {
      g_autobindPollingInterval = ConvertToUint32(value, 3600);
   }
   else if (!wcscmp(name, L"Objects.Conditions.PollingInterval"))
   {
      g_conditionPollingInterval = ConvertToUint32(value, 60);
   }
   else if (!wcscmp(name, L"Objects.ConfigurationPollingInterval"))
   {
      g_configurationPollingInterval = ConvertToUint32(value, 3600);
   }
   else if (!wcscmp(name, L"Objects.Interfaces.Enable8021xStatusPoll"))
   {
      UpdateServerFlag(AF_ENABLE_8021X_STATUS_POLL, value);
   }
   else if (!wcscmp(name, L"Objects.NetworkMaps.UpdateInterval"))
   {
      g_mapUpdatePollingInterval = ConvertToUint32(value, 60);
   }
   else if (!wcscmp(name, L"Objects.Nodes.ResolveDNSToIPOnStatusPoll"))
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
   else if (!wcscmp(name, L"Objects.Nodes.ResolveDNSToIPOnStatusPoll.Interval"))
   {
      g_pollsBetweenPrimaryIpUpdate = ConvertToUint32(value, 1);
   }
   else if (!wcscmp(name, L"Objects.Nodes.ResolveNames"))
   {
      UpdateServerFlag(AF_RESOLVE_NODE_NAMES, value);
   }
   else if (!wcscmp(name, L"Objects.Nodes.SyncNamesWithDNS"))
   {
      UpdateServerFlag(AF_SYNC_NODE_NAMES_WITH_DNS, value);
   }
   else if (!wcscmp(name, L"Objects.Security.CheckTrustedObjects"))
   {
      UpdateServerFlag(AF_CHECK_TRUSTED_OBJECTS, value);
   }
   else if (!wcscmp(name, L"Objects.StatusPollingInterval"))
   {
      g_statusPollingInterval = ConvertToUint32(value, 60);
   }
   else if (!wcscmp(name, L"Objects.Subnets.DeleteEmpty"))
   {
      UpdateServerFlag(AF_DELETE_EMPTY_SUBNETS, value);
   }
   else if (!_tcsncmp(name, L"SNMP.Agent.", 11))
   {
      OnSNMPAgentConfigurationChange(name, value);
   }
   else if (!wcscmp(name, L"SNMP.Codepage"))
   {
      tchar_to_utf8(value, -1, g_snmpCodepage, sizeof(g_snmpCodepage));
   }
   else if (!wcscmp(name, L"SNMP.Traps.AllowVarbindsConversion"))
   {
      UpdateServerFlag(AF_ALLOW_TRAP_VARBIND_CONVERSION, value);
   }
   else if (!wcscmp(name, L"SNMP.Traps.LogAll"))
   {
      UpdateServerFlag(AF_LOG_ALL_SNMP_TRAPS, value);
   }
   else if (!wcscmp(name, L"SNMP.Traps.ProcessUnmanagedNodes"))
   {
      UpdateServerFlag(AF_TRAPS_FROM_UNMANAGED_NODES, value);
   }
   else if (!wcscmp(name, L"SNMP.Traps.RateLimit.Threshold"))
   {
      g_snmpTrapStormCountThreshold = ConvertToUint32(value, 0);
   }
   else if (!wcscmp(name, L"SNMP.Traps.RateLimit.Duration"))
   {
      g_snmpTrapStormDurationThreshold = ConvertToUint32(value, 15);
   }
   else if (!wcscmp(name, L"SNMP.Traps.UnmatchedTrapEvent"))
   {
      UpdateServerFlag(AF_ENABLE_UNMATCHED_TRAP_EVENT, value);
   }
   else if (!_tcsncmp(name, L"Syslog.", 7))
   {
      OnSyslogConfigurationChange(name, value);
   }
   else if (!wcscmp(name, L"Topology.PollingInterval"))
   {
      g_topologyPollingInterval = ConvertToUint32(value, 1800);
   }
   else if (!wcscmp(name, L"Topology.RoutingTable.UpdateInterval"))
   {
      g_routingTableUpdateInterval = ConvertToUint32(value, 300);
   }
   else if (!_tcsncmp(name, L"WindowsEvents.", 14))
   {
      OnWindowsEventsConfigurationChange(name, value);
   }
}

/**
 * Read string value from configuration table
 */
bool NXCORE_EXPORTABLE ConfigReadStrEx(DB_HANDLE dbHandle, const wchar_t *variable, wchar_t *buffer, size_t size, const wchar_t *defaultValue)
{
   if (defaultValue != nullptr)
      wcslcpy(buffer, defaultValue, size);
   if (wcslen(variable) > 127)
      return false;

   s_configCacheLock.readLock();
   const TCHAR *value = s_configCache.get(variable);
   s_configCacheLock.unlock();
   if (value != nullptr)
   {
      _tcslcpy(buffer, value, size);
		nxlog_debug_tag(L"config", 8, L"ConfigReadStr: (cached) name=%s value=\"%s\"", variable, buffer);
      return true;
   }

   if (s_configCacheLoaded)
      return false;

   if (g_flags & AF_SHUTDOWN) // Database may not be available when shutdown is initiated
      return false;

   bool success = false;
   DB_HANDLE hdb = (dbHandle == nullptr) ? DBConnectionPoolAcquireConnection() : dbHandle;
	DB_STATEMENT hStmt = DBPrepare(hdb, L"SELECT var_value FROM config WHERE var_name=?");
	if (hStmt != nullptr)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, variable, DB_BIND_STATIC);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
	   if (hResult != nullptr)
		{
			if (DBGetNumRows(hResult) > 0)
			{
				DBGetField(hResult, 0, 0, buffer, size);
				nxlog_debug_tag(L"config", 8, L"ConfigReadStr: name=%s value=\"%s\"", variable, buffer);
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
bool NXCORE_EXPORTABLE ConfigReadStr(const wchar_t *variable, wchar_t *buffer, size_t size, const wchar_t *defaultValue)
{
   return ConfigReadStrEx(nullptr, variable, buffer, size, defaultValue);
}

/**
 * Read string value from configuration table. Returns dynamically allocated string.
 */
wchar_t NXCORE_EXPORTABLE *ConfigReadStr(const wchar_t *variable, const wchar_t *defaultValue)
{
   wchar_t buffer[MAX_CONFIG_VALUE_LENGTH];
   bool success = ConfigReadStrEx(nullptr, variable, buffer, MAX_CONFIG_VALUE_LENGTH, nullptr);
   return success ? MemCopyStringW(buffer) : MemCopyStringW(defaultValue);
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
	DB_STATEMENT hStmt = DBPrepare(hdb, L"SELECT var_value FROM config WHERE var_name=?");
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
   char buffer[MAX_CONFIG_VALUE_LENGTH * 3];
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
   if (!wcsicmp(buffer, L"true"))
      return true;
   return wcstol(buffer, nullptr, 0) != 0;
}

/**
 * Read integer value from configuration table
 */
int32_t NXCORE_EXPORTABLE ConfigReadInt(const wchar_t *variable, int32_t defaultValue)
{
   return ConfigReadIntEx(nullptr, variable, defaultValue);
}

/**
 * Read integer value from configuration table
 */
int32_t NXCORE_EXPORTABLE ConfigReadIntEx(DB_HANDLE hdb, const wchar_t *variable, int32_t defaultValue)
{
   wchar_t buffer[64];
   if (ConfigReadStrEx(hdb, variable, buffer, 64, nullptr))
      return wcstol(buffer, nullptr, 0);
   return defaultValue;
}

/**
 * Read unsigned long value from configuration table
 */
uint32_t NXCORE_EXPORTABLE ConfigReadULong(const wchar_t *variable, uint32_t defaultValue)
{
   wchar_t buffer[64];
   if (ConfigReadStr(variable, buffer, 64, L""))
      return wcstoul(buffer, nullptr, 0);
   return defaultValue;
}

/**
 * Read signed long long value from configuration table
 */
int64_t NXCORE_EXPORTABLE ConfigReadInt64(const wchar_t *variable, int64_t defaultValue)
{
   wchar_t buffer[64];
   if (ConfigReadStr(variable, buffer, 64, L""))
      return wcstoll(buffer, nullptr, 10);
   return defaultValue;
}

/**
 * Read unsigned long long value from configuration table
 */
uint64_t NXCORE_EXPORTABLE ConfigReadUInt64(const wchar_t *variable, uint64_t defaultValue)
{
   wchar_t buffer[64];
   if (ConfigReadStr(variable, buffer, 64, L""))
      return wcstoull(buffer, nullptr, 10);
   return defaultValue;
}

/**
 * Read byte array (in hex form) from configuration table into integer array
 */
bool NXCORE_EXPORTABLE ConfigReadByteArray(const wchar_t *variable, int *buffer, size_t size, int defaultElementValue)
{
   bool success;
   wchar_t text[256];
   if (ConfigReadStr(variable, text, 256, L""))
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
	DB_STATEMENT hStmt = DBPrepare(hdb, L"SELECT var_value FROM config WHERE var_name=?");
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
		hStmt = DBPrepare(hdb, L"UPDATE config SET var_value=? WHERE var_name=?");
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
		hStmt = DBPrepare(hdb, L"INSERT INTO config (var_name,var_value,is_visible,need_server_restart) VALUES (?,?,?,?)");
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
   wchar_t szBuffer[256];
   for(size_t i = 0, j = 0; (i < size) && (i < 127); i++, j += 2)
      _sntprintf(&szBuffer[j], 256 - j, L"%02X", (char)((pnArray[i] > 127) ? 127 : ((pnArray[i] < -127) ? -127 : pnArray[i])));
   return ConfigWriteStr(variable, szBuffer, create, isVisible, needRestart);
}

/**
 * Delete configuratrion variable
 */
bool NXCORE_EXPORTABLE ConfigDelete(const wchar_t *variable)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   bool success = ExecuteQueryOnObject(hdb, variable, L"DELETE FROM config WHERE var_name=?");
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
   DB_STATEMENT hStmt = DBPrepare(hdb, L"SELECT var_value FROM config_clob WHERE var_name=?");
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
	DB_STATEMENT hStmt = DBPrepare(hdb, L"SELECT var_value FROM config_clob WHERE var_name=?");
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
		hStmt = DBPrepare(hdb, L"UPDATE config_clob SET var_value=? WHERE var_name=?");
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
		hStmt = DBPrepare(hdb, L"INSERT INTO config_clob (var_name,var_value) VALUES (?,?)");
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
   if (wcsncmp(key, L"Client.", 7))
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
