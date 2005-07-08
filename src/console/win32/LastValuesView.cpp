// LastValuesView.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "LastValuesView.h"
#include "LastValuesPropDlg.h"
#include "DataExportDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLastValuesView

IMPLEMENT_DYNCREATE(CLastValuesView, CMDIChildWnd)

CLastValuesView::CLastValuesView()
{
   m_dwNodeId = 0;
   m_dwNumItems = 0;
   m_pItemList = NULL;
   m_dwSeconds = 30;
   m_dwFlags = LVF_SHOW_GRID;
   m_nTimer = 0;
}

CLastValuesView::CLastValuesView(DWORD dwNodeId)
{
   m_dwNodeId = dwNodeId;
   m_dwNumItems = 0;
   m_pItemList = NULL;
   m_dwSeconds = 30;
   m_dwFlags = LVF_SHOW_GRID;
   m_nTimer = 0;
}

CLastValuesView::CLastValuesView(TCHAR *pszParams)
{
   m_dwNodeId = ExtractWindowParamULong(pszParams, _T("N"), 0);
   m_dwSeconds = ExtractWindowParamULong(pszParams, _T("S"), 30);
   m_dwFlags = ExtractWindowParamULong(pszParams, _T("F"), LVF_SHOW_GRID);
   m_dwNumItems = 0;
   m_pItemList = NULL;
   m_nTimer = 0;
}

CLastValuesView::~CLastValuesView()
{
   safe_free(m_pItemList);
}


BEGIN_MESSAGE_MAP(CLastValuesView, CMDIChildWnd)
	//{{AFX_MSG_MAP(CLastValuesView)
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_WM_SIZE()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_COMMAND(ID_ITEM_GRAPH, OnItemGraph)
	ON_COMMAND(ID_ITEM_SHOWDATA, OnItemShowdata)
	ON_UPDATE_COMMAND_UI(ID_ITEM_GRAPH, OnUpdateItemGraph)
	ON_UPDATE_COMMAND_UI(ID_ITEM_SHOWDATA, OnUpdateItemShowdata)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_LASTVALUES_PROPERTIES, OnLastvaluesProperties)
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_UPDATE_COMMAND_UI(ID_ITEM_EXPORTDATA, OnUpdateItemExportdata)
	ON_COMMAND(ID_ITEM_EXPORTDATA, OnItemExportdata)
	//}}AFX_MSG_MAP
   ON_MESSAGE(WM_GET_SAVE_INFO, OnGetSaveInfo)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLastValuesView message handlers

BOOL CLastValuesView::PreCreateWindow(CREATESTRUCT& cs) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CLastValuesView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   // Create image list
   m_imageList.Create(16, 16, ILC_COLOR24 | ILC_MASK, 4, 1);
   m_imageList.Add(theApp.LoadIcon(IDI_ACTIVE));
   m_imageList.Add(theApp.LoadIcon(IDI_DISABLED));
   m_imageList.Add(theApp.LoadIcon(IDI_UNSUPPORTED));
   m_imageList.Add(theApp.LoadIcon(IDI_TIPS));
   m_imageList.Add(theApp.LoadIcon(IDI_SORT_UP));
   m_imageList.Add(theApp.LoadIcon(IDI_SORT_DOWN));

   // Create list view control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_OWNERDRAWFIXED,
                        rect, this, IDC_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT | 
                                  LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES |
                                  LVS_EX_LABELTIP | 
                                  ((m_dwFlags & LVF_SHOW_GRID) ? LVS_EX_GRIDLINES : 0));
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);
   m_wndListCtrl.SetImageList(&m_imageList, LVSIL_SMALL);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, "ID", LVCFMT_LEFT, 40);
   m_wndListCtrl.InsertColumn(1, "Description", LVCFMT_LEFT, 250);
   m_wndListCtrl.InsertColumn(2, "Value", LVCFMT_LEFT | LVCFMT_BITMAP_ON_RIGHT, 100);
   m_wndListCtrl.InsertColumn(3, "Changed", LVCFMT_LEFT, 26);
   m_wndListCtrl.InsertColumn(4, "Timestamp", LVCFMT_LEFT, 124);

   if (m_dwNodeId != 0)
      PostMessage(WM_COMMAND, ID_VIEW_REFRESH);

   if (m_dwFlags & LVF_AUTOREFRESH)
      SetTimer(1, m_dwSeconds * 1000, NULL);

	return 0;
}


//
// WM_SETFOCUS message handler
//

void CLastValuesView::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
   m_wndListCtrl.SetFocus();
}


//
// WM_SIZE messahe handler
//

