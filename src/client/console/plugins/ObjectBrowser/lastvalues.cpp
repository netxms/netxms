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
** File: lastvalues.cpp
**
**/

#include "object_browser.h"


//
// Event table
//

BEGIN_EVENT_TABLE(nxLastValuesCtrl, wxWindow)
	EVT_SIZE(nxLastValuesCtrl::OnSize)
	EVT_LIST_COL_CLICK(wxID_LIST_CTRL, nxLastValuesCtrl::OnListColumnClick)
	EVT_CONTEXT_MENU(nxLastValuesCtrl::OnContextMenu)
	EVT_MENU(XRCID("menuDCIGraph"), nxLastValuesCtrl::OnDCIGraph)
	EVT_UPDATE_UI(XRCID("menuDCIGraph"), nxLastValuesCtrl::OnUpdateDCIGraph)
	EVT_MENU(XRCID("menuDCIHistory"), nxLastValuesCtrl::OnDCIHistory)
	EVT_UPDATE_UI(XRCID("menuDCIHistory"), nxLastValuesCtrl::OnUpdateDCIHistory)
	EVT_MENU(XRCID("menuDCIExport"), nxLastValuesCtrl::OnDCIExport)
	EVT_UPDATE_UI(XRCID("menuDCIExport"), nxLastValuesCtrl::OnUpdateDCIExport)
	EVT_MENU(XRCID("menuDCIUseMultipliers"), nxLastValuesCtrl::OnDCIUseMultipliers)
	EVT_UPDATE_UI(XRCID("menuDCIUseMultipliers"), nxLastValuesCtrl::OnUpdateDCIUseMultipliers)
END_EVENT_TABLE()


//
// Constructor
//

nxLastValuesCtrl::nxLastValuesCtrl(wxWindow *parent, const TCHAR *context)
                 : wxWindow(parent, wxID_ANY)
{
	wxConfigBase *cfg = wxConfig::Get();
	wxString path = cfg->GetPath();

	m_context = context;

	cfg->SetPath(m_context);
	cfg->Read(_T("LastValuesCtrl/SortMode"), &m_sortMode, 0);
	cfg->Read(_T("LastValuesCtrl/SortDir"), &m_sortDir, -1);
	cfg->Read(_T("LastValuesCtrl/UseMultipliers"), &m_useMultipliers, true);

	m_wndListCtrl = new wxListView(this, wxID_LIST_CTRL, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxNO_BORDER);
	m_wndListCtrl->InsertColumn(0, _T("ID"), wxLIST_FORMAT_LEFT, 60);
	m_wndListCtrl->InsertColumn(1, _T("Description"), wxLIST_FORMAT_LEFT, wxLIST_AUTOSIZE);
	m_wndListCtrl->InsertColumn(2, _T("Value"), wxLIST_FORMAT_LEFT, wxLIST_AUTOSIZE);
	m_wndListCtrl->InsertColumn(3, _T("Timestamp"), wxLIST_FORMAT_LEFT, wxLIST_AUTOSIZE);

	wxImageList *imgList = new wxImageList(16, 16);
	imgList->Add(wxXmlResource::Get()->LoadIcon(_T("icoItemActive")));
	imgList->Add(wxXmlResource::Get()->LoadIcon(_T("icoItemDisabled")));
	imgList->Add(wxXmlResource::Get()->LoadIcon(_T("icoItemUnsupported")));
	imgList->Add(wxXmlResource::Get()->LoadIcon(_T("icoSortUp")));
	imgList->Add(wxXmlResource::Get()->LoadIcon(_T("icoSortDown")));
	m_wndListCtrl->AssignImageList(imgList, wxIMAGE_LIST_SMALL);
	
	NXMCLoadListCtrlColumns(cfg, *m_wndListCtrl, _T("LastValuesCtrl"));

	// Mark sorting column
	m_wndListCtrl->SetColumnImage(m_sortMode, ((m_sortDir == 1) ? 3 : 4));

	m_data = NULL;

	cfg->SetPath(path);
}


//
// Destructor
//

nxLastValuesCtrl::~nxLastValuesCtrl()
{
	wxConfigBase *cfg = wxConfig::Get();
	wxString path = cfg->GetPath();
	cfg->SetPath(m_context);
	cfg->Write(_T("LastValuesCtrl/SortMode"), m_sortMode);
	cfg->Write(_T("LastValuesCtrl/SortDir"), m_sortDir);
	cfg->Write(_T("LastValuesCtrl/UseMultipliers"), m_useMultipliers);
	NXMCSaveListCtrlColumns(cfg, *m_wndListCtrl, _T("LastValuesCtrl"));
	cfg->SetPath(path);

	safe_free(m_data);
}


