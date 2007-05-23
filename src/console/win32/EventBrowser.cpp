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
   m_bIsBusy = FALSE;
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
   ON_COMMAND(ID_REQUEST_COMPLETED, OnRequestCompleted)
   ON_MESSAGE(NXCM_GET_SAVE_INFO, OnGetSaveInfo)
   ON_MESSAGE(NXCM_NETXMS_EVENT, OnNetXMSEvent)
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
   DWORD dwResult;

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
   m_wndListCtrl.InsertColumn(0, _T("Time"), LVCFMT_LEFT, 135);
   m_wndListCtrl.InsertColumn(1, _T("Severity"), LVCFMT_LEFT, 70);
   m_wndListCtrl.InsertColumn(2, _T("Source"), LVCFMT_LEFT, 140);
   m_wndListCtrl.InsertColumn(3, _T("Event"), LVCFMT_LEFT, 140);
   m_wndListCtrl.InsertColumn(4, _T("Message"), LVCFMT_LEFT, 500);
	
   // Create wait view
   m_wndWaitView.SetText(_T("Loading event log..."));
   m_wndWaitView.Create(NULL, NULL, WS_CHILD, rect, this, ID_WAIT_VIEW);

   theApp.OnViewCreate(VIEW_EVENT_LOG, this);
   dwResult = DoRequestArg2(NXCSubscribe, g_hSession,
                            (void *)NXC_CHANNEL_EVENTS,
                            _T("Changing event subscription..."));
   if (dwResult != RCC_SUCCESS)
      theApp.ErrorBox(dwResult, _T("Error changing event subscription: %s"));

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
	return 0;
}


//
// WM_DESTROY message handler
//

void CEventBrowser::OnDestroy() 
{
   DoRequestArg2(NXCUnsubscribe, g_hSession, (void *)NXC_CHANNEL_EVENTS,
                 _T("Changing event subscription..."));
   theApp.OnViewDestroy(VIEW_EVENT_LOG, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_SIZE message handler
//

void CEventBrowser::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
	
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
   m_wndWaitView.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_SETFOCUS message handler
//

void CEventBrowser::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
	
   if (m_bIsBusy)
      m_wndWaitView.SetFocus();
   else
      m_wndListCtrl.SetFocus();
}


//
// Add new event record (normally received from the server) to the list
//

void CEventBrowser::AddEvent(NXC_EVENT *pEvent, BOOL bAppend)
{
   int iIdx;
   struct tm *ptm;
   TCHAR szBuffer[64];

   ptm = localtime((const time_t *)&pEvent->dwTimeStamp);
   _tcsftime(szBuffer, 32, _T("%d-%b-%Y %H:%M:%S"), ptm);
   iIdx = m_wndListCtrl.InsertItem(bAppend ? 0x7FFFFFFF : 0, szBuffer, pEvent->dwSeverity);
   if (iIdx != -1)
   {
      NXC_OBJECT *pObject;

      m_wndListCtrl.SetItemText(iIdx, 1, g_szStatusTextSmall[pEvent->dwSeverity]);
      pObject = NXCFindObjectById(g_hSession, pEvent->dwSourceId);
      m_wndListCtrl.SetItemText(iIdx, 2, pObject ? pObject->szName : _T("<unknown>"));
      m_wndListCtrl.SetItemText(iIdx, 3, NXCGetEventName(g_hSession, pEvent->dwEventCode));
      m_wndListCtrl.SetItemText(iIdx, 4, pEvent->szMessage);

      m_wndListCtrl.EnsureVisible(iIdx, FALSE);
   }
}


//
// Event loading thread
//

static THREAD_RESULT THREAD_CALL LoadEvents(void *pArg)
{
   NXCSyncEvents(g_hSession, g_dwMaxLogRecords);
   PostMessage((HWND)pArg, WM_COMMAND, ID_REQUEST_COMPLETED, 0);
	return THREAD_OK;
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CEventBrowser::OnViewRefresh() 
{
   if (!m_bIsBusy)
   {
      m_bIsBusy = TRUE;
      m_wndWaitView.ShowWindow(SW_SHOW);
      m_wndListCtrl.ShowWindow(SW_HIDE);
      m_wndWaitView.Start();
      m_wndListCtrl.DeleteAllItems();
      ThreadCreate(LoadEvents, 0, m_hWnd);
   }
}


//
// WM_REQUEST_COMPLETED message handler
//

void CEventBrowser::OnRequestCompleted(void)
{
   if (m_bIsBusy)
   {
      m_bIsBusy = FALSE;
      m_wndListCtrl.ShowWindow(SW_SHOW);
      m_wndWaitView.ShowWindow(SW_HIDE);
      m_wndWaitView.Stop();
   }
}


//
// Get save info for desktop saving
//

LRESULT CEventBrowser::OnGetSaveInfo(WPARAM wParam, WINDOW_SAVE_INFO *pInfo)
{
   pInfo->iWndClass = WNDC_EVENT_BROWSER;
   GetWindowPlacement(&pInfo->placement);
   pInfo->szParameters[0] = 0;
   return 1;
}


//
// WM_NETXMS_EVENT message handler
//

void CEventBrowser::OnNetXMSEvent(WPARAM wParam, NXC_EVENT *pEvent)
{
   AddEvent(pEvent, wParam == RECORD_ORDER_NORMAL);
   free(pEvent);
}
