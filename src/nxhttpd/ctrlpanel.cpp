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
		_T("<script type=\"text/javascript\" src=\"/ctrlpanel.js\"></script>\r\n")
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
		_T("    loadDivContent('ctrlpanel_view', '{$sid}', '&cmd=ctrlPanelView&view=' + viewId);\r\n")
		_T("    resizeElements();\r\n")
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
	static const TCHAR *viewName[] =
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
	static const TCHAR *viewImage[] =
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
		_T("</tr></table></div>\r\n");
	data.Translate(_T("{$img}"), viewImage[nView]);
	data.Translate(_T("{$name}"), viewName[nView]);
	response.AppendBody(data);

	switch(nView)
	{
		case CTRLPANEL_SERVER_CONFIG:
			CtrlPanelServerVariables(request, response);
			break;
		case CTRLPANEL_USERS:
			CtrlPanelUsers(request, response, TRUE);
			break;
		case CTRLPANEL_GROUPS:
			CtrlPanelUsers(request, response, FALSE);
			break;
		default:
			ShowInfoMessage(response, _T("Not implemented yet"));
			break;
	}
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
	BOOL bReload = FALSE, bNew;
	DWORD i, dwNumVars, dwResult;
	NXC_SERVER_VARIABLE *pVarList;
	String row, varName, value;
	TCHAR szTemp[1024];

	if (request.GetQueryParam(_T("edit"), varName))
	{
		bNew = !_tcscmp(varName, _T("!new!"));

		response.AppendBody(_T("<div class=\"input_form\">\r\n"));
		response.StartBox(bNew ? _T("Create new variable") : _T("Edit variable"));
		response.AppendBody(_T("<form name=\"formCfgVar\" action=\"\" method=\"GET\"><table class=\"input_form_inner_table\"><tr>")
								  _T("<td><table><tr><td>Name</td><td>"));
		if (bNew)
		{
			response.AppendBody(_T("<input type=\"input\" name=\"varName\" size=\"70\" value=\"\" />"));
		}
		else
		{
			response.AppendBody(_T("<input type=\"text\" size=\"70\" name=\"varName\" readonly=\"true\" value=\""));
			response.AppendBody(varName);
			response.AppendBody(_T("\" />"));
		}
		response.AppendBody(_T("</td></tr></table></td></tr><tr><td><table><tr><td>Value</td><td>")
		                    _T("<input type=\"text\" name=\"varValue\" size=\"70\" value=\""));
		request.GetQueryParam(_T("value"), value);
		response.AppendBody(EscapeHTMLText(value));
		response.AppendBody(_T("\" /></td></tr></table></td></tr><tr><td>\r\n"));
		response.AppendBody(_T("<table class=\"button_row\"><tr><td>\r\n"));
		AddButton(response, m_sid, _T("ok"), _T("OK"), _T("editVarButtonHandler"));
		response.AppendBody(_T("</td><td>\r\n"));
		AddButton(response, m_sid, _T("cancel"), _T("Cancel"), _T("editVarButtonHandler"));
		response.AppendBody(_T("</td></tr></table></td></tr></table></form>\r\n"));
		response.EndBox();
		response.AppendBody(_T("</div>\r\n"));
	}
	else
	{
		if (request.GetQueryParam(_T("update"), varName))
		{
			if (request.GetQueryParam(_T("value"), value))
			{
				dwResult = NXCSetServerVariable(m_hSession, varName, value);
				if (dwResult != RCC_SUCCESS)
				{
					ShowErrorMessage(response, dwResult);
				}
			}
			else
			{
				ShowErrorMessage(response, RCC_INTERNAL_ERROR);
			}
		}
		else if (request.GetQueryParam(_T("delete"), varName))
		{
			TCHAR *pErr, *pCurr = varName;
			size_t nLen;

			// Expect variable names encoded as *<len>.<name>
			while(1)
			{
				if (*pCurr != _T('*'))
					break;
				pCurr++;
				nLen = _tcstol(pCurr, &pErr, 10);
				if (*pErr != _T('.'))
					break;
				pCurr = pErr + 1;
				if ((_tcslen(pCurr) < nLen) || (nLen > 255))
					break;
				memcpy(szTemp, pCurr, sizeof(TCHAR) * nLen);
				szTemp[nLen] = 0;
				pCurr += nLen;
				dwResult = NXCDeleteServerVariable(m_hSession, szTemp);
				if (dwResult != RCC_SUCCESS)
				{
					ShowErrorMessage(response, dwResult);
					break;
				}
			}
		}

		dwResult = NXCGetServerVariables(m_hSession, &pVarList, &dwNumVars);
		if (dwResult == RCC_SUCCESS)
		{
			AddActionMenu(response, m_sid,
			              _T("New..."), _T("new.png"), _T("newServerVar"), _T(""),
			              _T("Delete"), _T("delete.png"), _T("deleteServerVar"), _T(""),
							  NULL);
			response.AppendBody(_T("<div id=\"ctrlpanel_data\">\r\n"));

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
				AddCheckbox(response, i, pVarList[i].szName, _T("Select variable"), _T("null"), FALSE);
				row = _T("");
				row.AddFormattedString(_T("</td><td>%s</td><td>%s</td><td>%s</td><td>"),
											  pVarList[i].szName, pVarList[i].szValue,
											  pVarList[i].bNeedRestart ? _T("Yes") : _T("No"));
				response.AppendBody(row);
				_stprintf(szTemp, _T(",'%s','%s'"), pVarList[i].szName, pVarList[i].szValue);
				AddActionLink(response, m_sid, _T("Edit"), _T("edit.png"), _T("editServerVar"), szTemp);
				response.AppendBody(_T("</td></tr>\r\n"));
			}

			response.EndBox(bReload);
			safe_free(pVarList);

			// Show control buttons
			if (!bReload)
			{
				response.AppendBody(_T("<table class=\"button_row\"><tr><td>\r\n"));
				AddButton(response, m_sid, _T("add"), _T("Add new variable"), _T("newServerVar"));
				response.AppendBody(_T("</td><td>\r\n"));
				AddButton(response, m_sid, _T("delete"), _T("Delete selected variables"), _T("deleteServerVar"));
				response.AppendBody(_T("</td></tr></table>\r\n"));
			}
			response.AppendBody(_T("</div>"));	// Close ctrlpanel_data div
		}
		else
		{
			ShowErrorMessage(response, dwResult);
		}
	}
}


