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
** File: srvcfg.cpp
**
**/

#include "nxmc.h"
#include "srvcfg.h"


//
// Event table
//

BEGIN_EVENT_TABLE(nxServerConfigEditor, nxView)
	EVT_SIZE(nxServerConfigEditor::OnSize)
END_EVENT_TABLE()


//
// Constructor
//

nxServerConfigEditor::nxServerConfigEditor(wxWindow *parent)
                     : nxView(parent)
{
	SetName(_T("srvcfgeditor"));
	SetLabel(_T("Server Settings"));
	RegisterUniqueView(_T("srvcfgeditor"), this);

	m_grid = new wxGrid(this, wxID_ANY);
	m_grid->CreateGrid(1, 3, wxGrid::wxGridSelectRows);
}


//
// Destructor
//

nxServerConfigEditor::~nxServerConfigEditor()
{
	UnregisterUniqueView(_T("srvcfgeditor"));
}


//
// Resize handler
//

void nxServerConfigEditor::OnSize(wxSizeEvent &event)
{
	wxSize size = GetClientSize();
	m_grid->SetSize(0, 0, size.x, size.y);
}
