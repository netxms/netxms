// AlarmView.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "AlarmView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Compare two list view items
//

static int CALLBACK CompareListItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
   NXC_ALARM *pAlarm1, *pAlarm2;
   NXC_OBJECT *pObject1, *pObject2;
   TCHAR szName1[MAX_OBJECT_NAME], szName2[MAX_OBJECT_NAME];
   int iResult;

   pAlarm1 = ((CAlarmView *)lParamSort)->FindAlarmInList(lParam1);
   pAlarm2 = ((CAlarmView *)lParamSort)->FindAlarmInList(lParam2);
   if ((pAlarm1 == NULL) || (pAlarm2 == NULL))
      return 0;

   switch(((CAlarmView *)lParamSort)->SortMode())
   {
      case 0:  // Severity
         iResult = COMPARE_NUMBERS(pAlarm1->nCurrentSeverity, pAlarm2->nCurrentSeverity);
         break;
      case 1:  // State
         iResult = -COMPARE_NUMBERS(pAlarm1->nState, pAlarm2->nState);
         break;
      case 2:  // Source
         pObject1 = NXCFindObjectById(g_hSession, pAlarm1->dwSourceObject);
         pObject2 = NXCFindObjectById(g_hSession, pAlarm2->dwSourceObject);

         if (pObject1 == NULL)
            _tcscpy(szName1, _T("<unknown>"));
         else
            _tcscpy(szName1, pObject1->szName);
         
         if (pObject2 == NULL)
            _tcscpy(szName2, _T("<unknown>"));
         else
            _tcscpy(szName2, pObject2->szName);

         iResult = _tcsicmp(szName1, szName2);
         break;
      case 3:  // Message
         iResult = _tcsicmp(pAlarm1->szMessage, pAlarm2->szMessage);
         break;
      case 4:  // Repeat count
         iResult = COMPARE_NUMBERS(pAlarm1->dwRepeatCount, pAlarm2->dwRepeatCount);
         break;
      case 5:  // Creation time
         iResult = COMPARE_NUMBERS(pAlarm1->dwCreationTime, pAlarm2->dwCreationTime);
         break;
      case 6:  // Last change time
         iResult = COMPARE_NUMBERS(pAlarm1->dwLastChangeTime, pAlarm2->dwLastChangeTime);
         break;
      default:
         iResult = 0;
         break;
   }
   return (((CAlarmView *)lParamSort)->SortDir() == 0) ? iResult : -iResult;
}


/////////////////////////////////////////////////////////////////////////////
// CAlarmView

CAlarmView::CAlarmView()
{
   m_pImageList = NULL;
   m_iSortDir = theApp.GetProfileInt(_T("AlarmView"), _T("SortDir"), 1);
   m_iSortMode = theApp.GetProfileInt(_T("AlarmView"), _T("SortMode"), 0);
   m_pObject = NULL;
   m_pAlarmList = NULL;
   m_dwNumAlarms = 0;
}

CAlarmView::~CAlarmView()
{
   delete m_pImageList;
   safe_free(m_pAlarmList);
}


BEGIN_MESSAGE_MAP(CAlarmView, CWnd)
	//{{AFX_MSG_MAP(CAlarmView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_WM_CONTEXTMENU()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_ALARM_ACKNOWLEDGE, OnAlarmAcknowledge)
	ON_UPDATE_COMMAND_UI(ID_ALARM_ACKNOWLEDGE, OnUpdateAlarmAcknowledge)
	ON_COMMAND(ID_ALARM_DELETE, OnAlarmDelete)
	ON_UPDATE_COMMAND_UI(ID_ALARM_DELETE, OnUpdateAlarmDelete)
	ON_COMMAND(ID_ALARM_DETAILS, OnAlarmDetails)
	ON_UPDATE_COMMAND_UI(ID_ALARM_DETAILS, OnUpdateAlarmDetails)
	ON_COMMAND(ID_ALARM_LASTDCIVALUES, OnAlarmLastdcivalues)
	ON_UPDATE_COMMAND_UI(ID_ALARM_LASTDCIVALUES, OnUpdateAlarmLastdcivalues)
	ON_COMMAND(ID_ALARM_TERMINATE, OnAlarmTerminate)
	ON_UPDATE_COMMAND_UI(ID_ALARM_TERMINATE, OnUpdateAlarmTerminate)
	//}}AFX_MSG_MAP
	ON_NOTIFY(LVN_COLUMNCLICK, ID_LIST_VIEW, OnListViewColumnClick)
   ON_MESSAGE(NXCM_SET_OBJECT, OnSetObject)
   ON_COMMAND_RANGE(OBJTOOL_MENU_FIRST_ID, OBJTOOL_MENU_LAST_ID, OnObjectTool)
   ON_UPDATE_COMMAND_UI_RANGE(OBJTOOL_MENU_FIRST_ID, OBJTOOL_MENU_LAST_ID, OnUpdateObjectTool)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CAlarmView message handlers


