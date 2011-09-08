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


//
// Item comparision callback
//

static int CALLBACK CompareItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
   return ((CLastValuesView *)lParamSort)->CompareListItems(lParam1, lParam2);
}


/////////////////////////////////////////////////////////////////////////////
// CLastValuesView

IMPLEMENT_DYNCREATE(CLastValuesView, CMDIChildWnd)

CLastValuesView::CLastValuesView()
{
   m_dwNodeId = 0;
   m_dwNumItems = 0;
   m_pItemList = NULL;
   m_nTimer = 0;
   m_dwSeconds = theApp.GetProfileInt(_T("LastValuesView"), _T("Seconds"), 30);
   m_dwFlags = theApp.GetProfileInt(_T("LastValuesView"), _T("Flags"), LVF_SHOW_GRID);
   m_iSortMode = theApp.GetProfileInt(_T("LastValuesView"), _T("SortMode"), 1);
   m_iSortDir = theApp.GetProfileInt(_T("LastValuesView"), _T("SortDir"), 1);
}

CLastValuesView::CLastValuesView(DWORD dwNodeId)
{
   m_dwNodeId = dwNodeId;
   m_dwNumItems = 0;
   m_pItemList = NULL;
   m_nTimer = 0;
   m_dwSeconds = theApp.GetProfileInt(_T("LastValuesView"), _T("Seconds"), 30);
   m_dwFlags = theApp.GetProfileInt(_T("LastValuesView"), _T("Flags"), LVF_SHOW_GRID);
   m_iSortMode = theApp.GetProfileInt(_T("LastValuesView"), _T("SortMode"), 1);
   m_iSortDir = theApp.GetProfileInt(_T("LastValuesView"), _T("SortDir"), 1);
}

CLastValuesView::CLastValuesView(TCHAR *pszParams)
{
   m_dwNodeId = ExtractWindowParamULong(pszParams, _T("N"), 0);
   m_dwSeconds = ExtractWindowParamULong(pszParams, _T("S"), 30);
   m_dwFlags = ExtractWindowParamULong(pszParams, _T("F"), LVF_SHOW_GRID);
   m_iSortDir = ExtractWindowParamLong(pszParams, _T("SD"), 1);
   m_iSortMode = ExtractWindowParamLong(pszParams, _T("SM"), 1);
   m_dwNumItems = 0;
   m_pItemList = NULL;
   m_nTimer = 0;
}

