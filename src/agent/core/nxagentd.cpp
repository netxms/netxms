/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2025 Raden Solutions
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
** File: nxagentd.cpp
**
**/

#include "nxagentd.h"
#include <nxstat.h>
#include <netxms_getopt.h>
#include <netxms-version.h>

#ifdef _WIN32
#include <conio.h>
#include <client/windows/handler/exception_handler.h>
#else
#include <signal.h>
#include <sys/wait.h>
#endif

#if HAVE_LOCALE_H
#include <locale.h>
#endif

#if HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

#if HAVE_SYS_SYSCTL_H && HAVE_SYSCTLBYNAME
#include <sys/sysctl.h>
#endif

#ifdef _WITH_ENCRYPTION
#include <openssl/ssl.h>
#endif

#if HAVE_GRP_H
#include <grp.h>
#endif

#if HAVE_PWD_H
#include <pwd.h>
#endif

NETXMS_EXECUTABLE_HEADER(nxagentd)

/**
 * Startup flags
 */
#define SF_LOG_UNRESOLVED_SYMBOLS   0x00000001
#define SF_REGISTER                 0x00000002
#define SF_ENABLE_WATCHDOG          0x00000004
#define SF_ENABLE_AUTOLOAD          0x00000008
#define SF_CATCH_EXCEPTIONS         0x00000010
#define SF_WRITE_FULL_DUMP          0x00000020
#define SF_BACKGROUND_LOG_WRITER    0x00000040
#define SF_USE_SYSTEMD_JOURNAL      0x00000080
#define SF_JSON_LOG                 0x00000100
#define SF_LOG_TO_STDOUT            0x00000200
#define SF_USE_SYSLOG               0x00000400
#define SF_ENABLE_CONTROL_CONNECTOR 0x00000800
#define SF_ENABLE_PUSH_CONNECTOR    0x00001000
#define SF_ENABLE_EVENT_CONNECTOR   0x00002000
#define SF_FATAL_EXIT_ON_CRT_ERROR  0x00004000

/**
 * Externals
 */
void ListenerThread();
void SessionWatchdog();
void MasterAgentListener();
void SNMPTrapReceiver();
void SyslogReceiver();
void TunnelManager();

#ifdef _WIN32
void ExternalSubagentWatchdog();
void UserAgentWatchdog(TCHAR *executableName);
#endif

void StartLocalDataCollector();
void ShutdownLocalDataCollector();

void StartNotificationProcessor();
void StopNotificationProcessor();
void QueueNotificationMessage(NXCPMessage *msg);

void LoadUserAgentNotifications();

void StartWatchdog();
void StopWatchdog();
int WatchdogMain(DWORD pid, const TCHAR *configSection);

void InitSessionList();

bool RegisterOnServer(const TCHAR *server, int32_t zoneUIN);

void StartPolicyHousekeeper();

void UpdateUserAgentsEnvironment();

void ParseTunnelList(const StringSet& tunnels);
void ParseTunnelList(const ObjectArray<ConfigEntry>& config);

void StartWebServiceHousekeeper();

void StartFileMonitor(const shared_ptr<Config>& config);
void StopFileMonitor();

uint32_t H_TFTPPut(const shared_ptr<ActionExecutionContext>& context);

#if !defined(_WIN32)
void InitStaticSubagents();
#endif

#ifdef _WIN32
extern TCHAR g_windowsServiceName[];
extern TCHAR g_windowsServiceDisplayName[];
#endif

void LIBNXAGENT_EXPORTABLE InitSubAgentAPI(
      void (*postEvent)(uint32_t, const TCHAR *, time_t),
      void (*postEvent2)(uint32_t, const TCHAR *, time_t, const StringMap &),
      bool (*enumerateSessions)(EnumerationCallbackResult(*)(AbstractCommSession *, void *), void*),
      shared_ptr<AbstractCommSession> (*findServerSession)(uint64_t),
      bool (*pushData)(const TCHAR *, const TCHAR *, uint32_t, Timestamp),
      DB_HANDLE (*getLocalDatabaseHandle)(),
      const TCHAR *dataDirectory,
      void (*executeAction)(const TCHAR*, const StringList&),
      bool (*getScreenInfoForUserSession)(uint32_t, uint32_t *, uint32_t *, uint32_t *),
      void (*queueNotificationMessage)(NXCPMessage*),
      void (*registerProblem)(int, const TCHAR*, const TCHAR*),
      void (*unregisterProblem)(const TCHAR*),
      ThreadPool *timerThreadPool
   );

int CreateConfig(bool forceCreate, const char *masterServers, const char *logFile, const char *fileStore,
   const char *configIncludeDir, int numSubAgents, char **subAgentList, const char *extraValues);

uint32_t H_SystemExecute(const shared_ptr<ActionExecutionContext>& context);

/**
 * OpenSSL APPLINK
 */
#if defined(_WIN32) && !_M_ARM64
#include <openssl/applink.c>
#endif

/**
 * Valid options for getopt()
 */
#if defined(_WIN32)
#define VALID_OPTIONS   "B:c:CdD:e:EfFG:hHiIkKlM:n:N:O:P:Q:r:RsSt:TUvW:X:Z:z:"
#else
#define VALID_OPTIONS   "B:c:CdD:fFg:G:hkKlM:p:O:P:Q:r:St:Tu:vW:xX:Z:z:"
#endif

/**
 * Agent startup commands
 */
enum class Command
{
   CHECK_CONFIG,
   CREATE_CONFIG,
   GET_LOG_LOCATION,
   HELP,
   INSTALL_EVENT_SOURCE,
   INSTALL_SERVICE,
   NONE,
   QUERY_CONFIG,
   REMOVE_EVENT_SOURCE,
   REMOVE_SERVICE,
   RESET_IDENTITY,
   RUN_AGENT,
   RUN_WATCHDOG,
   SHUTDOWN_EXT_AGENTS,
   START_SERVICE,
   STOP_SERVICE
};

/**
 * Global variables
 */
uuid g_agentId;
#if _OPENWRT
uint32_t g_dwFlags = AF_ENABLE_ACTIONS | AF_DISABLE_LOCAL_DATABASE;
#else
uint32_t g_dwFlags = AF_ENABLE_ACTIONS | AF_REQUIRE_ENCRYPTION;
#endif
uint32_t g_failFlags = 0;
TCHAR g_szLogFile[MAX_PATH] = AGENT_DEFAULT_LOG;
TCHAR g_szSharedSecret[MAX_SECRET_LENGTH] = _T("admin");
TCHAR g_szConfigFile[MAX_PATH] = AGENT_DEFAULT_CONFIG;
TCHAR g_szFileStore[MAX_PATH] = AGENT_DEFAULT_FILE_STORE;
TCHAR g_szDataDirectory[MAX_PATH] = AGENT_DEFAULT_DATA_DIR;
TCHAR g_dataDirRecoveryPath[MAX_PATH] = _T("");
TCHAR g_szPlatformSuffix[MAX_PSUFFIX_LENGTH] = _T("");
TCHAR g_szConfigServer[MAX_DB_STRING] = _T("not_set");
TCHAR g_registrar[MAX_DB_STRING] = _T("not_set");
TCHAR g_szListenAddress[MAX_PATH] = _T("*");
TCHAR g_szConfigIncludeDir[MAX_PATH] = AGENT_DEFAULT_CONFIG_D;
TCHAR g_szConfigPolicyDir[MAX_PATH] = AGENT_DEFAULT_CONFIG_D;
TCHAR g_szLogParserDirectory[MAX_PATH] = _T("");
TCHAR g_userAgentPolicyDirectory[MAX_PATH] = _T("");
TCHAR g_certificateDirectory[MAX_PATH] = _T("");
TCHAR g_masterAgent[MAX_PATH] = _T("not_set");
TCHAR g_snmpTrapListenAddress[MAX_PATH] = _T("*");
uint16_t g_wListenPort = AGENT_LISTEN_PORT;
TCHAR g_systemName[MAX_OBJECT_NAME] = _T("");
ObjectArray<ServerInfo> g_serverList(8, 8, Ownership::True);
uint32_t g_externalCommandTimeout = 0;   // External process execution timeout for external commands (actions) in milliseconds (0 = unset)
uint32_t g_externalMetricTimeout = 0;  // External process execution timeout for external metrics in milliseconds (0 = unset)
uint32_t g_externalMetricProviderTimeout = 30000;  // External metric provider timeout in milliseconds
uint32_t g_snmpTimeout = 0;
uint16_t g_snmpTrapPort = 162;
int64_t g_agentStartTime;
uint32_t g_startupDelay = 0;
uint32_t g_maxCommSessions = 0;
uint32_t g_longRunningQueryThreshold = 250;
uint32_t g_dcReconciliationBlockSize = 1024;
uint32_t g_dcReconciliationTimeout = 60000;
uint32_t g_dcWriterFlushInterval = 5000;
uint32_t g_dcWriterMaxTransactionSize = 10000;
uint32_t g_dcMinCollectorPoolSize = 4;
uint32_t g_dcMaxCollectorPoolSize = 64;
uint32_t g_dcOfflineExpirationTime = 10; // 10 days
int32_t g_zoneUIN = 0;
uint32_t g_tunnelKeepaliveInterval = 30;
uint16_t g_syslogListenPort = 514;
StringSet g_trustedRootCertificates;
#ifdef _WIN32
uint16_t g_sessionAgentPort = 28180;
#else
uint16_t g_sessionAgentPort = 0;
#endif
uint32_t g_dwIdleTimeout = 120;   // Session idle timeout
uint32_t g_webSvcCacheExpirationTime = 600;  // 10 minutes by default
bool g_restartPending = false;   // Restart pending flag

#ifndef _WIN32
TCHAR g_szPidFile[MAX_PATH] = _T("/var/run/nxagentd.pid");
#endif

/**
 * Static variables
 */
#ifdef _WIN32
static uint32_t s_startupFlags = SF_ENABLE_AUTOLOAD | SF_CATCH_EXCEPTIONS | SF_WRITE_FULL_DUMP | SF_ENABLE_CONTROL_CONNECTOR | SF_ENABLE_PUSH_CONNECTOR | SF_ENABLE_EVENT_CONNECTOR;
#else
static uint32_t s_startupFlags = SF_ENABLE_AUTOLOAD | SF_ENABLE_PUSH_CONNECTOR | SF_ENABLE_EVENT_CONNECTOR;
#endif
static StringList *s_actionList = new StringList();
static TCHAR *m_pszServerList = nullptr;
static TCHAR *m_pszControlServerList = nullptr;
static TCHAR *m_pszMasterServerList = nullptr;
static TCHAR *m_pszSubagentList = nullptr;
static TCHAR *s_externalMetrics = nullptr;
static TCHAR *s_backgroundExternalMetrics = nullptr;
static TCHAR *s_externalMetricProviders = nullptr;
static TCHAR *s_externalListsConfig = nullptr;
static TCHAR *s_externalTablesConfig = nullptr;
static TCHAR *s_externalSubAgentsList = nullptr;
static TCHAR *s_appAgentsList = nullptr;
static StringSet s_serverConnectionList;
static StringSet s_crlList;
static uint32_t s_crlReloadInterval = 14400; // 4 hours by default
static uint32_t s_enabledCiphers = 0xFFFF;
static THREAD s_sessionWatchdogThread = INVALID_THREAD_HANDLE;
static THREAD s_listenerThread = INVALID_THREAD_HANDLE;
static THREAD s_snmpTrapReceiverThread = INVALID_THREAD_HANDLE;
static THREAD s_snmpTrapSenderThread = INVALID_THREAD_HANDLE;
static THREAD s_syslogReceiverThread = INVALID_THREAD_HANDLE;
static THREAD s_masterAgentListenerThread = INVALID_THREAD_HANDLE;
static THREAD s_tunnelManagerThread = INVALID_THREAD_HANDLE;
static TCHAR s_processToWaitFor[MAX_PATH] = _T("");
static uint64_t s_maxLogSize = 16384 * 1024;
static uint32_t s_logHistorySize = 4;
static uint32_t s_logRotationMode = NXLOG_ROTATION_BY_SIZE;
static TCHAR s_dailyLogFileSuffix[64] = _T("");
static TCHAR s_executableName[MAX_PATH];
static int s_debugLevel = NXCONFIG_UNINITIALIZED_VALUE;
static int s_debugLevelOverride = NXCONFIG_UNINITIALIZED_VALUE; // Debug level set from command line
static TCHAR *s_debugTags = nullptr;
static uint32_t s_maxWebSvcPoolSize = 64;
static uint32_t s_defaultExecutionTimeout = 0;  // Default execution timeout for external processes (0 = unset)
static ThreadPool *s_timerThreadPool = nullptr;

#ifdef _WIN32
static TCHAR s_dumpDirectory[MAX_PATH] = _T("{default}");
static Condition s_shutdownCondition(true);
#else
static char s_umask[32] = "";
#endif

#if !defined(_WIN32)
static pid_t s_pid;
#endif

/**
 * Configuration file template
 */