//
// Compare two users
//

static int CompareUsers(const void *p1, const void *p2)
{
	return _tcsicmp(((NXC_USER *)p1)->szName, ((NXC_USER *)p2)->szName);
}


//
// Manage users
//

void ClientSession::CtrlPanelUsers(HttpRequest &request, HttpResponse &response, BOOL bUsers)
{
	String id, row;
	DWORD dwResult, dwNumUsers, i;
	NXC_USER *pUserList;
	TCHAR szTemp[256];
	BOOL bReload = FALSE;

	if (request.GetQueryParam(_T("edit"), id))
	{
	}
	else
	{
		if (NXCGetUserDB(m_hSession, &pUserList, &dwNumUsers))
		{
			AddActionMenu(response, m_sid,
			              _T("New..."), _T("new.png"), _T("newUser"), _T(""),
			              _T("Delete"), _T("delete.png"), _T("deleteUser"), _T(""),
							  NULL);
			response.AppendBody(_T("<div id=\"ctrlpanel_data\">\r\n"));

			qsort(pUserList, dwNumUsers, sizeof(NXC_USER), CompareUsers);

			response.StartBox(NULL, _T("objectTable"), _T("user_list"), NULL, bReload);
			response.StartTableHeader(NULL);
			if (bUsers)
			{
				response.AppendBody(_T("<td></td>\r\n")
										  _T("<td valign=\"center\">Login</td>")
										  _T("<td valign=\"center\">Full Name</td>")
										  _T("<td valign=\"center\">Description</td>")
										  _T("<td valign=\"center\">Status</td>")
										  _T("<td></td></tr>\r\n"));
			}
			else
			{
				response.AppendBody(_T("<td></td>\r\n")
										  _T("<td valign=\"center\">Name</td>")
										  _T("<td valign=\"center\">Description</td>")
										  _T("<td valign=\"center\">Status</td>")
										  _T("<td></td></tr>\r\n"));
			}

			for(i = 0; i < dwNumUsers; i++)
			{
				if ((bUsers && !(pUserList[i].dwId & GROUP_FLAG)) ||
				    (!bUsers && (pUserList[i].dwId & GROUP_FLAG)))
				{
					response.StartBoxRow();
					response.AppendBody(_T("<td>"));
					_stprintf(szTemp, _T("%d"), pUserList[i].dwId);
					AddCheckbox(response, i, szTemp, bUsers ? _T("Select user") : _T("Select group"), _T("onUserSelect"), FALSE);
					row = _T("");
					if (bUsers)
					{
						row.AddFormattedString(_T("</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>"),
													  pUserList[i].szName, pUserList[i].szFullName,
													  pUserList[i].szDescription,
													  pUserList[i].wFlags & UF_DISABLED ? _T("Disabled") : _T("Active"));
					}
					else
					{
						row.AddFormattedString(_T("</td><td>%s</td><td>%s</td><td>%s</td><td>"),
													  pUserList[i].szName, pUserList[i].szDescription,
													  pUserList[i].wFlags & UF_DISABLED ? _T("Disabled") : _T("Active"));
					}
					response.AppendBody(row);
					_stprintf(szTemp, _T(",%d"), pUserList[i].dwId);
					AddActionLink(response, m_sid, _T("Edit"), _T("edit.png"), _T("editUser"), szTemp);
					response.AppendBody(_T("</td></tr>\r\n"));
				}
			}

			response.EndBox(bReload);
		}
		else
		{
			ShowErrorMessage(response, RCC_ACCESS_DENIED);
		}
	}
}
