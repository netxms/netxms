// nxcon.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "nxcon.h"

#include "MainFrm.h"
#include "LoginDialog.h"
#include "CreateContainerDlg.h"
#include "CreateNodeDlg.h"
#include "CreateTemplateDlg.h"
#include "CreateTGDlg.h"
#include "NodePoller.h"
#include "MIBBrowserDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Constants
//

#define LAST_APP_MENU   3


//
// Wrapper for client library event handler
//

static void ClientEventHandler(DWORD dwEvent, DWORD dwCode, void *pArg)
{
   theApp.EventHandler(dwEvent, dwCode, pArg);
}


//
// Wrapper for client library debug callback
//

static void ClientDebugCallback(char *pszMsg)
{
   theApp.DebugCallback(pszMsg);
}


//
// CConsoleApp
//

BEGIN_MESSAGE_MAP(CConsoleApp, CWinApp)
	//{{AFX_MSG_MAP(CConsoleApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_VIEW_CONTROLPANEL, OnViewControlpanel)
	ON_COMMAND(ID_VIEW_EVENTS, OnViewEvents)
	ON_COMMAND(ID_VIEW_MAP, OnViewMap)
	ON_COMMAND(ID_CONNECT_TO_SERVER, OnConnectToServer)
	ON_COMMAND(ID_VIEW_OBJECTBROWSER, OnViewObjectbrowser)
	ON_COMMAND(ID_CONTROLPANEL_EVENTS, OnControlpanelEvents)
	ON_COMMAND(ID_VIEW_DEBUG, OnViewDebug)
	ON_COMMAND(ID_CONTROLPANEL_USERS, OnControlpanelUsers)
	ON_COMMAND(ID_VIEW_NETWORKSUMMARY, OnViewNetworksummary)
	ON_COMMAND(ID_TOOLS_MIBBROWSER, OnToolsMibbrowser)
	ON_COMMAND(ID_CONTROLPANEL_EVENTPOLICY, OnControlpanelEventpolicy)
	ON_COMMAND(ID_VIEW_ALARMS, OnViewAlarms)
	ON_COMMAND(ID_FILE_SETTINGS, OnFileSettings)
	ON_COMMAND(ID_CONTROLPANEL_ACTIONS, OnControlpanelActions)
	ON_COMMAND(ID_TOOLS_ADDNODE, OnToolsAddnode)
	ON_COMMAND(ID_CONTROLPANEL_SNMPTRAPS, OnControlpanelSnmptraps)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//
// CConsoleApp construction
//

CConsoleApp::CConsoleApp()
{
   m_bActionEditorActive = FALSE;
   m_bTrapEditorActive = FALSE;
   m_bCtrlPanelActive = FALSE;
   m_bAlarmBrowserActive = FALSE;
   m_bEventBrowserActive = FALSE;
   m_bEventEditorActive = FALSE;
   m_bUserEditorActive = FALSE;
   m_bObjectBrowserActive = FALSE;
   m_bDebugWindowActive = FALSE;
   m_bNetSummaryActive = FALSE;
   m_bEventPolicyEditorActive = FALSE;
   m_dwClientState = STATE_DISCONNECTED;
   memset(m_openDCEditors, 0, sizeof(DC_EDITOR) * MAX_DC_EDITORS);
}


//
// CConsoleApp destruction
//

CConsoleApp::~CConsoleApp()
{
}


//
// The one and only CConsoleApp object
//

CConsoleApp theApp;


//
// Setup working directory
//

BOOL CConsoleApp::SetupWorkDir()
{
   int iLastChar;

   // Get path to user's "Application Data" folder
   if (SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, g_szWorkDir) != S_OK)
   {
		AfxMessageBox(IDS_GETFOLDERPATH_FAILED, MB_OK | MB_ICONSTOP);
      return FALSE;
   }

   // Create working directory root
   strcat(g_szWorkDir, "\\NetXMS Console");
   if (!CreateDirectory(g_szWorkDir, NULL))
      if (GetLastError() != ERROR_ALREADY_EXISTS)
      {
         AfxMessageBox(IDS_WORKDIR_CREATION_FAILED, MB_OK | MB_ICONSTOP);
         return FALSE;
      }
   iLastChar = strlen(g_szWorkDir);

   // Create MIB cache directory
   strcpy(&g_szWorkDir[iLastChar], WORKDIR_MIBCACHE);
   if (!CreateDirectory(g_szWorkDir, NULL))
      if (GetLastError() != ERROR_ALREADY_EXISTS)
      {
         AfxMessageBox(IDS_WORKDIR_CREATION_FAILED, MB_OK | MB_ICONSTOP);
         return FALSE;
      }

   // Create image cache directory
   strcpy(&g_szWorkDir[iLastChar], WORKDIR_IMAGECACHE);
   if (!CreateDirectory(g_szWorkDir, NULL))
      if (GetLastError() != ERROR_ALREADY_EXISTS)
      {
         AfxMessageBox(IDS_WORKDIR_CREATION_FAILED, MB_OK | MB_ICONSTOP);
         return FALSE;
      }

   // Success
   g_szWorkDir[iLastChar] = 0;
   return TRUE;
}


//
// CConsoleApp initialization
//