static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
   { _T("Action"), CT_STRING_LIST, 0, 0, 0, 0, s_actionList, nullptr },
   { _T("AppAgent"), CT_STRING_CONCAT, '\n', 0, 0, 0, &s_appAgentsList, nullptr },
   { _T("BackgroundLogWriter"), CT_BOOLEAN_FLAG_32, 0, 0, SF_BACKGROUND_LOG_WRITER, 0, &s_startupFlags, nullptr },
   { _T("BackgroundExternalMetric"), CT_STRING_CONCAT, '\n', 0, 0, 0, &s_backgroundExternalMetrics, nullptr },
   { _T("ControlServers"), CT_STRING_CONCAT, ',', 0, 0, 0, &m_pszControlServerList, nullptr },
   { _T("CreateCrashDumps"), CT_BOOLEAN_FLAG_32, 0, 0, SF_CATCH_EXCEPTIONS, 0, &s_startupFlags, nullptr },
   { _T("CRL"), CT_STRING_SET, 0, 0, 0, 0, &s_crlList, nullptr },
   { _T("CRLReloadInterval"), CT_LONG, 0, 0, 0, 0, &s_crlReloadInterval, nullptr },
   { _T("DataCollectionMaxThreadPoolSize"), CT_LONG, 0, 0, 0, 0, &g_dcMaxCollectorPoolSize, nullptr },
   { _T("DataCollectionMinThreadPoolSize"), CT_LONG, 0, 0, 0, 0, &g_dcMinCollectorPoolSize, nullptr },
   { _T("DataReconciliationBlockSize"), CT_LONG, 0, 0, 0, 0, &g_dcReconciliationBlockSize, nullptr },
   { _T("DataReconciliationTimeout"), CT_LONG, 0, 0, 0, 0, &g_dcReconciliationTimeout, nullptr },
   { _T("DataWriterFlushInterval"), CT_LONG, 0, 0, 0, 0, &g_dcWriterFlushInterval, nullptr },
   { _T("DataWriterMaxTransactionSize"), CT_LONG, 0, 0, 0, 0, &g_dcWriterMaxTransactionSize, nullptr },
   { _T("DailyLogFileSuffix"), CT_STRING, 0, 0, 64, 0, s_dailyLogFileSuffix, nullptr },
   { _T("DebugLevel"), CT_LONG, 0, 0, 0, 0, &s_debugLevel, &s_debugLevel },
   { _T("DebugTags"), CT_STRING_CONCAT, ',', 0, 0, 0, &s_debugTags, nullptr },
   { _T("DefaultExecutionTimeout"), CT_LONG, 0, 0, 0, 0, &s_defaultExecutionTimeout, nullptr },
   { _T("DisableHeartbeatListener"), CT_BOOLEAN_FLAG_32, 0, 0, AF_DISABLE_HEARTBEAT, 0, &g_dwFlags, nullptr },
   { _T("DisableIPv4"), CT_BOOLEAN_FLAG_32, 0, 0, AF_DISABLE_IPV4, 0, &g_dwFlags, nullptr },
   { _T("DisableIPv6"), CT_BOOLEAN_FLAG_32, 0, 0, AF_DISABLE_IPV6, 0, &g_dwFlags, nullptr },
   { _T("DisableLocalDatabase"), CT_BOOLEAN_FLAG_32, 0, 0, AF_DISABLE_LOCAL_DATABASE, 0, &g_dwFlags, nullptr },
#ifdef _WIN32
   { _T("DumpDirectory"), CT_STRING, 0, 0, MAX_PATH, 0, s_dumpDirectory, nullptr },
#endif
   { _T("EnableActions"), CT_BOOLEAN_FLAG_32, 0, 0, AF_ENABLE_ACTIONS, 0, &g_dwFlags, nullptr },
   { _T("EnabledCiphers"), CT_LONG, 0, 0, 0, 0, &s_enabledCiphers, nullptr },
   { _T("EnableControlConnector"), CT_BOOLEAN_FLAG_32, 0, 0, SF_ENABLE_CONTROL_CONNECTOR, 0, &s_startupFlags, nullptr },
   { _T("EnableEventConnector"), CT_BOOLEAN_FLAG_32, 0, 0, SF_ENABLE_EVENT_CONNECTOR, 0, &s_startupFlags, nullptr },
   { _T("EnableModbusProxy"), CT_BOOLEAN_FLAG_32, 0, 0, AF_ENABLE_MODBUS_PROXY, 0, &g_dwFlags, nullptr },
   { _T("EnableProxy"), CT_BOOLEAN_FLAG_32, 0, 0, AF_ENABLE_PROXY, 0, &g_dwFlags, nullptr },
   { _T("EnablePushConnector"), CT_BOOLEAN_FLAG_32, 0, 0, SF_ENABLE_PUSH_CONNECTOR, 0, &s_startupFlags, nullptr },
   { _T("EnableSNMPProxy"), CT_BOOLEAN_FLAG_32, 0, 0, AF_ENABLE_SNMP_PROXY, 0, &g_dwFlags, nullptr },
   { _T("EnableSNMPTrapProxy"), CT_BOOLEAN_FLAG_32, 0, 0, AF_ENABLE_SNMP_TRAP_PROXY, 0, &g_dwFlags, nullptr },
   { _T("EnableSSLTrace"), CT_BOOLEAN_FLAG_32, 0, 0, AF_ENABLE_SSL_TRACE, 0, &g_dwFlags, nullptr },
   { _T("EnableSyslogProxy"), CT_BOOLEAN_FLAG_32, 0, 0, AF_ENABLE_SYSLOG_PROXY, 0, &g_dwFlags, nullptr },
   { _T("EnableSubagentAutoload"), CT_BOOLEAN_FLAG_32, 0, 0, SF_ENABLE_AUTOLOAD, 0, &s_startupFlags, nullptr },
   { _T("EnableTCPProxy"), CT_BOOLEAN_FLAG_32, 0, 0, AF_ENABLE_TCP_PROXY, 0, &g_dwFlags, nullptr },
   { _T("EnableTFTPProxy"), CT_BOOLEAN_FLAG_32, 0, 0, AF_ENABLE_TFTP_PROXY, 0, &g_dwFlags, nullptr },
   { _T("EnableWatchdog"), CT_BOOLEAN_FLAG_32, 0, 0, SF_ENABLE_WATCHDOG, 0, &s_startupFlags, nullptr },
   { _T("EnableWebServiceProxy"), CT_BOOLEAN_FLAG_32, 0, 0, AF_ENABLE_WEBSVC_PROXY, 0, &g_dwFlags, nullptr },
   { _T("ExternalCommandTimeout"), CT_LONG, 0, 0, 0, 0, &g_externalCommandTimeout, nullptr },
   { _T("ExternalList"), CT_STRING_CONCAT, '\n', 0, 0, 0, &s_externalListsConfig, nullptr },
   { _T("ExternalMasterAgent"), CT_STRING, 0, 0, MAX_PATH, 0, g_masterAgent, nullptr },
   { _T("ExternalMetric"), CT_STRING_CONCAT, '\n', 0, 0, 0, &s_externalMetrics, nullptr },
   { _T("ExternalMetricProvider"), CT_STRING_CONCAT, '\n', 0, 0, 0, &s_externalMetricProviders, nullptr },
   { _T("ExternalMetricProviderTimeout"), CT_LONG, 0, 0, 0, 0, &g_externalMetricProviderTimeout, nullptr },
   { _T("ExternalMetricTimeout"), CT_LONG, 0, 0, 0, 0, &g_externalMetricTimeout, nullptr },
   { _T("ExternalSubagent"), CT_STRING_CONCAT, '\n', 0, 0, 0, &s_externalSubAgentsList, nullptr },
   { _T("ExternalTable"), CT_STRING_CONCAT, '\n', 0, 0, 0, &s_externalTablesConfig, nullptr },
   { _T("FatalExitOnCRTError"), CT_BOOLEAN_FLAG_32, 0, 0, SF_FATAL_EXIT_ON_CRT_ERROR, 0, &s_startupFlags, nullptr },
#ifndef _WIN32
   { _T("FileModeCreationMask"), CT_MB_STRING, 0, 0, sizeof(s_umask), 0, s_umask, nullptr },
#endif
   { _T("FileStore"), CT_STRING, 0, 0, MAX_PATH, 0, g_szFileStore, nullptr },
   { _T("FullCrashDumps"), CT_BOOLEAN_FLAG_32, 0, 0, SF_WRITE_FULL_DUMP, 0, &s_startupFlags, nullptr },
   { _T("ListenAddress"), CT_STRING, 0, 0, MAX_PATH, 0, g_szListenAddress, nullptr },
   { _T("ListenPort"), CT_WORD, 0, 0, 0, 0, &g_wListenPort, nullptr },
   { _T("LogFile"), CT_STRING, 0, 0, MAX_PATH, 0, g_szLogFile, nullptr },
   { _T("LogHistorySize"), CT_LONG, 0, 0, 0, 0, &s_logHistorySize, nullptr },
   { _T("LogRotationMode"), CT_LONG, 0, 0, 0, 0, &s_logRotationMode, nullptr },
   { _T("LogUnresolvedSymbols"), CT_BOOLEAN_FLAG_32, 0, 0, SF_LOG_UNRESOLVED_SYMBOLS, 0, &s_startupFlags, nullptr },
   { _T("LongRunningQueryThreshold"), CT_LONG, 0, 0, 0, 0, &g_longRunningQueryThreshold, nullptr },
   { _T("MasterServers"), CT_STRING_CONCAT, ',', 0, 0, 0, &m_pszMasterServerList, nullptr },
   { _T("MaxLogSize"), CT_SIZE_BYTES, 0, 0, 0, 0, &s_maxLogSize, nullptr },
   { _T("MaxSessions"), CT_LONG, 0, 0, 0, 0, &g_maxCommSessions, nullptr },
   { _T("OfflineDataExpirationTime"), CT_LONG, 0, 0, 0, 0, &g_dcOfflineExpirationTime, nullptr },
   { _T("PlatformSuffix"), CT_STRING, 0, 0, MAX_PSUFFIX_LENGTH, 0, g_szPlatformSuffix, nullptr },
   { _T("RequireAuthentication"), CT_BOOLEAN_FLAG_32, 0, 0, AF_REQUIRE_AUTH, 0, &g_dwFlags, nullptr },
   { _T("RequireEncryption"), CT_BOOLEAN_FLAG_32, 0, 0, AF_REQUIRE_ENCRYPTION, 0, &g_dwFlags, nullptr },
   { _T("ServerConnection"), CT_STRING_SET, 0, 0, 0, 0, &s_serverConnectionList, nullptr },
   { _T("Servers"), CT_STRING_CONCAT, ',', 0, 0, 0, &m_pszServerList, nullptr },
   { _T("SessionIdleTimeout"), CT_LONG, 0, 0, 0, 0, &g_dwIdleTimeout, nullptr },
   { _T("SessionAgentPort"), CT_WORD, 0, 0, 0, 0, &g_sessionAgentPort, nullptr },
   { _T("SharedSecret"), CT_STRING, 0, 0, MAX_SECRET_LENGTH, 0, g_szSharedSecret, nullptr },
	{ _T("SNMPTimeout"), CT_LONG, 0, 0, 0, 0, &g_snmpTimeout, nullptr },
	{ _T("SNMPTrapListenAddress"), CT_STRING, 0, 0, MAX_PATH, 0, g_snmpTrapListenAddress, nullptr },
   { _T("SNMPTrapPort"), CT_WORD, 0, 0, 0, 0, &g_snmpTrapPort, nullptr },
   { _T("StartupDelay"), CT_LONG, 0, 0, 0, 0, &g_startupDelay, nullptr },
   { _T("SubAgent"), CT_STRING_CONCAT, '\n', 0, 0, 0, &m_pszSubagentList, nullptr },
   { _T("SyncTimeWithServer"), CT_BOOLEAN_FLAG_32, 0, 0, AF_SYNC_TIME_WITH_SERVER, 0, &g_dwFlags, nullptr },
   { _T("SyslogListenPort"), CT_WORD, 0, 0, 0, 0, &g_syslogListenPort, nullptr },
   { _T("SystemName"), CT_STRING, 0, 0, MAX_OBJECT_NAME, 0, g_systemName, nullptr },
   { _T("TrustedRootCertificate"), CT_STRING_SET, 0, 0, 0, 0, &g_trustedRootCertificates, nullptr },
   { _T("TunnelKeepaliveInterval"), CT_LONG, 0, 0, 0, 0, &g_tunnelKeepaliveInterval, nullptr },
   { _T("VerifyServerCertificate"), CT_BOOLEAN_FLAG_32, 0, 0, AF_CHECK_SERVER_CERTIFICATE, 0, &g_dwFlags, nullptr },
   { _T("WaitForProcess"), CT_STRING, 0, 0, MAX_PATH, 0, s_processToWaitFor, nullptr },
   { _T("WebServiceCacheExpirationTime"), CT_LONG, 0, 0, 0, 0, &g_webSvcCacheExpirationTime, nullptr },
   { _T("WebServiceThreadPoolSize"), CT_LONG, 0, 0, 0, 0, &s_maxWebSvcPoolSize, nullptr },
   { _T("WriteLogAsJson"), CT_BOOLEAN_FLAG_32, 0, 0, SF_JSON_LOG, 0, &s_startupFlags, nullptr },
   { _T("ZoneUIN"), CT_LONG, 0, 0, 0, 0, &g_zoneUIN, nullptr },
   /* Parameters below are deprecated and left for compatibility with older versions */
   { _T("ActionShellExec"), CT_STRING_LIST, 0, 0, 0, 0, s_actionList, nullptr },
   { _T("DataCollectionThreadPoolSize"), CT_LONG, 0, 0, 0, 0, &g_dcMaxCollectorPoolSize, nullptr }, // For compatibility, preferred is DataCollectionMaxThreadPoolSize
   { _T("EncryptedSharedSecret"), CT_STRING, 0, 0, MAX_SECRET_LENGTH, 0, g_szSharedSecret, nullptr },
   { _T("ExecTimeout"), CT_LONG, 0, 0, 0, 0, &s_defaultExecutionTimeout, nullptr },
   { _T("ExternalMetricShellExec"), CT_STRING_CONCAT, '\n', 0, 0, 0, &s_externalMetrics, nullptr },
   { _T("ExternalParameter"), CT_STRING_CONCAT, '\n', 0, 0, 0, &s_externalMetrics, nullptr },
   { _T("ExternalParameterShellExec"), CT_STRING_CONCAT, '\n', 0, 0, 0, &s_externalMetrics, nullptr },
   { _T("ExternalParameterProvider"), CT_STRING_CONCAT, '\n', 0, 0, 0, &s_externalMetricProviders, nullptr },
   { _T("ExternalParameterProviderTimeout"), CT_LONG, 0, 0, 0, 0, &g_externalMetricProviderTimeout, nullptr },
   { _T("ExternalParameterTimeout"), CT_LONG, 0, 0, 0, 0, &g_externalMetricTimeout, nullptr },
   { _T("ExternalParametersProvider"), CT_STRING_CONCAT, '\n', 0, 0, 0, &s_externalMetricProviders, nullptr },
   { _T("ZoneId"), CT_LONG, 0, 0, 0, 0, &g_zoneUIN, nullptr },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr, nullptr }
};

