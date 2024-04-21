/*
** Windows XP/2003/Vista/2008/7/8 NetXMS subagent
** Copyright (C) 2003-2021 Victor Kirhenshtein
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
** File: winnt_subagent.h
**
**/

#ifndef _winnt_subagent_h_
#define _winnt_subagent_h_

#define PSAPI_VERSION 1
#define NTDDI_VERSION NTDDI_WIN7

#include <nms_common.h>
#include <nms_agent.h>
#include <nxcpapi.h>
#include <nxlog.h>
#include <psapi.h>
#include <wtsapi32.h>
#include <iphlpapi.h>
#include <iprtrmib.h>
#include <rtinfo.h>

#define DEBUG_TAG          _T("winnt")

#define MAX_PROCESSES      4096
#define MAX_MODULES        512

/**
 * Attributes for I/O stats
 */
enum IOInfoType
{
   IOSTAT_DISK_QUEUE,
   IOSTAT_IO_READ_TIME,
   IOSTAT_IO_TIME,
   IOSTAT_IO_WRITE_TIME,
   IOSTAT_NUM_READS,
   IOSTAT_NUM_SREADS,
   IOSTAT_NUM_SWRITES,
   IOSTAT_NUM_WRITES
};

/**
 * Attributes for H_ProcInfo
 */
enum ProcessAttribute
{
   PROCINFO_CPUTIME,
   PROCINFO_GDI_OBJ,
   PROCINFO_HANDLES,
   PROCINFO_IO_OTHER_B,
   PROCINFO_IO_OTHER_OP,
   PROCINFO_IO_READ_B,
   PROCINFO_IO_READ_OP,
   PROCINFO_IO_WRITE_B,
   PROCINFO_IO_WRITE_OP,
   PROCINFO_KTIME,
   PROCINFO_MEMPERC,
   PROCINFO_PAGE_FAULTS,
   PROCINFO_THREADS,
   PROCINFO_USER_OBJ,
   PROCINFO_UTIME,
   PROCINFO_VMSIZE,
   PROCINFO_WKSET
};

/**
 * Process information aggregation method
 */
enum class ProcessInfoAgregationMethod
{
   MIN = 0,
   MAX = 1,
   AVG = 2,
   SUM = 3
};

/**
 * Attributes for H_NetIPStats and H_NetInterfacStats
 */
#define NETINFO_IP_FORWARDING        1

#define NETINFO_IF_BYTES_IN          1
#define NETINFO_IF_BYTES_OUT         2
#define NETINFO_IF_DESCR             3
#define NETINFO_IF_IN_ERRORS         4
#define NETINFO_IF_OPER_STATUS       5
#define NETINFO_IF_OUT_ERRORS        6
#define NETINFO_IF_PACKETS_IN        7
#define NETINFO_IF_PACKETS_OUT       8
#define NETINFO_IF_SPEED             9
#define NETINFO_IF_ADMIN_STATUS      10
#define NETINFO_IF_MTU               11
#define NETINFO_IF_BYTES_IN_64       12
#define NETINFO_IF_BYTES_OUT_64      13
#define NETINFO_IF_PACKETS_IN_64     14
#define NETINFO_IF_PACKETS_OUT_64    15

/**
 * Request types for H_MemoryInfo
 */
enum MemoryInfoType
{
   MEMINFO_PHYSICAL_AVAIL,
   MEMINFO_PHYSICAL_AVAIL_PCT,
   MEMINFO_PHYSICAL_CACHE,
   MEMINFO_PHYSICAL_CACHE_PCT,
   MEMINFO_PHYSICAL_FREE,
   MEMINFO_PHYSICAL_FREE_PCT,
   MEMINFO_PHYSICAL_TOTAL,
   MEMINFO_PHYSICAL_USED,
   MEMINFO_PHYSICAL_USED_PCT,
   MEMINFO_VIRTUAL_FREE,
   MEMINFO_VIRTUAL_FREE_PCT,
   MEMINFO_VIRTUAL_TOTAL,
   MEMINFO_VIRTUAL_USED,
   MEMINFO_VIRTUAL_USED_PCT
};

/**
 * Request types for H_FileSystemInfo
 */
#define FSINFO_FREE_BYTES      1
#define FSINFO_USED_BYTES      2
#define FSINFO_TOTAL_BYTES     3
#define FSINFO_FREE_SPACE_PCT  4
#define FSINFO_USED_SPACE_PCT  5

/**
 * Window list
 */
struct WINDOW_LIST
{
	DWORD pid;
	StringList *pWndList;
};

/**
 * Read CPU vendor ID
 */
void ReadCPUVendorId();

#endif   /* _winnt_subagent_h_ */
