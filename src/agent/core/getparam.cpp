/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: getparam.cpp
**
**/

#include "nxagentd.h"


//
// Externals
//

LONG H_AgentStats(char *cmd, char *arg, char *value);
LONG H_AgentUptime(char *cmd, char *arg, char *value);
LONG H_CRC32(char *cmd, char *arg, char *value);
LONG H_FileSize(char *cmd, char *arg, char *value);
LONG H_MD5Hash(char *cmd, char *arg, char *value);
LONG H_SHA1Hash(char *cmd, char *arg, char *value);
LONG H_SubAgentList(char *cmd, char *arg, NETXMS_VALUES_LIST *value);
LONG H_ActionList(char *cmd, char *arg, NETXMS_VALUES_LIST *value);
LONG H_ExternalParameter(char *pszCmd, char *pszArg, char *pValue);

#ifdef _WIN32
LONG H_ArpCache(char *cmd, char *arg, NETXMS_VALUES_LIST *value);
LONG H_InterfaceList(char *cmd, char *arg, NETXMS_VALUES_LIST *value);
LONG H_ProcessList(char *cmd, char *arg, NETXMS_VALUES_LIST *value);
LONG H_ProcCount(char *cmd, char *arg, char *value);
LONG H_ProcCountSpecific(char *cmd, char *arg, char *value);
LONG H_ProcInfo(char *cmd, char *arg, char *value);
LONG H_DiskInfo(char *cmd, char *arg, char *value);
LONG H_MemoryInfo(char *cmd, char *arg, char *value);
LONG H_HostName(char *cmd, char *arg, char *value);
LONG H_SystemUname(char *cmd, char *arg, char *value);
LONG H_ThreadCount(char *cmd, char *arg, char *value);
LONG H_NetIPStats(char *cmd, char *arg, char *value);
LONG H_NetInterfaceStats(char *cmd, char *arg, char *value);
LONG H_ServiceState(char *cmd, char *arg, char *value);
LONG H_CPUCount(char *cmd, char *arg, char *value);
#endif


//
// Static data
//

static AGENT_PARAM *m_pParamList = NULL;
static int m_iNumParams = 0;
static NETXMS_SUBAGENT_ENUM *m_pEnumList = NULL;
static int m_iNumEnums = 0;
static DWORD m_dwTimedOutRequests = 0;
static DWORD m_dwAuthenticationFailures = 0;
static DWORD m_dwProcessedRequests = 0;
static DWORD m_dwFailedRequests = 0;
static DWORD m_dwUnsupportedRequests = 0;


//
// Handler for parameters which always returns string constant
//

static LONG H_StringConstant(char *cmd, char *arg, char *value)
{
   ret_string(value, arg);
   return SYSINFO_RC_SUCCESS;
}


//
// Handler for parameters which returns floating point value from specific variable
//

static LONG H_FloatPtr(char *cmd, char *arg, char *value)
{
   ret_double(value, *((double *)arg));
   return SYSINFO_RC_SUCCESS;
}


//
// Handler for parameters which returns DWORD value from specific variable
//

static LONG H_UIntPtr(char *cmd, char *arg, char *value)
{
   ret_uint(value, *((DWORD *)arg));
   return SYSINFO_RC_SUCCESS;
}


//
// Handler for parameters list
//

static LONG H_ParamList(char *cmd, char *arg, NETXMS_VALUES_LIST *value)
{
   int i;

   for(i = 0; i < m_iNumParams; i++)
      NxAddResultString(value, m_pParamList[i].name);
   return SYSINFO_RC_SUCCESS;
}


//
// Handler for enums list
//

static LONG H_EnumList(char *cmd, char *arg, NETXMS_VALUES_LIST *value)
{
   int i;

   for(i = 0; i < m_iNumEnums; i++)
      NxAddResultString(value, m_pEnumList[i].szName);
   return SYSINFO_RC_SUCCESS;
}


//
// Standard agent's parameters
//

