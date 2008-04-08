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
#include <nxqueue.h>


//
// Arrays and hashes
//

WX_DECLARE_OBJARRAY(wxTreeItemId, nxTreeItemList);
WX_DECLARE_HASH_MAP(DWORD, nxTreeItemList*, wxIntegerHash, wxIntegerEqual, nxObjectItemsHash);


//
// Custom events
//

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_LOCAL_EVENT_TYPE(nxEVT_DCI_DATA_RECEIVED, 0)
END_DECLARE_EVENT_TYPES()

#define EVT_NX_DCI_DATA_RECEIVED(fn)	EVT_LIBNXMC_EVENT(nxEVT_DCI_DATA_RECEIVED, fn)


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
// DCI last values control
//

class nxLastValuesCtrl : public wxWindow
{
private:
	wxListView *m_wndListCtrl;
	DWORD m_nodeId;
	DWORD m_dciCount;
	NXC_DCI_VALUE *m_data;
	int m_sortMode;
	int m_sortDir;
	bool m_useMultipliers;
	wxString m_context;

public:
	nxLastValuesCtrl(wxWindow *parent, const TCHAR *context);
	virtual ~nxLastValuesCtrl();

	int CompareListItems(long item1, long item2);

	void SetData(DWORD nodeId, DWORD dciCount, NXC_DCI_VALUE *valueList);
	void Clear() { m_wndListCtrl->DeleteAllItems(); }

	void SetUseMultipliers(bool flag) { m_useMultipliers = flag; }
	bool IsMultipliersUsed() { return m_useMultipliers; }

	// Event handlers
protected:
	void OnSize(wxSizeEvent &event);
	void OnListColumnClick(wxListEvent &event);
	void OnContextMenu(wxContextMenuEvent &event);
	void OnDCIGraph(wxCommandEvent &event);
	void OnUpdateDCIGraph(wxUpdateUIEvent &event);
	void OnDCIExport(wxCommandEvent &event);
	void OnUpdateDCIExport(wxUpdateUIEvent &event);
	void OnDCIHistory(wxCommandEvent &event);
	void OnUpdateDCIHistory(wxUpdateUIEvent &event);
	void OnDCIUseMultipliers(wxCommandEvent &event);
	void OnUpdateDCIUseMultipliers(wxUpdateUIEvent &event);

	DECLARE_EVENT_TABLE()
};


//
// Last values view
//

class nxNodeLastValues : public wxWindow
{
private:
	nxLastValuesCtrl *m_dciList;
	NXC_OBJECT *m_object;
	Queue m_workerQueue;
	THREAD m_workerThread;
	MUTEX m_mutexObject;
	wxTimer *m_timer;

public:
	nxNodeLastValues(wxWindow *parent, NXC_OBJECT *object);
	virtual ~nxNodeLastValues();

	void SetObject(NXC_OBJECT *object);
	
	DWORD GetNextRequest() { return (DWORD)m_workerQueue.GetOrBlock(); }
	void RequestCompleted(bool success, DWORD nodeId, DWORD numItems, NXC_DCI_VALUE *valueList);

	// Event handlers
protected:
	void OnSize(wxSizeEvent &event);
	void OnRequestCompleted(wxCommandEvent &event);
	void OnTimer(wxTimerEvent &event);

	DECLARE_EVENT_TABLE()
};


//
// Last values view
//

class nxLastValuesView : public nxView
{
private:
	NXC_OBJECT *m_node;
	nxLastValuesCtrl *m_dciList;
	DWORD m_dciCount;
	NXC_DCI_VALUE *m_dciValues;
	int m_rqId;

protected:
	virtual void RequestCompletionHandler(int rqId, DWORD rcc, const TCHAR *errMsg);

public:
	nxLastValuesView(NXC_OBJECT *node, wxWindow *parent = NULL);
	virtual ~nxLastValuesView();

	// Event handlers
protected:
	void OnSize(wxSizeEvent &event);
	void OnViewRefresh(wxCommandEvent &event);

	DECLARE_EVENT_TABLE()
};


//
// Time units for graphs
//

#define TIME_UNIT_MINUTE   0
#define TIME_UNIT_HOUR     1
#define TIME_UNIT_DAY      2

#define MAX_TIME_UNITS     3


//
// Graph view
//

class nxGraphView : public nxView
{
private:
	nxGraph *m_graph;
	int m_dciCount;
	DCIInfo *m_dciInfo[MAX_GRAPH_ITEMS];
	int m_rqId;
	int m_timeFrameType;
	int m_timeUnit;
	int m_numTimeUnits;
	time_t m_timeFrom;
	time_t m_timeTo;
	time_t m_timeFrame;
	wxTimer *m_processingTimer;

	void Preset(int timeUnit, int numUnits);

protected:
	virtual void RequestCompletionHandler(int rqId, DWORD rcc, const TCHAR *errMsg);

public:
	nxGraphView(int dciCount, DCIInfo **dciList, wxWindow *parent = NULL);
	virtual ~nxGraphView();

	// Event handlers
protected:
	void OnSize(wxSizeEvent &event);
	void OnViewRefresh(wxCommandEvent &event);
	void OnDataReceived(wxCommandEvent &event);
	void OnProcessingTimer(wxTimerEvent &event);
	void OnContextMenu(wxContextMenuEvent &event);

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
	void ObjectUpdated();

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
	nxObjectItemsHash m_objectItemsHash;
	NXC_OBJECT *m_currentObject;
	
	void AddObjectToTree(NXC_OBJECT *object, wxTreeItemId &root);
	void ClearObjectItemsHash();

public:
	nxObjectBrowser(wxWindow *parent = NULL);
	virtual ~nxObjectBrowser();

	// Event handlers
protected:
	void OnSize(wxSizeEvent &event);
	void OnViewRefresh(wxCommandEvent &event);
	void OnTreeItemExpanding(wxTreeEvent &event);
	void OnTreeSelChanged(wxTreeEvent &event);
	void OnTreeDeleteItem(wxTreeEvent &event);
	void OnTreeItemMenu(wxTreeEvent &event);
	void OnObjectChange(wxCommandEvent &event);
	void OnObjectBind(wxCommandEvent &event);
	void OnUpdateUIObjectBind(wxUpdateUIEvent &event);
	void OnObjectUnbind(wxCommandEvent &event);
	void OnUpdateUIObjectUnbind(wxUpdateUIEvent &event);
	void OnObjectLastValues(wxCommandEvent &event);
	void OnUpdateUIObjectLastValues(wxUpdateUIEvent &event);

	DECLARE_EVENT_TABLE()
};

#endif
