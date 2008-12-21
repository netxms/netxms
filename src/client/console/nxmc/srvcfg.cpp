/* 
** NetXMS - Network Management System
** Portable management console
** Copyright (C) 2007 Victor Kirhenshtein
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
** File: srvcfg.cpp
**
**/

#include "nxmc.h"
#include "srvcfg.h"
#include "vareditdlg.h"


//
// Event table
//

BEGIN_EVENT_TABLE(nxServerConfigEditor, nxView)
	EVT_SIZE(nxServerConfigEditor::OnSize)
	EVT_NX_REFRESH_VIEW(nxServerConfigEditor::OnRefreshView)
	EVT_LIST_ITEM_ACTIVATED(wxID_LIST_CTRL, nxServerConfigEditor::OnListItemActivated)
	EVT_CONTEXT_MENU(nxServerConfigEditor::OnContextMenu)
	EVT_MENU(XRCID("menuVarNew"), nxServerConfigEditor::OnVarNew)
	EVT_MENU(XRCID("menuVarEdit"), nxServerConfigEditor::OnVarEdit)
	EVT_UPDATE_UI(XRCID("menuVarEdit"), nxServerConfigEditor::OnUpdateUIVarEdit)
	EVT_MENU(XRCID("menuVarDelete"), nxServerConfigEditor::OnVarDelete)
	EVT_UPDATE_UI(XRCID("menuVarDelete"), nxServerConfigEditor::OnUpdateUIVarDelete)
END_EVENT_TABLE()


//
// Constructor
//

nxServerConfigEditor::nxServerConfigEditor(wxWindow *parent)
                     : nxView(parent)
{
	SetName(_T("srvcfgeditor"));
	SetLabel(_T("Server Settings"));
	SetIcon(wxXmlResource::Get()->LoadIcon(_T("icoSmallConfig")));
	RegisterUniqueView(_T("srvcfgeditor"), this);

	m_varList = NULL;
	m_numVars = 0;

	m_listView = new wxListView(this, wxID_LIST_CTRL, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_HRULES | wxLC_VRULES);
	m_listView->InsertColumn(0, _T("Variable"));
	m_listView->InsertColumn(1, _T("Value"));
	m_listView->InsertColumn(2, _T("Restart"));

	wxCommandEvent event(nxEVT_REFRESH_VIEW);
	AddPendingEvent(event);
}


//
// Destructor
//

nxServerConfigEditor::~nxServerConfigEditor()
{
	UnregisterUniqueView(_T("srvcfgeditor"));
	safe_free(m_varList);
}


//
// Resize handler
//

void nxServerConfigEditor::OnSize(wxSizeEvent &event)
{
	wxSize size = GetClientSize();
	m_listView->SetSize(0, 0, size.x, size.y);
}


//
// Refresh view
//

void nxServerConfigEditor::OnRefreshView(wxCommandEvent &event)
{
	if (IsBusy())
		return;	// Already reloading

	m_listView->DeleteAllItems();
	safe_free(m_varList);
	DoRequestArg3((void *)NXCGetServerVariables, CAST_FROM_POINTER(g_hSession, wxUIntPtr),
	              CAST_FROM_POINTER(&m_varList, wxUIntPtr),
	              CAST_FROM_POINTER(&m_numVars, wxUIntPtr),
	              _T("Error loading server variables: %s"));
}


//
// Variable comparision callback
//

static int CompareVariables(const void *p1, const void *p2)
{
	return _tcsicmp(((NXC_SERVER_VARIABLE *)p1)->szName, ((NXC_SERVER_VARIABLE *)p2)->szName);
}


//
// Request completion handler
//

void nxServerConfigEditor::RequestCompletionHandler(int rqId, DWORD rcc, const TCHAR *errMsg)
{
	DWORD i;
	long item;

	if (rcc == RCC_SUCCESS)
	{
		qsort(m_varList, m_numVars, sizeof(NXC_SERVER_VARIABLE), CompareVariables);
		Freeze();
		for(i = 0; i < m_numVars; i++)
		{
			item = m_listView->InsertItem(0x7FFFFFFF, m_varList[i].szName);
			m_listView->SetItem(item, 1, m_varList[i].szValue);
			m_listView->SetItem(item, 2, m_varList[i].bNeedRestart ? _T("Yes") : _T("No"));
		}
		NXMCAdjustListColumn(m_listView, 0);
		NXMCAdjustListColumn(m_listView, 1);
		Thaw();
	}
	else
	{
		NXMCShowClientError(rcc, errMsg);
	}
}


//
// Context menu handler
//

void nxServerConfigEditor::OnContextMenu(wxContextMenuEvent &event)
{
	wxMenu *menu;

	if (!IsBusy())
	{
		menu = wxXmlResource::Get()->LoadMenu(_T("menuCtxServerCfg"));
		if (menu != NULL)
		{
			PopupMenu(menu);
			delete menu;
		}
	}
}


//
// Handler for "new" menu
//

void nxServerConfigEditor::OnVarNew(wxCommandEvent &event)
{
	nxVarEditDlg dlg(this);

	if (dlg.ShowModal() == wxID_OK)
	{
      DoRequestArg3((void *)NXCSetServerVariable, (wxUIntPtr)g_hSession,
                    (wxUIntPtr)_tcsdup(dlg.m_name.c_str()),
						  (wxUIntPtr)_tcsdup(dlg.m_value.c_str()),
						  _T("Cannot update variable: %s"),
						  DRF_DELETE_ARG2 | DRF_DELETE_ARG3);
	}
}


//
// Handler for "edit" menu
//

void nxServerConfigEditor::OnVarEdit(wxCommandEvent &event)
{
	long item;

	item = m_listView->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (item != -1)
	{
		nxVarEditDlg dlg(this);
		wxListItem info;

		dlg.m_name = m_listView->GetItemText(item);
		info.SetId(item);
		info.SetColumn(1);
		info.SetMask(wxLIST_MASK_TEXT);
		m_listView->GetItem(info);
		dlg.m_value = info.GetText();
		if (dlg.ShowModal() == wxID_OK)
		{
         DoRequestArg3((void *)NXCSetServerVariable, (wxUIntPtr)g_hSession,
                       (wxUIntPtr)_tcsdup(dlg.m_name.c_str()),
							  (wxUIntPtr)_tcsdup(dlg.m_value.c_str()),
							  _T("Cannot update variable: %s"),
							  DRF_DELETE_ARG2 | DRF_DELETE_ARG3);
		}
	}
}

void nxServerConfigEditor::OnUpdateUIVarEdit(wxUpdateUIEvent &event)
{
	event.Enable(m_listView->GetSelectedItemCount() == 1);
}


//
// Handler for "delete" menu
//

void nxServerConfigEditor::OnVarDelete(wxCommandEvent &event)
{
}

void nxServerConfigEditor::OnUpdateUIVarDelete(wxUpdateUIEvent &event)
{
	event.Enable(m_listView->GetSelectedItemCount() > 0);
}


//
// Handler for list item activation
//

void nxServerConfigEditor::OnListItemActivated(wxListEvent &event)
{
	wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, XRCID("menuVarEdit"));
	wxPostEvent(this, evt);
}
