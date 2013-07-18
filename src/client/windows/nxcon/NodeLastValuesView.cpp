// NodeLastValuesView.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "NodeLastValuesView.h"
#include "DataExportDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Worker thread
//

static THREAD_RESULT THREAD_CALL LastValuesViewWorkerThread(void *pArg)
{
	((CNodeLastValuesView *)pArg)->WorkerThread();
	return THREAD_OK;
}


/////////////////////////////////////////////////////////////////////////////
// CNodeLastValuesView

CNodeLastValuesView::CNodeLastValuesView()
{
	m_nTimer = 0;
   m_iSortMode = theApp.GetProfileInt(_T("NodeLastValuesView"), _T("SortMode"), 1);
   m_iSortDir = theApp.GetProfileInt(_T("NodeLastValuesView"), _T("SortDir"), 1);
   m_bUseMultipliers = theApp.GetProfileInt(_T("NodeLastValuesView"), _T("UseMultipliers"), TRUE);
	m_hWorkerThread = INVALID_THREAD_HANDLE;
	m_pItemList = NULL;
	m_dwNumItems = 0;
}

CNodeLastValuesView::~CNodeLastValuesView()
{
   theApp.WriteProfileInt(_T("NodeLastValuesView"), _T("SortMode"), m_iSortMode);
   theApp.WriteProfileInt(_T("NodeLastValuesView"), _T("SortDir"), m_iSortDir);
   theApp.WriteProfileInt(_T("NodeLastValuesView"), _T("UseMultipliers"), m_bUseMultipliers);
	m_workerQueue.Clear();
	m_workerQueue.Put((void *)INVALID_INDEX);
	ThreadJoin(m_hWorkerThread);
	safe_free(m_pItemList);
}


BEGIN_MESSAGE_MAP(CNodeLastValuesView, CWnd)
	//{{AFX_MSG_MAP(CNodeLastValuesView)
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_ITEM_GRAPH, OnItemGraph)
	ON_COMMAND(ID_ITEM_EXPORTDATA, OnItemExportdata)
	ON_COMMAND(ID_ITEM_SHOWDATA, OnItemShowdata)
	ON_WM_TIMER()
	ON_COMMAND(ID_LATSTVALUES_USEMULTIPLIERS, OnLatstvaluesUsemultipliers)
	ON_COMMAND(ID_ITEM_CLEARDATA, OnItemCleardata)
	//}}AFX_MSG_MAP
   ON_MESSAGE(NXCM_SET_OBJECT, OnSetObject)
   ON_MESSAGE(NXCM_REQUEST_COMPLETED, OnRequestCompleted)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_VIEW, OnListViewColumnClick)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CNodeLastValuesView message handlers


//
// WM_CREATE message handler
//

int CNodeLastValuesView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;
   LVCOLUMN lvCol;

	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   // Create image list
   m_imageList.Create(16, 16, ILC_COLOR24 | ILC_MASK, 5, 1);
   m_imageList.Add(theApp.LoadIcon(IDI_ACTIVE));
   m_imageList.Add(theApp.LoadIcon(IDI_DISABLED));
   m_imageList.Add(theApp.LoadIcon(IDI_UNSUPPORTED));
   m_iSortImageBase = m_imageList.GetImageCount();
   m_imageList.Add(theApp.LoadIcon(IDI_SORT_UP));
   m_imageList.Add(theApp.LoadIcon(IDI_SORT_DOWN));

   // Create list view control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHAREIMAGELISTS,
                        rect, this, IDC_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT | 
                                  LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_GRIDLINES);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);
   m_wndListCtrl.SetImageList(&m_imageList, LVSIL_SMALL);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, _T("ID"), LVCFMT_LEFT, 55);
   m_wndListCtrl.InsertColumn(1, _T("Description"), LVCFMT_LEFT, 250);
   m_wndListCtrl.InsertColumn(2, _T("Value"), LVCFMT_LEFT | LVCFMT_BITMAP_ON_RIGHT, 100);
   m_wndListCtrl.InsertColumn(3, _T("Timestamp"), LVCFMT_LEFT, 124);

   // Mark sorting column
   lvCol.mask = LVCF_IMAGE | LVCF_FMT;
   lvCol.fmt = LVCFMT_BITMAP_ON_RIGHT | LVCFMT_IMAGE | LVCFMT_LEFT;
   lvCol.iImage = (m_iSortDir > 0) ? m_iSortImageBase : (m_iSortImageBase + 1);
   m_wndListCtrl.SetColumn(m_iSortMode, &lvCol);
		
	m_hWorkerThread = ThreadCreateEx(LastValuesViewWorkerThread, 0, this);

	return 0;
}


