/* $Id$ */
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
** File: smgr.cpp
**
**/

#include "nxhttpd.h"


//
// Static data
//

static MUTEX m_mutexSessionAccess = INVALID_MUTEX_HANDLE;
static ClientSession *m_sessionList[MAX_SESSIONS];


//
// Session watchdog thread
//

THREAD_RESULT THREAD_CALL SessionWatchdog(void *)
{
	int i;
	time_t now;

	DebugPrintf(INVALID_INDEX, _T("Session watchdog thread started"));
   while(!(g_dwFlags & AF_SHUTDOWN))
	{
		ThreadSleep(5);
		MutexLock(m_mutexSessionAccess);
		now = time(NULL);
		for(i = 0; i < MAX_SESSIONS; i++)
			if (m_sessionList[i] != NULL)
			{
				if (m_sessionList[i]->GetLastAccessTime() + (time_t)g_dwSessionTimeout < now)
				{
					DebugPrintf(i, _T("Session terminated by watchdog"));
					delete m_sessionList[i];
					m_sessionList[i] = NULL;
				}
			}
		MutexUnlock(m_mutexSessionAccess);
	}
	DebugPrintf(INVALID_INDEX, _T("Session watchdog thread terminated"));
	return THREAD_OK;
}


//
// Initialize sessions
//

void InitSessions(void)
{
	memset(m_sessionList, 0, sizeof(ClientSession *) * MAX_SESSIONS);
	m_mutexSessionAccess = MutexCreate();
}


//
// Register new session
//

static BOOL RegisterNewSession(ClientSession *pSession)
{
   DWORD i;
   BOOL bResult = FALSE;

   MutexLock(m_mutexSessionAccess);
   for(i = 0; i < MAX_SESSIONS; i++)
      if (m_sessionList[i] == NULL)
      {
         m_sessionList[i] = pSession;
			pSession->SetIndex(i);
         bResult = TRUE;
         break;
      }
   MutexUnlock(m_mutexSessionAccess);
   return bResult;
}


//
// Delete session
//

static void DeleteSession(ClientSession *pSession)
{
	DWORD dwIndex;

	MutexLock(m_mutexSessionAccess);
	dwIndex = pSession->GetIndex();
	m_sessionList[dwIndex] = NULL;
	delete pSession;
   MutexUnlock(m_mutexSessionAccess);
}


//
// Find session by SID
//

static ClientSession *FindSessionBySID(const TCHAR *pszSID)
{
   DWORD i;
	ClientSession *pSession = NULL;

   MutexLock(m_mutexSessionAccess);
   for(i = 0; i < MAX_SESSIONS; i++)
      if (m_sessionList[i] != NULL)
      {
         if (m_sessionList[i]->IsMySID(pszSID))
			{
				pSession = m_sessionList[i];
				break;
			}
      }
   MutexUnlock(m_mutexSessionAccess);
   return pSession;
}


//
// Session request handler
//

BOOL SessionRequestHandler(HttpRequest &request, HttpResponse &response)
{
	BOOL bProcessed = FALSE;
	const TCHAR *pszURI, *pszUriExt;
	TCHAR *pszExt, szModule[MAX_MODULE_NAME];

	pszURI = request.GetURI();
	pszUriExt = _tcsrchr(pszURI, _T('.'));
	if (pszUriExt != NULL)
	{
		pszUriExt++;
		if (!_tcscmp(pszUriExt, _T("app")))
		{
			nx_strncpy(szModule, (*pszURI == _T('/')) ? &pszURI[1] : pszURI, MAX_MODULE_NAME);
			pszExt = _tcsrchr(szModule, _T('.'));
			if (pszExt != NULL)
				*pszExt = 0;
			if (!_tcscmp(szModule, _T("login")))
			{
				String user, passwd;
				ClientSession *pSession;
				DWORD dwResult;

				if (request.GetQueryParam(_T("user"), user) &&
					 request.GetQueryParam(_T("pwd"), passwd))
				{
					pSession = new ClientSession;
					dwResult = pSession->DoLogin(user, passwd);
					if (dwResult == RCC_SUCCESS)
					{
						if (RegisterNewSession(pSession))
						{
							pSession->GenerateSID();
							pSession->ShowForm(response, FORM_OVERVIEW);
						}
						else
						{
							delete pSession;
							ShowFormLogin(response, _T("Allowed number of simultaneous sessions exceeded"));
						}
					}
					else
					{
						delete pSession;
						ShowFormLogin(response, (TCHAR *)NXCGetErrorText(dwResult));
					}
				}
				else
				{
					ShowFormLogin(response, NULL);
				}
				response.SetCode(HTTP_OK);
			}
			else if (!_tcscmp(szModule, _T("jsonlogin")))
			{
				String user, passwd;
				ClientSession *pSession;
				DWORD dwResult;

				if (request.GetQueryParam(_T("user"), user) &&
					 request.GetQueryParam(_T("passwd"), passwd))
				{
					JSONObjectBuilder json;

					pSession = new ClientSession;
					dwResult = pSession->DoLogin(user, passwd);
					if (dwResult == RCC_SUCCESS)
					{
						if (RegisterNewSession(pSession))
						{
							pSession->GenerateSID();
							json.AddRCC(RCC_SUCCESS);
							json.AddString(_T("sid"), pSession->GetSID());
						}
						else
						{
							delete pSession;
							json.AddRCC(RCC_INTERNAL_ERROR);
						}
					}
					else
					{
						delete pSession;
						json.AddRCC(dwResult);
					}
					response.SetJSONBody(json);
				}
				else
				{
					response.SetCode(HTTP_BADREQUEST);
					response.SetBody("ERROR 400: Bad request");
				}
			}
			else if (!_tcscmp(szModule, _T("main")))
			{
				String sid;

				if (request.GetQueryParam(_T("sid"), sid))
				{
					ClientSession *pSession;

					pSession = FindSessionBySID(sid);
					if (pSession != NULL)
					{
						pSession->SetLastAccessTime();
						if (!pSession->ProcessRequest(request, response))
						{
							DeleteSession(pSession);
							ShowFormLogin(response, NULL);
						}
					}
					else
					{
						response.SetCode(HTTP_OK);
						response.BeginPage(_T("NetXMS :: Session Expired"));
						response.SetBody("ERROR: You session has expired.<br><br><a href=\"/login.app\">Return to login page</a><br>");
						response.EndPage();
					}
				}
				else
				{
					response.SetCode(HTTP_FORBIDDEN);
					response.SetBody("ERROR 403: Forbidden");
				}
			}
			else if (!_tcscmp(szModule, _T("json")))
			{
				String sid;

				if (request.GetQueryParam(_T("sid"), sid))
				{
					ClientSession *pSession;

					pSession = FindSessionBySID(sid);
					if (pSession != NULL)
					{
						pSession->SetLastAccessTime();
						if (!pSession->ProcessJSONRequest(request, response))
						{
							DeleteSession(pSession);
						}
					}
					else
					{
						response.SetCode(HTTP_FORBIDDEN);
						response.SetBody("ERROR 403: Forbidden");
					}
				}
				else
				{
					response.SetCode(HTTP_FORBIDDEN);
					response.SetBody("ERROR 403: Forbidden");
				}
			}
			else
			{
				response.SetCode(HTTP_NOTFOUND);
				response.SetBody("ERROR 404: File not found");
			}
			bProcessed = TRUE;
		}
	}
	return bProcessed;
}
