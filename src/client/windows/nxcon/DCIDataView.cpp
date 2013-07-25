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
   m_nScale = 1;
}

CDCIDataView::CDCIDataView(DWORD dwNodeId, DWORD dwItemId, TCHAR *pszItemName)
{
   m_dwNodeId = dwNodeId;
   m_dwItemId = dwItemId;
   nx_strncpy(m_szItemName, pszItemName, MAX_OBJECT_NAME + MAX_ITEM_NAME + 4);
   m_nScale = 1;
}

CDCIDataView::CDCIDataView(TCHAR *pszParams)
{
   m_dwNodeId = ExtractWindowParamULong(pszParams, _T("N"), 0);
   m_dwItemId = ExtractWindowParamULong(pszParams, _T("I"), 0);
   if (!ExtractWindowParam(pszParams, _T("IN"), m_szItemName,
                           MAX_OBJECT_NAME + MAX_ITEM_NAME + 4))
      m_szItemName[0] = 0;
   m_nScale = 1;
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
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_DATA_SCALE_GBYTES, OnDataScaleGbytes)
	ON_COMMAND(ID_DATA_SCALE_GIGA, OnDataScaleGiga)
	ON_COMMAND(ID_DATA_SCALE_KBYTES, OnDataScaleKbytes)
	ON_COMMAND(ID_DATA_SCALE_KILO, OnDataScaleKilo)
	ON_COMMAND(ID_DATA_SCALE_MBYTES, OnDataScaleMbytes)
	ON_COMMAND(ID_DATA_SCALE_MEGA, OnDataScaleMega)
	ON_COMMAND(ID_DATA_SCALE_NORMAL, OnDataScaleNormal)
	ON_COMMAND(ID_DATA_COPYTOCLIPBOARD, OnDataCopytoclipboard)
	//}}AFX_MSG_MAP
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_VIEW, OnListViewItemChange)
   ON_MESSAGE(NXCM_GET_SAVE_INFO, OnGetSaveInfo)
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
   m_wndListCtrl.InsertColumn(0, _T("Timestamp"), LVCFMT_LEFT, 130);
   m_wndListCtrl.InsertColumn(1, _T("Value"), LVCFMT_LEFT, 
                              rect.right - 130 - GetSystemMetrics(SM_CXVSCROLL));

   DisplayScale();
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

      // Fill list control with data
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
                     _sntprintf_s(szBuffer, 256, _TRUNCATE, _T("%d"), pRow->value.dwInt32 / m_nScale);
                     break;
                  case DCI_DT_UINT:
                     _sntprintf_s(szBuffer, 256, _TRUNCATE, _T("%u"), pRow->value.dwInt32 / m_nScale);
                     break;
                  case DCI_DT_INT64:
                     _sntprintf_s(szBuffer, 256, _TRUNCATE, _T("%I64d"), pRow->value.ext.v64.qwInt64 / m_nScale);
                     break;
                  case DCI_DT_UINT64:
                     _sntprintf_s(szBuffer, 256, _TRUNCATE, _T("%I64u"), pRow->value.ext.v64.qwInt64 / m_nScale);
                     break;
                  case DCI_DT_FLOAT:
                     _sntprintf_s(szBuffer, 256, _TRUNCATE, _T("%f"), pRow->value.ext.v64.dFloat / m_nScale);
                     break;
                  default:
                     _sntprintf_s(szBuffer, 256, _TRUNCATE, _T("Unknown data type (%d)"), pData->wDataType);
                     break;
               }
               m_wndListCtrl.SetItemText(iItem, 1, szBuffer);
            }
         }
         inc_ptr(pRow, pData->wRowSize, NXC_DCI_ROW);
      }
      _sntprintf_s(szBuffer, 256, _TRUNCATE, _T("%d rows"), pData->dwNumRows);
      m_wndStatusBar.SetText(szBuffer, 0, 0);
      NXCDestroyDCIData(pData);
   }
   else
   {
      theApp.ErrorBox(dwResult, _T("Unable to retrieve collected data: %s"));
      iItem = m_wndListCtrl.InsertItem(0, _T(""));
      if (iItem != -1)
         m_wndListCtrl.SetItemText(iItem, 1, _T("ERROR LOADING DATA FROM SERVER"));
   }
}


//
// WM_NOTIFY::LVN_ITEMACTIVATE message handler
//

