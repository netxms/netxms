// DCIDataView.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "DCIDataView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDCIDataView

IMPLEMENT_DYNCREATE(CDCIDataView, CMDIChildWnd)

CDCIDataView::CDCIDataView()
{
   m_dwNodeId = 0;
   m_dwItemId = 0;
   m_szItemName[0] = 0;
}

CDCIDataView::CDCIDataView(DWORD dwNodeId, DWORD dwItemId, TCHAR *pszItemName)
{
   m_dwNodeId = dwNodeId;
   m_dwItemId = dwItemId;
   _tcsncpy(m_szItemName, pszItemName, MAX_OBJECT_NAME);
}

CDCIDataView::CDCIDataView(TCHAR *pszParams)
{
   TCHAR szBuffer[32];

   if (ExtractWindowParam(pszParams, _T("N"), szBuffer, 32))
      m_dwNodeId = _tcstoul(szBuffer, NULL, 0);
   else
      m_dwNodeId = 0;

   if (ExtractWindowParam(pszParams, _T("I"), szBuffer, 32))
      m_dwItemId = _tcstoul(szBuffer, NULL, 0);
   else
      m_dwItemId = 0;

   if (!ExtractWindowParam(pszParams, _T("IN"), m_szItemName, MAX_OBJECT_NAME))
      m_szItemName[0] = 0;
}

CDCIDataView::~CDCIDataView()
{
}


BEGIN_MESSAGE_MAP(CDCIDataView, CMDIChildWnd)
	//{{AFX_MSG_MAP(CDCIDataView)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	//}}AFX_MSG_MAP
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_VIEW, OnListViewItemChange)
   ON_MESSAGE(WM_GET_SAVE_INFO, OnGetSaveInfo)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDCIDataView message handlers


//
// WM_CREATE message handler
//

int CDCIDataView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;
   static int widths[3] = { 80, 200, -1 };

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   GetClientRect(&rect);

	m_wndStatusBar.Create(WS_CHILD | WS_VISIBLE, rect, this, IDC_STATUS_BAR);
   m_wndStatusBar.SetParts(3, widths);

   m_iStatusBarHeight = GetWindowSize(&m_wndStatusBar).cy;
   rect.bottom -= m_iStatusBarHeight;
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT, rect, this, IDC_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT | 
                                  LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, "Timestamp", LVCFMT_LEFT, 130);
   m_wndListCtrl.InsertColumn(1, "Value", LVCFMT_LEFT, 
                              rect.right - 130 - GetSystemMetrics(SM_CXVSCROLL));

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
	
	return 0;
}


//
// WM_DESTROY message handler
//

void CDCIDataView::OnDestroy() 
{
	CMDIChildWnd::OnDestroy();
	
	// TODO: Add your message handler code here
	
}


//
// WM_SIZE message handler
//

void CDCIDataView::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);

   m_wndStatusBar.SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOZORDER);
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy - m_iStatusBarHeight, SWP_NOZORDER);
}


//
// WM_SETFOCUS message handler
//

void CDCIDataView::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);

   m_wndListCtrl.SetFocus();
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CDCIDataView::OnViewRefresh() 
{
   DWORD dwResult;
   int iItem;
   NXC_DCI_DATA *pData;

   m_wndListCtrl.DeleteAllItems();
   dwResult = DoRequestArg7(NXCGetDCIData, g_hSession, (void *)m_dwNodeId, (void *)m_dwItemId,
                            (void *)1000, 0, 0, &pData, _T("Loading item data..."));
   if (dwResult == RCC_SUCCESS)
   {
      DWORD i;
      NXC_DCI_ROW *pRow;
      TCHAR szBuffer[256];

      for(i = 0, pRow = pData->pRows; i < pData->dwNumRows; i++)
      {
         FormatTimeStamp(pRow->dwTimeStamp, szBuffer, TS_LONG_DATE_TIME);
         iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, szBuffer);
         if (iItem != -1)
         {
            if (pData->wDataType == DCI_DT_STRING)
            {
               m_wndListCtrl.SetItemText(iItem, 1, pRow->value.szString);
            }
            else
            {
               switch(pData->wDataType)
               {
                  case DCI_DT_INT:
                     sprintf(szBuffer, "%ld", pRow->value.dwInt32);
                     break;
                  case DCI_DT_UINT:
                     sprintf(szBuffer, "%lu", pRow->value.dwInt32);
                     break;
                  case DCI_DT_INT64:
                     sprintf(szBuffer, "%I64d", pRow->value.qwInt64);
                     break;
                  case DCI_DT_UINT64:
                     sprintf(szBuffer, "%I64u", pRow->value.qwInt64);
                     break;
                  case DCI_DT_FLOAT:
                     sprintf(szBuffer, "%f", pRow->value.dFloat);
                     break;
                  default:
                     sprintf(szBuffer, "Unknown data type (%d)", pData->wDataType);
                     break;
               }
               m_wndListCtrl.SetItemText(iItem, 1, szBuffer);
            }
         }
         inc_ptr(pRow, pData->wRowSize, NXC_DCI_ROW);
      }
      sprintf(szBuffer, "%d rows", pData->dwNumRows);
      m_wndStatusBar.SetText(szBuffer, 0, 0);
      NXCDestroyDCIData(pData);
   }
   else
   {
      theApp.ErrorBox(dwResult, _T("Unable to retrieve colected data: %s"));
      iItem = m_wndListCtrl.InsertItem(0, "");
      if (iItem != -1)
         m_wndListCtrl.SetItemText(iItem, 1, _T("ERROR LOADING DATA FROM SERVER"));
   }
}


//
// WM_NOTIFY::LVN_ITEMACTIVATE message handler
//

void CDCIDataView::OnListViewItemChange(LPNMLISTVIEW pNMHDR, LRESULT *pResult)
{
   if (pNMHDR->iItem != -1)
      if (pNMHDR->uChanged & LVIF_STATE)
      {
         if (pNMHDR->uNewState & LVIS_FOCUSED)
         {
            char szBuffer[64];

            sprintf(szBuffer, "Current row: %d", pNMHDR->iItem + 1);
            m_wndStatusBar.SetText(szBuffer, 1, 0);
         }
         else
         {
            m_wndStatusBar.SetText("", 1, 0);
         }
      }
}


//
// Get save info for desktop saving
//

LRESULT CDCIDataView::OnGetSaveInfo(WPARAM wParam, WINDOW_SAVE_INFO *pInfo)
{
   pInfo->iWndClass = WNDC_DCI_DATA;
   GetWindowPlacement(&pInfo->placement);
   _sntprintf(pInfo->szParameters, MAX_DB_STRING, _T("N:%ld\x7FI:%ld\x7FIN:%s"),
              m_dwNodeId, m_dwItemId, m_szItemName);
   return 1;
}