BOOL CConsoleApp::InitInstance()
{
   BOOL bSetWindowPos;
   DWORD dwBytes;
   BYTE *pData;
   HMENU hMenu;

   // Setup our working directory
   if (!SetupWorkDir())
      return FALSE;

	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED, MB_OK | MB_ICONSTOP);
		return FALSE;
	}

   if (!NXCInitialize())
   {
		AfxMessageBox(IDS_NXC_INIT_FAILED, MB_OK | MB_ICONSTOP);
		return FALSE;
   }
   NXCSetEventHandler(ClientEventHandler);

	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Change the registry key under which our settings are stored.
	SetRegistryKey(_T("NetXMS"));

   // Load configuration from registry
   dwBytes = sizeof(WINDOWPLACEMENT);
   bSetWindowPos = GetProfileBinary(_T("General"), _T("WindowPlacement"), 
                                    &pData, (UINT *)&dwBytes);
   g_dwOptions = GetProfileInt(_T("General"), _T("Options"), 0);
   strcpy(g_szServer, (LPCTSTR)GetProfileString(_T("Connection"), _T("Server"), _T("localhost")));
   strcpy(g_szLogin, (LPCTSTR)GetProfileString(_T("Connection"), _T("Login"), NULL));
   g_dwEncryptionMethod = GetProfileInt(_T("Connection"), _T("Encryption"), CSCP_ENCRYPTION_NONE);

   // Create mutex for action list access
   g_mutexActionListAccess = CreateMutex(NULL, FALSE, NULL);

	// To create the main window, this code creates a new frame window
	// object and then sets it as the application's main window object.
	CMDIFrameWnd* pFrame = new CMainFrame;
	m_pMainWnd = pFrame;

	// create main MDI frame window
	if (!pFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;

   // Load context menus
   if (!m_ctxMenu.LoadMenu(IDM_CONTEXT))
      MessageBox(NULL, "FAILED", "", 0);

	// Load shared MDI menus and accelerator table
	HINSTANCE hInstance = AfxGetResourceHandle();

	hMenu  = ::LoadMenu(hInstance, MAKEINTRESOURCE(IDM_VIEW_SPECIFIC));

   // Modify application menu as needed
   if (g_dwOptions & UI_OPT_EXPAND_CTRLPANEL)
   {
      pFrame->GetMenu()->RemoveMenu(ID_VIEW_CONTROLPANEL, MF_BYCOMMAND);
      pFrame->GetMenu()->InsertMenu(ID_VIEW_DEBUG, MF_BYCOMMAND | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 8), "&Control panel");
   }

   m_hMDIMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hMDIMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), "&Window");

   m_hEventBrowserMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hEventBrowserMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), "&Window");
   InsertMenu(m_hEventBrowserMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 1), "&Event");

   m_hObjectBrowserMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hObjectBrowserMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), "&Window");
   InsertMenu(m_hObjectBrowserMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 2), "&Object");

   m_hUserEditorMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hUserEditorMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), "&Window");
   InsertMenu(m_hUserEditorMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 3), "&User");

   m_hDCEditorMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hDCEditorMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), "&Window");
   InsertMenu(m_hDCEditorMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 4), "&Item");

   m_hPolicyEditorMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hPolicyEditorMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), "&Window");
   InsertMenu(m_hPolicyEditorMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 5), "&Policy");

   m_hAlarmBrowserMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hAlarmBrowserMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), "&Window");
   InsertMenu(m_hAlarmBrowserMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 6), "&Alarm");

   m_hMapMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hMapMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), "&Window");
   InsertMenu(m_hMapMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 9), "&Object");

   m_hEventEditorMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hEventEditorMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), "&Window");
   InsertMenu(m_hEventEditorMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 10), "&Event");

	m_hMDIAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_MDI_DEFAULT));
	m_hAlarmBrowserAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_ALARM_BROWSER));
	m_hEventBrowserAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_MDI_DEFAULT));
	m_hObjectBrowserAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_OBJECT_BROWSER));
	m_hEventEditorAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_EVENT_EDITOR));
	m_hUserEditorAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_MDI_DEFAULT));
	m_hDCEditorAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_MDI_DEFAULT));
	m_hPolicyEditorAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_EPP));
	m_hMapAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_NETMAP));

	// The main window has been initialized, so show and update it.
   if (bSetWindowPos)
   {
      pFrame->SetWindowPlacement((WINDOWPLACEMENT *)pData);
      if (pFrame->IsIconic())
         pFrame->ShowWindow(SW_RESTORE);
   }
   else
   {
	   pFrame->ShowWindow(m_nCmdShow);
   }
	pFrame->UpdateWindow();
   //pFrame->PostMessage(WM_COMMAND, ID_VIEW_DEBUG, 0);
   pFrame->PostMessage(WM_COMMAND, ID_CONNECT_TO_SERVER, 0);

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CConsoleApp message handlers

int CConsoleApp::ExitInstance() 
{
   NXCSetDebugCallback(NULL);
   NXCDisconnect();
   NXCShutdown();
   NXCDestroyCCList(g_pCCList);

   // Save configuration
   WriteProfileInt(_T("General"), _T("Options"), g_dwOptions);

   // Free resources
   SafeFreeResource(m_hMDIMenu);
   SafeFreeResource(m_hMDIAccel);
   SafeFreeResource(m_hAlarmBrowserMenu);
   SafeFreeResource(m_hAlarmBrowserAccel);
   SafeFreeResource(m_hEventBrowserMenu);
   SafeFreeResource(m_hEventBrowserAccel);
   SafeFreeResource(m_hUserEditorMenu);
   SafeFreeResource(m_hUserEditorAccel);
   SafeFreeResource(m_hDCEditorMenu);
   SafeFreeResource(m_hDCEditorAccel);

   CloseHandle(g_mutexActionListAccess);

   return CWinApp::ExitInstance();
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
	CStatic	m_wndVersion;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	virtual BOOL OnInitDialog();
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
	DDX_Control(pDX, IDC_VERSION, m_wndVersion);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CAboutDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
   m_wndVersion.SetWindowText(_T("NetXMS Console Version ") NETXMS_VERSION_STRING);
	return TRUE;
}

