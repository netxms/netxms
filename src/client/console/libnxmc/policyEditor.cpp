/* $Id$ */
/* 
** NetXMS - Network Management System
** Portable management console
** Copyright (C) 2008 Victor Kirhenshtein
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
**/

#include "libnxmc.h"
#include "policyEditor.h"


//
// Cell renderer for object list
//

class nxGridCellObjListRenderer : public wxGridCellRenderer
{
public:
	virtual void Draw(wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, const wxRect& rect, int row, int col, bool isSelected);
	virtual wxSize GetBestSize(wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, int row, int col);
	virtual wxGridCellRenderer *Clone() const;
};

void nxGridCellObjListRenderer::Draw(wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, const wxRect& rect, int row, int col, bool isSelected)
{
	dc.DrawText( _T("OBJ"), rect.x, rect.y);
}

wxSize nxGridCellObjListRenderer::GetBestSize(wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, int row, int col)
{
	return wxSize(100, 40);
}

wxGridCellRenderer *nxGridCellObjListRenderer::Clone() const
{ 
	return new nxGridCellObjListRenderer; 
}


//
// Cell editor for object list
//

class nxGridCellObjListEditor : public wxGridCellEditor
{
public:
	nxGridCellObjListEditor() : wxGridCellEditor() { }
};


//
// Event table
//

BEGIN_EVENT_TABLE(nxPolicyEditor, wxGrid)
END_EVENT_TABLE()


//
// Constructor
//

nxPolicyEditor::nxPolicyEditor(wxWindow *parent, wxWindowID id)
			   : wxGrid(parent, id)
{
	RegisterDataType(_T("OBJECT_LIST"), new nxGridCellObjListRenderer, NULL);
	SetLabelBackgroundColour(wxColour(0, 0, 127));
	SetLabelTextColour(wxColour(255, 255, 255));
}


//
// Destructor
//

nxPolicyEditor::~nxPolicyEditor()
{
}


//
// Initializer
//

void nxPolicyEditor::Initialize(wxGridTableBase *table)
{
	SetTable(table, true, wxGrid::wxGridSelectRows);
}
