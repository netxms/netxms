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
// Alarm update actions
//

#define AU_ACK          0
#define AU_TERMINATE    1


//
// Send alarm list
//

void ClientSession::JSON_SendAlarmList(JSONObjectBuilder &json)
{
	NXC_OBJECT *object, *rootObj = NULL;
	const TCHAR *name;
	DWORD i;

	json.AddRCC(RCC_SUCCESS);
	json.StartArray(_T("alarmList"));

	MutexLock(m_mutexAlarmList);
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

		json.StartObject(NULL);
		json.AddUInt32(_T("id"), m_pAlarmList[i].dwAlarmId);
		json.AddUInt32(_T("sourceId"), m_pAlarmList[i].dwSourceObject);
		json.AddString(_T("sourceName"), name);
		json.AddUInt32(_T("creationTime"), m_pAlarmList[i].dwCreationTime);
		json.AddUInt32(_T("lastChangeTime"), m_pAlarmList[i].dwLastChangeTime);
		json.AddUInt32(_T("sourceEventCode"), m_pAlarmList[i].dwSourceEventCode);
		json.AddUInt64(_T("sourceEventId"), m_pAlarmList[i].qwSourceEventId);
		json.AddInt32(_T("currentSeverity"), m_pAlarmList[i].nCurrentSeverity);
		json.AddInt32(_T("originalSeverity"), m_pAlarmList[i].nOriginalSeverity);
		json.AddInt32(_T("state"), m_pAlarmList[i].nState);
		json.AddInt32(_T("helpdeskState"), m_pAlarmList[i].nHelpDeskState);
		json.AddUInt32(_T("ackByUser"), m_pAlarmList[i].dwAckByUser);
		json.AddUInt32(_T("terminateByUser"), m_pAlarmList[i].dwTermByUser);
		json.AddUInt32(_T("repeatCount"), m_pAlarmList[i].dwRepeatCount);
		json.AddUInt32(_T("timeout"), m_pAlarmList[i].dwTimeout);
		json.AddUInt32(_T("timeoutEvent"), m_pAlarmList[i].dwTimeoutEvent);
		json.AddString(_T("message"), m_pAlarmList[i].szMessage);
		json.AddString(_T("key"), m_pAlarmList[i].szKey);
		json.AddString(_T("helpdeskReference"), m_pAlarmList[i].szHelpDeskRef);
		json.EndObject();
	}
   MutexUnlock(m_mutexAlarmList);
	json.EndArray();
}


//
// Send user list to client
//

void ClientSession::JSON_SendUserList(JSONObjectBuilder &json)
{
	DWORD dwNumUsers, i, j;
	NXC_USER *pUserList;

	if (NXCGetUserDB(m_hSession, &pUserList, &dwNumUsers))
	{
		json.AddRCC(RCC_SUCCESS);

		// Users
		json.StartArray(_T("userList"));
		for(i = 0; i < dwNumUsers; i++)
		{
			if (!(pUserList[i].dwId & GROUP_FLAG))
			{
				json.StartObject(NULL);
				json.AddUInt32(_T("id"), pUserList[i].dwId);
				json.AddGUID(_T("guid"), pUserList[i].guid);
				json.AddUInt32(_T("flags"), pUserList[i].wFlags);
				json.AddUInt32(_T("systemRights"), pUserList[i].dwSystemRights);
				json.AddString(_T("name"), pUserList[i].szName);
				json.AddString(_T("fullName"), pUserList[i].szFullName);
				json.AddString(_T("description"), pUserList[i].szDescription);
				json.AddInt32(_T("authMethod"), pUserList[i].nAuthMethod);
				json.AddString(_T("certMappingData"), CHECK_NULL_EX(pUserList[i].pszCertMappingData));
				json.AddInt32(_T("certMappingMethod"), pUserList[i].nCertMappingMethod);
				json.EndObject();
			}
		}
		json.EndArray();

		// Groups
		json.StartArray(_T("groupList"));
		for(i = 0; i < dwNumUsers; i++)
		{
			if (pUserList[i].dwId & GROUP_FLAG)
			{
				json.StartObject(NULL);
				json.AddUInt32(_T("id"), pUserList[i].dwId);
				json.AddGUID(_T("guid"), pUserList[i].guid);
				json.AddUInt32(_T("flags"), pUserList[i].wFlags);
				json.AddUInt32(_T("systemRights"), pUserList[i].dwSystemRights);
				json.AddString(_T("name"), pUserList[i].szName);
				json.AddString(_T("fullName"), pUserList[i].szFullName);
				json.AddString(_T("description"), pUserList[i].szDescription);
				json.StartArray(_T("members"));
				for(j = 0; j < pUserList[i].dwNumMembers; j++)
					json.AddUInt32(NULL, pUserList[i].pdwMemberList[j]);
				json.EndArray();
				json.EndObject();
			}
		}
		json.EndArray();
	}
	else
	{
		json.AddRCC(RCC_ACCESS_DENIED);
	}
}


//
// Update alarms (acknowledge, terminate, etc.)
//

void ClientSession::JSON_UpdateAlarms(HttpRequest &request, int action, JSONObjectBuilder &json)
{
	
}


//
// Process requests to json module
//

BOOL ClientSession::ProcessJSONRequest(HttpRequest &request, HttpResponse &response)
{
	BOOL keepSession = TRUE;
	String cmd;

	if (request.GetQueryParam(_T("cmd"), cmd))
	{
		JSONObjectBuilder json;
		int code = HTTP_OK;

		if (!_tcscmp(cmd, _T("acknowledgeAlarms")))
		{
			JSON_UpdateAlarms(request, AU_ACK, json);
		}
		else if (!_tcscmp(cmd, _T("getAlarmList")))
		{
			JSON_SendAlarmList(json);
		}
		else if (!_tcscmp(cmd, _T("getUserList")))
		{
			JSON_SendUserList(json);
		}
		else if (!_tcscmp(cmd, _T("logout")))
		{
			json.AddRCC(RCC_SUCCESS);
			keepSession = FALSE;
		}
		else if (!_tcscmp(cmd, _T("terminateAlarms")))
		{
			JSON_UpdateAlarms(request, AU_TERMINATE, json);
		}
		else
		{
			code = HTTP_BADREQUEST;
		}

		if (code == HTTP_OK)
		{
			response.SetJSONBody(json);
		}
		else
		{
			response.SetCode(code);
			response.SetBody(response.GetCodeString());
		}
	}
	else
	{
		response.SetCode(HTTP_BADREQUEST);
		response.SetBody(_T("ERROR 400: Bad request"));
	}

	return keepSession;
}
