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
// Event table
//

BEGIN_EVENT_TABLE(nxView, wxWindow)
	EVT_TIMER(RQ_TIMER_ID, nxView::OnTimer)
END_EVENT_TABLE()


//
// Constructor
//

nxView::nxView(wxWindow *parent)
       : wxWindow(parent, wxID_ANY,  wxDefaultPosition, wxDefaultSize)
{
	m_icon = wxNullBitmap;
	m_timer = new wxTimer(this, RQ_TIMER_ID);
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
   DWORD dwResult;

	switch(data->dwNumParams)
	{
		case 0:
			dwResult = data->func();
			break;
		case 1:
			dwResult = data->func(data->arg1);
			break;
		case 2:
			dwResult = data->func(data->arg1, data->arg2);
			break;
		case 3:
			dwResult = data->func(data->arg1, data->arg2, data->arg3);
			break;
		case 4:
			dwResult = data->func(data->arg1, data->arg2, data->arg3, data->arg4);
			break;
		case 5:
			dwResult = data->func(data->arg1, data->arg2, data->arg3, 
											data->arg4, data->arg5);
			break;
		case 6:
			dwResult = data->func(data->arg1, data->arg2, data->arg3, 
											data->arg4, data->arg5, data->arg6);
			break;
		case 7:
			dwResult = data->func(data->arg1, data->arg2, data->arg3, 
											data->arg4, data->arg5, data->arg6,
											data->arg7);
		case 8:
			dwResult = data->func(data->arg1, data->arg2, data->arg3, 
											data->arg4, data->arg5, data->arg6,
											data->arg7, data->arg8);
		case 9:
			dwResult = data->func(data->arg1, data->arg2, data->arg3, 
											data->arg4, data->arg5, data->arg6,
											data->arg7, data->arg8, data->arg9);
			break;
	}
	if (data->hWnd != NULL)
		PostMessage(data->hWnd, NXCM_REQUEST_COMPLETED, data->wParam, dwResult);
	if (data->bDynamic)
		free(data);
   return dwResult;
}


//
// Execute async request
//

DWORD nxView::DoRequest(RqData *data)
{
	m_timer->Start(300, true);
	ThreadCreate(RequestThread, 0, data);
}

DWORD nxView::DoRequestArg1(void *func, void *arg1)
{
   RqData rqData;

   rqData.hWnd = NULL;
	rqData.isDynamic = false;
   rqData.numParams = 1;
   rqData.arg1 = arg1;
   rqData.func = (DWORD (*)(...))func;
   return ExecuteRequest(&rqData, pszInfoText);
}


//
// Timer event handler
//

void OnTimer(wxTimerEvent &event)
{
}
