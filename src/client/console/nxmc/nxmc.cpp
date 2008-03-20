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
** File: nxmc.cpp
**
**/

#include "nxmc.h"
#include "mainfrm.h"
#include "conlog.h"
#include "logindlg.h"


//
// Externals
//

DWORD DoLogin(nxLoginDialog &dlgLogin);


//
// Global variables
//

NXC_SESSION g_hSession = NULL;
DWORD g_appFlags = 0;
TCHAR g_userName[MAX_DB_STRING];
SNMP_MIBObject *g_mibRoot = NULL;


//
// Custom events
//

DEFINE_LOCAL_EVENT_TYPE(nxEVT_SET_STATUS_TEXT)


//
// Implement application object
//

IMPLEMENT_APP(nxApp)


//
// Constructor
//

nxApp::nxApp() : wxApp()
{
	m_mainFrame = NULL;
	m_tbIcon = NULL;
	m_soundPolicy = NULL;
}


//
// Application init
//

bool nxApp::OnInit()
{
	SetAppName(_T("nxmc"));
	SetVendorName(_T("NetXMS"));
#ifndef _WIN32
#ifdef UNICODE
	((wxStandardPaths &)wxStandardPaths::Get()).SetInstallPrefix(PREFIXW);
#else
	((wxStandardPaths &)wxStandardPaths::Get()).SetInstallPrefix(PREFIX);
#endif
#endif

   InitThreadLibrary();

	if (!wxApp::OnInit())
		return false;

#ifdef _WIN32
   WSADATA wsaData;
   WSAStartup(0x0002, &wsaData);
#endif

	if (!NXCInitialize())
		return false;

	nxConsoleLogger::Init();

	wxFileSystem::AddHandler(new wxArchiveFSHandler);
#ifdef _WIN32
	wxFileSystem::AddHandler(new wxMemoryFSHandler);
#endif
	wxImage::AddHandler(new wxPNGHandler);
	wxImage::AddHandler(new wxXPMHandler);

	wxXmlResource::Get()->InitAllHandlers();
	if (!NXMCLoadResources(_T("nxmc.xrs"), NXMC_LIB_INSTANCE_ARG(wxGetInstance()), wxMAKEINTRESOURCE(IDR_XRS)))
		return false;

	// Create application directories if needed
	wxFileName::Mkdir(wxStandardPaths::Get().GetUserDataDir(), 0700, wxPATH_MKDIR_FULL);

	// Create global config object
	wxConfig::Set(new wxConfig(GetAppName(), GetVendorName(), wxEmptyString, wxEmptyString, wxCONFIG_USE_LOCAL_FILE | wxCONFIG_USE_SUBDIR));

	LoadPlugins();
	NXMCInitializationComplete();

	if (!Connect())
		return false;
	NXMCSetSession(g_hSession);
	NXMCInitImageLists();

	LoadSoundPolicy();

	m_mainFrame = new nxMainFrame(wxDefaultPosition, wxSize(700, 500));
	SetTopWindow(m_mainFrame);
	NXMCSetMainEventHandler(m_mainFrame);
	m_mainFrame->UpdateMenuFromPlugins();

	// Hide main menu if requested
	if (g_appFlags & AF_HIDE_MAIN_MENU)
		m_mainFrame->SetMenuBar(NULL);

	bool flag;
	wxConfig::Get()->Read(_T("/MainFrame/IsMaximized"), &flag, true);
	if (flag || (g_appFlags & AF_START_MAXIMIZED))
		m_mainFrame->Maximize();

	if (g_appFlags & AF_OPEN_VIEW_ON_START)
	{
		nxView *view = NXMCCreateViewByClass(m_autoView, NXMCGetDefaultParent(), _T(""), NULL, NULL);
		if (view != NULL)
		{
			NXMCCreateView(view, VIEWAREA_MAIN);
		}
		else
		{
			wxLogWarning(_T("Unable to create view of class \"%s\" - probably class not registered"), m_autoView.c_str());
		}
	}

	if (g_appFlags & AF_TASKBAR_ICON)
	{
		m_tbIcon = new nxTaskBarIcon;
		m_tbIcon->SetIcon(wxXmlResource::Get()->LoadIcon(_T("icoTaskBar")), _T("NetXMS Alarm Notifier"));
	}

	if (!(g_appFlags & AF_HIDDEN))
	{
		m_mainFrame->Show(true);
		if (g_appFlags & AF_FULLSCREEN)
			m_mainFrame->ShowFullScreen(true, wxFULLSCREEN_NOTOOLBAR | wxFULLSCREEN_NOSTATUSBAR | wxFULLSCREEN_NOBORDER | wxFULLSCREEN_NOCAPTION);
	}

	return true;
}


//
// Initialize command line parsing
//

void nxApp::OnInitCmdLine(wxCmdLineParser& parser)
{
	wxApp::OnInitCmdLine(parser);
	parser.AddSwitch(_T("A"), _T("autoconnect"), _T("Automatically connect to server"));
	parser.AddSwitch(wxEmptyString, _T("empty"), _T("Start with empty workarea (don't open any predefined views)"));
	parser.AddSwitch(_T("F"), _T("fullscreen"), _T("Start in fullscreen mode"));
	parser.AddSwitch(_T("H"), _T("hidden"), _T("Start with main window hidden; also hide main window when minimize"));
	parser.AddSwitch(_T("M"), _T("maximized"), _T("Start with maximized main window"));
	parser.AddSwitch(wxEmptyString, _T("nomenu"), _T("Hide main application menu"));
	parser.AddSwitch(wxEmptyString, _T("nostatusbar"), _T("Hide status bar"));
	parser.AddSwitch(wxEmptyString, _T("notabs"), _T("Hide tabs in main working area"));
	parser.AddOption(_T("o"), _T("open"), _T("Open view of given class at startup"));
	parser.AddOption(wxEmptyString, _T("password"), _T("Password"));
	parser.AddOption(wxEmptyString, _T("server"), _T("Server to connect to"));
	parser.AddOption(_T("S"), _T("soundpolicy"), _T("Alarm sound policy file"));
	parser.AddSwitch(_T("T"), _T("taskbaricon"), _T("Add icon to task bar (system tray)"));
	parser.AddOption(wxEmptyString, _T("username"), _T("User name"));
}


