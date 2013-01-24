/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2013 Victor Kirhenshtein
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

/**
 * Parameter handlers
 */
LONG H_ActiveConnections(const TCHAR *cmd, const TCHAR *arg, TCHAR *pValue);
LONG H_AgentStats(const TCHAR *cmd, const TCHAR *arg, TCHAR *value);
LONG H_AgentTraps(const TCHAR *cmd, const TCHAR *arg, TCHAR *value);
LONG H_AgentUptime(const TCHAR *cmd, const TCHAR *arg, TCHAR *value);
LONG H_CRC32(const TCHAR *cmd, const TCHAR *arg, TCHAR *value);
LONG H_DirInfo(const TCHAR *cmd, const TCHAR *arg, TCHAR *value);
LONG H_FileTime(const TCHAR *cmd, const TCHAR *arg, TCHAR *value);
LONG H_IsSubagentLoaded(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue);
LONG H_MD5Hash(const TCHAR *cmd, const TCHAR *arg, TCHAR *value);
LONG H_SHA1Hash(const TCHAR *cmd, const TCHAR *arg, TCHAR *value);
LONG H_SubAgentList(const TCHAR *cmd, const TCHAR *arg, StringList *value);
LONG H_ActionList(const TCHAR *cmd, const TCHAR *arg, StringList *value);
LONG H_ExternalParameter(const TCHAR *cmd, const TCHAR *arg, TCHAR *value);
LONG H_PlatformName(const TCHAR *cmd, const TCHAR *arg, TCHAR *value);
LONG H_SystemTime(const TCHAR *cmd, const TCHAR *arg, TCHAR *value);

#ifdef _WIN32
LONG H_DiskInfo(const TCHAR *cmd, const TCHAR *arg, TCHAR *value);
LONG H_MemoryInfo(const TCHAR *cmd, const TCHAR *arg, TCHAR *value);
LONG H_HostName(const TCHAR *cmd, const TCHAR *arg, TCHAR *value);
LONG H_SystemUname(const TCHAR *cmd, const TCHAR *arg, TCHAR *value);
LONG H_CPUCount(const TCHAR *cmd, const TCHAR *arg, TCHAR *value);
LONG H_PhysicalDiskInfo(const TCHAR *cmd, const TCHAR *arg, TCHAR *pValue);
LONG H_FileSystems(const TCHAR *cmd, const TCHAR *arg, Table *value);
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
static DWORD m_dwTimedOutRequests = 0;
static DWORD m_dwAuthenticationFailures = 0;
static DWORD m_dwProcessedRequests = 0;
static DWORD m_dwFailedRequests = 0;
static DWORD m_dwUnsupportedRequests = 0;

/**
 * Handler for parameters which always returns string constant
 */
