/* 
** netxms subagent for darwin
** copyright (c) 2012 alex kirhenshtein
**
** this program is free software; you can redistribute it and/or modify
** it under the terms of the gnu general public license as published by
** the free software foundation; either version 2 of the license, or
** (at your option) any later version.
**
** this program is distributed in the hope that it will be useful,
** but without any warranty; without even the implied warranty of
** merchantability or fitness for a particular purpose.  see the
** gnu general public license for more details.
**
** you should have received a copy of the gnu general public license
** along with this program; if not, write to the free software
** foundation, inc., 675 mass ave, cambridge, ma 02139, usa.
**
**/

#ifndef __cpu__h__
#define __cpu__h__

#define MAKE_CPU_USAGE_PARAM(interval, source)	(const TCHAR *)((((DWORD)(interval)) << 16) | ((DWORD)(source)))
#define CPU_USAGE_PARAM_INTERVAL(p)					((CAST_FROM_POINTER((p), DWORD)) >> 16)
#define CPU_USAGE_PARAM_SOURCE(p)					((CAST_FROM_POINTER((p), DWORD)) & 0x0000FFFF)

//
// CPU stats
//

enum 
{
	CPU_USAGE_OVERAL,
	CPU_USAGE_USER,
	CPU_USAGE_NICE,
	CPU_USAGE_SYSTEM,
	CPU_USAGE_IDLE,
	CPU_USAGE_IOWAIT,
	CPU_USAGE_IRQ,
	CPU_USAGE_SOFTIRQ,
	CPU_USAGE_STEAL,
	CPU_USAGE_GUEST
};


void StartCpuUsageCollector();
void ShutdownCpuUsageCollector();

LONG H_CpuCount(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_CpuLoad(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_CpuUsage(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_CpuUsageEx(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);

#endif // __cpu__h__
