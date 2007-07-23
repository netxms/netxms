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
** File: objview.cpp
**
**/

#include "object_browser.h"


//
// Event table
//

BEGIN_EVENT_TABLE(nxObjectView, wxWindow)
	EVT_SIZE(nxObjectView::OnSize)
	EVT_PAINT(nxObjectView::OnPaint)
END_EVENT_TABLE()


//
// Constructor
//

nxObjectView::nxObjectView(wxWindow *parent)
             : wxWindow(parent, wxID_ANY)
{
	wxImageList *imgList;

	m_object = NULL;
	m_notebook = new wxAuiNotebook(this, wxID_NOTEBOOK_CTRL, wxDefaultPosition, wxDefaultSize, 0);
	m_headerOffset = 33;

	imgList = new wxImageList(16, 16);
	imgList->Add(wxXmlResource::Get()->LoadIcon(_T("icoSmallInformation")));
}


//
// Resize handler
//

void nxObjectView::OnSize(wxSizeEvent &event)
{
	wxSize size = GetClientSize();
	m_notebook->SetSize(0, m_headerOffset, size.x, size.y - m_headerOffset);
}


//
// Paint event handler
//

void nxObjectView::OnPaint(wxPaintEvent &event)
{
	wxPaintDC dc(this);
	wxSize size = GetClientSize();
	wxBrush *brush;

	brush = wxTheBrushList->FindOrCreateBrush(wxColour(0, 0, 128));
	dc.SetBrush(*brush);

	dc.DrawRectangle(0, 0, size.x, m_headerOffset);
	if (m_object != NULL)
	{
		dc.SetTextForeground(*wxWHITE);
		dc.DrawLabel(m_object->szName, wxRect(5, 0, size.x - 10, m_headerOffset), wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
	}
}


//
// Set current object
//

void nxObjectView::SetObject(NXC_OBJECT *object)
{
	m_object = object;
	RefreshRect(wxRect(0, 0, GetClientSize().x, m_headerOffset), false);

	Freeze();

	while(m_notebook->GetPageCount() > 0)
		m_notebook->DeletePage(0);

	m_notebook->AddPage(new nxObjectOverview(m_notebook, object), _T("Overview"), false,
	                    wxXmlResource::Get()->LoadIcon(_T("icoSmallInformation")));

	Thaw();
}
