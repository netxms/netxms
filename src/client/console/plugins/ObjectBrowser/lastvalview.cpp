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
** File: lastvalview.cpp
**
**/

#include "object_browser.h"


//
// Event table
//

BEGIN_EVENT_TABLE(nxLastValuesView, nxView)
	EVT_SIZE(nxLastValuesView::OnSize)
	EVT_NX_REFRESH_VIEW(nxLastValuesView::OnViewRefresh)
END_EVENT_TABLE()


//
// Constructor
//

nxLastValuesView::nxLastValuesView(NXC_OBJECT *node, wxWindow *parent)
                 : nxView((parent != NULL) ? parent : NXMCGetDefaultParent())
{
	TCHAR buffer[256];

	m_rqId = -1;
	m_node = node;

	m_dciList = new nxLastValuesCtrl(this, _T("LastValuesView"));

	_sntprintf(buffer, 256, _T("lastvalues_%d"), m_node->dwId);
	SetName(buffer);
	RegisterUniqueView(buffer, this);
	
	_sntprintf(buffer, 256, _T("Last Values - [%s]"), m_node->szName);
	SetLabel(buffer);
	
	SetIcon(wxXmlResource::Get()->LoadIcon(_T("icoLastValues")));

	wxCommandEvent event(nxEVT_REFRESH_VIEW);
	AddPendingEvent(event);
}


//
// Destructor
//

nxLastValuesView::~nxLastValuesView()
{
	TCHAR buffer[256];

	_sntprintf(buffer, 256, _T("lastvalues_%d"), m_node->dwId);
	UnregisterUniqueView(buffer);
}


//
// Resize handler
//

void nxLastValuesView::OnSize(wxSizeEvent &event)
{
	wxSize size = event.GetSize();
	m_dciList->SetSize(0, 0, size.x, size.y);
}


//
// View->Refresh menu handler
//

void nxLastValuesView::OnViewRefresh(wxCommandEvent &event)
{
	if (m_rqId != -1)
		return;	// Refresh already in progress

	m_dciList->Clear();
	m_rqId = DoRequestArg4(NXCGetLastValues, (wxUIntPtr)NXMCGetSession(), m_node->dwId,
	                       (wxUIntPtr)&m_dciCount, (wxUIntPtr)&m_dciValues, _T("Cannot get DCI values: %s"));
}


//
// Request completion handler
//

void nxLastValuesView::RequestCompletionHandler(int rqId, DWORD rcc, const TCHAR *errMsg)
{
	if (rqId != m_rqId)	// Sanity check
		return;

	if (rcc == RCC_SUCCESS)
	{
		m_dciList->SetData(m_dciCount, m_dciValues);
	}
	m_rqId = -1;
}
