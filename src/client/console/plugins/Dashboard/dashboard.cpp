/* 
** NetXMS - Network Management System
** Portable management console - Dashboard plugin
** Copyright (C) 2008 Victor Kirhenshtein
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
** File: dashboard.cpp
**
**/

#include "dashboard.h"


//
// Event table
//

BEGIN_EVENT_TABLE(nxDashboard, nxView)
	EVT_SIZE(nxDashboard::OnSize)
	EVT_NX_REFRESH_VIEW(nxDashboard::OnViewRefresh)
END_EVENT_TABLE()


//
// Constructor
//

nxDashboard::nxDashboard(wxWindow *parent)
            : nxView((parent != NULL) ? parent : NXMCGetDefaultParent())
{
	wxBoxSizer *sizer;

	SetName(_T("dashboard"));
	SetLabel(_T("Dashboard"));
	SetIcon(wxXmlResource::Get()->LoadIcon(_T("icoDashboard")));
	RegisterUniqueView(_T("dashboard"), this);

	m_alarmOverview = new nxAlarmOverview(this);
	m_alarmView = NXMCCreateViewByClass(_T("AlarmView"), this, _T("/Dashboard"), NULL, NULL);

	sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(m_alarmOverview, 0, wxEXPAND | wxALL, 10);
	sizer->Add(m_alarmView, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

	SetSizer(sizer);

	wxCommandEvent event(nxEVT_REFRESH_VIEW);
	AddPendingEvent(event);
}


//
// Destructor
//

nxDashboard::~nxDashboard()
{
	UnregisterUniqueView(_T("dashboard"));
}


//
// Resize handler
//

void nxDashboard::OnSize(wxSizeEvent &event)
{
	Layout();
}


//
// View->Refresh menu handler
//

void nxDashboard::OnViewRefresh(wxCommandEvent &event)
{
	m_alarmView->RefreshView();
	m_alarmOverview->RefreshView();
}
