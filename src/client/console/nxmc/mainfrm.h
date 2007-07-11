/* $Id: mainfrm.h,v 1.1 2007-07-11 19:46:58 victor Exp $ */
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
// Main frame class
//

class nxMainFrame : public wxFrame
{
protected:
	nxAuiManager m_mgr;
	wxAuiNotebook *m_notebook;
	wxAuiPaneInfo *m_currPane;	// Current pane for context menu operation

	wxAuiNotebook *CreateNotebook(void);

public:
	nxMainFrame(const wxPoint &pos, const wxSize &size);
	virtual ~nxMainFrame();

	// Event handlers
protected:
	void OnClose(wxCloseEvent &event);
	void OnConnect(wxCommandEvent &event);
	void OnFileExit(wxCommandEvent &event);
	void OnViewConsoleLog(wxCommandEvent &event);
	void OnHelpAbout(wxCommandEvent &event);
	void OnPaneClose(wxCommandEvent &event);
	void OnPaneDetach(wxCommandEvent &event);
	void OnPaneFloat(wxCommandEvent &event);
	void OnPaneMoveToNotebook(wxCommandEvent &event);
	void OnContextMenu(wxContextMenuEvent &event);

	DECLARE_EVENT_TABLE()
};


#endif
