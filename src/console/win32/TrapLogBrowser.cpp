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
   ON_COMMAND(ID_REQUEST_COMPLETED, OnRequestCompleted)
   ON_MESSAGE(NXCM_GET_SAVE_INFO, OnGetSaveInfo)
   ON_MESSAGE(NXCM_TRAP_LOG_RECORD, OnTrapLogRecord)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTrapLogBrowser message handlers

BOOL CTrapLogBrowser::PreCreateWindow(CREATESTRUCT& cs) 
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

   theApp.OnViewCreate(VIEW_TRAP_LOG, this);
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
   theApp.OnViewDestroy(VIEW_TRAP_LOG, this);
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
// Trap loading thread
//

static void LoadTraps(void *pArg)
{
   NXCSyncSNMPTrapLog(g_hSession, g_dwMaxLogRecords);
   PostMessage((HWND)pArg, WM_COMMAND, ID_REQUEST_COMPLETED, 0);
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CTrapLogBrowser::OnViewRefresh() 
{
   if (!m_bIsBusy)
   {
      m_bIsBusy = TRUE;
      m_wndWaitView.ShowWindow(SW_SHOW);
      m_wndListCtrl.ShowWindow(SW_HIDE);
      m_wndWaitView.Start();
      m_wndListCtrl.DeleteAllItems();
      _beginthread(LoadTraps, 0, m_hWnd);
   }
}


//
// NXCM_REQUEST_COMPLETED message handler
//

void CTrapLogBrowser::OnRequestCompleted(void)
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

LRESULT CTrapLogBrowser::OnGetSaveInfo(WPARAM wParam, WINDOW_SAVE_INFO *pInfo)
{
   pInfo->iWndClass = WNDC_TRAP_LOG_BROWSER;
   GetWindowPlacement(&pInfo->placement);
   pInfo->szParameters[0] = 0;
   return 1;
}


//
// NXCM_TRAP_LOG_RECORD message handler
//

void CTrapLogBrowser::OnTrapLogRecord(WPARAM wParam, NXC_SNMP_TRAP_LOG_RECORD *pRec)
{
   AddRecord(pRec, wParam == RECORD_ORDER_NORMAL);
   safe_free(pRec->pszTrapVarbinds);
   free(pRec);
}


//
// Add new record to list
//

void CTrapLogBrowser::AddRecord(NXC_SNMP_TRAP_LOG_RECORD *pRec, BOOL bAppend)
{
   int iIdx;
   time_t t;
   struct tm *ptm;
   char szBuffer[64];
   NXC_OBJECT *pNode;

   t = pRec->dwTimeStamp;
   ptm = localtime(&t);
   _tcsftime(szBuffer, 32, _T("%d-%b-%Y %H:%M:%S"), ptm);
   iIdx = m_wndListCtrl.InsertItem(bAppend ? 0x7FFFFFFF : 0, szBuffer);
   if (iIdx != -1)
   {
      m_wndListCtrl.SetItemText(iIdx, 1, IpToStr(pRec->dwIpAddr, szBuffer));
      pNode = NXCFindObjectById(g_hSession, pRec->dwObjectId);
      m_wndListCtrl.SetItemText(iIdx, 2, (pNode != NULL) ? pNode->szName : _T(""));
      m_wndListCtrl.SetItemText(iIdx, 3, pRec->szTrapOID);
      m_wndListCtrl.SetItemText(iIdx, 4, pRec->pszTrapVarbinds);

      if (bAppend)
         m_wndListCtrl.EnsureVisible(iIdx, FALSE);
   }
}
