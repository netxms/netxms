/* 
** NetXMS subagent for GNU/Linux
** Copyright (C) 2004-2013 Raden Solutions
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

#include <locale.h>
#include <sys/utsname.h>
#include <sys/statvfs.h>
#include <utmp.h>
#include <paths.h>
#include <mntent.h>
#include <sys/vfs.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <ifaddrs.h>

/*
 * Attributes for H_ProcInfo
 */

enum
{
	PROCINFO_CPUTIME,
	PROCINFO_KTIME,
	PROCINFO_PAGEFAULTS,
	PROCINFO_THREADS,
	PROCINFO_UTIME,
	PROCINFO_VMSIZE,
	PROCINFO_WKSET
};

#define INFOTYPE_MIN             0
#define INFOTYPE_MAX             1
#define INFOTYPE_AVG             2
#define INFOTYPE_SUM             3

/*
 * Process entry
 */
typedef struct t_ProcEnt
{
	UINT32 nPid;
	char szProcName[128];
	UINT32 parent;					// PID of parent process
	UINT32 group;					// Group ID
	char state;					// Process state
	long threads;				// Number of threads
	unsigned long ktime;		// Number of ticks spent in kernel mode
	unsigned long utime;		// Number of ticks spent in user mode
	unsigned long vmsize;	// Size of process's virtual memory in bytes
	long rss;					// Process's resident set size in pages
	unsigned long minflt;	// Number of minor page faults
	unsigned long majflt;	// Number of major page faults
} PROC_ENT;

typedef struct ifaddrs IFADDRS;

/*
 * Interface info
 */
typedef struct {
   UINT32 index;
   char addr[32];
   int mask;
   int type;
   char mac[16];
   char name[16];
} IFINFO;

/*
 * FS info types
 */
enum
{
	DISK_FREE,
	DISK_AVAIL,
	DISK_USED,
	DISK_TOTAL,
	DISK_FREE_PERC,
	DISK_AVAIL_PERC,
	DISK_USED_PERC
};


/*
 * Network interface stats
 */

#define IF_INFO_ADMIN_STATUS     0
#define IF_INFO_OPER_STATUS      1
#define IF_INFO_BYTES_IN         2
#define IF_INFO_BYTES_OUT        3
#define IF_INFO_DESCRIPTION      4
#define IF_INFO_IN_ERRORS        5
#define IF_INFO_OUT_ERRORS       6
#define IF_INFO_PACKETS_IN       7
#define IF_INFO_PACKETS_OUT      8
#define IF_INFO_SPEED            9


/*
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


/*
 * Load average intervals
 */

enum
{
	INTERVAL_1MIN,
	INTERVAL_5MIN,
	INTERVAL_15MIN,
};


/*
 * CPU stats
 */

enum 
{
	CPU_USAGE_OVERAL,
	CPU_USAGE_USER,
	CPU_USAGE_NICE,
	CPU_USAGE_SYSTEM,
	CPU_USAGE_IDLE,
	CPU_USAGE_IOWAIT,
	CPU_USAGE_IRQ,
	CPU_USAGE_SOFTIRQ,
	CPU_USAGE_STEAL,
	CPU_USAGE_GUEST,
};

#define MAKE_CPU_USAGE_PARAM(interval, source)	(const TCHAR *)((((DWORD)(interval)) << 16) | ((DWORD)(source)))
#define CPU_USAGE_PARAM_INTERVAL(p)					((CAST_FROM_POINTER((p), DWORD)) >> 16)
#define CPU_USAGE_PARAM_SOURCE(p)					((CAST_FROM_POINTER((p), DWORD)) & 0x0000FFFF)

/*
 * I/O stats
 */

#define IOSTAT_NUM_READS      0
#define IOSTAT_NUM_WRITES     1
#define IOSTAT_NUM_SREADS     2
#define IOSTAT_NUM_SWRITES    3
#define IOSTAT_IO_TIME        4

/*
 * Functions
 */

LONG H_DiskInfo(const TCHAR *, const TCHAR *, TCHAR *);
LONG H_FileSystems(const TCHAR *cmd, const TCHAR *arg, Table *value);
LONG H_MountPoints(const TCHAR *cmd, const TCHAR *arg, StringList *value);

LONG H_IoStats(const TCHAR *, const TCHAR *, TCHAR *);
LONG H_IoStatsTotal(const TCHAR *, const TCHAR *, TCHAR *);
LONG H_DiskQueue(const TCHAR *, const TCHAR *, TCHAR *);
LONG H_DiskQueueTotal(const TCHAR *, const TCHAR *, TCHAR *);

LONG H_NetIfInfoFromIOCTL(const TCHAR *, const TCHAR *, TCHAR *);
LONG H_NetIfInfoFromProc(const TCHAR *, const TCHAR *, TCHAR *);
LONG H_NetIpForwarding(const TCHAR *, const TCHAR *, TCHAR *);
LONG H_NetArpCache(const TCHAR *, const TCHAR *, StringList *);
LONG H_NetRoutingTable(const TCHAR *, const TCHAR *, StringList *);
LONG H_NetIfList(const TCHAR *, const TCHAR *, StringList *);

LONG H_ProcessList(const TCHAR *, const TCHAR *, StringList *);
LONG H_ProcessTable(const TCHAR *cmd, const TCHAR *arg, Table *value);
LONG H_Uptime(const TCHAR *, const TCHAR *, TCHAR *);
LONG H_Uname(const TCHAR *, const TCHAR *, TCHAR *);
LONG H_Hostname(const TCHAR *, const TCHAR *, TCHAR *);
LONG H_Hostname(const TCHAR *, const TCHAR *, TCHAR *);
LONG H_CpuCount(const TCHAR *, const TCHAR *, TCHAR *);
LONG H_CpuLoad(const TCHAR *, const TCHAR *, TCHAR *);
LONG H_CpuUsage(const TCHAR *, const TCHAR *, TCHAR *);
LONG H_CpuUsageEx(const TCHAR *, const TCHAR *, TCHAR *);
LONG H_ProcessCount(const TCHAR *, const TCHAR *, TCHAR *);
LONG H_ProcessDetails(const TCHAR *, const TCHAR *, TCHAR *);
LONG H_ThreadCount(const TCHAR *, const TCHAR *, TCHAR *);
LONG H_MemoryInfo(const TCHAR *, const TCHAR *, TCHAR *);
LONG H_SourcePkgSupport(const TCHAR *, const TCHAR *, TCHAR *);
LONG H_ConnectedUsers(const TCHAR *, const TCHAR *, TCHAR *);
LONG H_ActiveUserSessions(const TCHAR *, const TCHAR *, StringList *);

void StartCpuUsageCollector();
void ShutdownCpuUsageCollector();

void StartIoStatCollector();
void ShutdownIoStatCollector();

void InitDrbdCollector();
void StopDrbdCollector();

int ProcRead(PROC_ENT **pEnt, char *szProcName, char *szCmdLine);

#endif // __LINUX_SUBAGENT_H__
