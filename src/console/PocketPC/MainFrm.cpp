// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "nxpc.h"

#define VIEW_TITLE_SIZE    20

#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define PSPC_TOOLBAR_HEIGHT 24

const DWORD dwAdornmentFlags = 0; // exit button

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_VIEW_OBJECTS, OnViewObjects)
	ON_COMMAND(ID_VIEW_SUMMARY, OnViewSummary)
	ON_COMMAND(ID_VIEW_ALARMS, OnViewAlarms)
	ON_WM_SIZE()
	ON_COMMAND(ID_VIEW_REFRESH_ALL, OnViewRefreshAll)
	ON_WM_PAINT()
	ON_COMMAND(ID_VIEW_NEXT, OnViewNext)
	ON_COMMAND(ID_VIEW_PREV, OnViewPrev)
	ON_WM_ACTIVATE()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_VIEW_FULLSCREEN, OnViewFullscreen)
	//}}AFX_MSG_MAP
   ON_MESSAGE(WM_OBJECT_CHANGE, OnObjectChange)
   ON_MESSAGE(WM_ALARM_UPDATE, OnAlarmUpdate)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
   m_pwndCurrView = NULL;
   m_rgbTitleBkgnd = GetSysColor(COLOR_3DFACE);
   m_rgbTitleText = RGB(255, 255, 255);
   memset(m_pwndViewList, 0, sizeof(CDynamicView *) * (MAX_DYNAMIC_VIEWS + 3));
   m_dwNumViews = 0;
   m_bFullScreen = FALSE;
}

CMainFrame::~CMainFrame()
{
}


//
// WM_CREATE message handler
//

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
   static UINT nCmdArray[4] = { ID_VIEW_SUMMARY, ID_VIEW_ALARMS, ID_VIEW_OBJECTS, ID_APP_ABOUT };
   RECT rect, rcButton;

	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   // Create views
   GetClientRect(&rect);
   rect.top += VIEW_TITLE_SIZE;
   m_wndSummaryView.Create(NULL, _T("Network Status Summary"), WS_CHILD | WS_VISIBLE, rect, this, 0, NULL);
   m_wndAlarmView.Create(NULL, _T("Alarm Browser"), WS_CHILD | WS_VISIBLE, rect, this, 0, NULL);
   m_wndObjectView.Create(NULL, _T("Object Browser"), WS_CHILD | WS_VISIBLE, rect, this, 0, NULL);

   m_pwndViewList[0] = &m_wndSummaryView;
   m_pwndViewList[1] = &m_wndAlarmView;
   m_pwndViewList[2] = &m_wndObjectView;
   m_dwNumViews = 3;

   // Create control bar
	m_wndCommandBar.m_bShowSharedNewButton = FALSE;
	m_ToolTipsTable[0] = L"0";
	m_ToolTipsTable[1] = L"1";
	m_ToolTipsTable[2] = MakeString(IDS_SUMMARY);
	m_ToolTipsTable[3] = MakeString(IDS_ALARMS);
	m_ToolTipsTable[4] = MakeString(IDS_OBJECTS);
	m_ToolTipsTable[5] = MakeString(IDS_REFRESH);

	if(!m_wndCommandBar.Create(this) ||
	   !m_wndCommandBar.InsertMenuBar(IDR_MAINFRAME) ||
	   !m_wndCommandBar.AddAdornments() ||
      !m_wndCommandBar.LoadToolBar(IDR_MAINFRAME) ||
   	!m_wndCommandBar.SendMessage(TB_SETTOOLTIPS, (WPARAM)(6), (LPARAM)m_ToolTipsTable))
	{
		TRACE0("Failed to create CommandBar\n");
		return -1;      // fail to create
	}

	m_wndCommandBar.SetBarStyle(m_wndCommandBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_FIXED);

   // Create view control buttons
   rcButton.top = 2;
   rcButton.bottom = 16;
   rcButton.right = rect.right - 2;
   rcButton.left = rcButton.right - 14;
   m_wndBtnClose.Create(_T("Close"), WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, rcButton,
                        this, ID_VIEW_CLOSE);
   m_wndBtnClose.LoadBitmaps(IDB_BTN_CLOSE, 0, 0, IDB_BTN_CLOSE_DIS);
   m_wndBtnClose.SizeToContent();

   rcButton.left -= 16;
   m_wndBtnNext.Create(_T("Next"), WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, rcButton,
                       this, ID_VIEW_NEXT);
   m_wndBtnNext.LoadBitmaps(IDB_BTN_NEXT);
   m_wndBtnNext.SizeToContent();

   rcButton.left -= 16;
   m_wndBtnPrev.Create(_T("Previous"), WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, rcButton,
                       this, ID_VIEW_PREV);
   m_wndBtnPrev.LoadBitmaps(IDB_BTN_PREV);
   m_wndBtnPrev.SizeToContent();

   ActivateView(&m_wndSummaryView);

	return 0;
}


