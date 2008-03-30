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
// Page IDs
//

#define OBJECT_PAGE_OVERVIEW		0
#define OBJECT_PAGE_ALARMS			1
#define OBJECT_PAGE_LAST_VALUES  2


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

	SetOwnFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, _T("Verdana")));

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
		dc.SetFont(GetFont());
		dc.DrawLabel(m_object->szName, wxRect(5, 0, size.x - 10, m_headerOffset), wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
	}
}


//
// Set current object
//

void nxObjectView::SetObject(NXC_OBJECT *object)
{
	nxView *view;
	wxWindow *wnd;
	int page;
	size_t index;
	
	m_object = object;
	RefreshRect(wxRect(0, 0, GetClientSize().x, m_headerOffset), false);

	Freeze();

	index = m_notebook->GetSelection();
	wnd = (index != wxNOT_FOUND) ? m_notebook->GetPage(index) : NULL;
	page = (wnd != NULL) ? wnd->GetId() : OBJECT_PAGE_OVERVIEW;

	while(m_notebook->GetPageCount() > 0)
		m_notebook->DeletePage(0);

	wnd = new nxObjectOverview(m_notebook, object);
	wnd->SetId(OBJECT_PAGE_OVERVIEW);
	m_notebook->AddPage(wnd, _T("Overview"), page == OBJECT_PAGE_OVERVIEW,
	                    wxXmlResource::Get()->LoadIcon(_T("icoSmallInformation")));

	if ((object->iClass == OBJECT_NETWORK) || (object->iClass == OBJECT_SUBNET) ||
	    (object->iClass == OBJECT_NODE) || (object->iClass == OBJECT_SERVICEROOT) ||
	    (object->iClass == OBJECT_CONTAINER) || (object->iClass == OBJECT_ZONE))
	{
		view = NXMCCreateViewByClass(_T("AlarmView"), m_notebook, _T("ObjectBrowser"), object, NULL);
		if (view != NULL)
		{
			view->SetId(OBJECT_PAGE_ALARMS);
			m_notebook->AddPage(view, _T("Alarms"), page == OBJECT_PAGE_ALARMS, view->GetIcon());
			view->RefreshView();
		}
	}

	if (object->iClass == OBJECT_NODE)
	{
		wnd = new nxNodeLastValues(m_notebook, object);
		wnd->SetId(OBJECT_PAGE_LAST_VALUES);
		m_notebook->AddPage(wnd, _T("Last Values"), page == OBJECT_PAGE_LAST_VALUES,
								  wxXmlResource::Get()->LoadIcon(_T("icoLastValues")));
	}

	Thaw();
}


//
// Object updated
//

void nxObjectView::ObjectUpdated()
{
	size_t i;
	wxWindow *page;
	
	RefreshRect(wxRect(0, 0, GetClientSize().x, m_headerOffset), false);
	for(i = 0; i < m_notebook->GetPageCount(); i++)
	{
		page = m_notebook->GetPage(i);
		if (page->GetId() == OBJECT_PAGE_OVERVIEW)
			((nxObjectOverview *)page)->SetObject(m_object);	// Set same object again to cause view refresh
	}
}

