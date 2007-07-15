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
** File: nxview.cpp
**
**/

#include "libnxmc.h"


//
// Event table
//

BEGIN_EVENT_TABLE(nxView, wxWindow)
END_EVENT_TABLE()


//
// Constructor
//

nxView::nxView(wxWindow *parent)
       : wxWindow(parent, wxID_ANY,  wxDefaultPosition, wxDefaultSize)
{
	m_icon = wxNullBitmap;
}


//
// Destructor
//

nxView::~nxView()
{
}


//
// Set view label
//

void nxView::SetLabel(const wxString& label)
{
	m_label = label;
}


//
// Get view label
//

wxString nxView::GetLabel() const
{
	return m_label;
}

