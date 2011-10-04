// nxcon.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include <dbghelp.h>
#include <geolocation.h>
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
#include "CreateClusterDlg.h"
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
#include "ClusterPropsGeneral.h"
#include "ClusterPropsResources.h"
#include "DCIDataView.h"
#include "ObjectBrowser.h"
#include "PackageMgr.h"
#include "DesktopManager.h"
#include "CondPropsGeneral.h"
#include "CondPropsData.h"
#include "CondPropsScript.h"
#include "AgentConfigMgr.h"
#include "ObjectCommentsEditor.h"
#include "DetailsView.h"
#include "DiscoveryPropAddrList.h"
#include "DiscoveryPropGeneral.h"
#include "DiscoveryPropTargets.h"
#include "DiscoveryPropCommunities.h"
#include "AlarmBrowser.h"
#include "ObjectBrowser.h"
#include "DataCollectionEditor.h"
#include "CreateMPDlg.h"
#include "SelectMPDlg.h"
#include "ConsoleUpgradeDlg.h"
#include "FatalErrorDlg.h"
#include "GraphManagerDlg.h"
#include "CertManager.h"
#include "CreateIfDCIDlg.h"
#include "IfPropsGeneral.h"
#include "SituationManager.h"
#include "MapManager.h"
#include "ObjectPropsTrustedNodes.h"
#include "ObjectPropsCustomAttrs.h"
#include "SyslogParserCfg.h"
#include "TemplatePropsAutoApply.h"
#include "ContainerPropsAutoBind.h"
#include "NodePropsConn.h"
#include "ObjectPropsGeolocation.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#include <openssl/applink.c>
#endif


//
// Wrapper for client library debug callback
//

static void ClientDebugCallback(TCHAR *pszMsg)
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
	ON_COMMAND(ID_CONTROLPANEL_OBJECTTOOLS, OnControlpanelObjecttools)
	ON_COMMAND(ID_CONTROLPANEL_SCRIPTLIBRARY, OnControlpanelScriptlibrary)
	ON_COMMAND(ID_VIEW_SNMPTRAPLOG, OnViewSnmptraplog)
	ON_COMMAND(ID_DESKTOP_MANAGE, OnDesktopManage)
	ON_COMMAND(ID_TOOLS_CHANGEPASSWORD, OnToolsChangepassword)
	ON_COMMAND(ID_CONTROLPANEL_AGENTCONFIGS, OnControlpanelAgentconfigs)
	ON_COMMAND(ID_CONTROLPANEL_NETWORKDISCOVERY, OnControlpanelNetworkdiscovery)
	ON_COMMAND(ID_TOOLS_CREATEMP, OnToolsCreatemp)
	ON_COMMAND(ID_FILE_PAGESETUP, OnFilePagesetup)
	ON_COMMAND(ID_TOOLS_IMPORTMP, OnToolsImportmp)
	ON_COMMAND(ID_TOOLS_GRAPHS_MANAGE, OnToolsGraphsManage)
	ON_COMMAND(ID_CONTROLPANEL_CERTIFICATES, OnControlpanelCertificates)
	ON_COMMAND(ID_VIEW_SITUATIONS, OnViewSituations)
	ON_COMMAND(ID_CONTROLPANEL_NETWORKMAPS, OnControlpanelNetworkmaps)
	ON_COMMAND(ID_CONTROLPANEL_SYSLOGPARSER, OnControlpanelSyslogparser)
	//}}AFX_MSG_MAP
	ON_THREAD_MESSAGE(NXCM_GRAPH_LIST_UPDATED, OnGraphListUpdate)
	ON_COMMAND_RANGE(GRAPH_MENU_FIRST_ID, GRAPH_MENU_LAST_ID, OnPredefinedGraph)
END_MESSAGE_MAP()


//
// CConsoleApp construction
//

CConsoleApp::CConsoleApp()
{
   m_dwClientState = STATE_DISCONNECTED;
   memset(m_openObjectViews, 0, sizeof(OBJECT_VIEW) * MAX_OBJECT_VIEWS);
   memset(m_viewState, 0, sizeof(CONSOLE_VIEW) * MAX_VIEW_ID);
   m_bIgnoreErrors = FALSE;
   m_dwNumAlarms = 0;
   m_pAlarmList = NULL;
   m_mutexAlarmList = MutexCreate();
	m_pSituationList = NULL;
   m_mutexSituationList = MutexCreate();
   m_hDevMode = NULL;
   m_hDevNames = NULL;
	m_dwMainThreadId = GetCurrentThreadId();
	m_bReconnect = FALSE;
}


//
// CConsoleApp destruction
//

CConsoleApp::~CConsoleApp()
{
   safe_free(m_pAlarmList);
   MutexDestroy(m_mutexAlarmList);
	NXCDestroySituationList(m_pSituationList);
   MutexDestroy(m_mutexSituationList);
   SafeGlobalFree(m_hDevMode);
   SafeGlobalFree(m_hDevNames);
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
   _tcscat(g_szWorkDir, _T("\\NetXMS Console"));
   if (!CreateDirectory(g_szWorkDir, NULL))
      if (GetLastError() != ERROR_ALREADY_EXISTS)
      {
         AfxMessageBox(IDS_WORKDIR_CREATION_FAILED, MB_OK | MB_ICONSTOP);
         return FALSE;
      }
   iLastChar = (int)_tcslen(g_szWorkDir);

   // Create MIB cache directory
   _tcscpy(&g_szWorkDir[iLastChar], WORKDIR_MIBCACHE);
   if (!CreateDirectory(g_szWorkDir, NULL))
      if (GetLastError() != ERROR_ALREADY_EXISTS)
      {
         AfxMessageBox(IDS_WORKDIR_CREATION_FAILED, MB_OK | MB_ICONSTOP);
         return FALSE;
      }

   // Create image cache directory
   _tcscpy(&g_szWorkDir[iLastChar], WORKDIR_IMAGECACHE);
   if (!CreateDirectory(g_szWorkDir, NULL))
      if (GetLastError() != ERROR_ALREADY_EXISTS)
      {
         AfxMessageBox(IDS_WORKDIR_CREATION_FAILED, MB_OK | MB_ICONSTOP);
         return FALSE;
      }

   // Create background image cache directory
   _tcscpy(&g_szWorkDir[iLastChar], WORKDIR_BKIMAGECACHE);
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
   LONG nVer;

	CRYPTO_malloc_init();

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

   // Initialize speach engine
   SpeakerInit();

	// Change the registry key under which our settings are stored.
	SetRegistryKey(_T("NetXMS"));

   // Load configuration from registry
   dwBytes = sizeof(WINDOWPLACEMENT);
   bSetWindowPos = GetProfileBinary(_T("General"), _T("WindowPlacement"), 
                                    &pData, (UINT *)&dwBytes);
   g_dwOptions = GetProfileInt(_T("General"), _T("Options"), g_dwOptions);
   _tcscpy(g_szServer, (LPCTSTR)GetProfileString(_T("Connection"), _T("Server"), _T("localhost")));
   _tcscpy(g_szLogin, (LPCTSTR)GetProfileString(_T("Connection"), _T("Login"), NULL));
   _tcscpy(g_szLastCertName, (LPCTSTR)GetProfileString(_T("Connection"), _T("Certificate"), NULL));
	g_nAuthType = GetProfileInt(_T("Connection"), _T("AuthType"), NETXMS_AUTH_TYPE_PASSWORD);
   LoadAlarmSoundCfg(&g_soundCfg, NXCON_ALARM_SOUND_KEY);
   m_hDevMode = GetProfileGMem(_T("Printer"), _T("DevMode"));
   m_hDevNames = GetProfileGMem(_T("Printer"), _T("DevNames"));

   // Check if we are upgrading
   nVer = GetProfileInt(_T("General"), _T("CfgVersion"), 0);
   if (nVer < NXCON_CONFIG_VERSION)
   {
      // Upgrade configuration
      // Please note that thre should not be break after each case
      // to allow configuration upgrade from any old version to current
      switch(nVer)
      {
         case 0:
            g_dwOptions |= UI_OPT_CONFIRM_OBJ_DELETE;
         default:
            break;
      }
      WriteProfileInt(_T("General"), _T("CfgVersion"), NXCON_CONFIG_VERSION);
   }

	CreateObjectImageList();

	// Time zone information
	m_nTimeZoneType = GetProfileInt(_T("General"), _T("TimeZoneType"), TZ_TYPE_LOCAL);
	m_strCustomTimeZone = GetProfileString(_T("General"), _T("TimeZone"), _T("UTC"));
	if (m_nTimeZoneType == TZ_TYPE_CUSTOM)
	{
		TCHAR buffer[256];

		_sntprintf(buffer, 256, _T("TZ=%s"), (LPCTSTR)m_strCustomTimeZone);
		_tputenv(buffer);
		_tzset();
	}

   // Create mutexes for action and graph lists access
   g_mutexActionListAccess = CreateMutex(NULL, FALSE, NULL);
   g_mutexGraphListAccess = CreateMutex(NULL, FALSE, NULL);

	// To create the main window, this code creates a new frame window
	// object and then sets it as the application's main window object.
	CMDIFrameWnd* pFrame = new CMainFrame;
	m_pMainWnd = pFrame;

	// create main MDI frame window
	if (!pFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	m_hMainMenu = pFrame->GetMenu()->GetSafeHmenu();

   // Load context menus
   if (!m_ctxMenu.LoadMenu(IDR_MENU_CONTEXT))
      MessageBox(NULL, _T("FAILED"), _T(""), 0);

	// Load shared MDI menus and accelerator table
	HINSTANCE hInstance = AfxGetResourceHandle();

	hMenu = ::LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU_VIEW_SPECIFIC));

   // Modify application menu as needed
   if (g_dwOptions & UI_OPT_EXPAND_CTRLPANEL)
   {
      pFrame->GetMenu()->RemoveMenu(ID_VIEW_CONTROLPANEL, MF_BYCOMMAND);
      pFrame->GetMenu()->InsertMenu(ID_VIEW_DEBUG, MF_BYCOMMAND | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 8), _T("&Control panel"));
   }

   m_hMDIMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hMDIMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), _T("&Window"));

   m_hEventBrowserMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hEventBrowserMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), _T("&Window"));
   InsertMenu(m_hEventBrowserMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 1), _T("&Event"));

   m_hObjectBrowserMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hObjectBrowserMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), _T("&Window"));
   InsertMenu(m_hObjectBrowserMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 2), _T("&Object"));

   m_hUserEditorMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hUserEditorMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), _T("&Window"));
   InsertMenu(m_hUserEditorMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 3), _T("&User"));

   m_hDCEditorMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hDCEditorMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), _T("&Window"));
   InsertMenu(m_hDCEditorMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 4), _T("&Item"));

   m_hPolicyEditorMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hPolicyEditorMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), _T("&Window"));
   InsertMenu(m_hPolicyEditorMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 5), _T("&Policy"));

   m_hAlarmBrowserMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hAlarmBrowserMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), _T("&Window"));
   InsertMenu(m_hAlarmBrowserMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 6), _T("&Alarm"));

   m_hMapMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hMapMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), _T("&Window"));
   InsertMenu(m_hMapMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 21), _T("&Map"));
   InsertMenu(m_hMapMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 9), _T("&Object"));

   m_hEventEditorMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hEventEditorMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), _T("&Window"));
   InsertMenu(m_hEventEditorMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 10), _T("&Event"));

   m_hActionEditorMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hActionEditorMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), _T("&Window"));
   InsertMenu(m_hActionEditorMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 11), _T("&Action"));

   m_hTrapEditorMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hTrapEditorMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), _T("&Window"));
   InsertMenu(m_hTrapEditorMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 12), _T("T&rap"));

   m_hGraphMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hGraphMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), _T("&Window"));
   InsertMenu(m_hGraphMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 13), _T("&Graph"));

   m_hPackageMgrMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hPackageMgrMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), _T("&Window"));
   InsertMenu(m_hPackageMgrMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 14), _T("&Package"));

   m_hLastValuesMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hLastValuesMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), _T("&Window"));
   InsertMenu(m_hLastValuesMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 15), _T("&Item"));

   m_hServerCfgEditorMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hServerCfgEditorMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), _T("&Window"));
   InsertMenu(m_hServerCfgEditorMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 16), _T("V&ariable"));

   m_hAgentCfgEditorMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hAgentCfgEditorMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), _T("&Window"));
   InsertMenu(m_hAgentCfgEditorMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 17), _T("&Edit"));
   InsertMenu(m_hAgentCfgEditorMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 18), _T("&Config"));

   m_hObjToolsEditorMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hObjToolsEditorMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), _T("&Window"));
   InsertMenu(m_hObjToolsEditorMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 19), _T("&Object tools"));

   m_hScriptManagerMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hScriptManagerMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), _T("&Window"));
   InsertMenu(m_hScriptManagerMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 17), _T("&Edit"));
   InsertMenu(m_hScriptManagerMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 20), _T("&Script"));

   m_hDataViewMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hDataViewMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), _T("&Window"));
   InsertMenu(m_hDataViewMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 22), _T("&Data"));

   m_hAgentCfgMgrMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hAgentCfgMgrMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), _T("&Window"));
   InsertMenu(m_hAgentCfgMgrMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 23), _T("&Config"));

   m_hObjectCommentsMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hObjectCommentsMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), _T("&Window"));
   InsertMenu(m_hObjectCommentsMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 24), _T("&Comments"));
   InsertMenu(m_hObjectCommentsMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 17), _T("&Edit"));

   m_hCertManagerMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hCertManagerMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), _T("&Window"));
   InsertMenu(m_hCertManagerMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 25), _T("&Certificate"));

   m_hNodePollerMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hNodePollerMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), _T("&Window"));
   InsertMenu(m_hNodePollerMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 26), _T("&Poller"));

   m_hSituationManagerMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hSituationManagerMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), _T("&Window"));
   InsertMenu(m_hSituationManagerMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 27), _T("&Situation"));

   m_hMapManagerMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hMapManagerMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), _T("&Window"));
   InsertMenu(m_hMapManagerMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 28), _T("&Map"));

   m_hSyslogParserCfgMenu = LoadAppMenu(hMenu);
   InsertMenu(m_hSyslogParserCfgMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 0), _T("&Window"));
   InsertMenu(m_hSyslogParserCfgMenu, LAST_APP_MENU - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 17), _T("&Edit"));
   InsertMenu(m_hSyslogParserCfgMenu, LAST_APP_MENU, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMenu, 29), _T("&Parser"));

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
	m_hObjToolsEditorAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_OBJECT_TOOLS_EDITOR));
	m_hScriptManagerAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_SCRIPT_MANAGER));
	m_hDataViewAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_DATA_VIEW));
	m_hAgentCfgMgrAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_AGENT_CONFIG_MANAGER));
	m_hObjectCommentsAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_OBJECT_COMMENTS));
	m_hCertManagerAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_CERT_MANAGER));
	m_hNodePollerAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_NODE_POLLER));
	m_hSituationManagerAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_SITUATION_MANAGER));
	m_hMapManagerAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_MAP_MANAGER));
