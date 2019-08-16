/*
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Raden Solutions
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
** File: main.cpp
**
**/

#include "nxcore.h"
#include <netxmsdb.h>
#include <netxms_mt.h>
#include <hdlink.h>
#include <agent_tunnel.h>

#if !defined(_WIN32) && HAVE_READLINE_READLINE_H && HAVE_READLINE && !defined(UNICODE)
#include <readline/readline.h>
#include <readline/history.h>
#define USE_READLINE 1
#endif

#ifdef _WIN32
#include <errno.h>
#include <psapi.h>
#include <conio.h>
#else
#include <signal.h>
#include <sys/wait.h>
#endif

#if WITH_ZMQ
#include "zeromq.h"
#endif

#ifdef CUSTOM_INIT_CODE
#include <server_custom_init.cpp>
#endif

/**
 * Shutdown reasons
 */
#define SHUTDOWN_DEFAULT      0
#define SHUTDOWN_FROM_CONSOLE 1
#define SHUTDOWN_BY_SIGNAL    2

/**
 * Externals
 */
extern ThreadPool *g_clientThreadPool;
extern ThreadPool *g_syncerThreadPool;
extern ThreadPool *g_discoveryThreadPool;

void InitClientListeners();
void InitMobileDeviceListeners();
void InitCertificates();
bool LoadServerCertificate(RSA **serverKey);
void InitUsers();
void CleanupUsers();
void LoadPerfDataStorageDrivers();
void ShutdownPerfDataStorageDrivers();
void ImportLocalConfiguration(bool overwrite);
void RegisterPredictionEngines();
void ShutdownPredictionEngines();
void ExecuteStartupScripts();
void CloseAgentTunnels();
void StopDataCollection();
void StopObjectMaintenanceThreads();

void ExecuteScheduledAction(const ScheduledTaskParameters *parameters);
void ExecuteScheduledScript(const ScheduledTaskParameters *parameters);
void MaintenanceModeEnter(const ScheduledTaskParameters *parameters);
void MaintenanceModeLeave(const ScheduledTaskParameters *parameters);
void ProcessUnboundTunnels(const ScheduledTaskParameters *parameters);
void ScheduleDeployPolicy(const ScheduledTaskParameters *parameters);
void ScheduleUninstallPolicy(const ScheduledTaskParameters *parameters);

void InitCountryList();
void InitCurrencyList();

#if XMPP_SUPPORTED
void StartXMPPConnector();
void StopXMPPConnector();
#endif

/**
 * Syslog server control
 */
void StartSyslogServer();
void StopSyslogServer();

/**
 * Thread functions
 */
THREAD_RESULT THREAD_CALL Syncer(void *);
THREAD_RESULT THREAD_CALL NodePoller(void *);
THREAD_RESULT THREAD_CALL PollManager(void *);
THREAD_RESULT THREAD_CALL EventProcessor(void *);
THREAD_RESULT THREAD_CALL WatchdogThread(void *);
THREAD_RESULT THREAD_CALL ClientListenerThread(void *);
THREAD_RESULT THREAD_CALL MobileDeviceListenerThread(void *);
THREAD_RESULT THREAD_CALL ISCListener(void *);
THREAD_RESULT THREAD_CALL LocalAdminListener(void *);
THREAD_RESULT THREAD_CALL SNMPTrapReceiver(void *);
THREAD_RESULT THREAD_CALL BeaconPoller(void *);
THREAD_RESULT THREAD_CALL JobManagerThread(void *);
THREAD_RESULT THREAD_CALL UptimeCalculator(void *);
THREAD_RESULT THREAD_CALL ReportingServerConnector(void *);
THREAD_RESULT THREAD_CALL ServerStatCollector(void *);
THREAD_RESULT THREAD_CALL TunnelListenerThread(void *arg);

/**
 * Global variables
 */
NXCORE_EXPORTABLE_VAR(TCHAR g_szConfigFile[MAX_PATH]) = _T("{search}");
NXCORE_EXPORTABLE_VAR(TCHAR g_szLogFile[MAX_PATH]) = DEFAULT_LOG_FILE;
UINT32 g_logRotationMode = NXLOG_ROTATION_BY_SIZE;
UINT64 g_maxLogSize = 16384 * 1024;
UINT32 g_logHistorySize = 4;
TCHAR g_szDailyLogFileSuffix[64] = _T("");
NXCORE_EXPORTABLE_VAR(TCHAR g_szDumpDir[MAX_PATH]) = DEFAULT_DUMP_DIR;
char g_szCodePage[256] = ICONV_DEFAULT_CODEPAGE;
NXCORE_EXPORTABLE_VAR(TCHAR g_szListenAddress[MAX_PATH]) = _T("*");
#ifndef _WIN32
NXCORE_EXPORTABLE_VAR(TCHAR g_szPIDFile[MAX_PATH]) = _T("/var/run/netxmsd.pid");
#endif
UINT32 g_discoveryPollingInterval;
UINT32 g_statusPollingInterval;
UINT32 g_configurationPollingInterval;
UINT32 g_routingTableUpdateInterval;
UINT32 g_topologyPollingInterval;
UINT32 g_conditionPollingInterval;
UINT32 g_instancePollingInterval;
UINT32 g_icmpPollingInterval;
UINT32 g_icmpPingSize;
UINT32 g_icmpPingTimeout = 1500;    // ICMP ping timeout (milliseconds)
UINT32 g_auditFlags;
UINT32 g_slmPollingInterval;
UINT32 g_offlineDataRelevanceTime = 86400;
NXCORE_EXPORTABLE_VAR(TCHAR g_netxmsdDataDir[MAX_PATH]) = _T("");
NXCORE_EXPORTABLE_VAR(TCHAR g_netxmsdLibDir[MAX_PATH]) = _T("");
NXCORE_EXPORTABLE_VAR(int g_dbSyntax) = DB_SYNTAX_UNKNOWN;
NXCORE_EXPORTABLE_VAR(UINT32 g_processAffinityMask) = DEFAULT_AFFINITY_MASK;
UINT64 g_serverId = 0;
RSA *g_pServerKey = NULL;
time_t g_serverStartTime = 0;
UINT32 g_lockTimeout = 60000;   // Default timeout for acquiring mutex
UINT32 g_agentCommandTimeout = 4000;  // Default timeout for requests to agent
UINT32 g_thresholdRepeatInterval = 0;	// Disabled by default
UINT32 g_requiredPolls = 1;
INT32 g_instanceRetentionTime = 0; // Default instance retention time
DB_DRIVER g_dbDriver = NULL;
NXCORE_EXPORTABLE_VAR(ThreadPool *g_mainThreadPool) = NULL;
INT16 g_defaultAgentCacheMode = AGENT_CACHE_OFF;
InetAddressList g_peerNodeAddrList;
Condition g_dbPasswordReady(true);

/**
 * Static data
 */
static THREAD s_pollManagerThread = INVALID_THREAD_HANDLE;
static THREAD s_syncerThread = INVALID_THREAD_HANDLE;
static THREAD s_clientListenerThread = INVALID_THREAD_HANDLE;
static THREAD s_mobileDeviceListenerThread = INVALID_THREAD_HANDLE;
static THREAD s_tunnelListenerThread = INVALID_THREAD_HANDLE;
static THREAD s_eventProcessorThread = INVALID_THREAD_HANDLE;
static THREAD s_statCollectorThread = INVALID_THREAD_HANDLE;
static int m_nShutdownReason = SHUTDOWN_DEFAULT;
static StringSet s_components;

