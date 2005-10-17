// nxav.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "nxav.h"

#include "MainFrm.h"
#include "LoginDialog.h"
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

CAlarmViewApp theApp;

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

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Change the registry key under which our settings are stored.
	SetRegistryKey(_T("NetXMS"));

   // Load settings
   bAutoLogin = GetProfileInt(_T("Connection"), _T("AutoLogin"), FALSE);
   _tcscpy(g_szServer, (LPCTSTR)GetProfileString(_T("Connection"), _T("Server"), _T("localhost")));
   _tcscpy(g_szLogin, (LPCTSTR)GetProfileString(_T("Connection"), _T("Login"), NULL));
   _tcscpy(g_szPassword, (LPCTSTR)GetProfileString(_T("Connection"), _T("Password"), NULL));

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

         dlg.m_szServer = g_szServer;
         dlg.m_szLogin = g_szLogin;
         dlg.m_szPassword = g_szPassword;
         if (dlg.DoModal() != IDOK)
            return FALSE;
         nx_strncpy(g_szServer, (LPCTSTR)dlg.m_szServer, MAX_PATH);
         nx_strncpy(g_szLogin, (LPCTSTR)dlg.m_szLogin, MAX_USER_NAME);
         nx_strncpy(g_szPassword, (LPCTSTR)dlg.m_szPassword, MAX_SECRET_LENGTH);
      }

      // Save last connection parameters
      WriteProfileString(_T("Connection"), _T("Server"), g_szServer);
      WriteProfileString(_T("Connection"), _T("Login"), g_szLogin);
      //WriteProfileInt(_T("Connection"), _T("Encryption"), g_dwEncryptionMethod);

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

   // Create files from resources
   FileFromResource(IDF_BACKGROUND, _T("background.jpg"));
   FileFromResource(IDF_NORMAL, _T("normal.ico"));
   FileFromResource(IDF_WARNING, _T("warning.ico"));
   FileFromResource(IDF_MINOR, _T("minor.ico"));
   FileFromResource(IDF_MAJOR, _T("major.ico"));
   FileFromResource(IDF_CRITICAL, _T("critical.ico"));
   FileFromResource(IDF_ACK, _T("ack.png"));

   // Place main window to correct monitor
   CreateMonitorList();
   dwMonitor = GetProfileInt(_T("Settings"), _T("Monitor"), 0);
   if (dwMonitor >= g_dwNumMonitors)
      dwMonitor = 0;
   pFrame->MoveWindow(&g_pMonitorList[dwMonitor].rcWork, FALSE);

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

   _sntprintf(szBuffer, 512, (pszMessage != NULL) ? pszMessage : _T("Error: %s"), 
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
   _sntprintf(szBuffer, 256, _T("\\nxav-%08X"), GetCurrentProcessId());
   _tcscat(g_szWorkDir, szBuffer);
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

   _tcscpy(szName, g_szWorkDir);
   _tcscat(szName, _T("\\*"));

   hFile = FindFirstFile(szName, &data);
   if (hFile != INVALID_HANDLE_VALUE)
   {
      do
      {
         if ((!_tcscmp(data.cFileName, _T("."))) || 
             (!_tcscmp(data.cFileName, _T(".."))))
            continue;

         _tcscpy(szName, g_szWorkDir);
         _tcscat(szName, _T("\\"));
         _tcscat(szName, data.cFileName);
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
            m_pMainWnd->MessageBox("Server was shutdown", "Warning", MB_OK | MB_ICONSTOP);
            m_pMainWnd->DestroyWindow();
            break;
         case NX_NOTIFY_NEW_ALARM:
         case NX_NOTIFY_ALARM_DELETED:
         case NX_NOTIFY_ALARM_ACKNOWLEGED:
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
   dlg.m_strServer = (LPCTSTR)GetProfileString(_T("Connection"), _T("Server"), _T("localhost"));
   dlg.m_strUser = (LPCTSTR)GetProfileString(_T("Connection"), _T("Login"), NULL);
   dlg.m_strPassword = (LPCTSTR)GetProfileString(_T("Connection"), _T("Password"), NULL);
   if (dlg.DoModal() == IDOK)
   {
      WriteProfileInt(_T("Connection"), _T("AutoLogin"), dlg.m_bAutoLogin);
      if (dlg.m_bAutoLogin)
      {
         WriteProfileString(_T("Connection"), _T("Server"), (LPCTSTR)dlg.m_strServer);
         WriteProfileString(_T("Connection"), _T("Login"), (LPCTSTR)dlg.m_strUser);
      }
      WriteProfileString(_T("Connection"), _T("Password"), 
                         dlg.m_bAutoLogin ? (LPCTSTR)dlg.m_strPassword : _T(""));
   }
}
