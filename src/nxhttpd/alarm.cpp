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
// Compare alarms - QSortEx callback
//

static int CompareAlarmsCB(const void *p1, const void *p2, void *pArg)
{
	return ((ClientSession *)pArg)->CompareAlarms((NXC_ALARM *)p1, (NXC_ALARM *)p2);
}


//
// Compare alarms
//

int ClientSession::CompareAlarms(NXC_ALARM *p1, NXC_ALARM *p2)
{
	int nRet;
	NXC_OBJECT *pObj1, *pObj2;

	switch(m_nAlarmSortMode)
	{
		case 0:	// Severity
			nRet = COMPARE_NUMBERS(p1->nCurrentSeverity, p2->nCurrentSeverity);
			break;
		case 1:	// State
			nRet = COMPARE_NUMBERS(p1->nState, p2->nState);
			break;
		case 2:	// Source
			pObj1 = NXCFindObjectById(m_hSession, p1->dwSourceObject);
			pObj2 = NXCFindObjectById(m_hSession, p2->dwSourceObject);
			nRet = _tcsicmp((pObj1 != NULL) ? pObj1->szName : _T("(null)"), (pObj2 != NULL) ? pObj2->szName : _T("(null)"));
			break;
		case 3:	// Message
			nRet = _tcsicmp(p1->szMessage, p2->szMessage);
			break;
		case 4:	// Count
			nRet = COMPARE_NUMBERS(p1->dwRepeatCount, p2->dwRepeatCount);
			break;
		case 5:	// Created
			nRet = COMPARE_NUMBERS(p1->dwCreationTime, p2->dwCreationTime);
			break;
		case 6:	// Changed
			nRet = COMPARE_NUMBERS(p1->dwLastChangeTime, p2->dwLastChangeTime);
			break;
		default:
			nRet = 0;
			break;
	}
	return nRet * m_nAlarmSortDir;
}


//
// Add alarm table column header
//

static void AddAlarmColumnHeader(HttpResponse &response, TCHAR *sid, int nCol, DWORD dwObjectId,
										   int nSortDir, BOOL bSortBy)
{
	TCHAR szTemp[1024];
	static TCHAR *colNames[] = 
	{ 
		_T("Severity"),
		_T("State"),
		_T("Source"),
		_T("Message"),
		_T("Count"),
		_T("Created"),
		_T("Changed")
	};

	_sntprintf(szTemp, 1024, _T("<td valign=\"center\"><a href=\"\" onclick=\"javascript:loadDivContent('alarm_list','%s','&cmd=getAlarmList&mode=%d&dir=%d&obj=%d'); return false;\">%s</a>%s</td>\r\n"),
	           sid, nCol, bSortBy ? -nSortDir : nSortDir, dwObjectId, colNames[nCol],
				  (!bSortBy) ? _T("") : ((nSortDir < 0) ? _T("&nbsp;<img src=\"/images/sort_down.png\"/>") : _T("&nbsp;<img src=\"/images/sort_up.png\"/>")));
	response.AppendBody(szTemp);
}


//
// Show alarm list
//

void ClientSession::ShowAlarmList(HttpResponse &response, NXC_OBJECT *pRootObj, BOOL bReload)
{
	DWORD i;
	String row, name, msg;
	NXC_OBJECT *pObject;
	TCHAR szTemp1[64], szTemp2[64];

	response.StartBox(NULL, _T("objectTable"), _T("alarm_list"), bReload);
	response.StartTableHeader(NULL);
	response.AppendBody(_T("<td></td>"));
	for(i = 0; i < 7; i++)
		AddAlarmColumnHeader(response, m_sid, i, (pRootObj != NULL) ? pRootObj->dwId : 0,
		                     m_nAlarmSortDir, (m_nAlarmSortMode == (int)i));
	response.AppendBody(_T("</tr>\r\n"));

   MutexLock(m_mutexAlarmList, INFINITE);
	QSortEx(m_pAlarmList, m_dwNumAlarms, sizeof(NXC_ALARM), this, CompareAlarmsCB);
	for(i = 0; i < m_dwNumAlarms; i++)
	{
		if (pRootObj != NULL)
		{
			if (pRootObj->dwId != m_pAlarmList[i].dwSourceObject)
				if (!NXCIsParent(m_hSession, pRootObj->dwId, m_pAlarmList[i].dwSourceObject))
					continue;
		}
		pObject = NXCFindObjectById(m_hSession, m_pAlarmList[i].dwSourceObject);
		row = _T("");
		name = (pObject != NULL) ? pObject->szName : _T("<unknown>");
		msg = m_pAlarmList[i].szMessage;
		row.AddFormattedString(_T("<td><input name=\"A%d\" type=\"checkbox\"/></td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%d</td><td>%s</td><td>%s</td></tr>\r\n"),
		                       m_pAlarmList[i].dwAlarmId,
		                       g_szStatusTextSmall[m_pAlarmList[i].nCurrentSeverity],
									  g_szAlarmState[m_pAlarmList[i].nState],
									  EscapeHTMLText(name), EscapeHTMLText(msg),
									  m_pAlarmList[i].dwRepeatCount,
									  FormatTimeStamp(m_pAlarmList[i].dwCreationTime, szTemp1, TS_LONG_DATE_TIME),
									  FormatTimeStamp(m_pAlarmList[i].dwLastChangeTime, szTemp2, TS_LONG_DATE_TIME));
		response.StartBoxRow();
		response.AppendBody(row);
	}
   MutexUnlock(m_mutexAlarmList);
	response.EndBox(bReload);
}


//
// Send alarm list in a response to reload request
//

void ClientSession::SendAlarmList(HttpRequest &request, HttpResponse &response)
{
	NXC_OBJECT *pObject;
	DWORD dwId;
	String tmp;

	if (request.GetQueryParam(_T("mode"), tmp))
		m_nAlarmSortMode = _tcstol(tmp, NULL, 10);
	if (request.GetQueryParam(_T("dir"), tmp))
		m_nAlarmSortDir = _tcstol(tmp, NULL, 10);

	if (request.GetQueryParam(_T("obj"), tmp))
	{
		dwId = _tcstoul(tmp, NULL, 10);
		if (dwId != 0)
		{
			pObject = NXCFindObjectById(m_hSession, dwId);
			if (pObject != NULL)
			{
				ShowAlarmList(response, pObject, TRUE);
			}
		}
		else
		{
			ShowAlarmList(response, NULL, TRUE);
		}
	}
}
