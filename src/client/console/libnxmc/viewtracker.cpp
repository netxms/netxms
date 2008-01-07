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
** File: viewtracker.cpp
**
**/

#include "libnxmc.h"


//
// Static data
//

nxViewHash s_viewMap;
wxAuiManager *s_mgr = NULL;
wxAuiNotebook *s_notebook = NULL;


//
// Initialize view tracker
//

void LIBNXMC_EXPORTABLE InitViewTracker(wxAuiManager *mgr, wxAuiNotebook *nb)
{
	s_mgr = mgr;
	s_notebook = nb;
}


//
// Register unique view
//

void LIBNXMC_EXPORTABLE RegisterUniqueView(const TCHAR *name, nxView *view)
{
	s_viewMap[name] = view;
	view->SetName(name);
}


//
// Unregister unique view
//

void LIBNXMC_EXPORTABLE UnregisterUniqueView(const TCHAR *name)
{
	nxViewHash::iterator it;

	it = s_viewMap.find(name);
	if (it != s_viewMap.end())
		s_viewMap.erase(it);
}


//
// Find unique view
//

nxView LIBNXMC_EXPORTABLE *FindUniqueView(const TCHAR *name)
{
	nxViewHash::iterator it;

	it = s_viewMap.find(name);
	return (it != s_viewMap.end()) ? it->second : NULL;
}


//
// Activate view
//

void LIBNXMC_EXPORTABLE ActivateView(nxView *view)
{
	// First, try to find requested view in panes
	wxAuiPaneInfo &pane = s_mgr->GetPane(view->GetName());
	if (pane.IsOk())
	{
		pane.Show();
		s_mgr->Update();
		view->SetFocus();
	}
	else
	{
		// Find view in notebook
		size_t page;

		page = s_notebook->GetPageIndex(view);
		if (page != wxNOT_FOUND)
		{
			// Activate page
			s_notebook->SetSelection(page);
		}
		else
		{
			// View is in detached frame
			view->SetFocus();
		}
	}
}