static AGENT_PARAM m_stdParams[] =
{
#ifdef _WIN32
   { "Disk.Free(*)", H_DiskInfo, NULL },
   { "Disk.Total(*)", H_DiskInfo, NULL },
   { "Disk.Used(*)", H_DiskInfo, NULL },
   { "Net.Interface.AdminStatus(*)", H_NetInterfaceStats, (char *)NET_IF_ADMIN_STATUS },
   { "Net.Interface.BytesIn(*)", H_NetInterfaceStats, (char *)NET_IF_BYTES_IN },
   { "Net.Interface.BytesOut(*)", H_NetInterfaceStats, (char *)NET_IF_BYTES_OUT },
   { "Net.Interface.Description(*)", H_NetInterfaceStats, (char *)NET_IF_DESCR },
   { "Net.Interface.InErrors(*)", H_NetInterfaceStats, (char *)NET_IF_IN_ERRORS },
   { "Net.Interface.Link(*)", H_NetInterfaceStats, (char *)NET_IF_LINK },
   { "Net.Interface.OutErrors(*)", H_NetInterfaceStats, (char *)NET_IF_OUT_ERRORS },
   { "Net.Interface.PacketsIn(*)", H_NetInterfaceStats, (char *)NET_IF_PACKETS_IN },
   { "Net.Interface.PacketsOut(*)", H_NetInterfaceStats, (char *)NET_IF_PACKETS_OUT },
   { "Net.Interface.Speed(*)", H_NetInterfaceStats, (char *)NET_IF_SPEED },
   { "Net.IP.Forwarding", H_NetIPStats, (char *)NET_IP_FORWARDING },
   { "Process.Count(*)", H_ProcCountSpecific, NULL },
   { "Process.GdiObj(*)", H_ProcInfo, (char *)PROCINFO_GDI_OBJ },
   { "Process.IO.OtherB(*)", H_ProcInfo, (char *)PROCINFO_IO_OTHER_B },
   { "Process.IO.OtherOp(*)", H_ProcInfo, (char *)PROCINFO_IO_OTHER_OP },
   { "Process.IO.ReadB(*)", H_ProcInfo, (char *)PROCINFO_IO_READ_B },
   { "Process.IO.ReadOp(*)", H_ProcInfo, (char *)PROCINFO_IO_READ_OP },
   { "Process.IO.WriteB(*)", H_ProcInfo, (char *)PROCINFO_IO_WRITE_B },
   { "Process.IO.WriteOp(*)", H_ProcInfo, (char *)PROCINFO_IO_WRITE_OP },
   { "Process.KernelTime(*)", H_ProcInfo, (char *)PROCINFO_KTIME },
   { "Process.PageFaults(*)", H_ProcInfo, (char *)PROCINFO_PF },
   { "Process.UserObj(*)", H_ProcInfo, (char *)PROCINFO_USER_OBJ },
   { "Process.UserTime(*)", H_ProcInfo, (char *)PROCINFO_UTIME },
   { "Process.VMSize(*)", H_ProcInfo, (char *)PROCINFO_VMSIZE },
   { "Process.WkSet(*)", H_ProcInfo, (char *)PROCINFO_WKSET },
   { "System.CPU.Count", H_CPUCount, NULL },
   { "System.Hostname", H_HostName, NULL },
   { "System.Memory.Physical.Free", H_MemoryInfo, (char *)MEMINFO_PHYSICAL_FREE },
   { "System.Memory.Physical.Total", H_MemoryInfo, (char *)MEMINFO_PHYSICAL_TOTAL },
   { "System.Memory.Physical.Used", H_MemoryInfo, (char *)MEMINFO_PHYSICAL_USED },
   { "System.Memory.Swap.Free", H_MemoryInfo, (char *)MEMINFO_SWAP_FREE },
   { "System.Memory.Swap.Total", H_MemoryInfo, (char *)MEMINFO_SWAP_TOTAL },
   { "System.Memory.Swap.Used", H_MemoryInfo, (char *)MEMINFO_SWAP_USED },
   { "System.Memory.Virtual.Free", H_MemoryInfo, (char *)MEMINFO_VIRTUAL_FREE },
   { "System.Memory.Virtual.Total", H_MemoryInfo, (char *)MEMINFO_VIRTUAL_TOTAL },
   { "System.Memory.Virtual.Used", H_MemoryInfo, (char *)MEMINFO_VIRTUAL_USED },
   { "System.ProcessCount", H_ProcCount, NULL },
   { "System.ServiceState(*)", H_ServiceState, NULL },
   { "System.ThreadCount", H_ThreadCount, NULL },
   { "System.Uname", H_SystemUname, NULL },
#endif
   { "Agent.AcceptedConnections", H_UIntPtr, (char *)&g_dwAcceptedConnections },
   { "Agent.AcceptErrors", H_UIntPtr, (char *)&g_dwAcceptErrors },
   { "Agent.AuthenticationFailures", H_UIntPtr, (char *)&m_dwAuthenticationFailures },
   { "Agent.FailedRequests", H_UIntPtr, (char *)&m_dwFailedRequests },
   { "Agent.ProcessedRequests", H_UIntPtr, (char *)&m_dwProcessedRequests },
   { "Agent.RejectedConnections", H_UIntPtr, (char *)&g_dwRejectedConnections },
   { "Agent.TimedOutRequests", H_UIntPtr, (char *)&m_dwTimedOutRequests },
   { "Agent.UnsupportedRequests", H_UIntPtr, (char *)&m_dwUnsupportedRequests },
   { "Agent.Uptime", H_AgentUptime, NULL },
   { "Agent.Version", H_StringConstant, AGENT_VERSION_STRING },
   { "File.Hash.CRC32(*)", H_CRC32, NULL },
   { "File.Hash.MD5(*)", H_MD5Hash, NULL },
   { "File.Hash.SHA1(*)", H_SHA1Hash, NULL },
   { "File.Size(*)", H_FileSize, NULL }
};


