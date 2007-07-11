/* $Id: frame.cpp,v 1.2 2007-07-11 21:31:53 victor Exp $ */
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
** File: frame.cpp
**
**/

#include "nxmc.h"


//
// Event table
//

BEGIN_EVENT_TABLE(nxFrame, wxFrame)
	EVT_SIZE(nxFrame::OnSize)
END_EVENT_TABLE()


//
// Constructor
//

nxFrame::nxFrame(const wxString& title, wxWindow *child)
        : wxFrame(NULL, wxID_ANY, title)
{
	m_child = child;
	m_child->Reparent(this);
}


//
// Resize handler
//

void nxFrame::OnSize(wxSizeEvent &event)
{
	wxSize size = GetClientSize();
	m_child->SetSize(0, 0, size.x, size.y);
}
