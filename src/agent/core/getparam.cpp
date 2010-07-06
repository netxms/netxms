/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2010 Victor Kirhenshtein
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


//
// Externals
//

LONG H_ActiveConnections(const char *cmd, const char *arg, char *pValue);
LONG H_AgentStats(const char *cmd, const char *arg, char *value);
LONG H_AgentUptime(const char *cmd, const char *arg, char *value);
LONG H_CRC32(const char *cmd, const char *arg, char *value);
LONG H_DirInfo(const char *cmd, const char *arg, char *value);
LONG H_FileTime(const char *cmd, const char *arg, char *value);
LONG H_MD5Hash(const char *cmd, const char *arg, char *value);
LONG H_SHA1Hash(const char *cmd, const char *arg, char *value);
LONG H_SubAgentList(const char *cmd, const char *arg, StringList *value);
LONG H_ActionList(const char *cmd, const char *arg, StringList *value);
LONG H_ExternalParameter(const char *cmd, const char *arg, char *value);
LONG H_PlatformName(const char *cmd, const char *arg, char *value);

#ifdef _WIN32
LONG H_ArpCache(const char *cmd, const char *arg, StringList *value);
LONG H_InterfaceList(const char *cmd, const char *arg, StringList *value);
LONG H_IPRoutingTable(const char *cmd, const char *arg, StringList *pValue);
LONG H_DiskInfo(const char *cmd, const char *arg, char *value);
LONG H_MemoryInfo(const char *cmd, const char *arg, char *value);
LONG H_HostName(const char *cmd, const char *arg, char *value);
LONG H_SystemUname(const char *cmd, const char *arg, char *value);
LONG H_NetIPStats(const char *cmd, const char *arg, char *value);
LONG H_NetInterfaceStats(const char *cmd, const char *arg, char *value);
LONG H_CPUCount(const char *cmd, const char *arg, char *value);
LONG H_PhysicalDiskInfo(const char *cmd, const char *arg, char *pValue);
#endif


//
// Static data
//

static NETXMS_SUBAGENT_PARAM *m_pParamList = NULL;
static int m_iNumParams = 0;
static NETXMS_SUBAGENT_PUSHPARAM *m_pPushParamList = NULL;
static int m_iNumPushParams = 0;
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

static LONG H_StringConstant(const char *cmd, const char *arg, char *value)
{
   ret_string(value, arg);
   return SYSINFO_RC_SUCCESS;
}


//
// Handler for parameters which returns floating point value from specific variable
//

static LONG H_FloatPtr(const char *cmd, const char *arg, char *value)
{
   ret_double(value, *((double *)arg));
   return SYSINFO_RC_SUCCESS;
}


//
// Handler for parameters which returns DWORD value from specific variable
//

static LONG H_UIntPtr(const char *cmd, const char *arg, char *value)
{
   ret_uint(value, *((DWORD *)arg));
   return SYSINFO_RC_SUCCESS;
}


//
// Handler for Agent.SupportedCiphers
//

static LONG H_SupportedCiphers(const char *pszCmd, const char *pArg, char *pValue)
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

static LONG H_ParamList(const char *cmd, const char *arg, StringList *value)
{
   int i;

   for(i = 0; i < m_iNumParams; i++)
		if (m_pParamList[i].iDataType != DCI_DT_DEPRECATED)
			value->add(m_pParamList[i].szName);
   return SYSINFO_RC_SUCCESS;
}


//
// Handler for push parameters list
//

static LONG H_PushParamList(const char *cmd, const char *arg, StringList *value)
{
   int i;

   for(i = 0; i < m_iNumPushParams; i++)
		value->add(m_pPushParamList[i].name);
   return SYSINFO_RC_SUCCESS;
}


//
// Handler for enums list
//

static LONG H_EnumList(const char *cmd, const char *arg, StringList *value)
{
   int i;

   for(i = 0; i < m_iNumEnums; i++)
		value->add(m_pEnumList[i].szName);
   return SYSINFO_RC_SUCCESS;
}


//
// Standard agent's parameters
//

