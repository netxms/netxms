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
** File: alarm_overview.cpp
**
**/

#include "dashboard.h"


//
// Event table
//

BEGIN_EVENT_TABLE(nxAlarmOverview, nxView)
	EVT_PAINT(nxAlarmOverview::OnPaint)
END_EVENT_TABLE()


//
// Constructor
//

nxAlarmOverview::nxAlarmOverview(wxWindow *parent)
                : nxView(parent)
{
	memset(m_count, 0, sizeof(int) * 5);

	nxHeading *heading = new nxHeading(this, _T("Alarm Distribution"), wxDefaultPosition, wxSize(PIE_CHART_SIZE + 150, 20));

	m_pie = new wxPieCtrl(this, -1, wxPoint(0, heading->GetSize().y), wxSize(PIE_CHART_SIZE, PIE_CHART_SIZE));
	m_pie->SetHeight(15);
//	m_pie->SetAngle(90);

	SetMinSize(wxSize(PIE_CHART_SIZE + 150, PIE_CHART_SIZE));
	SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));

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
	int i;

	list = NXMCGetAlarmList();
	memset(m_count, 0, sizeof(int) * 5);
	for(it = list->begin(); it != list->end(); it++)
	{
		m_count[it->second->nCurrentSeverity]++;
	}
	NXMCUnlockAlarmList();

	m_pie->m_Series.Clear();
	
	for(i = STATUS_NORMAL; i <= STATUS_CRITICAL; i++)
	{
		part.SetLabel(NXMCGetStatusTextSmall(i));
		part.SetValue(m_count[i]);
		part.SetColour(NXMCGetStatusColor(i));
		m_pie->m_Series.Add(part);
	}
	
	Refresh();
	m_pie->Refresh(false);
}


//
// Paint event handler
//

void nxAlarmOverview::OnPaint(wxPaintEvent &event)
{
	wxPaintDC dc(this);
	wxSize size = GetClientSize();
	int i, y;
	TCHAR text[256];

	for(i = STATUS_NORMAL, y = 25; i <= STATUS_CRITICAL; i++, y += 20)
	{
		wxBrush *brush = new wxBrush(NXMCGetStatusColor(i));
		dc.SetBrush(*brush);
		dc.DrawRectangle(PIE_CHART_SIZE + 10, y + 2, 16, 16);
		dc.SetBrush(wxNullBrush);
		delete brush;

		_sntprintf(text, 256, _T("%s (%d)"), NXMCGetStatusTextSmall(i), m_count[i]);
		dc.DrawLabel(text, wxRect(PIE_CHART_SIZE + 35, y, size.x, 20), wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
	}
}
