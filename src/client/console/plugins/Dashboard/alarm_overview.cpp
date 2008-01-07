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
	m_pie = new wxPieCtrl(this, -1, wxDefaultPosition, wxSize(150,150));
	m_pie->SetHeight(15);
//	m_pie->SetAngle(90);

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
	RefreshView();
}


//
// Refresh view
//

void nxAlarmOverview::RefreshView()
{
	wxPiePart part; 
	nxAlarmList *list;
	nxAlarmList::iterator it;
	int i, count[5] = { 0, 0, 0, 0, 0 };

	list = NXMCGetAlarmList();
	for(it = list->begin(); it != list->end(); it++)
	{
		count[it->second->nCurrentSeverity]++;
	}
	NXMCUnlockAlarmList();

	m_pie->m_Series.Clear();
	
	for(i = STATUS_NORMAL; i <= STATUS_CRITICAL; i++)
	{
		part.SetLabel(NXMCGetStatusTextSmall(i));
		part.SetValue(count[i]);
		part.SetColour(NXMCGetStatusColor(i));
		m_pie->m_Series.Add(part);
	}
	
	m_pie->Refresh(false);
}

