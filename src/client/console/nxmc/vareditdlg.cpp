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
** File: vareditdlg.cpp
**
**/

#include "nxmc.h"
#include "vareditdlg.h"


//
// Event table
//

BEGIN_EVENT_TABLE(nxVarEditDlg, wxDialog)
	EVT_INIT_DIALOG(nxVarEditDlg::OnInitDialog)
END_EVENT_TABLE()


//
// Constructor
//

nxVarEditDlg::nxVarEditDlg(wxWindow *parent)
             : wxDialog()
{
	wxXmlResource::Get()->LoadDialog(this, parent, _T("nxVarEditDlg"));
	GetSizer()->Fit(this);
}


//
// Data exchange
//

bool nxVarEditDlg::TransferDataFromWindow(void)
{
	DX_GET_TEXTCTRL("textName", m_name);
	DX_GET_TEXTCTRL("textValue", m_value);
	return true;
}

bool nxVarEditDlg::TransferDataToWindow(void)
{
	DX_SET_TEXTCTRL("textName", m_name);
	DX_SET_TEXTCTRL("textValue", m_value);
	return true;
}


//
// Dialog initialization handler
//

void nxVarEditDlg::OnInitDialog(wxInitDialogEvent &event)
{
/*	wxWindow *wnd;

	wnd = FindWindowById(wxID_OK, this);
	if (wnd != NULL)
		((wxButton *)wnd)->SetDefault();
	else
		wxLogDebug(_T("nxVarEditDlg::OnInitDialog(): cannot find child with id wxID_OK"));*/
	event.Skip();
}
