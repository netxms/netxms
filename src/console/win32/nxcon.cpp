// nxcon.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "nxcon.h"

#include "MainFrm.h"
#include "LoginDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


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
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//
// CConsoleApp construction
//

CConsoleApp::CConsoleApp()
{
   m_bCtrlPanelActive = FALSE;
   m_bEventBrowserActive = FALSE;
   m_bEventEditorActive = FALSE;
   m_bObjectBrowserActive = FALSE;
   m_bDebugWindowActive = FALSE;
   m_dwClientState = STATE_DISCONNECTED;
   m_pRqWaitList = NULL;
   m_dwRqWaitListSize = 0;
   m_mutexRqWaitList = MutexCreate();
}


//
// CConsoleApp destruction
//

CConsoleApp::~CConsoleApp()
{
   if (m_pRqWaitList != NULL)
      MemFree(m_pRqWaitList);
   MutexDestroy(m_mutexRqWaitList);
}


//
// The one and only CConsoleApp object
//

CConsoleApp theApp;


//
// CConsoleApp initialization
//

BOOL CConsoleApp::InitInstance()
{
   BOOL bSetWindowPos;
   DWORD dwBytes;
   BYTE *pData;
   HMENU hMenu;

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

	// To create the main window, this code creates a new frame window
	// object and then sets it as the application's main window object.

	CMDIFrameWnd* pFrame = new CMainFrame;
	m_pMainWnd = pFrame;

	// create main MDI frame window
	if (!pFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;

	// Load shared MDI menus and accelerator table
	HINSTANCE hInstance = AfxGetResourceHandle();

	hMenu  = ::LoadMenu(hInstance, MAKEINTRESOURCE(IDM_VIEW_SPECIFIC));

   m_hMDIMenu = ::LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MAINFRAME));
   InsertMenu(m_hMDIMenu, 3, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), "&Window");

   m_hEventBrowserMenu = ::LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MAINFRAME));
   InsertMenu(m_hEventBrowserMenu, 3, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), "&Window");
   InsertMenu(m_hEventBrowserMenu, 3, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 1), "&Event");

   m_hObjectBrowserMenu = ::LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MAINFRAME));
   InsertMenu(m_hObjectBrowserMenu, 3, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), "&Window");
   InsertMenu(m_hObjectBrowserMenu, 3, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 2), "&Object");

	m_hMDIAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_MDI_DEFAULT));
	m_hEventBrowserAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_MDI_DEFAULT));
	m_hObjectBrowserAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_OBJECT_BROWSER));

   // Load configuration from registry
   dwBytes = sizeof(WINDOWPLACEMENT);
   bSetWindowPos = GetProfileBinary(_T("General"), _T("WindowPlacement"), 
                                    &pData, (UINT *)&dwBytes);
   strcpy(g_szServer, (LPCTSTR)GetProfileString(_T("Connection"), _T("Server"), _T("localhost")));
   strcpy(g_szLogin, (LPCTSTR)GetProfileString(_T("Connection"), _T("Login"), NULL));

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
   pFrame->PostMessage(WM_COMMAND, ID_CONNECT_TO_SERVER, 0);

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CConsoleApp message handlers

int CConsoleApp::ExitInstance() 
{
   NXCSetDebugCallback(NULL);
   NXCDisconnect();

   // Free resources
   SafeFreeResource(m_hMDIMenu);
   SafeFreeResource(m_hMDIAccel);
   SafeFreeResource(m_hEventBrowserMenu);
   SafeFreeResource(m_hEventBrowserAccel);

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

void CConsoleApp::OnViewCreate(DWORD dwView, CWnd *pWnd)
{
   switch(dwView)
   {
      case IDR_CTRLPANEL:
         m_bCtrlPanelActive = TRUE;
         m_pwndCtrlPanel = pWnd;
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
      case IDR_DEBUG_WINDOW:
         m_bDebugWindowActive = TRUE;
         m_pwndDebugWindow = (CDebugFrame *)pWnd;
         NXCSetDebugCallback(ClientDebugCallback);
         break;
      default:
         break;
   }
}


//
// This method called when view is destroyed
//

void CConsoleApp::OnViewDestroy(DWORD dwView, CWnd *pWnd)
{
   switch(dwView)
   {
      case IDR_CTRLPANEL:
         m_bCtrlPanelActive = FALSE;
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
      case IDR_DEBUG_WINDOW:
         m_bDebugWindowActive = FALSE;
         NXCSetDebugCallback(NULL);
         break;
      default:
         break;
   }
}

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

void CConsoleApp::OnViewMap() 
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);

	// create a new MDI child window
	pFrame->CreateNewChild(RUNTIME_CLASS(CMapFrame), IDR_MAPFRAME, m_hMDIMenu, m_hMDIAccel);
}

