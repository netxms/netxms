/* $Id$ */
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
** File: session.cpp
**
**/

#include "nxhttpd.h"


//
// Source for SID generation
//

struct SID_SOURCE
{
   time_t m_time;
   DWORD m_dwPID;
   DWORD m_dwRandom;
};


//
// Client library event handler
//

void SessionEventHandler(NXC_SESSION hSession, DWORD dwEvent, DWORD dwCode, void *pArg)
{
	ClientSession *pSession;

	pSession = (ClientSession *)NXCGetClientData(hSession);
	if (pSession != NULL)
		pSession->EventHandler(dwEvent, dwCode, pArg);
}


//
// Constructor
//

ClientSession::ClientSession()
{
	m_hSession = NULL;
	m_nCurrObjectView = OBJVIEW_OVERVIEW;
	m_dwNumAlarms = 0;
	m_pAlarmList = NULL;
	m_mutexAlarmList = MutexCreate();
	m_nAlarmSortMode = 0;
	m_nAlarmSortDir = -1;
	m_dwCurrAlarmRoot = 0;
	m_dwNumValues = 0;
	m_pValueList = NULL;
	m_nLastValuesSortMode = 1;
	m_nLastValuesSortDir = 1;
	m_tmLastAccess = time(NULL);
}


//
// Destructor
//

ClientSession::~ClientSession()
{
	if (m_hSession != NULL)
		NXCDisconnect(m_hSession);
	safe_free(m_pAlarmList);
	safe_free(m_pValueList);
	MutexDestroy(m_mutexAlarmList);
}


//
// Handler for client library events
//

void ClientSession::EventHandler(DWORD dwEvent, DWORD dwCode, void *pArg)
{
	if (dwEvent == NXC_EVENT_NOTIFICATION)
	{
		switch(dwCode)
		{
         case NX_NOTIFY_NEW_ALARM:
         case NX_NOTIFY_ALARM_DELETED:
         case NX_NOTIFY_ALARM_CHANGED:
         case NX_NOTIFY_ALARM_TERMINATED:
            OnAlarmUpdate(dwCode, (NXC_ALARM *)pArg);
            break;
			default:
				break;
		}
	}
}


//
// Do login
//

DWORD ClientSession::DoLogin(const TCHAR *pszLogin, const TCHAR *pszPassword)
{
	DWORD dwResult;

	dwResult = NXCConnect(0, g_szMasterServer, pszLogin, pszPassword, 0,
	                      NULL, NULL, &m_hSession,
								 _T("nxhttpd/") NETXMS_VERSION_STRING, NULL);
	if (dwResult == RCC_SUCCESS)
	{
		NXCSetClientData(m_hSession, this);
      dwResult = NXCSubscribe(m_hSession, NXC_CHANNEL_OBJECTS);
      if (dwResult == RCC_SUCCESS)
         dwResult = NXCSyncObjects(m_hSession);
	}

	// Synchronize alarms
	if (dwResult == RCC_SUCCESS)
	{
		dwResult = NXCLoadAllAlarms(m_hSession, FALSE, &m_dwNumAlarms, &m_pAlarmList);
		if (dwResult == RCC_SUCCESS)
	      dwResult = NXCSubscribe(m_hSession, NXC_CHANNEL_ALARMS);
	}

	if (dwResult == RCC_SUCCESS)
	{
		NXCSetEventHandler(m_hSession, SessionEventHandler);
	}

	// Synchronize user database
	if (dwResult == RCC_SUCCESS)
	{
		dwResult = NXCLoadUserDB(m_hSession);
	}

   // Disconnect if some of post-login operations was failed
   if ((dwResult != RCC_SUCCESS) && (m_hSession != NULL))
   {
      NXCDisconnect(m_hSession);
      m_hSession = NULL;
   }

	return dwResult;
}


//
// Generate SID
//

void ClientSession::GenerateSID(void)
{
   SID_SOURCE sid;
   BYTE hash[SHA1_DIGEST_SIZE];

   sid.m_time = time(NULL);
   sid.m_dwPID = getpid();
#ifdef _WIN32
   sid.m_dwRandom = GetTickCount();
#else
   sid.m_dwRandom = 0;	// FIXME
#endif
   CalculateSHA1Hash((BYTE *)&sid, sizeof(SID_SOURCE), hash);
   BinToStr(hash, SHA1_DIGEST_SIZE, m_sid);
}