//
// WM_CREATE message handler
//

int CAlarmView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;
   LVCOLUMN lvCol;

	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   // Create list view control
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHAREIMAGELISTS,
                        rect, this, ID_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_LABELTIP | LVS_EX_SUBITEMIMAGES |
                                  LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

   // Create image lists
   m_pImageList = CreateEventImageList();
   m_iSortImageBase = m_pImageList->GetImageCount();
   m_pImageList->Add(theApp.LoadIcon(IDI_SORT_UP));
   m_pImageList->Add(theApp.LoadIcon(IDI_SORT_DOWN));
   m_iStateImageBase = m_pImageList->GetImageCount();
   m_pImageList->Add(theApp.LoadIcon(IDI_TIPS));
   m_pImageList->Add(theApp.LoadIcon(IDI_ACK));
   m_pImageList->Add(theApp.LoadIcon(IDI_NACK));
   m_wndListCtrl.SetImageList(m_pImageList, LVSIL_SMALL);
	
   // Setup columns
   m_wndListCtrl.InsertColumn(0, _T("Severity"), LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(1, _T("State"), LVCFMT_LEFT, 95);
   m_wndListCtrl.InsertColumn(2, _T("Source"), LVCFMT_LEFT, 140);
   m_wndListCtrl.InsertColumn(3, _T("Message"), LVCFMT_LEFT, 400);
   m_wndListCtrl.InsertColumn(4, _T("Count"), LVCFMT_LEFT, 45);
   m_wndListCtrl.InsertColumn(5, _T("Created"), LVCFMT_LEFT, 135);
   m_wndListCtrl.InsertColumn(6, _T("Last Change"), LVCFMT_LEFT, 135);
   LoadListCtrlColumns(m_wndListCtrl, _T("ObjectBrowser"), _T("AlarmView"));

   // Mark sorting column in list control
   lvCol.mask = LVCF_IMAGE | LVCF_FMT;
   lvCol.fmt = LVCFMT_BITMAP_ON_RIGHT | LVCFMT_IMAGE | LVCFMT_LEFT;
   lvCol.iImage = m_iSortImageBase + m_iSortDir;
   m_wndListCtrl.SetColumn(m_iSortMode, &lvCol);

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);

   return 0;
}


//
// WM_SIZE message handler
//

void CAlarmView::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_SETFOCUS message handler
//

void CAlarmView::OnSetFocus(CWnd* pOldWnd) 
{
	CWnd::OnSetFocus(pOldWnd);
   m_wndListCtrl.SetFocus();
}


//
// Add new alarm record to list control
//

void CAlarmView::AddAlarm(NXC_ALARM *pAlarm)
{
   int iIdx;

   iIdx = m_wndListCtrl.InsertItem(0x7FFFFFFF, g_szStatusTextSmall[pAlarm->nCurrentSeverity],
                                   pAlarm->nCurrentSeverity);
   if (iIdx != -1)
   {
      m_wndListCtrl.SetItemData(iIdx, pAlarm->dwAlarmId);
      UpdateListItem(iIdx, pAlarm);
   }
}


//
// Find alarm record in list control by alarm id
// Will return record index or -1 if no records exist
//

