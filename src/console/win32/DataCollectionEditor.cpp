// DataCollectionEditor.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "DataCollectionEditor.h"
#include "DCIPropPage.h"
#include "DCIThresholdsPage.h"
#include "DCITransformPage.h"

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
	ON_COMMAND(ID_ITEM_SHOWDATA, OnItemShowdata)
	ON_UPDATE_COMMAND_UI(ID_ITEM_SHOWDATA, OnUpdateItemShowdata)
	ON_COMMAND(ID_ITEM_GRAPH, OnItemGraph)
	ON_UPDATE_COMMAND_UI(ID_ITEM_GRAPH, OnUpdateItemGraph)
	ON_COMMAND(ID_ITEM_COPY, OnItemCopy)
	ON_COMMAND(ID_FILE_EXPORT, OnFileExport)
	ON_UPDATE_COMMAND_UI(ID_FILE_EXPORT, OnUpdateFileExport)
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_COMMAND(ID_ITEM_DUPLICATE, OnItemDuplicate)
	ON_UPDATE_COMMAND_UI(ID_ITEM_COPY, OnUpdateItemCopy)
	ON_UPDATE_COMMAND_UI(ID_ITEM_DUPLICATE, OnUpdateItemDuplicate)
	ON_COMMAND(ID_ITEM_DISABLE, OnItemDisable)
	ON_COMMAND(ID_ITEM_ACTIVATE, OnItemActivate)
	ON_UPDATE_COMMAND_UI(ID_ITEM_ACTIVATE, OnUpdateItemActivate)
	ON_UPDATE_COMMAND_UI(ID_ITEM_DISABLE, OnUpdateItemDisable)
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

   // Create image list
   m_imageList.Create(16, 16, ILC_COLOR24 | ILC_MASK, 4, 1);
   m_imageList.Add(theApp.LoadIcon(IDI_ACTIVE));
   m_imageList.Add(theApp.LoadIcon(IDI_DISABLED));
   m_imageList.Add(theApp.LoadIcon(IDI_UNSUPPORTED));

   // Create list view control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT, rect, this, IDC_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT | 
                                  LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);
   m_wndListCtrl.SetImageList(&m_imageList, LVSIL_SMALL);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, "ID", LVCFMT_LEFT, 40);
   m_wndListCtrl.InsertColumn(1, "Source", LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(2, "Description", LVCFMT_LEFT, 150);
   m_wndListCtrl.InsertColumn(3, "Parameter", LVCFMT_LEFT, 150);
   m_wndListCtrl.InsertColumn(4, "Data type", LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(5, "Polling Interval", LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(6, "Retention Time", LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(7, "Status", LVCFMT_LEFT, 90);

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

   dwResult = DoRequestArg1(NXCCloseNodeDCIList, m_pItemList, "Unlocking node's data collection information...");
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
   m_wndListCtrl.SetItemText(iItem, 2, pItem->szDescription);
   m_wndListCtrl.SetItemText(iItem, 3, pItem->szName);
   m_wndListCtrl.SetItemText(iItem, 4, g_pszItemDataType[pItem->iDataType]);
   sprintf(szBuffer, "%d sec", pItem->iPollingInterval);
   m_wndListCtrl.SetItemText(iItem, 5, szBuffer);
   sprintf(szBuffer, "%d days", pItem->iRetentionTime);
   m_wndListCtrl.SetItemText(iItem, 6, szBuffer);
   m_wndListCtrl.SetItemText(iItem, 7, g_pszItemStatus[pItem->iStatus]);
   m_wndListCtrl.SetItem(iItem, 0, LVIF_IMAGE, NULL, pItem->iStatus, 0, 0, 0);
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
         item.pThresholdList = (NXC_DCI_THRESHOLD *)nx_memdup(
                           m_pItemList->pItems[dwIndex].pThresholdList, 
                           sizeof(NXC_DCI_THRESHOLD) * item.dwNumThresholds);
         if (EditItem(&item))
         {
            dwResult = DoRequestArg2(NXCUpdateDCI, (void *)m_pItemList->dwNodeId, &item,
                                     "Updating data collection item...");
            if (dwResult == RCC_SUCCESS)
            {
               free(m_pItemList->pItems[dwIndex].pThresholdList);
               memcpy(&m_pItemList->pItems[dwIndex], &item, sizeof(NXC_DCI));
               UpdateListItem(iItem, &m_pItemList->pItems[dwIndex]);
            }
            else
            {
               free(item.pThresholdList);
               theApp.ErrorBox(dwResult, "Unable to update data collection item: %s");
            }
         }
         else
         {
            free(item.pThresholdList);
         }
      }
   }
}


//
// Start item editing dialog
//

BOOL CDataCollectionEditor::EditItem(NXC_DCI *pItem)
{   
   BOOL bSuccess = FALSE;
   CPropertySheet dlg;
   CDCIPropPage pgCollection;
   CDCITransformPage pgTransform;
   CDCIThresholdsPage pgThresholds;

   // Setup "Collection" page
   pgCollection.m_iDataType = pItem->iDataType;
   pgCollection.m_iOrigin = pItem->iSource;
   pgCollection.m_iPollingInterval = pItem->iPollingInterval;
   pgCollection.m_iRetentionTime = pItem->iRetentionTime;
   pgCollection.m_iStatus = pItem->iStatus;
   pgCollection.m_strName = pItem->szName;
   pgCollection.m_strDescription = pItem->szDescription;
   pgCollection.m_pNode = NXCFindObjectById(m_pItemList->dwNodeId);

   // Setup "Transformation" page
   pgTransform.m_iDeltaProc = pItem->iDeltaCalculation;
   pgTransform.m_strFormula = CHECK_NULL_EX(pItem->pszFormula);

   // Setup "Thresholds" page
   pgThresholds.m_pItem = pItem;
   pgThresholds.m_strInstance = pItem->szInstance;

   // Setup property sheet and run
   dlg.SetTitle("Data Collection Item");
   dlg.m_psh.dwFlags |= PSH_NOAPPLYNOW;
   dlg.AddPage(&pgCollection);
   dlg.AddPage(&pgTransform);
   dlg.AddPage(&pgThresholds);
   if (dlg.DoModal() == IDOK)
   {
      pItem->iDataType = pgCollection.m_iDataType;
      pItem->iSource = pgCollection.m_iOrigin;
      pItem->iPollingInterval = pgCollection.m_iPollingInterval;
      pItem->iRetentionTime = pgCollection.m_iRetentionTime;
      pItem->iStatus = pgCollection.m_iStatus;
      strcpy(pItem->szName, (LPCTSTR)pgCollection.m_strName);
      strcpy(pItem->szDescription, (LPCTSTR)pgCollection.m_strDescription);
      StrStrip(pItem->szDescription);
      if (pItem->szDescription[0] == 0)
         strcpy(pItem->szDescription, pItem->szName);
      pItem->iDeltaCalculation = pgTransform.m_iDeltaProc;
      safe_free(pItem->pszFormula);
      pItem->pszFormula = strdup((LPCTSTR)pgTransform.m_strFormula);
      strcpy(pItem->szInstance, (LPCTSTR)pgThresholds.m_strInstance);
      StrStrip(pItem->szInstance);
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

void CDataCollectionEditor::OnUpdateItemShowdata(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() > 0);
}

void CDataCollectionEditor::OnUpdateItemGraph(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() == 1);
}

void CDataCollectionEditor::OnUpdateItemCopy(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() > 0);
}

