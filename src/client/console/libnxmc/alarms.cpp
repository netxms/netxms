/* 
** NetXMS - Network Management System
** Portable management console - plugin API library
** Copyright (C) 2007 Victor Kirhenshtein
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
** File: alarms.cpp
**
**/

#include "libnxmc.h"


//
// Static data
//

static nxAlarmList s_alarmList;
static MUTEX s_mutexAlarmAccess = INVALID_MUTEX_HANDLE;


//
// Initialize alarm list
//

void LIBNXMC_EXPORTABLE NXMCInitAlarms(DWORD count, NXC_ALARM *list)
{
	DWORD i;

	s_mutexAlarmAccess = MutexCreate();
	for(i = 0; i < count; i++)
		s_alarmList[list[i].dwAlarmId] = (NXC_ALARM *)nx_memdup(&list[i], sizeof(NXC_ALARM));
}


//
// Update alarm list
//

void LIBNXMC_EXPORTABLE NXMCUpdateAlarms(DWORD code, NXC_ALARM *data)
{
	nxAlarmList::iterator it;

	MutexLock(s_mutexAlarmAccess, INFINITE);
	switch(code)
	{
		case NX_NOTIFY_NEW_ALARM:
			s_alarmList[data->dwAlarmId] = (NXC_ALARM *)nx_memdup(data, sizeof(NXC_ALARM));
			break;
      case NX_NOTIFY_ALARM_DELETED:
      case NX_NOTIFY_ALARM_TERMINATED:
			it = s_alarmList.find(data->dwAlarmId);
			if (it != s_alarmList.end())
			{
				free(it->second);
				s_alarmList.erase(it);
			}
			break;
      case NX_NOTIFY_ALARM_CHANGED:
			it = s_alarmList.find(data->dwAlarmId);
			if (it != s_alarmList.end())
			{
				memcpy(it->second, data, sizeof(NXC_ALARM));
			}
			else
			{
				s_alarmList[data->dwAlarmId] = (NXC_ALARM *)nx_memdup(data, sizeof(NXC_ALARM));
			}
			break;
	}
	MutexUnlock(s_mutexAlarmAccess);
}


//
// Get alarm list
//

nxAlarmList LIBNXMC_EXPORTABLE *NXMCGetAlarmList()
{
	MutexLock(s_mutexAlarmAccess, INFINITE);
	return &s_alarmList;
}


//
// Unlock alarm list
//

void LIBNXMC_EXPORTABLE NXMCUnlockAlarmList()
{
	MutexUnlock(s_mutexAlarmAccess);
}
