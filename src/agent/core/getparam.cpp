/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2019 Victor Kirhenshtein
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
** File: getparam.cpp
**
**/

#include "nxagentd.h"

#if defined(_WIN32)
#include <intrin.h>
#elif HAVE_CPUID_H
#include <cpuid.h>
#endif

/**
 * Parameter handlers
 */
LONG H_ActionList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_ActiveConnections(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_AgentProxyStats(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_AgentTraps(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_AgentUptime(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_CRC32(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_DataCollectorQueueSize(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_DirInfo(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_ExternalParameter(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_ExternalList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_ExternalTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_FileTime(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_HostName(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_IsExtSubagentConnected(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_IsSubagentLoaded(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_MD5Hash(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_PlatformName(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_PushValue(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_PushValues(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_ResolverAddrByName(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_ResolverAddrByNameList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_ResolverNameByAddr(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_SessionAgents(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_SHA1Hash(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_SNMPProxyStats(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_SubAgentList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_SubAgentTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_SyslogStats(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_SystemTime(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_ThreadPoolInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_ThreadPoolList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_LineCount(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_ZoneConfigurations(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_ZoneProxies(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session);

#ifdef _WIN32
LONG H_PhysicalDiskInfo(const TCHAR *cmd, const TCHAR *arg, TCHAR *pValue, AbstractCommSession *session);
#endif

/**
 * Static data
 */
static NETXMS_SUBAGENT_PARAM *m_pParamList = NULL;
static int m_iNumParams = 0;
static NETXMS_SUBAGENT_PUSHPARAM *m_pPushParamList = NULL;
static int m_iNumPushParams = 0;
static NETXMS_SUBAGENT_LIST *m_pEnumList = NULL;
static int m_iNumEnums = 0;
static NETXMS_SUBAGENT_TABLE *m_pTableList = NULL;
static int m_iNumTables = 0;
static UINT32 m_dwTimedOutRequests = 0;
static UINT32 m_dwAuthenticationFailures = 0;
static UINT32 m_dwProcessedRequests = 0;
static UINT32 m_dwFailedRequests = 0;
static UINT32 m_dwUnsupportedRequests = 0;

/**
 * Handler for parameters which always returns string constant
 */
static LONG H_StringConstant(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   ret_string(value, arg);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for parameters which returns UINT32 value from specific variable
 */
static LONG H_UIntPtr(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   ret_uint(value, *((UINT32 *)arg));
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Agent ID
 */
static LONG H_AgentID(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   g_agentId.toString(value);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Agent.SupportedCiphers
 */
static LONG H_SupportedCiphers(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   UINT32 dwCiphers;

   dwCiphers = NXCPGetSupportedCiphers();
   if (dwCiphers == 0)
   {
      ret_string(pValue, _T("NONE"));
   }
   else
   {
      *pValue = 0;
      if (dwCiphers & NXCP_SUPPORT_AES_256)
         _tcscat(pValue, _T("AES-256 "));
      if (dwCiphers & NXCP_SUPPORT_AES_128)
         _tcscat(pValue, _T("AES-128 "));
      if (dwCiphers & NXCP_SUPPORT_BLOWFISH_256)
         _tcscat(pValue, _T("BF-256 "));
      if (dwCiphers & NXCP_SUPPORT_BLOWFISH_128)
         _tcscat(pValue, _T("BF-128 "));
      if (dwCiphers & NXCP_SUPPORT_IDEA)
         _tcscat(pValue, _T("IDEA "));
      if (dwCiphers & NXCP_SUPPORT_3DES)
         _tcscat(pValue, _T("3DES "));
      pValue[_tcslen(pValue) - 1] = 0;
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for parameters list
 */
static LONG H_ParamList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   int i;

   for(i = 0; i < m_iNumParams; i++)
		if (m_pParamList[i].dataType != DCI_DT_DEPRECATED)
			value->add(m_pParamList[i].name);
	ListParametersFromExtProviders(value);
	ListParametersFromExtSubagents(value);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for push parameters list
 */
static LONG H_PushParamList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   int i;

   for(i = 0; i < m_iNumPushParams; i++)
		value->add(m_pPushParamList[i].name);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for enums list
 */
static LONG H_EnumList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   int i;

   for(i = 0; i < m_iNumEnums; i++)
		value->add(m_pEnumList[i].name);
	ListListsFromExtSubagents(value);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for table list
 */
static LONG H_TableList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   int i;

   for(i = 0; i < m_iNumTables; i++)
		value->add(m_pTableList[i].name);
	ListTablesFromExtSubagents(value);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for component statuses based on failure flags
 */
static LONG H_ComponentStatus(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   UINT32 result = 0;
   switch(*pArg)
   {
      case 'D':
         if ((g_failFlags & FAIL_OPEN_DATABASE) != 0)
            result++;
         if ((g_failFlags & FAIL_UPGRADE_DATABASE) != 0)
            result++;
         break;
      case 'L':
         if ((g_failFlags & FAIL_OPEN_LOG) != 0)
            result++;
         break;
   }

   ret_uint(pValue, result);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for local database counters
 */
static LONG H_LocalDatabaseCounters(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   LIBNXDB_PERF_COUNTERS counters;
   DBGetPerfCounters(&counters);
   switch(*pArg)
   {
      case 'F':
         ret_int64(pValue, counters.failedQueries);
         break;
      case 'L':
         ret_int64(pValue, counters.longRunningQueries);
         break;
      case 'T':
         ret_int64(pValue, counters.totalQueries);
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for flag query parameters
 */
static LONG H_FlagValue(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   UINT32 flag = CAST_FROM_POINTER(arg, UINT32);
   ret_int(value, (g_dwFlags & flag) ? 1 : 0);
   return SYSINFO_RC_SUCCESS;
}

#if HAVE_GET_CPUID

/**
 * Handler for System.IsVirtual parameter
 */
static LONG H_SystemIsVirtual(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   unsigned int eax, ebx, ecx, edx;
   if (__get_cpuid(0x1, &eax, &ebx, &ecx, &edx) != 1)
      return SYSINFO_RC_UNSUPPORTED;
   ret_int(value, (ecx & 0x80000000) != 0 ? 1 : 0);
   return SYSINFO_RC_SUCCESS;
}

#elif defined(_WIN32)

/**
 * Handler for System.IsVirtual parameter
 */
static LONG H_SystemIsVirtual(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   int data[4];
   __cpuid(data, 0x01);
   if ((data[2] & 0x80000000) != 0)
   {
      // Check if running on Hyper-V
      __cpuid(data, 0x40000000);
      if (!memcmp((char *)&data[1], "Microsoft Hv", 12))
      {
         // Hyper-V, check if this OS running in root patition
         __cpuid(data, 0x40000003);
         if (data[1] & 0x01)
         {
            // Root partition
            ret_int(value, 0);
         }
         else
         {
            // Guest partition
            ret_int(value, 1);
         }
      }
      else
      {
         // Other virtualization product, assume guest OS
         ret_int(value, 1);
      }
   }
   else
   {
      ret_int(value, 0);
   }
   return SYSINFO_RC_SUCCESS;
}

#endif /* HAVE_GET_CPUID || _WIN32 */

/**
 * Handler for Agent.Heap.Active
 */
static LONG H_AgentHeapActive(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   INT64 bytes = GetActiveHeapMemory();
   if (bytes == -1)
      return SYSINFO_RC_UNSUPPORTED;
   ret_int64(value, bytes);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Agent.Heap.Allocated
 */
static LONG H_AgentHeapAllocated(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   INT64 bytes = GetAllocatedHeapMemory();
   if (bytes == -1)
      return SYSINFO_RC_UNSUPPORTED;
   ret_int64(value, bytes);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Agent.Heap.Mapped
 */
static LONG H_AgentHeapMapped(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   INT64 bytes = GetMappedHeapMemory();
   if (bytes == -1)
      return SYSINFO_RC_UNSUPPORTED;
   ret_int64(value, bytes);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Standard agent's parameters
 */
static NETXMS_SUBAGENT_PARAM m_stdParams[] =
{
#ifdef _WIN32
   { _T("PhysicalDisk.Firmware(*)"), H_PhysicalDiskInfo, _T("F"), DCI_DT_STRING, DCIDESC_PHYSICALDISK_FIRMWARE },
   { _T("PhysicalDisk.Model(*)"), H_PhysicalDiskInfo, _T("M"), DCI_DT_STRING, DCIDESC_PHYSICALDISK_MODEL },
   { _T("PhysicalDisk.SerialNumber(*)"), H_PhysicalDiskInfo, _T("N"), DCI_DT_STRING, DCIDESC_PHYSICALDISK_SERIALNUMBER },
   { _T("PhysicalDisk.SmartAttr(*)"), H_PhysicalDiskInfo, _T("A"), DCI_DT_STRING, DCIDESC_PHYSICALDISK_SMARTATTR },
   { _T("PhysicalDisk.SmartStatus(*)"), H_PhysicalDiskInfo, _T("S"), DCI_DT_INT, DCIDESC_PHYSICALDISK_SMARTSTATUS },
   { _T("PhysicalDisk.Temperature(*)"), H_PhysicalDiskInfo, _T("T"), DCI_DT_INT, DCIDESC_PHYSICALDISK_TEMPERATURE },
   { _T("System.IsVirtual"), H_SystemIsVirtual, NULL, DCI_DT_INT, DCIDESC_SYSTEM_IS_VIRTUAL },
#endif

#if HAVE_GET_CPUID
   { _T("System.IsVirtual"), H_SystemIsVirtual, NULL, DCI_DT_INT, DCIDESC_SYSTEM_IS_VIRTUAL },
#endif

   { _T("Agent.AcceptedConnections"), H_UIntPtr, (TCHAR *)&g_acceptedConnections, DCI_DT_COUNTER32, DCIDESC_AGENT_ACCEPTEDCONNECTIONS },
   { _T("Agent.AcceptErrors"), H_UIntPtr, (TCHAR *)&g_acceptErrors, DCI_DT_COUNTER32, DCIDESC_AGENT_ACCEPTERRORS },
   { _T("Agent.ActiveConnections"), H_ActiveConnections, NULL, DCI_DT_UINT, DCIDESC_AGENT_ACTIVECONNECTIONS },
   { _T("Agent.AuthenticationFailures"), H_UIntPtr, (TCHAR *)&m_dwAuthenticationFailures, DCI_DT_COUNTER32, DCIDESC_AGENT_AUTHENTICATIONFAILURES },
   { _T("Agent.ConfigurationServer"), H_StringConstant, g_szConfigServer, DCI_DT_STRING, DCIDESC_AGENT_CONFIG_SERVER },
   { _T("Agent.DataCollectorQueueSize"), H_DataCollectorQueueSize, NULL, DCI_DT_UINT, DCIDESC_AGENT_DATACOLLQUEUESIZE },
   { _T("Agent.FailedRequests"), H_UIntPtr, (TCHAR *)&m_dwFailedRequests, DCI_DT_COUNTER32, DCIDESC_AGENT_FAILEDREQUESTS },
   { _T("Agent.GeneratedTraps"), H_AgentTraps, _T("G"), DCI_DT_COUNTER64, DCIDESC_AGENT_GENERATED_TRAPS },
   { _T("Agent.Heap.Active"), H_AgentHeapActive, NULL, DCI_DT_UINT64, DCIDESC_AGENT_HEAP_ACTIVE },
   { _T("Agent.Heap.Allocated"), H_AgentHeapAllocated, NULL, DCI_DT_UINT64, DCIDESC_AGENT_HEAP_ALLOCATED },
   { _T("Agent.Heap.Mapped"), H_AgentHeapMapped, NULL, DCI_DT_UINT64, DCIDESC_AGENT_HEAP_MAPPED },
   { _T("Agent.ID"), H_AgentID, NULL, DCI_DT_STRING, DCIDESC_AGENT_ID },
   { _T("Agent.IsSubagentLoaded(*)"), H_IsSubagentLoaded, NULL, DCI_DT_INT, DCIDESC_AGENT_IS_SUBAGENT_LOADED },
   { _T("Agent.IsExternalSubagentConnected(*)"), H_IsExtSubagentConnected, NULL, DCI_DT_INT, DCIDESC_AGENT_IS_EXT_SUBAGENT_CONNECTED },
   { _T("Agent.LastTrapTime"), H_AgentTraps, _T("T"), DCI_DT_UINT64, DCIDESC_AGENT_LAST_TRAP_TIME },
   { _T("Agent.LocalDatabase.FailedQueries"), H_LocalDatabaseCounters, _T("F"), DCI_DT_COUNTER64, DCIDESC_AGENT_LOCALDB_FAILED_QUERIES },
   { _T("Agent.LocalDatabase.LongRunningQueries"), H_LocalDatabaseCounters, _T("L"), DCI_DT_COUNTER64, DCIDESC_AGENT_LOCALDB_SLOW_QUERIES },
   { _T("Agent.LocalDatabase.Status"), H_ComponentStatus, _T("D"), DCI_DT_UINT, DCIDESC_AGENT_LOCALDB_STATUS },
   { _T("Agent.LocalDatabase.TotalQueries"), H_LocalDatabaseCounters, _T("T"), DCI_DT_COUNTER64, DCIDESC_AGENT_LOCALDB_TOTAL_QUERIES },
   { _T("Agent.LogFile.Status"), H_ComponentStatus, _T("L"), DCI_DT_UINT, DCIDESC_AGENT_LOG_STATUS },
   { _T("Agent.ProcessedRequests"), H_UIntPtr, (TCHAR *)&m_dwProcessedRequests, DCI_DT_COUNTER32, DCIDESC_AGENT_PROCESSEDREQUESTS },
   { _T("Agent.Proxy.ActiveSessions"), H_AgentProxyStats, _T("A"), DCI_DT_UINT, DCIDESC_AGENT_PROXY_ACTIVESESSIONS },
   { _T("Agent.Proxy.ConnectionRequests"), H_AgentProxyStats, _T("C"), DCI_DT_COUNTER64, DCIDESC_AGENT_PROXY_CONNECTIONREQUESTS },
   { _T("Agent.Proxy.IsEnabled"), H_FlagValue, CAST_TO_POINTER(AF_ENABLE_PROXY, TCHAR *), DCI_DT_UINT, DCIDESC_AGENT_PROXY_ISENABLED },
   { _T("Agent.PushValue(*)"), H_PushValue, NULL, DCI_DT_STRING, DCIDESC_AGENT_PUSH_VALUE },
   { _T("Agent.Registrar"), H_StringConstant, g_szRegistrar, DCI_DT_STRING, DCIDESC_AGENT_REGISTRAR },
   { _T("Agent.RejectedConnections"), H_UIntPtr, (TCHAR *)&g_rejectedConnections, DCI_DT_COUNTER32, DCIDESC_AGENT_REJECTEDCONNECTIONS },
   { _T("Agent.SentTraps"), H_AgentTraps, _T("S"), DCI_DT_COUNTER64, DCIDESC_AGENT_SENT_TRAPS },
   { _T("Agent.SNMP.IsProxyEnabled"), H_FlagValue, CAST_TO_POINTER(AF_ENABLE_SNMP_PROXY, TCHAR *), DCI_DT_UINT, DCIDESC_AGENT_SNMP_ISPROXYENABLED },
   { _T("Agent.SNMP.IsTrapProxyEnabled"), H_FlagValue, CAST_TO_POINTER(AF_ENABLE_SNMP_TRAP_PROXY, TCHAR *), DCI_DT_UINT, DCIDESC_AGENT_SNMP_ISPROXYENABLED },
   { _T("Agent.SNMP.Requests"), H_SNMPProxyStats, _T("R"), DCI_DT_COUNTER64, DCIDESC_AGENT_SNMP_REQUESTS },
   { _T("Agent.SNMP.Responses"), H_SNMPProxyStats, _T("r"), DCI_DT_COUNTER64, DCIDESC_AGENT_SNMP_RESPONSES },
   { _T("Agent.SNMP.ServerRequests"), H_SNMPProxyStats, _T("S"), DCI_DT_COUNTER64, DCIDESC_AGENT_SNMP_SERVERREQUESTS },
   { _T("Agent.SourcePackageSupport"), H_StringConstant, _T("0"), DCI_DT_INT, DCIDESC_AGENT_SOURCEPACKAGESUPPORT },
   { _T("Agent.SupportedCiphers"), H_SupportedCiphers, NULL, DCI_DT_STRING, DCIDESC_AGENT_SUPPORTEDCIPHERS },
   { _T("Agent.SyslogProxy.IsEnabled"), H_FlagValue, CAST_TO_POINTER(AF_ENABLE_SYSLOG_PROXY, TCHAR *), DCI_DT_UINT, DCIDESC_AGENT_SYSLOGPROXY_ISENABLED },
   { _T("Agent.SyslogProxy.ReceivedMessages"), H_SyslogStats, _T("R"), DCI_DT_COUNTER64, DCIDESC_AGENT_SYSLOGPROXY_RECEIVEDMSGS },
   { _T("Agent.SyslogProxy.QueueSize"), H_SyslogStats, _T("Q"), DCI_DT_UINT, DCIDESC_AGENT_SYSLOGPROXY_QUEUESIZE },
   { _T("Agent.ThreadPool.ActiveRequests(*)"), H_ThreadPoolInfo, (TCHAR *)THREAD_POOL_ACTIVE_REQUESTS, DCI_DT_UINT, DCIDESC_AGENT_THREADPOOL_ACTIVEREQUESTS },
   { _T("Agent.ThreadPool.CurrSize(*)"), H_ThreadPoolInfo, (TCHAR *)THREAD_POOL_CURR_SIZE, DCI_DT_UINT, DCIDESC_AGENT_THREADPOOL_CURRSIZE },
   { _T("Agent.ThreadPool.Load(*)"), H_ThreadPoolInfo, (TCHAR *)THREAD_POOL_LOAD, DCI_DT_UINT, DCIDESC_AGENT_THREADPOOL_LOAD },
   { _T("Agent.ThreadPool.LoadAverage(*)"), H_ThreadPoolInfo, (TCHAR *)THREAD_POOL_LOADAVG_1, DCI_DT_UINT, DCIDESC_AGENT_THREADPOOL_LOADAVG },
   { _T("Agent.ThreadPool.LoadAverage5(*)"), H_ThreadPoolInfo, (TCHAR *)THREAD_POOL_LOADAVG_5, DCI_DT_UINT, DCIDESC_AGENT_THREADPOOL_LOADAVG_5 },
   { _T("Agent.ThreadPool.LoadAverage15(*)"), H_ThreadPoolInfo, (TCHAR *)THREAD_POOL_LOADAVG_15, DCI_DT_UINT, DCIDESC_AGENT_THREADPOOL_LOADAVG_15 },
   { _T("Agent.ThreadPool.MaxSize(*)"), H_ThreadPoolInfo, (TCHAR *)THREAD_POOL_MAX_SIZE, DCI_DT_UINT, DCIDESC_AGENT_THREADPOOL_MAXSIZE },
   { _T("Agent.ThreadPool.MinSize(*)"), H_ThreadPoolInfo, (TCHAR *)THREAD_POOL_MIN_SIZE, DCI_DT_UINT, DCIDESC_AGENT_THREADPOOL_MINSIZE },
   { _T("Agent.ThreadPool.ScheduledRequests(*)"), H_ThreadPoolInfo, (TCHAR *)THREAD_POOL_SCHEDULED_REQUESTS, DCI_DT_UINT, DCIDESC_AGENT_THREADPOOL_SCHEDULEDREQUESTS },
   { _T("Agent.ThreadPool.Usage(*)"), H_ThreadPoolInfo, (TCHAR *)THREAD_POOL_USAGE, DCI_DT_UINT, DCIDESC_AGENT_THREADPOOL_USAGE },
   { _T("Agent.TimedOutRequests"), H_UIntPtr, (TCHAR *)&m_dwTimedOutRequests, DCI_DT_COUNTER32, DCIDESC_AGENT_TIMEDOUTREQUESTS },
   { _T("Agent.UnsupportedRequests"), H_UIntPtr, (TCHAR *)&m_dwUnsupportedRequests, DCI_DT_COUNTER32, DCIDESC_AGENT_UNSUPPORTEDREQUESTS },
   { _T("Agent.Uptime"), H_AgentUptime, NULL, DCI_DT_UINT, DCIDESC_AGENT_UPTIME },
   { _T("Agent.Version"), H_StringConstant, NETXMS_BUILD_TAG, DCI_DT_STRING, DCIDESC_AGENT_VERSION },
   { _T("File.Count(*)"), H_DirInfo, (TCHAR *)DIRINFO_FILE_COUNT, DCI_DT_UINT, DCIDESC_FILE_COUNT },
   { _T("File.FolderCount(*)"), H_DirInfo, (TCHAR *)DIRINFO_FOLDER_COUNT, DCI_DT_UINT, DCIDESC_FILE_FOLDERCOUNT },
   { _T("File.Hash.CRC32(*)"), H_CRC32, NULL, DCI_DT_UINT, DCIDESC_FILE_HASH_CRC32 },
   { _T("File.Hash.MD5(*)"), H_MD5Hash, NULL, DCI_DT_STRING, DCIDESC_FILE_HASH_MD5 },
   { _T("File.Hash.SHA1(*)"), H_SHA1Hash, NULL, DCI_DT_STRING, DCIDESC_FILE_HASH_SHA1 },
   { _T("File.Size(*)"), H_DirInfo, (TCHAR *)DIRINFO_FILE_SIZE, DCI_DT_UINT64, DCIDESC_FILE_SIZE },
   { _T("File.LineCount(*)"), H_LineCount, (TCHAR *)DIRINFO_FILE_LINE_COUNT, DCI_DT_UINT64, _T("File line count {instance}") },
   { _T("File.Time.Access(*)"), H_FileTime, (TCHAR *)FILETIME_ATIME, DCI_DT_UINT64, DCIDESC_FILE_TIME_ACCESS },
   { _T("File.Time.Change(*)"), H_FileTime, (TCHAR *)FILETIME_CTIME, DCI_DT_UINT64, DCIDESC_FILE_TIME_CHANGE },
   { _T("File.Time.Modify(*)"), H_FileTime, (TCHAR *)FILETIME_MTIME, DCI_DT_UINT64, DCIDESC_FILE_TIME_MODIFY },
   { _T("Net.Resolver.AddressByName(*)"), H_ResolverAddrByName, NULL, DCI_DT_STRING, DCIDESC_NET_RESOLVER_ADDRBYNAME },
   { _T("Net.Resolver.NameByAddress(*)"), H_ResolverNameByAddr, NULL, DCI_DT_STRING, DCIDESC_NET_RESOLVER_NAMEBYADDR },
   { _T("System.CurrentTime"), H_SystemTime, NULL, DCI_DT_INT64, DCIDESC_SYSTEM_CURRENTTIME },
   { _T("System.FQDN"), H_HostName, _T("FQDN"), DCI_DT_STRING, DCIDESC_SYSTEM_FQDN },
   { _T("System.Hostname"), H_HostName, NULL, DCI_DT_STRING, DCIDESC_SYSTEM_HOSTNAME },
   { _T("System.PlatformName"), H_PlatformName, NULL, DCI_DT_STRING, DCIDESC_SYSTEM_PLATFORMNAME }
};

/**
 * Standard agent's lists
 */
static NETXMS_SUBAGENT_LIST m_stdLists[] =
{
   { _T("Agent.ActionList"), H_ActionList, NULL },
   { _T("Agent.PushValues"), H_PushValues, NULL },
   { _T("Agent.SubAgentList"), H_SubAgentList, NULL },
   { _T("Agent.SupportedLists"), H_EnumList, NULL },
   { _T("Agent.SupportedParameters"), H_ParamList, NULL },
   { _T("Agent.SupportedPushParameters"), H_PushParamList, NULL },
   { _T("Agent.SupportedTables"), H_TableList, NULL },
   { _T("Agent.ThreadPools"), H_ThreadPoolList, NULL },
   { _T("Net.Resolver.AddressByName(*)"), H_ResolverAddrByNameList, NULL },
};

/**
 * Standard agent's tables
 */
static NETXMS_SUBAGENT_TABLE m_stdTables[] =
{
   { _T("Agent.SessionAgents"), H_SessionAgents, NULL, _T("SESSION_ID"), DCTDESC_AGENT_SESSION_AGENTS },
   { _T("Agent.SubAgents"), H_SubAgentTable, NULL, _T("NAME"), DCTDESC_AGENT_SUBAGENTS },
   { _T("Agent.ZoneConfigurations"), H_ZoneConfigurations, NULL, _T("SERVER_ID"), DCTDESC_AGENT_ZONE_CONFIGURATIONS },
   { _T("Agent.ZoneProxies"), H_ZoneProxies, NULL, _T("SERVER_ID,PROXY_ID"), DCTDESC_AGENT_ZONE_PROXIES }
};

/**
 * Initialize dynamic parameters list from default static list
 */
BOOL InitParameterList()
{
   if ((m_pParamList != NULL) || (m_pEnumList != NULL) || (m_pTableList != NULL))
      return FALSE;

   m_iNumParams = sizeof(m_stdParams) / sizeof(NETXMS_SUBAGENT_PARAM);
	if (m_iNumParams > 0)
	{
		m_pParamList = (NETXMS_SUBAGENT_PARAM *)malloc(sizeof(NETXMS_SUBAGENT_PARAM) * m_iNumParams);
		if (m_pParamList == NULL)
			return FALSE;
		memcpy(m_pParamList, m_stdParams, sizeof(NETXMS_SUBAGENT_PARAM) * m_iNumParams);
	}

   m_iNumEnums = sizeof(m_stdLists) / sizeof(NETXMS_SUBAGENT_LIST);
	if (m_iNumEnums > 0)
	{
		m_pEnumList = (NETXMS_SUBAGENT_LIST *)malloc(sizeof(NETXMS_SUBAGENT_LIST) * m_iNumEnums);
		if (m_pEnumList == NULL)
			return FALSE;
		memcpy(m_pEnumList, m_stdLists, sizeof(NETXMS_SUBAGENT_LIST) * m_iNumEnums);
	}

   m_iNumTables = sizeof(m_stdTables) / sizeof(NETXMS_SUBAGENT_TABLE);
	if (m_iNumTables > 0)
	{
		m_pTableList = (NETXMS_SUBAGENT_TABLE *)malloc(sizeof(NETXMS_SUBAGENT_TABLE) * m_iNumTables);
		if (m_pTableList == NULL)
			return FALSE;
		memcpy(m_pTableList, m_stdTables, sizeof(NETXMS_SUBAGENT_TABLE) * m_iNumTables);
	}

   return TRUE;
}

/**
 * Add push parameter to list
 * by LWX
 */
void AddPushParameter(const TCHAR *name, int dataType, const TCHAR *description)
{
   int i;

   // Search for existing parameter
   for(i = 0; i < m_iNumPushParams; i++)
      if (!_tcsicmp(m_pPushParamList[i].name, name))
         break;
   if (i < m_iNumPushParams)
   {
      // Replace existing attributes
      m_pPushParamList[i].dataType = dataType;
      nx_strncpy(m_pPushParamList[i].description, description, MAX_DB_STRING);
   }
   else
   {
      // Add new parameter
      m_pPushParamList = (NETXMS_SUBAGENT_PUSHPARAM *)realloc(m_pPushParamList, sizeof(NETXMS_SUBAGENT_PUSHPARAM) * (m_iNumPushParams + 1));
      _tcslcpy(m_pPushParamList[m_iNumPushParams].name, name, MAX_PARAM_NAME - 1);
      m_pPushParamList[m_iNumPushParams].dataType = dataType;
      _tcslcpy(m_pPushParamList[m_iNumPushParams].description, description, MAX_DB_STRING);
      m_iNumPushParams++;
   }
}

/**
 * Add parameter to list
 */
void AddParameter(const TCHAR *pszName, LONG (* fpHandler)(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *), const TCHAR *pArg,
                  int iDataType, const TCHAR *pszDescription)
{
   int i;

   // Search for existing parameter
   for(i = 0; i < m_iNumParams; i++)
      if (!_tcsicmp(m_pParamList[i].name, pszName))
         break;
   if (i < m_iNumParams)
   {
      // Replace existing handler and attributes
      m_pParamList[i].handler = fpHandler;
      m_pParamList[i].dataType = iDataType;
      nx_strncpy(m_pParamList[i].description, pszDescription, MAX_DB_STRING);

      // If we are replacing System.PlatformName, add pointer to
      // platform suffix as argument, otherwise, use supplied pArg
      if (!_tcscmp(pszName, _T("System.PlatformName")))
      {
         m_pParamList[i].arg = g_szPlatformSuffix; // to be TCHAR
      }
      else
      {
         m_pParamList[i].arg = pArg;
      }
   }
   else
   {
      // Add new parameter
      m_pParamList = (NETXMS_SUBAGENT_PARAM *)realloc(m_pParamList, sizeof(NETXMS_SUBAGENT_PARAM) * (m_iNumParams + 1));
      nx_strncpy(m_pParamList[m_iNumParams].name, pszName, MAX_PARAM_NAME - 1);
      m_pParamList[m_iNumParams].handler = fpHandler;
      m_pParamList[m_iNumParams].arg = pArg;
      m_pParamList[m_iNumParams].dataType = iDataType;
      nx_strncpy(m_pParamList[m_iNumParams].description, pszDescription, MAX_DB_STRING);
      m_iNumParams++;
   }
}

/**
 * Add list
 */
void AddList(const TCHAR *name, LONG (* handler)(const TCHAR *, const TCHAR *, StringList *, AbstractCommSession *), const TCHAR *arg)
{
   int i;

   // Search for existing enum
   for(i = 0; i < m_iNumEnums; i++)
      if (!_tcsicmp(m_pEnumList[i].name, name))
         break;
   if (i < m_iNumEnums)
   {
      // Replace existing handler and arg
      m_pEnumList[i].handler = handler;
      m_pEnumList[i].arg = arg;
   }
   else
   {
      // Add new enum
      m_pEnumList = (NETXMS_SUBAGENT_LIST *)realloc(m_pEnumList, sizeof(NETXMS_SUBAGENT_LIST) * (m_iNumEnums + 1));
      _tcslcpy(m_pEnumList[m_iNumEnums].name, name, MAX_PARAM_NAME - 1);
      m_pEnumList[m_iNumEnums].handler = handler;
      m_pEnumList[m_iNumEnums].arg = arg;
      m_iNumEnums++;
   }
}

/**
 * Add table
 */
void AddTable(const TCHAR *name, LONG (* handler)(const TCHAR *, const TCHAR *, Table *, AbstractCommSession *), const TCHAR *arg,
				  const TCHAR *instanceColumns, const TCHAR *description, int numColumns, NETXMS_SUBAGENT_TABLE_COLUMN *columns)
{
   int i;

   // Search for existing table
   for(i = 0; i < m_iNumTables; i++)
      if (!_tcsicmp(m_pTableList[i].name, name))
         break;
   if (i < m_iNumTables)
   {
      // Replace existing handler and arg
      m_pTableList[i].handler = handler;
      m_pTableList[i].arg = arg;
      _tcslcpy(m_pTableList[m_iNumTables].instanceColumns, instanceColumns, MAX_COLUMN_NAME * MAX_INSTANCE_COLUMNS);
		_tcslcpy(m_pTableList[m_iNumTables].description, description, MAX_DB_STRING);
      m_pTableList[i].numColumns = numColumns;
      m_pTableList[i].columns = columns;
   }
   else
   {
      // Add new table
      m_pTableList = (NETXMS_SUBAGENT_TABLE *)realloc(m_pTableList, sizeof(NETXMS_SUBAGENT_TABLE) * (m_iNumTables + 1));
      _tcslcpy(m_pTableList[m_iNumTables].name, name, MAX_PARAM_NAME);
      m_pTableList[m_iNumTables].handler = handler;
      m_pTableList[m_iNumTables].arg = arg;
      _tcslcpy(m_pTableList[m_iNumTables].instanceColumns, instanceColumns, MAX_COLUMN_NAME * MAX_INSTANCE_COLUMNS);
		_tcslcpy(m_pTableList[m_iNumTables].description, description, MAX_DB_STRING);
      m_pTableList[m_iNumTables].numColumns = numColumns;
      m_pTableList[m_iNumTables].columns = columns;
      m_iNumTables++;
      nxlog_debug(7, _T("Table %s added (%d predefined columns, instance columns \"%s\")"), name, numColumns, instanceColumns);
   }
}

/**
 * Add external parameter or list
 */
bool AddExternalParameter(TCHAR *config, bool shellExec, bool isList)
{
   TCHAR *cmdLine = _tcschr(config, _T(':'));
   if (cmdLine == NULL)
      return false;

   *cmdLine = 0;
   cmdLine++;
   Trim(config);
   Trim(cmdLine);
   if ((*config == 0) || (*cmdLine == 0))
      return false;

	TCHAR *arg = (TCHAR *)malloc((_tcslen(cmdLine) + 2) * sizeof(TCHAR));
	arg[0] = shellExec ? _T('S') : _T('E');
	_tcscpy(&arg[1], cmdLine);
   if (isList)
   {
      AddList(config, H_ExternalList, arg);
   }
   else
   {
      AddParameter(config, H_ExternalParameter, arg, DCI_DT_STRING, _T(""));
   }
   return true;
}

/**
 * Add external table
 */
bool AddExternalTable(TCHAR *config, bool shellExec)
{
   TCHAR *options = _tcschr(config, _T(':'));
   if (options == NULL)
      return false;
   *options = 0;
   options++;

   TCHAR *cmdLine = _tcschr(options, _T(':'));
   if (cmdLine == NULL)
      return false;
   *cmdLine = 0;
   cmdLine++;

   Trim(config);
   Trim(options);
   Trim(cmdLine);
   if ((*config == 0) || (*options == 0) || (*cmdLine == 0))
      return false;

   TCHAR instanceColumns[256] = _T("");
   ExtractNamedOptionValue(options, _T("instanceColumns"), instanceColumns, 256);

   TCHAR description[256] = _T("");
   ExtractNamedOptionValue(options, _T("description"), description, 256);

   TCHAR separator[16] = _T(",");
   ExtractNamedOptionValue(options, _T("separator"), separator, 16);
   if (separator[0] == _T('\\'))
   {
      switch(separator[1])
      {
         case 'n':
            separator[0] = _T('\n');
            break;
         case 'r':
            separator[0] = _T('\r');
            break;
         case 's':
            separator[0] = _T(' ');
            break;
         case 't':
            separator[0] = _T('\t');
            break;
         case 'u':
            separator[0] = (TCHAR)_tcstoul(&separator[2], NULL, 10);
            break;
      }
   }

   ExternalTableDefinition *td = new ExternalTableDefinition;
   td->separator = separator[0];
   td->cmdLine = (TCHAR *)malloc((_tcslen(cmdLine) + 2) * sizeof(TCHAR));
   td->cmdLine[0] = shellExec ? _T('S') : _T('E');
   _tcscpy(&td->cmdLine[1], cmdLine);
   td->instanceColumns = SplitString(instanceColumns, _T(','), &td->instanceColumnCount);
   AddTable(config, H_ExternalTable, (const TCHAR *)td, instanceColumns, description, 0, NULL);
   return true;
}

/**
 * Get parameter's value
 */
UINT32 GetParameterValue(const TCHAR *param, TCHAR *value, AbstractCommSession *session)
{
   int i, rc;
   UINT32 dwErrorCode;

   session->debugPrintf(5, _T("Requesting parameter \"%s\""), param);
   for(i = 0; i < m_iNumParams; i++)
	{
      if (MatchString(m_pParamList[i].name, param, FALSE))
      {
         rc = m_pParamList[i].handler(param, m_pParamList[i].arg, value, session);
         switch(rc)
         {
            case SYSINFO_RC_SUCCESS:
               dwErrorCode = ERR_SUCCESS;
               m_dwProcessedRequests++;
               break;
            case SYSINFO_RC_ERROR:
               dwErrorCode = ERR_INTERNAL_ERROR;
               m_dwFailedRequests++;
               break;
            case SYSINFO_RC_NO_SUCH_INSTANCE:
               dwErrorCode = ERR_NO_SUCH_INSTANCE;
               m_dwFailedRequests++;
               break;
            case SYSINFO_RC_UNSUPPORTED:
               dwErrorCode = ERR_UNKNOWN_PARAMETER;
               m_dwUnsupportedRequests++;
               break;
            default:
               nxlog_write(MSG_UNEXPECTED_IRC, EVENTLOG_ERROR_TYPE, "ds", rc, param);
               dwErrorCode = ERR_INTERNAL_ERROR;
               m_dwFailedRequests++;
               break;
         }
         break;
      }
	}

   if (i == m_iNumParams)
   {
		rc = GetParameterValueFromExtProvider(param, value);
		if (rc == SYSINFO_RC_SUCCESS)
		{
         dwErrorCode = ERR_SUCCESS;
         m_dwProcessedRequests++;
		}
		else
		{
			dwErrorCode = ERR_UNKNOWN_PARAMETER;
		}
   }

   if ((dwErrorCode == ERR_UNKNOWN_PARAMETER) && (i == m_iNumParams))
   {
		dwErrorCode = GetParameterValueFromAppAgent(param, value);
		if (dwErrorCode == ERR_SUCCESS)
		{
         m_dwProcessedRequests++;
		}
		else if (dwErrorCode != ERR_UNKNOWN_PARAMETER)
		{
         m_dwFailedRequests++;
		}
   }

   if ((dwErrorCode == ERR_UNKNOWN_PARAMETER) && (i == m_iNumParams))
   {
		dwErrorCode = GetParameterValueFromExtSubagent(param, value);
		if (dwErrorCode == ERR_SUCCESS)
		{
         m_dwProcessedRequests++;
		}
		else if (dwErrorCode == ERR_UNKNOWN_PARAMETER)
		{
			m_dwUnsupportedRequests++;
		}
		else
		{
         m_dwFailedRequests++;
		}
   }

	session->debugPrintf(7, _T("GetParameterValue(): result is %d (%s)"), (int)dwErrorCode,
		dwErrorCode == ERR_SUCCESS ? _T("SUCCESS") : (dwErrorCode == ERR_UNKNOWN_PARAMETER ? _T("UNKNOWN_PARAMETER") : _T("INTERNAL_ERROR")));
   return dwErrorCode;
}

/**
 * Get list's value
 */
UINT32 GetListValue(const TCHAR *param, StringList *value, AbstractCommSession *session)
{
   int i, rc;
   UINT32 dwErrorCode;

   session->debugPrintf(5, _T("Requesting list \"%s\""), param);
   for(i = 0; i < m_iNumEnums; i++)
	{
      if (MatchString(m_pEnumList[i].name, param, FALSE))
      {
         rc = m_pEnumList[i].handler(param, m_pEnumList[i].arg, value, session);
         switch(rc)
         {
            case SYSINFO_RC_SUCCESS:
               dwErrorCode = ERR_SUCCESS;
               m_dwProcessedRequests++;
               break;
            case SYSINFO_RC_ERROR:
               dwErrorCode = ERR_INTERNAL_ERROR;
               m_dwFailedRequests++;
               break;
            case SYSINFO_RC_NO_SUCH_INSTANCE:
               dwErrorCode = ERR_NO_SUCH_INSTANCE;
               m_dwFailedRequests++;
               break;
            case SYSINFO_RC_UNSUPPORTED:
               dwErrorCode = ERR_UNKNOWN_PARAMETER;
               m_dwUnsupportedRequests++;
               break;
            default:
               nxlog_write(MSG_UNEXPECTED_IRC, EVENTLOG_ERROR_TYPE, "ds", rc, param);
               dwErrorCode = ERR_INTERNAL_ERROR;
               m_dwFailedRequests++;
               break;
         }
         break;
      }
	}

	if (i == m_iNumEnums)
   {
		dwErrorCode = GetListValueFromExtSubagent(param, value);
		if (dwErrorCode == ERR_SUCCESS)
		{
         m_dwProcessedRequests++;
		}
		else if (dwErrorCode == ERR_UNKNOWN_PARAMETER)
		{
			m_dwUnsupportedRequests++;
		}
		else
		{
         m_dwFailedRequests++;
		}
   }

	session->debugPrintf(7, _T("GetListValue(): result is %d (%s)"), (int)dwErrorCode,
		dwErrorCode == ERR_SUCCESS ? _T("SUCCESS") : (dwErrorCode == ERR_UNKNOWN_PARAMETER ? _T("UNKNOWN_PARAMETER") : _T("INTERNAL_ERROR")));
   return dwErrorCode;
}

/**
 * Get table's value
 */
UINT32 GetTableValue(const TCHAR *param, Table *value, AbstractCommSession *session)
{
   int i, rc;
   UINT32 dwErrorCode;

   session->debugPrintf(5, _T("Requesting table \"%s\""), param);
   for(i = 0; i < m_iNumTables; i++)
	{
      if (MatchString(m_pTableList[i].name, param, FALSE))
      {
         // pre-fill table columns if specified in table definition
         if (m_pTableList[i].numColumns > 0)
         {
            for(int c = 0; c < m_pTableList[i].numColumns; c++)
            {
               NETXMS_SUBAGENT_TABLE_COLUMN *col = &m_pTableList[i].columns[c];
               value->addColumn(col->name, col->dataType, col->displayName, col->isInstance);
            }
         }

         rc = m_pTableList[i].handler(param, m_pTableList[i].arg, value, session);
         switch(rc)
         {
            case SYSINFO_RC_SUCCESS:
               dwErrorCode = ERR_SUCCESS;
               m_dwProcessedRequests++;
               break;
            case SYSINFO_RC_ERROR:
               dwErrorCode = ERR_INTERNAL_ERROR;
               m_dwFailedRequests++;
               break;
            case SYSINFO_RC_NO_SUCH_INSTANCE:
               dwErrorCode = ERR_NO_SUCH_INSTANCE;
               m_dwFailedRequests++;
               break;
            case SYSINFO_RC_UNSUPPORTED:
               dwErrorCode = ERR_UNKNOWN_PARAMETER;
               m_dwUnsupportedRequests++;
               break;
            default:
               nxlog_write(MSG_UNEXPECTED_IRC, EVENTLOG_ERROR_TYPE, "ds", rc, param);
               dwErrorCode = ERR_INTERNAL_ERROR;
               m_dwFailedRequests++;
               break;
         }
         break;
      }
	}

	if (i == m_iNumTables)
   {
		dwErrorCode = GetTableValueFromExtSubagent(param, value);
		if (dwErrorCode == ERR_SUCCESS)
		{
         m_dwProcessedRequests++;
		}
		else if (dwErrorCode == ERR_UNKNOWN_PARAMETER)
		{
			m_dwUnsupportedRequests++;
		}
		else
		{
         m_dwFailedRequests++;
		}
   }

	session->debugPrintf(7, _T("GetTableValue(): result is %d (%s)"), (int)dwErrorCode,
		dwErrorCode == ERR_SUCCESS ? _T("SUCCESS") : (dwErrorCode == ERR_UNKNOWN_PARAMETER ? _T("UNKNOWN_PARAMETER") : _T("INTERNAL_ERROR")));
   return dwErrorCode;
}

/**
 * Put complete list of supported parameters into NXCP message
 */
void GetParameterList(NXCPMessage *pMsg)
{
   int i;
   UINT32 dwId, count;

	// Parameters
   for(i = 0, count = 0, dwId = VID_PARAM_LIST_BASE; i < m_iNumParams; i++)
   {
		if (m_pParamList[i].dataType != DCI_DT_DEPRECATED)
		{
			pMsg->setField(dwId++, m_pParamList[i].name);
			pMsg->setField(dwId++, m_pParamList[i].description);
			pMsg->setField(dwId++, (WORD)m_pParamList[i].dataType);
			count++;
		}
   }
	ListParametersFromExtProviders(pMsg, &dwId, &count);
	ListParametersFromExtSubagents(pMsg, &dwId, &count);
   pMsg->setField(VID_NUM_PARAMETERS, count);

	// Push parameters
   pMsg->setField(VID_NUM_PUSH_PARAMETERS, (UINT32)m_iNumPushParams);
   for(i = 0, dwId = VID_PUSHPARAM_LIST_BASE; i < m_iNumPushParams; i++)
   {
      pMsg->setField(dwId++, m_pPushParamList[i].name);
      pMsg->setField(dwId++, m_pPushParamList[i].description);
      pMsg->setField(dwId++, (WORD)m_pPushParamList[i].dataType);
   }

	// Lists
   pMsg->setField(VID_NUM_ENUMS, (UINT32)m_iNumEnums);
   for(i = 0, dwId = VID_ENUM_LIST_BASE; i < m_iNumEnums; i++)
   {
      pMsg->setField(dwId++, m_pEnumList[i].name);
   }
	ListListsFromExtSubagents(pMsg, &dwId, &count);

	// Tables
   pMsg->setField(VID_NUM_TABLES, (UINT32)m_iNumTables);
   for(i = 0, dwId = VID_TABLE_LIST_BASE; i < m_iNumTables; i++)
   {
      pMsg->setField(dwId++, m_pTableList[i].name);
		pMsg->setField(dwId++, m_pTableList[i].instanceColumns);
		pMsg->setField(dwId++, m_pTableList[i].description);
   }
	ListTablesFromExtSubagents(pMsg, &dwId, &count);
}

/**
 * Put list of supported tables into NXCP message
 */
void GetTableList(NXCPMessage *pMsg)
{
   int i;
   UINT32 dwId;

   for(i = 0, dwId = VID_TABLE_LIST_BASE; i < m_iNumTables; i++)
   {
      pMsg->setField(dwId++, m_pTableList[i].name);
		pMsg->setField(dwId++, m_pTableList[i].instanceColumns);
		pMsg->setField(dwId++, m_pTableList[i].description);
   }

   UINT32 count = (UINT32)m_iNumTables;
	ListTablesFromExtSubagents(pMsg, &dwId, &count);
   pMsg->setField(VID_NUM_TABLES, count);
}

/**
 * Put list of supported lists (enums) into NXCP message
 */
void GetEnumList(NXCPMessage *pMsg)
{
   int i;
   UINT32 dwId;

   for(i = 0, dwId = VID_ENUM_LIST_BASE; i < m_iNumEnums; i++)
   {
      pMsg->setField(dwId++, m_pEnumList[i].name);
   }

   UINT32 count = (UINT32)m_iNumEnums;
	ListListsFromExtSubagents(pMsg, &dwId, &count);
   pMsg->setField(VID_NUM_ENUMS, count);
}
