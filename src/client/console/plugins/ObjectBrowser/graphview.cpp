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
** File: graphview.cpp
**
**/

#include "object_browser.h"


//
// Static data
//

static time_t m_timeUnitSize[MAX_TIME_UNITS] = { 60, 3600, 86400 };


//
// Event table
//

BEGIN_EVENT_TABLE(nxGraphView, nxView)
	EVT_SIZE(nxGraphView::OnSize)
	EVT_NX_REFRESH_VIEW(nxGraphView::OnViewRefresh)
	EVT_NX_DCI_DATA_RECEIVED(nxGraphView::OnDataReceived)
	EVT_TIMER(1, nxGraphView::OnProcessingTimer)
	EVT_CONTEXT_MENU(nxGraphView::OnContextMenu)
END_EVENT_TABLE()


//
// Constructor
//

nxGraphView::nxGraphView(int dciCount, DCIInfo **dciList, wxWindow *parent)
            : nxView((parent != NULL) ? parent : NXMCGetDefaultParent())
{
	int i;
	NXC_OBJECT *node;
	TCHAR buffer[64];
	wxString title = _T("History Graph");

	m_rqId = -1;

	m_timeFrameType = 1;	// Back from now
	m_timeFrom = 0;
	m_timeTo = 0;
	m_timeFrame = 3600;	// 1 hour by default

	memset(m_dciInfo, 0, sizeof(DCIInfo *) * MAX_GRAPH_ITEMS);
	m_dciCount = dciCount;
	for(i = 0; i < m_dciCount; i++)
		m_dciInfo[i] = new DCIInfo(dciList[i]);

	// Create title
	if (dciCount == 1)
	{
		node = NXCFindObjectById(NXMCGetSession(), m_dciInfo[0]->m_dwNodeId);
		if (node != NULL)
		{
			title += _T(" - [");
			title += node->szName;
			title += _T(" - ");
			title += m_dciInfo[0]->m_pszDescription;
			title += _T("]");
		}
	}
	else
	{
		for(i = 1; i < m_dciCount; i++)
			if (m_dciInfo[i]->m_dwNodeId != m_dciInfo[0]->m_dwNodeId)
				break;
		if (i == m_dciCount)
		{
			// All DCI's from same node
			node = NXCFindObjectById(NXMCGetSession(), m_dciInfo[0]->m_dwNodeId);
			if (node != NULL)
			{
				title += _T(" - [");
				title += node->szName;
				title += _T("]");
			}
		}
	}
	SetLabel(title);

	// Create unique name
	_sntprintf(buffer, 64, _T("graph_%p"), this);
	SetName(buffer);

	m_graph = new nxGraph(this);
	m_graph->SetDCIInfo(m_dciInfo);

	SetIcon(wxXmlResource::Get()->LoadIcon(_T("icoGraph")));

	m_processingTimer = new wxTimer(this, 1);

	wxCommandEvent event(nxEVT_REFRESH_VIEW);
	AddPendingEvent(event);
}


//
// Destructor
//

nxGraphView::~nxGraphView()
{
	int i;

	for(i = 0; i < MAX_GRAPH_ITEMS; i++)
		delete m_dciInfo[i];
	delete m_processingTimer;
}


//
// Resize handler
//

void nxGraphView::OnSize(wxSizeEvent &event)
{
	wxSize size = event.GetSize();
	m_graph->SetSize(0, 0, size.x, size.y);
}


//
// Get data from server
//

static DWORD GetData(nxGraphView *view, int dciCount, DCIInfo **dciInfo, time_t timeFrom, time_t timeTo)
{
	int i;
	DWORD rcc;
	NXC_DCI_DATA *dciData;

	for(i = 0; i < dciCount; i++)
	{
		rcc = NXCGetDCIData(NXMCGetSession(), dciInfo[i]->m_dwNodeId, dciInfo[i]->m_dwItemId,
		                    0, timeFrom, timeTo, &dciData);
		if (rcc != RCC_SUCCESS)
			break;
		
		wxCommandEvent event(nxEVT_DCI_DATA_RECEIVED);
		event.SetInt(i);
		event.SetClientData(dciData);
		wxPostEvent(view, event);
	}
	return rcc;
}


//
// Handler for received DCI data
//

void nxGraphView::OnDataReceived(wxCommandEvent &event)
{
	m_graph->SetData(event.GetInt(), (NXC_DCI_DATA *)event.GetClientData());
}


//
// View->Refresh menu handler
//

void nxGraphView::OnViewRefresh(wxCommandEvent &event)
{
	if (m_rqId != -1)
		return;	// Refresh already in progress

   // Set new time frame
   if (m_timeFrameType == 1)
   {
      m_timeTo = time(NULL);
      m_timeTo += 60 - m_timeTo % 60;   // Round to minute boundary
      m_timeFrom = m_timeTo - m_timeFrame;
   }
   m_graph->SetTimeFrame(m_timeFrom, m_timeTo);

	m_rqId = DoRequestArg5((void *)GetData, (wxUIntPtr)this, m_dciCount, (wxUIntPtr)m_dciInfo,
	                       m_timeFrom, m_timeTo, _T("Cannot retrieve collected data: %s"));
	m_graph->SetUpdating(true);
	m_processingTimer->Start(300, wxTIMER_ONE_SHOT);
}


//
// Request completion handler
//

void nxGraphView::RequestCompletionHandler(int rqId, DWORD rcc, const TCHAR *errMsg)
{
	if (rqId != m_rqId)	// Sanity check
		return;

	m_rqId = -1;
	m_graph->SetUpdating(false);
	m_graph->Redraw();
}


//
// Handler for processing timer
//

void nxGraphView::OnProcessingTimer(wxTimerEvent &event)
{
	if (m_rqId != -1)	// Still processing request, show "Update" mark on graph
		m_graph->Refresh(false);
}


//
// Context menu handler
//

void nxGraphView::OnContextMenu(wxContextMenuEvent &event)
{
	wxMenu *menu;

	menu = wxXmlResource::Get()->LoadMenu(_T("menuCtxGraph"));
	if (menu != NULL)
	{
		PopupMenu(menu);
		delete menu;
	}
}


//
// Set time frame to given preset
//

void nxGraphView::Preset(int timeUnit, int numUnits)
{
   m_timeFrameType = 1;   // Back from now
   m_timeUnit = timeUnit;
   m_numTimeUnits = numUnits;
   m_timeFrame = m_numTimeUnits * m_timeUnitSize[m_timeUnit];
//   m_bFullRefresh = TRUE;

	wxCommandEvent event(nxEVT_REFRESH_VIEW);
	AddPendingEvent(event);
}
