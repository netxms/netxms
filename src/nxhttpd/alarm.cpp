/* 
** NetXMS - Network Management System
** HTTP Server
** Copyright (C) 2006, 2007 Alex Kirhenshtein and Victor Kirhenshtein
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
** File: alarm.cpp
**
**/

#include "nxhttpd.h"


//
// Handler for alarm updates
//

void ClientSession::OnAlarmUpdate(DWORD dwCode, NXC_ALARM *pAlarm)
{
   NXC_ALARM *pListItem;

   MutexLock(m_mutexAlarmList, INFINITE);
   switch(dwCode)
   {
      case NX_NOTIFY_NEW_ALARM:
         if (pAlarm->nState != ALARM_STATE_TERMINATED)
         {
            AddAlarmToList(pAlarm);
         }
         break;
      case NX_NOTIFY_ALARM_CHANGED:
         pListItem = FindAlarmInList(pAlarm->dwAlarmId);
         if (pListItem != NULL)
         {
//            m_iNumAlarms[pListItem->nCurrentSeverity]--;
//            m_iNumAlarms[pAlarm->nCurrentSeverity]++;
            memcpy(pListItem, pAlarm, sizeof(NXC_ALARM));
         }
         break;
      case NX_NOTIFY_ALARM_TERMINATED:
      case NX_NOTIFY_ALARM_DELETED:
         DeleteAlarmFromList(pAlarm->dwAlarmId);
//         m_iNumAlarms[pAlarm->nCurrentSeverity]--;
         break;
      default:
         break;
   }
   MutexUnlock(m_mutexAlarmList);
}


//
// Delete alarm from internal alarms list
//

void ClientSession::DeleteAlarmFromList(DWORD dwAlarmId)
{
   DWORD i;

   for(i = 0; i < m_dwNumAlarms; i++)
      if (m_pAlarmList[i].dwAlarmId == dwAlarmId)
      {
         m_dwNumAlarms--;
         memmove(&m_pAlarmList[i], &m_pAlarmList[i + 1],
                 sizeof(NXC_ALARM) * (m_dwNumAlarms - i));
         break;
      }
}


//
// Add new alarm to internal list
//

void ClientSession::AddAlarmToList(NXC_ALARM *pAlarm)
{
   m_pAlarmList = (NXC_ALARM *)realloc(m_pAlarmList, sizeof(NXC_ALARM) * (m_dwNumAlarms + 1));
   memcpy(&m_pAlarmList[m_dwNumAlarms], pAlarm, sizeof(NXC_ALARM));
   m_dwNumAlarms++;
   //m_iNumAlarms[pAlarm->nCurrentSeverity]++;
}


//
// Find alarm record in internal list
//

NXC_ALARM *ClientSession::FindAlarmInList(DWORD dwAlarmId)
{
   DWORD i;

   for(i = 0; i < m_dwNumAlarms; i++)
      if (m_pAlarmList[i].dwAlarmId == dwAlarmId)
         return &m_pAlarmList[i];
   return NULL;
}


//
// Show alarm list
//

