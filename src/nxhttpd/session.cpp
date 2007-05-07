/* $Id: session.cpp,v 1.2 2007-05-07 17:58:02 victor Exp $ */
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
	m_nCurrObjectView = OBJVIEW_OVERVIEW;
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
		_T("		<li><a href=\"/main.app?cmd=tools&sid={$sid}\">Tools</a></li>\r\n")
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
		_T("	function showObjectInfo(id, viewId)\r\n")
		_T("	{\r\n")
		_T("		var xmlHttp = XmlHttp.create();\r\n")
		_T("		xmlHttp.open(\"GET\", \"/main.app?cmd=objectView&sid={$sid}&view=\" + viewId + \"&id=\" + id, false);\r\n")
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
		data.AddFormattedString(_T("action=\"javascript:showObjectInfo(%d, 0);\" />"), ppRootObjects[i]->dwId);
	}

	data += _T("</tree>");
	safe_free(ppRootObjects);

	response.SetBody(data);
	response.SetType(_T("text/xml"));
}


//
// Check if view is valid for given object class
//

static BOOL IsValidObjectView(int nClass, int nView)
{
	switch(nView)
	{
		case OBJVIEW_OVERVIEW:
			return TRUE;
		case OBJVIEW_ALARMS:
			return ((nClass == OBJECT_NODE) || (nClass == OBJECT_SUBNET) ||
			        (nClass == OBJECT_NETWORK) || (nClass == OBJECT_CONTAINER) ||
			        (nClass == OBJECT_CLUSTER) || (nClass == OBJECT_SERVICEROOT));
		case OBJVIEW_LAST_VALUES:
		case OBJVIEW_PERFORMANCE:
			return (nClass == OBJECT_NODE);
	}
	return FALSE;
}


//
// Add object submenu item
//

static void AddObjectSubmenu(String &data, int nClass, int nViewId, TCHAR *pszViewName)
{
	if (IsValidObjectView(nClass, nViewId))
		data.AddFormattedString(_T("		<li><a href=\"\" onclick=\"javascript:showObjectInfo({$id},%d); return false;\">%s</a></li>\r\n"),
										nViewId, pszViewName);
}


//
// Show object view
//

void ClientSession::ShowObjectView(HttpRequest &request, HttpResponse &response)
{
	String id, data;
	NXC_OBJECT *pObject;
	int nNewView;

	if (!request.GetQueryParam(_T("id"), id))
		return;
	pObject = NXCFindObjectById(m_hSession, _tcstoul((TCHAR *)id, NULL, 10));
	if (pObject == NULL)
		return;

	// Calculate view to display
	if (request.GetQueryParam(_T("view"), data))
	{
		nNewView = atoi((TCHAR *)data);
		if ((nNewView <= OBJVIEW_CURRENT) || (nNewView >= OBJVIEW_LAST_VIEW_CODE))
			nNewView = m_nCurrObjectView;
	}
	else
	{
		nNewView = m_nCurrObjectView;
	}
	if (IsValidObjectView(pObject->iClass, nNewView))
	{
		m_nCurrObjectView = nNewView;
	}
	else
	{
		m_nCurrObjectView = OBJVIEW_OVERVIEW;
	}

	// Object's name
	data =
		_T("<div id=\"objview_header\">\r\n")
		_T("{$name}\r\n")
		_T("</div>\r\n");
	data.Translate(_T("{$name}"), pObject->szName);
	response.AppendBody(data);

	// Object menu
	data = _T("<div id=\"nav_submenu\">\r\n	<ul>\r\n");
	AddObjectSubmenu(data, pObject->iClass, OBJVIEW_OVERVIEW, _T("Overview"));
	AddObjectSubmenu(data, pObject->iClass, OBJVIEW_ALARMS, _T("Alarms"));
	AddObjectSubmenu(data, pObject->iClass, OBJVIEW_LAST_VALUES, _T("DataView"));
	AddObjectSubmenu(data, pObject->iClass, OBJVIEW_PERFORMANCE, _T("PerfView"));
	data += _T("	</ul>\r\n</div>\r\n");
	//data.Translate(_T("{$sid}"), m_sid);
	data.Translate(_T("{$id}"), (TCHAR *)id);
	response.AppendBody(data);

	// View itself
	switch(m_nCurrObjectView)
	{
		case OBJVIEW_OVERVIEW:
			ShowObjectOverview(response, pObject);
			break;
		default:
			response.AppendBody(_T("<br>Not implemented yet"));
			break;
	}
}


