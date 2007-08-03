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
** File: srvcfg.h
**
**/

#ifndef _srvcfg_h_
#define _srvcfg_h_

#include <wx/grid.h>


class nxServerConfigEditor : public nxView
{
private:
	wxGrid *m_grid;
	
public:
	nxServerConfigEditor(wxWindow *parent);
	virtual ~nxServerConfigEditor();

	// Event handlers
protected:
	void OnSize(wxSizeEvent &event);

	DECLARE_EVENT_TABLE()
};

#endif