int CAlarmView::FindAlarmListItem(DWORD dwAlarmId)
{
   LVFINDINFO lvfi;

   lvfi.flags = LVFI_PARAM;
   lvfi.lParam = dwAlarmId;
   return m_wndListCtrl.FindItem(&lvfi);
}


//
// Add new alarm to internal list
//

void CAlarmView::AddAlarmToList(NXC_ALARM *pAlarm)
{
   m_pAlarmList = (NXC_ALARM *)realloc(m_pAlarmList, sizeof(NXC_ALARM) * (m_dwNumAlarms + 1));
   memcpy(&m_pAlarmList[m_dwNumAlarms], pAlarm, sizeof(NXC_ALARM));
   m_dwNumAlarms++;
   //m_iNumAlarms[pAlarm->nCurrentSeverity]++;
}


//
// Find alarm record in internal list
//

NXC_ALARM *CAlarmView::FindAlarmInList(DWORD dwAlarmId)
{
   DWORD i;

   for(i = 0; i < m_dwNumAlarms; i++)
      if (m_pAlarmList[i].dwAlarmId == dwAlarmId)
         return &m_pAlarmList[i];
   return NULL;
}


//
// Delete alarm from internal alarms list
//

void CAlarmView::DeleteAlarmFromList(DWORD dwAlarmId)
{
   DWORD i;

   for(i = 0; i < m_dwNumAlarms; i++)
      if (m_pAlarmList[i].dwAlarmId == dwAlarmId)
      {
         m_dwNumAlarms--;
         memmove(&m_pAlarmList[i], &m_pAlarmList[i + 1],
                 sizeof(NXC_ALARM) * (m_dwNumAlarms - i));
         break;
      }
}


//
// Update existing list entry with new data
//

void CAlarmView::UpdateListItem(int nItem, NXC_ALARM *pAlarm)
{
   struct tm *ptm;
   TCHAR szBuffer[64];
   NXC_OBJECT *pObject;
   LVITEM lvi;

   pObject = NXCFindObjectById(g_hSession, pAlarm->dwSourceObject);

   // Set images
   lvi.mask = LVIF_IMAGE;
   lvi.iItem = nItem;

   lvi.iSubItem = 0;
   lvi.iImage = pAlarm->nCurrentSeverity;
   m_wndListCtrl.SetItem(&lvi);

   lvi.iSubItem = 1;
   lvi.iImage = m_iStateImageBase + pAlarm->nState;
   m_wndListCtrl.SetItem(&lvi);

   // Set texts
   m_wndListCtrl.SetItemText(nItem, 0, g_szStatusTextSmall[pAlarm->nCurrentSeverity]);
   m_wndListCtrl.SetItemText(nItem, 1, g_szAlarmState[pAlarm->nState]);
   m_wndListCtrl.SetItemText(nItem, 2, pObject->szName);
   m_wndListCtrl.SetItemText(nItem, 3, pAlarm->szMessage);

   _stprintf(szBuffer, _T("%d"), pAlarm->dwRepeatCount);
   m_wndListCtrl.SetItemText(nItem, 4, szBuffer);

   ptm = localtime((const time_t *)&pAlarm->dwCreationTime);
   _tcsftime(szBuffer, 32, _T("%d-%b-%Y %H:%M:%S"), ptm);
   m_wndListCtrl.SetItemText(nItem, 5, szBuffer);
   
   ptm = localtime((const time_t *)&pAlarm->dwLastChangeTime);
   _tcsftime(szBuffer, 32, _T("%d-%b-%Y %H:%M:%S"), ptm);
   m_wndListCtrl.SetItemText(nItem, 6, szBuffer);
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CAlarmView::OnViewRefresh() 
{
   NXC_ALARM *pList;
   DWORD i, dwNumAlarms;

   m_wndListCtrl.DeleteAllItems();
   safe_free(m_pAlarmList);

   dwNumAlarms = theApp.OpenAlarmList(&pList);
   m_pAlarmList = (NXC_ALARM *)malloc(sizeof(NXC_ALARM) * dwNumAlarms);
   for(i = 0, m_dwNumAlarms = 0; i < dwNumAlarms; i++)
   {
      if (MatchAlarm(&pList[i]))
      {
         AddAlarm(&pList[i]);
         memcpy(&m_pAlarmList[m_dwNumAlarms++], &pList[i], sizeof(NXC_ALARM));
      }
   }
   theApp.CloseAlarmList();

   m_wndListCtrl.SortItems(CompareListItems, (LPARAM)this);
}


//
// Check if alarm matched to current filter settings
//

BOOL CAlarmView::MatchAlarm(NXC_ALARM *pAlarm)
{
   if (m_pObject == NULL)
      return FALSE;
   if (m_pObject->dwId == pAlarm->dwSourceObject)
      return TRUE;
   return NXCIsParent(g_hSession, m_pObject->dwId, pAlarm->dwSourceObject);
}


//
// NXCM_SET_OBJECT message handler
//

void CAlarmView::OnSetObject(WPARAM wParam, NXC_OBJECT *pObject)
{
   m_pObject = pObject;
   SendMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
}


//
// WM_CONTEXTMENU message handler
//

void CAlarmView::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   int iItem;
   UINT uFlags;
   CMenu *pMenu, *pToolsMenu;
   CPoint pt;

   pt = point;
   pWnd->ScreenToClient(&pt);
   iItem = m_wndListCtrl.HitTest(pt, &uFlags);
   if ((iItem != -1) && (uFlags & LVHT_ONITEM))
   {
      BOOL bMenuInserted = FALSE;
      DWORD dwTemp;
      NXC_ALARM *pAlarm;
      NXC_OBJECT *pObject;

      pAlarm = FindAlarmInList(m_wndListCtrl.GetItemData(iItem));
      pMenu = theApp.GetContextMenu(6);
      if ((pAlarm != NULL) && (m_wndListCtrl.GetSelectedCount() == 1))
      {
         pObject = NXCFindObjectById(g_hSession, pAlarm->dwSourceObject);
         if (pObject != NULL)
         {
            dwTemp = 0;
            pToolsMenu = CreateToolsSubmenu(pObject, _T(""), &dwTemp);
            if (pToolsMenu->GetMenuItemCount() > 0)
            {
               pMenu->InsertMenu(5, MF_BYPOSITION | MF_STRING | MF_POPUP,
                                 (UINT)pToolsMenu->GetSafeHmenu(), _T("&Object tools"));
               pToolsMenu->Detach();
               bMenuInserted = TRUE;
            }
            delete pToolsMenu;
         }
      }
      pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, GetParentFrame(), NULL);
      if (bMenuInserted)
      {
         pMenu->DeleteMenu(5, MF_BYPOSITION);
      }
   }
}


