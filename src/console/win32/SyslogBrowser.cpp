// SyslogBrowser.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "SyslogBrowser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSyslogBrowser

IMPLEMENT_DYNCREATE(CSyslogBrowser, CMDIChildWnd)

CSyslogBrowser::CSyslogBrowser()
{
   m_pImageList = NULL;
   m_bIsBusy = FALSE;
}

CSyslogBrowser::~CSyslogBrowser()
{
   delete m_pImageList;
}


BEGIN_MESSAGE_MAP(CSyslogBrowser, CMDIChildWnd)
	//{{AFX_MSG_MAP(CSyslogBrowser)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	//}}AFX_MSG_MAP
   ON_COMMAND(ID_REQUEST_COMPLETED, OnRequestCompleted)
   ON_MESSAGE(NXCM_GET_SAVE_INFO, OnGetSaveInfo)
   ON_MESSAGE(NXCM_SYSLOG_RECORD, OnSyslogRecord)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSyslogBrowser message handlers

BOOL CSyslogBrowser::PreCreateWindow(CREATESTRUCT& cs) 
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

int CSyslogBrowser::OnCreate(LPCREATESTRUCT lpCreateStruct) 
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
   m_wndListCtrl.InsertColumn(0, "Time", LVCFMT_LEFT, 135);
   m_wndListCtrl.InsertColumn(1, "Severity", LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(2, "Facility", LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(3, "Hostname", LVCFMT_LEFT, 100);
   m_wndListCtrl.InsertColumn(4, "Tag", LVCFMT_LEFT, 90);
   m_wndListCtrl.InsertColumn(5, "Message", LVCFMT_LEFT, 500);
	
   // Create wait view
   m_wndWaitView.Create(NULL, NULL, WS_CHILD, rect, this, ID_WAIT_VIEW);

   ((CConsoleApp *)AfxGetApp())->OnViewCreate(IDR_SYSLOG_BROWSER, this);
   dwResult = DoRequestArg2(NXCSubscribe, g_hSession,
                            (void *)NXC_CHANNEL_SYSLOG,
                            _T("Subscribing to SYSLOG channel..."));
   if (dwResult != RCC_SUCCESS)
      theApp.ErrorBox(dwResult, _T("Cannot subscribe to SYSLOG channel: %s"));

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
	return 0;
}


//
// WM_DESTROY message handler
//

void CSyslogBrowser::OnDestroy() 
{
   DoRequestArg2(NXCUnsubscribe, g_hSession, (void *)NXC_CHANNEL_SYSLOG,
                 _T("Unsubscribing from SYSLOG channel..."));
   ((CConsoleApp *)AfxGetApp())->OnViewDestroy(IDR_SYSLOG_BROWSER, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_SIZE message handler
//

void CSyslogBrowser::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
	
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
   m_wndWaitView.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_SETFOCUS message handler
//

void CSyslogBrowser::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
	
   if (m_bIsBusy)
      m_wndWaitView.SetFocus();
   else
      m_wndListCtrl.SetFocus();
}


//
// Message loading thread
//

static void LoadMessages(void *pArg)
{
   NXCSyncSyslog(g_hSession, g_dwMaxLogRecords);
   PostMessage((HWND)pArg, WM_COMMAND, ID_REQUEST_COMPLETED, 0);
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CSyslogBrowser::OnViewRefresh() 
{
   if (!m_bIsBusy)
   {
      m_bIsBusy = TRUE;
      m_wndWaitView.ShowWindow(SW_SHOW);
      m_wndListCtrl.ShowWindow(SW_HIDE);
      m_wndWaitView.Start();
      m_wndListCtrl.DeleteAllItems();
      _beginthread(LoadMessages, 0, m_hWnd);
   }
}


//
// WM_REQUEST_COMPLETED message handler
//

void CSyslogBrowser::OnRequestCompleted(void)
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

LRESULT CSyslogBrowser::OnGetSaveInfo(WPARAM wParam, WINDOW_SAVE_INFO *pInfo)
{
   pInfo->iWndClass = WNDC_SYSLOG_BROWSER;
   GetWindowPlacement(&pInfo->placement);
   pInfo->szParameters[0] = 0;
   return 1;
}


//
// WM_SYSLOG_RECORD message handler
//

void CSyslogBrowser::OnSyslogRecord(WPARAM wParam, NXC_SYSLOG_RECORD *pRec)
{
   AddRecord(pRec);
   safe_free(pRec->pszText);
   free(pRec);
}


//
// Add new record to list
//

void CSyslogBrowser::AddRecord(NXC_SYSLOG_RECORD *pRec)
{
   int iIdx;
   struct tm *ptm;
   char szBuffer[64];
   static int nImage[8] = { 4, 4, 3, 3, 2, 1, 0, 0 };

   ptm = localtime((const time_t *)&pRec->dwTimeStamp);
   strftime(szBuffer, 32, "%d-%b-%Y %H:%M:%S", ptm);
   iIdx = m_wndListCtrl.InsertItem(0x7FFFFFFF, szBuffer, nImage[pRec->wSeverity]);
   if (iIdx != -1)
   {
      m_wndListCtrl.SetItemText(iIdx, 1, g_szSyslogSeverity[pRec->wSeverity]);
      if (pRec->wFacility <= 23)
      {
         m_wndListCtrl.SetItemText(iIdx, 2, g_szSyslogFacility[pRec->wFacility]);
      }
      else
      {
         _stprintf(szBuffer, _T("<%d>"), pRec->wFacility);
         m_wndListCtrl.SetItemText(iIdx, 2, szBuffer);
      }
      m_wndListCtrl.SetItemText(iIdx, 3, pRec->szHost);
      m_wndListCtrl.SetItemText(iIdx, 4, pRec->szTag);
      m_wndListCtrl.SetItemText(iIdx, 5, pRec->pszText);

      m_wndListCtrl.EnsureVisible(iIdx, FALSE);
   }
}
