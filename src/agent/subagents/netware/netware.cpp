/* $Id: netware.cpp,v 1.19 2007-06-08 21:23:18 victor Exp $ */

/*
** NetXMS subagent for Novell NetWare
** Copyright (C) 2004, 2005, 2006, 2007 Victor Kirhenshtein
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
** File: netware.cpp
**
**/

#include "nwagent.h"


//
// Static data
//

static CONDITION m_hCondShutdown = INVALID_CONDITION_HANDLE;
static CONDITION m_hCondTerminate = INVALID_CONDITION_HANDLE;
static THREAD m_hCollectorThread = INVALID_THREAD_HANDLE;
static int m_iCpuUtilHistory[MAX_CPU][CPU_HISTORY_SIZE];
static int m_iCpuHPos = 0;


//
// Memory information
//

static LONG H_MemoryInfo(char *pszParam, char *pArg, char *pValue)
{
	struct memory_info info;
	size64_t nTotalMem;

	memset(&info, 0, sizeof(struct memory_info));
	if (netware_mem_info(&info) != 0)
		return SYSINFO_RC_ERROR;

	// Field info.TotalKnownSystemMemoryUnder4Gb may be zero if
	// total memory is below 4GB (or it is only on older LibC,
	// I don't know).
	if (info.TotalKnownSystemMemory != 0)
		nTotalMem = info.TotalKnownSystemMemory;
	else
		nTotalMem = info.TotalKnownSystemMemoryUnder4Gb;

	switch((int)pArg)
	{
		case MEMINFO_PHYSICAL_TOTAL:
			ret_uint64(pValue, nTotalMem);
			break;
		case MEMINFO_PHYSICAL_USED:
			ret_uint64(pValue, info.TotalWorkMemory);
			break;
		case MEMINFO_PHYSICAL_FREE:
			ret_uint64(pValue, nTotalMem - info.TotalWorkMemory);
			break;
		default:
			break;
	}

	return SYSINFO_RC_SUCCESS;
}


//
// Disk information
//

static LONG H_DiskInfo(char *pszParam, char *pArg, char *pValue)
{
	struct volume_info vi;
	char szVolumeName[MAX_VOLUME_NAME_LEN + 1];
	DWORD dwClusterSize;    // Cluster size in bytes

	NxGetParameterArg(pszParam, 1, szVolumeName, MAX_VOLUME_NAME_LEN);
	memset(&vi, 0, sizeof(struct volume_info));
	if (netware_vol_info_from_name(&vi, szVolumeName) != 0)
		return SYSINFO_RC_UNSUPPORTED;

	dwClusterSize = vi.SectorSize * vi.SectorsPerCluster;
	switch(pArg[0])
	{
		case 'T':   // Total size
			ret_uint64(pValue, (QWORD)vi.VolumeSizeInClusters * (QWORD)dwClusterSize);
			break;
		case 'F':   // Free space
			ret_uint64(pValue, ((QWORD)vi.FreedClusters + (QWORD)vi.SubAllocFreeableClusters) * (QWORD)dwClusterSize + (QWORD)vi.FreeableLimboSectors * (QWORD)vi.SectorSize);
			break;
		case 'U':   // Used space
			ret_uint64(pValue, ((QWORD)(vi.VolumeSizeInClusters - vi.FreedClusters - vi.SubAllocFreeableClusters) * (QWORD)vi.SectorsPerCluster - (QWORD)vi.FreeableLimboSectors) * (QWORD)vi.SectorSize);
			break;
		case 'f':   // Free space percentage
			ret_double(pValue, (double)(((QWORD)vi.FreedClusters + (QWORD)vi.SubAllocFreeableClusters) * (QWORD)dwClusterSize + (QWORD)vi.FreeableLimboSectors * (QWORD)vi.SectorSize) /
					(double)((QWORD)vi.VolumeSizeInClusters * (QWORD)dwClusterSize) * 100);
			break;
		case 'u':   // Used space percentage
			ret_double(pValue, (double)((((QWORD)(vi.VolumeSizeInClusters - vi.FreedClusters - vi.SubAllocFreeableClusters) * (QWORD)vi.SectorsPerCluster - (QWORD)vi.FreeableLimboSectors) * (QWORD)vi.SectorSize)) /
					(double)((QWORD)vi.VolumeSizeInClusters * (QWORD)dwClusterSize) * 100);
			break;
		default:
			return SYSINFO_RC_UNSUPPORTED;
	}

	return SYSINFO_RC_SUCCESS;
}


