// ScriptManager.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ScriptManager.h"
#include "InputBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MODE_LIST    0
#define MODE_TREE    1


/////////////////////////////////////////////////////////////////////////////
// CScriptManager

IMPLEMENT_DYNCREATE(CScriptManager, CMDIChildWnd)

CScriptManager::CScriptManager()
{
   m_nMode = -1;
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
	ON_COMMAND(ID_SCRIPT_VIEWASLIST, OnScriptViewaslist)
	ON_UPDATE_COMMAND_UI(ID_SCRIPT_VIEWASLIST, OnUpdateScriptViewaslist)
	ON_COMMAND(ID_SCRIPT_VIEWASTREE, OnScriptViewastree)
	ON_UPDATE_COMMAND_UI(ID_SCRIPT_VIEWASTREE, OnUpdateScriptViewastree)
	ON_COMMAND(ID_SCRIPT_NEW, OnScriptNew)
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
   m_wndTreeCtrl.Create(WS_CHILD | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
                        rect, &m_wndSplitter, m_wndSplitter.IdFromRowCol(0, 0));
   m_wndTreeCtrl.SetImageList(&m_imageList, TVSIL_NORMAL);

   // Create list control
   m_wndListCtrl.Create(WS_CHILD | LVS_REPORT | LVS_NOCOLUMNHEADER | LVS_SHAREIMAGELISTS | LVS_SHOWSELALWAYS | LVS_SINGLESEL,
                        rect, &m_wndSplitter, m_wndSplitter.IdFromRowCol(0, 0));
   m_wndListCtrl.InsertColumn(0, _T("Script"));
   m_wndListCtrl.SetImageList(&m_imageList, LVSIL_SMALL);

   // Create script view
   m_wndScriptView.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, rect,
                          &m_wndSplitter, m_wndSplitter.IdFromRowCol(0, 1));
	
   // Finish splitter setup
   m_wndSplitter.SetColumnInfo(0, 150, 150);
   m_wndSplitter.InitComplete();
   m_wndSplitter.RecalcLayout();

   SetMode(MODE_TREE);

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
   NXC_SCRIPT_INFO *pList;

   m_wndListCtrl.DeleteAllItems();
   m_wndTreeCtrl.DeleteAllItems();
   m_hRootItem = m_wndTreeCtrl.InsertItem(_T("[root]"), 0, 0);

   dwResult = DoRequestArg3(NXCGetScriptList, g_hSession, &dwNumScripts,
                            &pList, _T("Loading list of scripts..."));
   if (dwResult == RCC_SUCCESS)
   {
      for(i = 0; i < dwNumScripts; i++)
      {
         InsertScript(pList[i].dwId, pList[i].szName);
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


//
// Set list or tree mode
//

void CScriptManager::SetMode(int nMode)
{
   if (m_nMode != nMode)
   {
      m_nMode = nMode;
      if (m_nMode == MODE_LIST)
      {
         m_wndListCtrl.ShowWindow(SW_SHOW);
         m_wndTreeCtrl.ShowWindow(SW_HIDE);
 
         m_wndTreeCtrl.SetDlgCtrlID(-1);
         m_wndListCtrl.SetDlgCtrlID(m_wndSplitter.IdFromRowCol(0, 0));
      }
      else
      {
         m_wndTreeCtrl.ShowWindow(SW_SHOW);
         m_wndListCtrl.ShowWindow(SW_HIDE);
 
         m_wndListCtrl.SetDlgCtrlID(-1);
         m_wndTreeCtrl.SetDlgCtrlID(m_wndSplitter.IdFromRowCol(0, 0));
      }
      m_wndSplitter.RecalcLayout();
   }
}


//
// Change script library view mode
//

void CScriptManager::OnScriptViewaslist() 
{
   SetMode(MODE_LIST);
}

void CScriptManager::OnUpdateScriptViewaslist(CCmdUI* pCmdUI) 
{
   pCmdUI->SetRadio(m_nMode == MODE_LIST);
}

void CScriptManager::OnScriptViewastree() 
{
   SetMode(MODE_TREE);
}

void CScriptManager::OnUpdateScriptViewastree(CCmdUI* pCmdUI) 
{
   pCmdUI->SetRadio(m_nMode == MODE_TREE);
}


//
// Create new script
//

void CScriptManager::OnScriptNew() 
{
   CInputBox dlg;
   DWORD dwResult;

   if (m_wndScriptView.ValidateClose())
   {
      dlg.m_strTitle = _T("Create New Script");
      dlg.m_strHeader = _T("Scrip name");
      if (dlg.DoModal() == IDOK)
      {
         dwResult = DoRequestArg4(NXCUpdateScript, g_hSession,
                                  (void *)0, (void *)((LPCTSTR)dlg.m_strText),
                                  _T(""), _T("Updating script..."));
         if (dwResult == RCC_SUCCESS)
         {
            InsertScript(0, (LPCTSTR)dlg.m_strText);
         }
      }
   }
}


//
// Insert script into list/tree
//

void CScriptManager::InsertScript(DWORD dwId, TCHAR *pszName)
{
   TCHAR *pszCurr, *pszNext;
   HTREEITEM hItem, hNextItem;
   int iItem;

   iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, pszName, 3);
   m_wndListCtrl.SetItemData(iItem, dwId);

   for(pszCurr = pszName, hItem = m_hRootItem; ;
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
   m_wndTreeCtrl.SetItemData(hItem, dwId);
}
