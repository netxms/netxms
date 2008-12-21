// ActionEditor.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ActionEditor.h"
#include "NewActionDlg.h"
#include "EditActionDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CActionEditor

IMPLEMENT_DYNCREATE(CActionEditor, CMDIChildWnd)

CActionEditor::CActionEditor()
{
   m_iSortMode = theApp.GetProfileInt(_T("ActionEditor"), _T("SortMode"), 0);
   m_iSortDir = theApp.GetProfileInt(_T("ActionEditor"), _T("SortDir"), 1);
}

CActionEditor::~CActionEditor()
{
   theApp.WriteProfileInt(_T("ActionEditor"), _T("SortMode"), m_iSortMode);
   theApp.WriteProfileInt(_T("ActionEditor"), _T("SortDir"), m_iSortDir);
}


BEGIN_MESSAGE_MAP(CActionEditor, CMDIChildWnd)
	//{{AFX_MSG_MAP(CActionEditor)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SETFOCUS()
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_WM_CONTEXTMENU()
	ON_UPDATE_COMMAND_UI(ID_ACTION_DELETE, OnUpdateActionDelete)
	ON_UPDATE_COMMAND_UI(ID_ACTION_PROPERTIES, OnUpdateActionProperties)
	ON_UPDATE_COMMAND_UI(ID_ACTION_RENAME, OnUpdateActionRename)
	ON_COMMAND(ID_ACTION_NEW, OnActionNew)
	ON_COMMAND(ID_ACTION_PROPERTIES, OnActionProperties)
	ON_COMMAND(ID_ACTION_DELETE, OnActionDelete)
	//}}AFX_MSG_MAP
   ON_NOTIFY(NM_DBLCLK, IDC_LIST_VIEW, OnListViewDoubleClick)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_VIEW, OnListViewColumnClick)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CActionEditor message handlers

BOOL CActionEditor::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         NULL, 
                                         GetSysColorBrush(COLOR_WINDOW), 
                                         AfxGetApp()->LoadIcon(IDI_EXEC));
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CActionEditor::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   theApp.OnViewCreate(VIEW_ACTION_EDITOR, this);
	
   // Create list view control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, rect, this, IDC_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT |
                                  LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);

   // Create image list
   m_imageList.Create(16, 16, ILC_COLOR8 | ILC_MASK, 8, 8);
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_EXEC));
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_REXEC));
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_EMAIL));
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_SMS));
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_FORWARD_EVENT));
   m_iSortImageBase = m_imageList.GetImageCount();
   m_imageList.Add(theApp.LoadIcon(IDI_SORT_UP));
   m_imageList.Add(theApp.LoadIcon(IDI_SORT_DOWN));
   m_wndListCtrl.SetImageList(&m_imageList, LVSIL_SMALL);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 100);
   m_wndListCtrl.InsertColumn(1, _T("Type"), LVCFMT_LEFT, 70);
   m_wndListCtrl.InsertColumn(2, _T("Recipient"), LVCFMT_LEFT, 150);
   m_wndListCtrl.InsertColumn(3, _T("Data"), LVCFMT_LEFT, 350);

	LoadListCtrlColumns(m_wndListCtrl, _T("ActionEditor"), _T("ListCtrl"));

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);

	return 0;
}


//
// WM_DESTROY message handler
//

void CActionEditor::OnDestroy() 
{
	SaveListCtrlColumns(m_wndListCtrl, _T("ActionEditor"), _T("ListCtrl"));
   theApp.OnViewDestroy(VIEW_ACTION_EDITOR, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_SETFOCUS message handler
//

void CActionEditor::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);

   m_wndListCtrl.SetFocus();
}


//
// WM_SIZE message handler
//

void CActionEditor::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
	
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_CLOSE message handler
//

void CActionEditor::OnClose() 
{
   DoRequestArg1(NXCUnlockActionDB, g_hSession, _T("Unlocking action configuration database..."));
	CMDIChildWnd::OnClose();
}


//
// Handler for double-clicks in list view
//

void CActionEditor::OnListViewDoubleClick(NMITEMACTIVATE *pInfo, LRESULT *pResult)
{
   PostMessage(WM_COMMAND, ID_ACTION_PROPERTIES, 0);
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CActionEditor::OnViewRefresh() 
{
   DWORD i;

   m_wndListCtrl.DeleteAllItems();
   LockActions();
   for(i = 0; i < g_dwNumActions; i++)
      AddItem(&g_pActionList[i]);
	SortList();
   UnlockActions();
}


//
// WM_CONTEXTMENU message handler
//

void CActionEditor::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   CMenu *pMenu;

   pMenu = theApp.GetContextMenu(7);
   pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this, NULL);
}


//
// Replace data in existing item
//