#ifndef _WIN32
static pthread_t m_signalHandlerThread;
#endif

/**
 * Register component
 */
void NXCORE_EXPORTABLE RegisterComponent(const TCHAR *id)
{
   s_components.add(id);
}

/**
 * Check if component with given ID is registered
 */
bool NXCORE_EXPORTABLE IsComponentRegistered(const TCHAR *id)
{
   return s_components.contains(id);
}

/**
 * Fill NXCP message with components data
 */
void FillComponentsMessage(NXCPMessage *msg)
{
   msg->setField(VID_NUM_COMPONENTS, (INT32)s_components.size());
   UINT32 fieldId = VID_COMPONENT_LIST_BASE;
   Iterator<const TCHAR> *it = s_components.iterator();
   while(it->hasNext())
   {
      msg->setField(fieldId++, it->next());
   }
   delete it;
}

/**
 * Disconnect from database (exportable function for startup module)
 */
void NXCORE_EXPORTABLE ShutdownDB()
{
   DBConnectionPoolShutdown();
	DBUnloadDriver(g_dbDriver);
}

/**
 * Check data directory for existence
 */
static BOOL CheckDataDir()
{
	TCHAR szBuffer[MAX_PATH];

	if (_tchdir(g_netxmsdDataDir) == -1)
	{
		nxlog_write(NXLOG_ERROR, _T("Data directory \"%s\" does not exist or is inaccessible"), g_netxmsdDataDir);
		return FALSE;
	}

#ifdef _WIN32
#define MKDIR(name) _tmkdir(name)
#else
#define MKDIR(name) _tmkdir(name, 0700)
#endif

	// Create directory for package files if it doesn't exist
	_tcscpy(szBuffer, g_netxmsdDataDir);
	_tcscat(szBuffer, DDIR_PACKAGES);
	if (MKDIR(szBuffer) == -1)
		if (errno != EEXIST)
		{
         nxlog_write(NXLOG_ERROR, _T("Error creating data directory \"%s\" (%s)"), szBuffer, _tcserror(errno));
			return FALSE;
		}

	// Create directory for map background images if it doesn't exist
	_tcscpy(szBuffer, g_netxmsdDataDir);
	_tcscat(szBuffer, DDIR_BACKGROUNDS);
	if (MKDIR(szBuffer) == -1)
		if (errno != EEXIST)
		{
         nxlog_write(NXLOG_ERROR, _T("Error creating data directory \"%s\" (%s)"), szBuffer, _tcserror(errno));
			return FALSE;
		}

	// Create directory for image library is if does't exists
	_tcscpy(szBuffer, g_netxmsdDataDir);
	_tcscat(szBuffer, DDIR_IMAGES);
	if (MKDIR(szBuffer) == -1)
	{
		if (errno != EEXIST)
		{
         nxlog_write(NXLOG_ERROR, _T("Error creating data directory \"%s\" (%s)"), szBuffer, _tcserror(errno));
			return FALSE;
		}
	}

	// Create directory for file store if does't exists
	_tcscpy(szBuffer, g_netxmsdDataDir);
	_tcscat(szBuffer, DDIR_FILES);
	if (MKDIR(szBuffer) == -1)
	{
		if (errno != EEXIST)
		{
			nxlog_write(NXLOG_ERROR, _T("Error creating data directory \"%s\" (%s)"), szBuffer, _tcserror(errno));
			return FALSE;
		}
	}

#undef MKDIR

	return TRUE;
}

/**
 * Load global configuration parameters
 */
