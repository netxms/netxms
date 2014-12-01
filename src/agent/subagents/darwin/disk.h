/* 
** NetXMS subagent for Darwin/OS X
** Copyright (C) 2012 Alex Kirhenshtein
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

#ifndef __DISK_H__
#define __DISK_H__

enum
{
	DISK_AVAIL,
	DISK_AVAIL_PERC,
	DISK_FREE,
	DISK_FREE_PERC,
	DISK_USED,
	DISK_USED_PERC,
	DISK_TOTAL,
};

LONG H_DiskInfo(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_FileSystems(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *);
LONG H_MountPoints(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);

#endif // __DISK_H__