/**
 * Help text
 */
static TCHAR m_szHelpText[] =
   _T("Usage: nxagentd [options]\n")
   _T("Where valid options are:\n")
   _T("   -B <uin>         : Set zone UIN\n")
   _T("   -c <file>        : Use configuration file <file> (default ") AGENT_DEFAULT_CONFIG _T(")\n")
   _T("   -C               : Load configuration file, dump resulting configuration, and exit\n")
   _T("   -d               : Run as daemon/service\n")
	_T("   -D <level>       : Set debug level (0..9)\n")
#ifdef _WIN32
   _T("   -e <name>        : Windows event source name\n")
#endif
	_T("   -f               : Run in foreground\n")
#if !defined(_WIN32)
   _T("   -g <gid>         : Change group ID to <gid> after start\n")
#endif
   _T("   -G <name>        : Use alternate global section <name> in configuration file\n")
   _T("   -h               : Display help and exit\n")
#ifdef _WIN32
   _T("   -H               : Hide agent's window when in standalone mode\n")
	_T("   -i               : Installed Windows service must be interactive\n")
   _T("   -I               : Install Windows service\n")
#endif
   _T("   -K               : Shutdown all connected external sub-agents\n")
   _T("   -l               : Get log file location\n")
   _T("   -M <addr>        : Download config from management server <addr>\n")
#ifdef _WIN32
   _T("   -n <name>        : Service name\n")
   _T("   -N <name>        : Service display name\n")
#endif
   _T("   -O <option>      : Set configuration option\n")
#ifndef _WIN32
   _T("   -p               : Path to pid file (default: /var/run/nxagentd.pid)\n")
#endif
   _T("   -P <text>        : Set platform suffix to <text>\n")
   _T("   -Q <entry>       : Load configuration file, query values for given entry, and exit\n")
   _T("   -r <addr>        : Register agent on management server <addr>\n")
#ifdef _WIN32
   _T("   -R               : Remove Windows service\n")
   _T("   -s               : Start Windows servive\n")
   _T("   -S               : Stop Windows service\n")
   _T("   -t <tag>:<level> : Set debug level for specific tag\n")
#else
#if WITH_SYSTEMD
   _T("   -S               : Run as systemd daemon\n")
#endif
   _T("   -t <tag>:<level> : Set debug level for specific tag\n")
   _T("   -T               : Reset agent identity\n")
   _T("   -u <uid>         : Change user ID to <uid> after start\n")
#endif
   _T("   -v               : Display version and exit\n")
   _T("\n");

/**
 * Server info: constructor
 */
ServerInfo::ServerInfo(const TCHAR *name, bool control, bool master)
{
#ifdef UNICODE
   m_name = MBStringFromWideString(name);
#else
   m_name = MemCopyStringA(name);
#endif

   char *p = strchr(m_name, '/');
   if (p != nullptr)
   {
      *p = 0;
      p++;
      m_address = InetAddress::resolveHostName(m_name);
      if (m_address.isValid())
      {
         int bits = strtol(p, nullptr, 10);
         if ((bits >= 0) && (bits <= ((m_address.getFamily() == AF_INET) ? 32 : 128)))
            m_address.setMaskBits(bits);
      }
      m_redoResolve = false;
   }
   else
   {
      m_address = InetAddress::resolveHostName(m_name);
      m_redoResolve = true;
   }

   m_control = control;
   m_master = master;
   m_lastResolveTime = time(nullptr);
}

/**
 * Server info: destructor
 */
ServerInfo::~ServerInfo()
{
   MemFree(m_name);
}

/**
 * Server info: resolve hostname if needed
 */
void ServerInfo::resolve(bool forceResolve)
{
   time_t now = time(nullptr);
   time_t age = now - m_lastResolveTime;
   if ((age >= 3600) || ((age > 300) && !m_address.isValid()) || forceResolve)
   {
      m_address = InetAddress::resolveHostName(m_name);
      m_lastResolveTime = now;
   }
}

/**
 * Server info: match address
 */
bool ServerInfo::match(const InetAddress &addr, bool forceResolve)
{
   m_mutex.lock();
   if (m_redoResolve)
      resolve(forceResolve);
   bool result = m_address.isValid() ? m_address.contains(addr) : false;
   m_mutex.unlock();
   return result;
}

/**
 * Get agent executable name
 */
const TCHAR *GetAgentExecutableName()
{
   return s_executableName;
}

#ifdef _WIN32

/**
 * Get our own console window handle (an alternative to Microsoft's GetConsoleWindow)
 */
static HWND GetConsoleHWND()
{
   DWORD cpid = GetCurrentProcessId();
	HWND hWnd = nullptr;
   while(true)
   {
	   hWnd = FindWindowEx(nullptr, hWnd, _T("ConsoleWindowClass"), nullptr);
      if (hWnd == nullptr)
         break;

   	DWORD wpid;
      GetWindowThreadProcessId(hWnd, &wpid);
	   if (cpid == wpid)
         break;
   }

	return hWnd;
}

/**
 * Get proc address and write log file
 */
static FARPROC GetProcAddressAndLog(HMODULE hModule, LPCSTR procName)
{
   FARPROC ptr = GetProcAddress(hModule, procName);
   if ((ptr == NULL) && (s_startupFlags & SF_LOG_UNRESOLVED_SYMBOLS))
      nxlog_write(NXLOG_WARNING, _T("Unable to resolve symbol \"%s\""), procName);
   return ptr;
}

/**
 * Shutdown thread (created by H_RestartAgent)
 */
static void ShutdownThread()
{
	nxlog_debug(1, _T("Shutdown thread started"));
   Shutdown();
   ExitProcess(0);
}

#endif   /* _WIN32 */

/**
 * Build command line for agent restart
 */
static StringBuffer BuildRestartCommandLine(bool withWaitPid)
{
   StringBuffer command;
   command.append(_T('"'));
   command.append(s_executableName);
   command.append(_T("\" -c \""));
   command.append(g_szConfigFile);
   if (s_debugLevelOverride != NXCONFIG_UNINITIALIZED_VALUE)
   {
      command.append(_T("\" -D "));
      command.append(s_debugLevelOverride);
   }
   else
   {
      command.append(_T("\""));
   }

#if !defined(_WIN32)
   command.append(_T(" -p \""));
   command.append(g_szPidFile);
   command.append(_T('"'));
#endif

   if (g_szPlatformSuffix[0] != 0)
   {
      command.append(_T(" -P \""));
      command.append(g_szPlatformSuffix);
      command.append(_T('"'));
   }

   shared_ptr<Config> config = g_config;
   const TCHAR *configSection = config->getAlias(_T("agent"));
   if ((configSection != NULL) && (*configSection != 0))
   {
      command.append(_T(" -G "));
      command.append(configSection);
   }

#if WITH_SYSTEMD
   if (g_dwFlags & AF_SYSTEMD_DAEMON)
      command.append(_T(" -S"));
#endif
   if (g_dwFlags & AF_DAEMON)
      command.append(_T(" -d"));
   else
      command.append(_T(" -f"));

   if (g_dwFlags & AF_CENTRAL_CONFIG)
   {
      command.append(_T(" -M "));
      command.append(g_szConfigServer);
   }

#ifdef _WIN32
   if (g_dwFlags & AF_HIDE_WINDOW)
      command.append(_T(" -H"));

   command.append(_T(" -n \""));
   command.append(g_windowsServiceName);
   command.append(_T("\" -e \""));
   command.append(g_windowsEventSourceName);
   command.append(_T('"'));

   if (withWaitPid)
   {
      command.append(_T(" -X "));
      command.append((g_dwFlags & AF_DAEMON) ? 0 : static_cast<uint32_t>(GetCurrentProcessId()));
   }
#else
   if (withWaitPid)
   {
      command.append(_T(" -X "));
      command.append(s_pid);
   }
#endif

   return command;
}

/**
 * Schedule delayed restart (intended only for use by Windows installer)
 */
