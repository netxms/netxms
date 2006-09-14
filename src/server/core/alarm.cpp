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
   pMsg->SetVariable(VID_TERMINATE_BY_USER, pAlarm->dwTermByUser);
   pMsg->SetVariable(VID_EVENT_CODE, pAlarm->dwSourceEventCode);
   pMsg->SetVariable(VID_EVENT_ID, pAlarm->qwSourceEventId);
   pMsg->SetVariable(VID_OBJECT_ID, pAlarm->dwSourceObject);
   pMsg->SetVariable(VID_CREATION_TIME, pAlarm->dwCreationTime);
   pMsg->SetVariable(VID_LAST_CHANGE_TIME, pAlarm->dwLastChangeTime);
   pMsg->SetVariable(VID_ALARM_KEY, pAlarm->szKey);
   pMsg->SetVariable(VID_ALARM_MESSAGE, pAlarm->szMessage);
   pMsg->SetVariable(VID_STATE, (WORD)pAlarm->nState);
   pMsg->SetVariable(VID_CURRENT_SEVERITY, (WORD)pAlarm->nCurrentSeverity);
   pMsg->SetVariable(VID_ORIGINAL_SEVERITY, (WORD)pAlarm->nOriginalSeverity);
   pMsg->SetVariable(VID_HELPDESK_STATE, (WORD)pAlarm->nHelpDeskState);
   pMsg->SetVariable(VID_HELPDESK_REF, pAlarm->szHelpDeskRef);
   pMsg->SetVariable(VID_REPEAT_COUNT, pAlarm->dwRepeatCount);
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
   hResult = DBSelect(g_hCoreDB, "SELECT alarm_id,source_object_id,"
                                 "source_event_code,source_event_id,message,"
                                 "original_severity,current_severity,"
                                 "alarm_key,creation_time,last_change_time,"
                                 "hd_state,hd_ref,ack_by,repeat_count,"
                                 "alarm_state FROM alarms WHERE alarm_state<2");
   if (hResult == NULL)
      return FALSE;

   m_dwNumAlarms = DBGetNumRows(hResult);
   if (m_dwNumAlarms > 0)
   {
      m_pAlarmList = (NXC_ALARM *)malloc(sizeof(NXC_ALARM) * m_dwNumAlarms);
      for(i = 0; i < m_dwNumAlarms; i++)
      {
         m_pAlarmList[i].dwAlarmId = DBGetFieldULong(hResult, i, 0);
         m_pAlarmList[i].dwSourceObject = DBGetFieldULong(hResult, i, 1);
         m_pAlarmList[i].dwSourceEventCode = DBGetFieldULong(hResult, i, 2);
         m_pAlarmList[i].qwSourceEventId = DBGetFieldUInt64(hResult, i, 3);
         nx_strncpy(m_pAlarmList[i].szMessage, DBGetField(hResult, i, 4), MAX_DB_STRING);
         DecodeSQLString(m_pAlarmList[i].szMessage);
         m_pAlarmList[i].nOriginalSeverity = (BYTE)DBGetFieldLong(hResult, i, 5);
         m_pAlarmList[i].nCurrentSeverity = (BYTE)DBGetFieldLong(hResult, i, 6);
         nx_strncpy(m_pAlarmList[i].szKey, DBGetField(hResult, i, 7), MAX_DB_STRING);
         DecodeSQLString(m_pAlarmList[i].szKey);
         m_pAlarmList[i].dwCreationTime = DBGetFieldULong(hResult, i, 8);
         m_pAlarmList[i].dwLastChangeTime = DBGetFieldULong(hResult, i, 9);
         m_pAlarmList[i].nHelpDeskState = (BYTE)DBGetFieldLong(hResult, i, 10);
         nx_strncpy(m_pAlarmList[i].szHelpDeskRef, DBGetField(hResult, i, 11), MAX_HELPDESK_REF_LEN);
         DecodeSQLString(m_pAlarmList[i].szHelpDeskRef);
         m_pAlarmList[i].dwAckByUser = DBGetFieldULong(hResult, i, 12);
         m_pAlarmList[i].dwRepeatCount = DBGetFieldULong(hResult, i, 13);
         m_pAlarmList[i].nState = (BYTE)DBGetFieldLong(hResult, i, 14);
      }
   }

   DBFreeResult(hResult);
   return TRUE;
}


//
// Create new alarm
//

