/* $Id: frame.cpp,v 1.4 2007-08-01 12:13:25 victor Exp $ */
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
** File: frame.cpp
**
**/

#include "nxmc.h"


//
// Event table
//

BEGIN_EVENT_TABLE(nxFrame, wxFrame)
	EVT_SIZE(nxFrame::OnSize)
	EVT_MENU(wxID_TOOL_ATTACH, nxFrame::OnFrameAttach)
	EVT_MENU(wxID_TOOL_CLOSE, nxFrame::OnFrameClose)
END_EVENT_TABLE()


//
// Constructor
//

nxFrame::nxFrame(const wxString& title, nxView *child, int area)
        : wxFrame(NULL, wxID_ANY, title)
{
	SetIcon(child->GetIcon());

	m_toolBar = CreateToolBar(wxTB_FLAT | wxTB_HORIZONTAL | wxTB_TEXT | wxTB_HORZ_LAYOUT);
	m_toolBar->AddTool(wxID_TOOL_ATTACH, _T("Attach"), wxXmlResource::Get()->LoadIcon(_T("icoAttachView")));
//	m_toolBar->SetDropdownMenu(wxID_TOOL_ATTACH, wxXmlResource::Get()->LoadMenu(_T("menuDropdownAttach")));
	m_toolBar->AddTool(wxID_TOOL_CLOSE, _T("Close"), wxXmlResource::Get()->LoadIcon(_T("icoClose")));
	m_toolBar->Realize();

	m_area = area;
	m_child = child;
	m_child->Reparent(this);
}


//
// Resize handler
//

void nxFrame::OnSize(wxSizeEvent &event)
{
	wxSize size = GetClientSize();
	m_child->SetSize(0, 0, size.x, size.y);
}


//
// Handler for "Attach" command
//

void nxFrame::OnFrameAttach(wxCommandEvent &event)
{
	wxGetApp().GetMainFrame()->AttachView(m_child, m_area);
	Close();
}


//
// Handler for "Close" command
//

void nxFrame::OnFrameClose(wxCommandEvent &event)
{
	Close();
}