static void LoadGlobalConfig()
{
   if (MetaDataReadInt32(_T("SingeTablePerfData"), 0))
   {
      nxlog_debug(1, _T("Using single table for performance data storage"));
      g_flags |= AF_SINGLE_TABLE_PERF_DATA;
   }

   g_conditionPollingInterval = ConfigReadInt(_T("ConditionPollingInterval"), 60);
   g_configurationPollingInterval = ConfigReadInt(_T("ConfigurationPollingInterval"), 3600);
	g_discoveryPollingInterval = ConfigReadInt(_T("DiscoveryPollingInterval"), 900);
   g_icmpPollingInterval = ConfigReadInt(_T("ICMP.PollingInterval"), 60);
	g_instancePollingInterval = ConfigReadInt(_T("InstancePollingInterval"), 600);
	g_routingTableUpdateInterval = ConfigReadInt(_T("RoutingTableUpdateInterval"), 300);
   g_slmPollingInterval = ConfigReadInt(_T("SlmPollingInterval"), 60);
   g_statusPollingInterval = ConfigReadInt(_T("StatusPollingInterval"), 60);
	g_topologyPollingInterval = ConfigReadInt(_T("Topology.PollingInterval"), 1800);
	DCObject::m_defaultPollingInterval = ConfigReadInt(_T("DefaultDCIPollingInterval"), 60);
   DCObject::m_defaultRetentionTime = ConfigReadInt(_T("DefaultDCIRetentionTime"), 30);
   g_defaultAgentCacheMode = (INT16)ConfigReadInt(_T("DefaultAgentCacheMode"), AGENT_CACHE_OFF);
   if ((g_defaultAgentCacheMode != AGENT_CACHE_ON) && (g_defaultAgentCacheMode != AGENT_CACHE_OFF))
   {
      nxlog_debug(1, _T("Invalid value %d of DefaultAgentCacheMode: reset to %d (OFF)"), g_defaultAgentCacheMode, AGENT_CACHE_OFF);
      ConfigWriteInt(_T("DefaultAgentCacheMode"), AGENT_CACHE_OFF, true, true, true);
      g_defaultAgentCacheMode = AGENT_CACHE_OFF;
   }
	if (ConfigReadBoolean(_T("DeleteEmptySubnets"), true))
		g_flags |= AF_DELETE_EMPTY_SUBNETS;
	if (ConfigReadBoolean(_T("EnableSNMPTraps"), true))
		g_flags |= AF_ENABLE_SNMP_TRAPD;
	if (ConfigReadBoolean(_T("ProcessTrapsFromUnmanagedNodes"), false))
		g_flags |= AF_TRAPS_FROM_UNMANAGED_NODES;
	if (ConfigReadBoolean(_T("EnableZoning"), false))
		g_flags |= AF_ENABLE_ZONING;
	if (ConfigReadBoolean(_T("EnableObjectTransactions"), false))
		g_flags |= AF_ENABLE_OBJECT_TRANSACTIONS;
	if (ConfigReadBoolean(_T("UseSNMPTrapsForDiscovery"), false))
      g_flags |= AF_SNMP_TRAP_DISCOVERY;
   if (ConfigReadBoolean(_T("UseSyslogForDiscovery"), false))
      g_flags |= AF_SYSLOG_DISCOVERY;
	if (ConfigReadBoolean(_T("ResolveNodeNames"), true))
		g_flags |= AF_RESOLVE_NODE_NAMES;
	if (ConfigReadBoolean(_T("SyncNodeNamesWithDNS"), false))
		g_flags |= AF_SYNC_NODE_NAMES_WITH_DNS;
	if (ConfigReadBoolean(_T("CheckTrustedNodes"), true))
		g_flags |= AF_CHECK_TRUSTED_NODES;
   if (ConfigReadBoolean(_T("NetworkDiscovery.EnableParallelProcessing"), false))
      g_flags |= AF_PARALLEL_NETWORK_DISCOVERY;
   if (ConfigReadBoolean(_T("NetworkDiscovery.MergeDuplicateNodes"), false))
      g_flags |= AF_MERGE_DUPLICATE_NODES;
	if (ConfigReadBoolean(_T("NXSL.EnableContainerFunctions"), true))
	{
		g_flags |= AF_ENABLE_NXSL_CONTAINER_FUNCTIONS;
		nxlog_debug(3, _T("NXSL container management functions enabled"));
	}
   if (ConfigReadBoolean(_T("NXSL.EnableFileIOFunctions"), false))
   {
      g_flags |= AF_ENABLE_NXSL_FILE_IO_FUNCTIONS;
      nxlog_debug(3, _T("NXSL file I/O functions enabled"));
   }
   if (ConfigReadBoolean(_T("UseFQDNForNodeNames"), true))
      g_flags |= AF_USE_FQDN_FOR_NODE_NAMES;
   if (ConfigReadBoolean(_T("ApplyDCIFromTemplateToDisabledDCI"), true))
      g_flags |= AF_APPLY_TO_DISABLED_DCI_FROM_TEMPLATE;
   if (ConfigReadBoolean(_T("ResolveDNSToIPOnStatusPoll"), false))
      g_flags |= AF_RESOLVE_IP_FOR_EACH_STATUS_POLL;
   if (ConfigReadBoolean(_T("CaseInsensitiveLoginNames"), false))
      g_flags |= AF_CASE_INSENSITIVE_LOGINS;
   if (ConfigReadBoolean(_T("TrapSourcesInAllZones"), false))
      g_flags |= AF_TRAP_SOURCES_IN_ALL_ZONES;
   if (ConfigReadBoolean(_T("ICMP.CollectPollStatistics"), true))
      g_flags |= AF_COLLECT_ICMP_STATISTICS;

   switch(ConfigReadInt(_T("NetworkDiscovery.Type"), 0))
   {
      case 1:  // Passive only
         g_flags |= AF_PASSIVE_NETWORK_DISCOVERY;
         break;
      case 2:  // Active only
         g_flags |= AF_ACTIVE_NETWORK_DISCOVERY;
         break;
      case 3:  // Active and passive
         g_flags |= AF_PASSIVE_NETWORK_DISCOVERY | AF_ACTIVE_NETWORK_DISCOVERY;
         break;
      default:
         break;
   }

   if (g_netxmsdDataDir[0] == 0)
   {
      GetNetXMSDirectory(nxDirData, g_netxmsdDataDir);
      DbgPrintf(1, _T("Data directory set to %s"), g_netxmsdDataDir);
   }
   else
   {
      DbgPrintf(1, _T("Using data directory %s"), g_netxmsdDataDir);
   }

   g_icmpPingTimeout = ConfigReadInt(_T("IcmpPingTimeout"), 1500);
	g_icmpPingSize = ConfigReadInt(_T("IcmpPingSize"), 46);
	g_lockTimeout = ConfigReadInt(_T("LockTimeout"), 60000);
	g_agentCommandTimeout = ConfigReadInt(_T("AgentCommandTimeout"), 4000);
	g_thresholdRepeatInterval = ConfigReadInt(_T("ThresholdRepeatInterval"), 0);
	g_requiredPolls = ConfigReadInt(_T("PollCountForStatusChange"), 1);
	g_offlineDataRelevanceTime = ConfigReadInt(_T("OfflineDataRelevanceTime"), 86400);
	g_instanceRetentionTime = ConfigReadInt(_T("InstanceRetentionTime"), 0); // Config values are in days

	UINT32 snmpTimeout = ConfigReadInt(_T("SNMPRequestTimeout"), 1500);
   SnmpSetDefaultTimeout(snmpTimeout);
}

/**
 * Initialize cryptografic functions
 */
static bool InitCryptography()
{
#ifdef _WITH_ENCRYPTION
	bool success = false;

   if (!InitCryptoLib(ConfigReadULong(_T("AllowedCiphers"), 0x7F)))
		return FALSE;
   nxlog_debug(4, _T("Supported ciphers: %s"), (const TCHAR *)NXCPGetSupportedCiphersAsText());

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   OPENSSL_init_ssl(0, NULL);
#else
   SSL_library_init();
   SSL_load_error_strings();
#endif

   if (LoadServerCertificate(&g_pServerKey))
   {
      nxlog_debug(1, _T("Server certificate loaded"));
   }
   if (g_pServerKey != NULL)
   {
      nxlog_debug(1, _T("Using server certificate key"));
      success = true;
   }
   else
   {
      TCHAR szKeyFile[MAX_PATH];
      _tcscpy(szKeyFile, g_netxmsdDataDir);
      _tcscat(szKeyFile, DFILE_KEYS);
      g_pServerKey = LoadRSAKeys(szKeyFile);
      if (g_pServerKey == NULL)
      {
         nxlog_debug(1, _T("Generating RSA key pair..."));
         g_pServerKey = RSAGenerateKey(NETXMS_RSA_KEYLEN);
         if (g_pServerKey != NULL)
         {
            int fd = _topen(szKeyFile, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, 0600);
            if (fd != -1)
            {
               UINT32 dwLen = i2d_RSAPublicKey(g_pServerKey, NULL);
               dwLen += i2d_RSAPrivateKey(g_pServerKey, NULL);
               BYTE *pKeyBuffer = (BYTE *)malloc(dwLen);

               BYTE *pBufPos = pKeyBuffer;
               i2d_RSAPublicKey(g_pServerKey, &pBufPos);
               i2d_RSAPrivateKey(g_pServerKey, &pBufPos);
               _write(fd, &dwLen, sizeof(UINT32));
               _write(fd, pKeyBuffer, dwLen);

               BYTE hash[SHA1_DIGEST_SIZE];
               CalculateSHA1Hash(pKeyBuffer, dwLen, hash);
               _write(fd, hash, SHA1_DIGEST_SIZE);

               _close(fd);
               free(pKeyBuffer);
               success = true;
            }
            else
            {
               nxlog_debug(0, _T("Failed to open %s for writing"), szKeyFile);
            }
         }
         else
         {
            nxlog_debug(0, _T("Failed to generate RSA key"));
         }
      }
      else
      {
         success = true;
      }
   }

	int iPolicy = ConfigReadInt(_T("DefaultEncryptionPolicy"), 1);
	if ((iPolicy < 0) || (iPolicy > 3))
		iPolicy = 1;
	SetAgentDEP(iPolicy);

	return success;
#else
	return TRUE;
#endif
}

/**
 * Check if process with given PID exists and is a NetXMS server process
 */
static bool IsNetxmsdProcess(UINT32 pid)
{
#ifdef _WIN32
	bool result = false;
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
	if (hProcess != NULL)
	{
	   TCHAR szExtModule[MAX_PATH], szIntModule[MAX_PATH];
		if ((GetModuleBaseName(hProcess, NULL, szExtModule, MAX_PATH) > 0) &&
			 (GetModuleBaseName(GetCurrentProcess(), NULL, szIntModule, MAX_PATH) > 0))
		{
			result = (_tcsicmp(szExtModule, szIntModule) == 0);
		}
		else
		{
			// Cannot read process name, for safety assume that it's a server process
			result = true;
		}
		CloseHandle(hProcess);
	}
	return result;
#else
	return kill((pid_t)pid, 0) != -1;
#endif
}