/* FIXME */	m_hSyslogParserCfgAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_AGENT_CFG_EDITOR));

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
		g_hSession = NULL;
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
   SafeFreeResource(m_hScriptManagerMenu);
   SafeFreeResource(m_hScriptManagerAccel);
   SafeFreeResource(m_hDataViewMenu);
   SafeFreeResource(m_hDataViewAccel);
   SafeFreeResource(m_hAgentCfgMgrMenu);
   SafeFreeResource(m_hAgentCfgMgrAccel);
   SafeFreeResource(m_hObjectCommentsMenu);
   SafeFreeResource(m_hObjectCommentsAccel);
   SafeFreeResource(m_hCertManagerMenu);
   SafeFreeResource(m_hCertManagerAccel);
   SafeFreeResource(m_hNodePollerMenu);
   SafeFreeResource(m_hNodePollerAccel);
   SafeFreeResource(m_hSituationManagerMenu);
   SafeFreeResource(m_hSituationManagerAccel);
   SafeFreeResource(m_hMapManagerMenu);
   SafeFreeResource(m_hMapManagerAccel);

   CloseHandle(g_mutexActionListAccess);
   CloseHandle(g_mutexGraphListAccess);

   SpeakerShutdown();

	delete g_pMIBRoot;

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

   if (dwView >= FIRST_OBJECT_VIEW)
   {
      // Register new object view
      for(i = 0; i < MAX_OBJECT_VIEWS; i++)
         if (m_openObjectViews[i].pWnd == NULL)
         {
            m_openObjectViews[i].pWnd = pWnd;
            m_openObjectViews[i].dwClass = dwView;
            m_openObjectViews[i].dwId = dwArg;
            break;
         }
   }
   else
   {
      // Register view
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

   if (dwView >= FIRST_OBJECT_VIEW)
   {
      // Unregister object view
      for(i = 0; i < MAX_OBJECT_VIEWS; i++)
         if (m_openObjectViews[i].pWnd == pWnd)
         {
            m_openObjectViews[i].pWnd = NULL;
            m_openObjectViews[i].dwClass = 0;
            m_openObjectViews[i].dwId = 0;
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

CMDIChildWnd *CConsoleApp::ShowEventBrowser()
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

CMDIChildWnd *CConsoleApp::ShowSyslogBrowser()
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

CMDIChildWnd *CConsoleApp::ShowTrapLogBrowser()
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
	ShowMapFrame();
}


//
// Show or create object browser window
//

CMDIChildWnd *CConsoleApp::ShowMapFrame(TCHAR *pszParams)
{
   CMDIChildWnd *pWnd;
	CMainFrame *pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);
   if (pszParams == NULL)
   {
      pWnd = pFrame->CreateNewChild(RUNTIME_CLASS(CMapFrame), IDR_MAPFRAME, m_hMapMenu, m_hMapAccel);
   }
   else
   {
      pWnd = new CMapFrame(pszParams);
      CreateChildFrameWithSubtitle(pWnd, IDR_MAPFRAME, NULL, m_hMapMenu, m_hMapAccel);
   }
   return pWnd;
}


//
// WM_COMMAND::ID_CONNECT_TO_SERVER message handler
//

void CConsoleApp::OnConnectToServer() 
{
	CLoginDialog dlgLogin;
   DWORD dwResult;

	m_bIgnoreErrors = FALSE;

   dlgLogin.m_strServer = g_szServer;
   dlgLogin.m_strLogin = g_szLogin;
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
	dlgLogin.m_nAuthType = g_nAuthType;
   do
   {
		dlgLogin.m_strLastCertName = g_szLastCertName;
		if (m_bReconnect)
		{
			m_bReconnect = FALSE;
		}
		else
		{
			if (dlgLogin.DoModal() != IDOK)
			{
				PostQuitMessage(1);
				break;
			}
	      _tcscpy(g_szPassword, (LPCTSTR)dlgLogin.m_strPassword);
		}
      _tcscpy(g_szServer, (LPCTSTR)dlgLogin.m_strServer);
      _tcscpy(g_szLogin, (LPCTSTR)dlgLogin.m_strLogin);
		g_nAuthType = dlgLogin.m_nAuthType;
		if (g_nAuthType == NETXMS_AUTH_TYPE_CERTIFICATE)
			nx_strncpy(g_szLastCertName, dlgLogin.m_pCertList[dlgLogin.m_nCertificateIndex].szName, MAX_DB_STRING);
		else
			g_szLastCertName[0] = 0;

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
      WriteProfileString(_T("Connection"), _T("Certificate"), g_szLastCertName);
      WriteProfileInt(_T("Connection"), _T("AuthType"), g_nAuthType);

      // Initiate connection
      dwResult = DoLogin(dlgLogin.m_bClearCache,
			(g_nAuthType == NETXMS_AUTH_TYPE_CERTIFICATE) ? dlgLogin.m_pCertList[dlgLogin.m_nCertificateIndex].pCert : NULL);
      if (dwResult == RCC_SUCCESS)
      {
         if (NXCIsDBConnLost(g_hSession))
            m_pMainWnd->MessageBox(_T("NetXMS server currently has no connection with backend database. You will not be able to access historical data, and many other functions may be disabled or not work as expected."),
                                   _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
			if (m_nTimeZoneType == TZ_TYPE_SERVER)
			{
				const TCHAR *tz = NXCGetServerTimeZone(g_hSession);
				if (tz != NULL)
				{
					TCHAR buffer[256];

					_sntprintf_s(buffer, 256, _TRUNCATE, _T("TZ=%s"), tz);
					_tputenv(buffer);
					_tzset();
				}
				else
				{
					m_pMainWnd->MessageBox(_T("Unable to switch to server's time zone because server does not provide time zone information"), _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
				}
			}
      }
      else
      {
			if (((dwResult == RCC_VERSION_MISMATCH) || (dwResult == RCC_BAD_PROTOCOL)) &&
				 (g_szUpgradeURL[0] != 0))
			{
				if (StartConsoleUpgrade())
				{
		         PostQuitMessage(1);
					break;
				}
			}
			else
			{
				ErrorBox(dwResult, _T("Unable to connect: %s"), _T("Connection error"));
			}
      }
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
            if (IsWindow(m_pMainWnd->m_hWnd))
            {
               if (m_pMainWnd->MessageBox(_T("Connection with the management server terminated unexpectedly.\nDo you wish to reconnect?"), _T("Error"), MB_YESNO | MB_ICONSTOP) == IDYES)
					{
						m_bReconnect = TRUE;
					   m_pMainWnd->PostMessage(WM_COMMAND, ID_CONNECT_TO_SERVER, 0);
					}
					else
					{
						m_pMainWnd->PostMessage(WM_CLOSE, 0, 0);
					}
            }
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
		case NXC_EVENT_SITUATION_UPDATE:
			MutexLock(m_mutexSituationList, INFINITE);
			if (m_pSituationList != NULL)
			{
				NXCUpdateSituationList(m_pSituationList, dwCode, (NXC_SITUATION *)pArg);
	         ((CMainFrame *)m_pMainWnd)->PostMessage(NXCM_SITUATION_CHANGE, dwCode, ((NXC_SITUATION *)pArg)->m_id);
			}
			MutexUnlock(m_mutexSituationList);
			break;
      case NXC_EVENT_NOTIFICATION:
         switch(dwCode)
         {
            case NX_NOTIFY_SHUTDOWN:
               if (!m_bIgnoreErrors)
               {
                  m_bIgnoreErrors = TRUE;
                  if (m_pMainWnd->MessageBox(_T("Server was shutdown. Do you wish to reconnect?"), _T("Warning"), MB_YESNO | MB_ICONSTOP) == IDYES)
						{
							m_bReconnect = TRUE;
						   m_pMainWnd->PostMessage(WM_COMMAND, ID_CONNECT_TO_SERVER, 0);
						}
						else
						{
							m_pMainWnd->PostMessage(WM_CLOSE, 0, 0);
						}
               }
               break;
            case NX_NOTIFY_DBCONN_STATUS:
               if (CAST_FROM_POINTER(pArg, int))
               {
                  m_pMainWnd->MessageBox(_T("Connection between NetXMS server and backend database restored."),
                                         _T("Information"), MB_OK | MB_ICONINFORMATION);
               }
               else
               {
                  m_pMainWnd->MessageBox(_T("Server has lost connection to backend database.\nAccess to historical data and most configuration fuctions will be unavailabe."),
                                         _T("Critical Alarm"), MB_OK | MB_ICONSTOP);
               }
               break;
            case NX_NOTIFY_EVENTDB_CHANGED:
               m_pMainWnd->PostMessage(NXCM_UPDATE_EVENT_LIST);
               break;
            case NX_NOTIFY_OBJTOOLS_CHANGED:
            case NX_NOTIFY_OBJTOOL_DELETED:
               m_pMainWnd->PostMessage(NXCM_UPDATE_OBJECT_TOOLS, dwCode, CAST_FROM_POINTER(pArg, LPARAM));
               break;
            case NX_NOTIFY_NEW_ALARM:
            case NX_NOTIFY_ALARM_DELETED:
            case NX_NOTIFY_ALARM_CHANGED:
            case NX_NOTIFY_ALARM_TERMINATED:
               OnAlarmUpdate(dwCode, (NXC_ALARM *)pArg);
               m_pMainWnd->PostMessage(NXCM_ALARM_UPDATE, dwCode, 
                                       (LPARAM)nx_memdup(pArg, sizeof(NXC_ALARM)));
               break;
            case NX_NOTIFY_ACTION_CREATED:
            case NX_NOTIFY_ACTION_MODIFIED:
            case NX_NOTIFY_ACTION_DELETED:
               UpdateActions(dwCode, (NXC_ACTION *)pArg);
               m_pMainWnd->PostMessage(NXCM_ACTION_UPDATE, dwCode, 
                                       (LPARAM)nx_memdup(pArg, sizeof(NXC_ACTION)));
               break;
            case NX_NOTIFY_TRAPCFG_CREATED:
            case NX_NOTIFY_TRAPCFG_MODIFIED:
            case NX_NOTIFY_TRAPCFG_DELETED:
               m_pMainWnd->PostMessage(NXCM_TRAPCFG_UPDATE, dwCode, 
                                       (LPARAM)NXCDuplicateTrapCfgEntry((NXC_TRAP_CFG_ENTRY *)pArg));
               break;
            case NX_NOTIFY_ETMPL_CHANGED:
            case NX_NOTIFY_ETMPL_DELETED:
               m_pMainWnd->PostMessage(NXCM_EVENTDB_UPDATE, dwCode, 
                                       (LPARAM)nx_memdup(pArg, sizeof(NXC_EVENT_TEMPLATE)));
               break;
				case NX_NOTIFY_GRAPHS_CHANGED:
					UpdateGraphList();
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
      pFrame->CreateNewChild(RUNTIME_CLASS(CEventEditor), IDR_EVENT_EDITOR,
                             m_hEventEditorMenu, m_hEventEditorAccel);
   }
}


//
// Handler for WM_COMMAND::ID_CONTROLPANEL_CERTIFICATES message
//

void CConsoleApp::OnControlpanelCertificates() 
{
	CMainFrame *pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);

	// create a new MDI child window or open existing
   if (m_viewState[VIEW_CERTIFICATE_MANAGER].bActive)
   {
      m_viewState[VIEW_CERTIFICATE_MANAGER].pWnd->BringWindowToTop();
   }
   else
   {
	   pFrame->CreateNewChild(RUNTIME_CLASS(CCertManager),
                             IDR_CERT_MANAGER,
                             m_hCertManagerMenu, m_hCertManagerAccel);
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

      dwResult = DoRequestArg1(NXCLockUserDB, g_hSession, _T("Locking user database..."));
      if (dwResult == RCC_SUCCESS)
      {
	      pFrame->CreateNewChild(RUNTIME_CLASS(CUserEditor), IDR_USER_EDITOR,
                                m_hUserEditorMenu, m_hUserEditorAccel);
      }
      else
      {
         ErrorBox(dwResult, _T("Unable to lock user database:\n%s"));
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
	   pFrame->CreateNewChild(RUNTIME_CLASS(CObjectToolsEditor),
                             IDR_OBJECT_TOOLS_EDITOR,
                             m_hObjToolsEditorMenu, m_hObjToolsEditorAccel);
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

void CConsoleApp::DebugPrintf(TCHAR *szFormat, ...)
{
   if (m_viewState[VIEW_DEBUG].bActive)
   {
      TCHAR szBuffer[1024];
      va_list args;

      va_start(args, szFormat);
      _vsntprintf(szBuffer, 1024, szFormat, args);
      va_end(args);

      ((CDebugFrame *)m_viewState[VIEW_DEBUG].pWnd)->AddMessage(szBuffer);
   }
}


//
// Edit properties of specific object
//

void CConsoleApp::ObjectProperties(DWORD dwObjectId)
{
	CObjectPropSheet wndPropSheet(_T("Object Properties"), GetMainWnd(), 0);
   CNodePropsGeneral wndNodeGeneral;
	CNodePropsConn wndNodeConn;
   CNetSrvPropsGeneral wndNetSrvGeneral;
	CIfPropsGeneral wndIfGeneral;
   CObjectPropsGeneral wndObjectGeneral;
   CObjectPropCaps wndObjectCaps;
   CObjectPropsSecurity wndObjectSecurity;
   CObjectPropsPresentation wndObjectPresentation;
   CObjectPropsRelations wndObjectRelations;
	CObjectPropsTrustedNodes wndObjectTrustedNodes;
   CNodePropsPolling wndNodePolling;
   CVPNCPropsGeneral wndVPNCGeneral;
   CObjectPropsStatus wndStatus;
   CCondPropsGeneral wndCondGeneral;
   CCondPropsData wndCondData;
   CCondPropsScript wndCondScript;
	CClusterPropsGeneral wndClusterGeneral;
	CClusterPropsResources wndClusterResources;
	CObjectPropsCustomAttrs wndCustomAttrs;
	CTemplatePropsAutoApply wndTemplateAutoApply;
	CContainerPropsAutoBind wndContainerAutoBind;
	CObjectPropsGeolocation wndGeoLocation;
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
            wndNodeGeneral.m_strOID = pObject->node.pszSnmpObjectId;
            wndNodeGeneral.m_dwIpAddr = pObject->dwIpAddr;
            wndPropSheet.AddPage(&wndNodeGeneral);

				// Create "Connectivity" tab
            wndNodeConn.m_iAgentPort = (int)pObject->node.wAgentPort;
				wndNodeConn.m_bForceEncryption = (pObject->node.dwFlags & NF_FORCE_ENCRYPTION) ? TRUE : FALSE;
            wndNodeConn.m_strSnmpAuthName = CHECK_NULL_EX(pObject->node.pszAuthName);
				wndNodeConn.m_strSnmpAuthPassword = CHECK_NULL_EX(pObject->node.pszAuthPassword);
				wndNodeConn.m_strSnmpPrivPassword = CHECK_NULL_EX(pObject->node.pszPrivPassword);
            wndNodeConn.m_iAuthType = pObject->node.wAuthMethod;
            wndNodeConn.m_strSecret = pObject->node.szSharedSecret;
            wndNodeConn.m_iSNMPVersion = (int)pObject->node.nSNMPVersion;
				wndNodeConn.m_snmpPort = (int)pObject->node.wSnmpPort;
				wndNodeConn.m_iSnmpAuth = (int)pObject->node.wSnmpAuthMethod;
				wndNodeConn.m_iSnmpPriv = (int)pObject->node.wSnmpPrivMethod;
            wndNodeConn.m_dwProxyNode = pObject->node.dwProxyNode;
				wndNodeConn.m_dwSNMPProxy = pObject->node.dwSNMPProxy;
            wndPropSheet.AddPage(&wndNodeConn);

            // Create "Polling" tab
            wndNodePolling.m_dwPollerNode = pObject->node.dwPollerNode;
            wndNodePolling.m_bDisableAgent = (pObject->node.dwFlags & NF_DISABLE_NXCP) ? TRUE : FALSE;
            wndNodePolling.m_bDisableICMP = (pObject->node.dwFlags & NF_DISABLE_ICMP) ? TRUE : FALSE;
            wndNodePolling.m_bDisableSNMP = (pObject->node.dwFlags & NF_DISABLE_SNMP) ? TRUE : FALSE;
            wndNodePolling.m_bDisableConfPolls = (pObject->node.dwFlags & NF_DISABLE_CONF_POLL) ? TRUE : FALSE;
            wndNodePolling.m_bDisableDataCollection = (pObject->node.dwFlags & NF_DISABLE_DATA_COLLECT) ? TRUE : FALSE;
            wndNodePolling.m_bDisableRoutePolls = (pObject->node.dwFlags & NF_DISABLE_ROUTE_POLL) ? TRUE : FALSE;
            wndNodePolling.m_bDisableStatusPolls = (pObject->node.dwFlags & NF_DISABLE_STATUS_POLL) ? TRUE : FALSE;
				wndNodePolling.m_nUseIfXTable = pObject->node.nUseIfXTable;
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
				wndNetSrvGeneral.m_iRequiredPolls = pObject->netsrv.wRequiredPollCount;
            wndPropSheet.AddPage(&wndNetSrvGeneral);
            break;
         case OBJECT_INTERFACE:
            wndIfGeneral.m_pObject = pObject;
            wndIfGeneral.m_dwObjectId = dwObjectId;
            wndIfGeneral.m_strName = pObject->szName;
            wndIfGeneral.m_nRequiredPolls = pObject->iface.wRequiredPollCount;
            wndPropSheet.AddPage(&wndIfGeneral);
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
			case OBJECT_CLUSTER:
				wndClusterGeneral.m_pObject = pObject;
            wndClusterGeneral.m_dwObjectId = dwObjectId;
            wndClusterGeneral.m_strName = pObject->szName;
            wndPropSheet.AddPage(&wndClusterGeneral);

				wndClusterResources.m_pObject = pObject;
            wndPropSheet.AddPage(&wndClusterResources);
				break;
			case OBJECT_TEMPLATE:
            wndObjectGeneral.m_dwObjectId = dwObjectId;
            wndObjectGeneral.m_strClass = g_szObjectClass[pObject->iClass];
            wndObjectGeneral.m_strName = pObject->szName;
            wndPropSheet.AddPage(&wndObjectGeneral);

				wndTemplateAutoApply.m_bEnableAutoApply = pObject->dct.isAutoApplyEnabled;
				wndTemplateAutoApply.m_strFilterScript = CHECK_NULL_EX(pObject->dct.pszAutoApplyFilter);
            wndPropSheet.AddPage(&wndTemplateAutoApply);
				break;
			case OBJECT_CONTAINER:
            wndObjectGeneral.m_dwObjectId = dwObjectId;
            wndObjectGeneral.m_strClass = g_szObjectClass[pObject->iClass];
            wndObjectGeneral.m_strName = pObject->szName;
            wndPropSheet.AddPage(&wndObjectGeneral);

				wndContainerAutoBind.m_bEnableAutoBind = pObject->container.isAutoBindEnabled;
				wndContainerAutoBind.m_strFilterScript = CHECK_NULL_EX(pObject->container.pszAutoBindFilter);
            wndPropSheet.AddPage(&wndContainerAutoBind);
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

		// Create "Location" tab
		if ((pObject->iClass == OBJECT_NODE) || (pObject->iClass == OBJECT_CONTAINER))
		{
			GeoLocation loc(pObject->geolocation.type, pObject->geolocation.latitude, pObject->geolocation.longitude);
			wndGeoLocation.m_locationType = pObject->geolocation.type;
			if (pObject->geolocation.type != GL_UNSET)
			{
				wndGeoLocation.m_strLatitude = loc.getLatitudeAsString();
				wndGeoLocation.m_strLongitude = loc.getLongitudeAsString();
			}
         wndPropSheet.AddPage(&wndGeoLocation);
		}

      // Create "Security" tab
      wndObjectSecurity.m_pObject = pObject;
      wndObjectSecurity.m_bInheritRights = pObject->bInheritRights;
      wndPropSheet.AddPage(&wndObjectSecurity);

      // Create "Trusted Nodes" tab
      wndObjectTrustedNodes.m_dwNumNodes = pObject->dwNumTrustedNodes;
      wndObjectTrustedNodes.m_pdwNodeList = pObject->pdwTrustedNodes;
      wndPropSheet.AddPage(&wndObjectTrustedNodes);

      // Create "Custom Attributes" tab
      wndCustomAttrs.m_pObject = pObject;
      wndPropSheet.AddPage(&wndCustomAttrs);

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

   pWnd = (CDataCollectionEditor *)FindObjectView(OV_DC_EDITOR, pObject->dwId);
   if (pWnd == NULL)
   {
      dwResult = DoRequestArg3(NXCOpenNodeDCIList, g_hSession, (void *)pObject->dwId, 
                               &pItemList, _T("Loading node's data collection information..."));
      if (dwResult == RCC_SUCCESS)
      {
         CreateChildFrameWithSubtitle(new CDataCollectionEditor(pItemList), IDR_DC_EDITOR,
                                      pObject->szName, m_hDCEditorMenu, m_hDCEditorAccel);
      }
      else
      {
         ErrorBox(dwResult, _T("Unable to load data collection information for node:\n%s"));
      }
   }
   else
   {
      // Data collection editor already open, activate it
      pWnd->BringWindowToTop();
   }
}


//
// Open object's comment editor
//

void CConsoleApp::ShowObjectComments(NXC_OBJECT *pObject)
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);
   CObjectCommentsEditor *pWnd;

   pWnd = (CObjectCommentsEditor *)FindObjectView(OV_OBJECT_COMMENTS, pObject->dwId);
   if (pWnd == NULL)
   {
      CreateChildFrameWithSubtitle(new CObjectCommentsEditor(pObject->dwId),
                                   IDR_OBJECT_COMMENTS_EDITOR,
                                   pObject->szName, m_hObjectCommentsMenu, m_hObjectCommentsAccel);
   }
   else
   {
      // Comment window already open, activate it
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
                            (void *)bIsManaged, _T("Changing object status..."));
   if (dwResult != RCC_SUCCESS)
   {
      TCHAR szBuffer[256];

      _sntprintf_s(szBuffer, 256, _TRUNCATE, _T("Unable to change management status for object %s:\n%s"), 
                   pObject->szName, NXCGetErrorText(dwResult));
      GetMainWnd()->MessageBox(szBuffer, _T("Error"), MB_ICONSTOP);
   }
}


//
// Find open data collection editor for given node, if any
//

CMDIChildWnd *CConsoleApp::FindObjectView(DWORD dwClass, DWORD dwId)
{
   DWORD i;

   for(i = 0; i < MAX_OBJECT_VIEWS; i++)
      if ((m_openObjectViews[i].dwClass == dwClass) &&
          (m_openObjectViews[i].dwId == dwId))
         return m_openObjectViews[i].pWnd;
   return NULL;
}


//
// Display message box with error text from client library
//

void CConsoleApp::ErrorBox(DWORD dwError, TCHAR *pszMessage, TCHAR *pszTitle)
{
   TCHAR szBuffer[1024];

   if (!m_bIgnoreErrors)
   {
      if (dwError == RCC_COMPONENT_LOCKED)
      {
         TCHAR szTemp[300], szLock[256];

         NXCGetLastLockOwner(g_hSession, szLock, 256);
         _sntprintf(szTemp, 300, _T("Already locked by %s"), szLock);
         _sntprintf(szBuffer, 1024, (pszMessage != NULL) ? pszMessage : _T("Error: %s"), szTemp);
      }
      else
      {
         _sntprintf(szBuffer, 1024, (pszMessage != NULL) ? pszMessage : _T("Error: %s"), 
                    NXCGetErrorText(dwError));
      }
      m_pMainWnd->MessageBox(szBuffer, (pszTitle != NULL) ? pszTitle : _T("Error"), MB_ICONSTOP);
   }
}


//
// Show window with DCI's data
//

CMDIChildWnd *CConsoleApp::ShowDCIData(DWORD dwNodeId, DWORD dwItemId,
                                       TCHAR *pszItemName, TCHAR *pszParams)
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
                                pWnd->GetItemName(), m_hDataViewMenu, m_hDataViewAccel);
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
      dwCurrTime = (DWORD)time(NULL);
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
         ErrorBox(dwResult, _T("Unable to load event processing policy:\n%s"));
      }
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
	const TCHAR *tz;
	TCHAR buffer[256];

	CPropertySheet wndPropSheet(_T("Settings"), GetMainWnd(), 0);
   CConsolePropsGeneral wndGeneral;

   // "General" page
   wndGeneral.m_bExpandCtrlPanel = (g_dwOptions & UI_OPT_EXPAND_CTRLPANEL) ? TRUE : FALSE;
   wndGeneral.m_bShowGrid = (g_dwOptions & UI_OPT_SHOW_GRID) ? TRUE : FALSE;
   wndGeneral.m_bSaveGraphSettings = (g_dwOptions & UI_OPT_SAVE_GRAPH_SETTINGS) ? TRUE : FALSE;
	wndGeneral.m_strTimeZone = m_strCustomTimeZone;
	wndGeneral.m_nTimeZoneType = m_nTimeZoneType;
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

      if (wndGeneral.m_bSaveGraphSettings)
         g_dwOptions |= UI_OPT_SAVE_GRAPH_SETTINGS;
      else
         g_dwOptions &= ~UI_OPT_SAVE_GRAPH_SETTINGS;

		m_strCustomTimeZone = wndGeneral.m_strTimeZone;
		m_nTimeZoneType = wndGeneral.m_nTimeZoneType;

		switch(m_nTimeZoneType)
		{
			case TZ_TYPE_LOCAL:
				_tputenv(_T("TZ="));
				break;
			case TZ_TYPE_SERVER:
				tz = NXCGetServerTimeZone(g_hSession);
				if (tz != NULL)
				{
					_sntprintf(buffer, 256, _T("TZ=%s"), tz);
					_tputenv(buffer);
				}
				else
				{
					m_pMainWnd->MessageBox(_T("Unable to switch to server's time zone because server does not provide time zone information"), _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
				}
				break;
			case TZ_TYPE_CUSTOM:
				_sntprintf(buffer, 256, _T("TZ=%s"), (LPCTSTR)m_strCustomTimeZone);
				_tputenv(buffer);
				break;
			default:
				break;
		}
		_tzset();

		WriteProfileInt(_T("General"), _T("TimeZoneType"), m_nTimeZoneType);
		WriteProfileString(_T("General"), _T("TimeZone"), m_strCustomTimeZone);
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
      pFrame->CreateNewChild(RUNTIME_CLASS(CActionEditor), IDR_ACTION_EDITOR,
                             m_hActionEditorMenu, m_hActionEditorAccel);
   }
}


//
// Network discovery config
//

class ND_CONFIG
{
public:
   CString strScript;
   DWORD dwFlags;
   BOOL bEnable;
   BOOL bActive;
   DWORD dwNumTargets;
   NXC_ADDR_ENTRY *pTargetList;
   DWORD dwNumFilters;
   NXC_ADDR_ENTRY *pFilterList;
   CString strCommunity;
	DWORD dwNumStrings;
	TCHAR **ppStringList;

	ND_CONFIG()
	{
		bActive = FALSE;
		bEnable = FALSE;
		dwFlags = 0;
		dwNumFilters = 0;
		pFilterList = NULL;
		dwNumTargets = 0;
		pTargetList = NULL;
		strScript = _T("");
		strCommunity = _T("public");
		dwNumStrings = 0;
		ppStringList = NULL;
	}
	~ND_CONFIG()
	{
		safe_free(pTargetList);
		safe_free(pFilterList);
		for(DWORD i = 0; i < dwNumStrings; i++)
			safe_free(ppStringList[i]);
		safe_free(ppStringList);
	}
};


//
// Read network discovery configuration
//

static DWORD GetDiscoveryConf(ND_CONFIG *pConf)
{
   DWORD i, dwNumVars, dwResult;
   NXC_SERVER_VARIABLE *pVarList;

   dwResult = NXCGetServerVariables(g_hSession, &pVarList, &dwNumVars);
   if (dwResult != RCC_SUCCESS)
		goto failure;

   for(i = 0; i < dwNumVars; i++)
   {
      if (!_tcsicmp(pVarList[i].szName, _T("RunNetworkDiscovery")))
      {
         pConf->bEnable = _tcstol(pVarList[i].szValue, NULL, 0) ? TRUE : FALSE;
      }
      else if (!_tcsicmp(pVarList[i].szName, _T("ActiveNetworkDiscovery")))
      {
         pConf->bActive = _tcstol(pVarList[i].szValue, NULL, 0) ? TRUE : FALSE;
      }
      else if (!_tcsicmp(pVarList[i].szName, _T("DiscoveryFilterFlags")))
      {
         pConf->dwFlags = _tcstoul(pVarList[i].szValue, NULL, 0);
      }
      else if (!_tcsicmp(pVarList[i].szName, _T("DiscoveryFilter")))
      {
         pConf->strScript = pVarList[i].szValue;
      }
      else if (!_tcsicmp(pVarList[i].szName, _T("DefaultCommunityString")))
      {
         pConf->strCommunity = pVarList[i].szValue;
      }
   }
   free(pVarList);

   dwResult = NXCGetAddrList(g_hSession, ADDR_LIST_DISCOVERY_TARGETS,
                             &pConf->dwNumTargets, &pConf->pTargetList);
   if (dwResult != RCC_SUCCESS)
		goto failure;

   dwResult = NXCGetAddrList(g_hSession, ADDR_LIST_DISCOVERY_FILTER,
                             &pConf->dwNumFilters, &pConf->pFilterList);
   if (dwResult != RCC_SUCCESS)
		goto failure;

	dwResult = NXCGetSnmpCommunityList(g_hSession, &pConf->dwNumStrings, &pConf->ppStringList);
   if (dwResult != RCC_SUCCESS)
		goto failure;

failure:
   return dwResult;
}


//
// Update network discovery configuration
//

static DWORD SetDiscoveryConf(ND_CONFIG *pConf)
{
   DWORD dwResult;
   TCHAR szBuffer[256];

   dwResult = NXCSetServerVariable(g_hSession, _T("RunNetworkDiscovery"), pConf->bEnable ? _T("1") : _T("0"));
   if (dwResult != RCC_SUCCESS)
      goto cleanup;

   dwResult = NXCSetServerVariable(g_hSession, _T("ActiveNetworkDiscovery"), pConf->bActive ? _T("1") : _T("0"));
   if (dwResult != RCC_SUCCESS)
      goto cleanup;

   _sntprintf_s(szBuffer, 256, _TRUNCATE, _T("%d"), pConf->dwFlags);
   dwResult = NXCSetServerVariable(g_hSession, _T("DiscoveryFilterFlags"), szBuffer);
   if (dwResult != RCC_SUCCESS)
      goto cleanup;

   dwResult = NXCSetServerVariable(g_hSession, _T("DiscoveryFilter"), (TCHAR *)((LPCTSTR)pConf->strScript));
   if (dwResult != RCC_SUCCESS)
      goto cleanup;

   dwResult = NXCSetServerVariable(g_hSession, _T("DefaultCommunityString"), (TCHAR *)((LPCTSTR)pConf->strCommunity));
   if (dwResult != RCC_SUCCESS)
      goto cleanup;

   dwResult = NXCSetAddrList(g_hSession, ADDR_LIST_DISCOVERY_TARGETS,
                             pConf->dwNumTargets, pConf->pTargetList);
   if (dwResult != RCC_SUCCESS)
      goto cleanup;

   dwResult = NXCSetAddrList(g_hSession, ADDR_LIST_DISCOVERY_FILTER,
                             pConf->dwNumFilters, pConf->pFilterList);
   if (dwResult != RCC_SUCCESS)
      goto cleanup;

	dwResult = NXCUpdateSnmpCommunityList(g_hSession, pConf->dwNumStrings, pConf->ppStringList);
   if (dwResult != RCC_SUCCESS)
      goto cleanup;

   dwResult = NXCResetServerComponent(g_hSession, SRV_COMPONENT_DISCOVERY_MGR);

cleanup:
   return dwResult;
}


//
// WM_COMMAND::ID_CONTROLPANEL_NETWORKDISCOVERY message handler
//

void CConsoleApp::OnControlpanelNetworkdiscovery() 
{
   CPropertySheet psh(_T("Network Discovery Configuration"), GetMainWnd(), 0);
   CDiscoveryPropGeneral pgGeneral;
   CDiscoveryPropAddrList pgAddrList;
   CDiscoveryPropTargets pgTargets;
   CDiscoveryPropCommunities pgCommunities;
   ND_CONFIG config;
   DWORD dwResult;

   dwResult = DoRequestArg1(GetDiscoveryConf, &config, _T("Reading network discovery configuration..."));
   if (dwResult == RCC_SUCCESS)
   {
      // "General"
      pgGeneral.m_nMode = config.bEnable ? (config.bActive ? 2 : 1) : 0;
      if (config.strScript.CompareNoCase(_T("none")) == 0)
      {
         pgGeneral.m_nFilter = 0;
      }
      else if (config.strScript.CompareNoCase(_T("auto")) == 0)
      {
         pgGeneral.m_nFilter = 2;
      }
      else
      {
         pgGeneral.m_nFilter = 1;
         pgGeneral.m_strScript = config.strScript;
      }
      pgGeneral.m_bAllowAgent = config.dwFlags & DFF_ALLOW_AGENT ? TRUE : FALSE;
      pgGeneral.m_bAllowSNMP = config.dwFlags & DFF_ALLOW_SNMP ? TRUE : FALSE;
      pgGeneral.m_bAllowRange = config.dwFlags & DFF_ONLY_RANGE ? TRUE : FALSE;
      pgGeneral.m_strCommunity = config.strCommunity;
      psh.AddPage(&pgGeneral);

      // "Address Filter"
      pgAddrList.m_dwAddrCount = config.dwNumFilters;
      pgAddrList.m_pAddrList = config.pFilterList;
      psh.AddPage(&pgAddrList);

      // "Active Discovery Targets"
      pgTargets.m_dwAddrCount = config.dwNumTargets;
      pgTargets.m_pAddrList = config.pTargetList;
      psh.AddPage(&pgTargets);

		// "SNMP Communities"
		pgCommunities.m_dwNumStrings = config.dwNumStrings;
		pgCommunities.m_ppStringList = config.ppStringList;
      psh.AddPage(&pgCommunities);

      if (psh.DoModal() == IDOK)
      {
         switch(pgGeneral.m_nMode)
         {
            case 0:
               config.bEnable = FALSE;
               config.bActive = FALSE;
               config.dwFlags = 0;
               config.dwNumFilters = 0;
               config.dwNumTargets = 0;
               break;
            case 1:
               config.bEnable = TRUE;
               config.bActive = FALSE;
               break;
            case 2:
               config.bEnable = TRUE;
               config.bActive = TRUE;
               break;
         }

         config.strCommunity = pgGeneral.m_strCommunity;
         switch(pgGeneral.m_nFilter)
         {
            case 0:
               config.strScript = _T("none");
               break;
            case 1:
               config.strScript = pgGeneral.m_strScript;
               break;
            case 2:
               config.strScript = _T("auto");
               break;
         }

         config.dwFlags = 0;
         if (pgGeneral.m_bAllowAgent)
            config.dwFlags |= DFF_ALLOW_AGENT;
         if (pgGeneral.m_bAllowSNMP)
            config.dwFlags |= DFF_ALLOW_SNMP;
         if (pgGeneral.m_bAllowRange)
            config.dwFlags |= DFF_ONLY_RANGE;

         config.dwNumFilters = pgAddrList.m_dwAddrCount;
         config.pFilterList = pgAddrList.m_pAddrList;

         config.dwNumTargets = pgTargets.m_dwAddrCount;
         config.pTargetList = pgTargets.m_pAddrList;

			config.dwNumStrings = pgCommunities.m_dwNumStrings;
			config.ppStringList = pgCommunities.m_ppStringList;

         dwResult = DoRequestArg1(SetDiscoveryConf, &config, _T("Updating network discovery configuration..."));
         if (dwResult != RCC_SUCCESS)
         {
            ErrorBox(dwResult, _T("Cannot update network discovery configuration: %s"));
         }
      }
   }
   else
   {
      ErrorBox(dwResult, _T("Cannot read network discovery configuration: %s"));
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
      pFrame->CreateNewChild(RUNTIME_CLASS(CTrapEditor), IDR_TRAP_EDITOR,
                             m_hTrapEditorMenu, m_hTrapEditorAccel);
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

      dwResult = DoRequestArg1(NXCLockPackageDB, g_hSession, _T("Locking package database..."));
      if (dwResult == RCC_SUCCESS)
      {
	      pFrame->CreateNewChild(RUNTIME_CLASS(CPackageMgr), IDR_PACKAGE_MGR,
                                m_hPackageMgrMenu, m_hPackageMgrAccel);
      }
      else
      {
         ErrorBox(dwResult, _T("Unable to lock package database:\n%s"));
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
      InsertMenu(hMenu, ID_VIEW_DEBUG, MF_BYCOMMAND | MF_POPUP, (UINT_PTR)GetSubMenu(hViewMenu, 8), _T("&Control panel"));
   }

   return hMenu;
}


//
// Create new object
//

void CConsoleApp::CreateObject(NXC_OBJECT_CREATE_INFO *pInfo)
{
   DWORD dwResult, dwObjectId;

   dwResult = DoRequestArg3(NXCCreateObject, g_hSession, pInfo, &dwObjectId, _T("Creating object..."));
   if (dwResult != RCC_SUCCESS)
   {
      ErrorBox(dwResult, _T("Error creating object: %s"));
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
		ci.pszComments = (TCHAR *)((LPCTSTR)dlg.m_strDescription);
      ci.cs.container.dwCategory = 1;
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
		ci.pszComments = NULL;
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
          (dlg.m_pParentObject->iClass != OBJECT_SERVICEROOT) &&
			 (dlg.m_pParentObject->iClass != OBJECT_CLUSTER))
         dlg.m_pParentObject = NULL;
   if (dlg.DoModal() == IDOK)
   {
		memset(&ci, 0, sizeof(NXC_OBJECT_CREATE_INFO));
      ci.dwParentId = (dlg.m_pParentObject != NULL) ? dlg.m_pParentObject->dwId : 0;
      ci.iClass = OBJECT_NODE;
      ci.pszName = (TCHAR *)((LPCTSTR)dlg.m_strObjectName);
		ci.pszComments = NULL;
      ci.cs.node.dwIpAddr = dlg.m_dwIpAddr;
      ci.cs.node.dwNetMask = 0;
      ci.cs.node.dwProxyNode = dlg.m_dwProxyNode;
		ci.cs.node.dwSNMPProxy = dlg.m_dwSNMPProxy;
      ci.cs.node.dwCreationFlags = 0;
      if (dlg.m_bCreateUnmanaged)
         ci.cs.node.dwCreationFlags |= NXC_NCF_CREATE_UNMANAGED;
      if (dlg.m_bDisableAgent)
         ci.cs.node.dwCreationFlags |= NXC_NCF_DISABLE_NXCP;
      if (dlg.m_bDisableICMP)
         ci.cs.node.dwCreationFlags |= NXC_NCF_DISABLE_ICMP;
      if (dlg.m_bDisableSNMP)
         ci.cs.node.dwCreationFlags |= NXC_NCF_DISABLE_SNMP;
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
		ci.pszComments = NULL;
      ci.cs.netsrv.iServiceType = dlg.m_iServiceType;
      ci.cs.netsrv.wPort = (WORD)dlg.m_iPort;
      ci.cs.netsrv.wProto = (WORD)dlg.m_iProtocolNumber;
      ci.cs.netsrv.pszRequest = (TCHAR *)((LPCTSTR)dlg.m_strRequest);
      ci.cs.netsrv.pszResponse = (TCHAR *)((LPCTSTR)dlg.m_strResponse);
      CreateObject(&ci);
   }
}


//
// Create cluster object
//

void CConsoleApp::CreateCluster(DWORD dwParent)
{
   NXC_OBJECT_CREATE_INFO ci;
   CCreateClusterDlg dlg;

   dlg.m_iObjectClass = OBJECT_CLUSTER;
   dlg.m_pParentObject = NXCFindObjectById(g_hSession, dwParent);
   if (dlg.m_pParentObject != NULL)
      if ((dlg.m_pParentObject->iClass != OBJECT_CONTAINER) &&
          (dlg.m_pParentObject->iClass != OBJECT_SERVICEROOT))
         dlg.m_pParentObject = NULL;
   if (dlg.DoModal() == IDOK)
   {
      ci.dwParentId = (dlg.m_pParentObject != NULL) ? dlg.m_pParentObject->dwId : 0;
      ci.iClass = OBJECT_CLUSTER;
      ci.pszName = (TCHAR *)((LPCTSTR)dlg.m_strObjectName);
		ci.pszComments = NULL;
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
      ci.pszName = (TCHAR *)((LPCTSTR)dlg.m_strObjectName);
		ci.pszComments = NULL;
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
      ci.pszName = (TCHAR *)((LPCTSTR)dlg.m_strObjectName);
		ci.pszComments = (TCHAR *)((LPCTSTR)dlg.m_strDescription);
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
      ci.pszName = (TCHAR *)((LPCTSTR)dlg.m_strObjectName);
		ci.pszComments = NULL;
      CreateObject(&ci);
   }
}


//
// Delete object on server
//

void CConsoleApp::DeleteNetXMSObject(NXC_OBJECT *pObject)
{
   DWORD dwResult;

   dwResult = DoRequestArg2(NXCDeleteObject, g_hSession, (void *)pObject->dwId, _T("Deleting object..."));
   if (dwResult != RCC_SUCCESS)
      ErrorBox(dwResult, _T("Unable to delete object: %s"));
}


//
// Perform forced poll on a node
//

void CConsoleApp::PollNode(DWORD dwObjectId, int iPollType)
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);
   CNodePoller *pWnd;

	pWnd = (CNodePoller *)pFrame->CreateNewChild(
		RUNTIME_CLASS(CNodePoller), IDR_NODE_POLLER, m_hNodePollerMenu, m_hNodePollerAccel);
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

   dlg.m_dwAllowedClasses = SCL_NODE | SCL_CONTAINER | SCL_SUBNET | SCL_ZONE | SCL_CLUSTER | SCL_CONDITION;
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

   dlg.m_dwAllowedClasses = SCL_NODE | SCL_CONTAINER | SCL_SUBNET | SCL_ZONE | SCL_CLUSTER | SCL_CONDITION;
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
      ErrorBox(RCC_NOT_IMPLEMENTED, _T("Error changing IP address for node: %s"));
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
#ifdef UNICODE
   WCHAR wszTemp[128];
#endif

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
                  _snprintf_s(szBuffer, MAX_DB_STRING, _TRUNCATE, "%lu", pRow->dwTimeStamp);
                  break;
               case 1:  // Text
#ifdef UNICODE
                  FormatTimeStamp(pRow->dwTimeStamp, wszTemp, TS_LONG_DATE_TIME);
                  WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, 
                                      wszTemp, -1, szBuffer, MAX_DB_STRING, NULL, NULL);
