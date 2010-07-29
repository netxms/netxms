// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "nxcon.h"

#include "MainFrm.h"
#include "SaveDesktopDlg.h"
#include "LastValuesView.h"
#include "ActionEditor.h"
#include "EventEditor.h"
#include "AlarmBrowser.h"
#include "ObjectBrowser.h"
#include "FatalErrorDlg.h"
#include "ObjectToolsEditor.h"
#include "TrapEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MAX_WINDOW_COUNT      128

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_CLOSE()
	ON_COMMAND(ID_DESKTOP_SAVE, OnDesktopSave)
	ON_COMMAND(ID_DESKTOP_SAVEAS, OnDesktopSaveas)
	ON_COMMAND(ID_DESKTOP_RESTORE, OnDesktopRestore)
	ON_COMMAND(ID_DESKTOP_NEW, OnDesktopNew)
	//}}AFX_MSG_MAP
//   ON_UPDATE_COMMAND_UI(ID_INDICATOR_CONNECT, OnUpdateConnState)
   ON_MESSAGE(NXCM_OBJECT_CHANGE, OnObjectChange)
   ON_MESSAGE(NXCM_USERDB_CHANGE, OnUserDBChange)
   ON_MESSAGE(NXCM_SITUATION_CHANGE, OnSituationChange)
   ON_MESSAGE(NXCM_STATE_CHANGE, OnStateChange)
   ON_MESSAGE(NXCM_ALARM_UPDATE, OnAlarmUpdate)
   ON_MESSAGE(NXCM_ACTION_UPDATE, OnActionUpdate)
   ON_MESSAGE(NXCM_EVENTDB_UPDATE, OnEventDBUpdate)
   ON_MESSAGE(NXCM_DEPLOYMENT_INFO, OnDeploymentInfo)
   ON_MESSAGE(NXCM_UPDATE_EVENT_LIST, OnUpdateEventList)
   ON_MESSAGE(NXCM_UPDATE_OBJECT_TOOLS, OnUpdateObjectTools)
	ON_MESSAGE(NXCM_SHOW_FATAL_ERROR, OnShowFatalError)
   ON_MESSAGE(NXCM_TRAPCFG_UPDATE, OnTrapCfgUpdate)
END_MESSAGE_MAP()

static UINT indicators[] =
{
   ID_SEPARATOR,
	ID_INDICATOR_CONNECT,
	ID_INDICATOR_DESKTOP,
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL
};


//
// CMainFrame construction/destruction
//

CMainFrame::CMainFrame()
{
   m_szDesktopName[0] = 0;
}

CMainFrame::~CMainFrame()
{
}


