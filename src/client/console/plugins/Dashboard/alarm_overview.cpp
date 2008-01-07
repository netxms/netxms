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

BEGIN_EVENT_TABLE(nxAlarmOverview, nxView)
//	EVT_SIZE(nxAlarmView::OnSize)
END_EVENT_TABLE()


//
// Constructor
//

nxAlarmOverview::nxAlarmOverview(wxWindow *parent)
                : nxView(parent)
{
	NXMCEvtConnect(nxEVT_NXC_ALARM_CHANGE, wxCommandEventHandler(nxAlarmOverview::OnAlarmChange), this);
}


//
// Destructor
//

nxAlarmOverview::~nxAlarmOverview()
{
	NXMCEvtDisconnect(nxEVT_NXC_ALARM_CHANGE, wxCommandEventHandler(nxAlarmOverview::OnAlarmChange), this);
}


//
// Handler for alarm change event
//

void nxAlarmOverview::OnAlarmChange(wxCommandEvent &event)
{
}


//
// Refresh view
//

void nxAlarmOverview::RefreshView()
{
}
