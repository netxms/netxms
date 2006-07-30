// nxcon.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "nxcon.h"
#include "MainFrm.h"
#include "ActionEditor.h"
#include "ConsolePropsGeneral.h"
#include "ControlPanel.h"
#include "CreateCondDlg.h"
#include "CreateContainerDlg.h"
#include "CreateNodeDlg.h"
#include "CreateTemplateDlg.h"
#include "CreateTGDlg.h"
#include "CreateNetSrvDlg.h"
#include "CreateVPNConnDlg.h"
#include "EventBrowser.h"
#include "EventEditor.h"
#include "EventPolicyEditor.h"
#include "NodePoller.h"
#include "MIBBrowserDlg.h"
#include "NetSummaryFrame.h"
#include "NetSrvPropsGeneral.h"
#include "NodePropsPolling.h"
#include "DeploymentView.h"
#include "LastValuesView.h"
#include "ObjectPropsRelations.h"
#include "VPNCPropsGeneral.h"
#include "RemoveTemplateDlg.h"
#include "AddrChangeDlg.h"
#include "AgentCfgEditor.h"
#include "TableView.h"
#include "WebBrowser.h"
#include "ObjectPropsStatus.h"
#include "ServerCfgEditor.h"
#include "ScriptManager.h"
#include "TrapEditor.h"
#include "TrapLogBrowser.h"
#include "GraphFrame.h"
#include "SyslogBrowser.h"
#include "MapFrame.h"
#include "UserEditor.h"
#include "ObjectToolsEditor.h"
#include "ObjectPropSheet.h"
#include "NodePropsGeneral.h"
#include "ObjectPropsGeneral.h"
#include "ObjectPropsSecurity.h"
#include "ObjectPropsPresentation.h"
#include "ObjectPropCaps.h"
#include "DCIDataView.h"
#include "LPPList.h"
#include "ObjectBrowser.h"
#include "ViewEditor.h"
#include "PackageMgr.h"
#include "ModuleManager.h"
#include "DesktopManager.h"
#include "CondPropsGeneral.h"
#include "CondPropsData.h"
#include "CondPropsScript.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


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
	ON_COMMAND(ID_CONTROLPANEL_AGENTPKG, OnControlpanelAgentpkg)
	ON_COMMAND(ID_CONTROLPANEL_SERVERCFG, OnControlpanelServercfg)
	ON_COMMAND(ID_VIEW_SYSLOG, OnViewSyslog)
	ON_COMMAND(ID_CONTROLPANEL_LOGPROCESSING, OnControlpanelLogprocessing)
	ON_COMMAND(ID_CONTROLPANEL_OBJECTTOOLS, OnControlpanelObjecttools)
	ON_COMMAND(ID_CONTROLPANEL_SCRIPTLIBRARY, OnControlpanelScriptlibrary)
	ON_COMMAND(ID_VIEW_SNMPTRAPLOG, OnViewSnmptraplog)
	ON_COMMAND(ID_CONTROLPANEL_VIEWBUILDER, OnControlpanelViewbuilder)
	ON_COMMAND(ID_CONTROLPANEL_MODULES, OnControlpanelModules)
	ON_COMMAND(ID_DESKTOP_MANAGE, OnDesktopManage)
	ON_COMMAND(ID_TOOLS_CHANGEPASSWORD, OnToolsChangepassword)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//
// CConsoleApp construction
//

CConsoleApp::CConsoleApp()
{
   m_dwClientState = STATE_DISCONNECTED;
   memset(m_openDCEditors, 0, sizeof(DC_EDITOR) * MAX_DC_EDITORS);
   memset(m_viewState, 0, sizeof(CONSOLE_VIEW) * MAX_VIEW_ID);
   m_bIgnoreErrors = FALSE;
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

   // Create background image cache directory
   strcpy(&g_szWorkDir[iLastChar], WORKDIR_BKIMAGECACHE);
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

	if (!ScintillaInit())
	{
		AfxMessageBox(IDS_SCINTILLA_INIT_FAILED, MB_OK | MB_ICONSTOP);
		return FALSE;
	}

   if (!NXCInitialize())
   {
		AfxMessageBox(IDS_NXC_INIT_FAILED, MB_OK | MB_ICONSTOP);
		return FALSE;
   }

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

   // Initialize speach engine
   SpeakerInit();

	// Change the registry key under which our settings are stored.
	SetRegistryKey(_T("NetXMS"));

   // Load configuration from registry
   dwBytes = sizeof(WINDOWPLACEMENT);
   bSetWindowPos = GetProfileBinary(_T("General"), _T("WindowPlacement"), 
                                    &pData, (UINT *)&dwBytes);
   g_dwOptions = GetProfileInt(_T("General"), _T("Options"), g_dwOptions);
   strcpy(g_szServer, (LPCTSTR)GetProfileString(_T("Connection"), _T("Server"), _T("localhost")));
   strcpy(g_szLogin, (LPCTSTR)GetProfileString(_T("Connection"), _T("Login"), NULL));
   LoadAlarmSoundCfg(&g_soundCfg, NXCON_ALARM_SOUND_KEY);

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

   // Create global fonts
   g_fontCode.CreateFont(-MulDiv(10, GetDeviceCaps(GetDC(m_pMainWnd->m_hWnd), LOGPIXELSY), 72),
                         0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                         OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                         FIXED_PITCH | FF_DONTCARE, "Courier");
	
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
   InsertMenu(m_hMapMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 21), "&Map");
   InsertMenu(m_hMapMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 9), "&Object");

   m_hEventEditorMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hEventEditorMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), "&Window");
   InsertMenu(m_hEventEditorMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 10), "&Event");

   m_hActionEditorMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hActionEditorMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), "&Window");
   InsertMenu(m_hActionEditorMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 11), "&Action");

   m_hTrapEditorMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hTrapEditorMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), "&Window");
   InsertMenu(m_hTrapEditorMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 12), "T&rap");

   m_hGraphMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hGraphMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), "&Window");
   InsertMenu(m_hGraphMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 13), "&Graph");

   m_hPackageMgrMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hPackageMgrMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), "&Window");
   InsertMenu(m_hPackageMgrMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 14), "&Package");

   m_hLastValuesMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hLastValuesMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), "&Window");
   InsertMenu(m_hLastValuesMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 15), "&Item");

   m_hServerCfgEditorMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hServerCfgEditorMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), "&Window");
   InsertMenu(m_hServerCfgEditorMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 16), "V&ariable");

   m_hAgentCfgEditorMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hAgentCfgEditorMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), "&Window");
   InsertMenu(m_hAgentCfgEditorMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 17), "&Edit");
   InsertMenu(m_hAgentCfgEditorMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 18), "&Config");

   m_hLPPEditorMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hDCEditorMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), "&Window");
   //InsertMenu(m_hDCEditorMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 4), "&Item");

   m_hObjToolsEditorMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hObjToolsEditorMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), "&Window");
   InsertMenu(m_hObjToolsEditorMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 19), "&Object tools");

   m_hScriptManagerMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hScriptManagerMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), "&Window");
   InsertMenu(m_hScriptManagerMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 17), "&Edit");
   InsertMenu(m_hScriptManagerMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 20), "&Script");

	m_hMDIAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_MDI_DEFAULT));
	m_hAlarmBrowserAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_ALARM_BROWSER));
	m_hEventBrowserAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_MDI_DEFAULT));
	m_hObjectBrowserAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_OBJECT_BROWSER));
	m_hEventEditorAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_EVENT_EDITOR));
	m_hUserEditorAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_MDI_DEFAULT));
	m_hDCEditorAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_DC_EDITOR));
	m_hPolicyEditorAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_EPP));
	m_hMapAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_NETMAP));
	m_hActionEditorAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_ACTION_EDITOR));
	m_hTrapEditorAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_TRAP_EDITOR));
	m_hGraphAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_GRAPH));
	m_hPackageMgrAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_PACKAGE_MGR));
	m_hLastValuesAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_LAST_VALUES));
	m_hServerCfgEditorAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_SERVER_CFG_EDITOR));
	m_hAgentCfgEditorAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_AGENT_CFG_EDITOR));
	m_hLPPEditorAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_MDI_DEFAULT));
	m_hObjToolsEditorAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_OBJECT_TOOLS_EDITOR));
	m_hScriptManagerAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_SCRIPT_MANAGER));

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
   TCHAR szBuffer[MAX_PATH + 32];
   BYTE bsServerId[8];

   if (g_hSession != NULL)
   {
      if (!(g_dwOptions & OPT_DONT_CACHE_OBJECTS))
      {
         NXCGetServerID(g_hSession, bsServerId);
         _tcscpy(szBuffer, g_szWorkDir);
         _tcscat(szBuffer, WORKFILE_OBJECTCACHE);
         BinToStr(bsServerId, 8, &szBuffer[_tcslen(szBuffer)]);
         _tcscat(szBuffer, _T("."));
         _tcscat(szBuffer, g_szLogin);
         NXCSaveObjectCache(g_hSession, szBuffer);
      }

      NXCSetDebugCallback(NULL);
      NXCDisconnect(g_hSession);
      NXCShutdown();
      NXCDestroyCCList(g_pCCList);
   }

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
   SafeFreeResource(m_hEventEditorMenu);
   SafeFreeResource(m_hEventEditorAccel);
   SafeFreeResource(m_hActionEditorMenu);
   SafeFreeResource(m_hActionEditorAccel);
   SafeFreeResource(m_hTrapEditorMenu);
   SafeFreeResource(m_hTrapEditorAccel);
   SafeFreeResource(m_hGraphMenu);
   SafeFreeResource(m_hGraphAccel);
   SafeFreeResource(m_hPackageMgrMenu);
   SafeFreeResource(m_hPackageMgrAccel);
   SafeFreeResource(m_hLastValuesMenu);
   SafeFreeResource(m_hLastValuesAccel);
   SafeFreeResource(m_hServerCfgEditorMenu);
   SafeFreeResource(m_hServerCfgEditorAccel);
   SafeFreeResource(m_hAgentCfgEditorMenu);
   SafeFreeResource(m_hAgentCfgEditorAccel);
   SafeFreeResource(m_hLPPEditorMenu);
   SafeFreeResource(m_hLPPEditorAccel);
   SafeFreeResource(m_hScriptManagerMenu);
   SafeFreeResource(m_hScriptManagerAccel);

   CloseHandle(g_mutexActionListAccess);

   SpeakerShutdown();

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
   ShowControlPanel();
}


