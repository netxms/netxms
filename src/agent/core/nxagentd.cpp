/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2020 Raden Solutions
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

#if HAVE_GETOPT_H
#include <getopt.h>
#endif

#ifdef _WIN32
#include <conio.h>
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

#if HAVE_SYS_SYSCTL_H
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
 * Externals
 */
THREAD_RESULT THREAD_CALL ListenerThread(void *);
THREAD_RESULT THREAD_CALL SessionWatchdog(void *);
THREAD_RESULT THREAD_CALL EventSender(void *);
THREAD_RESULT THREAD_CALL MasterAgentListener(void *arg);
THREAD_RESULT THREAD_CALL SNMPTrapReceiver(void *);
THREAD_RESULT THREAD_CALL SNMPTrapSender(void *);
THREAD_RESULT THREAD_CALL SyslogReceiver(void *);
THREAD_RESULT THREAD_CALL SyslogSender(void *);
THREAD_RESULT THREAD_CALL TunnelManager(void *);

#ifdef _WIN32
THREAD_RESULT THREAD_CALL UserAgentWatchdog(void *);
#endif

void ShutdownEventSender();
void ShutdownSNMPTrapSender();

void ShutdownSyslogSender();

void StartLocalDataCollector();
void ShutdownLocalDataCollector();

void LoadUserAgentNotifications();

void StartWatchdog();
void StopWatchdog();
int WatchdogMain(DWORD pid, const TCHAR *configSection);

void InitSessionList();
void DestroySessionList();

BOOL RegisterOnServer(const TCHAR *pszServer, UINT32 zoneUIN);

void UpdatePolicyInventory();

void UpdateUserAgentsEnvironment();

void ParseTunnelList(TCHAR *list);

#if !defined(_WIN32)
void InitStaticSubagents();
#endif

#ifdef _WIN32
extern TCHAR g_windowsServiceName[];
extern TCHAR g_windowsServiceDisplayName[];
#endif

void LIBNXAGENT_EXPORTABLE InitSubAgentAPI(
      void(*writeLog)(int, int, const TCHAR *),
      void(*postEvent1)(UINT32, const TCHAR *, time_t, const char *, va_list),
      void(*postEvent2)(UINT32, const TCHAR *, time_t, int, const TCHAR **),
      bool(*enumerateSessions)(EnumerationCallbackResult(*)(AbstractCommSession *, void *), void*),
      AbstractCommSession *(*findServerSession)(UINT64),
      bool(*sendFile)(void *, UINT32, const TCHAR *, long, bool, VolatileCounter *),
      bool(*pushData)(const TCHAR *, const TCHAR *, UINT32, time_t),
      DB_HANDLE(*getLocalDatabaseHandle)(),
      const TCHAR *dataDirectory,
      void(*executeAction)(const TCHAR *, const StringList *),
      bool(*getScreenInfoForUserSession)(uint32_t, uint32_t *, uint32_t *, uint32_t *));

int CreateConfig(bool forceCreate, const char *masterServers, const char *logFile, const char *fileStore,
   const char *configIncludeDir, int numSubAgents, char **subAgentList, const char *extraValues);

/**
 * OpenSSL APPLINK
 */
#ifdef _WIN32
#include <openssl/applink.c>
#endif

/**
 * Valid options for getopt()
 */
#if defined(_WIN32)
#define VALID_OPTIONS   "B:c:CdD:e:EfFG:hHiIkKlM:n:N:P:r:RsSUvW:X:Z:z:"
#else
#define VALID_OPTIONS   "B:c:CdD:fFg:G:hkKlM:p:P:r:Su:vW:X:Z:z:"
#endif

/**
 * Actions
 */
#define ACTION_NONE                    0
#define ACTION_RUN_AGENT               1
#define ACTION_INSTALL_SERVICE         2
#define ACTION_REMOVE_SERVICE          3
#define ACTION_START_SERVICE           4
#define ACTION_STOP_SERVICE            5
#define ACTION_CHECK_CONFIG            6
#define ACTION_INSTALL_EVENT_SOURCE    7
#define ACTION_REMOVE_EVENT_SOURCE     8
#define ACTION_CREATE_CONFIG           9
#define ACTION_HELP							10
#define ACTION_RUN_WATCHDOG            11
#define ACTION_SHUTDOWN_EXT_AGENTS     12
#define ACTION_GET_LOG_LOCATION        13

/**
 * Global variables
 */
uuid g_agentId;
#ifdef _WIN32
UINT32 g_dwFlags = AF_ENABLE_ACTIONS | AF_ENABLE_AUTOLOAD | AF_WRITE_FULL_DUMP | AF_ENABLE_PUSH_CONNECTOR | AF_ENABLE_CONTROL_CONNECTOR | AF_REQUIRE_ENCRYPTION;
#else
UINT32 g_dwFlags = AF_ENABLE_ACTIONS | AF_ENABLE_AUTOLOAD | AF_ENABLE_PUSH_CONNECTOR | AF_REQUIRE_ENCRYPTION;
#endif
UINT32 g_failFlags = 0;
TCHAR g_szLogFile[MAX_PATH] = AGENT_DEFAULT_LOG;
TCHAR g_szSharedSecret[MAX_SECRET_LENGTH] = _T("admin");
TCHAR g_szConfigFile[MAX_PATH] = AGENT_DEFAULT_CONFIG;
TCHAR g_szFileStore[MAX_PATH] = AGENT_DEFAULT_FILE_STORE;
TCHAR g_szDataDirectory[MAX_PATH] = AGENT_DEFAULT_DATA_DIR;
TCHAR g_dataDirRecoveryPath[MAX_PATH] = _T("");
TCHAR g_szPlatformSuffix[MAX_PSUFFIX_LENGTH] = _T("");
TCHAR g_szConfigServer[MAX_DB_STRING] = _T("not_set");
TCHAR g_szRegistrar[MAX_DB_STRING] = _T("not_set");
TCHAR g_szListenAddress[MAX_PATH] = _T("*");
TCHAR g_szConfigIncludeDir[MAX_PATH] = AGENT_DEFAULT_CONFIG_D;
TCHAR g_szConfigPolicyDir[MAX_PATH] = AGENT_DEFAULT_CONFIG_D;
TCHAR g_szLogParserDirectory[MAX_PATH] = _T("");
TCHAR g_userAgentPolicyDirectory[MAX_PATH] = _T("");
TCHAR g_certificateDirectory[MAX_PATH] = _T("");
TCHAR g_masterAgent[MAX_PATH] = _T("not_set");
TCHAR g_szSNMPTrapListenAddress[MAX_PATH] = _T("*");
UINT16 g_wListenPort = AGENT_LISTEN_PORT;
TCHAR g_systemName[MAX_OBJECT_NAME] = _T("");
ObjectArray<ServerInfo> g_serverList(8, 8, Ownership::True);
UINT32 g_execTimeout = 2000;     // External process execution timeout in milliseconds
UINT32 g_eppTimeout = 30;      // External parameter processor timeout in seconds
UINT32 g_snmpTimeout = 0;
UINT16 g_snmpTrapPort = 162;
time_t g_tmAgentStartTime;
UINT32 g_dwStartupDelay = 0;
UINT32 g_dwMaxSessions = 0;
UINT32 g_longRunningQueryThreshold = 250;
UINT32 g_dcReconciliationBlockSize = 1024;
UINT32 g_dcReconciliationTimeout = 60000;
UINT32 g_dcWriterFlushInterval = 5000;
UINT32 g_dcWriterMaxTransactionSize = 10000;
UINT32 g_dcMaxCollectorPoolSize = 64;
UINT32 g_dcOfflineExpirationTime = 10; // 10 days
UINT32 g_zoneUIN = 0;
UINT32 g_tunnelKeepaliveInterval = 30;
UINT16 g_syslogListenPort = 514;
#ifdef _WIN32
UINT16 g_sessionAgentPort = 28180;
#else
UINT16 g_sessionAgentPort = 0;
#endif
UINT32 g_dwIdleTimeout = 120;   // Session idle timeout

#if !defined(_WIN32)
TCHAR g_szPidFile[MAX_PATH] = _T("/var/run/nxagentd.pid");
#endif

/**
 * Static variables
 */
