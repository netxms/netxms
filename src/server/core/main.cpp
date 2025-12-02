/*
** NetXMS - Network Management System
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
** File: main.cpp
**
**/

#include "nxcore.h"
#include <agent_tunnel.h>
#include <hdlink.h>
#include <netxms-version.h>
#include <netxms_mt.h>
#include <netxmsdb.h>
#include <nms_objects.h>
#include <nxcore_2fa.h>
#include <nxcore_logs.h>
#include <nxcore_ps.h>
#include <nxcore_websvc.h>
#include <nms_users.h>
#include <nxnet.h>
#include <nxstat.h>
#include <netxms-editline.h>

#ifdef _WIN32
#include <errno.h>
#include <psapi.h>
#include <conio.h>
#else
#include <signal.h>
#include <sys/wait.h>
#endif

#ifdef CUSTOM_INIT_CODE
#include <server_custom_init.cpp>
#endif

#if defined(__FreeBSD__)
#include "../../agent/smbios/freebsd.cpp"
#elif defined(__linux__)
#include "../../agent/smbios/linux.cpp"
#elif defined(__sun)
#include "../../agent/smbios/solaris.cpp"
#elif defined(_WIN32)
#include "../../agent/smbios/windows.cpp"
#else
static BYTE *SMBIOS_Reader(size_t *size)
{
   return nullptr;
}
#endif

#define DEBUG_TAG_STARTUP  _T("startup")
#define DEBUG_TAG_SHUTDOWN _T("shutdown")

/**
 * Externals
 */
extern ThreadPool *g_clientThreadPool;
extern ThreadPool *g_fileTransferThreadPool;
extern ThreadPool *g_syncerThreadPool;
extern ThreadPool *g_discoveryThreadPool;
extern ThreadPool *g_pollerThreadPool;

extern Config g_serverConfig;

extern uint32_t g_clientFirstPacketTimeout;

void InitClientListeners();
void InitMobileDeviceListeners();
void InitCertificates();
bool LoadServerCertificate(RSA_KEY *serverKey);
bool LoadInternalCACertificate();
void CleanupServerCertificates();
void LoadAuthenticationTokens();
void InitUsers();
void CleanupUsers();
void CleanupObjects();
void LoadPerfDataStorageDrivers();
void ShutdownPerfDataStorageDrivers();
void ImportLocalConfiguration(bool overwrite);
void RegisterPredictionEngines();
void ShutdownPredictionEngines();
void ExecuteStartupScripts();
void CloseAgentTunnels();
void StopDataCollection();
void StopObjectMaintenanceThreads();
bool LoadPhysicalLinks();
void LoadObjectQueries();
THREAD StartEventProcessor();
void StartHouseKeeper();
void StopHouseKeeper();
void StartSNMPAgent();
void StopSNMPAgent();
void LoadTrapMappings();
void StartSnmpTrapReceiver();
void StopSnmpTrapReceiver();
void CheckNodeCountRestrictions();
void InitializeDeviceBackupInterface();
void LoadWellKnownPortList();
bool InitAIAssistant();

void CheckUserAuthenticationTokens(const shared_ptr<ScheduledTaskParameters>& parameters);
void ExecuteScheduledAction(const shared_ptr<ScheduledTaskParameters>& parameters);
void ExecuteScheduledAgentCommand(const shared_ptr<ScheduledTaskParameters>& parameters);
void ExecuteScheduledPackageDeployment(const shared_ptr<ScheduledTaskParameters>& parameters);
void ExecuteScheduledScript(const shared_ptr<ScheduledTaskParameters>& parameters);
void MaintenanceModeEnter(const shared_ptr<ScheduledTaskParameters>& parameters);
void MaintenanceModeLeave(const shared_ptr<ScheduledTaskParameters>& parameters);
void ProcessUnboundTunnels(const shared_ptr<ScheduledTaskParameters>& parameters);
void RenewAgentCertificates(const shared_ptr<ScheduledTaskParameters>& parameters);
void ReloadCRLs(const shared_ptr<ScheduledTaskParameters>& parameters);
void ExecuteReport(const shared_ptr<ScheduledTaskParameters>& parameters);
void ExpandCommentMacrosTask(const shared_ptr<ScheduledTaskParameters> &parameters);
void ScheduledFileUpload(const shared_ptr<ScheduledTaskParameters>& parameters);

void InitCountryList();
void InitCurrencyList();

void LoadOUIDatabase();
void LoadAssetManagementSchema();

void StartPackageDeploymentManager();
void StopPackageDeploymentManager();

bool InitWebAPI();
void StartWebAPI();
void ShutdownWebAPI();

/**
 * Syslog server control
 */
void StartSyslogServer();
void StopSyslogServer();

/**
 * Windows event log server control
 */
void StartWindowsEventProcessing();
void StopWindowsEventProcessing();

/**
 * Thread functions
 */
void Syncer();
void DiscoveredAddressPoller();
void PollManager(Condition *startCondition);
void ClientListenerThread();
void MobileDeviceListenerThread();
void ISCListener();
void LocalAdminListenerThread();
void BeaconPoller();
void ReportingServerConnector();
void ServerStatCollector();
void TunnelListenerThread();
void LDAPSyncThread();

/**
 * Global variables
 */
NXCORE_EXPORTABLE_VAR(TCHAR g_szConfigFile[MAX_PATH]) = _T("{search}");
NXCORE_EXPORTABLE_VAR(TCHAR g_szLogFile[MAX_PATH]) = DEFAULT_LOG_FILE;
uint32_t g_logRotationMode = NXLOG_ROTATION_BY_SIZE;
uint64_t g_maxLogSize = 16384 * 1024;
uint32_t g_logHistorySize = 4;
TCHAR g_szDailyLogFileSuffix[64] = _T("");
NXCORE_EXPORTABLE_VAR(TCHAR g_szDumpDir[MAX_PATH]) = DEFAULT_DUMP_DIR;
char g_codePage[256] = "";
NXCORE_EXPORTABLE_VAR(TCHAR g_szListenAddress[MAX_PATH]) = _T("*");
#ifndef _WIN32
NXCORE_EXPORTABLE_VAR(TCHAR g_szPIDFile[MAX_PATH]) = _T("/var/run/netxmsd.pid");
#endif
uint32_t g_discoveryPollingInterval;
uint32_t g_statusPollingInterval;
uint32_t g_configurationPollingInterval;
uint32_t g_routingTableUpdateInterval;
uint32_t g_topologyPollingInterval;
uint32_t g_conditionPollingInterval;
uint32_t g_instancePollingInterval;
uint32_t g_icmpPollingInterval;
uint32_t g_autobindPollingInterval;
uint32_t g_mapUpdatePollingInterval;
uint32_t g_icmpPingSize;
uint32_t g_icmpPingTimeout = 1500;    // ICMP ping timeout (milliseconds)
uint32_t g_auditFlags;
uint32_t g_pollsBetweenPrimaryIpUpdate = 1;
int64_t g_offlineDataRelevanceTime = 86400000;  // Default offline data relevance time (milliseconds)
PrimaryIPUpdateMode g_primaryIpUpdateMode = PrimaryIPUpdateMode::NEVER;
NXCORE_EXPORTABLE_VAR(TCHAR g_netxmsdDataDir[MAX_PATH]) = _T("");
NXCORE_EXPORTABLE_VAR(TCHAR g_netxmsdLibDir[MAX_PATH]) = _T("");
NXCORE_EXPORTABLE_VAR(uint32_t g_processAffinityMask) = DEFAULT_AFFINITY_MASK;
NXCORE_EXPORTABLE_VAR(uint64_t g_serverId) = 0;
RSA_KEY g_serverKey = nullptr;
time_t g_serverStartTime = 0;
uint32_t g_agentCommandTimeout = 4000;  // Default timeout for requests to agent
uint32_t g_agentRestartWaitTime = 0;   // Wait time for agent restart in seconds
uint32_t g_thresholdRepeatInterval = 0;	// Disabled by default
uint32_t g_requiredPolls = 1;
int32_t g_instanceRetentionTime = 7; // Default instance retention time (in days)
uint32_t g_snmpTrapStormCountThreshold = 0;
uint32_t g_snmpTrapStormDurationThreshold = 15;
DB_DRIVER g_dbDriver = nullptr;
NXCORE_EXPORTABLE_VAR(ThreadPool *g_mainThreadPool) = nullptr;
int16_t g_defaultAgentCacheMode = AGENT_CACHE_OFF;
InetAddressList g_peerNodeAddrList;
Condition g_dbPasswordReady(true);
wchar_t g_startupSqlScriptPath[MAX_PATH] = L"";
wchar_t g_dbSessionSetupSqlScriptPath[MAX_PATH] = L"";
char g_snmpCodepage[16] = "";

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
static ShutdownReason s_shutdownReason = ShutdownReason::OTHER;
static StringSet s_components;
static ObjectArray<LicenseProblem> s_licenseProblems(0, 16, Ownership::True);
static Mutex s_licenseProblemsLock;
static uint32_t s_licenseProblemId = 1;

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
   s_components.fillMessage(msg, VID_COMPONENT_LIST_BASE, VID_NUM_COMPONENTS);
}

