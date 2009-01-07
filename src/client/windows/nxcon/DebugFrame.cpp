// DebugFrame.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "DebugFrame.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDebugFrame

IMPLEMENT_DYNCREATE(CDebugFrame, CMDIChildWnd)

CDebugFrame::CDebugFrame()
{
}

CDebugFrame::~CDebugFrame()
{
}


BEGIN_MESSAGE_MAP(CDebugFrame, CMDIChildWnd)
	//{{AFX_MSG_MAP(CDebugFrame)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//
// WM_CREATE message handler
//

int CDebugFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   // Create list view control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT, rect, this, ID_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT | 
                                  LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, _T("Time"), LVCFMT_LEFT, 120);
   m_wndListCtrl.InsertColumn(1, _T("Message"), LVCFMT_LEFT, 600);
	
   theApp.OnViewCreate(VIEW_DEBUG, this);

	return 0;
}


//
// WM_DESTROY message handler
//

void CDebugFrame::OnDestroy() 
{
   theApp.OnViewDestroy(VIEW_DEBUG, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_SIZE message handler
//

void CDebugFrame::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
	
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_SETFOCUS message handler
//

void CDebugFrame::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
	
   m_wndListCtrl.SetFocus();	
}


//
// CDebugFrame::PreCreateWindow
//

BOOL CDebugFrame::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         NULL, 
                                         GetSysColorBrush(COLOR_WINDOW), 
                                         AfxGetApp()->LoadIcon(IDI_LOG));
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// Add new message to log
//

void CDebugFrame::AddMessage(TCHAR *pszMsg)
{
   int iIdx;
   time_t currTime;
   struct tm *ptm;
   TCHAR szBuffer[64];

   currTime = time(NULL);
   ptm = localtime((const time_t *)&currTime);
   _tcsftime(szBuffer, 32, _T("%d-%b-%Y %H:%M:%S"), ptm);
   iIdx = m_wndListCtrl.InsertItem(0x7FFFFFFF, szBuffer, 0);
   if (iIdx != -1)
   {
      m_wndListCtrl.SetItemText(iIdx, 1, pszMsg);
      m_wndListCtrl.EnsureVisible(iIdx, FALSE);
   }
}