static TCHAR *m_pszActionList = NULL;
static TCHAR *m_pszShellActionList = NULL;
static TCHAR *m_pszServerList = NULL;
static TCHAR *m_pszControlServerList = NULL;
static TCHAR *m_pszMasterServerList = NULL;
static TCHAR *m_pszSubagentList = NULL;
static TCHAR *s_externalParametersConfig = NULL;
static TCHAR *s_externalShellExecParametersConfig = NULL;
static TCHAR *s_externalParameterProvidersConfig = NULL;
static TCHAR *s_externalListsConfig = NULL;
static TCHAR *s_externalTablesConfig = NULL;
static TCHAR *s_externalSubAgentsList = NULL;
static TCHAR *s_appAgentsList = NULL;
static TCHAR *s_serverConnectionList = NULL;
static UINT32 s_enabledCiphers = 0xFFFF;
static THREAD s_sessionWatchdogThread = INVALID_THREAD_HANDLE;
static THREAD s_listenerThread = INVALID_THREAD_HANDLE;
static THREAD s_eventSenderThread = INVALID_THREAD_HANDLE;
static THREAD s_snmpTrapReceiverThread = INVALID_THREAD_HANDLE;
static THREAD s_snmpTrapSenderThread = INVALID_THREAD_HANDLE;
static THREAD s_syslogReceiverThread = INVALID_THREAD_HANDLE;
static THREAD s_syslogSenderThread = INVALID_THREAD_HANDLE;
static THREAD s_masterAgentListenerThread = INVALID_THREAD_HANDLE;
static THREAD s_tunnelManagerThread = INVALID_THREAD_HANDLE;
static TCHAR s_processToWaitFor[MAX_PATH] = _T("");
static TCHAR s_dumpDir[MAX_PATH] = _T("C:\\");
static UINT64 s_maxLogSize = 16384 * 1024;
static UINT32 s_logHistorySize = 4;
static UINT32 s_logRotationMode = NXLOG_ROTATION_BY_SIZE;
static TCHAR s_dailyLogFileSuffix[64] = _T("");
static TCHAR s_executableName[MAX_PATH];
static UINT32 s_debugLevel = (UINT32)NXCONFIG_UNINITIALIZED_VALUE;
static TCHAR *s_debugTags = NULL;

#if defined(_WIN32)
static CONDITION s_shutdownCondition = INVALID_CONDITION_HANDLE;
#endif

#if !defined(_WIN32)
static pid_t s_pid;
#endif

/**
 * Configuration file template
 */
static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
   { _T("Action"), CT_STRING_LIST, '\n', 0, 0, 0, &m_pszActionList, NULL },
   { _T("ActionShellExec"), CT_STRING_LIST, '\n', 0, 0, 0, &m_pszShellActionList, NULL },
   { _T("AppAgent"), CT_STRING_LIST, '\n', 0, 0, 0, &s_appAgentsList, NULL },
   { _T("BackgroundLogWriter"), CT_BOOLEAN, 0, 0, AF_BACKGROUND_LOG_WRITER, 0, &g_dwFlags, NULL },
   { _T("ControlServers"), CT_STRING_LIST, ',', 0, 0, 0, &m_pszControlServerList, NULL },
   { _T("CreateCrashDumps"), CT_BOOLEAN, 0, 0, AF_CATCH_EXCEPTIONS, 0, &g_dwFlags, NULL },
   { _T("DataCollectionThreadPoolSize"), CT_LONG, 0, 0, 0, 0, &g_dcMaxCollectorPoolSize, NULL },
   { _T("DataReconciliationBlockSize"), CT_LONG, 0, 0, 0, 0, &g_dcReconciliationBlockSize, NULL },
   { _T("DataReconciliationTimeout"), CT_LONG, 0, 0, 0, 0, &g_dcReconciliationTimeout, NULL },
   { _T("DataWriterFlushInterval"), CT_LONG, 0, 0, 0, 0, &g_dcWriterFlushInterval, NULL },
   { _T("DataWriterMaxTransactionSize"), CT_LONG, 0, 0, 0, 0, &g_dcWriterMaxTransactionSize, NULL },
   { _T("DailyLogFileSuffix"), CT_STRING, 0, 0, 64, 0, s_dailyLogFileSuffix, NULL },
   { _T("DebugLevel"), CT_LONG, 0, 0, 0, 0, &s_debugLevel, &s_debugLevel },
   { _T("DebugTags"), CT_STRING_LIST, ',', 0, 0, 0, &s_debugTags, NULL },
   { _T("DisableIPv4"), CT_BOOLEAN, 0, 0, AF_DISABLE_IPV4, 0, &g_dwFlags, NULL },
   { _T("DisableIPv6"), CT_BOOLEAN, 0, 0, AF_DISABLE_IPV6, 0, &g_dwFlags, NULL },
   { _T("DumpDirectory"), CT_STRING, 0, 0, MAX_PATH, 0, s_dumpDir, NULL },
   { _T("EnableActions"), CT_BOOLEAN, 0, 0, AF_ENABLE_ACTIONS, 0, &g_dwFlags, NULL },
   { _T("EnabledCiphers"), CT_LONG, 0, 0, 0, 0, &s_enabledCiphers, NULL },
   { _T("EnableControlConnector"), CT_BOOLEAN, 0, 0, AF_ENABLE_CONTROL_CONNECTOR, 0, &g_dwFlags, NULL },
   { _T("EnableProxy"), CT_BOOLEAN, 0, 0, AF_ENABLE_PROXY, 0, &g_dwFlags, NULL },
   { _T("EnablePushConnector"), CT_BOOLEAN, 0, 0, AF_ENABLE_PUSH_CONNECTOR, 0, &g_dwFlags, NULL },
   { _T("EnableSNMPProxy"), CT_BOOLEAN, 0, 0, AF_ENABLE_SNMP_PROXY, 0, &g_dwFlags, NULL },
   { _T("EnableSNMPTrapProxy"), CT_BOOLEAN, 0, 0, AF_ENABLE_SNMP_TRAP_PROXY, 0, &g_dwFlags, NULL },
   { _T("EnableSSLTrace"), CT_BOOLEAN, 0, 0, AF_ENABLE_SSL_TRACE, 0, &g_dwFlags, NULL },
   { _T("EnableSyslogProxy"), CT_BOOLEAN, 0, 0, AF_ENABLE_SYSLOG_PROXY, 0, &g_dwFlags, NULL },
   { _T("EnableSubagentAutoload"), CT_BOOLEAN, 0, 0, AF_ENABLE_AUTOLOAD, 0, &g_dwFlags, NULL },
   { _T("EnableTCPProxy"), CT_BOOLEAN, 0, 0, AF_ENABLE_TCP_PROXY, 0, &g_dwFlags, NULL },
   { _T("EnableWatchdog"), CT_BOOLEAN, 0, 0, AF_ENABLE_WATCHDOG, 0, &g_dwFlags, NULL },
   { _T("EncryptedSharedSecret"), CT_STRING, 0, 0, MAX_SECRET_LENGTH, 0, g_szSharedSecret, NULL },
   { _T("ExecTimeout"), CT_LONG, 0, 0, 0, 0, &g_execTimeout, NULL },
   { _T("ExternalList"), CT_STRING_LIST, '\n', 0, 0, 0, &s_externalListsConfig, NULL },
   { _T("ExternalMasterAgent"), CT_STRING, 0, 0, MAX_PATH, 0, g_masterAgent, NULL },
   { _T("ExternalParameter"), CT_STRING_LIST, '\n', 0, 0, 0, &s_externalParametersConfig, NULL },
   { _T("ExternalParameterShellExec"), CT_STRING_LIST, '\n', 0, 0, 0, &s_externalShellExecParametersConfig, NULL },
   { _T("ExternalParametersProvider"), CT_STRING_LIST, '\n', 0, 0, 0, &s_externalParameterProvidersConfig, NULL },
   { _T("ExternalParameterProviderTimeout"), CT_LONG, 0, 0, 0, 0, &g_eppTimeout, NULL },
   { _T("ExternalSubagent"), CT_STRING_LIST, '\n', 0, 0, 0, &s_externalSubAgentsList, NULL },
   { _T("ExternalTable"), CT_STRING_LIST, '\n', 0, 0, 0, &s_externalTablesConfig, NULL },
   { _T("FileStore"), CT_STRING, 0, 0, MAX_PATH, 0, g_szFileStore, NULL },
   { _T("FullCrashDumps"), CT_BOOLEAN, 0, 0, AF_WRITE_FULL_DUMP, 0, &g_dwFlags, NULL },
   { _T("ListenAddress"), CT_STRING, 0, 0, MAX_PATH, 0, g_szListenAddress, NULL },
   { _T("ListenPort"), CT_WORD, 0, 0, 0, 0, &g_wListenPort, NULL },
   { _T("LogFile"), CT_STRING, 0, 0, MAX_PATH, 0, g_szLogFile, NULL },
   { _T("LogHistorySize"), CT_LONG, 0, 0, 0, 0, &s_logHistorySize, NULL },
   { _T("LogRotationMode"), CT_LONG, 0, 0, 0, 0, &s_logRotationMode, NULL },
   { _T("LogUnresolvedSymbols"), CT_BOOLEAN, 0, 0, AF_LOG_UNRESOLVED_SYMBOLS, 0, &g_dwFlags, NULL },
   { _T("LongRunningQueryThreshold"), CT_LONG, 0, 0, 0, 0, &g_longRunningQueryThreshold, NULL },
   { _T("MasterServers"), CT_STRING_LIST, ',', 0, 0, 0, &m_pszMasterServerList, NULL },
   { _T("MaxLogSize"), CT_SIZE_BYTES, 0, 0, 0, 0, &s_maxLogSize, NULL },
   { _T("MaxSessions"), CT_LONG, 0, 0, 0, 0, &g_dwMaxSessions, NULL },
   { _T("OfflineDataExpirationTime"), CT_LONG, 0, 0, 0, 0, &g_dcOfflineExpirationTime, NULL },
   { _T("PlatformSuffix"), CT_STRING, 0, 0, MAX_PSUFFIX_LENGTH, 0, g_szPlatformSuffix, NULL },
   { _T("RequireAuthentication"), CT_BOOLEAN, 0, 0, AF_REQUIRE_AUTH, 0, &g_dwFlags, NULL },
   { _T("RequireEncryption"), CT_BOOLEAN, 0, 0, AF_REQUIRE_ENCRYPTION, 0, &g_dwFlags, NULL },
   { _T("ServerConnection"), CT_STRING_LIST, '\n', 0, 0, 0, &s_serverConnectionList, NULL },
   { _T("Servers"), CT_STRING_LIST, ',', 0, 0, 0, &m_pszServerList, NULL },
   { _T("SessionIdleTimeout"), CT_LONG, 0, 0, 0, 0, &g_dwIdleTimeout, NULL },
   { _T("SessionAgentPort"), CT_WORD, 0, 0, 0, 0, &g_sessionAgentPort, NULL },
   { _T("SharedSecret"), CT_STRING, 0, 0, MAX_SECRET_LENGTH, 0, g_szSharedSecret, NULL },
	{ _T("SNMPTimeout"), CT_LONG, 0, 0, 0, 0, &g_snmpTimeout, NULL },
	{ _T("SNMPTrapListenAddress"), CT_STRING, 0, 0, MAX_PATH, 0, &g_szSNMPTrapListenAddress, NULL },
   { _T("SNMPTrapPort"), CT_WORD, 0, 0, 0, 0, &g_snmpTrapPort, NULL },
   { _T("StartupDelay"), CT_LONG, 0, 0, 0, 0, &g_dwStartupDelay, NULL },
   { _T("SubAgent"), CT_STRING_LIST, '\n', 0, 0, 0, &m_pszSubagentList, NULL },
   { _T("SyslogListenPort"), CT_WORD, 0, 0, 0, 0, &g_syslogListenPort, NULL },
   { _T("SystemName"), CT_STRING, 0, 0, MAX_OBJECT_NAME, 0, g_systemName, NULL },
   { _T("TunnelKeepaliveInterval"), CT_LONG, 0, 0, 0, 0, &g_tunnelKeepaliveInterval, NULL },
   { _T("WaitForProcess"), CT_STRING, 0, 0, MAX_PATH, 0, s_processToWaitFor, NULL },
   { _T("WriteLogAsJson"), CT_BOOLEAN, 0, 0, AF_JSON_LOG, 0, &g_dwFlags, NULL },
   { _T("ZoneId"), CT_LONG, 0, 0, 0, 0, &g_zoneUIN, NULL }, // for backward compatibility
   { _T("ZoneUIN"), CT_LONG, 0, 0, 0, 0, &g_zoneUIN, NULL },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL, NULL }
};

