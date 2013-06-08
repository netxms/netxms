/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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

/**
 * Alarm manager class
 */
class NXCORE_EXPORTABLE AlarmManager
{
private:
   DWORD m_dwNumAlarms;
   NXC_ALARM *m_pAlarmList;
   MUTEX m_mutex;
   DWORD m_dwNotifyCode;
   NXC_ALARM *m_pNotifyAlarmInfo;
	CONDITION m_condShutdown;
	THREAD m_hWatchdogThread;

   void lock() { MutexLock(m_mutex); }
   void unlock() { MutexUnlock(m_mutex); }

   static void sendAlarmNotification(ClientSession *pSession, void *pArg);

   void updateAlarmInDB(NXC_ALARM *pAlarm);
   void notifyClients(DWORD dwCode, NXC_ALARM *pAlarm);
   void updateObjectStatus(DWORD dwObjectId);

public:
   AlarmManager();
   ~AlarmManager();

	void watchdogThread();

   BOOL init();
   void newAlarm(TCHAR *pszMsg, TCHAR *pszKey, int nState,
	              int iSeverity, DWORD dwTimeout, DWORD dwTimeoutEvent, Event *pEvent);
   DWORD ackById(DWORD dwAlarmId, DWORD dwUserId, bool sticky);
   DWORD resolveById(DWORD dwAlarmId, DWORD dwUserId, bool terminate);
   void resolveByKey(const TCHAR *key, bool useRegexp, bool terminate);
   void deleteAlarm(DWORD dwAlarmId, bool objectCleanup);
   bool deleteObjectAlarms(DWORD objectId, DB_HANDLE hdb);
	DWORD updateAlarmNote(DWORD alarmId, DWORD noteId, const TCHAR *text, DWORD userId);

   DWORD getAlarm(DWORD dwAlarmId, CSCPMessage *msg);
   DWORD getAlarmEvents(DWORD dwAlarmId, CSCPMessage *msg);
   void sendAlarmsToClient(DWORD dwRqId, ClientSession *pSession);
   void getAlarmStats(CSCPMessage *pMsg);
	DWORD getAlarmNotes(DWORD alarmId, CSCPMessage *msg);

   NetObj *getAlarmSourceObject(DWORD dwAlarmId);
   int getMostCriticalStatusForObject(DWORD dwObjectId);

   ObjectArray<NXC_ALARM> *getAlarms(DWORD objectId);
};

/**
 * Functions
 */
void FillAlarmInfoMessage(CSCPMessage *pMsg, NXC_ALARM *pAlarm);
void DeleteAlarmNotes(DB_HANDLE hdb, DWORD alarmId);
void DeleteAlarmEvents(DB_HANDLE hdb, DWORD alarmId);

/**
 * Global instance of alarm manager
 */
extern AlarmManager NXCORE_EXPORTABLE g_alarmMgr;

#endif