/**
 * Get registered server components as JSON array
 */
json_t NXCORE_EXPORTABLE *ComponentsToJson()
{
   json_t *json = json_array();
   s_components.forEach(
      [json] (const TCHAR *id) -> EnumerationCallbackResult
      {
         json_array_append_new(json, json_string_t(id));
         return _CONTINUE;
      });
   return json;
}

/**
 * Register license problem. Returns assigned problem ID.
 */
uint32_t NXCORE_EXPORTABLE RegisterLicenseProblem(const TCHAR *component, const TCHAR *type, const TCHAR *description)
{
   s_licenseProblemsLock.lock();
   auto p = new LicenseProblem(s_licenseProblemId++, component, type, description);
   s_licenseProblems.add(p);
   s_licenseProblemsLock.unlock();
   EventBuilder(EVENT_LICENSE_PROBLEM, g_dwMgmtNode)
      .param(_T("description"), description)
      .post();
   return p->id;
}

/**
 * Unregister license problem using problem ID.
 */
void NXCORE_EXPORTABLE UnregisterLicenseProblem(uint32_t id)
{
   s_licenseProblemsLock.lock();
   for(int i = 0; i < s_licenseProblems.size(); i++)
      if (s_licenseProblems.get(i)->id == id)
      {
         s_licenseProblems.remove(i);
         break;
      }
   s_licenseProblemsLock.unlock();
}

/**
 * Unregister license problem using component name and problem type.
 */
void NXCORE_EXPORTABLE UnregisterLicenseProblem(const TCHAR *component, const TCHAR *type)
{
   s_licenseProblemsLock.lock();
   for(int i = 0; i < s_licenseProblems.size(); i++)
   {
      LicenseProblem *p = s_licenseProblems.get(i);
      if (!_tcsicmp(p->component, component) && !_tcsicmp(p->type, type))
      {
         s_licenseProblems.remove(i);
         break;
      }
   }
   s_licenseProblemsLock.unlock();
}

/**
 * Fill NXCP message with list of license problems
 */
void NXCORE_EXPORTABLE FillLicenseProblemsMessage(NXCPMessage *msg)
{
   uint32_t fieldId = VID_LICENSE_PROBLEM_BASE;
   s_licenseProblemsLock.lock();
   for(int i = 0; i < s_licenseProblems.size(); i++)
   {
      LicenseProblem *p = s_licenseProblems.get(i);
      msg->setField(fieldId++, p->id);
      msg->setFieldFromTime(fieldId++, p->timestamp);
      msg->setField(fieldId++, p->component);
      msg->setField(fieldId++, p->type);
      msg->setField(fieldId++, p->description);
      fieldId += 5;
   }
   msg->setField(VID_LICENSE_PROBLEM_COUNT, s_licenseProblems.size());
   s_licenseProblemsLock.unlock();
}

/**
 * Get timestamp of given license problem
 */
time_t NXCORE_EXPORTABLE GetLicenseProblemTimestamp(uint32_t id)
{
   LockGuard lockGuard(s_licenseProblemsLock);
   for(int i = 0; i < s_licenseProblems.size(); i++)
      if (s_licenseProblems.get(i)->id == id)
         return s_licenseProblems.get(i)->timestamp;
   return 0;
}

/**
 * Disconnect from database (exportable function for startup module)
 */
void NXCORE_EXPORTABLE ShutdownDatabase()
{
   DBConnectionPoolShutdown();
   DBUnloadDriver(g_dbDriver);
}

/**
 * Create data directory
 */
static bool CreateDataDirectory(const TCHAR *name)
{
   TCHAR path[MAX_PATH];
   _tcscpy(path, g_netxmsdDataDir);
   _tcscat(path, name);
   NX_STAT_STRUCT st;
   if (CALL_STAT_FOLLOW_SYMLINK(path, &st) == 0)
   {
      if (S_ISDIR(st.st_mode))
         return true;

      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_STARTUP, _T("Cannot create data directory \"%s\" (file with same name already exists)"), path);
      return false;
   }

   if (!CreateDirectoryTree(path))
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_STARTUP, _T("Error creating data directory \"%s\" (%s)"), path, _tcserror(errno));
      return false;
   }
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("Data directory \"%s\" successfully created"), path);
   return true;
}

/**
 * Check data directory for existence
 */
static bool CheckDataDirectory()
{
   NX_STAT_STRUCT st;
   if (CALL_STAT_FOLLOW_SYMLINK(g_netxmsdDataDir, &st) != 0)
   {
      if (!CreateDirectoryTree(g_netxmsdDataDir))
      {
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_STARTUP, _T("Data directory \"%s\" does not exist and cannot be created, or is inaccessible"), g_netxmsdDataDir);
         return false;
      }
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("Data directory \"%s\" successfully created"), g_netxmsdDataDir);
	}

   if (!CreateDataDirectory(DDIR_PACKAGES))
      return false;

   if (!CreateDataDirectory(DDIR_BACKGROUNDS))
      return false;

   if (!CreateDataDirectory(DDIR_HOUSEKEEPER))
      return false;

   if (!CreateDataDirectory(DDIR_IMAGES))
      return false;

   if (!CreateDataDirectory(DDIR_FILES))
      return false;

   if (!CreateDataDirectory(DDIR_CRL))
      return false;

   if (!CreateDataDirectory(DDIR_MIBS))
      return false;

   return true;
}

/**
 * Load global configuration parameters
 */
