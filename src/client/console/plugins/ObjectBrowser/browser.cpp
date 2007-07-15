/* 
** NetXMS - Network Management System
** Portable management console - Object Browser plugin
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
** File: browser.cpp
**
**/

#include "object_browser.h"


//
// Event table
//

BEGIN_EVENT_TABLE(nxObjectBrowser, nxView)
	EVT_SIZE(nxObjectBrowser::OnSize)
END_EVENT_TABLE()


//
// Constructor
//

nxObjectBrowser::nxObjectBrowser()
                : nxView(NXMCGetDefaultParent())
{
	SetName(_T("objectbrowser"));
	SetLabel(_T("Object Browser"));
	m_wndSplitter = new wxSplitterWindow(this, wxID_ANY);
	m_wndSplitter->SplitVertically(new wxTreeCtrl(m_wndSplitter, wxID_TREE_CTRL),
	                               new nxObjectView(m_wndSplitter));
}


//
// Resize handler
//

void nxObjectBrowser::OnSize(wxSizeEvent &event)
{
	wxSize size = GetClientSize();
	m_wndSplitter->SetSize(0, 0, size.x, size.y);
}