//
// Host name
//

static LONG H_HostName(char *pszParam, char *pArg, char *pValue)
{
	char szName[256];
	int iErr;

	iErr = gethostname(szName, 256);
	if (iErr == 0)
		ret_string(pValue, szName);
	return (iErr == 0) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}


//
// CPU usage
//

static LONG H_CpuUsage(char *pszParam, char *pArg, char *pValue)
{
	int i, j, iSteps, iValue, iCpu, iCpuCount;
	char szBuffer[256];

	NxGetParameterArg(pszParam, 1, szBuffer, 256);
	if (szBuffer[0] == 0)   // All CPUs?
		iCpu = -1;
	else
		iCpu = strtol(szBuffer, NULL, 0);

	iSteps = (int)pArg;
	iCpuCount = NXGetCpuCount();

	if (iCpu >= iCpuCount)
		return SYSINFO_RC_UNSUPPORTED;

	for(i = m_iCpuHPos - 1, iValue = 0; iSteps > 0; iSteps--, i--)
	{
		if (i < 0)
			i = CPU_HISTORY_SIZE - 1;
		if (iCpu == -1)
		{
			for(j = 0; j < iCpuCount; j++)
				iValue += m_iCpuUtilHistory[j][i];
		}
		else
		{
			iValue += m_iCpuUtilHistory[iCpu][i];
		}
	}

	ret_double(pValue, (iCpu == -1) ? 
			(((double)iValue / (int)pArg) / iCpuCount) : 
			((double)iValue / (int)pArg));
	return SYSINFO_RC_SUCCESS;
}


//
// CPU count
//

static LONG H_CpuCount(char *pszParam, char *pArg, char *pValue)
{
	ret_uint(pValue, NXGetCpuCount());
	return SYSINFO_RC_SUCCESS;
}


//
// Platform name
//

static LONG H_PlatformName(char *pszParam, char *pArg, char *pValue)
{
	ret_string(pValue, "netware-i386");
	return SYSINFO_RC_SUCCESS;
}


//
// ARP cache
//

static LONG H_ArpCache(char *pszParam, char *pArg, NETXMS_VALUES_LIST *pValue)
{
	return SYSINFO_RC_UNSUPPORTED;
}


//
// Shutdown system
//

static LONG H_ActionShutdown(char *pszAction, NETXMS_VALUES_LIST *pArgList, char *pData)
{
	LONG nRet;

	switch(*pData)
	{
		case 'S':	// Shutdown
			ShutdownServer(getscreenhandle(), 1, NULL, SHUTDOWN_POWEROFF);
			nRet = ERR_SUCCESS;
			break;
		case 'R':	// Restart
			ShutdownServer(getscreenhandle(), 1, NULL, SHUTDOWN_RESET);
			nRet = ERR_SUCCESS;
			break;
		default:
			nRet = ERR_INTERNAL_ERROR;
			break;
	}
	return nRet;
}


//
// Collector thread
//

static THREAD_RESULT THREAD_CALL CollectorThread(void *pArg)
{
	struct cpu_info ci;
	int iSeq, iCpu, iNumCpu;

	while(1)
	{
		// Sleep one second
		if (ConditionWait(m_hCondShutdown, 1000))
			break;

		// CPU utilization
		iNumCpu = NXGetCpuCount();
		for(iCpu = 0; iCpu < iNumCpu; iCpu++)
		{
			iSeq = iCpu;   
			if (netware_cpu_info(&ci, &iSeq) == 0)
			{
				m_iCpuUtilHistory[iCpu][m_iCpuHPos] = ci.ProcessorUtilization;
			}
			else
			{
				m_iCpuUtilHistory[iCpu][m_iCpuHPos] = 0;
			}
		}
		m_iCpuHPos++;
		if (m_iCpuHPos == CPU_HISTORY_SIZE)
			m_iCpuHPos = 0;
	}
	return THREAD_OK;
}


//
// Called by master agent at startup
//

