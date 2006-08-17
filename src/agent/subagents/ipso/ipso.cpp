/* $Id: ipso.cpp,v 1.6 2006-08-17 19:22:57 victor Exp $ */

/* 
** NetXMS subagent for IPSO
** Copyright (C) 2004 Alex Kirhenshtein
** Copyright (C) 2006 Victor Kirhenshtein
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

static LONG H_IPSCTL(char *pszParam, char *pArg, char *pValue)
{
	char szName[256];

	if (!NxGetParameterArg(pszParam, 1, szName, 256))
		return SYSINFO_RC_UNSUPPORTED;

	return IPSCTLGetString(szName, pValue, MAX_RESULT_LENGTH);
}


//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
   { "Disk.Avail(*)",                H_DiskInfo,        (char *)DISK_AVAIL,
			DCI_DT_UINT64,	"Available disk space on {instance}" },
   { "Disk.AvailPerc(*)",            H_DiskInfo,        (char *)DISK_AVAIL_PERC,
			DCI_DT_FLOAT,	"Percentage of Available disk space on {instance}" },
   { "Disk.Free(*)",                 H_DiskInfo,        (char *)DISK_FREE,
			DCI_DT_UINT64,	"Free disk space on {instance}" },
   { "Disk.FreePerc(*)",             H_DiskInfo,        (char *)DISK_FREE_PERC,
			DCI_DT_FLOAT,	"Percentage of free disk space on {instance}" },
   { "Disk.Total(*)",                H_DiskInfo,        (char *)DISK_TOTAL,
			DCI_DT_UINT64,	"Total disk space on {instance}" },
   { "Disk.Used(*)",                 H_DiskInfo,        (char *)DISK_USED,
			DCI_DT_UINT64,	"Used disk space on {instance}" },
   { "Disk.UsedPerc(*)",             H_DiskInfo,        (char *)DISK_USED_PERC,
			DCI_DT_FLOAT,	"Percentage of used disk space on {instance}" },

   { "Net.IP.Forwarding",            H_NetIpForwarding, (char *)4,
			DCI_DT_INT,		"IP forwarding status" },
   { "Net.IP6.Forwarding",           H_NetIpForwarding, (char *)6,
			DCI_DT_INT,		"IPv6 forwarding status" },
   { "Net.Interface.AdminStatus(*)", H_NetIfStats,      (char *)NET_IF_ADMIN_STATUS,
			DCI_DT_INT,		"Administrative status of interface {instance}" },
   { "Net.Interface.BytesIn(*)",     H_NetIfStats,      (char *)NET_IF_BYTES_IN,
			DCI_DT_UINT64,		"Number of input bytes on interface {instance}" },
   { "Net.Interface.BytesOut(*)",    H_NetIfStats,      (char *)NET_IF_BYTES_OUT,
			DCI_DT_UINT64,		"Number of output bytes on interface {instance}" },
   { "Net.Interface.Description(*)", H_NetIfStats,      (char *)NET_IF_DESCRIPTION,
			DCI_DT_STRING,		"Description of interface {instance}" },
   { "Net.Interface.Link(*)",        H_NetIfStats,      (char *)NET_IF_LINK,
			DCI_DT_INT,		"Physical link status of interface {instance}" },
   { "Net.Interface.PacketsIn(*)",   H_NetIfStats,      (char *)NET_IF_PACKETS_IN,
			DCI_DT_UINT,		"Number of input packets on interface {instance}" },
   { "Net.Interface.PacketsOut(*)",  H_NetIfStats,      (char *)NET_IF_PACKETS_OUT,
			DCI_DT_UINT,		"Number of output packets on interface {instance}" },

   { "Process.Count(*)",             H_ProcessCount,    (char *)0,
			DCI_DT_UINT,	"Number of {instance} processes" },
   { "System.ProcessCount",          H_ProcessCount,    (char *)1,
			DCI_DT_UINT,	"Total number of processes" },

   { "System.CPU.Count",             H_CpuCount,        NULL,
			DCI_DT_INT,		"Number of CPU in the system" },
   { "System.CPU.LoadAvg",           H_CpuLoad,         NULL,
			DCI_DT_FLOAT,	"Average CPU load for last minute" },
   { "System.CPU.LoadAvg5",          H_CpuLoad,         NULL,
			DCI_DT_FLOAT,	"Average CPU load for last 5 minutes" },
   { "System.CPU.LoadAvg15",         H_CpuLoad,         NULL,
			DCI_DT_FLOAT,	"Average CPU load for last 15 minutes" },
/*   { "System.CPU.Usage",             H_CpuUsage,        NULL,
			DCI_DT_FLOAT,	"" },
   { "System.CPU.Usage5",            H_CpuUsage,        NULL,
			DCI_DT_FLOAT,	"" },
   { "System.CPU.Usage15",           H_CpuUsage,        NULL,
			DCI_DT_FLOAT,	"" }, */
   { "System.IPSCTL(*)",             H_IPSCTL,          NULL,
			DCI_DT_STRING,	"Value of given ipsctl parameter" },
   { "System.Hostname",              H_Hostname,        NULL,
			DCI_DT_STRING,	"Host name" },
   { "System.Memory.Physical.Free",  H_MemoryInfo,      (char *)PHYSICAL_FREE,
			DCI_DT_UINT64,	"Free physical memory" },
   { "System.Memory.Physical.Total", H_MemoryInfo,      (char *)PHYSICAL_TOTAL,
			DCI_DT_UINT64,	"Total amount of physical memory" },
   { "System.Memory.Physical.Used",  H_MemoryInfo,      (char *)PHYSICAL_USED,
			DCI_DT_UINT64,	"Used physical memory" },
   { "System.Memory.Swap.Free",      H_MemoryInfo,      (char *)SWAP_FREE,
			DCI_DT_UINT64,	"Free swap space" },
   { "System.Memory.Swap.Total",     H_MemoryInfo,      (char *)SWAP_TOTAL,
			DCI_DT_UINT64,	"Total amount of swap space" },
   { "System.Memory.Swap.Used",      H_MemoryInfo,      (char *)SWAP_USED,
			DCI_DT_UINT64,	"Used swap space" },
   { "System.Memory.Virtual.Free",   H_MemoryInfo,      (char *)VIRTUAL_FREE,
			DCI_DT_UINT64,	"Free virtual memory" },
   { "System.Memory.Virtual.Total",  H_MemoryInfo,      (char *)VIRTUAL_TOTAL,
			DCI_DT_UINT64,	"Total amount of virtual memory" },
   { "System.Memory.Virtual.Used",   H_MemoryInfo,      (char *)VIRTUAL_USED,
			DCI_DT_UINT64,	"Used virtual memory" },
   { "System.Uname",                 H_Uname,           NULL,
			DCI_DT_STRING,	"System uname" },
   { "System.Uptime",                H_Uptime,          NULL,
			DCI_DT_UINT,	"System uptime" },
};

