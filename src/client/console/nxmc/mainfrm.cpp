/* $Id: mainfrm.cpp,v 1.22 2008-03-09 19:56:37 victor Exp $ */
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
#include "ctrlpanel.h"


//
// Event table
//

BEGIN_EVENT_TABLE(nxMainFrame, wxFrame)
	EVT_CLOSE(nxMainFrame::OnClose)
	EVT_CONTEXT_MENU(nxMainFrame::OnContextMenu)
	EVT_ICONIZE(nxMainFrame::OnMinimize)
	EVT_MENU(XRCID("menuFileExit"), nxMainFrame::OnFileExit)
	EVT_MENU(XRCID("menuViewConsoleLog"), nxMainFrame::OnViewConsoleLog)
	EVT_MENU(XRCID("menuViewControlPanel"), nxMainFrame::OnViewControlPanel)
	EVT_MENU(XRCID("menuViewRefresh"), nxMainFrame::OnViewRefresh)
	EVT_MENU(XRCID("menuViewFullScreen"), nxMainFrame::OnViewFullScreen)
	EVT_UPDATE_UI(XRCID("menuViewFullScreen"), nxMainFrame::OnUpdateViewFullScreen)
	EVT_MENU(XRCID("menuPerspectiveSave"), nxMainFrame::OnPerspectiveSave)
	EVT_MENU(XRCID("menuPerspectiveDefault"), nxMainFrame::OnPerspectiveDefault)
	EVT_MENU(XRCID("menuHelpAbout"), nxMainFrame::OnHelpAbout)
	EVT_MENU(wxID_PANE_CLOSE, nxMainFrame::OnPaneClose)
	EVT_MENU(wxID_PANE_DETACH, nxMainFrame::OnPaneDetach)
	EVT_MENU(wxID_PANE_FLOAT, nxMainFrame::OnPaneFloat)
	EVT_MENU(wxID_PANE_MOVE_TO_NOTEBOOK, nxMainFrame::OnPaneMoveToNotebook)
	EVT_MENU(wxID_TAB_CLOSE, nxMainFrame::OnTabClose)
	EVT_MENU(wxID_TAB_DETACH, nxMainFrame::OnTabDetach)
	EVT_MENU(wxID_TAB_FLOAT, nxMainFrame::OnTabFloat)
	EVT_MENU(wxID_TAB_DOCK, nxMainFrame::OnTabDock)
	EVT_MENU_RANGE(wxID_PLUGIN_RANGE_START, wxID_PLUGIN_RANGE_END, nxMainFrame::OnPluginCommand)
	EVT_MENU_RANGE(wxID_PERSPECTIVE_START, wxID_PERSPECTIVE_END, nxMainFrame::OnLoadPerspective)
	EVT_NXC_ALARM_CHANGE(nxMainFrame::OnAlarmChange)
END_EVENT_TABLE()


//
// Constructor
//

nxMainFrame::nxMainFrame(const wxPoint &pos, const wxSize &size)
            :wxFrame((wxWindow *)NULL, wxID_ANY, _T("NetXMS Management Console"), pos, size)
{
#ifdef _WIN32
	wxIcon icon(_T("NXMC_ICON"), wxBITMAP_TYPE_ICO_RESOURCE);
	SetIcon(icon);
#endif

	m_currPane = NULL;
	m_currTab = NULL;

	m_mgr.SetManagedWindow(this);

	m_mgr.AddPane(CreateNotebook(), wxAuiPaneInfo().Name(_T("notebook")).CenterPane().PaneBorder(false));
	InitViewTracker(&m_mgr, m_notebook);
	NXMCInitAUI(&m_mgr, m_notebook, this);

	if (g_appFlags & AF_HIDE_TABS)
		m_notebook->SetTabCtrlHeight(0);

	if (!(g_appFlags & AF_EMPTY_WORKAREA))
		m_mgr.AddPane(new nxConsoleLogger(this), wxAuiPaneInfo().Name(_T("conlog")).Caption(_T("Console Log")).Bottom().BestSize(700, 150));

	SetMenuBar(wxXmlResource::Get()->LoadMenuBar(_T("menubarMain")));
	if (!(g_appFlags & AF_HIDE_STATUS_BAR))
		CreateStatusBar();

	m_mgr.Update();

	UpdatePerspectivesMenu();
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
   
   m_notebook = new nxAuiNotebook(this,
                                  wxPoint(clientSize.x, clientSize.y),
                                  wxSize(430,200));
	//m_notebook->SetArtProvider(new nxAuiConsoleTabArt);
	return m_notebook;
}