static void LoadGlobalConfig()
{
   if (MetaDataReadInt32(_T("SingeTablePerfData"), 0))
   {
      nxlog_debug_tag(_T("dc"), 1, _T("Using single table for performance data storage"));
      g_flags |= AF_SINGLE_TABLE_PERF_DATA;
   }

   g_conditionPollingInterval = ConfigReadInt(_T("Objects.Conditions.PollingInterval"), 60);
   g_configurationPollingInterval = ConfigReadInt(_T("Objects.ConfigurationPollingInterval"), 3600);
   g_discoveryPollingInterval = ConfigReadInt(_T("NetworkDiscovery.PassiveDiscovery.Interval"), 900);
   g_icmpPollingInterval = ConfigReadInt(_T("ICMP.PollingInterval"), 60);
   g_instancePollingInterval = ConfigReadInt(_T("DataCollection.InstancePollingInterval"), 600);
   g_routingTableUpdateInterval = ConfigReadInt(_T("Topology.RoutingTable.UpdateInterval"), 300);
   g_statusPollingInterval = ConfigReadInt(_T("Objects.StatusPollingInterval"), 60);
   g_topologyPollingInterval = ConfigReadInt(_T("Topology.PollingInterval"), 1800);
   g_autobindPollingInterval = ConfigReadInt(_T("Objects.AutobindPollingInterval"), 3600);
   g_mapUpdatePollingInterval = ConfigReadInt(_T("Objects.NetworkMaps.UpdateInterval"), 60);
   DCObject::m_defaultPollingInterval = ConfigReadInt(_T("DataCollection.DefaultDCIPollingInterval"), 60);
   DCObject::m_defaultRetentionTime = ConfigReadInt(_T("DataCollection.DefaultDCIRetentionTime"), 30);
   g_defaultAgentCacheMode = (INT16)ConfigReadInt(_T("Agent.DefaultCacheMode"), AGENT_CACHE_OFF);
   if ((g_defaultAgentCacheMode != AGENT_CACHE_ON) && (g_defaultAgentCacheMode != AGENT_CACHE_OFF))
   {
      nxlog_debug_tag(_T("dc"), 1, _T("Invalid value %d of Agent.DefaultCacheMode: reset to %d (OFF)"), g_defaultAgentCacheMode, AGENT_CACHE_OFF);
      ConfigWriteInt(_T("Agent.DefaultCacheMode"), AGENT_CACHE_OFF, true, true, true);
      g_defaultAgentCacheMode = AGENT_CACHE_OFF;
   }
   if (ConfigReadBoolean(_T("Objects.Subnets.DeleteEmpty"), true))
      g_flags |= AF_DELETE_EMPTY_SUBNETS;
   if (ConfigReadBoolean(_T("SNMP.Traps.Enable"), true))
      g_flags |= AF_ENABLE_SNMP_TRAPD;
   if (ConfigReadBoolean(_T("SNMP.Traps.ProcessUnmanagedNodes"), false))
      g_flags |= AF_TRAPS_FROM_UNMANAGED_NODES;
   if (ConfigReadBoolean(_T("SNMP.Traps.LogAll"), false))
      g_flags |= AF_LOG_ALL_SNMP_TRAPS;
   if (ConfigReadBoolean(_T("SNMP.Traps.AllowVarbindsConversion"), true))
      g_flags |= AF_ALLOW_TRAP_VARBIND_CONVERSION;
   if (ConfigReadBoolean(_T("SNMP.Traps.UnmatchedTrapEvent"), true))
      g_flags |= AF_ENABLE_UNMATCHED_TRAP_EVENT;
   if (ConfigReadBoolean(_T("Objects.EnableZoning"), false))
      g_flags |= AF_ENABLE_ZONING;
   if (ConfigReadBoolean(_T("NetworkDiscovery.UseSNMPTraps"), false))
      g_flags |= AF_SNMP_TRAP_DISCOVERY;
   if (ConfigReadBoolean(_T("NetworkDiscovery.UseSyslog"), false))
      g_flags |= AF_SYSLOG_DISCOVERY;
   if (ConfigReadBoolean(_T("Objects.Interfaces.Enable8021xStatusPoll"), true))
      g_flags |= AF_ENABLE_8021X_STATUS_POLL;
   if (ConfigReadBoolean(_T("Objects.Nodes.ResolveNames"), true))
      g_flags |= AF_RESOLVE_NODE_NAMES;
   if (ConfigReadBoolean(_T("Objects.Nodes.SyncNamesWithDNS"), false))
      g_flags |= AF_SYNC_NODE_NAMES_WITH_DNS;
   if (ConfigReadBoolean(_T("Objects.Security.CheckTrustedObjects"), false))
      g_flags |= AF_CHECK_TRUSTED_OBJECTS;
   if (ConfigReadBoolean(_T("Objects.AutobindOnConfigurationPoll"), true))
      g_flags |= AF_AUTOBIND_ON_CONF_POLL;
   if (ConfigReadBoolean(_T("NetworkDiscovery.EnableParallelProcessing"), false))
      g_flags |= AF_PARALLEL_NETWORK_DISCOVERY;
   if (ConfigReadBoolean(_T("NetworkDiscovery.MergeDuplicateNodes"), false))
      g_flags |= AF_MERGE_DUPLICATE_NODES;
   if (ConfigReadBoolean(_T("NXSL.EnableContainerFunctions"), true))
   {
      g_flags |= AF_ENABLE_NXSL_CONTAINER_FUNCTIONS;
      nxlog_debug_tag(_T("nxsl"), 3, _T("NXSL container management functions enabled"));
   }
   if (ConfigReadBoolean(_T("NXSL.EnableFileIOFunctions"), false))
   {
      g_flags |= AF_ENABLE_NXSL_FILE_IO_FUNCTIONS;
      nxlog_debug_tag(_T("nxsl"), 3, _T("NXSL file I/O functions enabled"));
   }
   if (ConfigReadBoolean(_T("NetworkDiscovery.UseFQDNForNodeNames"), true))
      g_flags |= AF_USE_FQDN_FOR_NODE_NAMES;
   if (ConfigReadBoolean(_T("DataCollection.ApplyDCIFromTemplateToDisabledDCI"), true))
      g_flags |= AF_APPLY_TO_DISABLED_DCI_FROM_TEMPLATE;
   if (ConfigReadBoolean(_T("Server.Security.CaseInsensitiveLoginNames"), false))
      g_flags |= AF_CASE_INSENSITIVE_LOGINS;
   if (ConfigReadBoolean(_T("SNMP.Traps.SourcesInAllZones"), false))
      g_flags |= AF_TRAP_SOURCES_IN_ALL_ZONES;
   if (ConfigReadBoolean(_T("ICMP.CollectPollStatistics"), true))
      g_flags |= AF_COLLECT_ICMP_STATISTICS;
   if (ConfigReadBoolean(_T("DataCollection.Scheduler.RequireConnectivity"), false))
      g_flags |= AF_DC_SCHEDULER_REQUIRES_CONNECTIVITY;

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

   switch(ConfigReadInt(_T("DBWriter.HouseKeeperInterlock"), 0))
   {
      case 0:  // Auto
         if (g_dbSyntax == DB_SYNTAX_MSSQL)
            g_flags |= AF_DBWRITER_HK_INTERLOCK;
         else
            g_flags &= ~AF_DBWRITER_HK_INTERLOCK;
         break;
      case 1:  // Off
         g_flags &= ~AF_DBWRITER_HK_INTERLOCK;
         break;
      case 2:  // On
         g_flags |= AF_DBWRITER_HK_INTERLOCK;
         break;
      default:
         break;
   }
   nxlog_write_tag(NXLOG_INFO, _T("db.writer"), _T("DBWriter/Housekeeper interlock is %s"), (g_flags & AF_DBWRITER_HK_INTERLOCK) ? _T("ON") : _T("OFF"));

   if (g_netxmsdDataDir[0] == 0)
   {
      GetNetXMSDirectory(nxDirData, g_netxmsdDataDir);
      nxlog_debug_tag(_T("core"), 1, _T("Data directory set to %s"), g_netxmsdDataDir);
   }
   else
   {
      nxlog_debug_tag(_T("core"), 1, _T("Using data directory %s"), g_netxmsdDataDir);
      SetNetXMSDataDirectory(g_netxmsdDataDir);
   }

   g_icmpPingSize = ConfigReadInt(_T("ICMP.PingSize"), 46);
   if (g_icmpPingSize < MIN_PING_SIZE)
      g_icmpPingSize = MIN_PING_SIZE;
   else if (g_icmpPingSize > MAX_PING_SIZE)
      g_icmpPingSize = MAX_PING_SIZE;

   g_icmpPingTimeout = ConfigReadInt(_T("ICMP.PingTimeout"), 1500);
   g_agentCommandTimeout = ConfigReadInt(_T("Agent.CommandTimeout"), 4000);
   g_agentRestartWaitTime = ConfigReadInt(_T("Agent.RestartWaitTime"), 0);
   g_thresholdRepeatInterval = ConfigReadInt(_T("DataCollection.ThresholdRepeatInterval"), 0);
   g_requiredPolls = ConfigReadInt(_T("Objects.PollCountForStatusChange"), 1);
   g_offlineDataRelevanceTime = ConfigReadInt(_T("DataCollection.OfflineDataRelevanceTime"), 86400) * 1000L; // Config value is in seconds
   g_instanceRetentionTime = ConfigReadInt(_T("DataCollection.InstanceRetentionTime"), 7); // Config values are in days
   g_snmpTrapStormCountThreshold = ConfigReadInt(_T("SNMP.Traps.RateLimit.Threshold"), 0);
   g_snmpTrapStormDurationThreshold = ConfigReadInt(_T("SNMP.Traps.RateLimit.Duration"), 15);
   ConfigReadStrUTF8(_T("SNMP.Codepage"), g_snmpCodepage, 16, "");

   switch(ConfigReadInt(_T("Objects.Nodes.ResolveDNSToIPOnStatusPoll"), static_cast<int>(PrimaryIPUpdateMode::NEVER)))
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
   g_pollsBetweenPrimaryIpUpdate = ConfigReadULong(_T("Objects.Nodes.ResolveDNSToIPOnStatusPoll.Interval"), 1);

   SnmpSetDefaultTimeout(ConfigReadInt(_T("SNMP.RequestTimeout"), 1500));
   SnmpSetDefaultRetryCount(ConfigReadInt(_T("SNMP.RetryCount"), 3));

   g_clientFirstPacketTimeout = ConfigReadULong(_T("Client.FirstPacketTimeout"), 2000);
   if (g_clientFirstPacketTimeout < 100)
      g_clientFirstPacketTimeout = 100;
   nxlog_debug_tag(_T("client.session"), 2, _T("Client first packet timeout set to %u milliseconds"), g_clientFirstPacketTimeout);
}

/**
 * Initialize cryptografic functions
 */