//
// WM_NOTIFY::LVN_COLUMNCLICK message handler
//

void CAlarmView::OnListViewColumnClick(LPNMLISTVIEW pNMHDR, LRESULT *pResult)
{
   LVCOLUMN lvCol;

   // Unmark old sorting column
   lvCol.mask = LVCF_FMT;
   lvCol.fmt = LVCFMT_LEFT;
   m_wndListCtrl.SetColumn(m_iSortMode, &lvCol);

   // Change current sort mode and resort list
   if (m_iSortMode == pNMHDR->iSubItem)
   {
      // Same column, change sort direction
      m_iSortDir = 1 - m_iSortDir;
   }
   else
   {
      // Another sorting column
      m_iSortMode = pNMHDR->iSubItem;
   }
   m_wndListCtrl.SortItems(CompareListItems, (LPARAM)this);

   // Mark new sorting column
   lvCol.mask = LVCF_IMAGE | LVCF_FMT;
   lvCol.fmt = LVCFMT_BITMAP_ON_RIGHT | LVCFMT_IMAGE | LVCFMT_LEFT;
   lvCol.iImage = (m_iSortDir == 0)  ? m_iSortImageBase : (m_iSortImageBase + 1);
   m_wndListCtrl.SetColumn(pNMHDR->iSubItem, &lvCol);
   
   *pResult = 0;
}


//
// Alarm acknowledgement worker function
//