void CDataCollectionEditor::OnUpdateItemDuplicate(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() > 0);
}

void CDataCollectionEditor::OnUpdateItemActivate(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() > 0);
}

void CDataCollectionEditor::OnUpdateItemDisable(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() > 0);
}

void CDataCollectionEditor::OnUpdateFileExport(CCmdUI* pCmdUI) 
{
   NXC_OBJECT *pObject;

   pObject = NXCFindObjectById(m_pItemList->dwNodeId);
   if (pObject != NULL)
      pCmdUI->Enable(pObject->iClass == OBJECT_TEMPLATE);
   else
      pCmdUI->Enable(FALSE);
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


//
// Show collected data for specific item
//

void CDataCollectionEditor::OnItemShowdata() 
{
   int iItem;
   DWORD dwItemId, dwIndex;
   char szBuffer[384];
   NXC_OBJECT *pObject;

   iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
   while(iItem != -1)
   {
      dwItemId = m_wndListCtrl.GetItemData(iItem);
      dwIndex = NXCItemIndex(m_pItemList, dwItemId);
      pObject = NXCFindObjectById(m_pItemList->dwNodeId);
      sprintf(szBuffer, "%s - %s", pObject->szName, 
              m_pItemList->pItems[dwIndex].szDescription);
      theApp.ShowDCIData(m_pItemList->dwNodeId, dwItemId, szBuffer);
      iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
   }
}


//
// WM_COMMAND::ID_ITEM_GRAPH message handler
//

void CDataCollectionEditor::OnItemGraph() 
{
   int iItem;
   DWORD dwItemId, dwIndex;
   char szBuffer[384];
   NXC_OBJECT *pObject;

   iItem = m_wndListCtrl.GetNextItem(-1, LVNI_FOCUSED);
   if (iItem != -1)
   {
      dwItemId = m_wndListCtrl.GetItemData(iItem);
      dwIndex = NXCItemIndex(m_pItemList, dwItemId);
      pObject = NXCFindObjectById(m_pItemList->dwNodeId);
      sprintf(szBuffer, "%s - %s", pObject->szName, 
              m_pItemList->pItems[dwIndex].szDescription);
      theApp.ShowDCIGraph(m_pItemList->dwNodeId, dwItemId, szBuffer);
   }
}


//
// Overriden PreCreateWindow()
//

BOOL CDataCollectionEditor::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, NULL, NULL, 
                                         AfxGetApp()->LoadIcon(IDI_DATACOLLECT));
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// Copy DCIs to host(s)
//