//
// Create new control panel window or open existing
//

CMDIChildWnd *CConsoleApp::ShowControlPanel(void) 
{
	CMainFrame *pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);
   CMDIChildWnd *pWnd;

	// create a new MDI child window or open existing
   if (m_viewState[VIEW_CTRLPANEL].bActive)
   {
      m_viewState[VIEW_CTRLPANEL].pWnd->BringWindowToTop();
      pWnd = m_viewState[VIEW_CTRLPANEL].pWnd;
   }
   else
   {
	   pWnd = pFrame->CreateNewChild(RUNTIME_CLASS(CControlPanel), IDR_CTRLPANEL,
                                    m_hMDIMenu, m_hMDIAccel);
   }
   return pWnd;
}


//
// This method called when new view is created
//

void CConsoleApp::OnViewCreate(DWORD dwView, CMDIChildWnd *pWnd, DWORD dwArg)
{
   DWORD i;

   if (dwView == IDR_DC_EDITOR)
   {
      // Register new DC editor
      for(i = 0; i < MAX_DC_EDITORS; i++)
         if (m_openDCEditors[i].pWnd == NULL)
         {
            m_openDCEditors[i].pWnd = pWnd;
            m_openDCEditors[i].dwNodeId = dwArg;
            break;
         }
   }
   else
   {
      // Regitesr view
      if (dwView < MAX_VIEW_ID)
      {
         m_viewState[dwView].bActive = TRUE;
         m_viewState[dwView].pWnd = pWnd;
         m_viewState[dwView].hWnd = pWnd->m_hWnd;
      }

      // Some view-specific processing
      switch(dwView)
      {
         case VIEW_DEBUG:
            NXCSetDebugCallback(ClientDebugCallback);
            break;
         default:
            break;
      }
   }
}


//
// This method called when view is destroyed
//

void CConsoleApp::OnViewDestroy(DWORD dwView, CMDIChildWnd *pWnd, DWORD dwArg)
{
   DWORD i;

   if (dwView == IDR_DC_EDITOR)
   {
      // Unregister DC editor
      for(i = 0; i < MAX_DC_EDITORS; i++)
         if (m_openDCEditors[i].pWnd == pWnd)
         {
            m_openDCEditors[i].pWnd = NULL;
            m_openDCEditors[i].dwNodeId = 0;
            break;
         }
   }
   else
   {
      // Unregister view
      if (dwView < MAX_VIEW_ID)
      {
         m_viewState[dwView].bActive = FALSE;
         m_viewState[dwView].pWnd = NULL;
         m_viewState[dwView].hWnd = NULL;
      }

      // Some view-specific processing
      switch(dwView)
      {
         case VIEW_DEBUG:
            NXCSetDebugCallback(NULL);
            break;
         default:
            break;
      }
   }
}


//
// Show event browser window
//

CMDIChildWnd *CConsoleApp::ShowEventBrowser(void)
{
	CMainFrame *pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);
   CMDIChildWnd *pWnd;

	// create a new MDI child window or open existing
   if (m_viewState[VIEW_EVENT_LOG].bActive)
   {
      m_viewState[VIEW_EVENT_LOG].pWnd->BringWindowToTop();
      pWnd = m_viewState[VIEW_EVENT_LOG].pWnd;
   }
   else
   {
	   pWnd = pFrame->CreateNewChild(RUNTIME_CLASS(CEventBrowser), IDR_EVENTS,
                                    m_hEventBrowserMenu, m_hEventBrowserAccel);
   }
   return pWnd;
}


//
// Show syslog browser window
//

CMDIChildWnd *CConsoleApp::ShowSyslogBrowser(void)
{
	CMainFrame *pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);
   CMDIChildWnd *pWnd;

	// create a new MDI child window or open existing
   if (m_viewState[VIEW_SYSLOG].bActive)
   {
      m_viewState[VIEW_SYSLOG].pWnd->BringWindowToTop();
      pWnd = m_viewState[VIEW_SYSLOG].pWnd;
   }
   else
   {
	   pWnd = pFrame->CreateNewChild(RUNTIME_CLASS(CSyslogBrowser), IDR_SYSLOG_BROWSER,
                                    m_hMDIMenu, m_hMDIAccel);
   }
   return pWnd;
}


//
// Show syslog browser window
//

CMDIChildWnd *CConsoleApp::ShowTrapLogBrowser(void)
{
	CMainFrame *pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);
   CMDIChildWnd *pWnd;

	// create a new MDI child window or open existing
   if (m_viewState[VIEW_TRAP_LOG].bActive)
   {
      m_viewState[VIEW_TRAP_LOG].pWnd->BringWindowToTop();
      pWnd = m_viewState[VIEW_TRAP_LOG].pWnd;
   }
   else
   {
	   pWnd = pFrame->CreateNewChild(RUNTIME_CLASS(CTrapLogBrowser), IDR_TRAP_LOG_BROWSER,
                                    m_hMDIMenu, m_hMDIAccel);
   }
   return pWnd;
}


//
// WM_COMMAND::ID_VIEW_EVENTS message handler
//

void CConsoleApp::OnViewEvents() 
{
   ShowEventBrowser();
}


//
// WM_COMMAND::ID_VIEW_SYSLOG message handler
//

void CConsoleApp::OnViewSyslog() 
{
   ShowSyslogBrowser();
}


//
// WM_COMMAND::ID_VIEW_SNMPTRAPLOG message handler
//

void CConsoleApp::OnViewSnmptraplog() 
{
   ShowTrapLogBrowser();
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
   dlgLogin.m_bEncrypt = (g_dwOptions & OPT_ENCRYPT_CONNECTION) ? TRUE : FALSE;
   if (g_dwOptions & OPT_DONT_CACHE_OBJECTS)
   {
      dlgLogin.m_bNoCache = TRUE;
      dlgLogin.m_bClearCache = TRUE;
   }
   else
   {
      dlgLogin.m_bNoCache = FALSE;
      dlgLogin.m_bClearCache = (g_dwOptions & OPT_CLEAR_OBJECT_CACHE) ? TRUE : FALSE;
   }
   dlgLogin.m_bMatchVersion = (g_dwOptions & OPT_MATCH_SERVER_VERSION) ? TRUE : FALSE;
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
      if (dlgLogin.m_bEncrypt)
         g_dwOptions |= OPT_ENCRYPT_CONNECTION;
      else
         g_dwOptions &= ~OPT_ENCRYPT_CONNECTION;
      if (dlgLogin.m_bMatchVersion)
         g_dwOptions |= OPT_MATCH_SERVER_VERSION;
      else
         g_dwOptions &= ~OPT_MATCH_SERVER_VERSION;
      if (dlgLogin.m_bNoCache)
         g_dwOptions |= OPT_DONT_CACHE_OBJECTS;
      else
         g_dwOptions &= ~OPT_DONT_CACHE_OBJECTS;
      if (dlgLogin.m_bClearCache)
         g_dwOptions |= OPT_CLEAR_OBJECT_CACHE;
      else
         g_dwOptions &= ~OPT_CLEAR_OBJECT_CACHE;

      // Save last connection parameters
      WriteProfileString(_T("Connection"), _T("Server"), g_szServer);
      WriteProfileString(_T("Connection"), _T("Login"), g_szLogin);

      // Initiate connection
      dwResult = DoLogin(dlgLogin.m_bClearCache);
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
   ShowObjectBrowser();
}


//
// Event handler for client libray events
//