static DWORD AcknowledgeAlarms(DWORD dwNumAlarms, DWORD *pdwAlarmList)
{
   DWORD i, dwResult = RCC_SUCCESS;

   for(i = 0; (i < dwNumAlarms) && (dwResult == RCC_SUCCESS); i++)
   {
      dwResult = NXCAcknowledgeAlarm(g_hSession, pdwAlarmList[i]);
      if (dwResult == RCC_ALARM_NOT_OUTSTANDING)
         dwResult = RCC_SUCCESS;
   }
   return dwResult;
}


//
// Handler for "Acknowledge" menu
//

void CAlarmView::OnAlarmAcknowledge() 
{
   int iItem;
   DWORD i, dwNumAlarms, *pdwAlarmList, dwResult;

   dwNumAlarms = m_wndListCtrl.GetSelectedCount();
   pdwAlarmList = (DWORD *)malloc(sizeof(DWORD) * dwNumAlarms);

   iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
   for(i = 0; (iItem != -1) && (i < dwNumAlarms); i++)
   {
      pdwAlarmList[i] = m_wndListCtrl.GetItemData(iItem);
      iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
   }

   dwResult = DoRequestArg2(AcknowledgeAlarms, (void *)dwNumAlarms, pdwAlarmList,
                            _T("Acknowledging alarm..."));
   if (dwResult != RCC_SUCCESS)
      theApp.ErrorBox(dwResult, _T("Cannot acknowledge alarm: %s"));
   free(pdwAlarmList);
}

void CAlarmView::OnUpdateAlarmAcknowledge(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() > 0);
}


//
// Alarm deletion worker function
//

static DWORD DeleteAlarms(DWORD dwNumAlarms, DWORD *pdwAlarmList)
{
   DWORD i, dwResult = RCC_SUCCESS;

   for(i = 0; (i < dwNumAlarms) && (dwResult == RCC_SUCCESS); i++)
   {
      dwResult = NXCDeleteAlarm(g_hSession, pdwAlarmList[i]);
      if (dwResult == RCC_ALARM_NOT_OUTSTANDING)
         dwResult = RCC_SUCCESS;
   }
   return dwResult;
}


//
// Handler for "Delete" menu
//

void CAlarmView::OnAlarmDelete() 
{
   int iItem;
   DWORD i, dwNumAlarms, *pdwAlarmList, dwResult;

   dwNumAlarms = m_wndListCtrl.GetSelectedCount();
   pdwAlarmList = (DWORD *)malloc(sizeof(DWORD) * dwNumAlarms);

   iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
   for(i = 0; (iItem != -1) && (i < dwNumAlarms); i++)
   {
      pdwAlarmList[i] = m_wndListCtrl.GetItemData(iItem);
      iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
   }

   dwResult = DoRequestArg2(DeleteAlarms, (void *)dwNumAlarms, pdwAlarmList,
                            _T("Deleting alarm..."));
   if (dwResult != RCC_SUCCESS)
      theApp.ErrorBox(dwResult, _T("Cannot delete alarm: %s"));
   free(pdwAlarmList);
}

void CAlarmView::OnUpdateAlarmDelete(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() > 0);
}


//
// Handler for "Details" menu
//

void CAlarmView::OnAlarmDetails() 
{
	// TODO: Add your command handler code here
	
}

void CAlarmView::OnUpdateAlarmDetails(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(FALSE);
}


//
// Handler for "Last DCI values" menu
//

void CAlarmView::OnAlarmLastdcivalues() 
{
   int nItem;
   NXC_ALARM *pAlarm;
   NXC_OBJECT *pObject;

   nItem = m_wndListCtrl.GetSelectionMark();
   if (nItem != -1)
   {
      pAlarm = FindAlarmInList(m_wndListCtrl.GetItemData(nItem));
      if (pAlarm != NULL)
      {
         pObject = NXCFindObjectById(g_hSession, pAlarm->dwSourceObject);
         if (pObject != NULL)
         {
            theApp.ShowLastValues(pObject);
         }
      }
   }
}

void CAlarmView::OnUpdateAlarmLastdcivalues(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() == 1);
}


//
// Alarm termination worker function
//

