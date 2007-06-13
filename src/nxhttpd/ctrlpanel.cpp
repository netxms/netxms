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
** File: ctrlpanel.cpp
**
**/

#include "nxhttpd.h"


//
// Control panel views
//

#define CTRLPANEL_SERVER_CONFIG     0
#define CTRLPANEL_ACTIONS           1
#define CTRLPANEL_GROUPS            2
#define CTRLPANEL_USERS             3
#define CTRLPANEL_AGENT_PACKAGES    4
#define CTRLPANEL_SCRIPT_LIBRARY    5
#define CTRLPANEL_DISCOVERY         6
#define CTRLPANEL_CERTIFICATES      7
#define CTRLPANEL_EPP               8
#define CTRLPANEL_SNMP_TRAPS        9
#define CTRLPANEL_MODULES           10
#define CTRLPANEL_OBJECT_TOOLS      11
#define CTRLPANEL_LAST_VIEW         12


//
// Show "Control Panel" form
//

void ClientSession::ShowFormControlPanel(HttpResponse &response)
{
	String data;

	data =
		_T("<script type=\"text/javascript\" src=\"/xtree.js\"></script>\r\n")
		_T("<table width=\"100%\"><tr><td width=\"20%\">\r\n")
		_T("<div id=\"ctrlpanel_tree\">\r\n")
		_T("	<div id=\"jsTree\"></div>\r\n")
		_T("</div></td>\r\n")
		_T("<td><div id=\"ctrlpanel_view\"></div></td>\r\n")
		_T("</tr></table>\r\n")
		_T("<script type=\"text/javascript\">\r\n")
		_T("	var tree = new WebFXTree(\"Control Panel\");\r\n")
		_T("  var item = new WebFXTreeItem(\"Agent Management\");\r\n")
		_T("	tree.add(item);\r\n")
		_T("	item.add(new WebFXTreeItem(\"Agent Configurations\"));\r\n")
		_T("	item.add(new WebFXTreeItem(\"Agent Packages\", \"javascript:showCtrlPanel(4);\", null, \"/images/ctrlpanel/package.png\", \"/images/ctrlpanel/package.png\"));\r\n")
		_T("  item = new WebFXTreeItem(\"Event Processing\");\r\n")
		_T("	tree.add(item);\r\n")
		_T("	item.add(new WebFXTreeItem(\"Actions\", \"javascript:showCtrlPanel(1);\", null, \"/images/ctrlpanel/actions.png\", \"/images/ctrlpanel/actions.png\"));\r\n")
		_T("	item.add(new WebFXTreeItem(\"Events\"));\r\n")
		_T("	item.add(new WebFXTreeItem(\"Event Processing Policy\", \"javascript:showCtrlPanel(8);\", null, \"/images/ctrlpanel/epp.png\", \"/images/ctrlpanel/epp.png\"));\r\n")
		_T("	item.add(new WebFXTreeItem(\"SNMP Traps\", \"javascript:showCtrlPanel(9);\", null, \"/images/ctrlpanel/snmptrap.png\", \"/images/ctrlpanel/snmptrap.png\"));\r\n")
		_T("	tree.add(new WebFXTreeItem(\"Object Tools\", \"javascript:showCtrlPanel(11);\", null, \"/images/ctrlpanel/objtools.png\", \"/images/ctrlpanel/objtools.png\"));\r\n")
		_T("	tree.add(new WebFXTreeItem(\"Script Library\", \"javascript:showCtrlPanel(5);\", null, \"/images/ctrlpanel/scripts.png\", \"/images/ctrlpanel/scripts.png\"));\r\n")
		_T("  item = new WebFXTreeItem(\"Server Configuration\", null, null, \"/images/ctrlpanel/servercfg.png\", \"/images/ctrlpanel/servercfg.png\");\r\n")
		_T("	tree.add(item);\r\n")
		_T("	item.add(new WebFXTreeItem(\"Configuration Variables\", \"javascript:showCtrlPanel(0);\", null, \"/images/ctrlpanel/cfgvar.png\", \"/images/ctrlpanel/cfgvar.png\"));\r\n")
		_T("	item.add(new WebFXTreeItem(\"Modules\", \"javascript:showCtrlPanel(10);\", null, \"/images/ctrlpanel/module.png\", \"/images/ctrlpanel/module.png\"));\r\n")
		_T("	item.add(new WebFXTreeItem(\"Network Discovery\", \"javascript:showCtrlPanel(6);\", null, \"/images/ctrlpanel/discovery.png\", \"/images/ctrlpanel/discovery.png\"));\r\n")
		_T("  item = new WebFXTreeItem(\"Users & Groups\", null, null, \"/images/ctrlpanel/users.png\", \"/images/ctrlpanel/users.png\");\r\n")
		_T("	tree.add(item);\r\n")
		_T("	item.add(new WebFXTreeItem(\"Certificates\", \"javascript:showCtrlPanel(7);\", null, \"/images/ctrlpanel/cert.png\", \"/images/ctrlpanel/cert.png\"));\r\n")
		_T("	item.add(new WebFXTreeItem(\"Groups\", \"javascript:showCtrlPanel(2);\", null, \"/images/ctrlpanel/group.png\", \"/images/ctrlpanel/group.png\"));\r\n")
		_T("	item.add(new WebFXTreeItem(\"Users\", \"javascript:showCtrlPanel(3);\", null, \"/images/ctrlpanel/user.png\", \"/images/ctrlpanel/user.png\"));\r\n")
		_T("	document.getElementById(\"jsTree\").innerHTML = tree;\r\n")
		_T("	function showCtrlPanel(viewId)\r\n")
		_T("	{\r\n")
		_T("		loadDivContent('ctrlpanel_view', '{$sid}', '&cmd=ctrlPanelView&view=' + viewId);\r\n")
		_T("     resizeElements();\r\n")
		_T("	}\r\n")
		_T("</script>\r\n");
	data.Translate(_T("{$sid}"), m_sid);
	response.AppendBody(data);
}