void CConsoleApp::EventHandler(DWORD dwEvent, DWORD dwCode, void *pArg)
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);
   NXC_DEPLOYMENT_STATUS *pStatus;

   switch(dwEvent)
   {
      case NXC_EVENT_CONNECTION_BROKEN:
         if (!m_bIgnoreErrors)
         {
            m_bIgnoreErrors = TRUE;
            m_pMainWnd->MessageBox(_T("Connection with the management server terminated unexpectedly"), _T("Error"), MB_OK | MB_ICONSTOP);
            m_pMainWnd->PostMessage(WM_CLOSE, 0, 0);
         }
         break;
      case NXC_EVENT_NEW_ELOG_RECORD:
         if (m_viewState[VIEW_EVENT_LOG].bActive)
         {
            void *pData;

            pData = nx_memdup(pArg, sizeof(NXC_EVENT));
            if (!PostMessage(m_viewState[VIEW_EVENT_LOG].hWnd, NXCM_NETXMS_EVENT, dwCode, (LPARAM)pData))
               free(pData);
         }
         break;
      case NXC_EVENT_NEW_SYSLOG_RECORD:
         if (m_viewState[VIEW_SYSLOG].bActive)
         {
            NXC_SYSLOG_RECORD *pRec;

            pRec = (NXC_SYSLOG_RECORD *)nx_memdup(pArg, sizeof(NXC_SYSLOG_RECORD));
            pRec->pszText = _tcsdup(((NXC_SYSLOG_RECORD *)pArg)->pszText);
            if (!PostMessage(m_viewState[VIEW_SYSLOG].hWnd, NXCM_SYSLOG_RECORD, dwCode, (LPARAM)pRec))
            {
               safe_free(pRec->pszText);
               free(pRec);
            }
         }
         break;
      case NXC_EVENT_NEW_SNMP_TRAP:
         if (m_viewState[VIEW_TRAP_LOG].bActive)
         {
            NXC_SNMP_TRAP_LOG_RECORD *pRec;

            pRec = (NXC_SNMP_TRAP_LOG_RECORD *)nx_memdup(pArg, sizeof(NXC_SNMP_TRAP_LOG_RECORD));
            pRec->pszTrapVarbinds = _tcsdup(((NXC_SNMP_TRAP_LOG_RECORD *)pArg)->pszTrapVarbinds);
            if (!PostMessage(m_viewState[VIEW_TRAP_LOG].hWnd, NXCM_TRAP_LOG_RECORD, dwCode, (LPARAM)pRec))
            {
               safe_free(pRec->pszTrapVarbinds);
               free(pRec);
            }
         }
         break;
      case NXC_EVENT_OBJECT_CHANGED:
         ((CMainFrame *)m_pMainWnd)->PostMessage(NXCM_OBJECT_CHANGE, dwCode, (LPARAM)pArg);
         break;
      case NXC_EVENT_USER_DB_CHANGED:
         ((CMainFrame *)m_pMainWnd)->PostMessage(NXCM_USERDB_CHANGE, dwCode, (LPARAM)pArg);
         break;
      case NXC_EVENT_DEPLOYMENT_STATUS:
         pStatus = (NXC_DEPLOYMENT_STATUS *)nx_memdup(pArg, sizeof(NXC_DEPLOYMENT_STATUS));
         pStatus->pszErrorMessage = _tcsdup(pStatus->pszErrorMessage);
         ((CMainFrame *)m_pMainWnd)->PostMessage(NXCM_DEPLOYMENT_INFO, dwCode, (LPARAM)pStatus);
         break;
      case NXC_EVENT_NOTIFICATION:
         switch(dwCode)
         {
            case NX_NOTIFY_SHUTDOWN:
               if (!m_bIgnoreErrors)
               {
                  m_bIgnoreErrors = TRUE;
                  m_pMainWnd->MessageBox(_T("Server was shutdown"), _T("Warning"), MB_OK | MB_ICONSTOP);
                  m_pMainWnd->PostMessage(WM_CLOSE, 0, 0);
               }
               break;
            case NX_NOTIFY_EVENTDB_CHANGED:
               m_pMainWnd->PostMessage(NXCM_UPDATE_EVENT_LIST);
               break;
            case NX_NOTIFY_OBJTOOLS_CHANGED:
               m_pMainWnd->PostMessage(NXCM_UPDATE_OBJECT_TOOLS);
               break;
            case NX_NOTIFY_NEW_ALARM:
            case NX_NOTIFY_ALARM_DELETED:
            case NX_NOTIFY_ALARM_ACKNOWLEGED:
               m_pMainWnd->PostMessage(NXCM_ALARM_UPDATE, dwCode, 
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
   if (m_viewState[VIEW_EVENT_EDITOR].bActive)
   {
      m_viewState[VIEW_EVENT_EDITOR].pWnd->BringWindowToTop();
   }
   else
   {
      DWORD dwResult;

      dwResult = DoRequestArg1(NXCLockEventDB, g_hSession, "Locking event configuration database...");
      if (dwResult == RCC_SUCCESS)
      {
	      pFrame->CreateNewChild(RUNTIME_CLASS(CEventEditor), IDR_EVENT_EDITOR,
                                m_hEventEditorMenu, m_hEventEditorAccel);
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
   if (m_viewState[VIEW_USER_MANAGER].bActive)
   {
      m_viewState[VIEW_USER_MANAGER].pWnd->BringWindowToTop();
   }
   else
   {
      DWORD dwResult;

      dwResult = DoRequestArg1(NXCLockUserDB, g_hSession, "Locking user database...");
      if (dwResult == RCC_SUCCESS)
      {
	      pFrame->CreateNewChild(RUNTIME_CLASS(CUserEditor), IDR_USER_EDITOR,
                                m_hUserEditorMenu, m_hUserEditorAccel);
      }
      else
      {
         ErrorBox(dwResult, "Unable to lock user database:\n%s");
      }
   }
}


//
// Handler for WM_COMMAND::ID_CONTROLPANEL_OBJECTTOOLS message
//

void CConsoleApp::OnControlpanelObjecttools() 
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);

	// create a new MDI child window or open existing
   if (m_viewState[VIEW_OBJECT_TOOLS].bActive)
   {
      m_viewState[VIEW_OBJECT_TOOLS].pWnd->BringWindowToTop();
   }
   else
   {
      DWORD dwResult;

      dwResult = DoRequestArg1(NXCLockObjectTools, g_hSession, _T("Locking object tools..."));
      if (dwResult == RCC_SUCCESS)
      {
	      pFrame->CreateNewChild(RUNTIME_CLASS(CObjectToolsEditor),
                                IDR_OBJECT_TOOLS_EDITOR,
                                m_hObjToolsEditorMenu, m_hObjToolsEditorAccel);
      }
      else
      {
         ErrorBox(dwResult, _T("Unable to lock object tools: %s"));
      }
   }
}


//
// Handler for WM_COMMAND::ID_CONTROLPANEL_SCRIPTLIBRARY message
//

void CConsoleApp::OnControlpanelScriptlibrary() 
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);

	// create a new MDI child window or open existing
   if (m_viewState[VIEW_SCRIPT_MANAGER].bActive)
   {
      m_viewState[VIEW_SCRIPT_MANAGER].pWnd->BringWindowToTop();
   }
   else
   {
	   pFrame->CreateNewChild(RUNTIME_CLASS(CScriptManager),
                             IDR_SCRIPT_MANAGER,
                             m_hScriptManagerMenu, m_hScriptManagerAccel);
   }
}


//
// Handler for WM_COMMAND::ID_VIEW_DEBUG message
//

void CConsoleApp::OnViewDebug() 
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);

	// create a new MDI child window or open existing
   if (m_viewState[VIEW_DEBUG].bActive)
   {
      m_viewState[VIEW_DEBUG].pWnd->BringWindowToTop();
   }
   else
   {
      pFrame->CreateNewChild(RUNTIME_CLASS(CDebugFrame), IDR_DEBUG_WINDOW,
                             m_hMDIMenu, m_hMDIAccel);
   }
}


//
// Print debug information
//

void CConsoleApp::DebugPrintf(char *szFormat, ...)
{
   if (m_viewState[VIEW_DEBUG].bActive)
   {
      char szBuffer[1024];
      va_list args;

      va_start(args, szFormat);
      vsprintf(szBuffer, szFormat, args);
      va_end(args);

      ((CDebugFrame *)m_viewState[VIEW_DEBUG].pWnd)->AddMessage(szBuffer);
   }
}


//
// Edit properties of specific object
//

