/* 
** NetXMS - Network Management System
** Portable management console - Alarm Browser plugin
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
** File: browser.cpp
**
**/

#include "alarm_browser.h"


//
// Event table
//

BEGIN_EVENT_TABLE(nxAlarmBrowser, nxView)
	EVT_SIZE(nxAlarmBrowser::OnSize)
	EVT_NX_REFRESH_VIEW(nxAlarmBrowser::OnViewRefresh)
END_EVENT_TABLE()


//
// Constructor
//

nxAlarmBrowser::nxAlarmBrowser()
               : nxView(NXMCGetDefaultParent())
{
	SetName(_T("alarmbrowser"));
	SetLabel(_T("Alarm Browser"));
	RegisterUniqueView(_T("alarmbrowser"), this);

	m_view = new nxAlarmView(this);

	wxCommandEvent event(nxEVT_REFRESH_VIEW);
	AddPendingEvent(event);
}


//
// Destructor
//

nxAlarmBrowser::~nxAlarmBrowser()
{
	UnregisterUniqueView(_T("alarmbrowser"));
}


//
// Resize handler
//

void nxAlarmBrowser::OnSize(wxSizeEvent &event)
{
	wxSize size = GetClientSize();
	m_view->SetSize(0, 0, size.x, size.y);
}


//
// View->Refresh menu handler
//

void nxAlarmBrowser::OnViewRefresh(wxCommandEvent &event)
{
	m_view->RefreshView();
}