//
// Standard agent's enumerations
//

static NETXMS_SUBAGENT_ENUM m_stdEnums[] =
{
#ifdef _WIN32
   { "Net.ArpCache", H_ArpCache, NULL },
   { "Net.InterfaceList", H_InterfaceList, NULL },
   { "System.ProcessList", H_ProcessList, NULL },
#endif
   { "Agent.ActionList", H_ActionList, NULL },
   { "Agent.SubAgentList", H_SubAgentList, NULL },
   { "Agent.SupportedEnums", H_EnumList, NULL },
   { "Agent.SupportedParameters", H_ParamList, NULL }
};


//
// Initialize dynamic parameters list from default static list
//

BOOL InitParameterList(void)
{
   if ((m_pParamList != NULL) || (m_pEnumList != NULL))
      return FALSE;

   m_iNumParams = sizeof(m_stdParams) / sizeof(AGENT_PARAM);
   m_pParamList = (AGENT_PARAM *)malloc(sizeof(AGENT_PARAM) * m_iNumParams);
   if (m_pParamList == NULL)
      return FALSE;
   memcpy(m_pParamList, m_stdParams, sizeof(AGENT_PARAM) * m_iNumParams);

   m_iNumEnums = sizeof(m_stdEnums) / sizeof(NETXMS_SUBAGENT_ENUM);
   m_pEnumList = (NETXMS_SUBAGENT_ENUM *)malloc(sizeof(NETXMS_SUBAGENT_ENUM) * m_iNumEnums);
   if (m_pEnumList == NULL)
      return FALSE;
   memcpy(m_pEnumList, m_stdEnums, sizeof(NETXMS_SUBAGENT_ENUM) * m_iNumEnums);

   return TRUE;
}


//
// Add parameter to list
//