void ScheduleDelayedRestart()
{
#ifdef _WIN32
   TCHAR path[MAX_PATH];
   GetNetXMSDirectory(nxDirBin, path);

   TCHAR exe[MAX_PATH];
   _tcscpy(exe, path);
   _tcslcat(exe, _T("\\nxreload.exe"), MAX_PATH);
   if (VerifyFileSignature(exe))
   {
      StringBuffer command;
      command.append(_T('"'));
      command.append(exe);
      command.append(_T("\" -- "));
      command.append(BuildRestartCommandLine(false));

      PROCESS_INFORMATION pi;
      STARTUPINFO si;
      memset(&si, 0, sizeof(STARTUPINFO));
      si.cb = sizeof(STARTUPINFO);

      nxlog_debug(1, _T("Starting reload helper:\n%s\n"), command.getBuffer());
      if (CreateProcess(NULL, command.getBuffer(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
      {
         CloseHandle(pi.hThread);
         CloseHandle(pi.hProcess);
      }
   }
   else
   {
      nxlog_debug(1, _T("Cannot verify signature of reload helper %s"), exe);
   }
#endif
}

/**
 * Restart agent
 */
LONG RestartAgent()
{
	nxlog_debug(1, _T("RestartAgent() called"));

   NotifyConnectedServers(_T("AgentRestart"));

	RestartExtSubagents();

   StringBuffer command = BuildRestartCommandLine(true);
#ifdef _WIN32
	nxlog_debug(1, _T("Restarting agent with command line '%s'"), command.getBuffer());

   DWORD dwResult;
   STARTUPINFO si;
   PROCESS_INFORMATION pi;

   // Fill in process startup info structure
   memset(&si, 0, sizeof(STARTUPINFO));
   si.cb = sizeof(STARTUPINFO);

   // Create new process
   if (!CreateProcess(nullptr, command.getBuffer(), nullptr, nullptr, FALSE,
         (g_dwFlags & AF_DAEMON) ? (CREATE_NO_WINDOW | DETACHED_PROCESS) : (CREATE_NEW_CONSOLE), nullptr, nullptr, &si, &pi))
   {
      TCHAR buffer[1024];
      nxlog_write(NXLOG_ERROR, _T("Unable to create process \"%s\" (%s)"),
            command.getBuffer(), GetSystemErrorText(GetLastError(), buffer, 1024));
      dwResult = ERR_EXEC_FAILED;
   }
   else
   {
      // Close all handles
      CloseHandle(pi.hThread);
      CloseHandle(pi.hProcess);
      dwResult = ERR_SUCCESS;
   }
   if ((dwResult == ERR_SUCCESS) && (!(g_dwFlags & AF_DAEMON)))
   {
      if (g_dwFlags & AF_HIDE_WINDOW)
      {
         s_shutdownCondition.set();
      }
      else
      {
         ThreadCreate(ShutdownThread);
      }
   }
   return dwResult;
#else
	nxlog_debug(1, _T("Restarting agent with command line '%s'"), command.cstr());
	return ProcessExecutor::execute(command) ? ERR_SUCCESS : ERR_EXEC_FAILED;
#endif
}

/**
 * Handler for Agent.Restart action
 */
static uint32_t H_RestartAgent(const shared_ptr<ActionExecutionContext>& context)
{
   return RestartAgent();
}

/**
 * Handler for Agent.RotateLog action
 */
static uint32_t H_RotateLog(const shared_ptr<ActionExecutionContext>& context)
{
   return nxlog_rotate() ? ERR_SUCCESS : ERR_INTERNAL_ERROR;
}

#ifdef _WIN32

/**
 * Handler for System.ExecuteInAllSessions action
 */
static uint32_t H_SystemExecuteInAllSessions(const shared_ptr<ActionExecutionContext>& context)
{
   if (!context->hasArgs())
      return ERR_BAD_ARGUMENTS;

   return ExecuteInAllSessions(context->getArg(0)) ? ERR_SUCCESS : ERR_EXEC_FAILED;
}

#else

/**
 * Handler for action Process.Terminate
 */
static uint32_t H_TerminateProcess(const shared_ptr<ActionExecutionContext>& context)
{
   if (!context->hasArgs())
      return ERR_BAD_ARGUMENTS;

   pid_t pid = _tcstol(context->getArg(0), nullptr, 0);
   if (pid <= 0)
      return ERR_BAD_ARGUMENTS;

   return (kill(pid, SIGKILL) == 0) ? ERR_SUCCESS : ERR_INTERNAL_ERROR;
}

#endif

/**
 * Signal handler for UNIX platforms
 */
#if !defined(_WIN32)

static THREAD_RESULT THREAD_CALL SignalHandler(void *pArg)
{
	sigset_t signals;
	sigemptyset(&signals);
	sigaddset(&signals, SIGTERM);
	sigaddset(&signals, SIGINT);
	sigaddset(&signals, SIGSEGV);
	sigaddset(&signals, SIGHUP);
	sigaddset(&signals, SIGUSR1);
	sigaddset(&signals, SIGUSR2);
#if !defined(__sun) && !defined(_AIX) && !defined(__hpux)
	sigaddset(&signals, SIGPIPE);
#endif

	while(true)
	{
		int nSignal;
		if (sigwait(&signals, &nSignal) == 0)
		{
			switch(nSignal)
			{
				case SIGTERM:
				case SIGINT:
					goto stop_handler;
				case SIGSEGV:
					abort();
					break;
				default:
					break;
			}
		}
		else
		{
			ThreadSleepMs(100);
		}
	}

stop_handler:
	sigprocmask(SIG_UNBLOCK, &signals, nullptr);
	return THREAD_OK;
}

#endif

/**
 * Load platform subagent
 */
static void LoadPlatformSubagent()
{
#ifdef _WIN32
   LoadSubAgent(_T("WINNT.NSM"));
#else
#if HAVE_SYS_UTSNAME_H && !defined(_STATIC_AGENT)
   struct utsname un;
   TCHAR szName[MAX_PATH];
   int i;

   if (uname(&un) != -1)
   {
      // Convert system name to lowercase
      for(i = 0; un.sysname[i] != 0; i++)
         un.sysname[i] = tolower(un.sysname[i]);
      if (!strcmp(un.sysname, "hp-ux"))
         strcpy(un.sysname, "hpux");
      _sntprintf(szName, MAX_PATH, _T("%hs.nsm"), un.sysname);
      LoadSubAgent(szName);
   }
#endif
#endif
}

/**
 * Configure agent directory: construct directory name and create it if needed
 */
static void ConfigureAgentDirectory(TCHAR *generatedPath, const TCHAR *suffix, const TCHAR *contentDescription)
{
   TCHAR buffer[MAX_PATH];
   TCHAR *path = (generatedPath != nullptr) ? generatedPath : buffer;
   TCHAR tail = g_szDataDirectory[_tcslen(g_szDataDirectory) - 1];
   _sntprintf(path, MAX_PATH, _T("%s%s%s") FS_PATH_SEPARATOR, g_szDataDirectory,
              ((tail != '\\') && (tail != '/')) ? FS_PATH_SEPARATOR : _T(""),
              suffix);
   CreateDirectoryTree(path);
   nxlog_debug_tag(DEBUG_TAG_STARTUP, 2, _T("%s directory: %s"), contentDescription, path);
}

/**
 * Parser server list
 */
static void ParseServerList(TCHAR *serverList, bool isControl, bool isMaster)
{
	TCHAR *curr, *next;
	for(curr = next = serverList; next != nullptr && *curr != 0; curr = next + 1)
	{
		next = _tcschr(curr, _T(','));
		if (next != nullptr)
			*next = 0;
		Trim(curr);
		if (*curr != 0)
		{
         g_serverList.add(new ServerInfo(curr, isControl, isMaster));
         nxlog_debug_tag(DEBUG_TAG_COMM, 3, _T("Added server access record %s (control=%s, master=%s)"), curr,
                  BooleanToString(isControl), BooleanToString(isMaster));
		}
	}
	MemFree(serverList);
}

/**
 * Get log destination flags
 */
static inline uint32_t GetLogDestinationFlag()
{
   if (s_startupFlags & SF_USE_SYSLOG)
      return NXLOG_USE_SYSLOG;
   if (s_startupFlags & SF_USE_SYSTEMD_JOURNAL)
      return NXLOG_USE_SYSTEMD;
   if (s_startupFlags & SF_LOG_TO_STDOUT)
      return NXLOG_USE_STDOUT;
   return 0;
}

/**
 * Reload all registered CRLs and schedule next reload
 */
static void ScheduledCRLReload()
{
   ReloadAllCRLs();
   ThreadPoolScheduleRelative(g_commThreadPool, s_crlReloadInterval, ScheduledCRLReload);
}

/**
 * Save agent ID to file
 */
static void SaveAgentIdToFile()
{
   TCHAR agentIdFile[MAX_PATH];
   GetNetXMSDirectory(nxDirData, agentIdFile);
   _tcscat(agentIdFile, FS_PATH_SEPARATOR _T("nxagentd.id"));
   FILE *fp = _tfopen(agentIdFile, _T("w"));
   if (fp != nullptr)
   {
      char buffer[64];
      fputs(g_agentId.toStringA(buffer), fp);
      fclose(fp);
      nxlog_write(NXLOG_INFO, _T("Agent ID backup file updated"));
   }
   else
   {
      nxlog_write(NXLOG_ERROR, _T("Cannot create agent ID backup file %s (%s)"), agentIdFile, _tcserror(errno));
   }
}

/**
 * Log agent information
 */
static void LogAgentInfo()
{
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("Core agent version ") NETXMS_VERSION_STRING _T(" (build tag ") NETXMS_BUILD_TAG _T(")"));
   TCHAR timezone[32];
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("System time zone is %s"), GetSystemTimeZone(timezone, 32));
   nxlog_write_tag(NXLOG_INFO, _T("config"), _T("Additional configuration files was loaded from %s"), g_szConfigIncludeDir);
   nxlog_write_tag(NXLOG_INFO, _T("logger"), _T("Debug level set to %d"), s_debugLevel);

   if (g_failFlags & FAIL_LOAD_CONFIG)
   {
      nxlog_write_tag(NXLOG_WARNING, _T("config"), _T("Configuration loaded with errors"));
      RegisterProblem(SEVERITY_MINOR, _T("agent-config-load"), _T("Agent configuration loaded with errors"));
   }
   nxlog_write_tag(NXLOG_INFO, _T("config"), _T("Main configuration file: %s"), g_szConfigFile);
   nxlog_write_tag(NXLOG_INFO, _T("config"), _T("Configuration tree:"));
   g_config->print();

   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("PATH = %s"), GetEnvironmentVariableEx(_T("PATH")).cstr());
#if defined(_AIX)
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("LIBPATH = %s"), GetEnvironmentVariableEx(_T("LIBPATH")).cstr());
#elif !defined(_WIN32)
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("LD_LIBRARY_PATH = %s"), GetEnvironmentVariableEx(_T("LD_LIBRARY_PATH")).cstr());
#ifdef __sunos
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("LD_LIBRARY_PATH_32 = %s"), GetEnvironmentVariableEx(_T("LD_LIBRARY_PATH_32")).cstr());
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("LD_LIBRARY_PATH_64 = %s"), GetEnvironmentVariableEx(_T("LD_LIBRARY_PATH_64")).cstr());
#endif
#endif
}

/**
 * Agent initialization
 */
BOOL Initialize()
{
   if (s_debugLevel == NXCONFIG_UNINITIALIZED_VALUE)
      s_debugLevel = 0;

#ifndef _WIN32
   bool validUmask = true;
   if (s_umask[0] != 0)
   {
      // umask should be given as 3 octal digits
      if (strlen(s_umask) == 3)
      {
         char *eptr;
         int mask = strtol(s_umask, &eptr, 8);
         if (*eptr == 0)
         {
            umask(mask);
         }
         else
         {
            validUmask = false;
         }
      }
      else
      {
         validUmask = false;
      }
   }
#endif

   // Open log file
   if (!nxlog_open((s_startupFlags & SF_USE_SYSLOG) ? NXAGENTD_SYSLOG_NAME : g_szLogFile,
                   GetLogDestinationFlag() |
	                ((s_startupFlags & SF_BACKGROUND_LOG_WRITER) ? NXLOG_BACKGROUND_WRITER : 0) |
                   ((g_dwFlags & AF_DAEMON) ? 0 : NXLOG_PRINT_TO_STDOUT) |
                   ((s_startupFlags & SF_JSON_LOG) ? NXLOG_JSON_FORMAT : 0)))
	{
	   s_debugLevel = 1;
	   g_failFlags |= FAIL_OPEN_LOG;
      nxlog_open(NXAGENTD_SYSLOG_NAME, NXLOG_USE_SYSLOG |
	               ((s_startupFlags & SF_BACKGROUND_LOG_WRITER) ? NXLOG_BACKGROUND_WRITER : 0) |
                  ((g_dwFlags & AF_DAEMON) ? 0 : NXLOG_PRINT_TO_STDOUT));
		_ftprintf(stderr, _T("ERROR: Cannot open log file \"%s\", logs will be written to syslog with debug level 1\n"), g_szLogFile);
      RegisterProblem(SEVERITY_MAJOR, _T("agent-log-open"), _T("Agent cannot open log file"));
	}
	else
   {
      if (!(s_startupFlags & (SF_USE_SYSLOG | SF_USE_SYSTEMD_JOURNAL)))
      {
         if (!nxlog_set_rotation_policy((int)s_logRotationMode, s_maxLogSize, (int)s_logHistorySize, s_dailyLogFileSuffix))
            if (!(g_dwFlags & AF_DAEMON))
               _tprintf(_T("WARNING: cannot set log rotation policy; using default values\n"));
      }
   }

   LogAgentInfo();
   nxlog_set_rotation_hook(LogAgentInfo);

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

	nxlog_set_debug_level(s_debugLevel);

#ifndef _WIN32
   if (s_umask[0] != 0)
   {
      if (validUmask)
         nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("File mode creation mask (umask) set to %hs"), s_umask);
      else
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_STARTUP, _T("Invalid umask value \"%hs\""), s_umask);
   }
#endif

	if (_tcscmp(g_masterAgent, _T("not_set")))
	{
		g_dwFlags |= AF_SUBAGENT_LOADER;
		nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("Switched to external subagent loader mode, master agent address is %s"), g_masterAgent);
	}

   if (g_dataDirRecoveryPath[0] != 0)
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("Data directory recovered from %s"), g_dataDirRecoveryPath);
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("Data directory: %s"), g_szDataDirectory);
   CreateDirectoryTree(g_szDataDirectory);

#ifdef _WIN32
   if (s_startupFlags & SF_CATCH_EXCEPTIONS)
   {
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("Crash dump generation is enabled (dump directory is %s)"), s_dumpDirectory);
      CreateDirectoryTree(s_dumpDirectory);
      if (g_failFlags & FAIL_CRASH_SERVER_START)
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_STARTUP, _T("Failed to start crash dump collector process"));
   }
   else
   {
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("Crash dump generation is disabled"));
   }
#endif

   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("File store: %s"), g_szFileStore);
   CreateDirectoryTree(g_szFileStore);
   SetEnvironmentVariable(_T("NETXMS_FILE_STORE"), g_szFileStore);

#ifndef _WIN32
   nxlog_debug_tag(DEBUG_TAG_STARTUP, 2, _T("Effective user ID %d"), (int)geteuid());
   nxlog_debug_tag(DEBUG_TAG_STARTUP, 2, _T("Effective group ID %d"), (int)getegid());
#endif

   nxlog_debug_tag(DEBUG_TAG_STARTUP, 2, _T("Configuration policy directory: %s"), g_szConfigPolicyDir);

   ConfigureAgentDirectory(g_szLogParserDirectory, SUBDIR_LOGPARSER_POLICY, _T("Log parser policy"));
   ConfigureAgentDirectory(g_userAgentPolicyDirectory, SUBDIR_USERAGENT_POLICY, _T("User agent policy"));
   ConfigureAgentDirectory(g_certificateDirectory, SUBDIR_CERTIFICATES, _T("Certificate"));
   ConfigureAgentDirectory(nullptr, SUBDIR_CRL, _T("CRL"));

