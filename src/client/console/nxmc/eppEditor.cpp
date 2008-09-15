/* 
** NetXMS - Network Management System
** Portable management console
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
** File: eppEditor.cpp
**
**/

#include "nxmc.h"
#include "eppEditor.h"


/****************************************************************************
 * Event policy edit control                                                *
 ****************************************************************************/


//
// Event table for editor control
//

BEGIN_EVENT_TABLE(nxEPPEditor, nxPolicyEditor)
END_EVENT_TABLE()


//
// Constructor
//

nxEPPEditor::nxEPPEditor(wxWindow *parent)
            : nxPolicyEditor(parent, 0)
{
	m_table = new nxEvenPolicyTable;
	m_epp = NULL;
	Initialize(m_table);
}


//
// Destructor
//

nxEPPEditor::~nxEPPEditor()
{
}


/****************************************************************************
 * Event policy editor view                                                 *
 ****************************************************************************/


//
// Event table for editor view
//

BEGIN_EVENT_TABLE(nxEventPolicyEditor, nxView)
	EVT_SIZE(nxEventPolicyEditor::OnSize)
	EVT_NX_REFRESH_VIEW(nxEventPolicyEditor::OnRefreshView)
END_EVENT_TABLE()


//
// Constructor
//

nxEventPolicyEditor::nxEventPolicyEditor(wxWindow *parent)
                    : nxView(parent)
{
	SetName(_T("eppeditor"));
	SetLabel(_T("Event Processing Policy"));
	SetIcon(wxXmlResource::Get()->LoadIcon(_T("icoSmallEPP")));
	RegisterUniqueView(_T("eppeditor"), this);

	m_eppEditor = new nxEPPEditor(this);
	m_epp = NULL;
	
	wxCommandEvent event(nxEVT_REFRESH_VIEW);
	AddPendingEvent(event);
}


//
// Destructor
//

nxEventPolicyEditor::~nxEventPolicyEditor()
{
	UnregisterUniqueView(_T("eppeditor"));
   NXCDestroyEventPolicy(m_epp);
}


//
// Resize handler
//

void nxEventPolicyEditor::OnSize(wxSizeEvent &event)
{
	wxSize size = GetClientSize();
	m_eppEditor->SetSize(0, 0, size.x, size.y);
}


//
// Refresh view
//

void nxEventPolicyEditor::OnRefreshView(wxCommandEvent &event)
{
	if (IsBusy())
		return;	// Already reloading

   DoRequestArg2((void *)NXCOpenEventPolicy, (wxUIntPtr)g_hSession, (wxUIntPtr)&m_epp, 
                 _T("Cannot load event processing policy: %s"));
}


//
// Request completion handler
//

void nxEventPolicyEditor::RequestCompletionHandler(int rqId, DWORD rcc, const TCHAR *errMsg)
{
	if (rcc == RCC_SUCCESS)
	{
		m_eppEditor->SetPolicy(m_epp);
	}
	else
	{
		NXMCShowClientError(rcc, errMsg);
	}
}


/****************************************************************************
 * Event policy table                                                       *
 ****************************************************************************/


//
// Event policy table constructor
//

nxEvenPolicyTable::nxEvenPolicyTable()
                  : wxGridTableBase()
{
	m_epp = NULL;

	SetColLabelValue(EPP_COL_RULE, _T("No."));
	SetColLabelValue(EPP_COL_SOURCE, _T("Source"));
	SetColLabelValue(EPP_COL_EVENT, _T("Event"));
	SetColLabelValue(EPP_COL_SEVERITY, _T("Severity"));
	SetColLabelValue(EPP_COL_SCRIPT, _T("Script"));
	SetColLabelValue(EPP_COL_ALARM, _T("Alarm"));
	SetColLabelValue(EPP_COL_SITUATION, _T("Situation"));
	SetColLabelValue(EPP_COL_ACTION, _T("Action"));
	SetColLabelValue(EPP_COL_OPTIONS, _T("Options"));
	SetColLabelValue(EPP_COL_COMMENT, _T("Comments"));
}


//
// Event policy table destructor
//

nxEvenPolicyTable::~nxEvenPolicyTable()
{
}


//
// Get cell value
//

wxString nxEvenPolicyTable::GetValue(int row, int col)
{
	wxString value;

	wxLogDebug(_T("nxEvenPolicyTable::GetValue(%d,%d)"), row, col);

	switch(col)
	{
		case EPP_COL_SOURCE:
			value = _T("10;20");
			break;
		default:
			value = wxString::Format(_T("R%02d:C%02d"), row, col);
			break;
	}

	return value;
}


//
// Set cell value
//

void nxEvenPolicyTable::SetValue(int row, int col, const wxString &value)
{
}


//
// Get type for cell
//

wxString nxEvenPolicyTable::GetTypeName(int row, int col)
{
	wxString value;

	wxLogDebug(_T("nxEvenPolicyTable::GetTypeName(%d,%d)"), row, col);

	switch(col)
	{
		case EPP_COL_SOURCE:
			value = _T("OBJECT_LIST");
			break;
		default:
			value = wxGRID_VALUE_STRING;
			break;
	}

	return value;
}


//
// Update grid
//

void nxEvenPolicyTable::UpdateGrid()
{
	if ((m_epp != NULL) && (GetView() != NULL))
	{
		wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_INSERTED,
                             0, m_epp->dwNumRules);
		GetView()->ProcessTableMessage(msg);
	}
}