/**
 * Check if remote netxmsd is running
 */
static bool PeerNodeIsRunning(const InetAddress& addr)
{
   bool result = false;

   TCHAR keyFile[MAX_PATH];
   _tcscpy(keyFile, g_netxmsdDataDir);
   _tcscat(keyFile, DFILE_KEYS);
   RSA *key = LoadRSAKeys(keyFile);

   AgentConnection *ac = new AgentConnection(addr);
   if (ac->connect(key))
   {
      TCHAR result[MAX_RESULT_LENGTH];
#ifdef _WIN32
      UINT32 rcc = ac->getParameter(_T("Process.Count(netxmsd.exe)"), MAX_RESULT_LENGTH, result);
#else
      UINT32 rcc = ac->getParameter(_T("Process.Count(netxmsd)"), MAX_RESULT_LENGTH, result);
#endif
      ac->decRefCount();
      if (key != NULL)
         RSA_free(key);
      if (rcc == ERR_SUCCESS)
      {
         return _tcstol(result, NULL, 10) > 0;
      }
   }
   else
   {
      ac->decRefCount();
      if (key != NULL)
         RSA_free(key);
   }

   UINT16 port = (UINT16)ConfigReadInt(_T("ClientListenerPort"), SERVER_LISTEN_PORT_FOR_CLIENTS);
   SOCKET s = ConnectToHost(addr, port, 5000);
   if (s != INVALID_SOCKET)
   {
      shutdown(s, SHUT_RDWR);
      closesocket(s);
      result = true;
   }
   return result;
}

/**
 * Database event handler
 */
static void DBEventHandler(DWORD dwEvent, const WCHAR *pszArg1, const WCHAR *pszArg2, bool connLost, void *userArg)
{
	if (!(g_flags & AF_SERVER_INITIALIZED))
		return;     // Don't try to do anything if server is not ready yet

	switch(dwEvent)
	{
		case DBEVENT_CONNECTION_LOST:
			PostEvent(EVENT_DB_CONNECTION_LOST, g_dwMgmtNode, NULL);
			g_flags |= AF_DB_CONNECTION_LOST;
			NotifyClientSessions(NX_NOTIFY_DBCONN_STATUS, FALSE);
			break;
		case DBEVENT_CONNECTION_RESTORED:
			PostEvent(EVENT_DB_CONNECTION_RESTORED, g_dwMgmtNode, NULL);
			g_flags &= ~AF_DB_CONNECTION_LOST;
			NotifyClientSessions(NX_NOTIFY_DBCONN_STATUS, TRUE);
			break;
		case DBEVENT_QUERY_FAILED:
			PostEvent(EVENT_DB_QUERY_FAILED, g_dwMgmtNode, "uud", pszArg1, pszArg2, connLost ? 1 : 0);
			break;
		default:
			break;
	}
}

/**
 * Send console message to session with open console
 */
static void SendConsoleMessage(ClientSession *session, void *arg)
{
	if (session->isConsoleOpen())
	{
		NXCPMessage msg;

		msg.setCode(CMD_ADM_MESSAGE);
		msg.setField(VID_MESSAGE, (TCHAR *)arg);
		session->postMessage(&msg);
	}
}

/**
 * Console writer
 */
static void LogConsoleWriter(const TCHAR *format, ...)
{
	TCHAR buffer[8192];
	va_list args;

	va_start(args, format);
	_vsntprintf(buffer, 8192, format, args);
	buffer[8191] = 0;
	va_end(args);

	WriteToTerminal(buffer);

	EnumerateClientSessions(SendConsoleMessage, buffer);
}

/**
 * Oracle session init callback
 */
static void OracleSessionInitCallback(DB_HANDLE hdb)
{
   if (!strcmp(DBGetDriverName(DBGetDriver(hdb)), "ORACLE"))
      DBQuery(hdb, _T("ALTER SESSION SET DDL_LOCK_TIMEOUT = 60"));
}

/**
 * Get database password
 */
static void GetDatabasePassword()
{
   if (_tcscmp(g_szDbPassword, _T("?")))
      return;

   nxlog_write(NXLOG_INFO, _T("Server is waiting for database password to be supplied"));
   g_dbPasswordReady.wait(INFINITE);
   nxlog_debug(1, _T("Database password received"));
}

/**
 * Server initialization
 */
BOOL NXCORE_EXPORTABLE Initialize()
{
	s_components.add(_T("CORE"));

	INT64 initStartTime = GetCurrentTimeMs();
	g_serverStartTime = static_cast<time_t>(initStartTime / 1000);

	if (!(g_flags & AF_USE_SYSLOG))
	{
		if (!nxlog_set_rotation_policy((int)g_logRotationMode, g_maxLogSize, (int)g_logHistorySize, g_szDailyLogFileSuffix))
			if (!(g_flags & AF_DAEMON))
				_tprintf(_T("WARNING: cannot set log rotation policy; using default values\n"));
	}
   if (!nxlog_open((g_flags & AF_USE_SYSLOG) ? NETXMSD_SYSLOG_NAME : g_szLogFile,
	                ((g_flags & AF_USE_SYSLOG) ? NXLOG_USE_SYSLOG : 0) |
	                ((g_flags & AF_USE_SYSTEMD_JOURNAL) ? NXLOG_USE_SYSTEMD : 0) |
	                ((g_flags & AF_BACKGROUND_LOG_WRITER) ? NXLOG_BACKGROUND_WRITER : 0) |
                   ((g_flags & AF_DAEMON) ? 0 : NXLOG_PRINT_TO_STDOUT) |
                   ((g_flags & AF_LOG_IN_JSON_FORMAT) ? NXLOG_JSON_FORMAT : 0)))
   {
		_ftprintf(stderr, _T("FATAL ERROR: Cannot open log file\n"));
      return FALSE;
   }
	nxlog_set_console_writer(LogConsoleWriter);

   if (g_netxmsdLibDir[0] == 0)
   {
      GetNetXMSDirectory(nxDirLib, g_netxmsdLibDir);
      nxlog_debug(1, _T("LIB directory set to %s"), g_netxmsdLibDir);
   }

	// Set code page
#ifndef _WIN32
	if (SetDefaultCodepage(g_szCodePage))
	{
		nxlog_write(NXLOG_INFO, _T("Code page set to %hs"), g_szCodePage);
	}
	else
	{
		nxlog_write(NXLOG_WARNING, _T("Unable to set codepage to %hs"), g_szCodePage);
	}
#endif

	// Set process affinity mask
	if (g_processAffinityMask != DEFAULT_AFFINITY_MASK)
	{
#ifdef _WIN32
		if (SetProcessAffinityMask(GetCurrentProcess(), g_processAffinityMask))
		   nxlog_write(NXLOG_INFO, _T("Process affinity mask set to 0x%08X"), g_processAffinityMask);
#else
		nxlog_write(NXLOG_WARNING, _T("Setting process CPU affinity is not supported on this operating system"));
#endif
	}

#ifdef _WIN32
	WSADATA wsaData;
	int wrc = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wrc != 0)
	{
      TCHAR buffer[1024];
      nxlog_write(NXLOG_ERROR, _T("Call to WSAStartup() failed (%s)"), GetSystemErrorText(wrc, buffer, 1024));
		return FALSE;
	}
