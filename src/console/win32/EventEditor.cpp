// EventEditor.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "EventEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEventEditor

IMPLEMENT_DYNCREATE(CEventEditor, CMDIChildWnd)

CEventEditor::CEventEditor()
{
}

CEventEditor::~CEventEditor()
{
}


BEGIN_MESSAGE_MAP(CEventEditor, CMDIChildWnd)
	//{{AFX_MSG_MAP(CEventEditor)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//
// CEventEditor message handlers
//

int CEventEditor::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;
   HREQUEST hRequest;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   // Create list view control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT, rect, this, ID_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT | 
                                  LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, "ID", LVCFMT_LEFT, 40);
   m_wndListCtrl.InsertColumn(1, "Name", LVCFMT_LEFT, 100);
   m_wndListCtrl.InsertColumn(2, "Severity", LVCFMT_LEFT, 70);
   m_wndListCtrl.InsertColumn(3, "Flags", LVCFMT_LEFT, 40);
   m_wndListCtrl.InsertColumn(4, "Message", LVCFMT_LEFT, 150);
   m_wndListCtrl.InsertColumn(5, "Description", LVCFMT_LEFT, 300);
	
   ((CConsoleApp *)AfxGetApp())->OnViewCreate(IDR_EVENT_EDITOR, this);

   hRequest = NXCOpenEventDB();
   if (hRequest != INVALID_REQUEST_HANDLE)
      ((CConsoleApp *)AfxGetApp())->RegisterRequest(hRequest, this);
	return 0;
}


//
// WM_DESTROY message handler
//

void CEventEditor::OnDestroy() 
{
   NXCCloseEventDB();
   ((CConsoleApp *)AfxGetApp())->OnViewDestroy(IDR_EVENT_EDITOR, this);
	CMDIChildWnd::OnDestroy();
}