#ifdef _WIN32
   WSADATA wsaData;
	int wrc = WSAStartup(MAKEWORD(2, 2), &wsaData);
   if (wrc != 0)
   {
      TCHAR buffer[1024];
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_STARTUP, _T("Call to WSAStartup() failed (%s)"), GetLastSocketErrorText(buffer, 1024));
      return FALSE;
   }
#endif

   if (!InitCryptoLib(s_enabledCiphers))
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_STARTUP, _T("Failed to initialize cryptography module"));
      return FALSE;
   }

   // Initialize libssl
#ifdef _WITH_ENCRYPTION
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   OPENSSL_init_ssl(0, nullptr);
#else
   SSL_library_init();
   SSL_load_error_strings();
#endif
#else /* not _WITH_ENCRYPTION */
   if ((g_dwFlags & AF_REQUIRE_ENCRYPTION) && !(g_dwFlags & AF_SUBAGENT_LOADER))
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_STARTUP, _T("Encryption set as required in configuration but agent was build without encryption support"));
      return false;
   }
#endif

   s_timerThreadPool = ThreadPoolCreate(_T("TIMER"), 2, 16);

   // Initialize API for subagents
   InitSubAgentAPI(PostEvent, PostEvent, EnumerateSessions, FindServerSessionByServerId,
      PushData, GetLocalDatabaseHandle, g_szDataDirectory, ExecuteAction, GetScreenInfoForUserSession, QueueNotificationMessage,
      RegisterProblem, UnregisterProblem, s_timerThreadPool);
   nxlog_debug_tag(DEBUG_TAG_STARTUP, 1, _T("Subagent API initialized"));

   DBInit();
   if (!(g_dwFlags & AF_DISABLE_LOCAL_DATABASE))
   {
      if (!OpenLocalDatabase())
      {
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_LOCALDB, _T("Local database unavailable"));
      }
   }
   else
   {
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_LOCALDB, _T("Local database is disabled"));
   }

   /*** Read agent ID ***/
   TCHAR agentIdText[MAX_DB_STRING];
   if (ReadMetadata(_T("AgentId"), agentIdText) != nullptr)
      g_agentId = uuid::parse(agentIdText);

   // Backup - read agent ID from file
   uuid backupAgentId;
   TCHAR agentIdFile[MAX_PATH];
   GetNetXMSDirectory(nxDirData, agentIdFile);
   _tcscat(agentIdFile, FS_PATH_SEPARATOR _T("nxagentd.id"));
   FILE *fp = _tfopen(agentIdFile, _T("r"));
   if (fp != nullptr)
   {
      char buffer[256] = "";
      fgets(buffer, 256, fp);
      fclose(fp);
#ifdef UNICODE
      WCHAR wbuffer[64];
      mb_to_wchar(buffer, -1, wbuffer, 63);
      wbuffer[63] = 0;
      backupAgentId = uuid::parse(wbuffer);
#else
      backupAgentId = uuid::parse(buffer);
#endif
   }

   if (g_agentId.isNull())
   {
      if (backupAgentId.isNull())
      {
         g_agentId = uuid::generate();
         WriteMetadata(_T("AgentId"), g_agentId.toString(agentIdText));
         SaveAgentIdToFile();
         nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("New agent ID generated"));
      }
      else
      {
         g_agentId = backupAgentId;
         nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("Agent ID recovered from backup file"));
      }
   }
   else if (backupAgentId.isNull() || !g_agentId.equals(backupAgentId))
   {
      SaveAgentIdToFile();
   }
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("Agent ID is %s"), g_agentId.toString(agentIdText));

   TCHAR hostname[256];
   if (GetLocalHostName(hostname, 256, true) == nullptr)
   {
      // Fallback to non-FQDN host name
      GetLocalHostName(hostname, 256, false);
   }
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("Local host name is \"%s\""), hostname);

   // Set system name to host name if not set in config
   if (g_systemName[0] == 0)
      GetLocalHostName(g_systemName, MAX_OBJECT_NAME, false);
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("Using system name \"%s\""), g_systemName);

   shared_ptr<Config> config = g_config;

	if (!(g_dwFlags & AF_SUBAGENT_LOADER))
	{
      InitSessionList();
	   g_commThreadPool = ThreadPoolCreate(_T("COMM"), MIN(g_maxCommSessions, 8), MAX(MIN(g_maxCommSessions * 2, 16), 512));
	   g_webSvcThreadPool = ThreadPoolCreate(_T("WEBSVC"), 4, s_maxWebSvcPoolSize);

		// Load local CRLs
		auto it = s_crlList.begin();
		while(it.hasNext())
		{
		   const TCHAR *location = it.next();
		   if (!_tcsncmp(location, _T("http://"), 7) || !_tcsncmp(location, _T("https://"), 8))
		   {
#ifdef UNICODE
		      char *url = UTF8StringFromWideString(location);
            AddRemoteCRL(url, true);
            MemFree(url);
#else
		      AddRemoteCRL(location, true);
#endif
		   }
		   else
		   {
		      AddLocalCRL(location);
		   }
		}
		s_crlList.clear();

		// Parse outgoing server connection (tunnel) list
      ParseTunnelList(s_serverConnectionList);
      unique_ptr<ObjectArray<ConfigEntry>> connections = config->getSubEntries(_T("/ServerConnection"));
      if (connections != nullptr)
         ParseTunnelList(*connections);

      // Parse server lists
      if (m_pszMasterServerList != nullptr)
         ParseServerList(m_pszMasterServerList, true, true);
		if (m_pszControlServerList != nullptr)
			ParseServerList(m_pszControlServerList, true, false);
		if (m_pszServerList != nullptr)
			ParseServerList(m_pszServerList, false, false);

      // Add built-in actions
      AddAction(_T("Agent.Restart"), false, nullptr, H_RestartAgent, _T("CORE"), _T("Restart agent"));
      AddAction(_T("Agent.RotateLog"), false, nullptr, H_RotateLog, _T("CORE"), _T("Rotate agent log"));
#ifndef _WIN32
      AddAction(_T("Process.Terminate"), false, nullptr, H_TerminateProcess, _T("CORE"), _T("Terminate process"));
#endif
      AddAction(_T("TFTP.Put"), false, nullptr, H_TFTPPut, _T("CORE"), _T("Transfer file to remote system via TFTP"));
      if (config->getValueAsBoolean(_T("/CORE/EnableArbitraryCommandExecution"), false))
      {
         nxlog_write(NXLOG_INFO, _T("Arbitrary command execution enabled"));
         AddAction(_T("System.Execute"), false, nullptr, H_SystemExecute, _T("CORE"), _T("Execute system command"));
#ifdef _WIN32
         AddAction(_T("System.ExecuteInAllSessions"), false, nullptr, H_SystemExecuteInAllSessions, _T("CORE"), _T("Execute system command in all sessions"));
#endif
      }
      else
      {
         nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("Arbitrary command execution disabled"));
      }

	   // Load platform subagents
#if !defined(_WIN32)
      InitStaticSubagents();
#endif
      if (s_startupFlags & SF_ENABLE_AUTOLOAD)
      {
         LoadPlatformSubagent();
      }
   }

   // Wait for external process if requested
	if (s_processToWaitFor[0] != 0)
	{
	   nxlog_debug_tag(DEBUG_TAG_STARTUP, 1, _T("Waiting for process %s"), s_processToWaitFor);
		if (!WaitForProcess(s_processToWaitFor))
	      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_STARTUP, _T("Call to WaitForProcess failed for process %s"), s_processToWaitFor);
	}

	// Load other subagents
   if (m_pszSubagentList != nullptr)
   {
      TCHAR *curr, *next;
      for(curr = next = m_pszSubagentList; next != nullptr && *curr != 0; curr = next + 1)
      {
         next = _tcschr(curr, _T('\n'));
         if (next != nullptr)
            *next = 0;
         Trim(curr);
         LoadSubAgent(curr);
      }
      MemFree(m_pszSubagentList);
   }

   // Parse action list
   for(int i = 0; i < s_actionList->size(); i++)
   {
      const TCHAR *action = s_actionList->get(i);
      if (!AddActionFromConfig(action))
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_STARTUP, _T("Unable to add action \"%s\""), action);
   }
   delete s_actionList;

   // Parse external parameters
   if (s_backgroundExternalMetrics != nullptr)
   {
      TCHAR *curr, *next;
      for(curr = next = s_backgroundExternalMetrics; next != nullptr && *curr != 0; curr = next + 1)
      {
         next = _tcschr(curr, _T('\n'));
         if (next != nullptr)
            *next = 0;
         Trim(curr);
         if (!AddBackgroundExternalMetric(curr))
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_STARTUP, _T("Unable to add external metric background\"%s\""), curr);
      }
      MemFree(s_backgroundExternalMetrics);
   }
   
   // Parse external parameters list
   if (s_externalMetrics != nullptr)
   {
      TCHAR *curr, *next;
      for(curr = next = s_externalMetrics; next != nullptr && *curr != 0; curr = next + 1)
      {
         next = _tcschr(curr, _T('\n'));
         if (next != nullptr)
            *next = 0;
         Trim(curr);
         if (!AddExternalMetric(curr, false))
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_STARTUP, _T("Unable to add external metric \"%s\""), curr);
      }
      MemFree(s_externalMetrics);
   }

   // Parse external lists
   if (s_externalListsConfig != nullptr)
   {
      TCHAR *curr, *next;
      for(curr = next = s_externalListsConfig; next != nullptr && *curr != 0; curr = next + 1)
      {
         next = _tcschr(curr, _T('\n'));
         if (next != nullptr)
            *next = 0;
         Trim(curr);
         if (!AddExternalMetric(curr, true))
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_STARTUP, _T("Unable to add external list \"%s\""), curr);
      }
      MemFree(s_externalListsConfig);
   }

   // Parse external tables
   if (s_externalTablesConfig != nullptr)
   {
      TCHAR *curr, *next;
      for(curr = next = s_externalTablesConfig; next != nullptr && *curr != 0; curr = next + 1)
      {
         next = _tcschr(curr, _T('\n'));
         if (next != nullptr)
            *next = 0;
         Trim(curr);
         if (!AddExternalTable(curr))
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_STARTUP, _T("Unable to add external table \"%s\""), curr);
      }
      MemFree(s_externalTablesConfig);
   }

   // Parse external tables (ConfigEntry)
   unique_ptr<ObjectArray<ConfigEntry>> extTables = config->getSubEntries(_T("/ExternalTable"));
   if (extTables != nullptr)
   {
      for (int i = 0; i < extTables->size(); i++)
      {
         ConfigEntry *e = extTables->get(i);
         if (!AddExternalTable(e))
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_STARTUP, _T("Unable to add external table \"%s\""), e->getName());
      }
   }

   // Parse external parameters providers list
   if (s_externalMetricProviders != nullptr)
   {
      TCHAR *curr, *next;
      for(curr = next = s_externalMetricProviders; next != nullptr && *curr != 0; curr = next + 1)
      {
         next = _tcschr(curr, _T('\n'));
         if (next != nullptr)
            *next = 0;
         Trim(curr);
         if (!AddMetricProvider(curr))
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_STARTUP, _T("Unable to add external metric provider \"%s\""), curr);
      }
      MemFree(s_externalMetricProviders);
   }

   // Parse external structured data providers
   unique_ptr<ObjectArray<ConfigEntry>> externalDataProvider = config->getSubEntries(_T("/ExternalDataProvider"));
   if (externalDataProvider != nullptr)
   {
      for (int i = 0; i < externalDataProvider->size(); i++)
      {
         ConfigEntry *e = externalDataProvider->get(i);
         if (!AddExternalStructuredDataProvider(e))
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_STARTUP, _T("Unable to add external data provider \"%s\""), e->getName());
      }
   }

   if (!(g_dwFlags & AF_SUBAGENT_LOADER))
   {
      // Parse external subagents list
	   if (!(g_dwFlags & AF_SUBAGENT_LOADER) && (s_externalSubAgentsList != nullptr))
      {
	      TCHAR *curr, *next;
         for(curr = next = s_externalSubAgentsList; next != nullptr && *curr != 0; curr = next + 1)
         {
            next = _tcschr(curr, _T('\n'));
            if (next != nullptr)
               *next = 0;
            Trim(curr);
            if (!AddExternalSubagent(curr))
               nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_STARTUP, _T("Unable to add external subagent \"%s\""), curr);
         }
         MemFree(s_externalSubAgentsList);
      }

      // Additional external subagents implicitly defined by EXT:* config sections
      unique_ptr<ObjectArray<ConfigEntry>> entries = config->getSubEntries(_T("/"), _T("EXT:*"));
      for(int i = 0; i < entries->size(); i++)
      {
         const TCHAR *name = entries->get(i)->getName() + 4;
         if (!AddExternalSubagent(name))
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_STARTUP, _T("Unable to add external subagent \"%s\""), name);
      }

      // Parse application agents list
	   if (!(g_dwFlags & AF_SUBAGENT_LOADER) && (s_appAgentsList != nullptr))
      {
	      TCHAR *curr, *next;
         for(curr = next = s_appAgentsList; next != nullptr && *curr != 0; curr = next + 1)
         {
            next = _tcschr(curr, _T('\n'));
            if (next != nullptr)
               *next = 0;
            Trim(curr);
            RegisterApplicationAgent(curr);
         }
         MemFree(s_appAgentsList);
      }

	   BYTE hwid[HARDWARE_ID_LENGTH];
	   if (GetSystemHardwareId(hwid))
	      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("System hardware ID is %s"), BinToStr(hwid, HARDWARE_ID_LENGTH, agentIdText));
	   else
         nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("System hardware ID is unknown"));
   }

   ThreadSleep(1);

   // If StartupDelay is greater than zero, then wait
   if (g_startupDelay > 0)
   {
      if (g_dwFlags & AF_DAEMON)
      {
         ThreadSleep(g_startupDelay);
      }
      else
      {
         _tprintf(_T("XXXXXX%*s]\rWAIT ["), g_startupDelay, _T(" "));
         fflush(stdout);
         for(uint32_t i = 0; i < g_startupDelay; i++)
         {
            ThreadSleep(1);
            _puttc(_T('.'), stdout);
            fflush(stdout);
         }
         _tprintf(_T("\n"));
      }
   }

   g_executorThreadPool = ThreadPoolCreate(_T("PROCEXEC"), std::max((GetExternalDataProviderCount() + 1) / 2, 1), std::max(GetExternalDataProviderCount() * 2, 16));
   StartExternalMetricProviders();
   StartBackgroundMetricCollection();

   // Agent start time
   g_agentStartTime = GetMonotonicClockTime();

   StartNotificationProcessor();

	if (g_dwFlags & AF_ENABLE_SNMP_TRAP_PROXY)
	{
      s_snmpTrapReceiverThread = ThreadCreateEx(SNMPTrapReceiver);
   }

   if (g_dwFlags & AF_ENABLE_SYSLOG_PROXY)
   {
      s_syslogReceiverThread = ThreadCreateEx(SyslogReceiver);
   }

	if (g_dwFlags & AF_SUBAGENT_LOADER)
	{
		s_masterAgentListenerThread = ThreadCreateEx(MasterAgentListener);
	}
	else
	{
      LoadUserAgentNotifications();
      StartLocalDataCollector();
      if (g_wListenPort != 0)
      {
         s_listenerThread = ThreadCreateEx(ListenerThread);
         s_sessionWatchdogThread = ThreadCreateEx(SessionWatchdog);
      }
      else
      {
         nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_COMM, _T("TCP listener is disabled"));
      }
      StartSessionAgentConnector();
      if (s_startupFlags & SF_ENABLE_PUSH_CONNECTOR)
      {
         StartPushConnector();
      }
      else
      {
         nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_COMM, _T("Push connector is disabled"));
      }
      if (s_startupFlags & SF_ENABLE_EVENT_CONNECTOR)
      {
         StartEventConnector();
      }
      else
      {
         nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_COMM, _T("Event connector is disabled"));
      }
      if (s_startupFlags & SF_ENABLE_CONTROL_CONNECTOR)
	   {
         StartControlConnector();
      }
      else
      {
         nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_COMM, _T("Control connector is disabled"));
      }

   	if (s_startupFlags & SF_REGISTER)
      {
         RegisterOnServer(g_registrar, g_zoneUIN);
      }

      s_tunnelManagerThread = ThreadCreateEx(TunnelManager);

      ThreadPoolScheduleRelative(g_commThreadPool, s_crlReloadInterval, ScheduledCRLReload);
   }

   ThreadSleep(1);

   StartFileMonitor(config);

	// Start watchdog process
   if (s_startupFlags & SF_ENABLE_WATCHDOG)
      StartWatchdog();

	if (!(g_dwFlags & AF_SUBAGENT_LOADER))
	{
	   // Delete file used for upgrade if exists
      TCHAR upgradeFileName[MAX_PATH];
	   ReadRegistryAsString(_T("upgrade.file"), upgradeFileName, MAX_PATH, _T(""));
	   if (upgradeFileName[0] != 0)
	   {
         _tremove(upgradeFileName);
         DeleteRegistryEntry(_T("upgrade.file"));
	   }

	   // Update policy inventory according to files that exist on file system
      StartPolicyHousekeeper();

      StartWebServiceHousekeeper();

#ifdef _WIN32
      if (g_config->getValueAsBoolean(_T("/CORE/AutoStartUserAgent"), false))
      {
         nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("Starting user agents for all logged in users"));

         TCHAR binDir[MAX_PATH];
         GetNetXMSDirectory(nxDirBin, binDir);

         StringBuffer command = _T("\"");
         command.append(binDir);
         command.append(_T("\\"));
         command.append(g_config->getValue(_T("/CORE/UserAgentExecutable"), _T("nxuseragent.exe")));
         if (VerifyFileSignature(command.cstr() + 1)) // skip leading "
         {
            command.append(_T("\""));
            ExecuteInAllSessions(command);
         }
         else
         {
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_STARTUP, _T("Signature validation failed for user agent executable \"%s\""), command.cstr() + 1);
         }
      }
      if (g_config->getValueAsBoolean(_T("/CORE/UserAgentWatchdog"), false))
      {
         ThreadCreate(UserAgentWatchdog, MemCopyString(g_config->getValue(_T("/CORE/UserAgentExecutable"), _T("nxuseragent.exe"))));
      }
      if (g_config->getValueAsBoolean(_T("/CORE/ExternalSubagentWatchdog"), false))
      {
         ThreadCreate(ExternalSubagentWatchdog);
      }