// App command to run the dialog
void CConsoleApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CConsoleApp message handlers


void CConsoleApp::OnViewControlpanel() 
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);

	// create a new MDI child window or open existing
   if (m_bCtrlPanelActive)
      m_pwndCtrlPanel->BringWindowToTop();
   else
	   pFrame->CreateNewChild(
		   RUNTIME_CLASS(CControlPanel), IDR_CTRLPANEL, m_hMDIMenu, m_hMDIAccel);	
}


//
// This method called when new view is created
//

void CConsoleApp::OnViewCreate(DWORD dwView, CWnd *pWnd, DWORD dwArg)
{
   DWORD i;

   switch(dwView)
   {
      case IDR_CTRLPANEL:
         m_bCtrlPanelActive = TRUE;
         m_pwndCtrlPanel = pWnd;
         break;
      case IDR_ALARMS:
         m_bAlarmBrowserActive = TRUE;
         m_pwndAlarmBrowser = (CAlarmBrowser *)pWnd;
         break;
      case IDR_EVENTS:
         m_bEventBrowserActive = TRUE;
         m_pwndEventBrowser = (CEventBrowser *)pWnd;
         break;
      case IDR_OBJECTS:
         m_bObjectBrowserActive = TRUE;
         m_pwndObjectBrowser = pWnd;
         break;
      case IDR_EVENT_EDITOR:
         m_bEventEditorActive = TRUE;
         m_pwndEventEditor = (CEventEditor *)pWnd;
         break;
      case IDR_ACTION_EDITOR:
         m_bActionEditorActive = TRUE;
         m_pwndActionEditor = (CActionEditor *)pWnd;
         break;
      case IDR_TRAP_EDITOR:
         m_bTrapEditorActive = TRUE;
         m_pwndTrapEditor = (CTrapEditor *)pWnd;
         break;
      case IDR_USER_EDITOR:
         m_bUserEditorActive = TRUE;
         m_pwndUserEditor = (CUserEditor *)pWnd;
         break;
      case IDR_DEBUG_WINDOW:
         m_bDebugWindowActive = TRUE;
         m_pwndDebugWindow = (CDebugFrame *)pWnd;
         NXCSetDebugCallback(ClientDebugCallback);
         break;
      case IDR_NETWORK_SUMMARY:
         m_bNetSummaryActive = TRUE;
         m_pwndNetSummary = (CNetSummaryFrame *)pWnd;
         break;
      case IDR_DC_EDITOR:
         // Register new DC editor
         for(i = 0; i < MAX_DC_EDITORS; i++)
            if (m_openDCEditors[i].pWnd == NULL)
            {
               m_openDCEditors[i].pWnd = pWnd;
               m_openDCEditors[i].dwNodeId = dwArg;
               break;
            }
         break;
      case IDR_EPP_EDITOR:
         m_bEventPolicyEditorActive = TRUE;
         m_pwndEventPolicyEditor = (CEventPolicyEditor *)pWnd;
         break;
      default:
         break;
   }
}


//
// This method called when view is destroyed
//

void CConsoleApp::OnViewDestroy(DWORD dwView, CWnd *pWnd, DWORD dwArg)
{
   DWORD i;

   switch(dwView)
   {
      case IDR_CTRLPANEL:
         m_bCtrlPanelActive = FALSE;
         break;
      case IDR_ALARMS:
         m_bAlarmBrowserActive = FALSE;
         break;
      case IDR_EVENTS:
         m_bEventBrowserActive = FALSE;
         break;
      case IDR_OBJECTS:
         m_bObjectBrowserActive = FALSE;
         break;
      case IDR_EVENT_EDITOR:
         m_bEventEditorActive = FALSE;
         break;
      case IDR_ACTION_EDITOR:
         m_bActionEditorActive = FALSE;
         break;
      case IDR_TRAP_EDITOR:
         m_bTrapEditorActive = FALSE;
         break;
      case IDR_USER_EDITOR:
         m_bUserEditorActive = FALSE;
         break;
      case IDR_DEBUG_WINDOW:
         m_bDebugWindowActive = FALSE;
         NXCSetDebugCallback(NULL);
         break;
      case IDR_NETWORK_SUMMARY:
         m_bNetSummaryActive = FALSE;
         break;
      case IDR_DC_EDITOR:
         // Unregister DC editor
         for(i = 0; i < MAX_DC_EDITORS; i++)
            if (m_openDCEditors[i].pWnd == pWnd)
            {
               m_openDCEditors[i].pWnd = NULL;
               m_openDCEditors[i].dwNodeId = 0;
               break;
            }
         break;
      case IDR_EPP_EDITOR:
         m_bEventPolicyEditorActive = FALSE;
         break;
      default:
         break;
   }
}


//
// WM_COMMAND::ID_VIEW_EVENTS message handler
//

void CConsoleApp::OnViewEvents() 
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);

	// create a new MDI child window or open existing
   if (m_bEventBrowserActive)
      m_pwndEventBrowser->BringWindowToTop();
   else
	   pFrame->CreateNewChild(
		   RUNTIME_CLASS(CEventBrowser), IDR_EVENTS, m_hEventBrowserMenu, m_hEventBrowserAccel);
}


