/* $Id: mainfrm.cpp,v 1.2 2007-07-11 21:31:53 victor Exp $ */
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
** File: mainfrm.cpp
**
**/

#include "nxmc.h"
#include "conlog.h"
#include "logindlg.h"


//
// Externals
//

DWORD DoLogin(nxLoginDialog &dlgLogin);


//
// Event table
//

BEGIN_EVENT_TABLE(nxMainFrame, wxFrame)
	EVT_CLOSE(nxMainFrame::OnClose)
	EVT_CONTEXT_MENU(nxMainFrame::OnContextMenu)
	EVT_NX_CONNECT(nxMainFrame::OnConnect)
	EVT_MENU(XRCID("menuFileExit"), nxMainFrame::OnFileExit)
	EVT_MENU(XRCID("menuViewConsoleLog"), nxMainFrame::OnViewConsoleLog)
	EVT_MENU(XRCID("menuHelpAbout"), nxMainFrame::OnHelpAbout)
	EVT_MENU(wxID_PANE_CLOSE, nxMainFrame::OnPaneClose)
	EVT_MENU(wxID_PANE_DETACH, nxMainFrame::OnPaneDetach)
	EVT_MENU(wxID_PANE_FLOAT, nxMainFrame::OnPaneFloat)
	EVT_MENU(wxID_PANE_MOVE_TO_NOTEBOOK, nxMainFrame::OnPaneMoveToNotebook)
END_EVENT_TABLE()


//
// Constructor
//

nxMainFrame::nxMainFrame(const wxPoint &pos, const wxSize &size)
            :wxFrame((wxWindow *)NULL, wxID_ANY, _T("NetXMS Management Console"), pos, size)
{
	m_mgr.SetManagedWindow(this);

	m_mgr.AddPane(CreateNotebook(), wxAuiPaneInfo().Name(_T("notebook")).CenterPane().PaneBorder(false));
	InitViewTracker(&m_mgr, m_notebook);

	m_mgr.AddPane(new nxConsoleLogger(this), wxAuiPaneInfo().Name(_T("conlog")).Caption(_T("Console Log")).Bottom().BestSize(700, 150));

	SetMenuBar(wxXmlResource::Get()->LoadMenuBar(_T("menubarMain")));
	CreateStatusBar();

	m_mgr.Update();
}


//
// Destructor
//

nxMainFrame::~nxMainFrame()
{
	m_mgr.UnInit();
}


//
// Create notebook
//

wxAuiNotebook *nxMainFrame::CreateNotebook()
{
   // create the notebook off-window to avoid flicker
   wxSize clientSize = GetClientSize();
   
   m_notebook = new wxAuiNotebook(this, wxID_ANY,
                                  wxPoint(clientSize.x, clientSize.y),
                                  wxSize(430,200),
                                  wxAUI_NB_DEFAULT_STYLE | wxAUI_NB_TAB_EXTERNAL_MOVE | wxNO_BORDER);
   
   wxBitmap bmp = wxArtProvider::GetBitmap(wxART_NORMAL_FILE, wxART_OTHER, wxSize(16,16));
   m_notebook->AddPage(new wxTextCtrl(m_notebook, wxID_ANY, wxT("Some text"),
                wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxNO_BORDER) , wxT("wxTextCtrl 1"), false, bmp);

	return m_notebook;
}


//
// Close event handler
//

void nxMainFrame::OnClose(wxCloseEvent &event)
{
	wxConfig::Get()->Write(_T("/MainFrame/IsMaximized"), IsMaximized());
	event.Skip();
}


//
// File->Exit menu handler
//

void nxMainFrame::OnFileExit(wxCommandEvent &event)
{
	Close(true);
}


//
// Help->About menu handler
//

void nxMainFrame::OnHelpAbout(wxCommandEvent &event)
{
	wxDialog *dlg;
	wxStaticText *versionTextCtrl;

	dlg = wxXmlResource::Get()->LoadDialog(this, _T("nxDlgAbout"));
	dlg->SetAffirmativeId(XRCID("buttonOK"));
	versionTextCtrl = XRCCTRL(*dlg, "staticTextVersion", wxStaticText);
	versionTextCtrl->SetLabel(_T("Version ") NETXMS_VERSION_STRING);
	dlg->ShowModal();
	delete dlg;
}


//
// File->Connect menu handler
//