#endif

	InitLocalNetInfo();

	// Create queue for delayed SQL queries
	g_dbWriterQueue = new Queue(256, 64);

	// Initialize database driver and connect to database
	if (!DBInit())
		return FALSE;
	g_dbDriver = DBLoadDriver(g_szDbDriver, g_szDbDrvParams, (nxlog_get_debug_level() >= 9), DBEventHandler, NULL);
	if (g_dbDriver == NULL)
		return FALSE;

   // Start local administrative interface listener if required
   if (g_flags & AF_ENABLE_LOCAL_CONSOLE)
      ThreadCreate(LocalAdminListener, 0, NULL);

	// Wait for database password if needed
	GetDatabasePassword();

	// Connect to database
	DB_HANDLE hdbBootstrap = NULL;
	TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
	for(int i = 0; ; i++)
	{
	   hdbBootstrap = DBConnect(g_dbDriver, g_szDbServer, g_szDbName, g_szDbLogin, g_szDbPassword, g_szDbSchema, errorText);
		if ((hdbBootstrap != NULL) || (i == 5))
			break;
		ThreadSleep(5);
	}
	if (hdbBootstrap == NULL)
	{
		nxlog_write(NXLOG_ERROR, _T("Unable to establish connection with database (%s)"), errorText);
		return FALSE;
	}
	nxlog_debug(1, _T("Successfully connected to database %s@%s"), g_szDbName, g_szDbServer);

	// Check database schema version
	INT32 schemaVersionMajor, schemaVersionMinor;
	if (!DBGetSchemaVersion(hdbBootstrap, &schemaVersionMajor, &schemaVersionMinor))
	{
	   nxlog_write(NXLOG_ERROR, _T("Unable to get database schema version"));
      DBDisconnect(hdbBootstrap);
      return FALSE;
	}

	if ((schemaVersionMajor != DB_SCHEMA_VERSION_MAJOR) || (schemaVersionMinor != DB_SCHEMA_VERSION_MINOR))
	{
		nxlog_write(NXLOG_ERROR, _T("Your database has format version %d.%d, but server is compiled for version %d.%d"), schemaVersionMajor, schemaVersionMinor, DB_SCHEMA_VERSION_MAJOR, DB_SCHEMA_VERSION_MINOR);
		DBDisconnect(hdbBootstrap);
		return FALSE;
	}

	// Read database syntax
	g_dbSyntax = DBGetSyntax(hdbBootstrap);
   if (g_dbSyntax == DB_SYNTAX_ORACLE)
   {
      DBSetSessionInitCallback(OracleSessionInitCallback);
   }
   else if (g_dbSyntax == DB_SYNTAX_PGSQL)
   {
      DB_RESULT hResult = DBSelect(hdbBootstrap, _T("SELECT version()"));
      if (hResult != NULL)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            char version[256];
            DBGetFieldA(hResult, 0, 0, version, 256);
            int major, minor;
            if (sscanf(version, "PostgreSQL %d.%d.", &major, &minor) == 2)
            {
               nxlog_debug(1, _T("Detected PostgreSQL version %d.%d"), major, minor);
               if ((major >= 10) || ((major == 9) && (minor >= 5)))
               {
                  g_flags |= AF_DB_SUPPORTS_MERGE;
               }
            }
         }
         DBFreeResult(hResult);
      }
   }
   else if (g_dbSyntax == DB_SYNTAX_TSDB)
   {
      // TimeScaleDB requires PostgreSQL 9.6.3+ so merge is always supported
      g_flags |= AF_DB_SUPPORTS_MERGE;
   }
	else if (g_dbSyntax == DB_SYNTAX_SQLITE)
	{
      // Disable startup database cache for SQLite
      g_flags &= ~AF_CACHE_DB_ON_STARTUP;
	}

	int baseSize = ConfigReadIntEx(hdbBootstrap, _T("DBConnectionPoolBaseSize"), 10);
	int maxSize = ConfigReadIntEx(hdbBootstrap, _T("DBConnectionPoolMaxSize"), 30);
	int cooldownTime = ConfigReadIntEx(hdbBootstrap, _T("DBConnectionPoolCooldownTime"), 300);
	int ttl = ConfigReadIntEx(hdbBootstrap, _T("DBConnectionPoolMaxLifetime"), 14400);

   DBDisconnect(hdbBootstrap);

	if (!DBConnectionPoolStartup(g_dbDriver, g_szDbServer, g_szDbName, g_szDbLogin, g_szDbPassword, g_szDbSchema, baseSize, maxSize, cooldownTime, ttl))
	{
      nxlog_write(NXLOG_ERROR, _T("Failed to initialize database connection pool"));
	   return FALSE;
	}

   UINT32 lrt = ConfigReadULong(_T("LongRunningQueryThreshold"), 0);
   if (lrt != 0)
   {
      DBSetLongRunningThreshold(lrt);
      nxlog_write(NXLOG_INFO, _T("Long running query threshold set at %d milliseconds"), lrt);
   }

   MetaDataPreLoad();

	// Read server ID
   TCHAR buffer[256];
	MetaDataReadStr(_T("ServerID"), buffer, 256, _T(""));
	StrStrip(buffer);
	if (buffer[0] != 0)
	{
      g_serverId = _tcstoull(buffer, NULL, 16);
	}
	else
	{
		// Generate new ID
		g_serverId = ((UINT64)time(NULL) << 31) | (UINT64)((UINT32)rand() & 0x7FFFFFFF);
      _sntprintf(buffer, 256, UINT64X_FMT(_T("016")), g_serverId);
		MetaDataWriteStr(_T("ServerID"), buffer);
	}
	nxlog_write(NXLOG_INFO, _T("Server ID ") UINT64X_FMT(_T("016")), g_serverId);

	// Initialize locks
retry_db_lock:
   InetAddress addr;
	if (!InitLocks(&addr, buffer))
	{
		if (addr.isValidUnicast())     // Database already locked by another server instance
		{
			// Check for lock from crashed/terminated local process
			if (GetLocalIpAddr().equals(addr))
			{
				UINT32 pid = ConfigReadULong(_T("DBLockPID"), 0);
				if (!IsNetxmsdProcess(pid) || (pid == GetCurrentProcessId()))
				{
					UnlockDB();
               nxlog_write(NXLOG_INFO, _T("Stalled database lock removed"));
					goto retry_db_lock;
				}
			}
			else if (g_peerNodeAddrList.hasAddress(addr))
			{
			   if (!PeerNodeIsRunning(addr))
			   {
               UnlockDB();
               nxlog_write(NXLOG_INFO, _T("Stalled database lock removed"));
               goto retry_db_lock;
			   }
			}
			nxlog_write(NXLOG_ERROR, _T("Database is already locked by another NetXMS server instance (IP address: %s, machine info: %s)"),
			         (const TCHAR *)addr.toString(), buffer);
		}
		else
      {
         nxlog_write(NXLOG_ERROR, _T("Error initializing component locks table"));
      }
		return FALSE;
	}
	g_flags |= AF_DB_LOCKED;

	// Load global configuration parameters
   ConfigPreLoad();
	LoadGlobalConfig();
   CASReadSettings();
   nxlog_debug(1, _T("Global configuration loaded"));

   // Setup thread pool resize parameters
   ThreadPoolSetResizeParameters(
            ConfigReadInt(_T("ThreadPool.Global.Responsiveness"), 12),
            ConfigReadInt(_T("ThreadPool.Global.WaitTimeHighWatermark"), 200),
            ConfigReadInt(_T("ThreadPool.Global.WaitTimeLowWatermark"), 100));

	// Check data directory
	if (!CheckDataDir())
		return FALSE;

	// Initialize cryptography
	if (!InitCryptography())
	{
		nxlog_write(NXLOG_ERROR, _T("Failed to initialize cryptografy module"));
		return FALSE;
	}

	// Initialize certificate store and CA
	InitCertificates();

	// Call custom initialization code
