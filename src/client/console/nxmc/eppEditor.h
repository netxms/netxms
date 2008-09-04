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
** File: eppEditor.h
**
**/

#ifndef _eppEditor_h_
#define _eppEditor_h_


//
// Column numbers
//

enum
{
	EPP_COL_RULE = 0,
	EPP_COL_SOURCE,
	EPP_COL_EVENT,
	EPP_COL_SEVERITY,
	EPP_COL_SCRIPT,
	EPP_COL_ALARM,
	EPP_COL_SITUATION,
	EPP_COL_ACTION,
	EPP_COL_OPTIONS,
	EPP_COL_COMMENT,
	EPP_NUMBER_OF_COLUMNS
};


//
// Event policy table class
//

class nxEvenPolicyTable : public wxGridTableBase
{
private:
	NXC_EPP *m_epp;

	void UpdateGrid();

public:
	nxEvenPolicyTable();
	virtual ~nxEvenPolicyTable();

	virtual int GetNumberRows() { return (m_epp != NULL) ? m_epp->dwNumRules : 0; }
	virtual int GetNumberCols() { return EPP_NUMBER_OF_COLUMNS; }
	virtual bool IsEmptyCell(int row, int col) { return false; }
	virtual wxString GetValue(int row, int col);
	virtual void SetValue(int row, int col, const wxString &value);
	virtual wxString GetTypeName(int row, int col);

	void SetPolicy(NXC_EPP *epp) { m_epp = epp; UpdateGrid(); }
};


//
// Policy editor class
//

class nxEPPEditor : public nxPolicyEditor
{
private:
	nxEvenPolicyTable *m_table;
	NXC_EPP *m_epp;

public:
	nxEPPEditor(wxWindow *parent);
	virtual ~nxEPPEditor();

	void SetPolicy(NXC_EPP *epp) { m_epp = epp; m_table->SetPolicy(m_epp); ForceRefresh(); }

	// Event handlers
protected:

	DECLARE_EVENT_TABLE()
};


//
// Editor view
//

class nxEventPolicyEditor : public nxView
{
private:
	nxEPPEditor *m_eppEditor;
	NXC_EPP *m_epp;

protected:
	virtual void RequestCompletionHandler(int rqId, DWORD rcc, const TCHAR *errMsg);
	
public:
	nxEventPolicyEditor(wxWindow *parent);
	virtual ~nxEventPolicyEditor();

	// Event handlers
protected:
	void OnSize(wxSizeEvent &event);
	void OnRefreshView(wxCommandEvent &event);

	DECLARE_EVENT_TABLE()
};


#endif
