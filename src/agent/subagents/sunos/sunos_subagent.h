/*
** NetXMS subagent for SunOS/Solaris
** Copyright (C) 2004 Victor Kirhenshtein
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
** $module: sunos_subagent.h
**
**/

#ifndef _sunos_subagent_h_
#define _sunos_subagent_h_

#include <nms_common.h>
#include <nms_agent.h>
#include <nms_threads.h>
#include <fcntl.h>
#include <kstat.h>


//
// Disk info types
//

#define DISK_FREE		0
#define DISK_USED		1
#define DISK_TOTAL	2


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

int mac_addr_dlpi(char *pszIfName, u_char *pMacAddr);
LONG ReadKStatValue(char *pszModule, LONG nInstance, char *pszName, char *pszStat, char *pValue);

THREAD_RESULT THREAD_CALL CPUStatCollector(void *pArg);


//
// Global variables
//

extern BOOL g_bShutdown;


#endif
