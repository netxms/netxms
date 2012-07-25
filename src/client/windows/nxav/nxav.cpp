// nxav.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "nxav.h"

#include "MainFrm.h"
#include "SettingsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CAlarmViewApp

BEGIN_MESSAGE_MAP(CAlarmViewApp, CWinApp)
	//{{AFX_MSG_MAP(CAlarmViewApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_CMD_SETTINGS, OnCmdSettings)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAlarmViewApp construction

CAlarmViewApp::CAlarmViewApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CAlarmViewApp object

CAlarmViewApp appAlarmViewer;

/////////////////////////////////////////////////////////////////////////////
// CAlarmViewApp initialization

BOOL CAlarmViewApp::InitInstance()
{
   DWORD dwResult, dwMonitor;
   BOOL bAutoLogin;

   // Initialize Windows sockets
	if (!AfxSocketInit())
	{
		AfxMessageBox(IDS_SOCKETS_INIT_FAILED, MB_OK | MB_ICONSTOP);
		return FALSE;
	}

   AfxEnableControlContainer();

   // Initialize client library
   if (!NXCInitialize())
   {
		AfxMessageBox(IDS_NXC_INIT_FAILED, MB_OK | MB_ICONSTOP);
		return FALSE;
   }

   if (!InitWorkDir())
      return FALSE;

	// Change the registry key under which our settings are stored.
	SetRegistryKey(_T("NetXMS"));

   // Load settings
   LoadAlarmSoundCfg(&m_soundCfg, NXAV_ALARM_SOUND_KEY);
   bAutoLogin = GetProfileInt(_T("Connection"), _T("AutoLogin"), FALSE);
   nx_strncpy(g_szServer, (LPCTSTR)GetProfileString(_T("Connection"), _T("Server"), _T("localhost")), MAX_PATH);
   nx_strncpy(g_szLogin, (LPCTSTR)GetProfileString(_T("Connection"), _T("Login"), NULL), MAX_USER_NAME);
   nx_strncpy(g_szPassword, (LPCTSTR)GetProfileString(_T("Connection"), _T("Password"), NULL), MAX_SECRET_LENGTH);
   g_dwOptions = GetProfileInt(_T("Connection"), _T("Options"), 0);
   g_bRepeatSound = GetProfileInt(_T("Settings"), _T("RepeatSound"), FALSE);

	// To create the main window, this code creates a new frame window
	// object and then sets it as the application's main window object.
	CMainFrame* pFrame = new CMainFrame;
	m_pMainWnd = pFrame;

	// create and load the frame with its resources
   pFrame->Create(NULL, _T("NetXMS Alarm Viewer"));

   do
   {
      // Ask for credentials
      if (!bAutoLogin)
      {
         CLoginDialog dlg;

         dlg.m_dwFlags = LOGIN_DLG_NO_OBJECT_CACHE;
         dlg.m_strServer = g_szServer;
         dlg.m_strLogin = g_szLogin;
         dlg.m_strPassword = g_szPassword;
         dlg.m_bEncrypt = (g_dwOptions & OPT_ENCRYPT_CONNECTION) ? TRUE : FALSE;
         dlg.m_bNoCache = FALSE;
         dlg.m_bClearCache = FALSE;
         dlg.m_bMatchVersion = (g_dwOptions & OPT_MATCH_SERVER_VERSION) ? TRUE : FALSE;
         if (dlg.DoModal() != IDOK)
            return FALSE;
         nx_strncpy(g_szServer, (LPCTSTR)dlg.m_strServer, MAX_PATH);
         nx_strncpy(g_szLogin, (LPCTSTR)dlg.m_strLogin, MAX_USER_NAME);
         nx_strncpy(g_szPassword, (LPCTSTR)dlg.m_strPassword, MAX_SECRET_LENGTH);
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

         _sntprintf_s(szBuffer, 1024, _TRUNCATE, _T("Unable to connect: %s"), NXCGetErrorText(dwResult));
         pFrame->MessageBox(szBuffer, _T("Error"), MB_ICONSTOP);
         bAutoLogin = FALSE;
      }
   } while(dwResult != RCC_SUCCESS);

   // Start background text-to-speach processor
   SpeakerInit();

   // Create files from resources
   FileFromResource(IDF_BACKGROUND, _T("background.jpg"));
   FileFromResource(IDF_NORMAL, _T("normal.ico"));
   FileFromResource(IDF_WARNING, _T("warning.ico"));
   FileFromResource(IDF_MINOR, _T("minor.ico"));
   FileFromResource(IDF_MAJOR, _T("major.ico"));
   FileFromResource(IDF_CRITICAL, _T("critical.ico"));
   FileFromResource(IDF_ACK, _T("ack.png"));
   FileFromResource(IDF_SOUND, _T("sound.png"));
   FileFromResource(IDF_NOSOUND, _T("nosound.png"));
   FileFromResource(IDF_ACK_ICO, _T("acknowledged.png"));
   FileFromResource(IDF_OUTSTANDING, _T("outstanding.png"));
   FileFromResource(IDF_RESOLVED, _T("resolved.png"));

   // Place main window to correct monitor
   CreateMonitorList();
   if (g_dwNumMonitors > 0)
   {
      dwMonitor = GetProfileInt(_T("Settings"), _T("Monitor"), 0);
      if (dwMonitor >= g_dwNumMonitors)
         dwMonitor = 0;
      pFrame->MoveWindow(&g_pMonitorList[dwMonitor].rcWork, FALSE);
   }
   else
   {
      RECT rect;

      // Cannot detect monitors, assume only one
      GetClientRect(GetDesktopWindow(), &rect);
      pFrame->MoveWindow(&rect, FALSE);
   }

	// The one and only window has been initialized, so show and update it.
	pFrame->ShowWindow(SW_SHOW);
	pFrame->UpdateWindow();
   pFrame->PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CAlarmViewApp message handlers





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
void CAlarmViewApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CAlarmViewApp message handlers


//
// Display NetXMS Client Library error message
//

void CAlarmViewApp::ErrorBox(DWORD dwError, TCHAR *pszMessage, TCHAR *pszTitle)
{
   TCHAR szBuffer[512];

   _sntprintf_s(szBuffer, 512, _TRUNCATE, (pszMessage != NULL) ? pszMessage : _T("Error: %s"), 
                NXCGetErrorText(dwError));
   m_pMainWnd->MessageBox(szBuffer, (pszTitle != NULL) ? pszTitle : _T("Error"), MB_ICONSTOP);
}


//
// Initialize working directory
//

BOOL CAlarmViewApp::InitWorkDir()
{
   TCHAR szBuffer[256];

   // Get path to user's "Application Data" folder
   if (SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, g_szWorkDir) != S_OK)
   {
		AfxMessageBox(IDS_GETFOLDERPATH_FAILED, MB_OK | MB_ICONSTOP);
      return FALSE;
   }

   // Create working directory
   _sntprintf_s(szBuffer, 256, _TRUNCATE, _T("\\nxav-%08X"), GetCurrentProcessId());
   _tcscat_s(g_szWorkDir, MAX_PATH, szBuffer);
   if (!CreateDirectory(g_szWorkDir, NULL))
      if (GetLastError() != ERROR_ALREADY_EXISTS)
      {
         AfxMessageBox(IDS_WORKDIR_CREATION_FAILED, MB_OK | MB_ICONSTOP);
         return FALSE;
      }

   return TRUE;
}


