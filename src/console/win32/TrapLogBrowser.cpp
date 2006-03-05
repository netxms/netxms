// TrapLogBrowser.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "TrapLogBrowser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTrapLogBrowser

IMPLEMENT_DYNCREATE(CTrapLogBrowser, CMDIChildWnd)

CTrapLogBrowser::CTrapLogBrowser()
{
   m_bIsBusy = FALSE;
}

CTrapLogBrowser::~CTrapLogBrowser()
{
}


BEGIN_MESSAGE_MAP(CTrapLogBrowser, CMDIChildWnd)
	//{{AFX_MSG_MAP(CTrapLogBrowser)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SETFOCUS()
	ON_WM_SIZE()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTrapLogBrowser message handlers

BOOL CTrapLogBrowser::PreCreateWindow(CREATESTRUCT& cs) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CTrapLogBrowser::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;
   DWORD dwResult;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   // Create list view control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT, rect, this, ID_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT | 
                                  LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, "Time", LVCFMT_LEFT, 135);
   m_wndListCtrl.InsertColumn(1, "Source IP", LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(2, "Source Object", LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(3, "Trap OID", LVCFMT_LEFT, 100);
   m_wndListCtrl.InsertColumn(4, "Varbinds", LVCFMT_LEFT, 500);
	
   // Create wait view
   m_wndWaitView.SetText(_T("Loading SNMP trap log..."));
   m_wndWaitView.Create(NULL, NULL, WS_CHILD, rect, this, ID_WAIT_VIEW);

   ((CConsoleApp *)AfxGetApp())->OnViewCreate(IDR_TRAP_LOG_BROWSER, this);
   dwResult = DoRequestArg2(NXCSubscribe, g_hSession,
                            (void *)NXC_CHANNEL_SNMP_TRAPS,
                            _T("Subscribing to SNMPTRAPS channel..."));
   if (dwResult != RCC_SUCCESS)
      theApp.ErrorBox(dwResult, _T("Cannot subscribe to SNMPTRAPS channel: %s"));

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
	return 0;
}


//
// WM_DESTROY message handler
//

void CTrapLogBrowser::OnDestroy() 
{
   DoRequestArg2(NXCUnsubscribe, g_hSession, (void *)NXC_CHANNEL_SNMP_TRAPS,
                 _T("Unsubscribing from SNMPTRAPS channel..."));
   ((CConsoleApp *)AfxGetApp())->OnViewDestroy(IDR_TRAP_LOG_BROWSER, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_SETFOCUS message handler
//

void CTrapLogBrowser::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
	
   if (m_bIsBusy)
      m_wndWaitView.SetFocus();
   else
      m_wndListCtrl.SetFocus();
}


//
// WM_SIZE message handler
//

void CTrapLogBrowser::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
	
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
   m_wndWaitView.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CTrapLogBrowser::OnViewRefresh() 
{
	// TODO: Add your command handler code here
	
}