//
// WM_SETFOCUS message handler
//

void CNodeLastValuesView::OnSetFocus(CWnd* pOldWnd) 
{
	CWnd::OnSetFocus(pOldWnd);
	m_wndListCtrl.SetFocus();
}


//
// WM_SIZE message handler
//

void CNodeLastValuesView::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);
	m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// NXCM_SET_OBJECT message handler
//

LRESULT CNodeLastValuesView::OnSetObject(WPARAM wParam, LPARAM lParam)
{
	if (m_nTimer != 0)
	{
		KillTimer(m_nTimer);
		m_nTimer = 0;
	}

   m_pObject = (NXC_OBJECT *)lParam;
	m_workerQueue.Clear();
	m_workerQueue.Put((void *)m_pObject->dwId);
	return 0;
}


//
// WM_DESTROY message handler
//

void CNodeLastValuesView::OnDestroy() 
{
	if (m_nTimer != 0)
		KillTimer(m_nTimer);
	CWnd::OnDestroy();
}


//
// Worker thread
//

void CNodeLastValuesView::WorkerThread(void)
{
	UINT32 dwNodeId, dwResult, dwNumItems;
	NXC_DCI_VALUE *pValueList;

	while(1)
	{
		dwNodeId = (DWORD)m_workerQueue.GetOrBlock();
		if (dwNodeId == INVALID_INDEX)
			break;

		dwResult = NXCGetLastValues(g_hSession, dwNodeId, &dwNumItems, &pValueList);
		if (dwResult == RCC_SUCCESS)
		{
			if (!::PostMessage(m_hWnd, NXCM_REQUEST_COMPLETED, dwNumItems, (LPARAM)pValueList))
			{
				safe_free(pValueList);
			}
		}
		else
		{
			::PostMessage(m_wndListCtrl.m_hWnd, LVM_DELETEALLITEMS, 0, 0);
		}
	}
}


//
// Callback for item sorting
//

static int CALLBACK CompareListItemsCB(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	return ((CNodeLastValuesView *)lParamSort)->CompareListItems(lParam1, lParam2);
}


//
// Compare two list items
//

int CNodeLastValuesView::CompareListItems(LPARAM nItem1, LPARAM nItem2)
{
	int nResult;

	if (m_pItemList == NULL)
		return 0;	// Paranoia check

	switch(m_iSortMode)
	{
		case 0:	// ID
			nResult = COMPARE_NUMBERS(m_pItemList[nItem1].dwId, m_pItemList[nItem2].dwId);
			break;
		case 1:	// Description
			nResult = _tcsicmp(m_pItemList[nItem1].szDescription, m_pItemList[nItem2].szDescription);
			break;
		case 2:	// Value
			nResult = _tcsicmp(m_pItemList[nItem1].szValue, m_pItemList[nItem2].szValue);
			break;
		case 3:	// Timestamp
			nResult = COMPARE_NUMBERS(m_pItemList[nItem1].dwTimestamp, m_pItemList[nItem2].dwTimestamp);
			break;
		default:
			nResult = 0;
			break;
	}
	return nResult * m_iSortDir;
}


//
// Handler for NXCM_REQUEST_COMPLETED message
//

