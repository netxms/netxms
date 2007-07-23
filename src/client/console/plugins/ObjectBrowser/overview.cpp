/* 
** NetXMS - Network Management System
** Portable management console - Object Browser plugin
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
** File: overview.cpp
**
**/

#include "object_browser.h"


//
// Header's event table
//

BEGIN_EVENT_TABLE(nxObjOverviewHeader, wxWindow)
	EVT_PAINT(nxObjOverviewHeader::OnPaint)
END_EVENT_TABLE()


//
// Header's constructor
//

nxObjOverviewHeader::nxObjOverviewHeader(wxWindow *parent)
                    : wxWindow(parent, wxID_ANY, wxDefaultPosition, wxSize(100, 40))
{
	m_object = NULL;
	SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
}


//
// Set current object
//

void nxObjOverviewHeader::SetObject(NXC_OBJECT *object)
{
	m_object = object;
	Refresh();
}


//
// Paint header
//

void nxObjOverviewHeader::OnPaint(wxPaintEvent &event)
{
	wxPaintDC dc(this);

	if (m_object == NULL)
		return;
		
	dc.SetFont(GetFont());
	dc.DrawText(m_object->szName, 40, 3);
}


//
// Event table
//

BEGIN_EVENT_TABLE(nxObjectOverview, wxWindow)
	EVT_SIZE(nxObjectOverview::OnSize)
	EVT_TEXT_URL(wxID_TEXT_CTRL, nxObjectOverview::OnTextURL)
END_EVENT_TABLE()


//
// Constructor
//

nxObjectOverview::nxObjectOverview(wxWindow *parent, NXC_OBJECT *object)
                 : wxWindow(parent, wxID_ANY)
{
	wxBoxSizer *sizer;

	SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));

	m_header = new nxObjOverviewHeader(this);

	sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(m_header, 0, wxALL | wxEXPAND, 7);
	sizer->Add(new nxHeading(this, _T("Attributes"), wxDefaultPosition, wxSize(100, 20)), 0, wxALL | wxEXPAND, 7);

	m_attrList = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_NO_HEADER | wxNO_BORDER);
	m_attrList->InsertColumn(0, _T("Name"), wxLIST_FORMAT_LEFT, wxLIST_AUTOSIZE);
	m_attrList->InsertColumn(1, _T("Value"), wxLIST_FORMAT_LEFT, wxLIST_AUTOSIZE);
	sizer->Add(m_attrList, 1, wxALL | wxEXPAND, 7);

	sizer->Add(new nxHeading(this, _T("Comments"), wxDefaultPosition, wxSize(100, 20)), 0, wxALL | wxEXPAND, 7);
	m_comments = new wxTextCtrl(this, wxID_TEXT_CTRL, wxEmptyString, wxDefaultPosition,
	                            wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP | wxTE_RICH | wxTE_AUTO_URL | wxNO_BORDER);
	sizer->Add(m_comments, 1, wxALL | wxEXPAND, 7);

	SetSizer(sizer);
	sizer->SetSizeHints(this);

	SetObject(object);
}


//
// Set current object
//

void nxObjectOverview::SetObject(NXC_OBJECT *object)
{
	TCHAR temp[32];

	m_object = object;
	m_header->SetObject(object);

	m_attrList->DeleteAllItems();
	InsertItem(_T("ID"), m_object->dwId);
	InsertItem(_T("Class"), NXMCGetClassName(m_object->iClass));
	InsertItem(_T("Status"), NXMCGetStatusText(m_object->iStatus));
	if (m_object->dwIpAddr != 0)
		InsertItem(_T("IP Address"), IpToStr(m_object->dwIpAddr, temp));

	// Class-specific information
	switch(m_object->iClass)
	{
		case OBJECT_SUBNET:
			InsertItem(_T("Network Mask"), IpToStr(m_object->subnet.dwIpNetMask, temp));
			break;
		case OBJECT_NODE:
			if (m_object->node.dwFlags & NF_IS_NATIVE_AGENT)
			{
				InsertItem(_T("NetXMS Agent"), _T("Active"));
				InsertItem(_T("Agent Version"), m_object->node.szAgentVersion);
				InsertItem(_T("Platform Name"), m_object->node.szPlatformName);
			}
			else
			{
				InsertItem(_T("NetXMS Agent"), _T("Inactive"));
			}
			if (m_object->node.dwFlags & NF_IS_SNMP)
			{
				InsertItem(_T("SNMP Agent"), _T("Active"));
				InsertItem(_T("SNMP OID"), m_object->node.szObjectId);
			}
			else
			{
				InsertItem(_T("SNMP Agent"), _T("Inactive"));
			}
			InsertItem(_T("Node Type"), NXMCGetNodeTypeName(m_object->node.dwNodeType));
			break;
		case OBJECT_INTERFACE:
			if (m_object->dwIpAddr != 0)
				InsertItem(_T("IP Netmask"), IpToStr(m_object->iface.dwIpNetMask, temp));
			InsertItem(_T("Index"), m_object->iface.dwIfIndex);
			InsertItem(_T("Type"), NXMCGetIfTypeName((int)m_object->iface.dwIfType));
			BinToStr(m_object->iface.bMacAddr, MAC_ADDR_LENGTH, temp);
			InsertItem(_T("MAC Address"), temp);
			break;
		default:
			break;
	}

	m_comments->SetValue(CHECK_NULL_EX(object->pszComments));

	AdjustAttrList();
}


//
// Insert item into attributes list
//

void nxObjectOverview::InsertItem(const TCHAR *name, const TCHAR *value)
{
	long item;

	item = m_attrList->InsertItem(0x7FFFFFFF, name);
	m_attrList->SetItem(item, 1, value);
}

void nxObjectOverview::InsertItem(const TCHAR *name, DWORD value)
{
	TCHAR buffer[32];

	_stprintf(buffer, _T("%d"), value);
	InsertItem(name, buffer);
}


//
// Resize handler
//

void nxObjectOverview::OnSize(wxSizeEvent &event)
{
	Layout();
}


//
// Adjust attribute list
//

void nxObjectOverview::AdjustAttrList()
{
	int i, count, w, h, width;
	wxListItem item;

	count = m_attrList->GetItemCount();
	for(i = 0, width = 0; i < count; i++)
	{
		item.SetId(i);
		item.SetColumn(1);
		item.SetMask(wxLIST_MASK_TEXT);
		m_attrList->GetItem(item);
		m_attrList->GetTextExtent(item.GetText(), &w, &h);
		if (width < w)
			width = w;
	}
	m_attrList->SetColumnWidth(1, width + 20);
}


//
// URL event handler
//

void nxObjectOverview::OnTextURL(wxTextUrlEvent &event)
{
	if (event.GetMouseEvent().LeftDown())
	{
		wxString url = m_comments->GetRange(event.GetURLStart(), event.GetURLEnd());
		wxLogDebug(_T("Opening URL: %s"), url.c_str());
		wxLaunchDefaultBrowser(url, wxBROWSER_NEW_WINDOW);
	}
}