#ifdef CUSTOM_INIT_CODE
	if (!ServerCustomInit())
	   return FALSE;
#endif

   // Create thread pools
	nxlog_debug(2, _T("Creating thread pools"));
	g_mainThreadPool = ThreadPoolCreate(_T("MAIN"),
            ConfigReadInt(_T("ThreadPool.Main.BaseSize"), 8),
            ConfigReadInt(_T("ThreadPool.Main.MaxSize"), 256));
	g_agentConnectionThreadPool = ThreadPoolCreate(_T("AGENT"),
            ConfigReadInt(_T("ThreadPool.Agent.BaseSize"), 4),
            ConfigReadInt(_T("ThreadPool.Agent.MaxSize"), 256));

	// Setup unique identifiers table
	if (!InitIdTable())
		return FALSE;
	nxlog_debug(2, _T("ID table created"));

	InitCountryList();
	InitCurrencyList();

	// Update status for unfinished jobs in job history
	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	DBQuery(hdb, _T("UPDATE job_history SET status=4,failure_message='Aborted due to server shutdown or crash' WHERE status NOT IN (3,4,5)"));
	DBConnectionPoolReleaseConnection(hdb);

	// Load and compile scripts
	LoadScripts();

	// Initialize persistent storage
	PersistentStorageInit();

	// Initialize watchdog
	WatchdogInit();

   // Start database _T("lazy") write thread
   StartDBWriter();

	// Load modules
	if (!LoadNetXMSModules())
		return FALSE;	// Mandatory module not loaded
	RegisterPredictionEngines();

	// Initialize mailer
	InitMailer();

	// Load users from database
	InitUsers();
	if (!LoadUsers())
	{
		nxlog_write(NXLOG_ERROR, _T("Unable to load users and user groups from database (probably database is corrupted)"));
		return FALSE;
	}
	nxlog_debug(2, _T("User accounts loaded"));

	// Initialize audit
	InitAuditLog();

	// Initialize event handling subsystem
	if (!InitEventSubsystem())
		return FALSE;

	// Initialize alarms
   LoadAlarmCategories();
	if (!InitAlarmManager())
		return FALSE;

	//Initialize notification channels
	LoadNotificationChannelDrivers();
	LoadNCConfiguration();

	// Initialize objects infrastructure and load objects from database
	LoadNetworkDeviceDrivers();
	ObjectsInit();
	if (!LoadObjects())
		return FALSE;
	nxlog_debug(1, _T("Objects loaded and initialized"));

	// Initialize and load event actions
	if (!LoadActions())
	{
		nxlog_write(NXLOG_ERROR, _T("Unable to initialize actions"));
		return FALSE;
	}

   // Initialize helpdesk link
   SetHDLinkEntryPoints(ResolveAlarmByHDRef, TerminateAlarmByHDRef);
   LoadHelpDeskLink();

	// Initialize data collection subsystem
   LoadPerfDataStorageDrivers();
	InitDataCollector();

	InitLogAccess();
	FileUploadJob::init();
	InitMappingTables();

	//Initialize user agent messages
	InitUserAgentNotifications();

   InitClientListeners();
   int importMode = ConfigReadInt(_T("ImportConfigurationOnStartup"), 1);
	if (importMode > 0)
	   ImportLocalConfiguration(importMode == 2);

	// Check if management node object presented in database
	CheckForMgmtNode();
	if (g_dwMgmtNode == 0)
	{
		nxlog_write(NXLOG_ERROR, _T("NetXMS server cannot create node object for itself - probably because platform subagent cannot be loaded (check above error messages, if any)"));
		return FALSE;
	}

	// Create syncer thread pool
   maxSize = ConfigReadInt(_T("ThreadPool.Syncer.MaxSize"), 1);
   if (maxSize > 1)
   {
      g_syncerThreadPool = ThreadPoolCreate(_T("SYNCER"), ConfigReadInt(_T("ThreadPool.Syncer.BaseSize"), 1), maxSize);
   }

   // Create network discovery thread pool
   maxSize = ConfigReadInt(_T("ThreadPool.Discovery.MaxSize"), 16);
   if (maxSize > 1)
   {
      g_discoveryThreadPool = ThreadPoolCreate(_T("DISCOVERY"), ConfigReadInt(_T("ThreadPool.Discovery.BaseSize"), 1), maxSize);
   }

	// Start threads
	ThreadCreate(WatchdogThread, 0, NULL);
	ThreadCreate(NodePoller, 0, NULL);
	ThreadCreate(JobManagerThread, 0, NULL);
	s_syncerThread = ThreadCreateEx(Syncer, 0, NULL);

	CONDITION pollManagerInitialized = ConditionCreate(true);
	s_pollManagerThread = ThreadCreateEx(PollManager, 0, pollManagerInitialized);

   StartHouseKeeper();

	// Start event processor
   s_eventProcessorThread = ThreadCreateEx(EventProcessor, 0, NULL);

	// Start SNMP trapper
	InitTraps();
	if (ConfigReadBoolean(_T("EnableSNMPTraps"), true))
		ThreadCreate(SNMPTrapReceiver, 0, NULL);

	// Start built-in syslog daemon
   StartSyslogServer();

	// Start beacon host poller
	ThreadCreate(BeaconPoller, 0, NULL);

	// Start inter-server communication listener
	if (ConfigReadBoolean(_T("EnableISCListener"), false))
		ThreadCreate(ISCListener, 0, NULL);

	// Start reporting server connector
	if (ConfigReadBoolean(_T("EnableReportingServer"), false))
		ThreadCreate(ReportingServerConnector, 0, NULL);

   // Start LDAP synchronization
   if (ConfigReadInt(_T("LdapSyncInterval"), 0))
		ThreadCreate(SyncLDAPUsers, 0, NULL);

   // Wait for initialization of critical threads
   ConditionWait(pollManagerInitialized, INFINITE);
   ConditionDestroy(pollManagerInitialized);
   nxlog_debug(2, _T("Poll manager initialized"));

   RegisterSchedulerTaskHandler(_T("Execute.Action"), ExecuteScheduledAction, SYSTEM_ACCESS_SCHEDULE_SCRIPT);
   RegisterSchedulerTaskHandler(_T("Execute.Script"), ExecuteScheduledScript, SYSTEM_ACCESS_SCHEDULE_SCRIPT);
   RegisterSchedulerTaskHandler(_T("Maintenance.Enter"), MaintenanceModeEnter, SYSTEM_ACCESS_SCHEDULE_MAINTENANCE);
   RegisterSchedulerTaskHandler(_T("Maintenance.Leave"), MaintenanceModeLeave, SYSTEM_ACCESS_SCHEDULE_MAINTENANCE);
	RegisterSchedulerTaskHandler(_T("Policy.Deploy"), ScheduleDeployPolicy, 0); //No access right because it will be used only by server
	RegisterSchedulerTaskHandler(_T("Policy.Uninstall"), ScheduleUninstallPolicy, 0); //No access right because it will be used only by server
   RegisterSchedulerTaskHandler(ALARM_SUMMARY_EMAIL_TASK_ID, SendAlarmSummaryEmail, 0); //No access right because it will be used only by server
   RegisterSchedulerTaskHandler(UNBOUND_TUNNEL_PROCESSOR_TASK_ID, ProcessUnboundTunnels, 0); //No access right because it will be used only by server
   RegisterSchedulerTaskHandler(DCT_RESET_POLL_TIMERS_TASK_ID, ResetObjectPollTimers, 0); //No access right because it will be used only by server
   InitializeTaskScheduler();

   // Schedule unbound agent tunnel processing
   ScheduledTask *task = FindScheduledTaskByHandlerId(UNBOUND_TUNNEL_PROCESSOR_TASK_ID);
   if (task != NULL)
   {
      // Make sure task is marked as system
      if (!task->isSystem())
      {
         task->setFlag(SCHEDULED_TASK_SYSTEM);
         task->saveToDatabase(false);
      }
   }
   else
   {
      AddRecurrentScheduledTask(UNBOUND_TUNNEL_PROCESSOR_TASK_ID, _T("*/5 * * * *"), _T(""), NULL, 0, 0, SYSTEM_ACCESS_FULL, _T(""), SCHEDULED_TASK_SYSTEM);
   }

   // Send summary emails
   if (ConfigReadBoolean(_T("EnableAlarmSummaryEmails"), false))
      EnableAlarmSummaryEmails();
   else
      DeleteScheduledTaskByHandlerId(ALARM_SUMMARY_EMAIL_TASK_ID);

   // Schedule poll timers reset
   AddUniqueRecurrentScheduledTask(DCT_RESET_POLL_TIMERS_TASK_ID, _T("0 0 1 * *"), _T(""), NULL, 0, 0, SYSTEM_ACCESS_FULL, _T(""), SCHEDULED_TASK_SYSTEM);

	// Start listeners
   s_tunnelListenerThread = ThreadCreateEx(TunnelListenerThread, 0, NULL);
	s_clientListenerThread = ThreadCreateEx(ClientListenerThread, 0, NULL);
	InitMobileDeviceListeners();
	s_mobileDeviceListenerThread = ThreadCreateEx(MobileDeviceListenerThread, 0, NULL);

	// Start uptime calculator for SLM
	ThreadCreate(UptimeCalculator, 0, NULL);

	nxlog_debug(2, _T("LIBDIR: %s"), g_netxmsdLibDir);

	// Call startup functions for the modules
   CALL_ALL_MODULES(pfServerStarted, ());

