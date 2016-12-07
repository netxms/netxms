/*
** NetXMS subagent for FreeBSD
** Copyright (C) 2004 - 2016 Raden Solutions
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

#ifndef __FREEBSD_SUBAGENT_H__
#define __FREEBSD_SUBAGENT_H__

#if __FreeBSD__ < 8
#define _SYS_LOCK_PROFILE_H_  /* prevent include of sys/lock_profile.h which can be C++ incompatible) */
#endif

#include <nms_common.h>
#include <nms_agent.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/utsname.h>
#include <sys/param.h>

#if __FreeBSD__ < 5
#include <sys/proc.h>
#endif

#include <sys/user.h>
#include <fcntl.h>
#include <kvm.h>
#include <paths.h>

enum
{
   PHYSICAL_FREE,
   PHYSICAL_FREE_PCT,
   PHYSICAL_USED,
   PHYSICAL_USED_PCT,
   PHYSICAL_TOTAL,
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
};

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

/**
 * CPU stats
 */
enum
{
   CPU_USAGE_OVERAL,
   CPU_USAGE_USER,
   CPU_USAGE_NICE,
   CPU_USAGE_SYSTEM,
   CPU_USAGE_IDLE,
   CPU_INTERRUPTS,
   CPU_CONTEXT_SWITCHES,
};

/**
 * Load average intervals
 */
enum
{
	INTERVAL_1MIN,
	INTERVAL_5MIN,
	INTERVAL_15MIN,
};

#define MAX_NAME	         64

#define MAKE_CPU_USAGE_PARAM(interval, source)	(const TCHAR *)((((DWORD)(interval)) << 16) | ((DWORD)(source)))
#define CPU_USAGE_PARAM_INTERVAL(p)					((CAST_FROM_POINTER((p), DWORD)) >> 16)
#define CPU_USAGE_PARAM_SOURCE(p)					((CAST_FROM_POINTER((p), DWORD)) & 0x0000FFFF)

#define INFOTYPE_MIN             0
#define INFOTYPE_MAX             1
#define INFOTYPE_AVG             2
#define INFOTYPE_SUM             3

int ExecSysctl(TCHAR *param, void *buffer, size_t buffSize);
void StartCpuUsageCollector();
void ShutdownCpuUsageCollector();

LONG H_ProcessList(const TCHAR *, const TCHAR *, StringList *, AbstractCommSession *);
LONG H_Uptime(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_Uname(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_Hostname(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_Hostname(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_CpuInfo(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_CpuLoad(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_CpuUsage(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_CpuUsageEx(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_CpuCswitch(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_CpuInterrupts(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_ProcessCount(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_ProcessInfo(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_MemoryInfo(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_SourcePkgSupport(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);

#endif // __FREEBSD_SUBAGENT_H__
