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
** File: tbicon.cpp
**
**/

#define _WIN32_IE 0x0600

#include "nxmc.h"
#include "tbicon.h"

//#ifdef _WIN32
//#include <shellapi.h>
//#endif


//
// Event table
//

BEGIN_EVENT_TABLE(nxTaskBarIcon, wxTaskBarIcon)
	EVT_MENU(wxID_TBICON_SHOW_CONSOLE, nxTaskBarIcon::OnShowConsole)
	EVT_MENU(wxID_TBICON_EXIT, nxTaskBarIcon::OnExit)
END_EVENT_TABLE()


//
// Create popup menu on request
//

wxMenu *nxTaskBarIcon::CreatePopupMenu()
{
	wxMenu *menu;

	menu = new wxMenu;
	menu->Append(wxID_TBICON_SHOW_CONSOLE, _T("Show &console"));
	menu->Append(wxID_TBICON_EXIT, _T("E&xit"));
	return menu;
}


//
// Handler for "Show console" menu
//

void nxTaskBarIcon::OnShowConsole(wxCommandEvent &event)
{
	nxMainFrame *frm = wxGetApp().GetMainFrame();

	frm->Show(true);
	if (frm->IsIconized())
		frm->Iconize(false);
}


//
// Handler for "Exit" menu
//

void nxTaskBarIcon::OnExit(wxCommandEvent &event)
{
	wxCommandEvent eventClose(wxEVT_COMMAND_MENU_SELECTED, XRCID("menuFileExit"));
	wxGetApp().GetMainFrame()->AddPendingEvent(eventClose);
}


//
// Show "balloon" popup
//

bool nxTaskBarIcon::ShowBalloon(wxString title, wxString message, unsigned int timeout, int severity)
{
	if (!IsOk())
		return false;

#ifdef _WIN32
	NOTIFYICONDATA notifyData;
	
	memset(&notifyData, 0, sizeof(NOTIFYICONDATA));
	notifyData.cbSize = sizeof(NOTIFYICONDATA);
	notifyData.hWnd = (HWND)((wxWindow *)m_win)->GetHWND();
	notifyData.uID = 99;		// wxWidget's hardcoded icon ID
	notifyData.uFlags = NIF_INFO;

	wxStrncpy(notifyData.szInfo, message.c_str(), WXSIZEOF(notifyData.szInfo));
	wxStrncpy(notifyData.szInfoTitle, title.c_str(), WXSIZEOF(notifyData.szInfoTitle));
	notifyData.uTimeout = timeout;

	switch(severity)
	{
		case STATUS_WARNING:
		case STATUS_MINOR:
		case STATUS_MAJOR:
			notifyData.dwInfoFlags = NIIF_WARNING;
			break;
		case STATUS_CRITICAL:
			notifyData.dwInfoFlags = NIIF_ERROR;
			break;
		default:
			notifyData.dwInfoFlags = NIIF_INFO;
			break;
	}

	if (m_iconAdded)
		return Shell_NotifyIcon(NIM_MODIFY, &notifyData) ? true : false;
	else
#endif
	return false;
}