static bool InitCryptography()
{
   if (!InitCryptoLib(ConfigReadULong(_T("Server.AllowedCiphers"), 0x7F)))
      return false;
   nxlog_debug_tag(_T("crypto"), 4, _T("Supported ciphers: %s"), NXCPGetSupportedCiphersAsText().cstr());

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   OPENSSL_init_ssl(0, nullptr);
#else
   SSL_library_init();
   SSL_load_error_strings();
#endif

   bool success = false;
   if (LoadServerCertificate(&g_serverKey))
   {
      nxlog_debug_tag(_T("crypto"), 1, _T("Server certificate loaded"));
   }
   LoadInternalCACertificate();
   if (g_serverKey != nullptr)
   {
      nxlog_debug_tag(_T("crypto"), 1, _T("Using server certificate key"));
      success = true;
   }
   else
   {
      wchar_t keyFile[MAX_PATH];
      wcscpy(keyFile, g_netxmsdDataDir);
      wcscat(keyFile, DFILE_KEYS);
      g_serverKey = RSALoadKey(keyFile);
      if (g_serverKey == nullptr)
      {
         nxlog_debug_tag(_T("crypto"), 1, _T("Generating RSA key pair..."));
         g_serverKey = RSAGenerateKey(NETXMS_RSA_KEYLEN);
         if (g_serverKey != nullptr)
         {
            if (RSASaveKey(g_serverKey, keyFile))
            {
               success = true;
            }
            else
            {
               nxlog_write_tag(NXLOG_ERROR, _T("crypto"), _T("Cannot save server key to file \"%s\""), keyFile);
            }
         }
         else
         {
            nxlog_write_tag(NXLOG_ERROR, _T("crypto"), _T("Failed to generate RSA key"));
         }
      }
      else
      {
         success = true;
      }
   }

   int iPolicy = ConfigReadInt(_T("Agent.DefaultEncryptionPolicy"), 1);
   if ((iPolicy < 0) || (iPolicy > 3))
      iPolicy = 1;
   SetAgentDEP(iPolicy);

   return success;
}

/**
 * Check if process with given PID exists and is a NetXMS server process
 */
static bool IsNetxmsdProcess(uint32_t pid)
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
   wchar_t keyFile[MAX_PATH];
   wcscpy(keyFile, g_netxmsdDataDir);
   wcscat(keyFile, DFILE_KEYS);
   RSA_KEY key = RSALoadKey(keyFile);

   shared_ptr<AgentConnection> ac = make_shared<AgentConnection>(addr);
   if (ac->connect(key))
   {
      TCHAR result[MAX_RESULT_LENGTH];
#ifdef _WIN32
      uint32_t rcc = ac->getParameter(_T("Process.Count(netxmsd.exe)"), result, MAX_RESULT_LENGTH);
#else
      uint32_t rcc = ac->getParameter(_T("Process.Count(netxmsd)"), result, MAX_RESULT_LENGTH);
#endif
      RSAFree(key);
      if (rcc == ERR_SUCCESS)
      {
         return _tcstol(result, nullptr, 10) > 0;
      }
   }
   else
   {
      RSAFree(key);
   }

   uint16_t port = static_cast<uint16_t>(ConfigReadInt(_T("Client.ListenerPort"), SERVER_LISTEN_PORT_FOR_CLIENTS));
   return TcpPing(addr, port, 5000) == TCP_PING_SUCCESS;
}

/**
 * Database event handler
 */
static void DBEventHandler(uint32_t event, const WCHAR *arg1, const WCHAR *arg2, bool connLost, void *context)
{
	if (!(g_flags & AF_SERVER_INITIALIZED))
		return;     // Don't try to do anything if server is not ready yet

	BYTE queryHash[MD5_DIGEST_SIZE];
	TCHAR queryHashText[MD5_DIGEST_SIZE * 2 + 1];
	switch(event)
	{
		case DBEVENT_CONNECTION_LOST:
			PostSystemEvent(EVENT_DB_CONNECTION_LOST, g_dwMgmtNode);
			InterlockedOr64(&g_flags, AF_DB_CONNECTION_LOST);
			NotifyClientSessions(NX_NOTIFY_DBCONN_STATUS, FALSE);
			break;
		case DBEVENT_CONNECTION_RESTORED:
			PostSystemEvent(EVENT_DB_CONNECTION_RESTORED, g_dwMgmtNode);
			InterlockedAnd64(&g_flags, ~AF_DB_CONNECTION_LOST);
			NotifyClientSessions(NX_NOTIFY_DBCONN_STATUS, TRUE);
			break;
		case DBEVENT_QUERY_FAILED:
		   CalculateMD5Hash(arg1, wcslen(arg1) * sizeof(WCHAR), queryHash);
		   EventBuilder(EVENT_DB_QUERY_FAILED, g_dwMgmtNode)
		      .param(_T("query"), arg1)
		      .param(_T("message"), arg2)
		      .param(_T("connectionLost"), connLost ? 1 : 0)
		      .param(_T("hash"), BinToStr(queryHash, MD5_DIGEST_SIZE, queryHashText))
		      .post();
			break;
		default:
			break;
	}
}

/**
 * Send console message to session with open console
 */
static void SendConsoleMessage(ClientSession *session, TCHAR *message)
{
	if (session->isConsoleOpen())
	{
		NXCPMessage msg;
		msg.setCode(CMD_ADM_MESSAGE);
		msg.setField(VID_MESSAGE, message);
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
 * Database session init callback
 */
static void DbSessionInitCallback(DB_HANDLE hdb)
{
   if ((g_dbSyntax == DB_SYNTAX_ORACLE) && (!strcmp(DBGetDriverName(DBGetDriver(hdb)), "ORACLE")))
   {
      DBQuery(hdb, _T("ALTER SESSION SET DDL_LOCK_TIMEOUT = 60"));
   }

   if (*g_dbSessionSetupSqlScriptPath != 0)
   {
      ExecuteSQLCommandFile(g_dbSessionSetupSqlScriptPath, hdb);
   }
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
 * Get log destination flags
 */
static inline UINT32 GetLogDestinationFlag()
{
   if (g_flags & AF_USE_SYSLOG)
      return NXLOG_USE_SYSLOG;
   if (g_flags & AF_USE_SYSTEMD_JOURNAL)
      return NXLOG_USE_SYSTEMD;
   if (g_flags & AF_LOG_TO_STDOUT)
      return NXLOG_USE_STDOUT;
   return 0;
}

/**
 * Scheduled task handler - enter maintenance mode
 */
static void DummyScheduledTaskExecutor(const shared_ptr<ScheduledTaskParameters>& parameters)
{
   nxlog_debug_tag(_T("scheduler"), 5, _T("Dummy scheduled task executed"));
}

/**
 * Server initialization
 */
bool NXCORE_EXPORTABLE Initialize()
{
	s_components.add(_T("CORE"));

	int64_t initStartTime = GetCurrentTimeMs();
	g_serverStartTime = static_cast<time_t>(initStartTime / 1000);

	if (!(g_flags & AF_USE_SYSLOG))
	{
		if (!nxlog_set_rotation_policy((int)g_logRotationMode, g_maxLogSize, (int)g_logHistorySize, g_szDailyLogFileSuffix))
			if (!(g_flags & AF_DAEMON))
				_tprintf(_T("WARNING: cannot set log rotation policy; using default values\n"));
	}
   if (!nxlog_open((g_flags & AF_USE_SYSLOG) ? NETXMSD_SYSLOG_NAME : g_szLogFile,
                   GetLogDestinationFlag() |
	                ((g_flags & AF_BACKGROUND_LOG_WRITER) ? NXLOG_BACKGROUND_WRITER : 0) |
                   ((g_flags & AF_DAEMON) ? 0 : NXLOG_PRINT_TO_STDOUT) |
                   ((g_flags & AF_LOG_IN_JSON_FORMAT) ? NXLOG_JSON_FORMAT : 0)))
   {
		_ftprintf(stderr, _T("FATAL ERROR: Cannot open log file\n"));
      return false;
   }
	nxlog_set_console_writer(LogConsoleWriter);
	nxlog_set_debug_writer(nullptr);

   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("Starting NetXMS server version ") NETXMS_VERSION_STRING _T(" build tag ") NETXMS_BUILD_TAG);
   wchar_t timezone[32];
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("System time zone is %s"), GetSystemTimeZone(timezone, 32));
   nxlog_write_tag(NXLOG_INFO, _T("logger"), _T("Debug level set to %d"), nxlog_get_debug_level());
   nxlog_write_tag(NXLOG_INFO, _T("config"), _T("Main configuration file: %s"), g_szConfigFile);
   nxlog_write_tag(NXLOG_INFO, _T("config"), _T("Configuration tree:"));
   g_serverConfig.print();

   nxlog_set_rotation_hook(
      [] () -> void
      {
         nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("NetXMS server version ") NETXMS_VERSION_STRING _T(" build tag ") NETXMS_BUILD_TAG);;
         nxlog_write_tag(NXLOG_INFO, _T("config"), _T("Main configuration file: %s"), g_szConfigFile);
         nxlog_write_tag(NXLOG_INFO, _T("config"), _T("Configuration tree:"));
         g_serverConfig.print();
      });

   if (g_netxmsdLibDir[0] == 0)
   {
      GetNetXMSDirectory(nxDirLib, g_netxmsdLibDir);
      nxlog_debug_tag(DEBUG_TAG_STARTUP, 1, _T("LIB directory set to %s"), g_netxmsdLibDir);
   }

	// Set code page
#ifndef _WIN32
   if (g_codePage[0] != 0)
   {
      if (SetDefaultCodepage(g_codePage))
      {
         nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("Code page set to %hs"), g_codePage);
      }
      else
      {
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_STARTUP, _T("Unable to set codepage to %hs"), g_codePage);
      }
   }
   else
   {
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("Using system default codepage"));
   }
