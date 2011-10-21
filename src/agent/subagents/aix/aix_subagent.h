/*
** NetXMS subagent for AIX
** Copyright (C) 2005-2009 Victor Kirhenshtein
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
** File: aix_subagent.h
**
**/

#ifndef _aix_subagent_h_
#define _aix_subagent_h_

#include <nms_common.h>
#include <nms_agent.h>
#include <nms_threads.h>
#include <sys/var.h>


//
// CPU stats
//

enum 
{
	CPU_USAGE_OVERAL,
	CPU_USAGE_USER,
	CPU_USAGE_SYSTEM,
	CPU_USAGE_IDLE,
	CPU_USAGE_IOWAIT
};

enum
{
	INTERVAL_1MIN,
	INTERVAL_5MIN,
	INTERVAL_15MIN,
};

#define MAKE_CPU_USAGE_PARAM(interval, source) (const char *)((((DWORD)(interval)) << 16) | ((DWORD)(source)))
#define CPU_USAGE_PARAM_INTERVAL(p) ((CAST_FROM_POINTER((p), DWORD)) >> 16)
#define CPU_USAGE_PARAM_SOURCE(p) ((CAST_FROM_POINTER((p), DWORD)) & 0x0000FFFF)


//
// Disk info types
//

#define DISK_AVAIL		0
#define DISK_AVAIL_PERC		1
#define DISK_FREE		2
#define DISK_FREE_PERC		3
#define DISK_USED		4
#define DISK_USED_PERC		5
#define DISK_TOTAL		6


//
// Request types for H_MemoryInfo
//

enum
{
	MEMINFO_PHYSICAL_FREE,
	MEMINFO_PHYSICAL_FREE_PERC,
	MEMINFO_PHYSICAL_TOTAL,
	MEMINFO_PHYSICAL_USED,
	MEMINFO_PHYSICAL_USED_PERC,
	MEMINFO_SWAP_FREE,
	MEMINFO_SWAP_FREE_PERC,
	MEMINFO_SWAP_TOTAL,
	MEMINFO_SWAP_USED,
	MEMINFO_SWAP_USED_PERC,
	MEMINFO_VIRTUAL_ACTIVE,
	MEMINFO_VIRTUAL_ACTIVE_PERC,
	MEMINFO_VIRTUAL_FREE,
	MEMINFO_VIRTUAL_FREE_PERC,
	MEMINFO_VIRTUAL_TOTAL,
	MEMINFO_VIRTUAL_USED,
	MEMINFO_VIRTUAL_USED_PERC
};


//
// Types for Process.XXX() parameters
//

enum
{
	PROCINFO_CPUTIME,
	PROCINFO_IO_READ_B,
	PROCINFO_IO_READ_OP,
	PROCINFO_IO_WRITE_B,
	PROCINFO_IO_WRITE_OP,
	PROCINFO_KTIME,
	PROCINFO_PF,
	PROCINFO_UTIME,
	PROCINFO_VMSIZE,
	PROCINFO_WKSET,
	PROCINFO_THREADS
};


//
// I/O stats request types
//

enum
{
	IOSTAT_NUM_READS,
	IOSTAT_NUM_WRITES,
	IOSTAT_NUM_RBYTES,
	IOSTAT_NUM_WBYTES,
	IOSTAT_IO_TIME,
	IOSTAT_QUEUE,
	IOSTAT_NUM_XFERS,
	IOSTAT_WAIT_TIME
};


//
// Process list entry structure
//

typedef struct t_ProcEnt
{
	unsigned int nPid;
	char szProcName[128];
} PROC_ENT;


//
// Functions
//

void StartCpuUsageCollector();
void ShutdownCpuUsageCollector();

void StartIOStatCollector();
void ShutdownIOStatCollector();


//
// Global variables
//

extern BOOL g_bShutdown;


#endif
