// DesktopManager.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "DesktopManager.h"
#include "UserSelectDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDesktopManager

IMPLEMENT_DYNCREATE(CDesktopManager, CMDIChildWnd)

CDesktopManager::CDesktopManager()
{
}

CDesktopManager::~CDesktopManager()
{
}


BEGIN_MESSAGE_MAP(CDesktopManager, CMDIChildWnd)
	//{{AFX_MSG_MAP(CDesktopManager)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SETFOCUS()
	ON_WM_SIZE()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_DESKTOP_DELETE, OnDesktopDelete)
	ON_UPDATE_COMMAND_UI(ID_DESKTOP_DELETE, OnUpdateDesktopDelete)
	ON_COMMAND(ID_DESKTOP_MOVE, OnDesktopMove)
	ON_UPDATE_COMMAND_UI(ID_DESKTOP_MOVE, OnUpdateDesktopMove)
	ON_COMMAND(ID_DESKTOP_COPY, OnDesktopCopy)
	ON_UPDATE_COMMAND_UI(ID_DESKTOP_COPY, OnUpdateDesktopCopy)
	//}}AFX_MSG_MAP
   ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_VIEW, OnTreeViewSelChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDesktopManager message handlers

BOOL CDesktopManager::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, NULL, NULL, 
                                         AfxGetApp()->LoadIcon(IDI_DESKTOP));
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CDesktopManager::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   theApp.OnViewCreate(VIEW_DESKTOP_MANAGER, this);
	
   // Create tree view control
   GetClientRect(&rect);
   m_wndTreeCtrl.Create(WS_CHILD | WS_VISIBLE | TVS_SHOWSELALWAYS |
                        TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS,
                        rect, this, IDC_TREE_VIEW);

   // Create image list
   m_imageList.Create(16, 16, ILC_COLOR24 | ILC_MASK, 4, 1);
   m_imageList.Add(theApp.LoadIcon(IDI_USER));
   m_imageList.Add(theApp.LoadIcon(IDI_USER_GROUP));
   m_imageList.Add(theApp.LoadIcon(IDI_EVERYONE));
   m_imageList.Add(theApp.LoadIcon(IDI_DESKTOP));
   m_wndTreeCtrl.SetImageList(&m_imageList, TVSIL_NORMAL);

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
	
	return 0;
}


//
// WM_DESTROY message handler
//