void CConsoleApp::OnConnectToServer() 
{
	CLoginDialog dlgLogin;

   dlgLogin.m_szServer = g_szServer;
   dlgLogin.m_szLogin = g_szLogin;
   while(1)
   {
      if (dlgLogin.DoModal() != IDOK)
      {
         PostQuitMessage(1);
         break;
      }
      strcpy(g_szServer, (LPCTSTR)dlgLogin.m_szServer);
      strcpy(g_szLogin, (LPCTSTR)dlgLogin.m_szLogin);
      strcpy(g_szPassword, (LPCTSTR)dlgLogin.m_szPassword);

      // Save last connection parameters
      WriteProfileString(_T("Connection"), _T("Server"), g_szServer);
      WriteProfileString(_T("Connection"), _T("Login"), g_szLogin);

      // Initiate connection
      m_bAuthFailed = FALSE;
      if (NXCConnect(g_szServer, g_szLogin, g_szPassword))
      {
         m_dlgProgress.m_szStatusText = "Connecting to server...";
         if (m_dlgProgress.DoModal() == IDOK)
         {
            // Now we are connected, request data sync
            NXCSyncObjects();
            break;
         }
         else
            m_pMainWnd->MessageBox(m_bAuthFailed ? "Client authentication failed (invalid login name or password)" :
                                                   "Unable to establish connection with the server", 
                                   "Connection error", MB_OK | MB_ICONSTOP);
      }
      else
      {
         m_pMainWnd->MessageBox("Unable to initiate connection to server", 
                                "Connection error", MB_OK | MB_ICONSTOP);
      }
   }
}

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
   static BOOL bNowConnecting = FALSE;
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);

   switch(dwEvent)
   {
      case NXC_EVENT_STATE_CHANGED:
         switch(dwCode)
         {
            case STATE_CONNECTING:
               bNowConnecting = TRUE;
               break;
            case STATE_IDLE:
               if (bNowConnecting)
               {
                  m_dlgProgress.Terminate(IDOK);
                  bNowConnecting = FALSE;
               }
               if ((m_bEventBrowserActive) && (m_dwClientState == STATE_SYNC_EVENTS))
                  m_pwndEventBrowser->EnableDisplay(TRUE);
               break;
            case STATE_DISCONNECTED:
               if (bNowConnecting)
               {
                  m_dlgProgress.Terminate(IDCANCEL);
                  bNowConnecting = FALSE;
               }
               break;
            case STATE_SYNC_EVENTS:
               if (m_bEventBrowserActive)
                  m_pwndEventBrowser->EnableDisplay(FALSE);
               break;
            default:
               break;
         }
         m_dwClientState = dwCode;
         break;
      case NXC_EVENT_LOGIN_RESULT:
         if (dwCode == 0)
            m_bAuthFailed = TRUE;
         break;
      case NXC_EVENT_NEW_ELOG_RECORD:
         if (m_bEventBrowserActive)
            m_pwndEventBrowser->AddEvent((NXC_EVENT *)pArg);
         MemFree(pArg);
//         Sleep(0);
         break;
      case NXC_EVENT_REQUEST_COMPLETED:
         OnRequestComplete(dwCode, (DWORD)pArg);
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
	   pFrame->CreateNewChild(
		   RUNTIME_CLASS(CEventEditor), IDR_OBJECTS, m_hMDIMenu, m_hMDIAccel);	
}


//
// Register callback for request completion
//

void CConsoleApp::RegisterRequest(HREQUEST hRequest, CWnd *pWnd)
{
   m_pRqWaitList = (RQ_WAIT_INFO *)MemReAlloc(m_pRqWaitList, sizeof(RQ_WAIT_INFO) * (m_dwRqWaitListSize + 1));
   m_pRqWaitList[m_dwRqWaitListSize].hRequest = hRequest;
   m_pRqWaitList[m_dwRqWaitListSize].pWnd = pWnd;
   m_dwRqWaitListSize++;
}


//
// This method called by client library event handler on NXC_EVENT_REQUEST_COMPLETE
//

void CConsoleApp::OnRequestComplete(HREQUEST hRequest, DWORD dwRetCode)
{
   DWORD i;

   for(i = 0; i < m_dwRqWaitListSize; i++)
      if (m_pRqWaitList[i].hRequest == hRequest)
      {
         m_pRqWaitList[i].pWnd->SendMessage(WM_REQUEST_COMPLETED, hRequest, dwRetCode);
         m_dwRqWaitListSize--;
         memmove(&m_pRqWaitList[i], &m_pRqWaitList[i + 1], sizeof(RQ_WAIT_INFO) * (m_dwRqWaitListSize - i));
         i--;
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
