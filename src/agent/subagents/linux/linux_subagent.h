/* 
** NetXMS subagent for GNU/Linux
** Copyright (C) 2004-2024 Raden Solutions
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

#ifndef __LINUX_SUBAGENT_H__
#define __LINUX_SUBAGENT_H__

#include <nms_common.h>
#include <nms_agent.h>
#include <nms_util.h>


#define DEBUG_TAG _T("linux")

/**
 * Attributes for H_ProcInfo
 */
enum
{
	PROCINFO_CPUTIME,
   PROCINFO_HANDLES,
	PROCINFO_KTIME,
   PROCINFO_MEMPERC,
	PROCINFO_PAGEFAULTS,
   PROCINFO_RSS,
   PROCINFO_THREADS,
	PROCINFO_UTIME,
   PROCINFO_VMREGIONS,
	PROCINFO_VMSIZE
};

#define INFOTYPE_MIN             0
#define INFOTYPE_MAX             1
#define INFOTYPE_AVG             2
#define INFOTYPE_SUM             3

/**
 * FS info types
 */
enum
{
   DISK_AVAIL,
   DISK_AVAIL_INODES,
   DISK_AVAIL_INODES_PERC,
   DISK_AVAIL_PERC,
   DISK_FREE,
   DISK_FREE_INODES,
   DISK_FREE_INODES_PERC,
   DISK_FREE_PERC,
   DISK_TOTAL,
   DISK_TOTAL_INODES,
   DISK_USED,
   DISK_USED_INODES,
   DISK_USED_INODES_PERC,
   DISK_USED_PERC
};

/**
 * Network interface stats
 */
enum
{
   IF_INFO_ADMIN_STATUS,
   IF_INFO_OPER_STATUS,
   IF_INFO_BYTES_IN,
   IF_INFO_BYTES_OUT,
   IF_INFO_DESCRIPTION,
   IF_INFO_ERRORS_IN,
   IF_INFO_ERRORS_OUT,
   IF_INFO_PACKETS_IN,
   IF_INFO_PACKETS_OUT,
   IF_INFO_SPEED,
   IF_INFO_BYTES_IN_64,
   IF_INFO_BYTES_OUT_64,
   IF_INFO_ERRORS_IN_64,
   IF_INFO_ERRORS_OUT_64,
   IF_INFO_PACKETS_IN_64,
   IF_INFO_PACKETS_OUT_64
};

/**
 * Memory stats
 */
enum
{
	PHYSICAL_FREE,
	PHYSICAL_FREE_PCT,
	PHYSICAL_USED,
	PHYSICAL_USED_PCT,
	PHYSICAL_TOTAL,
	PHYSICAL_AVAILABLE,
	PHYSICAL_AVAILABLE_PCT,
   PHYSICAL_CACHED,
   PHYSICAL_CACHED_PCT,
   PHYSICAL_BUFFERS,
   PHYSICAL_BUFFERS_PCT,
	SWAP_FREE,
	SWAP_FREE_PCT,
	SWAP_USED,
	SWAP_USED_PCT,
	SWAP_TOTAL,
	VIRTUAL_FREE,
	VIRTUAL_FREE_PCT,
	VIRTUAL_USED,
	VIRTUAL_USED_PCT,
	VIRTUAL_TOTAL,
	VIRTUAL_AVAILABLE,
	VIRTUAL_AVAILABLE_PCT,
};

/**
 * Load average intervals
 */
enum CpuUsageInterval
{
	INTERVAL_1MIN,
	INTERVAL_5MIN,
	INTERVAL_15MIN,
};

/**
 * CPU stats
 */

enum CpuUsageSource
{
	CPU_USAGE_OVERAL,
	CPU_USAGE_USER,
	CPU_USAGE_NICE,
	CPU_USAGE_SYSTEM,
	CPU_USAGE_IDLE,
	CPU_USAGE_IOWAIT,
	CPU_USAGE_IRQ,
	CPU_USAGE_SOFTIRQ,
	CPU_USAGE_STEAL, // time spent in virtualization stuff (xen)
	CPU_USAGE_GUEST,
	CPU_USAGE_NB_SOURCES // keep this item last
};

