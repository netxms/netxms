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
   m_wndRuleList.InsertColumn(0, "No.", 35, CF_CENTER | CF_TITLE_COLOR);
   m_wndRuleList.InsertColumn(1, "Source", 120);
   m_wndRuleList.InsertColumn(2, "Event", 120);
   m_wndRuleList.InsertColumn(3, "Severity", 80);
   m_wndRuleList.InsertColumn(4, "Action", 150);
   m_wndRuleList.InsertColumn(5, "Comments", 200);

   // Debug
   char szBuffer[256];
   for(int i = 0; i < 7; i++)
   {
      m_wndRuleList.InsertRow(i);
      sprintf(szBuffer, "%d", i);
      m_wndRuleList.AddItem(i, 0, szBuffer);
      if (i == 2)
      {
         m_wndRuleList.AddItem(i, 1, "10.0.0.2");
         m_wndRuleList.AddItem(i, 1, "10.0.0.10");
      }
      if (i == 3)
      {
         m_wndRuleList.AddItem(i, 1, "10.0.0.0/16");
         m_wndRuleList.AddItem(i, 2, "NX_NODE_NORMAL");
         m_wndRuleList.AddItem(i, 2, "NX_NODE_WARNING");
         m_wndRuleList.AddItem(i, 2, "NX_NODE_MINOR");
         m_wndRuleList.AddItem(i, 3, "hIlQgq|");
         m_wndRuleList.AddItem(i, 3, "hIlQhqg");
      }
   }
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