void AddParameter(char *szName, LONG (* fpHandler)(char *,char *,char *), char *pArg)
{
   m_pParamList = (AGENT_PARAM *)realloc(m_pParamList, sizeof(AGENT_PARAM) * (m_iNumParams + 1));
   strncpy(m_pParamList[m_iNumParams].name, szName, MAX_PARAM_NAME - 1);
   m_pParamList[m_iNumParams].handler = fpHandler;
   m_pParamList[m_iNumParams].arg = pArg;
   m_iNumParams++;
}


//
// Add enum to list
//

void AddEnum(char *szName, LONG (* fpHandler)(char *,char *,NETXMS_VALUES_LIST *), char *pArg)
{
   m_pEnumList = (NETXMS_SUBAGENT_ENUM *)realloc(m_pEnumList, sizeof(NETXMS_SUBAGENT_ENUM) * (m_iNumEnums + 1));
   strncpy(m_pEnumList[m_iNumEnums].szName, szName, MAX_PARAM_NAME - 1);
   m_pEnumList[m_iNumEnums].fpHandler = fpHandler;
   m_pEnumList[m_iNumEnums].pArg = pArg;
   m_iNumEnums++;
}


//
// Add external parameter
//

BOOL AddExternalParameter(char *pszCfgLine)
{
   char *pszCmdLine;

   pszCmdLine = strchr(pszCfgLine, ':');
   if (pszCmdLine == NULL)
      return FALSE;

   *pszCmdLine = 0;
   pszCmdLine++;
   StrStrip(pszCfgLine);
   StrStrip(pszCmdLine);
   if ((*pszCfgLine == 0) || (*pszCmdLine == 0))
      return FALSE;

   AddParameter(pszCfgLine, H_ExternalParameter, strdup(pszCmdLine));
   return TRUE;
}


//
// Get parameter's value
//

DWORD GetParameterValue(char *pszParam, char *pszValue)
{
   int i, rc;
   DWORD dwErrorCode;

   DebugPrintf("Requesting parameter \"%s\"", pszParam);
   for(i = 0; i < m_iNumParams; i++)
      if (MatchString(m_pParamList[i].name, pszParam, FALSE))
      {
         rc = m_pParamList[i].handler(pszParam, m_pParamList[i].arg, pszValue);
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
            case SYSINFO_RC_UNSUPPORTED:
               dwErrorCode = ERR_UNKNOWN_PARAMETER;
               m_dwUnsupportedRequests++;
               break;
            default:
               WriteLog(MSG_UNEXPECTED_IRC, EVENTLOG_ERROR_TYPE, "ds", rc, pszParam);
               dwErrorCode = ERR_INTERNAL_ERROR;
               m_dwFailedRequests++;
               break;
         }
         break;
      }
   if (i == m_iNumParams)
   {
      dwErrorCode = ERR_UNKNOWN_PARAMETER;
      m_dwUnsupportedRequests++;
   }
   return dwErrorCode;
}


//
// Get parameter's value
//

DWORD GetEnumValue(char *pszParam, NETXMS_VALUES_LIST *pValue)
{
   int i, rc;
   DWORD dwErrorCode;

   DebugPrintf("Requesting enum \"%s\"", pszParam);
   for(i = 0; i < m_iNumEnums; i++)
      if (MatchString(m_pEnumList[i].szName, pszParam, FALSE))
      {
         rc = m_pEnumList[i].fpHandler(pszParam, m_pEnumList[i].pArg, pValue);
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
            case SYSINFO_RC_UNSUPPORTED:
               dwErrorCode = ERR_UNKNOWN_PARAMETER;
               m_dwUnsupportedRequests++;
               break;
            default:
               WriteLog(MSG_UNEXPECTED_IRC, EVENTLOG_ERROR_TYPE, "ds", rc, pszParam);
               dwErrorCode = ERR_INTERNAL_ERROR;
               m_dwFailedRequests++;
               break;
         }
         break;
      }
   if (i == m_iNumEnums)
   {
      dwErrorCode = ERR_UNKNOWN_PARAMETER;
      m_dwUnsupportedRequests++;
   }
   return dwErrorCode;
}
