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
}

CActionEditor::~CActionEditor()
{
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
	//}}AFX_MSG_MAP
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
	
   theApp.OnViewCreate(IDR_ACTION_EDITOR, this);
	
   // Create list view control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS, rect, this, IDC_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT | LVS_EX_FULLROWSELECT);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);

   // Create image list
   m_imageList.Create(16, 16, ILC_COLOR8 | ILC_MASK, 8, 8);
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_EXEC));
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_REXEC));
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_EMAIL));
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_SMS));
   m_wndListCtrl.SetImageList(&m_imageList, LVSIL_SMALL);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, "Name", LVCFMT_LEFT, 100);
   m_wndListCtrl.InsertColumn(1, "Type", LVCFMT_LEFT, 70);
   m_wndListCtrl.InsertColumn(2, "Recipient", LVCFMT_LEFT, 150);
   m_wndListCtrl.InsertColumn(3, "Data", LVCFMT_LEFT, 350);

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);

	return 0;
}


//
// WM_DESTROY message handler
//

void CActionEditor::OnDestroy() 
{
   theApp.OnViewDestroy(IDR_ACTION_EDITOR, this);
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
   DoRequest(NXCUnlockActionDB, "Unlocking action configuration database...");
	CMDIChildWnd::OnClose();
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
      dwResult = DoRequestArg2(NXCCreateAction, (void *)((LPCTSTR)dlg.m_strName), 
                               &dwActionId, "Creating new action...");
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
            strncpy(g_pActionList[i].szName, dlg.m_strName, MAX_OBJECT_NAME);
            g_pActionList[i].pszData = strdup("");
         }

         iItem = AddItem(&g_pActionList[i]);
         SelectListViewItem(&m_wndListCtrl, iItem);
         PostMessage(WM_COMMAND, ID_ACTION_PROPERTIES, 0);

         UnlockActions();
      }
      else
      {
         theApp.ErrorBox(dwResult, "Error creating action: %s");
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
               action.pszData = strdup((LPCTSTR)dlg.m_strData);
               strncpy(action.szEmailSubject, (LPCTSTR)dlg.m_strSubject, MAX_EMAIL_SUBJECT_LEN);
               strncpy(action.szName, (LPCTSTR)dlg.m_strName, MAX_OBJECT_NAME);
               strncpy(action.szRcptAddr, (LPCTSTR)dlg.m_strRcpt, MAX_RCPT_ADDR_LEN);

               dwResult = DoRequestArg1(NXCModifyAction, &action, "Updating action configuration...");
               if (dwResult == RCC_SUCCESS)
               {
                  ReplaceItem(iItem, &action);
               }
               else
               {
                  theApp.ErrorBox(dwResult, "Error updating action configuration: %s");
               }
               free(action.pszData);
            }
         }
         else
         {
            UnlockActions();
            MessageBox("Internal error: cannot find requested action entry in list", "Internal Error", MB_OK | MB_ICONSTOP);
         }
      }
   }
}
