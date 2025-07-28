/*
** NetXMS subagent for AIX
** Copyright (C) 2005-2025 Victor Kirhenshtein
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
#include "libperfstat_compat.h"
#include <pwd.h>
#include <sys/procfs.h>

#define AIX_DEBUG_TAG	_T("aix")

/**
 * CPU stats
 */
enum 
{
	CPU_USAGE_OVERALL,
	CPU_USAGE_USER,
	CPU_USAGE_SYSTEM,
	CPU_USAGE_IDLE,
	CPU_USAGE_IOWAIT,
	CPU_PA_OVERALL,
	CPU_PA_USER,
	CPU_PA_SYSTEM,
	CPU_PA_IDLE,
	CPU_PA_IOWAIT
};

enum
{
	INTERVAL_1MIN,
	INTERVAL_5MIN,
	INTERVAL_15MIN,
};

#define MAKE_CPU_USAGE_PARAM(interval, source) (const TCHAR *)((((DWORD)(interval)) << 16) | ((DWORD)(source)))
#define CPU_USAGE_PARAM_INTERVAL(p) ((CAST_FROM_POINTER((p), DWORD)) >> 16)
#define CPU_USAGE_PARAM_SOURCE(p) ((CAST_FROM_POINTER((p), DWORD)) & 0x0000FFFF)

/**
 * FIle system info types
 */
enum
{
	FS_AVAIL,
	FS_AVAIL_INODES,
	FS_AVAIL_INODES_PERC,
	FS_AVAIL_PERC,
	FS_FREE,
	FS_FREE_INODES,
	FS_FREE_INODES_PERC,
	FS_FREE_PERC,
	FS_FSTYPE,
	FS_TOTAL,
	FS_TOTAL_INODES,
	FS_USED,
	FS_USED_INODES,
	FS_USED_INODES_PERC,
	FS_USED_PERC
};

/**
 * LVM info types
 */
enum
{
	LVM_LV_SIZE,
	LVM_LV_STATUS,
	LVM_PV_FREE,
	LVM_PV_FREE_PERC,
	LVM_PV_RESYNC,
	LVM_PV_STALE,
	LVM_PV_STATUS,
	LVM_PV_TOTAL,
	LVM_PV_USED,
	LVM_PV_USED_PERC,
	LVM_VG_FREE,
	LVM_VG_FREE_PERC,
	LVM_VG_LVOL_TOTAL,
	LVM_VG_PVOL_ACTIVE,
	LVM_VG_PVOL_TOTAL,
	LVM_VG_RESYNC,
	LVM_VG_STALE,
	LVM_VG_STATUS,
	LVM_VG_TOTAL,
	LVM_VG_USED,
	LVM_VG_USED_PERC
};

/**
 * Request types for H_MemoryInfo
 */
enum
{
	MEMINFO_PHYSICAL_AVAILABLE,
	MEMINFO_PHYSICAL_AVAILABLE_PERC,
	MEMINFO_PHYSICAL_CACHED,
	MEMINFO_PHYSICAL_CACHED_PERC,
	MEMINFO_PHYSICAL_CLIENT,
	MEMINFO_PHYSICAL_CLIENT_PERC,
	MEMINFO_PHYSICAL_COMP,
	MEMINFO_PHYSICAL_COMP_PERC,
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
	IOSTAT_NUM_WRITES,
	IOSTAT_NUM_RBYTES,
	IOSTAT_NUM_WBYTES,
	IOSTAT_IO_TIME,
	IOSTAT_QUEUE,
	IOSTAT_NUM_XFERS,
	IOSTAT_WAIT_TIME
};

/**
 * Network interface request types
 */
enum
{
	IF_INFO_ADMIN_STATUS,
	IF_INFO_OPER_STATUS,
	IF_INFO_BYTES_IN,
	IF_INFO_BYTES_OUT,
	IF_INFO_DESCRIPTION,
	IF_INFO_IN_ERRORS,
	IF_INFO_OUT_ERRORS,
	IF_INFO_PACKETS_IN,
	IF_INFO_PACKETS_OUT,
	IF_INFO_SPEED,
	IF_INFO_MTU
};

/**
 * Process list entry structure
 */
typedef struct t_ProcEnt
{
	unsigned int nPid;
	char szProcName[128];
} PROC_ENT;

// Functions
void StartCpuUsageCollector();
void ShutdownCpuUsageCollector();

void StartIOStatCollector();
void ShutdownIOStatCollector();

void ClearLvmData();

void ClearNetworkData();

// Global variables
extern BOOL g_bShutdown;

#endif
