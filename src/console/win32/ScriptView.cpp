// ScriptView.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ScriptView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Operation modes
//

#define MODE_LIST    0
#define MODE_VIEW    1
#define MODE_EDIT    2


/////////////////////////////////////////////////////////////////////////////
// CScriptView

CScriptView::CScriptView()
{
}

CScriptView::~CScriptView()
{
}


BEGIN_MESSAGE_MAP(CScriptView, CWnd)
	//{{AFX_MSG_MAP(CScriptView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CScriptView message handlers


//
// WM_CREATE message handler
//

int CScriptView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   GetClientRect(&rect);

   // Create image list
   m_imageList.Create(32, 32, ILC_COLOR8 | ILC_MASK, 4, 4);
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_CLOSED_FOLDER));
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_SCRIPT));

   // Create list control
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE, rect, this, ID_LIST_VIEW);
   m_wndListCtrl.SetImageList(&m_imageList, LVSIL_NORMAL);
   m_wndListCtrl.InsertColumn(0, _T("Name"));

   // Create editor
   m_wndEditor.Create(WS_CHILD | WS_VISIBLE | ES_MULTILINE, rect, this, ID_EDIT_BOX);
   m_wndEditor.SetFont(&g_fontCode);
	
	return 0;
}


//
// WM_SIZE message handler
//

void CScriptView::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
   m_wndEditor.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// Switch script view to list mode
//

void CScriptView::SetListMode(CTreeCtrl &wndTreeCtrl, HTREEITEM hRoot)
{
   HTREEITEM hItem;
   DWORD dwId;

   m_nMode = MODE_LIST;

   m_wndListCtrl.DeleteAllItems();
   hItem = wndTreeCtrl.GetChildItem(hRoot);
   while(hItem != NULL)
   {
      dwId = wndTreeCtrl.GetItemData(hItem);
      m_wndListCtrl.InsertItem(0x7FFFFFFF, (LPCTSTR)wndTreeCtrl.GetItemText(hItem),
                               (dwId == 0) ? 0 : 1);
      hItem = wndTreeCtrl.GetNextItem(hItem, TVGN_NEXT);
   }

   m_wndListCtrl.ShowWindow(SW_SHOW);
   m_wndEditor.ShowWindow(SW_HIDE);
}


//
// Switch script view to edit mode
//

void CScriptView::SetEditMode(DWORD dwScriptId)
{
   m_wndEditor.ShowWindow(SW_HIDE);
   m_wndListCtrl.ShowWindow(SW_HIDE);
}
