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

#include "nxcore.h"


//
// Global instance of alarm manager
//

AlarmManager g_alarmMgr;


//
// Fill CSCP message with alarm data
//

void FillAlarmInfoMessage(CSCPMessage *pMsg, NXC_ALARM *pAlarm)
{
   pMsg->SetVariable(VID_ALARM_ID, pAlarm->dwAlarmId);
   pMsg->SetVariable(VID_ACK_BY_USER, pAlarm->dwAckByUser);
   pMsg->SetVariable(VID_EVENT_ID, pAlarm->dwSourceEvent);
   pMsg->SetVariable(VID_OBJECT_ID, pAlarm->dwSourceObject);
   pMsg->SetVariable(VID_TIMESTAMP, pAlarm->dwTimeStamp);
   pMsg->SetVariable(VID_ALARM_KEY, pAlarm->szKey);
   pMsg->SetVariable(VID_ALARM_MESSAGE, pAlarm->szMessage);
   pMsg->SetVariable(VID_IS_ACK, pAlarm->wIsAck);
   pMsg->SetVariable(VID_SEVERITY, pAlarm->wSeverity);
}


//
// Alarm manager constructor
//

AlarmManager::AlarmManager()
{
   m_dwNumAlarms = 0;
   m_pAlarmList = NULL;
   m_mutex = MutexCreate();
}


//
// Alarm manager destructor
//

AlarmManager::~AlarmManager()
{
   safe_free(m_pAlarmList);
   MutexDestroy(m_mutex);
}


//
// Initialize alarm manager at system startup
//

BOOL AlarmManager::Init(void)
{
   DB_RESULT hResult;
   DWORD i;

   // Load unacknowleged alarms into memory
   hResult = DBSelect(g_hCoreDB, "SELECT alarm_id,alarm_timestamp,source_object_id,source_event_id,"
                                 "message,severity,alarm_key FROM alarms WHERE is_ack=0");
   if (hResult == NULL)
      return FALSE;

   m_dwNumAlarms = DBGetNumRows(hResult);
   if (m_dwNumAlarms > 0)
   {
      m_pAlarmList = (NXC_ALARM *)malloc(sizeof(NXC_ALARM) * m_dwNumAlarms);
      for(i = 0; i < m_dwNumAlarms; i++)
      {
         m_pAlarmList[i].dwAlarmId = DBGetFieldULong(hResult, i, 0);
         m_pAlarmList[i].dwTimeStamp = DBGetFieldULong(hResult, i, 1);
         m_pAlarmList[i].dwSourceObject = DBGetFieldULong(hResult, i, 2);
         m_pAlarmList[i].dwSourceEvent = DBGetFieldULong(hResult, i, 3);
         strncpy(m_pAlarmList[i].szMessage, DBGetField(hResult, i, 4), MAX_DB_STRING);
         DecodeSQLString(m_pAlarmList[i].szMessage);
         m_pAlarmList[i].wSeverity = (WORD)DBGetFieldLong(hResult, i, 5);
         strncpy(m_pAlarmList[i].szKey, DBGetField(hResult, i, 6), MAX_DB_STRING);
         DecodeSQLString(m_pAlarmList[i].szKey);
         m_pAlarmList[i].wIsAck = 0;
         m_pAlarmList[i].dwAckByUser = 0;
      }
   }

   DBFreeResult(hResult);
   return TRUE;
}


//
// Create new alarm
//