//
// Process user's request
// Should return FALSE if session should be terminated for any reason
//

BOOL ClientSession::ProcessRequest(HttpRequest &request, HttpResponse &response)
{
	String cmd;
	BOOL bRet = TRUE;

	response.SetCode(HTTP_OK);
	if (request.GetQueryParam(_T("cmd"), cmd))
	{
		if (!_tcscmp((const TCHAR *)cmd, _T("logout")))
		{
			bRet = FALSE;
		}
		else if (!_tcscmp((const TCHAR *)cmd, _T("overview")))
		{
			ShowForm(response, FORM_OVERVIEW);
		}
		else if (!_tcscmp((const TCHAR *)cmd, _T("objects")))
		{
			ShowForm(response, FORM_OBJECTS);
		}
		else if (!_tcscmp((const TCHAR *)cmd, _T("getObjectTree")))
		{
			SendObjectTree(request, response);
		}
		else if (!_tcscmp((const TCHAR *)cmd, _T("objectView")))
		{
			ShowObjectView(request, response);
		}
		else if (!_tcscmp((const TCHAR *)cmd, _T("getLastValues")))
		{
			SendLastValues(request, response);
		}
		else if (!_tcscmp((const TCHAR *)cmd, _T("alarms")))
		{
			ShowForm(response, FORM_ALARMS);
		}
		else if (!_tcscmp((const TCHAR *)cmd, _T("getAlarmList")))
		{
			SendAlarmList(request, response);
		}
		else if (!_tcscmp((const TCHAR *)cmd, _T("alarmCtrl")))
		{
			AlarmCtrlHandler(request, response);
		}
		else if (!_tcscmp((const TCHAR *)cmd, _T("tools")))
		{
			ShowForm(response, FORM_TOOLS);
		}
		else if (!_tcscmp((const TCHAR *)cmd, _T("ctrlPanel")))
		{
			ShowForm(response, FORM_CONTROL_PANEL);
		}
		else if (!_tcscmp((const TCHAR *)cmd, _T("ctrlPanelView")))
		{
			ShowCtrlPanelView(request, response);
		}
		else if (!_tcscmp((const TCHAR *)cmd, _T("pieChart")))
		{
			ShowPieChart(request, response);
		}
		else
		{
			response.SetCode(HTTP_BADREQUEST);
			response.SetBody(_T("ERROR 400: Bad request"));
		}
	}
	else
	{
		response.SetCode(HTTP_BADREQUEST);
		response.SetBody(_T("ERROR 400: Bad request"));
	}
	return bRet;
}


//
// Show main menu area
//

void ClientSession::ShowMainMenu(HttpResponse &response)
{
	String data;

	data = 
		_T("<div id=\"nav_menu\">\r\n")
		_T("	<ul>\r\n")
		_T("		<li><a href=\"/main.app?cmd=overview&sid={$sid}\">Overview</a></li>\r\n")
		_T("		<li><a href=\"/main.app?cmd=alarms&sid={$sid}\">Alarms</a></li>\r\n")
		_T("		<li><a href=\"/main.app?cmd=objects&sid={$sid}\">Objects</a></li>\r\n")
		_T("		<li><a href=\"/main.app?cmd=tools&sid={$sid}\">Tools</a></li>\r\n")
		_T("		<li><a href=\"/main.app?cmd=ctrlPanel&sid={$sid}\">Admin</a></li>\r\n")
		_T("		<li style=\"float: right\"><a href=\"/main.app?cmd=logout&sid={$sid}\">Logout</a></li>\r\n")
		_T("	</ul>\r\n")
		_T("</div>\r\n");
	data.translate(_T("{$sid}"), m_sid);
	response.AppendBody(data);
}


//
// Show form
//

