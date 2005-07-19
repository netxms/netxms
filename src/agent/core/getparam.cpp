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
LONG H_FileCount(char *pszCmd, char *pszArg, char *pValue);
LONG H_MD5Hash(char *cmd, char *arg, char *value);
LONG H_SHA1Hash(char *cmd, char *arg, char *value);
LONG H_SubAgentList(char *cmd, char *arg, NETXMS_VALUES_LIST *value);
LONG H_ActionList(char *cmd, char *arg, NETXMS_VALUES_LIST *value);
LONG H_ExternalParameter(char *pszCmd, char *pszArg, char *pValue);
LONG H_PlatformName(char *cmd, char *arg, char *value);

#ifdef _WIN32
LONG H_ArpCache(char *cmd, char *arg, NETXMS_VALUES_LIST *value);
LONG H_InterfaceList(char *cmd, char *arg, NETXMS_VALUES_LIST *value);
LONG H_IPRoutingTable(char *pszCmd, char *pArg, NETXMS_VALUES_LIST *pValue);
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
LONG H_PhysicalDiskInfo(char *pszParam, char *pszArg, char *pValue);
#endif


//
// Static data
//

static NETXMS_SUBAGENT_PARAM *m_pParamList = NULL;
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
// Handler for Agent.SupportedCiphers
//

static LONG H_SupportedCiphers(char *pszCmd, char *pArg, char *pValue)
{
   DWORD dwCiphers;

   dwCiphers = CSCPGetSupportedCiphers();
   if (dwCiphers == 0)
   {
      ret_string(pValue, "NONE");
   }
   else
   {
      *pValue = 0;
      if (dwCiphers & CSCP_SUPPORT_AES_256)
         strcat(pValue, "AES-256 ");
      if (dwCiphers & CSCP_SUPPORT_BLOWFISH)
         strcat(pValue, "BLOWFISH ");
      if (dwCiphers & CSCP_SUPPORT_IDEA)
         strcat(pValue, "IDEA ");
      if (dwCiphers & CSCP_SUPPORT_3DES)
         strcat(pValue, "3DES ");
      pValue[strlen(pValue) - 1] = 0;
   }
   return SYSINFO_RC_SUCCESS;
}


//
// Handler for parameters list
//