//
// Handler for WM_CREATE message
//

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
   static TBBUTTON tbButtons[] =
   {
      { 0, ID_VIEW_MAP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
      { 1, ID_VIEW_OBJECTBROWSER, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
      { 2, ID_VIEW_ALARMS, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
      { 3, ID_VIEW_EVENTS, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
      { 4, ID_VIEW_CONTROLPANEL, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
      { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0 },
      { 6, ID_CONTROLPANEL_EVENTPOLICY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
      { 7, ID_CONTROLPANEL_SNMPTRAPS, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
      { 5, ID_CONTROLPANEL_USERS, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
      { 8, ID_CONTROLPANEL_SERVERCFG, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
      { 9, ID_CONTROLPANEL_SCRIPTLIBRARY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
      { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0 },
      { 10, ID_APP_EXIT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 }
   };

	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   m_imageList.Create(16, 16, ILC_COLOR24 | ILC_MASK, 8, 1);
   m_imageList.Add(theApp.LoadIcon(IDI_NETMAP));
   m_imageList.Add(theApp.LoadIcon(IDI_TREE));
   m_imageList.Add(theApp.LoadIcon(IDI_ALARM));
   m_imageList.Add(theApp.LoadIcon(IDI_LOG));
   m_imageList.Add(theApp.LoadIcon(IDI_SETUP));
   m_imageList.Add(theApp.LoadIcon(IDI_USER_GROUP));
   m_imageList.Add(theApp.LoadIcon(IDI_RULEMGR));
   m_imageList.Add(theApp.LoadIcon(IDI_TRAP));
   m_imageList.Add(theApp.LoadIcon(IDI_DATABASE));
   m_imageList.Add(theApp.LoadIcon(IDI_SCRIPT_LIBRARY));
   m_imageList.Add(theApp.LoadIcon(IDI_EXIT));

   m_wndToolBar.CreateEx(this);
   m_wndToolBar.GetToolBarCtrl().SetImageList(&m_imageList);
   m_wndToolBar.GetToolBarCtrl().AddButtons(sizeof(tbButtons) / sizeof(TBBUTTON), tbButtons);

	if (!m_wndReBar.Create(this) ||
		!m_wndReBar.AddBar(&m_wndToolBar))
	{
		TRACE0("Failed to create rebar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) ||
		 !m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

   m_wndStatusBar.GetStatusBarCtrl().SetIcon(1, NULL);
   m_wndStatusBar.GetStatusBarCtrl().SetText(_T(""), 1, 0);
   m_wndStatusBar.GetStatusBarCtrl().SetIcon(2, NULL);
   m_wndStatusBar.GetStatusBarCtrl().SetText(_T(""), 2, 0);

	// TODO: Remove this if you don't want tool tips
	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY);

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CMDIFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers


void CMainFrame::OnDestroy() 
{
	CMDIFrameWnd::OnDestroy();
}


//
// Broadcast message to all MDI child windows
//

void CMainFrame::BroadcastMessage(UINT msg, WPARAM wParam, LPARAM lParam, BOOL bUsePost)
{
   CWnd *pWnd;

   pWnd = MDIGetActive();
   while(pWnd != NULL)
   {
      if (bUsePost)
         pWnd->PostMessage(msg, wParam, lParam);
      else
         pWnd->SendMessage(msg, wParam, lParam);

      // If pWnd still exist, call GetNextWindow()
      if (IsWindow(pWnd->m_hWnd))
         pWnd = pWnd->GetNextWindow();
      else
         pWnd = MDIGetActive();
   }
   theApp.DebugPrintf(_T("CMainFrame::BroadcastMessage(%d, %d, %d)"), msg, wParam, lParam);
}


//
// Handler for NXCM_OBJECT_CHANGE message
//

LRESULT CMainFrame::OnObjectChange(WPARAM wParam, LPARAM lParam)
{
   BroadcastMessage(NXCM_OBJECT_CHANGE, wParam, lParam, TRUE);
	return 0;
}


//
// Handler for NXCM_USERDB_CHANGE message
//

LRESULT CMainFrame::OnUserDBChange(WPARAM wParam, LPARAM lParam)
{
   BroadcastMessage(NXCM_USERDB_CHANGE, wParam, lParam, TRUE);
	return 0;
}


//
// Handler for NXCM_SITUATION_CHANGE message
//

LRESULT CMainFrame::OnSituationChange(WPARAM wParam, LPARAM lParam)
{
   BroadcastMessage(NXCM_SITUATION_CHANGE, wParam, lParam, TRUE);
	return 0;
}


//
// Handler for NXCM_DEPLOYMENT_INFO message
//

LRESULT CMainFrame::OnDeploymentInfo(WPARAM wParam, LPARAM lParam)
{
   BroadcastMessage(NXCM_DEPLOYMENT_INFO, wParam, lParam, FALSE);
   safe_free(((NXC_DEPLOYMENT_STATUS *)lParam)->pszErrorMessage)
   free((void *)lParam);
	return 0;
}


//
// NXCM_UPDATE_EVENT_LIST message handler
//

LRESULT CMainFrame::OnUpdateEventList(WPARAM wParam, LPARAM lParam)
{
   DoRequestArg1(NXCLoadEventDB, g_hSession, _T("Reloading event information..."));
   BroadcastMessage(WM_COMMAND, ID_UPDATE_EVENT_LIST, 0, TRUE);
	return 0;
}


//
// NXCM_UPDATE_OBJECT_TOOLS message handler
//

LRESULT CMainFrame::OnUpdateObjectTools(WPARAM wParam, LPARAM lParam)
{
   DoRequest(LoadObjectTools, _T("Reloading object tools information..."));
	CObjectToolsEditor *pWnd = theApp.GetObjectToolsEditor();
	if (pWnd != NULL)
		pWnd->OnObjectToolsUpdate((DWORD)lParam, wParam == NX_NOTIFY_OBJTOOL_DELETED);
	return 0;
}


//
// Handler for NXCM_STATE_CHANGE message
//

LRESULT CMainFrame::OnStateChange(WPARAM wParam, LPARAM lParam)
{
   if (wParam)
   {
      m_wndStatusBar.GetStatusBarCtrl().SetIcon(1, 
         (HICON)LoadImage(theApp.m_hInstance, MAKEINTRESOURCE(IDI_CONNECT), 
                          IMAGE_ICON, 16, 16, LR_SHARED));
      m_wndStatusBar.GetStatusBarCtrl().SetText(g_szServer, 1, 0);
   }
   else
   {
      m_wndStatusBar.GetStatusBarCtrl().SetIcon(1, NULL);
      m_wndStatusBar.GetStatusBarCtrl().SetText(_T(""), 1, 0);
   }
	return 0;
}


//
// NXCM_ALARM_UPDATE message handler
//

LRESULT CMainFrame::OnAlarmUpdate(WPARAM wParam, LPARAM lParam)
{
   CMDIChildWnd *pWnd;

   pWnd = theApp.GetAlarmBrowser();
   if (pWnd != NULL)
      ((CAlarmBrowser *)pWnd)->OnAlarmUpdate((DWORD)wParam, (NXC_ALARM *)lParam);
   pWnd = theApp.GetObjectBrowser();
   if (pWnd != NULL)
      ((CObjectBrowser *)pWnd)->OnAlarmUpdate((DWORD)wParam, (NXC_ALARM *)lParam);
   free((void *)lParam);
	return 0;
}


//
// NXCM_ACTION_UPDATE message handler
//

LRESULT CMainFrame::OnActionUpdate(WPARAM wParam, LPARAM lParam)
{
   CActionEditor *pWnd;

	pWnd = theApp.GetActionEditor();
   if (pWnd != NULL)
      pWnd->OnActionUpdate((DWORD)wParam, (NXC_ACTION *)lParam);
   free((void *)lParam);
	return 0;
}


//
// NXCM_TRAPCFG_UPDATE message handler
//

LRESULT CMainFrame::OnTrapCfgUpdate(WPARAM wParam, LPARAM lParam)
{
   CTrapEditor *pWnd;

	pWnd = theApp.GetTrapEditor();
   if (pWnd != NULL)
      pWnd->OnTrapCfgUpdate((DWORD)wParam, (NXC_TRAP_CFG_ENTRY *)lParam);
   NXCDestroyTrapCfgEntry((NXC_TRAP_CFG_ENTRY *)lParam);
	return 0;
}


//
// NXCM_EVENTDB_UPDATE message handler
//

LRESULT CMainFrame::OnEventDBUpdate(WPARAM wParam, LPARAM lParam)
{
   CEventEditor *pWnd;

	pWnd = theApp.GetEventEditor();
   if (pWnd != NULL)
      pWnd->OnEventDBUpdate((DWORD)wParam, (NXC_EVENT_TEMPLATE *)lParam);
   free((void *)lParam);
	return 0;
}


//
// WM_CLOSE message handler
//

void CMainFrame::OnClose() 
{
   WINDOWPLACEMENT wndPlacement;

   // Save window placement
   GetWindowPlacement(&wndPlacement);
   AfxGetApp()->WriteProfileBinary(_T("General"), _T("WindowPlacement"),
                                   (BYTE *)&wndPlacement, sizeof(WINDOWPLACEMENT));

   // Send WM_CLOSE to all active MDI windows
   BroadcastMessage(WM_CLOSE, 0, 0, FALSE);

   CMDIFrameWnd::OnClose();
}


//
// Overrided frame window title update
//

void CMainFrame::OnUpdateFrameTitle(BOOL bAddToTitle)
{
	CMDIChildWnd *pActiveChild = NULL;

   if (!bAddToTitle)
   {
      CMDIFrameWnd::OnUpdateFrameTitle(FALSE);
   }
   else
   {
      pActiveChild = MDIGetActive();
      if (pActiveChild != NULL)
      {
         if (pActiveChild->GetStyle() & WS_MAXIMIZE)
         {
            CMDIFrameWnd::OnUpdateFrameTitle(FALSE);
         }
         else
         {
            UpdateFrameTitleForDocument(pActiveChild->GetTitle());
         }
      }
      else
      {
         CMDIFrameWnd::OnUpdateFrameTitle(FALSE);
      }
   }
}


//
// Desktop saving worker function
//

static DWORD SaveDesktop(TCHAR *pszDesktopName, DWORD dwWindowCount, WINDOW_SAVE_INFO *pWndSaveInfo)
{
   DWORD i, dwResult, dwLastChar;
   TCHAR szVar[MAX_VARIABLE_NAME], szBuffer[MAX_DB_STRING];

   _sntprintf(szVar, MAX_VARIABLE_NAME - 32, _T("/Win32Console/Desktop/%s/"), pszDesktopName);
   dwLastChar = (DWORD)_tcslen(szVar);

   // Set window count
   _tcscpy(&szVar[dwLastChar], _T("WindowCount"));
   _sntprintf_s(szBuffer, MAX_DB_STRING, _TRUNCATE, _T("%d"), dwWindowCount);
   dwResult = NXCSetUserVariable(g_hSession, CURRENT_USER, szVar, szBuffer);

   for(i = 0; (i < dwWindowCount) && (dwResult == RCC_SUCCESS); i++)
   {
      _sntprintf_s(&szVar[dwLastChar], MAX_VARIABLE_NAME - dwLastChar, _TRUNCATE, _T("wnd_%d/class"), i);
      _sntprintf_s(szBuffer, MAX_DB_STRING, _TRUNCATE, _T("%d"), pWndSaveInfo[i].iWndClass);
      dwResult = NXCSetUserVariable(g_hSession, CURRENT_USER, szVar, szBuffer);

      if (dwResult == RCC_SUCCESS)
      {
         _sntprintf_s(&szVar[dwLastChar], MAX_VARIABLE_NAME - dwLastChar, _TRUNCATE, _T("wnd_%d/props"), i);
         dwResult = NXCSetUserVariable(g_hSession, CURRENT_USER, szVar, pWndSaveInfo[i].szParameters);
      }

      if (dwResult == RCC_SUCCESS)
      {
         _sntprintf_s(&szVar[dwLastChar], MAX_VARIABLE_NAME - dwLastChar, _TRUNCATE, _T("wnd_%d/placement"), i);
         BinToStr((BYTE *)&pWndSaveInfo[i].placement, sizeof(WINDOWPLACEMENT), szBuffer);
         dwResult = NXCSetUserVariable(g_hSession, CURRENT_USER, szVar, szBuffer);
      }
   }

   return dwResult;
}


//
// WM_COMMAND::ID_DESKTOP_SAVE message handler
//

void CMainFrame::OnDesktopSave() 
{
   if (m_szDesktopName[0] == 0)
   {
      SendMessage(WM_COMMAND, ID_DESKTOP_SAVEAS, 0);
   }
   else
   {
      CWnd *pWnd;
      DWORD dwWindowCount = 0, dwResult;
      WINDOW_SAVE_INFO *pWndSaveInfo;

      pWndSaveInfo = (WINDOW_SAVE_INFO *)malloc(sizeof(WINDOW_SAVE_INFO) * MAX_WINDOW_COUNT);
      pWnd = MDIGetActive();
      while(pWnd != NULL)
      {
         if (pWnd->SendMessage(NXCM_GET_SAVE_INFO, 0, (LPARAM)&pWndSaveInfo[dwWindowCount]) != 0)
         {
            dwWindowCount++;
         }
         pWnd = pWnd->GetNextWindow();
      }

      dwResult = DoRequestArg3(SaveDesktop, m_szDesktopName, (void *)dwWindowCount,
                               pWndSaveInfo, _T("Saving desktop..."));
      if (dwResult != RCC_SUCCESS)
      {
         theApp.ErrorBox(dwResult, _T("Error saving desktop configuration: %s"));
      }
      free(pWndSaveInfo);
   }
}


//
// WM_COMMAND::ID_DESKTOP_SAVEAS message handler
//

void CMainFrame::OnDesktopSaveas() 
{
   CSaveDesktopDlg dlg;

   if (dlg.DoModal() == IDOK)
   {
      TCHAR szName[MAX_OBJECT_NAME];

      nx_strncpy(szName, (LPCTSTR)dlg.m_strName, MAX_OBJECT_NAME);
      StrStrip(szName);
      if (szName[0] != 0)
      {
         _tcscpy(m_szDesktopName, szName);
         SendMessage(WM_COMMAND, ID_DESKTOP_SAVE, 0);
         SetDesktopIndicator();
      }
   }
}


//
// Set indicator on status bar to current desktop name
//

void CMainFrame::SetDesktopIndicator()
{
   if (m_szDesktopName[0] != 0)
   {
      m_wndStatusBar.GetStatusBarCtrl().SetIcon(2,
         (HICON)LoadImage(theApp.m_hInstance, MAKEINTRESOURCE(IDI_DESKTOP), 
                          IMAGE_ICON, 16, 16, LR_SHARED));
      m_wndStatusBar.GetStatusBarCtrl().SetText(m_szDesktopName, 2, 0);
   }
   else
   {
      m_wndStatusBar.GetStatusBarCtrl().SetIcon(2, NULL);
      m_wndStatusBar.GetStatusBarCtrl().SetText(_T(""), 2, 0);
   }
}


//
// Load desktop configuration from server
//

static DWORD LoadDesktop(TCHAR *pszName, DWORD *pdwWindowCount, WINDOW_SAVE_INFO **ppInfo)
{
   DWORD i, dwResult, dwLastChar;
   TCHAR szBuffer[256], szVar[MAX_VARIABLE_NAME];

   _sntprintf_s(szVar, MAX_VARIABLE_NAME - 32, _TRUNCATE, _T("/Win32Console/Desktop/%s/"), pszName);
   dwLastChar = (DWORD)_tcslen(szVar);

   // Set window count
   _tcscpy(&szVar[dwLastChar], _T("WindowCount"));
   dwResult = NXCGetUserVariable(g_hSession, CURRENT_USER, szVar, szBuffer, 256);
   if (dwResult == RCC_SUCCESS)
   {
      *pdwWindowCount = _tcstoul(szBuffer, 0, NULL);
      *ppInfo = (WINDOW_SAVE_INFO *)malloc(sizeof(WINDOW_SAVE_INFO) * (*pdwWindowCount));
      
      for(i = 0; i < *pdwWindowCount; i++)
      {
         _sntprintf_s(&szVar[dwLastChar], MAX_VARIABLE_NAME - dwLastChar, _TRUNCATE, _T("wnd_%d/class"), i);
         dwResult = NXCGetUserVariable(g_hSession, CURRENT_USER, szVar, szBuffer, 256);
         if (dwResult == RCC_SUCCESS)
         {
            (*ppInfo)[i].iWndClass = _tcstol(szBuffer, 0, NULL);

            _sntprintf_s(&szVar[dwLastChar], MAX_VARIABLE_NAME - dwLastChar, _TRUNCATE, _T("wnd_%d/placement"), i);
            dwResult = NXCGetUserVariable(g_hSession, CURRENT_USER, szVar, szBuffer, 256);
         }
         if (dwResult == RCC_SUCCESS)
         {
            StrToBin(szBuffer, (BYTE *)&(*ppInfo)[i].placement, sizeof(WINDOWPLACEMENT));

            _sntprintf_s(&szVar[dwLastChar], MAX_VARIABLE_NAME - dwLastChar, _TRUNCATE, _T("wnd_%d/props"), i);
            dwResult = NXCGetUserVariable(g_hSession, CURRENT_USER, szVar,
                                          (*ppInfo)[i].szParameters, MAX_WND_PARAM_LEN);
         }
      }

      if (dwResult != RCC_SUCCESS)
         safe_free(*ppInfo);
   }

   return dwResult;
}


//
// WM_COMMAND::ID_DESKTOP_RESTORE message handler
//

void CMainFrame::OnDesktopRestore() 
{
   CSaveDesktopDlg dlg;
   DWORD i, dwResult, dwWindowCount;
   WINDOW_SAVE_INFO *pWndInfo;
   TCHAR szName[MAX_OBJECT_NAME];
   CMDIChildWnd *pWnd;

   dlg.m_bRestore = TRUE;
   if (dlg.DoModal() == IDOK)
   {
      nx_strncpy(szName, (LPCTSTR)dlg.m_strName, MAX_OBJECT_NAME);
      StrStrip(szName);

      // Close existing windows
      BroadcastMessage(WM_CLOSE, 0, 0, FALSE);

      m_szDesktopName[0] = 0;
      dwResult = DoRequestArg3(LoadDesktop, szName, &dwWindowCount, &pWndInfo,
                               _T("Loading desktop configuration..."));
      if (dwResult == RCC_SUCCESS)
      {
         for(i = 0; i < dwWindowCount; i++)
         {
            switch(pWndInfo[i].iWndClass)
            {
               case WNDC_LAST_VALUES:
                  pWnd = theApp.ShowLastValues(NULL, pWndInfo[i].szParameters);
                  break;
               case WNDC_DCI_DATA:
                  pWnd = theApp.ShowDCIData(0, 0, NULL, pWndInfo[i].szParameters);
                  break;
               case WNDC_GRAPH:
                  pWnd = theApp.ShowDCIGraph(0, 0, NULL, NULL, pWndInfo[i].szParameters);
                  break;
               case WNDC_NETWORK_SUMMARY:
                  pWnd = theApp.ShowNetworkSummary();
                  break;
               case WNDC_ALARM_BROWSER:
                  pWnd = theApp.ShowAlarmBrowser(pWndInfo[i].szParameters);
                  break;
               case WNDC_EVENT_BROWSER:
                  pWnd = theApp.ShowEventBrowser();
                  break;
               case WNDC_SYSLOG_BROWSER:
                  pWnd = theApp.ShowSyslogBrowser();
                  break;
               case WNDC_TRAP_LOG_BROWSER:
                  pWnd = theApp.ShowTrapLogBrowser();
                  break;
               case WNDC_OBJECT_BROWSER:
                  pWnd = theApp.ShowObjectBrowser(pWndInfo[i].szParameters);
                  break;
               case WNDC_CONTROL_PANEL:
                  pWnd = theApp.ShowControlPanel();
                  break;
               default:
                  pWnd = NULL;
                  break;
            }

            if (pWnd != NULL)
            {
               RestoreMDIChildPlacement(pWnd, &pWndInfo[i].placement);
            }
         }

         safe_free(pWndInfo);
         _tcscpy(m_szDesktopName, szName);
      }

      SetDesktopIndicator();
   }
}


//
// WM_COMMAND::ID_DESKTOP_NEW message handler
//

void CMainFrame::OnDesktopNew() 
{
   m_szDesktopName[0] = 0;
   SetDesktopIndicator();
}


//
// NXCM_SHOW_FATAL_ERROR message handler
//

LRESULT CMainFrame::OnShowFatalError(WPARAM wParam, LPARAM lParam)
{
	return ((CFatalErrorDlg *)lParam)->DoModal() != IDOK;		// IDOK == Debug
}