//
// WM_COMMAND::ID_VIEW_MAP message handler
//

void CConsoleApp::OnViewMap() 
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);

	// create a new MDI child window
	pFrame->CreateNewChild(RUNTIME_CLASS(CMapFrame), IDR_MAPFRAME, m_hMapMenu, m_hMapAccel);
}


//
// WM_COMMAND::ID_CONNECT_TO_SERVER message handler
//

void CConsoleApp::OnConnectToServer() 
{
	CLoginDialog dlgLogin;
   DWORD dwResult;

   dlgLogin.m_szServer = g_szServer;
   dlgLogin.m_szLogin = g_szLogin;
   dlgLogin.m_iEncryption = g_dwEncryptionMethod;
   do
   {
      if (dlgLogin.DoModal() != IDOK)
      {
         PostQuitMessage(1);
         break;
      }
      strcpy(g_szServer, (LPCTSTR)dlgLogin.m_szServer);
      strcpy(g_szLogin, (LPCTSTR)dlgLogin.m_szLogin);
      strcpy(g_szPassword, (LPCTSTR)dlgLogin.m_szPassword);
      g_dwEncryptionMethod = dlgLogin.m_iEncryption;

      // Save last connection parameters
      WriteProfileString(_T("Connection"), _T("Server"), g_szServer);
      WriteProfileString(_T("Connection"), _T("Login"), g_szLogin);
      WriteProfileInt(_T("Connection"), _T("Encryption"), g_dwEncryptionMethod);

      // Initiate connection
      dwResult = DoLogin();
      if (dwResult != RCC_SUCCESS)
         ErrorBox(dwResult, "Unable to connect: %s", "Connection error");
   }
   while(dwResult != RCC_SUCCESS);
}


//
// WM_COMMAND::ID_VIEW_OBJECTBROWSER message handler
//

void CConsoleApp::OnViewObjectbrowser() 
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);

	// create a new MDI child window or open existing
   if (m_bObjectBrowserActive)
      m_pwndObjectBrowser->BringWindowToTop();
   else
	   pFrame->CreateNewChild(
		   RUNTIME_CLASS(CObjectBrowser), IDR_OBJECTS, m_hObjectBrowserMenu, m_hObjectBrowserAccel);	
}


//
// Event handler for client libray events
//

void CConsoleApp::EventHandler(DWORD dwEvent, DWORD dwCode, void *pArg)
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);

   switch(dwEvent)
   {
      case NXC_EVENT_STATE_CHANGED:
         m_dwClientState = dwCode;
         ((CMainFrame *)m_pMainWnd)->PostMessage(WM_STATE_CHANGE, dwCode, (LPARAM)pArg);
         break;
      case NXC_EVENT_NEW_ELOG_RECORD:
         if (m_bEventBrowserActive)
            m_pwndEventBrowser->AddEvent((NXC_EVENT *)pArg);
         free(pArg);
         break;
      case NXC_EVENT_OBJECT_CHANGED:
         ((CMainFrame *)m_pMainWnd)->PostMessage(WM_OBJECT_CHANGE, dwCode, (LPARAM)pArg);
         break;
      case NXC_EVENT_USER_DB_CHANGED:
         ((CMainFrame *)m_pMainWnd)->PostMessage(WM_USERDB_CHANGE, dwCode, (LPARAM)pArg);
         break;
      case NXC_EVENT_NOTIFICATION:
         switch(dwCode)
         {
            case NX_NOTIFY_SHUTDOWN:
               m_pMainWnd->MessageBox("Server was shutdown", "Warning", MB_OK | MB_ICONSTOP);
               PostQuitMessage(0);
               break;
            case NX_NOTIFY_EVENTDB_CHANGED:
               m_pMainWnd->PostMessage(WM_COMMAND, ID_UPDATE_EVENT_LIST, 0);
               break;
            case NX_NOTIFY_NEW_ALARM:
            case NX_NOTIFY_ALARM_DELETED:
            case NX_NOTIFY_ALARM_ACKNOWLEGED:
               m_pMainWnd->PostMessage(WM_ALARM_UPDATE, dwCode, 
                                       (LPARAM)nx_memdup(pArg, sizeof(NXC_ALARM)));
               break;
            case NX_NOTIFY_ACTION_CREATED:
            case NX_NOTIFY_ACTION_MODIFIED:
            case NX_NOTIFY_ACTION_DELETED:
               UpdateActions(dwCode, (NXC_ACTION *)pArg);
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
// Handler for WM_COMMAND(ID_CONTROLPANEL_EVENTS) message
//

void CConsoleApp::OnControlpanelEvents() 
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);

	// create a new MDI child window or open existing
   if (m_bEventEditorActive)
      m_pwndEventEditor->BringWindowToTop();
   else
   {
      DWORD dwResult;

      dwResult = DoRequest(NXCLockEventDB, "Locking event configuration database...");
      if (dwResult == RCC_SUCCESS)
      {
	      pFrame->CreateNewChild(
		      RUNTIME_CLASS(CEventEditor), IDR_EVENT_EDITOR, m_hEventEditorMenu, m_hEventEditorAccel);
      }
      else
      {
         ErrorBox(dwResult, "Unable to open event configuration database:\n%s");
      }
   }
}


//
// Handler for WM_COMMAND::ID_CONTROLPANEL_USERS message
//