void AlarmManager::NewAlarm(char *pszMsg, char *pszKey, int nState,
                            int iSeverity, Event *pEvent)
{
   NXC_ALARM alarm;
   char *pszExpMsg, *pszExpKey, *pszEscRef, szQuery[2048];
   DWORD dwObjectId = 0;

   // Expand alarm's message and key
   pszExpMsg = pEvent->ExpandText(pszMsg);
   pszExpKey = pEvent->ExpandText(pszKey);

   // Create new alarm structure
   memset(&alarm, 0, sizeof(NXC_ALARM));
   alarm.dwAlarmId = CreateUniqueId(IDG_ALARM);
   alarm.qwSourceEventId = pEvent->Id();
   alarm.dwSourceEventCode = pEvent->Code();
   alarm.dwSourceObject = pEvent->SourceId();
   alarm.dwCreationTime = (DWORD)time(NULL);
   alarm.dwLastChangeTime = alarm.dwCreationTime;
   alarm.nState = nState;
   alarm.nOriginalSeverity = iSeverity;
   alarm.nCurrentSeverity = iSeverity;
   alarm.dwRepeatCount = 1;
   alarm.nHelpDeskState = ALARM_HELPDESK_IGNORED;
   nx_strncpy(alarm.szMessage, pszExpMsg, MAX_DB_STRING);
   nx_strncpy(alarm.szKey, pszExpKey, MAX_DB_STRING);
   free(pszExpMsg);
   free(pszExpKey);

   // Add new alarm to active alarm list if needed
   if (alarm.nState != ALARM_STATE_TERMINATED)
   {
      Lock();

      m_dwNumAlarms++;
      m_pAlarmList = (NXC_ALARM *)realloc(m_pAlarmList, sizeof(NXC_ALARM) * m_dwNumAlarms);
      memcpy(&m_pAlarmList[m_dwNumAlarms - 1], &alarm, sizeof(NXC_ALARM));
      dwObjectId = alarm.dwSourceObject;

      Unlock();
   }

   // Save alarm to database
   pszExpMsg = EncodeSQLString(alarm.szMessage);
   pszExpKey = EncodeSQLString(alarm.szKey);
   pszEscRef = EncodeSQLString(alarm.szHelpDeskRef);
   sprintf(szQuery, "INSERT INTO alarms (alarm_id,creation_time,last_change_time,"
                    "source_object_id,source_event_code,message,original_severity,"
                    "current_severity,alarm_key,alarm_state,ack_by,hd_state,"
                    "hd_ref,repeat_count,term_by,source_event_id) VALUES "
                    "(%d,%d,%d,%d,%d,'%s',%d,%d,'%s',%d,%d,%d,'%s',%d,%d"
#ifdef _WIN32
                    "%I64d)",
#else
                    "%lld)",
#endif
           alarm.dwAlarmId, alarm.dwCreationTime, alarm.dwLastChangeTime,
           alarm.dwSourceObject, alarm.dwSourceEventCode, pszExpMsg,
           alarm.nOriginalSeverity, alarm.nCurrentSeverity, pszExpKey,
           alarm.nState, alarm.dwAckByUser, alarm.nHelpDeskState, pszEscRef,
           alarm.dwRepeatCount, alarm.dwTermByUser, alarm.qwSourceEventId);
   free(pszExpMsg);
   free(pszExpKey);
   free(pszEscRef);
   //DBQuery(g_hCoreDB, szQuery);
   QueueSQLRequest(szQuery);

   // Notify connected clients about new alarm
   NotifyClients(NX_NOTIFY_NEW_ALARM, &alarm);

   // Update status of related object if needed
   if ((dwObjectId != 0) && (alarm.nState != ALARM_STATE_TERMINATED))
      UpdateObjectStatus(dwObjectId);
}


//
// Acknowlege alarm with given ID
//

/*void AlarmManager::AckById(DWORD dwAlarmId, DWORD dwUserId)
{
   DWORD i, dwObject;

   Lock();
   for(i = 0; i < m_dwNumAlarms; i++)
      if (m_pAlarmList[i].dwAlarmId == dwAlarmId)
      {
         dwObject = m_pAlarmList[i].dwSourceObject;
         NotifyClients(NX_NOTIFY_ALARM_TERMINATED, &m_pAlarmList[i]);
         SetAlarmStateInDB(m_pAlarmList[i].dwAlarmId, dwUserId, ALARM_STATE_TERMINATED);
         m_dwNumAlarms--;
         memmove(&m_pAlarmList[i], &m_pAlarmList[i + 1], sizeof(NXC_ALARM) * (m_dwNumAlarms - i));
         break;
      }
   Unlock();

   UpdateObjectStatus(dwObject);
}*/


//
// Terminate alarm with given ID
//

void AlarmManager::TerminateById(DWORD dwAlarmId, DWORD dwUserId)
{
   DWORD i, dwObject;

   Lock();
   for(i = 0; i < m_dwNumAlarms; i++)
      if (m_pAlarmList[i].dwAlarmId == dwAlarmId)
      {
         dwObject = m_pAlarmList[i].dwSourceObject;
         NotifyClients(NX_NOTIFY_ALARM_TERMINATED, &m_pAlarmList[i]);
         SetAlarmStateInDB(m_pAlarmList[i].dwAlarmId, dwUserId, ALARM_STATE_TERMINATED);
         m_dwNumAlarms--;
         memmove(&m_pAlarmList[i], &m_pAlarmList[i + 1], sizeof(NXC_ALARM) * (m_dwNumAlarms - i));
         break;
      }
   Unlock();

   UpdateObjectStatus(dwObject);
}


