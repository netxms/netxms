/* 
** NetXMS - Network Management System
** Portable management console - Object Browser plugin
** Copyright (C) 2007, 2008 Victor Kirhenshtein
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
** File: nodelastval.cpp
**
**/

#include "object_browser.h"


//
// Worker thread
//

static THREAD_RESULT THREAD_CALL WorkerThread(void *arg)
{
	DWORD nodeId, rcc, numItems;
	NXC_DCI_VALUE *valueList;

	while(1)
	{
		nodeId = ((nxNodeLastValues *)arg)->GetNextRequest();
		if (nodeId == INVALID_INDEX)
			break;

		rcc = NXCGetLastValues(NXMCGetSession(), nodeId, &numItems, &valueList);
		((nxNodeLastValues *)arg)->RequestCompleted(rcc == RCC_SUCCESS, nodeId, numItems, valueList);
	}
	return THREAD_OK;
}


//
// Event table
//

BEGIN_EVENT_TABLE(nxNodeLastValues, wxWindow)
	EVT_SIZE(nxNodeLastValues::OnSize)
	EVT_NX_REQUEST_COMPLETED(nxNodeLastValues::OnRequestCompleted)
	EVT_TIMER(1, nxNodeLastValues::OnTimer)
END_EVENT_TABLE()


//
// Constructor
//

nxNodeLastValues::nxNodeLastValues(wxWindow *parent, NXC_OBJECT *object)
                 : wxWindow(parent, wxID_ANY)
{
	m_dciList = new nxLastValuesCtrl(this, _T("NodeLastValues"));
	m_mutexObject = MutexCreate();
	m_workerThread = ThreadCreateEx(WorkerThread, 0, this);
	m_timer = new wxTimer(this, 1);
	SetObject(object);
}


//
// Destructor
//

nxNodeLastValues::~nxNodeLastValues()
{
	m_workerQueue.Clear();
	m_workerQueue.Put((void *)INVALID_INDEX);
	ThreadJoin(m_workerThread);
	delete m_timer;
	MutexDestroy(m_mutexObject);
}


//
// Set current object
//

void nxNodeLastValues::SetObject(NXC_OBJECT *object)
{
	MutexLock(m_mutexObject, INFINITE);
	m_object = object;
	MutexUnlock(m_mutexObject);
	m_workerQueue.Clear();
	m_dciList->Clear();
	m_workerQueue.Put((void *)object->dwId);
}


//
// Resize handler
//

void nxNodeLastValues::OnSize(wxSizeEvent &event)
{
	wxSize size = event.GetSize();
	m_dciList->SetSize(0, 0, size.x, size.y);
}


//
// Entry point for worker thread on request completion
//

void nxNodeLastValues::RequestCompleted(bool success, DWORD nodeId, DWORD numItems, NXC_DCI_VALUE *valueList)
{
	MutexLock(m_mutexObject, INFINITE);
	if ((m_object != NULL) && (m_object->dwId == nodeId))
	{
		wxCommandEvent event(nxEVT_REQUEST_COMPLETED);
		event.SetInt(success);
		event.SetExtraLong(numItems);
		event.SetClientData(valueList);
		wxPostEvent(this, event);
	}
	else
	{
		// Ignore results
		if (success)
			safe_free(valueList);
	}
	MutexUnlock(m_mutexObject);
}


//
// Handler for request completion in main thread
//

void nxNodeLastValues::OnRequestCompleted(wxCommandEvent &event)
{
	wxLogDebug(_T("nxNodeLastValues::OnRequestCompleted() called"));
	if (event.GetInt())
	{
		m_dciList->SetData(event.GetExtraLong(), (NXC_DCI_VALUE *)event.GetClientData());
	}
	else
	{
		m_dciList->Clear();
	}
	m_timer->Start(15000, wxTIMER_ONE_SHOT);
}


//
// Timer event handler
//

void nxNodeLastValues::OnTimer(wxTimerEvent &event)
{
	MutexLock(m_mutexObject, INFINITE);
	if (m_object != NULL)
		m_workerQueue.Put((void *)m_object->dwId);
	MutexUnlock(m_mutexObject);
}