void CConsoleApp::OnControlpanelUsers() 
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);

	// create a new MDI child window or open existing
   if (m_bUserEditorActive)
      m_pwndUserEditor->BringWindowToTop();
   else
   {
      DWORD dwResult;

      dwResult = DoRequest(NXCLockUserDB, "Locking user database...");
      if (dwResult == RCC_SUCCESS)
      {
	      pFrame->CreateNewChild(
		      RUNTIME_CLASS(CUserEditor), IDR_USER_EDITOR, m_hUserEditorMenu, m_hUserEditorAccel);
      }
      else
      {
         ErrorBox(dwResult, "Unable to lock user database:\n%s");
      }
   }
}


//
// Handler for WM_COMMAND::ID_VIEW_DEBUG message
//

void CConsoleApp::OnViewDebug() 
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);

	// create a new MDI child window or open existing
   if (m_bDebugWindowActive)
      m_pwndDebugWindow->BringWindowToTop();
   else
	   pFrame->CreateNewChild(
		   RUNTIME_CLASS(CDebugFrame), IDR_DEBUG_WINDOW, m_hMDIMenu, m_hMDIAccel);	
}


//
// Print debug information
//

void CConsoleApp::DebugPrintf(char *szFormat, ...)
{
   if (m_bDebugWindowActive)
   {
      char szBuffer[1024];
      va_list args;

      va_start(args, szFormat);
      vsprintf(szBuffer, szFormat, args);
      va_end(args);

      m_pwndDebugWindow->AddMessage(szBuffer);
   }
}


//
// Edit properties of specific object
//

void CConsoleApp::ObjectProperties(DWORD dwObjectId)
{
	CObjectPropSheet wndPropSheet("Object Properties", GetMainWnd(), 0);
   CNodePropsGeneral wndNodeGeneral;
   CObjectPropsGeneral wndObjectGeneral;
   CObjectPropCaps wndObjectCaps;
   CObjectPropsSecurity wndObjectSecurity;
   CObjectPropsPresentation wndObjectPresentation;
   NXC_OBJECT *pObject;
   char szBuffer[32];

   pObject = NXCFindObjectById(dwObjectId);
   if (pObject != NULL)
   {
      // Create "General" tab
      if (pObject->iClass == OBJECT_NODE)
      {
         wndNodeGeneral.m_dwObjectId = dwObjectId;
         wndNodeGeneral.m_strName = pObject->szName;
         wndNodeGeneral.m_strOID = pObject->node.szObjectId;
         wndNodeGeneral.m_strPrimaryIp = IpToStr(pObject->dwIpAddr, szBuffer);
         wndNodeGeneral.m_iAgentPort = (int)pObject->node.wAgentPort;
         wndNodeGeneral.m_strCommunity = pObject->node.szCommunityString;
         wndNodeGeneral.m_iAuthType = pObject->node.wAuthMethod;
         wndNodeGeneral.m_strSecret = pObject->node.szSharedSecret;
         wndNodeGeneral.m_iSNMPVersion = (pObject->node.wSNMPVersion == SNMP_VERSION_1) ? 0 : 1;
         wndPropSheet.AddPage(&wndNodeGeneral);
      }
      else
      {
         wndObjectGeneral.m_dwObjectId = dwObjectId;
         wndObjectGeneral.m_strClass = g_szObjectClass[pObject->iClass];
         wndObjectGeneral.m_strName = pObject->szName;
         wndPropSheet.AddPage(&wndObjectGeneral);
      }

      // Create tabs specific for node objects
      if (pObject->iClass == OBJECT_NODE)
      {
         // Create "Capabilities" tab
         wndObjectCaps.m_pObject = pObject;
         wndPropSheet.AddPage(&wndObjectCaps);
      }

      // Create "Security" tab
      wndObjectSecurity.m_pObject = pObject;
      wndObjectSecurity.m_bInheritRights = pObject->bInheritRights;
      wndPropSheet.AddPage(&wndObjectSecurity);

      // Create "Presentation" tab
      wndObjectPresentation.m_dwImageId = pObject->dwImage;
      wndObjectPresentation.m_bUseDefaultImage = (pObject->dwImage == IMG_DEFAULT);
      wndPropSheet.AddPage(&wndObjectPresentation);

      wndPropSheet.SetObject(pObject);
      if (wndPropSheet.DoModal() == IDOK)
         wndPropSheet.SaveObjectChanges();
   }
}


//
// Get context menu
//

CMenu *CConsoleApp::GetContextMenu(int iIndex)
{
   return m_ctxMenu.GetSubMenu(iIndex);
}


//
// Start data collection editor for specific object
//

void CConsoleApp::StartObjectDCEditor(NXC_OBJECT *pObject)
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);
   NXC_DCI_LIST *pItemList;
   DWORD dwResult;
   CDataCollectionEditor *pWnd;

   pWnd = (CDataCollectionEditor *)FindOpenDCEditor(pObject->dwId);
   if (pWnd == NULL)
   {
      dwResult = DoRequestArg2(NXCOpenNodeDCIList, (void *)pObject->dwId, 
                               &pItemList, "Loading node's data collection information...");
      if (dwResult == RCC_SUCCESS)
      {
   	   CCreateContext context;
      
         pWnd = new CDataCollectionEditor(pItemList);

   	   // load the frame
	      context.m_pCurrentFrame = pFrame;

	      if (pWnd->LoadFrame(IDR_DC_EDITOR, WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL, &context))
         {
	         CString strFullString, strTitle;

	         if (strFullString.LoadString(IDR_DC_EDITOR))
		         AfxExtractSubString(strTitle, strFullString, CDocTemplate::docName);

            // add node name to title
            strTitle += " - [";
            strTitle += pObject->szName;
            strTitle += "]";

	         // set the handles and redraw the frame and parent
	         pWnd->SetHandles(m_hDCEditorMenu, m_hDCEditorAccel);
	         pWnd->SetTitle(strTitle);
	         pWnd->InitialUpdateFrame(NULL, TRUE);
         }
         else
	      {
		      delete pWnd;
	      }
      }
      else
      {
         ErrorBox(dwResult, "Unable to load data collection information for node:\n%s");
      }
   }
   else
   {
      // Data collection editor already open, activate it
      pWnd->BringWindowToTop();
   }
}


