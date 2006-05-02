// DesktopManager.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "DesktopManager.h"

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
	//}}AFX_MSG_MAP
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
   DWORD i, dwNumVars, dwResult;
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

static DWORD LoadDesktopInfo(CTreeCtrl *pTreeCtrl)
{
   NXC_USER *pUserList;
   DWORD i, dwNumUsers, dwResult;
   HTREEITEM hItem;

   hItem = pTreeCtrl->InsertItem(_T("[public]"), 2, 2, TVI_ROOT);
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
               dwResult = LoadUserDesktops(pUserList[i].dwId, pTreeCtrl, hItem);
               if (dwResult != RCC_SUCCESS)
                  break;
            }
      }
      pTreeCtrl->SortChildren(TVI_ROOT);
   }
   return dwResult;
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CDesktopManager::OnViewRefresh() 
{
   DWORD dwResult;

   m_wndTreeCtrl.DeleteAllItems();
   dwResult = DoRequestArg1(LoadDesktopInfo, &m_wndTreeCtrl,
                            _T("Loading saved desktop configurations..."));
}
