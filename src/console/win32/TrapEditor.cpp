// TrapEditor.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "TrapEditor.h"
#include <nxsnmp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTrapEditor

IMPLEMENT_DYNCREATE(CTrapEditor, CMDIChildWnd)

CTrapEditor::CTrapEditor()
{
   m_pTrapList = NULL;
   m_dwNumTraps = 0;
}

CTrapEditor::~CTrapEditor()
{
   if (m_pTrapList != NULL)
      NXCDestroyTrapList(m_dwNumTraps, m_pTrapList);
}


BEGIN_MESSAGE_MAP(CTrapEditor, CMDIChildWnd)
	//{{AFX_MSG_MAP(CTrapEditor)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SETFOCUS()
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTrapEditor message handlers

BOOL CTrapEditor::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         NULL, 
                                         GetSysColorBrush(COLOR_WINDOW), 
                                         AfxGetApp()->LoadIcon(IDI_TRAP));
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CTrapEditor::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   theApp.OnViewCreate(IDR_TRAP_EDITOR, this);
	
   // Create list view control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS, rect, this, IDC_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT |
                                  LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, "ID", LVCFMT_LEFT, 40);
   m_wndListCtrl.InsertColumn(1, "SNMP Trap ID", LVCFMT_LEFT, 200);
   m_wndListCtrl.InsertColumn(2, "Event", LVCFMT_LEFT, 200);
   m_wndListCtrl.InsertColumn(3, "Description", LVCFMT_LEFT, 250);

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);

	return 0;
}


//
// WM_DESTROY message handler
//

void CTrapEditor::OnDestroy() 
{
   theApp.OnViewDestroy(IDR_TRAP_EDITOR, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_SETFOCUS message handler
//

void CTrapEditor::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);

   m_wndListCtrl.SetFocus();
}


//
// WM_SIZE message handler
//

void CTrapEditor::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
	
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_CLOSE message handler
//

void CTrapEditor::OnClose() 
{
   DoRequest(NXCUnlockTrapCfg, "Unlocking SNMP trap configuration database...");
	CMDIChildWnd::OnClose();
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CTrapEditor::OnViewRefresh() 
{
   DWORD i, dwResult;

   if (m_pTrapList != NULL)
      NXCDestroyTrapList(m_dwNumTraps, m_pTrapList);
   m_wndListCtrl.DeleteAllItems();
   dwResult = DoRequestArg2(NXCLoadTrapCfg, &m_dwNumTraps, &m_pTrapList, _T("Loading SNMP trap configuration..."));
   if (dwResult == RCC_SUCCESS)
   {
      for(i = 0; i < m_dwNumTraps; i++)
         AddItem(i);
   }
   else
   {
      theApp.ErrorBox(dwResult, _T("Error loading SNMP trap configuration: %s"));
   }
}


//
// Add new item to list
//

void CTrapEditor::AddItem(DWORD dwIndex)
{
   TCHAR szBuffer[32];
   int iItem;

   _stprintf(szBuffer, _T("%d"), m_pTrapList[dwIndex].dwId);
   iItem = m_wndListCtrl.InsertItem(dwIndex, szBuffer);
   if (iItem != -1)
   {
      m_wndListCtrl.SetItemData(iItem, m_pTrapList[dwIndex].dwId);
      UpdateItem(iItem, dwIndex);
   }
}


//
// Update item in list
//

void CTrapEditor::UpdateItem(int iItem, DWORD dwIndex)
{
   TCHAR szBuffer[1024];

   SNMPConvertOIDToText(m_pTrapList[dwIndex].dwOidLen, m_pTrapList[dwIndex].pdwObjectId,
                        szBuffer, 1024);
   m_wndListCtrl.SetItemText(iItem, 1, szBuffer);
   m_wndListCtrl.SetItemText(iItem, 2, NXCGetEventName(m_pTrapList[dwIndex].dwEventId));
   m_wndListCtrl.SetItemText(iItem, 3, m_pTrapList[dwIndex].szDescription);
}
