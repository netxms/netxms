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
** File: object.cpp
**
**/

#include "nxhttpd.h"


//
// Show object browser form
//

void ClientSession::ShowFormObjects(HttpResponse &response)
{
	String data;

	data =
		_T("<script type=\"text/javascript\" src=\"/xtree.js\"></script>\r\n")
		_T("<script type=\"text/javascript\" src=\"/xloadtree.js\"></script>\r\n")
		_T("<script type=\"text/javascript\" src=\"/alarms.js\"></script>\r\n")
		_T("<table width=\"100%\"><tr><td width=\"20%\">\r\n")
		_T("<div id=\"object_tree\">\r\n")
		_T("	<div id=\"jsTree\"></div>\r\n")
		_T("</div></td>\r\n")
		_T("<td><div id=\"object_view\"></div></td>\r\n")
		_T("</tr></table>\r\n")
		_T("<script type=\"text/javascript\">\r\n")
		_T("	var net = new WebFXLoadTree(\"NetXMS Objects\", \"/main.app?cmd=getObjectTree&sid={$sid}\");\r\n")
		_T("	document.getElementById(\"jsTree\").innerHTML = net;\r\n")
		_T("	function showObjectInfo(id, viewId)\r\n")
		_T("	{\r\n")
		_T("     selectedAlarms = '';\r\n")
		_T("		loadDivContent('object_view', '{$sid}', '&cmd=objectView&view=' + viewId + '&id=' + id);\r\n")
		_T("     resizeElements();\r\n")
		_T("	}\r\n")
		_T("	function execObjectTool(id,tool)\r\n")
		_T("	{\r\n")
		_T("     selectedAlarms = '';\r\n")
		_T("		loadDivContent('object_view', '{$sid}', '&cmd=objectView&view=' + viewId + '&id=' + id);\r\n")
		_T("     resizeElements();\r\n")
		_T("	}\r\n")
		_T("	function manageObject(sid,id,action)\r\n")
		_T("	{\r\n")
		_T("     selectedAlarms = '';\r\n")
		_T("		loadDivContent('object_view', sid, '&cmd=objectView&view=6&id=' + id + '&action=' + action);\r\n")
		_T("     resizeElements();\r\n")
		_T("	}\r\n")
		_T("</script>\r\n");
	data.translate(_T("{$sid}"), m_sid);
	response.AppendBody(data);
}


//
// Comparision function for tree items sorting
//

static int CompareObjects(const void *p1, const void *p2)
{
   TCHAR szName1[MAX_OBJECT_NAME], szName2[MAX_OBJECT_NAME];
	NXC_OBJECT *obj1 = *((NXC_OBJECT **)p1);
	NXC_OBJECT *obj2 = *((NXC_OBJECT **)p2);
	int notContainer1, notContainer2, nResult;

	if ((obj1->dwId < 10) || (obj2->dwId < 10))
		return COMPARE_NUMBERS(obj1->dwId, obj2->dwId);

	notContainer1 = ((obj1->iClass == OBJECT_CONTAINER) || 
		              (obj1->iClass == OBJECT_TEMPLATEGROUP)) ? 0 : 1;
	notContainer2 = ((obj2->iClass == OBJECT_CONTAINER) || 
		              (obj2->iClass == OBJECT_TEMPLATEGROUP)) ? 0 : 1;
	nResult = COMPARE_NUMBERS(notContainer1, notContainer2);
	if (nResult != 0)
		return nResult;

   NXCGetComparableObjectNameEx(obj1, szName1);
   NXCGetComparableObjectNameEx(obj2, szName2);
   return _tcsicmp(szName1, szName2);
}


//
// Send object tree to client as XML
//