//
// Update main menu from plugin registrations
//

void nxMainFrame::UpdateMenuFromPlugins()
{
	nxmcArrayOfRegItems &regList = NXMCGetRegistrations();
	wxMenu *menu = GetMenuBar()->GetMenu(1);
	size_t i;

	for(i = 0; i < regList.GetCount(); i++)
	{
		if (regList[i].GetType() == REGITEM_VIEW_MENU)
		{
			menu->Append(regList[i].GetId() + ((nxmcPlugin *)regList[i].GetPlugin())->GetBaseId(), regList[i].GetName());
		}
	}
}


//
// Close event handler
//

void nxMainFrame::OnClose(wxCloseEvent &event)
{
	wxConfig::Get()->Write(_T("/MainFrame/IsMaximized"), IsMaximized());
	wxGetApp().DestroyTaskBarIcon();
	event.Skip();
}


//
// Minimize (iconize) event handler
//

void nxMainFrame::OnMinimize(wxIconizeEvent &event)
{
	if ((g_appFlags & AF_HIDDEN) && event.Iconized())
		Show(false);
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
	dlg->GetSizer()->Fit(dlg);
	versionTextCtrl = XRCCTRL(*dlg, "staticTextVersion", wxStaticText);
	versionTextCtrl->SetLabel(_T("Version ") NETXMS_VERSION_STRING);
	dlg->ShowModal();
	delete dlg;
}


//
// View->Full Screen menu handler
//

void nxMainFrame::OnViewFullScreen(wxCommandEvent &event)
{
	if (IsFullScreen())
	{
		ShowFullScreen(false);
	}
	else
	{
		ShowFullScreen(true, wxFULLSCREEN_NOTOOLBAR | wxFULLSCREEN_NOSTATUSBAR | wxFULLSCREEN_NOBORDER | wxFULLSCREEN_NOCAPTION);
	}
}

void nxMainFrame::OnUpdateViewFullScreen(wxUpdateUIEvent &event)
{
	event.Check(IsFullScreen());
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
		m_mgr.Update();
	}
}


//
// View->Control Panel menu handler
//