//
// Show network summary view
//

void CConsoleApp::OnViewNetworksummary() 
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);

	// create a new MDI child window or open existing
   if (m_bNetSummaryActive)
      m_pwndNetSummary->BringWindowToTop();
   else
	   pFrame->CreateNewChild(
		   RUNTIME_CLASS(CNetSummaryFrame), IDR_NETWORK_SUMMARY, m_hMDIMenu, m_hMDIAccel);	
}


//
// Set object's management status
//

void CConsoleApp::SetObjectMgmtStatus(NXC_OBJECT *pObject, BOOL bIsManaged)
{
   DWORD dwResult;

   dwResult = DoRequestArg2(NXCSetObjectMgmtStatus, (void *)pObject->dwId, 
                            (void *)bIsManaged, "Changing object status...");
   if (dwResult != RCC_SUCCESS)
   {
      char szBuffer[256];

      sprintf(szBuffer, "Unable to change management status for object %s:\n%s", 
              pObject->szName, NXCGetErrorText(dwResult));
      GetMainWnd()->MessageBox(szBuffer, "Error", MB_ICONSTOP);
   }
}


//
// Find open data collection editor for given node, if any
//

CWnd *CConsoleApp::FindOpenDCEditor(DWORD dwNodeId)
{
   DWORD i;

   for(i = 0; i < MAX_DC_EDITORS; i++)
      if (m_openDCEditors[i].dwNodeId == dwNodeId)
         return m_openDCEditors[i].pWnd;
   return NULL;
}


//
// Display message box with error text from client library
//

void CConsoleApp::ErrorBox(DWORD dwError, TCHAR *pszMessage, TCHAR *pszTitle)
{
   TCHAR szBuffer[512];

   _sntprintf(szBuffer, 512, (pszMessage != NULL) ? pszMessage : _T("Error: %s"), 
              NXCGetErrorText(dwError));
   m_pMainWnd->MessageBox(szBuffer, (pszTitle != NULL) ? pszTitle : _T("Error"), MB_ICONSTOP);
}


//
// Show window with DCI's data
//

void CConsoleApp::ShowDCIData(DWORD dwNodeId, DWORD dwItemId, char *pszItemName)
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);
   CCreateContext context;
   CDCIDataView *pWnd;

   pWnd = new CDCIDataView(dwNodeId, dwItemId);

   // load the frame
	context.m_pCurrentFrame = pFrame;

	if (pWnd->LoadFrame(IDR_DCI_DATA_VIEW, WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL, &context))
   {
	   CString strFullString, strTitle;

	   if (strFullString.LoadString(IDR_DCI_DATA_VIEW))
		   AfxExtractSubString(strTitle, strFullString, CDocTemplate::docName);

      // add node name to title
      strTitle += " - [";
      strTitle += pszItemName;
      strTitle += "]";

	   // set the handles and redraw the frame and parent
	   pWnd->SetHandles(m_hMDIMenu, m_hMDIAccel);
	   pWnd->SetTitle(strTitle);
	   pWnd->InitialUpdateFrame(NULL, TRUE);
   }
   else
	{
		delete pWnd;
	}
}


//
// Show graph for collected data
//

void CConsoleApp::ShowDCIGraph(DWORD dwNodeId, DWORD dwItemId, char *pszItemName)
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);
   CCreateContext context;
   CGraphFrame *pWnd;
   DWORD dwCurrTime;

   pWnd = new CGraphFrame;
   pWnd->AddItem(dwNodeId, dwItemId);
   dwCurrTime = time(NULL);
   pWnd->SetTimeFrame(dwCurrTime - 3600, dwCurrTime);    // Last hour

   // load the frame
	context.m_pCurrentFrame = pFrame;

	if (pWnd->LoadFrame(IDR_DCI_HISTORY_GRAPH, WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL, &context))
   {
	   CString strFullString, strTitle;

	   if (strFullString.LoadString(IDR_DCI_HISTORY_GRAPH))
		   AfxExtractSubString(strTitle, strFullString, CDocTemplate::docName);

      // add item name to title
      strTitle += " - [";
      strTitle += pszItemName;
      strTitle += "]";

	   // set the handles and redraw the frame and parent
	   pWnd->SetHandles(m_hMDIMenu, m_hMDIAccel);
	   pWnd->SetTitle(strTitle);
	   pWnd->InitialUpdateFrame(NULL, TRUE);
   }
   else
	{
		delete pWnd;
	}
}


//
// WM_COMMAND::ID_TOOLS_MIBBROWSER message handler
//

void CConsoleApp::OnToolsMibbrowser() 
{
   CMIBBrowserDlg dlg;
   dlg.DoModal();
}


//
// WM_COMMAND::ID_CONTROLPANEL_EVENTPOLICY message handler
//

