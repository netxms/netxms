// EventBrowser.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "EventBrowser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEventBrowser

IMPLEMENT_DYNCREATE(CEventBrowser, CMDIChildWnd)

CEventBrowser::CEventBrowser()
{
   m_pImageList = NULL;
}

CEventBrowser::~CEventBrowser()
{
   delete m_pImageList;
}


BEGIN_MESSAGE_MAP(CEventBrowser, CMDIChildWnd)
	//{{AFX_MSG_MAP(CEventBrowser)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEventBrowser message handlers

BOOL CEventBrowser::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         NULL, 
                                         GetSysColorBrush(COLOR_WINDOW), 
                                         AfxGetApp()->LoadIcon(IDI_LOG));
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CEventBrowser::OnCreate(LPCREATESTRUCT lpCreateStruct) 
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

   // Create image list
   m_pImageList = CreateEventImageList();
   m_wndListCtrl.SetImageList(m_pImageList, LVSIL_SMALL);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, "Time", LVCFMT_LEFT, 135);
   m_wndListCtrl.InsertColumn(1, "Severity", LVCFMT_LEFT, 70);
   m_wndListCtrl.InsertColumn(2, "Source", LVCFMT_LEFT, 140);
   m_wndListCtrl.InsertColumn(3, "Message", LVCFMT_LEFT, 500);
	
   ((CConsoleApp *)AfxGetApp())->OnViewCreate(IDR_EVENTS, this);

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
	return 0;
}


//
// WM_DESTROY message handler
//

void CEventBrowser::OnDestroy() 
{
   ((CConsoleApp *)AfxGetApp())->OnViewDestroy(IDR_EVENTS, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_SIZE message handler
//

void CEventBrowser::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
	
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_SETFOCUS message handler
//

void CEventBrowser::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
	
   m_wndListCtrl.SetFocus();	
}


//
// Add new event record (normally received from the server) to the list
//

void CEventBrowser::AddEvent(NXC_EVENT *pEvent)
{
   int iIdx;
   struct tm *ptm;
   char szBuffer[64];

   ptm = localtime((const time_t *)&pEvent->dwTimeStamp);
   strftime(szBuffer, 32, "%d-%b-%Y %H:%M:%S", ptm);
   iIdx = m_wndListCtrl.InsertItem(0x7FFFFFFF, szBuffer, pEvent->dwSeverity);
   if (iIdx != -1)
   {
      NXC_OBJECT *pObject;

      m_wndListCtrl.SetItemText(iIdx, 1, g_szStatusTextSmall[pEvent->dwSeverity]);
      pObject = NXCFindObjectById(pEvent->dwSourceId);
      m_wndListCtrl.SetItemText(iIdx, 2, pObject ? pObject->szName : "<unknown>");
      m_wndListCtrl.SetItemText(iIdx, 3, pEvent->szMessage);

      m_wndListCtrl.EnsureVisible(iIdx, FALSE);
   }
}


//
// Enable or disable data display
// This function is useful when large amount of data being added
//

void CEventBrowser::EnableDisplay(BOOL bEnable)
{
   m_wndListCtrl.ShowWindow(bEnable ? SW_SHOW : SW_HIDE);
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CEventBrowser::OnViewRefresh() 
{
   EnableDisplay(FALSE);
   DoRequest(NXCSyncEvents, "Loading events...");
   EnableDisplay(TRUE);
}