CLastValuesView::~CLastValuesView()
{
   safe_free(m_pItemList);
   theApp.WriteProfileInt(_T("LastValuesView"), _T("Seconds"), m_dwSeconds);
   theApp.WriteProfileInt(_T("LastValuesView"), _T("Flags"), m_dwFlags);
   theApp.WriteProfileInt(_T("LastValuesView"), _T("SortMode"), m_iSortMode);
   theApp.WriteProfileInt(_T("LastValuesView"), _T("SortDir"), m_iSortDir);
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
	ON_COMMAND(ID_ITEM_CLEARDATA, OnItemCleardata)
	ON_UPDATE_COMMAND_UI(ID_ITEM_CLEARDATA, OnUpdateItemCleardata)
	//}}AFX_MSG_MAP
   ON_MESSAGE(NXCM_GET_SAVE_INFO, OnGetSaveInfo)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_VIEW, OnListViewColumnClick)
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
   LVCOLUMN lvCol;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   // Create image list
   m_imageList.Create(16, 16, ILC_COLOR24 | ILC_MASK, 4, 1);
   m_imageList.Add(theApp.LoadIcon(IDI_ACTIVE));
   m_imageList.Add(theApp.LoadIcon(IDI_DISABLED));
   m_imageList.Add(theApp.LoadIcon(IDI_UNSUPPORTED));
   m_imageList.Add(theApp.LoadIcon(IDI_TIPS));
   m_iSortImageBase = m_imageList.GetImageCount();
   m_imageList.Add(theApp.LoadIcon(IDI_SORT_UP));
   m_imageList.Add(theApp.LoadIcon(IDI_SORT_DOWN));

   // Create list view control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_OWNERDRAWFIXED | LVS_SHAREIMAGELISTS,
                        rect, this, IDC_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT | 
                                  LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES |
                                  LVS_EX_LABELTIP | 
                                  ((m_dwFlags & LVF_SHOW_GRID) ? LVS_EX_GRIDLINES : 0));
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);
   m_wndListCtrl.SetImageList(&m_imageList, LVSIL_SMALL);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, _T("ID"), LVCFMT_LEFT, 55);
   m_wndListCtrl.InsertColumn(1, _T("Description"), LVCFMT_LEFT, 250);
   m_wndListCtrl.InsertColumn(2, _T("Value"), LVCFMT_LEFT | LVCFMT_BITMAP_ON_RIGHT, 100);
   m_wndListCtrl.InsertColumn(3, _T("Changed"), LVCFMT_LEFT, 26);
   m_wndListCtrl.InsertColumn(4, _T("Timestamp"), LVCFMT_LEFT, 124);

   // Mark sorting column
   lvCol.mask = LVCF_IMAGE | LVCF_FMT;
   lvCol.fmt = LVCFMT_BITMAP_ON_RIGHT | LVCFMT_IMAGE | LVCFMT_LEFT;
   lvCol.iImage = (m_iSortDir > 0) ? m_iSortImageBase : (m_iSortImageBase + 1);
   m_wndListCtrl.SetColumn(m_iSortMode, &lvCol);

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
				if (!((m_dwFlags & LVF_HIDE_EMPTY) && (m_pItemList[i].dwTimestamp <= 1)))
				{
					_sntprintf(szBuffer, 256, _T("%d"), m_pItemList[i].dwId);
					iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, szBuffer);
					m_wndListCtrl.SetItemData(iItem, m_pItemList[i].dwId);
					UpdateItem(iItem, &m_pItemList[i]);
				}
         }
			else
			{
				if ((m_dwFlags & LVF_HIDE_EMPTY) && (m_pItemList[i].dwTimestamp <= 1))
					m_wndListCtrl.DeleteItem(iItem);
				else
					UpdateItem(iItem, &m_pItemList[i]);
			}
      }
      m_wndListCtrl.SortItems(CompareItems, (DWORD)this);
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
	time_t t = (time_t)pValue->dwTimestamp;
   ptm = localtime(&t);
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
      m_wndListCtrl.SetItemText(iItem, 3, _T("X"));
   }
   else
   {
      item.iImage = -1;
      m_wndListCtrl.SetItemText(iItem, 3, _T(" "));
   }
   m_wndListCtrl.SetItem(&item);

   m_wndListCtrl.SetItemText(iItem, 1, pValue->szDescription);
	if ((pValue->nDataType != DCI_DT_STRING) &&
	    (m_dwFlags & LVF_USE_MULTIPLIERS))
	{
		TCHAR szBuffer[64];
		QWORD ui64;
		INT64 i64;
		double d;

		switch(pValue->nDataType)
		{
			case DCI_DT_INT:
			case DCI_DT_INT64:
				i64 = _tcstoll(pValue->szValue, NULL, 10);
				if (i64 >= 10000000000)
				{
					_sntprintf_s(szBuffer, 64, _TRUNCATE, INT64_FMT _T(" G"), i64 / 1000000000);
				}
				else if (i64 >= 10000000)
				{
					_sntprintf_s(szBuffer, 64, _TRUNCATE, INT64_FMT _T(" M"), i64 / 1000000);
				}
				else if (i64 >= 10000)
				{
					_sntprintf_s(szBuffer, 64, _TRUNCATE, INT64_FMT _T(" K"), i64 / 1000);
				}
				else if (i64 >= 0)
				{
					_sntprintf_s(szBuffer, 64, _TRUNCATE, INT64_FMT, i64);
				}
				else if (i64 <= -10000000000)
				{
					_sntprintf_s(szBuffer, 64, _TRUNCATE, INT64_FMT _T(" G"), i64 / 1000000000);
				}
				else if (i64 <= -10000000)
				{
					_sntprintf_s(szBuffer, 64, _TRUNCATE, INT64_FMT _T(" M"), i64 / 1000000);
				}
				else if (i64 <= -10000)
				{
					_sntprintf_s(szBuffer, 64, _TRUNCATE, INT64_FMT _T(" K"), i64 / 1000);
				}
				else
				{
					_sntprintf_s(szBuffer, 64, _TRUNCATE, INT64_FMT, i64);
				}
				break;
			case DCI_DT_UINT:
			case DCI_DT_UINT64:
				ui64 = _tcstoull(pValue->szValue, NULL, 10);
				if (ui64 >= 10000000000)
				{
					_sntprintf_s(szBuffer, 64, _TRUNCATE, UINT64_FMT _T(" G"), ui64 / 1000000000);
				}
				else if (ui64 >= 10000000)
				{
					_sntprintf_s(szBuffer, 64, _TRUNCATE, UINT64_FMT _T(" M"), ui64 / 1000000);
				}
				else if (ui64 >= 10000)
				{
					_sntprintf_s(szBuffer, 64, _TRUNCATE, UINT64_FMT _T(" K"), ui64 / 1000);
				}
				else
				{
					_sntprintf_s(szBuffer, 64, _TRUNCATE, UINT64_FMT, ui64);
				}
				break;
			case DCI_DT_FLOAT:
				d = _tcstod(pValue->szValue, NULL);
				if (d >= 10000000000)
				{
					_sntprintf_s(szBuffer, 64, _TRUNCATE, _T("%.2f G"), d / 1000000000);
				}
				else if (d >= 10000000)
				{
					_sntprintf_s(szBuffer, 64, _TRUNCATE, _T("%.2f M"), d / 1000000);
				}
				else if (d >= 10000)
				{
					_sntprintf_s(szBuffer, 64, _TRUNCATE, _T("%.2f K"), d / 1000);
				}
				else if (d >= 0)
				{
					_sntprintf_s(szBuffer, 64, _TRUNCATE, _T("%f"), d);
				}
				else if (d <= -10000000000)
				{
					_sntprintf_s(szBuffer, 64, _TRUNCATE, _T("%.2f G"), d / 1000000000);
				}
				else if (d <= -10000000)
				{
					_sntprintf_s(szBuffer, 64, _TRUNCATE, _T("%.2f M"), d / 1000000);
				}
				else if (d <= -10000)
				{
					_sntprintf_s(szBuffer, 64, _TRUNCATE, _T("%.2f K"), d / 1000);
				}
				else
				{
					_sntprintf_s(szBuffer, 64, _TRUNCATE, _T("%f"), d);
				}
				break;
			default:
				nx_strncpy(szBuffer, pValue->szValue, 64);
				break;
		}
		m_wndListCtrl.SetItemText(iItem, 2, szBuffer);
	}
	else
	{
		m_wndListCtrl.SetItemText(iItem, 2, pValue->szValue);
	}
   m_wndListCtrl.SetItemText(iItem, 4, szTimeStamp);
}


