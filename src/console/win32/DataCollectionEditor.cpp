// DataCollectionEditor.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "DataCollectionEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CDataCollectionEditor

IMPLEMENT_DYNCREATE(CDataCollectionEditor, CMDIChildWnd)

CDataCollectionEditor::CDataCollectionEditor()
{
   m_pItemList = NULL;
}

CDataCollectionEditor::CDataCollectionEditor(NXC_DCI_LIST *pList)
{
   m_pItemList = pList;
}

CDataCollectionEditor::~CDataCollectionEditor()
{
}


BEGIN_MESSAGE_MAP(CDataCollectionEditor, CMDIChildWnd)
	//{{AFX_MSG_MAP(CDataCollectionEditor)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_CLOSE()
	ON_WM_SIZE()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_ITEM_NEW, OnItemNew)
	ON_COMMAND(ID_ITEM_EDIT, OnItemEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDataCollectionEditor message handlers


//
// WM_CREATE message handler
//

int CDataCollectionEditor::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;
   DWORD i;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   // We should't create window without bound item list
   if (m_pItemList == NULL)
      return -1;

   // Create list view control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT, rect, this, ID_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT | 
                                  LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, "ID", LVCFMT_LEFT, 40);
   m_wndListCtrl.InsertColumn(1, "Source", LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(2, "Name", LVCFMT_LEFT, 200);
   m_wndListCtrl.InsertColumn(3, "Data type", LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(4, "Polling Interval", LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(5, "Retention Time", LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(6, "Status", LVCFMT_LEFT, 80);

   // Fill list view with data
   for(i = 0; i < m_pItemList->dwNumItems; i++)
      AddListItem(i, &m_pItemList->pItems[i]);
	
   ((CConsoleApp *)AfxGetApp())->OnViewCreate(IDR_DC_EDITOR, this, m_pItemList->dwNodeId);

	return 0;
}


//
// WM_DESTROY message handler
//

void CDataCollectionEditor::OnDestroy() 
{
   theApp.OnViewDestroy(IDR_DC_EDITOR, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_CLOSE message handler
//

void CDataCollectionEditor::OnClose() 
{
   DWORD dwResult;

   dwResult = DoRequestArg1(NXCCloseNodeDCIList, m_pItemList, "Saving node's data collection information...");
   if (dwResult != RCC_SUCCESS)
   {
      char szBuffer[256];

      sprintf(szBuffer, "Unable to close data collection configuration:\n%s", 
              NXCGetErrorText(dwResult));
      MessageBox(szBuffer, "Error", MB_ICONSTOP);
   }
	
	CMDIChildWnd::OnClose();
}


//
// WM_SIZE message handler
//

void CDataCollectionEditor::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// Add new item to list
//

void CDataCollectionEditor::AddListItem(DWORD dwIndex, NXC_DCI *pItem)
{
   int iItem;
   char szBuffer[32];

   sprintf(szBuffer, "%d", pItem->dwId);
   iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, szBuffer);
   if (iItem != -1)
   {
      m_wndListCtrl.SetItemData(iItem, dwIndex);
      UpdateListItem(iItem, pItem);
   }
}


//
// Update existing list view item
//

void CDataCollectionEditor::UpdateListItem(int iItem, NXC_DCI *pItem)
{
   char szBuffer[32];

   m_wndListCtrl.SetItemText(iItem, 1, g_pszItemOrigin[pItem->iSource]);
   m_wndListCtrl.SetItemText(iItem, 2, pItem->szName);
   m_wndListCtrl.SetItemText(iItem, 3, g_pszItemDataType[pItem->iDataType]);
   sprintf(szBuffer, "%ds", pItem->iPollingInterval);
   m_wndListCtrl.SetItemText(iItem, 4, szBuffer);
   sprintf(szBuffer, "%ds", pItem->iRetentionTime);
   m_wndListCtrl.SetItemText(iItem, 5, szBuffer);
   m_wndListCtrl.SetItemText(iItem, 6, g_pszItemStatus[pItem->iStatus]);
}


//
// WM_CONTEXTMENU message handler
//

void CDataCollectionEditor::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   CMenu *pMenu;

   pMenu = theApp.GetContextMenu(2);
   pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this, NULL);
}


//
// WM_COMMAND::ID_ITEM_NEW message handler
//

void CDataCollectionEditor::OnItemNew() 
{
	// TODO: Add your command handler code here
	
}


//
// WM_COMMAND::ID_ITEM_EDIT message handler
//

void CDataCollectionEditor::OnItemEdit() 
{
   int iItem;
   DWORD dwIndex;

   if (m_wndListCtrl.GetSelectedCount() == 1)
   {
      iItem = m_wndListCtrl.GetSelectionMark();
      dwIndex = m_wndListCtrl.GetItemData(iItem);
      if (EditItem(dwIndex))
         UpdateListItem(iItem, &m_pItemList->pItems[dwIndex]);
   }
}


//
// Start item editing dialog
//

BOOL CDataCollectionEditor::EditItem(DWORD dwIndex)
{
   CDCIPropDlg dlg;
   BOOL bSuccess = FALSE;

   dlg.m_iDataType = m_pItemList->pItems[dwIndex].iDataType;
   dlg.m_iOrigin = m_pItemList->pItems[dwIndex].iSource;
   dlg.m_iPollingInterval = m_pItemList->pItems[dwIndex].iPollingInterval;
   dlg.m_iRetentionTime = m_pItemList->pItems[dwIndex].iRetentionTime;
   dlg.m_iStatus = m_pItemList->pItems[dwIndex].iStatus;
   dlg.m_strName = m_pItemList->pItems[dwIndex].szName;
   if (dlg.DoModal() == IDOK)
   {
      m_pItemList->pItems[dwIndex].iDataType = dlg.m_iDataType;
      m_pItemList->pItems[dwIndex].iSource = dlg.m_iOrigin;
      m_pItemList->pItems[dwIndex].iPollingInterval = dlg.m_iPollingInterval;
      m_pItemList->pItems[dwIndex].iRetentionTime = dlg.m_iRetentionTime;
      m_pItemList->pItems[dwIndex].iStatus = dlg.m_iStatus;
      strcpy(m_pItemList->pItems[dwIndex].szName, (LPCTSTR)dlg.m_strName);
      bSuccess = TRUE;
   }
   return bSuccess;
}