#endif

	// Set process affinity mask
	if (g_processAffinityMask != DEFAULT_AFFINITY_MASK)
	{
#ifdef _WIN32
		if (SetProcessAffinityMask(GetCurrentProcess(), g_processAffinityMask))
		   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("Process affinity mask set to 0x%08X"), g_processAffinityMask);
#else
		nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_STARTUP, _T("Setting process CPU affinity is not supported on this operating system"));
#endif
	}

	// Log hardware ID
	SMBIOS_Parse(SMBIOS_Reader);
	BYTE hwid[SHA1_DIGEST_SIZE];
	if (GetSystemHardwareId(hwid))
	{
	   TCHAR hwidText[SHA1_DIGEST_SIZE * 2 + 1];
	   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("System hardware ID %s"), BinToStr(hwid, SHA1_DIGEST_SIZE, hwidText));
	}
	else
	{
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_STARTUP, _T("Cannot determine system hardware ID"));
	}

#ifdef _WIN32
	WSADATA wsaData;
	int wrc = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wrc != 0)
	{
      TCHAR buffer[1024];
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_STARTUP, _T("Call to WSAStartup() failed (%s)"), GetSystemErrorText(wrc, buffer, 1024));
		return false;
	}
#endif

	InitLocalNetInfo();

	// Initialize database driver and connect to database
	if (!DBInit())
		return false;
	g_dbDriver = DBLoadDriver(g_szDbDriver, g_szDbDrvParams, DBEventHandler, nullptr);
	if (g_dbDriver == nullptr)
		return false;

   // Start local administrative interface listener
   ThreadCreate(LocalAdminListenerThread);

	// Wait for database password if needed
	GetDatabasePassword();

	// Execute database password command if configured
	ExecuteDatabasePasswordCommand();

	// Retrieve database credentials from Vault if configured
	RetrieveDatabaseCredentialsFromVault();

   // Set up callback for DBConnect
   DBSetSessionInitCallback(DbSessionInitCallback);

	// Connect to database
	DB_HANDLE hdbBootstrap = nullptr;
	TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
	for(int i = 0; ; i++)
	{
	   hdbBootstrap = DBConnect(g_dbDriver, g_szDbServer, g_szDbName, g_szDbLogin, g_szDbPassword, g_szDbSchema, errorText);
		if ((hdbBootstrap != nullptr) || (i == 5))
			break;
		ThreadSleep(5);
	}
	if (hdbBootstrap == nullptr)
	{
		nxlog_write_tag(NXLOG_ERROR, _T("db"), _T("Unable to establish connection with database (%s)"), errorText);
		return false;
	}
	nxlog_debug_tag(_T("db"), 1, _T("Successfully connected to database %s@%s"), g_szDbName, g_szDbServer);

   // Run SQL commands from file if set
   if (*g_startupSqlScriptPath != 0)
   {
      ExecuteSQLCommandFile(g_startupSqlScriptPath, hdbBootstrap);
   }

	// Check database schema version
	int32_t schemaVersionMajor, schemaVersionMinor;
	if (!DBGetSchemaVersion(hdbBootstrap, &schemaVersionMajor, &schemaVersionMinor))
	{
	   nxlog_write_tag(NXLOG_ERROR, _T("db"), _T("Unable to get database schema version"));
      DBDisconnect(hdbBootstrap);
      return false;
	}

	if ((schemaVersionMajor != DB_SCHEMA_VERSION_MAJOR) || (schemaVersionMinor != DB_SCHEMA_VERSION_MINOR))
	{
		nxlog_write_tag(NXLOG_ERROR, _T("db"), _T("Your database has format version %d.%d, but server is compiled for version %d.%d"), schemaVersionMajor, schemaVersionMinor, DB_SCHEMA_VERSION_MAJOR, DB_SCHEMA_VERSION_MINOR);
		DBDisconnect(hdbBootstrap);
		return false;
	}

	// Read database syntax
	g_dbSyntax = DBGetSyntax(hdbBootstrap);

   if (g_dbSyntax == DB_SYNTAX_PGSQL)
   {
      DB_RESULT hResult = DBSelect(hdbBootstrap, _T("SELECT version()"));
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            char version[256];
            DBGetFieldA(hResult, 0, 0, version, 256);
            int major, minor;
            if (sscanf(version, "PostgreSQL %d.%d.", &major, &minor) == 2)
            {
               nxlog_debug_tag(_T("db"), 1, _T("Detected PostgreSQL version %d.%d"), major, minor);
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

      DB_RESULT hResult = DBSelect(hdbBootstrap, _T("SELECT extversion FROM pg_extension WHERE extname='timescaledb'"));
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            char version[256];
            DBGetFieldA(hResult, 0, 0, version, 256);
            int major, minor;
            if (sscanf(version, "%d.%d.", &major, &minor) == 2)
            {
               nxlog_debug_tag(_T("db"), 1, _T("Detected TimescaleDB version %d.%d"), major, minor);
               if (major >= 2)
               {
                  g_flags |= AF_TSDB_DROP_CHUNKS_V2;
               }
            }
         }
         DBFreeResult(hResult);
      }
   }
	else if (g_dbSyntax == DB_SYNTAX_SQLITE)
	{
      // Disable startup database cache for SQLite
      g_flags &= ~AF_CACHE_DB_ON_STARTUP;
	}

	int baseSize = ConfigReadIntEx(hdbBootstrap, _T("DBConnectionPool.BaseSize"), 10);
	int maxSize = ConfigReadIntEx(hdbBootstrap, _T("DBConnectionPool.MaxSize"), 30);
	int cooldownTime = ConfigReadIntEx(hdbBootstrap, _T("DBConnectionPool.CooldownTime"), 300);
	int ttl = ConfigReadIntEx(hdbBootstrap, _T("DBConnectionPool.MaxLifetime"), 14400);

   DBDisconnect(hdbBootstrap);

	if (!DBConnectionPoolStartup(g_dbDriver, g_szDbServer, g_szDbName, g_szDbLogin, g_szDbPassword, g_szDbSchema, baseSize, maxSize, cooldownTime, ttl))
	{
      nxlog_write_tag(NXLOG_ERROR, _T("db"), _T("Failed to initialize database connection pool"));
	   return false;
	}

   uint32_t lrt = ConfigReadULong(_T("LongRunningQueryThreshold"), 0);
   if (lrt != 0)
   {
      DBSetLongRunningThreshold(lrt);
      nxlog_write_tag(NXLOG_INFO, _T("db"), _T("Long running query threshold set at %u milliseconds"), lrt);
   }

   MetaDataPreLoad();

   // Read server ID
   TCHAR buffer[256];
   MetaDataReadStr(_T("ServerID"), buffer, 256, _T(""));
   Trim(buffer);
   if (buffer[0] != 0)
   {
      g_serverId = _tcstoull(buffer, nullptr, 16);
   }
   else
   {
      // Generate new ID
      g_serverId = ((UINT64)time(nullptr) << 31) | (UINT64)((UINT32)rand() & 0x7FFFFFFF);
      _sntprintf(buffer, 256, UINT64X_FMT(_T("016")), g_serverId);
      MetaDataWriteStr(_T("ServerID"), buffer);
   }
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("Server ID ") UINT64X_FMT(_T("016")), g_serverId);

   // Initialize locks
