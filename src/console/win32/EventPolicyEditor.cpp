// EventPolicyEditor.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "EventPolicyEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEventPolicyEditor

IMPLEMENT_DYNCREATE(CEventPolicyEditor, CMDIChildWnd)

CEventPolicyEditor::CEventPolicyEditor()
{
}

CEventPolicyEditor::~CEventPolicyEditor()
{
}


BEGIN_MESSAGE_MAP(CEventPolicyEditor, CMDIChildWnd)
	//{{AFX_MSG_MAP(CEventPolicyEditor)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_CLOSE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEventPolicyEditor message handlers


//
// Redefined PreCreateWindow()
//

BOOL CEventPolicyEditor::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, NULL, NULL, 
                                         AfxGetApp()->LoadIcon(IDI_RULEMGR));
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CEventPolicyEditor::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   // Create rule list control
   GetClientRect(&rect);
   m_wndRuleList.Create(WS_CHILD | WS_VISIBLE, rect, this, ID_RULE_LIST);

   // Setup columns
   m_wndRuleList.InsertColumn(0, "Rule", 50);
   m_wndRuleList.InsertColumn(1, "Source", 120);
   m_wndRuleList.InsertColumn(2, "Event", 120);
   m_wndRuleList.InsertColumn(3, "Severity", 80);
   m_wndRuleList.InsertColumn(4, "Action", 150);

   // Debug
   m_wndRuleList.InsertRow(0);
   m_wndRuleList.InsertRow(0);
   m_wndRuleList.InsertRow(0);
   m_wndRuleList.InsertRow(0);
   m_wndRuleList.InsertRow(0);
   m_wndRuleList.InsertRow(0);
   m_wndRuleList.InsertRow(0);

   theApp.OnViewCreate(IDR_EPP_EDITOR, this);
	return 0;
}


//
// WM_DESTROY message handler
//

void CEventPolicyEditor::OnDestroy() 
{
   NXCDestroyEventPolicy(theApp.m_pEventPolicy);
   theApp.OnViewDestroy(IDR_EPP_EDITOR, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_CLOSE message handler
//

void CEventPolicyEditor::OnClose() 
{
   DWORD dwResult;

	dwResult = DoRequest(NXCCloseEventPolicy, "Unlocking event processing policy...");
   if (dwResult != RCC_SUCCESS)
      theApp.ErrorBox(dwResult, "Error unlocking event processing policy: %s");
	CMDIChildWnd::OnClose();
}


//
// WM_SIZE message handler
//

void CEventPolicyEditor::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
	
   m_wndRuleList.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}
