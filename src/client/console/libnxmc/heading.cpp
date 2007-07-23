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
** File: heading.cpp
**
**/

#include "libnxmc.h"


//
// Event table
//

BEGIN_EVENT_TABLE(nxHeading, wxWindow)
	EVT_PAINT(nxHeading::OnPaint)
END_EVENT_TABLE()


//
// Constructor
//

nxHeading::nxHeading(wxWindow *parent, const wxString &text, const wxPoint &pos, const wxSize &size)
          : wxWindow(parent, wxID_ANY, pos, size)
{
	m_text = text;
	SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
}


//
// Paint event handler
//

void nxHeading::OnPaint(wxPaintEvent &event)
{
	wxPaintDC dc(this);
	wxSize size = GetClientSize();

	dc.DrawLabel(m_text, wxRect(0, 0, size.x, size.y - 3), wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
	dc.GradientFillLinear(wxRect(0, size.y - 3, size.x, size.y), wxColor(0, 0, 255), wxColor(128, 128, 255));
}