static DWORD __cdecl CopyItems(DWORD dwSourceNode, DWORD dwNumObjects, DWORD *pdwObjectList,
                               DWORD dwNumItems, DWORD *pdwItemList)
{
   DWORD i, dwResult;

   for(i = 0; i < dwNumObjects; i++)
   {
      dwResult = NXCCopyDCI(dwSourceNode, pdwObjectList[i], dwNumItems, pdwItemList);
      if (dwResult != RCC_SUCCESS)
         break;
   }
   return dwResult;
}


//
// WM_COMMAND::ID_ITEM_COPY message handler
//

void CDataCollectionEditor::OnItemCopy() 
{
   CObjectSelDlg dlg;
   int iPos, iItem;
   DWORD dwResult, dwNumItems, *pdwItemList;

   dwNumItems = m_wndListCtrl.GetSelectedCount();
   if (dwNumItems > 0)
   {
      pdwItemList = (DWORD *)malloc(sizeof(DWORD) * dwNumItems);

      // Build list of items to be copied
      iPos = 0;
      iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
      while(iItem != -1)
      {
         pdwItemList[iPos++] = m_wndListCtrl.GetItemData(iItem);
         iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
      }

      // Ask for destination nodes
      dlg.m_dwAllowedClasses = SCL_NODE;
      if (dlg.DoModal() == IDOK)
      {
         // Perform request(s) to server
         dwResult = DoRequestArg5(CopyItems, (void *)m_pItemList->dwNodeId, 
               (void *)dlg.m_dwNumObjects, dlg.m_pdwObjectList, 
               (void *)dwNumItems, pdwItemList, "Copying items...");
         if (dwResult != RCC_SUCCESS)
            theApp.ErrorBox(dwResult, "Error copying items: %s");
      }

      // Cleanup
      free(pdwItemList);
   }
}


//
// WM_COMMAND::ID_ITEM_DUPLICATE message handler
//