void CDesktopManager::OnDestroy() 
{
   theApp.OnViewDestroy(VIEW_DESKTOP_MANAGER, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_SETFOCUS message handler
//

void CDesktopManager::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
   m_wndTreeCtrl.SetFocus();
}


//
// WM_SIZE message handler
//

void CDesktopManager::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
   m_wndTreeCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// Load saved desktops for a single user
//

static DWORD LoadUserDesktops(DWORD dwUserId, CTreeCtrl *pTreeCtrl, HTREEITEM hRoot)
{
   UINT32 i, dwNumVars, dwResult;
   TCHAR **ppVarList, *ptr;
   HTREEITEM hItem;

   // Load list of existing desktop configurations
   dwResult = NXCEnumUserVariables(g_hSession, dwUserId,
                                   _T("/Win32Console/Desktop/*/WindowCount"),
                                   &dwNumVars, &ppVarList);
	if (dwResult == RCC_SUCCESS)
   {
      for(i = 0; i < dwNumVars; i++)
      {
         ptr = _tcschr(&ppVarList[i][22], _T('/'));
         if (ptr != NULL)
            *ptr = 0;
         hItem = pTreeCtrl->InsertItem(&ppVarList[i][22], 3, 3, hRoot);
         pTreeCtrl->SetItemData(hItem, dwUserId);
         free(ppVarList[i]);
      }
      safe_free(ppVarList);
      pTreeCtrl->SortChildren(hRoot);
   }
   return dwResult;
}


//
// Load all saved desktops
//

static DWORD LoadDesktopInfo(CTreeCtrl *pTreeCtrl, CDesktopManager *pMgr)
{
   NXC_USER *pUserList;
   UINT32 i, dwNumUsers, dwResult;
   HTREEITEM hItem;

   // Show all users only if currently logged in user
   // has appropriate rights, otherwiese show desktops
   // only for current user
   if (NXCGetCurrentSystemAccess(g_hSession) & SYSTEM_ACCESS_MANAGE_USERS)
   {
      hItem = pTreeCtrl->InsertItem(_T("[public]"), 2, 2, TVI_ROOT);
      pTreeCtrl->SetItemData(hItem, GROUP_EVERYONE);
      dwResult = LoadUserDesktops(GROUP_EVERYONE, pTreeCtrl, hItem);
      if (dwResult == RCC_SUCCESS)
      {
         if (NXCGetUserDB(g_hSession, &pUserList, &dwNumUsers))
         {
            for(i = 0; i < dwNumUsers; i++)
               if (((pUserList[i].dwId & GROUP_FLAG) == 0) &&
                   ((pUserList[i].wFlags & UF_DELETED) == 0))
               {
                  hItem = pTreeCtrl->InsertItem(pUserList[i].szName, 0, 0, TVI_ROOT);
                  pTreeCtrl->SetItemData(hItem, pUserList[i].dwId);
                  dwResult = LoadUserDesktops(pUserList[i].dwId, pTreeCtrl, hItem);
                  if (dwResult != RCC_SUCCESS)
                     break;
               }
         }
         pTreeCtrl->SortChildren(TVI_ROOT);
      }
      pMgr->m_bEnableCopy = TRUE;
   }
   else
   {
      pUserList = NXCFindUserById(g_hSession, NXCGetCurrentUserId(g_hSession));
      if (pUserList != NULL)
      {
         hItem = pTreeCtrl->InsertItem(pUserList->szName, 0, 0, TVI_ROOT);
         pTreeCtrl->SetItemData(hItem, pUserList->dwId);
         dwResult = LoadUserDesktops(pUserList->dwId, pTreeCtrl, hItem);
      }
      pMgr->m_bEnableCopy = FALSE;
   }
   pTreeCtrl->InvalidateRect(NULL, FALSE);
   return dwResult;
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CDesktopManager::OnViewRefresh() 
{
   DWORD dwResult;

   m_wndTreeCtrl.DeleteAllItems();
   dwResult = DoRequestArg2(LoadDesktopInfo, &m_wndTreeCtrl, this,
                            _T("Loading saved desktop configurations..."));
   if (dwResult != RCC_SUCCESS)
      theApp.ErrorBox(dwResult, _T("Cannot load desktop configurations: %s"));
}


//
// WM_CONTEXTMENU message handler
//

void CDesktopManager::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   CMenu *pMenu;
   HTREEITEM hItem;
   UINT uFlags = 0;
   CPoint pt;

   pt = point;
   m_wndTreeCtrl.ScreenToClient(&pt);

   hItem = m_wndTreeCtrl.HitTest(pt, &uFlags);
   if ((hItem != NULL) && (uFlags & TVHT_ONITEM))
      m_wndTreeCtrl.Select(hItem, TVGN_CARET);
   else
      m_wndTreeCtrl.Select(NULL, TVGN_CARET);

   pMenu = theApp.GetContextMenu(20);
   pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this, NULL);
}


//
// WM_NOTIFY::TVN_SELCHANGED message handler
//

void CDesktopManager::OnTreeViewSelChange(NMHDR *lpnmt, LRESULT *pResult)
{
   TVITEM item;

   item.hItem = ((LPNMTREEVIEW)lpnmt)->itemNew.hItem;
   item.pszText = m_szCurrConf;
   item.cchTextMax = MAX_DB_STRING;
   item.mask = TVIF_IMAGE | TVIF_PARAM | TVIF_TEXT;
   if (m_wndTreeCtrl.GetItem(&item))
   {
      if (item.iImage == 3)
      {
         m_dwCurrUser = (DWORD)item.lParam;
      }
      else
      {
         m_dwCurrUser = 0xFFFFFFFF;
      }
   }
   else
   {
      m_dwCurrUser = 0xFFFFFFFF;
   }
   *pResult = 0;
}