static LONG H_StringConstant(const TCHAR *cmd, const TCHAR *arg, TCHAR *value)
{
   ret_string(value, arg);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for parameters which returns DWORD value from specific variable
 */
static LONG H_UIntPtr(const TCHAR *cmd, const TCHAR *arg, TCHAR *value)
{
   ret_uint(value, *((DWORD *)arg));
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Agent.SupportedCiphers
 */ 
static LONG H_SupportedCiphers(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue)
{
   DWORD dwCiphers;

   dwCiphers = CSCPGetSupportedCiphers();
   if (dwCiphers == 0)
   {
      ret_string(pValue, _T("NONE"));
   }
   else
   {
      *pValue = 0;
      if (dwCiphers & CSCP_SUPPORT_AES_256)
         _tcscat(pValue, _T("AES-256 "));
      if (dwCiphers & CSCP_SUPPORT_AES_128)
         _tcscat(pValue, _T("AES-128 "));
      if (dwCiphers & CSCP_SUPPORT_BLOWFISH)
         _tcscat(pValue, _T("BLOWFISH "));
      if (dwCiphers & CSCP_SUPPORT_IDEA)
         _tcscat(pValue, _T("IDEA "));
      if (dwCiphers & CSCP_SUPPORT_3DES)
         _tcscat(pValue, _T("3DES "));
      pValue[_tcslen(pValue) - 1] = 0;
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for parameters list
 */
static LONG H_ParamList(const TCHAR *cmd, const TCHAR *arg, StringList *value)
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
static LONG H_PushParamList(const TCHAR *cmd, const TCHAR *arg, StringList *value)
{
   int i;

   for(i = 0; i < m_iNumPushParams; i++)
		value->add(m_pPushParamList[i].name);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for enums list
 */
static LONG H_EnumList(const TCHAR *cmd, const TCHAR *arg, StringList *value)
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
static LONG H_TableList(const TCHAR *cmd, const TCHAR *arg, StringList *value)
{
   int i;

   for(i = 0; i < m_iNumTables; i++)
		value->add(m_pTableList[i].name);
	ListTablesFromExtSubagents(value);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Standard agent's parameters
 */
static NETXMS_SUBAGENT_PARAM m_stdParams[] =
{
#ifdef _WIN32
   { _T("Disk.Free(*)"), H_DiskInfo, (TCHAR *)DISKINFO_FREE_BYTES, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.FreePerc(*)"), H_DiskInfo, (TCHAR *)DISKINFO_FREE_SPACE_PCT, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.Total(*)"), H_DiskInfo, (TCHAR *)DISKINFO_TOTAL_BYTES, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.Used(*)"), H_DiskInfo, (TCHAR *)DISKINFO_USED_BYTES, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.UsedPerc(*)"), H_DiskInfo, (TCHAR *)DISKINFO_USED_SPACE_PCT, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("FileSystem.Free(*)"), H_DiskInfo, (TCHAR *)DISKINFO_FREE_BYTES, DCI_DT_UINT64, DCIDESC_FS_FREE },
   { _T("FileSystem.FreePerc(*)"), H_DiskInfo, (TCHAR *)DISKINFO_FREE_SPACE_PCT, DCI_DT_FLOAT, DCIDESC_FS_FREEPERC },
   { _T("FileSystem.Total(*)"), H_DiskInfo, (TCHAR *)DISKINFO_TOTAL_BYTES, DCI_DT_UINT64, DCIDESC_FS_TOTAL },
   { _T("FileSystem.Used(*)"), H_DiskInfo, (TCHAR *)DISKINFO_USED_BYTES, DCI_DT_UINT64, DCIDESC_FS_USED },
   { _T("FileSystem.UsedPerc(*)"), H_DiskInfo, (TCHAR *)DISKINFO_USED_SPACE_PCT, DCI_DT_FLOAT, DCIDESC_FS_USEDPERC },
   { _T("PhysicalDisk.Firmware(*)"), H_PhysicalDiskInfo, _T("F"), DCI_DT_STRING, DCIDESC_PHYSICALDISK_FIRMWARE },
   { _T("PhysicalDisk.Model(*)"), H_PhysicalDiskInfo, _T("M"), DCI_DT_STRING, DCIDESC_PHYSICALDISK_MODEL },
   { _T("PhysicalDisk.SerialNumber(*)"), H_PhysicalDiskInfo, _T("N"), DCI_DT_STRING, DCIDESC_PHYSICALDISK_SERIALNUMBER },
   { _T("PhysicalDisk.SmartAttr(*)"), H_PhysicalDiskInfo, _T("A"), DCI_DT_STRING, DCIDESC_PHYSICALDISK_SMARTATTR },
   { _T("PhysicalDisk.SmartStatus(*)"), H_PhysicalDiskInfo, _T("S"), DCI_DT_INT, DCIDESC_PHYSICALDISK_SMARTSTATUS },
   { _T("PhysicalDisk.Temperature(*)"), H_PhysicalDiskInfo, _T("T"), DCI_DT_INT, DCIDESC_PHYSICALDISK_TEMPERATURE },
   { _T("System.CPU.Count"), H_CPUCount, NULL, DCI_DT_UINT, DCIDESC_SYSTEM_CPU_COUNT },
   { _T("System.Hostname"), H_HostName, NULL, DCI_DT_STRING, DCIDESC_SYSTEM_HOSTNAME },
   { _T("System.Memory.Physical.Free"), H_MemoryInfo, (TCHAR *)MEMINFO_PHYSICAL_FREE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE },
   { _T("System.Memory.Physical.FreePerc"), H_MemoryInfo, (TCHAR *)MEMINFO_PHYSICAL_FREE_PCT, DCI_DT_UINT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE_PCT },
   { _T("System.Memory.Physical.Total"), H_MemoryInfo, (TCHAR *)MEMINFO_PHYSICAL_TOTAL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_TOTAL },
   { _T("System.Memory.Physical.Used"), H_MemoryInfo, (TCHAR *)MEMINFO_PHYSICAL_USED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED },
   { _T("System.Memory.Physical.UsedPerc"), H_MemoryInfo, (TCHAR *)MEMINFO_PHYSICAL_USED_PCT, DCI_DT_UINT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED_PCT },
   { _T("System.Memory.Virtual.Free"), H_MemoryInfo, (TCHAR *)MEMINFO_VIRTUAL_FREE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE },
   { _T("System.Memory.Virtual.FreePerc"), H_MemoryInfo, (TCHAR *)MEMINFO_VIRTUAL_FREE_PCT, DCI_DT_UINT, DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE_PCT },
   { _T("System.Memory.Virtual.Total"), H_MemoryInfo, (TCHAR *)MEMINFO_VIRTUAL_TOTAL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_TOTAL },
   { _T("System.Memory.Virtual.Used"), H_MemoryInfo, (TCHAR *)MEMINFO_VIRTUAL_USED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED },
   { _T("System.Memory.Virtual.UsedPerc"), H_MemoryInfo, (TCHAR *)MEMINFO_VIRTUAL_USED_PCT, DCI_DT_UINT, DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED_PCT },
   { _T("System.Uname"), H_SystemUname, NULL, DCI_DT_STRING, DCIDESC_SYSTEM_UNAME },
#endif
   { _T("Agent.AcceptedConnections"), H_UIntPtr, (TCHAR *)&g_dwAcceptedConnections, DCI_DT_UINT, DCIDESC_AGENT_ACCEPTEDCONNECTIONS },
   { _T("Agent.AcceptErrors"), H_UIntPtr, (TCHAR *)&g_dwAcceptErrors, DCI_DT_UINT, DCIDESC_AGENT_ACCEPTERRORS },
   { _T("Agent.ActiveConnections"), H_ActiveConnections, NULL, DCI_DT_UINT, DCIDESC_AGENT_ACTIVECONNECTIONS },
   { _T("Agent.AuthenticationFailures"), H_UIntPtr, (TCHAR *)&m_dwAuthenticationFailures, DCI_DT_UINT, DCIDESC_AGENT_AUTHENTICATIONFAILURES },
   { _T("Agent.ConfigurationServer"), H_StringConstant, g_szConfigServer, DCI_DT_STRING, DCIDESC_AGENT_CONFIG_SERVER },
   { _T("Agent.FailedRequests"), H_UIntPtr, (TCHAR *)&m_dwFailedRequests, DCI_DT_UINT, DCIDESC_AGENT_FAILEDREQUESTS },
   { _T("Agent.GeneratedTraps"), H_AgentTraps, _T("G"), DCI_DT_UINT64, DCIDESC_AGENT_GENERATED_TRAPS },
   { _T("Agent.IsSubagentLoaded(*)"), H_IsSubagentLoaded, NULL, DCI_DT_INT, DCIDESC_AGENT_IS_SUBAGENT_LOADED },
   { _T("Agent.LastTrapTime"), H_AgentTraps, _T("T"), DCI_DT_UINT64, DCIDESC_AGENT_LAST_TRAP_TIME },
   { _T("Agent.ProcessedRequests"), H_UIntPtr, (TCHAR *)&m_dwProcessedRequests, DCI_DT_UINT, DCIDESC_AGENT_PROCESSEDREQUESTS },
   { _T("Agent.Registrar"), H_StringConstant, g_szRegistrar, DCI_DT_STRING, DCIDESC_AGENT_REGISTRAR },
   { _T("Agent.RejectedConnections"), H_UIntPtr, (TCHAR *)&g_dwRejectedConnections, DCI_DT_UINT, DCIDESC_AGENT_REJECTEDCONNECTIONS },
   { _T("Agent.SentTraps"), H_AgentTraps, _T("S"), DCI_DT_UINT64, DCIDESC_AGENT_SENT_TRAPS },
   { _T("Agent.SourcePackageSupport"), H_StringConstant, _T("0"), DCI_DT_INT, DCIDESC_AGENT_SOURCEPACKAGESUPPORT },
   { _T("Agent.SupportedCiphers"), H_SupportedCiphers, NULL, DCI_DT_STRING, DCIDESC_AGENT_SUPPORTEDCIPHERS },
   { _T("Agent.TimedOutRequests"), H_UIntPtr, (TCHAR *)&m_dwTimedOutRequests, DCI_DT_UINT, DCIDESC_AGENT_TIMEDOUTREQUESTS },
   { _T("Agent.UnsupportedRequests"), H_UIntPtr, (TCHAR *)&m_dwUnsupportedRequests, DCI_DT_UINT, DCIDESC_AGENT_UNSUPPORTEDREQUESTS },
   { _T("Agent.Uptime"), H_AgentUptime, NULL, DCI_DT_UINT, DCIDESC_AGENT_UPTIME },
   { _T("Agent.Version"), H_StringConstant, AGENT_VERSION_STRING, DCI_DT_STRING, DCIDESC_AGENT_VERSION },
   { _T("File.Count(*)"), H_DirInfo, (TCHAR *)DIRINFO_FILE_COUNT, DCI_DT_UINT, DCIDESC_FILE_COUNT },
   { _T("File.Hash.CRC32(*)"), H_CRC32, NULL, DCI_DT_UINT, DCIDESC_FILE_HASH_CRC32 },
   { _T("File.Hash.MD5(*)"), H_MD5Hash, NULL, DCI_DT_STRING, DCIDESC_FILE_HASH_MD5 },
   { _T("File.Hash.SHA1(*)"), H_SHA1Hash, NULL, DCI_DT_STRING, DCIDESC_FILE_HASH_SHA1 },
   { _T("File.Size(*)"), H_DirInfo, (TCHAR *)DIRINFO_FILE_SIZE, DCI_DT_UINT64, DCIDESC_FILE_SIZE },
   { _T("File.Time.Access(*)"), H_FileTime, (TCHAR *)FILETIME_ATIME, DCI_DT_UINT64, DCIDESC_FILE_TIME_ACCESS },
   { _T("File.Time.Change(*)"), H_FileTime, (TCHAR *)FILETIME_CTIME, DCI_DT_UINT64, DCIDESC_FILE_TIME_CHANGE },
   { _T("File.Time.Modify(*)"), H_FileTime, (TCHAR *)FILETIME_MTIME, DCI_DT_UINT64, DCIDESC_FILE_TIME_MODIFY },
   { _T("System.CurrentTime"), H_SystemTime, NULL, DCI_DT_INT64, DCIDESC_SYSTEM_CURRENTTIME },
   { _T("System.PlatformName"), H_PlatformName, NULL, DCI_DT_STRING, DCIDESC_SYSTEM_PLATFORMNAME }
};

/**
 * Standard agent's lists
 */
static NETXMS_SUBAGENT_LIST m_stdLists[] =
{
   { _T("Agent.ActionList"), H_ActionList, NULL },
   { _T("Agent.SubAgentList"), H_SubAgentList, NULL },
   { _T("Agent.SupportedLists"), H_EnumList, NULL },
   { _T("Agent.SupportedParameters"), H_ParamList, NULL },
   { _T("Agent.SupportedPushParameters"), H_PushParamList, NULL },
   { _T("Agent.SupportedTables"), H_TableList, NULL }
};

/**
 * Standard agent's tables
 */
static NETXMS_SUBAGENT_TABLE m_stdTables[] =
{
#ifdef _WIN32
   { _T("FileSystem.Volumes"), H_FileSystems, NULL, _T("VOLUME"), DCTDESC_FILESYSTEM_VOLUMES }
#endif
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


//
// Add push parameter to list
// by LWX

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
      nx_strncpy(m_pPushParamList[m_iNumPushParams].name, name, MAX_PARAM_NAME - 1); 
      m_pPushParamList[m_iNumPushParams].dataType = dataType;
      nx_strncpy(m_pPushParamList[m_iNumPushParams].description, description, MAX_DB_STRING); 
      m_iNumPushParams++;
   }
}


//
// Add parameter to list
// 

void AddParameter(const TCHAR *pszName, LONG (* fpHandler)(const TCHAR *, const TCHAR *, TCHAR *), const TCHAR *pArg,
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


//
// Add list
// 

void AddList(const TCHAR *name, LONG (* handler)(const TCHAR *, const TCHAR *, StringList *), const TCHAR *arg)
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
      nx_strncpy(m_pEnumList[m_iNumEnums].name, name, MAX_PARAM_NAME - 1);
      m_pEnumList[m_iNumEnums].handler = handler;
      m_pEnumList[m_iNumEnums].arg = arg;
      m_iNumEnums++;
   }
}


//
// Add table
// 

void AddTable(const TCHAR *name, LONG (* handler)(const TCHAR *, const TCHAR *, Table *), const TCHAR *arg)
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
   }
   else
   {
      // Add new enum
      m_pTableList = (NETXMS_SUBAGENT_TABLE *)realloc(m_pTableList, sizeof(NETXMS_SUBAGENT_TABLE) * (m_iNumTables + 1));
      nx_strncpy(m_pTableList[m_iNumTables].name, name, MAX_PARAM_NAME - 1);
      m_pTableList[m_iNumTables].handler = handler;
      m_pTableList[m_iNumTables].arg = arg;
      m_iNumTables++;
   }
}


