// UserEditor.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "UserEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUserEditor

IMPLEMENT_DYNCREATE(CUserEditor, CMDIChildWnd)

CUserEditor::CUserEditor()
{
}

CUserEditor::~CUserEditor()
{
}


BEGIN_MESSAGE_MAP(CUserEditor, CMDIChildWnd)
	//{{AFX_MSG_MAP(CUserEditor)
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUserEditor message handlers


//
// WM_CLOSE message handler
//

void CUserEditor::OnClose() 
{
   theApp.WaitForRequest(NXCUnlockUserDB());
	CMDIChildWnd::OnClose();
}


//
// WM_DESTROY message handler
//

void CUserEditor::OnDestroy() 
{
   theApp.OnViewDestroy(IDR_USER_EDITOR, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_CREATE message handler
//

int CUserEditor::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   theApp.OnViewCreate(IDR_USER_EDITOR, this);
	
   // Create list view control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS, rect, this, ID_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT | 
                                  LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, "ID", LVCFMT_LEFT, 40);
   m_wndListCtrl.InsertColumn(1, "Name", LVCFMT_LEFT, 150);

   return 0;
}