retry_db_lock:
   InetAddress addr;
   if (!LockDatabase(&addr, buffer))
   {
      if (addr.isValidUnicast())     // Database already locked by another server instance
      {
         // Check for lock from crashed/terminated local process
         if (IsLocalIPAddress(addr))
         {
            uint32_t pid = ConfigReadULong(_T("DBLockPID"), 0);
				if (!IsNetxmsdProcess(pid) || (pid == GetCurrentProcessId()))
				{
					UnlockDatabase();
               nxlog_write_tag(NXLOG_INFO, _T("db.lock"), _T("Stalled database lock removed"));
					goto retry_db_lock;
				}
			}
			else if (g_peerNodeAddrList.hasAddress(addr))
			{
			   if (!PeerNodeIsRunning(addr))
			   {
               UnlockDatabase();
               nxlog_write_tag(NXLOG_INFO, _T("db.lock"), _T("Stalled database lock removed"));
               goto retry_db_lock;
			   }
			}
			nxlog_write_tag(NXLOG_ERROR, _T("db.lock"), _T("Database is already locked by another NetXMS server instance (IP address: %s, machine info: %s)"), addr.toString().cstr(), buffer);
		}
      else if (!_tcsncmp(buffer, _T("NXDBMGR"), 7))
      {
         nxlog_write_tag(NXLOG_ERROR, _T("db.lock"), _T("Database is already locked by database manager"));
      }
		else
      {
         nxlog_write_tag(NXLOG_ERROR, _T("db.lock"), _T("Cannot lock database"));
      }
		return false;
	}
	g_flags |= AF_DB_LOCKED;
   nxlog_debug_tag(_T("db.lock"), 1, _T("Database lock set"));

   // Load global configuration parameters
   ConfigPreLoad();
   LoadGlobalConfig();
   CASReadSettings();
   nxlog_debug_tag(DEBUG_TAG_STARTUP, 1, _T("Global configuration loaded"));

   // Setup thread pool resize parameters
   ThreadPoolSetResizeParameters(
            ConfigReadInt(_T("ThreadPool.Global.Responsiveness"), 12),
            ConfigReadInt(_T("ThreadPool.Global.WaitTimeHighWatermark"), 100),
            ConfigReadInt(_T("ThreadPool.Global.WaitTimeLowWatermark"), 50));

   // Check data directory
   if (!CheckDataDirectory())
      return false;

   // Initialize cryptography
   if (!InitCryptography())
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_STARTUP, _T("Failed to initialize cryptography module"));
      return false;
   }

   // Initialize certificate store and CA
   InitCertificates();

   // Call custom initialization code
#ifdef CUSTOM_INIT_CODE
   if (!ServerCustomInit())
      return false;