#if XMPP_SUPPORTED
   if (ConfigReadBoolean(_T("EnableXMPPConnector"), true))
   {
      StartXMPPConnector();
   }
#endif

#if WITH_ZMQ
   StartZMQConnector();
#endif

   ExecuteStartupScripts();

   // Internal stat collector should be started last when all queues
   // and thread pools already created
   s_statCollectorThread = ThreadCreateEx(ServerStatCollector, 0, NULL);

	g_flags |= AF_SERVER_INITIALIZED;
	PostEvent(EVENT_SERVER_STARTED, g_dwMgmtNode, NULL);
	nxlog_debug(1, _T("Server initialization completed in %d milliseconds"), static_cast<int>(GetCurrentTimeMs() - initStartTime));
	return TRUE;
}

/**
 * Server shutdown
 */
void NXCORE_EXPORTABLE Shutdown()
{
	// Notify clients
	NotifyClientSessions(NX_NOTIFY_SHUTDOWN, 0);

	nxlog_write(NXLOG_INFO, _T("NetXMS Server stopped"));
   g_flags |= AF_SHUTDOWN;     // Set shutdown flag
	InitiateProcessShutdown();

   // Call shutdown functions for the modules
   // CALL_ALL_MODULES cannot be used here because it checks for shutdown flag
   for(UINT32 i = 0; i < g_dwNumModules; i++)
   {
      if (g_pModuleList[i].pfShutdown != NULL)
         g_pModuleList[i].pfShutdown();
   }

   ThreadJoin(s_statCollectorThread);

   StopHouseKeeper();
   ShutdownTaskScheduler();

   // Stop DCI cache loading thread
   g_dciCacheLoaderQueue.setShutdownMode();

	ShutdownPredictionEngines();
   StopObjectMaintenanceThreads();
   StopDataCollection();

   // Wait for critical threads
   ThreadJoin(s_pollManagerThread);
   ThreadJoin(s_syncerThread);

   nxlog_debug(2, _T("Waiting for listener threads to stop"));
   ThreadJoin(s_tunnelListenerThread);
   ThreadJoin(s_clientListenerThread);
   ThreadJoin(s_mobileDeviceListenerThread);

   CloseAgentTunnels();
   StopSyslogServer();

   nxlog_debug(2, _T("Waiting for event processor to stop"));
	g_eventQueue.put(INVALID_POINTER_VALUE);
	ThreadJoin(s_eventProcessorThread);

	ShutdownMailer();

#if XMPP_SUPPORTED
   StopXMPPConnector();
#endif

#if WITH_ZMQ
   StopZMQConnector();
#endif

	ThreadSleep(1);     // Give other threads a chance to terminate in a safe way
	nxlog_debug(2, _T("All threads were notified, continue with shutdown"));

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	SaveObjects(hdb, INVALID_INDEX, true);
	SaveCurrentFreeId();
	nxlog_debug(2, _T("All objects saved to database"));
	SaveUsers(hdb, INVALID_INDEX);
	nxlog_debug(2, _T("All users saved to database"));
   UpdatePStorageDatabase(hdb, INVALID_INDEX);
	nxlog_debug(2, _T("All persistent storage values saved"));
	DBConnectionPoolReleaseConnection(hdb);

	if (g_syncerThreadPool != NULL)
	   ThreadPoolDestroy(g_syncerThreadPool);

   if (g_discoveryThreadPool != NULL)
      ThreadPoolDestroy(g_discoveryThreadPool);

	StopDBWriter();
	nxlog_debug(1, _T("Database writer stopped"));

	CleanupUsers();
	PersistentStorageDestroy();

   ShutdownPerfDataStorageDrivers();

   CleanupActions();
   ShutdownEventSubsystem();
   ShutdownAlarmManager();
   nxlog_debug(1, _T("Event processing stopped"));

   ThreadPoolDestroy(g_clientThreadPool);
   ThreadPoolDestroy(g_agentConnectionThreadPool);
   ThreadPoolDestroy(g_mainThreadPool);
   WatchdogShutdown();

	// Remove database lock
	UnlockDB();

	DBConnectionPoolShutdown();
	DBUnloadDriver(g_dbDriver);
	nxlog_debug(1, _T("Database driver unloaded"));

	nxlog_debug(1, _T("Server shutdown complete"));
	nxlog_close();

	// Remove PID file
#ifndef _WIN32
	_tremove(g_szPIDFile);
#endif

	// Terminate process
#ifdef _WIN32
	if (!(g_flags & AF_DAEMON))
		ExitProcess(0);
#else
	exit(0);
#endif
}