void CActionEditor::ReplaceItem(int iItem, NXC_ACTION *pAction)
{
   if (iItem != -1)
   {
      m_wndListCtrl.SetItem(iItem, 0, LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM, 
                            pAction->szName, pAction->iType, 0, 0, pAction->dwId);
      m_wndListCtrl.SetItemText(iItem, 1, g_szActionType[pAction->iType]);
      m_wndListCtrl.SetItemText(iItem, 2, pAction->szRcptAddr);
      m_wndListCtrl.SetItemText(iItem, 3, pAction->pszData);
   }

}


//
// Add new item to list
//

int CActionEditor::AddItem(NXC_ACTION *pAction)
{
   int iIndex;

   iIndex = m_wndListCtrl.InsertItem(0x7FFFFFFF, pAction->szName, pAction->iType);
   if (iIndex != -1)
   {
      m_wndListCtrl.SetItemData(iIndex, pAction->dwId);
      m_wndListCtrl.SetItemText(iIndex, 1, g_szActionType[pAction->iType]);
      m_wndListCtrl.SetItemText(iIndex, 2, pAction->szRcptAddr);
      m_wndListCtrl.SetItemText(iIndex, 3, pAction->pszData);
   }
   return iIndex;
}


//
// Update status for menu items
//

void CActionEditor::OnUpdateActionDelete(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() > 0);
}

void CActionEditor::OnUpdateActionProperties(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() == 1);
}

void CActionEditor::OnUpdateActionRename(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() == 1);
}


//
// WM_COMMAND::ID_ACTION_NEW message handler
//

void CActionEditor::OnActionNew() 
{
   CNewActionDlg dlg;
   DWORD i, dwActionId, dwResult;
   int iItem;

   if (dlg.DoModal() == IDOK)
   {
      dwResult = DoRequestArg3(NXCCreateAction, g_hSession, (void *)((LPCTSTR)dlg.m_strName), 
                               &dwActionId, _T("Creating new action..."));
      if (dwResult == RCC_SUCCESS)
      {
         LockActions();

         // Check if we already received an update from server
         for(i = 0; i < g_dwNumActions; i++)
            if (g_pActionList[i].dwId == dwActionId)
               break;   // Already have this action in list
         if (i == g_dwNumActions)
         {
            // Action still not in list, add it
            g_dwNumActions++;
            g_pActionList = (NXC_ACTION *)realloc(g_pActionList, sizeof(NXC_ACTION) * g_dwNumActions);
            memset(&g_pActionList[i], 0, sizeof(NXC_ACTION));
            g_pActionList[i].dwId = dwActionId;
            g_pActionList[i].iType = ACTION_EXEC;
            nx_strncpy(g_pActionList[i].szName, dlg.m_strName, MAX_OBJECT_NAME);
            g_pActionList[i].pszData = _tcsdup(_T(""));
         }

         iItem = AddItem(&g_pActionList[i]);
         SelectListViewItem(&m_wndListCtrl, iItem);
			SortList();
         PostMessage(WM_COMMAND, ID_ACTION_PROPERTIES, 0);

         UnlockActions();
      }
      else
      {
         theApp.ErrorBox(dwResult, _T("Error creating action: %s"));
      }
   }
}


//
// WM_COMMAND::ID_ACTION_PROPERTIES message handler
//

void CActionEditor::OnActionProperties() 
{
   int iItem;

   if (m_wndListCtrl.GetSelectedCount() == 1)
   {
      iItem = m_wndListCtrl.GetSelectionMark();
      if (iItem != -1)
      {
         NXC_ACTION *pAction;

         LockActions();

         pAction = FindActionById(m_wndListCtrl.GetItemData(iItem));
         if (pAction != NULL)
         {
            CEditActionDlg dlg;

            dlg.m_iType = pAction->iType;
            dlg.m_strData = pAction->pszData;
            dlg.m_strName = pAction->szName;
            dlg.m_strRcpt = pAction->szRcptAddr;
            dlg.m_strSubject = pAction->szEmailSubject;
            UnlockActions();

            if (dlg.DoModal() == IDOK)
            {
               NXC_ACTION action;
               DWORD dwResult;

               memset(&action, 0, sizeof(NXC_ACTION));
               action.dwId = m_wndListCtrl.GetItemData(iItem);
               action.iType = dlg.m_iType;
               action.pszData = _tcsdup((LPCTSTR)dlg.m_strData);
               nx_strncpy(action.szEmailSubject, (LPCTSTR)dlg.m_strSubject, MAX_EMAIL_SUBJECT_LEN);
               nx_strncpy(action.szName, (LPCTSTR)dlg.m_strName, MAX_OBJECT_NAME);
               nx_strncpy(action.szRcptAddr, (LPCTSTR)dlg.m_strRcpt, MAX_RCPT_ADDR_LEN);

               dwResult = DoRequestArg2(NXCModifyAction, g_hSession, &action,
                                        _T("Updating action configuration..."));
               if (dwResult == RCC_SUCCESS)
               {
                  ReplaceItem(iItem, &action);
						LockActions();
						SortList();
						UnlockActions();
               }
               else
               {
                  theApp.ErrorBox(dwResult, _T("Error updating action configuration: %s"));
               }
               free(action.pszData);
            }
         }
         else
         {
            UnlockActions();
            MessageBox(_T("Internal error: cannot find requested action entry in list"), _T("Internal Error"), MB_OK | MB_ICONSTOP);
         }
      }
   }
}


