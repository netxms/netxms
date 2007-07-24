/* $Id: mainfrm.h,v 1.5 2007-07-24 22:34:21 victor Exp $ */
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
** File: mainfrm.h
**
**/

#ifndef _mainfrm_h_
#define _mainfrm_h_


//
// Customized wxAuiManager class
//

class nxAuiManager : public wxAuiManager
{
public:
	wxAuiPaneInfo *PaneHeaderFromPoint(int x, int y)
	{
		wxAuiDockUIPart *part = HitTest(x, y);
		if (part == NULL)
			return NULL;
		return (part->type == wxAuiDockUIPart::typeCaption) ? part->pane : NULL;
	}
};


//
// Customized wxAuiNotebook class
//

class nxAuiNotebook : public wxAuiNotebook
{
public:
	nxAuiNotebook(wxWindow *parent, const wxPoint& pos, const wxSize& size) : wxAuiNotebook(parent, wxID_ANY, pos, size, wxAUI_NB_DEFAULT_STYLE | wxAUI_NB_TAB_EXTERNAL_MOVE | wxNO_BORDER) { }
	nxView *TabFromPoint(const wxPoint &point)
	{
		wxWindow *wnd = NULL;
		wxPoint pt = ScreenToClient(point);
		wxAuiTabCtrl *tabs = GetTabCtrlFromPoint(pt);
		if (tabs != NULL)
		{
			pt = tabs->ScreenToClient(point);
			tabs->TabHitTest(pt.x, pt.y, &wnd);
		}
		return (nxView *)wnd;
	}
};


//
// Main frame class
//

class nxMainFrame : public wxFrame
{
protected:
	nxAuiManager m_mgr;
	nxAuiNotebook *m_notebook;
	wxAuiPaneInfo *m_currPane;	// Current pane for context menu operation
	wxWindow *m_currTab;			// Current tab for context menu operation

	wxAuiNotebook *CreateNotebook(void);

public:
	nxMainFrame(const wxPoint &pos, const wxSize &size);
	virtual ~nxMainFrame();

	void UpdateMenuFromPlugins();

	// Event handlers
protected:
	void OnClose(wxCloseEvent &event);
	void OnFileExit(wxCommandEvent &event);
	void OnViewConsoleLog(wxCommandEvent &event);
	void OnViewRefresh(wxCommandEvent &event);
	void OnHelpAbout(wxCommandEvent &event);
	void OnPaneClose(wxCommandEvent &event);
	void OnPaneDetach(wxCommandEvent &event);
	void OnPaneFloat(wxCommandEvent &event);
	void OnPaneMoveToNotebook(wxCommandEvent &event);
	void OnTabClose(wxCommandEvent &event);
	void OnTabDetach(wxCommandEvent &event);
	void OnTabFloat(wxCommandEvent &event);
	void OnTabDock(wxCommandEvent &event);
	void OnPluginCommand(wxCommandEvent &event);
	void OnContextMenu(wxContextMenuEvent &event);
	void OnAlarmChange(wxCommandEvent &event);

	DECLARE_EVENT_TABLE()
};


#endif
