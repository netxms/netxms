// nxpc.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "nxpc.h"

#include "MainFrm.h"
#include "LoginDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNxpcApp

BEGIN_MESSAGE_MAP(CNxpcApp, CWinApp)
	//{{AFX_MSG_MAP(CNxpcApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_CONNECT_TO_SERVER, OnConnectToServer)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNxpcApp construction

CNxpcApp::CNxpcApp()
	: CWinApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CNxpcApp object

CNxpcApp theApp;


//
// Setup working directory
//

BOOL CNxpcApp::SetupWorkDir()
{
   // Get path to user's "Application Data" folder
   if (!SHGetSpecialFolderPath(NULL, g_szWorkDir, CSIDL_APPDATA, TRUE))
   {
		AfxMessageBox(IDS_GETFOLDERPATH_FAILED, MB_OK | MB_ICONSTOP);
      return FALSE;
   }

   // Create working directory root
   wcscat(g_szWorkDir, L"\\Pocket NetXMS");
   if (!CreateDirectory(g_szWorkDir, NULL))
      if (GetLastError() != ERROR_ALREADY_EXISTS)
      {
         AfxMessageBox(IDS_WORKDIR_CREATION_FAILED, MB_OK | MB_ICONSTOP);
         return FALSE;
      }

   return TRUE;
}


//
//
// CNxpcApp initialization

BOOL CNxpcApp::InitInstance()
{
   // Setup our working directory
   if (!SetupWorkDir())
      return FALSE;

	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

   // Initialize NetXMS client library
   if (!NXCInitialize())
   {
		AfxMessageBox(IDS_NXC_INIT_FAILED, MB_OK | MB_ICONSTOP);
		return FALSE;
   }

	// Change the registry key under which our settings are stored.
	// You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("NetXMS"));

   // Load configuration from registry
   g_dwOptions = GetProfileInt(_T("General"), _T("Options"), 0);
   wcscpy(g_szServer, (LPCTSTR)GetProfileString(_T("Connection"), _T("Server"), _T("localhost")));
   wcscpy(g_szLogin, (LPCTSTR)GetProfileString(_T("Connection"), _T("Login"), NULL));
   g_dwEncryptionMethod = GetProfileInt(_T("Connection"), _T("Encryption"), CSCP_ENCRYPTION_NONE);

   // To create the main window, this code creates a new frame window
	// object and then sets it as the application's main window object.
	CMainFrame* pFrame = new CMainFrame;
	m_pMainWnd = pFrame;

	// create and load the frame with its resources
	pFrame->LoadFrame(IDR_MAINFRAME,
		WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL,
		NULL);

	// The one and only window has been initialized, so show and update it.
	pFrame->ShowWindow(m_nCmdShow);
	pFrame->UpdateWindow();

   // Show login dialog
   pFrame->PostMessage(WM_COMMAND, ID_CONNECT_TO_SERVER, 0);

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CNxpcApp message handlers





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
	virtual BOOL OnInitDialog();		// Added for WCE apps
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
void CNxpcApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CNxpcApp commands
// Added for WCE apps

BOOL CAboutDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CenterWindow();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


//
// Connect to server
//

void CNxpcApp::OnConnectToServer() 
{
	CLoginDlg dlgLogin;
   DWORD dwResult;

   dlgLogin.m_strServer = g_szServer;
   dlgLogin.m_strLogin = g_szLogin;
   dlgLogin.m_bClearCache = FALSE;
   //dlgLogin.m_iEncryption = g_dwEncryptionMethod;
   do
   {
      if (dlgLogin.DoModal() != IDOK)
      {
         PostQuitMessage(1);
         break;
      }
      wcscpy(g_szServer, (LPCTSTR)dlgLogin.m_strServer);
      wcscpy(g_szLogin, (LPCTSTR)dlgLogin.m_strLogin);
      wcscpy(g_szPassword, (LPCTSTR)dlgLogin.m_strPassword);
      //g_dwEncryptionMethod = dlgLogin.m_iEncryption;

      // Save last connection parameters
      WriteProfileString(_T("Connection"), _T("Server"), g_szServer);
      WriteProfileString(_T("Connection"), _T("Login"), g_szLogin);
      WriteProfileInt(_T("Connection"), _T("Encryption"), g_dwEncryptionMethod);

      // Initiate connection
      dwResult = DoLogin(dlgLogin.m_bClearCache);
      if (dwResult == RCC_SUCCESS)
      {
         m_pMainWnd->PostMessage(WM_COMMAND, ID_VIEW_REFRESH_ALL, 0);
      }
      else
      {
         ErrorBox(dwResult, _T("Unable to connect: %s"), _T("Connection error"));
      }
   }
   while(dwResult != RCC_SUCCESS);
}


//
// Display message box with error text from client library
//

void CNxpcApp::ErrorBox(DWORD dwError, TCHAR *pszMessage, TCHAR *pszTitle)
{
   TCHAR szBuffer[512];

   _sntprintf(szBuffer, 512, (pszMessage != NULL) ? pszMessage : _T("Error: %s"), 
              NXCGetErrorText(dwError));
   m_pMainWnd->MessageBox(szBuffer, (pszTitle != NULL) ? pszTitle : _T("Error"), MB_ICONSTOP);
}


//
// Handler for client library events
//

void CNxpcApp::EventHandler(DWORD dwEvent, DWORD dwCode, void *pArg)
{
   switch(dwEvent)
   {
      case NXC_EVENT_OBJECT_CHANGED:
         ((CMainFrame *)m_pMainWnd)->PostMessage(WM_OBJECT_CHANGE, dwCode, (LPARAM)pArg);
         break;
      case NXC_EVENT_NOTIFICATION:
         switch(dwCode)
         {
            case NX_NOTIFY_SHUTDOWN:
               m_pMainWnd->MessageBox(L"Server was shutdown", L"Warning", MB_OK | MB_ICONSTOP);
               m_pMainWnd->PostMessage(WM_CLOSE, 0, 0);
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
         break;
      default:
         break;
   }
}


//
// Application termination handler
//

int CNxpcApp::ExitInstance() 
{
   TCHAR szBuffer[MAX_PATH + 32];
   BYTE bsServerId[8];

   if (g_hSession != NULL)
   {
      NXCGetServerID(g_hSession, bsServerId);
      _tcscpy(szBuffer, g_szWorkDir);
      _tcscat(szBuffer, WORKFILE_OBJECTCACHE);
      BinToStr(bsServerId, 8, &szBuffer[_tcslen(szBuffer)]);
      NXCSaveObjectCache(g_hSession, szBuffer);

      NXCSetDebugCallback(NULL);
      NXCDisconnect(g_hSession);
      NXCShutdown();
      NXCDestroyCCList(g_pCCList);
   }
	
	return CWinApp::ExitInstance();
}
