// nxnotify.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "nxnotify.h"

#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNxnotifyApp

BEGIN_MESSAGE_MAP(CNxnotifyApp, CWinApp)
	//{{AFX_MSG_MAP(CNxnotifyApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_FILE_SETTINGS, OnFileSettings)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNxnotifyApp construction

CNxnotifyApp::CNxnotifyApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CNxnotifyApp object

CNxnotifyApp appNotify;

/////////////////////////////////////////////////////////////////////////////
// CNxnotifyApp initialization

BOOL CNxnotifyApp::InitInstance()
{
   NOTIFYICONDATA nid;
   BOOL bAutoLogin;
   DWORD dwResult;
   HMODULE hLib;

	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	// Standard initialization
#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

   // Create image list with severity icons
   hLib = GetModuleHandle(_T("NXUILIB.DLL"));
   g_imgListSeverity.Create(16, 16, ILC_COLOR32 | ILC_MASK, 5, 1);
   g_imgListSeverity.Add(::LoadIcon(hLib, MAKEINTRESOURCE(IDI_ICON_NORMAL)));
   g_imgListSeverity.Add(::LoadIcon(hLib, MAKEINTRESOURCE(IDI_ICON_WARNING)));
   g_imgListSeverity.Add(::LoadIcon(hLib, MAKEINTRESOURCE(IDI_ICON_MINOR)));
   g_imgListSeverity.Add(::LoadIcon(hLib, MAKEINTRESOURCE(IDI_ICON_MAJOR)));
   g_imgListSeverity.Add(::LoadIcon(hLib, MAKEINTRESOURCE(IDI_ICON_CRITICAL)));

	// Change the registry key under which our settings are stored.
	SetRegistryKey(_T("NetXMS"));

   // Load settings
   LoadAlarmSoundCfg(&m_soundCfg, NXNOTIFY_ALARM_SOUND_KEY);
   bAutoLogin = GetProfileInt(_T("Connection"), _T("AutoLogin"), FALSE);
   _tcscpy(g_szServer, (LPCTSTR)GetProfileString(_T("Connection"), _T("Server"), _T("localhost")));
   _tcscpy(g_szLogin, (LPCTSTR)GetProfileString(_T("Connection"), _T("Login"), NULL));
   _tcscpy(g_szPassword, (LPCTSTR)GetProfileString(_T("Connection"), _T("Password"), NULL));
   g_dwOptions = GetProfileInt(_T("Connection"), _T("Options"), 0);

	// To create the main window, this code creates a new frame window
	// object and then sets it as the application's main window object.
	CMainFrame* pFrame = new CMainFrame;
	m_pMainWnd = pFrame;

	// create and load the frame with its resources
	pFrame->LoadFrame(IDR_MAINFRAME, WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL, NULL);

	// The one and only window has been initialized, so show and update it.
   pFrame->CloseWindow();

   // Connect to server
   do
   {
      // Ask for credentials
      if (!bAutoLogin)
      {
         CLoginDialog dlg;

         dlg.m_dwFlags = LOGIN_DLG_NO_OBJECT_CACHE;
         dlg.m_szServer = g_szServer;
         dlg.m_szLogin = g_szLogin;
         dlg.m_szPassword = g_szPassword;
         dlg.m_bEncrypt = (g_dwOptions & OPT_ENCRYPT_CONNECTION) ? TRUE : FALSE;
         dlg.m_bNoCache = FALSE;
         dlg.m_bClearCache = FALSE;
         dlg.m_bMatchVersion = (g_dwOptions & OPT_MATCH_SERVER_VERSION) ? TRUE : FALSE;
         if (dlg.DoModal() != IDOK)
            return FALSE;
         nx_strncpy(g_szServer, (LPCTSTR)dlg.m_szServer, MAX_PATH);
         nx_strncpy(g_szLogin, (LPCTSTR)dlg.m_szLogin, MAX_USER_NAME);
         nx_strncpy(g_szPassword, (LPCTSTR)dlg.m_szPassword, MAX_SECRET_LENGTH);
         if (dlg.m_bEncrypt)
            g_dwOptions |= OPT_ENCRYPT_CONNECTION;
         else
            g_dwOptions &= ~OPT_ENCRYPT_CONNECTION;
         if (dlg.m_bMatchVersion)
            g_dwOptions |= OPT_MATCH_SERVER_VERSION;
         else
            g_dwOptions &= ~OPT_MATCH_SERVER_VERSION;
      }

      // Save last connection parameters
      WriteProfileString(_T("Connection"), _T("Server"), g_szServer);
      WriteProfileString(_T("Connection"), _T("Login"), g_szLogin);
      WriteProfileInt(_T("Connection"), _T("Options"), g_dwOptions);

      // Connect
      dwResult = DoLogin();
      if (dwResult != RCC_SUCCESS)
      {
         TCHAR szBuffer[1024];

         _sntprintf(szBuffer, 1024, _T("Unable to connect: %s"), NXCGetErrorText(dwResult));
         pFrame->MessageBox(szBuffer, _T("Error"), MB_ICONSTOP);
         bAutoLogin = FALSE;
      }
   } while(dwResult != RCC_SUCCESS);

   // Start background text-to-speach processor
   SpeakerInit();

   // Add icon to task bar status area
   nid.cbSize = sizeof(NOTIFYICONDATA);
   nid.hWnd = pFrame->m_hWnd;
   nid.uID = 0;
   nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
   nid.hIcon = appNotify.LoadIcon(IDR_MAINFRAME);
   _tcscpy(nid.szTip, _T("NetXMS Alarm Notifier"));
   nid.uCallbackMessage = NXNM_TASKBAR_CALLBACK;
   Shell_NotifyIcon(NIM_ADD, &nid);

   nid.uVersion = NOTIFYICON_VERSION;
   Shell_NotifyIcon(NIM_SETVERSION, &nid);

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CNxnotifyApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}


//
// Handle events in client library
//

void CNxnotifyApp::EventHandler(DWORD dwEvent, DWORD dwCode, void *pArg)
{
   if (dwEvent == NXC_EVENT_NOTIFICATION)
   {
      switch(dwCode)
      {
         case NX_NOTIFY_SHUTDOWN:
            m_pMainWnd->MessageBox("Server was shutdown", "Warning", MB_OK | MB_ICONSTOP);
            m_pMainWnd->DestroyWindow();
            break;
         case NX_NOTIFY_NEW_ALARM:
         case NX_NOTIFY_ALARM_DELETED:
         case NX_NOTIFY_ALARM_ACKNOWLEGED:
            m_pMainWnd->PostMessage(NXNM_ALARM_UPDATE, dwCode,
                                    (LPARAM)nx_memdup(pArg, sizeof(NXC_ALARM)));
            break;
         default:
            break;
      }
   }
}


//
// WM_COMMAND::ID_FILE_SETTINGS message handler
//

void CNxnotifyApp::OnFileSettings() 
{
   if (ConfigureAlarmSounds(&m_soundCfg))
   {
      SaveAlarmSoundCfg(&m_soundCfg, NXNOTIFY_ALARM_SOUND_KEY);
   }
}
