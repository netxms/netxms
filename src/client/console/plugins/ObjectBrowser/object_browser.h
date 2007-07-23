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
** File: object_browser.h
**
**/

#ifndef _object_browser_h_
#define _object_browser_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nxclapi.h>
#include <nxmc_api.h>


//
// libnxcl object index structure
//

struct NXC_OBJECT_INDEX
{
   DWORD key;
   NXC_OBJECT *object;
};


//
// Header for object overview
//

class nxObjOverviewHeader : public wxWindow
{
private:
	NXC_OBJECT *m_object;

public:
	nxObjOverviewHeader(wxWindow *parent);

	void SetObject(NXC_OBJECT *object);

protected:
	void OnPaint(wxPaintEvent &event);

	DECLARE_EVENT_TABLE()
};


//
// Object overview
//

class nxObjectOverview : public wxWindow
{
private:
	nxObjOverviewHeader *m_header;
	wxListCtrl *m_attrList;
	wxTextCtrl *m_comments;
	NXC_OBJECT *m_object;

	void InsertItem(const TCHAR *name, const TCHAR *value);
	void InsertItem(const TCHAR *name, DWORD value);
	void AdjustAttrList();

public:
	nxObjectOverview(wxWindow *parent, NXC_OBJECT *object);

	void SetObject(NXC_OBJECT *object);
	
	// Event handlers
protected:
	void OnSize(wxSizeEvent &event);
	void OnTextURL(wxTextUrlEvent &event);

	DECLARE_EVENT_TABLE()
};


//
// Object view class
//

class nxObjectView : public wxWindow
{
private:
	wxAuiNotebook *m_notebook;
	int m_headerOffset;
	NXC_OBJECT *m_object;

public:
	nxObjectView(wxWindow *parent);

	void SetObject(NXC_OBJECT *object);

	// Event handlers
protected:
	void OnSize(wxSizeEvent &event);
	void OnPaint(wxPaintEvent &event);

	DECLARE_EVENT_TABLE()
};


//
// Object tree item data
//

class nxObjectTreeItemData : public wxTreeItemData
{
private:
	NXC_OBJECT *m_object;
	
public:
	nxObjectTreeItemData(NXC_OBJECT *object) : wxTreeItemData() { m_object = object; }
	
	NXC_OBJECT *GetObject() { return m_object; }
};


//
// Object browser class
//

class nxObjectBrowser : public nxView
{
private:
	wxSplitterWindow *m_wndSplitter;
	wxTreeCtrl *m_wndTreeCtrl;
	nxObjectView *m_wndObjectView;
	bool m_isFirstResize;
	
	void AddObjectToTree(NXC_OBJECT *object, wxTreeItemId &root);
	void CreateTreeItemText(NXC_OBJECT *object, TCHAR *buffer);

public:
	nxObjectBrowser();
	virtual ~nxObjectBrowser();

	// Event handlers
protected:
	void OnSize(wxSizeEvent &event);
	void OnViewRefresh(wxCommandEvent &event);
	void OnTreeItemExpanding(wxTreeEvent &event);
	void OnTreeSelChanged(wxTreeEvent &event);

	DECLARE_EVENT_TABLE()
};

#endif