static NETXMS_SUBAGENT_ENUM m_enums[] =
{
   { "Net.ArpCache",                 H_NetArpCache,     NULL },
   { "Net.InterfaceList",            H_NetIfList,       NULL },
   { "Net.IP.RoutingTable",          H_NetRoutingTable, NULL },
   { "System.ProcessList",           H_ProcessList,     NULL },
};

static NETXMS_SUBAGENT_INFO m_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
	"FreeBSD",
	NETXMS_VERSION_STRING,
	NULL, NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_ENUM),
	m_enums
};

//
// Entry point for NetXMS agent
//

DECLARE_SUBAGENT_INIT(IPSO)
{
   *ppInfo = &m_info;
   return TRUE;
}


//
// Wrappers for ipsctl_get() function
//

LONG IPSCTLGetInt(char *pszName, LONG *pnValue)
{
	int nHandle, nErr;
	struct ipsctl_value *pValue;
	LONG nRet = SYSINFO_RC_ERROR;

	nHandle = ipsctl_open();
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
		ipsctl_close(nHandle);
	}
	return nRet;
}

LONG IPSCTLGetString(char *pszName, char *pszValue, int nSize)
{
	int nHandle, nErr;
	struct ipsctl_value *pData;
	LONG nRet = SYSINFO_RC_ERROR;

	nHandle = ipsctl_open();
	if (nHandle > 0)
	{
		nErr = ipsctl_get(nHandle, pszName, &pData);
		if (nErr == 0)
		{
			switch(pData->wType)
			{
				case 3:         // Integer
					if (pData->wSize == 8)
						ret_int64(pszValue, pData->data.nInt64);
					else
						ret_int(pszValue, pData->data.nInt);
					break;
				case 4:         // String
					nx_strncpy(pszValue, pData->data.szStr, nSize);
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
		ipsctl_close(nHandle);
	}
	return nRet;
}


///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.5  2006/08/17 07:38:52  victor
Improved handling of data returned by ipsctl_get()

Revision 1.4  2006/08/16 22:26:09  victor
- Most of Net.Interface.XXX functions implemented on IPSO
- Added function MACToStr

Revision 1.3  2006/07/24 06:49:47  victor
- Process and physical memory parameters are working
- Various other changes

Revision 1.2  2006/07/21 16:22:44  victor
Some parameters are working

Revision 1.1  2006/07/21 11:48:35  victor
Initial commit

Revision 1.7  2005/09/15 21:47:02  victor
Added macro DECLARE_SUBAGENT_INIT to simplify initialization function declaration

Revision 1.6  2005/08/22 23:00:05  alk
Net.IP.RoutingTable added

Revision 1.5  2005/03/10 19:04:07  alk
implemented:
	Net.Interface.AdminStatus(*)
	Net.Interface.Link(*)

Revision 1.4  2005/01/24 19:51:16  alk
reurn types/comments added
Process.Count(*)/System.ProcessCount fixed

Revision 1.3  2005/01/23 05:08:06  alk
+ System.CPU.Count
+ System.Memory.Physical.*
+ System.ProcessCount
+ System.ProcessList

Revision 1.2  2005/01/17 23:25:47  alk
Agent.SourcePackageSupport added

Revision 1.1  2005/01/17 17:14:32  alk
freebsd agent, incomplete (but working)


*/