#endif
	}

   return TRUE;
}

/**
 * Shutdown agent
 */
void Shutdown()
{
	nxlog_debug_tag(_T("shutdown"), 2, _T("Shutdown() called"));
	if (s_startupFlags & SF_ENABLE_WATCHDOG)
		StopWatchdog();

   g_dwFlags |= AF_SHUTDOWN;
   InitiateProcessShutdown();

	if (g_dwFlags & AF_SUBAGENT_LOADER)
	{
		ThreadJoin(s_masterAgentListenerThread);
	}
	else
	{
      ShutdownLocalDataCollector();
		ThreadJoin(s_sessionWatchdogThread);
		ThreadJoin(s_listenerThread);
		ThreadJoin(s_tunnelManagerThread);
		StopExternalSubagentConnectors();
	}
   if (g_dwFlags & AF_ENABLE_SNMP_TRAP_PROXY)
   {
      ThreadJoin(s_snmpTrapReceiverThread);
      ThreadJoin(s_snmpTrapSenderThread);
   }
   if (g_dwFlags & AF_ENABLE_SYSLOG_PROXY)
   {
      ThreadJoin(s_syslogReceiverThread);
   }

   StopFileMonitor();

   StopExternalMetricProviders();
   StopNotificationProcessor();

   if (!(g_dwFlags & AF_SUBAGENT_LOADER))
   {
      ThreadPoolDestroy(g_commThreadPool);
      ThreadPoolDestroy(g_webSvcThreadPool);
   }
   ThreadPoolDestroy(g_executorThreadPool);
   ThreadPoolDestroy(s_timerThreadPool);

   UnloadAllSubAgents();
   CloseLocalDatabase();
   nxlog_report_event(2, NXLOG_INFO, 0, _T("NetXMS Agent stopped"));
   nxlog_close();

   // Notify main thread about shutdown
#ifdef _WIN32
   s_shutdownCondition.set();
#endif

   // Remove PID file
#if !defined(_WIN32)
   _tremove(g_szPidFile);
#endif
}

/**
 * Common Main()
 */
void Main()
{
   nxlog_report_event(1, NXLOG_INFO, 0, _T("NetXMS Agent started"));

   if (g_dwFlags & AF_DAEMON)
   {
#if defined(_WIN32)
      s_shutdownCondition.wait(INFINITE);
#else
      StartMainLoop(SignalHandler, NULL);
#endif
   }
   else
   {
#if defined(_WIN32)
      if (g_dwFlags & AF_HIDE_WINDOW)
      {
         HWND hWnd;

         hWnd = GetConsoleHWND();
         if (hWnd != NULL)
            ShowWindow(hWnd, SW_HIDE);
         s_shutdownCondition.wait(INFINITE);
         ThreadSleep(1);
      }
      else
      {
         _tprintf(_T("Agent running. Press ESC to shutdown.\n"));
         while(1)
         {
            if (_getch() == 27)
               break;
         }
         _tprintf(_T("Agent shutting down...\n"));
         Shutdown();
      }
#else
      _tprintf(_T("Agent running. Press Ctrl+C to shutdown.\n"));
      StartMainLoop(SignalHandler, NULL);
      _tprintf(_T("\nStopping agent...\n"));
#endif
   }
}

/**
 * Do necessary actions on agent restart
 */
static void DoRestartActions(uint32_t oldPID)
{
#if defined(_WIN32)
   if (oldPID == 0)
   {
      // Service
      StopAgentService();
      WaitForService(SERVICE_STOPPED);
      StartAgentService();
      ExitProcess(0);
   }
   else
   {
      HANDLE hProcess;

      hProcess = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, oldPID);
      if (hProcess != nullptr)
      {
         if (WaitForSingleObject(hProcess, 60000) == WAIT_TIMEOUT)
         {
            TerminateProcess(hProcess, 0);
         }
         CloseHandle(hProcess);
      }
   }
#else
#if WITH_SYSTEMD
   if (RestartService(oldPID))
   {
      // successfully restarted agent service using systemd, exit this instance
      exit(0);
   }
#endif
   kill(oldPID, SIGTERM);
   int i;
   for(i = 0; i < 30; i++)
   {
      sleep(2);
      if (kill(oldPID, SIGCONT) == -1)
         break;
   }

   // Kill previous instance of agent if it's still running
   if (i == 30)
      kill(oldPID, SIGKILL);
#endif
}

/**
 * Initiate shutdown of connected external subagents and session agents
 */
static void InitiateExternalProcessShutdown(bool restart)
{
   NXCPMessage msg;
   msg.setCode(CMD_SHUTDOWN);
   msg.setField(VID_RESTART, restart);
   if (SendControlMessage(&msg))
      _tprintf(_T("Control message sent successfully to master agent\n"));
   else
      _tprintf(_T("ERROR: Unable to send control message to master agent\n"));
}

#ifndef _WIN32

/**
 * Get user ID
 */
static int GetUserId(const char *name)
{
   char *eptr;
   int id = (int)strtol(name, &eptr, 10);
   if (*eptr == 0)
      return id;

#if HAVE_GETPWNAM
   struct passwd *p = getpwnam(name);
   if (p == NULL)
   {
      _tprintf(_T("Invalid user ID \"%hs\"\n"), name);
      return 0;
   }
   return p->pw_uid;
#else
   _tprintf(_T("Invalid user ID \"%hs\"\n"), name);
   return 0;
#endif
}

/**
 * Get group ID
 */
static int GetGroupId(const char *name)
{
   char *eptr;
   int id = (int)strtol(name, &eptr, 10);
   if (*eptr == 0)
      return id;

#if HAVE_GETGRNAM
   struct group *g = getgrnam(name);
   if (g == NULL)
   {
      _tprintf(_T("Invalid group ID \"%hs\"\n"), name);
      return 0;
   }
   return g->gr_gid;
#else
   _tprintf(_T("Invalid group ID \"%hs\"\n"), name);
   return 0;
#endif
}

#endif

/**
 * Reset agent identity
 */
static int ResetIdentity()
{
   int exitCode = 0;
   if (OpenLocalDatabase())
   {
      if (WriteMetadata(_T("AgentId"), uuid::NULL_UUID.toString()))
      {
         _tprintf(_T("Agent ID was reset\n"));
      }
      else
      {
         _tprintf(_T("Cannot update agent ID\n"));
         exitCode = 6;
      }
      CloseLocalDatabase();
   }
   else
   {
      _tprintf(_T("Cannot open local database\n"));
      exitCode = 5;
   }

   TCHAR agentIdFile[MAX_PATH];
   GetNetXMSDirectory(nxDirData, agentIdFile);
   _tcscat(agentIdFile, FS_PATH_SEPARATOR _T("nxagentd.id"));
   _tremove(agentIdFile);

   ConfigureAgentDirectory(g_certificateDirectory, SUBDIR_CERTIFICATES, _T("Certificate"));
   _TDIR *dir = _topendir(g_certificateDirectory);
   if (dir != nullptr)
   {
      TCHAR path[MAX_PATH];
      struct _tdirent *d;
      while((d = _treaddir(dir)) != nullptr)
      {
         if (!_tcscmp(d->d_name, _T(".")) || !_tcscmp(d->d_name, _T("..")))
            continue;

         _sntprintf(path, MAX_PATH, _T("%s%s"), g_certificateDirectory, d->d_name);
         if (_tremove(path) == 0)
         {
            _tprintf(_T("Deleted certificate file %s\n"), path);
         }
         else
         {
            _tprintf(_T("Cannot delete certificate file %s (%s)\n"), path, _tcserror(errno));
            exitCode = 7;
         }
      }
      _tclosedir(dir);
   }
   else
   {
      _tprintf(_T("Cannot read certificate directory\n"));
   }
   return exitCode;
}

/**
 * Update agent environment from config
 */