static NETXMS_SUBAGENT_PARAM m_stdParams[] =
{
#ifdef _WIN32
   { "Disk.Free(*)", H_DiskInfo, (char *)DISKINFO_FREE_BYTES, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { "Disk.FreePerc(*)", H_DiskInfo, (char *)DISKINFO_FREE_SPACE_PCT, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { "Disk.Total(*)", H_DiskInfo, (char *)DISKINFO_TOTAL_BYTES, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { "Disk.Used(*)", H_DiskInfo, (char *)DISKINFO_USED_BYTES, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { "Disk.UsedPerc(*)", H_DiskInfo, (char *)DISKINFO_USED_SPACE_PCT, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { "FileSystem.Free(*)", H_DiskInfo, (char *)DISKINFO_FREE_BYTES, DCI_DT_UINT64, DCIDESC_FS_FREE },
   { "FileSystem.FreePerc(*)", H_DiskInfo, (char *)DISKINFO_FREE_SPACE_PCT, DCI_DT_FLOAT, DCIDESC_FS_FREEPERC },
   { "FileSystem.Total(*)", H_DiskInfo, (char *)DISKINFO_TOTAL_BYTES, DCI_DT_UINT64, DCIDESC_FS_TOTAL },
   { "FileSystem.Used(*)", H_DiskInfo, (char *)DISKINFO_USED_BYTES, DCI_DT_UINT64, DCIDESC_FS_USED },
   { "FileSystem.UsedPerc(*)", H_DiskInfo, (char *)DISKINFO_USED_SPACE_PCT, DCI_DT_FLOAT, DCIDESC_FS_USEDPERC },
   { "Net.Interface.AdminStatus(*)", H_NetInterfaceStats, (char *)NETINFO_IF_ADMIN_STATUS, DCI_DT_INT, DCIDESC_NET_INTERFACE_ADMINSTATUS },
   { "Net.Interface.BytesIn(*)", H_NetInterfaceStats, (char *)NETINFO_IF_BYTES_IN, DCI_DT_UINT, DCIDESC_NET_INTERFACE_BYTESIN },
   { "Net.Interface.BytesOut(*)", H_NetInterfaceStats, (char *)NETINFO_IF_BYTES_OUT, DCI_DT_UINT, DCIDESC_NET_INTERFACE_BYTESOUT },
   { "Net.Interface.Description(*)", H_NetInterfaceStats, (char *)NETINFO_IF_DESCR, DCI_DT_STRING, DCIDESC_NET_INTERFACE_DESCRIPTION },
   { "Net.Interface.InErrors(*)", H_NetInterfaceStats, (char *)NETINFO_IF_IN_ERRORS, DCI_DT_UINT, DCIDESC_NET_INTERFACE_INERRORS },
   { "Net.Interface.Link(*)", H_NetInterfaceStats, (char *)NETINFO_IF_OPER_STATUS, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { "Net.Interface.MTU(*)", H_NetInterfaceStats, (char *)NETINFO_IF_MTU, DCI_DT_UINT, DCIDESC_NET_INTERFACE_MTU },
   { "Net.Interface.OperStatus(*)", H_NetInterfaceStats, (char *)NETINFO_IF_OPER_STATUS, DCI_DT_INT, DCIDESC_NET_INTERFACE_OPERSTATUS },
   { "Net.Interface.OutErrors(*)", H_NetInterfaceStats, (char *)NETINFO_IF_OUT_ERRORS, DCI_DT_UINT, DCIDESC_NET_INTERFACE_OUTERRORS },
   { "Net.Interface.PacketsIn(*)", H_NetInterfaceStats, (char *)NETINFO_IF_PACKETS_IN, DCI_DT_UINT, DCIDESC_NET_INTERFACE_PACKETSIN },
   { "Net.Interface.PacketsOut(*)", H_NetInterfaceStats, (char *)NETINFO_IF_PACKETS_OUT, DCI_DT_UINT, DCIDESC_NET_INTERFACE_PACKETSOUT },
   { "Net.Interface.Speed(*)", H_NetInterfaceStats, (char *)NETINFO_IF_SPEED, DCI_DT_UINT, DCIDESC_NET_INTERFACE_SPEED },
   { "Net.IP.Forwarding", H_NetIPStats, (char *)NETINFO_IP_FORWARDING, DCI_DT_INT, DCIDESC_NET_IP_FORWARDING },
   { "PhysicalDisk.Firmware(*)", H_PhysicalDiskInfo, "F", DCI_DT_STRING, DCIDESC_PHYSICALDISK_FIRMWARE },
   { "PhysicalDisk.Model(*)", H_PhysicalDiskInfo, "M", DCI_DT_STRING, DCIDESC_PHYSICALDISK_MODEL },
   { "PhysicalDisk.SerialNumber(*)", H_PhysicalDiskInfo, "N", DCI_DT_STRING, DCIDESC_PHYSICALDISK_SERIALNUMBER },
   { "PhysicalDisk.SmartAttr(*)", H_PhysicalDiskInfo, "A", DCI_DT_STRING, DCIDESC_PHYSICALDISK_SMARTATTR },
   { "PhysicalDisk.SmartStatus(*)", H_PhysicalDiskInfo, "S", DCI_DT_INT, DCIDESC_PHYSICALDISK_SMARTSTATUS },
   { "PhysicalDisk.Temperature(*)", H_PhysicalDiskInfo, "T", DCI_DT_INT, DCIDESC_PHYSICALDISK_TEMPERATURE },
   { "System.CPU.Count", H_CPUCount, NULL, DCI_DT_UINT, DCIDESC_SYSTEM_CPU_COUNT },
   { "System.Hostname", H_HostName, NULL, DCI_DT_STRING, DCIDESC_SYSTEM_HOSTNAME },
   { "System.Memory.Physical.Free", H_MemoryInfo, (char *)MEMINFO_PHYSICAL_FREE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE },
   { "System.Memory.Physical.FreePerc", H_MemoryInfo, (char *)MEMINFO_PHYSICAL_FREE_PCT, DCI_DT_UINT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE_PCT },
   { "System.Memory.Physical.Total", H_MemoryInfo, (char *)MEMINFO_PHYSICAL_TOTAL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_TOTAL },
   { "System.Memory.Physical.Used", H_MemoryInfo, (char *)MEMINFO_PHYSICAL_USED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED },
   { "System.Memory.Physical.UsedPerc", H_MemoryInfo, (char *)MEMINFO_PHYSICAL_USED_PCT, DCI_DT_UINT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED_PCT },
   { "System.Memory.Virtual.Free", H_MemoryInfo, (char *)MEMINFO_VIRTUAL_FREE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE },
   { "System.Memory.Virtual.FreePerc", H_MemoryInfo, (char *)MEMINFO_VIRTUAL_FREE_PCT, DCI_DT_UINT, DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE_PCT },
   { "System.Memory.Virtual.Total", H_MemoryInfo, (char *)MEMINFO_VIRTUAL_TOTAL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_TOTAL },
   { "System.Memory.Virtual.Used", H_MemoryInfo, (char *)MEMINFO_VIRTUAL_USED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED },
   { "System.Memory.Virtual.UsedPerc", H_MemoryInfo, (char *)MEMINFO_VIRTUAL_USED_PCT, DCI_DT_UINT, DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED_PCT },
   { "System.Uname", H_SystemUname, NULL, DCI_DT_STRING, DCIDESC_SYSTEM_UNAME },
#endif
   { "Agent.AcceptedConnections", H_UIntPtr, (char *)&g_dwAcceptedConnections, DCI_DT_UINT, DCIDESC_AGENT_ACCEPTEDCONNECTIONS },
   { "Agent.AcceptErrors", H_UIntPtr, (char *)&g_dwAcceptErrors, DCI_DT_UINT, DCIDESC_AGENT_ACCEPTERRORS },
   { "Agent.ActiveConnections", H_ActiveConnections, NULL, DCI_DT_UINT, DCIDESC_AGENT_ACTIVECONNECTIONS },
   { "Agent.AuthenticationFailures", H_UIntPtr, (char *)&m_dwAuthenticationFailures, DCI_DT_UINT, DCIDESC_AGENT_AUTHENTICATIONFAILURES },
   { "Agent.ConfigurationServer", H_StringConstant, g_szConfigServer, DCI_DT_STRING, DCIDESC_AGENT_CONFIG_SERVER },
   { "Agent.FailedRequests", H_UIntPtr, (char *)&m_dwFailedRequests, DCI_DT_UINT, DCIDESC_AGENT_FAILEDREQUESTS },
   { "Agent.ProcessedRequests", H_UIntPtr, (char *)&m_dwProcessedRequests, DCI_DT_UINT, DCIDESC_AGENT_PROCESSEDREQUESTS },
   { "Agent.Registrar", H_StringConstant, g_szRegistrar, DCI_DT_STRING, DCIDESC_AGENT_REGISTRAR },
   { "Agent.RejectedConnections", H_UIntPtr, (char *)&g_dwRejectedConnections, DCI_DT_UINT, DCIDESC_AGENT_REJECTEDCONNECTIONS },
   { "Agent.SourcePackageSupport", H_StringConstant, "0", DCI_DT_INT, DCIDESC_AGENT_SOURCEPACKAGESUPPORT },
   { "Agent.SupportedCiphers", H_SupportedCiphers, NULL, DCI_DT_STRING, DCIDESC_AGENT_SUPPORTEDCIPHERS },
   { "Agent.TimedOutRequests", H_UIntPtr, (char *)&m_dwTimedOutRequests, DCI_DT_UINT, DCIDESC_AGENT_TIMEDOUTREQUESTS },
   { "Agent.UnsupportedRequests", H_UIntPtr, (char *)&m_dwUnsupportedRequests, DCI_DT_UINT, DCIDESC_AGENT_UNSUPPORTEDREQUESTS },
   { "Agent.Uptime", H_AgentUptime, NULL, DCI_DT_UINT, DCIDESC_AGENT_UPTIME },
   { "Agent.Version", H_StringConstant, AGENT_VERSION_STRING, DCI_DT_STRING, DCIDESC_AGENT_VERSION },
   { "File.Count(*)", H_DirInfo, (char *)DIRINFO_FILE_COUNT, DCI_DT_UINT, DCIDESC_FILE_COUNT },
   { "File.Hash.CRC32(*)", H_CRC32, NULL, DCI_DT_UINT, DCIDESC_FILE_HASH_CRC32 },
   { "File.Hash.MD5(*)", H_MD5Hash, NULL, DCI_DT_STRING, DCIDESC_FILE_HASH_MD5 },
   { "File.Hash.SHA1(*)", H_SHA1Hash, NULL, DCI_DT_STRING, DCIDESC_FILE_HASH_SHA1 },
   { "File.Size(*)", H_DirInfo, (char *)DIRINFO_FILE_SIZE, DCI_DT_UINT64, DCIDESC_FILE_SIZE },
   { "File.Time.Access(*)", H_FileTime, (char *)FILETIME_ATIME, DCI_DT_UINT64, DCIDESC_FILE_TIME_ACCESS },
   { "File.Time.Change(*)", H_FileTime, (char *)FILETIME_CTIME, DCI_DT_UINT64, DCIDESC_FILE_TIME_CHANGE },
   { "File.Time.Modify(*)", H_FileTime, (char *)FILETIME_MTIME, DCI_DT_UINT64, DCIDESC_FILE_TIME_MODIFY },
   { "System.PlatformName", H_PlatformName, NULL, DCI_DT_STRING, DCIDESC_SYSTEM_PLATFORMNAME }
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
#endif
   { "Agent.ActionList", H_ActionList, NULL },
   { "Agent.SubAgentList", H_SubAgentList, NULL },
   { "Agent.SupportedEnums", H_EnumList, NULL },
   { "Agent.SupportedParameters", H_ParamList, NULL },
   { "Agent.SupportedPushParameters", H_PushParamList, NULL }
};


//
// Initialize dynamic parameters list from default static list
//

BOOL InitParameterList()
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
// Add push parameter to list
//

void AddPushParameter(const TCHAR *name, int dataType, const TCHAR *description)
{
   int i;

   // Search for existing parameter
   for(i = 0; i < m_iNumPushParams; i++)
      if (!stricmp(m_pPushParamList[i].name, name))
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
      nx_strncpy(m_pPushParamList[m_iNumPushParams].name, name, MAX_PARAM_NAME - 1);
      m_pPushParamList[m_iNumPushParams].dataType = dataType;
      nx_strncpy(m_pPushParamList[m_iNumPushParams].description, description, MAX_DB_STRING);
      m_iNumPushParams++;
   }
}


//
// Add parameter to list
//

void AddParameter(const char *pszName, LONG (* fpHandler)(const char *, const char *, char *), const char *pArg,
                  int iDataType, const char *pszDescription)
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
      nx_strncpy(m_pParamList[i].szDescription, pszDescription, MAX_DB_STRING);

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
      nx_strncpy(m_pParamList[m_iNumParams].szName, pszName, MAX_PARAM_NAME - 1);
      m_pParamList[m_iNumParams].fpHandler = fpHandler;
      m_pParamList[m_iNumParams].pArg = pArg;
      m_pParamList[m_iNumParams].iDataType = iDataType;
      nx_strncpy(m_pParamList[m_iNumParams].szDescription, pszDescription, MAX_DB_STRING);
      m_iNumParams++;
   }
}


//
// Add enum to list
//

void AddEnum(const char *pszName, LONG (* fpHandler)(const char *, const char *, StringList *), const char *pArg)
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
      nx_strncpy(m_pEnumList[m_iNumEnums].szName, pszName, MAX_PARAM_NAME - 1);
      m_pEnumList[m_iNumEnums].fpHandler = fpHandler;
      m_pEnumList[m_iNumEnums].pArg = pArg;
      m_iNumEnums++;
   }
}


//
// Add external parameter
//

BOOL AddExternalParameter(char *pszCfgLine, BOOL bShellExec)
{
   char *pszCmdLine, *pszArg;

   pszCmdLine = strchr(pszCfgLine, ':');
   if (pszCmdLine == NULL)
      return FALSE;

   *pszCmdLine = 0;
   pszCmdLine++;
   StrStrip(pszCfgLine);
   StrStrip(pszCmdLine);
   if ((*pszCfgLine == 0) || (*pszCmdLine == 0))
      return FALSE;

	pszArg = (char *)malloc(strlen(pszCmdLine) + 2);
	pszArg[0] = bShellExec ? 'S' : 'E';
	strcpy(&pszArg[1], pszCmdLine);
   AddParameter(pszCfgLine, H_ExternalParameter, pszArg, DCI_DT_STRING, "");
   return TRUE;
}


//
// Get parameter's value
//

DWORD GetParameterValue(DWORD dwSessionId, char *pszParam, char *pszValue)
{
   int i, rc;
   DWORD dwErrorCode;

   DebugPrintf(dwSessionId, 5, "Requesting parameter \"%s\"", pszParam);
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
               nxlog_write(MSG_UNEXPECTED_IRC, EVENTLOG_ERROR_TYPE, "ds", rc, pszParam);
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

DWORD GetEnumValue(DWORD dwSessionId, char *pszParam, StringList *pValue)
{
   int i, rc;
   DWORD dwErrorCode;

   DebugPrintf(dwSessionId, 5, "Requesting enum \"%s\"", pszParam);
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
               nxlog_write(MSG_UNEXPECTED_IRC, EVENTLOG_ERROR_TYPE, "ds", rc, pszParam);
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
   DWORD dwId, count;

	// Parameters
   for(i = 0, count = 0, dwId = VID_PARAM_LIST_BASE; i < m_iNumParams; i++)
   {
		if (m_pParamList[i].iDataType != DCI_DT_DEPRECATED)
		{
			pMsg->SetVariable(dwId++, m_pParamList[i].szName);
			pMsg->SetVariable(dwId++, m_pParamList[i].szDescription);
			pMsg->SetVariable(dwId++, (WORD)m_pParamList[i].iDataType);
			count++;
		}
   }
   pMsg->SetVariable(VID_NUM_PARAMETERS, count);

	// Push parameters
   pMsg->SetVariable(VID_NUM_PUSH_PARAMETERS, (DWORD)m_iNumPushParams);
   for(i = 0, dwId = VID_PUSHPARAM_LIST_BASE; i < m_iNumPushParams; i++)
   {
      pMsg->SetVariable(dwId++, m_pPushParamList[i].name);
      pMsg->SetVariable(dwId++, m_pPushParamList[i].description);
      pMsg->SetVariable(dwId++, (WORD)m_pPushParamList[i].dataType);
   }

	// Enums
   pMsg->SetVariable(VID_NUM_ENUMS, (DWORD)m_iNumEnums);
   for(i = 0, dwId = VID_ENUM_LIST_BASE; i < m_iNumEnums; i++)
   {
      pMsg->SetVariable(dwId++, m_pEnumList[i].szName);
   }
}
