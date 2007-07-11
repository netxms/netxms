/* 
** NetXMS - Network Management System
** Portable management console
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
** File: busydlg.cpp
**
**/

#include "nxmc.h"
#include "busydlg.h"


//
// Event table
//

BEGIN_EVENT_TABLE(nxBusyDialog, wxDialog)
	EVT_NX_REQUEST_COMPLETED(OnRequestCompleted)
	EVT_NX_SET_STATUS_TEXT(OnSetStatusText)
END_EVENT_TABLE()


//
// Constructor
//

nxBusyDialog::nxBusyDialog(wxWindow *parent, TCHAR *initialText)
             : wxDialog()
{
	m_rcc = RCC_INTERNAL_ERROR;
	wxXmlResource::Get()->LoadDialog(this, parent, _T("nxBusyDialog"));
	XRCCTRL(*this, "textMessage", wxStaticText)->SetLabel((initialText != NULL) ? initialText : _T("Processing request..."));
}


//
// Handler for nxEVT_REQUEST_COMPLETED event
//

void nxBusyDialog::OnRequestCompleted(wxCommandEvent &event)
{
	m_rcc = CAST_FROM_POINTER(event.GetClientData(), DWORD);
	EndModal(wxID_OK);
}


//
// Handler for nxEVT_SET_STATUS_TEXT event
//

void nxBusyDialog::OnSetStatusText(wxCommandEvent &event)
{
	TCHAR *text;

	text = (TCHAR *)event.GetClientData();
	if (text != NULL)
	{
		XRCCTRL(*this, "textMessage", wxStaticText)->SetLabel(text);
		free(text);
	}
}


//
// Set status text (thread-safe)
//

void nxBusyDialog::SetStatusText(TCHAR *newText)
{
	wxCommandEvent event(nxEVT_SET_STATUS_TEXT);
	event.SetClientData(_tcsdup(CHECK_NULL(newText)));
	AddPendingEvent(event);
}


//
// Report operation completion (thread-safe)
//

void nxBusyDialog::ReportCompletion(DWORD rcc)
{
	wxCommandEvent event(nxEVT_REQUEST_COMPLETED);
	event.SetClientData(CAST_TO_POINTER(rcc, void *));
	AddPendingEvent(event);
}