void CConsoleApp::ObjectProperties(DWORD dwObjectId)
{
	CObjectPropSheet wndPropSheet("Object Properties", GetMainWnd(), 0);
   CNodePropsGeneral wndNodeGeneral;
   CNetSrvPropsGeneral wndNetSrvGeneral;
   CObjectPropsGeneral wndObjectGeneral;
   CObjectPropCaps wndObjectCaps;
   CObjectPropsSecurity wndObjectSecurity;
   CObjectPropsPresentation wndObjectPresentation;
   CObjectPropsRelations wndObjectRelations;
   CNodePropsPolling wndNodePolling;
   CVPNCPropsGeneral wndVPNCGeneral;
   CObjectPropsStatus wndStatus;
   CCondPropsGeneral wndCondGeneral;
   CCondPropsData wndCondData;
   CCondPropsScript wndCondScript;
   NXC_OBJECT *pObject;

   pObject = NXCFindObjectById(g_hSession, dwObjectId);
   if (pObject != NULL)
   {
      // Create "General" tab and class-specific tabs
      switch(pObject->iClass)
      {
         case OBJECT_NODE:
            wndNodeGeneral.m_dwObjectId = dwObjectId;
            wndNodeGeneral.m_strName = pObject->szName;
            wndNodeGeneral.m_strOID = pObject->node.szObjectId;
            wndNodeGeneral.m_dwIpAddr = pObject->dwIpAddr;
            wndNodeGeneral.m_iAgentPort = (int)pObject->node.wAgentPort;
            wndNodeGeneral.m_strCommunity = pObject->node.szCommunityString;
            wndNodeGeneral.m_iAuthType = pObject->node.wAuthMethod;
            wndNodeGeneral.m_strSecret = pObject->node.szSharedSecret;
            wndNodeGeneral.m_iSNMPVersion = (pObject->node.wSNMPVersion == SNMP_VERSION_1) ? 0 : 1;
            wndNodeGeneral.m_dwProxyNode = pObject->node.dwProxyNode;
            wndPropSheet.AddPage(&wndNodeGeneral);

            // Create "Polling" tab
            wndNodePolling.m_dwPollerNode = pObject->node.dwPollerNode;
            wndNodePolling.m_bDisableAgent = (pObject->node.dwFlags & NF_DISABLE_NXCP) ? TRUE : FALSE;
            wndNodePolling.m_bDisableICMP = (pObject->node.dwFlags & NF_DISABLE_ICMP) ? TRUE : FALSE;
            wndNodePolling.m_bDisableSNMP = (pObject->node.dwFlags & NF_DISABLE_SNMP) ? TRUE : FALSE;
            wndPropSheet.AddPage(&wndNodePolling);

            // Create "Capabilities" tab
            wndObjectCaps.m_pObject = pObject;
            wndPropSheet.AddPage(&wndObjectCaps);
            break;
         case OBJECT_NETWORKSERVICE:
            wndNetSrvGeneral.m_pObject = pObject;
            wndNetSrvGeneral.m_dwObjectId = dwObjectId;
            wndNetSrvGeneral.m_strName = pObject->szName;
            wndNetSrvGeneral.m_iServiceType = pObject->netsrv.iServiceType;
            wndNetSrvGeneral.m_iPort = pObject->netsrv.wPort;
            wndNetSrvGeneral.m_iProto = pObject->netsrv.wProto;
            wndNetSrvGeneral.m_strRequest = pObject->netsrv.pszRequest;
            wndNetSrvGeneral.m_strResponse = pObject->netsrv.pszResponse;
            wndNetSrvGeneral.m_dwIpAddr = pObject->dwIpAddr;
            wndNetSrvGeneral.m_dwPollerNode = pObject->netsrv.dwPollerNode;
            wndPropSheet.AddPage(&wndNetSrvGeneral);
            break;
         case OBJECT_VPNCONNECTOR:
            wndVPNCGeneral.m_pObject = pObject;
            wndVPNCGeneral.m_dwObjectId = dwObjectId;
            wndVPNCGeneral.m_strName = pObject->szName;
            wndVPNCGeneral.m_dwPeerGateway = pObject->vpnc.dwPeerGateway;
            wndPropSheet.AddPage(&wndVPNCGeneral);
            break;
         case OBJECT_CONDITION:
            wndCondGeneral.m_pObject = pObject;
            wndCondGeneral.m_dwObjectId = dwObjectId;
            wndCondGeneral.m_strName = pObject->szName;
            wndPropSheet.AddPage(&wndCondGeneral);

            wndCondData.m_pObject = pObject;
            wndPropSheet.AddPage(&wndCondData);

            wndCondScript.m_strScript = pObject->cond.pszScript;
            wndPropSheet.AddPage(&wndCondScript);
            break;
         default:
            wndObjectGeneral.m_dwObjectId = dwObjectId;
            wndObjectGeneral.m_strClass = g_szObjectClass[pObject->iClass];
            wndObjectGeneral.m_strName = pObject->szName;
            wndPropSheet.AddPage(&wndObjectGeneral);
            break;
      }

      // Create "Relations" tab
      wndObjectRelations.m_pObject = pObject;
      wndPropSheet.AddPage(&wndObjectRelations);

      // Create "Status Calculation" tab
      if ((pObject->iClass != OBJECT_TEMPLATEROOT) &&
          (pObject->iClass != OBJECT_TEMPLATEGROUP) &&
          (pObject->iClass != OBJECT_TEMPLATE))
      {
         wndStatus.m_iCalcAlg = pObject->iStatusCalcAlg;
         wndStatus.m_iPropAlg = pObject->iStatusPropAlg;
         wndStatus.m_iRelChange = pObject->iStatusShift;
         wndStatus.m_iFixedStatus = pObject->iFixedStatus;
         wndStatus.m_iStatusTranslation1 = pObject->iStatusTrans[0];
         wndStatus.m_iStatusTranslation2 = pObject->iStatusTrans[1];
         wndStatus.m_iStatusTranslation3 = pObject->iStatusTrans[2];
         wndStatus.m_iStatusTranslation4 = pObject->iStatusTrans[3];
         wndStatus.m_dSingleThreshold = (double)pObject->iStatusSingleTh / 100;
         wndStatus.m_dThreshold1 = (double)pObject->iStatusThresholds[0] / 100;
         wndStatus.m_dThreshold2 = (double)pObject->iStatusThresholds[1] / 100;
         wndStatus.m_dThreshold3 = (double)pObject->iStatusThresholds[2] / 100;
         wndStatus.m_dThreshold4 = (double)pObject->iStatusThresholds[3] / 100;
         wndPropSheet.AddPage(&wndStatus);
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
      dwResult = DoRequestArg3(NXCOpenNodeDCIList, g_hSession, (void *)pObject->dwId, 
                               &pItemList, "Loading node's data collection information...");
      if (dwResult == RCC_SUCCESS)
      {
         CreateChildFrameWithSubtitle(new CDataCollectionEditor(pItemList), IDR_DC_EDITOR,
                                      pObject->szName, m_hDCEditorMenu, m_hDCEditorAccel);
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
   ShowNetworkSummary();
}


//
// Set object's management status
//

void CConsoleApp::SetObjectMgmtStatus(NXC_OBJECT *pObject, BOOL bIsManaged)
{
   DWORD dwResult;

   dwResult = DoRequestArg3(NXCSetObjectMgmtStatus, g_hSession, (void *)pObject->dwId, 
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

   if (!m_bIgnoreErrors)
   {
      _sntprintf(szBuffer, 512, (pszMessage != NULL) ? pszMessage : _T("Error: %s"), 
                 NXCGetErrorText(dwError));
      m_pMainWnd->MessageBox(szBuffer, (pszTitle != NULL) ? pszTitle : _T("Error"), MB_ICONSTOP);
   }
}


//
// Show window with DCI's data
//

CMDIChildWnd *CConsoleApp::ShowDCIData(DWORD dwNodeId, DWORD dwItemId, char *pszItemName, TCHAR *pszParams)
{
   CDCIDataView *pWnd;

   if (pszParams == NULL)
   {
      pWnd = new CDCIDataView(dwNodeId, dwItemId, pszItemName);
   }
   else
   {
      pWnd = new CDCIDataView(pszParams);
   }
   CreateChildFrameWithSubtitle(pWnd, IDR_DCI_DATA_VIEW,
                                pWnd->GetItemName(), m_hMDIMenu, m_hMDIAccel);
   return pWnd;
}


//
// Show graph for collected data
//

CMDIChildWnd *CConsoleApp::ShowDCIGraph(DWORD dwNodeId, DWORD dwNumItems,
                                        NXC_DCI **ppItemList, TCHAR *pszItemName,
                                        TCHAR *pszParams)
{
   CGraphFrame *pWnd;
   DWORD i, dwCurrTime;

   pWnd = new CGraphFrame;
   if (pszParams == NULL)
   {
      for(i = 0; i < dwNumItems; i++)
         if (ppItemList[i] != NULL)
            pWnd->AddItem(dwNodeId, ppItemList[i]);
      dwCurrTime = time(NULL);
      pWnd->SetTimeFrame(dwCurrTime - 3600, dwCurrTime);    // Last hour
      pWnd->SetSubTitle(pszItemName);
   }
   else
   {
      // Restore desktop
      pWnd->RestoreFromServer(pszParams);
   }

   CreateChildFrameWithSubtitle(pWnd, IDR_DCI_HISTORY_GRAPH,
                                pWnd->GetSubTitle(), m_hGraphMenu, m_hGraphAccel);
   return pWnd;
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
	// create a new MDI child window or open existing
   if (m_viewState[VIEW_EPP_EDITOR].bActive)
   {
      m_viewState[VIEW_EPP_EDITOR].pWnd->BringWindowToTop();
   }
   else
   {
   	CMainFrame *pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);
      DWORD dwResult;

      dwResult = DoRequestArg2(NXCOpenEventPolicy, g_hSession, &m_pEventPolicy, 
                               _T("Loading event processing policy..."));
      if (dwResult == RCC_SUCCESS)
      {
	      pFrame->CreateNewChild(RUNTIME_CLASS(CEventPolicyEditor), IDR_EPP_EDITOR,
                                m_hPolicyEditorMenu, m_hPolicyEditorAccel);
      }
      else
      {
         ErrorBox(dwResult, "Unable to load event processing policy:\n%s");
      }
   }
}


//
// Show log processing policy list
//

void CConsoleApp::OnControlpanelLogprocessing() 
{
	// create a new MDI child window or open existing
   if (m_viewState[VIEW_LPP_EDITOR].bActive)
   {
      m_viewState[VIEW_LPP_EDITOR].pWnd->BringWindowToTop();
   }
   else
   {
   	CMainFrame *pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);

	   pFrame->CreateNewChild(RUNTIME_CLASS(CLPPList), IDR_LPP_EDITOR,
                             m_hLPPEditorMenu, m_hLPPEditorAccel);
   }
}


//
// Open view builder
//

void CConsoleApp::OnControlpanelViewbuilder() 
{
	// create a new MDI child window or open existing
   if (m_viewState[VIEW_BUILDER].bActive)
   {
      m_viewState[VIEW_BUILDER].pWnd->BringWindowToTop();
   }
   else
   {
   	CMainFrame *pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);

	   pFrame->CreateNewChild(RUNTIME_CLASS(CViewEditor), IDR_VIEW_BUILDER,
                             m_hViewBuilderMenu, m_hViewBuilderAccel);
   }
}


//
// Show alarm browser window
//

CMDIChildWnd *CConsoleApp::ShowAlarmBrowser(TCHAR *pszParams)
{
   CMDIChildWnd *pWnd;

	// create a new MDI child window or open existing
   if (m_viewState[VIEW_ALARMS].bActive)
   {
      m_viewState[VIEW_ALARMS].pWnd->BringWindowToTop();
      pWnd = m_viewState[VIEW_ALARMS].pWnd;
   }
   else
   {
   	CMainFrame *pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);

      if (pszParams == NULL)
      {
	      pWnd = pFrame->CreateNewChild(RUNTIME_CLASS(CAlarmBrowser), IDR_ALARMS,
                                       m_hAlarmBrowserMenu, m_hAlarmBrowserAccel);
      }
      else
      {
         pWnd = new CAlarmBrowser(pszParams);
         CreateChildFrameWithSubtitle(pWnd, IDR_ALARMS, NULL, m_hAlarmBrowserMenu, m_hAlarmBrowserAccel);
      }
   }
   return pWnd;
}


