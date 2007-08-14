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
** File: ctrlpanel.cpp
**
**/

#include "nxmc.h"
#include "ctrlpanel.h"
#include "srvcfg.h"


//
// Event table
//

BEGIN_EVENT_TABLE(nxControlPanel, nxView)
	EVT_SIZE(nxControlPanel::OnSize)
	EVT_LIST_ITEM_ACTIVATED(wxID_LIST_CTRL, nxControlPanel::OnListItemActivated)
END_EVENT_TABLE()


//
// Constructor
//

nxControlPanel::nxControlPanel(wxWindow *parent)
               : nxView(parent)
{
	size_t i;
	wxImageList *imageList;
	
	SetName(_T("ctrlpanel"));
	SetLabel(_T("Control Panel"));
	RegisterUniqueView(_T("ctrlpanel"), this);

	imageList = new wxImageList(32, 32);
	imageList->Add(wxXmlResource::Get()->LoadIcon(_T("icoUnknown")));
	
	m_wndListCtrl = new wxListView(this, wxID_LIST_CTRL, wxDefaultPosition, wxDefaultSize, wxLC_ICON | wxLC_AUTOARRANGE | wxLC_SINGLE_SEL);
	
	// Add built-in items
	AddItem(wxID_CTRLPANEL_SERVERCFG, _T("Server Settings"), wxXmlResource::Get()->LoadIcon(_T("icoConfig")), imageList);

	// Add items registered by plugins
	nxmcArrayOfRegItems &regList = NXMCGetRegistrations();
	for(i = 0; i < regList.GetCount(); i++)
	{
		if (regList[i].GetType() == REGITEM_CONTROL_PANEL)
		{
			AddItem(regList[i].GetId() + ((nxmcPlugin *)regList[i].GetPlugin())->GetBaseId(), regList[i].GetName(), wxNullIcon, imageList);
		}
	}
	
	m_wndListCtrl->AssignImageList(imageList, wxIMAGE_LIST_NORMAL);
}


//
// Destructor
//

nxControlPanel::~nxControlPanel()
{
	UnregisterUniqueView(_T("ctrlpanel"));
}


//
// Add new item to list
//

void nxControlPanel::AddItem(int cmd, const wxString &text, const wxIcon &icon, wxImageList *imglist)
{
	long item;
	
	if (icon.IsOk())
	{
		imglist->Add(icon);
		item = m_wndListCtrl->InsertItem(0x7FFFFFFF, text, imglist->GetImageCount() - 1);
	}
	else
	{
		item = m_wndListCtrl->InsertItem(0x7FFFFFFF, text, 0);
	}
	m_wndListCtrl->SetItemData(item, cmd);
}


//
// Resize handler
//

void nxControlPanel::OnSize(wxSizeEvent &event)
{
	wxSize size = GetClientSize();
	m_wndListCtrl->SetSize(0, 0, size.x, size.y);
}


//
// Handler for item activation
//

void nxControlPanel::OnListItemActivated(wxListEvent &event)
{
	int cmd;
	nxView *view;

	cmd = event.GetData();
	if ((cmd >= wxID_PLUGIN_RANGE_START) && (cmd <= wxID_PLUGIN_RANGE_END))
	{
		CallPluginCommandHandler(cmd);
	}
	else
	{
		switch(cmd)
		{
			case wxID_CTRLPANEL_SERVERCFG:
				view = FindUniqueView(_T("srvcfgeditor"));
				if (view != NULL)
				{
					ActivateView(view);
				}
				else
				{
					NXMCCreateView(new nxServerConfigEditor(this), VIEWAREA_MAIN);
				}
				break;
			default:
				break;
		}
	}
}