#else
                  FormatTimeStamp(pRow->dwTimeStamp, szBuffer, TS_LONG_DATE_TIME);
#endif
                  break;
               default:
                  strcpy(szBuffer, "<internal error>");
                  break;
            }
            _write(hFile, szBuffer, (unsigned int)strlen(szBuffer));
            _write(hFile, &separator[iSeparator], 1);
            switch(pData->wDataType)
            {
               case DCI_DT_STRING:
                  _write(hFile, pRow->value.szString, (unsigned int)_tcslen(pRow->value.szString));
                  break;
               case DCI_DT_INT:
                  _snprintf_s(szBuffer, MAX_DB_STRING, _TRUNCATE, "%d", pRow->value.dwInt32);
                  _write(hFile, szBuffer, (unsigned int)strlen(szBuffer));
                  break;
               case DCI_DT_UINT:
                  _snprintf_s(szBuffer, MAX_DB_STRING, _TRUNCATE, "%u", pRow->value.dwInt32);
                  _write(hFile, szBuffer, (unsigned int)strlen(szBuffer));
                  break;
               case DCI_DT_INT64:
                  _snprintf_s(szBuffer, MAX_DB_STRING, _TRUNCATE, "%I64d", pRow->value.qwInt64);
                  _write(hFile, szBuffer, (unsigned int)strlen(szBuffer));
                  break;
               case DCI_DT_UINT64:
                  _snprintf_s(szBuffer, MAX_DB_STRING, _TRUNCATE, "%I64u", pRow->value.qwInt64);
                  _write(hFile, szBuffer, (unsigned int)strlen(szBuffer));
                  break;
               case DCI_DT_FLOAT:
                  _snprintf_s(szBuffer, MAX_DB_STRING, _TRUNCATE, "%f", pRow->value.dFloat);
                  _write(hFile, szBuffer, (unsigned int)strlen(szBuffer));
                  break;
               default:
                  break;
            }
            _write(hFile, "\r\n", 2);
            inc_ptr(pRow, pData->wRowSize, NXC_DCI_ROW);
         }
         _close(hFile);
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

   if (g_pObjectToolList[dwIndex].dwFlags & TF_ASK_CONFIRMATION)
   {
      TCHAR szBuffer[4096];

      if ((g_pObjectToolList[dwIndex].pszConfirmationText != NULL) &&
          (*g_pObjectToolList[dwIndex].pszConfirmationText != 0))
      {
         _sntprintf_s(szBuffer, 4096, _TRUNCATE, _T("%lu"), pObject->dwId);
         SetEnvironmentVariable(_T("OBJECT_ID"), szBuffer);
         SetEnvironmentVariable(_T("OBJECT_IP_ADDR"), IpToStr(pObject->dwIpAddr, szBuffer));
         SetEnvironmentVariable(_T("OBJECT_NAME"), pObject->szName);
         ExpandEnvironmentStrings(g_pObjectToolList[dwIndex].pszConfirmationText, szBuffer, 4096);
      }
      else
      {
         _tcscpy(szBuffer, _T("This tool requires confirmation but does not provide confirmation message. Do you wish to run this tool?"));
      }

      if (m_pMainWnd->MessageBox(szBuffer, _T("Warning"), MB_YESNO | MB_ICONEXCLAMATION) != IDYES)
         return;
   }

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
      case TOOL_TYPE_SERVER_COMMAND:
			dwResult = DoRequestArg3(NXCExecuteServerCommand, g_hSession, (void *)pObject->dwId, g_pObjectToolList[dwIndex].pszData, _T("Executing command on server..."));
			if (dwResult == RCC_SUCCESS)
            m_pMainWnd->MessageBox(_T("Command executed successfully"), _T("Information"), MB_OK | MB_ICONINFORMATION);
			else
				ErrorBox(dwResult, _T("Error executing command on server: %s"));
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
}