//
// Get save info for desktop saving
//

LRESULT CLastValuesView::OnGetSaveInfo(WPARAM wParam, LPARAM lParam)
{
	WINDOW_SAVE_INFO *pInfo = CAST_TO_POINTER(lParam, WINDOW_SAVE_INFO *);
   pInfo->iWndClass = WNDC_LAST_VALUES;
   GetWindowPlacement(&pInfo->placement);
   _sntprintf(pInfo->szParameters, MAX_DB_STRING, _T("F:%d\x7FN:%d\x7FS:%d"),
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
      ppItemList[i]->dwId = (DWORD)m_wndListCtrl.GetItemData(iItem);
      dwIndex = FindItem(ppItemList[i]->dwId);
      if (dwIndex != INVALID_INDEX)
      {
         _tcscpy(ppItemList[i]->szName, m_pItemList[dwIndex].szName);
         _tcscpy(ppItemList[i]->szDescription, m_pItemList[dwIndex].szDescription);
         ppItemList[i]->iDataType = m_pItemList[dwIndex].nDataType;
         ppItemList[i]->iSource = m_pItemList[dwIndex].nSource;
      }
      iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
   }

   if (dwNumItems == 1)
   {
      _sntprintf(szBuffer, 384, _T("%s - %s"), pObject->szName,
                 ppItemList[0]->szDescription);
   }
   else
   {
      nx_strncpy(szBuffer, pObject->szName, 384);
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

   pObject = NXCFindObjectById(g_hSession, m_dwNodeId);
   iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
   while(iItem != -1)
   {
      dwItemId = (DWORD)m_wndListCtrl.GetItemData(iItem);
      _sntprintf_s(szBuffer, 384, _TRUNCATE, _T("%s - "), pObject->szName); 
      m_wndListCtrl.GetItemText(iItem, 1, &szBuffer[_tcslen(szBuffer)],
                                384 - (int)_tcslen(szBuffer));
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
   dlg.m_bHideEmpty = (m_dwFlags & LVF_HIDE_EMPTY) ? TRUE : FALSE;
   dlg.m_bUseMultipliers = (m_dwFlags & LVF_USE_MULTIPLIERS) ? TRUE : FALSE;
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
      if (dlg.m_bHideEmpty)
      {
         m_dwFlags |= LVF_HIDE_EMPTY;
      }
      if (dlg.m_bUseMultipliers)
      {
         m_dwFlags |= LVF_USE_MULTIPLIERS;
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
		PostMessage(WM_COMMAND, ID_VIEW_REFRESH);
   }
}


//
// WM_TIMER message handler
//

void CLastValuesView::OnTimer(UINT_PTR nIDEvent) 
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


//
// WM_COMMAND::ID_ITEM_EXPORTDATA message handler
//

void CLastValuesView::OnItemExportdata() 
{
   CDataExportDlg dlg;
   DWORD dwItemId, dwTimeFrom, dwTimeTo;

   dwItemId = (DWORD)m_wndListCtrl.GetItemData(m_wndListCtrl.GetSelectionMark());
   if (dlg.DoModal() == IDOK)
   {
      dlg.SaveLastSelection();
      dwTimeFrom = (DWORD)CTime(dlg.m_dateFrom.GetYear(), dlg.m_dateFrom.GetMonth(),
                         dlg.m_dateFrom.GetDay(), dlg.m_timeFrom.GetHour(),
                         dlg.m_timeFrom.GetMinute(),
                         dlg.m_timeFrom.GetSecond(), -1).GetTime();
      dwTimeTo = (DWORD)CTime(dlg.m_dateTo.GetYear(), dlg.m_dateTo.GetMonth(),
                       dlg.m_dateTo.GetDay(), dlg.m_timeTo.GetHour(),
                       dlg.m_timeTo.GetMinute(),
                       dlg.m_timeTo.GetSecond(), -1).GetTime();
      theApp.ExportDCIData(m_dwNodeId, dwItemId, dwTimeFrom, dwTimeTo,
                           dlg.m_iSeparator, dlg.m_iTimeStampFormat,
                           (LPCTSTR)dlg.m_strFileName);
   }
}


//
// Get DCI index by id
//

DWORD CLastValuesView::GetDCIIndex(DWORD dwId)
{
   DWORD i;

   for(i = 0; i < m_dwNumItems; i++)
      if (m_pItemList[i].dwId == dwId)
         return i;
   return INVALID_INDEX;
}


//
// Compare two list items for sorting
//

int CLastValuesView::CompareListItems(LPARAM lParam1, LPARAM lParam2)
{
   int iResult;
   DWORD dwIndex1, dwIndex2;
   TCHAR szText1[16], szText2[16];
   LVFINDINFO lvfi;

   dwIndex1 = GetDCIIndex((DWORD)lParam1);
   dwIndex2 = GetDCIIndex((DWORD)lParam2);
   if ((dwIndex1 == INVALID_INDEX) || (dwIndex2 == INVALID_INDEX))
      return 0;   // Sanity check

   switch(m_iSortMode)
   {
      case 0:     // Item ID
         iResult = COMPARE_NUMBERS(lParam1, lParam2);
         break;
      case 1:     // Item description
         iResult = _tcsicmp(m_pItemList[dwIndex1].szDescription, m_pItemList[dwIndex2].szDescription);
         break;
      case 2:     // Value
         iResult = _tcsicmp(m_pItemList[dwIndex1].szValue, m_pItemList[dwIndex2].szValue);
         break;
      case 3:     // Change flag
         lvfi.flags = LVFI_PARAM;
         lvfi.lParam = lParam1;
         m_wndListCtrl.GetItemText(m_wndListCtrl.FindItem(&lvfi), 3, szText1, 16);
         lvfi.lParam = lParam2;
         m_wndListCtrl.GetItemText(m_wndListCtrl.FindItem(&lvfi), 3, szText2, 16);
         iResult = _tcsicmp(szText1, szText2);
         break;
      case 4:     // Timestamp
         iResult = COMPARE_NUMBERS(m_pItemList[dwIndex1].dwTimestamp, m_pItemList[dwIndex2].dwTimestamp);
         break;
      default:
         iResult = 0;
         break;
   }
   return iResult * m_iSortDir;
}


//
// Handler for list view column click
//

void CLastValuesView::OnListViewColumnClick(NMHDR *pNMHDR, LRESULT *pResult)
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
      m_iSortDir = -m_iSortDir;
   }
   else
   {
      // Another sorting column
      m_iSortMode = ((LPNMLISTVIEW)pNMHDR)->iSubItem;
   }
   m_wndListCtrl.SortItems(CompareItems, (DWORD)this);

   // Mark new sorting column
   lvCol.mask = LVCF_IMAGE | LVCF_FMT;
   lvCol.fmt = LVCFMT_BITMAP_ON_RIGHT | LVCFMT_IMAGE | LVCFMT_LEFT;
   lvCol.iImage = (m_iSortDir > 0) ? m_iSortImageBase : (m_iSortImageBase + 1);
   m_wndListCtrl.SetColumn(((LPNMLISTVIEW)pNMHDR)->iSubItem, &lvCol);
   
   *pResult = 0;
}


