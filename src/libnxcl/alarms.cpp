/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2004 Victor Kirhenshtein
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
** $module: alarms.cpp
**
**/

#include "libnxcl.h"


//
// Load all alarms from server
//

DWORD LIBNXCL_EXPORTABLE NXCLoadAllAlarms(BOOL bIncludeAck, DWORD *pdwNumAlarms, 
                                          NXC_ALARM **ppAlarmList)
{
   CSCPMessage msg, *pResponce;
   DWORD dwRqId, dwRetCode = RCC_SUCCESS, dwNumAlarms = 0, dwAlarmId = 0;
   NXC_ALARM *pList = NULL;

   dwRqId = g_dwMsgId++;

   msg.SetCode(CMD_GET_ALL_ALARMS);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_IS_ACK, (WORD)bIncludeAck);
   SendMsg(&msg);

   do
   {
      pResponce = WaitForMessage(CMD_ALARM_DATA, dwRqId, 2000);
      if (pResponce != NULL)
      {
         dwAlarmId = pResponce->GetVariableLong(VID_ALARM_ID);
         if (dwAlarmId != 0)  // 0 is end of list indicator
         {
            pList = (NXC_ALARM *)MemReAlloc(pList, sizeof(NXC_ALARM) * (dwNumAlarms + 1));
            pList[dwNumAlarms].dwAlarmId = dwAlarmId;
            pList[dwNumAlarms].dwAckByUser = pResponce->GetVariableLong(VID_ACK_BY_USER);
            pList[dwNumAlarms].dwSourceEvent = pResponce->GetVariableLong(VID_EVENT_ID);
            pList[dwNumAlarms].dwSourceObject = pResponce->GetVariableLong(VID_OBJECT_ID);
            pList[dwNumAlarms].dwTimeStamp = pResponce->GetVariableLong(VID_TIMESTAMP);
            pResponce->GetVariableStr(VID_ALARM_KEY, pList[dwNumAlarms].szKey, MAX_DB_STRING);
            pResponce->GetVariableStr(VID_ALARM_MESSAGE, pList[dwNumAlarms].szMessage, MAX_DB_STRING);
            pList[dwNumAlarms].wIsAck = pResponce->GetVariableShort(VID_IS_ACK);
            pList[dwNumAlarms].wSeverity = pResponce->GetVariableShort(VID_SEVERITY);
            dwNumAlarms++;
         }
         delete pResponce;
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
      MemFree(pList);
      *ppAlarmList = NULL;
      *pdwNumAlarms = 0;
   }

   return dwRetCode;
}


//
// Acknowlege alarm by ID
//

DWORD LIBNXCL_EXPORTABLE NXCAcknowlegeAlarm(DWORD dwAlarmId)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = g_dwMsgId++;

   msg.SetCode(CMD_ACK_ALARM);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_ALARM_ID, dwAlarmId);
   SendMsg(&msg);

   return WaitForRCC(dwRqId);
}


//
// Delete alarm by ID
//

DWORD LIBNXCL_EXPORTABLE NXCDeleteAlarm(DWORD dwAlarmId)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = g_dwMsgId++;

   msg.SetCode(CMD_DELETE_ALARM);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_ALARM_ID, dwAlarmId);
   SendMsg(&msg);

   return WaitForRCC(dwRqId);
}
