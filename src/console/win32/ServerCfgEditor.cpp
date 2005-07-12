// ServerCfgEditor.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ServerCfgEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CServerCfgEditor

IMPLEMENT_DYNCREATE(CServerCfgEditor, CMDIChildWnd)

CServerCfgEditor::CServerCfgEditor()
{
}

CServerCfgEditor::~CServerCfgEditor()
{
}


BEGIN_MESSAGE_MAP(CServerCfgEditor, CMDIChildWnd)
	//{{AFX_MSG_MAP(CServerCfgEditor)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServerCfgEditor message handlers

BOOL CServerCfgEditor::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         NULL, 
                                         GetSysColorBrush(COLOR_WINDOW), 
                                         AfxGetApp()->LoadIcon(IDI_SETUP));
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CServerCfgEditor::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   theApp.OnViewCreate(IDR_SERVER_CFG_EDITOR, this);
	
   // Create list view control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER,
                        rect, this, IDC_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT |
                                  LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 150);
   m_wndListCtrl.InsertColumn(1, _T("Value"), LVCFMT_LEFT, 250);
   m_wndListCtrl.InsertColumn(2, _T("Restart"), LVCFMT_LEFT, 50);

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);

	return 0;
}


//
// WM_DESTROY message handler
//

void CServerCfgEditor::OnDestroy() 
{
   theApp.OnViewDestroy(IDR_SERVER_CFG_EDITOR, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_SIZE message handler
//

void CServerCfgEditor::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_SETFOCUS message handler
//

void CServerCfgEditor::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
   m_wndListCtrl.SetFocus();	
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CServerCfgEditor::OnViewRefresh() 
{
   DWORD i, dwResult, dwNumVars;
   NXC_SERVER_VARIABLE *pVarList;

   m_wndListCtrl.DeleteAllItems();
   dwResult = DoRequestArg3(NXCGetServerVariables, g_hSession, &pVarList,
                            &dwNumVars, _T("Loading server's configuration variables..."));
   if (dwResult == RCC_SUCCESS)
   {
      for(i = 0; i < dwNumVars; i++)
         AddItem(&pVarList[i]);
      safe_free(pVarList);
   }
   else
   {
      theApp.ErrorBox(dwResult, _T("Error loading server variables: %s"));
   }
}


//
// Add new item to list
//

void CServerCfgEditor::AddItem(NXC_SERVER_VARIABLE *pVar)
{
   int iItem;

   iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, pVar->szName);
   if (iItem != -1)
   {
      m_wndListCtrl.SetItemText(iItem, 1, pVar->szValue);
      m_wndListCtrl.SetItemText(iItem, 2, pVar->bNeedRestart ? _T("Yes") : _T("No"));
   }
}