//
// Add external parameter
//  

BOOL AddExternalParameter(TCHAR *pszCfgLine, BOOL bShellExec) //to be TCHAR
{
   TCHAR *pszCmdLine, *pszArg;

   pszCmdLine = _tcschr(pszCfgLine, _T(':'));
   if (pszCmdLine == NULL)
      return FALSE;

   *pszCmdLine = 0;
   pszCmdLine++;
   StrStrip(pszCfgLine);
   StrStrip(pszCmdLine);
   if ((*pszCfgLine == 0) || (*pszCmdLine == 0))
      return FALSE;

	pszArg = (TCHAR *)malloc((_tcslen(pszCmdLine) + 2) * sizeof(TCHAR));
	pszArg[0] = bShellExec ? _T('S') : _T('E');
	_tcscpy(&pszArg[1], pszCmdLine);
   AddParameter(pszCfgLine, H_ExternalParameter, pszArg, DCI_DT_STRING, _T(""));
   return TRUE;
}

/**
 * Get parameter's value
 */
DWORD GetParameterValue(DWORD dwSessionId, TCHAR *pszParam, TCHAR *pszValue)
{
   int i, rc;
   DWORD dwErrorCode;

   DebugPrintf(dwSessionId, 5, _T("Requesting parameter \"%s\""), pszParam);
   for(i = 0; i < m_iNumParams; i++)
	{
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
               nxlog_write(MSG_UNEXPECTED_IRC, EVENTLOG_ERROR_TYPE, "ds", rc, pszParam);
               dwErrorCode = ERR_INTERNAL_ERROR;
               m_dwFailedRequests++;
               break;
         }
         break;
      }
	}

   if (i == m_iNumParams)
   {
		rc = GetParameterValueFromExtProvider(pszParam, pszValue);
		if (rc == SYSINFO_RC_SUCCESS)
		{
         dwErrorCode = ERR_SUCCESS;
         m_dwProcessedRequests++;
		}
		else
		{
			dwErrorCode = ERR_UNKNOWN_PARAMETER;
			m_dwUnsupportedRequests++;
		}
   }

   if ((dwErrorCode == ERR_UNKNOWN_PARAMETER) && (i == m_iNumParams))
   {
		dwErrorCode = GetParameterValueFromExtSubagent(pszParam, pszValue);
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

	DebugPrintf(dwSessionId, 7, _T("GetParameterValue(): result is %d (%s)"), (int)dwErrorCode,
		dwErrorCode == ERR_SUCCESS ? _T("SUCCESS") : (dwErrorCode == ERR_UNKNOWN_PARAMETER ? _T("UNKNOWN_PARAMETER") : _T("INTERNAL_ERROR")));
   return dwErrorCode;
}

