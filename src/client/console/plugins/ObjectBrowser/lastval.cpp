/* 
** NetXMS - Network Management System
** Portable management console - Object Browser plugin
** Copyright (C) 2007, 2008 Victor Kirhenshtein
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
** File: lastval.cpp
**
**/

#include "object_browser.h"


//
// Event table
//

BEGIN_EVENT_TABLE(nxNodeLastValues, wxWindow)
	EVT_SIZE(nxNodeLastValues::OnSize)
END_EVENT_TABLE()


//
// Constructor
//

nxNodeLastValues::nxNodeLastValues(wxWindow *parent, NXC_OBJECT *object)
                 : wxWindow(parent, wxID_ANY)
{
	m_dciList = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxNO_BORDER);
	m_dciList->InsertColumn(0, _T("ID"), wxLIST_FORMAT_LEFT, 60);
	m_dciList->InsertColumn(1, _T("Description"), wxLIST_FORMAT_LEFT, wxLIST_AUTOSIZE);
	m_dciList->InsertColumn(2, _T("Value"), wxLIST_FORMAT_LEFT, wxLIST_AUTOSIZE);
	m_dciList->InsertColumn(3, _T("Timestamp"), wxLIST_FORMAT_LEFT, wxLIST_AUTOSIZE);
}


//
// Set current object
//

void nxNodeLastValues::SetObject(NXC_OBJECT *object)
{
	m_object = object;
	m_dciList->DeleteAllItems();
}


//
// Resize handler
//

void nxNodeLastValues::OnSize(wxSizeEvent &event)
{
	wxSize size = event.GetSize();
	m_dciList->SetSize(0, 0, size.x, size.y);
}
