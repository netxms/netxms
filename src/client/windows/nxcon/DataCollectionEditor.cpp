// DataCollectionEditor.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "DataCollectionEditor.h"
#include "DCIPropPage.h"
#include "DCISchedulePage.h"
#include "DCIThresholdsPage.h"
#include "DCITransformPage.h"
#include "DCIPerfTabPage.h"
#include "DataExportDlg.h"
#include "ObjectSelDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Compare two list view items
//

static int CALLBACK ItemCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
   int iResult = 0;
   NXC_DCI *pItem1, *pItem2;
   NXC_OBJECT *pObject1, *pObject2;

   pItem1 = ((CDataCollectionEditor *)lParamSort)->GetItem((DWORD)lParam1);
   pItem2 = ((CDataCollectionEditor *)lParamSort)->GetItem((DWORD)lParam2);
   if ((pItem1 == NULL) || (pItem2 == NULL))
      return 0;   // Shouldn't happen

   switch(((CDataCollectionEditor *)lParamSort)->SortMode())
   {
      case 0:  // ID
         iResult = COMPARE_NUMBERS(lParam1, lParam2);
         break;
      case 1:  // Source
         iResult = _tcsicmp(g_pszItemOrigin[pItem1->iSource], g_pszItemOrigin[pItem2->iSource]);
         break;
      case 2:  // Description
         iResult = _tcsicmp(pItem1->szDescription, pItem2->szDescription);
         break;
      case 3:  // Parameter
         iResult = _tcsicmp(pItem1->szName, pItem2->szName);
         break;
      case 4:  // Data type
         iResult = _tcsicmp(g_pszItemDataType[pItem1->iDataType], g_pszItemDataType[pItem2->iDataType]);
         break;
      case 5:  // Polling interval
         iResult = COMPARE_NUMBERS(pItem1->iPollingInterval, pItem2->iPollingInterval);
         break;
      case 6:  // Retention time
         iResult = COMPARE_NUMBERS(pItem1->iRetentionTime, pItem2->iRetentionTime);
         break;
      case 7:  // Status
         iResult = COMPARE_NUMBERS(pItem1->iStatus, pItem2->iStatus);
         break;
      case 8:  // Template
         pObject1 = NXCFindObjectById(g_hSession, pItem1->dwTemplateId);
         pObject2 = NXCFindObjectById(g_hSession, pItem2->dwTemplateId);
         iResult = _tcsicmp((pObject1 != NULL) ? pObject1->szName : _T(""),
                            (pObject2 != NULL) ? pObject2->szName : _T(""));
         break;
      default:
         break;
   }
   return (((CDataCollectionEditor *)lParamSort)->SortDir() == 0) ? iResult : -iResult;
}


/////////////////////////////////////////////////////////////////////////////
// CDataCollectionEditor

IMPLEMENT_DYNCREATE(CDataCollectionEditor, CMDIChildWnd)

CDataCollectionEditor::CDataCollectionEditor()
{
   m_pItemList = NULL;
   m_bIsTemplate = FALSE;

   m_iSortMode = theApp.GetProfileInt(_T("DCEditor"), _T("SortMode"), 0);
   m_iSortDir = theApp.GetProfileInt(_T("DCEditor"), _T("SortDir"), 0);
}

CDataCollectionEditor::CDataCollectionEditor(NXC_DCI_LIST *pList)
{
   NXC_OBJECT *pObject;

   m_pItemList = pList;

   pObject = NXCFindObjectById(g_hSession, pList->dwNodeId);
   if (pObject != NULL)
      m_bIsTemplate = (pObject->iClass == OBJECT_TEMPLATE);
   else
      m_bIsTemplate = FALSE;

   m_iSortMode = theApp.GetProfileInt(_T("DCEditor"), _T("SortMode"), 0);
   m_iSortDir = theApp.GetProfileInt(_T("DCEditor"), _T("SortDir"), 1);
}

