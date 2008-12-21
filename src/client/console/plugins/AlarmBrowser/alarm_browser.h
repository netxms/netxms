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
** File: alarm_browser.h
**
**/

#ifndef _alarm_browser_h_
#define _alarm_browser_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nxclapi.h>
#include <nxmc_api.h>


//
// Alarm view class
//

class nxAlarmView : public nxView
{
private:
	wxListView *m_wndListCtrl;
	long m_sortMode;
	long m_sortDir;
	wxString m_context;
	NXC_OBJECT *m_object;

	void AddAlarm(NXC_ALARM *alarm);
	void UpdateAlarm(long item, NXC_ALARM *alarm);
	void SortAlarmList();
	bool MatchAlarm(NXC_ALARM *pAlarm);

public:
	nxAlarmView(wxWindow *parent, const TCHAR *context, NXC_OBJECT *object = NULL);
	virtual ~nxAlarmView();

	virtual void RefreshView();

	// Event handlers
protected:
	void OnSize(wxSizeEvent &event);
	void OnListColumnClick(wxListEvent &event);
	void OnContextMenu(wxContextMenuEvent &event);
	void OnAlarmChange(wxCommandEvent &event);
	void OnAlarmAck(wxCommandEvent &event);
	void OnUpdateAlarmAck(wxUpdateUIEvent &event);
	void OnAlarmTerminate(wxCommandEvent &event);
	void OnUpdateAlarmTerminate(wxUpdateUIEvent &event);
	void OnAlarmDelete(wxCommandEvent &event);
	void OnUpdateAlarmDelete(wxUpdateUIEvent &event);
	void OnAlarmIncreaseFont(wxCommandEvent &event);
	void OnAlarmDecreaseFont(wxCommandEvent &event);

	DECLARE_EVENT_TABLE()
};


//
// Alarm browser class
//

class nxAlarmBrowser : public nxView
{
private:
	nxAlarmView *m_view;
	
public:
	nxAlarmBrowser();
	virtual ~nxAlarmBrowser();

	// Event handlers
protected:
	void OnSize(wxSizeEvent &event);
	void OnViewRefresh(wxCommandEvent &event);

	DECLARE_EVENT_TABLE()
};

#endif