static LONG H_ParamList(char *cmd, char *arg, NETXMS_VALUES_LIST *value)
{
   int i;

   for(i = 0; i < m_iNumParams; i++)
      NxAddResultString(value, m_pParamList[i].szName);
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

static NETXMS_SUBAGENT_PARAM m_stdParams[] =
{
#ifdef _WIN32
   { "Disk.Free(*)", H_DiskInfo, NULL, DCI_DT_UINT64, "Free disk space on *" },
   { "Disk.Total(*)", H_DiskInfo, NULL, DCI_DT_UINT64, "Total disk space on *" },
   { "Disk.Used(*)", H_DiskInfo, NULL, DCI_DT_UINT64, "Used disk space on *" },
   { "Net.Interface.AdminStatus(*)", H_NetInterfaceStats, (char *)NET_IF_ADMIN_STATUS, DCI_DT_INT, "Administrative status of interface {instance}" },
   { "Net.Interface.BytesIn(*)", H_NetInterfaceStats, (char *)NET_IF_BYTES_IN, DCI_DT_UINT, "Number of input bytes on interface {instance}" },
   { "Net.Interface.BytesOut(*)", H_NetInterfaceStats, (char *)NET_IF_BYTES_OUT, DCI_DT_UINT, "Number of output bytes on interface {instance}" },
   { "Net.Interface.Description(*)", H_NetInterfaceStats, (char *)NET_IF_DESCR, DCI_DT_STRING, "" },
   { "Net.Interface.InErrors(*)", H_NetInterfaceStats, (char *)NET_IF_IN_ERRORS, DCI_DT_UINT, "Number of input errors on interface {instance}" },
   { "Net.Interface.Link(*)", H_NetInterfaceStats, (char *)NET_IF_LINK, DCI_DT_INT, "Link status for interface {instance}" },
   { "Net.Interface.OutErrors(*)", H_NetInterfaceStats, (char *)NET_IF_OUT_ERRORS, DCI_DT_UINT, "Number of output errors on interface {instance}" },
   { "Net.Interface.PacketsIn(*)", H_NetInterfaceStats, (char *)NET_IF_PACKETS_IN, DCI_DT_UINT, "Number of input packets on interface {instance}" },
   { "Net.Interface.PacketsOut(*)", H_NetInterfaceStats, (char *)NET_IF_PACKETS_OUT, DCI_DT_UINT, "Number of output packets on interface {instance}" },
   { "Net.Interface.Speed(*)", H_NetInterfaceStats, (char *)NET_IF_SPEED, DCI_DT_UINT, "Speed of interface {instance}" },
   { "Net.IP.Forwarding", H_NetIPStats, (char *)NET_IP_FORWARDING, DCI_DT_INT, "IP forwarding status" },
   { "PhysicalDisk.Firmware(*)", H_PhysicalDiskInfo, "F", DCI_DT_STRING, "Firmware version of hard disk {instance}" },
   { "PhysicalDisk.Model(*)", H_PhysicalDiskInfo, "M", DCI_DT_STRING, "Model of hard disk {instance}" },
   { "PhysicalDisk.SerialNumber(*)", H_PhysicalDiskInfo, "N", DCI_DT_STRING, "Serial number of hard disk {instance}" },
   { "PhysicalDisk.SmartAttr(*)", H_PhysicalDiskInfo, "A", DCI_DT_STRING, "" },
   { "PhysicalDisk.SmartStatus(*)", H_PhysicalDiskInfo, "S", DCI_DT_INT, "Status of hard disk {instance} reported by SMART" },
   { "PhysicalDisk.Temperature(*)", H_PhysicalDiskInfo, "T", DCI_DT_INT, "Temperature of hard disk {instance}" },
   { "Process.Count(*)", H_ProcCountSpecific, NULL, DCI_DT_UINT, "" },
   { "Process.GdiObj(*)", H_ProcInfo, (char *)PROCINFO_GDI_OBJ, DCI_DT_UINT64, "" },
   { "Process.IO.OtherB(*)", H_ProcInfo, (char *)PROCINFO_IO_OTHER_B, DCI_DT_UINT64, "" },
   { "Process.IO.OtherOp(*)", H_ProcInfo, (char *)PROCINFO_IO_OTHER_OP, DCI_DT_UINT64, "" },
   { "Process.IO.ReadB(*)", H_ProcInfo, (char *)PROCINFO_IO_READ_B, DCI_DT_UINT64, "" },
   { "Process.IO.ReadOp(*)", H_ProcInfo, (char *)PROCINFO_IO_READ_OP, DCI_DT_UINT64, "" },
   { "Process.IO.WriteB(*)", H_ProcInfo, (char *)PROCINFO_IO_WRITE_B, DCI_DT_UINT64, "" },
   { "Process.IO.WriteOp(*)", H_ProcInfo, (char *)PROCINFO_IO_WRITE_OP, DCI_DT_UINT64, "" },
   { "Process.KernelTime(*)", H_ProcInfo, (char *)PROCINFO_KTIME, DCI_DT_UINT64, "" },
   { "Process.PageFaults(*)", H_ProcInfo, (char *)PROCINFO_PF, DCI_DT_UINT64, "" },
   { "Process.UserObj(*)", H_ProcInfo, (char *)PROCINFO_USER_OBJ, DCI_DT_UINT64, "" },
   { "Process.UserTime(*)", H_ProcInfo, (char *)PROCINFO_UTIME, DCI_DT_UINT64, "" },
   { "Process.VMSize(*)", H_ProcInfo, (char *)PROCINFO_VMSIZE, DCI_DT_UINT64, "" },
   { "Process.WkSet(*)", H_ProcInfo, (char *)PROCINFO_WKSET, DCI_DT_UINT64, "" },
   { "System.CPU.Count", H_CPUCount, NULL, DCI_DT_UINT, "Number of CPU in the system" },
   { "System.Hostname", H_HostName, NULL, DCI_DT_STRING, "Host name" },
   { "System.Memory.Physical.Free", H_MemoryInfo, (char *)MEMINFO_PHYSICAL_FREE, DCI_DT_UINT64, "Free physical memory" },
   { "System.Memory.Physical.Total", H_MemoryInfo, (char *)MEMINFO_PHYSICAL_TOTAL, DCI_DT_UINT64, "Total amount of physical memory" },
   { "System.Memory.Physical.Used", H_MemoryInfo, (char *)MEMINFO_PHYSICAL_USED, DCI_DT_UINT64, "Used physical memory" },
   { "System.Memory.Virtual.Free", H_MemoryInfo, (char *)MEMINFO_VIRTUAL_FREE, DCI_DT_UINT64, "Free virtual memory" },
   { "System.Memory.Virtual.Total", H_MemoryInfo, (char *)MEMINFO_VIRTUAL_TOTAL, DCI_DT_UINT64, "Total amount of virtual memory" },
   { "System.Memory.Virtual.Used", H_MemoryInfo, (char *)MEMINFO_VIRTUAL_USED, DCI_DT_UINT64, "Used virtual memory" },
   { "System.ProcessCount", H_ProcCount, NULL, DCI_DT_INT, "Total number of processes" },
   { "System.ServiceState(*)", H_ServiceState, NULL, DCI_DT_INT, "State of {instance} service" },
   { "System.ThreadCount", H_ThreadCount, NULL, DCI_DT_INT, "Total number of threads" },
   { "System.Uname", H_SystemUname, NULL, DCI_DT_STRING, "System uname" },
#endif
   { "Agent.AcceptedConnections", H_UIntPtr, (char *)&g_dwAcceptedConnections, DCI_DT_UINT, "" },
   { "Agent.AcceptErrors", H_UIntPtr, (char *)&g_dwAcceptErrors, DCI_DT_UINT, "" },
   { "Agent.AuthenticationFailures", H_UIntPtr, (char *)&m_dwAuthenticationFailures, DCI_DT_UINT, "" },
   { "Agent.FailedRequests", H_UIntPtr, (char *)&m_dwFailedRequests, DCI_DT_UINT, "" },
   { "Agent.ProcessedRequests", H_UIntPtr, (char *)&m_dwProcessedRequests, DCI_DT_UINT, "" },
   { "Agent.RejectedConnections", H_UIntPtr, (char *)&g_dwRejectedConnections, DCI_DT_UINT, "" },
   { "Agent.SourcePackageSupport", H_StringConstant, "0", DCI_DT_INT, "" },
   { "Agent.SupportedCiphers", H_SupportedCiphers, NULL, DCI_DT_STRING, "List of ciphers supported by agent" },
   { "Agent.TimedOutRequests", H_UIntPtr, (char *)&m_dwTimedOutRequests, DCI_DT_UINT, "" },
   { "Agent.UnsupportedRequests", H_UIntPtr, (char *)&m_dwUnsupportedRequests, DCI_DT_UINT, "" },
   { "Agent.Uptime", H_AgentUptime, NULL, DCI_DT_UINT, "Agent's uptime" },
   { "Agent.Version", H_StringConstant, AGENT_VERSION_STRING, DCI_DT_STRING, "Agent's version" },
   { "File.Count(*)", H_FileCount, NULL, DCI_DT_UINT, "" },
   { "File.Hash.CRC32(*)", H_CRC32, NULL, DCI_DT_UINT, "CRC32 checksum of {instance}" },
   { "File.Hash.MD5(*)", H_MD5Hash, NULL, DCI_DT_STRING, "MD5 hash of {instance}" },
   { "File.Hash.SHA1(*)", H_SHA1Hash, NULL, DCI_DT_STRING, "SHA1 hash of {instance}" },
   { "File.Size(*)", H_FileSize, NULL, DCI_DT_UINT64, "" },
   { "System.PlatformName", H_PlatformName, NULL, DCI_DT_STRING, "" }
};


//
// Standard agent's enumerations
//

static NETXMS_SUBAGENT_ENUM m_stdEnums[] =
{
#ifdef _WIN32
   { "Net.ArpCache", H_ArpCache, NULL },
   { "Net.InterfaceList", H_InterfaceList, NULL },
   { "Net.IP.RoutingTable", H_IPRoutingTable, NULL },
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

   m_iNumParams = sizeof(m_stdParams) / sizeof(NETXMS_SUBAGENT_PARAM);
   m_pParamList = (NETXMS_SUBAGENT_PARAM *)malloc(sizeof(NETXMS_SUBAGENT_PARAM) * m_iNumParams);
   if (m_pParamList == NULL)
      return FALSE;
   memcpy(m_pParamList, m_stdParams, sizeof(NETXMS_SUBAGENT_PARAM) * m_iNumParams);

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

void AddParameter(char *pszName, LONG (* fpHandler)(char *,char *,char *), char *pArg,
                  int iDataType, char *pszDescription)
{
   int i;

   // Search for existing parameter
   for(i = 0; i < m_iNumParams; i++)
      if (!stricmp(m_pParamList[i].szName, pszName))
         break;
   if (i < m_iNumParams)
   {
      // Replace existing handler and attributes
      m_pParamList[i].fpHandler = fpHandler;
      m_pParamList[i].iDataType = iDataType;
      strncpy(m_pParamList[i].szDescription, pszDescription, MAX_DB_STRING);

      // If we are replacing System.PlatformName, add pointer to
      // platform suffix as argument, otherwise, use supplied pArg
      if (!strcmp(pszName, "System.PlatformName"))
      {
         m_pParamList[i].pArg = g_szPlatformSuffix;
      }
      else
      {
         m_pParamList[i].pArg = pArg;
      }
   }
   else
   {
      // Add new parameter
      m_pParamList = (NETXMS_SUBAGENT_PARAM *)realloc(m_pParamList, sizeof(NETXMS_SUBAGENT_PARAM) * (m_iNumParams + 1));
      strncpy(m_pParamList[m_iNumParams].szName, pszName, MAX_PARAM_NAME - 1);
      m_pParamList[m_iNumParams].fpHandler = fpHandler;
      m_pParamList[m_iNumParams].pArg = pArg;
      m_pParamList[m_iNumParams].iDataType = iDataType;
      strncpy(m_pParamList[m_iNumParams].szDescription, pszDescription, MAX_DB_STRING);
      m_iNumParams++;
   }
}


//
// Add enum to list
//

void AddEnum(char *pszName, LONG (* fpHandler)(char *,char *,NETXMS_VALUES_LIST *), char *pArg)
{
   int i;

   // Search for existing enum
   for(i = 0; i < m_iNumEnums; i++)
      if (!stricmp(m_pEnumList[i].szName, pszName))
         break;
   if (i < m_iNumEnums)
   {
      // Replace existing handler and arg
      m_pEnumList[i].fpHandler = fpHandler;
      m_pEnumList[i].pArg = pArg;
   }
   else
   {
      // Add new enum
      m_pEnumList = (NETXMS_SUBAGENT_ENUM *)realloc(m_pEnumList, sizeof(NETXMS_SUBAGENT_ENUM) * (m_iNumEnums + 1));
      strncpy(m_pEnumList[m_iNumEnums].szName, pszName, MAX_PARAM_NAME - 1);
      m_pEnumList[m_iNumEnums].fpHandler = fpHandler;
      m_pEnumList[m_iNumEnums].pArg = pArg;
      m_iNumEnums++;
   }
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

   AddParameter(pszCfgLine, H_ExternalParameter, strdup(pszCmdLine), DCI_DT_STRING, "");
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
      if (MatchString(m_pParamList[i].szName, pszParam, FALSE))
      {
         rc = m_pParamList[i].fpHandler(pszParam, m_pParamList[i].pArg, pszValue);
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


//
// Put complete list of supported parameters into CSCP message
//

void GetParameterList(CSCPMessage *pMsg)
{
   int i;
   DWORD dwId;

   pMsg->SetVariable(VID_NUM_PARAMETERS, (DWORD)m_iNumParams);
   for(i = 0, dwId = VID_PARAM_LIST_BASE; i < m_iNumParams; i++)
   {
      pMsg->SetVariable(dwId++, m_pParamList[i].szName);
      pMsg->SetVariable(dwId++, m_pParamList[i].szDescription);
      pMsg->SetVariable(dwId++, (WORD)m_pParamList[i].iDataType);
   }
}
