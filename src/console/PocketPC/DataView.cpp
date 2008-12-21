// DataView.cpp : implementation file
//

#include "stdafx.h"
#include "nxpc.h"
#include "DataView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDataView

CDataView::CDataView()
{
}

CDataView::~CDataView()
{
}


BEGIN_MESSAGE_MAP(CDataView, CDynamicView)
	//{{AFX_MSG_MAP(CDataView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//
// Get view's fingerprint
//

QWORD CDataView::GetFingerprint()
{
   return CREATE_VIEW_FINGERPRINT(VIEW_CLASS_COLLECTED_DATA, m_dwNodeId, m_dwItemId);
}


//
// Initialize view. Called after view object creation, but
// before WM_CREATE. Data is a pointer to object.
//

void CDataView::InitView(void *pData)
{
   m_dwNodeId = ((DATA_VIEW_INIT *)pData)->dwNodeId;
   m_dwItemId = ((DATA_VIEW_INIT *)pData)->dwItemId;
}


/////////////////////////////////////////////////////////////////////////////
// CDataView message handlers


//
// WM_CREATE message handler
//

int CDataView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CDynamicView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   // Setup list control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT, rect, this, ID_LIST_CTRL);
   m_wndListCtrl.InsertColumn(0, _T("Timestamp"), LVCFMT_LEFT, 124);
   m_wndListCtrl.InsertColumn(1, _T("Value"), LVCFMT_LEFT, 
                              rect.right - 124 - GetSystemMetrics(SM_CXVSCROLL));

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH);
	
	return 0;
}


//
// WM_SIZE message handler
//

void CDataView::OnSize(UINT nType, int cx, int cy) 
{
	CDynamicView::OnSize(nType, cx, cy);
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CDataView::OnViewRefresh() 
{
   DWORD dwResult;
   int iItem;
   NXC_DCI_DATA *pData;

   m_wndListCtrl.DeleteAllItems();
   dwResult = DoRequestArg7(NXCGetDCIData, g_hSession, (void *)m_dwNodeId, (void *)m_dwItemId,
                            (void *)100, 0, 0, &pData, _T("Loading item data..."));
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
                     _stprintf(szBuffer, _T("%d"), pRow->value.dwInt32);
                     break;
                  case DCI_DT_UINT:
                     _stprintf(szBuffer, _T("%u"), pRow->value.dwInt32);
                     break;
                  case DCI_DT_INT64:
                     _stprintf(szBuffer, _T("%I64d"), pRow->value.qwInt64);
                     break;
                  case DCI_DT_UINT64:
                     _stprintf(szBuffer, _T("%I64u"), pRow->value.qwInt64);
                     break;
                  case DCI_DT_FLOAT:
                     _stprintf(szBuffer, _T("%f"), pRow->value.dFloat);
                     break;
                  default:
                     _stprintf(szBuffer, _T("Unknown data type (%d)"), pData->wDataType);
                     break;
               }
               m_wndListCtrl.SetItemText(iItem, 1, szBuffer);
            }
         }
         inc_ptr(pRow, pData->wRowSize, NXC_DCI_ROW);
      }
      NXCDestroyDCIData(pData);
   }
   else
   {
      theApp.ErrorBox(dwResult, _T("Unable to retrieve colected data: %s"));
      iItem = m_wndListCtrl.InsertItem(0, _T(""));
      if (iItem != -1)
         m_wndListCtrl.SetItemText(iItem, 1, _T("ERROR LOADING DATA FROM SERVER"));
   }
}