//
// Process command line options
//

bool nxApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
	if (parser.Found(_T("autoconnect")))
		g_appFlags |= AF_AUTOCONNECT;
	if (parser.Found(_T("empty")))
		g_appFlags |= AF_EMPTY_WORKAREA;
	if (parser.Found(_T("fullscreen")))
		g_appFlags |= AF_FULLSCREEN;
	if (parser.Found(_T("hidden")))
		g_appFlags |= AF_HIDDEN;
	if (parser.Found(_T("maximized")))
		g_appFlags |= AF_START_MAXIMIZED;
	if (parser.Found(_T("nomenu")))
		g_appFlags |= AF_HIDE_MAIN_MENU;
	if (parser.Found(_T("nostatusbar")))
		g_appFlags |= AF_HIDE_STATUS_BAR;
	if (parser.Found(_T("notabs")))
		g_appFlags |= AF_HIDE_TABS;
	if (parser.Found(_T("taskbaricon")))
		g_appFlags |= AF_TASKBAR_ICON;

	if (parser.Found(_T("open"), &m_autoView))
		g_appFlags |= AF_OPEN_VIEW_ON_START;

	if (parser.Found(_T("server"), &m_acServer))
		g_appFlags |= AF_OVERRIDE_SERVER;
	if (parser.Found(_T("username"), &m_acUsername))
		g_appFlags |= AF_OVERRIDE_USERNAME;
	if (parser.Found(_T("password"), &m_acPassword))
		g_appFlags |= AF_OVERRIDE_PASSWORD;

	if (!parser.Found(_T("soundpolicy"), &m_soundPolicyFile))
		m_soundPolicyFile = _T("default.spl");

	return wxApp::OnCmdLineParsed(parser);
}


//
// Exit handler
//

int nxApp::OnExit()
{
	if (g_appFlags & AF_SAVE_OBJECT_CACHE)
	{
      BYTE serverId[8];
		TCHAR serverIdAsText[32];
		wxString cacheFile;

		NXCGetServerID(g_hSession, serverId);
		BinToStr(serverId, 8, serverIdAsText);
		cacheFile = wxStandardPaths::Get().GetUserDataDir();
		cacheFile += FS_PATH_SEPARATOR;
		cacheFile += _T("cache.");
		cacheFile += serverIdAsText;
		cacheFile += _T(".");
		cacheFile += g_userName;
		NXCSaveObjectCache(g_hSession, cacheFile.c_str());
	}
	NXCDisconnect(g_hSession);
	nxConsoleLogger::Shutdown();
	delete m_tbIcon;
	delete m_soundPolicy;
	return wxApp::OnExit();
}


//
// Connect to server
//

bool nxApp::Connect()
{
	nxLoginDialog dlg(NULL);
	DWORD rcc = RCC_INTERNAL_ERROR;
	int result;
	wxConfigBase *cfg;

	cfg = wxConfig::Get();
	dlg.m_server = (g_appFlags & AF_OVERRIDE_SERVER) ? m_acServer : cfg->Read(_T("/Connect/Server"), _T("localhost"));
	dlg.m_login = (g_appFlags & AF_OVERRIDE_USERNAME) ? m_acUsername : cfg->Read(_T("/Connect/Login"), _T(""));
	if (g_appFlags & AF_OVERRIDE_PASSWORD)
		dlg.m_password = m_acPassword;
	cfg->Read(_T("/Connect/Encrypt"), &dlg.m_isEncrypt, false);
	cfg->Read(_T("/Connect/ClearCache"), &dlg.m_isClearCache, true);
	cfg->Read(_T("/Connect/DisableCaching"), &dlg.m_isCacheDisabled, true);
	cfg->Read(_T("/Connect/MatchVersion"), &dlg.m_isMatchVersion, false);
	do
	{
		if (g_appFlags & AF_AUTOCONNECT)
		{
			g_appFlags &= ~AF_AUTOCONNECT;
		}
		else
		{
			result = dlg.ShowModal();
			if (result == wxID_CANCEL)
			{
				//Close(true);
				break;
			}
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
			NXMCShowClientError(rcc, _T("Cannot establish session with management server: %s"));
		}
	} while(rcc != RCC_SUCCESS);

	return (rcc == RCC_SUCCESS);
}


//
// Load sound policy
//

void nxApp::LoadSoundPolicy()
{
	wxXmlDocument doc;
	wxXmlNode *root;

	if (doc.Load(m_soundPolicyFile))
	{
		root = doc.GetRoot();
		if (root->GetName() == _T("soundpolicy"))
		{
			m_soundPolicy = new AlarmSoundPolicy(root);
			wxLogInfo(_T("Alarm sound policy successfully loaded from %s"), m_soundPolicyFile.c_str());
		}
		else
		{
			wxLogWarning(_T("Invalid format of alarm sound policy file %s"), m_soundPolicyFile.c_str());
		}
	}
	else
	{
		wxLogWarning(_T("Unable to load alarm sound policy from %s"), m_soundPolicyFile.c_str());
	}
}


//
// Play alarm sound
//

void nxApp::PlayAlarmSound(int action, NXC_ALARM *alarm)
{
	if (m_soundPolicy != NULL)
		m_soundPolicy->HandleAlarm(action, alarm);
}
