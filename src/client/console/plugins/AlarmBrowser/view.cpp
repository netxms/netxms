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

BEGIN_EVENT_TABLE(nxAlarmView, nxView)
	EVT_SIZE(nxAlarmView::OnSize)
	EVT_LIST_ITEM_RIGHT_CLICK(wxID_LIST_CTRL, nxAlarmView::OnListItemRightClick)
	EVT_LIST_COL_CLICK(wxID_LIST_CTRL, nxAlarmView::OnListColumnClick)
	EVT_MENU(XRCID("menuAlarmAck"), nxAlarmView::OnAlarmAck)
	EVT_MENU(XRCID("menuAlarmTerminate"), nxAlarmView::OnAlarmTerminate)
	EVT_MENU(XRCID("menuAlarmDelete"), nxAlarmView::OnAlarmDelete)
END_EVENT_TABLE()


//
// Constructor
//

nxAlarmView::nxAlarmView(wxWindow *parent, const TCHAR *context, NXC_OBJECT *object)
            : nxView(parent)
{
	wxConfigBase *cfg = wxConfig::Get();
	wxString path = cfg->GetPath();

	m_context = context;
	m_object = object;

	cfg->SetPath(m_context);
	cfg->Read(_T("AlarmView/SortMode"), &m_sortMode, 0);
	cfg->Read(_T("AlarmView/SortDir"), &m_sortDir, -1);

	m_wndListCtrl = new wxListView(this, wxID_LIST_CTRL, wxDefaultPosition, wxDefaultSize,
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
	imgList->Add(wxXmlResource::Get()->LoadIcon(_T("icoSortUp")));
	imgList->Add(wxXmlResource::Get()->LoadIcon(_T("icoSortDown")));
	m_wndListCtrl->AssignImageList(imgList, wxIMAGE_LIST_SMALL);
	NXMCLoadListCtrlColumns(cfg, *m_wndListCtrl, _T("AlarmView"));

	cfg->SetPath(path);
	
	SetIcon(wxXmlResource::Get()->LoadIcon(_T("icoAlarmView")));

	NXMCEvtConnect(nxEVT_NXC_ALARM_CHANGE, wxCommandEventHandler(nxAlarmView::OnAlarmChange), this);
}


//
// Destructor
//

nxAlarmView::~nxAlarmView()
{
	NXMCEvtDisconnect(nxEVT_NXC_ALARM_CHANGE, wxCommandEventHandler(nxAlarmView::OnAlarmChange), this);

	wxConfigBase *cfg = wxConfig::Get();
	wxString path = cfg->GetPath();
	cfg->SetPath(m_context);
	cfg->Write(_T("AlarmView/SortMode"), m_sortMode);
	cfg->Write(_T("AlarmView/SortDir"), m_sortDir);
	NXMCSaveListCtrlColumns(cfg, *m_wndListCtrl, _T("AlarmView"));
	cfg->SetPath(path);
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
	nxAlarmList *list;
	nxAlarmList::iterator it;

	m_wndListCtrl->DeleteAllItems();
	list = NXMCGetAlarmList();
	
	for(it = list->begin(); it != list->end(); it++)
	{
		if (MatchAlarm(it->second))
			AddAlarm(it->second);
	}

	NXMCUnlockAlarmList();

	SortAlarmList();
}


//
// Check if alarm matched to current filter settings
//

bool nxAlarmView::MatchAlarm(NXC_ALARM *alarm)
{
	if (m_object == NULL)
		return true;
	if (m_object->dwId == alarm->dwSourceObject)
		return true;
	return NXCIsParent(NXMCGetSession(), m_object->dwId, alarm->dwSourceObject) ? true : false;
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

	if (!IsBusy())
	{
		menu = wxXmlResource::Get()->LoadMenu(_T("menuCtxAlarm"));
		if (menu != NULL)
		{
			PopupMenu(menu);
			delete menu;
		}
	}
}


//
// Alarm comparision callback
//

static nxAlarmList *s_alarmList;

static int wxCALLBACK CompareAlarms(long item1, long item2, long sortData)
{
	long mode = sortData & 0x7FFF;
	int rc;
	nxAlarmList::iterator it1, it2;
	NXC_OBJECT *obj1, *obj2;

	it1 = s_alarmList->find(item1);
	it2 = s_alarmList->find(item2);
	if ((it1 == s_alarmList->end()) || (it2 == s_alarmList->end()))
	{
		wxLogDebug(_T("CompareAlarms: invalid iterator returned"));
		return 0;	// Shouldn't happen
	}

	switch(mode)
	{
		case 0:		// Severity
			rc = COMPARE_NUMBERS(it1->second->nCurrentSeverity, it2->second->nCurrentSeverity);
			break;
		case 1:		// State
			rc = COMPARE_NUMBERS(it1->second->nState, it2->second->nState);
			break;
		case 2:		// Source
			obj1 = NXCFindObjectById(NXMCGetSession(), it1->second->dwSourceObject);
			obj2 = NXCFindObjectById(NXMCGetSession(), it2->second->dwSourceObject);
			rc = _tcsicmp((obj1 != NULL) ? obj1->szName : _T("(null)"), (obj2 != NULL) ? obj2->szName : _T("(null)"));
			break;
		case 3:		// Message
			rc = _tcsicmp(it1->second->szMessage, it2->second->szMessage);
			break;
		case 4:		// Count
			rc = COMPARE_NUMBERS(it1->second->dwRepeatCount, it2->second->dwRepeatCount);
			break;
		case 5:		// Created
			rc = COMPARE_NUMBERS(it1->second->dwCreationTime, it2->second->dwCreationTime);
			break;
		case 6:		// Last changed
			rc = COMPARE_NUMBERS(it1->second->dwLastChangeTime, it2->second->dwLastChangeTime);
			break;
		default:
			rc = 0;
			break;
	}

	return (sortData & 0x8000) ? rc : -rc;
}


//
// Sort alarm list
//

void nxAlarmView::SortAlarmList()
{
	s_alarmList = NXMCGetAlarmList();
	m_wndListCtrl->SortItems(CompareAlarms, m_sortMode | ((m_sortDir == 1) ? 0x8000 : 0));
	NXMCUnlockAlarmList();

	// Mark sorting column
	m_wndListCtrl->SetColumnImage(m_sortMode, STATUS_TESTING + ((m_sortDir == 1) ? 4 : 5));
}


//
// Handler for column click
//

void nxAlarmView::OnListColumnClick(wxListEvent &event)
{
	m_wndListCtrl->ClearColumnImage(m_sortMode);

	if (m_sortMode == event.GetColumn())
	{
		m_sortDir = -m_sortDir;
	}
	else
	{
		m_sortMode = event.GetColumn();
	}
	SortAlarmList();
}


//
// Handler for alarm change event
//

void nxAlarmView::OnAlarmChange(wxCommandEvent &event)
{
	long item;
	NXC_ALARM *alarm;
	
	alarm = (NXC_ALARM *)event.GetClientData();
	switch(event.GetInt())
	{
		case NX_NOTIFY_NEW_ALARM:
		case NX_NOTIFY_ALARM_CHANGED:
			item = m_wndListCtrl->FindItem(-1, alarm->dwAlarmId);
			if (item != -1)
			{
				if (MatchAlarm(alarm))
					UpdateAlarm(item, (NXC_ALARM *)event.GetClientData());
				else
					m_wndListCtrl->DeleteItem(item);		// Alarm no longer match filter
			}
			else
			{
				if (MatchAlarm(alarm))
					AddAlarm(alarm);
			}
			SortAlarmList();
			break;
		case NX_NOTIFY_ALARM_TERMINATED:
		case NX_NOTIFY_ALARM_DELETED:
			item = m_wndListCtrl->FindItem(-1, alarm->dwAlarmId);
			if (item != -1)
				m_wndListCtrl->DeleteItem(item);
			break;
	}
	event.Skip();
}


//
// Handler for acknowledge alarm menu
//

static DWORD AckAlarms(int count, DWORD *list)
{
	int i;
	DWORD rcc;
	NXC_SESSION session = NXMCGetSession();
	
	for(i = 0; i < count; i++)
	{
		rcc = NXCAcknowledgeAlarm(session, list[i]);
		if (rcc != RCC_SUCCESS)
			break;
	}
	delete list;
	return rcc;
}

void nxAlarmView::OnAlarmAck(wxCommandEvent &event)
{
	int i, count = m_wndListCtrl->GetSelectedItemCount();
	DWORD *idList = new DWORD[count];
	long item;

	for(i = 0, item = -1; i < count; i++)
	{
		item = m_wndListCtrl->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;
		idList[i] = m_wndListCtrl->GetItemData(item);
	}
	DoRequestArg2((void *)AckAlarms, count, CAST_FROM_POINTER(idList, wxUIntPtr), _T("Cannot acknowledge alarm: %s"));
}


//
// Handler for terminate alarm menu
//

static DWORD TerminateAlarms(int count, DWORD *list)
{
	int i;
	DWORD rcc;
	NXC_SESSION session = NXMCGetSession();
	
	for(i = 0; i < count; i++)
	{
		rcc = NXCTerminateAlarm(session, list[i]);
		if (rcc != RCC_SUCCESS)
			break;
	}
	delete list;
	return rcc;
}

void nxAlarmView::OnAlarmTerminate(wxCommandEvent &event)
{
	int i, count = m_wndListCtrl->GetSelectedItemCount();
	DWORD *idList = new DWORD[count];
	long item;

	for(i = 0, item = -1; i < count; i++)
	{
		item = m_wndListCtrl->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;
		idList[i] = m_wndListCtrl->GetItemData(item);
	}
	DoRequestArg2((void *)TerminateAlarms, count, CAST_FROM_POINTER(idList, wxUIntPtr), _T("Cannot terminate alarm: %s"));
}


//
// Handler for delete alarm menu
//

static DWORD DeleteAlarms(int count, DWORD *list)
{
	int i;
	DWORD rcc;
	NXC_SESSION session = NXMCGetSession();
	
	for(i = 0; i < count; i++)
	{
		rcc = NXCDeleteAlarm(session, list[i]);
		if (rcc != RCC_SUCCESS)
			break;
	}
	delete list;
	return rcc;
}

void nxAlarmView::OnAlarmDelete(wxCommandEvent &event)
{
	int i, count = m_wndListCtrl->GetSelectedItemCount();
	DWORD *idList = new DWORD[count];
	long item;

	for(i = 0, item = -1; i < count; i++)
	{
		item = m_wndListCtrl->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;
		idList[i] = m_wndListCtrl->GetItemData(item);
	}
	DoRequestArg2((void *)DeleteAlarms, count, CAST_FROM_POINTER(idList, wxUIntPtr), _T("Cannot delete alarm: %s"));
}