//
// Worker function for action deletion
//

static DWORD DeleteActions(DWORD dwNumActions, DWORD *pdwDeleteList, CListCtrl *pList)
{
   DWORD i, dwResult = RCC_SUCCESS;
   LVFINDINFO lvfi;
   int iItem;

   lvfi.flags = LVFI_PARAM;
   for(i = 0; i < dwNumActions; i++)
   {
      dwResult = NXCDeleteAction(g_hSession, pdwDeleteList[i]);
      if (dwResult == RCC_SUCCESS)
      {
         lvfi.lParam = pdwDeleteList[i];
         iItem = pList->FindItem(&lvfi);
         if (iItem != -1)
            pList->DeleteItem(iItem);
      }
      else
      {
         break;
      }
   }
   return dwResult;
}


//
// WM_COMMAND::ID_ACTION_DELETE message handler
//

void CActionEditor::OnActionDelete() 
{
   int iItem;
   DWORD i, dwNumItems, *pdwDeleteList, dwResult;

   dwNumItems = m_wndListCtrl.GetSelectedCount();
   if (dwNumItems > 0)
   {
      pdwDeleteList = (DWORD *)malloc(sizeof(DWORD) * dwNumItems);
      iItem = m_wndListCtrl.GetNextItem(-1, LVIS_SELECTED);
      for(i = 0; (i < dwNumItems) && (iItem != -1); i++)
      {
         pdwDeleteList[i] = m_wndListCtrl.GetItemData(iItem);
         iItem = m_wndListCtrl.GetNextItem(iItem, LVIS_SELECTED);
      }

      dwResult = DoRequestArg3(DeleteActions, (void *)dwNumItems, pdwDeleteList, 
                               &m_wndListCtrl, _T("Deleting action(s)..."));
      if (dwResult != RCC_SUCCESS)
         theApp.ErrorBox(dwResult, _T("Error deleting action: %s"));
      free(pdwDeleteList);
   }
}


//
// Action comparision procedure for sorting
//

static int CALLBACK ActionCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
   CActionEditor *pWnd = (CActionEditor *)lParamSort;
   NXC_ACTION *pAction1, *pAction2;
   int iResult;
   
   pAction1 = FindActionById(lParam1);
   pAction2 = FindActionById(lParam2);

	if ((pAction1 == NULL) || (pAction2 == NULL))
	{
		theApp.DebugPrintf(_T("WARNING: pAction == NULL in ActionCompareProc() !!!"));
		return 0;
	}
   
   switch(pWnd->GetSortMode())
   {
      case 0:     // Name
         iResult = _tcsicmp(pAction1->szName, pAction2->szName);
         break;
      case 1:     // Type
         iResult = _tcsicmp(g_szActionType[pAction1->iType], g_szActionType[pAction2->iType]);
         break;
      case 2:     // Recipient
         iResult = _tcsicmp(pAction1->szRcptAddr, pAction2->szRcptAddr);
         break;
      case 3:     // Data
         iResult = _tcsicmp(pAction1->pszData, pAction2->pszData);
         break;
      default:
         iResult = 0;
         break;
   }

   return iResult * pWnd->GetSortDir();
}


//
// Sort action list
//

void CActionEditor::SortList()
{
   LVCOLUMN lvc;

   m_wndListCtrl.SortItems(ActionCompareProc, (LPARAM)this);
   lvc.mask = LVCF_IMAGE | LVCF_FMT;
   lvc.fmt = LVCFMT_LEFT | LVCFMT_IMAGE | LVCFMT_BITMAP_ON_RIGHT;
   lvc.iImage = (m_iSortDir == 1) ? m_iSortImageBase : m_iSortImageBase + 1;
   m_wndListCtrl.SetColumn(m_iSortMode, &lvc);
}


//
// WM_NOTIFY::LVN_COLUMNCLICK message handler
//

void CActionEditor::OnListViewColumnClick(LPNMLISTVIEW pNMHDR, LRESULT *pResult)
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
      m_iSortDir = -m_iSortDir;
   }
   else
   {
      // Another sorting column
      m_iSortMode = pNMHDR->iSubItem;
   }

	LockActions();
   SortList();
	UnlockActions();
   
   *pResult = 0;
}