void ClientSession::SendObjectTree(HttpRequest &request, HttpResponse &response)
{
	String parent, data, tmp;
	const TCHAR *ico;
	int parentId = 0;
	NXC_OBJECT **ppRootObjects = NULL;
	NXC_OBJECT_INDEX *pIndex = NULL;
	DWORD i, j, dwNumObjects, dwNumRootObj;

	if (request.GetQueryParam(_T("parent"), parent))
	{
		parentId = atoi((const TCHAR *)parent);
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

	qsort(ppRootObjects, dwNumRootObj, sizeof(NXC_OBJECT *), CompareObjects);

	data = _T("<tree>");

	for(i = 0; i < dwNumRootObj; i++)
	{
		ico = _T("");
		switch(ppRootObjects[i]->iClass)
		{
			case OBJECT_SUBNET:
				ico = "/images/objects/subnet.png";
				break;
			case OBJECT_NODE:
				ico = "/images/objects/node.png";
				break;
			case OBJECT_INTERFACE:
				ico = "/images/objects/interface.png";
				break;
			case OBJECT_NETWORKSERVICE:
				ico = "/images/objects/service.png";
				break;
			case OBJECT_NETWORK:
			case OBJECT_SERVICEROOT:
				ico = "/images/objects/network.png";
				break;
			case OBJECT_CONTAINER:
				ico = "/images/objects/container.png";
				break;
			case OBJECT_TEMPLATEROOT:
				ico = "/images/objects/template_root.png";
				break;
			case OBJECT_TEMPLATEGROUP:
				ico = "/images/objects/template_group.png";
				break;
			case OBJECT_TEMPLATE:
				ico = "/images/objects/template.png";
				break;
		}

		data += _T("<tree text=\"");
		tmp = ppRootObjects[i]->szName;
		data += EscapeHTMLText(tmp);
		data += _T("\" ");
		if (*ico != 0)
		{
			data.addFormattedString(_T("openIcon=\"%s\" icon=\"%s\" "), ico, ico);
		}

		if (ppRootObjects[i]->dwNumChilds > 0)
		{
			data.addFormattedString(_T("src=\"/main.app?cmd=getObjectTree&amp;sid=%s&amp;parent=%d\" "),
			                        m_sid, ppRootObjects[i]->dwId);
		}
		data.addFormattedString(_T("action=\"javascript:showObjectInfo(%d, 0);\" />"), ppRootObjects[i]->dwId);
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
		case OBJVIEW_MANAGE:
			return TRUE;
		case OBJVIEW_ALARMS:
			return ((nClass == OBJECT_NODE) || (nClass == OBJECT_SUBNET) ||
			        (nClass == OBJECT_NETWORK) || (nClass == OBJECT_CONTAINER) ||
			        (nClass == OBJECT_CLUSTER) || (nClass == OBJECT_SERVICEROOT));
		case OBJVIEW_LAST_VALUES:
		case OBJVIEW_PERFORMANCE:
		case OBJVIEW_TOOLS:
			return (nClass == OBJECT_NODE);
	}
	return FALSE;
}


//
// Add object submenu item
//

static void AddObjectSubmenu(String &data, int nClass, int nViewId, const TCHAR *pszViewName)
{
	if (IsValidObjectView(nClass, nViewId))
		data.addFormattedString(_T("		<li><a href=\"\" onclick=\"javascript:showObjectInfo({$id},%d); return false;\">%s</a></li>\r\n"),
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
	pObject = NXCFindObjectById(m_hSession, _tcstoul((const TCHAR *)id, NULL, 10));
	if (pObject == NULL)
		return;

	// Calculate view to display
	if (request.GetQueryParam(_T("view"), data))
	{
		nNewView = atoi((const TCHAR *)data);
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
		_T("<table><tr><td valign=\"middle\"><img src=\"/images/status/{$status}.png\"></td><td>{$name}</td>\r\n")
		_T("</tr></table></div>\r\n");
	data.translate(_T("{$status}"), g_szStatusImageName[pObject->iStatus]);
	data.translate(_T("{$name}"), pObject->szName);
	response.AppendBody(data);

	// Object menu and start of object_data div
	data = _T("<div id=\"nav_submenu\">\r\n	<ul>\r\n");
	AddObjectSubmenu(data, pObject->iClass, OBJVIEW_OVERVIEW, _T("Overview"));
	AddObjectSubmenu(data, pObject->iClass, OBJVIEW_ALARMS, _T("Alarms"));
	AddObjectSubmenu(data, pObject->iClass, OBJVIEW_LAST_VALUES, _T("DataView"));
	AddObjectSubmenu(data, pObject->iClass, OBJVIEW_PERFORMANCE, _T("PerfView"));
	AddObjectSubmenu(data, pObject->iClass, OBJVIEW_TOOLS, _T("Tools"));
	AddObjectSubmenu(data, pObject->iClass, OBJVIEW_MANAGE, _T("Manage"));
	data += _T("	</ul>\r\n</div><div id=\"object_data\">\r\n");
	//data.translate(_T("{$sid}"), m_sid);
	data.translate(_T("{$id}"), (const TCHAR *)id);
	response.AppendBody(data);

	// View itself
	switch(m_nCurrObjectView)
	{
		case OBJVIEW_OVERVIEW:
			ShowObjectOverview(response, pObject);
			break;
		case OBJVIEW_ALARMS:
			ShowAlarmList(response, pObject, FALSE, _T(""));
			break;
		case OBJVIEW_LAST_VALUES:
			ShowLastValues(response, pObject, FALSE);
			break;
		case OBJVIEW_TOOLS:
			ShowObjectTools(response, pObject);
			break;
		case OBJVIEW_MANAGE:
			ShowObjectMgmt(request, response, pObject);
			break;
		default:
			ShowInfoMessage(response, _T("Not implemented yet"));
			break;
	}
	response.AppendBody(_T("</div>"));	// Close object_data div
}


//
// Show single object attribute
//

static void ShowObjectAttribute(HttpResponse &response, const TCHAR *pszName, const TCHAR *pszValue)
{
	TCHAR szBuffer[1024];
	String value;

	value = pszValue;
	_sntprintf(szBuffer, 1024, _T("<tr><td width=\"30%%\">%s</td><td>%s</td></tr>\r\n"),
	           pszName, EscapeHTMLText(value));
	response.AppendBody(szBuffer);
}

static void ShowObjectAttribute(HttpResponse &response, const TCHAR *pszName, DWORD dwValue)
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
		_T("<div class=\"subheader\"><table><tr><td>Attributes</td></tr></table></div>\r\n")
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
				ShowObjectAttribute(response, _T("SNMP OID"), CHECK_NULL_EX(pObject->node.pszSnmpObjectId));
			}
			else
			{
				ShowObjectAttribute(response, _T("SNMP Agent"), _T("Inactive"));
			}
			ShowObjectAttribute(response, _T("CDP Support"), (pObject->node.dwFlags & NF_IS_CDP) ? _T("Yes") : _T("No"));
			ShowObjectAttribute(response, _T("SONMP Support"), (pObject->node.dwFlags & NF_IS_SONMP) ? _T("Yes") : _T("No"));
			ShowObjectAttribute(response, _T("LLDP Support"), (pObject->node.dwFlags & NF_IS_LLDP) ? _T("Yes") : _T("No"));
			break;
		default:
			break;
	}

	// Finish attributes table and start comments
	response.AppendBody(_T("</table></div><br>\r\n")
	                    _T("<div class=\"subheader\"><table><tr><td>Comments</td></tr></table></div>\r\n"));
	temp = CHECK_NULL_EX(pObject->pszComments);
	response.AppendBody(EscapeHTMLText(temp));
	response.AppendBody(_T("</div>\r\n"));
}


