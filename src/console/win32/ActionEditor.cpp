// ActionEditor.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ActionEditor.h"

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
   m_wndListCtrl.DeleteAllItems();
}