//
// Overriden PreCreateWindow method
//

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;

	cs.lpszClass = AfxRegisterWndClass(0);
	return TRUE;
}


//
// Make string from resource
//

LPTSTR CMainFrame::MakeString(UINT stringID)
{
	TCHAR buffer[255];
	TCHAR* theString;

	::LoadString(AfxGetInstanceHandle(), stringID, buffer, 255);
	theString = new TCHAR[lstrlen(buffer) + 1];
	lstrcpy(theString, buffer);
	return theString;
}   

 
/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers
void CMainFrame::OnSetFocus(CWnd* pOldWnd)
{
	// Forward focus to the view window
   if (m_pwndCurrView != NULL)
	   m_pwndCurrView->SetFocus();
}

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	// Let the view have first crack at the command
   if (m_pwndCurrView != NULL)
	   if (m_pwndCurrView->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		   return TRUE;

	// otherwise, do default handling
	return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}


//
// WM_COMMAND::ID_VIEW_SUMMARY message handler
//

void CMainFrame::OnViewSummary() 
{
   ActivateView(&m_wndSummaryView);
}


//
// WM_COMMAND::ID_VIEW_ALARMS message handler
//

void CMainFrame::OnViewAlarms() 
{
   ActivateView(&m_wndAlarmView);
}


//
// WM_COMMAND::ID_VIEW_OBJECTS message handler
//

void CMainFrame::OnViewObjects() 
{
   ActivateView(&m_wndObjectView);
}


//
// Activate view
//

void CMainFrame::ActivateView(CWnd *pwndView)
{
   RECT rect;

   m_pwndCurrView = pwndView;
   m_pwndCurrView->SetWindowPos(&wndTop, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);

   GetClientRect(&rect);
   rect.bottom = VIEW_TITLE_SIZE - 1;
   InvalidateRect(&rect);

   // Enable or disable "Close" button
   m_wndBtnClose.EnableWindow(m_pwndCurrView->GetDlgCtrlID() != 0);
}


//
// WM_SIZE message handler
//