static BOOL SubAgentInit(TCHAR *pszConfigFile)
{
	// Setup internal variables
	memset(m_iCpuUtilHistory, 0, sizeof(int) * CPU_HISTORY_SIZE * MAX_CPU);

	// Start collector thread
	m_hCondShutdown = ConditionCreate(TRUE);
	m_hCollectorThread = ThreadCreateEx(CollectorThread, 0, NULL);

	return TRUE;
}


//
// Called by master agent at unload
//

static void SubAgentShutdown(void)
{
	if (m_hCondShutdown != INVALID_CONDITION_HANDLE)
		ConditionSet(m_hCondShutdown);
	ThreadJoin(m_hCollectorThread);
	ConditionDestroy(m_hCondShutdown);

	// Notify main thread that NLM can exit
	ConditionSet(m_hCondTerminate);
}


//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ "Disk.Free(*)", H_DiskInfo, "F", DCI_DT_UINT64, DCIDESC_DISK_FREE },
	{ "Disk.FreePerc(*)", H_DiskInfo, "f", DCI_DT_FLOAT, DCIDESC_DISK_FREEPERC },
	{ "Disk.Total(*)", H_DiskInfo, "T", DCI_DT_UINT64, DCIDESC_DISK_TOTAL },
	{ "Disk.Used(*)", H_DiskInfo, "U", DCI_DT_UINT64, DCIDESC_DISK_USED },
	{ "Disk.UsedPerc(*)", H_DiskInfo, "u", DCI_DT_FLOAT, DCIDESC_DISK_USEDPERC },
	{ "System.CPU.Count", H_CpuCount, NULL, DCI_DT_UINT, DCIDESC_SYSTEM_CPU_COUNT },
	{ "System.CPU.Usage", H_CpuUsage, (char *)60, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE },
	{ "System.CPU.Usage5", H_CpuUsage, (char *)300, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5 },
	{ "System.CPU.Usage15", H_CpuUsage, (char *)900, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15 },
	{ "System.CPU.Usage(*)", H_CpuUsage, (char *)60, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_EX },
	{ "System.CPU.Usage5(*)", H_CpuUsage, (char *)300, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_EX },
	{ "System.CPU.Usage15(*)", H_CpuUsage, (char *)900, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_EX },
	{ "System.Hostname", H_HostName, NULL, DCI_DT_STRING, DCIDESC_SYSTEM_HOSTNAME },
	{ "System.Memory.Physical.Free", H_MemoryInfo, (char *)MEMINFO_PHYSICAL_FREE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE },
	{ "System.Memory.Physical.Total", H_MemoryInfo, (char *)MEMINFO_PHYSICAL_TOTAL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_TOTAL },
	{ "System.Memory.Physical.Used", H_MemoryInfo, (char *)MEMINFO_PHYSICAL_USED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED },
	{ "System.PlatformName", H_PlatformName, NULL, DCI_DT_STRING, DCIDESC_SYSTEM_PLATFORMNAME }
};
static NETXMS_SUBAGENT_ENUM m_enums[] =
{
	{ "Net.ArpCache", H_ArpCache, NULL }
};
static NETXMS_SUBAGENT_ACTION m_actions[] =
{
	{ "System.Restart", H_ActionShutdown, "R", "Restart system" },
	{ "System.Shutdown", H_ActionShutdown, "S", "Shutdown system" }
};

static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	"NETWARE", 
	NETXMS_VERSION_STRING,
	SubAgentInit, SubAgentShutdown, NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_ENUM),
	m_enums,
	sizeof(m_actions) / sizeof(NETXMS_SUBAGENT_ACTION),
	m_actions
};


//
// Entry point for NetXMS agent
//

extern "C" BOOL NxSubAgentInit_NETWARE(NETXMS_SUBAGENT_INFO **ppInfo)
{
	*ppInfo = &m_info;
	return TRUE;
}


//
// NetWare entry point
// We use main() instead of _init() and _fini() to implement
// automatic unload of the subagent after unload handler is called
//

int main(int argc, char *argv[])
{
	m_hCondTerminate = ConditionCreate(TRUE);
	ConditionWait(m_hCondTerminate, INFINITE);
	ConditionDestroy(m_hCondTerminate);
	sleep(1);
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.18  2007/06/07 22:07:11  alk
descriptions changed to defines


*/