#endif

   if (!InitWebAPI())
      return false;

   // Create thread pools
   nxlog_debug_tag(DEBUG_TAG_STARTUP, 2, _T("Creating thread pools"));
   g_mainThreadPool = ThreadPoolCreate(_T("MAIN"),
         ConfigReadInt(_T("ThreadPool.Main.BaseSize"), 8),
         ConfigReadInt(_T("ThreadPool.Main.MaxSize"), 256));
   g_agentConnectionThreadPool = ThreadPoolCreate(_T("AGENT"),
         ConfigReadInt(_T("ThreadPool.Agent.BaseSize"), 32),
         ConfigReadInt(_T("ThreadPool.Agent.MaxSize"), 256));
   g_pollerThreadPool = ThreadPoolCreate( _T("POLLERS"),
         ConfigReadInt(_T("ThreadPool.Poller.BaseSize"), 10),
         ConfigReadInt(_T("ThreadPool.Poller.MaxSize"), 250),
         256 * 1024);
   g_fileTransferThreadPool = ThreadPoolCreate( _T("FILE-TRANSFER"),
         ConfigReadInt(_T("ThreadPool.FileTransfer.BaseSize"), 2),
         ConfigReadInt(_T("ThreadPool.FileTransfer.MaxSize"), 16));

   // Setup unique identifiers table
   if (!InitIdTable())
      return false;
   nxlog_debug_tag(DEBUG_TAG_STARTUP, 2, _T("ID table created"));

   InitCountryList();
   InitCurrencyList();
   LoadOUIDatabase();
   LoadWellKnownPortList();

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
      return false;   // Mandatory module not loaded
   InitializeDeviceBackupInterface();
   RegisterPredictionEngines();
   InitAIAssistant();

   // Load users and authentication methods
   LoadTwoFactorAuthenticationMethods();
   InitUsers();
   if (!LoadUsers())
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_STARTUP, _T("Unable to load users and user groups from database (probably database is corrupted)"));
      return false;
   }
   LoadAuthenticationTokens();
   nxlog_debug_tag(DEBUG_TAG_STARTUP, 2, _T("User accounts loaded"));

   // Initialize audit
   InitAuditLog();

   // Initialize event handling subsystem
   if (!InitEventSubsystem())
      return false;

   // Initialize alarms
   LoadAlarmCategories();
   if (!InitAlarmManager())
      return false;

   // Initialize objects infrastructure and load objects from database
   LoadGeoAreas();
   LoadNetworkDeviceDrivers();
   ObjectsInit();
   LoadObjectCategories();
   LoadSshKeys();
   if (!LoadObjects())
      return false;
   nxlog_debug_tag(DEBUG_TAG_STARTUP, 1, _T("Objects loaded and initialized"));

   // Check if management node object presented in database
   CheckForMgmtNode();
   if (g_dwMgmtNode == 0)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_STARTUP, _T("NetXMS server cannot create node object for itself - probably because platform subagent cannot be loaded (check above error messages, if any)"));
      return false;
   }

   // Initialize and load event actions
   if (!LoadActions())
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_STARTUP, _T("Unable to initialize actions"));
      return false;
   }

   // Initialize notification channels
   LoadNotificationChannelDrivers();
   LoadNotificationChannels();

   // Initialize helpdesk link
   SetHDLinkEntryPoints(ResolveAlarmByHDRef, TerminateAlarmByHDRef, AddAlarmSystemComment);
   LoadHelpDeskLink();

   InitMappingTables();

   InitUserAgentNotifications();

   if (!LoadPhysicalLinks())
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_STARTUP, _T("Unable to load physical links"));
      return false;
   }

   // Initialize data collection subsystem
   LoadPerfDataStorageDrivers();
   LoadWebServiceDefinitions();
   InitDataCollector();
   LoadAssetManagementSchema();

   LoadObjectQueries();

   int importMode = ConfigReadInt(_T("Server.ImportConfigurationOnStartup"), 1);
   if (importMode > 0)
      ImportLocalConfiguration(importMode == 2);

   InitClientListeners();

   // Create syncer thread pool
   maxSize = ConfigReadInt(_T("ThreadPool.Syncer.MaxSize"), 1);
   if (maxSize > 1)
   {
      if (g_dbSyntax != DB_SYNTAX_MYSQL)
         g_syncerThreadPool = ThreadPoolCreate(_T("SYNCER"), ConfigReadInt(_T("ThreadPool.Syncer.BaseSize"), 1), maxSize);
      else
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_STARTUP, _T("Parallel syncer is not supported on MySQL/MariaDB"));
   }

   // Create network discovery thread pool
   g_discoveryThreadPool = ThreadPoolCreate(_T("DISCOVERY"), ConfigReadInt(_T("ThreadPool.Discovery.BaseSize"), 8), ConfigReadInt(_T("ThreadPool.Discovery.MaxSize"), 64));

   // Start threads
   ThreadCreate(DiscoveredAddressPoller);
   s_syncerThread = ThreadCreateEx(Syncer);

   Condition pollManagerInitialized(true);
   s_pollManagerThread = ThreadCreateEx(PollManager, &pollManagerInitialized);

   StartHouseKeeper();

   // Start event processor
   s_eventProcessorThread = StartEventProcessor();

   // Start SNMP agent and trap receiver
   LoadTrapMappings();
   if (ConfigReadBoolean(_T("SNMP.Traps.Enable"), true))
      StartSnmpTrapReceiver();
   StartSNMPAgent();

   StartSyslogServer();
   StartWindowsEventProcessing();

   // Start beacon host poller
   ThreadCreate(BeaconPoller);

   // Start inter-server communication listener
   if (ConfigReadBoolean(_T("EnableISCListener"), false))
      ThreadCreate(ISCListener);

   // Start reporting server connector
   if (ConfigReadBoolean(_T("ReportingServer.Enable"), false))
      ThreadCreate(ReportingServerConnector);

   // Start LDAP synchronization
   ThreadCreate(LDAPSyncThread);

   // Wait for initialization of critical threads
   pollManagerInitialized.wait(INFINITE);
   nxlog_debug_tag(DEBUG_TAG_STARTUP, 2, _T("Poll manager initialized"));

   RegisterSchedulerTaskHandler(_T("Agent.DeployPackage"), ExecuteScheduledPackageDeployment, SYSTEM_ACCESS_MANAGE_PACKAGES);
   RegisterSchedulerTaskHandler(_T("Agent.ExecuteCommand"), ExecuteScheduledAgentCommand, SYSTEM_ACCESS_SCHEDULE_SCRIPT);
   RegisterSchedulerTaskHandler(_T("DataCollection.RemoveTemplate"), DataCollectionTarget::removeTemplate, 0);
   RegisterSchedulerTaskHandler(_T("Dummy"), DummyScheduledTaskExecutor, SYSTEM_ACCESS_USER_SCHEDULED_TASKS);
   RegisterSchedulerTaskHandler(_T("Execute.Action"), ExecuteScheduledAction, SYSTEM_ACCESS_SCHEDULE_SCRIPT);
   RegisterSchedulerTaskHandler(_T("Execute.Script"), ExecuteScheduledScript, SYSTEM_ACCESS_SCHEDULE_SCRIPT);
   RegisterSchedulerTaskHandler(_T("Maintenance.Enter"), MaintenanceModeEnter, SYSTEM_ACCESS_SCHEDULE_MAINTENANCE);
   RegisterSchedulerTaskHandler(_T("Maintenance.Leave"), MaintenanceModeLeave, SYSTEM_ACCESS_SCHEDULE_MAINTENANCE);
   RegisterSchedulerTaskHandler(_T("Objects.ExpandCommentMacros"), ExpandCommentMacrosTask, 0);     // No access right because it will be used only by server
   RegisterSchedulerTaskHandler(_T("System.CheckUserAuthTokens"), CheckUserAuthenticationTokens, 0); //No access right because it will be used only by server
   RegisterSchedulerTaskHandler(_T("Upload.File"), ScheduledFileUpload, SYSTEM_ACCESS_SCHEDULE_FILE_UPLOAD);
   RegisterSchedulerTaskHandler(ALARM_SUMMARY_EMAIL_TASK_ID, SendAlarmSummaryEmail, 0); //No access right because it will be used only by server
   RegisterSchedulerTaskHandler(DCT_RESET_POLL_TIMERS_TASK_ID, ResetObjectPollTimers, 0); //No access right because it will be used only by server
   RegisterSchedulerTaskHandler(EXECUTE_REPORT_TASK_ID, ExecuteReport, SYSTEM_ACCESS_REPORTING_SERVER);
   RegisterSchedulerTaskHandler(RELOAD_CRLS_TASK_ID, ReloadCRLs, 0); //No access right because it will be used only by server
   RegisterSchedulerTaskHandler(RENEW_AGENT_CERTIFICATES_TASK_ID, RenewAgentCertificates, 0); //No access right because it will be used only by server
   RegisterSchedulerTaskHandler(UNBOUND_TUNNEL_PROCESSOR_TASK_ID, ProcessUnboundTunnels, 0); //No access right because it will be used only by server
   InitializeTaskScheduler();

   // Schedule unbound agent tunnel processing and automatic agent certificate renewal
   AddUniqueRecurrentScheduledTask(UNBOUND_TUNNEL_PROCESSOR_TASK_ID, _T("*/5 * * * *"), _T(""), nullptr, 0, 0, SYSTEM_ACCESS_FULL, _T("Process unbound agent tunnels"), nullptr, true);
   AddUniqueRecurrentScheduledTask(RENEW_AGENT_CERTIFICATES_TASK_ID, _T("0 12 * * *"), _T(""), nullptr, 0, 0, SYSTEM_ACCESS_FULL, _T("Renew agent certificates"), nullptr, true);

   // Send summary emails
   if (ConfigReadBoolean(_T("Alarms.SummaryEmail.Enable"), false))
      EnableAlarmSummaryEmails();
   else
      DeleteScheduledTaskByHandlerId(ALARM_SUMMARY_EMAIL_TASK_ID);

   // Schedule automatic CRL reload
   AddUniqueRecurrentScheduledTask(RELOAD_CRLS_TASK_ID, _T("0 */4 * * *"), _T(""), nullptr, 0, 0, SYSTEM_ACCESS_FULL, _T("Reload certificate revocation lists"), nullptr, true);

   // Schedule automatic comments macros expansion
   AddUniqueRecurrentScheduledTask(_T("Objects.ExpandCommentMacros"), _T("0 */4 * * *"), _T(""), nullptr, 0, 0, SYSTEM_ACCESS_FULL, _T("Expand macros in object comments"), nullptr, true);

   // Schedule poll timers reset
   AddUniqueRecurrentScheduledTask(DCT_RESET_POLL_TIMERS_TASK_ID, _T("0 0 1 * *"), _T(""), nullptr, 0, 0, SYSTEM_ACCESS_FULL, _T(""), nullptr, true);

   // Schedule checks of user authentication tokens
   AddUniqueRecurrentScheduledTask(_T("System.CheckUserAuthTokens"), _T("0 * * * *"), _T(""), nullptr, 0, 0, SYSTEM_ACCESS_FULL, _T("Check for expired user authentication tokens"), nullptr, true);

   // Start listeners
   s_tunnelListenerThread = ThreadCreateEx(TunnelListenerThread);
   s_clientListenerThread = ThreadCreateEx(ClientListenerThread);
   InitMobileDeviceListeners();
   s_mobileDeviceListenerThread = ThreadCreateEx(MobileDeviceListenerThread);

   // Start web server
   StartWebAPI();

   StartPackageDeploymentManager();

   // Validate scripts in script library
   ValidateScripts();

   // Call startup functions for the modules
   CALL_ALL_MODULES(pfServerStarted, ());

   ExecuteStartupScripts();
   CheckNodeCountRestrictions();

   // Internal stat collector should be started last when all queues
   // and thread pools already created
   s_statCollectorThread = ThreadCreateEx(ServerStatCollector);

   g_flags |= AF_SERVER_INITIALIZED;
   PostSystemEvent(EVENT_SERVER_STARTED, g_dwMgmtNode);
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("Server initialization completed in %d milliseconds"), static_cast<int>(GetCurrentTimeMs() - initStartTime));
   return true;
}

/**
 * Shutdown reason texts
 */
static const TCHAR *s_shutdownReasonText[] = { _T("due to unknown reason"), _T("from local console"), _T("from remote console"), _T("by signal"), _T("by service manager") };

/**
 * Server shutdown
 */
