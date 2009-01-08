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


//
// Compare two list view items
//

static int CALLBACK CompareListItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
   return ((CTableView *)lParamSort)->CompareItems(lParam1, lParam2);
}


/////////////////////////////////////////////////////////////////////////////
// CTableView

IMPLEMENT_DYNCREATE(CTableView, CMDIChildWnd)

CTableView::CTableView()
{
   m_dwNodeId = 0;
   m_dwToolId = 0;
   m_bIsBusy = FALSE;
   m_iSortDir = 0;
   m_iSortMode = 0;
   m_pData = NULL;
}

CTableView::CTableView(DWORD dwNodeId, DWORD dwToolId)
{
   m_dwNodeId = dwNodeId;
   m_dwToolId = dwToolId;
   m_bIsBusy = FALSE;
   m_iSortDir = 0;
   m_iSortMode = 0;
   m_pData = NULL;
}

CTableView::~CTableView()
{
   if (m_pData != NULL)
      NXCDestroyTableData(m_pData);
}


BEGIN_MESSAGE_MAP(CTableView, CMDIChildWnd)
	//{{AFX_MSG_MAP(CTableView)
	ON_WM_CREATE()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
   ON_MESSAGE(NXCM_TABLE_DATA, OnTableData)
	ON_NOTIFY(LVN_COLUMNCLICK, ID_LIST_VIEW, OnListViewColumnClick)
END_MESSAGE_MAP()


//
// Data requesting thread starter
//

static THREAD_RESULT THREAD_CALL RequestDataStarter(void *pArg)
{
   ((CTableView *)pArg)->RequestData();
	return THREAD_OK;
}


//
// Data requesting thread
//

void CTableView::RequestData()
{
   DWORD dwResult;
   HWND hWnd = m_hWnd;

   if (m_pData != NULL)
   {
      NXCDestroyTableData(m_pData);
      m_pData = NULL;
   }
   dwResult = NXCExecuteTableTool(g_hSession, m_dwNodeId, m_dwToolId, &m_pData);
   theApp.DebugPrintf(_T("CTableView::RequestData(): Execution result: %d (%s)"),
                      dwResult, NXCGetErrorText(dwResult));
   if (!::PostMessage(hWnd, NXCM_TABLE_DATA, dwResult, 0))
   {
      theApp.DebugPrintf(_T("CTableView::RequestData(): Failed to post message to window %08X"), hWnd);
//      if (dwResult == RCC_SUCCESS)
//         NXCDestroyTableData(pData);
   }
}


/////////////////////////////////////////////////////////////////////////////
// CTableView message handlers

BOOL CTableView::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         NULL, NULL,
                                         AfxGetApp()->LoadIcon(IDI_DOCUMENT));
	return CMDIChildWnd::PreCreateWindow(cs);
}


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
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL | LVS_SHAREIMAGELISTS, rect, this, ID_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

   // Create image list
   m_imageList.Create(16, 16, ILC_COLOR8 | ILC_MASK, 8, 8);
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_SORT_UP));
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_SORT_DOWN));
   m_wndListCtrl.GetHeaderCtrl()->SetImageList(&m_imageList);

   // Create wait view
   m_wndWaitView.SetText(_T("Receiving data from server..."));
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

   if (m_pData != NULL)
   {
      // Data was already shown once, save column info
      SaveListCtrlColumns(m_wndListCtrl, _T("TableView"), m_pData->pszTitle);
   }

   m_wndWaitView.ShowWindow(SW_SHOW);
   m_wndListCtrl.ShowWindow(SW_HIDE);
   m_wndWaitView.Start();
   m_wndListCtrl.DeleteAllItems();
   while(m_wndListCtrl.DeleteColumn(0));

   m_wndStatusBar.SetText(_T("Requesting data..."), 0, 0);
   m_bIsBusy = TRUE;

   ThreadCreate(RequestDataStarter, 0, this);
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

