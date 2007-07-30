/* $Id: logindlg.h,v 1.2 2007-07-30 08:03:00 victor Exp $ */
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
** File: logindlg.h
**
**/

#ifndef _logindlg_h_
#define _logindlg_h_

class nxLoginDialog : public wxDialog
{
protected:
	virtual bool TransferDataFromWindow(void);
	virtual bool TransferDataToWindow(void);

public:
	nxLoginDialog(wxWindow *parent);

	wxString m_login;
	wxString m_password;
	wxString m_server;
	bool m_isEncrypt;
	bool m_isCacheDisabled;
	bool m_isClearCache;
	bool m_isMatchVersion;

// Event handlers
protected:
	void OnInitDialog(wxInitDialogEvent &event);

	DECLARE_EVENT_TABLE()
};

#endif