//
// Handler for "Clear data" menu item
//

static DWORD ClearDCIData(DWORD dwNodeId, DWORD dwItemCount, DWORD *pdwItemList)
{
	DWORD i, dwResult;

	for(i = 0; i < dwItemCount; i++)
	{
		dwResult = NXCClearDCIData(g_hSession, dwNodeId, pdwItemList[i]);
		if (dwResult != RCC_SUCCESS)
			break;
	}
	return dwResult;
}

void CLastValuesView::OnItemCleardata() 
{
   int iItem;
   DWORD i, dwItemCount, *pdwItemList, dwResult;

	if (MessageBox(_T("All collected data for selected items will be deleted. Are you sure?"), _T("Warning"), MB_YESNO | MB_ICONEXCLAMATION) != IDYES)
		return;	// Action cancelled by user

	dwItemCount = m_wndListCtrl.GetSelectedCount();
	pdwItemList = (DWORD *)malloc(sizeof(DWORD) * dwItemCount);

   iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
   for(i = 0; (iItem != -1) && (i < dwItemCount); i++)
   {
      pdwItemList[i] = (DWORD)m_wndListCtrl.GetItemData(iItem);
      iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
   }

   dwResult = DoRequestArg3(ClearDCIData, (void *)m_dwNodeId, (void *)dwItemCount, pdwItemList,
                            _T("Clearing DCI data..."));
   if (dwResult != RCC_SUCCESS)
   {
      theApp.ErrorBox(dwResult, _T("Unable to clear DCI data: %s"));
   }
	safe_free(pdwItemList);
}

void CLastValuesView::OnUpdateItemCleardata(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() > 0);
}
