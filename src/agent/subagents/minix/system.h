/* 
** NetXMS subagent for NetBSD
** Copyright (C) 2004 Alex Kirhenshtein
** Copyright (C) 2008 Mark Ibell
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
	PHYSICAL_TOTAL
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

LONG H_ProcessList(const char *, const char *, StringList *, AbstractCommSession *);
LONG H_Uptime(const char *, const char *, char *, AbstractCommSession *);
LONG H_Uname(const char *, const char *, char *, AbstractCommSession *);
LONG H_Hostname(const char *, const char *, char *, AbstractCommSession *);
LONG H_Hostname(const char *, const char *, char *, AbstractCommSession *);
LONG H_CpuCount(const char *, const char *, char *, AbstractCommSession *);
LONG H_CpuLoad(const char *, const char *, char *, AbstractCommSession *);
LONG H_CpuUsage(const char *, const char *, char *, AbstractCommSession *);
LONG H_ProcessCount(const char *, const char *, char *, AbstractCommSession *);
LONG H_ProcessInfo(const char *, const char *, char *, AbstractCommSession *);
LONG H_MemoryInfo(const char *, const char *, char *, AbstractCommSession *);
LONG H_SourcePkgSupport(const char *, const char *, char *, AbstractCommSession *);

#endif // __SYSTEM_H__

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.3  2005/01/23 05:08:06  alk
+ System.CPU.Count
+ System.Memory.Physical.*
+ System.ProcessCount
+ System.ProcessList

Revision 1.2  2005/01/17 23:25:48  alk
Agent.SourcePackageSupport added

Revision 1.1  2005/01/17 17:14:32  alk
freebsd agent, incomplete (but working)

Revision 1.1  2004/10/22 22:08:35  alk
source restructured;
implemented:
	Net.IP.Forwarding
	Net.IP6.Forwarding
	Process.Count(*)
	Net.ArpCache
	Net.InterfaceList (if-type not implemented yet)
	System.ProcessList


*/
