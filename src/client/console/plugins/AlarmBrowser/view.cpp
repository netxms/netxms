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
** File: view.cpp
**
**/

#include "alarm_browser.h"


//
// Event table
//

BEGIN_EVENT_TABLE(nxAlarmView, wxWindow)
	EVT_SIZE(nxAlarmView::OnSize)
	EVT_LIST_ITEM_RIGHT_CLICK(wxID_LIST_CTRL, nxAlarmView::OnListItemRightClick)
END_EVENT_TABLE()


//
// Constructor
//

nxAlarmView::nxAlarmView(wxWindow *parent)
            : wxWindow(parent, wxID_ANY)
{
	m_wndListCtrl = new wxListCtrl(this, wxID_LIST_CTRL, wxDefaultPosition, wxDefaultSize,
	                               wxLC_REPORT | wxLC_HRULES | wxLC_VRULES);
	m_wndListCtrl->InsertColumn(0, _T("Severity"), wxLIST_FORMAT_LEFT, 70);
	m_wndListCtrl->InsertColumn(1, _T("State"), wxLIST_FORMAT_LEFT, 70);
	m_wndListCtrl->InsertColumn(2, _T("Source"), wxLIST_FORMAT_LEFT, 120);
	m_wndListCtrl->InsertColumn(3, _T("Message"), wxLIST_FORMAT_LEFT, 200);
	m_wndListCtrl->InsertColumn(4, _T("Count"), wxLIST_FORMAT_LEFT, 40);
	m_wndListCtrl->InsertColumn(5, _T("Created"), wxLIST_FORMAT_LEFT, 70);
	m_wndListCtrl->InsertColumn(6, _T("Last Change"), wxLIST_FORMAT_LEFT, 70);
	wxImageList *imgList = NXMCGetImageListCopy(IMAGE_LIST_STATUS_SMALL);
	imgList->Add(wxXmlResource::Get()->LoadIcon(_T("icoSmallOutstanding")));
	imgList->Add(wxXmlResource::Get()->LoadIcon(_T("icoSmallAcknowledged")));
	imgList->Add(wxXmlResource::Get()->LoadIcon(_T("icoSmallTerminated")));
	m_wndListCtrl->AssignImageList(imgList, wxIMAGE_LIST_SMALL);
}


//
// Resize handler
//

void nxAlarmView::OnSize(wxSizeEvent &event)
{
	wxSize size = GetClientSize();
	m_wndListCtrl->SetSize(0, 0, size.x, size.y);
}


//
// Refresh view
//

void nxAlarmView::RefreshView()
{
	nxArrayOfAlarms *list;
	size_t i;

	m_wndListCtrl->DeleteAllItems();
	list = NXMCGetAlarmList();

	for(i = 0; i < list->GetCount(); i++)
	{
		AddAlarm((*list)[i]);
	}

	NXMCUnlockAlarmList();
}


//
// Add alarm to view
//

void nxAlarmView::AddAlarm(NXC_ALARM *alarm)
{
	long item;

	item = m_wndListCtrl->InsertItem(0x7FFFFFFF, NXMCGetStatusTextSmall(alarm->nCurrentSeverity), alarm->nCurrentSeverity);
	if (item != -1)
	{
		m_wndListCtrl->SetItemData(item, alarm->dwAlarmId);
		UpdateAlarm(item, alarm);
	}
}


//
// Update alarm in view
//

void nxAlarmView::UpdateAlarm(long item, NXC_ALARM *alarm)
{
	NXC_OBJECT *object;
	TCHAR temp[64];

	m_wndListCtrl->SetItem(item, 0, NXMCGetStatusTextSmall(alarm->nCurrentSeverity), alarm->nCurrentSeverity);
	m_wndListCtrl->SetItem(item, 1, NXMCGetAlarmStateName(alarm->nState), alarm->nState + STATUS_TESTING + 1);
	
	object = NXCFindObjectById(NXMCGetSession(), alarm->dwSourceObject);
	if (object != NULL)
	{
		m_wndListCtrl->SetItem(item, 2, object->szName);
	}
	else
	{
		m_wndListCtrl->SetItem(item, 2, _T("<unknown>"));
	}

	m_wndListCtrl->SetItem(item, 3, alarm->szMessage);

	_stprintf(temp, _T("%d"), alarm->dwRepeatCount);
	m_wndListCtrl->SetItem(item, 4, temp);

	m_wndListCtrl->SetItem(item, 5, NXMCFormatTimeStamp(alarm->dwCreationTime, temp, TS_LONG_DATE_TIME));
	m_wndListCtrl->SetItem(item, 6, NXMCFormatTimeStamp(alarm->dwLastChangeTime, temp, TS_LONG_DATE_TIME));
}


//
// Handler for right click on item
//

void nxAlarmView::OnListItemRightClick(wxListEvent &event)
{
	wxMenu *menu;

	menu = wxXmlResource::Get()->LoadMenu(_T("menuCtxAlarm"));
	if (menu != NULL)
	{
		PopupMenu(menu);
		delete menu;
	}
}
