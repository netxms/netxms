/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2004, 2005, 2006, 2007 Victor Kirhenshtein
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

#include "libnxcl.h"


//
// Fill alarm record from message
//

static void AlarmFromMsg(CSCPMessage *pMsg, NXC_ALARM *pAlarm)
{
   pAlarm->dwAckByUser = pMsg->GetVariableLong(VID_ACK_BY_USER);
   pAlarm->dwTermByUser = pMsg->GetVariableLong(VID_TERMINATED_BY_USER);
   pAlarm->qwSourceEventId = pMsg->GetVariableInt64(VID_EVENT_ID);
   pAlarm->dwSourceEventCode = pMsg->GetVariableLong(VID_EVENT_CODE);
   pAlarm->dwSourceObject = pMsg->GetVariableLong(VID_OBJECT_ID);
   pAlarm->dwCreationTime = pMsg->GetVariableLong(VID_CREATION_TIME);
   pAlarm->dwLastChangeTime = pMsg->GetVariableLong(VID_LAST_CHANGE_TIME);
   pMsg->GetVariableStr(VID_ALARM_KEY, pAlarm->szKey, MAX_DB_STRING);
   pMsg->GetVariableStr(VID_ALARM_MESSAGE, pAlarm->szMessage, MAX_DB_STRING);
   pAlarm->nState = (BYTE)pMsg->GetVariableShort(VID_STATE);
   pAlarm->nCurrentSeverity = (BYTE)pMsg->GetVariableShort(VID_CURRENT_SEVERITY);
   pAlarm->nOriginalSeverity = (BYTE)pMsg->GetVariableShort(VID_ORIGINAL_SEVERITY);
   pAlarm->dwRepeatCount = pMsg->GetVariableLong(VID_REPEAT_COUNT);
   pAlarm->nHelpDeskState = (BYTE)pMsg->GetVariableShort(VID_HELPDESK_STATE);
   pMsg->GetVariableStr(VID_HELPDESK_REF, pAlarm->szHelpDeskRef, MAX_HELPDESK_REF_LEN);
	pAlarm->dwTimeout = pMsg->GetVariableLong(VID_ALARM_TIMEOUT);
	pAlarm->dwTimeoutEvent = pMsg->GetVariableLong(VID_ALARM_TIMEOUT_EVENT);
   pAlarm->pUserData = NULL;
}


//
// Process CMD_ALARM_UPDATE message
//

void ProcessAlarmUpdate(NXCL_Session *pSession, CSCPMessage *pMsg)
{
   NXC_ALARM alarm;
   DWORD dwCode;

   dwCode = pMsg->GetVariableLong(VID_NOTIFICATION_CODE);
   alarm.dwAlarmId = pMsg->GetVariableLong(VID_ALARM_ID);
   AlarmFromMsg(pMsg, &alarm);
   pSession->CallEventHandler(NXC_EVENT_NOTIFICATION, dwCode, &alarm);
}


//
// Load all alarms from server
//

DWORD LIBNXCL_EXPORTABLE NXCLoadAllAlarms(NXC_SESSION hSession, BOOL bIncludeAck,
                                          DWORD *pdwNumAlarms, NXC_ALARM **ppAlarmList)
{
   CSCPMessage msg, *pResponse;
   DWORD dwRqId, dwRetCode = RCC_SUCCESS, dwNumAlarms = 0, dwAlarmId = 0;
   NXC_ALARM *pList = NULL;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_GET_ALL_ALARMS);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_IS_ACK, (WORD)bIncludeAck);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   do
   {
      pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_ALARM_DATA, dwRqId);
      if (pResponse != NULL)
      {
         dwAlarmId = pResponse->GetVariableLong(VID_ALARM_ID);
         if (dwAlarmId != 0)  // 0 is end of list indicator
         {
            pList = (NXC_ALARM *)realloc(pList, sizeof(NXC_ALARM) * (dwNumAlarms + 1));
            pList[dwNumAlarms].dwAlarmId = dwAlarmId;
            AlarmFromMsg(pResponse, &pList[dwNumAlarms]);
            dwNumAlarms++;
         }
         delete pResponse;
      }
      else
      {
         dwRetCode = RCC_TIMEOUT;
         dwAlarmId = 0;
      }
   }
   while(dwAlarmId != 0);

   // Destroy results on failure or save on success
   if (dwRetCode == RCC_SUCCESS)
   {
      *ppAlarmList = pList;
      *pdwNumAlarms = dwNumAlarms;
   }
   else
   {
      safe_free(pList);
      *ppAlarmList = NULL;
      *pdwNumAlarms = 0;
   }

   return dwRetCode;
}


//
// Acknowledge alarm by ID
//

DWORD LIBNXCL_EXPORTABLE NXCAcknowledgeAlarm(NXC_SESSION hSession, DWORD dwAlarmId)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_ACK_ALARM);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_ALARM_ID, dwAlarmId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Terminate alarm by ID
//

DWORD LIBNXCL_EXPORTABLE NXCTerminateAlarm(NXC_SESSION hSession, DWORD dwAlarmId)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_TERMINATE_ALARM);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_ALARM_ID, dwAlarmId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Delete alarm by ID
//

DWORD LIBNXCL_EXPORTABLE NXCDeleteAlarm(NXC_SESSION hSession, DWORD dwAlarmId)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_DELETE_ALARM);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_ALARM_ID, dwAlarmId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}
