// ScriptManager.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ScriptManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CScriptManager

IMPLEMENT_DYNCREATE(CScriptManager, CMDIChildWnd)

CScriptManager::CScriptManager()
{
}

CScriptManager::~CScriptManager()
{
}


BEGIN_MESSAGE_MAP(CScriptManager, CMDIChildWnd)
	//{{AFX_MSG_MAP(CScriptManager)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	//}}AFX_MSG_MAP
   ON_NOTIFY(TVN_SELCHANGING, AFX_IDW_PANE_FIRST, OnTreeViewSelChanging)
   ON_NOTIFY(TVN_SELCHANGED, AFX_IDW_PANE_FIRST, OnTreeViewSelChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScriptManager message handlers

BOOL CScriptManager::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         NULL, 
                                         GetSysColorBrush(COLOR_WINDOW), 
                                         AfxGetApp()->LoadIcon(IDI_SCRIPT_LIBRARY));
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CScriptManager::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect = { 0, 0, 100, 100 };

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   theApp.OnViewCreate(IDR_SCRIPT_MANAGER, this);

   // Create image list
   m_imageList.Create(16, 16, ILC_COLOR8 | ILC_MASK, 4, 4);
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_SCRIPT_LIBRARY));
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_CLOSED_FOLDER));
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_OPEN_FOLDER));
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_SCRIPT));

   // Create splitter
   m_wndSplitter.CreateStatic(this, 1, 2, WS_CHILD | WS_VISIBLE, IDC_SPLITTER);

   // Create tree view control
   m_wndTreeCtrl.Create(WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
                        rect, &m_wndSplitter, m_wndSplitter.IdFromRowCol(0, 0));
   m_wndTreeCtrl.SetImageList(&m_imageList, TVSIL_NORMAL);

   // Create script view
   m_wndScriptView.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, rect,
                          &m_wndSplitter, m_wndSplitter.IdFromRowCol(0, 1));
	
   // Finish splitter setup
   m_wndSplitter.SetColumnInfo(0, 150, 50);
   m_wndSplitter.InitComplete();
   m_wndSplitter.RecalcLayout();

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);

	return 0;
}


//
// WM_DESTROY message handler
//

void CScriptManager::OnDestroy() 
{
   theApp.OnViewDestroy(IDR_SCRIPT_MANAGER, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_SIZE message handler
//

void CScriptManager::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
   m_wndSplitter.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_SETFOCUS message handler
//

void CScriptManager::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
	
	// TODO: Add your message handler code here
	
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CScriptManager::OnViewRefresh() 
{
   DWORD i, dwResult, dwNumScripts;
   TCHAR *pszCurr, *pszNext;
   HTREEITEM hItem, hNextItem;
   NXC_SCRIPT_INFO *pList;

   m_wndTreeCtrl.DeleteAllItems();
   m_hRootItem = m_wndTreeCtrl.InsertItem(_T("[root]"), 0, 0);

   dwResult = DoRequestArg3(NXCGetScriptList, g_hSession, &dwNumScripts,
                            &pList, _T("Loading list of scripts..."));
   if (dwResult == RCC_SUCCESS)
   {
      for(i = 0; i < dwNumScripts; i++)
      {
         for(pszCurr = pList[i].szName, hItem = m_hRootItem; ;
             pszCurr = pszNext + 2, hItem = hNextItem)
         {
            pszNext = _tcsstr(pszCurr, _T("::"));
            if (pszNext == NULL)
               break;
            *pszNext = 0;
            hNextItem = FindTreeCtrlItem(m_wndTreeCtrl, hItem, pszCurr);
            if (hNextItem == NULL)
            {
               hNextItem = m_wndTreeCtrl.InsertItem(pszCurr, 1, 1, hItem);
               m_wndTreeCtrl.SetItemData(hNextItem, 0);
            }
         }
         hItem = m_wndTreeCtrl.InsertItem(pszCurr, 3, 3, hItem);
         m_wndTreeCtrl.SetItemData(hItem, pList[i].dwId);
      }
      safe_free(pList);
   }
   else
   {
      theApp.ErrorBox(dwResult, _T("Cannot load list of scripts: %s"));
   }
}


//
// WM_NOTIFY::TVN_SELCHANGED message handler
//

void CScriptManager::OnTreeViewSelChange(LPNMTREEVIEW lpnmt, LRESULT *pResult)
{
   DWORD dwId;
   CString strName;

   dwId = m_wndTreeCtrl.GetItemData(lpnmt->itemNew.hItem);
   if (dwId != 0)
   {
      BuildScriptName(lpnmt->itemNew.hItem, strName);
      m_wndScriptView.SetEditMode(dwId, (LPCTSTR)strName);
   }
   else
   {
      m_wndScriptView.SetListMode(m_wndTreeCtrl, lpnmt->itemNew.hItem);
   }
   *pResult = 0;
}


//
// WM_NOTIFY::TVN_SELCHANGING message handler
//

void CScriptManager::OnTreeViewSelChanging(LPNMTREEVIEW lpnmt, LRESULT *pResult)
{
   DWORD dwId;

   *pResult = 0;
   if (lpnmt->itemOld.hItem != NULL)
   {
      dwId = m_wndTreeCtrl.GetItemData(lpnmt->itemOld.hItem);
      if (dwId != 0)
      {
         if (!m_wndScriptView.ValidateClose())
            *pResult = 1;
      }
   }
}


//
// Advanced command routing
//

BOOL CScriptManager::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
   if (m_wndScriptView.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
      return TRUE;
	
	return CMDIChildWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}


//
// Build full script name
//

void CScriptManager::BuildScriptName(HTREEITEM hLast, CString &strName)
{
   HTREEITEM hItem;

   strName = m_wndTreeCtrl.GetItemText(hLast);
   hItem = m_wndTreeCtrl.GetParentItem(hLast);
   while(hItem != m_hRootItem)
   {
      strName.Insert(0, _T("::"));
      strName.Insert(0, (LPCTSTR)m_wndTreeCtrl.GetItemText(hItem));
      hItem = m_wndTreeCtrl.GetParentItem(hItem);
   }
}