//
// Resize handler
//

void nxLastValuesCtrl::OnSize(wxSizeEvent &event)
{
	wxSize size = event.GetSize();
	m_wndListCtrl->SetSize(0, 0, size.x, size.y);
}


//
// Callback for item sorting
// TODO: will not work on 64-bit Windows because long is always 32-bit in MS world
//

static int wxCALLBACK CompareListItemsCB(long item1, long item2, long sortData)
{
	return ((nxLastValuesCtrl *)sortData)->CompareListItems(item1, item2);
}


//
// Compare two list items
//

int nxLastValuesCtrl::CompareListItems(long item1, long item2)
{
	int result;

	if (m_data == NULL)
		return 0;	// Paranoia check

	switch(m_sortMode)
	{
		case 0:	// ID
			result = COMPARE_NUMBERS(m_data[item1].dwId, m_data[item2].dwId);
			break;
		case 1:	// Description
			result = _tcsicmp(m_data[item1].szDescription, m_data[item2].szDescription);
			break;
		case 2:	// Value
			result = _tcsicmp(m_data[item1].szValue, m_data[item2].szValue);
			break;
		case 3:	// Timestamp
			result = COMPARE_NUMBERS(m_data[item1].dwTimestamp, m_data[item2].dwTimestamp);
			break;
		default:
			result = 0;
			break;
	}
	return result * m_sortDir;
}


//
// Set new data
//

void nxLastValuesCtrl::SetData(DWORD nodeId, DWORD dciCount, NXC_DCI_VALUE *valueList)
{
	DWORD i;
	long item;
	TCHAR buffer[64];

	safe_free(m_data);
	m_nodeId = nodeId;
	m_dciCount = dciCount;
	m_data = valueList;

	m_wndListCtrl->DeleteAllItems();
	for(i = 0; i < m_dciCount; i++)
	{
		_sntprintf(buffer, 64, _T("%d"), m_data[i].dwId);
		item = m_wndListCtrl->InsertItem(0x7FFFFFFF, buffer, m_data[i].nStatus);
		if (item != -1)
		{
			m_wndListCtrl->SetItemData(item, i);
			m_wndListCtrl->SetItem(item, 1, m_data[i].szDescription);

			if ((m_data[i].nDataType != DCI_DT_STRING) &&
				 (m_useMultipliers))
			{
				QWORD ui64;
				INT64 i64;
				double d;

				switch(m_data[i].nDataType)
				{
					case DCI_DT_INT:
					case DCI_DT_INT64:
						i64 = _tcstoll(m_data[i].szValue, NULL, 10);
						if (i64 >= _LL(10000000000))
						{
							_sntprintf(buffer, 64, INT64_FMT _T(" G"), i64 / 1000000000);
						}
						else if (i64 >= 10000000)
						{
							_sntprintf(buffer, 64, INT64_FMT _T(" M"), i64 / 1000000);
						}
						else if (i64 >= 10000)
						{
							_sntprintf(buffer, 64, INT64_FMT _T(" K"), i64 / 1000);
						}
						else if (i64 >= 0)
						{
							_sntprintf(buffer, 64, INT64_FMT, i64);
						}
						else if (i64 <= _LL(-10000000000))
						{
							_sntprintf(buffer, 64, INT64_FMT _T(" G"), i64 / 1000000000);
						}
						else if (i64 <= -10000000)
						{
							_sntprintf(buffer, 64, INT64_FMT _T(" M"), i64 / 1000000);
						}
						else if (i64 <= 10000)
						{
							_sntprintf(buffer, 64, INT64_FMT _T(" K"), i64 / 1000);
						}
						else
						{
							_sntprintf(buffer, 64, INT64_FMT, i64);
						}
						break;
					case DCI_DT_UINT:
					case DCI_DT_UINT64:
						ui64 = _tcstoull(m_data[i].szValue, NULL, 10);
						if (ui64 >= _ULL(10000000000))
						{
							_sntprintf(buffer, 64, UINT64_FMT _T(" G"), ui64 / 1000000000);
						}
						else if (ui64 >= 10000000)
						{
							_sntprintf(buffer, 64, UINT64_FMT _T(" M"), ui64 / 1000000);
						}
						else if (ui64 >= 10000)
						{
							_sntprintf(buffer, 64, UINT64_FMT _T(" K"), ui64 / 1000);
						}
						else
						{
							_sntprintf(buffer, 64, UINT64_FMT, ui64);
						}
						break;
					case DCI_DT_FLOAT:
						d = _tcstod(m_data[i].szValue, NULL);
						if (d >= 10000000000.0)
						{
							_sntprintf(buffer, 64, _T("%.2f G"), d / 1000000000.0);
						}
						else if (d >= 10000000)
						{
							_sntprintf(buffer, 64, _T("%.2f M"), d / 1000000);
						}
						else if (d >= 10000)
						{
							_sntprintf(buffer, 64, _T("%.2f K"), d / 1000);
						}
						else if (d >= 0)
						{
							_sntprintf(buffer, 64, _T("%f"), d);
						}
						else if (d <= -10000000000.0)
						{
							_sntprintf(buffer, 64, _T("%.2f G"), d / 1000000000.0);
						}
						else if (d <= -10000000)
						{
							_sntprintf(buffer, 64, _T("%.2f M"), d / 1000000);
						}
						else if (d <= 10000)
						{
							_sntprintf(buffer, 64, _T("%.2f K"), d / 1000);
						}
						else
						{
							_sntprintf(buffer, 64, _T("%f"), d);
						}
						break;
					default:
						nx_strncpy(buffer, m_data[i].szValue, 64);
						break;
				}
				m_wndListCtrl->SetItem(item, 2, buffer);
			}
			else
			{
				m_wndListCtrl->SetItem(item, 2, m_data[i].szValue);
			}

			m_wndListCtrl->SetItem(item, 3, NXMCFormatTimeStamp(m_data[i].dwTimestamp, buffer, TS_LONG_DATE_TIME));
		}
	}
	m_wndListCtrl->SortItems(CompareListItemsCB, (long)this);
}


