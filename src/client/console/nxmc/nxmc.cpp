#include "nxmc.h"
#include "mainfrm.h"
#include "conlog.h"
#include <wx/splash.h>


//
// Global variables
//

NXC_SESSION g_hSession = NULL;
DWORD g_appFlags = 0;
TCHAR g_userName[MAX_DB_STRING];


//
// Custom events
//

DEFINE_LOCAL_EVENT_TYPE(nxEVT_CONNECT)
DEFINE_LOCAL_EVENT_TYPE(nxEVT_REQUEST_COMPLETED)
DEFINE_LOCAL_EVENT_TYPE(nxEVT_SET_STATUS_TEXT)


//
// Implement application object
//

IMPLEMENT_APP(nxApp)


//
// Application init
//

bool nxApp::OnInit()
{
	SetAppName(_T("nxmc"));
	SetVendorName(_T("NetXMS"));

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
	wxImage::AddHandler(new wxPNGHandler);
	wxImage::AddHandler(new wxICOHandler);

	wxXmlResource::Get()->InitAllHandlers();
#ifdef _WIN32
	HRSRC hRes;
	HGLOBAL hMem;
	void *data;

	wxFileSystem::AddHandler(new wxMemoryFSHandler);
	hRes = FindResource(wxGetInstance(), MAKEINTRESOURCE(IDR_XRS), _T("XRS"));
	if (hRes != NULL)
	{
		hMem = LoadResource(NULL, hRes);
		if (hMem != NULL)
		{
			data = LockResource(hMem);
			if (data != NULL)
			{
				wxMemoryFSHandler::AddFile(_T("resource.xrs"), data, SizeofResource(NULL, hRes));
				UnlockResource(hMem);
			}
			FreeResource(hMem);
		}
	}
	if (!wxXmlResource::Get()->Load(_T("memory:resource.xrs")))
	  return false;
#else
	if (!wxXmlResource::Get()->Load(_T("/root/nxmc.xrs")))
	  return false;
#endif

/*	wxBitmap bitmap = wxXmlResource::Get()->LoadBitmap(_T("bmpSplash"));
	wxSplashScreen *splash = new wxSplashScreen(bitmap,
		wxSPLASH_CENTRE_ON_SCREEN | wxSPLASH_TIMEOUT,
		6000, NULL, -1, wxDefaultPosition, wxDefaultSize,
		wxSIMPLE_BORDER|wxSTAY_ON_TOP);
	wxYield();*/

	// Create application directories if needed
	wxFileName::Mkdir(wxStandardPaths::Get().GetUserDataDir(), 0700, wxPATH_MKDIR_FULL);

	// Create global config object
	wxConfig::Set(new wxConfig(GetAppName(), GetVendorName(), wxEmptyString, wxEmptyString, wxCONFIG_USE_LOCAL_FILE));

	LoadPlugins();
	NXMCInitializationComplete();

	m_mainFrame = new nxMainFrame(wxDefaultPosition, wxSize(700, 500));
	SetTopWindow(m_mainFrame);

	bool flag;
	wxConfig::Get()->Read(_T("/MainFrame/IsMaximized"), &flag, true);
	if (flag)
		m_mainFrame->Maximize();
	m_mainFrame->Show(true);

	// Cause main window to show login dialog
	wxCommandEvent event(nxEVT_CONNECT);
	wxPostEvent(m_mainFrame, event);

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
