/* 
** NetXMS - Network Management System
** Portable management console - plugin API library
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
** File: nxview.cpp
**
**/

#include "libnxmc.h"


//
// Constants
//

#define RQ_TIMER_ID		1000


//
// Implementation of RqData class
//

RqData::RqData(int id, wxEvtHandler *owner, void *func, int numArgs, const TCHAR *errMsg, DWORD flags)
{ 
	m_id = id;
	m_owner = owner;
	m_func = (DWORD (*)(...))func;
	m_numArgs = numArgs;
	m_errMsg = (errMsg != NULL) ? _tcsdup(errMsg) : NULL;
	m_flags = flags;
}

RqData::~RqData()
{
	int i;
	DWORD m;

	safe_free(m_errMsg);
	for(i = 0, m = 1; i < 9; i++, m <<= 1)
		if (m_flags & m)
			safe_free((void *)m_arg[i]);
}


//
// Event table
//

BEGIN_EVENT_TABLE(nxView, wxWindow)
	EVT_TIMER(RQ_TIMER_ID, nxView::OnTimer)
	EVT_NX_REQUEST_COMPLETED(nxView::OnRequestCompleted)
END_EVENT_TABLE()


//
// Constructor
//

nxView::nxView(wxWindow *parent, const wxPoint& pos, const wxSize& size)
       : wxWindow(parent, wxID_ANY, pos, size)
{
	m_icon = wxNullIcon;
	m_timer = new wxTimer(this, RQ_TIMER_ID);
	m_activeRequestCount = 0;
	m_freeRqId = 0;
	m_isBusy = false;
}


//
// Destructor
//

nxView::~nxView()
{
	delete m_timer;
}


//
// Set view label
//

void nxView::SetLabel(const wxString& label)
{
	m_label = label;
}


//
// Get view label
//

wxString nxView::GetLabel() const
{
	return m_label;
}


//
// Request processing thread
//

static THREAD_RESULT THREAD_CALL RequestThread(void *arg)
{
   RqData *data = (RqData *)arg;

	switch(data->m_numArgs)
	{
		case 0:
			data->m_rcc = data->m_func();
			break;
		case 1:
			data->m_rcc = data->m_func(data->m_arg[0]);
			break;
		case 2:
			data->m_rcc = data->m_func(data->m_arg[0], data->m_arg[1]);
			break;
		case 3:
			data->m_rcc = data->m_func(data->m_arg[0], data->m_arg[1], data->m_arg[2]);
			break;
		case 4:
			data->m_rcc = data->m_func(data->m_arg[0], data->m_arg[1], data->m_arg[2], data->m_arg[3]);
			break;
		case 5:
			data->m_rcc = data->m_func(data->m_arg[0], data->m_arg[1], data->m_arg[2], 
											data->m_arg[3], data->m_arg[4]);
			break;
		case 6:
			data->m_rcc = data->m_func(data->m_arg[0], data->m_arg[1], data->m_arg[2], 
											data->m_arg[3], data->m_arg[4], data->m_arg[5]);
			break;
		case 7:
			data->m_rcc = data->m_func(data->m_arg[0], data->m_arg[1], data->m_arg[2], 
											data->m_arg[3], data->m_arg[4], data->m_arg[5],
											data->m_arg[6]);
			break;
		case 8:
			data->m_rcc = data->m_func(data->m_arg[0], data->m_arg[1], data->m_arg[2], 
											data->m_arg[3], data->m_arg[4], data->m_arg[5],
											data->m_arg[6], data->m_arg[7]);
			break;
		case 9:
			data->m_rcc = data->m_func(data->m_arg[0], data->m_arg[1], data->m_arg[2], 
											data->m_arg[3], data->m_arg[4], data->m_arg[5],
											data->m_arg[6], data->m_arg[7], data->m_arg[8]);
			break;
	}
	
	wxCommandEvent event(nxEVT_REQUEST_COMPLETED);
	event.SetClientData(data);
	wxPostEvent(data->m_owner, event);
	
   return THREAD_OK;
}


//
// Execute async request
//

int nxView::DoRequest(RqData *data)
{
	if (m_activeRequestCount == 0)
		m_timer->Start(300, true);
	m_activeRequestCount++;
	ThreadCreate(RequestThread, 0, data);
	return data->m_id;
}

int nxView::DoRequestArg1(void *func, wxUIntPtr arg1, const TCHAR *errMsg, DWORD flags)
{
   RqData *data;

	data = new RqData(m_freeRqId++, this, func, 1, errMsg, flags);
   data->m_arg[0] = arg1;
   return DoRequest(data);
}

int nxView::DoRequestArg2(void *func, wxUIntPtr arg1, wxUIntPtr arg2, const TCHAR *errMsg, DWORD flags)
{
   RqData *data;

	data = new RqData(m_freeRqId++, this, func, 2, errMsg, flags);
   data->m_arg[0] = arg1;
   data->m_arg[1] = arg2;
   return DoRequest(data);
}

int nxView::DoRequestArg3(void *func, wxUIntPtr arg1, wxUIntPtr arg2, wxUIntPtr arg3, const TCHAR *errMsg, DWORD flags)
{
   RqData *data;

	data = new RqData(m_freeRqId++, this, func, 3, errMsg, flags);
   data->m_arg[0] = arg1;
   data->m_arg[1] = arg2;
   data->m_arg[2] = arg3;
   return DoRequest(data);
}


//
// Timer event handler
//

void nxView::OnTimer(wxTimerEvent &event)
{
	if (m_activeRequestCount > 0)
	{
		// Set busy mode
		SetCursor(*wxHOURGLASS_CURSOR);
		m_isBusy = true;
	}
}


//
// Request completion event handler
//

void nxView::OnRequestCompleted(wxCommandEvent &event)
{
	nxIntToStringHash::iterator it;
	
	RqData *data = (RqData *)event.GetClientData();
	m_activeRequestCount--;
	if (m_activeRequestCount == 0)
	{
		m_timer->Stop();

		// Exit busy mode
		SetCursor(wxNullCursor);
		m_isBusy = false;
	}
	RequestCompletionHandler(data->m_id, data->m_rcc, data->m_errMsg);
	delete data;
}


//
// Virtual request completion handler
//

void nxView::RequestCompletionHandler(int rqId, DWORD rcc, const TCHAR *errMsg)
{
	if ((rcc != RCC_SUCCESS) && (errMsg != NULL))
	{
		NXMCShowClientError(rcc, errMsg);
	}
}


//
// Virtual refresh view method
//

void nxView::RefreshView()
{
}