void CLastValuesView::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CLastValuesView::OnViewRefresh() 
{
   DWORD i, dwResult;
   LVFINDINFO lvfi;
   int iItem;
   TCHAR szBuffer[256];

   safe_free(m_pItemList);
   m_pItemList = NULL;
   m_dwNumItems = 0;
   dwResult = DoRequestArg4(NXCGetLastValues, g_hSession, (void *)m_dwNodeId,
                            &m_dwNumItems, &m_pItemList, _T("Loading last DCI values..."));
   if (dwResult == RCC_SUCCESS)
   {
      // Add new items to list or update existing
      lvfi.flags = LVFI_PARAM;
      for(i = 0; i < m_dwNumItems; i++)
      {
         lvfi.lParam = m_pItemList[i].dwId;
         iItem = m_wndListCtrl.FindItem(&lvfi, -1);
         if (iItem == -1)
         {
            _sntprintf(szBuffer, 256, _T("%ld"), m_pItemList[i].dwId);
            iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, szBuffer);
            m_wndListCtrl.SetItemData(iItem, m_pItemList[i].dwId);
         }
         UpdateItem(iItem, &m_pItemList[i]);
      }
   }
   else
   {
      theApp.ErrorBox(dwResult, _T("Error loading last DCI values: %s"));
   }
}


//
// Update list box item with new data
//

void CLastValuesView::UpdateItem(int iItem, NXC_DCI_VALUE *pValue)
{
   TCHAR szOldValue[1024], szOldTimeStamp[64], szTimeStamp[64];
   LVITEM item;
   struct tm *ptm;

   // Create timestamp
   ptm = localtime((time_t *)&pValue->dwTimestamp);
   _tcsftime(szTimeStamp, 64, _T("%d-%b-%Y %H:%M:%S"), ptm);

   // Set or clear modification flag
   m_wndListCtrl.GetItemText(iItem, 2, szOldValue, 1024);
   m_wndListCtrl.GetItemText(iItem, 4, szOldTimeStamp, 64);
   item.mask = LVIF_IMAGE;
   item.iItem = iItem;
   item.iSubItem = 0;
   m_wndListCtrl.GetItem(&item);
   if (_tcscmp(pValue->szValue, szOldValue) || 
       ((item.iImage == 1) && (!_tcscmp(szTimeStamp, szOldTimeStamp))))
   {
      item.iImage = 1;  // Should be shown in red
      m_wndListCtrl.SetItemText(iItem, 3, "X");
   }
   else
   {
      item.iImage = -1;
      m_wndListCtrl.SetItemText(iItem, 3, " ");
   }
   m_wndListCtrl.SetItem(&item);

   m_wndListCtrl.SetItemText(iItem, 1, pValue->szDescription);
   m_wndListCtrl.SetItemText(iItem, 2, pValue->szValue);
   m_wndListCtrl.SetItemText(iItem, 4, szTimeStamp);
}


//
// Get save info for desktop saving
//

LRESULT CLastValuesView::OnGetSaveInfo(WPARAM wParam, WINDOW_SAVE_INFO *pInfo)
{
   pInfo->iWndClass = WNDC_LAST_VALUES;
   GetWindowPlacement(&pInfo->placement);
   _sntprintf(pInfo->szParameters, MAX_DB_STRING, _T("F:%ld\x7FN:%ld\x7FS:%ld"),
              m_dwFlags, m_dwNodeId, m_dwSeconds);
   return 1;
}


//
// WM_COMMAND::ID_ITEM_GRAPH message handler
//

void CLastValuesView::OnItemGraph() 
{
   int iItem;
   DWORD i, dwNumItems, dwIndex;
   NXC_DCI **ppItemList;
   TCHAR szBuffer[384];
   NXC_OBJECT *pObject;

   pObject = NXCFindObjectById(g_hSession, m_dwNodeId);
   dwNumItems = m_wndListCtrl.GetSelectedCount();
   ppItemList = (NXC_DCI **)malloc(sizeof(NXC_DCI *) * dwNumItems);

   iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
   for(i = 0; (iItem != -1) && (i < dwNumItems); i++)
   {
      ppItemList[i] = (NXC_DCI *)malloc(sizeof(NXC_DCI));
      memset(ppItemList[i], 0, sizeof(NXC_DCI));
      ppItemList[i]->dwId = m_wndListCtrl.GetItemData(iItem);
      dwIndex = FindItem(ppItemList[i]->dwId);
      if (dwIndex != INVALID_INDEX)
      {
         _tcscpy(ppItemList[i]->szName, m_pItemList[dwIndex].szName);
         _tcscpy(ppItemList[i]->szDescription, m_pItemList[dwIndex].szDescription);
         ppItemList[i]->iDataType = m_pItemList[dwIndex].iDataType;
         ppItemList[i]->iSource = m_pItemList[dwIndex].iSource;
      }
      iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
   }

   if (dwNumItems == 1)
   {
      sprintf(szBuffer, "%s - %s", pObject->szName,
              ppItemList[0]->szDescription);
   }
   else
   {
      strcpy(szBuffer, pObject->szName);
   }

   theApp.ShowDCIGraph(m_dwNodeId, dwNumItems, ppItemList, szBuffer);
   for(i = 0; i < dwNumItems; i++)
      free(ppItemList[i]);
   free(ppItemList);
}


