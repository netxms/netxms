// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "nxpc.h"

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
	//}}AFX_MSG_MAP
   ON_MESSAGE(WM_OBJECT_CHANGE, OnObjectChange)
   ON_MESSAGE(WM_ALARM_UPDATE, OnAlarmUpdate)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
   m_pwndCurrView = NULL;
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
   RECT rect;

	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   // Create views
   GetClientRect(&rect);
   m_wndSummaryView.Create(NULL, NULL, WS_CHILD, rect, this, 0, NULL);
   m_wndAlarmView.Create(NULL, NULL, WS_CHILD, rect, this, 0, NULL);
   m_wndObjectView.Create(NULL, NULL, WS_CHILD, rect, this, 0, NULL);

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

   ActivateView(&m_wndSummaryView);

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs


	cs.lpszClass = AfxRegisterWndClass(0);
	return TRUE;
}



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
   if (m_pwndCurrView != NULL)
      m_pwndCurrView->ShowWindow(SW_HIDE);
   m_pwndCurrView = pwndView;
   m_pwndCurrView->ShowWindow(SW_SHOW);
}


//
// WM_SIZE message handler
//

void CMainFrame::OnSize(UINT nType, int cx, int cy) 
{
	CFrameWnd::OnSize(nType, cx, cy);

   m_wndAlarmView.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
   m_wndObjectView.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
   m_wndSummaryView.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
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