//
// Execute web tool (open specified URL)
//

void CConsoleApp::ExecuteWebTool(NXC_OBJECT *pObject, TCHAR *pszURL)
{
   TCHAR szRealURL[4096];

   _sntprintf_s(szRealURL, 4096, _TRUNCATE, _T("%lu"), pObject->dwId);
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
   _sntprintf_s(szCmdLine, 4096, _TRUNCATE, _T("%lu"), pObject->dwId);
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
	NXC_OBJECT *object;

	object = NXCFindObjectById(g_hSession, dwObjectId);
   dlg.m_dwAllowedClasses = (object->iClass == OBJECT_TEMPLATE) ? (SCL_TEMPLATEGROUP | SCL_TEMPLATEROOT) : (SCL_CONTAINER | SCL_SERVICEROOT);
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
// WM_COMMAND::ID_CONTROLPANEL_AGENTCONFIGS
//

void CConsoleApp::OnControlpanelAgentconfigs() 
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);

	// create a new MDI child window or open existing
   if (m_viewState[VIEW_AGENT_CONFIG_MANAGER].bActive)
   {
      m_viewState[VIEW_AGENT_CONFIG_MANAGER].pWnd->BringWindowToTop();
   }
   else
   {
	   pFrame->CreateNewChild(RUNTIME_CLASS(CAgentConfigMgr), IDR_AGENT_CONFIG_MANAGER,
                             m_hAgentCfgMgrMenu, m_hAgentCfgMgrAccel);
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
   CPasswordChangeDlg dlg(IDD_CHANGE_PASSWORD_CONFIRM);

   if (dlg.DoModal() == IDOK)
   {
      DWORD dwResult;

      dwResult = DoRequestArg4(NXCSetPassword, g_hSession, 
                               (void *)NXCGetCurrentUserId(g_hSession),
                               dlg.m_szPassword, dlg.m_szOldPassword, _T("Changing password..."));
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


//
// Show details window
//

void CConsoleApp::ShowDetailsWindow(DWORD dwType, HWND hwndOrigin, Table *pData)
{
   CDetailsView *pWnd;

	// create a new MDI child window or open existing
   if (m_viewState[VIEW_ALARM_DETAILS].bActive)
   {
      m_viewState[VIEW_ALARM_DETAILS].pWnd->BringWindowToTop();
   }
   else
   {
   	CMainFrame *pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);

      pWnd = new CDetailsView(dwType, 400, 500, pData, NULL);
      CreateChildFrameWithSubtitle(pWnd, IDR_ALARM_DETAILS, _T("AAA"),
                                   m_hMDIMenu, m_hMDIAccel);
   }
}


//
// (Re)load alarm list
//

DWORD CConsoleApp::LoadAlarms()
{
   m_dwNumAlarms = 0;
   safe_free(m_pAlarmList);
   return NXCLoadAllAlarms(g_hSession, FALSE, &m_dwNumAlarms, &m_pAlarmList);
}


//
// Find alarm record in internal list
//

NXC_ALARM *CConsoleApp::FindAlarmInList(DWORD dwAlarmId)
{
   DWORD i;

   for(i = 0; i < m_dwNumAlarms; i++)
      if (m_pAlarmList[i].dwAlarmId == dwAlarmId)
         return &m_pAlarmList[i];
   return NULL;
}


//
// Add new alarm to internal list
//

void CConsoleApp::AddAlarmToList(NXC_ALARM *pAlarm)
{
   m_pAlarmList = (NXC_ALARM *)realloc(m_pAlarmList, sizeof(NXC_ALARM) * (m_dwNumAlarms + 1));
   memcpy(&m_pAlarmList[m_dwNumAlarms], pAlarm, sizeof(NXC_ALARM));
   m_dwNumAlarms++;
   //m_iNumAlarms[pAlarm->nCurrentSeverity]++;
}


//
// Delete alarm from internal alarms list
//

void CConsoleApp::DeleteAlarmFromList(DWORD dwAlarmId)
{
   DWORD i;

   for(i = 0; i < m_dwNumAlarms; i++)
      if (m_pAlarmList[i].dwAlarmId == dwAlarmId)
      {
         PlayAlarmSound(&m_pAlarmList[i], FALSE, g_hSession, &g_soundCfg);
         m_dwNumAlarms--;
         memmove(&m_pAlarmList[i], &m_pAlarmList[i + 1],
                 sizeof(NXC_ALARM) * (m_dwNumAlarms - i));
         break;
      }
}


//
// Handler for alarm updates
//

void CConsoleApp::OnAlarmUpdate(DWORD dwCode, NXC_ALARM *pAlarm)
{
   NXC_ALARM *pListItem;

   MutexLock(m_mutexAlarmList, INFINITE);
   switch(dwCode)
   {
      case NX_NOTIFY_NEW_ALARM:
         if (pAlarm->nState != ALARM_STATE_TERMINATED)
         {
            AddAlarmToList(pAlarm);
            PlayAlarmSound(pAlarm, TRUE, g_hSession, &g_soundCfg);
         }
         break;
      case NX_NOTIFY_ALARM_CHANGED:
         pListItem = FindAlarmInList(pAlarm->dwAlarmId);
         if (pListItem != NULL)
         {
//            m_iNumAlarms[pListItem->nCurrentSeverity]--;
//            m_iNumAlarms[pAlarm->nCurrentSeverity]++;
            memcpy(pListItem, pAlarm, sizeof(NXC_ALARM));
         }
         break;
      case NX_NOTIFY_ALARM_TERMINATED:
      case NX_NOTIFY_ALARM_DELETED:
         DeleteAlarmFromList(pAlarm->dwAlarmId);
//         m_iNumAlarms[pAlarm->nCurrentSeverity]--;
         break;
      default:
         break;
   }
   MutexUnlock(m_mutexAlarmList);
}


//
// Open (and lock) alarm list
//

DWORD CConsoleApp::OpenAlarmList(NXC_ALARM **ppList)
{
   MutexLock(m_mutexAlarmList, INFINITE);
   *ppList = m_pAlarmList;
   return m_dwNumAlarms;
}


//
// Close alarm list
//

void CConsoleApp::CloseAlarmList()
{
   MutexUnlock(m_mutexAlarmList);
}


//
// "Tools -> Create management pack..." menu handler
//

void CConsoleApp::OnToolsCreatemp() 
{
   CCreateMPDlg dlg;
   DWORD dwResult;
   HANDLE hFile;
   TCHAR *pszContent;

   if (dlg.DoModal() != IDOK)
      return;

   dwResult = DoRequestArg9(NXCExportConfiguration, g_hSession, (void *)((LPCTSTR)dlg.m_strDescription),
                            (void *)dlg.m_dwNumEvents, dlg.m_pdwEventList,
                            (void *)dlg.m_dwNumTemplates, dlg.m_pdwTemplateList,
                            (void *)dlg.m_dwNumTraps, dlg.m_pdwTrapList,
                            &pszContent, _T("Exporting configuration..."));
   if (dwResult == RCC_SUCCESS)
   {
      hFile = CreateFile(dlg.m_strFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
      if (hFile != INVALID_HANDLE_VALUE)
      {
#ifdef UNICODE
         char *pszText;
         int nLen;

         nLen = (int)wcslen(pszContent);
         pszText = (char *)malloc(nLen * 2 + 1);
         WideCharToMultiByte(CP_UTF8, 0, pszContent, -1, pszText, nLen * 2 + 1, NULL, NULL);
         WriteFile(hFile, pszText, (DWORD)strlen(pszText), &dwResult, NULL);
         free(pszText);
#else
         WriteFile(hFile, pszContent, _tcslen(pszContent), &dwResult, NULL);
#endif
         CloseHandle(hFile);

         m_pMainWnd->MessageBox(_T("Configuration was successfully exported"), _T("Success"), MB_OK | MB_ICONINFORMATION);
      }
      else
      {
         TCHAR szBuffer[1024];

         _tcscpy(szBuffer, _T("Cannot create output file: "));
         GetSystemErrorText(GetLastError(), &szBuffer[27], 990);
         m_pMainWnd->MessageBox(szBuffer, _T("Error"), MB_OK | MB_ICONSTOP);
      }
      free(pszContent);
   }
   else
   {
      ErrorBox(dwResult, _T("Cannot create management pack: %s"));
   }
}


//
// "File -> Page Setup..." menu handler
//

void CConsoleApp::OnFilePagesetup() 
{
   CPrintDialog dlg(TRUE);

   dlg.m_pd.hDevMode = CopyGlobalMem(m_hDevMode);
   dlg.m_pd.hDevNames = CopyGlobalMem(m_hDevNames);

   if (dlg.DoModal() == IDOK)
   {
      SafeGlobalFree(m_hDevMode);
      SafeGlobalFree(m_hDevNames);
      m_hDevMode = CopyGlobalMem(dlg.m_pd.hDevMode);
      m_hDevNames = CopyGlobalMem(dlg.m_pd.hDevNames);
      WriteProfileGMem(_T("Printer"), _T("DevMode"), m_hDevMode);
      WriteProfileGMem(_T("Printer"), _T("DevNames"), m_hDevNames);
   }

   SafeGlobalFree(dlg.m_pd.hDevMode);
   SafeGlobalFree(dlg.m_pd.hDevNames);
}


//
// Write global memory block to registry
//

void CConsoleApp::WriteProfileGMem(TCHAR *pszSection, TCHAR *pszKey, HGLOBAL hMem)
{
   void *p;

   p = GlobalLock(hMem);
   WriteProfileBinary(pszSection, pszKey, (BYTE *)p, (UINT)GlobalSize(hMem));
   GlobalUnlock(hMem);
}


//
// Read global memory block from registry
//

HGLOBAL CConsoleApp::GetProfileGMem(TCHAR *pszSection, TCHAR *pszKey)
{
   BYTE *pData, *pMem;
   UINT nSize;
   HGLOBAL hMem = NULL;

   if (GetProfileBinary(pszSection, pszKey, &pData, &nSize))
   {
      hMem = GlobalAlloc(GMEM_MOVEABLE, nSize);
      if (hMem != NULL)
      {
         pMem = (BYTE *)GlobalLock(hMem);
         CopyMemory(pMem, pData, nSize);
         GlobalUnlock(hMem);
      }
      delete pData;
   }
   return hMem;
}


//
// "Tools -> Create management pack..." menu handler
//

void CConsoleApp::OnToolsImportmp() 
{
   CSelectMPDlg dlg;
   DWORD dwResult, dwSize, dwSizeHigh, dwFlags;
   TCHAR *pszContent, szErrorText[1024];
   HANDLE hFile;

   dwFlags = GetProfileInt(_T("MPImportDialog"), _T("Flags"), 0);
   dlg.m_strFile = GetProfileString(_T("MPImportDialog"), _T("FileName"), _T(""));
   if (dwFlags & CFG_IMPORT_REPLACE_EVENT_BY_CODE)
      dlg.m_bReplaceEventByCode = TRUE;
   if (dwFlags & CFG_IMPORT_REPLACE_EVENT_BY_NAME)
      dlg.m_bReplaceEventByName = TRUE;
   if (dlg.DoModal() == IDOK)
   {
      dwFlags = 0;
      if (dlg.m_bReplaceEventByCode)
         dwFlags |= CFG_IMPORT_REPLACE_EVENT_BY_CODE;
      if (dlg.m_bReplaceEventByName)
         dwFlags |= CFG_IMPORT_REPLACE_EVENT_BY_NAME;
      WriteProfileInt(_T("MPImportDialog"), _T("Flags"), dwFlags);
      WriteProfileString(_T("MPImportDialog"), _T("FileName"), (LPCTSTR)dlg.m_strFile);

      hFile = CreateFile((LPCTSTR)dlg.m_strFile, GENERIC_READ, FILE_SHARE_READ,
                         NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
      if (hFile != INVALID_HANDLE_VALUE)
      {
         dwSize = GetFileSize(hFile, &dwSizeHigh);
         if ((dwSizeHigh == 0) && (dwSize <= 4194304))
         {
            pszContent = (TCHAR *)malloc((dwSize + 1) * sizeof(TCHAR));
            memset(pszContent, 0, (dwSize + 1) * sizeof(TCHAR));
#ifdef UNICODE
            char *pTemp = (char *)malloc(dwSize);
            ReadFile(hFile, pTemp, dwSize, &dwResult, NULL);
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pTemp, dwSize, pszContent, dwSize);
            free(pTemp);
#else
            ReadFile(hFile, pszContent, dwSize, &dwResult, NULL);
#endif
         }
         else
         {
            m_pMainWnd->MessageBox(_T("Input file is too large"), _T("Error"), MB_OK | MB_ICONSTOP);
            pszContent = NULL;
         }
         CloseHandle(hFile);
         if (pszContent != NULL)
         {
            dwResult = DoRequestArg5(NXCImportConfiguration, g_hSession, pszContent,
                                     (void *)dwFlags, szErrorText,
                                     (void *)1024, _T("Importing configuration..."));
            free(pszContent);
            if (dwResult == RCC_SUCCESS)
            {
               m_pMainWnd->MessageBox(_T("Configuration successfully imported"), _T("Information"), MB_OK | MB_ICONINFORMATION);
            }
            else
            {
               if (dwResult == RCC_CONFIG_PARSE_ERROR)
               {
                  TCHAR szBuffer[2048];

                  _sntprintf(szBuffer, 2048, _T("Error parsing management pack file:\n%s"), szErrorText);
                  m_pMainWnd->MessageBox(szBuffer, _T("Error"), MB_OK | MB_ICONSTOP);
               }
               else if (dwResult == RCC_CONFIG_VALIDATION_ERROR)
               {
                  TCHAR szBuffer[2048];

                  _sntprintf(szBuffer, 2048, _T("Management pack validation failed:\n%s"), szErrorText);
                  m_pMainWnd->MessageBox(szBuffer, _T("Error"), MB_OK | MB_ICONSTOP);
               }
               else
               {
                  ErrorBox(dwResult, _T("Cannot import management pack: %s"));
               }
            }
         }
      }
      else
      {
         TCHAR szBuffer[4096];

         _sntprintf(szBuffer, 4096, _T("Cannot open input file %s: %s"),
                    (LPCTSTR)dlg.m_strFile, 
                    GetSystemErrorText(GetLastError(), szErrorText, 1024));
         m_pMainWnd->MessageBox(szBuffer, _T("Error"), MB_OK | MB_ICONSTOP);
      }
   }
}


//
// Start console upgrade. Return TRUE if upgrade was started.
//

BOOL CConsoleApp::StartConsoleUpgrade()
{
	CConsoleUpgradeDlg dlg;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	TCHAR szLocalFile[MAX_PATH], szTempDir[MAX_PATH];
	DWORD dwResult;
	HANDLE hFile;
	BOOL bRet;

	dlg.m_strURL = g_szUpgradeURL;
	if (dlg.DoModal() != IDOK)
		return FALSE;

	dwResult = GetTempPath(MAX_PATH, szTempDir);
	if ((dwResult == 0) || (dwResult > MAX_PATH))
	{
		m_pMainWnd->MessageBox(_T("Cannot obtain name of temporary directory"), _T("Error"), MB_OK | MB_ICONSTOP);
		return FALSE;
	}

	if (GetTempFileName(szTempDir, _T("NXC"), 0xA00A, szLocalFile) == 0)
	{
		m_pMainWnd->MessageBox(_T("Cannot obtain name of temporary file"), _T("Error"), MB_OK | MB_ICONSTOP);
		return FALSE;
	}

	hFile = CreateFile(szLocalFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		bRet = DownloadUpgradeFile(hFile);
		CloseHandle(hFile);
		if (bRet)
		{
			TCHAR szArgs[256] = _T("ARG0 /SILENT /RUNCONSOLE");

			memset(&si, 0, sizeof(STARTUPINFO));
			si.cb = sizeof(STARTUPINFO);
			if (!CreateProcess(szLocalFile, szArgs, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
			{
				m_pMainWnd->MessageBox(_T("Cannot create process"), _T("Error"), MB_OK | MB_ICONSTOP);
				bRet = FALSE;
			}
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
		}
		if (!bRet)
			DeleteFile(szLocalFile);
	}
	else
	{
      TCHAR szBuffer[1024];

      _tcscpy(szBuffer, _T("Cannot create local file: "));
      GetSystemErrorText(GetLastError(), &szBuffer[26], 990);
      m_pMainWnd->MessageBox(szBuffer, _T("Error"), MB_OK | MB_ICONSTOP);
		bRet = FALSE;
	}
	return bRet;
}


//
// Open predefined graph
//

void CConsoleApp::OnPredefinedGraph(UINT nCmd)
{
   CGraphFrame *pWnd;
	UINT nIndex;

	nIndex = nCmd - GRAPH_MENU_FIRST_ID;
	if (nIndex < g_dwNumGraphs)
	{
		pWnd = new CGraphFrame;
		pWnd->RestoreFromServer(g_pGraphList[nIndex].pszConfig);
		CreateChildFrameWithSubtitle(pWnd, IDR_DCI_HISTORY_GRAPH,
											  pWnd->GetSubTitle(), m_hGraphMenu, m_hGraphAccel);
	}
}


//
// Create graph submenu
//

static void CreateGraphSubmenu(CMenu *pMenu, TCHAR *pszCurrPath, DWORD *pdwStart)
{
	DWORD i, j;
   TCHAR szName[MAX_DB_STRING], szPath[MAX_DB_STRING];
	int nPos; 

	for(i = *pdwStart, nPos = 0; i < g_dwNumGraphs; i++, nPos++)
	{
      // Separate item name and path
      memset(szPath, 0, sizeof(TCHAR) * MAX_DB_STRING);
      if (_tcsstr(g_pGraphList[i].pszName, _T("->")) != NULL)
      {
         for(j = (DWORD)_tcslen(g_pGraphList[i].pszName) - 1; j > 0; j--)
            if ((g_pGraphList[i].pszName[j] == _T('>')) &&
                (g_pGraphList[i].pszName[j - 1] == _T('-')))
            {
               _tcscpy(szName, &g_pGraphList[i].pszName[j + 1]);
               j--;
               break;
            }
         memcpy(szPath, g_pGraphList[i].pszName, j * sizeof(TCHAR));
      }
      else
      {
         _tcscpy(szName, g_pGraphList[i].pszName);
      }

		// Add menu item if we are on right level, otherwise create submenu or go one level up
      if (!_tcsicmp(szPath, pszCurrPath))
      {
         StrStrip(szName);
         pMenu->InsertMenu(nPos, MF_BYPOSITION, GRAPH_MENU_FIRST_ID + i, szName);
      }
      else
      {
         for(j = (DWORD)_tcslen(pszCurrPath); (szPath[j] == _T(' ')) || (szPath[j] == _T('\t')); j++);
         if ((*pszCurrPath == 0) ||
             ((!_memicmp(szPath, pszCurrPath, _tcslen(pszCurrPath) * sizeof(TCHAR))) &&
              (szPath[j] == _T('-')) && (szPath[j + 1] == _T('>'))))
         {
            CMenu *pSubMenu;
            TCHAR *pszName;

            // Submenu of current menu
            if (*pszCurrPath != 0)
               j += 2;
            pszName = &szPath[j];
            for(; (szPath[j] != 0) && (memcmp(&szPath[j], _T("->"), sizeof(TCHAR) * 2)); j++);
            szPath[j] = 0;
            pSubMenu = new CMenu;
				pSubMenu->CreatePopupMenu();
				CreateGraphSubmenu(pSubMenu, szPath, &i);
            StrStrip(pszName);
            pMenu->InsertMenu(nPos, MF_ENABLED | MF_STRING | MF_POPUP | MF_BYPOSITION,
                              (UINT)pSubMenu->Detach(), pszName);
            delete pSubMenu;
         }
         else
         {
            i--;
            break;
         }
		}
	}
   *pdwStart = i;
}


//
// Update Tools->Graph submenu
//

static void UpdateGraphSubmenu(CMenu *pToolsMenu)
{
	CMenu *pMenu;
	DWORD i;

	if (pToolsMenu == NULL)
		return;

	pMenu = pToolsMenu->GetSubMenu(3);
	if (pMenu != NULL)
	{
		// Clear menu
		while(pMenu->GetMenuItemCount() > 2)
			pMenu->DeleteMenu(0, MF_BYPOSITION);

		// Add new items
		if (g_dwNumGraphs > 0)
		{
			i = 0;
			CreateGraphSubmenu(pMenu, _T(""), &i);
		}
		else
		{
			pMenu->InsertMenu(0, MF_BYPOSITION, ID_ALWAYS_DISABLED, _T("<no graphs defined>"));
		}
	}
}


//
// Callback for qsort() which compares graph names
//

static int CompareGraphNames(const void *elem1, const void *elem2)
{
	int nRet;

	TranslateStr(((NXC_GRAPH *)elem1)->pszName, _T("->"), _T("\x01\x02"));
	TranslateStr(((NXC_GRAPH *)elem2)->pszName, _T("->"), _T("\x01\x02"));
	nRet = _tcsicmp(((NXC_GRAPH *)elem1)->pszName, ((NXC_GRAPH *)elem2)->pszName);
	TranslateStr(((NXC_GRAPH *)elem1)->pszName, _T("\x01\x02"), _T("->"));
	TranslateStr(((NXC_GRAPH *)elem2)->pszName, _T("\x01\x02"), _T("->"));
	return nRet;
}


//
// Update UI after graph list update
//

#define UPDATE_MENU(handle, pos) \
{ \
	menu.Attach(handle); \
	UpdateGraphSubmenu(menu.GetSubMenu(pos)); \
	menu.Detach(); \
}

void CConsoleApp::OnGraphListUpdate(WPARAM wParam, LPARAM lParam)
{
	CMenu menu;

	LockGraphs();

	// Sort graph entries
	qsort(g_pGraphList, g_dwNumGraphs, sizeof(NXC_GRAPH), CompareGraphNames);

	// Update menus
	UPDATE_MENU(m_hMainMenu, 3);
	UPDATE_MENU(m_hMDIMenu, 3);
	UPDATE_MENU(m_hAlarmBrowserMenu, 4);
	UPDATE_MENU(m_hEventBrowserMenu, 4);
	UPDATE_MENU(m_hEventEditorMenu, 4);
	UPDATE_MENU(m_hUserEditorMenu, 4);
	UPDATE_MENU(m_hDCEditorMenu, 4);
	UPDATE_MENU(m_hPolicyEditorMenu, 4);
	UPDATE_MENU(m_hMapMenu, 5);
	UPDATE_MENU(m_hTrapEditorMenu, 4);
	UPDATE_MENU(m_hActionEditorMenu, 4);
	UPDATE_MENU(m_hGraphMenu, 4);
	UPDATE_MENU(m_hPackageMgrMenu, 4);
	UPDATE_MENU(m_hLastValuesMenu, 4);
	UPDATE_MENU(m_hServerCfgEditorMenu, 4);
	UPDATE_MENU(m_hAgentCfgEditorMenu, 5);
	UPDATE_MENU(m_hObjToolsEditorMenu, 4);
	UPDATE_MENU(m_hScriptManagerMenu, 5);
	UPDATE_MENU(m_hDataViewMenu, 4);
	UPDATE_MENU(m_hAgentCfgMgrMenu, 4);
	UPDATE_MENU(m_hObjectCommentsMenu, 5);
	UPDATE_MENU(m_hObjectBrowserMenu, 4);

	UnlockGraphs();
}


//
// Static data for excepion handler
//

static FILE *m_pExInfoFile = NULL;


//
// Writer for SEHShowCallStack()
//

static void ExceptionDataWriter(const TCHAR *pszText)
{
	if (m_pExInfoFile != NULL)
		_fputts(pszText, m_pExInfoFile);
}


//
// Exception handler
//

static BOOL ExceptionHandler(EXCEPTION_POINTERS *pInfo)
{
	TCHAR szBuffer[MAX_PATH], szInfoFile[MAX_PATH], szDumpFile[MAX_PATH];
	HANDLE hFile;
	time_t t;
	DWORD dwSize;
	MINIDUMP_EXCEPTION_INFORMATION mei;
	OSVERSIONINFO ver;
   SYSTEM_INFO sysInfo;
	CFatalErrorDlg dlg;
	BYTE *pData;
	BOOL bRet;

	t = time(NULL);

	// Create info file
	_sntprintf(szInfoFile, MAX_PATH, _T("%s\\nxcon-%d-%u.info"),
	           g_szWorkDir, GetCurrentProcessId(), (DWORD)t);
	m_pExInfoFile = _tfopen(szInfoFile, _T("w"));
	if (m_pExInfoFile != NULL)
	{
		fprintf(m_pExInfoFile, "NETXMS CONSOLE CRASH DUMP\n%s\n", ctime(&t));
		_ftprintf(m_pExInfoFile, _T("EXCEPTION: %08X (%s) at %08X\n"),
		          pInfo->ExceptionRecord->ExceptionCode,
		          SEHExceptionName(pInfo->ExceptionRecord->ExceptionCode),
		          pInfo->ExceptionRecord->ExceptionAddress);

		// NetXMS and OS version
		ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx(&ver);
		if (ver.dwMajorVersion == 5)
		{
			switch(ver.dwMinorVersion)
			{
				case 0:
					_tcscpy(szBuffer, _T("2000"));
					break;
				case 1:
					_tcscpy(szBuffer, _T("XP"));
					break;
				case 2:
					_tcscpy(szBuffer, _T("Server 2003"));
					break;
				default:
					_sntprintf_s(szBuffer, MAX_PATH, _TRUNCATE, _T("NT %d.%d"), ver.dwMajorVersion, ver.dwMinorVersion);
					break;
			}
		}
		else
		{
			_sntprintf_s(szBuffer, MAX_PATH, _TRUNCATE, _T("NT %d.%d"), ver.dwMajorVersion, ver.dwMinorVersion);
		}
		_ftprintf(m_pExInfoFile, _T("\nNetXMS Console Version: ") NETXMS_VERSION_STRING _T("\n")
		                         _T("OS Version: Windows %s Build %d %s\n"),
		          szBuffer, ver.dwBuildNumber, ver.szCSDVersion);

		// Processor architecture
		fprintf(m_pExInfoFile, "Processor architecture: ");
		GetSystemInfo(&sysInfo);
		switch(sysInfo.wProcessorArchitecture)
		{
			case PROCESSOR_ARCHITECTURE_INTEL:
				fprintf(m_pExInfoFile, "Intel x86\n");
				break;
			case PROCESSOR_ARCHITECTURE_MIPS:
				fprintf(m_pExInfoFile, "MIPS\n");
				break;
			case PROCESSOR_ARCHITECTURE_ALPHA:
				fprintf(m_pExInfoFile, "ALPHA\n");
				break;
			case PROCESSOR_ARCHITECTURE_PPC:
				fprintf(m_pExInfoFile, "PowerPC\n");
				break;
			case PROCESSOR_ARCHITECTURE_IA64:
				fprintf(m_pExInfoFile, "Intel IA-64\n");
				break;
			case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
				fprintf(m_pExInfoFile, "Intel x86 on Win64\n");
				break;
			case PROCESSOR_ARCHITECTURE_AMD64:
				fprintf(m_pExInfoFile, "AMD64 (Intel EM64T)\n");
				break;
			default:
				fprintf(m_pExInfoFile, "UNKNOWN\n");
				break;
		}

#ifdef _X86_
		fprintf(m_pExInfoFile, "\nRegister information:\n"
		        "  eax=%08X  ebx=%08X  ecx=%08X  edx=%08X\n"
		        "  esi=%08X  edi=%08X  ebp=%08X  esp=%08X\n"
		        "  cs=%04X  ds=%04X  es=%04X  ss=%04X  fs=%04X  gs=%04X  flags=%08X\n",
		        pInfo->ContextRecord->Eax, pInfo->ContextRecord->Ebx,
		        pInfo->ContextRecord->Ecx, pInfo->ContextRecord->Edx,
		        pInfo->ContextRecord->Esi, pInfo->ContextRecord->Edi,
		        pInfo->ContextRecord->Ebp, pInfo->ContextRecord->Esp,
		        pInfo->ContextRecord->SegCs, pInfo->ContextRecord->SegDs,
		        pInfo->ContextRecord->SegEs, pInfo->ContextRecord->SegSs,
		        pInfo->ContextRecord->SegFs, pInfo->ContextRecord->SegGs,
		        pInfo->ContextRecord->EFlags);
#endif

		fprintf(m_pExInfoFile, "\nCall stack:\n");
		SEHShowCallStack(pInfo->ContextRecord);

		fclose(m_pExInfoFile);
	}

	// Create minidump
	_sntprintf(szDumpFile, MAX_PATH, _T("%s\\nxcon-%d-%u.mdmp"),
	           g_szWorkDir, GetCurrentProcessId(), (DWORD)t);
   hFile = CreateFile(szDumpFile, GENERIC_WRITE, 0, NULL,
                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
   if (hFile != INVALID_HANDLE_VALUE)
   {
		mei.ThreadId = GetCurrentThreadId();
		mei.ExceptionPointers = pInfo;
		mei.ClientPointers = FALSE;
      MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile,
                        MiniDumpNormal, &mei, NULL, NULL);
      CloseHandle(hFile);
   }

	// Prepare error dialog
	dwSize = 0;
	pData = LoadFile(szInfoFile, &dwSize);
	pData = (BYTE *)realloc(pData, dwSize + 1);
	pData[dwSize] = 0;
	dlg.m_strText = WideStringFromMBString(CHECK_NULL_A((char *)pData));
	dlg.m_strFile = szDumpFile;

	// Dialog should be executed from GUI thread
	if (GetCurrentThreadId() == theApp.m_dwMainThreadId)
	{
		bRet = (dlg.DoModal() != IDOK);
	}
	else
	{
		bRet = (BOOL)theApp.m_pMainWnd->SendMessage(NXCM_SHOW_FATAL_ERROR, 0, (LPARAM)&dlg);
	}

	return bRet;
}


//
// Main message loop
//

int CConsoleApp::Run() 
{
	int nRet;

#ifndef _DEBUG
	SetExceptionHandler(ExceptionHandler, ExceptionDataWriter, NULL, _T("nxcon.exe"), NULL, FALSE, FALSE);
	__try
	{
#endif
		nRet = CWinApp::Run();
#ifndef _DEBUG
	}
	__except(___ExceptionHandler((EXCEPTION_POINTERS *)_exception_info()))
	{
		ExitProcess(99);
	}
#endif

	return nRet;
}


//
// Start graph manager
//

void CConsoleApp::OnToolsGraphsManage() 
{
	CGraphManagerDlg dlg;

	dlg.DoModal();
}


//
// Interface DCI types
//

#define IFDCI_IN_BYTES     0
#define IFDCI_OUT_BYTES    1
#define IFDCI_IN_PACKETS   2
#define IFDCI_OUT_PACKETS  3
#define IFDCI_IN_ERRORS    4
#define IFDCI_OUT_ERRORS   5


//
// Create single interface DCI
//

static DWORD CreateDCI(NXC_OBJECT *pNode, NXC_OBJECT *pInterface, int nType,
                       NXC_DCI_LIST *pList, LPCTSTR pszText, BOOL bDelta,
                       int nInterval, int nRetention)
{
	DWORD dwResult, dwItemId, dwIndex;
	NXC_DCI item;

	dwResult = NXCCreateNewDCI(g_hSession, pList, &dwItemId);
	if (dwResult == RCC_SUCCESS)
	{
	   memset(&item, 0, sizeof(NXC_DCI));
      item.dwId = dwItemId;
		item.iPollingInterval = nInterval;
		item.iRetentionTime = nRetention;
		item.iSource = (pNode->node.dwFlags & NF_IS_NATIVE_AGENT) ? DS_NATIVE_AGENT : DS_SNMP_AGENT;
		item.iStatus = ITEM_STATUS_ACTIVE;
		item.iDataType = DCI_DT_UINT;
		item.iDeltaCalculation = bDelta ? DCM_AVERAGE_PER_SECOND : DCM_ORIGINAL_VALUE;
		nx_strncpy(item.szDescription, pszText, MAX_DB_STRING);
		if (item.iSource == DS_NATIVE_AGENT)
		{
			switch(nType)
			{
				case IFDCI_IN_BYTES:
					_sntprintf_s(item.szName, MAX_ITEM_NAME, _TRUNCATE, _T("Net.Interface.BytesIn(%d)"), pInterface->iface.dwIfIndex);
					break;
				case IFDCI_OUT_BYTES:
					_sntprintf_s(item.szName, MAX_ITEM_NAME, _TRUNCATE, _T("Net.Interface.BytesOut(%d)"), pInterface->iface.dwIfIndex);
					break;
				case IFDCI_IN_PACKETS:
					_sntprintf_s(item.szName, MAX_ITEM_NAME, _TRUNCATE, _T("Net.Interface.PacketsIn(%d)"), pInterface->iface.dwIfIndex);
					break;
				case IFDCI_OUT_PACKETS:
					_sntprintf_s(item.szName, MAX_ITEM_NAME, _TRUNCATE, _T("Net.Interface.PacketsOut(%d)"), pInterface->iface.dwIfIndex);
					break;
				case IFDCI_IN_ERRORS:
					_sntprintf_s(item.szName, MAX_ITEM_NAME, _TRUNCATE, _T("Net.Interface.InErrors(%d)"), pInterface->iface.dwIfIndex);
					break;
				case IFDCI_OUT_ERRORS:
					_sntprintf_s(item.szName, MAX_ITEM_NAME, _TRUNCATE, _T("Net.Interface.OutErrors(%d)"), pInterface->iface.dwIfIndex);
					break;
			}
		}
		else	// SNMP
		{
			switch(nType)
			{
				case IFDCI_IN_BYTES:
					_sntprintf_s(item.szName, MAX_ITEM_NAME, _TRUNCATE, _T(".1.3.6.1.2.1.2.2.1.10.%d"), pInterface->iface.dwIfIndex);
					break;
				case IFDCI_OUT_BYTES:
					_sntprintf_s(item.szName, MAX_ITEM_NAME, _TRUNCATE, _T(".1.3.6.1.2.1.2.2.1.16.%d"), pInterface->iface.dwIfIndex);
					break;
				case IFDCI_IN_PACKETS:
					_sntprintf_s(item.szName, MAX_ITEM_NAME, _TRUNCATE, _T(".1.3.6.1.2.1.2.2.1.11.%d"), pInterface->iface.dwIfIndex);
					break;
				case IFDCI_OUT_PACKETS:
					_sntprintf_s(item.szName, MAX_ITEM_NAME, _TRUNCATE, _T(".1.3.6.1.2.1.2.2.1.17.%d"), pInterface->iface.dwIfIndex);
					break;
				case IFDCI_IN_ERRORS:
					_sntprintf_s(item.szName, MAX_ITEM_NAME, _TRUNCATE, _T(".1.3.6.1.2.1.2.2.1.14.%d"), pInterface->iface.dwIfIndex);
					break;
				case IFDCI_OUT_ERRORS:
					_sntprintf_s(item.szName, MAX_ITEM_NAME, _TRUNCATE, _T(".1.3.6.1.2.1.2.2.1.20.%d"), pInterface->iface.dwIfIndex);
					break;
			}
		}

      dwIndex = NXCItemIndex(pList, dwItemId);
      if (dwIndex != INVALID_INDEX)
      {
			dwResult = NXCUpdateDCI(g_hSession, pNode->dwId, &item);
         if (dwResult == RCC_SUCCESS)
         {
            memcpy(&pList->pItems[dwIndex], &item, sizeof(NXC_DCI));
         }
		}
		else
		{
			dwResult = RCC_INTERNAL_ERROR;
		}
	}
	return dwResult;
}


//
// Worker function for interface DCIs creation
//

static DWORD CreateInterfaceDCI(NXC_OBJECT *pNode, NXC_OBJECT *pInterface, CCreateIfDCIDlg *pDlg)
{
   CDataCollectionEditor *pWnd;
	NXC_DCI_LIST *pList;
	DWORD dwResult;

	// Check if DC editor for given node already open. If yes, we already
	// have the lock and don't need to call NXCOpenNodeDCIList
	pWnd = (CDataCollectionEditor *)theApp.FindObjectView(OV_DC_EDITOR, pNode->dwId);
	if (pWnd != NULL)
	{
		pList = pWnd->GetDCIList();
	}
	else
	{
		dwResult = NXCOpenNodeDCIList(g_hSession, pNode->dwId, &pList);
		if (dwResult != RCC_SUCCESS)
			return dwResult;	// Unable to obtain lock on DCI list
	}

	if (pDlg->m_bInBytes)
	{
		dwResult = CreateDCI(pNode, pInterface, IFDCI_IN_BYTES, pList, pDlg->m_strInBytes,
		                     pDlg->m_bDeltaInBytes, pDlg->m_dwInterval, pDlg->m_dwRetention);
		if (dwResult != RCC_SUCCESS)
			goto cleanup;
	}

	if (pDlg->m_bInErrors)
	{
		dwResult = CreateDCI(pNode, pInterface, IFDCI_IN_ERRORS, pList, pDlg->m_strInErrors,
		                     pDlg->m_bDeltaInErrors, pDlg->m_dwInterval, pDlg->m_dwRetention);
		if (dwResult != RCC_SUCCESS)
			goto cleanup;
	}

	if (pDlg->m_bInPackets)
	{
		dwResult = CreateDCI(pNode, pInterface, IFDCI_IN_PACKETS, pList, pDlg->m_strInPackets,
		                     pDlg->m_bDeltaInPackets, pDlg->m_dwInterval, pDlg->m_dwRetention);
		if (dwResult != RCC_SUCCESS)
			goto cleanup;
	}

	if (pDlg->m_bOutBytes)
	{
		dwResult = CreateDCI(pNode, pInterface, IFDCI_OUT_BYTES, pList, pDlg->m_strOutBytes,
		                     pDlg->m_bDeltaOutBytes, pDlg->m_dwInterval, pDlg->m_dwRetention);
		if (dwResult != RCC_SUCCESS)
			goto cleanup;
	}

	if (pDlg->m_bOutErrors)
	{
		dwResult = CreateDCI(pNode, pInterface, IFDCI_OUT_ERRORS, pList, pDlg->m_strOutErrors,
		                     pDlg->m_bDeltaOutErrors, pDlg->m_dwInterval, pDlg->m_dwRetention);
		if (dwResult != RCC_SUCCESS)
			goto cleanup;
	}

	if (pDlg->m_bOutPackets)
	{
		dwResult = CreateDCI(pNode, pInterface, IFDCI_OUT_PACKETS, pList, pDlg->m_strOutPackets,
		                     pDlg->m_bDeltaOutPackets, pDlg->m_dwInterval, pDlg->m_dwRetention);
		if (dwResult != RCC_SUCCESS)
			goto cleanup;
	}

cleanup:
	if (pWnd != NULL)
	{
		pWnd->RefreshItemList();
	}
	else
	{
		dwResult = NXCCloseNodeDCIList(g_hSession, pList);
	}

	return dwResult;
}


//
// Create DCIs for given interface
//

void CConsoleApp::CreateIfDCI(NXC_OBJECT *pObject)
{
	CCreateIfDCIDlg dlg;
	NXC_OBJECT *pNode;
	DWORD dwResult;

	dlg.m_bDeltaInBytes = GetProfileInt(_T("CreateIfDCIDlg"), _T("DeltaInBytes"), TRUE);
	dlg.m_bDeltaInErrors = GetProfileInt(_T("CreateIfDCIDlg"), _T("DeltaInErrors"), TRUE);
	dlg.m_bDeltaInPackets = GetProfileInt(_T("CreateIfDCIDlg"), _T("DeltaInPackets"), TRUE);
	dlg.m_bDeltaOutBytes = GetProfileInt(_T("CreateIfDCIDlg"), _T("DeltaOutBytes"), TRUE);
	dlg.m_bDeltaOutErrors = GetProfileInt(_T("CreateIfDCIDlg"), _T("DeltaOutErrors"), TRUE);
	dlg.m_bDeltaOutPackets = GetProfileInt(_T("CreateIfDCIDlg"), _T("DeltaOutPackets"), TRUE);
	dlg.m_bInBytes = GetProfileInt(_T("CreateIfDCIDlg"), _T("InBytes"), TRUE);
	dlg.m_bInErrors = GetProfileInt(_T("CreateIfDCIDlg"), _T("InErrors"), FALSE);
	dlg.m_bInPackets = GetProfileInt(_T("CreateIfDCIDlg"), _T("InPackets"), FALSE);
	dlg.m_bOutBytes = GetProfileInt(_T("CreateIfDCIDlg"), _T("OutBytes"), TRUE);
	dlg.m_bOutErrors = GetProfileInt(_T("CreateIfDCIDlg"), _T("OutErrors"), FALSE);
	dlg.m_bOutPackets = GetProfileInt(_T("CreateIfDCIDlg"), _T("OutPackets"), FALSE);
	dlg.m_dwInterval = GetProfileInt(_T("CreateIfDCIDlg"), _T("Interval"), 60);
	dlg.m_dwRetention = GetProfileInt(_T("CreateIfDCIDlg"), _T("Retention"), 30);
	dlg.m_strInBytes = GetProfileString(_T("CreateIfDCIDlg"), _T("InBytesText"), _T("Inbound traffic on {interface} (bytes/sec)"));
	dlg.m_strInErrors = GetProfileString(_T("CreateIfDCIDlg"), _T("InErrorsText"), _T("Inbound error rate on {interface} (errors/sec)"));
	dlg.m_strInPackets = GetProfileString(_T("CreateIfDCIDlg"), _T("InPacketsText"), _T("Inbound traffic on {interface} (packets/sec)"));
	dlg.m_strOutBytes = GetProfileString(_T("CreateIfDCIDlg"), _T("OutBytesText"), _T("Outbound traffic on {interface} (bytes/sec)"));
	dlg.m_strOutErrors = GetProfileString(_T("CreateIfDCIDlg"), _T("OutErrorsText"), _T("Outbound error rate on {interface} (errors/sec)"));
	dlg.m_strOutPackets = GetProfileString(_T("CreateIfDCIDlg"), _T("OutPacketsText"), _T("Outbound traffic on {interface} (packets/sec)"));

	dlg.m_strInBytes.Replace(_T("{interface}"), pObject->szName);
	dlg.m_strInErrors.Replace(_T("{interface}"), pObject->szName);
	dlg.m_strInPackets.Replace(_T("{interface}"), pObject->szName);
	dlg.m_strOutBytes.Replace(_T("{interface}"), pObject->szName);
	dlg.m_strOutErrors.Replace(_T("{interface}"), pObject->szName);
	dlg.m_strOutPackets.Replace(_T("{interface}"), pObject->szName);

	if (dlg.DoModal() == IDOK)
	{
		WriteProfileInt(_T("CreateIfDCIDlg"), _T("DeltaInBytes"), dlg.m_bDeltaInBytes);
		WriteProfileInt(_T("CreateIfDCIDlg"), _T("DeltaInErrors"), dlg.m_bDeltaInErrors);
		WriteProfileInt(_T("CreateIfDCIDlg"), _T("DeltaInPackets"), dlg.m_bDeltaInPackets);
		WriteProfileInt(_T("CreateIfDCIDlg"), _T("DeltaOutBytes"), dlg.m_bDeltaOutBytes);
		WriteProfileInt(_T("CreateIfDCIDlg"), _T("DeltaOutErrors"), dlg.m_bDeltaOutErrors);
		WriteProfileInt(_T("CreateIfDCIDlg"), _T("DeltaOutPackets"), dlg.m_bDeltaOutPackets);
		WriteProfileInt(_T("CreateIfDCIDlg"), _T("InBytes"), dlg.m_bInBytes);
		WriteProfileInt(_T("CreateIfDCIDlg"), _T("InErrors"), dlg.m_bInErrors);
		WriteProfileInt(_T("CreateIfDCIDlg"), _T("InPackets"), dlg.m_bInPackets);
		WriteProfileInt(_T("CreateIfDCIDlg"), _T("OutBytes"), dlg.m_bOutBytes);
		WriteProfileInt(_T("CreateIfDCIDlg"), _T("OutErrors"), dlg.m_bOutErrors);
		WriteProfileInt(_T("CreateIfDCIDlg"), _T("OutPackets"), dlg.m_bOutPackets);
		WriteProfileInt(_T("CreateIfDCIDlg"), _T("Interval"), dlg.m_dwInterval);
		WriteProfileInt(_T("CreateIfDCIDlg"), _T("Retention"), dlg.m_dwRetention);

		if (pObject->dwNumParents > 0)
		{
			pNode = NXCFindObjectById(g_hSession, pObject->pdwParentList[0]);
			if (pNode != NULL)
			{
				dwResult = DoRequestArg3(CreateInterfaceDCI, pNode, pObject, &dlg, _T("Creating data collection items..."));
				if (dwResult == RCC_SUCCESS)
				{
					m_pMainWnd->MessageBox(_T("Interface DCIs successfully created"), _T("Success"), MB_OK | MB_ICONINFORMATION);
				}
				else
				{
					ErrorBox(dwResult, _T("Cannot add interface DCIs: %s"));
				}
			}
			else
			{
				m_pMainWnd->MessageBox(_T("Cannot add interface DCIs: parent node object inaccessible"), _T("Error"), MB_OK | MB_ICONSTOP);
			}
		}
		else
		{
			m_pMainWnd->MessageBox(_T("Internal error"), _T("Error"), MB_OK | MB_ICONSTOP);
		}
	}
}


//
// Load situations
//

DWORD CConsoleApp::LoadSituations()
{
	NXCDestroySituationList(m_pSituationList);
	m_pSituationList = NULL;
	return NXCGetSituationList(g_hSession, &m_pSituationList);
}


//
// Get (and lock) situation list
//

NXC_SITUATION_LIST *CConsoleApp::GetSituationList()
{
	if (m_pSituationList != NULL)
		MutexLock(m_mutexSituationList, INFINITE);
	return m_pSituationList;
}


//
// Unlock situation list
//

void CConsoleApp::UnlockSituationList()
{
	MutexUnlock(m_mutexSituationList);
}


//
// Get situation's name
//

TCHAR *CConsoleApp::GetSituationName(DWORD id, TCHAR *buffer)
{
	int i;

	if (id == 0)
	{
		_tcscpy(buffer, _T("<none>"));
	}
	else
	{
		MutexLock(m_mutexSituationList, INFINITE);
		if (m_pSituationList != NULL)
		{
			for(i = 0; i < m_pSituationList->m_count; i++)
			{
				if (m_pSituationList->m_situations[i].m_id == id)
				{
					nx_strncpy(buffer, m_pSituationList->m_situations[i].m_name, MAX_DB_STRING);
					break;
				}
			}
			if (i == m_pSituationList->m_count)
			{
				_tcscpy(buffer, _T("<unknown>"));
			}
		}
		else
		{
			_tcscpy(buffer, _T("<unknown>"));
		}
		MutexUnlock(m_mutexSituationList);
	}
	return buffer;
}


//
// Open situations manager
//

void CConsoleApp::OnViewSituations() 
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);

	// create a new MDI child window or open existing
   if (m_viewState[VIEW_SITUATION_MANAGER].bActive)
   {
      m_viewState[VIEW_SITUATION_MANAGER].pWnd->BringWindowToTop();
   }
   else
   {
		if (m_pSituationList != NULL)
		{
			pFrame->CreateNewChild(RUNTIME_CLASS(CSituationManager), IDR_SITUATION_MANAGER,
										  m_hSituationManagerMenu, m_hSituationManagerAccel);
		}
		else
		{
			ErrorBox(RCC_ACCESS_DENIED, _T("Cannot open situation manager: %s"));
		}
   }
}


//
// Open map manager
//

void CConsoleApp::OnControlpanelNetworkmaps() 
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);

	// create a new MDI child window or open existing
   if (m_viewState[VIEW_MAP_MANAGER].bActive)
   {
      m_viewState[VIEW_MAP_MANAGER].pWnd->BringWindowToTop();
   }
   else
   {
		pFrame->CreateNewChild(RUNTIME_CLASS(CMapManager), IDR_MAP_MANAGER,
									  m_hMapManagerMenu, m_hMapManagerAccel);
   }
}