/**
 * Help text
 */
static TCHAR m_szHelpText[] =
   _T("Usage: nxagentd [options]\n")
   _T("Where valid options are:\n")
   _T("   -B <uin>   : Set zone UIN\n")
   _T("   -c <file>  : Use configuration file <file> (default ") AGENT_DEFAULT_CONFIG _T(")\n")
   _T("   -C         : Load configuration file, dump resulting configuration, and exit\n")
   _T("   -d         : Run as daemon/service\n")
	_T("   -D <level> : Set debug level (0..9)\n")
#ifdef _WIN32
   _T("   -e <name>  : Windows event source name\n")
#endif
	_T("   -f         : Run in foreground\n")
#if !defined(_WIN32)
   _T("   -g <gid>   : Change group ID to <gid> after start\n")
#endif
   _T("   -G <name>  : Use alternate global section <name> in configuration file\n")
   _T("   -h         : Display help and exit\n")
#ifdef _WIN32
   _T("   -H         : Hide agent's window when in standalone mode\n")
	_T("   -i         : Installed Windows service must be interactive\n")
   _T("   -I         : Install Windows service\n")
#endif
   _T("   -K         : Shutdown all connected external sub-agents\n")
   _T("   -l         : Get log file location\n")
   _T("   -M <addr>  : Download config from management server <addr>\n")
#ifdef _WIN32
   _T("   -n <name>  : Service name\n")
   _T("   -N <name>  : Service display name\n")
#endif
#if !defined(_WIN32)
   _T("   -p         : Path to pid file (default: /var/run/nxagentd.pid)\n")
#endif
   _T("   -P <text>  : Set platform suffix to <text>\n")
   _T("   -r <addr>  : Register agent on management server <addr>\n")
#ifdef _WIN32
   _T("   -R         : Remove Windows service\n")
   _T("   -s         : Start Windows servive\n")
   _T("   -S         : Stop Windows service\n")
#else
#if WITH_SYSTEMD
   _T("   -S         : Run as systemd daemon\n")
#endif
   _T("   -u <uid>   : Change user ID to <uid> after start\n")
#endif
   _T("   -v         : Display version and exit\n")
   _T("\n");

/**
 * Server info: constructor
 */