static void UpdateEnvironment()
{
   shared_ptr<Config> config = g_config;
   unique_ptr<ObjectArray<ConfigEntry>> entrySet = config->getSubEntries(_T("/ENV"), _T("*"));
   if (entrySet == nullptr)
      return;

   for(int i = 0; i < entrySet->size(); i++)
   {
      ConfigEntry *e = entrySet->get(i);
      SetEnvironmentVariable(e->getName(), e->getValue());
   }
   UpdateUserAgentsEnvironment();
}

/**
 * Query values of configuration entry
 */
static int QueryConfig(const Config& config, const TCHAR *path)
{
   ConfigEntry *e = config.getEntry(path);
   if ((e == nullptr) || (e->getValueCount() == 0))
      return 2;

   for(int i = 0; i < e->getValueCount(); i++)
      ConsolePrintf(_T("%s\n"), e->getValue(i));
   return 0;
}

/**
 * Search for given configuration file and place result into provided buffer (should at least MAX_PATH in size)
 */
static void SearchConfig(const TCHAR *file, TCHAR *buffer)
{
#ifdef _WIN32
   TCHAR path[MAX_PATH];
   GetNetXMSDirectory(nxDirEtc, path);
   _tcscat(path, _T("\\"));
   _tcscat(path, file);
   if (_taccess(path, 4) == 0)
   {
      _tcscpy(buffer, path);
   }
   else
   {
      _tcscpy(buffer, _T("C:\\"));
      _tcscat(buffer, file);
   }
#else
   TCHAR path[MAX_PATH] = _T("");
   String homeDir = GetEnvironmentVariableEx(_T("NETXMS_HOME"));
   if (!homeDir.isEmpty())
   {
      _sntprintf(path, MAX_PATH, _T("%s/etc/%s"), homeDir.cstr(), file);
   }
   if ((path[0] != 0) && (_taccess(path, 4) == 0))
   {
      _tcscpy(buffer, path);
   }
   else
   {
      _tcscpy(path, SYSCONFDIR _T("/"));
      _tcscat(path, file);
      if (_taccess(path, 4) == 0)
      {
         _tcscpy(buffer, path);
      }
      else
      {
         _tcscpy(buffer, _T("/etc/"));
         _tcscat(buffer, file);
      }
   }
#endif
}

/**
 * Application entry point
 */
