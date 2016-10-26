/* 
** NetXMS subagent for IPSO
** Copyright (C) 2004 Alex Kirhenshtein
** Copyright (C) 2006-2011 Victor Kirhenshtein
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
**/

#include "ipso.h"


//
// Handler for System.IPSCTL(*)
//

static LONG H_IPSCTL(char *pszParam, char *pArg, char *pValue, AbstractCommSession *session)
{
	char szName[256];

	if (!AgentGetParameterArg(pszParam, 1, szName, 256))
		return SYSINFO_RC_UNSUPPORTED;

	return IPSCTLGetString(0, szName, pValue, MAX_RESULT_LENGTH);
}


//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
   { "Disk.Avail(*)",                H_DiskInfo,        (char *)DISK_AVAIL,
			DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
   { "Disk.AvailPerc(*)",            H_DiskInfo,        (char *)DISK_AVAIL_PERC,
			DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
   { "Disk.Free(*)",                 H_DiskInfo,        (char *)DISK_FREE,
			DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
   { "Disk.FreePerc(*)",             H_DiskInfo,        (char *)DISK_FREE_PERC,
			DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
   { "Disk.Total(*)",                H_DiskInfo,        (char *)DISK_TOTAL,
			DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
   { "Disk.Used(*)",                 H_DiskInfo,        (char *)DISK_USED,
			DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
   { "Disk.UsedPerc(*)",             H_DiskInfo,        (char *)DISK_USED_PERC,
			DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },

   { "FileSystem.Avail(*)",                H_DiskInfo,        (char *)DISK_AVAIL,
			DCI_DT_UINT64,	DCIDESC_FS_AVAIL },
   { "FileSystem.AvailPerc(*)",            H_DiskInfo,        (char *)DISK_AVAIL_PERC,
			DCI_DT_FLOAT,	DCIDESC_FS_AVAILPERC },
   { "FileSystem.Free(*)",                 H_DiskInfo,        (char *)DISK_FREE,
			DCI_DT_UINT64,	DCIDESC_FS_FREE },
   { "FileSystem.FreePerc(*)",             H_DiskInfo,        (char *)DISK_FREE_PERC,
			DCI_DT_FLOAT,	DCIDESC_FS_FREEPERC },
   { "FileSystem.Total(*)",                H_DiskInfo,        (char *)DISK_TOTAL,
			DCI_DT_UINT64,	DCIDESC_FS_TOTAL },
   { "FileSystem.Used(*)",                 H_DiskInfo,        (char *)DISK_USED,
			DCI_DT_UINT64,	DCIDESC_FS_USED },
   { "FileSystem.UsedPerc(*)",             H_DiskInfo,        (char *)DISK_USED_PERC,
			DCI_DT_FLOAT,	DCIDESC_FS_USEDPERC },

   { "Net.IP.Forwarding",            H_NetIpForwarding, (char *)4,
			DCI_DT_INT,		DCIDESC_NET_IP_FORWARDING },
   { "Net.IP6.Forwarding",           H_NetIpForwarding, (char *)6,
			DCI_DT_INT,		DCIDESC_NET_IP6_FORWARDING },
   { "Net.Interface.AdminStatus(*)", H_NetIfStats,      (char *)NET_IF_ADMIN_STATUS,
			DCI_DT_INT,		DCIDESC_NET_INTERFACE_ADMINSTATUS },
   { "Net.Interface.BytesIn(*)",     H_NetIfStats,      (char *)NET_IF_BYTES_IN,
			DCI_DT_UINT64,		DCIDESC_NET_INTERFACE_BYTESIN },
   { "Net.Interface.BytesOut(*)",    H_NetIfStats,      (char *)NET_IF_BYTES_OUT,
			DCI_DT_UINT64,		DCIDESC_NET_INTERFACE_BYTESOUT },
   { "Net.Interface.Description(*)", H_NetIfStats,      (char *)NET_IF_DESCRIPTION,
			DCI_DT_STRING,		DCIDESC_NET_INTERFACE_DESCRIPTION },
   { "Net.Interface.Link(*)",        H_NetIfStats,      (char *)NET_IF_LINK,
			DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
   { "Net.Interface.OperStatus(*)",  H_NetIfStats,      (char *)NET_IF_LINK,
			DCI_DT_INT,		DCIDESC_NET_INTERFACE_OPERSTATUS },
   { "Net.Interface.PacketsIn(*)",   H_NetIfStats,      (char *)NET_IF_PACKETS_IN,
			DCI_DT_UINT,		DCIDESC_NET_INTERFACE_PACKETSIN },
   { "Net.Interface.PacketsOut(*)",  H_NetIfStats,      (char *)NET_IF_PACKETS_OUT,
			DCI_DT_UINT,		DCIDESC_NET_INTERFACE_PACKETSOUT },

   { "Process.Count(*)",             H_ProcessCount,    (char *)0,
			DCI_DT_UINT,	DCIDESC_PROCESS_COUNT },
   { "System.ProcessCount",          H_ProcessCount,    (char *)1,
			DCI_DT_UINT,	DCIDESC_SYSTEM_PROCESSCOUNT },

   { "System.CPU.Count",             H_CpuCount,        NULL,
			DCI_DT_INT,		DCIDESC_SYSTEM_CPU_COUNT },
   { "System.CPU.LoadAvg",           H_CpuLoad,         NULL,
			DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_LOADAVG },
   { "System.CPU.LoadAvg5",          H_CpuLoad,         NULL,
			DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_LOADAVG5 },
   { "System.CPU.LoadAvg15",         H_CpuLoad,         NULL,
			DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_LOADAVG15 },
/*   { "System.CPU.Usage",             H_CpuUsage,        NULL,
			DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE },
   { "System.CPU.Usage5",            H_CpuUsage,        NULL,
			DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5 },
   { "System.CPU.Usage15",           H_CpuUsage,        NULL,
			DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15 }, */
   { "System.IPSCTL(*)",             H_IPSCTL,          NULL,
			DCI_DT_STRING,	"Value of given ipsctl parameter" },
   { "System.Hostname",              H_Hostname,        NULL,
			DCI_DT_STRING,	DCIDESC_SYSTEM_HOSTNAME },
   { "System.Memory.Physical.Free",  H_MemoryInfo,      (char *)PHYSICAL_FREE,
			DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE },
	{ "System.Memory.Physical.FreePerc", H_MemoryInfo,   (char *)PHYSICAL_FREE_PCT,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE_PCT },
   { "System.Memory.Physical.Total", H_MemoryInfo,      (char *)PHYSICAL_TOTAL,
			DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_PHYSICAL_TOTAL },
   { "System.Memory.Physical.Used",  H_MemoryInfo,      (char *)PHYSICAL_USED,
			DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED },
	{ "System.Memory.Physical.UsedPerc", H_MemoryInfo,   (char *)PHYSICAL_USED_PCT,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED_PCT },
   { "System.Memory.Swap.Free",      H_MemoryInfo,      (char *)SWAP_FREE,
			DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_SWAP_FREE },
	{ "System.Memory.Swap.FreePerc",  H_MemoryInfo,      (char *)SWAP_FREE_PCT,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_MEMORY_SWAP_FREE_PCT },
   { "System.Memory.Swap.Total",     H_MemoryInfo,      (char *)SWAP_TOTAL,
			DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_SWAP_TOTAL },
   { "System.Memory.Swap.Used",      H_MemoryInfo,      (char *)SWAP_USED,
			DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_SWAP_USED },
	{ "System.Memory.Swap.UsedPerc",      H_MemoryInfo,  (char *)SWAP_USED_PCT,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_MEMORY_SWAP_USED_PCT },
   { "System.Memory.Virtual.Free",   H_MemoryInfo,      (char *)VIRTUAL_FREE,
			DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE },
	{ "System.Memory.Virtual.FreePerc", H_MemoryInfo,    (char *)VIRTUAL_FREE_PCT,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE_PCT },
   { "System.Memory.Virtual.Total",  H_MemoryInfo,      (char *)VIRTUAL_TOTAL,
			DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_TOTAL },
   { "System.Memory.Virtual.Used",   H_MemoryInfo,      (char *)VIRTUAL_USED,
			DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED },
	{ "System.Memory.Virtual.UsedPerc", H_MemoryInfo,    (char *)VIRTUAL_USED_PCT,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED_PCT },
   { "System.Uname",                 H_Uname,           NULL,
			DCI_DT_STRING,	DCIDESC_SYSTEM_UNAME },
   { "System.Uptime",                H_Uptime,          NULL,
			DCI_DT_UINT,	DCIDESC_SYSTEM_UPTIME },
};

static NETXMS_SUBAGENT_LIST m_enums[] =
{
   { "Net.ArpCache",                 H_NetArpCache,     NULL },
   { "Net.InterfaceList",            H_NetIfList,       NULL },
   { "Net.IP.RoutingTable",          H_NetRoutingTable, NULL },
   { "System.ProcessList",           H_ProcessList,     NULL },
};

static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	"IPSO",
	NETXMS_VERSION_STRING,
	NULL,
	NULL,
	NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_LIST),
	m_enums,
	0, NULL,	// tables
   0, NULL,	// actions
	0, NULL	// push parameters
};

//
// Entry point for NetXMS agent
//

DECLARE_SUBAGENT_ENTRY_POINT(IPSO)
{
   *ppInfo = &m_info;
   return TRUE;
}


//
// Wrappers for ipsctl_get() function
//

LONG IPSCTLGetInt(int nCallerHandle, char *pszName, LONG *pnValue)
{
	int nHandle, nErr;
	struct ipsctl_value *pValue;
	LONG nRet = SYSINFO_RC_ERROR;

	nHandle = (nCallerHandle > 0) ? nCallerHandle : ipsctl_open();
	if (nHandle > 0)
	{
                nErr = ipsctl_get(nHandle, pszName, &pValue);
		if (nErr == 0)
		{
			if (pValue->wType == 3)
			{
				if (pValue->wSize == 8)
					*pnValue = (LONG)pValue->data.nInt64;
				else
					*pnValue = pValue->data.nInt;
				nRet = SYSINFO_RC_SUCCESS;
			}
		}
		else
		{
                        if (nErr == 2)   // No such parameter
                                nRet = SYSINFO_RC_UNSUPPORTED;
		}
		if (nHandle != nCallerHandle)
			ipsctl_close(nHandle);
	}
	return nRet;
}

LONG IPSCTLGetString(int nCallerHandle, char *pszName,
                     char *pszValue, int nSize)
{
	int nHandle, nErr;
	struct ipsctl_value *pData;
	LONG nRet = SYSINFO_RC_ERROR;

	nHandle = (nCallerHandle > 0) ? nCallerHandle : ipsctl_open();
	if (nHandle > 0)
	{
		nErr = ipsctl_get(nHandle, pszName, &pData);
		if (nErr == 0)
		{
			switch(pData->wType)
			{
				case 3:         // Integer (unsigned?)
					if (pData->wSize == 8)
						ret_uint64(pszValue, pData->data.qwUInt64);
					else
						ret_uint(pszValue, pData->data.dwUInt);
					break;
				case 4:         // String
					nx_strncpy(pszValue, pData->data.szStr, nSize);
					break;
				case 5:         // IP address
					IpToStr(ntohl(pData->data.ipAddr.dwAddr), pszValue);
					if (pData->data.ipAddr.nMaskBits != 32)
					{
						sprintf(&pszValue[strlen(pszValue)], "/%d",
						        pData->data.ipAddr.nMaskBits);
					}
					break;
				case 7:         // MAC address ?  
					MACToStr((BYTE *)pData->data.szStr, pszValue);
					break; 
				case 13:	// Kbits ?
					if (pData->wSize == 8)
						ret_uint64(pszValue, pData->data.qwUInt64 * 1000);
					else
						ret_uint(pszValue, pData->data.dwUInt * 1000);
					break;
				default:        // Unknown data type
					nx_strncpy(pszValue, "<unknown type>", nSize);
					break;
			}
			nRet = SYSINFO_RC_SUCCESS;
		}
		else
		{
			if (nErr == 2)   // No such parameter
				nRet = SYSINFO_RC_UNSUPPORTED;
		}
		if (nHandle != nCallerHandle)
			ipsctl_close(nHandle);
	}
	return nRet;
}
