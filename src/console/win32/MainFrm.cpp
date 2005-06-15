// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "nxcon.h"

#include "MainFrm.h"
#include "SaveDesktopDlg.h"
#include "LastValuesView.h"

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
	ON_COMMAND(ID_UPDATE_EVENT_LIST, OnUpdateEventList)
	ON_WM_CLOSE()
	ON_COMMAND(ID_DESKTOP_SAVE, OnDesktopSave)
	ON_COMMAND(ID_DESKTOP_SAVEAS, OnDesktopSaveas)
	ON_COMMAND(ID_DESKTOP_RESTORE, OnDesktopRestore)
	ON_COMMAND(ID_DESKTOP_NEW, OnDesktopNew)
	//}}AFX_MSG_MAP
//   ON_UPDATE_COMMAND_UI(ID_INDICATOR_CONNECT, OnUpdateConnState)
   ON_MESSAGE(WM_OBJECT_CHANGE, OnObjectChange)
   ON_MESSAGE(WM_USERDB_CHANGE, OnUserDBChange)
   ON_MESSAGE(WM_STATE_CHANGE, OnStateChange)
   ON_MESSAGE(WM_ALARM_UPDATE, OnAlarmUpdate)
   ON_MESSAGE(WM_DEPLOYMENT_INFO, OnDeploymentInfo)
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
	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_wndToolBar.CreateEx(this) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

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
   m_wndStatusBar.GetStatusBarCtrl().SetText("", 1, 0);
   m_wndStatusBar.GetStatusBarCtrl().SetIcon(2, NULL);
   m_wndStatusBar.GetStatusBarCtrl().SetText("", 2, 0);

	// TODO: Remove this if you don't want tool tips
	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY);

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CMDIFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

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
   theApp.DebugPrintf("CMainFrame::BroadcastMessage(%d, %d, %d)", msg, wParam, lParam);
}


//
// Handler for WM_OBJECT_CHANGE message
//

void CMainFrame::OnObjectChange(WPARAM wParam, LPARAM lParam)
{
   BroadcastMessage(WM_OBJECT_CHANGE, wParam, lParam, TRUE);
}


//
// Handler for WM_USERDB_CHANGE message
//

void CMainFrame::OnUserDBChange(WPARAM wParam, LPARAM lParam)
{
   BroadcastMessage(WM_USERDB_CHANGE, wParam, lParam, TRUE);
}


//
// Handler for WM_DEPLOYMENT_INFO message
//

void CMainFrame::OnDeploymentInfo(WPARAM wParam, LPARAM lParam)
{
   BroadcastMessage(WM_DEPLOYMENT_INFO, wParam, lParam, FALSE);
   safe_free(((NXC_DEPLOYMENT_STATUS *)lParam)->pszErrorMessage)
   free((void *)lParam);
}


//
// WM_COMMAND::ID_UPDATE_EVENT_LIST message handler
//

void CMainFrame::OnUpdateEventList(void) 
{
   DoRequestArg1(NXCLoadEventDB, g_hSession, "Reloading event information...");
}


//
// Handler for WM_STATE_CHANGE message
//

void CMainFrame::OnStateChange(WPARAM wParam, LPARAM lParam)
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
      m_wndStatusBar.GetStatusBarCtrl().SetText("", 1, 0);
   }
}


//
// WM_ALARM_UPDATE message handler
//

void CMainFrame::OnAlarmUpdate(WPARAM wParam, LPARAM lParam)
{
   CAlarmBrowser *pWnd;

   pWnd = theApp.GetAlarmBrowser();
   if (pWnd != NULL)
      pWnd->OnAlarmUpdate(wParam, (NXC_ALARM *)lParam);
   free((void *)lParam);
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
   dwLastChar = _tcslen(szVar);

   // Set window count
   _tcscpy(&szVar[dwLastChar], _T("WindowCount"));
   _stprintf(szBuffer, _T("%d"), dwWindowCount);
   dwResult = NXCSetUserVariable(g_hSession, szVar, szBuffer);

   for(i = 0; (i < dwWindowCount) && (dwResult == RCC_SUCCESS); i++)
   {
      _stprintf(&szVar[dwLastChar], _T("wnd_%d/class"), i);
      _stprintf(szBuffer, _T("%d"), pWndSaveInfo[i].iWndClass);
      dwResult = NXCSetUserVariable(g_hSession, szVar, szBuffer);

      if (dwResult == RCC_SUCCESS)
      {
         _stprintf(&szVar[dwLastChar], _T("wnd_%d/props"), i);
         dwResult = NXCSetUserVariable(g_hSession, szVar, pWndSaveInfo[i].szParameters);
      }

      if (dwResult == RCC_SUCCESS)
      {
         _stprintf(&szVar[dwLastChar], _T("wnd_%d/placement"), i);
         BinToStr((BYTE *)&pWndSaveInfo[i].placement, sizeof(WINDOWPLACEMENT), szBuffer);
         dwResult = NXCSetUserVariable(g_hSession, szVar, szBuffer);
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
         if (pWnd->SendMessage(WM_GET_SAVE_INFO, 0, (LPARAM)&pWndSaveInfo[dwWindowCount]) != 0)
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

      _tcsncpy(szName, (LPCTSTR)dlg.m_strName, MAX_OBJECT_NAME);
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

   _sntprintf(szVar, MAX_VARIABLE_NAME - 32, _T("/Win32Console/Desktop/%s/"), pszName);
   dwLastChar = _tcslen(szVar);

   // Set window count
   _tcscpy(&szVar[dwLastChar], _T("WindowCount"));
   dwResult = NXCGetUserVariable(g_hSession, szVar, szBuffer, 256);
   if (dwResult == RCC_SUCCESS)
   {
      *pdwWindowCount = _tcstoul(szBuffer, 0, NULL);
      *ppInfo = (WINDOW_SAVE_INFO *)malloc(sizeof(WINDOW_SAVE_INFO) * (*pdwWindowCount));
      
      for(i = 0; i < *pdwWindowCount; i++)
      {
         _stprintf(&szVar[dwLastChar], _T("wnd_%d/class"), i);
         dwResult = NXCGetUserVariable(g_hSession, szVar, szBuffer, 256);
         if (dwResult == RCC_SUCCESS)
         {
            (*ppInfo)[i].iWndClass = _tcstol(szBuffer, 0, NULL);

            _stprintf(&szVar[dwLastChar], _T("wnd_%d/placement"), i);
            dwResult = NXCGetUserVariable(g_hSession, szVar, szBuffer, 256);
         }
         if (dwResult == RCC_SUCCESS)
         {
            StrToBin(szBuffer, (BYTE *)&(*ppInfo)[i].placement, sizeof(WINDOWPLACEMENT));

            _stprintf(&szVar[dwLastChar], _T("wnd_%d/props"), i);
            dwResult = NXCGetUserVariable(g_hSession, szVar,
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
      _tcsncpy(szName, (LPCTSTR)dlg.m_strName, MAX_OBJECT_NAME);
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
               case WNDC_OBJECT_BROWSER:
                  pWnd = theApp.ShowObjectBrowser(pWndInfo[i].szParameters);
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
