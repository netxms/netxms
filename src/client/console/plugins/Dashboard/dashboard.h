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
** File: dashboard.h
**
**/

#ifndef _dashboard_h_
#define _dashboard_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nxclapi.h>
#include <nxmc_api.h>
#include <wxPieCtrl.h>


//
// Alarm summary view
//

class nxAlarmOverview : public nxView
{
private:
	wxPieCtrl *m_pie;
	
public:
	nxAlarmOverview(wxWindow *parent);
	virtual ~nxAlarmOverview();

	virtual void RefreshView();

	// Event handlers
protected:
//	void OnSize(wxSizeEvent &event);
	void OnAlarmChange(wxCommandEvent &event);

	DECLARE_EVENT_TABLE()
};


//
// Dashboard class
//

class nxDashboard : public nxView
{
private:
	nxView *m_alarmView;
	nxAlarmOverview *m_alarmOverview;

public:
	nxDashboard(wxWindow *parent = NULL);
	virtual ~nxDashboard();

	// Event handlers
protected:
	void OnSize(wxSizeEvent &event);
	void OnViewRefresh(wxCommandEvent &event);

	DECLARE_EVENT_TABLE()
};

#endif
