/* $Id: session.cpp,v 1.1 2007-05-07 11:35:42 victor Exp $ */
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
// Constructor
//

ClientSession::ClientSession()
{
	m_hSession = NULL;
}


//
// Destructor
//

ClientSession::~ClientSession()
{
	if (m_hSession != NULL)
		NXCDisconnect(m_hSession);
}


//
// Do login
//

DWORD ClientSession::DoLogin(TCHAR *pszLogin, TCHAR *pszPassword)
{
	DWORD dwResult;

	dwResult = NXCConnect(0, g_szMasterServer, pszLogin, pszPassword, 0,
	                      NULL, NULL, &m_hSession,
								 _T("nxhttpd/") NETXMS_VERSION_STRING, NULL);
	if (dwResult == RCC_SUCCESS)
	{
      dwResult = NXCSubscribe(m_hSession, NXC_CHANNEL_OBJECTS);
      if (dwResult == RCC_SUCCESS)
         dwResult = NXCSyncObjects(m_hSession);
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
		_T("		<li style=\"float: right\"><a href=\"/main.app?cmd=logout&sid={$sid}\">Logout</a></li>\r\n")
		_T("	</ul>\r\n")
		_T("</div>\r\n");
	data.Translate(_T("{$sid}"), m_sid);
	response.AppendBody(data);
}


//
// Show form
//

void ClientSession::ShowForm(HttpResponse &response, int nForm)
{
	static TCHAR *formName[] =
	{
		_T("NetXMS :: Overview"),
		_T("NetXMS :: Object Browser"),
		_T("NetXMS :: Alarms")
	};

	response.BeginPage(formName[nForm]);
	ShowMainMenu(response);
	response.AppendBody(_T("<div id=\"main-copy\">\r\n"));
	switch(nForm)
	{
		case FORM_OBJECTS:
			ShowFormObjects(response);
			break;
		default:
			break;
	}
	response.AppendBody(_T("</div>\r\n"));
	response.EndPage();
}


//
// Show object browser form
//

void ClientSession::ShowFormObjects(HttpResponse &response)
{
	String data;

	data =
		_T("<script type=\"text/javascript\" src=\"/xtree.js\"></script>\r\n")
		_T("<script type=\"text/javascript\" src=\"/xmlextras.js\"></script>\r\n")
		_T("<script type=\"text/javascript\" src=\"/xloadtree.js\"></script>\r\n")
		_T("<div class=\"left\">\r\n")
		_T("	<div id=\"jsTree\"></div>\r\n")
		_T("</div>\r\n")
		_T("<div class=\"right\">\r\n")
		_T("	<div id=\"objectData\"></div>\r\n")
		_T("</div>\r\n")
		_T("<script type=\"text/javascript\">\r\n")
		_T("	var net = new WebFXLoadTree(\"NetXMS Objects\", \"/main.app?cmd=getObjectTree&sid={$sid}\");\r\n")
		_T("	document.getElementById(\"jsTree\").innerHTML = net;\r\n")
		_T("	function showObjectInfo(id)\r\n")
		_T("	{\r\n")
		_T("		var xmlHttp = XmlHttp.create();\r\n")
		_T("		xmlHttp.open(\"GET\", \"/main.app?cmd=objectInfo&sid={$sid}&id=\" + id, false);\r\n")
		_T("		xmlHttp.send(null);\r\n")
		_T("		document.getElementById(\"objectData\").innerHTML = xmlHttp.responseText;\r\n")
		_T("	}\r\n")
		_T("</script>\r\n");
	data.Translate(_T("{$sid}"), m_sid);
	response.AppendBody(data);
}


//
// Send object tree to client as XML
//

void ClientSession::SendObjectTree(HttpRequest &request, HttpResponse &response)
{
	String parent, data, ico, tmp;
	int parentId = 0;
	NXC_OBJECT **ppRootObjects = NULL;
	NXC_OBJECT_INDEX *pIndex = NULL;
	DWORD i, j, dwNumObjects, dwNumRootObj;

	if (request.GetQueryParam(_T("parent"), parent))
	{
		parentId = atoi((TCHAR *)parent);
	}

	if (parentId == 0)
	{
		pIndex = (NXC_OBJECT_INDEX *)NXCGetObjectIndex(m_hSession, &dwNumObjects);
		ppRootObjects = (NXC_OBJECT **)malloc(sizeof(NXC_OBJECT *) * dwNumObjects);
		for(i = 0, dwNumRootObj = 0; i < dwNumObjects; i++)
		{
			if (!pIndex[i].pObject->bIsDeleted)
			{
				for(j = 0; j < pIndex[i].pObject->dwNumParents; j++)
				{
					if (NXCFindObjectByIdNoLock(m_hSession, pIndex[i].pObject->pdwParentList[j]) != NULL)
					{
						break;
					}
				}
				if (j == pIndex[i].pObject->dwNumParents)
				{
					ppRootObjects[dwNumRootObj++] = pIndex[i].pObject;
				}
			}
		}
		NXCUnlockObjectIndex(m_hSession);
	}
	else
	{
		dwNumRootObj = 0;
		NXC_OBJECT *object = NXCFindObjectById(m_hSession, parentId);
		if (object != NULL)
		{
			ppRootObjects = (NXC_OBJECT **)malloc(sizeof(NXC_OBJECT *) * object->dwNumChilds);
			for (i = 0; i < object->dwNumChilds; i++)
			{
				NXC_OBJECT *childObject = NXCFindObjectById(m_hSession, object->pdwChildList[i]);
				if (childObject != NULL)
				{
					ppRootObjects[dwNumRootObj++] = childObject;
				}
			}
		}
	}

	data = _T("<tree>");

	for(i = 0; i < dwNumRootObj; i++)
	{
		ico = _T("");
		switch(ppRootObjects[i]->iClass)
		{
			case OBJECT_SUBNET:
				ico = "/images/_subnet.png";
				break;
			case OBJECT_NODE:
				ico = "/images/_node.png";
				break;
			case OBJECT_INTERFACE:
				ico = "/images/_interface.png";
				break;
			case OBJECT_NETWORK:
				ico = "/images/_network.png";
				break;
		}

		data += _T("<tree text=\"");
		tmp = ppRootObjects[i]->szName;
		data += EscapeHTMLText(tmp);
		data += _T("\" ");
		if (ico.Size() > 0)
		{
			data.AddFormattedString(_T("openIcon=\"%s\" icon=\"%s\" "), (TCHAR *)ico, (TCHAR *)ico);
		}

		if (ppRootObjects[i]->dwNumChilds > 0)
		{
			data.AddFormattedString(_T("src=\"/main.app?cmd=getObjectTree&amp;sid=%s&amp;parent=%d\" "),
			                        m_sid, ppRootObjects[i]->dwId);
		}
		data.AddFormattedString(_T("action=\"javascript:showObjectInfo(%d);\" />"), ppRootObjects[i]->dwId);
	}

	data += _T("</tree>");
	safe_free(ppRootObjects);

	response.SetBody(data);
	response.SetType(_T("text/xml"));
printf("SENDING:\n%s\n", (TCHAR *)data);
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
printf("COMMAND: %s\n", (TCHAR *)cmd);
		if (!_tcscmp((TCHAR *)cmd, _T("logout")))
		{
			bRet = FALSE;
		}
		else if (!_tcscmp((TCHAR *)cmd, _T("objects")))
		{
			ShowForm(response, FORM_OBJECTS);
		}
		else if (!_tcscmp((TCHAR *)cmd, _T("getObjectTree")))
		{
			SendObjectTree(request, response);
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
