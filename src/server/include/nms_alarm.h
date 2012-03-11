/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006, 2007 Victor Kirhenshtein
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
** File: nms_alarm.h
**
**/

#ifndef _nms_alarm_h_
#define _nms_alarm_h_


//
// Alarm manager class
//

class AlarmManager
{
private:
   DWORD m_dwNumAlarms;
   NXC_ALARM *m_pAlarmList;
   MUTEX m_mutex;
   DWORD m_dwNotifyCode;
   NXC_ALARM *m_pNotifyAlarmInfo;
	CONDITION m_condShutdown;
	THREAD m_hWatchdogThread;

   void Lock() { MutexLock(m_mutex); }
   void Unlock() { MutexUnlock(m_mutex); }

   static void SendAlarmNotification(ClientSession *pSession, void *pArg);

   void UpdateAlarmInDB(NXC_ALARM *pAlarm);
   void NotifyClients(DWORD dwCode, NXC_ALARM *pAlarm);
   void UpdateObjectStatus(DWORD dwObjectId);

public:
   AlarmManager();
   ~AlarmManager();

	void WatchdogThread();

   BOOL Init();
   void NewAlarm(TCHAR *pszMsg, TCHAR *pszKey, int nState,
	              int iSeverity, DWORD dwTimeout, DWORD dwTimeoutEvent, Event *pEvent);
   DWORD AckById(DWORD dwAlarmId, DWORD dwUserId);
   DWORD TerminateById(DWORD dwAlarmId, DWORD dwUserId);
   void TerminateByKey(const TCHAR *key, bool useRegexp);
   void DeleteAlarm(DWORD dwAlarmId);

   void sendAlarmsToClient(DWORD dwRqId, ClientSession *pSession);

   NetObj *GetAlarmSourceObject(DWORD dwAlarmId);

   int GetMostCriticalStatusForObject(DWORD dwObjectId);

   void GetAlarmStats(CSCPMessage *pMsg);
};


//
// Functions
//

void FillAlarmInfoMessage(CSCPMessage *pMsg, NXC_ALARM *pAlarm);


//
// Global instance of alarm manager
//

extern AlarmManager g_alarmMgr;

#endif