//
// WM_COMMAND::ID_VIEW_ALARMS message handler
//

void CConsoleApp::OnViewAlarms() 
{
   ShowAlarmBrowser();
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
   if (m_viewState[VIEW_ACTION_EDITOR].bActive)
   {
      m_viewState[VIEW_ACTION_EDITOR].pWnd->BringWindowToTop();
   }
   else
   {
      DWORD dwResult;

      dwResult = DoRequestArg1(NXCLockActionDB, g_hSession, "Locking action configuration database...");
      if (dwResult == RCC_SUCCESS)
      {
	      pFrame->CreateNewChild(RUNTIME_CLASS(CActionEditor), IDR_ACTION_EDITOR,
                                m_hActionEditorMenu, m_hActionEditorAccel);
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
   if (m_viewState[VIEW_TRAP_EDITOR].bActive)
   {
      m_viewState[VIEW_TRAP_EDITOR].pWnd->BringWindowToTop();
   }
   else
   {
      DWORD dwResult;

      dwResult = DoRequestArg1(NXCLockTrapCfg, g_hSession, "Locking SNMP trap configuration database...");
      if (dwResult == RCC_SUCCESS)
      {
	      pFrame->CreateNewChild(RUNTIME_CLASS(CTrapEditor), IDR_TRAP_EDITOR,
                                m_hTrapEditorMenu, m_hTrapEditorAccel);
      }
      else
      {
         ErrorBox(dwResult, "Unable to lock SNMP trap configuration database:\n%s");
      }
   }
}


//
// WM_COMMAND::ID_CONTROLPANEL_AGENTPKG message handler
//

void CConsoleApp::OnControlpanelAgentpkg() 
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);

	// create a new MDI child window or open existing
   if (m_viewState[VIEW_PACKAGE_MANAGER].bActive)
   {
      m_viewState[VIEW_PACKAGE_MANAGER].pWnd->BringWindowToTop();
   }
   else
   {
      DWORD dwResult;

      dwResult = DoRequestArg1(NXCLockPackageDB, g_hSession, "Locking package database...");
      if (dwResult == RCC_SUCCESS)
      {
	      pFrame->CreateNewChild(RUNTIME_CLASS(CPackageMgr), IDR_PACKAGE_MGR,
                                m_hPackageMgrMenu, m_hPackageMgrAccel);
      }
      else
      {
         ErrorBox(dwResult, "Unable to lock package database:\n%s");
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

   dwResult = DoRequestArg3(NXCCreateObject, g_hSession, pInfo, &dwObjectId, "Creating object...");
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

   dlg.m_pParentObject = NXCFindObjectById(g_hSession, dwParent);
   if (dlg.m_pParentObject != NULL)
      if ((dlg.m_pParentObject->iClass != OBJECT_CONTAINER) &&
          (dlg.m_pParentObject->iClass != OBJECT_SERVICEROOT))
         dlg.m_pParentObject = NULL;
   if (dlg.DoModal() == IDOK)
   {
      ci.dwParentId = (dlg.m_pParentObject != NULL) ? dlg.m_pParentObject->dwId : 0;
      ci.iClass = OBJECT_CONTAINER;
      ci.pszName = (TCHAR *)((LPCTSTR)dlg.m_strObjectName);
      ci.cs.container.dwCategory = 1;
      ci.cs.container.pszDescription = (TCHAR *)((LPCTSTR)dlg.m_strDescription);
      CreateObject(&ci);
   }
}


//
// Create condition object
//

void CConsoleApp::CreateCondition(DWORD dwParent)
{
   NXC_OBJECT_CREATE_INFO ci;
   CCreateCondDlg dlg;

   dlg.m_pParentObject = NXCFindObjectById(g_hSession, dwParent);
   if (dlg.m_pParentObject != NULL)
      if ((dlg.m_pParentObject->iClass != OBJECT_CONTAINER) &&
          (dlg.m_pParentObject->iClass != OBJECT_SERVICEROOT))
         dlg.m_pParentObject = NULL;
   if (dlg.DoModal() == IDOK)
   {
      ci.dwParentId = (dlg.m_pParentObject != NULL) ? dlg.m_pParentObject->dwId : 0;
      ci.iClass = OBJECT_CONDITION;
      ci.pszName = (TCHAR *)((LPCTSTR)dlg.m_strObjectName);
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

   dlg.m_pParentObject = NXCFindObjectById(g_hSession, dwParent);
   if (dlg.m_pParentObject != NULL)
      if ((dlg.m_pParentObject->iClass != OBJECT_CONTAINER) &&
          (dlg.m_pParentObject->iClass != OBJECT_SERVICEROOT))
         dlg.m_pParentObject = NULL;
   if (dlg.DoModal() == IDOK)
   {
      ci.dwParentId = (dlg.m_pParentObject != NULL) ? dlg.m_pParentObject->dwId : 0;
      ci.iClass = OBJECT_NODE;
      ci.pszName = (TCHAR *)((LPCTSTR)dlg.m_strObjectName);
      ci.cs.node.dwIpAddr = dlg.m_dwIpAddr;
      ci.cs.node.dwNetMask = 0;
      CreateObject(&ci);
   }
}


//
// Create network service object
//

void CConsoleApp::CreateNetworkService(DWORD dwParent)
{
   NXC_OBJECT_CREATE_INFO ci;
   CCreateNetSrvDlg dlg;

   dlg.m_pParentObject = NXCFindObjectById(g_hSession, dwParent);
   if (dlg.m_pParentObject != NULL)
      if (dlg.m_pParentObject->iClass != OBJECT_NODE)
         dlg.m_pParentObject = NULL;
   dlg.m_iServiceType = NETSRV_HTTP;
   dlg.m_iPort = 80;
   dlg.m_iProtocolNumber = 6;
   dlg.m_iProtocolType = 0;
   if (dlg.DoModal() == IDOK)
   {
      ci.dwParentId = (dlg.m_pParentObject != NULL) ? dlg.m_pParentObject->dwId : 0;
      ci.iClass = OBJECT_NETWORKSERVICE;
      ci.pszName = (TCHAR *)((LPCTSTR)dlg.m_strObjectName);
      ci.cs.netsrv.iServiceType = dlg.m_iServiceType;
      ci.cs.netsrv.wPort = (WORD)dlg.m_iPort;
      ci.cs.netsrv.wProto = (WORD)dlg.m_iProtocolNumber;
      ci.cs.netsrv.pszRequest = (TCHAR *)((LPCTSTR)dlg.m_strRequest);
      ci.cs.netsrv.pszResponse = (TCHAR *)((LPCTSTR)dlg.m_strResponse);
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

   dlg.m_iObjectClass = OBJECT_TEMPLATE;
   dlg.m_pParentObject = NXCFindObjectById(g_hSession, dwParent);
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

   dlg.m_iObjectClass = OBJECT_TEMPLATEGROUP;
   dlg.m_pParentObject = NXCFindObjectById(g_hSession, dwParent);
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
// Create VPN connector object
//

void CConsoleApp::CreateVPNConnector(DWORD dwParent)
{
   NXC_OBJECT_CREATE_INFO ci;
   CCreateVPNConnDlg dlg;

   dlg.m_iObjectClass = OBJECT_VPNCONNECTOR;
   dlg.m_pParentObject = NXCFindObjectById(g_hSession, dwParent);
   if (dlg.m_pParentObject != NULL)
      if (dlg.m_pParentObject->iClass != OBJECT_NODE)
         dlg.m_pParentObject = NULL;
   if (dlg.DoModal() == IDOK)
   {
      ci.dwParentId = (dlg.m_pParentObject != NULL) ? dlg.m_pParentObject->dwId : 0;
      ci.iClass = OBJECT_VPNCONNECTOR;
      ci.pszName = (char *)((LPCTSTR)dlg.m_strObjectName);
      CreateObject(&ci);
   }
}


//
// Delete object on server
//

void CConsoleApp::DeleteNetXMSObject(NXC_OBJECT *pObject)
{
   DWORD dwResult;

   dwResult = DoRequestArg2(NXCDeleteObject, g_hSession, (void *)pObject->dwId, "Deleting object...");
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

   dwResult = DoRequestArg2(NXCWakeUpNode, g_hSession, (void *)dwObjectId, _T("Sending Wake-On-LAN magic packet to node..."));
   if (dwResult != RCC_SUCCESS)
      ErrorBox(dwResult, _T("Unable to send WOL magic packet: %s"));
   else
      m_pMainWnd->MessageBox(_T("Wake-On-LAN magic packet was successfully sent to node"),
                             _T("Information"), MB_ICONINFORMATION | MB_OK);
}


//
// Deploy package to nodes
//

void CConsoleApp::DeployPackage(DWORD dwPkgId, DWORD dwNumObjects, DWORD *pdwObjectList)
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);
   CMDIChildWnd *pWnd;

	pWnd = pFrame->CreateNewChild(
		RUNTIME_CLASS(CDeploymentView), IDR_DEPLOYMENT_VIEW, m_hMDIMenu, m_hMDIAccel);
   if (pWnd != NULL)
   {
      DEPLOYMENT_JOB *pJob;

      pJob = (DEPLOYMENT_JOB *)malloc(sizeof(DEPLOYMENT_JOB));
      pJob->dwPkgId = dwPkgId;
      pJob->dwNumObjects = dwNumObjects;
      pJob->pdwObjectList = (DWORD *)nx_memdup(pdwObjectList, sizeof(DWORD) * dwNumObjects);
      pWnd->PostMessage(NXCM_START_DEPLOYMENT, 0, (LPARAM)pJob);
   }
}


//
// Show last collected DCI values
//

CMDIChildWnd *CConsoleApp::ShowLastValues(NXC_OBJECT *pObject, TCHAR *pszParams)
{
   CLastValuesView *pWnd;

   if (pObject != NULL)
   {
      if (pObject->iClass != OBJECT_NODE)
         return NULL;

      pWnd = new CLastValuesView(pObject->dwId);
   }
   else
   {
      pWnd = new CLastValuesView(pszParams);
      pObject = NXCFindObjectById(g_hSession, pWnd->GetObjectId());
   }
   CreateChildFrameWithSubtitle(pWnd, IDR_LAST_VALUES_VIEW,
                                (pObject != NULL) ? pObject->szName : _T("unknown object"),
                                m_hLastValuesMenu, m_hLastValuesAccel);
   return pWnd;
}


//
// Create child frame and add subtitle (- [<text>]) to it
//

void CConsoleApp::CreateChildFrameWithSubtitle(CMDIChildWnd *pWnd, UINT nId, 
                                               TCHAR *pszSubTitle, HMENU hMenu, HACCEL hAccel)
{
	CMainFrame *pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);
   CCreateContext context;

   // load the frame
	context.m_pCurrentFrame = pFrame;

	if (pWnd->LoadFrame(nId, WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL, &context))
   {
	   CString strFullString, strTitle;

	   if (strFullString.LoadString(nId))
		   AfxExtractSubString(strTitle, strFullString, CDocTemplate::docName);

      // add frame subtitle
      if (pszSubTitle != NULL)
      {
         strTitle += " - [";
         strTitle += pszSubTitle;
         strTitle += "]";
      }

	   // set the handles and redraw the frame and parent
	   pWnd->SetHandles(hMenu, hAccel);
	   pWnd->SetTitle(strTitle);
	   pWnd->InitialUpdateFrame(NULL, TRUE);
   }
   else
	{
		delete pWnd;
	}
}


//
// Apply template worker function
//

static DWORD ApplyTemplateToNodes(DWORD dwTemplateId, DWORD dwNumNodes, DWORD *pdwNodeList)
{
   DWORD i, dwResult = RCC_SUCCESS;

   for(i = 0; (i < dwNumNodes) && (dwResult == RCC_SUCCESS); i++)
      dwResult = NXCApplyTemplate(g_hSession, dwTemplateId, pdwNodeList[i]);
   return dwResult;
}


//
// Apply template object to node(s)
//

void CConsoleApp::ApplyTemplate(NXC_OBJECT *pObject)
{
   CObjectSelDlg dlg;
   DWORD dwResult;

   dlg.m_dwAllowedClasses = SCL_NODE;
   if (dlg.DoModal() == IDOK)
   {
      dwResult = DoRequestArg3(ApplyTemplateToNodes, (void *)pObject->dwId,
                               (void *)dlg.m_dwNumObjects, dlg.m_pdwObjectList,
                               _T("Applying template..."));
      if (dwResult != RCC_SUCCESS)
         ErrorBox(dwResult, _T("Error applying template: %s"));
   }
}


//
// Show network summary window
//

CMDIChildWnd *CConsoleApp::ShowNetworkSummary(void)
{
	CMainFrame *pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);
   CMDIChildWnd *pWnd;

	// create a new MDI child window or open existing
   if (m_viewState[VIEW_NETWORK_SUMMARY].bActive)
   {
      m_viewState[VIEW_NETWORK_SUMMARY].pWnd->BringWindowToTop();
      pWnd = m_viewState[VIEW_NETWORK_SUMMARY].pWnd;
   }
   else
   {
	   pWnd = pFrame->CreateNewChild(RUNTIME_CLASS(CNetSummaryFrame),
                                    IDR_NETWORK_SUMMARY, m_hMDIMenu, m_hMDIAccel);	
   }
   return pWnd;
}


//
// Object binding worker function
//

static DWORD DoObjectBinding(DWORD dwCmd, DWORD dwParent, DWORD dwNumChilds,
                             DWORD *pdwChildList, BOOL bRemoveDCI)
{
   DWORD i, dwResult;

   switch(dwCmd)
   {
      case 0:  // Bind object
         for(i = 0; i < dwNumChilds; i++)
         {
            dwResult = NXCBindObject(g_hSession, dwParent, pdwChildList[i]);
            if (dwResult != RCC_SUCCESS)
               break;
         }
         break;
      case 1:  // Unbind object
         for(i = 0; i < dwNumChilds; i++)
         {
            dwResult = NXCUnbindObject(g_hSession, dwParent, pdwChildList[i]);
            if (dwResult != RCC_SUCCESS)
               break;
         }
         break;
      case 2:  // Unbind node from template (remove template)
         for(i = 0; i < dwNumChilds; i++)
         {
            dwResult = NXCRemoveTemplate(g_hSession, dwParent, pdwChildList[i], bRemoveDCI);
            if (dwResult != RCC_SUCCESS)
               break;
         }
         break;
      default:
         break;
   }
   return dwResult;
}


//
// Bind new object(s) to current
//

void CConsoleApp::BindObject(NXC_OBJECT *pObject)
{
   CObjectSelDlg dlg;
   DWORD dwResult;

   dlg.m_dwAllowedClasses = SCL_NODE | SCL_CONTAINER | SCL_SUBNET | SCL_ZONE;
   if (dlg.DoModal() == IDOK)
   {
      dwResult = DoRequestArg5(DoObjectBinding, (void *)0, (void *)pObject->dwId,
                               (void *)dlg.m_dwNumObjects, dlg.m_pdwObjectList,
                               (void *)0, _T("Binding objects..."));
      if (dwResult != RCC_SUCCESS)
         ErrorBox(dwResult, _T("Cannot bind object: %s"));
   }
}


//
// Unbind object(s) from current
//

void CConsoleApp::UnbindObject(NXC_OBJECT *pObject)
{
   CObjectSelDlg dlg;
   DWORD dwResult, dwCmd = 1;
   BOOL bRun = TRUE, bRemoveDCI = FALSE;

   dlg.m_dwAllowedClasses = SCL_NODE | SCL_CONTAINER | SCL_SUBNET | SCL_ZONE;
   dlg.m_dwParentObject = pObject->dwId;
   if (dlg.DoModal() == IDOK)
   {
      if (pObject->iClass == OBJECT_TEMPLATE)
      {
         CRemoveTemplateDlg dlgRemove;

         dlgRemove.m_iRemoveDCI = 0;
         if (dlgRemove.DoModal() == IDOK)
         {
            bRemoveDCI = dlgRemove.m_iRemoveDCI;
            dwCmd = 2;
         }
         else
         {
            bRun = FALSE;  // Cancel unbind
         }
      }

      if (bRun)
      {
         dwResult = DoRequestArg5(DoObjectBinding, (void *)dwCmd, (void *)pObject->dwId,
                                  (void *)dlg.m_dwNumObjects, dlg.m_pdwObjectList,
                                  (void *)bRemoveDCI, _T("Unbinding objects..."));
         if (dwResult != RCC_SUCCESS)
            ErrorBox(dwResult, _T("Cannot unbind object: %s"));
      }
   }
}


//
// Change IP address for node
//

void CConsoleApp::ChangeNodeAddress(DWORD dwNodeId)
{
   CAddrChangeDlg dlg;
   DWORD dwResult;

   if (dlg.DoModal() == IDOK)
   {
      dwResult = DoRequestArg3(NXCChangeNodeIP, g_hSession, (void *)dwNodeId,
                               (void *)dlg.m_dwIpAddr, _T("Changing node's IP address..."));
      if (dwResult != RCC_SUCCESS)
         ErrorBox(dwResult, _T("Error changing IP address for node: %s"));
   }
}


//
// Show or create object browser window
//

CMDIChildWnd *CConsoleApp::ShowObjectBrowser(TCHAR *pszParams)
{
   CMDIChildWnd *pWnd;

	// create a new MDI child window or open existing
   if (m_viewState[VIEW_OBJECTS].bActive)
   {
      m_viewState[VIEW_OBJECTS].pWnd->BringWindowToTop();
      pWnd = m_viewState[VIEW_OBJECTS].pWnd;
   }
   else
   {
   	CMainFrame *pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);

      if (pszParams == NULL)
      {
	      pWnd = pFrame->CreateNewChild(RUNTIME_CLASS(CObjectBrowser), IDR_OBJECTS,
                                       m_hObjectBrowserMenu, m_hObjectBrowserAccel);
      }
      else
      {
         pWnd = new CObjectBrowser(pszParams);
         CreateChildFrameWithSubtitle(pWnd, IDR_OBJECTS, NULL, m_hObjectBrowserMenu, m_hObjectBrowserAccel);
      }
   }
   return pWnd;
}