LRESULT CTableView::OnTableData(WPARAM wParam, LPARAM lParam)
{
   DWORD i, j;
   int iItem, nPos;
   TCHAR szBuffer[256];
   NXC_OBJECT *pNode;
   LVCOLUMN lvCol;

   if (wParam == RCC_SUCCESS)
   {
      // Create columns
      for(i = 0; i < m_pData->dwNumCols; i++)
         m_wndListCtrl.InsertColumn(i + 1, m_pData->ppszColNames[i], LVCFMT_LEFT, 100);
      LoadListCtrlColumns(m_wndListCtrl, _T("TableView"), m_pData->pszTitle);

      // Fill list control with data
      for(i = 0, nPos = 0; i < m_pData->dwNumRows; i++)
      {
         iItem = m_wndListCtrl.InsertItem(i, m_pData->ppszData[nPos++], -1);
         for(j = 1; j < m_pData->dwNumCols; j++)
            m_wndListCtrl.SetItemText(iItem, j, m_pData->ppszData[nPos++]);
         m_wndListCtrl.SetItemData(iItem, i);
      }

      m_wndListCtrl.SortItems(CompareListItems, (LPARAM)this);

      pNode = NXCFindObjectById(g_hSession, m_dwNodeId);
      _sntprintf(szBuffer, 256, _T("%s - [%s]"), m_pData->pszTitle, 
                 (pNode != NULL) ? pNode->szName : _T("<unknown>"));
      SetTitle(szBuffer); 
      OnUpdateFrameTitle(TRUE);

      // Mark sorting column in list control
      lvCol.mask = LVCF_IMAGE | LVCF_FMT;
      lvCol.fmt = LVCFMT_BITMAP_ON_RIGHT | LVCFMT_IMAGE | LVCFMT_LEFT;
      lvCol.iImage = m_iSortDir;
      m_wndListCtrl.SetColumn(m_iSortMode, &lvCol);

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

	return 0;
}


//
// WM_NOTIFY::LVN_COLUMNCLICK message handler
//

void CTableView::OnListViewColumnClick(NMHDR *pNMHDR, LRESULT *pResult)
{
   LVCOLUMN lvCol;

   // Unmark old sorting column
   lvCol.mask = LVCF_FMT;
   lvCol.fmt = LVCFMT_LEFT;
   m_wndListCtrl.SetColumn(m_iSortMode, &lvCol);

   // Change current sort mode and resort list
   if (m_iSortMode == ((LPNMLISTVIEW)pNMHDR)->iSubItem)
   {
      // Same column, change sort direction
      m_iSortDir = 1 - m_iSortDir;
   }
   else
   {
      // Another sorting column
      m_iSortMode = ((LPNMLISTVIEW)pNMHDR)->iSubItem;
   }
   m_wndListCtrl.SortItems(CompareListItems, (LPARAM)this);

   // Mark new sorting column
   lvCol.mask = LVCF_IMAGE | LVCF_FMT;
   lvCol.fmt = LVCFMT_BITMAP_ON_RIGHT | LVCFMT_IMAGE | LVCFMT_LEFT;
   lvCol.iImage = m_iSortDir;
   m_wndListCtrl.SetColumn(((LPNMLISTVIEW)pNMHDR)->iSubItem, &lvCol);
   
   *pResult = 0;
}


//
// Compare two list items
//

int CTableView::CompareItems(int nItem1, int nItem2)
{
   LONG n1, n2;
   int nRet;
   TCHAR *pstr1, *pstr2, *eptr1, *eptr2;

   pstr1 = m_pData->ppszData[nItem1 * m_pData->dwNumCols + m_iSortMode];
   pstr2 = m_pData->ppszData[nItem2 * m_pData->dwNumCols + m_iSortMode];

   // First, try to compare as numbers
   n1 = _tcstol(pstr1, &eptr1, 10);
   n2 = _tcstol(pstr2, &eptr2, 10);
   if ((*eptr1 == 0) && (*eptr2 == 0))
   {
      nRet = COMPARE_NUMBERS(n1, n2);
   }
   else
   {
      nRet = _tcsicmp(pstr1, pstr2);
   }
   return m_iSortDir == 0 ? nRet : -nRet;
}


//
// WM_CLOSE message handler
//

void CTableView::OnClose() 
{
   if (m_pData != NULL)
   {
      // Data was shown, save column info
      SaveListCtrlColumns(m_wndListCtrl, _T("TableView"), m_pData->pszTitle);
   }
	
	CMDIChildWnd::OnClose();
}
