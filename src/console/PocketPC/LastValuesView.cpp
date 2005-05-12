// LastValuesView.cpp : implementation file
//

#include "stdafx.h"
#include "nxpc.h"
#include "LastValuesView.h"
#include "DataView.h"
#include "GraphView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLastValuesView

CLastValuesView::CLastValuesView()
{
   m_dwNodeId = 0;
}

CLastValuesView::~CLastValuesView()
{
}


BEGIN_MESSAGE_MAP(CLastValuesView, CDynamicView)
	//{{AFX_MSG_MAP(CLastValuesView)
	ON_WM_CREATE()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_WM_SIZE()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_DCI_SHOWHISTORY, OnDciShowhistory)
	ON_COMMAND(ID_DCI_GRAPH, OnDciGraph)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//
// Get view's fingerprint
// Should be overriden in child classes.
//

QWORD CLastValuesView::GetFingerprint()
{
   return CREATE_VIEW_FINGERPRINT(VIEW_CLASS_LAST_VALUES, 0, m_dwNodeId);
}


//
// Initialize view. Called after view object creation, but
// before WM_CREATE. Data is a pointer to object.
//

void CLastValuesView::InitView(void *pData)
{
   m_dwNodeId = ((NXC_OBJECT *)pData)->dwId;
}


/////////////////////////////////////////////////////////////////////////////
// CLastValuesView message handlers


//
// WM_CREATE message handler
//

int CLastValuesView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CDynamicView::OnCreate(lpCreateStruct) == -1)
		return -1;

   // Create list control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT, rect, this, ID_LIST_CTRL);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);
   m_wndListCtrl.InsertColumn(0, L"ID", LVCFMT_LEFT, 30);
   m_wndListCtrl.InsertColumn(1, L"Description", LVCFMT_LEFT, 130);
   m_wndListCtrl.InsertColumn(2, L"Value", LVCFMT_LEFT, 70);
   m_wndListCtrl.InsertColumn(3, L"Timestamp", LVCFMT_LEFT, 124);
	
   PostMessage(WM_COMMAND, ID_VIEW_REFRESH);
	
	return 0;
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CLastValuesView::OnViewRefresh() 
{
   DWORD i, dwResult, dwNumItems;
   NXC_DCI_VALUE *pItemList;
   int iItem;
   TCHAR szBuffer[256];

   m_wndListCtrl.DeleteAllItems();
   dwResult = DoRequestArg4(NXCGetLastValues, g_hSession, (void *)m_dwNodeId,
                            &dwNumItems, &pItemList, _T("Loading last DCI values..."));
   if (dwResult == RCC_SUCCESS)
   {
      for(i = 0; i < dwNumItems; i++)
      {
         _sntprintf(szBuffer, 256, _T("%ld"), pItemList[i].dwId);
         iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, szBuffer);
         if (iItem != -1)
         {
            m_wndListCtrl.SetItemData(iItem, pItemList[i].dwId);
            m_wndListCtrl.SetItemText(iItem, 1, pItemList[i].szDescription);
            m_wndListCtrl.SetItemText(iItem, 2, pItemList[i].szValue);
            m_wndListCtrl.SetItemText(iItem, 3, FormatTimeStamp(pItemList[i].dwTimestamp, szBuffer, TS_LONG_DATE_TIME));
         }
      }
   }
   else
   {
      theApp.ErrorBox(dwResult, _T("Error loading last DCI values: %s"));
   }
}


//
// WM_SIZE message handler
//

void CLastValuesView::OnSize(UINT nType, int cx, int cy) 
{
	CDynamicView::OnSize(nType, cx, cy);
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_CONTEXTMENU message handler
//

void CLastValuesView::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   CMenu *pMenu;
   int iNumItems, iItem;
   UINT uFlags;
   CPoint pt;

   pt = point;
   pWnd->ScreenToClient(&pt);
   iItem = m_wndListCtrl.HitTest(pt, &uFlags);
   if ((iItem != -1) && (uFlags & LVHT_ONITEM))
   {
      if (m_wndListCtrl.GetItemState(iItem, LVIS_SELECTED) != LVIS_SELECTED)
         SelectListViewItem(&m_wndListCtrl, iItem);
   }

   pMenu = theApp.GetContextMenu(2);

   // Disable some menu items
   iNumItems = m_wndListCtrl.GetSelectedCount();
   pMenu->EnableMenuItem(ID_DCI_SHOWHISTORY, MF_BYCOMMAND | (iNumItems == 1) ? MF_ENABLED : MF_GRAYED);

   pMenu->TrackPopupMenu(TPM_LEFTALIGN, point.x, point.y, this, NULL);
}


//
// WM_COMMAND::ID_DCI_SHOWHISTORY
//

void CLastValuesView::OnDciShowhistory() 
{
   CDataView *pwndView;
   TCHAR szTitle[300];
   int iItem;
   DATA_VIEW_INIT init;
   NXC_OBJECT *pObject;

   iItem = m_wndListCtrl.GetSelectionMark();
   if (iItem != -1)
   {
      init.dwNodeId = m_dwNodeId;
      init.dwItemId = m_wndListCtrl.GetItemData(iItem);
      pObject = NXCFindObjectById(g_hSession, m_dwNodeId);
      _stprintf(szTitle, _T("%s - "), pObject->szName); 
      m_wndListCtrl.GetItemText(iItem, 1, &szTitle[_tcslen(szTitle)],
                                256 - _tcslen(szTitle));
      _tcscat(szTitle, _T(" - Collected Data"));
      pwndView = new CDataView;
      pwndView->InitView(&init);
      ((CMainFrame *)theApp.m_pMainWnd)->CreateView(pwndView, szTitle);
   }
}


//
// WM_COMMAND::ID_DCI_GRAPH
//

void CLastValuesView::OnDciGraph() 
{
   CGraphView *pwndView;
   TCHAR szTitle[300];
   int iItem;
   GRAPH_VIEW_INIT init;
   NXC_OBJECT *pObject;
   DWORD i;

   init.dwNodeId = m_dwNodeId;
   init.dwNumItems = m_wndListCtrl.GetSelectedCount();
   if (init.dwNumItems > 0)
   {
      iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
      for(i = 0; (iItem != -1) && (i < init.dwNumItems); i++)
      {
         init.pdwItemList[i] = m_wndListCtrl.GetItemData(iItem);
         iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
      }

      pObject = NXCFindObjectById(g_hSession, m_dwNodeId);
      _tcscpy(szTitle, pObject->szName); 
      if (init.dwNumItems == 1)
      {
         iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
         _tcscat(szTitle, _T(" - ["));
         m_wndListCtrl.GetItemText(iItem, 1, &szTitle[_tcslen(szTitle)],
                                   256 - _tcslen(szTitle));
         _tcscat(szTitle, _T("]"));
      }
      _tcscat(szTitle, _T(" - History Graph"));

      pwndView = new CGraphView;
      pwndView->InitView(&init);
      ((CMainFrame *)theApp.m_pMainWnd)->CreateView(pwndView, szTitle);
   }
}
