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
	ON_WM_SETFOCUS()
	ON_UPDATE_COMMAND_UI(ID_ITEM_EDIT, OnUpdateItemEdit)
	ON_COMMAND(ID_ITEM_DELETE, OnItemDelete)
	ON_UPDATE_COMMAND_UI(ID_ITEM_DELETE, OnUpdateItemDelete)
	//}}AFX_MSG_MAP
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_VIEW, OnListViewDblClk)
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
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT, rect, this, IDC_LIST_VIEW);
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
   m_wndListCtrl.InsertColumn(6, "Status", LVCFMT_LEFT, 90);

   // Fill list view with data
   for(i = 0; i < m_pItemList->dwNumItems; i++)
      AddListItem(&m_pItemList->pItems[i]);
	
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
      theApp.ErrorBox(dwResult, "Unable to close data collection configuration:\n%s");
	
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

int CDataCollectionEditor::AddListItem(NXC_DCI *pItem)
{
   int iItem;
   char szBuffer[32];

   sprintf(szBuffer, "%d", pItem->dwId);
   iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, szBuffer);
   if (iItem != -1)
   {
      m_wndListCtrl.SetItemData(iItem, pItem->dwId);
      UpdateListItem(iItem, pItem);
   }
   return iItem;
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
   sprintf(szBuffer, "%d sec", pItem->iPollingInterval);
   m_wndListCtrl.SetItemText(iItem, 4, szBuffer);
   sprintf(szBuffer, "%d days", pItem->iRetentionTime);
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
   DWORD dwResult, dwItemId, dwIndex;
   NXC_DCI item;
   int iItem;

   // Prepare default data collection item
   memset(&item, 0, sizeof(NXC_DCI));
   item.iPollingInterval = 60;
   item.iRetentionTime = 30;
   item.iSource = DS_NATIVE_AGENT;
   item.iStatus = ITEM_STATUS_ACTIVE;

   if (EditItem(&item))
   {
      dwResult = DoRequestArg2(NXCCreateNewDCI, m_pItemList, &dwItemId, 
                               "Creating new data collection item...");
      if (dwResult == RCC_SUCCESS)
      {
         dwIndex = NXCItemIndex(m_pItemList, dwItemId);
         item.dwId = dwItemId;
         if (dwIndex != INVALID_INDEX)
         {
            dwResult = DoRequestArg2(NXCUpdateDCI, (void *)m_pItemList->dwNodeId, &item,
                                     "Updating data collection item...");
            if (dwResult == RCC_SUCCESS)
            {
               memcpy(&m_pItemList->pItems[dwIndex], &item, sizeof(NXC_DCI));
               iItem = AddListItem(&item);
               SelectListItem(iItem);
            }
            else
            {
               theApp.ErrorBox(dwResult, "Unable to update data collection item: %s");
            }
         }
      }
      else
      {
         theApp.ErrorBox(dwResult, "Unable to create new data collection item: %s");
      }
   }
}


//
// WM_COMMAND::ID_ITEM_EDIT message handler
//

void CDataCollectionEditor::OnItemEdit() 
{
   int iItem;
   DWORD dwItemId, dwIndex, dwResult;
   NXC_DCI item;

   if (m_wndListCtrl.GetSelectedCount() == 1)
   {
      iItem = m_wndListCtrl.GetNextItem(-1, LVIS_FOCUSED);
      dwItemId = m_wndListCtrl.GetItemData(iItem);
      dwIndex = NXCItemIndex(m_pItemList, dwItemId);
      if (dwIndex != INVALID_INDEX)
      {
         memcpy(&item, &m_pItemList->pItems[dwIndex], sizeof(NXC_DCI));
         if (EditItem(&item))
         {
            dwResult = DoRequestArg2(NXCUpdateDCI, (void *)m_pItemList->dwNodeId, &item,
                                     "Updating data collection item...");
            if (dwResult == RCC_SUCCESS)
            {
               memcpy(&m_pItemList->pItems[dwIndex], &item, sizeof(NXC_DCI));
               UpdateListItem(iItem, &m_pItemList->pItems[dwIndex]);
            }
            else
            {
               theApp.ErrorBox(dwResult, "Unable to update data collection item: %s");
            }
         }
      }
   }
}


//
// Start item editing dialog
//

BOOL CDataCollectionEditor::EditItem(NXC_DCI *pItem)
{
   CDCIPropDlg dlg;
   BOOL bSuccess = FALSE;

   dlg.m_iDataType = pItem->iDataType;
   dlg.m_iOrigin = pItem->iSource;
   dlg.m_iPollingInterval = pItem->iPollingInterval;
   dlg.m_iRetentionTime = pItem->iRetentionTime;
   dlg.m_iStatus = pItem->iStatus;
   dlg.m_strName = pItem->szName;
   if (dlg.DoModal() == IDOK)
   {
      pItem->iDataType = dlg.m_iDataType;
      pItem->iSource = dlg.m_iOrigin;
      pItem->iPollingInterval = dlg.m_iPollingInterval;
      pItem->iRetentionTime = dlg.m_iRetentionTime;
      pItem->iStatus = dlg.m_iStatus;
      strcpy(pItem->szName, (LPCTSTR)dlg.m_strName);
      bSuccess = TRUE;
   }
   return bSuccess;
}


//
// WM_SETFOCUS message handler
//

void CDataCollectionEditor::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
   m_wndListCtrl.SetFocus();
}


//
// UPDATE_COMMAND_UI handlers
//

void CDataCollectionEditor::OnUpdateItemEdit(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() == 1);
}

void CDataCollectionEditor::OnUpdateItemDelete(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() > 0);
}


//
// Clear list view selection and select specified item
//

void CDataCollectionEditor::SelectListItem(int iItem)
{
   int iOldItem;

   // Clear current selection
   iOldItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
   while(iOldItem != -1)
   {
      m_wndListCtrl.SetItemState(iOldItem, 0, LVIS_SELECTED);
      iOldItem = m_wndListCtrl.GetNextItem(iOldItem, LVNI_SELECTED);
   }

   // Remove current focus
   iOldItem = m_wndListCtrl.GetNextItem(-1, LVNI_FOCUSED);
   if (iOldItem != -1)
      m_wndListCtrl.SetItemState(iOldItem, 0, LVIS_FOCUSED);

   // Select and set focus to new item
   m_wndListCtrl.EnsureVisible(iItem, FALSE);
   m_wndListCtrl.SetItemState(iItem, LVIS_SELECTED | LVIS_FOCUSED, 
                              LVIS_SELECTED | LVIS_FOCUSED);
   m_wndListCtrl.SetSelectionMark(iItem);
}


//
// WM_COMMAND::ID_ITEM_DELETE message handler
//

void CDataCollectionEditor::OnItemDelete() 
{
   int iItem;
   DWORD dwItemId, dwResult;

   iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
   while(iItem != -1)
   {
      dwItemId = m_wndListCtrl.GetItemData(iItem);
      dwResult = DoRequestArg2(NXCDeleteDCI, m_pItemList, (void *)dwItemId,
                               "Deleting data collection item...");
      if (dwResult != RCC_SUCCESS)
      {
         theApp.ErrorBox(dwResult, "Unable to delete data collection item: %s");
         break;
      }
      m_wndListCtrl.DeleteItem(iItem);
      iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
   }
}


//
// Handler for WM_NOTIFY::NM_DBLCLK from IDC_LIST_VIEW
//

void CDataCollectionEditor::OnListViewDblClk(LPNMITEMACTIVATE pNMHDR, LRESULT *pResult)
{
   PostMessage(WM_COMMAND, ID_ITEM_EDIT, 0);
}