//
// Handler for column click
//

void nxLastValuesCtrl::OnListColumnClick(wxListEvent &event)
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

	m_wndListCtrl->SortItems(CompareListItemsCB, (long)this);

	// Mark sorting column
	m_wndListCtrl->SetColumnImage(m_sortMode, ((m_sortDir == 1) ? 3 : 4));
}


//
// Context menu handler
//

void nxLastValuesCtrl::OnContextMenu(wxContextMenuEvent &event)
{
	wxMenu *menu;

	menu = wxXmlResource::Get()->LoadMenu(_T("menuCtxLastValues"));
	if (menu != NULL)
	{
		PopupMenu(menu);
		delete menu;
	}
}


//
// Handler for graph open menu
//

void nxLastValuesCtrl::OnDCIGraph(wxCommandEvent &event)
{
	int i, count;
	long item, index;
	DCIInfo *list[MAX_GRAPH_ITEMS];
	NXC_DCI dci;

	count = m_wndListCtrl->GetSelectedItemCount();
	if ((count < 1) || (count > MAX_GRAPH_ITEMS))
		return;

	for(i = 0, item = m_wndListCtrl->GetFirstSelected(); i < count; i++)
	{
		index = m_wndListCtrl->GetItemData(item);
		dci.dwId = m_data[index].dwId;
		dci.iDataType = m_data[index].nDataType;
		dci.iSource = m_data[index].nSource;
		_tcscpy(dci.szName, m_data[index].szName);
		_tcscpy(dci.szDescription, m_data[index].szDescription);
		list[i] = new DCIInfo(m_nodeId, &dci);
		item = m_wndListCtrl->GetNextSelected(item);
	}

	NXMCCreateView(new nxGraphView(count, list), VIEWAREA_MAIN);
}

void nxLastValuesCtrl::OnUpdateDCIGraph(wxUpdateUIEvent &event)
{
	event.Enable((m_wndListCtrl->GetSelectedItemCount() > 0) && (m_wndListCtrl->GetSelectedItemCount() <= MAX_GRAPH_ITEMS));
}


//
// Handler for history menu
//

void nxLastValuesCtrl::OnDCIHistory(wxCommandEvent &event)
{
}

void nxLastValuesCtrl::OnUpdateDCIHistory(wxUpdateUIEvent &event)
{
	event.Enable(m_wndListCtrl->GetSelectedItemCount() == 1);
}


//
// Handler for export menu
//

void nxLastValuesCtrl::OnDCIExport(wxCommandEvent &event)
{
}

void nxLastValuesCtrl::OnUpdateDCIExport(wxUpdateUIEvent &event)
{
	event.Enable(m_wndListCtrl->GetSelectedItemCount() == 1);
}


//
// Handler for "Use multipliers" menu
//

void nxLastValuesCtrl::OnDCIUseMultipliers(wxCommandEvent &event)
{
	m_useMultipliers = !m_useMultipliers;
}

void nxLastValuesCtrl::OnUpdateDCIUseMultipliers(wxUpdateUIEvent &event)
{
	event.Check(m_useMultipliers);
}