//
// Open syslog parser configurator
//

void CConsoleApp::OnControlpanelSyslogparser() 
{
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, m_pMainWnd);

	// create a new MDI child window or open existing
   if (m_viewState[VIEW_SYSLOG_PARSER_CFG].bActive)
   {
      m_viewState[VIEW_SYSLOG_PARSER_CFG].pWnd->BringWindowToTop();
   }
   else
   {
		pFrame->CreateNewChild(RUNTIME_CLASS(CSyslogParserCfg), IDR_SYSLOG_PARSER_CFG,
									  m_hSyslogParserCfgMenu, m_hSyslogParserCfgAccel);
   }
}


//
// Add node to cluster
//

void CConsoleApp::AddNodeToCluster(NXC_OBJECT *node)
{
   CObjectSelDlg dlg;
   DWORD dwResult;

   dlg.m_dwAllowedClasses = SCL_CLUSTER;
	dlg.m_bSingleSelection = TRUE;
	dlg.m_bAllowEmptySelection = FALSE;
   if (dlg.DoModal() == IDOK)
   {
      dwResult = DoRequestArg3(NXCAddClusterNode, g_hSession, (void *)dlg.m_pdwObjectList[0],
		                         (void *)node->dwId, _T("Adding cluster node..."));
      if (dwResult != RCC_SUCCESS)
         ErrorBox(dwResult, _T("Cannot add node to cluster: %s"));
   }
}
