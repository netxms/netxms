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

#include "nxmc.h"
#include "tbicon.h"


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