/**
 * Get list's value
 */
DWORD GetListValue(DWORD dwSessionId, TCHAR *pszParam, StringList *pValue)
{
   int i, rc;
   DWORD dwErrorCode;

   DebugPrintf(dwSessionId, 5, _T("Requesting list \"%s\""), pszParam);
   for(i = 0; i < m_iNumEnums; i++)
	{
      if (MatchString(m_pEnumList[i].name, pszParam, FALSE))
      {
         rc = m_pEnumList[i].handler(pszParam, m_pEnumList[i].arg, pValue);
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
	}

	if (i == m_iNumEnums)
   {
		dwErrorCode = GetListValueFromExtSubagent(pszParam, pValue);
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

	DebugPrintf(dwSessionId, 7, _T("GetListValue(): result is %d (%s)"), (int)dwErrorCode,
		dwErrorCode == ERR_SUCCESS ? _T("SUCCESS") : (dwErrorCode == ERR_UNKNOWN_PARAMETER ? _T("UNKNOWN_PARAMETER") : _T("INTERNAL_ERROR")));
   return dwErrorCode;
}

/**
 * Get table's value
 */
DWORD GetTableValue(DWORD dwSessionId, TCHAR *pszParam, Table *pValue)
{
   int i, rc;
   DWORD dwErrorCode;

   DebugPrintf(dwSessionId, 5, _T("Requesting table \"%s\""), pszParam);
   for(i = 0; i < m_iNumTables; i++)
	{
      if (MatchString(m_pTableList[i].name, pszParam, FALSE))
      {
         rc = m_pTableList[i].handler(pszParam, m_pTableList[i].arg, pValue);
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
	}

	if (i == m_iNumTables)
   {
		dwErrorCode = GetTableValueFromExtSubagent(pszParam, pValue);
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

	DebugPrintf(dwSessionId, 7, _T("GetTableValue(): result is %d (%s)"), (int)dwErrorCode,
		dwErrorCode == ERR_SUCCESS ? _T("SUCCESS") : (dwErrorCode == ERR_UNKNOWN_PARAMETER ? _T("UNKNOWN_PARAMETER") : _T("INTERNAL_ERROR")));
   return dwErrorCode;
}

/**
 * Put complete list of supported parameters into NXCP message
 */
void GetParameterList(CSCPMessage *pMsg)
{
   int i;
   DWORD dwId, count;

	// Parameters
   for(i = 0, count = 0, dwId = VID_PARAM_LIST_BASE; i < m_iNumParams; i++)
   {
		if (m_pParamList[i].dataType != DCI_DT_DEPRECATED)
		{
			pMsg->SetVariable(dwId++, m_pParamList[i].name);
			pMsg->SetVariable(dwId++, m_pParamList[i].description);
			pMsg->SetVariable(dwId++, (WORD)m_pParamList[i].dataType);
			count++;
		}
   }
	ListParametersFromExtProviders(pMsg, &dwId, &count);
	ListParametersFromExtSubagents(pMsg, &dwId, &count);
   pMsg->SetVariable(VID_NUM_PARAMETERS, count);

	// Push parameters
   pMsg->SetVariable(VID_NUM_PUSH_PARAMETERS, (DWORD)m_iNumPushParams);
   for(i = 0, dwId = VID_PUSHPARAM_LIST_BASE; i < m_iNumPushParams; i++)
   {
      pMsg->SetVariable(dwId++, m_pPushParamList[i].name);
      pMsg->SetVariable(dwId++, m_pPushParamList[i].description);
      pMsg->SetVariable(dwId++, (WORD)m_pPushParamList[i].dataType);
   }

	// Lists
   pMsg->SetVariable(VID_NUM_ENUMS, (DWORD)m_iNumEnums);
   for(i = 0, dwId = VID_ENUM_LIST_BASE; i < m_iNumEnums; i++)
   {
      pMsg->SetVariable(dwId++, m_pEnumList[i].name);
   }
	ListListsFromExtSubagents(pMsg, &dwId, &count);

	// Tables
   pMsg->SetVariable(VID_NUM_TABLES, (DWORD)m_iNumTables);
   for(i = 0, dwId = VID_TABLE_LIST_BASE; i < m_iNumTables; i++)
   {
      pMsg->SetVariable(dwId++, m_pTableList[i].name);
		pMsg->SetVariable(dwId++, m_pTableList[i].instanceColumn);
		pMsg->SetVariable(dwId++, m_pTableList[i].description);
   }
	ListTablesFromExtSubagents(pMsg, &dwId, &count);
}

/**
 * Put list of supported tables into NXCP message
 */
void GetTableList(CSCPMessage *pMsg)
{
   int i;
   DWORD dwId, count;

   pMsg->SetVariable(VID_NUM_TABLES, (DWORD)m_iNumTables);
   for(i = 0, dwId = VID_TABLE_LIST_BASE; i < m_iNumTables; i++)
   {
      pMsg->SetVariable(dwId++, m_pTableList[i].name);
		pMsg->SetVariable(dwId++, m_pTableList[i].instanceColumn);
		pMsg->SetVariable(dwId++, m_pTableList[i].description);
   }
	ListTablesFromExtSubagents(pMsg, &dwId, &count);
}

/**
 * Put list of supported lists (enums) into NXCP message
 */
void GetEnumList(CSCPMessage *pMsg)
{
   int i;
   DWORD dwId, count;

   pMsg->SetVariable(VID_NUM_ENUMS, (DWORD)m_iNumEnums);
   for(i = 0, dwId = VID_ENUM_LIST_BASE; i < m_iNumEnums; i++)
   {
      pMsg->SetVariable(dwId++, m_pEnumList[i].name);
   }
	ListListsFromExtSubagents(pMsg, &dwId, &count);
}