static DWORD TerminateAlarms(DWORD dwNumAlarms, DWORD *pdwAlarmList)
{
   DWORD i, dwResult = RCC_SUCCESS;

   for(i = 0; (i < dwNumAlarms) && (dwResult == RCC_SUCCESS); i++)
      dwResult = NXCTerminateAlarm(g_hSession, pdwAlarmList[i]);
   return dwResult;
}


//
// Handler for "Terminate" menu
//

void CAlarmView::OnAlarmTerminate() 
{
   int iItem;
   DWORD i, dwNumAlarms, *pdwAlarmList, dwResult;

   dwNumAlarms = m_wndListCtrl.GetSelectedCount();
   pdwAlarmList = (DWORD *)malloc(sizeof(DWORD) * dwNumAlarms);

   iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
   for(i = 0; (iItem != -1) && (i < dwNumAlarms); i++)
   {
      pdwAlarmList[i] = m_wndListCtrl.GetItemData(iItem);
      iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
   }

   dwResult = DoRequestArg2(TerminateAlarms, (void *)dwNumAlarms, pdwAlarmList,
                            _T("Terminating alarm..."));
   if (dwResult != RCC_SUCCESS)
      theApp.ErrorBox(dwResult, _T("Cannot terminate alarm: %s"));
   free(pdwAlarmList);
}

void CAlarmView::OnUpdateAlarmTerminate(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() > 0);
}


//
// Handler for object tools
//

void CAlarmView::OnObjectTool(UINT nID)
{
   int nItem;
   NXC_ALARM *pAlarm;
   NXC_OBJECT *pObject;

   nItem = m_wndListCtrl.GetNextItem(-1, LVIS_FOCUSED);
   if (nItem != -1)
   {
      pAlarm = FindAlarmInList(m_wndListCtrl.GetItemData(nItem));
      if (pAlarm != NULL)
      {
         pObject = NXCFindObjectById(g_hSession, pAlarm->dwSourceObject);
         if (pObject != NULL)
         {
            theApp.ExecuteObjectTool(pObject, nID - OBJTOOL_MENU_FIRST_ID);
         }
      }
   }
}

void CAlarmView::OnUpdateObjectTool(CCmdUI *pCmdUI)
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() == 1);
}


//
// Handler for alarm updates
//

void CAlarmView::OnAlarmUpdate(DWORD dwCode, NXC_ALARM *pAlarm)
{
   int iItem;
   NXC_ALARM *pListItem;

   iItem = FindAlarmListItem(pAlarm->dwAlarmId);
   switch(dwCode)
   {
      case NX_NOTIFY_NEW_ALARM:
         if ((iItem == -1) && (pAlarm->nState != ALARM_STATE_TERMINATED))
         {
            if (MatchAlarm(pAlarm))
            {
               AddAlarm(pAlarm);
               AddAlarmToList(pAlarm);
               m_wndListCtrl.SortItems(CompareListItems, (LPARAM)this);
            }
         }
         break;
      case NX_NOTIFY_ALARM_CHANGED:
         pListItem = FindAlarmInList(pAlarm->dwAlarmId);
         if (pListItem != NULL)
         {
//            m_iNumAlarms[pListItem->nCurrentSeverity]--;
//            m_iNumAlarms[pAlarm->nCurrentSeverity]++;
            memcpy(pListItem, pAlarm, sizeof(NXC_ALARM));
            if (iItem != -1)
            {
               UpdateListItem(iItem, pAlarm);
            }
            else
            {
               if (MatchAlarm(pAlarm))
                  AddAlarm(pAlarm);
            }
            m_wndListCtrl.SortItems(CompareListItems, (LPARAM)this);
         }
         else if (MatchAlarm(pAlarm))
         {
            AddAlarm(pAlarm);
            AddAlarmToList(pAlarm);
            m_wndListCtrl.SortItems(CompareListItems, (LPARAM)this);
         }
         break;
      case NX_NOTIFY_ALARM_DELETED:
      case NX_NOTIFY_ALARM_TERMINATED:
         if (iItem != -1)
         {
            DeleteAlarmFromList(pAlarm->dwAlarmId);
            m_wndListCtrl.DeleteItem(iItem);
//            m_iNumAlarms[pAlarm->nCurrentSeverity]--;
         }
         break;
      default:
         break;
   }
}