enum
{
	CPU_INTERRUPTS,
	CPU_CONTEXT_SWITCHES,
};

#define MAKE_CPU_USAGE_PARAM(interval, source)	(const TCHAR *)((((DWORD)(interval)) << 16) | ((DWORD)(source)))
#define CPU_USAGE_PARAM_INTERVAL(p)					((CAST_FROM_POINTER((p), DWORD)) >> 16)
#define CPU_USAGE_PARAM_SOURCE(p)					((CAST_FROM_POINTER((p), DWORD)) & 0x0000FFFF)

/**
 * I/O stats
 */
enum
{
   IOSTAT_NUM_READS       = 0,
   IOSTAT_NUM_WRITES      = 1,
   IOSTAT_NUM_SREADS      = 2,
   IOSTAT_NUM_SWRITES     = 3,
   IOSTAT_IO_TIME         = 4,
   IOSTAT_READ_WAIT_TIME  = 5,
   IOSTAT_WRITE_WAIT_TIME = 6,
   IOSTAT_WAIT_TIME       = 7,
   IOSTAT_DISK_QUEUE      = 8
};

/**
 * Functions
 */

LONG H_FileSystemInfo(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_FileSystems(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_FileSystemType(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_MountPoints(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);

LONG H_IsVirtual(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_HypervisorType(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_HypervisorVersion(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);

LONG H_IoDevices(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_IoLogicalDevices(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);

LONG H_IoStatsCumulative(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_IoStatsNonCumulative(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_IoStatsTotalSectors(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_IoStatsTotalFloat(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_IoStatsTotalTimePct(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_IoStatsTotalNonCumulativeInteger(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_IoStatsTotalNonCumulativeFloat(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);

LONG H_NetIfInfoFromIOCTL(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_NetIfInfoFromProc(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_NetIfInfoSpeed(const TCHAR*, const TCHAR*, TCHAR*, AbstractCommSession*);
LONG H_NetIpForwarding(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_NetIpNeighborsList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_NetIpNeighborsTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_NetArpCache(const TCHAR *, const TCHAR *, StringList *, AbstractCommSession *);
LONG H_NetRoutingTable(const TCHAR *, const TCHAR *, StringList *, AbstractCommSession *);
LONG H_NetIfList(const TCHAR *, const TCHAR *, StringList *, AbstractCommSession *);
LONG H_NetIfNames(const TCHAR *, const TCHAR *, StringList *, AbstractCommSession *);
LONG H_NetIfTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_NetworkAdaptersTable(const TCHAR* cmd, const TCHAR* arg, Table* value, AbstractCommSession* session);

LONG H_ProcessList(const TCHAR *, const TCHAR *, StringList *, AbstractCommSession *);
LONG H_ProcessTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *);
LONG H_Uptime(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_Uname(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_CpuCount(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_CpuInfo(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_CpuCswitch(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_CpuInterrupts(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_CpuLoad(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_CpuUsage(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_CpuUsageEx(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_CpuVendorId(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_ProcessCount(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_ProcessDetails(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_StorageDeviceTable(const TCHAR* cmd, const TCHAR* arg, Table* value, AbstractCommSession* session);
LONG H_SystemProcessCount(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_ThreadCount(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_MemoryInfo(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_OSInfo(const TCHAR*, const TCHAR*, TCHAR*, AbstractCommSession*);
LONG H_SourcePkgSupport(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_ConnectedUsers(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_UserSessionList(const TCHAR *, const TCHAR *, StringList *, AbstractCommSession *);
LONG H_UserSessionTable(const TCHAR *, const TCHAR *, Table *, AbstractCommSession *);
LONG H_SysMsgQueue(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_HardwareSystemInfo(const TCHAR* param, const TCHAR* arg, TCHAR* value, AbstractCommSession* session);

void StartCpuUsageCollector();
void ShutdownCpuUsageCollector();

void StartIoStatCollector();
void ShutdownIoStatCollector();

void InitDrbdCollector();
void StopDrbdCollector();

void ReadCPUVendorId();

uint64_t GetTotalMemorySize();

#endif // __LINUX_SUBAGENT_H__