LRESULT CNodeLastValuesView::OnRequestCompleted(WPARAM wParam, LPARAM lParam)
{
	DWORD i;
	int nItem;
	TCHAR szBuffer[128];

	safe_free(m_pItemList);
	m_dwNumItems = (DWORD)wParam;
	m_pItemList = (NXC_DCI_VALUE *)lParam;
	m_wndListCtrl.DeleteAllItems();
	for(i = 0; i < m_dwNumItems; i++)
	{
		_sntprintf_s(szBuffer, 128, _TRUNCATE, _T("%d"), m_pItemList[i].dwId);
		nItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, szBuffer, m_pItemList[i].nStatus);
		if (nItem != -1)
		{
			m_wndListCtrl.SetItemData(nItem, i);
			m_wndListCtrl.SetItemText(nItem, 1, m_pItemList[i].szDescription);

			if ((m_pItemList[i].nDataType != DCI_DT_STRING) &&
				 (m_bUseMultipliers))
			{
				QWORD ui64;
				INT64 i64;
				double d;

				switch(m_pItemList[i].nDataType)
				{
					case DCI_DT_INT:
					case DCI_DT_INT64:
						i64 = _tcstoll(m_pItemList[i].szValue, NULL, 10);
						if (i64 >= 10000000000)
						{
							_sntprintf_s(szBuffer, 128, _TRUNCATE, INT64_FMT _T(" G"), i64 / 1000000000);
						}
						else if (i64 >= 10000000)
						{
							_sntprintf_s(szBuffer, 128, _TRUNCATE, INT64_FMT _T(" M"), i64 / 1000000);
						}
						else if (i64 >= 10000)
						{
							_sntprintf_s(szBuffer, 128, _TRUNCATE, INT64_FMT _T(" K"), i64 / 1000);
						}
						else if (i64 >= 0)
						{
							_sntprintf_s(szBuffer, 128, _TRUNCATE, INT64_FMT, i64);
						}
						else if (i64 <= -10000000000)
						{
							_sntprintf_s(szBuffer, 128, _TRUNCATE, INT64_FMT _T(" G"), i64 / 1000000000);
						}
						else if (i64 <= -10000000)
						{
							_sntprintf_s(szBuffer, 128, _TRUNCATE, INT64_FMT _T(" M"), i64 / 1000000);
						}
						else if (i64 <= 10000)
						{
							_sntprintf_s(szBuffer, 128, _TRUNCATE, INT64_FMT _T(" K"), i64 / 1000);
						}
						else
						{
							_sntprintf_s(szBuffer, 128, _TRUNCATE, INT64_FMT, i64);
						}
						break;
					case DCI_DT_UINT:
					case DCI_DT_UINT64:
						ui64 = _tcstoull(m_pItemList[i].szValue, NULL, 10);
						if (ui64 >= 10000000000)
						{
							_sntprintf_s(szBuffer, 128, _TRUNCATE, UINT64_FMT _T(" G"), ui64 / 1000000000);
						}
						else if (ui64 >= 10000000)
						{
							_sntprintf_s(szBuffer, 128, _TRUNCATE, UINT64_FMT _T(" M"), ui64 / 1000000);
						}
						else if (ui64 >= 10000)
						{
							_sntprintf_s(szBuffer, 128, _TRUNCATE, UINT64_FMT _T(" K"), ui64 / 1000);
						}
						else
						{
							_sntprintf_s(szBuffer, 128, _TRUNCATE, UINT64_FMT, ui64);
						}
						break;
					case DCI_DT_FLOAT:
						d = _tcstod(m_pItemList[i].szValue, NULL);
						if (d >= 10000000000)
						{
							_sntprintf_s(szBuffer, 128, _TRUNCATE, _T("%.2f G"), d / 1000000000);
						}
						else if (d >= 10000000)
						{
							_sntprintf_s(szBuffer, 128, _TRUNCATE, _T("%.2f M"), d / 1000000);
						}
						else if (d >= 10000)
						{
							_sntprintf_s(szBuffer, 128, _TRUNCATE, _T("%.2f K"), d / 1000);
						}
						else if (d >= 0)
						{
							_sntprintf_s(szBuffer, 128, _TRUNCATE, _T("%f"), d);
						}
						else if (d <= -10000000000)
						{
							_sntprintf_s(szBuffer, 128, _TRUNCATE, _T("%.2f G"), d / 1000000000);
						}
						else if (d <= -10000000)
						{
							_sntprintf_s(szBuffer, 128, _TRUNCATE, _T("%.2f M"), d / 1000000);
						}
						else if (d <= 10000)
						{
							_sntprintf_s(szBuffer, 128, _TRUNCATE, _T("%.2f K"), d / 1000);
						}
						else
						{
							_sntprintf_s(szBuffer, 128, _TRUNCATE,_T("%f"), d);
						}
						break;
					default:
						nx_strncpy(szBuffer, m_pItemList[i].szValue, 64);
						break;
				}
				m_wndListCtrl.SetItemText(nItem, 2, szBuffer);
			}
			else
			{
				m_wndListCtrl.SetItemText(nItem, 2, m_pItemList[i].szValue);
			}

			m_wndListCtrl.SetItemText(nItem, 3, FormatTimeStamp(m_pItemList[i].dwTimestamp, szBuffer, TS_LONG_DATE_TIME));
		}
	}
	m_wndListCtrl.SortItems(CompareListItemsCB, (UINT_PTR)this);
	m_nTimer = SetTimer(1, 30000, NULL);
	return 0;
}