void nxMainFrame::OnConnect(wxCommandEvent &event)
{
	nxLoginDialog dlg(this);
	DWORD rcc;
	int result;
	wxConfigBase *cfg;

	cfg = wxConfig::Get();
	dlg.m_server = cfg->Read(_T("/Connect/Server"), _T("localhost"));
	dlg.m_login = cfg->Read(_T("/Connect/Login"), _T(""));
	cfg->Read(_T("/Connect/Encrypt"), &dlg.m_isEncrypt, false);
	cfg->Read(_T("/Connect/ClearCache"), &dlg.m_isClearCache, true);
	cfg->Read(_T("/Connect/DisableCaching"), &dlg.m_isCacheDisabled, true);
	cfg->Read(_T("/Connect/MatchVersion"), &dlg.m_isMatchVersion, false);
	do
	{
		result = dlg.ShowModal();
		if (result == wxID_CANCEL)
		{
			Close(true);
			break;
		}

		rcc = DoLogin(dlg);
		if (rcc == RCC_SUCCESS)
		{
			cfg->Write(_T("/Connect/Server"), dlg.m_server);
			cfg->Write(_T("/Connect/Login"), dlg.m_login);
			cfg->Write(_T("/Connect/Encrypt"), dlg.m_isEncrypt);
			cfg->Write(_T("/Connect/ClearCache"), dlg.m_isClearCache);
			cfg->Write(_T("/Connect/DisableCaching"), dlg.m_isCacheDisabled);
			cfg->Write(_T("/Connect/MatchVersion"), dlg.m_isMatchVersion);
			if (!dlg.m_isCacheDisabled)
			{
				g_appFlags |= AF_SAVE_OBJECT_CACHE;
			}
			nx_strncpy(g_userName, dlg.m_login.c_str(), MAX_DB_STRING);
			wxLogMessage(_T("Successfully connected to server %s as %s"), dlg.m_server.c_str(), dlg.m_login.c_str());
		}
		else
		{
			wxGetApp().ShowClientError(rcc, _T("Cannot establish session with management server: %s"));
		}
	} while(rcc != RCC_SUCCESS);
}


//
// View->Console Log menu handler
//

void nxMainFrame::OnViewConsoleLog(wxCommandEvent &event)
{
	nxView *view;

	view = FindUniqueView(_T("conlog"));
	if (view != NULL)
	{
		ActivateView(view);
	}
	else
	{
		m_mgr.AddPane(new nxConsoleLogger(this), wxAuiPaneInfo().Name(_T("conlog")).Caption(_T("Console Log")).Bottom().BestSize(700, 150));
	}
	m_mgr.Update();
}


//
// Context menu handler
//

void nxMainFrame::OnContextMenu(wxContextMenuEvent &event)
{
	wxPoint pt;

	pt = ScreenToClient(event.GetPosition());
	if ((m_currPane = m_mgr.PaneHeaderFromPoint(pt.x, pt.y)) != NULL)
	{
		wxMenu menu;

		menu.Append(wxID_PANE_DETACH, _T("&Detach"));
		menu.Append(wxID_PANE_FLOAT, _T("&Float"));
		menu.Append(wxID_PANE_MOVE_TO_NOTEBOOK, _T("&Move to main area"));
		menu.Append(wxID_PANE_CLOSE, _T("&Close"));
		PopupMenu(&menu);
	}
	else
	{
		event.Skip();
	}
}


//
// Close pane on request from context menu
//

void nxMainFrame::OnPaneClose(wxCommandEvent &event)
{
	if (m_currPane != NULL)
	{
		m_mgr.ClosePane(*m_currPane);
		m_mgr.Update();
	}
}


//
// Detach pane on request from context menu
//

void nxMainFrame::OnPaneDetach(wxCommandEvent &event)
{
	wxWindow *wnd;
	nxFrame *frame;

	if (m_currPane != NULL)
	{
		wnd = m_currPane->window;
		wxString caption = _T("NetXMS Console - ") + m_currPane->caption;
		m_mgr.DetachPane(wnd);
		frame = new nxFrame(caption, wnd);
		m_mgr.Update();
		frame->Show(true);
	}
}


//
// Float pane on request from context menu
//

void nxMainFrame::OnPaneFloat(wxCommandEvent &event)
{
	if (m_currPane != NULL)
	{
		m_currPane->Float();
		m_mgr.Update();
	}
}


//
// Move pane to notebook on request from context menu
//

void nxMainFrame::OnPaneMoveToNotebook(wxCommandEvent &event)
{
	wxWindow *wnd;

	if (m_currPane != NULL)
	{
		wnd = m_currPane->window;
		wxString caption = m_currPane->caption;
		m_mgr.DetachPane(wnd);
		m_notebook->AddPage(wnd, caption, true);
		m_mgr.Update();
	}
}