void CMainFrame::OnSize(UINT nType, int cx, int cy) 
{
   DWORD i;

   if (m_bFullScreen)
   {
      CSize scrSize;
      
      scrSize = GetScreenSize();
      if ((cx != scrSize.cx) || (cy != scrSize.cy))
      {
         SetWindowPos(NULL, 0, 0, scrSize.cx, scrSize.cy, SWP_NOZORDER);
         return;
      }
   }

	CFrameWnd::OnSize(nType, cx, cy);

   m_wndAlarmView.SetWindowPos(NULL, 0, VIEW_TITLE_SIZE, cx, cy - VIEW_TITLE_SIZE, SWP_NOZORDER);
   m_wndObjectView.SetWindowPos(NULL, 0, VIEW_TITLE_SIZE, cx, cy - VIEW_TITLE_SIZE, SWP_NOZORDER);
   m_wndSummaryView.SetWindowPos(NULL, 0, VIEW_TITLE_SIZE, cx, cy - VIEW_TITLE_SIZE, SWP_NOZORDER);
   for(i = 0; i < MAX_DYNAMIC_VIEWS; i++)
      if (m_pwndViewList[i] != NULL)
         m_pwndViewList[i]->SetWindowPos(NULL, 0, VIEW_TITLE_SIZE, cx, cy - VIEW_TITLE_SIZE, SWP_NOZORDER);
   m_wndBtnClose.SetWindowPos(NULL, cx - 16, 2, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
   m_wndBtnNext.SetWindowPos(NULL, cx - 32, 2, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
   m_wndBtnPrev.SetWindowPos(NULL, cx - 48, 2, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}


//
// Handler for WM_OBJECT_CHANGE message
//

void CMainFrame::OnObjectChange(WPARAM wParam, LPARAM lParam)
{
   m_wndObjectView.OnObjectChange(wParam, (NXC_OBJECT *)lParam);
}


//
// WM_ALARM_UPDATE message handler
//

void CMainFrame::OnAlarmUpdate(WPARAM wParam, LPARAM lParam)
{
   m_wndAlarmView.OnAlarmUpdate(wParam, (NXC_ALARM *)lParam);
   free((void *)lParam);
}


//
// Refresh all views
//

void CMainFrame::OnViewRefreshAll() 
{
   m_wndAlarmView.PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
   m_wndObjectView.PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
   m_wndSummaryView.PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
}


//
// WM_PAINT message handler
//

void CMainFrame::OnPaint() 
{
   RECT rect;
   TCHAR szBuffer[256];

	CPaintDC dc(this); // device context for painting

   GetClientRect(&rect);
   rect.bottom = VIEW_TITLE_SIZE - 1;
   dc.FillSolidRect(&rect, m_rgbTitleBkgnd);
   dc.MoveTo(0, VIEW_TITLE_SIZE - 1);
   dc.LineTo(rect.right, VIEW_TITLE_SIZE - 1);

   if (m_pwndCurrView != NULL)
   {
      m_pwndCurrView->GetWindowText(szBuffer, 256);
      rect.top += 2;
      rect.left += 5;
      rect.right -= 52;
      rect.bottom -= 2;
      dc.DrawText(szBuffer, -1, &rect, DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
   }
}


//
// Register new view
//

BOOL CMainFrame::RegisterView(CDynamicView *pView)
{
   BOOL bResult = FALSE;

   if (m_dwNumViews < MAX_DYNAMIC_VIEWS + 3)
   {
      m_pwndViewList[m_dwNumViews++] = pView;
      bResult = TRUE;
   }
   return bResult;
}


//
// Unregister view
//

void CMainFrame::UnregisterView(CDynamicView *pView)
{
   DWORD i;

   for(i = 0; i < m_dwNumViews; i++)
      if (m_pwndViewList[i] == pView)
      {
         m_dwNumViews--;
         memmove(&m_pwndViewList[i], &m_pwndViewList[i + 1],
                 sizeof(CWnd *) * (m_dwNumViews - i));
         if (m_pwndCurrView == pView)
            ActivateView(m_pwndViewList[(i > 0) ? (i - 1) : (m_dwNumViews - 1)]);
         break;
      }
}


//
// Create new view (already prepared for creation)
//

void CMainFrame::CreateView(CDynamicView *pwndView, TCHAR *pszTitle)
{
   RECT rect;
   DWORD i;

   // Check if we already have view with same fingerprints
   for(i = 3; i < m_dwNumViews; i++)
      if (((CDynamicView *)m_pwndViewList[i])->GetFingerprint() == pwndView->GetFingerprint())
         break;

   if (i == m_dwNumViews)
   {
      GetClientRect(&rect);
      rect.top += VIEW_TITLE_SIZE;
      if (pwndView->Create(NULL, pszTitle, WS_CHILD | WS_VISIBLE, rect, this, 1, NULL))
         ActivateView(pwndView);
   }
   else
   {
      delete pwndView;
      ActivateView(m_pwndViewList[i]);
   }
}


//
// WM_COMMAND::ID_VIEW_NEXT message handler
//

void CMainFrame::OnViewNext(void) 
{
   DWORD dwIndex;

   if (m_pwndCurrView != NULL)
   {
      dwIndex = FindViewInList(m_pwndCurrView);
      ActivateView(m_pwndViewList[(dwIndex < (m_dwNumViews - 1)) ? (dwIndex + 1) : 0]);
   }
}


//
// WM_COMMAND::ID_VIEW_PREV message handler
//

void CMainFrame::OnViewPrev(void) 
{
   DWORD dwIndex;

   if (m_pwndCurrView != NULL)
   {
      dwIndex = FindViewInList(m_pwndCurrView);
      ActivateView(m_pwndViewList[(dwIndex > 0) ? (dwIndex - 1) : (m_dwNumViews - 1)]);
   }
}


//
// Find view in list
//

DWORD CMainFrame::FindViewInList(CWnd *pwndView)
{
   DWORD i;

   for(i = 0; i < MAX_DYNAMIC_VIEWS + 3; i++)
      if (m_pwndViewList[i] == pwndView)
         return i;
   return 0;
}


//
// Switch to/from fullscreen mode
//

void CMainFrame::FullScreen(BOOL bFullScreen)
{
   if (!m_bFullScreen == !bFullScreen)
      return;     // Already in requested mode

   if (bFullScreen)
   {
      m_bFullScreen = TRUE;
      RefreshFullScreen();
   }
   else
   {
      CSize scrSize;
      int iMenuHeight;

      scrSize = GetScreenSize();
      iMenuHeight = GetSystemMetrics(SM_CYMENU);
      m_bFullScreen = FALSE;
      SetWindowPos(NULL, 0, iMenuHeight, scrSize.cx, scrSize.cy - iMenuHeight * 2, SWP_NOZORDER);
	   SHFullScreen(m_hWnd, SHFS_SHOWSTARTICON);
	   SHFullScreen(m_hWnd, SHFS_SHOWSIPBUTTON);
	   SHFullScreen(m_hWnd, SHFS_SHOWTASKBAR);
      m_wndCommandBar.ShowWindow(SW_SHOWNOACTIVATE);
   }
}


//
// Refresh fullscreen state
//

void CMainFrame::RefreshFullScreen(void)
{
   CSize scrSize;

   if (!m_bFullScreen)
      return;

	// Before hide command bar we should hipe SIP panel
	SHSipPreference(m_hWnd, SIP_FORCEDOWN);

	SetForegroundWindow();

	// If command bar should be hidden then we should resize our window
	// first to avoid blinking
   scrSize = GetScreenSize();
   SetWindowPos(NULL, 0, 0, scrSize.cx, scrSize.cy, SWP_NOZORDER);
   m_wndCommandBar.ShowWindow(SW_HIDE);

	SHFullScreen(m_hWnd, SHFS_HIDETASKBAR);
	SHFullScreen(m_hWnd, SHFS_HIDESIPBUTTON);
	SHFullScreen(m_hWnd, SHFS_HIDESTARTICON);
}


//
// WM_ACIVATE message hander
//

void CMainFrame::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized) 
{
	CFrameWnd::OnActivate(nState, pWndOther, bMinimized);
   //if (((nState == WA_ACTIVE) || (nState == WA_CLICKACTIVE)) && (m_bFullScreen))
   if (m_bFullScreen)
      RefreshFullScreen();
}


//
// WM_CONTEXTMENU message handler
//

void CMainFrame::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   CMenu *pMenu;

   pMenu = theApp.GetContextMenu(4);
   pMenu->CheckMenuItem(ID_VIEW_FULLSCREEN, MF_BYCOMMAND | (m_bFullScreen ? MF_CHECKED : 0));
   pMenu->TrackPopupMenu(TPM_LEFTALIGN, point.x, point.y, this, NULL);
}


//
// WM_COMMAND::ID_VIEW_FULLSCREEN
//

void CMainFrame::OnViewFullscreen() 
{
   ToggleFullScreen();
}
