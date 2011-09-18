/* $Id$ */

/* 
** NetXMS subagent for HP-UX
** Copyright (C) 2006 Alex Kirhenshtein
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
*/

#ifndef __SYSTEM_H__
#define __SYSTEM_H__

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
	VIRTUAL_TOTAL
};


//
// Types for Process.XXX() parameters
//

#define PROCINFO_IO_READ_B       1
#define PROCINFO_IO_READ_OP      2
#define PROCINFO_IO_WRITE_B      3
#define PROCINFO_IO_WRITE_OP     4
#define PROCINFO_KTIME           5
#define PROCINFO_PF              6
#define PROCINFO_UTIME           7
#define PROCINFO_VMSIZE          8
#define PROCINFO_WKSET           9
#define PROCINFO_THREADS         10


LONG H_ProcessList(const char *, const char *, StringList *);
LONG H_Uptime(const char *, const char *, char *);
LONG H_Uname(const char *, const char *, char *);
LONG H_Hostname(const char *, const char *, char *);
LONG H_Hostname(const char *, const char *, char *);
LONG H_CpuLoad(const char *, const char *, char *);
LONG H_CpuUsage(const char *, const char *, char *);
LONG H_ProcessCount(const char *, const char *, char *);
LONG H_ProcessDetails(const char *, const char *, char *);
LONG H_SysProcessCount(const char *, const char *, char *);
LONG H_MemoryInfo(const char *, const char *, char *);
LONG H_SourcePkgSupport(const char *, const char *, char *);
LONG H_ConnectedUsers(const char *, const char *, char *);
LONG H_OpenFiles(const char *, const char *, char *);

void StartCpuUsageCollector(void);
void ShutdownCpuUsageCollector(void);

#endif // __SYSTEM_H__