void ClientSession::ShowForm(HttpResponse &response, int nForm)
{
	static const TCHAR *formName[] =
	{
		_T("NetXMS :: Overview"),
		_T("NetXMS :: Object Browser"),
		_T("NetXMS :: Alarms"),
		_T("NetXMS :: Tools"),
		_T("NetXMS :: Control Panel")
	};

	response.BeginPage(formName[nForm]);
	ShowMainMenu(response);
	response.AppendBody(_T("<div id=\"clientarea\">\r\n"));
	switch(nForm)
	{
		case FORM_OBJECTS:
			ShowFormObjects(response);
			break;
		case FORM_ALARMS:
			response.AppendBody(_T("<script type=\"text/javascript\" src=\"/alarms.js\"></script>\r\n")
			                    _T("<div id=\"alarm_view\">\r\n"));
			ShowAlarmList(response, NULL, FALSE, _T(""));
			response.AppendBody(_T("</div>\r\n"));
			break;
		case FORM_OVERVIEW:
			ShowFormOverview(response);
			break;
		case FORM_CONTROL_PANEL:
			ShowFormControlPanel(response);
			break;
		default:
			ShowInfoMessage(response, _T("Not implemented yet"));
			break;
	}
	response.AppendBody(_T("</div>\r\n"));
	response.EndPage();
}


//
// Show "Overview" form
//

void ClientSession::ShowFormOverview(HttpResponse &response)
{
	DWORD dwResult;
	NXC_SERVER_STATS stats;
	TCHAR szTmp[1024];
	String str;

	response.AppendBody(_T("<table width=\"98%\" align=\"center\"><tr><td><table width=\"100%\"><tr><td>\r\n"));

	response.StartBox(_T("Message of the Day"), NULL, NULL, _T("infoBoxTable"));
	response.AppendBody(_T("<tr><td>No message of the day was set</td></tr>"));
	response.EndBox();

	response.AppendBody(_T("</td></tr><tr><td>\r\n"));

	response.StartBox(_T("Server Stats"), NULL, NULL, _T("infoBoxTable"));
	dwResult = NXCGetServerStats(m_hSession, &stats);
	if (dwResult == RCC_SUCCESS)
	{
		response.StartBoxRow();

		_sntprintf(szTmp, 1024, _T("<td>Server version:</td><td>%s</td></tr>\r\n"), stats.szServerVersion);
		response.AppendBody(szTmp);

		_sntprintf(szTmp, 1024, _T("<td>Server Uptime:</td><td>%d days, %02d:%02d</td></tr>\r\n"),
		           stats.dwServerUptime / 86400, (stats.dwServerUptime % 86400) / 3600,
					  (stats.dwServerUptime % 3600) / 60);
		response.AppendBody(szTmp);

		_sntprintf(szTmp, 1024, _T("<td>Total number of objects:</td><td>%d</td></tr>\r\n"), stats.dwNumObjects);
		response.AppendBody(szTmp);

		_sntprintf(szTmp, 1024, _T("<td>Total number of nodes:</td><td>%d</td></tr>\r\n"), stats.dwNumNodes);
		response.AppendBody(szTmp);

		_sntprintf(szTmp, 1024, _T("<td>Total number of DCIs:</td><td>%d</td></tr>\r\n"), stats.dwNumDCI);
		response.AppendBody(szTmp);

		_sntprintf(szTmp, 1024, _T("<td>Active client sessions:</td><td>%d</td></tr>\r\n"), stats.dwNumClientSessions);
		response.AppendBody(szTmp);

		if (stats.dwServerProcessVMSize > 0)
		{
			_sntprintf(szTmp, 1024, _T("<td>Physical memory used by server:</td><td>%d KBytes</td></tr>\r\n"), stats.dwServerProcessWorkSet);
			response.AppendBody(szTmp);

			_sntprintf(szTmp, 1024, _T("<td>Virtual memory used by server:</td><td>%d KBytes</td></tr>\r\n"), stats.dwServerProcessVMSize);
			response.AppendBody(szTmp);
		}

		_sntprintf(szTmp, 1024, _T("<td>Condition poller queue size:</td><td>%d</td></tr>\r\n"), stats.dwQSizeConditionPoller);
		response.AppendBody(szTmp);

		_sntprintf(szTmp, 1024, _T("<td>Configuration poller queue size:</td><td>%d</td></tr>\r\n"), stats.dwQSizeConfPoller);
		response.AppendBody(szTmp);

		_sntprintf(szTmp, 1024, _T("<td>Data collector queue size:</td><td>%d</td></tr>\r\n"), stats.dwQSizeDCIPoller);
		response.AppendBody(szTmp);

		_sntprintf(szTmp, 1024, _T("<td>Database writer queue size:</td><td>%d</td></tr>\r\n"), stats.dwQSizeDBWriter);
		response.AppendBody(szTmp);

		_sntprintf(szTmp, 1024, _T("<td>Event queue size:</td><td>%d</td></tr>\r\n"), stats.dwQSizeEvents);
		response.AppendBody(szTmp);

		_sntprintf(szTmp, 1024, _T("<td>Discovery poller queue size:</td><td>%d</td></tr>\r\n"), stats.dwQSizeDiscovery);
		response.AppendBody(szTmp);

		_sntprintf(szTmp, 1024, _T("<td>Node poller queue size:</td><td>%d</td></tr>\r\n"), stats.dwQSizeNodePoller);
		response.AppendBody(szTmp);

		_sntprintf(szTmp, 1024, _T("<td>Route poller queue size:</td><td>%d</td></tr>\r\n"), stats.dwQSizeRoutePoller);
		response.AppendBody(szTmp);

		_sntprintf(szTmp, 1024, _T("<td>Status poller queue size:</td><td>%d</td></tr>\r\n"), stats.dwQSizeStatusPoller);
		response.AppendBody(szTmp);
	}
	else
	{
		ShowErrorMessage(response, dwResult);
	}
	response.EndBox();

	response.AppendBody(_T("</td></tr></table></td><td width=\"5%\"><table><tr><td>\r\n"));

	response.StartBox(_T("Alarm Distribution"));
	str = _T("<img src=\"/main.app?sid={$sid}&cmd=pieChart&type=0\">\r\n");
	str.translate(_T("{$sid}"), m_sid);
	response.AppendBody(str);
	response.EndBox();

	response.AppendBody(_T("</td></tr><tr><td>\r\n"));

	response.StartBox(_T("Node Status Distribution"));
	str = _T("<img src=\"/main.app?sid={$sid}&cmd=pieChart&type=1\">\r\n");
	str.translate(_T("{$sid}"), m_sid);
	response.AppendBody(str);
	response.EndBox();
	
	response.AppendBody(_T("</td></tr></table>\r\n"));

	response.AppendBody(_T("</td></tr></table>\r\n"));
}


