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
	VIRTUAL_TOTAL,
};


#define INFOTYPE_MIN             0
#define INFOTYPE_MAX             1
#define INFOTYPE_AVG             2
#define INFOTYPE_SUM             3


LONG H_ProcessList(const TCHAR *, const TCHAR *, StringList *, AbstractCommSession *);
LONG H_Uptime(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_Uname(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_Hostname(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_Hostname(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_ProcessCount(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_ProcessInfo(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_MemoryInfo(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_SourcePkgSupport(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);

#endif // __SYSTEM_H__