CDataCollectionEditor::~CDataCollectionEditor()
{
   theApp.WriteProfileInt(_T("DCEditor"), _T("SortMode"), m_iSortMode);
   theApp.WriteProfileInt(_T("DCEditor"), _T("SortDir"), m_iSortDir);
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
	ON_COMMAND(ID_ITEM_EXPORTDATA, OnItemExportdata)
	ON_UPDATE_COMMAND_UI(ID_ITEM_EXPORTDATA, OnUpdateItemExportdata)
	ON_UPDATE_COMMAND_UI(ID_ITEM_MOVETOTEMPLATE, OnUpdateItemMovetotemplate)
	ON_COMMAND(ID_ITEM_MOVETOTEMPLATE, OnItemMovetotemplate)
	ON_COMMAND(ID_ITEM_MOVE, OnItemMove)
	ON_UPDATE_COMMAND_UI(ID_ITEM_MOVE, OnUpdateItemMove)
	ON_COMMAND(ID_ITEM_CLEARDATA, OnItemCleardata)
	ON_UPDATE_COMMAND_UI(ID_ITEM_CLEARDATA, OnUpdateItemCleardata)
	//}}AFX_MSG_MAP
	ON_NOTIFY(NM_DBLCLK, ID_LIST_VIEW, OnListViewDblClk)
	ON_NOTIFY(LVN_COLUMNCLICK, ID_LIST_VIEW, OnListViewColumnClick)
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
   LVCOLUMN lvCol;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   // We should't create window without bound item list
   if (m_pItemList == NULL)
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
                        rect, this, ID_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT | 
                                  LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);
   m_wndListCtrl.SetImageList(&m_imageList, LVSIL_SMALL);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, _T("ID"), LVCFMT_LEFT, 55);
   m_wndListCtrl.InsertColumn(1, _T("Source"), LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(2, _T("Description"), LVCFMT_LEFT, 150);
   m_wndListCtrl.InsertColumn(3, _T("Parameter"), LVCFMT_LEFT, 150);
   m_wndListCtrl.InsertColumn(4, _T("Data type"), LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(5, _T("Polling Interval"), LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(6, _T("Retention Time"), LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(7, _T("Status"), LVCFMT_LEFT, 90);
   m_wndListCtrl.InsertColumn(8, _T("Template"), LVCFMT_LEFT, 120);

   // Fill list view with data
   for(i = 0; i < m_pItemList->dwNumItems; i++)
      AddListItem(&m_pItemList->pItems[i]);
   m_wndListCtrl.SortItems(ItemCompareProc, (LPARAM)this);

   // Mark sorting column
   lvCol.mask = LVCF_IMAGE | LVCF_FMT;
   lvCol.fmt = LVCFMT_BITMAP_ON_RIGHT | LVCFMT_IMAGE | LVCFMT_LEFT;
   lvCol.iImage = (m_iSortDir == 0)  ? m_iSortImageBase : (m_iSortImageBase + 1);
   m_wndListCtrl.SetColumn(m_iSortMode, &lvCol);

   theApp.OnViewCreate(OV_DC_EDITOR, this, m_pItemList->dwNodeId);

	return 0;
}


//
// WM_DESTROY message handler
//

void CDataCollectionEditor::OnDestroy() 
{
   theApp.OnViewDestroy(OV_DC_EDITOR, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_CLOSE message handler
//

void CDataCollectionEditor::OnClose() 
{
   DWORD dwResult;

   dwResult = DoRequestArg2(NXCCloseNodeDCIList, g_hSession, m_pItemList,
                            _T("Unlocking node's data collection information..."));
   if (dwResult != RCC_SUCCESS)
      theApp.ErrorBox(dwResult, _T("Unable to close data collection configuration:\n%s"));
	
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
   TCHAR szBuffer[32];

   _sntprintf_s(szBuffer, 32, _TRUNCATE, _T("%d"), pItem->dwId);
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
   TCHAR szBuffer[32];
   NXC_OBJECT *pObject;

   m_wndListCtrl.SetItemText(iItem, 1, g_pszItemOrigin[pItem->iSource]);
   m_wndListCtrl.SetItemText(iItem, 2, pItem->szDescription);
   m_wndListCtrl.SetItemText(iItem, 3, pItem->szName);
   m_wndListCtrl.SetItemText(iItem, 4, g_pszItemDataType[pItem->iDataType]);
   _sntprintf_s(szBuffer, 32, _TRUNCATE, _T("%d sec"), pItem->iPollingInterval);
   m_wndListCtrl.SetItemText(iItem, 5, szBuffer);
   _sntprintf_s(szBuffer, 32, _TRUNCATE, _T("%d days"), pItem->iRetentionTime);
   m_wndListCtrl.SetItemText(iItem, 6, szBuffer);
   m_wndListCtrl.SetItemText(iItem, 7, g_pszItemStatus[pItem->iStatus]);
   if (pItem->dwTemplateId != 0)
   {
      pObject = NXCFindObjectById(g_hSession, pItem->dwTemplateId);
      m_wndListCtrl.SetItemText(iItem, 8, (pObject != NULL) ? pObject->szName : _T("<unknown>"));
   }
   else
   {
      m_wndListCtrl.SetItemText(iItem, 8, _T(""));
   }
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
	item.nBaseUnits = DCI_BASEUNITS_OTHER;
	item.nMultiplier = 1;

   if (EditItem(&item))
   {
      dwResult = DoRequestArg3(NXCCreateNewDCI, g_hSession, m_pItemList, &dwItemId, 
                               _T("Creating new data collection item..."));
      if (dwResult == RCC_SUCCESS)
      {
         dwIndex = NXCItemIndex(m_pItemList, dwItemId);
         item.dwId = dwItemId;
         if (dwIndex != INVALID_INDEX)
         {
            dwResult = DoRequestArg3(NXCUpdateDCI, g_hSession, (void *)m_pItemList->dwNodeId,
                                     &item, _T("Updating data collection item..."));
            if (dwResult == RCC_SUCCESS)
            {
               memcpy(&m_pItemList->pItems[dwIndex], &item, sizeof(NXC_DCI));
               iItem = AddListItem(&item);
               SelectListItem(iItem);
               m_wndListCtrl.SortItems(ItemCompareProc, (LPARAM)this);
            }
            else
            {
               theApp.ErrorBox(dwResult, _T("Unable to update data collection item: %s"));
            }
         }
      }
      else
      {
         theApp.ErrorBox(dwResult, _T("Unable to create new data collection item: %s"));
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
            dwResult = DoRequestArg3(NXCUpdateDCI, g_hSession, (void *)m_pItemList->dwNodeId,
                                     &item, _T("Updating data collection item..."));
            if (dwResult == RCC_SUCCESS)
            {
               free(m_pItemList->pItems[dwIndex].pThresholdList);
               memcpy(&m_pItemList->pItems[dwIndex], &item, sizeof(NXC_DCI));
               UpdateListItem(iItem, &m_pItemList->pItems[dwIndex]);
            }
            else
            {
               free(item.pThresholdList);
               theApp.ErrorBox(dwResult, _T("Unable to update data collection item: %s"));
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
   CDCISchedulePage pgSchedule;
   CDCITransformPage pgTransform;
   CDCIThresholdsPage pgThresholds;
	CDCIPerfTabPage pgPerfTab;

   // Setup "Collection" page
   pgCollection.m_iDataType = pItem->iDataType;
   pgCollection.m_iOrigin = pItem->iSource;
   pgCollection.m_iPollingInterval = pItem->iPollingInterval;
   pgCollection.m_iRetentionTime = pItem->iRetentionTime;
   pgCollection.m_bAdvSchedule = (pItem->wFlags & DCF_ADVANCED_SCHEDULE) ? TRUE : FALSE;
   pgCollection.m_iStatus = pItem->iStatus;
   pgCollection.m_strName = pItem->szName;
   pgCollection.m_strDescription = pItem->szDescription;
   pgCollection.m_pNode = NXCFindObjectById(g_hSession, m_pItemList->dwNodeId);
	pgCollection.m_dwResourceId = pItem->dwResourceId;
	pgCollection.m_dwProxyNode = pItem->dwProxyNode;
	pgCollection.m_snmpPort = pItem->nSnmpPort;

   // Setup schedule page
   if (pItem->wFlags & DCF_ADVANCED_SCHEDULE)
   {
      pgSchedule.m_dwNumSchedules = pItem->dwNumSchedules;
      pgSchedule.m_ppScheduleList = CopyStringList(pItem->ppScheduleList, pItem->dwNumSchedules);
   }
   else
   {
      pgSchedule.m_dwNumSchedules = 0;
      pgSchedule.m_ppScheduleList = NULL;
   }

   // Setup "Transformation" page
   pgTransform.m_iDeltaProc = pItem->iDeltaCalculation;
   pgTransform.m_strFormula = CHECK_NULL_EX(pItem->pszFormula);
	pgTransform.m_dwNodeId = m_pItemList->dwNodeId;
	pgTransform.m_dwItemId = pItem->dwId;

   // Setup "Thresholds" page
   pgThresholds.m_pItem = pItem;
   pgThresholds.m_strInstance = pItem->szInstance;
   pgThresholds.m_bAllThresholds = (pItem->wFlags & DCF_ALL_THRESHOLDS) ? TRUE : FALSE;

	// Setup "Performance Tab" page
	pgPerfTab.m_strConfig = CHECK_NULL_EX(pItem->pszPerfTabSettings);
	pgPerfTab.m_dciDescription = pItem->szDescription;

   // Setup property sheet and run
   dlg.SetTitle(_T("Data Collection Item"));
   dlg.m_psh.dwFlags |= PSH_NOAPPLYNOW;
   dlg.AddPage(&pgCollection);
   dlg.AddPage(&pgSchedule);
   dlg.AddPage(&pgTransform);
   dlg.AddPage(&pgThresholds);
   dlg.AddPage(&pgPerfTab);
   if (dlg.DoModal() == IDOK)
   {
      pItem->iDataType = pgCollection.m_iDataType;
      pItem->iSource = pgCollection.m_iOrigin;
      pItem->iPollingInterval = pgCollection.m_iPollingInterval;
      pItem->iRetentionTime = pgCollection.m_iRetentionTime;
      pItem->iStatus = pgCollection.m_iStatus;
      nx_strncpy(pItem->szName, (LPCTSTR)pgCollection.m_strName, MAX_ITEM_NAME);
      nx_strncpy(pItem->szDescription, (LPCTSTR)pgCollection.m_strDescription, MAX_DB_STRING);
      StrStrip(pItem->szDescription);
      if (pItem->szDescription[0] == 0)
         nx_strncpy(pItem->szDescription, pItem->szName, MAX_DB_STRING);
      pItem->iDeltaCalculation = pgTransform.m_iDeltaProc;
      safe_free(pItem->pszFormula);
      pItem->pszFormula = _tcsdup((LPCTSTR)pgTransform.m_strFormula);
      nx_strncpy(pItem->szInstance, (LPCTSTR)pgThresholds.m_strInstance, MAX_DB_STRING);
      StrStrip(pItem->szInstance);
      DestroyStringList(pItem->ppScheduleList, pItem->dwNumSchedules);
		if (pgCollection.m_bAdvSchedule)
			pItem->wFlags |= DCF_ADVANCED_SCHEDULE;
		else
			pItem->wFlags &= ~DCF_ADVANCED_SCHEDULE;
		pItem->dwResourceId = pgCollection.m_dwResourceId;
		pItem->dwProxyNode = pgCollection.m_dwProxyNode;
		pItem->nSnmpPort = pgCollection.m_snmpPort;
      if (pItem->wFlags & DCF_ADVANCED_SCHEDULE)
      {
         pItem->dwNumSchedules = pgSchedule.m_dwNumSchedules;
         pItem->ppScheduleList = CopyStringList(pgSchedule.m_ppScheduleList, pItem->dwNumSchedules);
      }
      else
      {
         pItem->dwNumSchedules = 0;
         pItem->ppScheduleList = NULL;
      }
		if (pgThresholds.m_bAllThresholds)
			pItem->wFlags |= DCF_ALL_THRESHOLDS;
		else
			pItem->wFlags &= ~DCF_ALL_THRESHOLDS;
		safe_free(pItem->pszPerfTabSettings);
		pItem->pszPerfTabSettings = _tcsdup((LPCTSTR)pgPerfTab.m_strConfig);
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
   pCmdUI->Enable((m_wndListCtrl.GetSelectedCount() > 0) && (!m_bIsTemplate));
}

void CDataCollectionEditor::OnUpdateItemGraph(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable((m_wndListCtrl.GetSelectedCount() > 0) && (!m_bIsTemplate));
}

void CDataCollectionEditor::OnUpdateItemExportdata(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable((m_wndListCtrl.GetSelectedCount() == 1) && (!m_bIsTemplate));
}

void CDataCollectionEditor::OnUpdateItemCopy(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() > 0);
}

void CDataCollectionEditor::OnUpdateItemMove(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() > 0);
}

void CDataCollectionEditor::OnUpdateItemMovetotemplate(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(!m_bIsTemplate && (m_wndListCtrl.GetSelectedCount() > 0));
}

void CDataCollectionEditor::OnUpdateItemDuplicate(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() > 0);
}

void CDataCollectionEditor::OnUpdateItemActivate(CCmdUI* pCmdUI) 
{
   int iItem;
   DWORD dwIndex;
   BOOL bEnable = FALSE;

   if (m_wndListCtrl.GetSelectedCount() > 0)
   {
      iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
      while(iItem != -1)
      {
         dwIndex = NXCItemIndex(m_pItemList, m_wndListCtrl.GetItemData(iItem));
         if (m_pItemList->pItems[dwIndex].iStatus != ITEM_STATUS_ACTIVE)
         {
            bEnable = TRUE;
            break;
         }
         iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
      }
   }
   pCmdUI->Enable(bEnable);
}

void CDataCollectionEditor::OnUpdateItemDisable(CCmdUI* pCmdUI) 
{
   int iItem;
   DWORD dwIndex;
   BOOL bEnable = FALSE;

   if (m_wndListCtrl.GetSelectedCount() > 0)
   {
      iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
      while(iItem != -1)
      {
         dwIndex = NXCItemIndex(m_pItemList, m_wndListCtrl.GetItemData(iItem));
         if (m_pItemList->pItems[dwIndex].iStatus != ITEM_STATUS_DISABLED)
         {
            bEnable = TRUE;
            break;
         }
         iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
      }
   }
   pCmdUI->Enable(bEnable);
}

void CDataCollectionEditor::OnUpdateFileExport(CCmdUI* pCmdUI) 
{
   NXC_OBJECT *pObject;

   pObject = NXCFindObjectById(g_hSession, m_pItemList->dwNodeId);
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
      dwItemId = (DWORD)m_wndListCtrl.GetItemData(iItem);
      dwResult = DoRequestArg3(NXCDeleteDCI, g_hSession, m_pItemList, (void *)dwItemId,
                               _T("Deleting data collection item..."));
      if (dwResult != RCC_SUCCESS)
      {
         theApp.ErrorBox(dwResult, _T("Unable to delete data collection item: %s"));
         break;
      }
      m_wndListCtrl.DeleteItem(iItem);
      iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
   }
}


//
// Handler for WM_NOTIFY::NM_DBLCLK from IDC_LIST_VIEW
//

void CDataCollectionEditor::OnListViewDblClk(NMHDR *pNMHDR, LRESULT *pResult)
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
   TCHAR szBuffer[384];
   NXC_OBJECT *pObject;

   iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
   while(iItem != -1)
   {
      dwItemId = (DWORD)m_wndListCtrl.GetItemData(iItem);
      dwIndex = NXCItemIndex(m_pItemList, dwItemId);
      pObject = NXCFindObjectById(g_hSession, m_pItemList->dwNodeId);
      _sntprintf_s(szBuffer, 384, _TRUNCATE, _T("%s - %s"), pObject->szName, 
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
   DWORD i, dwNumItems, dwIndex;
   NXC_DCI **ppItemList;
   TCHAR szBuffer[384];
   NXC_OBJECT *pObject;

   pObject = NXCFindObjectById(g_hSession, m_pItemList->dwNodeId);
   if (pObject == NULL)    // Paranoid check
      return;

   dwNumItems = m_wndListCtrl.GetSelectedCount();
   ppItemList = (NXC_DCI **)malloc(sizeof(NXC_DCI *) * dwNumItems);

   iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
   for(i = 0; (iItem != -1) && (i < dwNumItems); i++)
   {
      dwIndex = NXCItemIndex(m_pItemList, (DWORD)m_wndListCtrl.GetItemData(iItem));
      if (dwIndex != INVALID_INDEX)
      {
         ppItemList[i] = &m_pItemList->pItems[dwIndex];
      }
      else
      {
         ppItemList[i] = NULL;
      }
      iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
   }

   if ((dwNumItems == 1) && (ppItemList[0] != NULL))
   {
      _sntprintf_s(szBuffer, 384, _TRUNCATE, _T("%s - %s"), pObject->szName,
                   ppItemList[0]->szDescription);
   }
   else
   {
      _tcscpy(szBuffer, pObject->szName);
   }

   theApp.ShowDCIGraph(m_pItemList->dwNodeId, dwNumItems, ppItemList, szBuffer);
   free(ppItemList);
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
      dwResult = DoRequestArg6(NXCCopyDCI, g_hSession, (void *)m_pItemList->dwNodeId, 
                               (void *)m_pItemList->dwNodeId, (void *)dwNumItems, 
                               pdwItemList, (void *)FALSE, _T("Copying items..."));
      if (dwResult != RCC_SUCCESS)
         theApp.ErrorBox(dwResult, _T("Error copying items: %s"));
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
      /* TODO: implement export */
      MessageBox(dlg.m_ofn.lpstrFile, _T("FILE"));
   }
}


//
// Refresh DCI list
//

void CDataCollectionEditor::RefreshItemList()
{
   DWORD i;

   m_wndListCtrl.DeleteAllItems();
   if (m_pItemList != NULL)
   {
      for(i = 0; i < m_pItemList->dwNumItems; i++)
         AddListItem(&m_pItemList->pItems[i]);
      m_wndListCtrl.SortItems(ItemCompareProc, (LPARAM)this);
   }
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CDataCollectionEditor::OnViewRefresh() 
{
   DWORD dwResult, dwObjectId;

   dwObjectId = m_pItemList->dwNodeId;

   // Re-open node's DCI list
   dwResult = DoRequestArg2(NXCCloseNodeDCIList, g_hSession, m_pItemList,
                            _T("Unlocking node's data collection information..."));
   if (dwResult != RCC_SUCCESS)
      theApp.ErrorBox(dwResult, _T("Unable to close data collection configuration:\n%s"));
   m_pItemList = NULL;

   dwResult = DoRequestArg3(NXCOpenNodeDCIList, g_hSession, (void *)dwObjectId, 
                            &m_pItemList, _T("Loading node's data collection information..."));
   if (dwResult != RCC_SUCCESS)
      theApp.ErrorBox(dwResult, _T("Unable to load data collection information for node:\n%s"));

   // Fill list view with data
   RefreshItemList();
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
      dwResult = DoRequestArg5(NXCSetDCIStatus, g_hSession, (void *)m_pItemList->dwNodeId,
                               (void *)dwNumItems, pdwItemList, (void *)iStatus,
                               _T("Updating DCI status..."));
      if (dwResult == RCC_SUCCESS)
      {
         DWORD dwIndex;

         iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
         while(iItem != -1)
         {
            dwIndex = NXCItemIndex(m_pItemList, (DWORD)m_wndListCtrl.GetItemData(iItem));
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


//
// WM_NOTIFY::LVN_COLUMNCLICK message handler
//

void CDataCollectionEditor::OnListViewColumnClick(NMHDR *pNMHDR, LRESULT *pResult)
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
      m_iSortDir = 1 - m_iSortDir;
   }
   else
   {
      // Another sorting column
      m_iSortMode = ((LPNMLISTVIEW)pNMHDR)->iSubItem;
   }
   m_wndListCtrl.SortItems(ItemCompareProc, (LPARAM)this);

   // Mark new sorting column
   lvCol.mask = LVCF_IMAGE | LVCF_FMT;
   lvCol.fmt = LVCFMT_BITMAP_ON_RIGHT | LVCFMT_IMAGE | LVCFMT_LEFT;
   lvCol.iImage = (m_iSortDir == 0)  ? m_iSortImageBase : (m_iSortImageBase + 1);
   m_wndListCtrl.SetColumn(((LPNMLISTVIEW)pNMHDR)->iSubItem, &lvCol);
   
   *pResult = 0;
}


//
// WM_COMMAND::ID_ITEM_EXPORTDATA message handler
//

void CDataCollectionEditor::OnItemExportdata() 
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
      theApp.ExportDCIData(m_pItemList->dwNodeId, dwItemId, dwTimeFrom, dwTimeTo,
                           dlg.m_iSeparator, dlg.m_iTimeStampFormat,
                           (LPCTSTR)dlg.m_strFileName);
   }
}


//
// Move DCIs to host(s)
//

DWORD CDataCollectionEditor::MoveItemsToTemplate(DWORD dwTemplate,
                                                 DWORD dwNumItems, DWORD *pdwItemList)
{
   DWORD i, dwResult, dwNode;
   NXC_OBJECT *pObject;
   BOOL bNeedApply = TRUE;

   dwNode = m_pItemList->dwNodeId;

   // Check if node already bound to template
   pObject = NXCFindObjectById(g_hSession, dwTemplate);
   if (pObject != NULL)
   {
      for(i = 0; i < pObject->dwNumChilds; i++)
         if (pObject->pdwChildList[i] == dwNode)
         {
            bNeedApply = FALSE;
            break;
         }
   }

   // Copy DCIs to template
   dwResult = NXCCopyDCI(g_hSession, dwNode, dwTemplate, dwNumItems, pdwItemList, TRUE);
   if (dwResult == RCC_SUCCESS)
   {
      // Close node's DCI list
      dwResult = NXCCloseNodeDCIList(g_hSession, m_pItemList);
      
      // Apply template to item if needed
      if ((dwResult == RCC_SUCCESS) && (bNeedApply))
      {
         dwResult = NXCApplyTemplate(g_hSession, dwTemplate, dwNode);
      }

      // Re-open DCI list
      if (dwResult == RCC_SUCCESS)
      {
         m_pItemList = NULL;
         Sleep(750);    // Allow template apply operation to complete
         dwResult = NXCOpenNodeDCIList(g_hSession, dwNode, &m_pItemList);
         if (dwResult == RCC_SUCCESS)
         {
            RefreshItemList();
         }
         else
         {
            m_wndListCtrl.DeleteAllItems();
         }
      }
   }
   return dwResult;
}


//
// Wrapper for CDataCollectionEditor::MoveItemsToTemplate
//

static DWORD __cdecl __MoveItemsToTemplate(CDataCollectionEditor *pCaller,
                                           DWORD dwTemplate, DWORD dwNumItems,
                                           DWORD *pdwItemList)
{
   return pCaller->MoveItemsToTemplate(dwTemplate, dwNumItems, pdwItemList);
}


//
// WM_COMMAND::ID_ITEM_MOVETOTEMPLATE message handler
//

void CDataCollectionEditor::OnItemMovetotemplate() 
{
   CObjectSelDlg dlg;
   int iPos, iItem;
   DWORD dwResult, dwNumItems, *pdwItemList;

   if (m_bIsTemplate)
      return;

   dwNumItems = m_wndListCtrl.GetSelectedCount();
   if (dwNumItems > 0)
   {
      pdwItemList = (DWORD *)malloc(sizeof(DWORD) * dwNumItems);

      // Build list of items to be moved
      iPos = 0;
      iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
      while(iItem != -1)
      {
         pdwItemList[iPos++] = m_wndListCtrl.GetItemData(iItem);
         iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
      }

      // Ask for destination nodes
      dlg.m_dwAllowedClasses = SCL_TEMPLATE;
      dlg.m_bSingleSelection = TRUE;
      if (dlg.DoModal() == IDOK)
      {
         // Perform request(s) to server
         dwResult = DoRequestArg4(__MoveItemsToTemplate, this, 
                                  (void *)dlg.m_pdwObjectList[0], 
                                  (void *)dwNumItems, pdwItemList, _T("Moving items..."));
         if (dwResult != RCC_SUCCESS)
            theApp.ErrorBox(dwResult, _T("Error moving items: %s"));
      }

      // Cleanup
      free(pdwItemList);
   }
}


//
// WM_COMMAND::ID_ITEM_COPY message handler
//

void CDataCollectionEditor::OnItemCopy() 
{
   CopyOrMoveItems(FALSE);
}


//
// WM_COMMAND::ID_ITEM_MOVE message handler
//

void CDataCollectionEditor::OnItemMove() 
{
   CopyOrMoveItems(TRUE);
}


//
// Copy DCIs to host(s)
//

static DWORD __cdecl CopyItems(DWORD dwSourceNode, DWORD dwNumObjects, DWORD *pdwObjectList,
                               DWORD dwNumItems, DWORD *pdwItemList, BOOL bMove)
{
   DWORD i, dwResult;

   for(i = 0; i < dwNumObjects; i++)
   {
      dwResult = NXCCopyDCI(g_hSession, dwSourceNode, pdwObjectList[i],
                            dwNumItems, pdwItemList, bMove);
      if (dwResult != RCC_SUCCESS)
         break;
   }
   return dwResult;
}


//
// Copy or move DCIs to another node or template
//

void CDataCollectionEditor::CopyOrMoveItems(BOOL bMove)
{
   CObjectSelDlg dlg;
   int iPos, iItem, *pnIdxList;
   DWORD i, dwResult, dwNumItems, *pdwItemList;

   dwNumItems = m_wndListCtrl.GetSelectedCount();
   if (dwNumItems > 0)
   {
      pdwItemList = (DWORD *)malloc(sizeof(DWORD) * dwNumItems);
      if (bMove)
         pnIdxList = (int *)malloc(sizeof(int) * dwNumItems);

      // Build list of items to be copied
      iPos = 0;
      iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
      while(iItem != -1)
      {
         if (bMove)
            pnIdxList[iPos] = iItem;
         pdwItemList[iPos++] = m_wndListCtrl.GetItemData(iItem);
         iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
      }

      // Ask for destination nodes
      dlg.m_dwAllowedClasses = SCL_NODE | SCL_CLUSTER | SCL_TEMPLATE;
      dlg.m_bSingleSelection = bMove;
      if (dlg.DoModal() == IDOK)
      {
         // Perform request(s) to server
         dwResult = DoRequestArg6(CopyItems, (void *)m_pItemList->dwNodeId, 
               (void *)dlg.m_dwNumObjects, dlg.m_pdwObjectList, 
               (void *)dwNumItems, pdwItemList, (void *)bMove,
               bMove ? _T("Moving items...") : _T("Copying items..."));
         if (dwResult == RCC_SUCCESS)
         {
            if (bMove)
               for(i = 0, iPos = dwNumItems - 1; i < dwNumItems; i++)
                  m_wndListCtrl.DeleteItem(pnIdxList[iPos--]);
         }
         else
         {
            theApp.ErrorBox(dwResult, bMove ? _T("Error moving items: %s") : _T("Error copying items: %s"));
         }
      }

      // Cleanup
      free(pdwItemList);
      if (bMove)
         free(pnIdxList);
   }
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

void CDataCollectionEditor::OnItemCleardata() 
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
      pdwItemList[i] = m_wndListCtrl.GetItemData(iItem);
      iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
   }

   dwResult = DoRequestArg3(ClearDCIData, (void *)m_pItemList->dwNodeId, (void *)dwItemCount, pdwItemList,
                            _T("Clearing DCI data..."));
   if (dwResult != RCC_SUCCESS)
   {
      theApp.ErrorBox(dwResult, _T("Unable to clear DCI data: %s"));
   }
	safe_free(pdwItemList);
}

void CDataCollectionEditor::OnUpdateItemCleardata(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() > 0);
}