//
// Handler for list view column click
//

void CNodeLastValuesView::OnListViewColumnClick(NMHDR *pNMHDR, LRESULT *pResult)
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
   m_wndListCtrl.SortItems(CompareListItemsCB, (UINT_PTR)this);

   // Mark new sorting column
   lvCol.mask = LVCF_IMAGE | LVCF_FMT;
   lvCol.fmt = LVCFMT_BITMAP_ON_RIGHT | LVCFMT_IMAGE | LVCFMT_LEFT;
   lvCol.iImage = (m_iSortDir > 0) ? m_iSortImageBase : (m_iSortImageBase + 1);
   m_wndListCtrl.SetColumn(((LPNMLISTVIEW)pNMHDR)->iSubItem, &lvCol);
   
   *pResult = 0;
}


//
// WM_CONTEXTMENU message handler
//

void CNodeLastValuesView::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   CMenu *pMenu;
	UINT nCount;

   pMenu = theApp.GetContextMenu(26);
	nCount = m_wndListCtrl.GetSelectedCount();
	pMenu->EnableMenuItem(ID_ITEM_SHOWDATA, MF_BYCOMMAND | ((nCount > 0) ? MF_ENABLED : MF_GRAYED));
	pMenu->EnableMenuItem(ID_ITEM_GRAPH, MF_BYCOMMAND | ((nCount > 0) ? MF_ENABLED : MF_GRAYED));
	pMenu->EnableMenuItem(ID_ITEM_EXPORTDATA, MF_BYCOMMAND | ((nCount == 1) ? MF_ENABLED : MF_GRAYED));
	pMenu->EnableMenuItem(ID_ITEM_CLEARDATA, MF_BYCOMMAND | ((nCount > 0) ? MF_ENABLED : MF_GRAYED));
	pMenu->CheckMenuItem(ID_LATSTVALUES_USEMULTIPLIERS, MF_BYCOMMAND | (m_bUseMultipliers ? MF_CHECKED : MF_UNCHECKED));
   pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this, NULL);
}


//
// Handler for "Graph" context menu item
//