//
// Add tool
//

static void AddTool(HttpResponse &response, TCHAR *pszName, TCHAR *pszImage, DWORD dwObjectId, int nTool)
{
	String tmp;

	tmp.addFormattedString(_T("<table class=\"inner_table\"><tr><td style=\"padding-right: 0.4em;\">")
	                       _T("<img src=\"%s\"></td><td>")
								  _T("<a href=\"#\" onclick=\"javascript:execObjectTool(%d,%d); return false;\">%s</a></td></tr></table>\r\n"),
								  pszImage, dwObjectId, nTool, pszName);
	response.AppendBody(tmp);
}


//
// Show "Tools" object's view
//

void ClientSession::ShowObjectTools(HttpResponse &response, NXC_OBJECT *pObject)
{
	response.AppendBody(_T("<div id=\"object_tools\">\r\n"));
/*	AddTool(response, _T("Create child object"), _T("/images/file.png"), pObject->dwId, OBJTOOL_CREATE);
	AddTool(response, _T("Delete"), _T("/images/terminate.png"), pObject->dwId, OBJTOOL_DELETE);
	response.AppendBody(_T("<br>\r\n"));
	AddTool(response, _T("Manage"), _T("/images/status/normal.png"), pObject->dwId, OBJTOOL_MANAGE);
	AddTool(response, _T("Unmanage"), _T("/images/status/unmanaged.png"), pObject->dwId, OBJTOOL_UNMANAGE);*/
	response.AppendBody(_T("</div>\r\n"));
}