void CDCIDataView::OnListViewItemChange(NMHDR *pNMHDR, LRESULT *pResult)
{
   if (((LPNMLISTVIEW)pNMHDR)->iItem != -1)
      if (((LPNMLISTVIEW)pNMHDR)->uChanged & LVIF_STATE)
      {
         if (((LPNMLISTVIEW)pNMHDR)->uNewState & LVIS_FOCUSED)
         {
            TCHAR szBuffer[64];

            _sntprintf_s(szBuffer, 64, _TRUNCATE, _T("Current row: %d"), ((LPNMLISTVIEW)pNMHDR)->iItem + 1);
            m_wndStatusBar.SetText(szBuffer, 1, 0);
         }
         else
         {
            m_wndStatusBar.SetText(_T(""), 1, 0);
         }
      }
}


//
// Get save info for desktop saving
//

LRESULT CDCIDataView::OnGetSaveInfo(WPARAM wParam, LPARAM lParam)
{
	WINDOW_SAVE_INFO *pInfo = (WINDOW_SAVE_INFO *)lParam;
   pInfo->iWndClass = WNDC_DCI_DATA;
   GetWindowPlacement(&pInfo->placement);
   _sntprintf(pInfo->szParameters, MAX_DB_STRING, _T("N:%d\x7FI:%d\x7FIN:%s"),
              m_dwNodeId, m_dwItemId, m_szItemName);
   return 1;
}


//
// WM_CONTEXTMENU message handler
//

void CDCIDataView::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   CMenu *pMenu;

   pMenu = theApp.GetContextMenu(21);
   pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this, NULL);
}


//
// Scale presets
//

void CDCIDataView::OnDataScaleKbytes() 
{
   SetScale(1024);
}

void CDCIDataView::OnDataScaleKilo() 
{
   SetScale(1000);
}

void CDCIDataView::OnDataScaleMbytes() 
{
   SetScale(1048576);
}

void CDCIDataView::OnDataScaleMega() 
{
   SetScale(1000000);
}

void CDCIDataView::OnDataScaleGbytes() 
{
   SetScale(1073741824);
}

void CDCIDataView::OnDataScaleGiga() 
{
   SetScale(1000000000);
}

void CDCIDataView::OnDataScaleNormal() 
{
   SetScale(1);
}


//
// Display current scale on status bar
//

void CDCIDataView::DisplayScale()
{
   TCHAR szBuffer[256];

   _sntprintf_s(szBuffer, 256, _TRUNCATE, _T("1:%d"), m_nScale);
   switch(m_nScale)
   {
      case 1024:
         _tcscat(szBuffer, _T(" (Kbytes)"));
         break;
      case 1048576:
         _tcscat(szBuffer, _T(" (Mbytes)"));
         break;
      case 1073741824:
         _tcscat(szBuffer, _T(" (Gbytes)"));
         break;
      default:
         break;
   }
   m_wndStatusBar.SetText(szBuffer, 2, 0);
}


//
// Set new scale for values
//

void CDCIDataView::SetScale(int nScale)
{
   m_nScale = nScale;
   DisplayScale();
   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
}


//
// WM_COMMAND::ID_DATA_COPYTOCLIPBOARD message handler
//

void CDCIDataView::OnDataCopytoclipboard() 
{
   CString str;
   int i, nLines;
   HANDLE hMem;
   TCHAR *pMem;

   str.Empty();
   nLines = m_wndListCtrl.GetItemCount();
   for(i = 0; i < nLines; i++)
   {
      str += m_wndListCtrl.GetItemText(i, 0);
      str += _T(" ");
      str += m_wndListCtrl.GetItemText(i, 1);
      str += _T("\r\n");
   }

   hMem = GlobalAlloc(GMEM_MOVEABLE, (str.GetLength() + 1) * sizeof(TCHAR));
   if (hMem != NULL)
   {
      pMem = (TCHAR *)GlobalLock(hMem);
      if (pMem != NULL)
      {
         _tcscpy(pMem, (LPCTSTR)str);
         GlobalUnlock(hMem);

         if (OpenClipboard())
         {
            EmptyClipboard();
            SetClipboardData(CF_TEXT, hMem);
            CloseClipboard();
         }
         else
         {
            MessageBox(_T("Cannot open clipboard"), _T("Error"), MB_OK | MB_ICONSTOP);
         }
      }
      GlobalFree(hMem);
   }
}
