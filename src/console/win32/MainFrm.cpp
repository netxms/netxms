// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "nxcon.h"

#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_COMMAND(ID_UPDATE_EVENT_LIST, OnUpdateEventList)
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
//   ON_UPDATE_COMMAND_UI(ID_INDICATOR_CONNECT, OnUpdateConnState)
   ON_MESSAGE(WM_OBJECT_CHANGE, OnObjectChange)
   ON_MESSAGE(WM_USERDB_CHANGE, OnUserDBChange)
   ON_MESSAGE(WM_STATE_CHANGE, OnStateChange)
   ON_MESSAGE(WM_ALARM_UPDATE, OnAlarmUpdate)
END_MESSAGE_MAP()

static UINT indicators[] =
{
   ID_SEPARATOR,
	ID_INDICATOR_CONNECT,
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL
};


//
// CMainFrame construction/destruction
//

CMainFrame::CMainFrame()
{
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
// WM_COMMAND::ID_UPDATE_EVENT_LIST message handler
//

void CMainFrame::OnUpdateEventList(void) 
{
   DoRequest(NXCLoadEventDB, "Reloading event information...");
}


//
// Handler for WM_STATE_CHANGE message
//

void CMainFrame::OnStateChange(WPARAM wParam, LPARAM lParam)
{
   if (wParam == STATE_CONNECTED)
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