//
// Add object management link
//

static void AddObjectMgmtLink(HttpResponse &response, NXC_OBJECT *pObject, const TCHAR *sid,
                              const TCHAR *pszName, const TCHAR *pszImage, int nAction)
{
	TCHAR szTemp[256];

	response.AppendBody(_T("<td>"));
	_stprintf(szTemp, _T(",%d,%d"), pObject->dwId, nAction);
	AddActionLink(response, sid, pszName, pszImage, _T("manageObject"), szTemp);
	response.AppendBody(_T("</td>"));
}


//
// Show "Manage" object's view
//

void ClientSession::ShowObjectMgmt(HttpRequest &request, HttpResponse &response, NXC_OBJECT *pObject)
{
	String action;
	int nAction;
	DWORD dwResult;

	if (request.GetQueryParam(_T("action"), action))
	{
		nAction = _tcstol(action, NULL, 10);
		switch(nAction)
		{
			case OBJECT_ACTION_MANAGE:
				dwResult = NXCSetObjectMgmtStatus(m_hSession, pObject->dwId, TRUE);
				break;
			case OBJECT_ACTION_UNMANAGE:
				dwResult = NXCSetObjectMgmtStatus(m_hSession, pObject->dwId, FALSE);
				break;
			default:
				dwResult = RCC_SUCCESS;
				break;
		}
		if (dwResult == RCC_SUCCESS)
		{
			ShowSuccessMessage(response);
		}
		else
		{
			ShowErrorMessage(response, dwResult);
		}
	}

	// Show list of available actions
	response.AppendBody(_T("<div class=\"action_menu\"><table><tr>"));
	AddObjectMgmtLink(response, pObject, m_sid, _T("Manage"), _T("status/normal.png"), OBJECT_ACTION_MANAGE);
	AddObjectMgmtLink(response, pObject, m_sid, _T("Unmanage"), _T("status/unmanaged.png"), OBJECT_ACTION_UNMANAGE);
	AddObjectMgmtLink(response, pObject, m_sid, _T("Delete"), _T("status/unmanaged.png"), OBJECT_ACTION_DELETE);
	AddObjectMgmtLink(response, pObject, m_sid, _T("Properties"), _T("edit.png"), OBJECT_ACTION_PROPERTIES);
	AddObjectMgmtLink(response, pObject, m_sid, _T("Security"), _T("security.png"), OBJECT_ACTION_SECURITY);
	response.AppendBody(_T("</tr></table></div>\r\n"));

	response.AppendBody(_T("<div id=\"object_tools\">\r\n"));

	response.AppendBody(_T("</div>\r\n"));
}