void AlarmManager::NewAlarm(char *pszMsg, char *pszKey, BOOL bIsAck, int iSeverity, Event *pEvent)
{
   NXC_ALARM alarm;
   char *pszExpMsg, *pszExpKey, szQuery[2048];

   // Expand alarm's message and key
   pszExpMsg = pEvent->ExpandText(pszMsg);
   pszExpKey = pEvent->ExpandText(pszKey);

   // Create new alarm structure
   alarm.dwAlarmId = CreateUniqueId(IDG_ALARM);
   alarm.dwSourceEvent = pEvent->Id();
   alarm.dwSourceObject = pEvent->SourceId();
   alarm.dwTimeStamp = time(NULL);
   alarm.dwAckByUser = 0;
   alarm.wIsAck = bIsAck;
   alarm.wSeverity = iSeverity;
   strncpy(alarm.szMessage, pszExpMsg, MAX_DB_STRING);
   strncpy(alarm.szKey, pszExpKey, MAX_DB_STRING);
   free(pszExpMsg);
   free(pszExpKey);

   // Add new alarm to active alarm list if needed
   if (!alarm.wIsAck)
   {
      Lock();

      m_dwNumAlarms++;
      m_pAlarmList = (NXC_ALARM *)realloc(m_pAlarmList, sizeof(NXC_ALARM) * m_dwNumAlarms);
      memcpy(&m_pAlarmList[m_dwNumAlarms - 1], &alarm, sizeof(NXC_ALARM));

      Unlock();
   }

   // Save alarm to database
   pszExpMsg = EncodeSQLString(alarm.szMessage);
   pszExpKey = EncodeSQLString(alarm.szKey);
   sprintf(szQuery, "INSERT INTO alarms (alarm_id,alarm_timestamp,source_object_id,"
                    "source_event_id,message,severity,alarm_key,is_ack,ack_by)"
                    " VALUES (%ld,%ld,%ld,%ld,'%s',%d,'%s',%d,%ld)",
           alarm.dwAlarmId, alarm.dwTimeStamp, alarm.dwSourceObject, alarm.dwSourceEvent,
           pszExpMsg, alarm.wSeverity, pszExpKey, alarm.wIsAck, alarm.dwAckByUser);
   free(pszExpMsg);
   free(pszExpKey);
   DBQuery(g_hCoreDB, szQuery);

   // Notify connected clients about new alarm
   NotifyClients(NX_NOTIFY_NEW_ALARM, &alarm);
}


//
// Acknowlege alarm with given ID
//

void AlarmManager::AckById(DWORD dwAlarmId, DWORD dwUserId)
{
   DWORD i;

   Lock();
   for(i = 0; i < m_dwNumAlarms; i++)
      if (m_pAlarmList[i].dwAlarmId == dwAlarmId)
      {
         NotifyClients(NX_NOTIFY_ALARM_ACKNOWLEGED, &m_pAlarmList[i]);
         AckAlarmInDB(m_pAlarmList[i].dwAlarmId, dwUserId);
         m_dwNumAlarms--;
         memmove(&m_pAlarmList[i], &m_pAlarmList[i + 1], sizeof(NXC_ALARM) * (m_dwNumAlarms - i));
         break;
      }
   Unlock();
}


//
// Acknowlege all alarms with given key
//

void AlarmManager::AckByKey(char *pszKey)
{
   DWORD i;

   Lock();
   for(i = 0; i < m_dwNumAlarms; i++)
      if (!strcmp(pszKey, m_pAlarmList[i].szKey))
      {
         NotifyClients(NX_NOTIFY_ALARM_ACKNOWLEGED, &m_pAlarmList[i]);
         AckAlarmInDB(m_pAlarmList[i].dwAlarmId, 0);
         m_dwNumAlarms--;
         memmove(&m_pAlarmList[i], &m_pAlarmList[i + 1], sizeof(NXC_ALARM) * (m_dwNumAlarms - i));
         i--;
      }
   Unlock();
}


//
// Delete alarm with given ID
//