void ClientSession::ShowAlarmList(HttpResponse &response, NXC_OBJECT *pRootObj)
{
	DWORD i;
	String row, name, msg;
	NXC_OBJECT *pObject;

	//response.AppendBody(_T("<div id=\"alarm_list\">);
	response.StartBox(NULL, NULL, _T("alarm_list"));
	AddTableHeader(response, NULL,
	               _T("<input name=\"A0\" type=\"checkbox\"/>"), NULL,
	               _T("Severity"), NULL,
	               _T("State"), NULL,
	               _T("Source"), NULL,
	               _T("Message"), NULL,
	               _T("Count"), NULL,
	               _T("Created"), NULL,
	               _T("Changed"), NULL,
						NULL);

   MutexLock(m_mutexAlarmList, INFINITE);
	for(i = 0; i < m_dwNumAlarms; i++)
	{
		if (pRootObj != NULL)
		{

		}
		pObject = NXCFindObjectById(m_hSession, m_pAlarmList[i].dwSourceObject);
		row = _T("");
		name = (pObject != NULL) ? pObject->szName : _T("<unknown>");
		msg = m_pAlarmList[i].szMessage;
		row.AddFormattedString(_T("<td><input name=\"A%d\" type=\"checkbox\"/></td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%d</td><td>%s</td><td>%s</td></tr>"),
		                       m_pAlarmList[i].dwAlarmId,
		                       g_szStatusTextSmall[m_pAlarmList[i].nCurrentSeverity],
									  g_szAlarmState[m_pAlarmList[i].nState],
									  EscapeHTMLText(name), EscapeHTMLText(msg),
									  m_pAlarmList[i].dwRepeatCount,
									  "tt", "tt");
		response.StartBoxRow();
		response.AppendBody(row);
	}
   MutexUnlock(m_mutexAlarmList);

	response.AppendBody(_T("</table></div>"));

/*	response.AppendBody(
		_T("<script type=\"text/javascript\" src=\"/sortabletable.js\"></script>\r\n")
		_T("<script type=\"text/javascript\" src=\"/columnlist.js\"></script>\r\n")
		_T("<div>\r\n")
		_T("	<div id=\"alarm_container\" class=\"webfx-columnlist\" style=\"width: 98%;\">\r\n")
		_T("		<div id=\"alarm_head\" class=\"webfx-columnlist-head\">\r\n")
		_T("			<table cellspacing=\"0\" cellpadding=\"0\">\r\n")
		_T("				<tr>\r\n")
		_T("					<td>Severity<img src=\"/images/asc.png\"/></td><td>State<img src=\"/images/asc.png\"/></td><td>Source<img src=\"/images/asc.png\"/></td><td>Message<img src=\"/images/asc.png\"/></td><td>Count<img src=\"/images/asc.png\"/></td><td>Created<img src=\"/images/asc.png\"/></td><td>Last Change<img src=\"/images/asc.png\"/></td>\r\n")
		_T("				</tr>\r\n")
		_T("			</table>\r\n")
		_T("		</div>\r\n")
		_T("		<div id=\"alarm_body\" class=\"webfx-columnlist-body\">\r\n")
		_T("			<table cellspacing=\"0\" cellpadding=\"0\">\r\n")
		_T("				<colgroup span=\"7\">\r\n")
		_T("					<col style=\"width: 10%;\" />\r\n")
		_T("					<col style=\"width: 10%;\" />\r\n")
		_T("					<col style=\"width: 20%;\" />\r\n")
		_T("					<col style=\"width: 35%;\" />\r\n")
		_T("					<col style=\"width: 5%;\" />\r\n")
		_T("					<col style=\"width: 10%;\" />\r\n")
		_T("					<col style=\"width: 10%;\" />\r\n")
		_T("				</colgroup>\r\n")
	);

   MutexLock(m_mutexAlarmList, INFINITE);
	for(i = 0; i < m_dwNumAlarms; i++)
	{
		if (pRootObj != NULL)
		{

		}
		pObject = NXCFindObjectById(m_hSession, m_pAlarmList[i].dwSourceObject);
		row = _T("");
		name = (pObject != NULL) ? pObject->szName : _T("<unknown>");
		msg = m_pAlarmList[i].szMessage;
		row.AddFormattedString(_T("<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%d</td><td>%s</td><td>%s</td></tr>"),
		                       g_szStatusTextSmall[m_pAlarmList[i].nCurrentSeverity],
									  g_szAlarmState[m_pAlarmList[i].nState],
									  EscapeHTMLText(name), EscapeHTMLText(msg),
									  m_pAlarmList[i].dwRepeatCount,
									  "tt", "tt");
		response.AppendBody(row);
	}
   MutexUnlock(m_mutexAlarmList);

	response.AppendBody(
		_T("			</table>\r\n")
		_T("		</div>\r\n")
		_T("	</div>\r\n")
		_T("	<script type=\"text/javascript\">\r\n")
		_T("		var alarmList = new WebFXColumnList();\r\n")
		_T("		alarmList.moveColumns = false;\r\n")
		_T("		alarmList.bind(document.getElementById('alarm_container'),\r\n")
		_T("		               document.getElementById('alarm_head'),\r\n")
		_T("		               document.getElementById('alarm_body'));\r\n")
		_T("		alarmList.setSortTypes([TYPE_STRING, TYPE_STRING, TYPE_STRING, TYPE_STRING, TYPE_NUMBER, TYPE_STRING, TYPE_STRING]);\r\n")
		_T("		alarmList._setAlignment();\r\n")
		_T("	</script>\r\n")
		_T("</div>\r\n")
	);*/
}