//
// WM_COMMAND::ID_ITEM_SHOWDATA message handler
//

void CLastValuesView::OnItemShowdata() 
{
   int iItem;
   DWORD dwItemId;
   TCHAR szBuffer[384];
   NXC_OBJECT *pObject;

   iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
   while(iItem != -1)
   {
      dwItemId = m_wndListCtrl.GetItemData(iItem);
      pObject = NXCFindObjectById(g_hSession, m_dwNodeId);
      _stprintf(szBuffer, "%s - ", pObject->szName); 
      m_wndListCtrl.GetItemText(iItem, 1, &szBuffer[_tcslen(szBuffer)],
                                384 - _tcslen(szBuffer));
      theApp.ShowDCIData(m_dwNodeId, dwItemId, szBuffer);
      iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
   }
}


//
// UI update handlers
//

void CLastValuesView::OnUpdateItemGraph(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() > 0);
}

void CLastValuesView::OnUpdateItemShowdata(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() > 0);
}

void CLastValuesView::OnUpdateItemExportdata(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() == 1);
}


//
// WM_CONTEXTMENU message handler
//

void CLastValuesView::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   CMenu *pMenu;

   pMenu = theApp.GetContextMenu(14);
   pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this, NULL);
}


//
// Find item in list by id
//

DWORD CLastValuesView::FindItem(DWORD dwId)
{
   DWORD i;

   for(i = 0; i < m_dwNumItems; i++)
      if (m_pItemList[i].dwId == dwId)
         return i;
   return INVALID_INDEX;
}


//
// Change properties
//

void CLastValuesView::OnLastvaluesProperties() 
{
   CLastValuesPropDlg dlg;

   dlg.m_dwSeconds = m_dwSeconds;
   dlg.m_bShowGrid = (m_dwFlags & LVF_SHOW_GRID) ? TRUE : FALSE;
   dlg.m_bRefresh = (m_dwFlags & LVF_AUTOREFRESH) ? TRUE : FALSE;
   if (dlg.DoModal() == IDOK)
   {
      if (m_nTimer != 0)
      {
         KillTimer(m_nTimer);
         m_nTimer = 0;
      }

      m_dwSeconds = dlg.m_dwSeconds;
      m_dwFlags = 0;
      if (dlg.m_bRefresh)
      {
         m_dwFlags |= LVF_AUTOREFRESH;
         m_nTimer = SetTimer(1, m_dwSeconds * 1000, NULL);
      }
      if (dlg.m_bShowGrid)
      {
         m_dwFlags |= LVF_SHOW_GRID;
         m_wndListCtrl.SetExtendedStyle(m_wndListCtrl.GetExtendedStyle() | LVS_EX_GRIDLINES);
      }
      else
      {
         m_wndListCtrl.SetExtendedStyle(m_wndListCtrl.GetExtendedStyle() & (~LVS_EX_GRIDLINES));
      }
   }
}


//
// WM_TIMER message handler
//

void CLastValuesView::OnTimer(UINT nIDEvent) 
{
   PostMessage(WM_COMMAND, ID_VIEW_REFRESH);
}


//
// WM_DESTROY message handler
//

void CLastValuesView::OnDestroy() 
{
   if (m_nTimer != 0)
      KillTimer(m_nTimer);

	CMDIChildWnd::OnDestroy();
}

void CLastValuesView::OnItemExportdata() 
{
   CDataExportDlg dlg;
   DWORD dwItemId, dwTimeFrom, dwTimeTo;

   dwItemId = m_wndListCtrl.GetItemData(m_wndListCtrl.GetSelectionMark());
   if (dlg.DoModal() == IDOK)
   {
      dlg.SaveLastSelection();
      dwTimeFrom = CTime(dlg.m_dateFrom.GetYear(), dlg.m_dateFrom.GetMonth(),
                         dlg.m_dateFrom.GetDay(), dlg.m_timeFrom.GetHour(),
                         dlg.m_timeFrom.GetMinute(),
                         dlg.m_timeFrom.GetSecond(), -1).GetTime();
      dwTimeTo = CTime(dlg.m_dateTo.GetYear(), dlg.m_dateTo.GetMonth(),
                       dlg.m_dateTo.GetDay(), dlg.m_timeTo.GetHour(),
                       dlg.m_timeTo.GetMinute(),
                       dlg.m_timeTo.GetSecond(), -1).GetTime();
      theApp.ExportDCIData(m_dwNodeId, dwItemId, dwTimeFrom, dwTimeTo,
                           dlg.m_iSeparator, dlg.m_iTimeStampFormat,
                           (LPCTSTR)dlg.m_strFileName);
   }
}