void nxMainFrame::OnViewControlPanel(wxCommandEvent &event)
{
	nxView *view;

	view = FindUniqueView(_T("ctrlpanel"));
	if (view != NULL)
	{
		ActivateView(view);
	}
	else
	{
		m_mgr.AddPane(new nxControlPanel(this), wxAuiPaneInfo().Name(_T("ctrlpanel")).Caption(_T("Control Panel")).Bottom().BestSize(700, 150));
		m_mgr.Update();
	}
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
	else if ((m_currTab = m_notebook->TabFromPoint(event.GetPosition())) != NULL)
	{
		wxMenu menu;

		menu.Append(wxID_TAB_DETACH, _T("&Detach"));
		menu.Append(wxID_TAB_FLOAT, _T("&Float"));
		menu.Append(wxID_TAB_DOCK, _T("D&ock"));
		menu.Append(wxID_TAB_CLOSE, _T("&Close"));
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
		frame = new nxFrame(caption, (nxView *)wnd, VIEWAREA_DOCKED);
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


//
// Close pane on request from context menu
//

void nxMainFrame::OnTabClose(wxCommandEvent &event)
{
	if (m_currTab != NULL)
	{
		m_notebook->DeletePage(m_notebook->GetPageIndex(m_currTab));
	}
}


//
// Detach pane on request from context menu
//

void nxMainFrame::OnTabDetach(wxCommandEvent &event)
{
	nxFrame *frame;

	if (m_currTab != NULL)
	{
		wxString caption = _T("NetXMS Console - ") + m_currTab->GetLabel();
		m_notebook->RemovePage(m_notebook->GetPageIndex(m_currTab));
		frame = new nxFrame(caption, (nxView *)m_currTab, VIEWAREA_MAIN);
		frame->Show(true);
	}
}


//
// Float tab on request from context menu
//

void nxMainFrame::OnTabFloat(wxCommandEvent &event)
{
	if (m_currTab != NULL)
	{
		m_notebook->RemovePage(m_notebook->GetPageIndex(m_currTab));
		m_currTab->Reparent(this);
		m_mgr.AddPane(m_currTab, wxAuiPaneInfo().Name(m_currTab->GetName()).Caption(m_currTab->GetLabel()).Float());
		m_mgr.Update();
	}
}


//
// Move tab from notebook to dock area on request from context menu
//

void nxMainFrame::OnTabDock(wxCommandEvent &event)
{
	if (m_currTab != NULL)
	{
		m_notebook->RemovePage(m_notebook->GetPageIndex(m_currTab));
		m_currTab->Reparent(this);
		m_mgr.AddPane(m_currTab, wxAuiPaneInfo().Name(m_currTab->GetName()).Caption(m_currTab->GetLabel()).Left().BestSize(200, 300));
		m_mgr.Update();
	}
}


//
// Handler for menu items added by plugins
//

void nxMainFrame::OnPluginCommand(wxCommandEvent &event)
{
	CallPluginCommandHandler(event.GetId());
}


//
// Handler for View->Refresh menu
//

void nxMainFrame::OnViewRefresh(wxCommandEvent &event)
{
	wxWindow *page;
	
	page = m_notebook->GetPage(m_notebook->GetSelection());
	if (page != NULL)
	{
		wxCommandEvent event(nxEVT_REFRESH_VIEW);
		page->AddPendingEvent(event);
	}
}


//
// Handler for alarm change events
//

void nxMainFrame::OnAlarmChange(wxCommandEvent &event)
{
	NXC_ALARM *alarm;
	
	alarm = (NXC_ALARM *)event.GetClientData();
	if (g_appFlags & AF_TASKBAR_ICON)	// Show balloon popup
	{
		if (event.GetInt() == NX_NOTIFY_NEW_ALARM)
		{
			NXC_OBJECT *object;

			object = NXCFindObjectById(g_hSession, alarm->dwSourceObject);
			if (object != NULL)
			{
				wxString msg = _T("Severity: ");

				msg += NXMCGetStatusText(alarm->nCurrentSeverity);
				msg += _T("\r\n");
				msg += alarm->szMessage;
				wxGetApp().GetTaskBarIcon().ShowBalloon(object->szName, msg.c_str(), 15000, alarm->nCurrentSeverity);
			}
		}
	}	
	// nxMainFrame is a final destination for client library events,
	// so it should destroy dynamic data associated with event
	safe_free(alarm);
}


//
// Attach view
//

void nxMainFrame::AttachView(nxView *view, int area)
{
	switch(area)
	{
		case VIEWAREA_MAIN:
			view->Reparent(m_notebook);
			m_notebook->AddPage(view, view->GetLabel(), true, view->GetIcon());
			break;
		case VIEWAREA_DOCKED:
			view->Reparent(this);
			m_mgr.AddPane(view, wxAuiPaneInfo().Name(view->GetName()).Caption(view->GetLabel()));
			m_mgr.Update();
			break;
		case VIEWAREA_DOCKED_LEFT:
			view->Reparent(this);
			m_mgr.AddPane(view, wxAuiPaneInfo().Name(view->GetName()).Caption(view->GetLabel()).Left());
			m_mgr.Update();
			break;
		case VIEWAREA_DOCKED_RIGHT:
			view->Reparent(this);
			m_mgr.AddPane(view, wxAuiPaneInfo().Name(view->GetName()).Caption(view->GetLabel()).Right());
			m_mgr.Update();
			break;
		case VIEWAREA_DOCKED_TOP:
			view->Reparent(this);
			m_mgr.AddPane(view, wxAuiPaneInfo().Name(view->GetName()).Caption(view->GetLabel()).Top());
			m_mgr.Update();
			break;
		case VIEWAREA_DOCKED_BOTTOM:
			view->Reparent(this);
			m_mgr.AddPane(view, wxAuiPaneInfo().Name(view->GetName()).Caption(view->GetLabel()).Bottom());
			m_mgr.Update();
			break;
		default:
			break;
	}
}


//
// Handler for Perspective -> Save menu
//

void nxMainFrame::OnPerspectiveSave(wxCommandEvent &event)
{
	wxString name, data, keyName, path;
	BYTE hash[MD5_DIGEST_SIZE];
	TCHAR hashText[MD5_DIGEST_SIZE * 2 + 1];
	wxConfigBase *cfg;
	wxTextEntryDialog dlg(this, _T("Please enter name for perspective:"), _T("Save Perspective"));

	if (dlg.ShowModal() == wxID_OK)
	{
		name = dlg.GetValue();
		data = m_mgr.SavePerspective();
		CalculateMD5Hash((const BYTE *)name.c_str(), _tcslen(name.c_str()) * sizeof(TCHAR), hash);
		BinToStr(hash, MD5_DIGEST_SIZE, hashText);
		keyName = _T("/Perspectives/");
		keyName += hashText;

		cfg = wxConfig::Get();
		path = cfg->GetPath();
		cfg->SetPath(keyName);
		cfg->Write(_T("Name"), name);
		cfg->Write(_T("MgrData"), data);
		cfg->SetPath(path);

		UpdatePerspectivesMenu();
	}
}


//
// Handler for Perspective -> Default
//

void nxMainFrame::OnPerspectiveDefault(wxCommandEvent &event)
{
//	m_mgr.LoadPerspective(s);
}


//
// Update perspectives menu
//

void nxMainFrame::UpdatePerspectivesMenu()
{
	wxMenu *menu;
	size_t i, count;
	int id;
	long index;
	wxString path, entry, name;
	wxConfigBase *cfg;

	menu = GetMenuBar()->GetMenu(2);

	// Clear existing items
	count = menu->GetMenuItemCount();
	for(i = 0; i < count; i++)
	{
		id = menu->FindItemByPosition(i)->GetId();
		if ((id >= wxID_PERSPECTIVE_START) && (id <= wxID_PERSPECTIVE_END))
		{
			safe_free(m_perspectives[id]);
			menu->Delete(id);
			count--;
			i--;
		}
	}
	m_perspectives.clear();

	// Add new items
	cfg = wxConfig::Get();
	path = cfg->GetPath();
	cfg->SetPath(_T("/Perspectives"));

	if (cfg->GetFirstGroup(entry, index))
	{
		id = wxID_PERSPECTIVE_START;
		do
		{
			cfg->SetPath(entry);

			name = cfg->Read(_T("Name"), entry);
			m_perspectives[id] = _tcsdup(entry.c_str());
			menu->Append(id, name);
			id++;
			
			cfg->SetPath(_T(".."));
		}
		while(cfg->GetNextGroup(entry, index));
	}

	cfg->SetPath(path);
}


//
// Handler for load perspective menus
//

void nxMainFrame::OnLoadPerspective(wxCommandEvent &event)
{
	nxIntToStringHash::iterator it;
	wxString path;
	wxConfigBase *cfg;

	it = m_perspectives.find(event.GetId());
	if (it != m_perspectives.end())
	{
		cfg = wxConfig::Get();
		path = cfg->GetPath();
		cfg->SetPath(_T("/Perspectives"));
		cfg->SetPath(it->second);

		m_mgr.LoadPerspective(cfg->Read(_T("MgrData"), _T("")));

		cfg->SetPath(path);
	}
}