void CConsoleApp::OnControlpanelEventpolicy() 
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);

	// create a new MDI child window or open existing
   if (m_bEventPolicyEditorActive)
   {
      m_pwndEventPolicyEditor->BringWindowToTop();
   }
   else
   {
      DWORD dwResult;

      dwResult = DoRequestArg1(NXCOpenEventPolicy, &m_pEventPolicy, "Loading event processing policy...");
      if (dwResult == RCC_SUCCESS)
      {
	      pFrame->CreateNewChild(
		      RUNTIME_CLASS(CEventPolicyEditor), IDR_EPP_EDITOR, m_hPolicyEditorMenu, m_hPolicyEditorAccel);
      }
      else
      {
         ErrorBox(dwResult, "Unable to load event processing policy:\n%s");
      }
   }
}


//
// WM_COMMAND::ID_VIEW_ALARMS message handler
//

void CConsoleApp::OnViewAlarms() 
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);

	// create a new MDI child window or open existing
   if (m_bAlarmBrowserActive)
      m_pwndAlarmBrowser->BringWindowToTop();
   else
	   pFrame->CreateNewChild(
		   RUNTIME_CLASS(CAlarmBrowser), IDR_ALARMS, m_hAlarmBrowserMenu, m_hAlarmBrowserAccel);
}


//
// WM_COMMAND::ID_FILE_SETTINGS message handler
//

void CConsoleApp::OnFileSettings() 
{
	CPropertySheet wndPropSheet("Settings", GetMainWnd(), 0);
   CConsolePropsGeneral wndGeneral;

   // "General" page
   wndGeneral.m_bExpandCtrlPanel = (g_dwOptions & UI_OPT_EXPAND_CTRLPANEL) ? TRUE : FALSE;
   wndGeneral.m_bShowGrid = (g_dwOptions & UI_OPT_SHOW_GRID) ? TRUE : FALSE;
   wndPropSheet.AddPage(&wndGeneral);

   if (wndPropSheet.DoModal() == IDOK)
   {
      if (wndGeneral.m_bExpandCtrlPanel)
         g_dwOptions |= UI_OPT_EXPAND_CTRLPANEL;
      else
         g_dwOptions &= ~UI_OPT_EXPAND_CTRLPANEL;

      if (wndGeneral.m_bShowGrid)
         g_dwOptions |= UI_OPT_SHOW_GRID;
      else
         g_dwOptions &= ~UI_OPT_SHOW_GRID;
   }
}


//
// WM_COMMAND::ID_CONTROLPANEL_ACTIONS message handler
//

void CConsoleApp::OnControlpanelActions() 
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);

	// create a new MDI child window or open existing
   if (m_bActionEditorActive)
   {
      m_pwndActionEditor->BringWindowToTop();
   }
   else
   {
      DWORD dwResult;

      dwResult = DoRequest(NXCLockActionDB, "Locking action configuration database...");
      if (dwResult == RCC_SUCCESS)
      {
	      pFrame->CreateNewChild(
		      RUNTIME_CLASS(CActionEditor), IDR_ACTION_EDITOR, m_hMDIMenu, m_hMDIAccel);
      }
      else
      {
         ErrorBox(dwResult, "Unable to lock action configuration database:\n%s");
      }
   }
}


//
// WM_COMMAND::ID_CONTROLPANEL_SNMPTRAPS message handler
//

void CConsoleApp::OnControlpanelSnmptraps() 
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);

	// create a new MDI child window or open existing
   if (m_bTrapEditorActive)
   {
      m_pwndTrapEditor->BringWindowToTop();
   }
   else
   {
      DWORD dwResult;

      dwResult = DoRequest(NXCLockTrapCfg, "Locking SNMP trap configuration database...");
      if (dwResult == RCC_SUCCESS)
      {
	      pFrame->CreateNewChild(
		      RUNTIME_CLASS(CTrapEditor), IDR_TRAP_EDITOR, m_hMDIMenu, m_hMDIAccel);
      }
      else
      {
         ErrorBox(dwResult, "Unable to lock SNMP trap configuration database:\n%s");
      }
   }
}


//
// Load application menu from resources and modify it as needed
//

HMENU CConsoleApp::LoadAppMenu(HMENU hViewMenu)
{
   HMENU hMenu;

   hMenu = ::LoadMenu(m_hInstance, MAKEINTRESOURCE(IDR_MAINFRAME));

   if (g_dwOptions & UI_OPT_EXPAND_CTRLPANEL)
   {
      RemoveMenu(hMenu, ID_VIEW_CONTROLPANEL, MF_BYCOMMAND);
      InsertMenu(hMenu, ID_VIEW_DEBUG, MF_BYCOMMAND | MF_POPUP, (UINT_PTR)GetSubMenu(hViewMenu, 8), "&Control panel");
   }

   return hMenu;
}


//
// Create new object
//

void CConsoleApp::CreateObject(NXC_OBJECT_CREATE_INFO *pInfo)
{
   DWORD dwResult, dwObjectId;

   dwResult = DoRequestArg2(NXCCreateObject, pInfo, &dwObjectId, "Creating object...");
   if (dwResult != RCC_SUCCESS)
   {
      ErrorBox(dwResult, "Error creating object: %s");
   }
}


//
// Create container object
//