void CNodeLastValuesView::OnItemGraph() 
{
   int iItem;
   DWORD i, dwNumItems, dwIndex;
   NXC_DCI **ppItemList;
   TCHAR szBuffer[384];

	if ((m_pObject == NULL) || (m_pItemList == NULL))
		return;	// Paranoia check

   dwNumItems = m_wndListCtrl.GetSelectedCount();
   ppItemList = (NXC_DCI **)malloc(sizeof(NXC_DCI *) * dwNumItems);

   iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
   for(i = 0; (iItem != -1) && (i < dwNumItems); i++)
   {
      ppItemList[i] = (NXC_DCI *)malloc(sizeof(NXC_DCI));
      memset(ppItemList[i], 0, sizeof(NXC_DCI));
		dwIndex = (DWORD)m_wndListCtrl.GetItemData(iItem);
      ppItemList[i]->dwId = m_pItemList[dwIndex].dwId;
      _tcscpy(ppItemList[i]->szName, m_pItemList[dwIndex].szName);
      _tcscpy(ppItemList[i]->szDescription, m_pItemList[dwIndex].szDescription);
      ppItemList[i]->iDataType = m_pItemList[dwIndex].nDataType;
      ppItemList[i]->iSource = m_pItemList[dwIndex].nSource;
      iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
   }

   if (dwNumItems == 1)
   {
      _sntprintf(szBuffer, 384, _T("%s - %s"), m_pObject->szName,
                 ppItemList[0]->szDescription);
   }
   else
   {
      nx_strncpy(szBuffer, m_pObject->szName, 384);
   }

   theApp.ShowDCIGraph(m_pObject->dwId, dwNumItems, ppItemList, szBuffer);
   for(i = 0; i < dwNumItems; i++)
      free(ppItemList[i]);
   free(ppItemList);
}


//
// Handler for "Export data" context menu item
//

void CNodeLastValuesView::OnItemExportdata() 
{
   CDataExportDlg dlg;
   DWORD dwItemId, dwTimeFrom, dwTimeTo;

	if ((m_pObject == NULL) || (m_pItemList == NULL))
		return;	// Paranoia check

   dwItemId = m_pItemList[m_wndListCtrl.GetItemData(m_wndListCtrl.GetSelectionMark())].dwId;
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
      theApp.ExportDCIData(m_pObject->dwId, dwItemId, dwTimeFrom, dwTimeTo,
                           dlg.m_iSeparator, dlg.m_iTimeStampFormat,
                           (LPCTSTR)dlg.m_strFileName);
   }
}


//
// Handler for "Show data" context menu item
//

void CNodeLastValuesView::OnItemShowdata() 
{
   int iItem;
   DWORD dwItemId;
   TCHAR szBuffer[384];

	if ((m_pObject == NULL) || (m_pItemList == NULL))
		return;	// Paranoia check

   iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
   while(iItem != -1)
   {
      dwItemId = m_pItemList[m_wndListCtrl.GetItemData(iItem)].dwId;
      _sntprintf_s(szBuffer, 384, _TRUNCATE, _T("%s - "), m_pObject->szName); 
      m_wndListCtrl.GetItemText(iItem, 1, &szBuffer[_tcslen(szBuffer)],
                                384 - (int)_tcslen(szBuffer));
      theApp.ShowDCIData(m_pObject->dwId, dwItemId, szBuffer);
      iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
   }
}


//
// WM_TIMER message handler
//

void CNodeLastValuesView::OnTimer(UINT_PTR nIDEvent) 
{
	m_workerQueue.Put((void *)m_pObject->dwId);
}


//
// Control usage of multipliers for large values
//

void CNodeLastValuesView::OnLatstvaluesUsemultipliers() 
{
	m_bUseMultipliers = !m_bUseMultipliers;
	m_workerQueue.Put((void *)m_pObject->dwId);	// Refresh view
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

void CNodeLastValuesView::OnItemCleardata() 
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
      pdwItemList[i] = m_pItemList[m_wndListCtrl.GetItemData(iItem)].dwId;
      iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
   }

   dwResult = DoRequestArg3(ClearDCIData, (void *)m_pObject->dwId, (void *)dwItemCount, pdwItemList,
                            _T("Clearing DCI data..."));
   if (dwResult != RCC_SUCCESS)
   {
      theApp.ErrorBox(dwResult, _T("Unable to clear DCI data: %s"));
   }
	safe_free(pdwItemList);
}
