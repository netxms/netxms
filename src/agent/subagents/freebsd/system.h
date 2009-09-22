/* $Id$ */

/* 
** NetXMS subagent for FreeBSD
** Copyright (C) 2004 Alex Kirhenshtein
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

#ifndef __SYSTEM_H__
#define __SYSTEM_H__

enum
{
	PHYSICAL_FREE,
	PHYSICAL_USED,
	PHYSICAL_TOTAL,
	SWAP_FREE,
	SWAP_USED,
	SWAP_TOTAL,
	VIRTUAL_FREE,
	VIRTUAL_USED,
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

#define INFOTYPE_MIN             0
#define INFOTYPE_MAX             1
#define INFOTYPE_AVG             2
#define INFOTYPE_SUM             3


LONG H_ProcessList(const char *, const char *, StringList *);
LONG H_Uptime(const char *, const char *, char *);
LONG H_Uname(const char *, const char *, char *);
LONG H_Hostname(const char *, const char *, char *);
LONG H_Hostname(const char *, const char *, char *);
LONG H_CpuCount(const char *, const char *, char *);
LONG H_CpuLoad(const char *, const char *, char *);
LONG H_CpuUsage(const char *, const char *, char *);
LONG H_ProcessCount(const char *, const char *, char *);
LONG H_ProcessInfo(const char *, const char *, char *);
LONG H_MemoryInfo(const char *, const char *, char *);
LONG H_SourcePkgSupport(const char *, const char *, char *);

#endif // __SYSTEM_H__