//
// WM_COMMAND::ID_DESKTOP_DELETE message handlers
//

void CDesktopManager::OnDesktopDelete() 
{
   DWORD dwResult;
   TCHAR szVar[MAX_DB_STRING];

   _sntprintf(szVar, MAX_DB_STRING, _T("/Win32Console/Desktop/%s/*"), m_szCurrConf);
   dwResult = DoRequestArg3(NXCDeleteUserVariable, g_hSession, (void *)m_dwCurrUser,
                            szVar, _T("Deleting desktop configuration..."));
   if (dwResult == RCC_SUCCESS)
   {
      m_wndTreeCtrl.DeleteItem(m_wndTreeCtrl.GetSelectedItem());
   }
   else
   {
      theApp.ErrorBox(dwResult, _T("Cannot delete desktop confguration: %s"));
   }
}

void CDesktopManager::OnUpdateDesktopDelete(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_dwCurrUser != 0xFFFFFFFF);
}


//
// WM_COMMAND::ID_DESKTOP_MOVE message handlers
//

void CDesktopManager::OnDesktopMove() 
{
   MoveOrCopyDesktop(TRUE);
}

void CDesktopManager::OnUpdateDesktopMove(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable((m_dwCurrUser != 0xFFFFFFFF) && m_bEnableCopy);
}


//
// WM_COMMAND::ID_DESKTOP_COPY message handlers
//

void CDesktopManager::OnDesktopCopy() 
{
   MoveOrCopyDesktop(FALSE);
}

void CDesktopManager::OnUpdateDesktopCopy(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable((m_dwCurrUser != 0xFFFFFFFF) && m_bEnableCopy);
}


//
// Move or copy desktop configuration
//

void CDesktopManager::MoveOrCopyDesktop(BOOL bMove)
{
   CUserSelectDlg dlg;
   DWORD dwResult;
   TCHAR szVar[MAX_DB_STRING];
   HTREEITEM hItem, hChildItem;

   dlg.m_bOnlyUsers = TRUE;
   dlg.m_bAddPublic = TRUE;
   dlg.m_bSingleSelection = TRUE;
   if (dlg.DoModal() == IDOK)
   {
      _sntprintf(szVar, MAX_DB_STRING, _T("/Win32Console/Desktop/%s/*"), m_szCurrConf);
      dwResult = DoRequestArg5(NXCCopyUserVariable, g_hSession, (void *)m_dwCurrUser,
                               (void *)dlg.m_pdwUserList[0], szVar, (void *)bMove,
                               bMove ? _T("Moving desktop configuration...") : _T("Copying desktop configuration..."));
      if (dwResult == RCC_SUCCESS)
      {
         // Find destination user's item
         hItem = m_wndTreeCtrl.GetChildItem(TVI_ROOT);
         while(hItem != NULL)
         {
            if (m_wndTreeCtrl.GetItemData(hItem) == dlg.m_pdwUserList[0])
               break;
            hItem = m_wndTreeCtrl.GetNextItem(hItem, TVGN_NEXT);
         }
         if (hItem != NULL)
         {
            if (FindTreeCtrlItem(m_wndTreeCtrl, hItem, m_szCurrConf) == NULL)
            {
               hChildItem = m_wndTreeCtrl.InsertItem(m_szCurrConf, 3, 3, hItem);
               m_wndTreeCtrl.SetItemData(hChildItem, dlg.m_pdwUserList[0]);
               m_wndTreeCtrl.SortChildren(hItem);
            }
         }

         if (bMove)
            m_wndTreeCtrl.DeleteItem(m_wndTreeCtrl.GetSelectedItem());
      }
      else
      {
         theApp.ErrorBox(dwResult, bMove ? _T("Cannot move desktop confguration: %s") : _T("Cannot copy desktop confguration: %s"));
      }
   }
}
