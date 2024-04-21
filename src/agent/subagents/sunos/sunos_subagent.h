/*
** NetXMS subagent for SunOS/Solaris
** Copyright (C) 2004-2020 Raden Solutions
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
** File: sunos_subagent.h
**
**/

#ifndef _sunos_subagent_h_
#define _sunos_subagent_h_

#include <nms_common.h>
#include <nms_agent.h>
#include <nms_threads.h>
#include <fcntl.h>
#include <kstat.h>

#define AGENT_NAME _T("SunOS")

#define DEBUG_TAG _T("sunos")

//
// Subagent flags
//

#define SF_IF_ALL_ZONES 0x00000001
#define SF_GLOBAL_ZONE  0x00000002
#define SF_SOLARIS_11   0x00000004

/**
 * File system info types
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
   DISK_FSTYPE,
   DISK_TOTAL,
   DISK_TOTAL_INODES,
   DISK_USED,
   DISK_USED_INODES,
   DISK_USED_INODES_PERC,
   DISK_USED_PERC
};

/**
 * Request types for H_MemoryInfo
 */
enum
{
   MEMINFO_PHYSICAL_FREE,
   MEMINFO_PHYSICAL_FREEPCT,
   MEMINFO_PHYSICAL_TOTAL,
   MEMINFO_PHYSICAL_USED,
   MEMINFO_PHYSICAL_USEDPCT,
   MEMINFO_SWAP_FREE,
   MEMINFO_SWAP_FREEPCT,
   MEMINFO_SWAP_TOTAL,
   MEMINFO_SWAP_USED,
   MEMINFO_SWAP_USEDPCT,
   MEMINFO_VIRTUAL_FREE,
   MEMINFO_VIRTUAL_FREEPCT,
   MEMINFO_VIRTUAL_TOTAL,
   MEMINFO_VIRTUAL_USED,
   MEMINFO_VIRTUAL_USEDPCT
};

/**
 * Types for Process.XXX() parameters
 */
enum
{
   PROCINFO_CPUTIME,
   PROCINFO_HANDLES,
   PROCINFO_IO_READ_B,
   PROCINFO_IO_READ_OP,
   PROCINFO_IO_WRITE_B,
   PROCINFO_IO_WRITE_OP,
   PROCINFO_KTIME,
   PROCINFO_MEMPERC,
   PROCINFO_PF,
   PROCINFO_RSS,
   PROCINFO_SYSCALLS,
   PROCINFO_THREADS,
   PROCINFO_UTIME,
   PROCINFO_VMSIZE
};

/**
 * I/O stats request types
 */
enum
{	
   IOSTAT_NUM_READS,
   IOSTAT_NUM_READS_MIN,
   IOSTAT_NUM_READS_MAX,
   IOSTAT_NUM_WRITES,
   IOSTAT_NUM_WRITES_MIN,
   IOSTAT_NUM_WRITES_MAX,
   IOSTAT_NUM_RBYTES,
   IOSTAT_NUM_RBYTES_MIN,
   IOSTAT_NUM_RBYTES_MAX,
   IOSTAT_NUM_WBYTES,
   IOSTAT_NUM_WBYTES_MIN,
   IOSTAT_NUM_WBYTES_MAX,
   IOSTAT_IO_TIME,
   IOSTAT_QUEUE,
   IOSTAT_QUEUE_MIN,
   IOSTAT_QUEUE_MAX
};

//
// Functions
//
int mac_addr_dlpi(char *pszIfName, u_char *pMacAddr);
LONG ReadKStatValue(const char *pszModule, LONG nInstance, const char *pszName,
      const char *pszStat, TCHAR *pValue, kstat_named_t *pRawValue);

void CPUStatCollector();
void IOStatCollector();

void kstat_lock();
void kstat_unlock();

void ReadCPUVendorId();

//
// Global variables
//
extern BOOL g_bShutdown;
extern DWORD g_flags;


#endif
