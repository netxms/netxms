/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: alarm.cpp
**
**/

#include "nms_core.h"


//
// Global instance of alarm manager
//

AlarmManager g_alarmMgr;


//
// Alarm manager constructor
//

AlarmManager::AlarmManager()
{
   m_dwNumAlarms = 0;
   m_pAlarmList = NULL;
}


//
// Alarm manager destructor
//

AlarmManager::~AlarmManager()
{
   safe_free(m_pAlarmList);
}


//
// Initialize alarm manager at system startup
//

BOOL AlarmManager::Init(void)
{
   return TRUE;
}


//
// Create new alarm
//

void AlarmManager::NewAlarm(char *pszMsg, char *pszKey, Event *pEvent)
{
}


//
// Acknowlege alarm with given ID
//

void AlarmManager::AckById(DWORD dwAlarmId, DWORD dwUserId)
{
}


//
// Acknowlege all alarms with given key
//

void AlarmManager::AckByKey(char *pszKey)
{
}


//
// Delete alarm with given ID
//

void AlarmManager::DeleteAlarm(DWORD dwAlarmId)
{
}
