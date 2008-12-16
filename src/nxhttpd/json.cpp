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
// JSON helpers
//

static void jsonSetDWORD(HttpResponse &response, int offset, const TCHAR *name, DWORD val, BOOL last)
{
	TCHAR buffer[128];

	_sntprintf(buffer, 128, _T("%*s%s: %d%s\r\n"), offset, _T(""), name, val, last ? _T("") : _T(","));
	response.AppendBody(buffer);
}

static void jsonSetString(HttpResponse &response, int offset, const TCHAR *name, const TCHAR *val, BOOL last)
{
	TCHAR buffer[128];

	_sntprintf(buffer, 128, _T("%*s%s: \""), offset, _T(""), name);
	response.AppendBody(buffer);
	response.AppendBody(val);
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

	response.AppendBody(_T("{\r\n"));
	jsonSetDWORD(response, 2, _T("rcc"), 0, FALSE);
	response.AppendBody(_T("  \"alarmList\": [\r\n"));

	MutexLock(m_mutexAlarmList, INFINITE);
	for(i = 0; i < m_dwNumAlarms; i++)
	{
		if (rootObj != NULL)
		{
			if (rootObj->dwId != m_pAlarmList[i].dwSourceObject)
				if (!NXCIsParent(m_hSession, rootObj->dwId, m_pAlarmList[i].dwSourceObject))
					continue;
		}
		object = NXCFindObjectById(m_hSession, m_pAlarmList[i].dwSourceObject);
		name = (object != NULL) ? object->szName : _T("<unknown>");

		response.AppendBody(_T("    {\r\n"));
		
		jsonSetDWORD(response, 6, _T("id"), m_pAlarmList[i].dwAlarmId, FALSE);
		jsonSetDWORD(response, 6, _T("sourceId"), m_pAlarmList[i].dwSourceObject, FALSE);
		jsonSetString(response, 6, _T("sourceName"), name, FALSE);

		response.AppendBody(_T("    }")); 
	}
   MutexUnlock(m_mutexAlarmList);

	response.AppendBody(_T("  ]\r\n}\r\n"));
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
		response.SetType("application/json");

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