void CDataCollectionEditor::OnItemDuplicate() 
{
   int iPos, iItem;
   DWORD dwResult, dwNumItems, *pdwItemList;

   dwNumItems = m_wndListCtrl.GetSelectedCount();
   if (dwNumItems > 0)
   {
      pdwItemList = (DWORD *)malloc(sizeof(DWORD) * dwNumItems);

      // Build list of items to be copied
      iPos = 0;
      iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
      while(iItem != -1)
      {
         pdwItemList[iPos++] = m_wndListCtrl.GetItemData(iItem);
         iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
      }

      // Perform request(s) to server
      dwResult = DoRequestArg4(NXCCopyDCI, (void *)m_pItemList->dwNodeId, 
               (void *)m_pItemList->dwNodeId, (void *)dwNumItems, 
               pdwItemList, "Copying items...");
      if (dwResult != RCC_SUCCESS)
         theApp.ErrorBox(dwResult, "Error copying items: %s");
      PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);

      // Cleanup
      free(pdwItemList);
   }
}


//
// WM_COMMAND::ID_FILE_EXPORT message handler
//

void CDataCollectionEditor::OnFileExport() 
{
   CFileDialog dlg(FALSE);

   dlg.m_ofn.lpstrDefExt = _T("dct");
   dlg.m_ofn.Flags |= OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
   dlg.m_ofn.lpstrFilter = _T("Data Collection Templates (*.dct)\0*.dct\0XML Files (*.xml)\0*.xml\0All Files (*.*)\0*.*\0");
   if (dlg.DoModal() == IDOK)
   {
      MessageBox(dlg.m_ofn.lpstrFile,"FILE");
   }
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CDataCollectionEditor::OnViewRefresh() 
{
   DWORD i, dwResult, dwObjectId;

   dwObjectId = m_pItemList->dwNodeId;

   // Re-open node's DCI list
   dwResult = DoRequestArg1(NXCCloseNodeDCIList, m_pItemList, "Unlocking node's data collection information...");
   if (dwResult != RCC_SUCCESS)
      theApp.ErrorBox(dwResult, "Unable to close data collection configuration:\n%s");

   dwResult = DoRequestArg2(NXCOpenNodeDCIList, (void *)dwObjectId, 
                            &m_pItemList, "Loading node's data collection information...");
   if (dwResult != RCC_SUCCESS)
      theApp.ErrorBox(dwResult, "Unable to load data collection information for node:\n%s");

   // Fill list view with data
   m_wndListCtrl.DeleteAllItems();
   for(i = 0; i < m_pItemList->dwNumItems; i++)
      AddListItem(&m_pItemList->pItems[i]);
}


//
// WM_COMMAND::ID_ITEM_DISABLE message handler
//

void CDataCollectionEditor::OnItemDisable() 
{
   ChangeItemsStatus(ITEM_STATUS_DISABLED);
}


//
// WM_COMMAND::ID_ITEM_ACTIVATE message handler
//

void CDataCollectionEditor::OnItemActivate() 
{
   ChangeItemsStatus(ITEM_STATUS_ACTIVE);
}


//
// Change status for one or more items at once
//

void CDataCollectionEditor::ChangeItemsStatus(int iStatus)
{
   int iItem;
   DWORD iPos, dwNumItems, *pdwItemList, dwResult;

   dwNumItems = m_wndListCtrl.GetSelectedCount();
   if (dwNumItems > 0)
   {
      pdwItemList = (DWORD *)malloc(sizeof(DWORD) * dwNumItems);

      // Build list of items to be copied
      iPos = 0;
      iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
      while(iItem != -1)
      {
         pdwItemList[iPos++] = m_wndListCtrl.GetItemData(iItem);
         iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
      }

      // Send request to server
      dwResult = DoRequestArg4(NXCSetDCIStatus, (void *)m_pItemList->dwNodeId,
                               (void *)dwNumItems, pdwItemList, (void *)iStatus,
                               _T("Updating DCI status..."));
      if (dwResult == RCC_SUCCESS)
      {
         DWORD dwIndex;

         iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
         while(iItem != -1)
         {
            dwIndex = NXCItemIndex(m_pItemList, m_wndListCtrl.GetItemData(iItem));
            m_pItemList->pItems[dwIndex].iStatus = iStatus;
            UpdateListItem(iItem, &m_pItemList->pItems[dwIndex]);
            iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
         }
      }
      else
      {
         theApp.ErrorBox(dwResult, _T("Error setting DCI status: %s"));
         PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
      }

      free(pdwItemList);
   }
}