void AlarmManager::DeleteAlarm(DWORD dwAlarmId)
{
   DWORD i;
   char szQuery[256];

   // Delete alarm from in-memory list
   Lock();
   for(i = 0; i < m_dwNumAlarms; i++)
      if (m_pAlarmList[i].dwAlarmId == dwAlarmId)
      {
         NotifyClients(NX_NOTIFY_ALARM_DELETED, &m_pAlarmList[i]);
         m_dwNumAlarms--;
         memmove(&m_pAlarmList[i], &m_pAlarmList[i + 1], sizeof(NXC_ALARM) * (m_dwNumAlarms - i));
         break;
      }
   Unlock();

   // Delete from database
   sprintf(szQuery, "DELETE FROM alarms WHERE alarm_id=%ld", dwAlarmId);
   DBQuery(g_hCoreDB, szQuery);
}


//
// Set ack flag for record in database
//

void AlarmManager::AckAlarmInDB(DWORD dwAlarmId, DWORD dwUserId)
{
   char szQuery[256];

   sprintf(szQuery, "UPDATE alarms SET is_ack=1,ack_by=%ld WHERE alarm_id=%ld",
           dwUserId, dwAlarmId);
   DBQuery(g_hCoreDB, szQuery);
}


//
// Callback for client session enumeration
//

void AlarmManager::SendAlarmNotification(ClientSession *pSession, void *pArg)
{
   pSession->OnAlarmUpdate(((AlarmManager *)pArg)->m_dwNotifyCode,
                           ((AlarmManager *)pArg)->m_pNotifyAlarmInfo);
}


//
// Notify connected clients about changes
//

void AlarmManager::NotifyClients(DWORD dwCode, NXC_ALARM *pAlarm)
{
   m_dwNotifyCode = dwCode;
   m_pNotifyAlarmInfo = pAlarm;
   EnumerateClientSessions(SendAlarmNotification, this);
}


//
// Send all alarms to client
//

void AlarmManager::SendAlarmsToClient(DWORD dwRqId, BOOL bIncludeAck, ClientSession *pSession)
{
   DWORD i, dwUserId;
   NetObj *pObject;
   CSCPMessage msg;

   dwUserId = pSession->GetUserId();

   // Prepare message
   msg.SetCode(CMD_ALARM_DATA);
   msg.SetId(dwRqId);

   if (bIncludeAck)
   {
      // Request for all alarms including acknowleged,
      // so we have to load them from database
   }
   else
   {
      // Unacknowleged alarms can be sent directly from memory
      Lock();

      for(i = 0; i < m_dwNumAlarms; i++)
      {
         pObject = FindObjectById(m_pAlarmList[i].dwSourceObject);
         if (pObject != NULL)
         {
            if (pObject->CheckAccessRights(dwUserId, OBJECT_ACCESS_READ_ALARMS))
            {
               FillAlarmInfoMessage(&msg, &m_pAlarmList[i]);
               pSession->SendMessage(&msg);
               msg.DeleteAllVariables();
            }
         }
      }

      Unlock();
   }

   // Send end-of-list indicator
   msg.SetVariable(VID_ALARM_ID, (DWORD)0);
   pSession->SendMessage(&msg);
}


//
// Get source object for given alarm id
//

NetObj *AlarmManager::GetAlarmSourceObject(DWORD dwAlarmId)
{
   DWORD i, dwObjectId = 0;
   char szQuery[256];
   DB_RESULT hResult;

   // First, look at our in-memory list
   Lock();
   for(i = 0; i < m_dwNumAlarms; i++)
      if (m_pAlarmList[i].dwAlarmId == dwAlarmId)
      {
         dwObjectId = m_pAlarmList[i].dwSourceObject;
         break;
      }
   Unlock();

   // If not found, search database
   if (i == m_dwNumAlarms)
   {
      sprintf(szQuery, "SELECT source_object_id FROM alarms WHERE alarm_id=%ld", dwAlarmId);
      hResult = DBSelect(g_hCoreDB, szQuery);
      if (hResult != NULL)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            dwObjectId = DBGetFieldULong(hResult, 0, 0);
         }
         DBFreeResult(hResult);
      }
   }

   return FindObjectById(dwObjectId);
}