//
// Show pie chart
//

void ClientSession::ShowPieChart(HttpRequest &request, HttpResponse &response)
{
	PieChart pie;
	String value;
	int nType, counters[9];
	DWORD i, dwNumObjects;
	NXC_OBJECT_INDEX *pIndex;

	if (request.GetQueryParam("type", value))
	{
		nType = _tcstol(value, NULL, 10);
		if ((nType < 0) && (nType > 1))
			nType = 0;
	}
	else
	{
		nType = 0;
	}

	switch(nType)
	{
		case 0:	// Alarm severity distribution
			memset(counters, 0, sizeof(int) * 5);
		   MutexLock(m_mutexAlarmList);
			for(i = 0; i < m_dwNumAlarms; i++)
			{
				counters[m_pAlarmList[i].nCurrentSeverity]++;
			}
		   MutexUnlock(m_mutexAlarmList);
			pie.SetValue(_T("Normal"), counters[0]);
			pie.SetValue(_T("Warning"), counters[1]);
			pie.SetValue(_T("Minor"), counters[2]);
			pie.SetValue(_T("Major"), counters[3]);
			pie.SetValue(_T("Critical"), counters[4]);
			break;
		case 1:	// Node status distribution
			memset(counters, 0, sizeof(int) * 9);
			pIndex = (NXC_OBJECT_INDEX *)NXCGetObjectIndex(m_hSession, &dwNumObjects);
			for(i = 0; i < dwNumObjects; i++)
			{
				if (pIndex[i].pObject->iClass == OBJECT_NODE)
				{
					counters[pIndex[i].pObject->iStatus]++;
				}
			}
			NXCUnlockObjectIndex(m_hSession);
			pie.SetValue(_T("Normal"), counters[0]);
			pie.SetValue(_T("Warning"), counters[1]);
			pie.SetValue(_T("Minor"), counters[2]);
			pie.SetValue(_T("Major"), counters[3]);
			pie.SetValue(_T("Critical"), counters[4]);
			pie.SetValue(_T("Unknown"), counters[5]);
			pie.SetValue(_T("Unmanaged"), counters[6]);
			pie.SetValue(_T("Disabled"), counters[7]);
			pie.SetValue(_T("Testing"), counters[8]);
			break;
	}

	if (pie.Build())
	{
		response.SetType("image/png");
		response.SetBody((char *)pie.GetRawImage(), pie.GetRawImageSize());
	}
	else
	{
		response.SetType("text/plain");
		response.SetBody("error");
	}
}