void NXCORE_EXPORTABLE Shutdown()
{
   // Notify clients
   NotifyClientSessions(NX_NOTIFY_SHUTDOWN, 0);

   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_SHUTDOWN, _T("NetXMS Server stopped %s"), s_shutdownReasonText[static_cast<int>(s_shutdownReason)]);
   InterlockedOr64(&g_flags, AF_SHUTDOWN);     // Set shutdown flag
   InitiateProcessShutdown();

   // Call shutdown functions for the modules
   // CALL_ALL_MODULES cannot be used here because it checks for shutdown flag
   for(int i = 0; i < g_moduleList.size(); i++)
   {
      if (g_moduleList.get(i)->pfShutdown != nullptr)
         g_moduleList.get(i)->pfShutdown();
   }

   ShutdownWebAPI();

   ThreadJoin(s_statCollectorThread);

   StopPackageDeploymentManager();

   StopHouseKeeper();
   ShutdownTaskScheduler();

   StopSNMPAgent();
   StopSnmpTrapReceiver();

   // Stop DCI cache loading thread
   g_dciCacheLoaderQueue.setShutdownMode();

	ShutdownPredictionEngines();
   StopObjectMaintenanceThreads();
   StopDataCollection();

   // Wait for critical threads
   ThreadJoin(s_pollManagerThread);
   ThreadJoin(s_syncerThread);

   nxlog_debug_tag(DEBUG_TAG_SHUTDOWN, 2, _T("Waiting for listener threads to stop"));
   ThreadJoin(s_tunnelListenerThread);
   ThreadJoin(s_clientListenerThread);
   ThreadJoin(s_mobileDeviceListenerThread);

   CloseAgentTunnels();
   StopSyslogServer();
   StopWindowsEventProcessing();

   nxlog_debug_tag(DEBUG_TAG_SHUTDOWN, 2, _T("Waiting for event processor to stop"));
   g_eventQueue.put(INVALID_POINTER_VALUE);
   ThreadJoin(s_eventProcessorThread);

   ThreadSleep(1);     // Give other threads a chance to terminate in a safe way
   nxlog_debug_tag(DEBUG_TAG_SHUTDOWN, 2, _T("All threads were notified, continue with shutdown"));

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   SaveObjects(hdb, INVALID_INDEX, true);
   nxlog_debug_tag(DEBUG_TAG_SHUTDOWN, 2, _T("All objects saved to database"));
   SaveUsers(hdb, INVALID_INDEX);
   nxlog_debug_tag(DEBUG_TAG_SHUTDOWN, 2, _T("All users saved to database"));
   UpdatePStorageDatabase(hdb, INVALID_INDEX);
   nxlog_debug_tag(DEBUG_TAG_SHUTDOWN, 2, _T("All persistent storage values saved"));
   DBConnectionPoolReleaseConnection(hdb);

	if (g_syncerThreadPool != nullptr)
	   ThreadPoolDestroy(g_syncerThreadPool);

   ThreadPoolDestroy(g_discoveryThreadPool);

   StopDBWriter();
   nxlog_debug_tag(DEBUG_TAG_SHUTDOWN, 1, _T("Database writer stopped"));

   CleanupUsers();
   PersistentStorageDestroy();

   ShutdownPerfDataStorageDrivers();

   CleanupActions();
   ShutdownEventSubsystem();
   ShutdownAlarmManager();
   ShutdownNotificationChannels();
   nxlog_debug_tag(DEBUG_TAG_SHUTDOWN, 1, _T("Event processing stopped"));

   DisableAgentConnections();
   CleanupObjects();

   ThreadPoolDestroy(g_clientThreadPool);
   ThreadPoolDestroy(g_agentConnectionThreadPool);
   ThreadPoolDestroy(g_mainThreadPool);
   WatchdogShutdown();

   SaveCurrentFreeId();

	// Remove database lock
	UnlockDatabase();

	DBConnectionPoolShutdown();
	DBUnloadDriver(g_dbDriver);
	nxlog_debug_tag(DEBUG_TAG_SHUTDOWN, 1, _T("Database driver unloaded"));

	nxlog_debug_tag(DEBUG_TAG_SHUTDOWN, 1, _T("Server shutdown complete"));
	nxlog_close();

   RSAFree(g_serverKey);
   CleanupServerCertificates();

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
void NXCORE_EXPORTABLE FastShutdown(ShutdownReason reason)
{
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_SHUTDOWN, _T("NetXMS Server stopped %s using fast shutdown procedure"), s_shutdownReasonText[static_cast<int>(s_shutdownReason)]);

   InterlockedOr64(&g_flags, AF_SHUTDOWN);     // Set shutdown flag
	InitiateProcessShutdown();

	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	SaveObjects(hdb, INVALID_INDEX, true);
	nxlog_debug_tag(DEBUG_TAG_SHUTDOWN, 2, _T("All objects saved to database"));
	SaveUsers(hdb, INVALID_INDEX);
	nxlog_debug_tag(DEBUG_TAG_SHUTDOWN, 2, _T("All users saved to database"));
   UpdatePStorageDatabase(hdb, INVALID_INDEX);
   nxlog_debug_tag(DEBUG_TAG_SHUTDOWN, 2, _T("All persistent storage values saved"));
	DBConnectionPoolReleaseConnection(hdb);

	// Remove database lock first, because we have a chance to lose DB connection
	UnlockDatabase();

	// Stop database writers
	StopDBWriter();
	nxlog_debug_tag(DEBUG_TAG_SHUTDOWN, 1, _T("Database writer stopped"));

	nxlog_debug_tag(DEBUG_TAG_SHUTDOWN, 1, _T("Server shutdown complete"));
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
				      if (s_shutdownReason == ShutdownReason::OTHER)
				         s_shutdownReason = ShutdownReason::BY_SIGNAL;
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

#if HAVE_LIBEDIT

/**
 * Get command line prompt
 */
static wchar_t *Prompt(EditLine *el)
{
   //static TCHAR prompt[] = _T("\1\x1b[33m\1netxmsd:\1\x1b[0m\1 ");
   static TCHAR prompt[] = _T("netxmsd: ");
   return prompt;
}

#endif

/**
 * Common main()
 */
THREAD_RESULT NXCORE_EXPORTABLE THREAD_CALL Main(void *pArg)
{
	nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_STARTUP, _T("NetXMS Server started"));

	if (IsStandalone())
   {
      if (!(g_flags & AF_DEBUG_CONSOLE_DISABLED))
	   {
#if HAVE_LIBEDIT
         HistoryT *h = history_tinit();

         HistEventT ev;
         history_t(h, &ev, H_SETSIZE, 100);  // Remember 100 events
         history_t(h, &ev, H_LOAD, ".netxmsd_history");

         EditLine *el = el_init("netxmsd", stdin, stdout, stderr);
         el_tset(el, EL_PROMPT, Prompt);
         el_tset(el, EL_EDITOR, "emacs");
         el_tset(el, EL_HIST, history_t, h);
         el_tset(el, EL_BIND, _T("^K"), _T("ed-kill-line"), nullptr);
         el_tset(el, EL_BIND, _T("^L"), _T("ed-clear-screen"), nullptr);
         el_source(el, nullptr);
#endif

		   WriteToTerminal(L"\nNetXMS Server V" NETXMS_VERSION_STRING L" Build " NETXMS_BUILD_TAG L" Ready\n"
				             L"Enter \"\x1b[1mhelp\x1b[0m\" for command list or \"\x1b[1mdown\x1b[0m\" for server shutdown\n"
				             L"System Console\n\n");

		   wchar_t command[256];
         LocalTerminalConsole ctx;
		   while(true)
		   {
#if HAVE_LIBEDIT
            int numchars;
            const wchar_t *line = el_tgets(el, &numchars);
            if ((line == nullptr) || (numchars == 0))
               break;

            wcslcpy(command, line, 256);
#else
			   WriteToTerminal(L"\x1b[33mnetxmsd:\x1b[0m ");
			   fflush(stdout);
			   if (_fgetts(command, 255, stdin) == nullptr)
				   break;   // Error reading stdin
#endif

			   Trim(command);
            if (command[0] == '!')
            {
#if HAVE_LIBEDIT
               HistEventT p;
               int rc;
               if (command[1] == '!')
               {
                  rc = history_t(h, &p, H_CURR);
               }
               else
               {
                  int n = _tcstol(&command[1], nullptr, 10);
                  if (n < 0)
                  {
                     n = -n;
                     while(n-- > 0)
                     {
                        rc = history_t(h, &p, H_NEXT);
                     }
                  }
                  else if (n > 0)
                  {
                     rc = history_t(h, &p, H_LAST);
                     while((--n > 0) && (rc >= 0))
                     {
                        rc = history_t(h, &p, H_PREV);
                     }
                  }
                  else
                  {
                     rc = -1;
                  }
                  HistEventT tmp;
                  history_t(h, &tmp, H_FIRST);
               }
               if (rc >= 0)
               {
                  line = p.str;
                  _tcslcpy(command, p.str, 256);
                  Trim(command);
                  WriteToTerminal(line);
               }
               else
               {
                  command[0] = 0;
               }
#else
               command[0] = 0;
#endif
            }
			   if (command[0] != 0)
			   {
#if HAVE_LIBEDIT
               history_t(h, &ev, H_ENTER, line);
#endif
               if (ProcessConsoleCommand(command, &ctx) == CMD_EXIT_SHUTDOWN)
                  break;
			   }
			   else
			   {
#if !HAVE_LIBEDIT
			      WriteToTerminal(L"\n");
#endif
			   }
		   }

#if HAVE_LIBEDIT
		   el_reset(el);
         el_end(el);
         history_t(h, &ev, H_SAVE, ".netxmsd_history");
         history_tend(h);
#endif

   		if (!(g_flags & AF_SHUTDOWN))
   		{
   		   s_shutdownReason = ShutdownReason::FROM_LOCAL_CONSOLE;
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
         WriteToTerminal(_T("Server running. Press Ctrl+C to shutdown.\n"));
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
void NXCORE_EXPORTABLE InitiateShutdown(ShutdownReason reason)
{
   s_shutdownReason = reason;
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