int main(int argc, char *argv[])
{
   Command command = Command::RUN_AGENT;
   int exitCode = 0;
   BOOL bRestart = FALSE;
   uint32_t oldPID, dwMainPID;
	char *eptr;
   TCHAR configSection[MAX_DB_STRING] = DEFAULT_CONFIG_SECTION;
   TCHAR configEntry[MAX_DB_STRING];
   const char *extraConfigValues = nullptr;
   bool forceCreateConfig = false;
   bool restartExternalProcesses = false;
   StringBuffer cmdLineConfigValues;   // configuration values passed on command line
#ifdef _WIN32
   TCHAR szModuleName[MAX_PATH];
   HKEY hKey;
   DWORD dwSize;
   ProcessExecutor *crashServer = nullptr;
   google_breakpad::ExceptionHandler *exceptionHandler = nullptr;
#else
	int uid = 0, gid = 0;
#endif

   InitNetXMSProcess(false);

   // Check for alternate config file location
#ifdef _WIN32
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\NetXMS\\Agent"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
   {
      dwSize = MAX_PATH * sizeof(TCHAR);
      RegQueryValueEx(hKey, _T("ConfigFile"), NULL, NULL, (BYTE *)g_szConfigFile, &dwSize);
      dwSize = MAX_PATH * sizeof(TCHAR);
      RegQueryValueEx(hKey, _T("ConfigIncludeDir"), NULL, NULL, (BYTE *)g_szConfigIncludeDir, &dwSize);
      RegCloseKey(hKey);
   }
#else
   MutableString env = GetEnvironmentVariableEx(_T("NXAGENTD_CONFIG"));
   if (!env.isEmpty())
      _tcslcpy(g_szConfigFile, env, MAX_PATH);

	env = GetEnvironmentVariableEx(_T("NXAGENTD_CONFIG_D"));
   if (!env.isEmpty())
      _tcslcpy(g_szConfigIncludeDir, env, MAX_PATH);
#endif

#if defined(_WIN32) || HAVE_DECL_GETOPT_LONG
   static struct option longOptions[] =
   {
      { (char *)"debug-tag", 1, nullptr, 't' },
      { (char *)"reset-identity", 0, nullptr, 'T' },
      { (char *)"zone-uin", 1, nullptr, 'B' },
      { nullptr, 0, 0, 0 }
   };
#endif

   // Parse command line
	if (argc == 1)
		command = Command::HELP;
   opterr = 1;

   int ch;
#if defined(_WIN32) || HAVE_DECL_GETOPT_LONG
   while((ch = getopt_long(argc, argv, VALID_OPTIONS, longOptions, nullptr)) != -1)
#else
   while((ch = getopt(argc, argv, VALID_OPTIONS)) != -1)
#endif
   {
      switch(ch)
      {
         case 'B':  //zone UIN
         {
            int32_t zoneUIN = strtol(optarg, &eptr, 0);
            if ((*eptr != 0))
            {
               fprintf(stderr, "Invalid zone UIN: %s\n", optarg);
               command = Command::NONE;
               exitCode = 1;
            }
            else
               g_zoneUIN = zoneUIN;
            break;
         }
         case 'c':   // Configuration file
#ifdef UNICODE
            MultiByteToWideCharSysLocale(optarg, g_szConfigFile, MAX_PATH);
				g_szConfigFile[MAX_PATH - 1] = 0;
#else
            strlcpy(g_szConfigFile, optarg, MAX_PATH);
#endif
            break;
         case 'C':   // Configuration check only
            command = Command::CHECK_CONFIG;
            break;
         case 'd':   // Run as daemon
            g_dwFlags |= AF_DAEMON;
            break;
         case 'D':   // Turn on debug output
				s_debugLevelOverride = strtol(optarg, &eptr, 0);
				if ((*eptr != 0) || (s_debugLevel > 9))
				{
					fprintf(stderr, "Invalid debug level: %s\n", optarg);
					command = Command::NONE;
					exitCode = 1;
				}
				s_debugLevel = s_debugLevelOverride;
            break;
			case 'f':	// Run in foreground
            g_dwFlags &= ~AF_DAEMON;
				break;
         case 'F':   // Force create config
            forceCreateConfig = true;
            break;
#ifndef _WIN32
			case 'g':	// set group ID
				gid = GetGroupId(optarg);
				break;
#endif
         case 'G':
#ifdef UNICODE
            MultiByteToWideCharSysLocale(optarg, configSection, MAX_DB_STRING);
				configSection[MAX_DB_STRING - 1] = 0;
#else
            strlcpy(configSection, optarg, MAX_DB_STRING);
#endif
            break;
         case 'h':   // Display help and exit
            command = Command::HELP;
            break;
#ifdef _WIN32
         case 'H':   // Hide window
            g_dwFlags |= AF_HIDE_WINDOW;
            break;
#endif
         case 'k':   // Delayed restart of external sub-agents and session agents
            restartExternalProcesses = true;
            command = Command::SHUTDOWN_EXT_AGENTS;
            break;
         case 'K':   // Shutdown external sub-agents and session agents
            command = Command::SHUTDOWN_EXT_AGENTS;
            break;
         case 'l':
            command = Command::GET_LOG_LOCATION;
            break;
         case 'O':   // Override configuration values
            if (optarg != nullptr)
            {
               if (!cmdLineConfigValues.isEmpty())
                  cmdLineConfigValues.append(_T("\n"));
               cmdLineConfigValues.appendMBString(optarg);
            }
            break;
#ifndef _WIN32
         case 'p':   // PID file
#ifdef UNICODE
				MultiByteToWideCharSysLocale(optarg, g_szPidFile, MAX_PATH);
				g_szPidFile[MAX_PATH - 1] = 0;
#else
            strlcpy(g_szPidFile, optarg, MAX_PATH);
#endif
            break;
#endif
         case 'M':
            g_dwFlags |= AF_CENTRAL_CONFIG;
#ifdef UNICODE
            MultiByteToWideCharSysLocale(optarg, g_szConfigServer, MAX_DB_STRING);
				g_szConfigServer[MAX_DB_STRING - 1] = 0;
#else
            strlcpy(g_szConfigServer, optarg, MAX_DB_STRING);
#endif
            break;
         case 'r':
            s_startupFlags |= SF_REGISTER;
#ifdef UNICODE
            MultiByteToWideCharSysLocale(optarg, g_registrar, MAX_DB_STRING);
				g_registrar[MAX_DB_STRING - 1] = 0;
#else
            strlcpy(g_registrar, optarg, MAX_DB_STRING);
#endif
            break;
         case 'P':   // Platform suffix
#ifdef UNICODE
            MultiByteToWideCharSysLocale(optarg, g_szPlatformSuffix, MAX_PSUFFIX_LENGTH);
				g_szPlatformSuffix[MAX_PSUFFIX_LENGTH - 1] = 0;
#else
            strlcpy(g_szPlatformSuffix, optarg, MAX_PSUFFIX_LENGTH);
#endif
            break;
         case 'Q':   // Query config
            command = Command::QUERY_CONFIG;
#ifdef UNICODE
            MultiByteToWideCharSysLocale(optarg, configEntry, MAX_DB_STRING);
            configEntry[MAX_DB_STRING - 1] = 0;
#else
            strlcpy(configEntry, optarg, MAX_DB_STRING);
#endif
            break;
         case 't':
            nxlog_set_debug_level_tag(optarg);
            break;
         case 'T':   // Reset identity
            command = Command::RESET_IDENTITY;
            break;
#ifndef _WIN32
			case 'u':	// set user ID
				uid = GetUserId(optarg);
				break;
#endif
         case 'v':   // Print version and exit
            _tprintf(_T("NetXMS Core Agent Version ") NETXMS_VERSION_STRING _T(" Build ") NETXMS_BUILD_TAG IS_UNICODE_BUILD_STRING _T("\n"));
            command = Command::NONE;
            break;
         case 'W':   // Watchdog process
            command = Command::RUN_WATCHDOG;
            dwMainPID = strtoul(optarg, nullptr, 10);
            break;
         case 'X':   // Agent is being restarted
            bRestart = TRUE;
            oldPID = strtoul(optarg, nullptr, 10);
            break;
         case 'Z':   // Create configuration file
            command = Command::CREATE_CONFIG;
#ifdef UNICODE
            MultiByteToWideCharSysLocale(optarg, g_szConfigFile, MAX_PATH);
				g_szConfigFile[MAX_PATH - 1] = 0;
#else
            strlcpy(g_szConfigFile, optarg, MAX_PATH);
#endif
            break;
         case 'z':   // Extra config parameters
            extraConfigValues = optarg;
            break;
#ifdef _WIN32
         case 'i':
            g_dwFlags |= AF_INTERACTIVE_SERVICE;
            break;
         case 'I':   // Install Windows service
            command = Command::INSTALL_SERVICE;
            break;
         case 'R':   // Remove Windows service
            command = Command::REMOVE_SERVICE;
            break;
         case 's':   // Start Windows service
            command = Command::START_SERVICE;
            break;
         case 'S':   // Stop Windows service
            command = Command::STOP_SERVICE;
            break;
         case 'E':   // Install Windows event source
            command = Command::INSTALL_EVENT_SOURCE;
            break;
         case 'U':   // Remove Windows event source
            command = Command::REMOVE_EVENT_SOURCE;
            break;
         case 'e':   // Event source name
#ifdef UNICODE
            MultiByteToWideCharSysLocale(optarg, g_windowsEventSourceName, MAX_PATH);
				g_windowsEventSourceName[MAX_PATH - 1] = 0;
#else
            strlcpy(g_windowsEventSourceName, optarg, MAX_PATH);
#endif
            break;
         case 'n':   // Service name
#ifdef UNICODE
            MultiByteToWideCharSysLocale(optarg, g_windowsServiceName, MAX_PATH);
				g_windowsServiceName[MAX_PATH - 1] = 0;
#else
            strlcpy(g_windowsServiceName, optarg, MAX_PATH);
#endif
            break;
         case 'N':   // Service display name
#ifdef UNICODE
            MultiByteToWideCharSysLocale(optarg, g_windowsServiceDisplayName, MAX_PATH);
				g_windowsServiceDisplayName[MAX_PATH - 1] = 0;
#else
            strlcpy(g_windowsServiceDisplayName, optarg, MAX_PATH);
#endif
            break;
#else /* not _WIN32 */
         case 'S':   // Run as systemd daemon with type "simple"
            g_dwFlags |= AF_DAEMON | AF_SYSTEMD_DAEMON;
            break;
#endif
         case '?':
            command = Command::HELP;
            exitCode = 1;
            break;
         default:
            break;
      }
   }

	if (!_tcscmp(g_szConfigFile, _T("{search}")))
	   SearchConfig(_T("nxagentd.conf"), g_szConfigFile);
	if (!_tcscmp(g_szConfigIncludeDir, _T("{search}")))
      SearchConfig(_T("nxagentd.conf.d"), g_szConfigIncludeDir);

   if (bRestart)
      DoRestartActions(oldPID);

   // Do requested action
   shared_ptr<Config> config;
   switch(command)
   {
      case Command::RUN_AGENT:
         // Set default value for session idle timeout based on
         // connect() timeout, if possible
#if HAVE_SYSCTLBYNAME && !defined(_IPSO)
         {
            LONG nVal;
				size_t nSize;

				nSize = sizeof(nVal);
            if (sysctlbyname("net.inet.tcp.keepinit", &nVal, &nSize, NULL, 0) == 0)
            {
               g_dwIdleTimeout = nVal / 1000 + 15;
            }
         }
#endif

         if (g_dwFlags & AF_CENTRAL_CONFIG)
         {
            if (s_debugLevel > 0)
               _tprintf(_T("Downloading configuration from %s...\n"), g_szConfigServer);
            if (DownloadConfig(g_szConfigServer))
            {
               if (s_debugLevel > 0)
                  _tprintf(_T("Configuration downloaded successfully\n"));
            }
            else
            {
               if (s_debugLevel > 0)
                  _tprintf(_T("Configuration download failed\n"));
            }
         }

			if (!LoadConfig(configSection, cmdLineConfigValues, true, false))
			{
            _tprintf(_T("WARNING: configuration loaded with errors\n"));
		      g_failFlags |= FAIL_LOAD_CONFIG;
			}

         config = g_config;
         // Check if master section starts with EXT:
         // If yes, switch to external subagent mode
         if (!_tcsnicmp(configSection, _T("EXT:"), 4))
         {
            _tcslcpy(g_masterAgent, &configSection[4], MAX_PATH);
         }

         if (!config->parseTemplate(configSection, m_cfgTemplate))
         {
            _tprintf(_T("WARNING: configuration loaded with errors\n"));
            g_failFlags |= FAIL_LOAD_CONFIG;
         }
         DecryptPassword(_T("netxms"), g_szSharedSecret, g_szSharedSecret, MAX_SECRET_LENGTH);

         // Calculate execution timeouts
         // Default value is in DefaultExecutionTimeout (old versions may use ExecTimeout)
         // Then individual timeouts can be set by ExternalCommandTimeout, ExternalMetricTimeout, and ExternalMetricProviderTimeout
         if (s_defaultExecutionTimeout == 0)
            s_defaultExecutionTimeout = 5000;   // 5 seconds default
         if (g_externalCommandTimeout == 0)
            g_externalCommandTimeout = s_defaultExecutionTimeout;
         if (g_externalMetricTimeout == 0)
            g_externalMetricTimeout = s_defaultExecutionTimeout;
         if (g_externalMetricProviderTimeout == 0)
            g_externalMetricProviderTimeout = s_defaultExecutionTimeout;

         // try to guess executable path
#ifdef _WIN32
         GetModuleFileName(GetModuleHandle(nullptr), s_executableName, MAX_PATH);
#else
#ifdef UNICODE
         char __buffer[MAX_PATH];
#else
#define __buffer s_executableName
#endif
         if (realpath(argv[0], __buffer) == nullptr)
         {
            // fallback
            GetNetXMSDirectory(nxDirBin, s_executableName);
            _tcslcat(s_executableName, _T("/nxagentd"), sizeof(s_executableName) / sizeof(s_executableName[0]));
         }
         else
         {
#ifdef UNICODE
            size_t len = strlen(__buffer);
            mb_to_wchar(__buffer, len, s_executableName, MAX_PATH);
#endif
         }
#endif

#ifdef _WIN32
         // Set default dump directory
         if (!_tcscmp(s_dumpDirectory, _T("{default}")))
         {
            GetNetXMSDirectory(nxDirData, s_dumpDirectory);
            _tcslcat(s_dumpDirectory, _T("\\dump"), MAX_PATH);
         }

         // Configure exception handling
         if (s_startupFlags & SF_FATAL_EXIT_ON_CRT_ERROR)
         {
            EnableFatalExitOnCRTError(true);
         }
         if (s_startupFlags & SF_CATCH_EXCEPTIONS)
         {
            TCHAR pipeName[64];
            _sntprintf(pipeName, 64, _T("\\\\.\\pipe\\nxagentd-crashsrv-%u"), GetCurrentProcessId());

            TCHAR crashServerCmdLine[256];
            _sntprintf(crashServerCmdLine, 256, _T("nxcrashsrv.exe nxagentd-crashsrv-%u \"%s\""), GetCurrentProcessId(), s_dumpDirectory);
            crashServer = new ProcessExecutor(crashServerCmdLine, false);
            if (crashServer->execute())
            {
               // Wait for server's named pipe to appear
               bool success = false;
               uint32_t timeout = 2000;
               while (timeout > 0)
               {
                  if (WaitNamedPipe(pipeName, timeout))
                  {
                     success = true;
                     break;   // Success
                  }
                  if (GetLastError() != ERROR_FILE_NOT_FOUND)
                     break;   // Unrecoverable error
                  Sleep(200);
                  timeout -= 200;
               }
               if (success)
               {
                  static google_breakpad::CustomInfoEntry clientInfoEntries[] = { { L"ProcessName", L"nxagentd" } };
                  static google_breakpad::CustomClientInfo clientInfo = { clientInfoEntries, 1 };
                  exceptionHandler = new google_breakpad::ExceptionHandler(s_dumpDirectory, nullptr, nullptr, nullptr, google_breakpad::ExceptionHandler::HANDLER_EXCEPTION | google_breakpad::ExceptionHandler::HANDLER_PURECALL,
                     static_cast<MINIDUMP_TYPE>(((s_startupFlags & SF_WRITE_FULL_DUMP) ? MiniDumpWithFullMemory : MiniDumpNormal) | MiniDumpWithHandleData | MiniDumpWithProcessThreadData),
                     pipeName, &clientInfo);
               }
               else
               {
                  g_failFlags |= FAIL_CRASH_SERVER_START;
                  delete_and_null(crashServer);
               }
            }
            else
            {
               g_failFlags |= FAIL_CRASH_SERVER_START;
               delete_and_null(crashServer);
            }
         }
#endif

         if ((!_tcsicmp(g_szLogFile, _T("{syslog}"))) ||
             (!_tcsicmp(g_szLogFile, _T("{eventlog}"))))
         {
            s_startupFlags |= SF_USE_SYSLOG;
            s_startupFlags &= ~(SF_USE_SYSTEMD_JOURNAL | SF_LOG_TO_STDOUT);
         }
         else if (!_tcsicmp(g_szLogFile, _T("{systemd}")))
         {
            s_startupFlags |= SF_USE_SYSTEMD_JOURNAL;
            s_startupFlags &= ~(SF_USE_SYSLOG | SF_LOG_TO_STDOUT);
         }
         else if (!_tcsicmp(g_szLogFile, _T("{stdout}")))
         {
            s_startupFlags |= SF_LOG_TO_STDOUT;
            s_startupFlags &= ~(SF_USE_SYSLOG | SF_USE_SYSTEMD_JOURNAL);
         }
         else
         {
            s_startupFlags &= ~(SF_USE_SYSLOG | SF_USE_SYSTEMD_JOURNAL | SF_LOG_TO_STDOUT);
         }
#ifdef _WIN32
         UpdateEnvironment();
         if (g_dwFlags & AF_DAEMON)
         {
            InitService();
         }
         else
         {
            if (Initialize())
            {
               Main();
            }
            else
            {
               ConsolePrintf(_T("Agent initialization failed\n"));
               nxlog_close();
               exitCode = 3;
            }
         }
#else    /* _WIN32 */
         if ((g_dwFlags & AF_DAEMON) && !(g_dwFlags & AF_SYSTEMD_DAEMON))
         {
            if (daemon(0, 0) == -1)
            {
               perror("Unable to setup itself as a daemon");
               exitCode = 4;
            }
         }
         if (exitCode == 0)
         {
#ifndef _WIN32
            if (gid == 0)
            {
               const TCHAR *v = config->getValue(_T("/%agent/GroupId"));
               if (v != NULL)
               {
#ifdef UNICODE
                  char vmb[64];
                  WideCharToMultiByteSysLocale(v, vmb, 64);
                  vmb[63] = 0;
                  gid = GetGroupId(vmb);
#else
                  gid = GetGroupId(v);
#endif
               }
            }
            if (uid == 0)
            {
               const TCHAR *v = config->getValue(_T("/%agent/UserId"));
               if (v != NULL)
               {
#ifdef UNICODE
                  char vmb[64];
                  WideCharToMultiByteSysLocale(v, vmb, 64);
                  vmb[63] = 0;
                  uid = GetUserId(vmb);
#else
                  uid = GetUserId(v);
#endif
               }
            }

            if (gid > 0)
            {
               if (setgid(gid) != 0)
                  _tprintf(_T("setgid(%d) call failed (%s)\n"), gid, _tcserror(errno));
            }
            if (uid > 0)
            {
               if (setuid(uid) != 0)
                  _tprintf(_T("setuid(%d) call failed (%s)\n"), uid, _tcserror(errno));
            }
#endif

            UpdateEnvironment();

            s_pid = getpid();
            if (Initialize())
            {
               // Write PID file
               FILE *fp = _tfopen(g_szPidFile, _T("w"));
               if (fp != nullptr)
               {
                  _ftprintf(fp, _T("%d"), s_pid);
                  fclose(fp);
               }
               Main();
               Shutdown();
            }
            else
            {
               ConsolePrintf(_T("Agent initialization failed\n"));
               nxlog_close();
               exitCode = 3;
            }
         }
#endif   /* _WIN32 */
         break;
      case Command::CHECK_CONFIG:
         {
            bool validConfig = LoadConfig(configSection, cmdLineConfigValues, true, false);
            if (validConfig)
            {
               config = g_config;
               config->print(stdout);
               validConfig = config->parseTemplate(configSection, m_cfgTemplate);
            }

            if (!validConfig)
            {
               ConsolePrintf(_T("Configuration file check failed\n"));
               exitCode = 2;
            }
         }
         break;
      case Command::QUERY_CONFIG:
         {
            bool validConfig = LoadConfig(configSection, cmdLineConfigValues, true, true);
            if (validConfig)
            {
               config = g_config;
               exitCode = QueryConfig(*config, configEntry);
            }
            else
            {
               ConsolePrintf(_T("Configuration file load failed\n"));
               exitCode = 2;
            }
         }
         break;
		case Command::RUN_WATCHDOG:
		   if (s_debugLevel == NXCONFIG_UNINITIALIZED_VALUE)
		      s_debugLevel = 0;
		   nxlog_set_debug_level(s_debugLevel);
			exitCode = WatchdogMain(dwMainPID, _tcscmp(configSection, DEFAULT_CONFIG_SECTION) ? configSection : NULL);
			break;
      case Command::CREATE_CONFIG:
         exitCode = CreateConfig(forceCreateConfig, CHECK_NULL_A(argv[optind]), CHECK_NULL_A(argv[optind + 1]), 
               CHECK_NULL_A(argv[optind + 2]), CHECK_NULL_A(argv[optind + 3]), argc - optind - 4, &argv[optind + 4], extraConfigValues);
         break;
      case Command::SHUTDOWN_EXT_AGENTS:
         InitiateExternalProcessShutdown(restartExternalProcesses);
         exitCode = 0;
         break;
      case Command::GET_LOG_LOCATION:
         if (LoadConfig(configSection, cmdLineConfigValues, true, true))
         {
            if (g_config->parseTemplate(configSection, m_cfgTemplate))
            {
               _tprintf(_T("%s\n"), g_szLogFile);
            }
            else
            {
               ConsolePrintf(_T("Configuration file parsing failed\n"));
               exitCode = 2;
            }
         }
         else
         {
            ConsolePrintf(_T("Configuration file load failed\n"));
            exitCode = 2;
         }
         break;
      case Command::RESET_IDENTITY:
         if (LoadConfig(configSection, cmdLineConfigValues, true, false))
         {
            if (g_config->parseTemplate(configSection, m_cfgTemplate))
            {
               exitCode = ResetIdentity();
            }
            else
            {
               ConsolePrintf(_T("Configuration file parsing failed\n"));
               exitCode = 2;
            }
         }
         else
         {
            ConsolePrintf(_T("Configuration file load failed\n"));
            exitCode = 2;
         }
         break;
#ifdef _WIN32
      case Command::INSTALL_SERVICE:
         GetModuleFileName(GetModuleHandle(NULL), szModuleName, MAX_PATH);
         InstallService(szModuleName, g_szConfigFile, s_debugLevel);
         break;
      case Command::REMOVE_SERVICE:
         RemoveService();
         break;
      case Command::INSTALL_EVENT_SOURCE:
         GetModuleFileName(GetModuleHandle(NULL), szModuleName, MAX_PATH);
         InstallEventSource(szModuleName);
         break;
      case Command::REMOVE_EVENT_SOURCE:
         RemoveEventSource();
         break;
      case Command::START_SERVICE:
         StartAgentService();
         break;
      case Command::STOP_SERVICE:
         StopAgentService();
         break;
#endif
		case Command::HELP:
         _fputts(m_szHelpText, stdout);
			break;
      default:
         break;
   }

#ifdef _WIN32
   delete exceptionHandler;
   delete crashServer;
#endif

   return exitCode;
}