void CConsoleApp::CreateContainer(DWORD dwParent)
{
   NXC_OBJECT_CREATE_INFO ci;
   CCreateContainerDlg dlg;

   dlg.m_pParentObject = NXCFindObjectById(dwParent);
   if (dlg.m_pParentObject != NULL)
      if ((dlg.m_pParentObject->iClass != OBJECT_CONTAINER) &&
          (dlg.m_pParentObject->iClass != OBJECT_SERVICEROOT))
         dlg.m_pParentObject = NULL;
   if (dlg.DoModal() == IDOK)
   {
      ci.dwParentId = (dlg.m_pParentObject != NULL) ? dlg.m_pParentObject->dwId : 0;
      ci.iClass = OBJECT_CONTAINER;
      ci.pszName = (char *)((LPCTSTR)dlg.m_strObjectName);
      ci.cs.container.dwCategory = 1;
      ci.cs.container.pszDescription = (char *)((LPCTSTR)dlg.m_strDescription);
      CreateObject(&ci);
   }
}


//
// Create node object
//

void CConsoleApp::CreateNode(DWORD dwParent)
{
   NXC_OBJECT_CREATE_INFO ci;
   CCreateNodeDlg dlg;

   dlg.m_pParentObject = NXCFindObjectById(dwParent);
   if (dlg.m_pParentObject != NULL)
      if ((dlg.m_pParentObject->iClass != OBJECT_CONTAINER) &&
          (dlg.m_pParentObject->iClass != OBJECT_SERVICEROOT))
         dlg.m_pParentObject = NULL;
   if (dlg.DoModal() == IDOK)
   {
      ci.dwParentId = (dlg.m_pParentObject != NULL) ? dlg.m_pParentObject->dwId : 0;
      ci.iClass = OBJECT_NODE;
      ci.pszName = (char *)((LPCTSTR)dlg.m_strObjectName);
      ci.cs.node.dwIpAddr = dlg.m_dwIpAddr;
      ci.cs.node.dwNetMask = 0;
      CreateObject(&ci);
   }
}


//
// Create template object
//

void CConsoleApp::CreateTemplate(DWORD dwParent)
{
   NXC_OBJECT_CREATE_INFO ci;
   CCreateTemplateDlg dlg;

   dlg.m_pParentObject = NXCFindObjectById(dwParent);
   if (dlg.m_pParentObject != NULL)
      if ((dlg.m_pParentObject->iClass != OBJECT_TEMPLATEGROUP) &&
          (dlg.m_pParentObject->iClass != OBJECT_TEMPLATEROOT))
         dlg.m_pParentObject = NULL;
   if (dlg.DoModal() == IDOK)
   {
      ci.dwParentId = (dlg.m_pParentObject != NULL) ? dlg.m_pParentObject->dwId : 0;
      ci.iClass = OBJECT_TEMPLATE;
      ci.pszName = (char *)((LPCTSTR)dlg.m_strObjectName);
      CreateObject(&ci);
   }
}


//
// Create template group object
//

void CConsoleApp::CreateTemplateGroup(DWORD dwParent)
{
   NXC_OBJECT_CREATE_INFO ci;
   CCreateTGDlg dlg;

   dlg.m_pParentObject = NXCFindObjectById(dwParent);
   if (dlg.m_pParentObject != NULL)
      if ((dlg.m_pParentObject->iClass != OBJECT_TEMPLATEGROUP) &&
          (dlg.m_pParentObject->iClass != OBJECT_TEMPLATEROOT))
         dlg.m_pParentObject = NULL;
   if (dlg.DoModal() == IDOK)
   {
      ci.dwParentId = (dlg.m_pParentObject != NULL) ? dlg.m_pParentObject->dwId : 0;
      ci.iClass = OBJECT_TEMPLATEGROUP;
      ci.pszName = (char *)((LPCTSTR)dlg.m_strObjectName);
      ci.cs.templateGroup.pszDescription = (char *)((LPCTSTR)dlg.m_strDescription);
      CreateObject(&ci);
   }
}


//
// Delete object on server
//

void CConsoleApp::DeleteNetXMSObject(NXC_OBJECT *pObject)
{
   DWORD dwResult;

   dwResult = DoRequestArg1(NXCDeleteObject, (void *)pObject->dwId, "Deleting object...");
   if (dwResult != RCC_SUCCESS)
      ErrorBox(dwResult, "Unable to delete object: %s");
}


//
// Perform forced poll on a node
//

void CConsoleApp::PollNode(DWORD dwObjectId, int iPollType)
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);
   CNodePoller *pWnd;

	pWnd = (CNodePoller *)pFrame->CreateNewChild(
		RUNTIME_CLASS(CNodePoller), IDR_NODE_POLLER, m_hMDIMenu, m_hMDIAccel);
   if (pWnd != NULL)
   {
      pWnd->m_dwObjectId = dwObjectId;
      pWnd->m_iPollType = iPollType;
      pWnd->PostMessage(WM_COMMAND, ID_POLL_RESTART, 0);
   }
}


//
// WM_COMMAND::ID_TOOLS_ADDNODE message handler
//

void CConsoleApp::OnToolsAddnode() 
{
   CreateNode(0);
}


//
// Wake up node
//

void CConsoleApp::WakeUpNode(DWORD dwObjectId)
{
   DWORD dwResult;

   dwResult = DoRequestArg1(NXCWakeUpNode, (void *)dwObjectId, _T("Sending Wake-On-LAN magic packet to node..."));
   if (dwResult != RCC_SUCCESS)
      ErrorBox(dwResult, _T("Unable to send WOL magic packet: %s"));
   else
      m_pMainWnd->MessageBox(_T("Wake-On-LAN magic packet was successfully sent to node"),
                             _T("Information"), MB_ICONINFORMATION | MB_OK);
}
