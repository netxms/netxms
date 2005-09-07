// TableView.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "TableView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTableView

IMPLEMENT_DYNCREATE(CTableView, CMDIChildWnd)

CTableView::CTableView()
{
   m_dwNodeId = 0;
   m_dwToolId = 0;
   m_bIsBusy = FALSE;
}

CTableView::CTableView(DWORD dwNodeId, DWORD dwToolId)
{
   m_dwNodeId = dwNodeId;
   m_dwToolId = dwToolId;
   m_bIsBusy = FALSE;
}

CTableView::~CTableView()
{
}


BEGIN_MESSAGE_MAP(CTableView, CMDIChildWnd)
	//{{AFX_MSG_MAP(CTableView)
	ON_WM_CREATE()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	//}}AFX_MSG_MAP
   ON_MESSAGE(WM_TABLE_DATA, OnTableData)
END_MESSAGE_MAP()


//
// Data requesting thread starter
//

static void RequestDataStarter(void *pArg)
{
   ((CTableView *)pArg)->RequestData();
}


//
// Data requesting thread
//

void CTableView::RequestData()
{
   DWORD dwResult;
   NXC_TABLE_DATA *pData;
   HWND hWnd = m_hWnd;

   dwResult = NXCExecuteTableTool(g_hSession, m_dwNodeId, m_dwToolId, &pData);
   theApp.DebugPrintf(_T("CTableView::RequestData(): Execution result: %d (%s)"),
                      dwResult, NXCGetErrorText(dwResult));
   if (!::PostMessage(hWnd, WM_TABLE_DATA, dwResult, (LPARAM)pData))
   {
      theApp.DebugPrintf(_T("CTableView::RequestData(): Failed to post message to window %08X"), hWnd);
      if (dwResult == RCC_SUCCESS)
         NXCDestroyTableData(pData);
   }
}


/////////////////////////////////////////////////////////////////////////////
// CTableView message handlers


//
// WM_CREATE message handler
//

int CTableView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;
   int nTemp = -1;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   // Create status bar
   GetClientRect(&rect);
	m_wndStatusBar.Create(WS_CHILD | WS_VISIBLE, rect, this, IDC_STATUS_BAR);
   m_wndStatusBar.SetParts(1, &nTemp);
   m_iStatusBarHeight = GetWindowSize(&m_wndStatusBar).cy;

   // Create list view control
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL, rect, this, IDC_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

   // Create wait view
   m_wndWaitView.Create(NULL, NULL, WS_CHILD, rect, this, ID_WAIT_VIEW);
	
   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
	return 0;
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CTableView::OnViewRefresh() 
{
   if (m_bIsBusy)
      return;     // Already requesting data

   m_wndWaitView.ShowWindow(SW_SHOW);
   m_wndListCtrl.ShowWindow(SW_HIDE);
   m_wndWaitView.Start();
   m_wndListCtrl.DeleteAllItems();
   while(m_wndListCtrl.DeleteColumn(0));

   m_wndStatusBar.SetText(_T("Requesting data..."), 0, 0);
   m_bIsBusy = TRUE;

   _beginthread(RequestDataStarter, 0, this);
}


//
// WM_SIZE message handler
//

void CTableView::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);

   m_wndStatusBar.SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOZORDER);
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy - m_iStatusBarHeight, SWP_NOZORDER);
   m_wndWaitView.SetWindowPos(NULL, 0, 0, cx, cy - m_iStatusBarHeight, SWP_NOZORDER);
}


//
// WM_SETFOCUS message handler
//

void CTableView::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
   if (m_bIsBusy)
      m_wndWaitView.SetFocus();
   else
      m_wndListCtrl.SetFocus();
}


//
// WM_TABLE_DATA message handler
//

void CTableView::OnTableData(WPARAM wParam, NXC_TABLE_DATA *pData)
{
   DWORD i, j;
   int iItem, nPos;
   TCHAR szBuffer[256];
   NXC_OBJECT *pNode;

   if (wParam == RCC_SUCCESS)
   {
      // Create columns
      for(i = 0; i < pData->dwNumCols; i++)
         m_wndListCtrl.InsertColumn(i, pData->ppszColNames[i], LVCFMT_LEFT, 100);

      // Fill list control with data
      for(i = 0, nPos = 0; i < pData->dwNumRows; i++)
      {
         iItem = m_wndListCtrl.InsertItem(i, pData->ppszData[nPos++]);
         for(j = 1; j < pData->dwNumCols; j++)
            m_wndListCtrl.SetItemText(iItem, j, pData->ppszData[nPos++]);
         m_wndListCtrl.SetItemData(iItem, i);
      }

      pNode = NXCFindObjectById(g_hSession, m_dwNodeId);
      _sntprintf(szBuffer, 256, _T("%s - [%s]"), pData->pszTitle, 
                 (pNode != NULL) ? pNode->szName : _T("<unknown>"));
      SetTitle(szBuffer); 
      OnUpdateFrameTitle(TRUE);

      NXCDestroyTableData(pData);
      m_wndStatusBar.SetText(_T("Ready"), 0, 0);
   }
   else
   {
      theApp.ErrorBox(wParam, _T("Error getting data from node: %s"));
      m_wndStatusBar.SetText(_T("Error"), 0, 0);
   }
   m_wndListCtrl.ShowWindow(SW_SHOW);
   m_wndWaitView.ShowWindow(SW_HIDE);
   m_wndWaitView.Stop();
   m_bIsBusy = FALSE;
}