//
// Show control panel view
//

void ClientSession::ShowCtrlPanelView(HttpRequest &request, HttpResponse &response)
{
	int nView;
	String data;
	static TCHAR *viewName[] =
	{
		_T("Server Configuration Variables"),
		_T("Actions"),
		_T("Groups"),
		_T("Users"),
		_T("Agent Packages"),
		_T("Script Library"),
		_T("Network Discovery"),
		_T("Certificates"),
		_T("Event Processing Policy"),
		_T("SNMP Traps"),
		_T("Modules"),
		_T("Object Tools")
	};
	static TCHAR *viewImage[] =
	{
		_T("cfgvar"),
		_T("actions"),
		_T("group"),
		_T("user"),
		_T("package"),
		_T("scripts"),
		_T("discovery"),
		_T("cert"),
		_T("epp"),
		_T("snmptrap"),
		_T("module"),
		_T("objtools")
	};

	// Get and validate view id
	if (!request.GetQueryParam(_T("view"), data))
		return;
	nView = _tcstol(data, NULL, 10);
	if ((nView < 0) || (nView >= CTRLPANEL_LAST_VIEW))
		return;

	// View name
	data =
		_T("<div id=\"objview_header\">\r\n")
		_T("<table width=\"100%\" style=\"border-bottom: 3px solid rgb(204,204,204);\"><tr><td valign=\"middle\" width=\"1%\"><img src=\"/images/ctrlpanel/{$img}.png\"></td><td>{$name}</td>\r\n")
		_T("</tr></table></div><div id=\"ctrlpanel_data\">\r\n");
	data.Translate(_T("{$img}"), viewImage[nView]);
	data.Translate(_T("{$name}"), viewName[nView]);
	response.AppendBody(data);

	switch(nView)
	{
		case CTRLPANEL_SERVER_CONFIG:
			CtrlPanelServerVariables(request, response);
			break;
		default:
			ShowInfoMessage(response, _T("Not implemented yet"));
			break;
	}
	response.AppendBody(_T("</div>"));	// Close ctrlpanel_data div
}


//
// Compare two variables
//

static int CompareVariables(const void *p1, const void *p2)
{
	return _tcsicmp(((NXC_SERVER_VARIABLE *)p1)->szName, ((NXC_SERVER_VARIABLE *)p2)->szName);
}


//
// Show server configuration variables
//

void ClientSession::CtrlPanelServerVariables(HttpRequest &request, HttpResponse &response)
{
	BOOL bReload = FALSE;
	DWORD i, dwNumVars, dwResult;
	NXC_SERVER_VARIABLE *pVarList;
	String row;

	dwResult = NXCGetServerVariables(m_hSession, &pVarList, &dwNumVars);
	if (dwResult == RCC_SUCCESS)
	{
		qsort(pVarList, dwNumVars, sizeof(NXC_SERVER_VARIABLE), CompareVariables);

		response.StartBox(NULL, _T("objectTable"), _T("var_list"), NULL, bReload);
		response.StartTableHeader(NULL);
		response.AppendBody(_T("<td></td>\r\n")
								  _T("<td valign=\"center\">Variable</td>")
								  _T("<td valign=\"center\">Value</td>")
								  _T("<td valign=\"center\">Restart</td>")
								  _T("<td></td></tr>\r\n"));

		for(i = 0; i < dwNumVars; i++)
		{
			response.StartBoxRow();
			response.AppendBody(_T("<td>"));
			AddCheckbox(response, pVarList[i].szName, _T("Select variable"), _T("onAlarmSelect"), FALSE);
			row = _T("");
			row.AddFormattedString(_T("</td><td>%s</td><td>%s</td><td>%s</td>")
										  _T("<td>Edit</td></tr>\r\n"),
										  pVarList[i].szName, pVarList[i].szValue,
										  pVarList[i].bNeedRestart ? _T("Yes") : _T("No"));
			response.AppendBody(row);
		}

		response.EndBox(bReload);
		safe_free(pVarList);

		// Show control buttons
		if (!bReload)
		{
			response.AppendBody(_T("<table class=\"button_row\"><tr><td>\r\n"));
			AddButton(response, m_sid, _T("add"), _T("Add new variable"), _T("alarmButtonHandler"));
			response.AppendBody(_T("</td><td>\r\n"));
			AddButton(response, m_sid, _T("delete"), _T("Delete selected variables"), _T("alarmButtonHandler"));
			response.AppendBody(_T("</td></tr></table>\r\n"));
		}
	}
	else
	{
		ShowErrorMessage(response, dwResult);
	}
}
