/*
** Windows XP/2003/Vista/2008/7/8 NetXMS subagent
** Copyright (C) 2003-2012 Victor Kirhenshtein
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

#include <nms_common.h>
#include <nms_agent.h>
#include <psapi.h>
#include <wtsapi32.h>


#define MAX_PROCESSES      4096
#define MAX_MODULES        512

/**
 * Attributes for H_ProcInfo
 */
#define PROCINFO_GDI_OBJ         1
#define PROCINFO_IO_OTHER_B      2
#define PROCINFO_IO_OTHER_OP     3
#define PROCINFO_IO_READ_B       4
#define PROCINFO_IO_READ_OP      5
#define PROCINFO_IO_WRITE_B      6
#define PROCINFO_IO_WRITE_OP     7
#define PROCINFO_KTIME           8
#define PROCINFO_PF              9
#define PROCINFO_USER_OBJ        10
#define PROCINFO_UTIME           11
#define PROCINFO_VMSIZE          12
#define PROCINFO_WKSET           13
#define PROCINFO_CPUTIME         14

#define INFOTYPE_MIN             0
#define INFOTYPE_MAX             1
#define INFOTYPE_AVG             2
#define INFOTYPE_SUM             3

/**
 * Window list
 */
struct WINDOW_LIST
{
	DWORD dwPID;
	StringList *pWndList;
};

#endif   /* _winnt_subagent_h_ */