//
// Edit agent's configuration file
//

void CConsoleApp::EditAgentConfig(NXC_OBJECT *pNode)
{
   CMainFrame *pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);
   CAgentCfgEditor *pWnd;

   pWnd = new CAgentCfgEditor(pNode->dwId);
   CreateChildFrameWithSubtitle(pWnd, IDR_AGENT_CFG_EDITOR, pNode->szName,
                                m_hAgentCfgEditorMenu, m_hAgentCfgEditorAccel);
}


//
// Worker function for DCI data export
//

static DWORD DoDataExport(DWORD dwNodeId, DWORD dwItemId, DWORD dwTimeFrom,
                          DWORD dwTimeTo, int iSeparator, int iTimeStampFormat,
                          const TCHAR *pszFile)
{
   DWORD i, dwResult;
   NXC_DCI_DATA *pData;
   NXC_DCI_ROW *pRow;
   int hFile;
   char szBuffer[MAX_DB_STRING];
   static char separator[4] = { '\t', ' ', ',', ';' };

   dwResult = NXCGetDCIData(g_hSession, dwNodeId, dwItemId, 0, dwTimeFrom,
                            dwTimeTo, &pData);
   if (dwResult == RCC_SUCCESS)
   {
      hFile = _topen(pszFile, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, S_IREAD | S_IWRITE);
      if (hFile != -1)
      {
         pRow = pData->pRows;
         for(i = 0; i < pData->dwNumRows; i++)
         {
            switch(iTimeStampFormat)
            {
               case 0:  // RAW
                  sprintf(szBuffer, "%lu", pRow->dwTimeStamp);
                  break;
               case 1:  // Text
                  FormatTimeStamp(pRow->dwTimeStamp, szBuffer, TS_LONG_DATE_TIME);
                  break;
               default:
                  strcpy(szBuffer, "<internal error>");
                  break;
            }
            write(hFile, szBuffer, strlen(szBuffer));
            write(hFile, &separator[iSeparator], 1);
            switch(pData->wDataType)
            {
               case DCI_DT_STRING:
                  write(hFile, pRow->value.szString, strlen(pRow->value.szString));
                  break;
               case DCI_DT_INT:
                  sprintf(szBuffer, "%d", pRow->value.dwInt32);
                  write(hFile, szBuffer, strlen(szBuffer));
                  break;
               case DCI_DT_UINT:
                  sprintf(szBuffer, "%u", pRow->value.dwInt32);
                  write(hFile, szBuffer, strlen(szBuffer));
                  break;
               case DCI_DT_INT64:
                  sprintf(szBuffer, "%I64d", pRow->value.qwInt64);
                  write(hFile, szBuffer, strlen(szBuffer));
                  break;
               case DCI_DT_UINT64:
                  sprintf(szBuffer, "%I64u", pRow->value.qwInt64);
                  write(hFile, szBuffer, strlen(szBuffer));
                  break;
               case DCI_DT_FLOAT:
                  sprintf(szBuffer, "%f", pRow->value.dFloat);
                  write(hFile, szBuffer, strlen(szBuffer));
                  break;
               default:
                  break;
            }
            write(hFile, "\r\n", 2);
            inc_ptr(pRow, pData->wRowSize, NXC_DCI_ROW);
         }
         close(hFile);
      }
      else
      {
         dwResult = RCC_IO_ERROR;
      }
      NXCDestroyDCIData(pData);
   }
   return dwResult;
}


