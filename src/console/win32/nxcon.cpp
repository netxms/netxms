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
// Constants
//

#define LAST_APP_MENU   4


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

   m_hMDIMenu = ::LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MAINFRAME));
   InsertMenu(m_hMDIMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), "&Window");

   m_hEventBrowserMenu = ::LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MAINFRAME));
   InsertMenu(m_hEventBrowserMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), "&Window");
   InsertMenu(m_hEventBrowserMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 1), "&Event");

   m_hObjectBrowserMenu = ::LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MAINFRAME));
   InsertMenu(m_hObjectBrowserMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), "&Window");
   InsertMenu(m_hObjectBrowserMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 2), "&Object");

   m_hUserEditorMenu = ::LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MAINFRAME));
   InsertMenu(m_hUserEditorMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), "&Window");
   InsertMenu(m_hUserEditorMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 3), "&User");

   m_hDCEditorMenu = ::LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MAINFRAME));
   InsertMenu(m_hDCEditorMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), "&Window");
   InsertMenu(m_hDCEditorMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 4), "&Item");

	m_hMDIAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_MDI_DEFAULT));
	m_hEventBrowserAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_MDI_DEFAULT));
	m_hObjectBrowserAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_OBJECT_BROWSER));
	m_hUserEditorAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_MDI_DEFAULT));
	m_hDCEditorAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_MDI_DEFAULT));

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

   // Free resources
   SafeFreeResource(m_hMDIMenu);
   SafeFreeResource(m_hMDIAccel);
   SafeFreeResource(m_hEventBrowserMenu);
   SafeFreeResource(m_hEventBrowserAccel);
   SafeFreeResource(m_hUserEditorMenu);
   SafeFreeResource(m_hUserEditorAccel);
   SafeFreeResource(m_hDCEditorMenu);
   SafeFreeResource(m_hDCEditorAccel);

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

void CConsoleApp::OnViewCreate(DWORD dwView, CWnd *pWnd, DWORD dwArg)
{
   DWORD i;

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
      case IDR_EVENTS:
         m_bEventBrowserActive = FALSE;
         break;
      case IDR_OBJECTS:
         m_bObjectBrowserActive = FALSE;
         break;
      case IDR_EVENT_EDITOR:
         m_bEventEditorActive = FALSE;
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
	pFrame->CreateNewChild(RUNTIME_CLASS(CMapFrame), IDR_MAPFRAME, m_hMDIMenu, m_hMDIAccel);
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

      // Save last connection parameters
      WriteProfileString(_T("Connection"), _T("Server"), g_szServer);
      WriteProfileString(_T("Connection"), _T("Login"), g_szLogin);

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
         switch(dwCode)
         {
            case STATE_CONNECTING:
               break;
            case STATE_IDLE:
               if ((m_bEventBrowserActive) && (m_dwClientState == STATE_SYNC_EVENTS))
                  m_pwndEventBrowser->EnableDisplay(TRUE);
               break;
            case STATE_DISCONNECTED:
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
      case NXC_EVENT_NEW_ELOG_RECORD:
         if (m_bEventBrowserActive)
            m_pwndEventBrowser->AddEvent((NXC_EVENT *)pArg);
         MemFree(pArg);
         break;
      case NXC_EVENT_OBJECT_CHANGED:
         ((CMainFrame *)m_pMainWnd)->PostMessage(WM_OBJECT_CHANGE, dwCode, (LPARAM)pArg);
         break;
      case NXC_EVENT_USER_DB_CHANGED:
         ((CMainFrame *)m_pMainWnd)->PostMessage(WM_USERDB_CHANGE, dwCode, (LPARAM)pArg);
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

      dwResult = DoRequest(NXCOpenEventDB, "Opening event configuration database...");
      if (dwResult == RCC_SUCCESS)
      {
	      pFrame->CreateNewChild(
		      RUNTIME_CLASS(CEventEditor), IDR_EVENT_EDITOR, m_hMDIMenu, m_hMDIAccel);
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

void CConsoleApp::ErrorBox(DWORD dwError, char *pszMessage, char *pszTitle)
{
   char szBuffer[512];

   sprintf(szBuffer, (pszMessage != NULL) ? pszMessage : "Error: %s", 
           NXCGetErrorText(dwError));
   m_pMainWnd->MessageBox(szBuffer, (pszTitle != NULL) ? pszTitle : "Error", MB_ICONSTOP);
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
		      RUNTIME_CLASS(CEventPolicyEditor), IDR_EPP_EDITOR, m_hMDIMenu, m_hMDIAccel);
      }
      else
      {
         ErrorBox(dwResult, "Unable to load event processing policy:\n%s");
      }
   }
}
