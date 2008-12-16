/* 
** NetXMS - Network Management System
** HTTP Server
** Copyright (C) 2006, 2007, 2008 Alex Kirhenshtein and Victor Kirhenshtein
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
** File: json.cpp
**
**/

#include "nxhttpd.h"


//
// Escape string
//

static void EscapeStringForJSON(String &str)
{
	int i;
	TCHAR chr[2], buffer[16];
   
	str.EscapeCharacter(_T('\\'), _T('\\'));
   str.EscapeCharacter(_T('"'), _T('\\'));
	
   str.Translate(_T("\r"), _T("\\r"));
   str.Translate(_T("\n"), _T("\\n"));
   str.Translate(_T("\t"), _T("\\t"));

	chr[1] = 0;
	for(i = 1; i < ' '; i++)
	{
		_sntprintf(buffer, 16, _T("\\u%04X"), i);
		chr[0] = i;
		str.Translate(chr, buffer);
	}
}


//
// JSON helpers
//

void json_set_dword(HttpResponse &response, int offset, const TCHAR *name, DWORD val, BOOL last)
{
	TCHAR buffer[128];

	_sntprintf(buffer, 128, _T("%*s\"%s\": %u%s\r\n"), offset, _T(""), name, val, last ? _T("") : _T(","));
	response.AppendBody(buffer);
}

void json_set_qword(HttpResponse &response, int offset, const TCHAR *name, QWORD val, BOOL last)
{
	TCHAR buffer[128];

	_sntprintf(buffer, 128, _T("%*s\"%s\": ") UINT64_FMT _T("%s\r\n"), offset, _T(""), name, val, last ? _T("") : _T(","));
	response.AppendBody(buffer);
}

void json_set_string(HttpResponse &response, int offset, const TCHAR *name, const TCHAR *val, BOOL last)
{
	TCHAR buffer[128];
	String temp;

	_sntprintf(buffer, 128, _T("%*s\"%s\": \""), offset, _T(""), name);
	response.AppendBody(buffer);
	temp = val;
	EscapeStringForJSON(temp);
	response.AppendBody(temp);
	_sntprintf(buffer, 128, _T("\"%s\r\n"), last ? _T("") : _T(","));
	response.AppendBody(buffer);
}


//
// Send alarm list
//

void ClientSession::JSON_SendAlarmList(HttpResponse &response)
{
	NXC_OBJECT *object, *rootObj = NULL;
	const TCHAR *name;
	DWORD i;
	BOOL needSep;

	response.AppendBody(_T("{\r\n"));
	json_set_dword(response, 2, _T("rcc"), RCC_SUCCESS, FALSE);
	response.AppendBody(_T("  \"alarmList\": [\r\n"));

	MutexLock(m_mutexAlarmList, INFINITE);
	for(i = 0, needSep = FALSE; i < m_dwNumAlarms; i++)
	{
		if (rootObj != NULL)
		{
			if (rootObj->dwId != m_pAlarmList[i].dwSourceObject)
				if (!NXCIsParent(m_hSession, rootObj->dwId, m_pAlarmList[i].dwSourceObject))
					continue;
		}
		object = NXCFindObjectById(m_hSession, m_pAlarmList[i].dwSourceObject);
		name = (object != NULL) ? object->szName : _T("<unknown>");

		if (needSep)
		{
			response.AppendBody(_T(",\r\n"));
		}
		else
		{
			needSep = TRUE;
		}
		response.AppendBody(_T("    {\r\n"));
		
		json_set_dword(response, 6, _T("id"), m_pAlarmList[i].dwAlarmId, FALSE);
		json_set_dword(response, 6, _T("sourceId"), m_pAlarmList[i].dwSourceObject, FALSE);
		json_set_string(response, 6, _T("sourceName"), name, FALSE);
		json_set_dword(response, 6, _T("creationTime"), m_pAlarmList[i].dwCreationTime, FALSE);
		json_set_dword(response, 6, _T("lastChangeTime"), m_pAlarmList[i].dwLastChangeTime, FALSE);
		json_set_dword(response, 6, _T("sourceEventCode"), m_pAlarmList[i].dwSourceEventCode, FALSE);
		json_set_qword(response, 6, _T("sourceEventId"), m_pAlarmList[i].qwSourceEventId, FALSE);
		json_set_dword(response, 6, _T("currentSeverity"), m_pAlarmList[i].nCurrentSeverity, FALSE);
		json_set_dword(response, 6, _T("originalSeverity"), m_pAlarmList[i].nOriginalSeverity, FALSE);
		json_set_dword(response, 6, _T("state"), m_pAlarmList[i].nState, FALSE);
		json_set_dword(response, 6, _T("helpdeskState"), m_pAlarmList[i].nHelpDeskState, FALSE);
		json_set_dword(response, 6, _T("ackByUser"), m_pAlarmList[i].dwAckByUser, FALSE);
		json_set_dword(response, 6, _T("terminateByUser"), m_pAlarmList[i].dwTermByUser, FALSE);
		json_set_dword(response, 6, _T("repeatCount"), m_pAlarmList[i].dwRepeatCount, FALSE);
		json_set_dword(response, 6, _T("timeout"), m_pAlarmList[i].dwTimeout, FALSE);
		json_set_dword(response, 6, _T("timeoutEvent"), m_pAlarmList[i].dwTimeoutEvent, FALSE);
		json_set_string(response, 6, _T("message"), m_pAlarmList[i].szMessage, FALSE);
		json_set_string(response, 6, _T("key"), m_pAlarmList[i].szKey, FALSE);
		json_set_string(response, 6, _T("helpdeskReference"), m_pAlarmList[i].szHelpDeskRef, TRUE);

		response.AppendBody(_T("    }")); 
	}
   MutexUnlock(m_mutexAlarmList);

	response.AppendBody(_T("\r\n  ]\r\n}\r\n"));
}


//
// Process requests to json module
//

BOOL ClientSession::ProcessJSONRequest(HttpRequest &request, HttpResponse &response)
{
	BOOL doLogout = FALSE;
	String cmd;

	if (request.GetQueryParam(_T("cmd"), cmd))
	{
		response.SetCode(HTTP_OK);
		response.SetBody(_T(""));
		//response.SetType("application/json");

		if (!_tcscmp(cmd, _T("getAlarms")))
		{
			JSON_SendAlarmList(response);
		}
		else
		{
			response.SetCode(HTTP_BADREQUEST);
			response.SetBody(_T("ERROR 400: Bad request"));
			response.SetType("text/plain");
		}
	}
	else
	{
		response.SetCode(HTTP_BADREQUEST);
		response.SetBody(_T("ERROR 400: Bad request"));
	}

	return doLogout;
}