//
// Export DCI's data
//

void CConsoleApp::ExportDCIData(DWORD dwNodeId, DWORD dwItemId,
                                DWORD dwTimeFrom, DWORD dwTimeTo,
                                int iSeparator, int iTimeStampFormat,
                                const TCHAR *pszFile)
{
   DWORD dwResult;
   
   dwResult = DoRequestArg7(DoDataExport, (void *)dwNodeId, (void *)dwItemId,
                            (void *)dwTimeFrom, (void *)dwTimeTo,
                            (void *)iSeparator, (void *)iTimeStampFormat,
                            (void *)pszFile, _T("Exporting data..."));
   if (dwResult == RCC_SUCCESS)
      m_pMainWnd->MessageBox(_T("Collected DCI data was successfuly exported."),
                             _T("Information"), MB_OK | MB_ICONINFORMATION);
   else
      ErrorBox(dwResult, _T("Error exporting DCI data: %s"));
}


//
// WM_COMMAND::ID_CONTROLPANEL_SERVERCFG message handler
//

void CConsoleApp::OnControlpanelServercfg() 
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);

	// create a new MDI child window or open existing
   if (m_viewState[VIEW_SERVER_CONFIG].bActive)
   {
      m_viewState[VIEW_SERVER_CONFIG].pWnd->BringWindowToTop();
   }
   else
   {
	   pFrame->CreateNewChild(RUNTIME_CLASS(CServerCfgEditor),
                             IDR_SERVER_CFG_EDITOR,
                             m_hServerCfgEditorMenu, m_hServerCfgEditorAccel);
   }
}


//
// Execute object tool
//