/**
 * Fast server shutdown - normally called only by Windows service on system shutdown
 */
void NXCORE_EXPORTABLE FastShutdown()
{
   DbgPrintf(1, _T("Using fast shutdown procedure"));

	g_flags |= AF_SHUTDOWN;     // Set shutdown flag
	InitiateProcessShutdown();

	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	SaveObjects(hdb, INVALID_INDEX, true);
	DbgPrintf(2, _T("All objects saved to database"));
	SaveUsers(hdb, INVALID_INDEX);
	DbgPrintf(2, _T("All users saved to database"));
   UpdatePStorageDatabase(hdb, INVALID_INDEX);
	DbgPrintf(2, _T("All persistent storage values saved"));
	DBConnectionPoolReleaseConnection(hdb);

	// Remove database lock first, because we have a chance to lose DB connection
	UnlockDB();

	// Stop database writers
	StopDBWriter();
	DbgPrintf(1, _T("Database writer stopped"));

   DbgPrintf(1, _T("Server shutdown complete"));
	nxlog_close();
}

/**
 * Signal handler for UNIX platforms
 */
#ifndef _WIN32

void SignalHandlerStub(int nSignal)
{
	// should be unused, but JIC...
	if (nSignal == SIGCHLD)
	{
		while (waitpid(-1, NULL, WNOHANG) > 0)
			;
	}
}

THREAD_RESULT NXCORE_EXPORTABLE THREAD_CALL SignalHandler(void *pArg)
{
	sigset_t signals;
	int nSignal;

	m_signalHandlerThread = pthread_self();

	// default for SIGCHLD: ignore
	signal(SIGCHLD, &SignalHandlerStub);

	sigemptyset(&signals);
	sigaddset(&signals, SIGTERM);
	sigaddset(&signals, SIGINT);
	sigaddset(&signals, SIGSEGV);
	sigaddset(&signals, SIGCHLD);
	sigaddset(&signals, SIGHUP);
	sigaddset(&signals, SIGUSR1);
	sigaddset(&signals, SIGUSR2);
#if !defined(__sun) && !defined(_AIX) && !defined(__hpux)
	sigaddset(&signals, SIGPIPE);
#endif

	sigprocmask(SIG_BLOCK, &signals, NULL);

	while(1)
	{
		if (sigwait(&signals, &nSignal) == 0)
		{
			switch(nSignal)
			{
				case SIGTERM:
				case SIGINT:
				   // avoid repeat Shutdown() call
				   if (!(g_flags & AF_SHUTDOWN))
				   {
                  m_nShutdownReason = SHUTDOWN_BY_SIGNAL;
                  if (IsStandalone())
                  {
                     Shutdown(); // will never return
                  }
                  else
                  {
                     InitiateProcessShutdown();
                  }
				   }
				   break;
				case SIGSEGV:
					abort();
					break;
				case SIGCHLD:
					while (waitpid(-1, NULL, WNOHANG) > 0)
						;
					break;
				case SIGUSR1:
					if (g_flags & AF_SHUTDOWN)
						goto stop_handler;
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
 * Common main()
 */
THREAD_RESULT NXCORE_EXPORTABLE THREAD_CALL Main(void *pArg)
{
	nxlog_write(NXLOG_INFO, _T("NetXMS Server started"));

	if (IsStandalone())
   {
      if (!(g_flags & AF_DEBUG_CONSOLE_DISABLED))
	   {
		   char *ptr, szCommand[256];
		   LocalTerminalConsole ctx;
#ifdef UNICODE
   		WCHAR wcCommand[256];
#endif

		   WriteToTerminal(_T("\nNetXMS Server V") NETXMS_VERSION_STRING _T(" Build ") NETXMS_BUILD_TAG IS_UNICODE_BUILD_STRING _T(" Ready\n")
				             _T("Enter \"\x1b[1mhelp\x1b[0m\" for command list or \"\x1b[1mdown\x1b[0m\" for server shutdown\n")
				             _T("System Console\n\n"));

#if USE_READLINE
		   // Initialize readline library if we use it
		   rl_bind_key('\t', RL_INSERT_CAST rl_insert);
#endif

		   while(1)
		   {
#if USE_READLINE
   			ptr = readline("\x1b[33mnetxmsd:\x1b[0m ");
#else
			   WriteToTerminal(_T("\x1b[33mnetxmsd:\x1b[0m "));
			   fflush(stdout);
			   if (fgets(szCommand, 255, stdin) == NULL)
				   break;   // Error reading stdin
			   ptr = strchr(szCommand, '\n');
			   if (ptr != NULL)
				   *ptr = 0;
			   ptr = szCommand;
#endif

			   if (ptr != NULL)
			   {
#ifdef UNICODE
#if HAVE_MBSTOWCS
			      mbstowcs(wcCommand, ptr, 255);
#else
				   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, ptr, -1, wcCommand, 256);
#endif
				   wcCommand[255] = 0;
				   StrStrip(wcCommand);
				   if (wcCommand[0] != 0)
				   {
					   if (ProcessConsoleCommand(wcCommand, &ctx) == CMD_EXIT_SHUTDOWN)
#else
				   StrStrip(ptr);
				   if (*ptr != 0)
				   {
					   if (ProcessConsoleCommand(ptr, &ctx) == CMD_EXIT_SHUTDOWN)
#endif
						   break;
#if USE_READLINE
					   add_history(ptr);
#endif
				   }
#if USE_READLINE
				   free(ptr);
#endif
			   }
			   else
			   {
				   _tprintf(_T("\n"));
			   }
		   }

#if USE_READLINE
   		free(ptr);
#endif
   		if (!(g_flags & AF_SHUTDOWN))
   		{
            m_nShutdownReason = SHUTDOWN_FROM_CONSOLE;
            Shutdown();
   		}
      }
      else
      {
         // standalone with debug console disabled
#ifdef _WIN32
         _tprintf(_T("Server running. Press ESC to shutdown.\n"));
         while(1)
         {
            if (_getch() == 27)
               break;
         }
         _tprintf(_T("Server shutting down...\n"));
         Shutdown();
#else
         _tprintf(_T("Server running. Press Ctrl+C to shutdown.\n"));
         // Shutdown will be called from signal handler
         SleepAndCheckForShutdown(INFINITE);
#endif
      }
	}
	else
	{
      SleepAndCheckForShutdown(INFINITE);
		// On Win32, Shutdown() will be called by service control handler
#ifndef _WIN32
		Shutdown();
#endif
	}
	return THREAD_OK;
}

/**
 * Initiate server shutdown
 */
void InitiateShutdown()
{
#ifdef _WIN32
	Shutdown();
#else
	if (IsStandalone())
	{
		Shutdown();
	}
	else
	{
		pthread_kill(m_signalHandlerThread, SIGTERM);
	}
#endif
}

/**
 *DLL Entry point
 */
#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