ServerInfo::ServerInfo(const TCHAR *name, bool control, bool master)
{
#ifdef UNICODE
   m_name = MBStringFromWideString(name);
#else
   m_name = strdup(name);
#endif

   char *p = strchr(m_name, '/');
   if (p != NULL)
   {
      *p = 0;
      p++;
      m_address = InetAddress::resolveHostName(m_name);
      if (m_address.isValid())
      {
         int bits = strtol(p, NULL, 10);
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
   m_lastResolveTime = time(NULL);
   m_mutex = MutexCreate();
}

/**
 * Server info: destructor
 */
ServerInfo::~ServerInfo()
{
   MemFree(m_name);
   MutexDestroy(m_mutex);
}

/**
 * Server info: resolve hostname if needed
 */
void ServerInfo::resolve(bool forceResolve)
{
   time_t now = time(NULL);
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
   MutexLock(m_mutex);
   if (m_redoResolve)
      resolve(forceResolve);
   bool result = m_address.isValid() ? m_address.contain(addr) : false;
   MutexUnlock(m_mutex);
   return result;
}

#ifdef _WIN32

/**
 * Get our own console window handle (an alternative to Microsoft's GetConsoleWindow)
 */
static HWND GetConsoleHWND()
{
   DWORD cpid = GetCurrentProcessId();
	HWND hWnd = NULL;
   while(true)
   {
	   hWnd = FindWindowEx(NULL, hWnd, _T("ConsoleWindowClass"), NULL);
      if (hWnd == NULL)
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
   FARPROC ptr;

   ptr = GetProcAddress(hModule, procName);
   if ((ptr == NULL) && (g_dwFlags & AF_LOG_UNRESOLVED_SYMBOLS))
      nxlog_write(NXLOG_WARNING, _T("Unable to resolve symbol \"%s\""), procName);
   return ptr;
}

/**
 * Shutdown thread (created by H_RestartAgent)
 */
static THREAD_RESULT THREAD_CALL ShutdownThread(void *pArg)
{
	DebugPrintf(1, _T("Shutdown thread started"));
   Shutdown();
   ExitProcess(0);
   return THREAD_OK; // Never reached
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
   command.append(_T("\" -D "));
   command.append(s_debugLevel);

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
      command.append((g_dwFlags & AF_DAEMON) ? 0 : static_cast<UINT32>(GetCurrentProcessId()));
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
   if (!CreateProcess(NULL, command.getBuffer(), NULL, NULL, FALSE,
                      (g_dwFlags & AF_DAEMON) ? (CREATE_NO_WINDOW | DETACHED_PROCESS) : (CREATE_NEW_CONSOLE),
                      NULL, NULL, &si, &pi))
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
         ConditionSet(s_shutdownCondition);
      }
      else
      {
         ThreadCreate(ShutdownThread, 0, NULL);
      }
   }
   return dwResult;
#else
	nxlog_debug(1, _T("Restarting agent with command line '%s'"), command.cstr());
   return ExecuteCommand(command.getBuffer(), NULL, NULL);
#endif
}

/**
 * Handler for Agent.Restart action
 */
static LONG H_RestartAgent(const TCHAR *action, const StringList *args, const TCHAR *data, AbstractCommSession *session)
{
   return RestartAgent();
}

/**
 * Handler for System.Execute action
 */
static LONG H_SystemExecute(const TCHAR *action, const StringList *args, const TCHAR *data, AbstractCommSession *session)
{
   return ERR_NOT_IMPLEMENTED;
}

#ifdef _WIN32

/**
 * Handler for System.ExecuteInAllSessions action
 */
static LONG H_SystemExecuteInAllSessions(const TCHAR *action, const StringList *args, const TCHAR *data, AbstractCommSession *session)
{
   if (args->isEmpty())
      return ERR_BAD_ARGUMENTS;
   StringBuffer command;
   for (int i = 0; i < args->size(); i++)
   {
      command.append(command.isEmpty() ? _T("\"") : _T(" \""));
      command.append(args->get(i));
      command.append(_T("\""));
   }
   return ExecuteInAllSessions(command) ? ERR_SUCCESS : ERR_EXEC_FAILED;
}

#endif

/**
 * This function writes message from subagent to agent's log
 */
static void WriteSubAgentMsg(int logLevel, int debugLevel, const TCHAR *message)
{
	if (logLevel == NXLOG_DEBUG)
	   nxlog_debug(debugLevel, message);
	else
		nxlog_write(logLevel, _T("%s"), message);
}

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
	sigprocmask(SIG_UNBLOCK, &signals, NULL);
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
 * Send file to server (subagent API)
 */
static bool SendFileToServer(void *session, UINT32 requestId, const TCHAR *file, long offset, bool allowCompression, VolatileCounter *cancellationFlag)
{
	if (session == NULL)
		return false;
	return ((CommSession *)session)->sendFile(requestId, file, offset, allowCompression, cancellationFlag);
}

/**
 * Configure agent directory: construct directory name and create it if needed
 */
static void ConfigureAgentDirectory(TCHAR *fullName, const TCHAR *suffix, const TCHAR *contentDescription)
{
   TCHAR tail = g_szDataDirectory[_tcslen(g_szDataDirectory) - 1];
   _sntprintf(fullName, MAX_PATH, _T("%s%s%s") FS_PATH_SEPARATOR, g_szDataDirectory,
              ((tail != '\\') && (tail != '/')) ? FS_PATH_SEPARATOR : _T(""),
              suffix);
   CreateFolder(fullName);
   nxlog_debug(2, _T("%s directory: %s"), contentDescription, fullName);
}

/**
 * Parser server list
 */
static void ParseServerList(TCHAR *serverList, bool isControl, bool isMaster)
{
	TCHAR *pItem, *pEnd;

	for(pItem = pEnd = serverList; pEnd != NULL && *pItem != 0; pItem = pEnd + 1)
	{
		pEnd = _tcschr(pItem, _T(','));
		if (pEnd != NULL)
			*pEnd = 0;
		StrStrip(pItem);
		if (*pItem != 0)
		{
         g_serverList.add(new ServerInfo(pItem, isControl, isMaster));
         nxlog_debug(3, _T("Added server access record %s (control=%s, master=%s)"), pItem,
                  isControl ? _T("true") : _T("false"), isMaster ? _T("true") : _T("false"));
		}
	}
	MemFree(serverList);
}

/**
 * Get log destination flags
 */
static inline UINT32 GetLogDestinationFlag()
{
   if (g_dwFlags & AF_USE_SYSLOG)
      return NXLOG_USE_SYSLOG;
   if (g_dwFlags & AF_USE_SYSTEMD_JOURNAL)
      return NXLOG_USE_SYSTEMD;
   if (g_dwFlags & AF_LOG_TO_STDOUT)
      return NXLOG_USE_STDOUT;
   return 0;
}

/**
 * Agent initialization
 */
BOOL Initialize()
{
   TCHAR *pItem, *pEnd;

   if (s_debugLevel == (UINT32)NXCONFIG_UNINITIALIZED_VALUE)
      s_debugLevel = 0;

   // Open log file
   if (!nxlog_open((g_dwFlags & AF_USE_SYSLOG) ? NXAGENTD_SYSLOG_NAME : g_szLogFile,
                   GetLogDestinationFlag() |
	                ((g_dwFlags & AF_BACKGROUND_LOG_WRITER) ? NXLOG_BACKGROUND_WRITER : 0) |
                   ((g_dwFlags & AF_DAEMON) ? 0 : NXLOG_PRINT_TO_STDOUT) |
                   ((g_dwFlags & AF_JSON_LOG) ? NXLOG_JSON_FORMAT : 0)))
	{
	   s_debugLevel = 1;
	   g_failFlags |= FAIL_OPEN_LOG;
      nxlog_open(NXAGENTD_SYSLOG_NAME, NXLOG_USE_SYSLOG |
	               ((g_dwFlags & AF_BACKGROUND_LOG_WRITER) ? NXLOG_BACKGROUND_WRITER : 0) |
                  ((g_dwFlags & AF_DAEMON) ? 0 : NXLOG_PRINT_TO_STDOUT));
		_ftprintf(stderr, _T("ERROR: Cannot open log file \"%s\", logs will be written to syslog with debug level 1\n"), g_szLogFile);
	}
	else
   {
      if (!(g_dwFlags & (AF_USE_SYSLOG | AF_USE_SYSTEMD_JOURNAL)))
      {
         if (!nxlog_set_rotation_policy((int)s_logRotationMode, s_maxLogSize, (int)s_logHistorySize, s_dailyLogFileSuffix))
            if (!(g_dwFlags & AF_DAEMON))
               _tprintf(_T("WARNING: cannot set log rotation policy; using default values\n"));
      }
   }
	nxlog_write(NXLOG_INFO, _T("Core agent version ") NETXMS_BUILD_TAG);
	nxlog_write(NXLOG_INFO, _T("Additional configuration files was loaded from %s"), g_szConfigIncludeDir);
	nxlog_write_tag(NXLOG_INFO, _T("logger"), _T("Debug level set to %d"), s_debugLevel);

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

	nxlog_set_debug_level(s_debugLevel);

	if (_tcscmp(g_masterAgent, _T("not_set")))
	{
		g_dwFlags |= AF_SUBAGENT_LOADER;
		nxlog_write(NXLOG_INFO, _T("Switched to external subagent loader mode, master agent address is %s"), g_masterAgent);
	}

   if (g_dataDirRecoveryPath[0] != 0)
      nxlog_write(NXLOG_INFO, _T("Data directory recovered from %s"), g_dataDirRecoveryPath);
	nxlog_write(NXLOG_INFO, _T("Data directory: %s"), g_szDataDirectory);
   CreateFolder(g_szDataDirectory);

   nxlog_write(NXLOG_INFO, _T("File store: %s"), g_szFileStore);
   CreateFolder(g_szFileStore);
   SetEnvironmentVariable(_T("NETXMS_FILE_STORE"), g_szFileStore);

#ifndef _WIN32
   nxlog_debug(2, _T("Effective user ID %d"), (int)geteuid());
   nxlog_debug(2, _T("Effective group ID %d"), (int)getegid());
#endif

   nxlog_debug(2, _T("Configuration policy directory: %s"), g_szConfigPolicyDir);

   ConfigureAgentDirectory(g_szLogParserDirectory, LOGPARSER_AP_FOLDER, _T("Log parser policy"));
   ConfigureAgentDirectory(g_userAgentPolicyDirectory, USERAGENT_AP_FOLDER, _T("User agent policy"));
   ConfigureAgentDirectory(g_certificateDirectory, CERTIFICATES_FOLDER, _T("Certificate"));

#ifdef _WIN32
   WSADATA wsaData;
	int wrc = WSAStartup(MAKEWORD(2, 2), &wsaData);
   if (wrc != 0)
   {
      TCHAR buffer[1024];
      nxlog_write(NXLOG_ERROR, _T("Call to WSAStartup() failed (%s)"), GetLastSocketErrorText(buffer, 1024));
      return FALSE;
   }
#endif

   // Initialize API for subagents
   InitSubAgentAPI(WriteSubAgentMsg, PostEvent, PostEvent, EnumerateSessions, FindServerSessionByServerId,
         SendFileToServer, PushData, GetLocalDatabaseHandle, g_szDataDirectory, ExecuteAction,
         GetScreenInfoForUserSession);
   nxlog_debug(1, _T("Subagent API initialized"));

   g_executorThreadPool = ThreadPoolCreate(_T("PROCEXEC"), 1, 32);

   if (!InitCryptoLib(s_enabledCiphers))
   {
      nxlog_write(NXLOG_ERROR, _T("Failed to initialize cryptografy module"));
      return FALSE;
   }

   // Initialize libssl - it is not used by core agent
   // but may be needed by some subagents. Allowing first load of libssl by
   // subagent via dlopen() may lead to undesired side effects
#ifdef _WITH_ENCRYPTION
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   OPENSSL_init_ssl(0, NULL);
#else
   SSL_library_init();
   SSL_load_error_strings();
#endif
#else /* not _WITH_ENCRYPTION */
   if ((g_dwFlags & AF_REQUIRE_ENCRYPTION) && !(g_dwFlags & AF_SUBAGENT_LOADER))
   {
      nxlog_write(NXLOG_ERROR, _T("Encryption set as required in configuration but agent was build without encryption support"));
      return false;
   }
#endif

   DBInit();
   if (!OpenLocalDatabase())
   {
      nxlog_write(NXLOG_ERROR, _T("Unable to open local database"));
   }

   TCHAR agentIdText[MAX_DB_STRING];
   if (ReadMetadata(_T("AgentId"), agentIdText) != NULL)
      g_agentId = uuid::parse(agentIdText);
   if (g_agentId.isNull())
   {
      g_agentId = uuid::generate();
      WriteMetadata(_T("AgentId"), g_agentId.toString(agentIdText));
      nxlog_debug(1, _T("New agent ID generated"));
   }
   nxlog_write(NXLOG_INFO, _T("Agent ID is %s"), g_agentId.toString(agentIdText));

   TCHAR hostname[256];
   if (GetLocalHostName(hostname, 256, true) == NULL)
   {
      // Fallback to non-FQDN host name
      GetLocalHostName(hostname, 256, false);
   }
   nxlog_write(NXLOG_INFO, _T("Local host name is \"%s\""), hostname);

   // Set system name to host name if not set in config
   if (g_systemName[0] == 0)
      GetLocalHostName(g_systemName, MAX_OBJECT_NAME, false);
   nxlog_write(NXLOG_INFO, _T("Using system name \"%s\""), g_systemName);

   shared_ptr<Config> config = g_config;

	if (!(g_dwFlags & AF_SUBAGENT_LOADER))
	{
	   g_commThreadPool = ThreadPoolCreate(_T("COMM"), 1, 32);
	   if (g_dwFlags & AF_ENABLE_SNMP_PROXY)
	   {
	      g_snmpProxyThreadPool = ThreadPoolCreate(_T("SNMPPROXY"), 1, 128);
	   }
	   InitSessionList();

		// Initialize built-in parameters
		if (!InitParameterList())
			return FALSE;

		// Parse outgoing server connection (tunnel) list
      if (s_serverConnectionList != NULL)
         ParseTunnelList(s_serverConnectionList);

		// Parse server lists
		if (m_pszMasterServerList != NULL)
			ParseServerList(m_pszMasterServerList, true, true);
		if (m_pszControlServerList != NULL)
			ParseServerList(m_pszControlServerList, true, false);
		if (m_pszServerList != NULL)
			ParseServerList(m_pszServerList, false, false);

		// Add built-in actions
		AddAction(_T("Agent.Restart"), AGENT_ACTION_SUBAGENT, NULL, H_RestartAgent, _T("CORE"), _T("Restart agent"));
      if (config->getValueAsBoolean(_T("/CORE/EnableArbitraryCommandExecution"), false))
      {
         nxlog_write(NXLOG_INFO, _T("Arbitrary command execution enabled"));
         AddAction(_T("System.Execute"), AGENT_ACTION_SUBAGENT, NULL, H_SystemExecute, _T("CORE"), _T("Execute system command"));
#ifdef _WIN32
         AddAction(_T("System.ExecuteInAllSessions"), AGENT_ACTION_SUBAGENT, NULL, H_SystemExecuteInAllSessions, _T("CORE"), _T("Execute system command in all sessions"));
#endif
      }
      else
      {
         nxlog_write(NXLOG_INFO, _T("Arbitrary command execution disabled"));
      }

	   // Load platform subagents
#if !defined(_WIN32)
		InitStaticSubagents();
#endif
		if (g_dwFlags & AF_ENABLE_AUTOLOAD)
		{
		   LoadPlatformSubagent();
		}
	}

	// Wait for external process if requested
	if (s_processToWaitFor[0] != 0)
	{
	   DebugPrintf(1, _T("Waiting for process %s"), s_processToWaitFor);
		if (!WaitForProcess(s_processToWaitFor))
	      nxlog_write(NXLOG_ERROR, _T("Call to WaitForProcess failed for process %s"), s_processToWaitFor);
	}

	// Load other subagents
   if (m_pszSubagentList != NULL)
   {
      for(pItem = pEnd = m_pszSubagentList; pEnd != NULL && *pItem != 0; pItem = pEnd + 1)
      {
         pEnd = _tcschr(pItem, _T('\n'));
         if (pEnd != NULL)
            *pEnd = 0;
         Trim(pItem);
         LoadSubAgent(pItem);
      }
      MemFree(m_pszSubagentList);
   }

   // Parse action list
   if (m_pszActionList != NULL)
   {
      for(pItem = pEnd = m_pszActionList; pEnd != NULL && (*pItem != 0); pItem = pEnd + 1)
      {
         pEnd = _tcschr(pItem, _T('\n'));
         if (pEnd != NULL)
            *pEnd = 0;
         Trim(pItem);
         if (!AddActionFromConfig(pItem, FALSE))
            nxlog_write(NXLOG_WARNING, _T("Unable to add action \"%s\""), pItem);
      }
      MemFree(m_pszActionList);
   }
   if (m_pszShellActionList != NULL)
   {
      for(pItem = pEnd = m_pszShellActionList; pEnd != NULL && (*pItem != 0); pItem = pEnd + 1)
      {
         pEnd = _tcschr(pItem, _T('\n'));

         if (pEnd != NULL)
            *pEnd = 0;
         Trim(pItem);
         if (!AddActionFromConfig(pItem, TRUE))
            nxlog_write(NXLOG_WARNING, _T("Unable to add action \"%s\""), pItem);
      }
      MemFree(m_pszShellActionList);
   }

   // Parse external parameters list
   if (s_externalParametersConfig != NULL)
   {
      for(pItem = pEnd = s_externalParametersConfig; pEnd != NULL && *pItem != 0; pItem = pEnd + 1)
      {
         pEnd = _tcschr(pItem, _T('\n'));
         if (pEnd != NULL)
            *pEnd = 0;
         Trim(pItem);
         if (!AddExternalParameter(pItem, FALSE, FALSE))
            nxlog_write(NXLOG_WARNING, _T("Unable to add external parameter \"%s\""), pItem);
      }
      MemFree(s_externalParametersConfig);
   }
   if (s_externalShellExecParametersConfig != NULL)
   {
      for(pItem = pEnd = s_externalShellExecParametersConfig; pEnd != NULL && *pItem != 0; pItem = pEnd + 1)
      {
         pEnd = _tcschr(pItem, _T('\n'));
         if (pEnd != NULL)
            *pEnd = 0;
         Trim(pItem);
         if (!AddExternalParameter(pItem, TRUE, FALSE))
            nxlog_write(NXLOG_WARNING, _T("Unable to add external parameter \"%s\""), pItem);
      }
      MemFree(s_externalShellExecParametersConfig);
   }

   // Parse external lists
   if (s_externalListsConfig != NULL)
   {
      for(pItem = pEnd = s_externalListsConfig; pEnd != NULL && *pItem != 0; pItem = pEnd + 1)
      {
         pEnd = _tcschr(pItem, _T('\n'));
         if (pEnd != NULL)
            *pEnd = 0;
         Trim(pItem);
         if (!AddExternalParameter(pItem, FALSE, TRUE))
            nxlog_write(NXLOG_WARNING, _T("Unable to add external list \"%s\""), pItem);
      }
      MemFree(s_externalListsConfig);
   }

   // Parse external tables
   if (s_externalTablesConfig != NULL)
   {
      for(pItem = pEnd = s_externalTablesConfig; pEnd != NULL && *pItem != 0; pItem = pEnd + 1)
      {
         pEnd = _tcschr(pItem, _T('\n'));
         if (pEnd != NULL)
            *pEnd = 0;
         Trim(pItem);
         if (!AddExternalTable(pItem, false))
            nxlog_write(NXLOG_WARNING, _T("Unable to add external table \"%s\""), pItem);
      }
      MemFree(s_externalTablesConfig);
   }

   // Parse external parameters providers list
   if (s_externalParameterProvidersConfig != NULL)
   {
      for(pItem = pEnd = s_externalParameterProvidersConfig; pEnd != NULL && *pItem != 0; pItem = pEnd + 1)
      {
         pEnd = _tcschr(pItem, _T('\n'));
         if (pEnd != NULL)
            *pEnd = 0;
         Trim(pItem);
         if (!AddParametersProvider(pItem))
            nxlog_write(NXLOG_WARNING, _T("Unable to add external parameters provider \"%s\""), pItem);
      }
      MemFree(s_externalParameterProvidersConfig);
   }

	if (!(g_dwFlags & AF_SUBAGENT_LOADER))
	{
      // Parse external subagents list
	   if (!(g_dwFlags & AF_SUBAGENT_LOADER) && (s_externalSubAgentsList != NULL))
      {
         for(pItem = pEnd = s_externalSubAgentsList; pEnd != NULL && *pItem != 0; pItem = pEnd + 1)
         {
            pEnd = _tcschr(pItem, _T('\n'));
            if (pEnd != NULL)
               *pEnd = 0;
            Trim(pItem);
            if (!AddExternalSubagent(pItem))
               nxlog_write(NXLOG_WARNING, _T("Unable to add external subagent \"%s\""), pItem);
         }
         MemFree(s_externalSubAgentsList);
      }

      // Additional external subagents implicitly defined by EXT:* config sections
      ObjectArray<ConfigEntry> *entries = config->getSubEntries(_T("/"), _T("EXT:*"));
      for(int i = 0; i < entries->size(); i++)
      {
         const TCHAR *name = entries->get(i)->getName() + 4;
         if (!AddExternalSubagent(name))
            nxlog_write(NXLOG_WARNING, _T("Unable to add external subagent \"%s\""), name);
      }
      delete entries;

      // Parse application agents list
	   if (!(g_dwFlags & AF_SUBAGENT_LOADER) && (s_appAgentsList != NULL))
      {
         for(pItem = pEnd = s_appAgentsList; pEnd != NULL && *pItem != 0; pItem = pEnd + 1)
         {
            pEnd = _tcschr(pItem, _T('\n'));
            if (pEnd != NULL)
               *pEnd = 0;
            Trim(pItem);
            RegisterApplicationAgent(pItem);
         }
         MemFree(s_appAgentsList);
      }
   }

   ThreadSleep(1);

   // If StartupDelay is greater than zero, then wait
   if (g_dwStartupDelay > 0)
   {
      if (g_dwFlags & AF_DAEMON)
      {
         ThreadSleep(g_dwStartupDelay);
      }
      else
      {
         UINT32 i;

         _tprintf(_T("XXXXXX%*s]\rWAIT ["), g_dwStartupDelay, _T(" "));
         fflush(stdout);
         for(i = 0; i < g_dwStartupDelay; i++)
         {
            ThreadSleep(1);
            _puttc(_T('.'), stdout);
            fflush(stdout);
         }
         _tprintf(_T("\n"));
      }
   }

	StartParamProvidersPoller();

   // Agent start time
   g_tmAgentStartTime = time(NULL);

	s_eventSenderThread = ThreadCreateEx(EventSender, 0, NULL);

	if (g_dwFlags & AF_ENABLE_SNMP_TRAP_PROXY)
	{
      s_snmpTrapSenderThread = ThreadCreateEx(SNMPTrapSender, 0, NULL);
      s_snmpTrapReceiverThread = ThreadCreateEx(SNMPTrapReceiver, 0, NULL);
   }

   if (g_dwFlags & AF_ENABLE_SYSLOG_PROXY)
   {
      s_syslogSenderThread = ThreadCreateEx(SyslogSender, 0, NULL);
      s_syslogReceiverThread = ThreadCreateEx(SyslogReceiver, 0, NULL);
   }

	if (g_dwFlags & AF_SUBAGENT_LOADER)
	{
		s_masterAgentListenerThread = ThreadCreateEx(MasterAgentListener, 0, NULL);
	}
	else
	{
      LoadUserAgentNotifications();
      StartLocalDataCollector();
      if (g_wListenPort != 0)
      {
         s_listenerThread = ThreadCreateEx(ListenerThread, 0, NULL);
         s_sessionWatchdogThread = ThreadCreateEx(SessionWatchdog, 0, NULL);
      }
      else
      {
         nxlog_write(NXLOG_INFO, _T("TCP listener is disabled"));
      }
		StartPushConnector();
      StartSessionAgentConnector();
      if (g_dwFlags & AF_ENABLE_CONTROL_CONNECTOR)
	   {
         StartControlConnector();
      }

   	if (g_dwFlags & AF_REGISTER)
      {
         RegisterOnServer(g_szRegistrar, g_zoneUIN);
      }

      s_tunnelManagerThread = ThreadCreateEx(TunnelManager, 0, NULL);
	}

#ifdef _WIN32
   s_shutdownCondition = ConditionCreate(TRUE);
#endif
   ThreadSleep(1);

	// Start watchdog process
   if (g_dwFlags & AF_ENABLE_WATCHDOG)
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
      UpdatePolicyInventory();

#ifdef _WIN32
      if (g_config->getValueAsBoolean(_T("/CORE/AutoStartUserAgent"), false))
      {
         nxlog_debug(NXLOG_INFO, _T("Starting user agents for all logged in users"));

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
            nxlog_write(NXLOG_WARNING, _T("Signature validation failed for user agent executable \"%s\""), command.cstr() + 1);
         }
      }
      if (g_config->getValueAsBoolean(_T("/CORE/UserAgentWatchdog"), false))
      {
         ThreadCreate(UserAgentWatchdog, 0, MemCopyString(g_config->getValue(_T("/CORE/UserAgentExecutable"), _T("nxuseragent.exe"))));
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
	DebugPrintf(2, _T("Shutdown() called"));
	if (g_dwFlags & AF_ENABLE_WATCHDOG)
		StopWatchdog();

   g_dwFlags |= AF_SHUTDOWN;
   InitiateProcessShutdown();

   ShutdownEventSender();
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
	ThreadJoin(s_eventSenderThread);
	if (g_dwFlags & AF_ENABLE_SNMP_TRAP_PROXY)
	{
      ShutdownSNMPTrapSender();
      ThreadJoin(s_snmpTrapReceiverThread);
      ThreadJoin(s_snmpTrapSenderThread);
	}
   if (g_dwFlags & AF_ENABLE_SYSLOG_PROXY)
   {
      ShutdownSyslogSender();
      ThreadJoin(s_syslogReceiverThread);
      ThreadJoin(s_syslogSenderThread);
   }

	DestroySessionList();

   if (!(g_dwFlags & AF_SUBAGENT_LOADER))
   {
      if (g_dwFlags & AF_ENABLE_SNMP_PROXY)
      {
         ThreadPoolDestroy(g_snmpProxyThreadPool);
      }
      ThreadPoolDestroy(g_commThreadPool);
   }
   ThreadPoolDestroy(g_executorThreadPool);

   UnloadAllSubAgents();
   CloseLocalDatabase();
   nxlog_report_event(2, NXLOG_INFO, 0, _T("NetXMS Agent stopped"));
   nxlog_close();

   // Notify main thread about shutdown
#ifdef _WIN32
   ConditionSet(s_shutdownCondition);
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
      ConditionWait(s_shutdownCondition, INFINITE);
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
         ConditionWait(s_shutdownCondition, INFINITE);
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
static void DoRestartActions(UINT32 dwOldPID)
{
#if defined(_WIN32)
   if (dwOldPID == 0)
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

      hProcess = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, dwOldPID);
      if (hProcess != NULL)
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
   if (RestartService(dwOldPID))
   {
      // successfully restarted agent service using systemd, exit this instance
      exit(0);
   }
#endif
   kill(dwOldPID, SIGTERM);
   int i;
   for(i = 0; i < 30; i++)
   {
      sleep(2);
      if (kill(dwOldPID, SIGCONT) == -1)
         break;
   }

   // Kill previous instance of agent if it's still running
   if (i == 30)
      kill(dwOldPID, SIGKILL);
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
 * Update agent environment from config
 */
static void UpdateEnvironment()
{
   shared_ptr<Config> config = g_config;
   ObjectArray<ConfigEntry> *entrySet = config->getSubEntries(_T("/ENV"), _T("*"));
   if (entrySet == nullptr)
      return;

   for(int i = 0; i < entrySet->size(); i++)
   {
      ConfigEntry *e = entrySet->get(i);
      SetEnvironmentVariable(e->getName(), e->getValue());
   }
   delete entrySet;

   UpdateUserAgentsEnvironment();
}

/**
 * Application entry point
 */
int main(int argc, char *argv[])
{
   int ch, iExitCode = 0, iAction = ACTION_RUN_AGENT;
   BOOL bRestart = FALSE;
   UINT32 dwOldPID, dwMainPID;
	char *eptr;
   TCHAR configSection[MAX_DB_STRING] = DEFAULT_CONFIG_SECTION;
   const char *extraConfigValues = NULL;
   bool forceCreateConfig = false;
   bool restartExternalProcesses = false;
#ifdef _WIN32
   TCHAR szModuleName[MAX_PATH];
   HKEY hKey;
   DWORD dwSize;
#else
   TCHAR *pszEnv;
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
   pszEnv = _tgetenv(_T("NXAGENTD_CONFIG"));
   if (pszEnv != NULL)
      _tcslcpy(g_szConfigFile, pszEnv, MAX_PATH);

	pszEnv = _tgetenv(_T("NXAGENTD_CONFIG_D"));
   if (pszEnv != NULL)
      _tcslcpy(g_szConfigIncludeDir, pszEnv, MAX_PATH);
#endif

#if defined(_WIN32) || HAVE_DECL_GETOPT_LONG
   static struct option longOptions[] =
   {
      { (char *)"zone-uin", 1, NULL, 'B' },
      { NULL, 0, 0, 0 }
   };
#endif

   // Parse command line
	if (argc == 1)
		iAction = ACTION_HELP;
   opterr = 1;

#if defined(_WIN32) || HAVE_DECL_GETOPT_LONG
   while((ch = getopt_long(argc, argv, VALID_OPTIONS, longOptions, NULL)) != -1)
#else
   while((ch = getopt(argc, argv, VALID_OPTIONS)) != -1)
#endif
   {
      switch(ch)
      {
         case 'B':  //zone UIN
         {
            UINT32 zoneUIN = strtoul(optarg, &eptr, 0);
            if ((*eptr != 0))
            {
               fprintf(stderr, "Invalid zone UIN: %s\n", optarg);
               iAction = -1;
               iExitCode = 1;
            }
            else
               g_zoneUIN = zoneUIN;
            break;
         }
         case 'c':   // Configuration file
#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, g_szConfigFile, MAX_PATH);
				g_szConfigFile[MAX_PATH - 1] = 0;
#else
            strlcpy(g_szConfigFile, optarg, MAX_PATH);
#endif
            break;
         case 'C':   // Configuration check only
            iAction = ACTION_CHECK_CONFIG;
            break;
         case 'd':   // Run as daemon
            g_dwFlags |= AF_DAEMON;
            break;
         case 'D':   // Turn on debug output
				s_debugLevel = strtoul(optarg, &eptr, 0);
				if ((*eptr != 0) || (s_debugLevel > 9))
				{
					fprintf(stderr, "Invalid debug level: %s\n", optarg);
					iAction = -1;
					iExitCode = 1;
				}
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
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, configSection, MAX_DB_STRING);
				configSection[MAX_DB_STRING - 1] = 0;
#else
            strlcpy(configSection, optarg, MAX_DB_STRING);
#endif
            break;
         case 'h':   // Display help and exit
            iAction = ACTION_HELP;
            break;
#ifdef _WIN32
         case 'H':   // Hide window
            g_dwFlags |= AF_HIDE_WINDOW;
            break;
#endif
         case 'k':   // Delayed restart of external sub-agents and session agents
            restartExternalProcesses = true;
            iAction = ACTION_SHUTDOWN_EXT_AGENTS;
            break;
         case 'K':   // Shutdown external sub-agents and session agents
            iAction = ACTION_SHUTDOWN_EXT_AGENTS;
            break;
         case 'l':
            iAction = ACTION_GET_LOG_LOCATION;
            break;
#ifndef _WIN32
         case 'p':   // PID file
#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, g_szPidFile, MAX_PATH);
				g_szPidFile[MAX_PATH - 1] = 0;
#else
            strlcpy(g_szPidFile, optarg, MAX_PATH);
#endif
            break;
#endif
         case 'M':
            g_dwFlags |= AF_CENTRAL_CONFIG;
#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, g_szConfigServer, MAX_DB_STRING);
				g_szConfigServer[MAX_DB_STRING - 1] = 0;
#else
            strlcpy(g_szConfigServer, optarg, MAX_DB_STRING);
#endif
            break;
         case 'r':
            g_dwFlags |= AF_REGISTER;
#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, g_szRegistrar, MAX_DB_STRING);
				g_szRegistrar[MAX_DB_STRING - 1] = 0;
#else
            strlcpy(g_szRegistrar, optarg, MAX_DB_STRING);
#endif
            break;
         case 'P':   // Platform suffix
#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, g_szPlatformSuffix, MAX_PSUFFIX_LENGTH);
				g_szPlatformSuffix[MAX_PSUFFIX_LENGTH - 1] = 0;
#else
            strlcpy(g_szPlatformSuffix, optarg, MAX_PSUFFIX_LENGTH);
#endif
            break;
#ifndef _WIN32
			case 'u':	// set user ID
				uid = GetUserId(optarg);
				break;
#endif
         case 'v':   // Print version and exit
            _tprintf(_T("NetXMS Core Agent Version ") NETXMS_VERSION_STRING _T(" Build ") NETXMS_BUILD_TAG IS_UNICODE_BUILD_STRING _T("\n"));
            iAction = ACTION_NONE;
            break;
         case 'W':   // Watchdog process
            iAction = ACTION_RUN_WATCHDOG;
            dwMainPID = strtoul(optarg, NULL, 10);
            break;
         case 'X':   // Agent is being restarted
            bRestart = TRUE;
            dwOldPID = strtoul(optarg, NULL, 10);
            break;
         case 'Z':   // Create configuration file
            iAction = ACTION_CREATE_CONFIG;
#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, g_szConfigFile, MAX_PATH);
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
            iAction = ACTION_INSTALL_SERVICE;
            break;
         case 'R':   // Remove Windows service
            iAction = ACTION_REMOVE_SERVICE;
            break;
         case 's':   // Start Windows service
            iAction = ACTION_START_SERVICE;
            break;
         case 'S':   // Stop Windows service
            iAction = ACTION_STOP_SERVICE;
            break;
         case 'E':   // Install Windows event source
            iAction = ACTION_INSTALL_EVENT_SOURCE;
            break;
         case 'U':   // Remove Windows event source
            iAction = ACTION_REMOVE_EVENT_SOURCE;
            break;
         case 'e':   // Event source name
#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, g_windowsEventSourceName, MAX_PATH);
				g_windowsEventSourceName[MAX_PATH - 1] = 0;
#else
            strlcpy(g_windowsEventSourceName, optarg, MAX_PATH);
#endif
            break;
         case 'n':   // Service name
#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, g_windowsServiceName, MAX_PATH);
				g_windowsServiceName[MAX_PATH - 1] = 0;
#else
            strlcpy(g_windowsServiceName, optarg, MAX_PATH);
#endif
            break;
         case 'N':   // Service display name
#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, g_windowsServiceDisplayName, MAX_PATH);
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
            iAction = ACTION_HELP;
            iExitCode = 1;
            break;
         default:
            break;
      }
   }

#if !defined(_WIN32)
	if (!_tcscmp(g_szConfigFile, _T("{search}")))
	{
      TCHAR path[MAX_PATH] = _T("");
      const TCHAR *homeDir = _tgetenv(_T("NETXMS_HOME"));
      if (homeDir != NULL)
      {
         _sntprintf(path, MAX_PATH, _T("%s/etc/nxagentd.conf"), homeDir);
      }
		if ((path[0] != 0) && (_taccess(path, 4) == 0))
		{
			_tcscpy(g_szConfigFile, path);
		}
		else if (_taccess(SYSCONFDIR _T("/nxagentd.conf"), 4) == 0)
		{
			_tcscpy(g_szConfigFile, SYSCONFDIR _T("/nxagentd.conf"));
		}
		else if (_taccess(_T("/Database/etc/nxagentd.conf"), 4) == 0)	// for ZeroShell
		{
			_tcscpy(g_szConfigFile, _T("/Database/etc/nxagentd.conf"));
		}
		else
		{
			_tcscpy(g_szConfigFile, _T("/etc/nxagentd.conf"));
		}
	}
	if (!_tcscmp(g_szConfigIncludeDir, _T("{search}")))
	{
      TCHAR path[MAX_PATH] = _T("");
      const TCHAR *homeDir = _tgetenv(_T("NETXMS_HOME"));
      if (homeDir != NULL)
      {
         _sntprintf(path, MAX_PATH, _T("%s/etc/nxagentd.conf.d"), homeDir);
      }
		if ((path[0] != 0) && (_taccess(path, 4) == 0))
		{
			_tcscpy(g_szConfigIncludeDir, path);
		}
		else if (_taccess(SYSCONFDIR _T("/nxagentd.conf.d"), 4) == 0)
		{
			_tcscpy(g_szConfigIncludeDir, SYSCONFDIR _T("/nxagentd.conf.d"));
		}
		else if (_taccess(_T("/Database/etc/nxagentd.conf.d"), 4) == 0)
		{
			_tcscpy(g_szConfigIncludeDir, _T("/Database/etc/nxagentd.conf.d"));
		}
		else
		{
			_tcscpy(g_szConfigIncludeDir, _T("/etc/nxagentd.conf.d"));
		}
	}
#else
	if (!_tcscmp(g_szConfigFile, _T("{search}")))
	{
      TCHAR path[MAX_PATH];
      GetNetXMSDirectory(nxDirEtc, path);
      _tcscat(path, _T("\\nxagentd.conf"));
      if (_taccess(path, 4) == 0)
      {
         _tcscpy(g_szConfigFile, path);
      }
      else
      {
         _tcscpy(g_szConfigFile, _T("C:\\nxagentd.conf"));
      }
   }
	if (!_tcscmp(g_szConfigIncludeDir, _T("{search}")))
	{
      TCHAR path[MAX_PATH];
      GetNetXMSDirectory(nxDirEtc, path);
      _tcscat(path, _T("\\nxagentd.conf.d"));
      if (_taccess(path, 4) == 0)
      {
         _tcscpy(g_szConfigIncludeDir, path);
      }
      else
      {
         _tcscpy(g_szConfigIncludeDir, _T("C:\\nxagentd.conf.d"));
      }
   }
#endif

   if (bRestart)
      DoRestartActions(dwOldPID);

   // Do requested action
   switch(iAction)
   {
      case ACTION_RUN_AGENT:
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

			if (LoadConfig(configSection, true))
			{
			   shared_ptr<Config> config = g_config;
            // Check if master section starts with EXT:
            // If yes, switch to external subagent mode
            if (!_tcsnicmp(configSection, _T("EXT:"), 4))
            {
               _tcslcpy(g_masterAgent, &configSection[4], MAX_PATH);
            }

				if (config->parseTemplate(configSection, m_cfgTemplate))
				{
				   DecryptPassword(_T("netxms"), g_szSharedSecret, g_szSharedSecret, MAX_SECRET_LENGTH);

               // try to guess executable path
#ifdef _WIN32
               GetModuleFileName(GetModuleHandle(NULL), s_executableName, MAX_PATH);
#else
#ifdef UNICODE
               char __buffer[PATH_MAX];
#else
#define __buffer s_executableName
#endif
               if (realpath(argv[0], __buffer) == NULL)
               {
                  // fallback
                  TCHAR *path = _tgetenv(_T("NETXMS_HOME"));
                  if (path != NULL)
                  {
                     _tcslcpy(s_executableName, path, sizeof(s_executableName) / sizeof(s_executableName[0]));
                  }
                  else
                  {
                     _tcslcpy(s_executableName, PREFIX, sizeof(s_executableName) / sizeof(s_executableName[0]));
                  }
                  _tcslcat(s_executableName, _T("/bin/nxagentd"), sizeof(s_executableName) / sizeof(s_executableName[0]));
               }
               else
               {
#ifdef UNICODE
                  int len = strlen(__buffer);
                  MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, __buffer, len, s_executableName, MAX_PATH);
#endif
               }
#endif

					// Set exception handler
#ifdef _WIN32
					if (g_dwFlags & AF_CATCH_EXCEPTIONS)
						SetExceptionHandler(SEHServiceExceptionHandler, SEHServiceExceptionDataWriter, s_dumpDir,
												  _T("nxagentd"), g_dwFlags & AF_WRITE_FULL_DUMP, !(g_dwFlags & AF_DAEMON));
					__try {
#endif
					if ((!_tcsicmp(g_szLogFile, _T("{syslog}"))) ||
						 (!_tcsicmp(g_szLogFile, _T("{eventlog}"))))
					{
						g_dwFlags |= AF_USE_SYSLOG;
                  g_dwFlags &= ~(AF_USE_SYSTEMD_JOURNAL | AF_LOG_TO_STDOUT);
					}
					else if (!_tcsicmp(g_szLogFile, _T("{systemd}")))
					{
                  g_dwFlags |= AF_USE_SYSTEMD_JOURNAL;
                  g_dwFlags &= ~(AF_USE_SYSLOG | AF_LOG_TO_STDOUT);
					}
               else if (!_tcsicmp(g_szLogFile, _T("{stdout}")))
               {
                  g_dwFlags |= AF_LOG_TO_STDOUT;
                  g_dwFlags &= ~(AF_USE_SYSLOG | AF_USE_SYSTEMD_JOURNAL);
               }
               else
               {
                  g_dwFlags &= ~(AF_USE_SYSLOG | AF_USE_SYSTEMD_JOURNAL | AF_LOG_TO_STDOUT);
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
							iExitCode = 3;
						}
					}
#else    /* _WIN32 */
					if ((g_dwFlags & AF_DAEMON) && !(g_dwFlags & AF_SYSTEMD_DAEMON))
               {
						if (daemon(0, 0) == -1)
						{
							perror("Unable to setup itself as a daemon");
							iExitCode = 4;
						}
               }
					if (iExitCode == 0)
               {
#ifndef _WIN32
					   if (gid == 0)
					   {
					      const TCHAR *v = config->getValue(_T("/%agent/GroupId"));
					      if (v != NULL)
					      {
#ifdef UNICODE
					         char vmb[64];
					         WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, v, -1, vmb, 64, NULL, NULL);
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
                        WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, v, -1, vmb, 64, NULL, NULL);
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
						   FILE *fp;

							// Write PID file
							fp = _tfopen(g_szPidFile, _T("w"));
							if (fp != NULL)
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
							iExitCode = 3;
						}
	            }
#endif   /* _WIN32 */

#if defined(_WIN32)
					if (s_shutdownCondition != INVALID_CONDITION_HANDLE)
						ConditionDestroy(s_shutdownCondition);
#endif
#ifdef _WIN32
					LIBNETXMS_EXCEPTION_HANDLER
#endif
				}
				else
				{
					ConsolePrintf(_T("Error parsing configuration file\n"));
					iExitCode = 2;
				}
         }
         else
         {
            ConsolePrintf(_T("Error loading configuration file\n"));
            iExitCode = 2;
         }
         break;
      case ACTION_CHECK_CONFIG:
         {
            bool validConfig = LoadConfig(configSection, true);
            if (validConfig)
            {
               shared_ptr<Config> config = g_config;
               config->print(stdout);
               validConfig = config->parseTemplate(configSection, m_cfgTemplate);
            }

            if (!validConfig)
            {
               ConsolePrintf(_T("Configuration file check failed\n"));
               iExitCode = 2;
            }
         }
         break;
		case ACTION_RUN_WATCHDOG:
		   if (s_debugLevel == (UINT32)NXCONFIG_UNINITIALIZED_VALUE)
		      s_debugLevel = 0;
		   nxlog_set_debug_level(s_debugLevel);
			iExitCode = WatchdogMain(dwMainPID, _tcscmp(configSection, DEFAULT_CONFIG_SECTION) ? configSection : NULL);
			break;
      case ACTION_CREATE_CONFIG:
         iExitCode = CreateConfig(forceCreateConfig, CHECK_NULL_A(argv[optind]), CHECK_NULL_A(argv[optind + 1]),
                                  CHECK_NULL_A(argv[optind + 2]), CHECK_NULL_A(argv[optind + 3]),
			                         argc - optind - 4, &argv[optind + 4], extraConfigValues);
         break;
      case ACTION_SHUTDOWN_EXT_AGENTS:
         InitiateExternalProcessShutdown(restartExternalProcesses);
         iExitCode = 0;
         break;
      case ACTION_GET_LOG_LOCATION:
         {
            if (LoadConfig(configSection, true))
            {
               if (g_config->parseTemplate(configSection, m_cfgTemplate))
               {
                  _tprintf(_T("%s\n"), g_szLogFile);
               }
               else
               {
                  ConsolePrintf(_T("Configuration file parsing failed\n"));
                  iExitCode = 2;
               }
            }
            else
            {
               ConsolePrintf(_T("Configuration file load failed\n"));
               iExitCode = 2;
            }
         }
         break;
#ifdef _WIN32
      case ACTION_INSTALL_SERVICE:
         GetModuleFileName(GetModuleHandle(NULL), szModuleName, MAX_PATH);
         InstallService(szModuleName, g_szConfigFile, s_debugLevel);
         break;
      case ACTION_REMOVE_SERVICE:
         RemoveService();
         break;
      case ACTION_INSTALL_EVENT_SOURCE:
         GetModuleFileName(GetModuleHandle(NULL), szModuleName, MAX_PATH);
         InstallEventSource(szModuleName);
         break;
      case ACTION_REMOVE_EVENT_SOURCE:
         RemoveEventSource();
         break;
      case ACTION_START_SERVICE:
         StartAgentService();
         break;
      case ACTION_STOP_SERVICE:
         StopAgentService();
         break;
#endif
		case ACTION_HELP:
         _fputts(m_szHelpText, stdout);
			break;
      default:
         break;
   }

   return iExitCode;
}
