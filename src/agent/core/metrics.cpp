/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2023 Raden Solutions
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
** File: metrics.cpp
**
**/

#include "nms_agent.h"
#include "nxagentd.h"
#include "unicode.h"
#include <netxms-version.h>
#include <unordered_map>

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
LONG H_AgentEventSender(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_AgentUptime(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_CertificateInfo(const TCHAR* param, const TCHAR* arg, TCHAR* value, AbstractCommSession* session);
LONG H_CRC32(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_DataCollectorQueueSize(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_DirInfo(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_ExternalList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_ExternalMetric(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_ExternalMetricExitCode(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_ExternalTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_FileContent(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_FileLineCount(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_FileTime(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_FileType(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_HostName(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_IsExtSubagentConnected(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_IsSubagentLoaded(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_MD5Hash(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_NotificationStats(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_PhysicalDiskInfo(const TCHAR *cmd, const TCHAR *arg, TCHAR *pValue, AbstractCommSession *session);
LONG H_PlatformName(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_ProblemsTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_PushValue(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_PushValues(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_ResolverAddrByName(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_ResolverAddrByNameList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_ResolverNameByAddr(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_SessionAgentCount(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_SessionAgents(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_SHA1Hash(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_SNMPAddressRangeScan(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_SNMPProxyStats(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_SubAgentList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_SubAgentTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_SyslogStats(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_SystemDate(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_SystemTime(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_SystemTimeZone(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_TCPAddressRangeScan(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_TFTPGet(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_ThreadPoolInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_ThreadPoolList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_ZoneConfigurations(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_ZoneProxies(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session);

#if WITH_MODBUS
LONG H_ModbusCoil(const TCHAR *metric, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_ModbusConnectionStatus(const TCHAR *metric, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_ModbusDeviceIdentification(const TCHAR *metric, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_ModbusDiscreteInput(const TCHAR *metric, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_ModbusHoldingRegister(const TCHAR *metric, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_ModbusInputRegister(const TCHAR *metric, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
#endif

/**
 * Authentication failure count
 */
VolatileCounter g_authenticationFailures = 0;

/**
 * Request processing counters
 */
static VolatileCounter s_processedRequests = 0;
static VolatileCounter s_failedRequests = 0;
static VolatileCounter s_unsupportedRequests = 0;

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
   ret_uint(value, *reinterpret_cast<const uint32_t*>(arg));
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
 * Handler for Agent.IsUserAgentInstalled parameter
 */
static LONG H_IsUserAgentInstalled(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   ret_int(value, IsUserAgentInstalled());
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Agent.IsRestartPending parameter
 */
static LONG H_IsAgentRestartPending(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   ret_int(value, g_restartPending ? 1 : 0);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Agent.SupportedCiphers
 */
static LONG H_SupportedCiphers(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   uint32_t ciphers = NXCPGetSupportedCiphers();
   if (ciphers == 0)
   {
      ret_string(value, _T("NONE"));
   }
   else
   {
      *value = 0;
      if (ciphers & NXCP_SUPPORT_AES_256)
         _tcscat(value, _T("AES-256 "));
      if (ciphers & NXCP_SUPPORT_AES_128)
         _tcscat(value, _T("AES-128 "));
      if (ciphers & NXCP_SUPPORT_BLOWFISH_256)
         _tcscat(value, _T("BF-256 "));
      if (ciphers & NXCP_SUPPORT_BLOWFISH_128)
         _tcscat(value, _T("BF-128 "));
      if (ciphers & NXCP_SUPPORT_IDEA)
         _tcscat(value, _T("IDEA "));
      if (ciphers & NXCP_SUPPORT_3DES)
         _tcscat(value, _T("3DES "));
      value[_tcslen(value) - 1] = 0;
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for component statuses based on failure flags
 */
static LONG H_ComponentStatus(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   uint32_t result = 0;
   switch(*pArg)
   {
      case 'C':
         if ((g_failFlags & FAIL_LOAD_CONFIG) != 0)
            result++;
         break;
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
   uint32_t flag = CAST_FROM_POINTER(arg, uint32_t);
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
   ret_int(value, (ecx & 0x80000000) != 0 ? VTYPE_FULL : VTYPE_NONE);
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
            ret_int(value, VTYPE_NONE);
         }
         else
         {
            // Guest partition
            ret_int(value, VTYPE_FULL);
         }
      }
      else
      {
         // Other virtualization product, assume guest OS
         ret_int(value, VTYPE_FULL);
      }
   }
   else
   {
      ret_int(value, VTYPE_NONE);
   }
   return SYSINFO_RC_SUCCESS;
}

#endif /* HAVE_GET_CPUID || _WIN32 */

/**
 * Handler for Agent.Heap.Active
 */
static LONG H_AgentHeapActive(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   int64_t bytes = GetActiveHeapMemory();
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
   int64_t bytes = GetAllocatedHeapMemory();
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
   int64_t bytes = GetMappedHeapMemory();
   if (bytes == -1)
      return SYSINFO_RC_UNSUPPORTED;
   ret_int64(value, bytes);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.HardwareId
 */
static LONG H_SystemHardwareId(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   BYTE hwid[HARDWARE_ID_LENGTH];
   if (!GetSystemHardwareId(hwid))
      return SYSINFO_RC_ERROR;
   BinToStr(hwid, HARDWARE_ID_LENGTH, value);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Hardware.System.SerialNumber
 */
static LONG H_HardwareSerialNumber(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char serial[256];
   if (!GetHardwareSerialNumber(serial, 256))
      return SYSINFO_RC_ERROR;
   ret_mbstring(value, serial);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Forward declarations for handlers
 */
static LONG H_MetricList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
static LONG H_PushMetricList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
static LONG H_ListOfLists(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
static LONG H_TableList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);

/**
 * Standard agent's parameters
 */
static NETXMS_SUBAGENT_PARAM s_standardParams[] =
{
#ifdef _WIN32
   { _T("System.IsVirtual"), H_SystemIsVirtual, nullptr, DCI_DT_INT, DCIDESC_SYSTEM_IS_VIRTUAL },
#endif

#if HAVE_GET_CPUID
   { _T("System.IsVirtual"), H_SystemIsVirtual, nullptr, DCI_DT_INT, DCIDESC_SYSTEM_IS_VIRTUAL },
#endif

   { _T("Agent.AcceptedConnections"), H_UIntPtr, (TCHAR *)&g_acceptedConnections, DCI_DT_COUNTER32, DCIDESC_AGENT_ACCEPTEDCONNECTIONS },
   { _T("Agent.AcceptErrors"), H_UIntPtr, (TCHAR *)&g_acceptErrors, DCI_DT_COUNTER32, DCIDESC_AGENT_ACCEPTERRORS },
   { _T("Agent.ActiveConnections"), H_ActiveConnections, nullptr, DCI_DT_UINT, DCIDESC_AGENT_ACTIVECONNECTIONS },
   { _T("Agent.AuthenticationFailures"), H_UIntPtr, (TCHAR *)&g_authenticationFailures, DCI_DT_COUNTER32, DCIDESC_AGENT_AUTHENTICATIONFAILURES },
   { _T("Agent.ConfigurationLoadStatus"), H_ComponentStatus, _T("C"), DCI_DT_UINT, DCIDESC_AGENT_CONFIG_LOAD_STATUS },
   { _T("Agent.ConfigurationServer"), H_StringConstant, g_szConfigServer, DCI_DT_STRING, DCIDESC_AGENT_CONFIG_SERVER },
   { _T("Agent.DataCollectorQueueSize"), H_DataCollectorQueueSize, nullptr, DCI_DT_UINT, DCIDESC_AGENT_DATACOLLQUEUESIZE },
   { _T("Agent.Events.Generated"), H_AgentEventSender, _T("G"), DCI_DT_COUNTER64, DCIDESC_AGENT_EVENTS_GENERATED },
   { _T("Agent.Events.LastTimestamp"), H_AgentEventSender, _T("T"), DCI_DT_UINT64, DCIDESC_AGENT_EVENTS_LAST_TIMESTAMP },
   { _T("Agent.FailedRequests"), H_UIntPtr, (TCHAR *)&s_failedRequests, DCI_DT_COUNTER32, DCIDESC_AGENT_FAILEDREQUESTS },
   { _T("Agent.Heap.Active"), H_AgentHeapActive, nullptr, DCI_DT_UINT64, DCIDESC_AGENT_HEAP_ACTIVE },
   { _T("Agent.Heap.Allocated"), H_AgentHeapAllocated, nullptr, DCI_DT_UINT64, DCIDESC_AGENT_HEAP_ALLOCATED },
   { _T("Agent.Heap.Mapped"), H_AgentHeapMapped, nullptr, DCI_DT_UINT64, DCIDESC_AGENT_HEAP_MAPPED },
   { _T("Agent.ID"), H_AgentID, nullptr, DCI_DT_STRING, DCIDESC_AGENT_ID },
   { _T("Agent.IsExternalSubagentConnected(*)"), H_IsExtSubagentConnected, nullptr, DCI_DT_INT, DCIDESC_AGENT_IS_EXT_SUBAGENT_CONNECTED },
   { _T("Agent.IsRestartPending"), H_IsAgentRestartPending, nullptr, DCI_DT_INT, DCIDESC_AGENT_IS_RESTART_PENDING },
   { _T("Agent.IsSubagentLoaded(*)"), H_IsSubagentLoaded, nullptr, DCI_DT_INT, DCIDESC_AGENT_IS_SUBAGENT_LOADED },
   { _T("Agent.IsUserAgentInstalled"), H_IsUserAgentInstalled, nullptr, DCI_DT_INT, DCIDESC_AGENT_IS_USERAGENT_INSTALLED },
   { _T("Agent.LocalDatabase.FailedQueries"), H_LocalDatabaseCounters, _T("F"), DCI_DT_COUNTER64, DCIDESC_AGENT_LOCALDB_FAILED_QUERIES },
   { _T("Agent.LocalDatabase.LongRunningQueries"), H_LocalDatabaseCounters, _T("L"), DCI_DT_COUNTER64, DCIDESC_AGENT_LOCALDB_SLOW_QUERIES },
   { _T("Agent.LocalDatabase.Status"), H_ComponentStatus, _T("D"), DCI_DT_UINT, DCIDESC_AGENT_LOCALDB_STATUS },
   { _T("Agent.LocalDatabase.TotalQueries"), H_LocalDatabaseCounters, _T("T"), DCI_DT_COUNTER64, DCIDESC_AGENT_LOCALDB_TOTAL_QUERIES },
   { _T("Agent.LogFile.Status"), H_ComponentStatus, _T("L"), DCI_DT_UINT, DCIDESC_AGENT_LOG_STATUS },
   { _T("Agent.NotificationProcessor.QueueSize"), H_NotificationStats, nullptr, DCI_DT_UINT, DCIDESC_AGENT_NOTIFICATIONPROC_QUEUESIZE },
   { _T("Agent.ProcessedRequests"), H_UIntPtr, (TCHAR *)&s_processedRequests, DCI_DT_COUNTER32, DCIDESC_AGENT_PROCESSEDREQUESTS },
   { _T("Agent.Proxy.ActiveSessions"), H_AgentProxyStats, _T("A"), DCI_DT_UINT, DCIDESC_AGENT_PROXY_ACTIVESESSIONS },
   { _T("Agent.Proxy.ConnectionRequests"), H_AgentProxyStats, _T("C"), DCI_DT_COUNTER64, DCIDESC_AGENT_PROXY_CONNECTIONREQUESTS },
   { _T("Agent.Proxy.IsEnabled"), H_FlagValue, CAST_TO_POINTER(AF_ENABLE_PROXY, TCHAR *), DCI_DT_UINT, DCIDESC_AGENT_PROXY_ISENABLED },
   { _T("Agent.PushValue(*)"), H_PushValue, nullptr, DCI_DT_STRING, DCIDESC_AGENT_PUSH_VALUE },
   { _T("Agent.Registrar"), H_StringConstant, g_registrar, DCI_DT_STRING, DCIDESC_AGENT_REGISTRAR },
   { _T("Agent.RejectedConnections"), H_UIntPtr, (TCHAR *)&g_rejectedConnections, DCI_DT_COUNTER32, DCIDESC_AGENT_REJECTEDCONNECTIONS },
   { _T("Agent.SessionAgentCount"), H_SessionAgentCount, _T("*"), DCI_DT_UINT, DCIDESC_AGENT_SESSION_AGENTS_COUNT },
   { _T("Agent.SNMP.IsProxyEnabled"), H_FlagValue, CAST_TO_POINTER(AF_ENABLE_SNMP_PROXY, TCHAR *), DCI_DT_UINT, DCIDESC_AGENT_SNMP_ISPROXYENABLED },
   { _T("Agent.SNMP.IsTrapProxyEnabled"), H_FlagValue, CAST_TO_POINTER(AF_ENABLE_SNMP_TRAP_PROXY, TCHAR *), DCI_DT_UINT, DCIDESC_AGENT_SNMP_ISPROXYENABLED },
   { _T("Agent.SNMP.Requests"), H_SNMPProxyStats, _T("R"), DCI_DT_COUNTER64, DCIDESC_AGENT_SNMP_REQUESTS },
   { _T("Agent.SNMP.Responses"), H_SNMPProxyStats, _T("r"), DCI_DT_COUNTER64, DCIDESC_AGENT_SNMP_RESPONSES },
   { _T("Agent.SNMP.ServerRequests"), H_SNMPProxyStats, _T("S"), DCI_DT_COUNTER64, DCIDESC_AGENT_SNMP_SERVERREQUESTS },
   { _T("Agent.SNMP.Traps"), H_SNMPProxyStats, _T("T"), DCI_DT_COUNTER64, DCIDESC_AGENT_SNMP_TRAPS },
   { _T("Agent.SourcePackageSupport"), H_StringConstant, _T("0"), DCI_DT_INT, DCIDESC_AGENT_SOURCEPACKAGESUPPORT },
   { _T("Agent.SupportedCiphers"), H_SupportedCiphers, nullptr, DCI_DT_STRING, DCIDESC_AGENT_SUPPORTEDCIPHERS },
   { _T("Agent.SyslogProxy.IsEnabled"), H_FlagValue, CAST_TO_POINTER(AF_ENABLE_SYSLOG_PROXY, TCHAR *), DCI_DT_UINT, DCIDESC_AGENT_SYSLOGPROXY_ISENABLED },
   { _T("Agent.SyslogProxy.ReceivedMessages"), H_SyslogStats, _T("R"), DCI_DT_COUNTER64, DCIDESC_AGENT_SYSLOGPROXY_RECEIVEDMSGS },
   { _T("Agent.TCPProxy.ConnectionRequests"), H_AgentProxyStats, _T("T"), DCI_DT_COUNTER64, DCIDESC_AGENT_TCPPROXY_CONNECTIONREQUESTS },
   { _T("Agent.TCPProxy.IsEnabled"), H_FlagValue, CAST_TO_POINTER(AF_ENABLE_TCP_PROXY, TCHAR *), DCI_DT_UINT, DCIDESC_AGENT_TCPPROXY_ISENABLED },
   { _T("Agent.ThreadPool.ActiveRequests(*)"), H_ThreadPoolInfo, (TCHAR *)THREAD_POOL_ACTIVE_REQUESTS, DCI_DT_UINT, DCIDESC_AGENT_THREADPOOL_ACTIVEREQUESTS },
   { _T("Agent.ThreadPool.AverageWaitTime(*)"), H_ThreadPoolInfo, (TCHAR *)THREAD_POOL_AVG_WAIT_TIME, DCI_DT_UINT, DCIDESC_AGENT_THREADPOOL_AVERAGEWAITTIME },
   { _T("Agent.ThreadPool.CurrSize(*)"), H_ThreadPoolInfo, (TCHAR *)THREAD_POOL_CURR_SIZE, DCI_DT_UINT, DCIDESC_AGENT_THREADPOOL_CURRSIZE },
   { _T("Agent.ThreadPool.Load(*)"), H_ThreadPoolInfo, (TCHAR *)THREAD_POOL_LOAD, DCI_DT_UINT, DCIDESC_AGENT_THREADPOOL_LOAD },
   { _T("Agent.ThreadPool.LoadAverage(*)"), H_ThreadPoolInfo, (TCHAR *)THREAD_POOL_LOADAVG_1, DCI_DT_UINT, DCIDESC_AGENT_THREADPOOL_LOADAVG },
   { _T("Agent.ThreadPool.LoadAverage5(*)"), H_ThreadPoolInfo, (TCHAR *)THREAD_POOL_LOADAVG_5, DCI_DT_UINT, DCIDESC_AGENT_THREADPOOL_LOADAVG_5 },
   { _T("Agent.ThreadPool.LoadAverage15(*)"), H_ThreadPoolInfo, (TCHAR *)THREAD_POOL_LOADAVG_15, DCI_DT_UINT, DCIDESC_AGENT_THREADPOOL_LOADAVG_15 },
   { _T("Agent.ThreadPool.MaxSize(*)"), H_ThreadPoolInfo, (TCHAR *)THREAD_POOL_MAX_SIZE, DCI_DT_UINT, DCIDESC_AGENT_THREADPOOL_MAXSIZE },
   { _T("Agent.ThreadPool.MinSize(*)"), H_ThreadPoolInfo, (TCHAR *)THREAD_POOL_MIN_SIZE, DCI_DT_UINT, DCIDESC_AGENT_THREADPOOL_MINSIZE },
   { _T("Agent.ThreadPool.ScheduledRequests(*)"), H_ThreadPoolInfo, (TCHAR *)THREAD_POOL_SCHEDULED_REQUESTS, DCI_DT_UINT, DCIDESC_AGENT_THREADPOOL_SCHEDULEDREQUESTS },
   { _T("Agent.ThreadPool.Usage(*)"), H_ThreadPoolInfo, (TCHAR *)THREAD_POOL_USAGE, DCI_DT_UINT, DCIDESC_AGENT_THREADPOOL_USAGE },
   { _T("Agent.UnsupportedRequests"), H_UIntPtr, (TCHAR *)&s_unsupportedRequests, DCI_DT_COUNTER32, DCIDESC_AGENT_UNSUPPORTEDREQUESTS },
   { _T("Agent.Uptime"), H_AgentUptime, nullptr, DCI_DT_UINT, DCIDESC_AGENT_UPTIME },
   { _T("Agent.UserAgentCount"), H_SessionAgentCount, _T("U"), DCI_DT_UINT, DCIDESC_AGENT_USER_AGENTS_COUNT },
   { _T("Agent.Version"), H_StringConstant, NETXMS_VERSION_STRING, DCI_DT_STRING, DCIDESC_AGENT_VERSION },
   { _T("Agent.WebServiceProxy.IsEnabled"), H_FlagValue, CAST_TO_POINTER(AF_ENABLE_WEBSVC_PROXY, TCHAR *), DCI_DT_UINT, DCIDESC_AGENT_WEBSVCPROXY_ISENABLED },
   { _T("File.Content(*)"), H_FileContent, nullptr, DCI_DT_STRING, _T("Content of file {instance}") },
   { _T("File.Count(*)"), H_DirInfo, (TCHAR *)DIRINFO_FILE_COUNT, DCI_DT_UINT, DCIDESC_FILE_COUNT },
   { _T("File.FolderCount(*)"), H_DirInfo, (TCHAR *)DIRINFO_FOLDER_COUNT, DCI_DT_UINT, DCIDESC_FILE_FOLDERCOUNT },
   { _T("File.Hash.CRC32(*)"), H_CRC32, nullptr, DCI_DT_UINT, DCIDESC_FILE_HASH_CRC32 },
   { _T("File.Hash.MD5(*)"), H_MD5Hash, nullptr, DCI_DT_STRING, DCIDESC_FILE_HASH_MD5 },
   { _T("File.Hash.SHA1(*)"), H_SHA1Hash, nullptr, DCI_DT_STRING, DCIDESC_FILE_HASH_SHA1 },
   { _T("File.Size(*)"), H_DirInfo, (TCHAR *)DIRINFO_FILE_SIZE, DCI_DT_UINT64, DCIDESC_FILE_SIZE },
   { _T("File.LineCount(*)"), H_FileLineCount, (TCHAR *)DIRINFO_FILE_LINE_COUNT, DCI_DT_UINT64, _T("File {instance} line count") },
   { _T("File.Time.Access(*)"), H_FileTime, (TCHAR *)FILETIME_ATIME, DCI_DT_UINT64, DCIDESC_FILE_TIME_ACCESS },
   { _T("File.Time.Change(*)"), H_FileTime, (TCHAR *)FILETIME_CTIME, DCI_DT_UINT64, DCIDESC_FILE_TIME_CHANGE },
   { _T("File.Time.Modify(*)"), H_FileTime, (TCHAR *)FILETIME_MTIME, DCI_DT_UINT64, DCIDESC_FILE_TIME_MODIFY },
   { _T("File.Type(*)"), H_FileType, nullptr, DCI_DT_UINT, _T("Type of file {instance}") },
   { _T("Hardware.System.SerialNumber"), H_HardwareSerialNumber, nullptr, DCI_DT_STRING, DCIDESC_HARDWARE_SYSTEM_SERIALNUMBER },
#if WITH_MODBUS
   { _T("Modbus.Coil(*)"), H_ModbusCoil, nullptr, DCI_DT_UINT, _T("Value of MODBUS coil {instance}") },
   { _T("Modbus.ConnectionStatus(*)"), H_ModbusConnectionStatus, nullptr, DCI_DT_INT, _T("Status of MODBUS connection to {instance}") },
   { _T("Modbus.DiscreteInput(*)"), H_ModbusDiscreteInput, nullptr, DCI_DT_UINT, _T("Value of MODBUS discrete input {instance}") },
   { _T("Modbus.HoldingRegister(*)"), H_ModbusHoldingRegister, nullptr, DCI_DT_UINT, _T("Value of MODBUS holding register {instance}") },
   { _T("Modbus.InputRegister(*)"), H_ModbusInputRegister, nullptr, DCI_DT_UINT, _T("Value of MODBUS input register {instance}") },
#endif
   { _T("Net.Resolver.AddressByName(*)"), H_ResolverAddrByName, nullptr, DCI_DT_STRING, DCIDESC_NET_RESOLVER_ADDRBYNAME },
   { _T("Net.Resolver.NameByAddress(*)"), H_ResolverNameByAddr, nullptr, DCI_DT_STRING, DCIDESC_NET_RESOLVER_NAMEBYADDR },
   { _T("PhysicalDisk.Capacity(*)"), H_PhysicalDiskInfo, _T("C"), DCI_DT_UINT64, DCIDESC_PHYSICALDISK_CAPACITY },
   { _T("PhysicalDisk.DeviceType(*)"), H_PhysicalDiskInfo, _T("D"), DCI_DT_STRING, DCIDESC_PHYSICALDISK_DEVICETYPE },
   { _T("PhysicalDisk.Firmware(*)"), H_PhysicalDiskInfo, _T("F"), DCI_DT_STRING, DCIDESC_PHYSICALDISK_FIRMWARE },
   { _T("PhysicalDisk.Model(*)"), H_PhysicalDiskInfo, _T("M"), DCI_DT_STRING, DCIDESC_PHYSICALDISK_MODEL },
   { _T("PhysicalDisk.PowerCycles(*)"), H_PhysicalDiskInfo, _T("P"), DCI_DT_UINT, DCIDESC_PHYSICALDISK_POWER_CYCLES },
   { _T("PhysicalDisk.PowerOnTime(*)"), H_PhysicalDiskInfo, _T("O"), DCI_DT_UINT, DCIDESC_PHYSICALDISK_POWERON_TIME },
   { _T("PhysicalDisk.SerialNumber(*)"), H_PhysicalDiskInfo, _T("N"), DCI_DT_STRING, DCIDESC_PHYSICALDISK_SERIALNUMBER },
   { _T("PhysicalDisk.SmartAttr(*)"), H_PhysicalDiskInfo, _T("A"), DCI_DT_STRING, DCIDESC_PHYSICALDISK_SMARTATTR },
   { _T("PhysicalDisk.SmartStatus(*)"), H_PhysicalDiskInfo, _T("S"), DCI_DT_INT, DCIDESC_PHYSICALDISK_SMARTSTATUS },
   { _T("PhysicalDisk.Temperature(*)"), H_PhysicalDiskInfo, _T("T"), DCI_DT_INT, DCIDESC_PHYSICALDISK_TEMPERATURE },
   { _T("System.CurrentDate"), H_SystemDate, nullptr, DCI_DT_STRING, DCIDESC_SYSTEM_CURRENTDATE },
   { _T("System.CurrentTime"), H_SystemTime, nullptr, DCI_DT_INT64, DCIDESC_SYSTEM_CURRENTTIME },
   { _T("System.FQDN"), H_HostName, _T("FQDN"), DCI_DT_STRING, DCIDESC_SYSTEM_FQDN },
   { _T("System.HardwareId"), H_SystemHardwareId, nullptr, DCI_DT_STRING, DCIDESC_SYSTEM_HARDWAREID },
   { _T("System.Hostname"), H_HostName, nullptr, DCI_DT_STRING, DCIDESC_SYSTEM_HOSTNAME },
   { _T("System.PlatformName"), H_PlatformName, nullptr, DCI_DT_STRING, DCIDESC_SYSTEM_PLATFORMNAME },
   { _T("System.TimeZone"), H_SystemTimeZone, nullptr, DCI_DT_STRING, DCIDESC_SYSTEM_TIMEZONE },
   { _T("X509.Certificate.ExpirationDate(*)"), H_CertificateInfo, _T("D"), DCI_DT_STRING, _T("Expiration date (YYYY-MM-DD) of X.509 certificate from file {instance}") },
   { _T("X509.Certificate.ExpirationTime(*)"), H_CertificateInfo, _T("E"), DCI_DT_UINT64, _T("Expiration time of X.509 certificate from file {instance}") },
   { _T("X509.Certificate.ExpiresIn(*)"), H_CertificateInfo, _T("U"), DCI_DT_INT, _T("Days until expiration of X.509 certificate from file {instance}") },
   { _T("X509.Certificate.Issuer(*)"), H_CertificateInfo, _T("I"), DCI_DT_STRING, _T("Issuer of X.509 certificate from file {instance}") },
   { _T("X509.Certificate.Subject(*)"), H_CertificateInfo, _T("S"), DCI_DT_STRING, _T("Subject of X.509 certificate from file {instance}") },
   { _T("X509.Certificate.TemplateID(*)"), H_CertificateInfo, _T("T"), DCI_DT_STRING, _T("Template ID of X.509 certificate from file {instance}") },

   // Deprecated parameters
   { _T("Agent.GeneratedTraps"), H_AgentEventSender, _T("G"), DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Agent.LastTrapTime"), H_AgentEventSender, _T("T"), DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Agent.SentTraps"), H_AgentEventSender, _T("S"), DCI_DT_DEPRECATED, DCIDESC_DEPRECATED }
};

/**
 * Standard agent's lists
 */
static NETXMS_SUBAGENT_LIST s_standardLists[] =
{
   { _T("Agent.ActionList"), H_ActionList, nullptr },
   { _T("Agent.PushValues"), H_PushValues, nullptr },
   { _T("Agent.SubAgentList"), H_SubAgentList, nullptr },
   { _T("Agent.SupportedLists"), H_ListOfLists, nullptr },
   { _T("Agent.SupportedParameters"), H_MetricList, nullptr },
   { _T("Agent.SupportedPushParameters"), H_PushMetricList, nullptr },
   { _T("Agent.SupportedTables"), H_TableList, nullptr },
   { _T("Agent.ThreadPools"), H_ThreadPoolList, nullptr },
#if WITH_MODBUS
   { _T("Modbus.DeviceIdentification(*)"), H_ModbusDeviceIdentification, nullptr },
#endif
   { _T("Net.Resolver.AddressByName(*)"), H_ResolverAddrByNameList, nullptr },
   { _T("SNMP.ScanAddressRange(*)"), H_SNMPAddressRangeScan, nullptr },
   { _T("TCP.ScanAddressRange(*)"), H_TCPAddressRangeScan, nullptr },
   { _T("TFTP.Get(*)"), H_TFTPGet, nullptr }
};

/**
 * Standard agent's tables
 */
static NETXMS_SUBAGENT_TABLE s_standardTables[] =
{
   { _T("Agent.Problems"), H_ProblemsTable, nullptr, _T("KEY"), _T("Registered agent problems") },
   { _T("Agent.SessionAgents"), H_SessionAgents, nullptr, _T("SESSION_ID"), DCTDESC_AGENT_SESSION_AGENTS },
   { _T("Agent.SubAgents"), H_SubAgentTable, nullptr, _T("NAME"), DCTDESC_AGENT_SUBAGENTS },
   { _T("Agent.ZoneConfigurations"), H_ZoneConfigurations, nullptr, _T("SERVER_ID"), DCTDESC_AGENT_ZONE_CONFIGURATIONS },
   { _T("Agent.ZoneProxies"), H_ZoneProxies, nullptr, _T("SERVER_ID,PROXY_ID"), DCTDESC_AGENT_ZONE_PROXIES }
};

class BackgroundExternalMetric
{
protected:
   TCHAR m_name[MAX_PARAM_NAME];
   TCHAR m_description[MAX_DB_STRING];
   TCHAR m_cmdTemplate[MAX_CMD_LEN];
   int m_dataType;
   Mutex m_mutex;
   uint32_t m_pollingInterval;
   
   class CachedExecutor {
   public:
      CachedExecutor(const StringBuffer& cmdLine): 
         m_executor(cmdLine)
      {
         _tcscpy(m_value, _T(""));
      }
      
      LineOutputProcessExecutor m_executor;
      TCHAR m_value[MAX_RESULT_LENGTH];   
   };
   
   std::unordered_map<std::string, std::unique_ptr<CachedExecutor>> m_executors;
   
public:
   BackgroundExternalMetric(TCHAR *name, uint32_t interval, TCHAR *description, TCHAR* cmdTemplate, int dataType) {
      _tcscpy(m_name, name);
      _tcscpy(m_description, description);
      m_dataType = dataType;
      _tcscpy(m_cmdTemplate, cmdTemplate);
      m_pollingInterval = interval;
   };

   void poll();
   
   const TCHAR* getName() const { return m_name;}
   const int& getDataType() const { return m_dataType;}
   const TCHAR* getDescription() const { return m_description;}
   void getValue(TCHAR *value, const TCHAR* param, AbstractCommSession *session);
};

void BackgroundExternalMetric::poll()
{
   if (!(g_dwFlags & AF_SHUTDOWN))
   {
      for (auto it = m_executors.begin(); it != m_executors.end(); ++it) 
      {
         CachedExecutor& CachedExecutor = *(it->second);
         
         if(!CachedExecutor.m_executor.isRunning())
         {   
            const StringList& values = CachedExecutor.m_executor.getData();
            ret_string(CachedExecutor.m_value, values.size() > 0 ? values.get(0) : _T(""));
            
            CachedExecutor.m_executor.execute();
         }
      }

      ThreadPoolScheduleRelative(g_backgroundThreadPool, m_pollingInterval * 1000, this, &BackgroundExternalMetric::poll);  
   }
}

void BackgroundExternalMetric::getValue(TCHAR *value, const TCHAR* param, AbstractCommSession *session)
{
   std::string key(MBStringFromWideString(param));
   
   m_mutex.lock();
   // Check if an executor for the given cmdline exists in the map
   if (!m_executors[key])
   {
      // Substitute $1 .. $9 with actual arguments
      StringBuffer cmdLine;
      cmdLine.setAllocationStep(1024);

      for (const TCHAR *sptr = m_cmdTemplate; *sptr != 0; sptr++)
      {
         if (*sptr == _T('$'))
         {
            sptr++;
            if (*sptr == 0)
               break;   // Single $ character at the end of line
            if ((*sptr >= _T('1')) && (*sptr <= _T('9')))
            {
               TCHAR buffer[1024];
               if (AgentGetParameterArg(param, *sptr - '0', buffer, 1024))
               {
                  cmdLine.append(buffer);
               }
            }
            else
            {
               cmdLine.append(*sptr);
            }
         }
         else
         {
            cmdLine.append(*sptr);
         }
      }

      // Create a new executor for the cmdline on the heap and store it in the map
      m_executors[key] = std::make_unique<CachedExecutor>(cmdLine);
   }
   m_mutex.unlock();

   std::unique_ptr<CachedExecutor>& CachedExecutor = m_executors[key];
      
   _tcscpy(value, CachedExecutor->m_value);
}

/**
 * Provided metrics
 */
static StructArray<NETXMS_SUBAGENT_PARAM> s_metrics(s_standardParams, sizeof(s_standardParams) / sizeof(NETXMS_SUBAGENT_PARAM), 64);
static StructArray<NETXMS_SUBAGENT_PUSHPARAM> s_pushMetrics(0, 64);
static StructArray<NETXMS_SUBAGENT_LIST> s_lists(s_standardLists, sizeof(s_standardLists) / sizeof(NETXMS_SUBAGENT_LIST), 16);
static StructArray<NETXMS_SUBAGENT_TABLE> s_tables(s_standardTables, sizeof(s_standardTables) / sizeof(NETXMS_SUBAGENT_TABLE), 16);
static ObjectArray<BackgroundExternalMetric> s_metricsBackground(0, 8, Ownership::True);

/**
 * Handler for metrics list
 */
static LONG H_MetricList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   for(int i = 0; i < s_metrics.size(); i++)
   {
      const NETXMS_SUBAGENT_PARAM *p = s_metrics.get(i);
      if (p->dataType != DCI_DT_DEPRECATED)
         value->add(p->name);
   }
   for(int i = 0; i < s_metricsBackground.size(); i++)
   {
      const BackgroundExternalMetric *p = s_metricsBackground.get(i);
      if (p->getDataType() != DCI_DT_DEPRECATED)
         value->add(p->getName());
   }
   ListParametersFromExtProviders(value);
   ListParametersFromExtSubagents(value);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for push metrics list
 */
static LONG H_PushMetricList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   for(int i = 0; i < s_pushMetrics.size(); i++)
      value->add(s_pushMetrics.get(i)->name);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for list of lists
 */
static LONG H_ListOfLists(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   for(int i = 0; i < s_lists.size(); i++)
      value->add(s_lists.get(i)->name);
   ListListsFromExtSubagents(value);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for table list
 */
static LONG H_TableList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   for(int i = 0; i < s_tables.size(); i++)
      value->add(s_tables.get(i)->name);
   ListTablesFromExtProviders(value);
   ListTablesFromExtSubagents(value);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Add push metric to list
 */
void AddPushMetric(const TCHAR *name, int dataType, const TCHAR *description)
{
   // Search for existing parameter
   NETXMS_SUBAGENT_PUSHPARAM *p = nullptr;
   for(int i = 0; i < s_pushMetrics.size(); i++)
      if (!_tcsicmp(s_pushMetrics.get(i)->name, name))
      {
         p = s_pushMetrics.get(i);
         break;
      }
   if (p != nullptr)
   {
      // Replace existing attributes
      p->dataType = dataType;
      _tcslcpy(p->description, description, MAX_DB_STRING);
   }
   else
   {
      // Add new parameter
      NETXMS_SUBAGENT_PUSHPARAM np;
      _tcslcpy(np.name, name, MAX_PARAM_NAME);
      np.dataType = dataType;
      _tcslcpy(np.description, description, MAX_DB_STRING);
      s_pushMetrics.add(np);
   }
}

/**
 * Add metric to list
 */
void AddMetric(const TCHAR *name, LONG (*handler)(const TCHAR*, const TCHAR*, TCHAR*, AbstractCommSession*), const TCHAR *arg, int dataType, const TCHAR *description)
{
   // Search for existing metric
   NETXMS_SUBAGENT_PARAM *p = nullptr;
   for(int i = 0; i < s_metrics.size(); i++)
      if (!_tcsicmp(s_metrics.get(i)->name, name))
      {
         p = s_metrics.get(i);
         break;
      }
   if (p != nullptr)
   {
      // Replace existing handler and attributes
      p->handler = handler;
      p->dataType = dataType;
      _tcslcpy(p->description, description, MAX_DB_STRING);

      // If we are replacing System.PlatformName, add pointer to
      // platform suffix as argument, otherwise, use supplied pArg
      if (!_tcscmp(name, _T("System.PlatformName")))
      {
         p->arg = g_szPlatformSuffix; // to be TCHAR
      }
      else
      {
         p->arg = arg;
      }
   }
   else
   {
      // Add new metric
      NETXMS_SUBAGENT_PARAM np;
      _tcslcpy(np.name, name, MAX_PARAM_NAME);
      np.handler = handler;
      np.arg = arg;
      np.dataType = dataType;
      _tcslcpy(np.description, description, MAX_DB_STRING);
      s_metrics.add(np);
   }
}

/**
 * Add list
 */
void AddList(const TCHAR *name, LONG (*handler)(const TCHAR*, const TCHAR*, StringList*, AbstractCommSession*), const TCHAR *arg)
{
   // Search for existing enum
   NETXMS_SUBAGENT_LIST *p = nullptr;
   for(int i = 0; i < s_lists.size(); i++)
      if (!_tcsicmp(s_lists.get(i)->name, name))
      {
         p = s_lists.get(i);
         break;
      }
   if (p != nullptr)
   {
      // Replace existing handler and arg
      p->handler = handler;
      p->arg = arg;
   }
   else
   {
      // Add new enum
      NETXMS_SUBAGENT_LIST np;
      _tcslcpy(np.name, name, MAX_PARAM_NAME - 1);
      np.handler = handler;
      np.arg = arg;
      s_lists.add(np);
   }
}

/**
 * Add table
 */
void AddTable(const TCHAR *name, LONG (* handler)(const TCHAR *, const TCHAR *, Table *, AbstractCommSession *), const TCHAR *arg,
         const TCHAR *instanceColumns, const TCHAR *description, int numColumns, NETXMS_SUBAGENT_TABLE_COLUMN *columns)
{
   // Search for existing table
   NETXMS_SUBAGENT_TABLE *p = nullptr;
   for(int i = 0; i < s_tables.size(); i++)
      if (!_tcsicmp(s_tables.get(i)->name, name))
      {
         p = s_tables.get(i);
         break;
      }
   if (p != nullptr)
   {
      // Replace existing handler and arg
      p->handler = handler;
      p->arg = arg;
      _tcslcpy(p->instanceColumns, instanceColumns, MAX_COLUMN_NAME * MAX_INSTANCE_COLUMNS);
		_tcslcpy(p->description, description, MAX_DB_STRING);
      p->numColumns = numColumns;
      p->columns = columns;
   }
   else
   {
      // Add new table
      NETXMS_SUBAGENT_TABLE np;
      _tcslcpy(np.name, name, MAX_PARAM_NAME);
      np.handler = handler;
      np.arg = arg;
      _tcslcpy(np.instanceColumns, instanceColumns, MAX_COLUMN_NAME * MAX_INSTANCE_COLUMNS);
		_tcslcpy(np.description, description, MAX_DB_STRING);
      np.numColumns = numColumns;
      np.columns = columns;
      s_tables.add(np);
      nxlog_debug(7, _T("Table %s added (%d predefined columns, instance columns \"%s\")"), name, numColumns, instanceColumns);
   }
}

/**
 * Add external metric or list
 */
bool AddBackgroundExternalMetric(TCHAR *config)
{
   TCHAR *intervalStr = _tcschr(config, _T(':'));
   if (intervalStr == nullptr)
      return false;

   *intervalStr = 0;
   intervalStr++;

   TCHAR *cmdTemplate = _tcschr(intervalStr, _T(':'));
   if (cmdTemplate == nullptr)
      return false;

   *cmdTemplate = 0;
   cmdTemplate++;
   
   Trim(config);
   Trim(intervalStr);
   Trim(cmdTemplate);
   if ((*config == 0) || (*intervalStr == 0) || (*cmdTemplate == 0))
      return false;

   TCHAR description[] = _T("");
   uint32_t interval = _tcstoul(intervalStr, nullptr, 0);
   BackgroundExternalMetric *pMetric = new BackgroundExternalMetric(config, interval, description, cmdTemplate, DCI_DT_STRING);
   s_metricsBackground.add(pMetric);

   return true;
}

/**
 * Add external metric or list
 */
bool AddExternalMetric(TCHAR *config, bool isList)
{
   TCHAR *cmdLine = _tcschr(config, _T(':'));
   if (cmdLine == nullptr)
      return false;

   *cmdLine = 0;
   cmdLine++;
   Trim(config);
   Trim(cmdLine);
   if ((*config == 0) || (*cmdLine == 0))
      return false;

	TCHAR *arg = MemCopyString(cmdLine);
   if (isList)
   {
      AddList(config, H_ExternalList, arg);
   }
   else
   {
      AddMetric(config, H_ExternalMetric, arg, DCI_DT_STRING, _T(""));
      TCHAR nameExitCode[MAX_PARAM_NAME];
      _tcslcpy(nameExitCode, config, MAX_PARAM_NAME);
      _tcslcat(nameExitCode, _T(".ExitCode"), MAX_PARAM_NAME);
      AddMetric(nameExitCode, H_ExternalMetricExitCode, arg, DCI_DT_INT, _T(""));
   }
   return true;
}

/**
 * Add external table
 */
bool AddExternalTable(TCHAR *config)
{
   TCHAR *options = _tcschr(config, _T(':'));
   if (options == nullptr)
      return false;
   *options = 0;
   options++;

   TCHAR *cmdLine = _tcschr(options, _T(':'));
   if (cmdLine == nullptr)
      return false;
   *cmdLine = 0;
   cmdLine++;

   Trim(config);
   Trim(options);
   Trim(cmdLine);
   if ((*config == 0) || (*cmdLine == 0))
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
            separator[0] = (TCHAR)_tcstoul(&separator[2], nullptr, 10);
            break;
      }
   }

   ExternalTableDefinition *td = new ExternalTableDefinition();
   td->separator = separator[0];
   td->cmdLine = MemCopyString(cmdLine);
   td->instanceColumns = SplitString(instanceColumns, _T(','), &td->instanceColumnCount);
   if (ExtractNamedOptionValueAsBool(options, _T("backgroundPolling"), false))
   {
      uint32_t pollingInterval = ExtractNamedOptionValueAsUInt(options, _T("pollingInterval"), 60);
      AddTableProvider(config, td, pollingInterval, ExtractNamedOptionValueAsUInt(options, _T("timeout"), 0), description);
   }
   else
   {
      AddTable(config, H_ExternalTable, reinterpret_cast<const TCHAR*>(td), instanceColumns, description, 0, nullptr);
   }
   return true;
}

/**
 * Add external table
 */
bool AddExternalTable(ConfigEntry *config)
{
   ExternalTableDefinition *td = new ExternalTableDefinition();
   TCHAR *separator = MemCopyString(config->getSubEntryValue(_T("Separator"), 0, _T(",")));
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
            separator[0] = (TCHAR)_tcstoul(&separator[2], nullptr, 10);
            break;
      }
   }
   td->separator = separator[0];
   MemFree(separator);

   td->cmdLine = MemCopyString(config->getSubEntryValue(_T("Command"), 0, _T("echo no command specified")));
   td->instanceColumns = SplitString(config->getSubEntryValue(_T("InstanceColumns"), 0, _T("")), _T(','), &td->instanceColumnCount);
   unique_ptr<ObjectArray<ConfigEntry>> columnTypes = config->getSubEntries(_T("ColumnType"));
   if ((columnTypes != nullptr) && !columnTypes->isEmpty())
   {
      ConfigEntry *configEntry = columnTypes->get(0);
      for (int i = 0; i < configEntry->getValueCount(); i++)
      {
         TCHAR *line = MemCopyString(configEntry->getValue(i));
         TCHAR *textDataType = _tcschr(line, _T(':'));
         if (textDataType != nullptr)
         {
            *textDataType = 0;
            textDataType++;
            Trim(line);
            Trim(textDataType);
            if ((*line != 0) && (*textDataType != 0))
            {
               int dataType = TextToDataType(textDataType);
               td->columnDataTypes.set(line, (dataType == -1) ? DCI_DT_INT : dataType);
            }
         }
         MemFree(line);
      }
   }
   if (config->getSubEntryValueAsInt(_T("PollingInterval"), 0, -1) >= 0)
   {
      AddTableProvider(config->getName(), td, config->getSubEntryValueAsUInt(_T("PollingInterval"), 0, 60), config->getSubEntryValueAsUInt(_T("Timeout"), 0, 0), config->getSubEntryValue(_T("Description")));
   }
   else
   {
      AddTable(config->getName(), H_ExternalTable, reinterpret_cast<const TCHAR *>(td), *td->instanceColumns, config->getSubEntryValue(_T("Description"), 0, _T("")), 0, nullptr);
   }
   return true;
}

/**
 * Get symbolic name for error code
 */
static const TCHAR *GetErrorCodeSymbolicName(uint32_t e)
{
   switch(e)
   {
      case ERR_SUCCESS:
         return _T("SUCCESS");
      case ERR_UNKNOWN_METRIC:
         return _T("UNKNOWN_METRIC");
      case ERR_UNSUPPORTED_METRIC:
         return _T("UNSUPPORTED_METRIC");
      case ERR_NO_SUCH_INSTANCE:
         return _T("NO_SUCH_INSTANCE");
   }
   return _T("INTERNAL_ERROR");
}

/**
 * Get metric value
 */
uint32_t GetMetricValue(const TCHAR *param, TCHAR *value, AbstractCommSession *session)
{
   uint32_t errorCode = ERR_UNKNOWN_METRIC;

   session->debugPrintf(5, _T("Requesting metric \"%s\""), param);
   for(int i = 0; i < s_metrics.size(); i++)
	{
      NETXMS_SUBAGENT_PARAM *p = s_metrics.get(i);
      if (MatchString(p->name, param, FALSE))
      {
         LONG rc = p->handler(param, p->arg, value, session);
         switch(rc)
         {
            case SYSINFO_RC_SUCCESS:
               errorCode = ERR_SUCCESS;
               InterlockedIncrement(&s_processedRequests);
               break;
            case SYSINFO_RC_ACCESS_DENIED:
               errorCode = ERR_ACCESS_DENIED;
               InterlockedIncrement(&s_failedRequests);
               break;
            case SYSINFO_RC_ERROR:
               errorCode = ERR_INTERNAL_ERROR;
               InterlockedIncrement(&s_failedRequests);
               break;
            case SYSINFO_RC_NO_SUCH_INSTANCE:
               errorCode = ERR_NO_SUCH_INSTANCE;
               InterlockedIncrement(&s_failedRequests);
               break;
            case SYSINFO_RC_UNSUPPORTED:
               errorCode = ERR_UNSUPPORTED_METRIC;
               InterlockedIncrement(&s_unsupportedRequests);
               break;
            case SYSINFO_RC_UNKNOWN:
               errorCode = ERR_UNKNOWN_METRIC;
               break;
            default:
               nxlog_write(NXLOG_ERROR, _T("Internal error: unexpected return code %d in GetMetricValue(\"%s\")"), rc, param);
               errorCode = ERR_INTERNAL_ERROR;
               InterlockedIncrement(&s_failedRequests);
               break;
         }
         break;
      }
	}

   if (errorCode == ERR_UNKNOWN_METRIC)
   {   
      for(int i = 0; i < s_metricsBackground.size(); i++)
      {
         BackgroundExternalMetric *p = s_metricsBackground.get(i);
         if (MatchString(p->getName(), param, FALSE))
         {
            p->getValue(value, param, session);
            session->debugPrintf(3, _T("Requesting \"%s\""), param);
            errorCode = ERR_SUCCESS;
         }
      }
   }
   
   if (errorCode == ERR_UNKNOWN_METRIC)
   {
		LONG rc = GetParameterValueFromExtProvider(param, value);
      switch(rc)
      {
         case SYSINFO_RC_SUCCESS:
            errorCode = ERR_SUCCESS;
            InterlockedIncrement(&s_processedRequests);
            break;
         case SYSINFO_RC_ACCESS_DENIED:
            errorCode = ERR_ACCESS_DENIED;
            InterlockedIncrement(&s_failedRequests);
            break;
         case SYSINFO_RC_ERROR:
            errorCode = ERR_INTERNAL_ERROR;
            InterlockedIncrement(&s_failedRequests);
            break;
         case SYSINFO_RC_NO_SUCH_INSTANCE:
            errorCode = ERR_NO_SUCH_INSTANCE;
            InterlockedIncrement(&s_failedRequests);
            break;
         case SYSINFO_RC_UNSUPPORTED:
            errorCode = ERR_UNSUPPORTED_METRIC;
            InterlockedIncrement(&s_unsupportedRequests);
            break;
         case SYSINFO_RC_UNKNOWN:
            break;
         default:
            nxlog_write(NXLOG_ERROR, _T("Internal error: unexpected return code %d in GetParameterValueFromExtProvider(\"%s\")"), rc, param);
            errorCode = ERR_INTERNAL_ERROR;
            InterlockedIncrement(&s_failedRequests);
            break;
      }
   }

   if (errorCode == ERR_UNKNOWN_METRIC)
   {
		errorCode = GetParameterValueFromAppAgent(param, value);
		if (errorCode == ERR_SUCCESS)
		{
         InterlockedIncrement(&s_processedRequests);
		}
		else if (errorCode != ERR_UNKNOWN_METRIC)
		{
         InterlockedIncrement(&s_failedRequests);
		}
   }

   if (errorCode == ERR_UNKNOWN_METRIC)
   {
		errorCode = GetParameterValueFromExtSubagent(param, value);
		if (errorCode == ERR_SUCCESS)
		{
         InterlockedIncrement(&s_processedRequests);
		}
		else if ((errorCode == ERR_UNSUPPORTED_METRIC) || (errorCode == ERR_UNKNOWN_METRIC))
		{
         InterlockedIncrement(&s_unsupportedRequests);
		}
		else
		{
         InterlockedIncrement(&s_failedRequests);
		}
   }

	session->debugPrintf(7, _T("GetMetricValue(\"%s\"): %u (%s) value = \"%s\""), param, errorCode, GetErrorCodeSymbolicName(errorCode), (errorCode == ERR_SUCCESS) ? value : _T(""));
   return errorCode;
}

/**
 * Get list's value
 */
uint32_t GetListValue(const TCHAR *param, StringList *value, AbstractCommSession *session)
{
   uint32_t errorCode = ERR_UNKNOWN_METRIC;
   session->debugPrintf(5, _T("Requesting list \"%s\""), param);
   for(int i = 0; i < s_lists.size(); i++)
	{
      NETXMS_SUBAGENT_LIST *list = s_lists.get(i);
      if (MatchString(list->name, param, false))
      {
         LONG rc = list->handler(param, list->arg, value, session);
         switch(rc)
         {
            case SYSINFO_RC_SUCCESS:
               errorCode = ERR_SUCCESS;
               InterlockedIncrement(&s_processedRequests);
               break;
            case SYSINFO_RC_ACCESS_DENIED:
               errorCode = ERR_ACCESS_DENIED;
               InterlockedIncrement(&s_failedRequests);
               break;
            case SYSINFO_RC_ERROR:
               errorCode = ERR_INTERNAL_ERROR;
               InterlockedIncrement(&s_failedRequests);
               break;
            case SYSINFO_RC_NO_SUCH_INSTANCE:
               errorCode = ERR_NO_SUCH_INSTANCE;
               InterlockedIncrement(&s_failedRequests);
               break;
            case SYSINFO_RC_UNSUPPORTED:
               errorCode = ERR_UNSUPPORTED_METRIC;
               InterlockedIncrement(&s_unsupportedRequests);
               break;
            default:
               nxlog_write(NXLOG_ERROR, _T("Internal error: unexpected return code %d in GetListValue(\"%s\")"), rc, param);
               errorCode = ERR_INTERNAL_ERROR;
               InterlockedIncrement(&s_failedRequests);
               break;
         }
         break;
      }
	}

	if (errorCode == ERR_UNKNOWN_METRIC)
   {
		errorCode = GetListValueFromExtSubagent(param, value);
		if (errorCode == ERR_SUCCESS)
		{
         InterlockedIncrement(&s_processedRequests);
		}
		else if ((errorCode == ERR_UNSUPPORTED_METRIC) || (errorCode == ERR_UNKNOWN_METRIC))
		{
         InterlockedIncrement(&s_unsupportedRequests);
		}
		else
		{
         InterlockedIncrement(&s_failedRequests);
		}
   }

	session->debugPrintf(7, _T("GetListValue(): result is %u (%s)"), errorCode, GetErrorCodeSymbolicName(errorCode));
   return errorCode;
}

/**
 * Get table's value
 */
uint32_t GetTableValue(const TCHAR *param, Table *value, AbstractCommSession *session)
{
   uint32_t errorCode = ERR_UNKNOWN_METRIC;
   session->debugPrintf(5, _T("Requesting table \"%s\""), param);
   for(int i = 0; i < s_tables.size(); i++)
	{
      NETXMS_SUBAGENT_TABLE *t = s_tables.get(i);
      if (MatchString(t->name, param, FALSE))
      {
         // pre-fill table columns if specified in table definition
         if (t->numColumns > 0)
         {
            for(int c = 0; c < t->numColumns; c++)
            {
               NETXMS_SUBAGENT_TABLE_COLUMN *col = &t->columns[c];
               value->addColumn(col->name, col->dataType, col->displayName, col->isInstance);
            }
         }

         LONG rc = t->handler(param, t->arg, value, session);
         switch(rc)
         {
            case SYSINFO_RC_SUCCESS:
               errorCode = ERR_SUCCESS;
               InterlockedIncrement(&s_processedRequests);
               break;
            case SYSINFO_RC_ACCESS_DENIED:
               errorCode = ERR_ACCESS_DENIED;
               InterlockedIncrement(&s_failedRequests);
               break;
            case SYSINFO_RC_ERROR:
               errorCode = ERR_INTERNAL_ERROR;
               InterlockedIncrement(&s_failedRequests);
               break;
            case SYSINFO_RC_NO_SUCH_INSTANCE:
               errorCode = ERR_NO_SUCH_INSTANCE;
               InterlockedIncrement(&s_failedRequests);
               break;
            case SYSINFO_RC_UNSUPPORTED:
               errorCode = ERR_UNSUPPORTED_METRIC;
               InterlockedIncrement(&s_unsupportedRequests);
               break;
            default:
               nxlog_write(NXLOG_ERROR, _T("Internal error: unexpected return code %d in GetTableValue(\"%s\")"), rc, param);
               errorCode = ERR_INTERNAL_ERROR;
               InterlockedIncrement(&s_failedRequests);
               break;
         }
         break;
      }
	}

   if (errorCode == ERR_UNKNOWN_METRIC)
   {
      session->debugPrintf(7, _T("GetTableValue(): requesting table from external data providers"));
      LONG rc = GetTableValueFromExtProvider(param, value);
      switch(rc)
      {
         case SYSINFO_RC_SUCCESS:
            errorCode = ERR_SUCCESS;
            InterlockedIncrement(&s_processedRequests);
            break;
         case SYSINFO_RC_ACCESS_DENIED:
            errorCode = ERR_ACCESS_DENIED;
            InterlockedIncrement(&s_failedRequests);
            break;
         case SYSINFO_RC_ERROR:
            errorCode = ERR_INTERNAL_ERROR;
            InterlockedIncrement(&s_failedRequests);
            break;
         case SYSINFO_RC_UNSUPPORTED:
            errorCode = ERR_UNSUPPORTED_METRIC;
            InterlockedIncrement(&s_unsupportedRequests);
            break;
         case SYSINFO_RC_UNKNOWN:
            break;
      }
   }

	if (errorCode == ERR_UNKNOWN_METRIC)
   {
      session->debugPrintf(7, _T("GetTableValue(): requesting table from external subagents"));
		errorCode = GetTableValueFromExtSubagent(param, value);
		if (errorCode == ERR_SUCCESS)
		{
         InterlockedIncrement(&s_processedRequests);
		}
		else if ((errorCode == ERR_UNKNOWN_METRIC) || (errorCode == ERR_UNSUPPORTED_METRIC))
		{
         InterlockedIncrement(&s_unsupportedRequests);
		}
		else
		{
         InterlockedIncrement(&s_failedRequests);
		}
   }

   if (errorCode == ERR_SUCCESS)
   {
      session->debugPrintf(7, _T("GetTableValue(): result is SUCCESS, value contains %d rows"), value->getNumRows());
      value->dump(session->getDebugTag(), 7, _T("   "), true, _T('|'));
   }
   else
   {
	   session->debugPrintf(7, _T("GetTableValue(): result is %u (%s)"), errorCode, GetErrorCodeSymbolicName(errorCode));
   }
   return errorCode;
}

/**
 * Put complete list of supported parameters into NXCP message
 */
void GetParameterList(NXCPMessage *msg)
{
   int i;
   uint32_t fieldId, count;

	// Parameters
   for(i = 0, count = 0, fieldId = VID_PARAM_LIST_BASE; i < s_metrics.size(); i++)
   {
      NETXMS_SUBAGENT_PARAM *p = s_metrics.get(i);
		if (p->dataType != DCI_DT_DEPRECATED)
		{
			msg->setField(fieldId++, p->name);
			msg->setField(fieldId++, p->description);
			msg->setField(fieldId++, static_cast<uint16_t>(p->dataType));
			count++;
		}
   }

   for(i = 0; i < s_metricsBackground.size(); i++)
   {
      BackgroundExternalMetric *p = s_metricsBackground.get(i);
		if (p->getDataType() != DCI_DT_DEPRECATED)
		{
			msg->setField(fieldId++, p->getName());
			msg->setField(fieldId++, p->getDescription());
			msg->setField(fieldId++, static_cast<uint16_t>(p->getDataType()));
			count++;
		}
   }
   
	ListParametersFromExtProviders(msg, &fieldId, &count);
	ListParametersFromExtSubagents(msg, &fieldId, &count);
   msg->setField(VID_NUM_PARAMETERS, count);

	// Push parameters
   msg->setField(VID_NUM_PUSH_PARAMETERS, static_cast<uint32_t>(s_pushMetrics.size()));
   for(i = 0, fieldId = VID_PUSHPARAM_LIST_BASE; i < s_pushMetrics.size(); i++)
   {
      NETXMS_SUBAGENT_PUSHPARAM *p = s_pushMetrics.get(i);
      msg->setField(fieldId++, p->name);
      msg->setField(fieldId++, p->description);
      msg->setField(fieldId++, static_cast<uint16_t>(p->dataType));
   }

	// Lists
   count = static_cast<uint32_t>(s_lists.size());
   for(i = 0, fieldId = VID_ENUM_LIST_BASE; i < s_lists.size(); i++)
   {
      msg->setField(fieldId++, s_lists.get(i)->name);
   }
	ListListsFromExtSubagents(msg, &fieldId, &count);
   msg->setField(VID_NUM_ENUMS, count);

	// Tables
   count = static_cast<uint32_t>(s_tables.size());
   for(i = 0, fieldId = VID_TABLE_LIST_BASE; i < s_tables.size(); i++)
   {
      NETXMS_SUBAGENT_TABLE *t = s_tables.get(i);
      msg->setField(fieldId++, t->name);
		msg->setField(fieldId++, t->instanceColumns);
		msg->setField(fieldId++, t->description);
   }
   ListTablesFromExtProviders(msg, &fieldId, &count);
	ListTablesFromExtSubagents(msg, &fieldId, &count);
   msg->setField(VID_NUM_TABLES, count);
}

/**
 * Put list of supported tables into NXCP message
 */
void GetTableList(NXCPMessage *msg)
{
   uint32_t fieldId = VID_TABLE_LIST_BASE;
   for(int i = 0; i < s_tables.size(); i++)
   {
      NETXMS_SUBAGENT_TABLE *t = s_tables.get(i);
      msg->setField(fieldId++, t->name);
		msg->setField(fieldId++, t->instanceColumns);
		msg->setField(fieldId++, t->description);
   }

   uint32_t count = static_cast<uint32_t>(s_tables.size());
   ListTablesFromExtProviders(msg, &fieldId, &count);
	ListTablesFromExtSubagents(msg, &fieldId, &count);
   msg->setField(VID_NUM_TABLES, count);
}

/**
 * Put list of supported lists (enums) into NXCP message
 */
void GetEnumList(NXCPMessage *msg)
{
   uint32_t fieldId = VID_ENUM_LIST_BASE;
   for(int i = 0; i < s_lists.size(); i++)
   {
      msg->setField(fieldId++, s_lists.get(i)->name);
   }

   uint32_t count = static_cast<uint32_t>(s_lists.size());
	ListListsFromExtSubagents(msg, &fieldId, &count);
   msg->setField(VID_NUM_ENUMS, count);
}

/**
 * Get number of background metrics
 */
int GetBackgroundMetricCount()
{
   return s_metricsBackground.size();
}

/**
 * Start background metrics
*/
void StartBackgroundMetrics()
{
   for(int i = 0; (i < s_metricsBackground.size()) && !(g_dwFlags & AF_SHUTDOWN); i++)
   {
      BackgroundExternalMetric *p = s_metricsBackground.get(i);
      ThreadPoolExecute(g_backgroundThreadPool, p, &BackgroundExternalMetric::poll);
   }
}