//
// Terminate all alarms with given key
//

void AlarmManager::TerminateByKey(char *pszKey)
{
   DWORD i, j, dwNumObjects, *pdwObjectList;

   pdwObjectList = (DWORD *)malloc(sizeof(DWORD) * m_dwNumAlarms);

   Lock();
   for(i = 0, dwNumObjects = 0; i < m_dwNumAlarms; i++)
      if (!strcmp(pszKey, m_pAlarmList[i].szKey))
      {
         // Add alarm's source object to update list
         for(j = 0; j < dwNumObjects; j++)
         {
            if (pdwObjectList[j] == m_pAlarmList[i].dwSourceObject)
               break;
         }
         if (j == dwNumObjects)
         {
            pdwObjectList[dwNumObjects++] = m_pAlarmList[i].dwSourceObject;
         }

         // Acknowlege alarm
         NotifyClients(NX_NOTIFY_ALARM_TERMINATED, &m_pAlarmList[i]);
         SetAlarmStateInDB(m_pAlarmList[i].dwAlarmId, 0, ALARM_STATE_TERMINATED);
         m_dwNumAlarms--;
         memmove(&m_pAlarmList[i], &m_pAlarmList[i + 1], sizeof(NXC_ALARM) * (m_dwNumAlarms - i));
         i--;
      }
   Unlock();

   // Update status of objects
   for(i = 0; i < dwNumObjects; i++)
      UpdateObjectStatus(pdwObjectList[i]);
   free(pdwObjectList);
}


//
// Delete alarm with given ID
//

void AlarmManager::DeleteAlarm(DWORD dwAlarmId)
{
   DWORD i, dwObject;
   char szQuery[256];

   // Delete alarm from in-memory list
   Lock();
   for(i = 0; i < m_dwNumAlarms; i++)
      if (m_pAlarmList[i].dwAlarmId == dwAlarmId)
      {
         dwObject = m_pAlarmList[i].dwSourceObject;
         NotifyClients(NX_NOTIFY_ALARM_DELETED, &m_pAlarmList[i]);
         m_dwNumAlarms--;
         memmove(&m_pAlarmList[i], &m_pAlarmList[i + 1], sizeof(NXC_ALARM) * (m_dwNumAlarms - i));
         break;
      }
   Unlock();

   // Delete from database
   sprintf(szQuery, "DELETE FROM alarms WHERE alarm_id=%d", dwAlarmId);
   //DBQuery(g_hCoreDB, szQuery);
   QueueSQLRequest(szQuery);

   UpdateObjectStatus(dwObject);
}


//
// Set state for record in database
//

void AlarmManager::SetAlarmStateInDB(DWORD dwAlarmId, DWORD dwUserId, int nState)
{
   char szQuery[256];

   sprintf(szQuery, "UPDATE alarms SET alarm_state=%d,%s=%d WHERE alarm_id=%d",
           nState, (nState == ALARM_STATE_TERMINATED) ? "term_by" : "ack_by",
           dwUserId, dwAlarmId);
   //DBQuery(g_hCoreDB, szQuery);
   QueueSQLRequest(szQuery);
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
      sprintf(szQuery, "SELECT source_object_id FROM alarms WHERE alarm_id=%d", dwAlarmId);
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


//
// Get most critical status among active alarms for given object
// Will return STATUS_UNKNOWN if there are no active alarms
//

int AlarmManager::GetMostCriticalStatusForObject(DWORD dwObjectId)
{
   DWORD i;
   int iStatus = STATUS_UNKNOWN;

   Lock();

   for(i = 0; i < m_dwNumAlarms; i++)
   {
      if ((m_pAlarmList[i].dwSourceObject == dwObjectId) &&
          ((m_pAlarmList[i].nCurrentSeverity > iStatus) || (iStatus == STATUS_UNKNOWN)))
      {
         iStatus = (int)m_pAlarmList[i].nCurrentSeverity;
      }
   }

   Unlock();
   return iStatus;
}


//
// Update object status after alarm acknowlegement or deletion
//

void AlarmManager::UpdateObjectStatus(DWORD dwObjectId)
{
   NetObj *pObject;

   pObject = FindObjectById(dwObjectId);
   if (pObject != NULL)
      pObject->CalculateCompoundStatus();
}


//
// Fill message with alarm stats
//

void AlarmManager::GetAlarmStats(CSCPMessage *pMsg)
{
   DWORD i, dwCount[5];

   Lock();
   pMsg->SetVariable(VID_NUM_ALARMS, m_dwNumAlarms);
   memset(dwCount, 0, sizeof(DWORD) * 5);
   for(i = 0; i < m_dwNumAlarms; i++)
      dwCount[m_pAlarmList[i].nCurrentSeverity]++;
   Unlock();
   pMsg->SetVariableToInt32Array(VID_ALARMS_BY_SEVERITY, 5, dwCount);
}
