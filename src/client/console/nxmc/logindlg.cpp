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
** File: logindlg.cpp
**
**/

#include "nxmc.h"
#include "logindlg.h"


//
// Event table
//

BEGIN_EVENT_TABLE(nxLoginDialog, wxDialog)
	EVT_INIT_DIALOG(nxLoginDialog::OnInitDialog)
END_EVENT_TABLE()


//
// Constructor
//

nxLoginDialog::nxLoginDialog(wxWindow *parent)
              : wxDialog()
{
	wxXmlResource::Get()->LoadDialog(this, parent, _T("nxLoginDialog"));
}


//
// Data exchange
//

bool nxLoginDialog::TransferDataFromWindow(void)
{
	DX_GET_COMBOBOX("comboServer", m_server);
	DX_GET_TEXTCTRL("textUserName", m_login);
	DX_GET_TEXTCTRL("textPassword", m_password);
	DX_GET_CHECKBOX("cbEncrypt", m_isEncrypt);
	DX_GET_CHECKBOX("cbClearCache", m_isClearCache);
	DX_GET_CHECKBOX("cbNoCache", m_isCacheDisabled);
	DX_GET_CHECKBOX("cbMatchVersion", m_isMatchVersion);
	return true;
}

bool nxLoginDialog::TransferDataToWindow(void)
{
	DX_SET_COMBOBOX("comboServer", m_server);
	DX_SET_TEXTCTRL("textUserName", m_login);
	DX_SET_TEXTCTRL("textPassword", m_password);
	DX_SET_CHECKBOX("cbEncrypt", m_isEncrypt);
	DX_SET_CHECKBOX("cbClearCache", m_isClearCache);
	DX_SET_CHECKBOX("cbNoCache", m_isCacheDisabled);
	DX_SET_CHECKBOX("cbMatchVersion", m_isMatchVersion);

	if (m_isCacheDisabled)
	{
		wxCheckBox *cb;

		cb = XRCCTRL(*this, "cbClearCache", wxCheckBox);
		cb->SetValue(true);
		cb->Disable();
	}
	return true;
}


//
// Dialog initialization handler
//

void nxLoginDialog::OnInitDialog(wxInitDialogEvent &event)
{
	wxWindow *wnd;

	wnd = FindWindowById(wxID_OK, this);
	if (wnd != NULL)
		((wxButton *)wnd)->SetDefault();
	event.Skip();
}
