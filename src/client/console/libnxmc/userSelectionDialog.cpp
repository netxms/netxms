/* $Id$ */
/* 
** NetXMS - Network Management System
** Portable management console
** Copyright (C) 2008 Alex Kirhenshtein
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
**/

#include "libnxmc.h"
#include "userSelectionDialog.h"

//
// Event table
//

BEGIN_EVENT_TABLE(nxUserSelectionDialog, wxDialog)
	EVT_INIT_DIALOG(nxUserSelectionDialog::OnInitDialog)
	EVT_LIST_COL_CLICK(XRCID("wndListCtrl"), nxUserSelectionDialog::OnListColumnClick)
	EVT_LIST_ITEM_ACTIVATED(XRCID("wndListCtrl"), nxUserSelectionDialog::OnListItemActivate)
END_EVENT_TABLE()


//
// Constructor
//

nxUserSelectionDialog::nxUserSelectionDialog(wxWindow *parent)
			: wxDialog()
{
	wxConfig::Get()->Read(_T("/UserSelDlg/SortMode"), &m_sortMode, 0);
	wxConfig::Get()->Read(_T("/UserSelDlg/SortDir"), &m_sortDir, 1);

	wxXmlResource::Get()->LoadDialog(this, parent, _T("nxUserSelDlg"));
	GetSizer()->Fit(this);
}


//
// Destructor
//

nxUserSelectionDialog::~nxUserSelectionDialog()
{
	wxConfig::Get()->Write(_T("/UserSelDlg/SortMode"), m_sortMode);
	wxConfig::Get()->Write(_T("/UserSelDlg/SortDir"), m_sortDir);
}


//
// Data exchange
//

bool nxUserSelectionDialog::TransferDataFromWindow(void)
{
	return true;
}

bool nxUserSelectionDialog::TransferDataToWindow(void)
{
	wxListCtrl *wndListCtrl = XRCCTRL(*this, "wndListCtrl", wxListCtrl);

	int width = wndListCtrl->GetClientSize().x;

	wndListCtrl->InsertColumn(0, _T("Name"), wxLIST_FORMAT_LEFT, 150);
	wndListCtrl->InsertColumn(1, _T("Full Name"), wxLIST_FORMAT_LEFT, width - 150 - wxSystemSettings::GetMetric(wxSYS_VSCROLL_X));

	return true;
}


//
// Dialog initialization handler
//

void nxUserSelectionDialog::OnInitDialog(wxInitDialogEvent &event)
{
	wxWindow *wnd;

	wnd = FindWindowById(wxID_OK, this);
	if (wnd != NULL)
	{
		((wxButton *)wnd)->SetDefault();
	}
	else
	{
		wxLogDebug(_T("nxUserSelectionDialog::OnInitDialog(): cannot find child with id wxID_OK"));
	}

	event.Skip();
}


//
// Handler for column click
//

void nxUserSelectionDialog::OnListColumnClick(wxListEvent &event)
{

}


//
// Handler for item activate (Enter or Double click)
//

void nxUserSelectionDialog::OnListItemActivate(wxListEvent &event)
{

}