void CConsoleApp::ExecuteObjectTool(NXC_OBJECT *pObject, DWORD dwIndex)
{
   DWORD dwResult;

   if ((dwIndex >= g_dwNumObjectTools) ||
       (pObject->iClass != OBJECT_NODE))
      return;

   switch(g_pObjectToolList[dwIndex].wType)
   {
      case TOOL_TYPE_INTERNAL:
         if (!_tcsicmp(g_pObjectToolList[dwIndex].pszData, _T("wakeup")))
         {
            WakeUpNode(pObject->dwId);
         }
         else
         {
            m_pMainWnd->MessageBox(_T("This type of tool is not implemented yet"), _T("Warning"), MB_OK | MB_ICONSTOP);
         }
         break;
      case TOOL_TYPE_ACTION:
         if (pObject->node.dwFlags & NF_IS_NATIVE_AGENT)
         {
            dwResult = DoRequestArg3(NXCExecuteAction, g_hSession, (void *)pObject->dwId,
                                     g_pObjectToolList[dwIndex].pszData,
                                     _T("Executing action on agent..."));
            if (dwResult == RCC_SUCCESS)
            {
               m_pMainWnd->MessageBox(_T("Action executed successfully"), _T("Information"), MB_OK | MB_ICONINFORMATION);
            }
            else
            {
               ErrorBox(dwResult, _T("Error executing action on agent: %s"));
            }
         }
         else
         {
            m_pMainWnd->MessageBox(_T("Node doesn't have NetXMS agent"), _T("Error"), MB_OK | MB_ICONSTOP);
         }
         break;
      case TOOL_TYPE_TABLE_AGENT:
         if (pObject->node.dwFlags & NF_IS_NATIVE_AGENT)
         {
            ExecuteTableTool(pObject, g_pObjectToolList[dwIndex].dwId);
         }
         else
         {
            m_pMainWnd->MessageBox(_T("Node doesn't have NetXMS agent"), _T("Error"), MB_OK | MB_ICONSTOP);
         }
         break;
      case TOOL_TYPE_TABLE_SNMP:
         if (pObject->node.dwFlags & NF_IS_SNMP)
         {
            ExecuteTableTool(pObject, g_pObjectToolList[dwIndex].dwId);
         }
         else
         {
            m_pMainWnd->MessageBox(_T("Node doesn't have SNMP agent"), _T("Error"), MB_OK | MB_ICONSTOP);
         }
         break;
      case TOOL_TYPE_COMMAND:
         ExecuteCmdTool(pObject, g_pObjectToolList[dwIndex].pszData);
         break;
      case TOOL_TYPE_URL:
         ExecuteWebTool(pObject, g_pObjectToolList[dwIndex].pszData);
         break;
      default:
         m_pMainWnd->MessageBox(_T("This type of tool is not implemented yet"), _T("Warning"), MB_OK | MB_ICONSTOP);
         break;
   }
}


//
// Execute table tool on given node
//

void CConsoleApp::ExecuteTableTool(NXC_OBJECT *pNode, DWORD dwToolId)
{
   CTableView *pWnd;

   pWnd = new CTableView(pNode->dwId, dwToolId);
   CreateChildFrameWithSubtitle(pWnd, IDR_TABLE_VIEW,
                                pNode->szName, m_hMDIMenu, m_hMDIAccel);
//   return pWnd;
}


//
// Execute web tool (open specified URL)
//

void CConsoleApp::ExecuteWebTool(NXC_OBJECT *pObject, TCHAR *pszURL)
{
   TCHAR szRealURL[4096];

   _stprintf(szRealURL, _T("%lu"), pObject->dwId);
   SetEnvironmentVariable(_T("OBJECT_ID"), szRealURL);
   SetEnvironmentVariable(_T("OBJECT_IP_ADDR"), IpToStr(pObject->dwIpAddr, szRealURL));
   SetEnvironmentVariable(_T("OBJECT_NAME"), pObject->szName);
   ExpandEnvironmentStrings(pszURL, szRealURL, 4096);
   StartWebBrowser(szRealURL);
}


//
// Execute external command tool
//

void CConsoleApp::ExecuteCmdTool(NXC_OBJECT *pObject, TCHAR *pszCmd)
{
   TCHAR szCmdLine[4096];
   STARTUPINFO si;
   PROCESS_INFORMATION pi;

   // Prepare command
   _stprintf(szCmdLine, _T("%lu"), pObject->dwId);
   SetEnvironmentVariable(_T("OBJECT_ID"), szCmdLine);
   SetEnvironmentVariable(_T("OBJECT_IP_ADDR"), IpToStr(pObject->dwIpAddr, szCmdLine));
   SetEnvironmentVariable(_T("OBJECT_NAME"), pObject->szName);
   ExpandEnvironmentStrings(pszCmd, szCmdLine, 4096);

   // Fill in process startup info structure
   memset(&si, 0, sizeof(STARTUPINFO));
   si.cb = sizeof(STARTUPINFO);
   si.dwFlags = 0;

   // Create new process
   if (!CreateProcess(NULL, szCmdLine, NULL, NULL, FALSE, 
                      0, NULL, NULL, &si, &pi))
   {
      TCHAR szError[256], szMsg[4096];

      GetSystemErrorText(GetLastError(), szError, 256);
      _sntprintf(szMsg, 4096, _T("Cannot execute command \"%s\":\n%s"), szCmdLine, szError);
      m_pMainWnd->MessageBox(szMsg, _T("Error"), MB_OK | MB_ICONSTOP);
   }
   else
   {
      // Close all handles
      CloseHandle(pi.hThread);
      CloseHandle(pi.hProcess);
   }
}


//
// Start embedded web browser
//

void CConsoleApp::StartWebBrowser(TCHAR *pszURL)
{
   CWebBrowser *pWnd;

   pWnd = new CWebBrowser(pszURL);
   CreateChildFrameWithSubtitle(pWnd, IDR_WEB_BROWSER, pszURL, m_hMDIMenu, m_hMDIAccel);
}


//
// Execute object move
//

static DWORD DoObjectMove(DWORD dwObject, DWORD dwOldParent, DWORD dwNewParent)
{
   DWORD dwResult;

   dwResult = NXCBindObject(g_hSession, dwNewParent, dwObject);
   if (dwResult == RCC_SUCCESS)
   {
      dwResult = NXCUnbindObject(g_hSession, dwOldParent, dwObject);
      if (dwResult != RCC_SUCCESS)
         NXCUnbindObject(g_hSession, dwNewParent, dwObject);   // Undo binding to new parent
   }
   return dwResult;
}


//
// Move object between containers
//

void CConsoleApp::MoveObject(DWORD dwObjectId, DWORD dwParentId)
{
   CObjectSelDlg dlg;
   DWORD dwResult;

   dlg.m_dwAllowedClasses = SCL_CONTAINER | SCL_SERVICEROOT;
   dlg.m_bSingleSelection = TRUE;
   if (dlg.DoModal() == IDOK)
   {
      dwResult = DoRequestArg3(DoObjectMove, (void *)dwObjectId, (void *)dwParentId,
                               (void *)dlg.m_pdwObjectList[0], _T("Moving object..."));
      if (dwResult != RCC_SUCCESS)
         ErrorBox(dwResult, _T("Error moving object: %s"));
   }
}


//
// WM_COMMAND::ID_CONTROLPANEL_MODULES
//

void CConsoleApp::OnControlpanelModules() 
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);

	// create a new MDI child window or open existing
   if (m_viewState[VIEW_MODULE_MANAGER].bActive)
   {
      m_viewState[VIEW_MODULE_MANAGER].pWnd->BringWindowToTop();
   }
   else
   {
	   pFrame->CreateNewChild(RUNTIME_CLASS(CModuleManager), IDR_MODULE_MANAGER,
                             m_hMDIMenu, m_hMDIAccel);
   }
}


//
// WM_COMMAND::ID_DESKTOP_MANAGE
//

void CConsoleApp::OnDesktopManage() 
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);

	// create a new MDI child window or open existing
   if (m_viewState[VIEW_DESKTOP_MANAGER].bActive)
   {
      m_viewState[VIEW_DESKTOP_MANAGER].pWnd->BringWindowToTop();
   }
   else
   {
	   pFrame->CreateNewChild(RUNTIME_CLASS(CDesktopManager), IDR_DESKTOP_MANAGER,
                             m_hMDIMenu, m_hMDIAccel);
   }
}


//
// WM_COMMAND::ID_TOOLS_CHANGEPASSWORD message handler
//

void CConsoleApp::OnToolsChangepassword() 
{
   CPasswordChangeDlg dlg(IDD_SET_PASSWORD);

   if (dlg.DoModal() == IDOK)
   {
      DWORD dwResult;

      dwResult = DoRequestArg3(NXCSetPassword, g_hSession, 
                               (void *)NXCGetCurrentUserId(g_hSession),
                               dlg.m_szPassword, _T("Changing password..."));
      if (dwResult == RCC_SUCCESS)
      {
         m_pMainWnd->MessageBox(_T("Password was successfully changed"),
                                _T("Information"), MB_ICONINFORMATION | MB_OK);
      }
      else
      {
         ErrorBox(dwResult, _T("Cannot change password: %s"));
      }
   }
}