//
// Show single object attribute
//

static void ShowObjectAttribute(HttpResponse &response, TCHAR *pszName, TCHAR *pszValue)
{
	TCHAR szBuffer[1024];
	String value;

	value = pszValue;
	_sntprintf(szBuffer, 1024, _T("<tr><td width=\"30%%\">%s</td><td>%s</td></tr>\r\n"),
	           pszName, EscapeHTMLText(value));
	response.AppendBody(szBuffer);
}

static void ShowObjectAttribute(HttpResponse &response, TCHAR *pszName, DWORD dwValue)
{
	TCHAR szBuffer[16];

	_stprintf(szBuffer, _T("%d"), dwValue);
	ShowObjectAttribute(response, pszName, szBuffer);
}


//
// Show "Overview" object's view
//

void ClientSession::ShowObjectOverview(HttpResponse &response, NXC_OBJECT *pObject)
{
	TCHAR szTemp[256];
	String temp;

	response.AppendBody(
		_T("<div class=\"subheader\"><span>Attributes</span></div>\r\n")
		_T("<div>\r\n")
		_T("	<table width=\"100%\">\r\n")
	);

	// Common attributes
	ShowObjectAttribute(response, _T("ID"), pObject->dwId);
	ShowObjectAttribute(response, _T("Class"), g_szObjectClass[pObject->iClass]);
	ShowObjectAttribute(response, _T("Status"), g_szStatusText[pObject->iStatus]);
	if (pObject->dwIpAddr != 0)
		ShowObjectAttribute(response, _T("IP Address"), IpToStr(pObject->dwIpAddr, szTemp));

	// Class-specific attributes
	switch(pObject->iClass)
	{
		case OBJECT_SUBNET:
			ShowObjectAttribute(response, _T("Network Mask"), IpToStr(pObject->subnet.dwIpNetMask, szTemp));
			break;
		case OBJECT_INTERFACE:
			if (pObject->dwIpAddr != 0)
				ShowObjectAttribute(response, _T("IP Netmask"), IpToStr(pObject->iface.dwIpNetMask, szTemp));
			ShowObjectAttribute(response, _T("Index"), pObject->iface.dwIfIndex);
			ShowObjectAttribute(response, _T("Type"), 
						  pObject->iface.dwIfType > MAX_INTERFACE_TYPE ?
									_T("Unknown") : g_szInterfaceTypes[pObject->iface.dwIfType]);
			BinToStr(pObject->iface.bMacAddr, MAC_ADDR_LENGTH, szTemp);
			ShowObjectAttribute(response, _T("MAC Address"), szTemp);
			break;
		case OBJECT_NODE:
			if (pObject->node.dwFlags & NF_IS_NATIVE_AGENT)
			{
				ShowObjectAttribute(response, _T("NetXMS Agent"), _T("Active"));
				ShowObjectAttribute(response, _T("Agent Version"), pObject->node.szAgentVersion);
				ShowObjectAttribute(response, _T("Platform Name"), pObject->node.szPlatformName);
			}
			else
			{
				ShowObjectAttribute(response, _T("NetXMS Agent"), _T("Inactive"));
			}
			if (pObject->node.dwFlags & NF_IS_SNMP)
			{
				ShowObjectAttribute(response, _T("SNMP Agent"), _T("Active"));
				ShowObjectAttribute(response, _T("SNMP OID"), pObject->node.szObjectId);
			}
			else
			{
				ShowObjectAttribute(response, _T("SNMP Agent"), _T("Inactive"));
			}
			ShowObjectAttribute(response, _T("Node Type"), CodeToText(pObject->node.dwNodeType, g_ctNodeType, _T("Unknown")));
			break;
		default:
			break;
	}

	// Finish attributes table and start comments
	response.AppendBody(_T("</table></div><br>\r\n")
	                    _T("<div class=\"subheader\"><span>Comments</span></div>\r\n"));
	temp = CHECK_NULL_EX(pObject->pszComments);
	response.AppendBody(EscapeHTMLText(temp));
	response.AppendBody(_T("</div>\r\n"));
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
		else if (!_tcscmp((TCHAR *)cmd, _T("objectView")))
		{
			ShowObjectView(request, response);
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
