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

DEFINE_LOCAL_EVENT_TYPE(nxEVT_REQUEST_COMPLETED)
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
}


//
// Application init
//

bool nxApp::OnInit()
{
	SetAppName(_T("nxmc"));
	SetVendorName(_T("NetXMS"));
#ifndef _WIN32
	((wxStandardPaths &)wxStandardPaths::Get()).SetInstallPrefix(PREFIX);
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

	m_mainFrame = new nxMainFrame(wxDefaultPosition, wxSize(700, 500));
	SetTopWindow(m_mainFrame);
	m_mainFrame->UpdateMenuFromPlugins();

	bool flag;
	wxConfig::Get()->Read(_T("/MainFrame/IsMaximized"), &flag, true);
	if (flag)
		m_mainFrame->Maximize();
	m_mainFrame->Show(true);

	return true;
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
	return wxApp::OnExit();
}


//
// Show client library error
//

void nxApp::ShowClientError(DWORD rcc, TCHAR *msgTemplate)
{
	TCHAR msg[4096];

	_sntprintf(msg, 4096, msgTemplate, NXCGetErrorText(rcc));
	wxLogMessage(msg);
	wxMessageBox(msg, _T("NetXMS Console - Error"), wxOK | wxICON_ERROR, m_mainFrame);
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
			//Close(true);
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

	return (rcc == RCC_SUCCESS);
}