//
// Delete working directory on exit
//

void CAlarmViewApp::DeleteWorkDir()
{
   HANDLE hFile;
   WIN32_FIND_DATA data;
   TCHAR szName[1024];

   nx_strncpy(szName, g_szWorkDir, 1024);
   _tcscat_s(szName, 1024, _T("\\*"));

   hFile = FindFirstFile(szName, &data);
   if (hFile != INVALID_HANDLE_VALUE)
   {
      do
      {
         if ((!_tcscmp(data.cFileName, _T("."))) || 
             (!_tcscmp(data.cFileName, _T(".."))))
            continue;

         nx_strncpy(szName, g_szWorkDir, 1024);
         _tcscat_s(szName, 1024, _T("\\"));
         _tcscat_s(szName, 1024, data.cFileName);
         DeleteFile(szName);
      } while(FindNextFile(hFile, &data));

      FindClose(hFile);
   }

   RemoveDirectory(g_szWorkDir);
}


//
// Handle application exit
//

int CAlarmViewApp::ExitInstance() 
{
   DeleteWorkDir();
   SpeakerShutdown();
	return CWinApp::ExitInstance();
}


//
// Handle events in client library
//

void CAlarmViewApp::EventHandler(DWORD dwEvent, DWORD dwCode, void *pArg)
{
   if (dwEvent == NXC_EVENT_NOTIFICATION)
   {
      switch(dwCode)
      {
         case NX_NOTIFY_SHUTDOWN:
            m_pMainWnd->MessageBox(_T("Server was shutdown"), _T("Warning"),
                                   MB_OK | MB_ICONSTOP);
            m_pMainWnd->DestroyWindow();
            break;
         case NX_NOTIFY_NEW_ALARM:
         case NX_NOTIFY_ALARM_DELETED:
         case NX_NOTIFY_ALARM_CHANGED:
         case NX_NOTIFY_ALARM_TERMINATED:
            m_pMainWnd->PostMessage(WM_ALARM_UPDATE, dwCode, 
                                    (LPARAM)nx_memdup(pArg, sizeof(NXC_ALARM)));
            break;
         default:
            break;
      }
   }
}


//
// Handler for "Settings..." button
//

void CAlarmViewApp::OnCmdSettings() 
{
   CSettingsDlg dlg;

   dlg.m_bAutoLogin = GetProfileInt(_T("Connection"), _T("AutoLogin"), FALSE);
   dlg.m_bRepeatSound = g_bRepeatSound;
   dlg.m_strServer = (LPCTSTR)GetProfileString(_T("Connection"), _T("Server"), _T("localhost"));
   dlg.m_strUser = (LPCTSTR)GetProfileString(_T("Connection"), _T("Login"), NULL);
   dlg.m_strPassword = (LPCTSTR)GetProfileString(_T("Connection"), _T("Password"), NULL);
   memcpy(&dlg.m_soundCfg, &m_soundCfg, sizeof(ALARM_SOUND_CFG));
   if (dlg.DoModal() == IDOK)
   {
      memcpy(&m_soundCfg, &dlg.m_soundCfg, sizeof(ALARM_SOUND_CFG));
      SaveAlarmSoundCfg(&m_soundCfg, NXAV_ALARM_SOUND_KEY);
      WriteProfileInt(_T("Connection"), _T("AutoLogin"), dlg.m_bAutoLogin);
      if (dlg.m_bAutoLogin)
      {
         WriteProfileString(_T("Connection"), _T("Server"), (LPCTSTR)dlg.m_strServer);
         WriteProfileString(_T("Connection"), _T("Login"), (LPCTSTR)dlg.m_strUser);
      }
      WriteProfileString(_T("Connection"), _T("Password"), 
                         dlg.m_bAutoLogin ? (LPCTSTR)dlg.m_strPassword : _T(""));
      g_bRepeatSound = dlg.m_bRepeatSound;
      WriteProfileInt(_T("Settings"), _T("RepeatSound"), g_bRepeatSound);
   }
}
